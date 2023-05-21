#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include <math.h>

// Node Server URL's
#define NODE_CONFIG_URL "/getnodeprops"

// Prototypes
bool checkConnection(const IPAddress& ip);

// Access Point Credentials
const char* ssid = "VehicleAP";
const char* password = "12345678";

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
  String ip;
  int id;
  bool connectionStatus;
  float distance = 0;
  const float x;
  const float y;
};

struct Vehicle {

  float x;
  float y;
};

struct FireSensor {
  String ip;
  bool connectionStatus;
  float humadityLevel;
  float tempertureLevel;
  float somekeLevel;
};

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
      String ip;
    };

    SystemNetwork(IPAddress* IPs, int stationCount){
      
      for(int i = 0; i < stationCount; i++){
        String url = "http://" + IPs[0].toString() + NODE_CONFIG_URL;
        NodeResponse response = getNodeConfiguration(url);

        switch(response.id){
          case 1:
            this->NODE_1.id = response.id;
            this->NODE_1.ip = response.ip;
            this->NODE_1.x = NODE1_XY.x;
            this->NODE_1.y = NODE1_XY.y;
            break;
          case 2:
            this->NODE_2.id = response.id;
            this->NODE_2.ip = response.ip;
            this->NODE_2.x = NODE1_XY.x;
            this->NODE_2.y = NODE1_XY.y;
            break;
          case 3:
            this->NODE_3.id = response.id;
            this->NODE_3.ip = response.ip;
            this->NODE_3.x = NODE1_XY.x;
            this->NODE_3.y = NODE1_XY.y;
            break;
          case 4:
            this->FIRE_SENSOR_NODE.id = response.id;
            this->FIRE_SENSOR_NODE.ip = response.ip;
            this->FIRE_SENSOR_NODE.x = FIRE_SENSOR_XY.x;
            this->FIRE_SENSOR_NODE.y = FIRE_SENSOR_XY.y;
            break;
        }
      }
    }

    void getDistanceMeasurements(int ESP_ID){

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

          // Print values
          Serial.print("ID: ");
          Serial.print(id);
          Serial.print(", IP: ");
          Serial.print(ip);

        }
      } else {
        Serial.print("HTTP request failed with error code: ");
        Serial.println(httpResponseCode);
      }

      http.end();

      return node;
    }
};

void setup() {
  // Set up access point mode
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);

  // Print the access point IP address
  Serial.begin(115200);
  Serial.print("Access Point IP address: ");
  Serial.println(WiFi.softAPIP());

  while(WiFi.softAPgetStationNum() < 4);
  
  int stationCount = WiFi.softAPgetStationNum();
  IPAddress* IPs = new IPAddress[stationCount];

  // Print the IP addresses of connected stations
  for (int i = 0; i < stationCount; i++) {
    IPAddress stationIP = WiFi.softAPgetStationIP(i);
    IPs[i] = stationIP;
    Serial.print("Station ");
    Serial.print(i + 1);
    Serial.print(" IP address: ");
    Serial.println(stationIP);
  }

  SystemNetwork SystemNetwork(IPs);


}

void loop() {
  // Get the number of connected stations
  int stationCount = WiFi.softAPgetStationNum();
  
  // Print the IP addresses of connected stations
  for (int i = 0; i < stationCount; i++) {
    IPAddress stationIP = WiFi.softAPgetStationIP(i);
    Serial.print("Station ");
    Serial.print(i + 1);
    Serial.print(" IP address: ");
    Serial.println(stationIP);
  }

  // Add a delay before checking the connections again
  delay(5000);
}

bool checkConnection(const IPAddress& ip) {
  int packetsReceived = WiFi.ping(ip);

  if (packetsReceived) {
    return true;
  } else {
    return false;
  }
}