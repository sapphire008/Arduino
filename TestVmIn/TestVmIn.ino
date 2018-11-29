
#include <analogShield.h>   //Include to use analog shield.
#include <SPI.h>  //required for ChipKIT but does not affect Arduino

const int dacOut0 = 1; // this is the input channel
const int adcIn0 = 0; // this is the corresponding channel for ADC 
int writeValue = 0; // value to write to DAC
float volt = 4.1; // V, current voltage to check

int adcValue = 0; // value read from ADC
float outValue = 0; // mapped value

double mapf(double x, double in_min, double in_max, double out_min, double out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}


void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);

}

void loop() {
  // put your main code here, to run repeatedly:
  writeValue = mapf(volt, -5, 5, 0, 65535);
  analog.write(dacOut0, writeValue); // write to DAC output
  delay(2); // delay
  adcValue = analog.read(adcIn0); // read from ADC
  outValue = mapf(adcValue, 0, 65535, -5, 5); // map the read value back

  Serial.print("ADC = ");
  Serial.println(outValue);

  delay(200); // delay for each print
}
