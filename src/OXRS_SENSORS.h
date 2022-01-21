// h file v0.2.0

#ifndef OXRS_SENSORS_H
#define OXRS_SENSORS_H

#include <Arduino.h>
#include <OXRS_MQTT.h>
#include <ArduinoJson.h>
#include <Wire.h>             // For I2C
#include <Adafruit_MCP9808.h> // For temp sensor
#include <Adafruit_SSD1306.h> // for OLED display
#include <RTClib.h>           // for PCF8523 RTC
#include <hp_BH1750.h>        // for bh1750 lux sensor
#include <Adafruit_SHT4x.h>   // for SHT40 Temp / humidity sensor

#if defined(ESP32)
#include <WiFi.h>
#include <ETH.h>
#include <Ethernet.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <Ethernet.h>
#endif

#define DEFAULT_UPDATE_MS 60000

#define DEFAULT_SLEEPENABLE true

// MCP9808 temperature sensor
#define MCP9808_I2C_ADDRESS 0x18
#define MCP9808_MODE 0
#define TEMP_C 0
#define TEMP_F 1

// general temp monitoring varible
#define TEMP_C 1
#define TEMP_F 2

// SHT40 temperature and humidity sensor
#define SHT40_I2C_ADDRESS 0x44

// BH1750 LUX sensor
#define BH1750_I2C_ADDRESS 0x23 // or 0x5C

// PCF8523 RTC Module
#define PCF8523_I2C_ADDRESS 0x68
#define PCF8523_12 0
#define PCF8523_24 1

// OLED Screen
#define OLED_I2C_ADDRESS 0x3C
#define OLED_RESET -1
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
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
#if defined(ESP32)
  OXRS_SENSORS(WiFiClass &wifi, OXRS_MQTT &mqtt);
  OXRS_SENSORS(WiFiClass &wifi, EthernetClass &ethernet, OXRS_MQTT &mqtt);
  OXRS_SENSORS(WiFiClass &wifi, ETHClass &eth, OXRS_MQTT &mqtt);
#elif defined(ESP8266)
  OXRS_SENSORS(ESP8266WiFiClass &wifi, OXRS_MQTT &mqtt);
  OXRS_SENSORS(ESP8266WiFiClass &wifi, EthernetClass &ethernet, OXRS_MQTT &mqtt);
#endif

  void begin();
  void begin(uint8_t pin_sda, uint8_t pin_scl);

  void oled();

  void tele();

  void conf(JsonVariant json);
  void cmnd(JsonVariant json);

private:
  EthernetClass *_ethernet;
#if defined(ESP32)
  ETHClass *_eth;
  WiFiClass *_wifi32;
#elif defined(ESP8266)
  ESP8266WiFiClass *_wifi8266;
#endif

  OXRS_MQTT *_sensorMqtt;

  uint32_t _updateMs = DEFAULT_UPDATE_MS;
  uint32_t _lastUpdate;

  // i2c scan results
  bool _sensorFound = false; // set to true if sensor requiring tele mqtt updated is needed
  bool _tempsensorFound = false;
  bool _luxsensorFound = false;
  bool _oledFound = false;
  bool _rtcFound = false;
  bool _humsensorFound = false;

  // OLED varibles
  uint8_t _screenMode = OLED_MODE_ONE;
  uint8_t _sleepEnable = DEFAULT_SLEEPENABLE;
  uint8_t _sleepState = !_sleepEnable;
  String _screenLineOne = "Hello";
  String _screenLineTwo = "World";

  // mqtt temperature reading
  float _temperature;
  float _tempShtc;
  float _tempShtf;
  float _humSht;

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

  void scanI2CBus();

  void off_screen();
  void one_screen();
  void two_screen();
  void time_screen();
  void temp_screen();
  void hum_screen();
  void lux_screen();
  void IP_screen();
  void MAC_screen();
};

#endif
