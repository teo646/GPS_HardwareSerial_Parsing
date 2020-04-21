#include <WiFi.h>
#include <WebServer.h>
#include <AutoConnect.h>

#include <SPI.h>
#include "FS.h"
#include "SD.h"
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

void writeFile(fs::FS &fs, String path, String Data){ //writeFile(SD, "/hello.txt", "Hello ");
    Serial.printf("Writing file: %s\n", path);

    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("Failed to open file for writing");
        return;
    }
    if(file.println(Data)){
        Serial.println("File written");
    } else {
        Serial.println("Write failed");
    }
    file.close();
}

void appendFile(fs::FS &fs, String path, String Data){
    Serial.printf("Appending to file: %s\n", path);

    File file = fs.open(path, FILE_APPEND);
    if(!file){
        Serial.println("Failed to open file for appending");
        return;
    }
    if(file.print(",") && file.println(Data)){
        Serial.println("Message appended");
    } else {
        Serial.println("Append failed");
    }
    file.close();
}


void setup()
{
   // connect at 115200 so we can read the GPS fast enough and echo without dropping chars
  // also spit it out
  Serial.begin(115200);
  matrix.begin(0x70);
  if(!SD.begin()){
    Serial.println("Card Mount Failed");
    return;
  }
  uint8_t cardType = SD.cardType();
  if(cardType == CARD_NONE){
      Serial.println("No SD card attached");
      return;
  }
  setupFileSystem();
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
  gpsSetup();
  portal.onDetect(atDetect);
  if ( portal.begin() ) {
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
