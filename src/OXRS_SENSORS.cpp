#include "OXRS_SENSORS.h"

#include <Wire.h>             // For I2C
#include <Adafruit_MCP9808.h> // For temp sensor
#include <BH1750.h>           // for BH1750 lux sensor
#include <Adafruit_SHT4x.h>   // for SHT40 temp/humidity sensor

// MCP9808 temp sensor
Adafruit_MCP9808 _mcp9808 = Adafruit_MCP9808();

// BH1750 lux sensor
BH1750 _bh1750;

// SHT40 temperature/humitity Sensor
Adafruit_SHT4x _sht40 = Adafruit_SHT4x();

double round(float value)
{
  return (int)(value * 10.0f) / 10.0f;
}

void OXRS_SENSORS::begin()
{
  Serial.println(F("[sens] scanning for I2C devices..."));

  if (scanI2CAddress(BH1750_I2C_ADDRESS, "BH1750"))
  {
    _bh1750Found = _bh1750.begin(BH1750::CONTINUOUS_HIGH_RES_MODE);
  }

  if (scanI2CAddress(SHT40_I2C_ADDRESS, "SHT40"))
  {
    _sht40Found = _sht40.begin();
    if (_sht40Found)
    {
      _sht40.setPrecision(SHT4X_MED_PRECISION);
      _sht40.setHeater(SHT4X_NO_HEATER);
    }
  }

  if (scanI2CAddress(MCP9808_I2C_ADDRESS, "MCP9808"))
  {
    _mcp9808Found = _mcp9808.begin(MCP9808_I2C_ADDRESS);
    if (_mcp9808Found)
    {
      _mcp9808.setResolution(MCP9808_MODE);
    }
  }
}

bool OXRS_SENSORS::scanI2CAddress(byte address, const char * name)
{
  Serial.print(F("[sens] - 0x"));
  Serial.print(address, HEX);
  Serial.print(F("..."));

  // Check if there is anything responding on this address
  Wire.beginTransmission(address);
  if (Wire.endTransmission() == 0)
  {
    Serial.println(name);
    return true;
  }
  else
  {
    Serial.println(F("empty"));
    return false;
  }
}

void OXRS_SENSORS::setConfigSchema(JsonVariant json)
{
  JsonObject _updateSeconds = json.createNestedObject("sensorUpdateSeconds");
  _updateSeconds["title"] = "Sensor Update Interval (seconds)";
  _updateSeconds["description"] = "How often to read and report values from the connected I2C sensors (defaults to 60 seconds, setting to 0 disables sensor reports). Must be a number between 0 and 86400 (i.e. 1 day).";
  _updateSeconds["type"] = "integer";
  _updateSeconds["minimum"] = 0;
  _updateSeconds["maximum"] = 86400;

  if (_mcp9808Found || _sht40Found)
  {
    JsonObject _tempUnits = json.createNestedObject("sensorTempUnits");
    _tempUnits["title"] = "Sensor Temperature Units";
    _tempUnits["description"] = "Publish temperature reports in celcius (default) or farenhite";
    _tempUnits["type"] = "string";
    JsonArray _tempUnitsEnum = _tempUnits.createNestedArray("enum");
    _tempUnitsEnum.add("c");
    _tempUnitsEnum.add("f");
    JsonArray _tempUnitsEnumNames = _tempUnits.createNestedArray("enumNames");
    _tempUnitsEnumNames.add("celcius");
    _tempUnitsEnumNames.add("farenhite");
  }
}

void OXRS_SENSORS::setCommandSchema(JsonVariant json)
{
  // no commands
}

void OXRS_SENSORS::conf(JsonVariant json)
{
  if (json.containsKey("sensorUpdateSeconds"))
  {
    _updateMs = json["sensorUpdateSeconds"].as<uint32_t>() * 1000L;
  }

  if (json.containsKey("sensorTempUnits"))
  {
    if (strcmp(json["sensorTempUnits"], "c") == 0)
    {
      _tempUnits = TEMP_C;
    }
    else if (strcmp(json["sensorTempUnits"], "f") == 0)
    {
      _tempUnits = TEMP_F;
    }
  }
}

void OXRS_SENSORS::cmnd(JsonVariant json)
{
  // no commands to handle
}

void OXRS_SENSORS::tele(JsonVariant json)
{
  // Ignore if sensor publishing has been disabled
  if (_updateMs == 0) { return; }

  // Check if we are ready to publish
  if ((millis() - _lastUpdate) > _updateMs)
  {
    float temperature = NAN;
    float humidity = NAN;
    float lux = NAN;

    // Earlier sensors in these checks have precedence
    if (_mcp9808Found)
    {
      temperature = _mcp9808.readTempC();
    }

    if (_sht40Found)
    {
      sensors_event_t humid, temp;
      _sht40.getEvent(&humid, &temp);

      if (isnan(temperature))
      {
        temperature = temp.temperature;
      }
      
      if (isnan(humidity))
      {
        humidity = humid.relative_humidity;
      }
    }

    if (_bh1750Found && _bh1750.measurementReady())
    {
      if (isnan(lux))
      {
        lux = _bh1750.readLightLevel();
      }
    }

    if (!isnan(temperature))
    {
      if (_tempUnits == TEMP_F)
      {
        json["temperature"] = round((temperature * 1.8) + 32);
      }
      else
      {
        json["temperature"] = round(temperature);
      }
    }

    if (!isnan(humidity))
    {
      json["humidity"] = round(humidity);
    }

    if (!isnan(lux))
    {
      json["lux"] = round(lux);
    }

    // Reset our timer
    _lastUpdate = millis();
  }
}