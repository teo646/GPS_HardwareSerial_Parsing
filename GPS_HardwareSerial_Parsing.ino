#include <WiFi.h>
#include <WebServer.h>
#include <AutoConnect.h>
#include <SPI.h>
#include <ESPmDNS.h>


WebServer Server;
AutoConnect portal(Server);
AutoConnectConfig Config("", "");
const char* mDnsHostName = "Teo";  

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
  Config.autoRise = true;
  Config.boundaryOffset = 256;
  Config.autoReconnect = true; //
  Config.ota=AC_OTA_BUILTIN;
  uint64_t chipid = ESP.getEfuseMac();  //
  Config.apid = "TeoGPS_" + String((uint32_t)chipid, HEX); // //
  Config.apip = IPAddress(192,168,10,1);      // Sets SoftAP IP address
  Config.gateway = IPAddress(0,0,0,0);     // Sets WLAN router IP address
  Config.netmask = IPAddress(255,255,255,0);    // Sets WLAN scope
  Config.psk = "12345678";  // The lenght should be from 8 to up to 63.
  Config.ticker = true;  //
  Config.tickerPort = LED_BUILTIN;//
  Config.tickerOn = LOW;//
  Config.homeUri = "/";
  //Config.immediateStart = true;
  Config.retainPortal = true;
  Config.portalTimeout = 1000;
  Config.bootUri = AC_ONBOOTURI_HOME;
  Config.autoReset=false; //
  //Config.autoSave = AC_SAVECREDENTIAL_NEVER;
  portal.config(Config);
  portal.onDetect(atDetect); 
  Serial.println("Starting File System."); 
  setupFileSystem();
  delay(100);
  Serial.println("Starting Http Server.");
  setupHttpServer();
  delay(100);
  Serial.println("Starting GPS.");
  gpsSetup();
  delay(100);
  Serial.println("Starting GPS Http.");
  gpsHttpSetup();
  delay(100);
  Serial.println("Portal Being.");
  MDNS.begin(mDnsHostName);
  MDNS.setInstanceName("teo's gps tracker");
  MDNS.addService("_http", "_tcp", 80);
  if ( portal.begin() ){
    Serial.println("Started, IP:" + WiFi.localIP().toString());
  }else {
    Serial.println("Connection failed.");
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
