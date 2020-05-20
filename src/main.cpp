#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <PubSubClient.h>
#include "DHT.h"         // Librairie des capteurs DHT
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <DHT_U.h>
#include <string>
#include <iostream>

//using namespace std;

#define wifi_ssid "AndroidAP"
#define wifi_password "password"
//TO DO
#define mqtt_server "test.mosquitto.org"
const int mqttPort = 1883; //port utilisé par le Broker
//#define mqtt_user "guest"  //s'il a été configuré sur Mosquitto
//#define mqtt_password "guest" //idem
#define temperature_topic "EPSI/LEA/sensor/temperature"  //Topic température
#define humidity_topic "EPSI/LEA/sensor/humidity"        //Topic humidité
#define topic_test "EPSI/LEA/test" //Topic Test
ESP8266WiFiMulti WiFiMulti;

//Buffer qui permet de décoder les messages MQTT reçus
char message_buff[100];

long lastMsg = 0;   //Horodatage du dernier message publié sur MQTT
long lastRecu = 0;
bool debug = false;  //Affiche sur la console si True
long tps=0;

#define DHTPIN 14    // Pin sur lequel est branché le DHT
#define DHTTYPE DHT11       // DHT 11

//Création des objets
DHT dht(DHTPIN, DHTTYPE);
WiFiClient espClient;
PubSubClient client(espClient);


//Connexion au réseau WiFi
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connexion a ");
  Serial.println(wifi_ssid);

  WiFi.begin(wifi_ssid, wifi_password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
 Serial.println("Connexion WiFi etablie  !! ");
 Serial.print("=> Addresse IP : ");
 Serial.print(WiFi.localIP());
}

// Déclenche les actions à la réception d'un message
// D'après http://m2mio.tumblr.com/post/30048662088/a-simple-example-arduino-mqtt-m2mio
void callback(char* topic, byte* payload, unsigned int length) {

  int i = 0;
  if ( debug ) {
    Serial.println("Message recu =>  topic: " + String(topic));
    Serial.print(" | longueur: " + String(length,DEC));
  }
  // create character buffer with ending null terminator (string)
  for(i=0; i < length; i++) {
    message_buff[i] = payload[i];
  }
  message_buff[i] = '\0';

  String msgString = String(message_buff);
  if ( debug ) {
    Serial.println("Payload: " + msgString);
  }

  if ( msgString == "ON" ) {
    digitalWrite(D2,HIGH);
  } else {
    digitalWrite(D2,LOW);
  }
}

void reconnect(){
  while (!client.connected()) {
    Serial.println("Connection au serveur MQTT ...");
    if (client.connect("ESP32Client_LEA")) {
      Serial.println("LEA_MQTT connecté");
    }
    else {
      Serial.print("echec, code erreur= ");
      Serial.println(client.state());
      Serial.println("nouvel essai dans 2s");
    delay(2000);
    }
  }
}

void setup_mqtt(){
  client.setServer(mqtt_server, mqttPort);
  client.setCallback(callback);//Déclaration de la fonction de souscription
  reconnect();
}

void setup() {
  Serial.begin(115200);     //Facultatif pour le debug
  //pinMode(D2,OUTPUT);     //Pin 2
  setup_wifi(); //On se connecte au réseau wifi
  setup_mqtt();
  client.publish(topic_test, "Initialisation ...");
  //client.setServer(mqtt_server, 1883);    //Configuration de la connexion au serveur MQTT
  //client.setCallback(callback);  //La fonction de callback qui est executée à chaque réception de message
  //dht.begin();
}
//Fonction pour publier un float sur un topic
void mqtt_publish(String topic, String t){
  char top[topic.length()+1];
  topic.toCharArray(top,topic.length()+1);
  char t_char[50];
  //String t_str = String(t);
  t.toCharArray(t_char, t.length() + 1);
  client.publish(top,t_char);
}

void loop() {
  dht.begin();
  reconnect();
  client.loop();
  // Delay between measurements.
  delay(2000);
  // Get temperature event and print its value.
  // dht.humidity().getSensor(&sensor);
  // dht.humidity().getEvent(&event_h);
  // dht.humidity().getEvent(&event_t);
  float t = dht.readTemperature() ;
  float h = dht.readHumidity();
  if (isnan(t) || isnan(h)) {
    Serial.println(F("Error reading data, check your DHT "));
  }
  else {
  String temperature;
  String humidity;
  temperature += F("{‘value’ : ");
  temperature += String(t, 2);
  temperature += F(" ; unity: °C}");
  humidity += F("{'value' : ");
  humidity += String(h,2);
  humidity += F(" ; unity: %}");
  Serial.print(temperature);
  Serial.print(humidity);
  mqtt_publish(temperature_topic,temperature);
  mqtt_publish(humidity_topic,humidity);
  }
}
