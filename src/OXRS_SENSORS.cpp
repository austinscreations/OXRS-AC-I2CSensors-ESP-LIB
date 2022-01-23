// cpp file v0.2.0

#include "OXRS_SENSORS.h"

// Pointer to the MQTT lib so we can get/set config
// OXRS_MQTT * _sensorMqtt;

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

#if defined(ESP32)
OXRS_SENSORS::OXRS_SENSORS(WiFiClass &wifi, OXRS_MQTT &mqtt)
{
  _wifi32 = &wifi;
  _ethernet = NULL;
  _eth = NULL;
  _sensorMqtt = &mqtt;
}
OXRS_SENSORS::OXRS_SENSORS(WiFiClass &wifi, EthernetClass &ethernet, OXRS_MQTT &mqtt)
{
  _wifi32 = &wifi;
  _ethernet = &ethernet;
  _eth = NULL;
  _sensorMqtt = &mqtt;
}
OXRS_SENSORS::OXRS_SENSORS(WiFiClass &wifi, ETHClass &eth, OXRS_MQTT &mqtt)
{
  _wifi32 = &wifi;
  _ethernet = NULL;
  _eth = &eth;
  _sensorMqtt = &mqtt;
}
#elif defined(ESP8266)
OXRS_SENSORS::OXRS_SENSORS(ESP8266WiFiClass &wifi, OXRS_MQTT &mqtt)
{
  _wifi8266 = &wifi;
  _ethernet = NULL;
  _sensorMqtt = &mqtt;
}
OXRS_SENSORS::OXRS_SENSORS(ESP8266WiFiClass &wifi, EthernetClass &ethernet, OXRS_MQTT &mqtt)
{
  _wifi8266 = &wifi;
  _ethernet = &ethernet;
  _sensorMqtt = &mqtt;
}
#endif

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
  if (json.containsKey("year")) // set RTC time
  {
    if (_rtcFound == true) // we have an RTC to update
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

#if defined(ESP32)
  if (_ethernet && _wifi32)
  {
    _display.print(_ethernet->localIP());
  }
  else if (_eth && _wifi32)
  {
    _display.print(_eth->localIP());
  }
  else
  {
    _display.print(_wifi32->localIP());
  }
#elif defined(ESP8266)
  if (_ethernet && _wifi8266)
  {
    _display.print(_ethernet->localIP());
  }
  else
  {
    _display.print(_wifi8266->localIP());
  }
#endif
}

void OXRS_SENSORS::MAC_screen() // line two showing mac address
{
  _display.setTextSize(1);
  byte mac[6];
#if defined(ESP32)
  _wifi32->macAddress(mac);
#elif defined(ESP8266)
  _wifi8266->macAddress(mac);
#endif
  //  WiFi.macAddress(mac);
  char mac_display[18];
  sprintf_P(mac_display, PSTR("%02X:%02X:%02X:%02X:%02X:%02X"), mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  _display.setCursor(0, 16); // line 2
  _display.print(mac_display);
}

void OXRS_SENSORS::oled() // control the OLED screen if availble
{
  if (_oledFound == true)
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
}
