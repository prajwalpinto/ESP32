#include <Arduino.h>
#include <Wire.h>

// Your defined pins
#define I2C_SDA 16
#define I2C_SCL 17

void setup() {
  Serial.begin(115200);
  while (!Serial); // Wait for serial monitor
  Serial.println("\nI2C Scanner");

  // Force the ESP32-S3 to use these specific pins
  Wire.begin(I2C_SDA, I2C_SCL);
}

void loop() {
  byte error, address;
  int nDevices = 0;

  Serial.println("Scanning...");

  for (address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (address < 16) Serial.print("0");
      Serial.print(address, HEX);
      Serial.println("  !");
      nDevices++;
    } else if (error == 4) {
      Serial.print("Unknown error at address 0x");
      if (address < 16) Serial.print("0");
      Serial.println(address, HEX);
    }
  }
  
  if (nDevices == 0) {
    Serial.println("No I2C devices found\n");
    Serial.println("Check: 1. Wiring (SDA/SCL swapped?)");
    Serial.println("       2. Voltage (Try 3.3V instead of 5V)");
    Serial.println("       3. Pull-up resistors needed?");
  } else {
    Serial.println("done\n");
  }
  
  delay(5000); 
}