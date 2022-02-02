
/*--------------------------- Version ------------------------------------*/
#define FW_NAME "OXRS-AC-SensorLibrary-ESP32-FW"
#define FW_SHORT_NAME "Sensor Library"
#define FW_MAKER "Austin's Creations"
#define FW_VERSION "0.0.4"

/*--------------------------- Libraries ----------------------------------*/
#include <SPI.h>          // for ethernet
#include <Ethernet.h>     // for ethernet
#include <WiFi.h>         // For networking
#include <PubSubClient.h> // For MQTT
#include <OXRS_MQTT.h>    // For MQTT
#include <OXRS_API.h>     // For REST API
#include <OXRS_SENSORS.h> // for Qwiic i2c sensors

/*--------------------------- Configuration ------------------------------*/
// Should be no user configuration in this file, everything should be in;
#include "config.h"

/*--------------------------- Constants ----------------------------------*/
// Serial
#define SERIAL_BAUD_RATE 115200

// REST API
#define REST_API_PORT 80

// MQTT
#define MQTT_MAX_MESSAGE_SIZE 4096

// REST API
#define REST_API_PORT 80

/*--------------------------- Global Variables ---------------------------*/

/*--------------------------- Instantiate Global Objects -----------------*/
// client
EthernetClient client;
// WiFiClient client;

// MQTT client
PubSubClient mqttClient(client);
OXRS_MQTT mqtt(mqttClient);

// REST API
EthernetServer server(REST_API_PORT);
// WiFiServer server(REST_API_PORT);
OXRS_API api(mqtt);

OXRS_SENSORS sensor(mqtt);

/*--------------------------- Program ------------------------------------*/
void mqttCallback(char *topic, uint8_t *payload, unsigned int length)
{
  // Pass this message down to our MQTT handler
  mqtt.receive(topic, payload, length);
}

/**
  Setup
*/
void setup()
{
  Ethernet.init(15);
  // Startup logging to serial
  Serial.begin(SERIAL_BAUD_RATE);
  Serial.println("/n");
  Serial.println(F("========================================"));
  Serial.print(F("FIRMWARE: "));
  Serial.println(FW_NAME);
  Serial.print(F("MAKER:    "));
  Serial.println(FW_MAKER);
  Serial.print(F("VERSION:  "));
  Serial.println(FW_VERSION);
  Serial.println(F("========================================"));

  // starts up the i2c line and starts scanning / setting up sensors
  Wire.begin();
  // Wire.Begin(33,32);

  sensor.begin(); // start sensors

  byte mac[6];

  sensor.oled(WiFi.macAddress(mac)); // start screen - starts with MAC address showing

  initialiseEthernet(mac);
  //  initialiseWifi(mac);

  sensor.oled(Ethernet.localIP()); // update screen - should show IP address
  // sensor.oled(WiFi.localIP()); // update screen - should show IP address

  initialiseMqtt(mac);

  initialiseRestApi();
}

void loop()
{
  ethernet_loop();
  mqtt.loop();

  EthernetClient client = server.available();
  api.checkEthernet(&client);
  //  WiFiClient client = server.available();
  //  api.checkWifi(&client);

  sensor.oled(); // update screen (sleep state)
  sensor.tele(); // updates the tele sensor data
}

void ethernet_loop()
{
  switch (Ethernet.maintain())
  {
  case 1:
    // renewed fail
    Serial.println("Error: renewed fail");
    break;

  case 2:
    // renewed success
    Serial.println("Renewed success");
    // print your local IP address:
    Serial.print("My IP address: ");
    Serial.println(Ethernet.localIP());
    break;

  case 3:
    // rebind fail
    Serial.println("Error: rebind fail");
    break;

  case 4:
    // rebind success
    Serial.println("Rebind success");
    // print your local IP address:
    Serial.print("My IP address: ");
    Serial.println(Ethernet.localIP());
    break;

  default:
    // nothing happened
    break;
  }
}

/**
  MQTT
*/
void getFirmwareJson(JsonVariant json)
{
  JsonObject firmware = json.createNestedObject("firmware");

  firmware["name"] = FW_NAME;
  firmware["shortName"] = FW_SHORT_NAME;
  firmware["maker"] = FW_MAKER;
  firmware["version"] = FW_VERSION;
}

void getNetworkJson(JsonVariant json)
{
  byte mac[6];
  WiFi.macAddress(mac);

  char mac_display[18];
  sprintf_P(mac_display, PSTR("%02X:%02X:%02X:%02X:%02X:%02X"), mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  JsonObject network = json.createNestedObject("network");

  network["ip"] = WiFi.localIP();
  network["mac"] = mac_display;
}

void getConfigSchemaJson(JsonVariant json)
{
  JsonObject configSchema = json.createNestedObject("configSchema");

  // Config schema metadata
  configSchema["$schema"] = "http://json-schema.org/draft-07/schema#";
  configSchema["title"] = FW_NAME;
  configSchema["type"] = "object";

  JsonObject properties = configSchema.createNestedObject("properties");

  sensor.setConfigSchema(properties);
}

void getCommandSchemaJson(JsonVariant json)
{
  JsonObject commandSchema = json.createNestedObject("commandSchema");

  // Command schema metadata
  commandSchema["$schema"] = "http://json-schema.org/draft-07/schema#";
  commandSchema["title"] = FW_NAME;
  commandSchema["type"] = "object";

  JsonObject properties = configSchema.createNestedObject("properties");

  sensor.setConfigSchema(properties);
}

/**
  API callbacks
*/
void apiAdopt(JsonVariant json)
{
  // Build device adoption info
  getFirmwareJson(json);
  getNetworkJson(json);
  getConfigSchemaJson(json);
  getCommandSchemaJson(json);
}

/**
  MQTT callbacks
*/
void mqttConnected()
{
  // Build device adoption info
  DynamicJsonDocument json(JSON_ADOPT_MAX_SIZE);
  mqtt.publishAdopt(api.getAdopt(json.as<JsonVariant>()));

  // Log the fact we are now connected
  Serial.println("mqtt connected");
}

void mqttDisconnected(int state)
{
  // Log the disconnect reason
  // See https://github.com/knolleary/pubsubclient/blob/2d228f2f862a95846c65a8518c79f48dfc8f188c/src/PubSubClient.h#L44
  switch (state)
  {
  case MQTT_CONNECTION_TIMEOUT:
    Serial.println(F("mqtt connection timeout"));
    break;
  case MQTT_CONNECTION_LOST:
    Serial.println(F("mqtt connection lost"));
    break;
  case MQTT_CONNECT_FAILED:
    Serial.println(F("mqtt connect failed"));
    break;
  case MQTT_DISCONNECTED:
    Serial.println(F("mqtt disconnected"));
    break;
  case MQTT_CONNECT_BAD_PROTOCOL:
    Serial.println(F("mqtt bad protocol"));
    break;
  case MQTT_CONNECT_BAD_CLIENT_ID:
    Serial.println(F("mqtt bad client id"));
    break;
  case MQTT_CONNECT_UNAVAILABLE:
    Serial.println(F("mqtt unavailable"));
    break;
  case MQTT_CONNECT_BAD_CREDENTIALS:
    Serial.println(F("mqtt bad credentials"));
    break;
  case MQTT_CONNECT_UNAUTHORIZED:
    Serial.println(F("mqtt unauthorised"));
    break;
  }
}

void jsonConfig(JsonVariant json) // config payload
{
  sensor.conf(json); // check if we have new config
}

void jsonCommand(JsonVariant json) // do something payloads
{
  sensor.cmnd(json); // check if we have new command

  if (json.containsKey("restart") && json["restart"].as<bool>())
  {
    ESP.restart();
  }
}

/*
 WIFI
 */
void initialiseWifi(byte *mac)
{
  // Determine MAC address
  WiFi.macAddress(mac);

  // Display MAC address on serial
  char mac_display[18];
  sprintf_P(mac_display, PSTR("%02X:%02X:%02X:%02X:%02X:%02X"), mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  Serial.print(F("mac address: "));
  Serial.println(mac_display);

  // Connect to WiFi
  Serial.print(F("connecting to "));
  Serial.print(WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(F("."));
  }
  Serial.println();

  Serial.print(F("ip address: "));
  Serial.println(WiFi.localIP());
}

void initialiseEthernet(byte *mac)
{
  // Determine MAC address
  WiFi.macAddress(mac);

  // Display MAC address on serial
  char mac_display[18];
  sprintf_P(mac_display, PSTR("%02X:%02X:%02X:%02X:%02X:%02X"), mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  Serial.print(F("mac address: "));
  Serial.println(mac_display);

  Serial.println("Initialize Ethernet with DHCP:");
  if (Ethernet.begin(mac) == 0)
  {
    Serial.println("Failed to configure Ethernet using DHCP");
    if (Ethernet.hardwareStatus() == EthernetNoHardware)
    {
      Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    }
    else if (Ethernet.linkStatus() == LinkOFF)
    {
      Serial.println("Ethernet cable is not connected.");
    }
    // no point in carrying on, so do nothing forevermore:
    while (true)
    {
      delay(1);
    }
  }
  Serial.println();
  // print your local IP address:
  Serial.print(F("IP address: "));
  Serial.println(Ethernet.localIP());
}

/*
 MQTT
 */
void initialiseMqtt(byte *mac)
{
  // Set the default client id to the last 3 bytes of the MAC address
  char clientId[32];
  sprintf_P(clientId, PSTR("%02x%02x%02x"), mac[3], mac[4], mac[5]);
  mqtt.setClientId(clientId);

  // Register our callbacks
  mqtt.onConnected(mqttConnected);
  mqtt.onDisconnected(mqttDisconnected);
  mqtt.onConfig(jsonConfig);
  mqtt.onCommand(jsonCommand);

  // Start listening for MQTT messages
  mqttClient.setCallback(mqttCallback);
}

void initialiseRestApi(void)
{
  // NOTE: this must be called *after* initialising MQTT since that sets
  //       the default client id, which has lower precendence than MQTT
  //       settings stored in file and loaded by the API

  // Set up the REST API
  api.begin();

  // Register our callbacks
  api.onAdopt(apiAdopt);

  server.begin();
}
