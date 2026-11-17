#include <Wire.h>
#include <U8g2lib.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// DS18B20 Data wire connected to GPIO 2
#define ONE_WIRE_BUS 2

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// Constructor for the 0.42-inch OLED (72x40) on ESP32-C3 (SCL=6, SDA=5)
U8G2_SSD1306_72X40_ER_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, 6, 5);

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Start the DS18B20 sensor
  sensors.begin();

  // Start the OLED display
  u8g2.begin();
}

void loop() {
  // Request temperature from the probe
  sensors.requestTemperatures(); 
  float tempC = sensors.getTempCByIndex(0);

  // Print to Serial Monitor for debugging
  Serial.print("Temperature: ");
  Serial.print(tempC);
  Serial.println(" °C");

  // Update OLED Display - Full Screen Temperature Focus
  u8g2.clearBuffer();                    
  
  if (tempC == DEVICE_DISCONNECTED_C) {
    u8g2.setFont(u8g2_font_helvB10_tr);
    u8g2.setCursor(10, 26);
    u8g2.print("Error");
  } else {
    // Format the temperature string (e.g., "21.1 C")
    char tempString[10];
    dtostrf(tempC, 4, 1, tempString);
    strcat(tempString, " C");

    // Use a larger, bold font centered nicely on the 72x40 display
    u8g2.setFont(u8g2_font_helvB18_te);
    
    // Draw centered or neatly positioned across the 72x40 canvas
    u8g2.setCursor(2, 28);
    u8g2.print(tempString);
  }
  
  u8g2.sendBuffer(); // Push buffer to the screen
  
  delay(2000); // Read every 2 seconds
}