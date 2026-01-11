#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <SPI.h>
#include <XPT2046_Touchscreen.h> 

// --- PIN DEFINITIONS ---
// Display Pins
#define TFT_CS    10
#define TFT_DC    8
#define TFT_RST   4
#define TFT_MOSI  11 
#define TFT_SCK   12 

// Touch Pins
#define T_CS      5     
#define T_MISO    9     // <--- NEW MISO PIN: GPIO 9
#define T_IRQ     7     

// Define the SPI Settings used by the ILI9341 display
SPISettings settingsA(40000000, MSBFIRST, SPI_MODE0); 

// Initialize the display
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCK, TFT_RST);

// Initialize the touch controller
XPT2046_Touchscreen ts(T_CS, T_IRQ);

// --- CALIBRATION VALUES (Landscape Mode: Rotation 1) ---
#define TS_MINX 120
#define TS_MAXX 920
#define TS_MINY 100
#define TS_MAXY 900
#define MIN_PRESSURE 1 // Low pressure for debugging

void setup() {
  Serial.begin(115200);
  Serial.println("--- System Initialized. Testing GPIO 9 MISO ---"); 
  
  // 1. Initialize SPI Bus with all pins explicitly defined
  // SCK=12, MISO=9, MOSI=11, CS=10
  SPI.begin(TFT_SCK, T_MISO, TFT_MOSI, TFT_CS); 
  
  // 2. Initialize Display with SPI Transaction Control
  SPI.beginTransaction(settingsA); 
  tft.begin();
  tft.setRotation(1); 
  tft.fillScreen(ILI9341_BLACK); 
  SPI.endTransaction();
  
  // 3. Initialize Touch
  ts.begin(); 
  ts.setRotation(1); 

  // Draw instructions
  tft.setTextColor(ILI9341_GREEN);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.println("Touch Panel Active!");
  tft.setTextSize(1);
  tft.setCursor(10, 40);
  tft.println("Touch for Raw Data (GPIO 9).");
  tft.setCursor(10, tft.height() - 15);
  tft.println("Press BOOT to clear screen.");
}

void loop() {
  // Check both touch detection methods: the interrupt pin and simple polling
  if (ts.tirqTouched() || ts.touched()) {
    TS_Point p = ts.getPoint();
    
    if (p.z > MIN_PRESSURE) {
      
      // Map the raw touch point to the screen resolution
      int x = map(p.y, TS_MINY, TS_MAXY, tft.width(), 0);
      int y = map(p.x, TS_MINX, TS_MAXX, tft.height(), 0);
      
      tft.fillCircle(x, y, 5, ILI9341_MAGENTA);
      
      // Print raw data 
      Serial.print("Raw X="); Serial.print(p.x);
      Serial.print("\tRaw Y="); Serial.print(p.y);
      Serial.print("\tPressure Z="); Serial.println(p.z); 
    }
  }

  // Clear screen using the BOOT button (GPIO 0)
  if (digitalRead(0) == LOW) {
    tft.fillScreen(ILI9341_BLACK); 
    tft.setTextColor(ILI9341_GREEN);
    tft.setTextSize(2);
    tft.setCursor(10, 10);
    tft.println("Touch Panel Active!");
    tft.setTextSize(1);
    tft.setCursor(10, 40);
    tft.println("Touch for Raw Data (GPIO 9).");
    tft.setCursor(10, tft.height() - 15);
    tft.println("Press BOOT to clear screen.");
    delay(500); 
  }
}