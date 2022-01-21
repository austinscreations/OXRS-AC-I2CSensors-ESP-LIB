# OXRS-AC-IO-SENSORS-ESP-LIB
library for interfacing qwiic i2c sensors into OXRS firmware devices

currently supports:
128x32 OLED, PCF8523 RTC, SHT40 hum&temp, MCP9808 temp, BH1750 LUX (with default i2c addresses)

needed addon libraries:
``` c++
#include <Arduino.h>
#include <OXRS_MQTT.h>
#include <ArduinoJson.h>
#include <Wire.h>                     // For I2C
#include <Adafruit_MCP9808.h>         // For temp sensor
#include <Adafruit_SSD1306.h>         // for OLED display
#include <RTClib.h>                   // for PCF8523 RTC
#include <hp_BH1750.h>                // for bh1750 lux sensor
#include <Adafruit_SHT4x.h>           // for SHT40 Temp / humidity sensor

#if defined(ESP32)
    #include <WiFi.h>
    #include <ETH.h>
    #include <Ethernet.h> 
  #elif defined(ESP8266)
    #include <ESP8266WiFi.h>
    #include <Ethernet.h>
#endif
```

see the examples folder for example setups for esp8266, esp32, and LilyGO POE Ethernet
the esp8266 and esp32 examples have the code needed to run as is and with commenting / uncommenting lines of code can do ethernet or wifi usage

the core of the OXRS_SENSORS:
```c++
#include <OXRS_SENSORS.h>             // for Qwiic i2c sensors (this is the library)

// initialize the library instance
// the only variable that may change is mqtt should you change the OXRS_MQTT initializer
OXRS_SENSORS sensor(WiFi, ETH, mqtt);      // use this initializer when using ethernet on LilyGO POE
OXRS_SENSORS sensor(WiFi, Ethernet, mqtt); // use this initializer when using ethernet
OXRS_SENSORS sensor(WiFi, mqtt);           // use this initializer when using wifi

// starts up the i2c line and starts scanning / setting up sensors
sensor.begin();      // standard i2c GPIO
sensor.begin(33,32); // LILYGO POE PWM Sheild - SDA / SCL GPIO values
sensor.begin(4,0);   // D1 Mini PWM module    - SDA / SCL GPIO values

// for updating the screen
// this is usually called before starting Ethernet or wifi - will start by showing mac address
// called again after initializing internet connection will then update with IP address
// then it should be call within your main loop to ensure it remains updated.
sensor.oled(); 

// for updating the sensors and sending their data via mqtt on the TELE topic
// placed in main program loop
sensor.tele(); // updates the tele sensor data

// checks and updates an config options
// placed within void jsonConfig(JsonVariant json) of your main sketch
sensor.conf(json); // check if we have new config

// checks and updates an command options
// placed within void jsonCommand(JsonVariant json) of your main sketch
sensor.cmnd(json); // check if we have new command

```
