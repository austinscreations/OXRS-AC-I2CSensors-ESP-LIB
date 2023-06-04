#ifndef OXRS_SENSORS_H
#define OXRS_SENSORS_H

#include <Arduino.h>
#include <ArduinoJson.h>

#define DEFAULT_UPDATE_MS 60000

// Temperature units
#define TEMP_C 0
#define TEMP_F 1

// MCP9808 temperature sensor
#define MCP9808_I2C_ADDRESS 0x18
#define MCP9808_MODE 0

// SHT40 temperature and humidity sensor
#define SHT40_I2C_ADDRESS 0x44

// BH1750 LUX sensor
#define BH1750_I2C_ADDRESS 0x23 // or 0x5C

class OXRS_SENSORS
{
public:
  void begin();

  void setConfigSchema(JsonVariant json);
  void setCommandSchema(JsonVariant json);

  void conf(JsonVariant json);
  void cmnd(JsonVariant json);

  void tele(JsonVariant json);

private:
  uint32_t _updateMs = DEFAULT_UPDATE_MS;
  uint32_t _lastUpdate;
  uint8_t  _tempUnits = TEMP_C;

  // I2C scan results
  bool _mcp9808Found = false;
  bool _bh1750Found = false;
  bool _sht40Found = false;

  bool scanI2CAddress(byte address);
};

#endif
