#include "config.h"

#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <FastLED.h>

// ----------------------------------------------

#define LED_PIN     0
#define NUM_LEDS    3
#define BRIGHTNESS  64
#define LED_TYPE    WS2811
#define COLOR_ORDER GRB
#define UPDATES_PER_SECOND 100
CRGB leds[NUM_LEDS];

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
const char* topic = "/devices/rgb/RGBLT000001-CUBE/update";

// ----------------------------------------------

String deviceName = "Desk RGB Light";
String jsonObject = "";
bool connectionFlag = false;

// ----------------------------------------------
// Color constructors
CRGBPalette16 currentPalette;
TBlendType    currentBlending;

extern CRGBPalette16 myRedWhiteBluePalette;
extern const TProgmemPalette16 myRedWhiteBluePalette_p PROGMEM;

// ----------------------------------------------

void callback(char* topic, byte* payload, unsigned int length) {

  for (int i = 0; i < length; i++) {
    jsonObject.concat((char) payload[i]);
  }

  // Create JSON object...
  StaticJsonBuffer<200> jsonBuffer;

  // Get root of JSON object
  JsonObject& root = jsonBuffer.parseObject(jsonObject);

  //int numOfLeds = root["numOfLeds"].as<int>();
  int hue = root["hue"].as<int>();
  int saturation = root["saturation"].as<int>();
  int value = root["value"].as<int>();
  int brightness = root["brightness"].as<int>();
  String state = root["state"].as<String>();
  String option = root["option"].as<String>();
  
 
 
  if(root["state"].as<String>() == "off") {
    Serial.println("Turn off");
    LedOff(); 
  }
  if(root["state"].as<String>() == "on") {
    Serial.println("Turn on");
    if(LedOn()) {

      FastLED.setBrightness(brightness);
      // ****************************************************
      if(root["option"].as<String>() == "solid") {
        // ConstPalleteColor(red, green, blue);
        ConstPalleteColorHSV(hue, saturation, value);
      }

      else if(root["option"].as<String>() == "rainbow") {
        rainbow_march(200, 10);
        FastLED.show();
      }

      if(root["option"].as<String>() == "whiteAndBlack") {
        SetupBlackAndWhiteStripedPalette();
        FastLED.show();
      }
      // ****************************************************
    }; 
  }
  
  
  // Clear JSON object
  jsonObject = "";
}

WiFiClient wifiClient;
PubSubClient client(mqttServer, mqttPort, callback, wifiClient);

// ====================================================
// Power State
bool LedOn() {
  digitalWrite(2, HIGH);   // turn the Light on (HIGH is the voltage level);
  return true;
}

void LedOff() {
  digitalWrite(2, LOW);    // turn the Light off by making the voltage LOW
}

// ====================================================
// Color Palettes

void ConstPalleteColor(uint8_t R, uint8_t G, uint8_t B) {    
    for( int i = 0; i < NUM_LEDS; i++) {
        fill_solid( &(leds[i]), NUM_LEDS, CRGB( R, G, B) );
        FastLED.show();
    }
}

void ConstPalleteColorHSV(uint8_t H, uint8_t S, uint8_t V) {    
    for( int i = 0; i < NUM_LEDS; i++) {
        fill_solid( &(leds[i]), NUM_LEDS, CHSV( H, S, V) );
        FastLED.show();
    }
}

void SetupBlackAndWhiteStripedPalette()
{
    // 'black out' all 16 palette entries...
    fill_solid( currentPalette, 16, CRGB::Black);
    // and set every fourth one to white.
    currentPalette[0] = CRGB::White;
    currentPalette[4] = CRGB::White;
    currentPalette[8] = CRGB::White;
    currentPalette[12] = CRGB::White;
}
 
void rainbow_march(uint8_t thisdelay, uint8_t deltahue) {

    uint8_t thishue = millis()*(255-thisdelay)/255;
    fill_rainbow(leds, NUM_LEDS, thishue, deltahue); 
}

void SetupPurpleAndGreenPalette()
{
    CRGB purple = CHSV( HUE_PURPLE, 255, 255);
    CRGB green  = CHSV( HUE_GREEN, 255, 255);
    CRGB black  = CRGB::Black;
    
    currentPalette = CRGBPalette16(
                                   green,  green,  black,  black,
                                   purple, purple, black,  black,
                                   green,  green,  black,  black,
                                   purple, purple, black,  black );
}

const TProgmemPalette16 myRedWhiteBluePalette_p PROGMEM =
{
    CRGB::Red,
    CRGB::Gray, // 'white' is too bright compared to red and blue
    CRGB::Blue,
    CRGB::Black,
    
    CRGB::Red,
    CRGB::Gray,
    CRGB::Blue,
    CRGB::Black,
    
    CRGB::Red,
    CRGB::Red,
    CRGB::Gray,
    CRGB::Gray,
    CRGB::Blue,
    CRGB::Blue,
    CRGB::Black,
    CRGB::Black
};
// ====================================================










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

  // Leds setup
  delay( 3000 ); // power-up safety delay
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
  FastLED.setBrightness(  BRIGHTNESS );
  
  currentPalette = RainbowColors_p;
  currentBlending = LINEARBLEND;
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
  // TODO...
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
