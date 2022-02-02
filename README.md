# OXRS-AC-IO-SENSORS-ESP-LIB
Library for interfacing QWIIC I2C sensors into OXRS firmware devices

Currently supports:
128x32 OLED, PCF8523 RTC, SHT40 hum&temp, MCP9808 temp, BH1750 LUX (with default i2c addresses)

Dependencies:
``` c++
#include <Arduino.h>
#include <OXRS_MQTT.h>
#include <ArduinoJson.h>
#include <Wire.h>                     // For I2C
#include <Adafruit_MCP9808.h>         // For temp sensor
#include <Adafruit_SSD1306.h>         // For OLED display
#include <RTClib.h>                   // For PCF8523 RTC
#include <BH1750.h>                   // For BH1750 lux sensor
#include <Adafruit_SHT4x.h>           // For SHT40 Temp / humidity sensor

#if defined(ESP32)
  #include <WiFi.h>
  #include <ETH.h>
  #include <Ethernet.h> 
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <Ethernet.h>
#endif
```

See the examples folder for example setups for esp8266, esp32, and LilyGO POE Ethernet.
The esp8266 and esp32 examples have the code needed to run as is and with commenting / uncommenting lines of code can do ethernet or wifi usage.

The core of OXRS_SENSORS:
```c++
#include <OXRS_SENSORS.h>             // for Qwiic i2c sensors (this is the library)

// initialize the library instance
// the only variable that may change is mqtt should you change the OXRS_MQTT initializer
OXRS_SENSORS sensors(mqtt);      

Wire.begin(); // needs to be called before starting the sensors
Wire.begin(SDA,SCL); // for instanaces where different I2C pins are used
// 33,32 are used on LilyGO sheild
// 4,0   are used on D1 Mini PWM

// starts scanning / setting up sensors
sensors.begin();

// for updating the screen
// this is usually called before starting Ethernet or wifi - will start by showing mac address
// called again after initializing internet connection will then update with IP address
// then it should be call within your main loop to ensure it remains updated.
sensors.oled();                      // to update OLED
sensors.oled(WiFi.macAddress(mac));  // to update OLED MAC address display
sensors.oled(Ethernet.localIP());    // to update IP address on the display (wifi or ethernet)

// for updating the sensors and sending their data via mqtt on the TELE topic
// placed in main program loop
sensors.tele();

// adds config schema for sensors library to the schema payload
// placed after JsonObject properties = configSchema.createNestedObject("properties");
// your device specific schema can be placed after
sensors.setConfigSchema(properties);

// adds command schema for sensors library to the schema payload
// placed after JsonObject properties = commandSchema.createNestedObject("properties");
// your device specific schema can be placed after
sensors.setCommandSchema(properties);

// checks and updates an config options
// placed within void jsonConfig(JsonVariant json) of your main sketch
sensors.conf(json);

// checks and updates an command options
// placed within void jsonCommand(JsonVariant json) of your main sketch
sensors.cmnd(json);
```
