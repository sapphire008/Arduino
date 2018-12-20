#include <analogShield.h>   // Include to use analog shield.
#include <SPI.h>  // required for ChipKIT but does not affect Arduino
#include <math.h> // library for math
using namespace std;

const float APThreshold = -30; // mv, spike threshold
const int amplitude = -100; // pA, maximum amplitude of the injected SK current
const float tau = 0.75; // sec, decay time of the exponential
const float plateau = 0.01; // sec, time to keep the current at its plateau before exponential rise
const int samplingRate = 13600; // sampling rate, tuned to match the acquisition
const float startTime = 0.002; // delay onset of the negative currents
const float tsDuration = 5.5; // duration of the exponential time series
const int factor = 10; // sampling factor, helpful when tsDuration is too long (> 0.5 seconds)
const int cutoff =  samplingRate * tsDuration - 2*factor;//samplingRate*(startTime+0.25); // // samplingRate*(startTime+0.25); cutoff index of the negative current,3400, 4760, 6120

// Parameters for DAC and ADC channels of Arduino
const int modDACOut0 = 1; // this DAC channel will generate artificial modulations
const int gateADCIn0 = 1; // this ADC channel will set the gating to start or stop detection
const int vmADCIn0 = 0; // this ADC channel will input the action potentials to the box, which will be the target of modulation

// initialize values
int vmValue = 0; // Vm value
int gateValue = 0; // Gate value
float vmOut = 0; // converted Vm
int gateOut = 0; // converted gate (binary)
int modWriteValue = 0; // value to be written for the DAC output

// Array to store negative stim;
float* skArray; // initialize to track the injected SK current
int count = 0; // counting if the array has been exhausted

double mapf(double x, double in_min, double in_max, double out_min, double out_max)
{   
    // mapping floats
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

float *fakeSK( float duration, float startTime, float endTime, float amplitude=-50, int bitRate=10000 ){
    /*
      implementng as a negative square step for now
    */
    // convert to index
    const int arrLength = bitRate * duration;
    const int startIndex = bitRate * startTime;
    const int endIndex = bitRate * endTime;
    float skArr[arrLength];
    // looping over to create the negative step
    for (int a = 0; a < arrLength; a++) {
        if (a < startIndex || a > endIndex){
            skArr[a] = 0;
        }
        else {
            skArr[a] = amplitude;
        }
    }
    return skArr;
}


float *fakeSKExp( float duration, float startTime, float tau=0.1, float amplitude=-50, float plateauTime=0.01, float bitRate=10000 ){
    /*
     Exponentially decaying SK current
     * startTime: delay the exponential rise [sec]
     * tau: time constant of the exponential rise [sec]
     * amplitude: maximum amplitde of the exponential rise [pA]
     * plateauTime: stay at the max value for a certain period of time
     */
    // convert to index
    const int arrLength = bitRate * duration;
    const int startIndex = bitRate * startTime;
    const int plateauIndex = bitRate * (startTime + plateauTime);
    float skArr[arrLength];
    Serial.print("declared array:");
    Serial.println(arrLength);
    // looping over to create the SK current
    for (int a = 0; a < arrLength; a++) {
        if (a < startIndex){
            skArr[a] = 0;
        }
        else if (a < plateauIndex) {
          skArr[a] = amplitude;
        }
        else {
            skArr[a] = amplitude * exp(-1.0/tau * ((a-plateauIndex)/bitRate));
        }
    }
    return skArr;
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(250000);//1000000);
  // fake SK current array
  Serial.println("Starting");
  //skArray = fakeSK(0.03, 0.01, 0.02, amplitude, samplingRate);
  skArray = fakeSKExp(tsDuration, startTime, tau, amplitude, plateau, samplingRate / factor);
  Serial.println("Ending");
}

void loop() {
  // Read the inputs
  vmValue = analog.read(vmADCIn0);
  vmOut = mapf(vmValue, 0, 65535, -100, 100); // the mapping is a guess. But very close. Might need to be fixed in actual experiments
  gateValue = analog.read(gateADCIn0);
  gateOut = round(gateValue / 65535.0);

  if (factor > 1 && count >0 && count % factor != 0) {
    count ++;
    delayMicroseconds(20); // the if statements below probably takes this amount of time to execute
    return;
  }
  
  // Detect spikes (only when the gate is on)
  if (gateOut > 0.5 && count < cutoff) {
     if (vmOut > APThreshold){
      if (count > 0){ // another spike was detected
        count =  ceil(startTime * samplingRate / factor + 2 * factor); // reset bacak to the value at the onset of the current
      }
      modWriteValue = skArray[count/factor];
      if (modWriteValue > 600 || modWriteValue < -600){
        modWriteValue = 0;
      }
      modWriteValue = mapf(modWriteValue, -1000, 1000, 0, 65535);
      analog.write(modDACOut0, modWriteValue);
      count++;
     }
     else { // if vmOut <= APThreshold
      if (count == 0){
         modWriteValue = mapf(0, -1000, 1000, 0, 65535); // reset the value
         analog.write(modDACOut0, modWriteValue);
      }
      else{ // if count > 0
        modWriteValue = skArray[count/factor];
        if (modWriteValue > 600 || modWriteValue < -600){
          modWriteValue = 0;
        }
        modWriteValue = mapf(modWriteValue, -1000, 1000, 0, 65535);
        analog.write(modDACOut0, modWriteValue); // keep going
        count++;
      }
     }
  }
  else {
    modWriteValue = mapf(0, -1000, 1000, 0, 65535); // reset the value
    analog.write(modDACOut0, modWriteValue);
    count = 0;  // reset
  }
  // delay(200); // delay for each print
}
