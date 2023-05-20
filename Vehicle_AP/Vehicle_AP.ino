#include <ESP8266WiFi.h>
#include <ESP8266WiFiAP.h>
#include <ESP8266WiFiGeneric.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266WiFiScan.h>
#include <ESP8266WiFiSTA.h>
#include <ESP8266WiFiType.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <WiFiServer.h>
#include <WiFiUdp.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h> 
#include <ESP8266WebServer.h>
#include "SystemLibrary.h"

#define APSSID "VehicleAP" // SSID & Title
#define APPASSWORD "12345678" // Blank password = Open AP

// Network Config
IPAddress APIP(192, 168, 2, 1);
DNSServer dnsServer; ESP8266WebServer webServer(80);

// Time Config
int controlTimeFlagState = 0;
int currentTimeFlagState = 0; // Current time flag in terms of ms
int duration = 100; // ms

void setup() {
  
  Serial.begin(9600);
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(APIP, APIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(APSSID, APPASSWORD);

  webServer.on("/",[]() { 
    Serial.println("EVET!");
    myFunc(5);
    webServer.send(200, "text/html", "Success!");
  });

  // webServer.on("/1ON",[]() { 
  //   digitalWrite(13, HIGH);
  //   webServer.send(200, "text/html", index()); 
  // });
  webServer.onNotFound([]() {
    Serial.println("NOT FOUND!");
  });

  webServer.begin();
  digitalWrite(16, LOW); // The built-in led will notify us that the setup is finished.
  digitalWrite(16, HIGH);
}


void loop() {

  currentTimeFlagState = millis();

  if(currentTimeFlagState % duration == 10){
    Serial.println(currentTimeFlagState);


    controlTimeFlagState = currentTimeFlagState;
  }

  dnsServer.processNextRequest(); 
  webServer.handleClient();
}












