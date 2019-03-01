#include "config.h"

#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <Time.h>
#include <Servo.h>

// ----------------------------------------------
// Local WiFi credentials
const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;

// Cloud MQTT broker credentials
const char* mqttServer = MQTT_SERVER;
const int   mqttPort = MQTT_PORT;
const char* mqttUser = MQTT_USERNAME;
const char* mqttPassword = MQTT_PASSWORD;

// Topic
const char* topic = "/devices/rollet/RT000001-CUBE/update";

// ----------------------------------------------

String deviceName = "Rollet1";
String jsonObject = "";
bool connectionFlag = false;

// ----------------------------------------------
WiFiClient wifiClient;
Servo servo;

void callback(char* topic, byte* payload, unsigned int length) {

  for (int i = 0; i < length; i++) {
    jsonObject.concat((char) payload[i]);
  }

  // Create JSON object
  StaticJsonBuffer<200> jsonBuffer;

  // Get root of JSON object
  JsonObject& root = jsonBuffer.parseObject(jsonObject);

  // There are 3 servo states:
  // - up
  // - down
  // - stop
  // There are 2 states of power:
  // - on
  // - off

  if(root["state"].as<String>() == "off") {
    ServoOff();
  }
  
  if(root["state"].as<String>() == "on") {
    ServoOn();
    
    if(root["action"].as<String>() == "stop") {
      ServoStop();  
    }
    if(root["action"].as<String>() == "up") {
      ServoUp();
    }
    if(root["action"].as<String>() == "down") {
      ServoDown();
    }  
  }
  
   // Clear JSON object
  jsonObject = "";
}

void ServoOn() {
  digitalWrite(0, HIGH);
  Serial.println ("servo on");
  //  responseMessage("msg1", "msg2");
}

void ServoOff() {
  digitalWrite(0, LOW);
  Serial.println ("servo off");
  //  responseMessage("msg1", "msg2");
}

void ServoStop(){
    digitalWrite(2, LOW);
  
    servo.write(85);  //default(90) , stop
    servo.detach();

    Serial.println ("servo stop");
    //  responseMessage("msg1", "msg2");
}

void ServoDown(){
    servo.attach(2);
    servo.write(0);    //default(0) , left
    digitalWrite(2, HIGH);

    Serial.println ("servo down");
}

void ServoUp(){
    servo.attach(2);
    servo.write(180);  // default (180) , right

    digitalWrite(2, HIGH);
    Serial.println ("servo up");
}

PubSubClient client(mqttServer, mqttPort, callback, wifiClient);

void setup() {
  // put your setup code here, to run once:
  
  WiFi.hostname(deviceName); // reset router to fix hostanme issue // problem with DNS ?
  Serial.begin(115200);
  delay(500);

  // initialize digital pin LED_BUILTIN as an output.
  pinMode(2, OUTPUT);

  // connection to the broker...
  if(connect()) {
    subscribe();
  }
}

void loop() {
  
  // reset connection if necessary...
  if(connectionFlag == true) {
    client.loop();
  }
  else {
    if(connect()) {
      subscribe();
    }
  }
}

void subscribe() {
  client.setCallback(callback);
  client.subscribe(topic);
   //subscript to a topic
  Serial.println("MQTT subscribed");
}

// Send back JSON object as byte message. 
void responseMessage(String message1, String message2) {
  
  // Json object...
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["state"] = message1;
  root["time"] = message2;

  char JSONmessageBuffer[100];
  root.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
  
  // publish message
  if (client.publish(topic, JSONmessageBuffer) == true) {
    Serial.println("Success sending message ");
    Serial.println(JSONmessageBuffer);
  } else {
    Serial.println("Error sending message");
  }
  
  // Empty the JSON object
  jsonObject = "";

}

bool connect() {
  // Have sure that before connection client is disconnected...
  if (client.connected()) {
    client.disconnect();  
  }

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Generate client name based on MAC address and last 8 bits of microsecond counter
  String clientName;
  clientName += "esp8266-";
  uint8_t mac[6];
  WiFi.macAddress(mac);
  clientName += macToStr(mac);
  clientName += "-";
  clientName += String(micros() & 0xff, 16);

  Serial.print("Connecting to ");
  Serial.print(mqttServer);
  Serial.print(" as ");
  Serial.println(clientName);
  
  if (client.connect((char*) clientName.c_str(), mqttUser, mqttPassword)) {
    Serial.println("connected");
    connectionFlag = true;
    return true;
  }
  else {
    Serial.println(client.state());
    connectionFlag = false;
    return false;
  }
}

String macToStr(const uint8_t* mac)
{
  String result;
  for (int i = 0; i < 6; ++i) {
    result += String(mac[i], 16);
    if (i < 5)
      result += ':';
  }
  return result;
}
