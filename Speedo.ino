/*
 * © 2019 Ron Simpkin
 * Public domain - do with it what you will
 * 
 * Works on Arduino Uno and Pro-mini and probably any other 328p based Arduino running at 16MHz
 * 
 * Limitations:
 * lowest output frequency available is > 0.0595Hz
 * highest output frequency is 4MHz -> but very poor resolution up this high (next highest is 2MHz)
 * 
 * Digital Pins are _fixed_
 * Digital pin 2 for incoming pulses
 * Digital pin 9 for outgoing pulses
 * Default pin for analog in is A2
 * 
 * Un-comment ONE define to set the mode
 * 
 * 1. MODE_FIXED:
 *    outputs a fixed frequency = RPM * PULSES_PER_REVOLUTION / 60
 *    change the #defines to suit your needs - all values must be postfixed UL  
 * 2. MODE_ANALOG_IN:
 *    outputs a frequency with a settable minimum and maximum depending on
 *    the voltage on an analog pin (default pin A2)
 * 3. MODE_PULSE_IN
 *    takes an input signal on pin 2 and outputs a signal on pin 9
 *    set SOURCE_PULSES equal to the number of pulses in per revolution
 *    set PULSES_PER_REVOLUTION equal to the number of pulses you need to go out each full revolution
 *    set RPM_CUTOFF to the minimum RPM input you want to map directly to output pulses
 *    when the input drops below RPM_CUTOFF then the output is RPM_MIN
 *    NB. SOURCE_PULSES can't be set to 1. If it is actually 1 set it to 2 and double the other settings.
 */

//#define MODE_FIXED
//#define MODE_ANALOG_IN
#define MODE_PULSE_IN


const int pulseInPin = 2;
const int pulseOutPin = 9;
const int analogInPin = A2;
float fTarget, fMinimum, lastTarget;

#ifdef MODE_FIXED
  #define PULSES_PER_REVOLUTION 1
  #define RPM 2000UL
#endif

#ifdef MODE_ANALOG_IN
  #define PULSES_PER_REVOLUTION 8
  #define RPM_MIN 40
  #define RPM_MAX 800UL
  unsigned long analogValue;
  unsigned long lastValue;
#endif

#ifdef MODE_PULSE_IN
  #define SOURCE_PULSES 2
  #define PULSES_PER_REVOLUTION 10
  #define RPM_CUTOFF 2UL
  #define RPM_MIN 0
  volatile int pulIn = 0;
  volatile int lastPul = 0;
  volatile unsigned long now;
  volatile unsigned long startMicros[SOURCE_PULSES];
  volatile unsigned long lastPulMicros = 0;
  volatile float fIn = 0;
  volatile int suppressStart = 0;
  float fCutoff;
#endif

void setup()
{
  pinMode(pulseOutPin, OUTPUT); // set pin 9 as output - it's this pin that get togged by PWM

  noInterrupts();
  // Configure timer 1 (16bit resolution)
  TCCR1A = 0;
  TCCR1B = 0;
  OCR1A = 0;
  // Select Phase & Frequency Correct PWM Mode, and OCR1A as TOP. 
  // Enable COM1A0 to toggle OC1A on compare match (pin 9)
  TCCR1A |= ((1 << WGM10) | (1 << COM1A0));
   

#ifdef MODE_FIXED
  fTarget = (float) RPM * PULSES_PER_REVOLUTION / 60;
  setPWM(fTarget, fTarget);
#endif 
#ifdef RPM_MIN
  fMinimum = (float) RPM_MIN * PULSES_PER_REVOLUTION / 60;
#endif

#ifdef MODE_PULSE_IN
  fCutoff = (float)RPM_CUTOFF * PULSES_PER_REVOLUTION/ 60;
  for (int i=0; i < SOURCE_PULSES; i++){
    startMicros[i] = 0;
  }

  pinMode(pulseInPin, INPUT_PULLUP);
  // use interrupt 0 (pin 2) and run function countPulse when pin 2 goes HIGH
  attachInterrupt(digitalPinToInterrupt(pulseInPin), countPulseIn, RISING);
#endif
  interrupts();
}

void loop(){ 
#ifdef MODE_ANALOG_IN
  int pinVal = analogRead(analogInPin);  
  analogValue = map(pinVal, 0, 1023, RPM_MIN, RPM_MAX);
  fTarget = (float) analogValue * PULSES_PER_REVOLUTION / 60;
  if (lastValue != analogValue){
    setPWM(fTarget, max(fTarget, fMinimum));
    lastValue = analogValue;
  }
#else if MODE_PULSE_IN
  float f = fIn;
  fTarget = f * PULSES_PER_REVOLUTION;
  if (fTarget < fCutoff){
    fTarget = fMinimum;
  }  
  if (micros() - lastPulMicros > 1000000UL){
    fTarget = fMinimum;
  }
  if (lastTarget != fTarget){
    lastTarget = fTarget;
    if (suppressStart == SOURCE_PULSES){
      setPWM(fTarget, fTarget);
    }
  }
#endif
}

void setPWM(float fTgt, float fMin){
  float pwmClock;
  
  if (fTgt <= 0.0595){OCR1A = 0;return;}
  /* Set all of TCCR1B in one operation - Binary assignment!*/
  if (fMin > 61.1 ){
    //TCCR1B |= ((1 << CS10) | (1 << WGM13)); // Fcpu/4 61.1Hz -> 4MHz
    TCCR1B = B00010001;
    pwmClock = 4000000.00;
  }else if (fMin > 7.64){
    //TCCR1B |= ((1 << CS11) | (1 << WGM13)); // Fcpu/32 7.64Hz-> 500KHz
    TCCR1B = B00010010;
    pwmClock = 500000.00;
  }else if (fMin > 0.952){
    //TCCR1B |= ((1 << CS10) | (1 << CS11) | (1 << WGM13)); // Fcpu/256 952mHz->62.5KHz
    TCCR1B = B00010011;
    pwmClock = 62500.00;
  }else if (fMin > 0.239){
    //TCCR1B |= ((1 << CS12) | (1 << WGM13)); // Fcpu/1024 239mHz -> 16.25 KHz
    TCCR1B = B00010100;
    pwmClock = 15625.00;
  }else{
    //TCCR1B |= ((1 << CS10) | (1 << CS12) | (1 << WGM13)); // Fcpu/4096 59.5mHz -> 3.9 KHz
    TCCR1B = B00010101;
    pwmClock = 3906.25;
  }
  OCR1A = round(pwmClock/fTgt);

}

#ifdef MODE_PULSE_IN
void countPulseIn() {
  lastPul = pulIn;
  now = micros();
  if (pulIn < SOURCE_PULSES -1){
    pulIn++;
  }else{
    pulIn = 0;
  }
  if (suppressStart > SOURCE_PULSES -1){
    fIn = (float)1000000/(now - startMicros[pulIn]);
  }else{
    suppressStart++;
  }
  lastPulMicros = now;
  startMicros[pulIn] = now;
}
#endif
