#include <analogShield.h>   // Include to use analog shield.
#include <SPI.h>  // required for ChipKIT but does not affect Arduino
#include <math.h> // library for math
using namespace std;

// Basic parameters
const float APThreshold = -30; // mv, spike threshold
const int amplitude = -50; // pA, maximum amplitude of the injected SK current
const float tau = 1; // sec, decay time of the exponential
const float plateau = 0.01; // sec, time to keep the current at its plateau before exponential rise
const int samplingRate = 1000; // sampling rate, tuned to match the acquisition
const float startTime = 0.1; // delay onset of the negative currents
const float tsDuration = 5.5; // duration of the exponential time series
const int cutoff =  samplingRate * tsDuration; // // samplingRate*(startTime+0.25); cutoff index of the negative current,3400, 4760, 6120
const int delayIndex = startTime * samplingRate - 1; // delay this many indices

// Parameters for DAC and ADC channels of Arduino
const int modDACOut0 = 1; // this DAC channel will generate artificial modulations
const int gateADCIn0 = 1; // this ADC channel will set the gating to start or stop detection
const int vmADCIn0 = 0; // this ADC channel will input the action potentials to the box, which will be the target of modulation

// clocks
unsigned long startMicros;  //some global variables available anywhere in the program
unsigned long currentMicros;
const unsigned long period = 1000;  //the value is a number of microseconds, corresponding to sampling rate

// initialize values
int vmValue = 0; // Vm value
int gateValue = 0; // Gate value
float vmOut = 0; // converted Vm
float previousVmOut = -100; // record the Vm a step before
int gateOut = 0; // converted gate (binary)
int modWriteValue = 0; // value to be written for the DAC output

// Array to store negative stim;
float* skArray; // initialize to track the injected SK current
int count = 0; // indexing if the array has been exhausted
int delayCount = 0; // counting the delay period cycles
int spikeCount = 0; // counting the number of spikes detected so far

double mapf(double x, double in_min, double in_max, double out_min, double out_max)
{   
    // mapping floats
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

float *fakeSKExp( float duration, float startTime, float tau=0.1, float amplitude=-50, float plateauTime=0.01, float bitRate=10000, float initValue=0 ){
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
            skArr[a] = initValue;
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
  Serial.begin(250000);  //start Serial in case we need to print debugging info
  Serial.println("Starting");
  skArray = fakeSKExp(tsDuration, 0, tau, amplitude, plateau, samplingRate);
  Serial.println("Initialized");
  startMicros = micros();  //initial start time
}


void loop()
{
  currentMicros = micros();  //get the current "time" (actually the number of milliseconds since the program started)
  if (currentMicros - startMicros >= period)  //test whether the period has elapsed
  {  
    //IMPORTANT to save the start time of the loop state
    startMicros = currentMicros; 
    
    // Read the inputs
    vmValue = analog.read(vmADCIn0);
    vmOut = mapf(vmValue, 0, 65535, -100, 100); // the mapping is a guess. But very close. Might need to be fixed in actual experiments
    gateValue = analog.read(gateADCIn0);
    gateOut = round(gateValue / 65535.0);
    
    // Only do something when the gate is on
    if (gateOut > 0.5) {
      //Serial.print(previousVmOut);
      //Serial.print(",");
      Serial.println(vmOut);
      if (previousVmOut <APThreshold && vmOut > APThreshold){
        //Serial.println("spike");
      }
      else{ // stayed subthreshold
        
      }
      previousVmOut = vmOut; // record the vm from current step for next loop to compare
      
    } else { // reset everything when gate is off to prepare for next gate on
      spikeCount = 0;
      count = 0;
      delayCount = 0;
      previousVmOut = -100;
    } // end if (gateOut > 0.5)
  } // end if (currentMicros - startMicros >= period*1000)  //test whether the period has elapsed
} // end loop()

void loop_old()
{ 
  currentMicros = micros();  //get the current "time" (actually the number of milliseconds since the program started)
  if (currentMicros - startMicros >= period)  //test whether the period has elapsed
  { 
    // Read the inputs
    vmValue = analog.read(vmADCIn0);
    vmOut = mapf(vmValue, 0, 65535, -100, 100); // the mapping is a guess. But very close. Might need to be fixed in actual experiments
    gateValue = analog.read(gateADCIn0);
    gateOut = round(gateValue / 65535.0);
    
    if (gateOut > 0.5 && count < cutoff) { // gate on, array not exhausted
        if (vmOut > APThreshold){ // crossed the threshold. Spike is detected
          delayCount = 1; // as long as the cell is spiking, reset the delayCount
          delayCount ++;
           // Serial.print("Spike: ");
          // Serial.print(delayCount);
          if (count == 0 || modWriteValue > 600 || modWriteValue < -600){
            modWriteValue = NAN;
          }
          else{
            modWriteValue = skArray[count];
            analog.write(modDACOut0, modWriteValue);
          }
          Serial.print(",");
          Serial.println(modWriteValue);
          count ++; // keep taking values
        }
        else{ // subthreshold
          Serial.print("Low: ");
          Serial.print(delayCount);
          if (count==0){ // not starting to exhuast the array yet
                modWriteValue = mapf(0, -1000, 1000, 0, 65535); // reset the value
                analog.write(modDACOut0, modWriteValue);
                Serial.print(", count = 0");
              }
          else{ // subthreshold, but following previous spiking: the hyperpolarization with long delay must happen here, assuming the ISI is slower than the delay
                if (delayCount < delayIndex){
                  if (delayCount >0){
                    delayCount ++; // increment value
                  }
                  Serial.print(", count>0, inc");
                }
                else{ // enough wait was accumulated
                  if (delayCount > 0){
                    count = 0; // resetting the index of the array
                    delayCount = 0;
                  }
                  Serial.print(",count>0,  reset, ");
                }
                
                modWriteValue = skArray[count];
                if (modWriteValue > 600 || modWriteValue < -600){
                  modWriteValue = NAN;
                } else{
                  modWriteValue = mapf(modWriteValue, -1000, 1000, 0, 65535);
                  analog.write(modDACOut0, modWriteValue); // keep going
                }
                count++;     
              }
            Serial.print(",");
            Serial.println(modWriteValue);
        }
    }
    else{
      modWriteValue = mapf(0, -1000, 1000, 0, 65535); // reset the value
      analog.write(modDACOut0, modWriteValue);
      delayCount = 0;
    }
    startMicros = currentMicros;  //IMPORTANT to save the start time of the loop state
  }
}
