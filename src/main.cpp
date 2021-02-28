#include <Arduino.h>

#include <Vault.h>

#include <SimpleTimer.h>    //https://github.com/marcelloromani/Arduino-SimpleTimer/tree/master/SimpleTimer
#include <ESP8266WiFi.h>    //if you get an error here you need to install the ESP8266 board manager 
#include <ESP8266mDNS.h>    //if you get an error here you need to install the ESP8266 board manager 
#include <PubSubClient.h>   //https://github.com/knolleary/pubsubclient
#include <ArduinoOTA.h>     //https://github.com/esp8266/Arduino/tree/master/libraries/ArduinoOTA

#define DEBUG_FAUXMO                Serial

#include <fauxmoESP.h>

#define USER_MQTT_CLIENT_NAME     "test1"  

WiFiClient espClient;
PubSubClient mqtt(espClient);
SimpleTimer timer;
fauxmoESP fauxmo;


const byte BUFFER_SIZE = 50;

char messageBuffer[BUFFER_SIZE];

const char* ssid = USER_SSID ; 
const char* password = USER_PASSWORD ;
const char* mqtt_server = USER_MQTT_SERVER ;
const int mqtt_port = USER_MQTT_PORT ;
const char *mqtt_user = USER_MQTT_USERNAME ;
const char *mqtt_pass = USER_MQTT_PASSWORD ;
const char *mqtt_client_name = USER_MQTT_CLIENT_NAME; 


void wifi_setup() {
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}


void mqtt_connect() {
  int retries = 0;
  while (!mqtt.connected()) {
    if(retries < 150) {
      Serial.print("Attempting MQTT connection...");
      if (mqtt.connect(mqtt_client_name, mqtt_user, mqtt_pass)) {
        Serial.println("connected");
        mqtt.publish(USER_MQTT_CLIENT_NAME"/checkIn", "Connected"); 
      } 
      else {
        Serial.print("failed, rc=");
        Serial.print(mqtt.state());
        Serial.println(" try again in 5 seconds");
        retries++;
        delay(5000);
      }
    }
    
    if(retries > 149) {
      ESP.restart();
    }
  }
}


void checkIn() {
  mqtt.publish(USER_MQTT_CLIENT_NAME"/checkIn","OK"); 
}

//Run once setup
void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  
  wifi_setup();
  
  mqtt.setServer(mqtt_server, mqtt_port);
  
  ArduinoOTA.setHostname(USER_MQTT_CLIENT_NAME);
  ArduinoOTA.begin(); 
  delay(10);

  fauxmo.addDevice("light five");
  fauxmo.setPort(80); // required for gen3 devices
  fauxmo.enable(true);

  fauxmo.onSetState([](unsigned char device_id, const char * device_name, bool state, unsigned char value) {
    Serial.printf("[MAIN] Device #%d (%s) state: %s value: %d\n", device_id, device_name, state ? "ON" : "OFF", value);
    mqtt.publish(USER_MQTT_CLIENT_NAME"/checkIn","Alexa callback"); 
  });

  timer.setInterval(60000, checkIn);
}

void loop() {
  if (!mqtt.connected()) {
    mqtt_connect();
  }
  mqtt.loop();

  ArduinoOTA.handle();

  timer.run();
}
