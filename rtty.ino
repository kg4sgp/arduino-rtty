// To be used on an arduino
// Uses Fast PWM to produce 8kHz 8bit audio
// Maybe in time go to 16 bit using Timer 2 but not yet...

#include "baudot.h"
#include "pwmsine.h"

char pflag = 0;
unsigned int sampleRate = 7750;
unsigned int tableSize = sizeof(sine)/sizeof(char);
unsigned int pstn = 0;
int sign = -1;
unsigned int change = 0;
unsigned int count = 0;
int sym = 0x00;

int fmark = 870;
int fspac = 700;
int baud = 45;
int bits = 8;
char lsbf = 0;

unsigned char bitPstn = 0;
int bytePstn = 0;
unsigned char tx = 1;

unsigned char charbuf = 0;
unsigned char shiftToNum = 0;
unsigned char justshifted = 0;

char msg[] = "\n\nCQ CQ CQ DE KG4SGP KG4SGP KG4SGP KN\n\n";

// compute values for tones.
unsigned int dmark = (unsigned int)((2*(long)tableSize*(long)fmark)/((long)sampleRate));
unsigned int dspac = (unsigned int)((2*(long)tableSize*(long)fspac)/((long)sampleRate));
int msgSize = sizeof(msg)/sizeof(char);
unsigned int sampPerSymb = (unsigned int)(sampleRate/baud);

void setup() {

  // stop interrupts for setup
  cli();

  // setup pins for output
  pinMode(3, OUTPUT);
  pinMode(13, OUTPUT);

  // setup counter 2 for fast PWM output on pin 3 (arduino)
  TCCR2A = _BV(COM2B1) | _BV(WGM21) | _BV(WGM20);
  TCCR2B = _BV(CS21);
  TIMSK2 = _BV(TOIE2);

  // begin serial communication
  Serial.begin(9600);
  Serial.println("Started");

  // re-enable interrupts
  sei();
}

// int main
void loop() {
  if(pflag == 1){
    pflag = 0;
    Serial.println(sym);
  }
}

char calcAmp(){
  pstn += change;

  // if the position rolls off, change sign and start where you left off
  if(pstn >= tableSize) {
    pstn = pstn%tableSize;
    sign *= -1;
  }

  return (char)(128+(sign*sine[pstn]));
}

void setCbuff(){
    int i = 0;
    for(i = 0; i < (sizeof(baudot_letters)/sizeof(char)); i++){
      
      if(msg[bytePstn] == baudot_letters[i]) {
        if(shiftToNum == 1){
          shiftToNum = 0;
          charbuf = ((baudot[31])<<2)+3;
          justshifted = 1;
        } else {
          charbuf = ((baudot[i])<<2)+3;
        }
      }
      
      if(msg[bytePstn] != ' ' && msg[bytePstn] != 10
		&& msg[bytePstn] == baudot_figures[i]) {
        if(shiftToNum == 0){
          shiftToNum = 1;
          charbuf = ((baudot[30])<<2)+3;
          justshifted = 1;
        } else {
          charbuf = ((baudot[i])<<2)+3;
        }
      }      
      
      sym = charbuf;
      pflag = 1;
    }
}

void setSymb(char mve){

    if((charbuf&(0x01<<mve))>>mve) {
      change = dmark;
    } else {
      change = dspac;
    }
}

// This interrupt run on every sample (7128.5 times a second)
ISR(TIMER2_OVF_vect) {
  count++;
  if (tx == 0) return;

  // if were done transmitting the last 1 or 0
  // read the next bit and change the tone
  // if end of the array, stop transmitting
  if (count >= sampPerSymb){
    count = 0;
    bitPstn++;
    if(bitPstn > (bits-1)) {
      bitPstn = 0;
      
      if(justshifted != 1) {
        bytePstn++;
      } else {
        justshifted = 0;
      }
      
      setCbuff();
      if (bytePstn > (msgSize-2)){
        // clear variables used here
        bitPstn = 0;
        bytePstn = 0;
        count = 0;
        return;
      }
    }

    unsigned char mve = 0;
    // 0 or 1 to transmit?
    if (lsbf != 1) {
      mve = (bits-1)-bitPstn; // MSB first
    } else {
      mve = bitPstn; // LSB first
    }
    setSymb(mve);
  }

  OCR2B = calcAmp();
}
