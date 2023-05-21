// ESP-3
// Trilateration sensor code for the sensor with id = 1

#include <ESP8266WiFi.h>        // Include the Wi-Fi library
#include <ESP8266WebServer.h>   // Include Server library
#include <ArduinoJson.h>

// Pre-Definitions
#define SUCCESS_PIN 2           // Pin that indicates Wi-Fi connection with AP is established
#define FAILURE_PIN 1           // Pin that indicates Wi-Fi connection is not established or lost
#define NODE_ID 2

// Prototypes
void connectionStateToggle(void);
void handleDistance();
bool calculateDistance(float* distanceVariable);

// Wi-Fi Configuration Variables
const char* ssid     = "VehicleAP";      // The SSID (name) of the Wi-Fi network Vehicle provides
const char* password = "12345678";       // The password of the Wi-Fi network

// Initalize Server
ESP8266WebServer server(80);

// Time Variables for polling mode handling
unsigned long previousTime = 0;
const unsigned long interval = 500; // Blink interval in milliseconds

// Trilateration Variables to be sent
float distance_node_vehicle;

// RSSI Calibration constants
const int RSSI_SAMPLES = 10;
const int RSSI_AT_ONE_METER = -50;  // RSSI at one meter distance
const int SIGNAL_LOSS_EXPONENT = 2;  // Signal loss exponent (path loss)

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
    Serial.print(++i);
    Serial.print(' ');
  }

  // Serial.println('\n');
  // Serial.println("Connection established!");  

  // ESP-1 Server
  // Purpose of this server is to get distance by RSSI data from this to NodeMCU Vehicle.
  // Vehicle will be making 3 request each to corresponding ESP module to gather nessecary distance data.
  // Then vehicle going to implement trilateration.
  server.on("/getnodeprops", handleNodeProps);
  server.on("/getnodedistance", handleDistance);

  server.begin();
  // Serial.println("HTTP server started");
}

void loop() {

  server.handleClient();

  unsigned long currentTime = millis();

  if (currentTime - previousTime >= interval) { // Time conditional to run the loop without blocking it. Avoiding use of delay()
    previousTime = currentTime;

    connectionStateToggle();

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
  StaticJsonDocument<100> NODE_PROPS;
  NODE_PROPS["id"] = NODE_ID;
  NODE_PROPS["ip"] = WiFi.localIP().toString();

  String NODE_PROPS_BUFFER;
  serializeJson(NODE_PROPS, NODE_PROPS_BUFFER);

  server.send(200, "application/json", NODE_PROPS_BUFFER);
}

void handleDistance(){

  // Create JSON NODE_PROPSument for HTTP
  StaticJsonDocument<100> NODE_PROPS;
  NODE_PROPS["id"] = NODE_ID;
  NODE_PROPS["ip"] = WiFi.localIP().toString();

  bool calculatedOK = calculateDistance(&distance_node_vehicle);
  if(calculatedOK){
    NODE_PROPS["distance"] = distance_node_vehicle;
    
    String NODE_PROPS_BUFFER;
    serializeJson(NODE_PROPS, NODE_PROPS_BUFFER);
    server.send(200, "application/json", NODE_PROPS_BUFFER);
  }
}

bool calculateDistance(float* distanceVariable){

  if(WiFi.status() == WL_CONNECTED){

    int rssiSum = 0, averageRssi;
    float signalLoss, distance;

    for (int i = 0; i < RSSI_SAMPLES; i++) {
      int rssi = WiFi.RSSI();
      rssiSum += rssi;
      delay(10);
    }

    averageRssi = rssiSum / RSSI_SAMPLES;
    signalLoss = RSSI_AT_ONE_METER - averageRssi;
    distance = pow(10, signalLoss / (10 * SIGNAL_LOSS_EXPONENT));

    *distanceVariable = distance;

    return 1;
  } 
  else return 0;
  
};




