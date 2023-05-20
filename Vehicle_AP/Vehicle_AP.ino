#include <ESP8266WiFi.h>
#include <math.h>

// Prototypes
bool checkConnection(const IPAddress& ip);

// Access Point Credentials
const char* ssid = "VehicleAP";
const char* password = "12345678";

// Nodes/Sensor IP Addresses
IPAddress NODE1_IP(192, 168, 1, 101);
IPAddress NODE2_IP(192, 168, 1, 102);
IPAddress NODE3_IP(192, 168, 1, 103);
IPAddress FIRE_SENSOR(192, 168, 1, 104);

// Coordinate System configuration - Node positions
struct Coordinates {
  float x;
  float y;
};

Coordinates NODE1_XY = { 0, -5.0 };
Coordinates NODE2_XY = { 0, 0 };
Coordinates NODE3_XY = { 10.0, 0 };
Coordinates FIRE_SENSOR_XY = { 6.5, -2 };   // Fire Sensor's coordinates likely to be pre-defined. It's hard to get both Vehicle's and Sensor's coordinates by Wi-Fi trilateration.

// System & Localization Node & Network Abstraction
struct TrilaterationNode {
  IPAddress ip;
  int id;
  bool connectionStatus;
  float distance = 0;
  float x;
  float y;
};

struct FireSensor {
  IDAddress ip;
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

    SystemNetwork(TrilaterationNode& node_1, TrilaterationNode& node_2, TrilaterationNode& node_3){
      
      if(!checkConnection(node_1.ip)) node_1.connectionStatus = 0;
      if(!checkConnection(node_2.ip)) node_2.connectionStatus = 0;
      if(!checkConnection(node_3.ip)) node_3.connectionStatus = 0;

      switch()

      NODE_1 = node_1;
      NODE_2 = node_2;
      NODE_3 = node_3;
    }

    void configureNodeProperties(){

    }

    void getDistanceMeasurement(int ESP_ID){

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