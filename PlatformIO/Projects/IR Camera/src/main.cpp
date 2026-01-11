#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Adafruit_MLX90640.h>
#include <SPI.h>

// --- PIN DEFINITIONS ---
// Display Pins (Same as your working weather station)
#define TFT_CS    10
#define TFT_DC    8
#define TFT_RST   4
#define TFT_MOSI  11
#define TFT_SCK   12

// IR Sensor Pins (I2C)
#define I2C_SDA   16
#define I2C_SCL   17

// --- CONFIGURATION ---
// MLX90640 Address (Default is 0x33)
#define MLX90640_I2C_ADDR 0x33

// Scale factor: 32x24 sensor -> 320x240 screen (Scale = 10)
#define PIXEL_SCALE 10 

// Display Settings
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCK, TFT_RST);
SPISettings settingsA(40000000, MSBFIRST, SPI_MODE0);

// Sensor Object
Adafruit_MLX90640 mlx;

// Frame buffer for the thermal sensor (32x24 = 768 pixels)
float frame[32 * 24]; 

// Variables for auto-ranging colors
float minTemp = 20.0;
float maxTemp = 40.0;

// Function Prototypes
void initializeDisplay();
void initializeSensor();
uint16_t mapTempToColor(float val, float minVal, float maxVal);
void drawInterface();

// -------------------------------------------------------------------
// SETUP
// -------------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  
  // 1. Initialize Display
  initializeDisplay();
  tft.println("Initializing IR...");

  // 2. Initialize Sensor
  initializeSensor();

  tft.fillScreen(ILI9341_BLACK);
}

// -------------------------------------------------------------------
// LOOP
// -------------------------------------------------------------------
void loop() {
  // 1. Capture Data from MLX90640
  // getFrame returns 0 on success
  if (mlx.getFrame(frame) != 0) {
    Serial.println("Failed to read from sensor");
    return;
  }

  // 2. Find Min/Max Temp in the current frame for Auto-Scaling
  minTemp = 1000.0; // Start high
  maxTemp = -1000.0; // Start low

  for (int i = 0; i < 768; i++) {
    float t = frame[i];
    if (t < minTemp) minTemp = t;
    if (t > maxTemp) maxTemp = t;
  }

  // Add a small buffer to prevent flickering if min == max
  if ((maxTemp - minTemp) < 1.0) maxTemp = minTemp + 1.0;

  // 3. Draw the Thermal Image
  SPI.beginTransaction(settingsA);
  
  // Iterate through 24 rows and 32 columns
  for (uint8_t h = 0; h < 24; h++) {
    for (uint8_t w = 0; w < 32; w++) {
      // Calculate index in the 1D array
      // NOTE: Depending on how you mounted the sensor, you might need to flip these logic
      // Standard: index = h * 32 + w
      float t = frame[h * 32 + w]; 

      // Get Color
      uint16_t color = mapTempToColor(t, minTemp, maxTemp);

      // Draw 'Big Pixel' (10x10 rectangle)
      // x = w * 10, y = h * 10
      tft.fillRect(w * PIXEL_SCALE, h * PIXEL_SCALE, PIXEL_SCALE, PIXEL_SCALE, color);
    }
  }

  // 4. Draw Overlay Information (Center Temp, Min/Max)
  // We draw this AFTER the image so it sits on top
  drawInterface();
  
  SPI.endTransaction();
  
  // No delay needed; the sensor read takes time naturally (~250ms at 4Hz)
}

// -------------------------------------------------------------------
// HELPER FUNCTIONS
// -------------------------------------------------------------------

void initializeDisplay() {
  SPI.begin();
  SPI.beginTransaction(settingsA); 
  tft.begin();
  tft.setRotation(1); // Landscape
  tft.fillScreen(ILI9341_BLACK); 
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  SPI.endTransaction();
}

void initializeSensor() {
  // Start I2C on user defined pins
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(400000); // 400 KHz I2C speed

  if (!mlx.begin(MLX90640_I2C_ADDR, &Wire)) {
    Serial.println("MLX90640 not found!");
    tft.fillScreen(ILI9341_RED);
    tft.setCursor(10, 10);
    tft.println("Sensor Error!");
    tft.println("Check wiring:");
    tft.println("SDA -> 16");
    tft.println("SCL -> 17");
    while (1);
  }

  Serial.println("MLX90640 Found!");
  
  // FIX 1: Correct Refresh Rate Syntax
  mlx.setRefreshRate(MLX90640_4_HZ); 
  
  // FIX 2: Correct Mode Syntax (Interleaved = false/off for chess mode)
  mlx.setMode(MLX90640_INTERLEAVED); 
}

// Map temperature to a simple heatmap (Blue -> Green -> Red)
uint16_t mapTempToColor(float val, float minVal, float maxVal) {
  // Constrain value
  if (val < minVal) val = minVal;
  if (val > maxVal) val = maxVal;

  // Map to 0.0 - 1.0
  float rel = (val - minVal) / (maxVal - minVal);

  uint8_t r = 0, g = 0, b = 0;

  // Simple Heatmap logic
  // Cold (0.0 to 0.5): Blue fades to Green
  // Hot (0.5 to 1.0): Green fades to Red
  if (rel < 0.5) {
    float localRel = rel * 2.0; // 0.0 to 1.0
    b = 255 * (1.0 - localRel);
    g = 255 * localRel;
  } else {
    float localRel = (rel - 0.5) * 2.0; // 0.0 to 1.0
    g = 255 * (1.0 - localRel);
    r = 255 * localRel;
  }

  return tft.color565(r, g, b);
}

void drawInterface() {
  // Draw Crosshair in center
  int centerX = tft.width() / 2;
  int centerY = tft.height() / 2;
  
  tft.drawFastHLine(centerX - 10, centerY, 20, ILI9341_WHITE);
  tft.drawFastVLine(centerX, centerY - 10, 20, ILI9341_WHITE);

  // Calculate center temperature (approximate pixel index)
  // Center is roughly row 12, col 16
  float centerTemp = frame[12 * 32 + 16];

  // Draw Text Backgrounds for readability
  tft.setTextSize(1);
  tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK); // White text, Black bg

  // Show Center Temp
  tft.setCursor(centerX + 5, centerY + 5);
  tft.print(centerTemp, 1);
  tft.print("C");

  // Show Range (Min / Max) at bottom
  tft.setCursor(5, 220);
  tft.print("Min: "); tft.print(minTemp, 0);
  
  tft.setCursor(240, 220);
  tft.print("Max: "); tft.print(maxTemp, 0);
}