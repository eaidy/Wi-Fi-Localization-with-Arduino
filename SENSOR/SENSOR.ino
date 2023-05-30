// ESP-3
// Trilateration sensor code for the sensor with id = 1

#include <ESP8266WiFi.h>        // Include the Wi-Fi library
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>   // Include Server library
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include <dht.h>

#define TRUE 1
#define FALSE 0

// Pre-Definitions
#define SUCCESS_PIN 6           // Pin that indicates Wi-Fi connection with AP is established
#define FAILURE_PIN 7           // Pin that indicates Wi-Fi connection is not established or lost
#define NODE_ID 4


#define BUZZER_PIN 3
#define MQ2_PIN A1
#define FLAME_PIN A0
#define DHT_PIN 2
#define LED_PIN 4

#define INIT_URL ("http://192.168.1.1:80/initnode" + String(NODE_ID))
#define SENSOR_URL "http://192.168.1.1:80/sensorprops"

// Prototypes
void connectionStateToggle(void);

// Wi-Fi Configuration Variables
const char* ssid     = "VehicleAP";      // The SSID (name) of the Wi-Fi network Vehicle provides
const char* password = "12345678";       // The password of the Wi-Fi network

// Initalize Server
ESP8266WebServer server(80);

// Time Variables for polling mode handling
unsigned long previousTime = 0;
const unsigned long interval = 650; // Blink interval in milliseconds

// Fire state
int fire_state = FALSE;

struct SensorProps {
  float humadity;
  float temperture;
  int flame;
  int gass;
}

SensorValues sensorValues;

// DHT Sensor
dht DHT;

void setup() {

  Serial.begin(115200);         // Start the Serial communication to send messages to the computer
  pinMode(SUCCESS_PIN, OUTPUT);
  pinMode(FAILURE_PIN, OUTPUT);

  // Initilize status pins
  digitalWrite(SUCCESS_PIN, LOW);
  digitalWrite(FAILURE_PIN, HIGH);

  delay(10);
  // Serial.println('\n');
  
  // Connection begin
  WiFi.begin(ssid, password);             // Connect to the network
  // Serial.print("Connecting to ");
  // Serial.print(ssid); Serial.println(" ...");

  int i = 0;
  while (WiFi.status() != WL_CONNECTED) { // Wait for the Wi-Fi to connect
    delay(1000);
  }

  sendPropsToVehicle();

  // Serial.println('\n');
  // Serial.println("Connection established!");  

  // ESP-1 Server
  // Purpose of this server is to get distance by RSSI data from this to NodeMCU Vehicle.
  // Vehicle will be making 3 request each to corresponding ESP module to gather nessecary distance data.
  // Then vehicle going to implement trilateration.
  server.on("/getnodeprops", handleNodeProps);

  server.begin();
  // Serial.println("HTTP server started");
}

void loop() {

  server.handleClient();

  unsigned long currentTime = millis();

  if (currentTime - previousTime >= interval) { // Time conditional to run the loop without blocking it. Avoiding use of delay()
    previousTime = currentTime;

    connectionStateToggle();

    sensorValues = readSensorValues();
    fire_state = checkFireState(sensorValues);

    if(fire_state){
      sendSensorProps();
    }

    Serial.print("IP Address : ");
    Serial.print(WiFi.localIP());
  }
}

void connectionStateToggle(){
  if(WiFi.status() == WL_CONNECTED){
    digitalWrite(SUCCESS_PIN, HIGH);
    digitalWrite(FAILURE_PIN, LOW);
  } 
  else {
    digitalWrite(SUCCESS_PIN, LOW);
    digitalWrite(FAILURE_PIN, HIGH);
  }
};

void handleNodeProps(){
  StaticJsonDocument<200> NODE_PROPS;
  NODE_PROPS["id"] = NODE_ID;
  NODE_PROPS["ip"] = WiFi.localIP().toString();

  String NODE_PROPS_BUFFER;
  serializeJson(NODE_PROPS, NODE_PROPS_BUFFER);
  server.send(200, "application/json", NODE_PROPS_BUFFER);
}

void sendPropsToVehicle(){
  StaticJsonDocument<100> NODE_PROPS;
  NODE_PROPS["id"] = NODE_ID;
  NODE_PROPS["ip"] = WiFi.localIP();

  String NODE_PROPS_BUFFER;
  serializeJson(NODE_PROPS, NODE_PROPS_BUFFER);

  if (WiFi.status() == WL_CONNECTED) { //Check WiFi connection status

    WiFiClient client;
    HTTPClient http;    //Declare object of class HTTPClient
  
    http.begin(client, INIT_URL);      //Specify request destination
    http.addHeader("Content-Type", "application/json");  //Specify content-type header
 
    int httpCode = http.POST(NODE_PROPS_BUFFER);   //Send the request
    String payload = http.getString();                  //Get the response payload

    Serial.println(httpCode);   //Print HTTP return code
    Serial.println(payload);    //Print request response payload
 
    http.end();  //Close connection
 
  } else {
    Serial.println("Error in WiFi connection");
  }

};

void sendSensorProps(){
  StaticJsonDocument<100> NODE_PROPS;

  NODE_PROPS["id"] = NODE_ID;
  NODE_PROPS["ip"] = WiFi.localIP();
  NODE_PROPS["firestate"] = fire_state;

  String NODE_PROPS_BUFFER;
  serializeJson(NODE_PROPS, NODE_PROPS_BUFFER);

  if (WiFi.status() == WL_CONNECTED) { //Check WiFi connection status

    WiFiClient client;
    HTTPClient http;    //Declare object of class HTTPClient
  
    http.begin(client, SENSOR_URL);      //Specify request destination
    http.addHeader("Content-Type", "application/json");  //Specify content-type header
 
    int httpCode = http.POST(NODE_PROPS_BUFFER);   //Send the request
    String payload = http.getString();                  //Get the response payload

    Serial.println(httpCode);   //Print HTTP return code
    Serial.println(payload);    //Print request response payload
 
    http.end();  //Close connection
 
  } else {
    Serial.println("Error in WiFi connection");
  }

}

int checkFireState(SensorValues sensorValuesArg){
  if (sensorValuesArg.flame < 500 && sensorValuesArg.gass > 800 && sensorValuesArg.temperture > 30) {
    Serial.println("Yangin Tespit Edildi!");
    // Aktif buzzer ve ledi çalıştır
    digitalWrite(BUZZER_PIN, HIGH);
    digitalWrite(LED_PIN, HIGH);
    // Buraya ek olarak yapmak istediğiniz başka bir tepkiyi ekleyebilirsiniz.
    // Örneğin, SMS gönderme, acil durum bildirimi vb.
    return TRUE;
  } else {
    // Yangın tespit edilmediğinde buzzer ve ledi kapat
    digitalWrite(BUZZER_PIN, LOW);
    digitalWrite(LED_PIN, LOW);
    return FALSE;
  }
}

SensorValues readSensorValues(){
  SensorValues sensorValuesBuffer;

  int chk = DHT.read11(DHTPIN);
  sensorValuesBuffer.temperture = DHT.temperature;
  sensorValuesBuffer.humadity = DHT.humidity;
  sensorValuesBuffer.flame = analogRead(FLAMEPIN);
  sensorValuesBuffer.gass = analogRead(MQ2PIN);

  return sensorValuesBuffer;

}



