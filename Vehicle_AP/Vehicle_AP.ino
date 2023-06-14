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

#define MOVE_DURATION 750
#define TURN_DURATION 400

// Node Server URL's
#define NODE_CONFIG_URL "/getnodeprops"

unsigned long previousTime = 0;

// Prototypes

// Access Point Credentials
const char* ssid = "VehicleAP";
const char* password = "12345678";

ESP8266WebServer server(80);

// Navigation properties

struct Navigation {
  double currentDistance;
  double previousDistance = -1; // -1 means initialization. It indicates first movement is always will be Forward.
};

Navigation navigationControl;

// Coordinate System configuration - Node positions
struct Coordinates {
  double x;
  double y;
};

Coordinates NODE1_XY = { 0, -5.0 };
Coordinates NODE2_XY = { 0, 0 };
Coordinates NODE3_XY = { 10.0, 0 };
Coordinates FIRE_SENSOR_XY = { 6.5, -2 };   // Fire Sensor's coordinates likely to be pre-defined. It's hard to get both Vehicle's and Sensor's coordinates by Wi-Fi trilateration.

// System & Localization Node & Network Abstractions
struct TrilaterationNode {
  String ip = "";
  int id;
  double distance = 0;
  double x = 0;
  double y = 0;
};

struct Vehicle {
  double x;
  double y;
};

struct FireSensor {
  String ip = "";
  int id = 4;
  double distance = 0;
  int fire_state = FALSE;
  const double x = FIRE_SENSOR_XY.x;
  const double y = FIRE_SENSOR_XY.y;
};

struct NodeResponse {
  int id;
  const char* ip;
  double distance;
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
        DynamicJsonDocument jsonDoc(100); 
        DeserializationError error = deserializeJson(jsonDoc, response);

        if (error) {
          Serial.print("JSON parsing failed: ");
          Serial.println(error.c_str());
        } else {
          // Assign parsed JSON properties
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

    bool configureNode(int NODE_ID, String IP, double distance = 0){
      
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

    void setSensorProps(FireSensor sensorProps){
      FIRE_SENSOR_NODE.id = sensorProps.id;
      FIRE_SENSOR_NODE.ip = sensorProps.ip;
      FIRE_SENSOR_NODE.fire_state = sensorProps.fire_state;
    }

    void getAll(){
      if(NODE_1.ip != ""){
        String url1 = "http://" + NODE_1.ip + ":80/getnodeprops";
        getNodeProps(url1);
      }
      if(NODE_2.ip != ""){
        String url2 = "http://" + NODE_2.ip + ":80/getnodeprops";
        getNodeProps(url2);
      }
      if(NODE_3.ip != ""){
        String url3 = "http://" + NODE_3.ip + ":80/getnodeprops";
        getNodeProps(url3);
      }
      if(FIRE_SENSOR_NODE.ip != ""){
        String url4 = "http://" + FIRE_SENSOR_NODE.ip + ":80/getnodeprops";
        getNodeProps(url4);
      }
    }
    void updateVehicleLocation(){
    }

    // Method to perform 2D trilateration
    Coordinates trilateration() {

        double d12 = sqrt(pow((NODE_2.x - NODE_1.x), 2) + pow((NODE_2.y - NODE_1.y), 2));
        double d13 = sqrt(pow((NODE_3.x - NODE_1.x), 2) + pow((NODE_3.y - NODE_1.y), 2));
        double d23 = sqrt(pow((NODE_3.x - NODE_2.x), 2) + pow((NODE_3.y - NODE_2.y), 2));

        if (d12 <= NODE_1.distance + NODE_2.distance && d13 <= NODE_1.distance + NODE_3.distance && d23 <= NODE_2.distance + NODE_3.distance){

          Serial.println("Circles intersect! Trilateration implementing...");

          double r1sq = NODE_1.distance * NODE_1.distance;
          double r2sq = NODE_2.distance * NODE_2.distance;
          double r3sq = NODE_3.distance * NODE_3.distance;

          // Calculate coefficients for the linear system of equations
          double A = (-2 * NODE_1.x + 2 * NODE_2.x);
          double B = (-2 * NODE_1.y + 2 * NODE_2.y);
          double C = (NODE_1.distance*NODE_1.distance - NODE_2.distance*NODE_2.distance - NODE_1.x*NODE_1.x + NODE_2.x*NODE_2.x - NODE_1.y*NODE_1.y + NODE_2.y*NODE_2.y);
          double D = (-2 * NODE_2.x + 2 * NODE_3.x);
          double E = (-2 * NODE_2.y + 2 * NODE_3.y);
          double F = (NODE_2.distance*NODE_2.distance - NODE_3.distance*NODE_3.distance - NODE_2.x*NODE_2.x + NODE_3.x*NODE_3.x - NODE_2.y*NODE_2.y + NODE_3.y*NODE_3.y);

          // Calculate the coordinates of the trilateration point
          double denominator = A * E - B * D;
          double x = (C * E - B * F) / denominator;
          double y = (A * F - C * D) / denominator;

          // Return the trilateration point
          return {x, y};
        } else {
          Serial.println("Circles don't intersect. Operation terminated.");
          return {VEHICLE.x, VEHICLE.y};
        }
    }

    double getSensorDistance(){
      double distance = sqrt(pow((VEHICLE.x - FIRE_SENSOR_NODE.x), 2) + pow((VEHICLE.y - FIRE_SENSOR_NODE.y), 2));
      return distance;
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

  server.on("/sensorprops", HTTP_POST, handleSensorProps);

  server.enableCORS(true);
  server.begin();
  
  // while(WiFi.softAPgetStationNum() < 4);

}

// ----------- MAIN

void loop() {
// LOOP
  server.handleClient();

  unsigned long currentTime = millis();

  if (currentTime - previousTime >= 1000 && !VehicleNetwork.FIRE_SENSOR_NODE.fire_state) { // Time conditional to run the loop without blocking it. Avoiding use of delay()
    previousTime = currentTime;

    char buffer[100];

    VehicleNetwork.getAll();  // Fetch every NODE properties
    Coordinates temp = VehicleNetwork.trilateration();
    sprintf(buffer, "x : %lf\ny : %lf", temp.x, temp.y);
    Serial.println(buffer);
    
  }

  if(VehicleNetwork.FIRE_SENSOR_NODE.fire_state){

    VehicleNetwork.getAll();
    navigationControl.currentDistance = VehicleNetwork.getSensorDistance();

    if(navigationControl.previousDistance < 0){
      moveForward();
    }
    else if(abs(navigationControl.currentDistance - navigationControl.previousDistance) < 0.1){
      moveForward();
    }
    else if((navigationControl.currentDistance - navigationControl.previousDistance) < 0){
      moveForward();
    }
    else if((navigationControl.currentDistance - navigationControl.previousDistance) > 0){
      turnRight();
    }
    else if((navigationControl.currentDistance <=  0.3)){
      stopVehicle();
    }

    navigationControl.previousDistance = navigationControl.currentDistance;

  }

}

// ----------- MAIN

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

void handleSensorProps(){
    FireSensor sensorBuffer;

    if (server.hasArg("plain")== false){ //Check if body received
      server.send(200, "text/plain", "NO BODY AVAILABLE!");
      return;
    }

    String sensorProps;
    sensorProps = server.arg("plain");

    DynamicJsonDocument jsonDoc(100);
    DeserializationError error = deserializeJson(jsonDoc, sensorProps);

    if (error) {
      Serial.print("JSON parsing failed: ");
      Serial.println(error.c_str());    
    } 
    else {
      String ip = jsonDoc["ip"];
      int id = jsonDoc["id"];
      double fireState = jsonDoc["firestate"];
      sensorBuffer.ip = ip;
      sensorBuffer.id = id;
      sensorBuffer.fire_state = fireState;

      VehicleNetwork.setSensorProps(sensorBuffer);
    }

    server.send(200, "text/plain", "SUCCESS!");
}

// Sensor / Engine methods

double getDistance() {
	digitalWrite(US_TRIG, LOW);
	delayMicroseconds(2);
	digitalWrite(US_TRIG, HIGH);
	delayMicroseconds(10);
	digitalWrite(US_TRIG, LOW);
	long duration = pulseIn(US_ECHO, HIGH);
	double distance = duration * 0.034 / 2;
	return distance;
}


void moveForward() {
  digitalWrite(IN_1, HIGH);
  digitalWrite(IN_2, LOW);
  digitalWrite(IN_3, HIGH);
  digitalWrite(IN_4, LOW);
  analogWrite(EN_A, 255); // Set speed for motor A
  analogWrite(EN_B, 255); // Set speed for motor B
  delay(MOVE_DURATION);
  stopVehicle();
}

void moveBackward() {
  digitalWrite(IN_1, LOW);
  digitalWrite(IN_2, HIGH);
  digitalWrite(IN_3, LOW);
  digitalWrite(IN_4, HIGH);
  analogWrite(EN_A, 255); // Set speed for motor A
  analogWrite(EN_B, 255); // Set speed for motor B
  delay(MOVE_DURATION);
  stopVehicle();
}

void turnLeft() {
  digitalWrite(IN_1, LOW);
  digitalWrite(IN_2, HIGH);
  digitalWrite(IN_3, HIGH);
  digitalWrite(IN_4, LOW);
  analogWrite(EN_A, 127); // Set speed for motor A
  analogWrite(EN_B, 127); // Set speed for motor B
  delay(TURN_DURATION);
  stopVehicle();
}

void turnRight() {
  digitalWrite(IN_1, HIGH);
  digitalWrite(IN_2, LOW);
  digitalWrite(IN_3, LOW);
  digitalWrite(IN_4, HIGH);
  analogWrite(EN_A, 127); // Set speed for motor A
  analogWrite(EN_B, 127); // Set speed for motor B
  delay(TURN_DURATION);
  stopVehicle();
}

void stopVehicle() {
  digitalWrite(IN_1, LOW);
  digitalWrite(IN_2, LOW);
  digitalWrite(IN_3, LOW);
  digitalWrite(IN_4, LOW);
  analogWrite(EN_A, 0); // Set speed for motor A to 0 (stop)
  analogWrite(EN_B, 0); // Set speed for motor B to 0 (stop)
}