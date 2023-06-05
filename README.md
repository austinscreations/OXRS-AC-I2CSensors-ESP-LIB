# OXRS-AC-SENSORS-ESP-LIB
Library for interfacing QWIIC I2C sensors into OXRS firmware devices

Currently supports:
SHT40 hum&temp, MCP9808 temp, BH1750 LUX (with default i2c addresses)

Dependencies:
``` c++
#include <Arduino.h>
#include <ArduinoJson.h>
#include <Wire.h>                     // For I2C
#include <Adafruit_MCP9808.h>         // For temp sensor
#include <BH1750.h>                   // For BH1750 lux sensor - non-adafruit library: https://github.com/claws/BH1750
#include <Adafruit_SHT4x.h>           // For SHT40 Temp / humidity sensor
```

See the examples folder for example setups for esp8266, esp32, and LilyGO POE Ethernet.
The esp8266 and esp32 examples have the code needed to run as is and with commenting / uncommenting lines of code can do ethernet or wifi usage.

The core of OXRS_SENSORS:
```c++
#include <OXRS_SENSORS.h>             // for Qwiic i2c sensors (this is the library)

// initialize the library instance
OXRS_SENSORS sensors;      

Wire.begin(); // needs to be called before starting the sensors
Wire.begin(SDA,SCL); // for instanaces where different I2C pins are used
// 33,32 are used on LilyGO sheild
// 4,0   are used on D1 Mini PWM

// starts scanning / setting up sensors
sensors.begin();

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

// publish sensor telemetry (if any)
// placed in the main program loop
StaticJsonDocument<150> telemetry;
sensors.tele(telemetry.as<JsonVariant>());

if (telemetry.size() > 0)
{
  oxrs.publishTelemetry(telemetry.as<JsonVariant>());
}
```
