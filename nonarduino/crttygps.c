// Arduino GPS Parser and RTTY Modulator

#define USART_BAUDRATE 9600
#define BAUD_PRESCALE (((F_CPU / (USART_BAUDRATE *16UL)))-1)

#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>
#include "ita2.h"
#include "pwmsine.h"

unsigned int sampleRate = 7750;
unsigned int tableSize = (unsigned int)(sizeof(sine)/sizeof(char));
unsigned int pstn = 0;
signed char sign = (signed char)-1;
unsigned int change = 0;
unsigned int count = 1;

int fmark = 870;
int fspac = 700;
int baud = 45;
int bits = 8;
char lsbf = (char)0;

const unsigned char maxmsg = (unsigned char)128;
char msg[maxmsg] = "";

unsigned char bitPstn = (unsigned char)0;
unsigned int bytePstn = (unsigned char)0;
unsigned char tx = (unsigned char)0;

unsigned char charbuf = (unsigned char)0;
unsigned char shiftToNum = (unsigned char)0;
unsigned char justshifted = (unsigned char)0;

// compute values for tones.
unsigned int dmark = (unsigned int)((2*(long)tableSize*(long)fmark)/((long)sampleRate));
unsigned int dspac = (unsigned int)((2*(long)tableSize*(long)fspac)/((long)sampleRate));
unsigned char msgSize;
unsigned int sampPerSymb = (unsigned int)(sampleRate/baud);

char delim[] = ":";
char nl[] = "\n";
char call[] = "W8UPD";
char nulls[] = "RRRRR";

char lat_deg = (char)0;
char lon_deg = (char)0;
float lat_f = 0;
float lon_f = 0;

// GPS Data
const char buflen = (char)16;
char utc_time[buflen] = "";
char longitude[buflen] = "";
char NS[1] = "";
char latitude[buflen] = "";
char EW[1] = "";
char altitude[buflen] = "";
char alt_units[1] = "";
char strbuf[2] = "";

// intialize Finite fsm_state Machine
char buffer[buflen] = { (char)0 }; 
char fsm_state = (char)0;
char commas = (char)0;
char lastcomma = (char)0;


char calcAmp(){
  pstn += change;

  // if the position rolls off, change sign and start where you left off
  if(pstn >= tableSize) {
    pstn = pstn >> 10; // (mod 1024)
    sign = (signed char)-sign;
  }

  // return the pwm value
  return (char)(128+(int)(sign*sine[pstn]));
}




// sets the character buffer, the current character being sent
void setCbuff(){
    unsigned int i = 0;

    // Note: the <<2)+3 is how we put the start and stop bits in
    // the ita2 table is MSB on right in the form 000xxxxx
    // so when we shift two and add three it becomes 0xxxxx11 which is
    // exactly the form we need for one start bit and two stop bits when read
    // from left to right

    // try to find a match of the current character in ita2
    for(i = 0; i < (unsigned int)(sizeof(ita2_letters)/sizeof(char)); i++){

      // look in letters
      if((unsigned char)msg[bytePstn] == ita2_letters[i]) {

        // if coming from numbers, send shift to letters
        if(shiftToNum == (unsigned char)1){
          shiftToNum = (unsigned char)0;
          //bytePstn++;
          charbuf = (unsigned char)(((ita2[31])<<2)+(unsigned char)3);
          justshifted = (unsigned char)1;
        } else {
          charbuf = (unsigned char)(((ita2[i])<<2)+(unsigned char)3);
        }
      }

      //look in numbers
      if(msg[bytePstn] != ' ' && msg[bytePstn] != (char)10
		&& (unsigned char)msg[bytePstn] == ita2_figures[i]) {
        if(shiftToNum == (unsigned char)0){
          shiftToNum = (unsigned char)1;
          //bytePstn++;
          charbuf = (unsigned char)(((ita2[30])<<2)+(unsigned char)3);
          justshifted = (unsigned char)1;
        } else {
          charbuf = (unsigned char)(((ita2[i])<<2)+(unsigned char)3);
        }
      }      
      

    }
    
  // dont increment bytePstn if were transmitting a shift-to character
  if(justshifted != (unsigned char)1) {
    //print letter you're transmitting
    //Serial.write(msg[bytePstn]);
    UDR0 = msg[bytePstn];
    bytePstn++;
  } else {
    justshifted = (unsigned char)0;
  }
}




void setSymb(unsigned char hmve){

    // if its a 1 set change to dmark otherwise set change to dspace
    if( (bool) ( ( (unsigned char)charbuf & ( 0x01<<hmve ) ) >> hmve ) ) {
      change = dmark;
    } else {
      change = dspac;
    }
}



int main(void){

  // set things up
  //cli();

  // setup pins for output
  DDRD |= _BV(3);

  // setup counter 2 for fast PWM output on pin 3 (arduino)
  TCCR2A = _BV(COM2B1) | _BV(WGM21) | _BV(WGM20);
  // TCCR2B = _BV(CS20); // TinyDuino at 8MHz
  TCCR2B = _BV(CS21); // Arduino at 16MHz
  TIMSK2 = _BV(TOIE2);
  
  // begin serial communication
  UCSR0B = (1 << RXEN0) | (1 << TXEN0);
  UCSR0C = (1 << UCSZ00) | (1 << UCSZ01);
  UBRR0H = (BAUD_PRESCALE >> 8);
  UBRR0L = BAUD_PRESCALE;
  UCSR0B |= (1 << RXCIE0);
  UDR0 = 'S';

  //Serial.begin(9600);
  //Serial.print('S');

  // re-enable interrupts
  sei();

  /* Does NOT Terminate! */
  for(;;) {
    /* Idle here while interrupts take care of things */
  }

  return 0;
}





// This interrupt run on every sample (7128.5 times a second)
// though it should be every 7128.5 the sample rate had to be set differently
// to produce the correct baud rate and frequencies (that agree with the math)
// Tune your device accordingly!

ISR(TIMER2_OVF_vect) {
  if (tx == (unsigned char)0) return;
  count--;

  // if we've played enough samples for the symbol were transmitting, get next symbol
  if (count == (unsigned int)0){
    bitPstn = (unsigned char)(bitPstn+(unsigned char)1);
    count = sampPerSymb;
    
    // if were transmitting a stop bit make it 1.5x as long
    if (bitPstn == (unsigned char)(bits-1)) count += count >> 1; // div by 2

    // if were at the end of the character return to the begining of a
    // character and grab the next one
    if(bitPstn > (unsigned char)(bits-1)) {
      bitPstn = (unsigned char)0;
      
      setCbuff();
      // if were at the end of the message, return to the begining
      if (bytePstn >= (unsigned int)msgSize){
        // clear variables used here
        msgSize = (unsigned char)0;
        bitPstn = (unsigned char)0;
        bytePstn = (unsigned char)0;
        count = (unsigned int)1;
        tx = (unsigned char)0;
        return;
      }
    }

    unsigned char hmve = (unsigned char)0;
    // endianness thing
    if (lsbf != 1) {
      // MSB first
      hmve = (char)((bits-1)-bitPstn);
    } else {
      // LSB first
      hmve = bitPstn;
    }

    // get if 0 or 1 that were transmitting
    setSymb(hmve);
  }

  // set PWM duty cycle
  OCR2B = calcAmp();
}

ISR(USART_RX_vect) {
  if (tx == (char)1) return;   
 
  // move buffer down, make way for new char
  for(int i = 0; i < buflen-1; i++){
    buffer[i] = buffer[i+1];      
  }

  buffer[buflen-1] = UDR0;

  if(buffer[buflen-1] == ',') commas++;

  // reset FSM on new data frame
  if(buffer[buflen-1] == '$') {
    commas = 0;
    fsm_state = 0;
  }

  // Finite fsm_state Machine
  //UDR0 = fsm_state+48;
  if (fsm_state == (char)0) {

    if (buffer[buflen-6] == '$' &&
        buffer[buflen-5] == 'G' &&
        buffer[buflen-4] == 'P' &&
        buffer[buflen-3] == 'G' &&
        buffer[buflen-2] == 'G' &&
        buffer[buflen-1] == 'A'){
      fsm_state = 1;
    }

  } else if (fsm_state == (char)1) {

    // Grab time info
    // If this is the second comma in the last element of the buffer array
    if (buffer[buflen-1] == ',' && commas == 2){

      utc_time[0] = '\0';
      for (unsigned char i = lastcomma; i < buflen-1; i++) {

        strbuf[0] = buffer[i];
        strncat(utc_time, strbuf, (buflen-1)-i);
      }

      //next fsm_state
      fsm_state = 2;
    }

  } else if (fsm_state == (char)2) { 

    // Grab latitude

    if (buffer[buflen-1] == ',' && commas == 3){
      latitude[0] = '\0';
      for (unsigned char i = lastcomma; i < buflen-1; i++) {
        strbuf[0] = buffer[i];
        strncat(latitude, strbuf, (buflen-1)-i);
      }

      fsm_state = 3;
    }

  } else if (fsm_state == (char)3) {

    // Grab N or S reading
    if (buffer[buflen-1] == ',' && commas == 4){
      strncpy(NS, &buffer[buflen-2], 1);
      fsm_state = 4;
    }

  } else if (fsm_state == (char)4) {

    // Grab longitude
    if (buffer[buflen-1] == ',' && commas == 5){

      longitude[0] = '\0';
      for (unsigned char i = lastcomma; i < buflen-1; i++) {
        strbuf[0] = buffer[i];
        strncat(longitude, strbuf, (buflen-1)-i);
      }

      fsm_state = 5;
    }

  } else if (fsm_state == (char)5) {

    // Grab E or W reading
    if (buffer[buflen-1] == ',' && commas == 6){
      strncpy(EW, &buffer[buflen-2], 1);
      fsm_state = 6;
    }

  } else if (fsm_state == (char)6) {

    // Grab altitude
    if (buffer[buflen-1] == ',' && commas == 10){

      altitude[0] = '\0';
      for (unsigned char i = lastcomma; i < buflen-1; i++) {
        strbuf[0] = buffer[i];
        strncat(altitude, strbuf, (buflen-1)-i);
      }

      fsm_state = 7;
    }

  } else if (fsm_state == (char)7) {

    // Grab altitude units
    if (buffer[buflen-1] == ',' && commas == 11){
      strncpy(alt_units, &buffer[buflen-2], 1);
      fsm_state = 8;
    }

  } else if (fsm_state == (char)8) {
    
    // clear lat and lon
    lat_deg = 0;
    lon_deg = 0;
    lat_f = 0;
    lon_f = 0;

    // convert lat and lon from deg decimal-minutes, to decimal degrees
    lat_deg = (char)( ((latitude[0]-48)*10) + (latitude[1]-48));
    lat_f = lat_deg + ((float)atof(&latitude[2]))/60;
    lon_deg = (char)( ((longitude[0]-48)*100)+((longitude[1]-48)*10)+(longitude[2]-48) );
    lon_f = lon_deg + ((float)atof(&longitude[3]))/60;

    // make negative if needed
    if (NS[0] == 'S') lat_f = -lat_f;     
    if (EW[0] == 'W') lon_f = -lon_f;   
    
    // convert back to strings
    dtostrf(lat_f, 8, 5, latitude);
    dtostrf(lon_f, 8, 5, longitude);        
    
    // asseble the message
    strncpy(msg, nulls, maxmsg);
    strncat(msg, nl, maxmsg);
    strncat(msg, delim, maxmsg);
    strncat(msg, call, maxmsg);
    strncat(msg, delim, maxmsg);
    strncat(msg, latitude, maxmsg);
    strncat(msg, delim, maxmsg);
    strncat(msg, longitude, maxmsg);
    strncat(msg, delim, maxmsg);
    strncat(msg, altitude, maxmsg);
    strncat(msg, delim, maxmsg);
    strncat(msg, utc_time, 6);
    strncat(msg, nl, maxmsg);
    strncat(msg, nl, maxmsg);
    msgSize = (char)strlen(msg);

    // reset finite state machine and transmit...
    commas = 0;
    fsm_state = 0;
    tx = 1;
  }
  
  if(buffer[buflen-1] == ',') {
    lastcomma = buflen-1;
  } else {
    lastcomma--;
  }

}
