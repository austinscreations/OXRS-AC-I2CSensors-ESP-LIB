// cpp file v0.4.1

#include "OXRS_SENSORS.h"

// OLED display
Adafruit_SSD1306 _display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// PCF8523 RTC
RTC_PCF8523 _rtc;

// MCP9808 temp sensor
Adafruit_MCP9808 _tempSensor = Adafruit_MCP9808();

// BH1750 lux sensor
hp_BH1750 _luxSensor;

// SHT40 Temperature / Humitity Sensor
Adafruit_SHT4x _sht4 = Adafruit_SHT4x();

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
  Wire.begin();
  scanI2CBus();
}

void OXRS_SENSORS::begin(uint8_t pin_sda, uint8_t pin_scl)
{
  Wire.begin(pin_sda, pin_scl);
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
    _luxsensorFound = _luxSensor.begin(BH1750_TO_GROUND);
    if (!_luxsensorFound)
    {
      return;
    }
    Serial.println(F("BH1750"));
    _luxSensor.start(); // start getting value (for non-blocking use)
    _sensorFound = true;
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
    _humsensorFound = _sht4.begin();
    if (!_humsensorFound)
    {
      return;
    }
    Serial.println(F("SHT40"));
    _sht4.setPrecision(SHT4X_MED_PRECISION);
    _sht4.setHeater(SHT4X_NO_HEATER);
    _sensorFound = true;
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
    _tempsensorFound = _tempSensor.begin(MCP9808_I2C_ADDRESS);
    if (!_tempsensorFound)
    {
      return;
    }
    // Set the temp sensor resolution (higher res takes longer for reading)
    _tempSensor.setResolution(MCP9808_MODE);

    Serial.println(F("MCP9808"));
    _sensorFound = true;
  }
  else
  {
    Serial.println(F("empty"));
  }

  Serial.print(F("[sens] - 0x"));
  Serial.print(OLED_I2C_ADDRESS, HEX);
  Serial.print(F("..."));
  // Check if there is anything responding on this address
  Wire.beginTransmission(OLED_I2C_ADDRESS);
  if (Wire.endTransmission() == 0)
  {
    _oledFound = _display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDRESS);
    if (!_oledFound)
    {
      return;
    }
    _display.clearDisplay();
    _display.display();
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
    _rtcFound = _rtc.begin();
    if (!_rtcFound)
    {
      return;
    }

    if (!_rtc.initialized() || _rtc.lostPower())
    {
      Serial.println(F("[sens] RTC is NOT initialized!"));
      _rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
      Serial.println(F("[sens] RTC Time set with compile time"));
    }
    _rtc.start();
  }
  else
  {
    Serial.println(F("empty"));
  }
}

void OXRS_SENSORS::tele()
{
  if ((millis() - _lastUpdate) > _updateMs)
  {
    StaticJsonDocument<150> json;

    if (_updateMs == 0)
    {
      return;
    }

    if (_tempsensorFound == true)
    {
      if (_tempMode == TEMP_C)
      {
        _temperature = _tempSensor.readTempC();
      }
      if (_tempMode == TEMP_F)
      {
        _temperature = _tempSensor.readTempF();
      }
      if (_temperature != NAN)
      {
        // Publish temp to mqtt
        char payload[8];
        sprintf(payload, "%2.1f", _temperature);
        json["MCP9808-temp"] = payload;
      }
    }

    if (_humsensorFound == true)
    {
      sensors_event_t humidity, temp;
      _sht4.getEvent(&humidity, &temp); // populate temp and humidity objects with fresh data

      _tempShtc = temp.temperature;
      _tempShtf = (temp.temperature * 1.8) + 32;
      _humSht = humidity.relative_humidity;
      char payload[8];
      char payload2[8];
      if (_tempMode == TEMP_C)
      {
        sprintf(payload, "%2.1f", _tempShtc);
      }
      if (_tempMode == TEMP_F)
      {
        sprintf(payload, "%2.1f", _tempShtf);
      }
      // Publish temp to mqtt
      json["SHT40-temp"] = payload;
      sprintf(payload2, "%2.1f", _humSht);
      json["SHT40-hum"] = payload2;
    }

    if (_luxsensorFound == true)
    {
      if (_luxSensor.hasValue() == true) // non blocking reading
      {
        float lux = _luxSensor.getLux();
        _luxSensor.start();
        char payload[8];
        sprintf(payload, "%2.1f", lux);
        json["lux"] = payload;
      }
    }

    char cstr2[1];
    itoa(_sleepEnable, cstr2, 10);
    json["OLEDsleepEnable"] = cstr2;

    char cstr1[1];
    itoa(_sleepState, cstr1, 10);
    json["OLEDsleepState"] = cstr1;

    _sensorMqtt->publishTelemetry(json.as<JsonVariant>());
    // Reset our timer
    _lastUpdate = millis();
  }
}

void OXRS_SENSORS::setConfigSchema(JsonVariant json)
{
  if (_tempsensorFound == false && _humsensorFound == false)
  {
    // do nothing - no temp value available to control
  }
  else
  {
    JsonObject _tempMode = json.createNestedObject("tempMode");
    _tempMode["type"] = "string";
    JsonArray _tempEnum = _tempMode.createNestedArray("enum");
    _tempEnum.add("c");
    _tempEnum.add("f");
    JsonArray _tempEnumNames = _tempMode.createNestedArray("enumNames");
    _tempEnumNames.add("celcius");
    _tempEnumNames.add("farenhite");
  }

  if (_rtcFound == true)
  {
    JsonObject _clockMode = json.createNestedObject("clockMode");
    _clockMode["type"] = "string";
    JsonArray _clockEnum = _clockMode.createNestedArray("enum");
    _clockEnum.add("12");
    _clockEnum.add("24");
  }

  JsonObject _updateMillis = json.createNestedObject("updateMillis");
  _updateMillis["type"] = "integer";
  _updateMillis["minimum"] = 0;

  if (_oledFound == true)
  {
    JsonObject _sleepOledenable = json.createNestedObject("sleepOledenable");
    _sleepOledenable["type"] = "boolean";
  }
}

void OXRS_SENSORS::setCommandSchema(JsonVariant json)
{

  if (_rtcFound == true)
  {
    JsonObject _rtcItems = json.createNestedObject("RTC");
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

  if (_oledFound == true)
  {
    JsonObject _screenMode = json.createNestedObject("screenMode");
    _screenMode["type"] = "string";
    JsonArray _screenEnum = _screenMode.createNestedArray("enum");
    _screenEnum.add("off");
    _screenEnum.add("one");
    if (_rtcFound == false && _tempsensorFound && _humsensorFound == false)
    {
    }
    else
    {
      _screenEnum.add("two");
    }
    if (_luxsensorFound == false && _tempsensorFound == false && _humsensorFound == false)
    {
    }
    else
    {
      _screenEnum.add("three");
    }
    if (_humsensorFound == true)
    {
      _screenEnum.add("four");
    }
    _screenEnum.add("five");
    JsonArray _screenEnumNames = _screenMode.createNestedArray("enumNames");
    _screenEnumNames.add("off");
    _screenEnumNames.add("IP Address & MAC Address");
    if (_rtcFound == false && _tempsensorFound && _humsensorFound == false)
    {
    }
    else
    {
      _screenEnumNames.add("Time & Temperature");
    }
    if (_luxsensorFound == false && _tempsensorFound == false && _humsensorFound == false)
    {
    }
    else
    {
      _screenEnumNames.add("LUX & Temperature");
    }
    if (_humsensorFound == true)
    {
      _screenEnumNames.add("Humidity & Temperature");
    }
    _screenEnumNames.add("2 Lines of Custom Text");

    JsonObject _oneOLED = json.createNestedObject("oneOLED");
    _oneOLED["type"] = "string";
    _oneOLED["maxLength"] = 10;

    JsonObject _twoOLED = json.createNestedObject("twoOLED");
    _twoOLED["type"] = "string";
    _twoOLED["maxLength"] = 10;
  }

  JsonObject _sleeping = json.createNestedObject("sleep");
  _sleeping["type"] = "boolean";
}

void OXRS_SENSORS::conf(JsonVariant json)
{
  if (json.containsKey("screenMode")) // for what mode the OLED is in
  {
    if (strcmp(json["screenMode"], "off") == 0)
    {
      _screenMode = OLED_MODE_OFF;
    }
    if (strcmp(json["screenMode"], "one") == 0)
    {
      _screenMode = OLED_MODE_ONE;
    }
    if (strcmp(json["screenMode"], "two") == 0)
    {
      _screenMode = OLED_MODE_TWO;
    }
    if (strcmp(json["screenMode"], "three") == 0)
    {
      _screenMode = OLED_MODE_THREE;
    }
    if (strcmp(json["screenMode"], "four") == 0)
    {
      _screenMode = OLED_MODE_FOUR;
    }
    if (strcmp(json["screenMode"], "five") == 0)
    {
      _screenMode = OLED_MODE_FIVE;
    }
  }

  if (json.containsKey("clockMode")) // for what mode the tower lights are in
  {
    if (strcmp(json["clockMode"], "12") == 0)
    {
      _clockMode = PCF8523_12;
    }
    else if (strcmp(json["clockMode"], "24") == 0)
    {
      _clockMode = PCF8523_24;
    }
  }

  if (json.containsKey("tempMode")) // for what mode the tower lights are in
  {
    if (strcmp(json["tempMode"], "c") == 0)
    {
      _tempMode = TEMP_C;
    }
    else if (strcmp(json["tempMode"], "f") == 0)
    {
      _tempMode = TEMP_F;
    }
  }

  if (json.containsKey("sleepOledenable"))
  {
    _sleepEnable = json["sleepOledenable"].as<bool>() ? HIGH : LOW;
  }

  if (json.containsKey("updateMillis")) // for updating i2c sensors
  {
    _updateMs = json["updateMillis"].as<uint32_t>();
  }
}

void OXRS_SENSORS::cmnd(JsonVariant json)
{
  if (json.containsKey("RTC"))
  {
    if (_rtcFound == true) // we have an RTC to update
    {
      for (JsonVariant RTC : json["RTC"].as<JsonArray>())
      {
        jsonRtcCommand(RTC);
      }
    }
  }

  if (json.containsKey("screenMode")) // for what mode the OLED is in
  {
    if (strcmp(json["screenMode"], "off") == 0)
    {
      _screenMode = OLED_MODE_OFF;
    }
    if (strcmp(json["screenMode"], "one") == 0)
    {
      _screenMode = OLED_MODE_ONE;
    }
    if (strcmp(json["screenMode"], "two") == 0)
    {
      _screenMode = OLED_MODE_TWO;
    }
    if (strcmp(json["screenMode"], "three") == 0)
    {
      _screenMode = OLED_MODE_THREE;
    }
    if (strcmp(json["screenMode"], "four") == 0)
    {
      _screenMode = OLED_MODE_FOUR;
    }
    if (strcmp(json["screenMode"], "five") == 0)
    {
      _screenMode = OLED_MODE_FIVE;
    }
  }

  if (json.containsKey("oneOLED")) // custom text for OLED
  {
    _screenLineOne = json["oneOLED"].as<String>();
  }

  if (json.containsKey("twoOLED")) // custom text for OLED
  {
    _screenLineTwo = json["twoOLED"].as<String>();
  }

  if (json.containsKey("sleep"))
  {
    if (_sleepEnable == true)
    {
      _sleepState = json["sleep"].as<bool>() ? HIGH : LOW;
    }
  }
}

void OXRS_SENSORS::jsonRtcCommand(JsonVariant json)
{
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
    _rtc.adjust(DateTime(json["year"].as<uint16_t>(), json["month"].as<uint8_t>(), json["day"].as<uint8_t>(), json["hour"].as<uint8_t>(), json["minute"].as<uint8_t>(), json["seconds"].as<uint8_t>()));
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
  _display.clearDisplay();
  _display.display();
}

void OXRS_SENSORS::one_screen() // custom text on line one
{
  _display.setCursor(0, 0);
  _display.print(_screenLineOne);
}

void OXRS_SENSORS::two_screen() // custom text on line two
{
  _display.setCursor(0, 16);
  _display.print(_screenLineTwo);
}

void OXRS_SENSORS::time_screen() // show time on line one
{
  if (_rtcFound == true) // show the time if time is availble
  {
    _display.setCursor(0, 0);
    if (_clockMode == PCF8523_12) // 12 hour clock requested
    {
      if (_currentHour >= 13)
      {
        _modHour = _currentHour - 12;
        if (_modHour < 10)
        {
          _display.print(" ");
        }
        _display.print(_modHour);
      }
      else if (_currentHour == 0)
      {
        _display.print("12");
      }
      else
      {
        if (_currentHour < 10)
        {
          _display.print(" ");
        }
        _display.print(_currentHour);
      }

      _display.print(":");
      if (_currentMinute < 10)
      {
        _display.print("0");
      }
      _display.print(_currentMinute);

      if (_currentHour >= 13)
      {
        _display.print(" PM");
      }
      else
      {
        _display.print(" AM");
      }
    }

    if (_clockMode == PCF8523_24) // 24 hour clock requested
    {
      if (_currentHour < 10)
      {
        _display.print('0');
      }
      _display.print(_currentHour);
      _display.print(":");
      if (_currentMinute < 10)
      {
        _display.print('0');
      }
      _display.print(_currentMinute);
      _display.print("h");
    }
  }
}

void OXRS_SENSORS::temp_screen() // line two showing temp
{
  if (_humsensorFound == true)
  {
    _display.setCursor(0, 16); // line 2
    if (_tempMode == TEMP_C)
    {
      _display.print(_c, 1);
      _display.print("\tC");
    }
    if (_tempMode == TEMP_F)
    {
      _display.print(_f, 1);
      _display.print("\tF");
    }
  }
  if (_tempsensorFound == true && _humsensorFound == false)
  {
    _display.setCursor(0, 16); // line 2
    if (_tempMode == TEMP_C)
    {
      _display.print(_c, 1);
      _display.print("\tC");
    }
    if (_tempMode == TEMP_F)
    {
      _display.print(_f, 1);
      _display.print("\tF");
    }
  }
}

void OXRS_SENSORS::hum_screen()
{
  if (_humsensorFound == true)
  {
    _display.setCursor(0, 0); // line 1
    _display.print(_h, 1);
    _display.print("% rH");
  }
}

void OXRS_SENSORS::lux_screen() // line one showing lux
{
  if (_luxsensorFound == true)
  {
    _display.setCursor(0, 0); // line 1
    _display.print(_l, 1);
    _display.print(" LUX");
  }
}

void OXRS_SENSORS::IP_screen() // line one showing IP address
{
  _display.setTextSize(1);
  _display.setCursor(0, 0); // line 1
  _display.print(_ipAddress);
}

void OXRS_SENSORS::MAC_screen() // line two showing mac address
{
  _display.setTextSize(1);
  char mac_display[18];
  sprintf_P(mac_display, PSTR("%02X:%02X:%02X:%02X:%02X:%02X"), _mac[0], _mac[1], _mac[2], _mac[3], _mac[4], _mac[5]);
  _display.setCursor(0, 16); // line 2
  _display.print(mac_display);
}

void OXRS_SENSORS::oled() // control the OLED screen if availble
{
  if (_oledFound == true)
  {
    oledUpdate();
  }
}

void OXRS_SENSORS::oled(byte *mac) // control the OLED screen if availble
{
  memcpy(_mac, mac, 6);

  if (_oledFound == true)
  {
    oledUpdate();
  }
}

void OXRS_SENSORS::oled(IPAddress ip) // control the OLED screen if availble
{
  _ipAddress = ip;
  if (_oledFound == true)
  {
    oledUpdate();
  }
}

void OXRS_SENSORS::oledUpdate() // control the OLED screen if availble
{
  if ((millis() - _previousOledtemp) > OLED_INTERVAL_TEMP)
  {
    if (_humsensorFound == true)
    {
      sensors_event_t humidity, temp;
      _sht4.getEvent(&humidity, &temp); // populate temp and humidity objects with fresh data

      _c = temp.temperature;
      _f = (temp.temperature * 1.8) + 32;
      _h = humidity.relative_humidity;
    }
    if (_tempsensorFound == true && _humsensorFound == false)
    {
      _c = _tempSensor.readTempC();
      _f = _tempSensor.readTempF();
    }
    // Reset our timer
    _previousOledtemp = millis();
  }

  if ((millis() - _previousOledlux) > OLED_INTERVAL_LUX)
  {
    if (_luxsensorFound == true && _luxSensor.hasValue() == true)
    {
      _l = _luxSensor.getLux();
      _luxSensor.start(); // start the next reading
      _previousOledlux = millis();
    }
  }

  if (_rtcFound == true)
  {
    // Time Keeping
    DateTime now = _rtc.now();
    _currentHour = now.hour();
    _currentMinute = now.minute();
  }

  if (_sleepState == true) // screen goes to sleep
  {
    off_screen();
  }
  else
  {
    _display.clearDisplay();
    _display.setTextSize(2);
    _display.setTextColor(WHITE);

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

    _display.display();
  }
}