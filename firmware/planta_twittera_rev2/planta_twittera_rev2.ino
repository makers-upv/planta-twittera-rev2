/***************************************************
  Planta Twittera 2r0
  by Jaime Laborda
  MakersUPV
 ****************************************************/

// Adafruit IO
#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME    "aio_username"
#define AIO_KEY         "aio_key"  // Obtained from account info on io.adafruit.com

// DHT 11 sensor
#define DHTPIN 14 //D5
#define DHTTYPE DHT11

// maximum number of seconds between resets that
// counts as a double reset
#define DRD_TIMEOUT 2.0
#define DRD_ADDRESS 0x00

//Sleep interval
#define SLEEP_INTERVAL "1" //1 minute

//Calibration soil sensor
#define SOIL_HUM_100 100
#define SOIL_HUM_0 430

// Libraries
#include <FS.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include "DHT.h"
#include <Wire.h>
#include <BH1750.h>

//for LED status
#include <Ticker.h>
Ticker ticker;

//Double Reset Detector
#include <DoubleResetDetect.h>
DoubleResetDetect drd(DRD_TIMEOUT, DRD_ADDRESS);



bool shouldSaveConfig = false;


void tick()
{
  //toggle state
  bool state = digitalRead(BUILTIN_LED);  // get the current state of GPIO1 pin
  digitalWrite(BUILTIN_LED, !state);     // set pin to the opposite state
}

//gets called when WiFiManager enters configuration mode
void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
  //entered config mode, make led toggle faster
  ticker.attach(0.1, tick);
}

// DHT sensor
DHT dht(DHTPIN, DHTTYPE, 15);

//Lux sensor
BH1750 lightMeter;



/*************************** Sketch Code ************************************/

void setup() {

  Serial.begin(115200);
  //set led pin as output

  pinMode(BUILTIN_LED, OUTPUT);

  // Init sensor
  dht.begin();

  //Lux sensor
  Wire.begin();
  lightMeter.begin(BH1750::ONE_TIME_HIGH_RES_MODE);

  //set led pin as output
  pinMode(BUILTIN_LED, OUTPUT);
  pinMode(0, INPUT);
  // start ticker with 0.5 because we start in AP mode and try to connect
  ticker.attach(0.5, tick);

  //define your default values here, if there are different values in config.json, they are overwritten.
  char* aio_username = AIO_USERNAME;
  char* aio_key = AIO_KEY;
  char* sleep_interval = SLEEP_INTERVAL; //minutes

  //read configuration from FS json
  Serial.println("mounting FS...");

  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
          Serial.println("\nparsed json");
          strcpy(aio_username, json["param_aio_username"]);
          strcpy(aio_key, json["param_aio_key"]);
          strcpy(sleep_interval, json["sleep_interval"]); //minutes

          Serial.print("Loaded aio_username: ");
          Serial.println(aio_username);
          Serial.print("Loaded aio_key: ");
          Serial.println(aio_key);
          Serial.print("Loaded sleep_interval: ");
          Serial.println(sleep_interval);

        } else {
          Serial.println("failed to load json config");
        }
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }

  WiFiManager wifiManager;
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  //WifiManager & custom parameters
  WiFiManagerParameter custom_aio_username("aio_username", "AIO USERNAME", aio_username, 40);
  WiFiManagerParameter custom_aio_key("aio_key", "AIO KEY", aio_key, 40);
  WiFiManagerParameter custom_sleep_interval("sleep_interval", "INTERVAL", sleep_interval, 40);

  wifiManager.addParameter(&custom_aio_username);
  wifiManager.addParameter(&custom_aio_key);
  wifiManager.addParameter(&custom_sleep_interval);


  if (drd.detect()) {
    Serial.println("** Double reset boot **");
    wifiManager.resetSettings();
  }

  //wifiManager.resetSettings();
  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wifiManager.setAPCallback(configModeCallback);

  if (!wifiManager.autoConnect("Planta Twittera")) {
    Serial.println("failed to connect and hit timeout");
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(1000);
  }
  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");
  ticker.detach();
  //keep LED on
  digitalWrite(BUILTIN_LED, LOW);

  strcpy(aio_username, custom_aio_username.getValue());
  strcpy(aio_key, custom_aio_key.getValue());
  strcpy(sleep_interval, custom_sleep_interval.getValue());

  if (shouldSaveConfig) {
    shouldSaveConfig = false;

    //save the custom parameters to FS
    Serial.println("saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();

    json["param_aio_username"] = aio_username;
    json["param_aio_key"] = aio_key;
    json["sleep_interval"] = sleep_interval;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }

    json.printTo(Serial);
    json.printTo(configFile);
    configFile.close();
    //end save
    Serial.println("FS Config Updated");
  }

  // Create an ESP8266 WiFiClient class to connect to the MQTT server.
  WiFiClient client;

  String str_aio_username = String(AIO_USERNAME);
  String str_aio_key = String(AIO_KEY);

  // Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
  Serial.println("ParametersClient: " + String(aio_username) + " " + String(aio_key));
  Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, str_aio_username.c_str(), str_aio_key.c_str());

  // Parameters feed configuration
  String str_temperature = str_aio_username + "/feeds/temperature";
  String str_humidity = str_aio_username + "/feeds/humidity";
  String str_soil_humidity = str_aio_username + "/feeds/soil_humidity";
  String str_luminosity = str_aio_username + "/feeds/luminosity";

  Adafruit_MQTT_Publish temperature = Adafruit_MQTT_Publish(&mqtt, str_temperature.c_str());
  Adafruit_MQTT_Publish humidity = Adafruit_MQTT_Publish(&mqtt, str_humidity.c_str());
  Adafruit_MQTT_Publish soil_humidity = Adafruit_MQTT_Publish(&mqtt, str_soil_humidity.c_str());
  Adafruit_MQTT_Publish luminosity = Adafruit_MQTT_Publish(&mqtt, str_luminosity.c_str());

  // connect to adafruit io

  Serial.print(F("Connecting to Adafruit IO... "));

  int8_t ret;

  while ((ret = mqtt.connect()) != 0) {

    switch (ret) {
      case 1: Serial.println(F("Wrong protocol")); break;
      case 2: Serial.println(F("ID rejected")); break;
      case 3: Serial.println(F("Server unavail")); break;
      case 4: Serial.println(F("Bad user/pass")); break;
      case 5: Serial.println(F("Not authed")); break;
      case 6: Serial.println(F("Failed to subscribe")); break;
      default: Serial.println(F("Connection failed")); break;
    }

    if (ret >= 0)
      mqtt.disconnect();

    Serial.println(F("Retrying connection..."));
    delay(5000);

  }

  Serial.println(F("Adafruit IO Connected!"));

  //Aqui hago cosas

  // Grab the current state of the sensor
  int humidity_data = (int)dht.readHumidity();
  int temperature_data = (int)dht.readTemperature();

  // By default, the temperature report is in Celsius, for Fahrenheit uncomment
  //    following line.
  // temperature_data = temperature_data*(9.0/5.0) + 32.0;

  // Read Soil Humidity
  int soil_humidity_data = (int)readSoilHumidity();

  // Lux sensor
  uint16_t luminosity_data = lightMeter.readLightLevel();

  // Publish data
  if (! temperature.publish(temperature_data))
    Serial.println(F("Failed to publish temperature"));
  else
    Serial.println(F("Temperature published!"));

  if (! humidity.publish(humidity_data))
    Serial.println(F("Failed to publish humidity"));
  else
    Serial.println(F("Humidity published!"));

  if (! soil_humidity.publish(soil_humidity_data))
    Serial.println(F("Failed to publish soil humidity"));
  else
    Serial.println(F("Soil Humidity published!"));

  if (! luminosity.publish(luminosity_data))
    Serial.println(F("Failed to publish luminosity"));
  else
    Serial.println(F("Luminosity published!"));

  delay(2000);

  //Deep sleep to save power
  ESP.deepSleep(atoi(sleep_interval) * 60e6); //in seconds
}

void loop() {
  //Do nothing in the loop
}

float readSoilHumidity () {
  return map(analogRead(0), SOIL_HUM_0, SOIL_HUM_100 , 0, 100);
}

void saveConfigCallback () {
  shouldSaveConfig = true;
  //ticker.attach(0.5, tick);
}

