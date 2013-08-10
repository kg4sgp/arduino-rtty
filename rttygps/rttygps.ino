// Arduino RTTY Modulator
// Uses Fast PWM to produce ~8kHz 8bit audio

#include "baudot.h"
#include "pwmsine.h"

// Yeah, I really need to get rid of these globals.
// Thats next on the to-do list

unsigned int sampleRate = 31250;
unsigned int tableSize = sizeof(sine)/sizeof(char);
unsigned int pstn = 0;
int sign = -1;
unsigned int change = 0;
unsigned int count = 0;

int fmark = 870;
int fspac = 700;
int baud = 45;
int bits = 8;
char lsbf = 0;

String msg;

unsigned char bitPstn = 0;
int bytePstn = 0;
unsigned char tx = 0;

unsigned char charbuf = 0;
unsigned char shiftToNum = 0;
unsigned char justshifted = 0;

// compute values for tones.
unsigned int dmark = (unsigned int)((2*(long)tableSize*(long)fmark)/((long)sampleRate));
unsigned int dspac = (unsigned int)((2*(long)tableSize*(long)fspac)/((long)sampleRate));
int msgSize;
unsigned int sampPerSymb = (unsigned int)(sampleRate/baud);

String delim = ":";
String nl = "\n";
String call = "W8UPD-1";
String nulls = "RRRRR";

void setup() {

  // stop interrupts for setup
  cli();

  // setup pins for output
  pinMode(3, OUTPUT);
  pinMode(13, OUTPUT);

  // setup counter 2 for fast PWM output on pin 3 (arduino)
  TCCR2A = _BV(COM2B1) | _BV(WGM21) | _BV(WGM20);
  TCCR2B = _BV(CS20);
  TIMSK2 = _BV(TOIE2);

  // begin serial communication
  Serial.begin(9600);
  Serial.println("Started");

  // re-enable interrupts
  sei();
}

int getgps(){

  TIMSK2 |= (0 << TOIE2);
  
  // GPS Data
  String time = "";
  String longitude = "";
  String NS = "";
  String latitude = "";
  String EW = "";
  String altitude = "";
  String aUnits = "";

  // intialize Finite State Machine
  int go = 0;
  char buffer[16] = { 0 }; 
  int state = 0;
  int commaCount = 0;

  while(go != 1){
    
    if (Serial.available()) {

      buffer[15] = Serial.read();
      //Serial.print(buffer[15]);

      if(buffer[15] == ',') commaCount++;

      // reset FSM on new data frame
      if(buffer[15] == '$') {
        commaCount = 0;
        state = 0;
      }

      // Finite State Machine
      if (state == 0) {
        if (buffer[10] == '$'){         
          if (buffer[11] == 'G' && buffer [12] == 'P' &&
               buffer[13] == 'G' && buffer[14] == 'G' && buffer [15] == 'A'){
            state = 1;
          }
        }

      } else if (state == 1){

        // Grab time info
        // If this is the second comma in the last element of the buffer array
        if (buffer[15] == ',' && commaCount == 2){
          for(int i = 14; i >= 0; i--){
            // copy the data, counting down until a comma is reached
            if (buffer[i] == ',') break;
            time = String(buffer[i]) + time;
          }          
          //next state
          state = 2;
        }

      } else if (state == 2) { 

        // Grab latitude
        if (buffer[15] == ',' && commaCount == 3){
          for(int i = 14; i >= 0; i--){
            if (buffer[i] == ',') break;
            latitude = String(buffer[i]) + latitude;
          }
          state = 3;
        }

      } else if (state == 3) {

        // Grab N or S reading
        if (buffer[15] == ',' && commaCount == 4){
          NS = String(buffer[14]);
          state = 4;
        }

      } else if (state == 4) {

        // Grab longitude
        if (buffer[15] == ',' && commaCount == 5){
          for(int i = 14; i >= 0; i--){
            if (buffer[i] == ',') break;
            longitude = String(buffer[i]) + longitude;
          }

          state = 5;
        }

      } else if (state == 5) {

        // Grab E or W reading
        if (buffer[15] == ',' && commaCount == 6){
          EW = String(buffer[14]) + EW;
          state = 6;
        }

      } else if (state == 6) {

        // Grab altitude
        if (buffer[15] == ',' && commaCount == 10){
          for(int i = 14; i >= 0; i--){
            if (buffer[i] == ',') break;
            altitude = String(buffer[i]) + altitude;
          }
          state = 7;
        }

      } else if (state == 7) {

        // Grab altitude units
        if (buffer[15] == ',' && commaCount == 11){
          aUnits = String(buffer[14]) + aUnits;
          state = 8;
        }

      } else if (state == 8) {
        msg = nl;
	msg += nl;
        msg += nulls;
	msg += delim;
        msg += call;
        msg += delim;
        msg += latitude;
        msg += delim;
        msg += longitude;
        msg += delim;
        msg += altitude;
        msg += delim;
        msg += time;
        msg += nl;
        
        return 1;
      }
      
      for(int i = 0; i <16; i++){
        buffer[i] = buffer[i+1];      
      }
      
    }       
  }
  TIMSK2 |= (1 << TOIE2);
}

char calcAmp(){
  pstn += change;

  // if the position rolls off, change sign and start where you left off
  if(pstn >= tableSize) {
    pstn = pstn%tableSize;
    sign *= -1;
  }

  // return the pwm value
  return (char)(128+(sign*sine[pstn]));
}

// sets the character buffer, the current character being sent
void setCbuff(){
    int i = 0;

    // Note: the <<2)+3 is how we put the start and stop bits in
    // the baudot table is MSB on right in the form 000xxxxx
    // so when we shift two and add three it becomes 0xxxxx11 which is
    // exactly the form we need for one start bit and two stop bits when read
    // from left to right

    // try to find a match of the current character in baudot
    for(i = 0; i < (sizeof(baudot_letters)/sizeof(char)); i++){

      // look in letters
      if(msg.charAt(bytePstn) == baudot_letters[i]) {

        // if coming from numbers, send shift to letters
        if(shiftToNum == 1){
          shiftToNum = 0;
          //bytePstn++;
          charbuf = ((baudot[31])<<2)+3;
          justshifted = 1;
        } else {
          charbuf = ((baudot[i])<<2)+3;
        }
      }

      //look in numbers
      if(msg.charAt(bytePstn) != ' ' && msg.charAt(bytePstn) != 10
		&& msg.charAt(bytePstn) == baudot_figures[i]) {
        if(shiftToNum == 0){
          shiftToNum = 1;
          //bytePstn++;
          charbuf = ((baudot[30])<<2)+3;
          justshifted = 1;
        } else {
          charbuf = ((baudot[i])<<2)+3;
        }
      }      
      

    }
    
  // dont increment bytePstn if were transmitting a shift-to character
  if(justshifted != 1) {
    //print letter you're transmitting
    Serial.print(msg.charAt(bytePstn));
    bytePstn++;
  } else {
    justshifted = 0;
  }
}

void setSymb(char mve){

    // if its a 1 set change to dmark other wise set change to dspace
    if((charbuf&(0x01<<mve))>>mve) {
      change = dmark;
    } else {
      change = dspac;
    }
}

// int main
void loop() {
  if(tx == 0){
    getgps();
    tx = 1;
  }
}

// This interrupt run on every sample (7128.5 times a second)
// though it should be every 7128.5 the sample rate had to be set differently
// to produce the correct baud rate and frequencies (that agree with the math)
// Why?! I'm going to figure that one out
ISR(TIMER2_OVF_vect) {
  if (tx == 0) return;
  count++;

  // if we've played enough samples for the symbol were transmitting, get next symbol
  if (count >= sampPerSymb){
    count = 0;
    bitPstn++;

    // if were at the end of the character return to the begining of a
    // character and grab the next one
    if(bitPstn > (bits-1)) {
      bitPstn = 0;
      
      setCbuff();
      // if were at the end of the message, return to the begining
      if (bytePstn > msg.length()-1){

        tx = 0;

        // clear variables used here
        bitPstn = 0;
        bytePstn = 0;
        count = 0;
        return;
      }
    }

    unsigned char mve = 0;
    // endianness thing
    if (lsbf != 1) {
      // MSB first
      mve = (bits-1)-bitPstn;
    } else {
      // LSB first
      mve = bitPstn;
    }

    // get if 0 or 1 that were transmitting
    setSymb(mve);
  }

  // set PWM duty cycle
  OCR2B = calcAmp();
}
