/**
 * Describes the voices
 */
#include "Sinus2048Int.h" // 2048
#include "Tri2048Int.h" // 
#include "Saw2048Int.h" // 
#include "Log.h" // 
#include "Essential_FrequencyGenerator.h"


const int TRI = 0;
const int SAW = 1;
const int RMP = 2;
const int SQR = 3;
const int SIN = 4;
const int WMAX = 5; // number of waveforms

const uint32_t ISR_RATE = 40000;

const int MAXVOICE = 1;
const uint16_t maxPWMAmplitude = 1440; // 1440, 960

uint16_t maxPWMAmplitude2 = maxPWMAmplitude / 2;
const uint16_t maxDACAmplitude = 4095;
uint16_t maxDACAmplitude2 = maxDACAmplitude / 2;
const uint16_t tableAmplitude = 2047;


/**
 * Note: the voice-index is the same as the adsr index! (a slot)
 */
typedef struct {
  uint32_t step;
  uint32_t gstep; // glide step
  uint32_t tableIndex;
  int16_t oldOutput;
  int16_t vOutput;
  uint16_t waveForm;              // which wavetable to play (saw, sin etc.)
  uint16_t clockCnt;
  uint16_t clockDividerIndex;          // how many clock pulses to count for one period
  uint16_t volume;                // volume of this lfo
  int16_t sample;                // a sample value
  int16_t glide;                 // amount to glide to the next sample value
  int16_t gstart;                // where to start to glide
  
 } Voice_type;

// Interrupt setup:
const uint16_t isr_cnt =  25;     // interrupt rate 100us= 10khz; 50us = 20khz; 25us = 40KHz
//const uint32_t ISR_RATE = 10000;  // interrupt rate 10000Hz = 10 pulses per ms = 100us per isr
const uint16_t rateMultiplier = 100; //100us per isr
const int DAC0 = 0;
const int DAC1 = 1;

const int LFOPrecision = 11;      // multiply index counter with 2^11 to be able to go below 1 Hz with a good enough precision

/**
 * One entry per simultane playing lfo
 */
Voice_type lfo; //
 
uint32_t tableLength = SawLengthInt;  // length of the 16bit high res int wavetable
uint32_t tableLength2 = tableLength / 2;
uint32_t lfoTableIndexMax = (tableLength << LFOPrecision) - 1; ;  // here the table overflows
uint32_t lfoTableIndexMax2 = lfoTableIndexMax >> 1;               // half of the table, used for the clock led

// global vars 
bool state = false;

/**
 * Method declaration
 */
void getWaveAmp(uint16_t);


void initVoices() {
  /** 
   *  Setup Timer 2 to 25u (40000 per second)
   */
  Timer2.setMode(TIMER_CH1, TIMER_OUTPUTCOMPARE);
  Timer2.setPeriod(isr_cnt);                          // in microseconds (25u)
  Timer2.setCompare(TIMER_CH1, 1);                   
  Timer2.attachInterrupt(TIMER_CH1, handler_Synth);   // in this routine the synth moves through the wave-table
  
  lfo.step = 0;
  lfo.waveForm = SAW;
  lfo.volume = 4095; 
  
}

/**
 * Select the waveform of a voice
 */
void selectWaveForm(int w) {
  lfo.waveForm = w;
}

/**
   @param val the potentiometer value (0-406)
*/
int setFreeFreq(uint16_t val) {
  val = getLog(val);
  uint64_t z = tableLength  * (uint64_t) val; // multiply low res table-length by target frequency
  z = z << LFOPrecision; // << 11 = *2048 to increase precision of table pointer calculation
  lfo.step = z / ISR_RATE;
  return val;
}



/**
 * Here we need to get lower than 1 HZ !
 * range = 1 - 1600 equals 1/16 Hz - 100Hz
 * @param lf the freq. of the LFO = [Hz * 16]
 * @param n lfo index
 * @param p one period in [1/10 ms}
 */
void setLFOFreq(uint16_t lf, int p) {
   uint64_t z = tableLength  * (uint64_t) lf; // multiply low res table-length by target frequency
   z = z << LFOPrecision; // << 11 = *2048 to increase precision of table pointer calculation
   lfo.step = z / ISR_RATE;
   /*Serial.print(" LFO z=");
   Serial.print(z);
   Serial.print(" step=");
   Serial.print(voice[LFO].step);
   Serial.print(" used step=");
   Serial.println(voice[LFO].step >> 4);
   */
}


void addLFOStep() {
  //lfo[n].oldOutput = lfo[n].vOutput; // previous value
  //Serial.print("old=");
  //Serial.print(lfo[n].oldOutput);

  uint16_t indexInTable = 0;

  lfo.tableIndex += lfo.step; //
  indexInTable = lfo.tableIndex >> LFOPrecision; //remember: the tableIndex was multiplied by 2048, so divide here
  indexInTable = indexInTable & 0x7ff; // & 2048 makes sure, that the table index is always inside the table!
  //Serial.print("indexInTable=");
  //Serial.println(indexInTable);
  int16_t c = 0;
  // High Res table (0-4095 as amplitude)
  
  switch (lfo.waveForm) {
    case SAW:
      c = getSawInt(indexInTable);
      break;
    case RMP:
      c = getSawInt(indexInTable);
      c = c * -1;
      break;
    case TRI:
      c = getTriInt(indexInTable);
      break;
    case SIN:
      c = getSinInt(indexInTable);
      break;
    case SQR:
      if ( indexInTable < tableLength2) {
        c = maxDACAmplitude2;
      }
      else {
        c = -maxDACAmplitude2;
      }
      break;
  } 
  /*Serial.print(" raw out=");
    Serial.print(c);
    Serial.print(" vol=");
    Serial.print(lfo[n].volume);
  */
 
  int32_t z1 = c * lfo.volume; // volume = 0-4095
  lfo.vOutput = z1 >> 12; // : 4096
  // Serial.print(" out=");
  // Serial.print(lfo[n].vOutput);
}

/**
 * Increase all active Wavetable-pointers
 * and get the acutal active analog output value
 * @return the new analog output value
 */
void addLFOStep9() {
  //lfo[n].oldOutput = lfo[n].vOutput; // previous value
  //Serial.print("old=");
  //Serial.print(lfo[n].oldOutput);
  uint16_t indexInTable = 0; 

  //lfo.tableIndex += (lfo.step >> 1); // step / 16
  lfo.tableIndex += lfo.step; // step / 16
  if ( lfo.tableIndex >= lfoTableIndexMax) 
    lfo.tableIndex = 0;
  
 
  indexInTable = ( lfo.tableIndex >> LFOPrecision); //remember: the tableIndex was multiplied by 2048, so divide here
  int16_t c = 0;
  // High Res table (0-4095 as amplitude)
  switch(lfo.waveForm) {
    case SAW:
      c = getSawInt(indexInTable);
    break;
    case RMP:
      c = getSawInt(indexInTable);  
      c = c * -1;
    break;
    case SQR:
      if ( indexInTable < tableLength2) {
        c = maxDACAmplitude2; 
      }
      else {
        c = -maxDACAmplitude2; 
      }
      break;
    }
   /*Serial.print(" raw out=");
    Serial.print(c);
    Serial.print(" vol=");
    Serial.print(lfo[n].volume);
    */
    int32_t z1 = c * lfo.volume;
    lfo.vOutput = z1 >> 12; // : 4096
   // Serial.print(" out=");
   // Serial.print(lfo[n].vOutput);
}

 

