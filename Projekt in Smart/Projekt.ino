#include <SPI.h>
#include <ArduinoJson.h>

#include <PubSubClient.h>
#include <WiFiManager.h>   // Bibliothek einbinden, um Uebergabe der WiFi Credentials ueber einen AP zu ermoeglichen
#include <WiFiClient.h>

//WLAN
WiFiClient espClient;
PubSubClient client(espClient);
WiFiManager wifiManager;

char* ssid = "";
char* password = "";

//NODE RED SERVER
const char* mqtt_server = "mqtt.iot.informatik.uni-oldenburg.de";
const int mqtt_port = 2883;
const char* mqtt_pw = "SoftSkills";
const char* mqtt_user = "sutk";

//NODE RED TOPICS
const char* mqtt_topic_publish = "automatictemp"; // Topic für Daten Arduino zu Nodered
const char* mqtt_topic_subscribe = "temp";   // Topic für Daten Nodered zu Arduino

//STEPPER
const int stepPin = D5;  // Schritte
const int dirPin = D0;  // Richtung
const int notAus = D2; // Motor

//API
const String city = "Oldenburg";
const String api_key = "4b938fe647c8f51e66693eadec971cd2";
int temperature_Celsius_Int = 0;

//VARIABELEN
int currentstate = 0; //jetziger Step
int futurestate = 0; //zukünftiger Step ergeben aus Temeperature
int temperature = 5; // aus Node red gegeben oder bei Auto Steuerung aus der API
int lastTemp = 5; // Letzte aktuelle Temperatur
boolean automatic = false; // Boolean für Auto Steuerung True = an, False = aus

int lastMsg = 0; //Für 5 Sekunden Delay bei der API
int lastMsg2 = 0; // Für 5 Sekunden Delay beim checken des Temperaturwertes 
int val = 0; // Value zum publishen der Payload bei Auto Steuerung


//METHODEN


//MOTOR
void drehen(){   //Methode zum drehen des Motors um den Abstand vom jetzigem Step zum zukünftigen Step in die jeweils richtige Richtung
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
currentstate = futurestate; // sobald Motor sich gedreht hat sind beide Werte identisch
}


//WETTER
void getCurrentWeatherConditions() {            //Ermittelt den Celsius Wert in Oldenburg per API
  
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
  Serial.println(temperature_Celsius);

  
}



//PAYLOAD
void callback(char* topic, byte* payload, unsigned int length) { //Bekommt einen neuen Wert von Node Red sobald ein neuer Wert existiert
  char receivedPayload[length];
  
  for (int i = 0; i < length; i++) {
   
    receivedPayload[i] = (char) payload[i];
  }
 
  temperature = atoi(receivedPayload);
  Serial.println(temperature);

  
  }

//SETUP
void setup() {


Serial.begin(9600);

//WiFi
wifiManager.autoConnect("Wemos_D1");

//NODE RED SERVER
client.setServer(mqtt_server, mqtt_port);
client.setCallback(callback);

  //Stepper
  pinMode(dirPin, OUTPUT);  
  pinMode(stepPin, OUTPUT);
  //pinmode(notAus, OUTPUT); 


}

//AUTOMATIC
void checkAuto(){                   // Guckt ob automatische Steuerung aktiviert wurde oder nicht
  if (temperature == 100){
    automatic = true;
    
  }
  
  if (temperature == 200){
    automatic = false;
    temperature = lastTemp;
    
  }

/*  //NOT AUS 
  if (temperature == 1000){
    digitalwrite(notAus, HIGH);
  temperature = lastTemp;
  }

  if (temperature == 2000){
    digitalwrite(notAus, HIGH);
    temperature = lastTemp;
  }
  */
}

//TEMPERATURCHECK
void currentTemperatureCheck(){         //Gibt die Aktuelle Einstellung der Heizung aus und berechnet den zukünftigen Schritt neu aus
  if (millis() - lastMsg2 > 5000){
  lastMsg2 = millis();
  futurestate = (temperature -5)*10;
  Serial.println("Heizung ist auf: " + temperature + " Grad gestellt!");
   lastTemp = temperature;
  Serial.println("Zukuenftiger Schritt ist bei: "+ futurestate);
  Serial.println("Jetziger Schritt ist bei: " + currentstate);
  Serial.println();

  if (automatic == true){               // Wenn automatic true ist sendet der den aktuellen Temperaturwert zu Nodered
  val = temperature;
  String val_str = String(val);
  char val_buff [val_str.length() + 1];
  val_str.toCharArray(val_buff, val_str.length()+1);
  client.publish(mqtt_topic_publish, val_buff);
}
}
}

//RECONNECT 
void reconnect() {
  // Schleife bis wir connected sind
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");

    //Client ID MUSS inviduell sein, da der MQTT Broker nicht mehrere Clients mit derselben ID bedienen kann
    String clientId = "Client-";
    clientId += String(random(0xffff), HEX);

    // Verbinden   
    if (client.connect(clientId.c_str(), mqtt_user, mqtt_pw)) {
      Serial.println("connected");
      
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

//LOOP
void loop (){

  client.loop();
  
  checkAuto();                       //gucken ob autosteurung aus node red an ist
  

if (automatic == false){             // Bei false Manuelle Steuerung der Temperatur über Node Red

if (currentstate != futurestate){
drehen();                            //Motor dreht sich wenn der zukünftige Schrittwert sich verändert hat
}

}
else{                                 //sonst automatische Steuerung
if(millis() - lastMsg > 5000){
  getCurrentWeatherConditions();     // Daten aus Wetter Api nehmen und umgerechnet zu nodered schicken
  if (temperature < 5){
    temperature = 24;
  }
  else if (temperature < 10){
    temperature = 23;
  }
  else if (temperature < 15){
    temperature = 21;
  }
  else if (temperature < 20){
    temperature = 18;
  }
  else {
    temperature = 5;
  }
 
  lastMsg = millis();

}

drehen();                            //Motor um den neu errechneten Wert aus der API drehen
}

if (!client.connected()) {            //Neu verbinden falls Verbindung verloren
    reconnect();
  }

  
currentTemperatureCheck();           //temperatur ausgeben und neuen Futurestate berechnen
 
 
 }
