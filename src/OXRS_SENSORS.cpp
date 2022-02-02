// cpp file v0.4.1

#include "OXRS_SENSORS.h"

// OLED display
Adafruit_SSD1306 _ssd1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// PCF8523 RTC
RTC_PCF8523 _pcf8523;

// MCP9808 temp sensor
Adafruit_MCP9808 _mcp9808 = Adafruit_MCP9808();

// BH1750 lux sensor
BH1750 _bh1750;

// SHT40 Temperature / Humitity Sensor
Adafruit_SHT4x _sht40 = Adafruit_SHT4x();

/*
 *
 *  Main program
 *
 */
OXRS_SENSORS::OXRS_SENSORS(OXRS_MQTT &mqtt)
{
  _sensorMqtt = &mqtt;
}

void OXRS_SENSORS::begin()
{
  scanI2CBus();
}

void OXRS_SENSORS::scanI2CBus()
{
  Serial.println(F("[sens] scanning for i2c devices..."));

  Serial.print(F("[sens] - 0x"));
  Serial.print(BH1750_I2C_ADDRESS, HEX);
  Serial.print(F("..."));
  // Check if there is anything responding on this address
  Wire.beginTransmission(BH1750_I2C_ADDRESS);
  if (Wire.endTransmission() == 0)
  {
    _bh1750Found = _bh1750.begin(BH1750::CONTINUOUS_HIGH_RES_MODE);
    if (!_bh1750Found)
    {
      return;
    }
    Serial.println(F("BH1750"));
  }
  else
  {
    Serial.println(F("empty"));
  }

  Serial.print(F("[sens] - 0x"));
  Serial.print(SHT40_I2C_ADDRESS, HEX);
  Serial.print(F("..."));
  // Check if there is anything responding on this address
  Wire.beginTransmission(SHT40_I2C_ADDRESS);
  if (Wire.endTransmission() == 0)
  {
    _sht40Found = _sht40.begin();
    if (!_sht40Found)
    {
      return;
    }
    Serial.println(F("SHT40"));
    _sht40.setPrecision(SHT4X_MED_PRECISION);
    _sht40.setHeater(SHT4X_NO_HEATER);
  }
  else
  {
    Serial.println(F("empty"));
  }

  Serial.print(F("[sens] - 0x"));
  Serial.print(MCP9808_I2C_ADDRESS, HEX);
  Serial.print(F("..."));
  Wire.beginTransmission(MCP9808_I2C_ADDRESS);
  if (Wire.endTransmission() == 0)
  {
    _mcp9808Found = _mcp9808.begin(MCP9808_I2C_ADDRESS);
    if (!_mcp9808Found)
    {
      return;
    }
    // Set the temp sensor resolution (higher res takes longer for reading)
    _mcp9808.setResolution(MCP9808_MODE);

    Serial.println(F("MCP9808"));
  }
  else
  {
    Serial.println(F("empty"));
  }

  Serial.print(F("[sens] - 0x"));
  Serial.print(SSD1306_I2C_ADDRESS, HEX);
  Serial.print(F("..."));
  // Check if there is anything responding on this address
  Wire.beginTransmission(SSD1306_I2C_ADDRESS);
  if (Wire.endTransmission() == 0)
  {
    _ssd1306Found = _ssd1306.begin(SSD1306_SWITCHCAPVCC, SSD1306_I2C_ADDRESS);
    if (!_ssd1306Found)
    {
      return;
    }
    _ssd1306.clearDisplay();
    _ssd1306.display();
    Serial.println(F("OLED"));
  }
  else
  {
    Serial.println(F("empty"));
  }

  Serial.print(F("[sens] - 0x"));
  Serial.print(PCF8523_I2C_ADDRESS, HEX);
  Serial.print(F("..."));
  // Check if there is anything responding on this address
  Wire.beginTransmission(PCF8523_I2C_ADDRESS);
  if (Wire.endTransmission() == 0)
  {
    Serial.println(F("RTC"));
    _pcf8523Found = _pcf8523.begin();
    if (!_pcf8523Found)
    {
      return;
    }

    if (!_pcf8523.initialized() || _pcf8523.lostPower())
    {
      Serial.println(F("[sens] RTC is NOT initialized!"));
      _pcf8523.adjust(DateTime(F(__DATE__), F(__TIME__)));
      Serial.println(F("[sens] RTC Time set with compile time"));
    }
    _pcf8523.start();
  }
  else
  {
    Serial.println(F("empty"));
  }
}

void OXRS_SENSORS::tele()
{
  // shortcut if sensor reporting is disabled
  if (_updateMs == 0) { return; }

  if ((millis() - _lastUpdate) > _updateMs)
  {
    StaticJsonDocument<150> json;
    char payload[8];

    // MCP9808 temp sensor has precedence over SHT40
    if (_mcp9808Found)
    {
      float temperature = NAN;

      if (_tempMode == TEMP_C)
      {
        temperature = _mcp9808.readTempC();
      }
      else if (_tempMode == TEMP_F)
      {
        temperature = _mcp9808.readTempF();
      }

      if (temperature != NAN)
      {
        sprintf(payload, "%2.1f", temperature);
        json["temperature"] = payload;
      }
    }

    if (_sht40Found)
    {
      sensors_event_t humid, temp;
      _sht40.getEvent(&humid, &temp);

      // only publish temp reading from this sensor if the MCP9808 is not found
      if (!_mcp9808Found)
      {
        // sensor reading is in C, so check if we need to convert to F
        float temperature = temp.temperature;
        if (_tempMode == TEMP_F)
        {
          temperature = (temperature * 1.8) + 32;
        }

        sprintf(payload, "%2.1f", temperature);
        json["temperature"] = payload;
      }

      // always publish humidity if this sensor is found
      float humidity = humid.relative_humidity;
      sprintf(payload, "%2.1f", humidity);
      json["humidity"] = payload;
    }

    if (_bh1750Found && _bh1750.measurementReady())
    {
      float lux = _bh1750.readLightLevel();
      sprintf(payload, "%2.1f", lux);
      json["lux"] = payload;
    }

    if (_ssd1306Found)
    {
      char cstr2[1];
      itoa(_sleepEnable, cstr2, 10);
      json["oledSleepEnable"] = cstr2;

      char cstr1[1];
      itoa(_sleepState, cstr1, 10);
      json["oledSleepState"] = cstr1;
    }
    
    // Check we have something to publish
    if (!json.isNull())
    {
      _sensorMqtt->publishTelemetry(json.as<JsonVariant>());
    }

    // Reset our timer
    _lastUpdate = millis();
  }
}

void OXRS_SENSORS::setConfigSchema(JsonVariant json)
{
  JsonObject _updateSeconds = json.createNestedObject("sensorUpdateSeconds");
  _updateSeconds["title"] = "Sensor Update Interval (seconds)";
  _updateSeconds["description"] = "How often to read and report the values from the connected I2C sensors  (defaults to 60 seconds, setting to 0 disables sensor reports). Must be a number between 0 and 86400 (i.e. 1 day).";
  _updateSeconds["type"] = "integer";
  _updateSeconds["minimum"] = 0;
  _updateSeconds["maximum"] = 86400;

  if (_mcp9808Found || _sht40Found)
  {
    JsonObject _tempMode = json.createNestedObject("sensorTempMode");
    _tempMode["type"] = "string";
    JsonArray _tempEnum = _tempMode.createNestedArray("enum");
    _tempEnum.add("c");
    _tempEnum.add("f");
    JsonArray _tempEnumNames = _tempMode.createNestedArray("enumNames");
    _tempEnumNames.add("celcius");
    _tempEnumNames.add("farenhite");
  }

  if (_pcf8523Found)
  {
    JsonObject _clockMode = json.createNestedObject("sensorClockMode");
    _clockMode["type"] = "string";
    JsonArray _clockEnum = _clockMode.createNestedArray("enum");
    _clockEnum.add("12");
    _clockEnum.add("24");
  }

  if (_ssd1306Found)
  {
    JsonObject _sleepOledenable = json.createNestedObject("sensorSleepOLEDEnable");
    _sleepOledenable["type"] = "boolean";
  }
}

void OXRS_SENSORS::setCommandSchema(JsonVariant json)
{
  if (_pcf8523Found)
  {
    JsonObject _rtcItems = json.createNestedObject("sensorRTC");
    _rtcItems["type"] = "array";

    JsonObject _rtcItems2 = _rtcItems.createNestedObject("items");
    _rtcItems2["type"] = "object";

    JsonObject _rtcProperties = _rtcItems2.createNestedObject("properties");

    JsonObject _rtcYear = _rtcProperties.createNestedObject("year");
    _rtcYear["type"] = "string";

    JsonObject _rtcMonth = _rtcProperties.createNestedObject("month");
    _rtcMonth["type"] = "string";

    JsonObject _rtcDay = _rtcProperties.createNestedObject("day");
    _rtcDay["type"] = "string";

    JsonObject _rtcHour = _rtcProperties.createNestedObject("hour");
    _rtcHour["type"] = "string";
    _rtcHour["description"] = "Requires 24 Hour Format";

    JsonObject _rtcMinute = _rtcProperties.createNestedObject("minute");
    _rtcMinute["type"] = "string";

    JsonObject _rtcSeconds = _rtcProperties.createNestedObject("seconds");
    _rtcSeconds["type"] = "string";

    JsonArray _required = _rtcItems2.createNestedArray("required");
    _required.add("year");
    _required.add("month");
    _required.add("day");
    _required.add("hour");
    _required.add("minute");
    _required.add("seconds");
  }

  if (_ssd1306Found)
  {
    JsonObject _screenMode = json.createNestedObject("sensorScreenMode");
    _screenMode["type"] = "string";
    JsonArray _screenEnum = _screenMode.createNestedArray("enum");
    _screenEnum.add("off");
    _screenEnum.add("one");
    if (!_pcf8523Found && _mcp9808Found && !_sht40Found)
    {
    }
    else
    {
      _screenEnum.add("two");
    }
    if (!_bh1750Found && !_mcp9808Found && !_sht40Found)
    {
    }
    else
    {
      _screenEnum.add("three");
    }
    if (_sht40Found)
    {
      _screenEnum.add("four");
    }
    _screenEnum.add("five");
    JsonArray _screenEnumNames = _screenMode.createNestedArray("enumNames");
    _screenEnumNames.add("off");
    _screenEnumNames.add("IP Address & MAC Address");
    if (!_pcf8523Found && _mcp9808Found && !_sht40Found)
    {
    }
    else
    {
      _screenEnumNames.add("Time & Temperature");
    }
    if (!_bh1750Found && !_mcp9808Found && !_sht40Found)
    {
    }
    else
    {
      _screenEnumNames.add("LUX & Temperature");
    }
    if (_sht40Found)
    {
      _screenEnumNames.add("Humidity & Temperature");
    }
    _screenEnumNames.add("2 Lines of Custom Text");

    JsonObject _oneOLED = json.createNestedObject("sensorOneOLED");
    _oneOLED["type"] = "string";
    _oneOLED["maxLength"] = 10;

    JsonObject _twoOLED = json.createNestedObject("sensorTwoOLED");
    _twoOLED["type"] = "string";
    _twoOLED["maxLength"] = 10;

    JsonObject _sleeping = json.createNestedObject("sensorSleepOLED");
    _sleeping["type"] = "boolean";
  }
}

void OXRS_SENSORS::conf(JsonVariant json)
{
  if (json.containsKey("sensorUpdateSeconds"))
  {
    _updateMs = json["sensorUpdateSeconds"].as<uint32_t>() * 1000L;
  }

  if (json.containsKey("sensorScreenMode")) // for what mode the OLED is in
  {
    if (strcmp(json["sensorScreenMode"], "off") == 0)
    {
      _screenMode = OLED_MODE_OFF;
    }
    if (strcmp(json["sensorScreenMode"], "one") == 0)
    {
      _screenMode = OLED_MODE_ONE;
    }
    if (strcmp(json["sensorScreenMode"], "two") == 0)
    {
      _screenMode = OLED_MODE_TWO;
    }
    if (strcmp(json["sensorScreenMode"], "three") == 0)
    {
      _screenMode = OLED_MODE_THREE;
    }
    if (strcmp(json["sensorScreenMode"], "four") == 0)
    {
      _screenMode = OLED_MODE_FOUR;
    }
    if (strcmp(json["sensorScreenMode"], "five") == 0)
    {
      _screenMode = OLED_MODE_FIVE;
    }
  }

  if (json.containsKey("sensorClockMode"))
  {
    if (strcmp(json["sensorClockMode"], "12") == 0)
    {
      _clockMode = PCF8523_12;
    }
    else if (strcmp(json["sensorClockMode"], "24") == 0)
    {
      _clockMode = PCF8523_24;
    }
  }

  if (json.containsKey("sensorTempMode"))
  {
    if (strcmp(json["sensorTempMode"], "c") == 0)
    {
      _tempMode = TEMP_C;
    }
    else if (strcmp(json["sensorTempMode"], "f") == 0)
    {
      _tempMode = TEMP_F;
    }
  }

  if (json.containsKey("sensorSleepOLEDEnable"))
  {
    _sleepEnable = json["sensorSleepOLEDEnable"].as<bool>() ? HIGH : LOW;
  }
}

void OXRS_SENSORS::cmnd(JsonVariant json)
{
  if (json.containsKey("sensorRTC"))
  {
    for (JsonVariant RTC : json["sensorRTC"].as<JsonArray>())
    {
      jsonRtcCommand(RTC);
    }
  }

  if (json.containsKey("sensorScreenMode")) // for what mode the OLED is in
  {
    if (strcmp(json["sensorScreenMode"], "off") == 0)
    {
      _screenMode = OLED_MODE_OFF;
    }
    if (strcmp(json["sensorScreenMode"], "one") == 0)
    {
      _screenMode = OLED_MODE_ONE;
    }
    if (strcmp(json["sensorScreenMode"], "two") == 0)
    {
      _screenMode = OLED_MODE_TWO;
    }
    if (strcmp(json["sensorScreenMode"], "three") == 0)
    {
      _screenMode = OLED_MODE_THREE;
    }
    if (strcmp(json["sensorScreenMode"], "four") == 0)
    {
      _screenMode = OLED_MODE_FOUR;
    }
    if (strcmp(json["sensorScreenMode"], "five") == 0)
    {
      _screenMode = OLED_MODE_FIVE;
    }
  }

  if (json.containsKey("sensorOneOLED")) // custom text for OLED
  {
    _screenLineOne = json["sensorOneOLED"].as<String>();
  }

  if (json.containsKey("sensorTwoOLED")) // custom text for OLED
  {
    _screenLineTwo = json["sensorTwoOLED"].as<String>();
  }

  if (json.containsKey("sensorSleepOLED"))
  {
    if (_sleepEnable)
    {
      _sleepState = json["sensorSleepOLED"].as<bool>() ? HIGH : LOW;
    }
  }
}

void OXRS_SENSORS::jsonRtcCommand(JsonVariant json)
{
  // TODO: you can use RegEx in your JSON command schema to validate a string 
  //       to ensure it is a known date/time format - this might be easier?
  
  if (json.containsKey("year")) // set RTC time
  {
    if (!json.containsKey("month"))
    {
      return;
    }
    if (!json.containsKey("day"))
    {
      return;
    }
    if (!json.containsKey("hour"))
    {
      return;
    }
    if (!json.containsKey("minute"))
    {
      return;
    }
    if (!json.containsKey("seconds"))
    {
      return;
    }
    //      January 21, 2014 at 3am you would call:
    //      rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
    _pcf8523.adjust(DateTime(json["year"].as<uint16_t>(), json["month"].as<uint8_t>(), json["day"].as<uint8_t>(), json["hour"].as<uint8_t>(), json["minute"].as<uint8_t>(), json["seconds"].as<uint8_t>()));
  }
}

/*
 *
 * OLED code
 *
 */

// single line limit with font 2 is 10 charactors

void OXRS_SENSORS::off_screen() // custom text on line one
{
  _ssd1306.clearDisplay();
  _ssd1306.display();
}

void OXRS_SENSORS::one_screen() // custom text on line one
{
  _ssd1306.setCursor(0, 0);
  _ssd1306.print(_screenLineOne);
}

void OXRS_SENSORS::two_screen() // custom text on line two
{
  _ssd1306.setCursor(0, 16);
  _ssd1306.print(_screenLineTwo);
}

void OXRS_SENSORS::time_screen() // show time on line one
{
  if (_pcf8523Found) // show the time if time is availble
  {
    _ssd1306.setCursor(0, 0);
    if (_clockMode == PCF8523_12) // 12 hour clock requested
    {
      if (_currentHour >= 13)
      {
        _modHour = _currentHour - 12;
        if (_modHour < 10)
        {
          _ssd1306.print(" ");
        }
        _ssd1306.print(_modHour);
      }
      else if (_currentHour == 0)
      {
        _ssd1306.print("12");
      }
      else
      {
        if (_currentHour < 10)
        {
          _ssd1306.print(" ");
        }
        _ssd1306.print(_currentHour);
      }

      _ssd1306.print(":");
      if (_currentMinute < 10)
      {
        _ssd1306.print("0");
      }
      _ssd1306.print(_currentMinute);

      if (_currentHour >= 13)
      {
        _ssd1306.print(" PM");
      }
      else
      {
        _ssd1306.print(" AM");
      }
    }

    if (_clockMode == PCF8523_24) // 24 hour clock requested
    {
      if (_currentHour < 10)
      {
        _ssd1306.print('0');
      }
      _ssd1306.print(_currentHour);
      _ssd1306.print(":");
      if (_currentMinute < 10)
      {
        _ssd1306.print('0');
      }
      _ssd1306.print(_currentMinute);
      _ssd1306.print("h");
    }
  }
}

void OXRS_SENSORS::temp_screen() // line two showing temp
{
  if (_sht40Found)
  {
    _ssd1306.setCursor(0, 16); // line 2
    if (_tempMode == TEMP_C)
    {
      _ssd1306.print(_c, 1);
      _ssd1306.print("\tC");
    }
    if (_tempMode == TEMP_F)
    {
      _ssd1306.print(_f, 1);
      _ssd1306.print("\tF");
    }
  }
  else if (_mcp9808Found)
  {
    _ssd1306.setCursor(0, 16); // line 2
    if (_tempMode == TEMP_C)
    {
      _ssd1306.print(_c, 1);
      _ssd1306.print("\tC");
    }
    if (_tempMode == TEMP_F)
    {
      _ssd1306.print(_f, 1);
      _ssd1306.print("\tF");
    }
  }
}

void OXRS_SENSORS::hum_screen()
{
  if (_sht40Found)
  {
    _ssd1306.setCursor(0, 0); // line 1
    _ssd1306.print(_h, 1);
    _ssd1306.print("% rH");
  }
}

void OXRS_SENSORS::lux_screen() // line one showing lux
{
  if (_bh1750Found)
  {
    _ssd1306.setCursor(0, 0); // line 1
    _ssd1306.print(_l, 1);
    _ssd1306.print(" LUX");
  }
}

void OXRS_SENSORS::IP_screen() // line one showing IP address
{
  _ssd1306.setTextSize(1);
  _ssd1306.setCursor(0, 0); // line 1
  _ssd1306.print(_ipAddress);
}

void OXRS_SENSORS::MAC_screen() // line two showing mac address
{
  _ssd1306.setTextSize(1);
  char mac_display[18];
  sprintf_P(mac_display, PSTR("%02X:%02X:%02X:%02X:%02X:%02X"), _mac[0], _mac[1], _mac[2], _mac[3], _mac[4], _mac[5]);
  _ssd1306.setCursor(0, 16); // line 2
  _ssd1306.print(mac_display);
}

void OXRS_SENSORS::oled() // control the OLED screen if availble
{
  if (_ssd1306Found)
  {
    oledUpdate();
  }
}

void OXRS_SENSORS::oled(byte *mac) // control the OLED screen if availble
{
  memcpy(_mac, mac, 6);

  if (_ssd1306Found)
  {
    oledUpdate();
  }
}

void OXRS_SENSORS::oled(IPAddress ip) // control the OLED screen if availble
{
  _ipAddress = ip;
  if (_ssd1306Found)
  {
    oledUpdate();
  }
}

void OXRS_SENSORS::oledUpdate() // control the OLED screen if availble
{
  if ((millis() - _previousOledtemp) > OLED_INTERVAL_TEMP)
  {
    // MCP9808 temp sensor has precedence over SHT40
    if (_mcp9808Found)
    {
      _c = _mcp9808.readTempC();
      _f = _mcp9808.readTempF();
    }
    
    if (_sht40Found)
    {
      sensors_event_t humid, temp;
      _sht40.getEvent(&humid, &temp);

      if (!_mcp9808Found)
      {
        _c = temp.temperature;
        _f = (temp.temperature * 1.8) + 32;
      }

      _h = humid.relative_humidity;
    }

    // Reset our timer
    _previousOledtemp = millis();
  }

  if ((millis() - _previousOledlux) > OLED_INTERVAL_LUX)
  {
    if (_bh1750Found && _bh1750.measurementReady())
    {
      _l = _bh1750.readLightLevel();
      _previousOledlux = millis();
    }
  }

  if (_pcf8523Found)
  {
    // Time Keeping
    DateTime now = _pcf8523.now();
    _currentHour = now.hour();
    _currentMinute = now.minute();
  }

  if (_sleepState) // screen goes to sleep
  {
    off_screen();
  }
  else
  {
    _ssd1306.clearDisplay();
    _ssd1306.setTextSize(2);
    _ssd1306.setTextColor(WHITE);

    if (_screenMode == OLED_MODE_OFF)
    {
      off_screen();
    }
    if (_screenMode == OLED_MODE_ONE)
    {
      IP_screen();
      MAC_screen();
    }
    if (_screenMode == OLED_MODE_TWO)
    {
      time_screen();
      temp_screen();
    }
    if (_screenMode == OLED_MODE_THREE)
    {
      lux_screen();
      temp_screen();
    }
    if (_screenMode == OLED_MODE_FOUR)
    {
      hum_screen();
      temp_screen();
    }
    if (_screenMode == OLED_MODE_FIVE)
    {
      one_screen();
      two_screen();
    }

    _ssd1306.display();
  }
}