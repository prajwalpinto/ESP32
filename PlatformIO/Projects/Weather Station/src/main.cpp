#include "secrets.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <SPI.h>

// --- CUSTOM FONTS FOR SMOOTH TEXT ---
// These fonts are included with the Adafruit GFX library and provide a much cleaner appearance.
#include <Fonts/FreeSansBold24pt7b.h> 
#include <Fonts/FreeSansBold18pt7b.h> // Smaller bold font for main temp
#include <Fonts/FreeSans12pt7b.h>     // Medium font for description and titles

// --- DISPLAY PIN DEFINITIONS (ILI9341) ---
// Note: We are using the pins confirmed in our previous steps.
#define TFT_CS    10
#define TFT_DC    8
#define TFT_RST   4
#define TFT_MOSI  11 // Shared Data
#define TFT_SCK   12 // Shared Clock

// --- WEATHER API CONSTANTS ---
// We use metric for simplicity, change to 'imperial' for Fahrenheit
const char* WEATHER_UNIT = "metric"; 
const char* WEATHER_LANGUAGE = "en";

// Define the SPI Settings used by the ILI9341 display
SPISettings settingsA(40000000, MSBFIRST, SPI_MODE0); 

// Initialize the display
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_SCK, TFT_RST);

// --- STRUCTURES FOR DATA ---
struct WeatherData {
  float temp;
  float feels_like;
  float humidity;
  String description;
  String icon;
  int sunrise;
  int sunset;
  int timezone_offset;
  float wind_speed; 
  float temp_min; // Daily low temperature
  float temp_max; // Daily high temperature
};
WeatherData current_weather;

// -------------------------------------------------------------------
// C++ FUNCTION PROTOTYPES (MANDATORY FOR PlatformIO/main.cpp)
// -------------------------------------------------------------------
void connectWiFi();
void initializeDisplay();
bool fetchWeatherData(WeatherData& data);
void displayWeatherData(const WeatherData& data);
void drawWeatherIcon(const String& icon_code, int x, int y, int color);

// -------------------------------------------------------------------
// SETUP AND LOOP
// -------------------------------------------------------------------

void setup() {
  Serial.begin(115200);
  
  // 1. Initialize Display (with SPI synchronization fix)
  initializeDisplay();
  // Using the built-in font size 2 for the startup message
  tft.setTextSize(2); 
  tft.setCursor(10, 10);
  tft.println("Initializing...");
  
  // 2. Connect to Wi-Fi
  connectWiFi();

  // 3. Initial Weather Fetch
  tft.fillScreen(ILI9341_BLACK);
  // Using the built-in font size 2 for the fetching message
  tft.setTextSize(2); 
  tft.setCursor(10, 10);
  tft.setTextColor(ILI9341_WHITE);
  tft.println("Fetching weather...");
  
  if (fetchWeatherData(current_weather)) {
    Serial.println("Weather data fetched successfully.");
    displayWeatherData(current_weather);
  } else {
    tft.println("\nFailed to get weather.");
  }
}

void loop() {
  // Check WiFi status every loop and attempt to reconnect if disconnected
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
    // Only fetch weather if connection is restored
    if (WiFi.status() == WL_CONNECTED) {
      if (fetchWeatherData(current_weather)) {
        displayWeatherData(current_weather);
      }
    }
  }

  // Fetch and update weather every 15 minutes (15 * 60 * 1000 ms)
  static unsigned long last_update = 0;
  if (millis() - last_update > 900000) { 
    if (fetchWeatherData(current_weather)) {
        displayWeatherData(current_weather);
    }
    last_update = millis();
  }

  delay(1000); 
}

// -------------------------------------------------------------------
// FUNCTION IMPLEMENTATIONS
// -------------------------------------------------------------------

void initializeDisplay() {
  // This is the critical SPI synchronization fix from the last step
  SPI.begin();
  SPI.beginTransaction(settingsA); 
  tft.begin();
  tft.setRotation(1); // Landscape mode (320x240)
  tft.fillScreen(ILI9341_BLACK); 
  tft.setTextWrap(true);
  tft.setTextColor(ILI9341_WHITE);
  // We use setTextSize(1) here as a baseline; custom fonts ignore setTextSize.
  tft.setTextSize(1);
  tft.setCursor(10, 10);
  SPI.endTransaction();
}

void connectWiFi() {
  SPI.beginTransaction(settingsA); 
  tft.fillScreen(ILI9341_BLACK);
  tft.setCursor(10, 10);
  tft.setTextColor(ILI9341_YELLOW);
  tft.setTextSize(2);
  tft.print("Connecting to ");
  tft.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    tft.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    tft.fillScreen(ILI9341_DARKGREEN);
    tft.setTextColor(ILI9341_WHITE);
    tft.setCursor(10, 10);
    tft.println("CONNECTED!");
    tft.setTextSize(1);
    tft.print("IP: ");
    tft.println(WiFi.localIP());
    delay(1000);
  } else {
    tft.fillScreen(ILI9341_RED);
    tft.setTextColor(ILI9341_WHITE);
    tft.setCursor(10, 10);
    tft.println("WIFI FAILED!");
    tft.setTextSize(1);
    tft.println("Check credentials.");
    // Halt and allow user to restart/check wiring
    while(true); 
  }
  SPI.endTransaction();
}

bool fetchWeatherData(WeatherData& data) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Not connected to Wi-Fi. Cannot fetch weather.");
    return false;
  }

  HTTPClient http;
  
  // Construct the API URL
  String serverPath = "http://api.openweathermap.org/data/2.5/weather?";
  serverPath += "lat=" + String(LATITUDE);
  serverPath += "&lon=" + String(LONGITUDE);
  serverPath += "&units=" + String(WEATHER_UNIT);
  serverPath += "&lang=" + String(WEATHER_LANGUAGE);
  serverPath += "&appid=" + String(OPENWEATHERMAP_API_KEY);

  Serial.print("Requesting: ");
  Serial.println(serverPath);

  // Begin HTTP request
  http.begin(serverPath.c_str());
  int httpResponseCode = http.GET();

  if (httpResponseCode > 0) {
    Serial.printf("HTTP Response code: %d\n", httpResponseCode);
    
    // Check if JSON payload is received
    String payload = http.getString();
    Serial.println("Received payload.");

    // Use StaticJsonDocument for parsing
    StaticJsonDocument<4096> doc; 
    DeserializationError error = deserializeJson(doc, payload);

    if (error) {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      http.end();
      return false;
    }

    // Parse the data
    data.temp = doc["main"]["temp"].as<float>();
    data.feels_like = doc["main"]["feels_like"].as<float>();
    data.humidity = doc["main"]["humidity"].as<float>();
    // Retrieve min/max temperature for the day
    data.temp_min = doc["main"]["temp_min"].as<float>(); 
    data.temp_max = doc["main"]["temp_max"].as<float>(); 
    data.description = doc["weather"][0]["description"].as<String>();
    data.icon = doc["weather"][0]["icon"].as<String>();
    data.timezone_offset = doc["timezone"].as<int>();
    // Get the wind speed from the "wind" object
    data.wind_speed = doc["wind"]["speed"].as<float>(); 
    
    http.end();
    return true;
  } else {
    Serial.printf("HTTP Error: %s\n", http.errorToString(httpResponseCode).c_str());
    http.end();
    return false;
  }
}

void displayWeatherData(const WeatherData& data) {
  SPI.beginTransaction(settingsA); 
  tft.fillScreen(ILI9341_BLACK);

  // --- 2. CURRENT TEMPERATURE (Using FreeSansBold18pt7b) ---
  // Position: Y=60 baseline (Adjusted from 70 to 60 to prevent overlap with High/Low)
  tft.setFont(&FreeSansBold18pt7b); // Smaller bold font
  tft.setTextColor(ILI9341_CYAN);
  
  // Create temp string
  String temp_str = String(data.temp, 1) + (String(WEATHER_UNIT).equals("metric") ? "°C" : "°F");
  
  tft.setCursor(5, 60); 
  tft.print(temp_str);

  // --- 3. DAILY HIGH/LOW (Using FreeSans12pt7b) ---
  // Position: Y=95 baseline (Adjusted from 100 to 95 to prevent overlap with Current Temp)
  tft.setFont(&FreeSans12pt7b);
  tft.setTextColor(ILI9341_ORANGE); 
  
  // High/Low string format: H: X°C | L: Y°C
  String unit_char = String(WEATHER_UNIT).equals("metric") ? "C" : "F";
  String hi_low_str = "H: " + String(data.temp_max, 0) + "°" + unit_char + " | L: " + String(data.temp_min, 0) + "°" + unit_char;
  
  tft.setCursor(5, 95); 
  tft.print(hi_low_str);


  // --- 4. WEATHER ICON (Placeholder) ---
  // Position: Y=60 baseline (Keep icon position consistent, near the top temps)
  tft.setFont(NULL); // Switch back to built-in font for the icon drawing function
  tft.setTextSize(1);
  tft.setCursor(tft.width() - 35, 60); // Set cursor for drawing function
  drawWeatherIcon(data.icon, tft.width() - 35, 60, ILI9341_YELLOW);

  // --- 5. DESCRIPTION (Using FreeSans12pt7b) ---
  // Position: Y=135 baseline (Remains the same, now with more room)
  tft.setFont(&FreeSans12pt7b);
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(5, 135);
  
  // Create a mutable copy to call toUpperCase()
  String description_display = data.description;
  description_display.toUpperCase();
  tft.println(description_display);

  // --- 7. DRAW SEPARATOR LINE ---
  // Draw a horizontal line to separate the main display from the details section
  tft.drawLine(0, 150, tft.width(), 150, ILI9341_DARKGREY);


  // --- 5. DETAILS (Humidity, Feels Like, Wind) ---
  // Use FreeSans12pt7b for smooth, non-boxy text.
  tft.setFont(&FreeSans12pt7b); 
  tft.setTextSize(1);
  tft.setTextColor(ILI9341_LIGHTGREY);

  // Humidity (Y position: Adjusted to 175)
  tft.setCursor(5, 175);
  tft.print("Humidity: ");
  tft.print(data.humidity, 0);
  tft.println("%");

  // Feels Like (Y position: Adjusted to 200)
  tft.setCursor(5, 200); 
  tft.print("Feels Like: ");
  tft.print(data.feels_like, 1);
  tft.println(String(WEATHER_UNIT).equals("metric") ? "C" : "F");
  
  // Wind Speed (Y position: Adjusted to 225)
  tft.setCursor(5, 225); 
  tft.print("Wind: ");
  
  // Logic to convert m/s to km/h if metric is used
  float wind_display_value = data.wind_speed;
  String wind_unit_string;

  if (String(WEATHER_UNIT).equals("metric")) {
    // 1 m/s = 3.6 km/h
    wind_display_value = data.wind_speed * 3.6;
    wind_unit_string = " km/h";
  } else {
    // Imperial uses mph, which is already returned by the API when units=imperial
    wind_unit_string = " mph";
  }

  tft.print(wind_display_value, 1);
  tft.println(wind_unit_string);
  
  // --- 6. LAST UPDATED TIME (REMOVED) ---

  SPI.endTransaction();
}

// Simple icon drawing function based on OpenWeatherMap icon codes
void drawWeatherIcon(const String& icon_code, int x, int y, int color) {
  // Clear/Sunny (e.g., 01d)
  if (icon_code.startsWith("01")) {
    tft.fillCircle(x, y, 20, ILI9341_YELLOW); // Sun
  } 
  // Clouds (e.g., 03d, 04d)
  else if (icon_code.startsWith("0") || icon_code.startsWith("04")) {
    tft.fillCircle(x, y, 15, ILI9341_LIGHTGREY);
    tft.fillCircle(x + 10, y + 5, 15, ILI9341_LIGHTGREY);
  }
  // Rain/Showers (e.g., 09d, 10d)
  else if (icon_code.startsWith("09") || icon_code.startsWith("10")) {
    tft.fillCircle(x, y, 15, ILI9341_LIGHTGREY); // Cloud
    tft.drawLine(x - 10, y + 20, x - 5, y + 25, ILI9341_BLUE); // Rain drops
    tft.drawLine(x, y + 20, x + 5, y + 25, ILI9341_BLUE);
  }
  // Snow (e.g., 13d)
  else if (icon_code.startsWith("13")) {
    tft.fillCircle(x, y, 15, ILI9341_WHITE); // Snowflakes (simple)
    tft.drawCircle(x, y, 10, ILI9341_WHITE);
  }
  // Thunderstorm/Mist, etc. (Default)
  else {
    tft.drawRect(x - 10, y - 10, 20, 20, ILI9341_RED);
    tft.setTextColor(ILI9341_RED);
    tft.setCursor(x - 5, y - 5);
    tft.print("?");
  }
}