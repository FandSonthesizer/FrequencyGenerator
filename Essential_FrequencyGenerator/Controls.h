
const int CHANGELIMIT = 3000;
const int adjustDiff = 110, stableDiff = 30;
const int stableTime = 1200; //[ms]

const uint16_t P_WAVE  = 0;
const uint16_t P_CLOCK  = 1; // LFO - freq.

const uint16_t MAXPOT  = 2;
const int clockPin = PB4;
const int out1 = PB9;

const int AVERAGE =       10;     // For the gliding average method (LP filter for noisy analog values)



/*
uint16_t pin[MAXPOT];             // Analog input pins
uint16_t mcount[MAXPOT]; // 
uint32_t avgsum[MAXPOT]; 
uint16_t Avalue[MAXPOT];  // 0-4096, analog in
*/
typedef struct {
  bool adjustStart;
  bool adjustEnd;
  uint32_t adjustTime;
  uint16_t Avalue;
  uint32_t avgsum;
  uint16_t mcount;
  uint16_t pin;
} Analog_type;
Analog_type a_channel[MAXPOT]; 

void initControls() {
  
  // Define the analog input pins: 
  a_channel[P_WAVE].pin     = PA0;
  a_channel[P_CLOCK].pin    = PA1; 
  pinMode(PC13, OUTPUT); // LED
  pinMode(out1, OUTPUT); // test
  for (int n = 0; n < MAXPOT; n++) {
     a_channel[n].mcount = 0;
     a_channel[n].avgsum = 0;
     a_channel[n].Avalue = 0;
     pinMode(a_channel[n].pin, INPUT_ANALOG);
  }

 
}

/**
 * Gliding Average value.
 * This method simulates a capacitor to even out the noisyness of the stm32's adc.
 * @param newval the ne value, read from adc
 * @param potCh which channel to read
 */
uint32 glidingAverage(uint16 newval, uint16 potCh) {
   uint32 average = 0; 
   if (a_channel[potCh].mcount < AVERAGE) { // initial loading of the capacitor
      a_channel[potCh].avgsum += newval;
      average = a_channel[potCh].avgsum / a_channel[potCh].mcount;
      a_channel[potCh].mcount++;
   }
   else {
    // now hold and change the charge
      a_channel[potCh].avgsum -= a_channel[potCh].avgsum / AVERAGE;
      a_channel[potCh].avgsum += newval;
      average = a_channel[potCh].avgsum / AVERAGE;
   }
   return average;
}


/**
 * read an analog channel. Store value in Avalue[potCh]
 * @param potCh the analoge channel to read
 * @return true if the value was changed
 */
bool updateAnalogChannel(uint16 potCh) {
   // Read the Potentiometer value 
   bool c = false;
   /*Serial.print("Channel=");
   Serial.print(potCh);
   */
   int Nscale = adc_read(ADC1, a_channel[potCh].pin);       // ADC2 read noisy voltage value from pot.
   //Serial.print(" Nscale=");
  // Serial.print(Nscale);
   uint32 m = glidingAverage(  Nscale, potCh );   // Add the new adc value into a gliding average value
   //Serial.print(" GAverage=");
  // Serial.print(m);
   uint32 d = abs(m - a_channel[potCh].Avalue);
   //Serial.print(" diff=");
   //Serial.println(d);
   if (d > 5) {
      a_channel[potCh].Avalue = m;
      c = true;
   }
   return c;
}

//------- Read analog input
bool readAnalogChannel(int ch, uint32_t mil) {
    bool change = false;
    int old = a_channel[ch].Avalue;
    int r_val = adc_read(ADC2, a_channel[ch].pin); 
    int diff = abs(r_val - old);
    //Serial.print(" diff=");
    //Serial.println(diff);
     int val = r_val; 
     //int val = glidingAverage(r_val, ch ); 
     diff = abs(val - old);
     if (!a_channel[ch].adjustStart) {
          if (diff > adjustDiff) {
            //Serial.print(" Not yet in adjust mode, Diff="); 
            /*Serial.print(" r-val=");
            Serial.print(r_val);
            Serial.print(" avg-val=");
            Serial.print(val);
            Serial.print(" diff=");
            Serial.println(diff);
            */
            //Serial.println(" Start adjust mode!");
            a_channel[ch].adjustStart = true;
            a_channel[ch].adjustTime = mil;
            a_channel[ch].Avalue = val;
          }
        }
        else {
        // already in adjust mode
          if (diff < stableDiff) {
            // is the same value
            int tdiff = mil - a_channel[ch].adjustTime;
            if (tdiff > stableTime) {
              a_channel[ch].adjustStart = false;
              //Serial.println("Adjust Mode finished!");
            }
          }
          a_channel[ch].Avalue = val;
          change = true;
         //Serial.print("Adjust mode: Attack Value=");
          //Serial.println(val);
          //EEPROM.write(EEP_L1_Wave, sel);
        }
}




