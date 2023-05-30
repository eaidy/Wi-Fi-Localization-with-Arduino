#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include <math.h>

#define FALSE 0
#define TRUE 1

// NodeMCU PIN connections
#define FLAME_SENSOR_1 12
#define FLAME_SENSOR_2 13
#define FLAME_SENSOR_3 15

#define US_ECHO 3
#define US_TRIG 1

#define EN_A 16
#define EN_B 14
#define IN_1 5
#define IN_2 4
#define IN_3 0
#define IN_4 2

// Node Server URL's
#define NODE_CONFIG_URL "/getnodeprops"

unsigned long previousTime = 0;

// Prototypes

// Access Point Credentials
const char* ssid = "VehicleAP";
const char* password = "12345678";

ESP8266WebServer server(80);

// Coordinate System configuration - Node positions
struct Coordinates {
  float x;
  float y;
};

Coordinates NODE1_XY = { 0, -5.0 };
Coordinates NODE2_XY = { 0, 0 };
Coordinates NODE3_XY = { 10.0, 0 };
Coordinates FIRE_SENSOR_XY = { 6.5, -2 };   // Fire Sensor's coordinates likely to be pre-defined. It's hard to get both Vehicle's and Sensor's coordinates by Wi-Fi trilateration.

// System & Localization Node & Network Abstractions
struct TrilaterationNode {
  String ip = "";
  int id;
  bool connectionStatus = FALSE;
  float distance = 0;
  float x = 0;
  float y = 0;
};

struct Vehicle {
  float x;
  float y;
};

struct FireSensor {
  String ip = "";
  int id = 4;
  float distance = 0;
  bool connectionStatus;
  float humadityLevel;
  float tempertureLevel;
  float somekeLevel;
  const float x = FIRE_SENSOR_XY.x;
  const float y = FIRE_SENSOR_XY.y;
};

// Entire System's Class, everything about this network system is going to be configured here
class SystemNetwork {

  public:

    // Trilateration Nodes
    TrilaterationNode NODE_1;
    TrilaterationNode NODE_2;
    TrilaterationNode NODE_3;

    // Fire Sensor
    FireSensor FIRE_SENSOR_NODE;

    // Vehicle
    Vehicle VEHICLE;

    struct NodeResponse {
      int id;
      const char* ip;
      float distance;
    };

    SystemNetwork(){
      NODE_1.x = NODE1_XY.x;
      NODE_1.y = NODE1_XY.y;
      NODE_2.x = NODE2_XY.x;
      NODE_2.y = NODE2_XY.y;
      NODE_3.x = NODE3_XY.x;
      NODE_3.y = NODE3_XY.y;
    }

    NodeResponse getNodeProps(const String& url) {
      WiFiClient client;
      HTTPClient http;

      NodeResponse node;

      // Send GET request
      http.begin(client, url);
      int httpResponseCode = http.GET();

      if (httpResponseCode == HTTP_CODE_OK) {
        // Read response as String
        String response = http.getString();

        // Parse JSON response
        DynamicJsonDocument jsonDoc(100); // Adjust the buffer size as per your response size
        DeserializationError error = deserializeJson(jsonDoc, response);

        if (error) {
          Serial.print("JSON parsing failed: ");
          Serial.println(error.c_str());
        } else {
          // Access parsed JSON values
          node.id = jsonDoc["id"];
          node.ip = jsonDoc["ip"];
          node.distance = jsonDoc["distance"];

          configureNode(node.id, node.ip, node.distance);
        }
      } else {
        Serial.print("HTTP request failed with error code: ");
        Serial.println(httpResponseCode);
      }

      http.end();

      return node;
    }

    bool configureNode(int NODE_ID, String IP, float distance = 0){
      
      switch(NODE_ID){
        case 1:
          NODE_1.id = NODE_ID;
          NODE_1.ip = IP;
          NODE_1.distance = distance;
          Serial.print("ID : ");
          Serial.print(NODE_1.id);
          Serial.print("\nIPAddress : ");
          Serial.print(NODE_1.ip);
          Serial.print("\nDistance From Vehicle : ");
          Serial.print(NODE_1.distance);
          Serial.print("\n");
          break;
        case 2:
          NODE_2.id = NODE_ID;
          NODE_2.ip = IP;
          NODE_1.distance = distance;
          Serial.print("ID : ");
          Serial.print(NODE_2.id);
          Serial.print("\nIPAddress : ");
          Serial.print(NODE_2.ip);
          Serial.print("\nDistance From Vehicle : ");
          Serial.print(NODE_2.distance);
          Serial.print("\n");
          break;
        case 3:
          NODE_3.id = NODE_ID;
          NODE_3.ip = IP;
          NODE_1.distance = distance;
          Serial.print("ID : ");
          Serial.print(NODE_3.id);
          Serial.print("\nIPAddress : ");
          Serial.print(NODE_3.ip);
          Serial.print("\nDistance From Vehicle : ");
          Serial.print(NODE_3.distance);
          Serial.print("\n");
          break;
        case 4:
          FIRE_SENSOR_NODE.id = NODE_ID;
          FIRE_SENSOR_NODE.ip = IP;
          FIRE_SENSOR_NODE.distance = distance;
          Serial.print("ID : ");
          Serial.print(FIRE_SENSOR_NODE.id);
          Serial.print("\nIPAddress : ");
          Serial.print(FIRE_SENSOR_NODE.ip);
          Serial.print("\nDistance From Vehicle : ");
          Serial.print(FIRE_SENSOR_NODE.distance);
          Serial.print("\n");
          break;
        default:
          return FALSE;
      }

      return TRUE;
    }
};

IPAddress local_IP(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);

SystemNetwork VehicleNetwork;

void setup() {
  // Set up pin modes
  pinMode(EN_A, OUTPUT);
  pinMode(EN_B, OUTPUT);
  pinMode(IN_1, OUTPUT);
  pinMode(IN_2, OUTPUT);
  pinMode(IN_3, OUTPUT);
  pinMode(IN_4, OUTPUT);
  pinMode(US_ECHO, INPUT);
  pinMode(US_TRIG, OUTPUT);
  pinMode(FLAME_SENSOR_1, INPUT);
  pinMode(FLAME_SENSOR_2, INPUT);
  pinMode(FLAME_SENSOR_3, INPUT);

  // Set up access point mode
  WiFi.softAPConfig(local_IP, gateway, subnet);

  while (!WiFi.softAP(ssid, password)) {
    Serial.println("Failed, trying again.");
    delay(100);
  }

  // Print the access point IP address
  Serial.begin(115200);
  Serial.print("Access Point IP address: ");
  Serial.println(WiFi.softAPIP());

  server.on("/initnode1", HTTP_POST, handleInitNode);
  server.on("/initnode2", HTTP_POST, handleInitNode);
  server.on("/initnode3", HTTP_POST, handleInitNode);
  server.on("/initnode4", HTTP_POST, handleInitNode);
  server.enableCORS(true);
  server.begin();
  
  // while(WiFi.softAPgetStationNum() < 4);

}

void loop() {
// LOOP
  server.handleClient();

  unsigned long currentTime = millis();

  if (currentTime - previousTime >= 1000) { // Time conditional to run the loop without blocking it. Avoiding use of delay()
    previousTime = currentTime;

    if(VehicleNetwork.NODE_1.ip != ""){
      String url1 = "http://" + VehicleNetwork.NODE_1.ip + ":80/getnodeprops";
      VehicleNetwork.getNodeProps(url1);
    }
    if(VehicleNetwork.NODE_2.ip != ""){
      String url2 = "http://" + VehicleNetwork.NODE_2.ip + ":80/getnodeprops";
      VehicleNetwork.getNodeProps(url2);
    }
    if(VehicleNetwork.NODE_3.ip != ""){
      String url3 = "http://" + VehicleNetwork.NODE_3.ip + ":80/getnodeprops";
      VehicleNetwork.getNodeProps(url3);
    }
    
  }

}

void handleInitNode(){
      if (server.hasArg("plain")== false){ //Check if body received
 
        server.send(200, "text/plain", "NO BODY AVAILABLE!");
        return;
 
      }

      String props;
      props = server.arg("plain");

      DynamicJsonDocument jsonDoc(100);
      DeserializationError error = deserializeJson(jsonDoc, props);

      if (error) {
        Serial.print("JSON parsing failed: ");
        Serial.println(error.c_str());
      } 
      else {
        String ip = jsonDoc["ip"];
        int id = jsonDoc["id"];
        VehicleNetwork.configureNode(id, ip);
      }
 
      server.send(200, "text/plain", "SUCCESS!");
}

// Sensor / Engine methods

int getDistance() {
	digitalWrite(US_TRIG, LOW);
	delayMicroseconds(2);
	digitalWrite(US_TRIG, HIGH);
	delayMicroseconds(10);
	digitalWrite(US_TRIG, LOW);
	long duration = pulseIn(US_ECHO, HIGH, 30000);
	int distance = duration * 0.034 / 2;
	return distance;
}


void moveForward() {
  digitalWrite(IN_1, HIGH);
  digitalWrite(IN_2, LOW);
  digitalWrite(IN_3, HIGH);
  digitalWrite(IN_4, LOW);
  analogWrite(EN_A, 255); // Set speed for motor A
  analogWrite(EN_B, 255); // Set speed for motor B
}

void moveBackward() {
  digitalWrite(IN_1, LOW);
  digitalWrite(IN_2, HIGH);
  digitalWrite(IN_3, LOW);
  digitalWrite(IN_4, HIGH);
  analogWrite(EN_A, 255); // Set speed for motor A
  analogWrite(EN_B, 255); // Set speed for motor B
}

void turnLeft(int angle = 0) {
  digitalWrite(IN_1, LOW);
  digitalWrite(IN_2, HIGH);
  digitalWrite(IN_3, HIGH);
  digitalWrite(IN_4, LOW);
  analogWrite(EN_A, 127); // Set speed for motor A
  analogWrite(EN_B, 127); // Set speed for motor B
}

void turnRight(int angle = 0) {
  digitalWrite(IN_1, HIGH);
  digitalWrite(IN_2, LOW);
  digitalWrite(IN_3, LOW);
  digitalWrite(IN_4, HIGH);
  analogWrite(EN_A, 127); // Set speed for motor A
  analogWrite(EN_B, 127); // Set speed for motor B
}

void stopVehicle() {
  digitalWrite(IN_1, LOW);
  digitalWrite(IN_2, LOW);
  digitalWrite(IN_3, LOW);
  digitalWrite(IN_4, LOW);
  analogWrite(EN_A, 0); // Set speed for motor A to 0 (stop)
  analogWrite(EN_B, 0); // Set speed for motor B to 0 (stop)
}