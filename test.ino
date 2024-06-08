#include "secrets.h"
#include <WiFiClientSecure.h>
#include <MQTTClient.h>
#include <ArduinoJson.h>
#include "WiFi.h"

//dependecies for dht11
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

#define DHTPIN 14
#define DHTTYPE DHT11

// The MQTT topics that this device should publish/subscribe
#define AWS_IOT_PUBLISH_TOPIC   "esp32/motion_data" 

DHT_Unified dht(DHTPIN, DHTTYPE);

//variables to send over the cloud
float humidity = 0.0;
float temperature = 0.0;

//delay between each reading
uint32_t delayMS;

WiFiClientSecure net = WiFiClientSecure();
MQTTClient client = MQTTClient(256);


////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
//                            connect to Wifi function                            //
//           connect to wifi using SSID and password defined in secrets.h         //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////
void connectWifi()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to Wi-Fi");

  while (WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to Wi-Fi");
}


////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
//                            connect to AWS function                             //
//           connect to AWS using secret key defined in secrets.h                 //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////

void connectAWS()
{
  // Configure WiFiClientSecure to use the AWS IoT device credentials
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);

  // Connect to the MQTT broker on the AWS endpoint we defined earlier
  client.begin(AWS_IOT_ENDPOINT, 8883, net);
  Serial.println("Connecting to AWS IOT");
  while (!client.connect(THINGNAME)) {
    Serial.print(".");
    delay(100);
  }

  if(!client.connected()){
    Serial.println("AWS IoT Timeout!");
    return;
  }
  Serial.println("Connected to AWS IOT");
}


////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
//                            publish message function                            //
//           construct a json file using the sensor readings, and                 //
//                           publishing it to aws topic                           //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////

void publishMessage()
{
  StaticJsonDocument<200> doc;
  doc["humidity"] = humidity;
  doc["temperature"] = temperature;
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer); // print to client

  client.publish(AWS_IOT_PUBLISH_TOPIC, jsonBuffer);
}


////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
//                                setup function                                  //
//                     sets up DHT11, connect to wifi and aws                     //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////

void setup() {

  Serial.begin(115200);
  
  // Initialize device.
  dht.begin();
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
  Serial.println(F("------------------------------------"));
  Serial.println(F("Temperature Sensor"));
  Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
  Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
  Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
  Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("°C"));
  Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("°C"));
  Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("°C"));
  Serial.println(F("------------------------------------"));
  // Print humidity sensor details.
  dht.humidity().getSensor(&sensor);
  Serial.println(F("Humidity Sensor"));
  Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
  Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
  Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
  Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("%"));
  Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("%"));
  Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("%"));
  Serial.println(F("------------------------------------"));
  // Set delay between sensor readings based on sensor details.
  delayMS = sensor.min_delay / 1000;

  connectWifi();
  connectAWS();
  
}

////////////////////////////////////////////////////////////////////////////////////
//                                                                                //
//                                loop function                                   //
//                     take temperature and humidity reading, and                 //
//                            update their variables                              //
//                                                                                //
////////////////////////////////////////////////////////////////////////////////////

void loop() {

  // Get new sensor events with the readings //

  // Delay between measurements.
  delay(delayMS);
  // Get temperature event and print its value.
  sensors_event_t event;
  dht.temperature().getEvent(&event);
  if (isnan(event.temperature)) {
    Serial.println(F("Error reading temperature!"));
  }
  else {
    Serial.print(F("Temperature: "));
    Serial.print(event.temperature);
    Serial.println(F("°C"));
    temperature = event.temperature;
  }
  // Get humidity event and print its value.
  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity)) {
    Serial.println(F("Error reading humidity!"));
  }
  else {
    Serial.print(F("Humidity: "));
    Serial.print(event.relative_humidity);
    Serial.println(F("%"));
    humidity = event.relative_humidity;
  }

  if (WiFi.status() != WL_CONNECTED){
    connectWifi();
  }

  if(!client.connected()){
    Serial.print("AWS IoT Timeout!");
    connectAWS();
  }

  publishMessage();
  client.loop();
  delay(1000);
}

