
#include <analogShield.h>   //Include to use analog shield.
#include <SPI.h>  //required for ChipKIT but does not affect Arduino

const int dacOut0 = 3;
int writeValue = 0; // value to write to DAC
int volt = -2; // mV

void setup() {
  // initialize serial communications at 9600 bps:
  Serial.begin(9600);
}

void loop() {

  writeValue = map(volt, -5, 5, 0, 65535);
  analog.write(dacOut0, writeValue); // write to DAC output

  // wait 2 milliseconds before the next loop for the analog-to-digital
  // converter to settle after the last reading:
  delay(10);
}
