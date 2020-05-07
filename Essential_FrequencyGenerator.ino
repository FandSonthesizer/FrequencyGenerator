
/*
 * Frequency generator:
 * Connect 2 Potentiometers To A0 and A1
 * Output is Analog 1
 */
#include "Voice.h" 
#include "Graphic.h"
#include "Controls.h"
#include "PWM.h" 

const bool align = false; 
const bool doLifesign = true; 
const bool debug = false;


uint32_t triggerCount;
uint64_t gTick = 0;

int uTick = 0;
uint16_t freq = 0;
uint16_t lifeCounter = 0;

bool start = true;
bool life = false, o1 = false; // lifesign led state
int controlNumber = 0;
uint32_t globalTic = 0, mil = 0;
int lifeRate = 499;

bool updateC;

  // Frequency Generator:
void setup() {
  
  initGraphic();
  initPwm();
  initVoices();
  initControls();

  Serial.begin(115200);
}


  
void loop() {
  // put your main code here, to run repeatedly:
  if (millis() > 500 && start) {
     start = false;
     if (align) {
      Serial.println("Start Alignment");
      pwmWrite(PWM_OUT, maxPWMAmplitude);
      pwmWrite(PWM_OUT2, maxPWMAmplitude);
    }
    else{
      Serial.println("F&S Sonthesizer Frequency Generator");
    }
  }
  else {
    if (!start && updateC && !debug) {
      uTick = false;
     //Serial.println(controlNumber);
      updateAnalogControls(controlNumber);
      controlNumber++;
      if (controlNumber >= MAXPOT)
        controlNumber = 0;
      updateC = false;
    }
   
   updateDisplay();
 }
}

void updateAnalogControls(int which) {
  int old,val,diff, r_val;
 
  switch(which) {
      case 0:
      //------------LFO waveform poti: ---------------------------
      if (readAnalogChannel(P_WAVE, mil)) {
          int sel = SAW;
          int val = a_channel[P_WAVE].Avalue;
          
          if (val > 2972 )
            sel = SQR;  
          else if (val > 1024 )
            sel = RMP;  
          lfo.waveForm = sel;
          requestToUpdate = true;
        /* Serial.print(val);
           Serial.print(" sel=");
            Serial.println(sel);
         */   
      }
      break;
      case 1:
      // LFO clock divider or freq.
      if (readAnalogChannel(P_CLOCK, mil)) {
        int val = a_channel[P_CLOCK].Avalue;
        Serial.println(val);
        if (val <= 1)
          val = 1; // minimum
        freq = setFreeFreq(val);
        requestToUpdate = true;
      }
      break;
  }
}
  
/**
   * Update the display
   */
  void updateDisplay() {
    if (debug)
      return;
   switch (page) {
    case BASE: // first page, shown on start-up
      if (requestToUpdate) {
       // Serial.print("freq=");
       // Serial.println(freq);
        displayPage0(freq, lfo.waveForm);
      }
      break;
   }
   if (requestToUpdate) {
    requestToUpdate = false;
    display.display();
   }
  }


  /**
 * Interrupt routine, triggered every 25us (40Khz)
 * In this routine, we travel along the wavetable and output the amplitude values we find.
 */
 void handler_Synth(void) {
  gTick++;   // count up all the time at 40khz
  globalTic++;      
      
  if (globalTic > 39) {
    globalTic = 0;        // 40 tics at 40 khz is one millisec.
    mil++;                // increase millisecond tic, used for button debounce etc.
    lifeCounter++;
    if (doLifesign && lifeCounter > lifeRate) {  // Lifesign routine 499
      lifeCounter = 0;
      life = !life;
      digitalWrite(PC13, life); // Lifesign
      
    }
    uTick++;
    if (debug) {
       o1 = !o1;
      digitalWrite(out1, o1); // Lifesign
    }  
  }
  if (!debug) {
    if (uTick > 99) { // analog update every 100ms 
       updateC = true;
       uTick = 0;
    }
    
   if (!start && !align) {
    // doVoice
       addLFOStep();
       bool dac0LFO = false;
       int ch2 = 0;
       // PWM DAC
       ch2 = (lfo.vOutput * maxPWMAmplitude2) / tableAmplitude; // use maxAmplitude to scale it
       ch2 += maxPWMAmplitude2;
       
     // output of this lfo has changed...
       pwmWrite(PWM_OUT, ch2);  //  
       
       //vres = ch2 >> 2; // scope value
       lfo.oldOutput = ch2;
   }
    // Lifesign of interrupt routine, let the onboard LED blink
  
   
  }
  
}


  
