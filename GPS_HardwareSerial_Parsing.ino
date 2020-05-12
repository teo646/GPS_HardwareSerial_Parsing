#include <WiFi.h>
#include <WebServer.h>
#include <AutoConnect.h>
#include <SPI.h>

#include <Adafruit_GFX.h>
#include "Adafruit_LEDBackpack.h"

WebServer Server;
AutoConnect portal(Server);
AutoConnectConfig Config;

Adafruit_7segment matrix = Adafruit_7segment();

void sendRedirect(String uri) {
  WebServerClass& server = portal.host();
  server.sendHeader("Location", uri, true);
  server.send(302, "text/plain", "");
  server.client().stop();
}

bool atDetect(IPAddress softapIP) {
  Serial.println("Captive portal started, SoftAP IP:" + softapIP.toString());
  return true;
}




void setup()
{
   // connect at 115200 so we can read the GPS fast enough and echo without dropping chars
  // also spit it out
  Serial.begin(115200);
  matrix.begin(0x70);
  Config.boundaryOffset = 256;
  Config.autoReconnect = true;
  Config.ota=AC_OTA_BUILTIN;
  uint64_t chipid = ESP.getEfuseMac();
  Config.apid = "TeoGPS_" + String((uint32_t)chipid, HEX);
  Config.psk = "";
  Config.ticker = true;
  Config.tickerPort = LED_BUILTIN;
  Config.tickerOn = LOW;
  //Config.autoSave = AC_SAVECREDENTIAL_NEVER;
  portal.config(Config);
  
  portal.onDetect(atDetect);

  
  if ( portal.begin() ) {
    setupFileSystem();
    gpsSetup();
    setupHttpServer();
    gpsHttpSetup();
    Serial.println("Started, IP:" + WiFi.localIP().toString());
  }else {
    Serial.println("Connection failed.");
    while (true) { yield(); }
  }
 
}


void loop() // run over and over again
{
  portal.handleClient();
  gpsLoop();
  if (WiFi.status() == WL_IDLE_STATUS) {
    Serial.println("Wifi is Idle Reseting...");
    //ESP.restart();
    delay(100);
  }        
}
