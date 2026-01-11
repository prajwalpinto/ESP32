#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <SPI.h>
#include <WiFi.h>          // ESP32 Wi-Fi library
#include <NTPClient.h>     // Requires NTPClient library installed

// --- CONFIGURATION PINS (Left Header) ---
#define TFT_CS    10
#define TFT_DC    8
#define TFT_RST   4
#define TFT_MOSI  11
#define TFT_SCK   12

// --- WIFI CONFIGURATION ---
const char *ssid     = "";   // <--- CHANGE THIS
const char *password = ""; // <--- CHANGE THIS

// --- TIME CONFIGURATION ---
const long utcOffsetInSeconds = -14400; // -14400 seconds is -4 hours (for Atlantic Time, adjust for your zone)
// Standard Time Zones in seconds:
// EST (New York) = -18000
// PST (Los Angeles) = -28800
// GMT/UTC (London) = 0
// CEST (Berlin) = +7200

// Define NTP Client and connection details
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

// Initialize the display
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCK, TFT_RST);

void setup() {
  Serial.begin(115200);

  // 1. Initialize Display
  tft.begin();
  tft.setRotation(1); // Landscape mode
  tft.fillScreen(ILI9341_BLACK); 

  tft.setCursor(10, 10);
  tft.setTextColor(ILI9341_YELLOW);
  tft.setTextSize(2);
  tft.println("Connecting to WiFi...");
  
  // 2. Connect to Wi-Fi
  WiFi.begin(ssid, password);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  // 3. Check connection
  if (WiFi.status() == WL_CONNECTED) {
    tft.fillScreen(ILI9341_BLACK);
    tft.setCursor(10, 10);
    tft.setTextColor(ILI9341_GREEN);
    tft.println("WiFi Connected!");
    Serial.println("\nWiFi Connected.");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    
    // 4. Initialize and update NTP client
    timeClient.begin();
    timeClient.update();
    delay(2000);
    tft.fillScreen(ILI9341_BLACK);
  } else {
    tft.setCursor(10, 40);
    tft.setTextColor(ILI9341_RED);
    tft.println("WiFi Failed!");
  }
}

void loop() {
  // Check if we are connected before trying to update the time
  if (WiFi.status() == WL_CONNECTED) {
    
    // The NTPClient library updates time only every 60 seconds by default.
    timeClient.update();
    
    // Get the formatted time string (HH:MM:SS)
    String formattedTime = timeClient.getFormattedTime();
    
    // Use a large font for the clock
    tft.setTextSize(5);
    tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK); // Set text and background color (to quickly erase the old time)
    
    // Calculate position to center the clock roughly (adjust 50/100 as needed)
    tft.setCursor(50, 100); 
    
    // Print the time
    tft.print(formattedTime);
    
    Serial.println(formattedTime);
  }

  // Wait 1 second before updating the display again
  delay(1000); 
}