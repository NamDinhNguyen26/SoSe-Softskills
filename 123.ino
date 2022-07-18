
//#include <ArduinoHttpClient.h>
#include <SPI.h>

#include <WiFiClient.h>

#include <ArduinoJson.h>


#include <PubSubClient.h>


#include <WiFiManager.h>                  // Bibliothek einbinden, um Uebergabe der WiFi Credentials ueber einen AP zu ermoeglichen


//WLAN
const char* mqtt_topic_publish = "automatictemp";
const char* mqtt_topic_subscribe_auto = "automatic";
const char* mqtt_topic_subscribe = "temp";
 char* ssid = "iPhone von Ali";
 char* password = "12345678";

WiFiClient espClient;
PubSubClient client(espClient);
WiFiManager wifiManager;

//NODE RED SERVER
const char* mqtt_server = "mqtt.iot.informatik.uni-oldenburg.de";
const int mqtt_port = 2883;
const char* mqtt_pw = "SoftSkills";
const char* mqtt_user = "sutk";

//STEPPER
const int stepPin = D5;
const int dirPin = D0;

//API
const String city = "Oldenburg";
const String api_key = "4b938fe647c8f51e66693eadec971cd2";
int temperature_Celsius_Int = 0;


//VARIABELEN
int currentstate = 0; //jetziger Step
int futurestate = 0; //zukünftiger Step ergeben aus Temeprature
int temperature = 0; // aus Node red in C
int currenttemperature = 0; // Kopie von Temperature
boolean automatic = false; // Boolean für Auto Steuerung



void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");

    // Create a random client ID: Client ID MUSS inviduell sein, da der MQTT Broker nicht mehrere Clients mit derselben ID bedienen kann
    String clientId = "Client-";
    clientId += String(random(0xffff), HEX);

    // Attempt to connect    
    if (client.connect(clientId.c_str(), mqtt_user, mqtt_pw)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.subscribe(mqtt_topic_subscribe);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}




void drehen(){
int dreh = currentstate - futurestate;
if (dreh > 0){
  digitalWrite(dirPin,HIGH);
  
for(int x = 0; x<dreh; x++) {
   digitalWrite(stepPin,HIGH);
  delayMicroseconds(500);
  digitalWrite(stepPin,LOW);
  delayMicroseconds(500);
}
}
else if (dreh < 0){
  dreh = dreh * -1;

  digitalWrite(dirPin,LOW);
  
  for(int x = 0; x<dreh; x++) {
   digitalWrite(stepPin,HIGH);
  delayMicroseconds(500);
  digitalWrite(stepPin,LOW);
  delayMicroseconds(500);
  }
  }


currentstate = futurestate;

}


//Wetter
void getCurrentWeatherConditions() {
  
  if (espClient.connect("api.openweathermap.org", 80)) {
   espClient.println("GET /data/2.5/weather?q=" + city + ",DE&units=metric&lang=de&APPID=" + api_key);
    espClient.println("Host: api.openweathermap.org");
    espClient.println("Connection: close");
    espClient.println();
  } 
  
  const size_t capacity = JSON_ARRAY_SIZE(2) + 2 * JSON_OBJECT_SIZE(1) + 2 * JSON_OBJECT_SIZE(2) + 2 * JSON_OBJECT_SIZE(4) + JSON_OBJECT_SIZE(5) + JSON_OBJECT_SIZE(6) + JSON_OBJECT_SIZE(14) + 360;
  DynamicJsonDocument doc(capacity);
  deserializeJson(doc, espClient);
  espClient.stop();

  int temperature_Celsius = doc["main"]["temp"];
  temperature = (int)temperature_Celsius;
  

  
}

void callback(char* topic, byte* payload, unsigned int length) {
  char receivedPayload[length];
  
  for (int i = 0; i < length; i++) {
   
    receivedPayload[i] = (char) payload[i];
  }
 
  temperature = atoi(receivedPayload);
  futurestate = (temperature -5)*10;
  

}



void setup() {

//Connect API
wifiManager.autoConnect("Wemos_D1");

setup_wifi();

client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
   
  

  //Stepper
  pinMode(dirPin, OUTPUT);  // Pins are outputs
  pinMode(stepPin, OUTPUT);


}

void settemperature(int t){
  temperature = t;
  futurestate = t *10; // zu ermitteln
}

void setautomatic(boolean t){
  automatic = t;
}


void loop (){
  

if (automatic == false){
if (currentstate != futurestate){
drehen();

}

else{
//getCurrentWeatherConditions();

if (temperature == 5){
  //Motor so und so 
}
drehen();

delay(1000);

Serial.print(temperature);
settemperature(5);
Serial.print(temperature);
}
}

if (!client.connected()) {
    reconnect();
  }
  client.loop();
}
