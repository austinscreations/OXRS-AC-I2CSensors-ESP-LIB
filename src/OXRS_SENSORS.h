// h file v0.5.0

#ifndef OXRS_SENSORS_H
#define OXRS_SENSORS_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Wire.h>             // For I2C
#include <Adafruit_MCP9808.h> // For temp sensor
#include <Adafruit_SSD1306.h> // for OLED display
#include <RTClib.h>           // for PCF8523 RTC
#include <BH1750.h>           // for BH1750 lux sensor
#include <Adafruit_SHT4x.h>   // for SHT40 Temp / humidity sensor

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#else
#include <WiFi.h>
#endif

#define DEFAULT_UPDATE_MS 60000

#define DEFAULT_OLED_SLEEP_ENABLE true

// general temp monitoring varible
#define TEMP_C 0
#define TEMP_F 1

// MCP9808 temperature sensor
#define MCP9808_I2C_ADDRESS 0x18
#define MCP9808_MODE 0

// SHT40 temperature and humidity sensor
#define SHT40_I2C_ADDRESS 0x44

// BH1750 LUX sensor
#define BH1750_I2C_ADDRESS 0x23 // or 0x5C

// PCF8523 RTC Module
#define PCF8523_I2C_ADDRESS 0x68
#define PCF8523_12 0
#define PCF8523_24 1

// OLED Screen
#define SSD1306_I2C_ADDRESS 0x3C
#define OLED_INTERVAL_TEMP 10000L
#define OLED_INTERVAL_LUX 2500L

// Supported OLED modes
#define OLED_MODE_OFF 0   // screen is off
#define OLED_MODE_ONE 1   // screen shows IP and MAC address
#define OLED_MODE_TWO 2   // screen shows Time and Temp
#define OLED_MODE_THREE 3 // screen shows Temp and LUX
#define OLED_MODE_FOUR 4  // screen shows Temp and Humidity
#define OLED_MODE_FIVE 5  // screen shows Custom text per - line

class OXRS_SENSORS
{
public:
  void begin();

  void oled();
  void oled(IPAddress ip);
  void oled(byte * mac);

  void tele(JsonVariant json);

  void conf(JsonVariant json);
  void cmnd(JsonVariant json);

  void setConfigSchema(JsonVariant json);
  void setCommandSchema(JsonVariant json);

  float getTemperatureValue(void);
  bool  getTemperatureUnits(void);

private:

  IPAddress _ipAddress;

  byte _mac[6];

  uint32_t _updateMs = DEFAULT_UPDATE_MS;
  uint32_t _lastUpdate;

  // i2c scan results
  bool _bh1750Found = false;
  bool _mcp9808Found = false;
  bool _sht40Found = false;
  bool _ssd1306Found = false;
  bool _pcf8523Found = false;

  // OLED varibles
  uint8_t _screenMode = OLED_MODE_ONE;
  uint8_t _sleepEnable = DEFAULT_OLED_SLEEP_ENABLE;
  uint8_t _sleepState = !_sleepEnable;
  String _screenLineOne = "Hello";
  String _screenLineTwo = "World";

  // universal tmperature variable - for degrees c / f
  bool _tempMode = TEMP_C;

  // sensor varibles - using with OLED
  float _c; // temp reading variable
  float _f; // temp reading variable
  float _h; // hum  reading variable sht40
  float _l; // lux reading variable
  unsigned long _previousOledtemp = 0;
  unsigned long _previousOledlux = 0;

  // RTC varibles
  uint8_t _modHour;
  int _currentHour;
  int _currentMinute;
  uint8_t _clockMode = PCF8523_24;

  void jsonRtcCommand(JsonVariant json);

  void scanI2CBus();
  bool scanI2CAddress(byte address);

  void off_screen();
  void one_screen();
  void two_screen();
  void time_screen();
  void temp_screen();
  void hum_screen();
  void lux_screen();
  void IP_screen();
  void MAC_screen();
  void oledUpdate();
};

#endif
