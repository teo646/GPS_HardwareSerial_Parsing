#define _USE_MATH_DEFINES
#include <Adafruit_GPS.h>
#include <math.h>

// what's the name of the hardware serial port?
#define GPSSerial Serial1

// Connect to the GPS on the hardware port
Adafruit_GPS GPS(&GPSSerial);

// Set GPSECHO to 'false' to turn off echoing the GPS data to the Serial console
// Set to 'true' if you want to debug and listen to the raw GPS sentences
#define GPSECHO false

uint32_t gpsTimer = millis();
float last_latitude = 0;
float last_longitude = 0;
String gpsFileName = "";

void getGpsDataHandler(){
    if(gpsFileName == ""){
      Server.send(500, "application/json", "{\"error\":\"No GPS Fix Yet\"}");
      return;
    }
    Serial.println("reading file " + gpsFileName);
    File file = SD.open("/" + gpsFileName);
    Server.send(200, "application/json", "{}");
}

void gpsGetCurrentPosition(){
    Server.send(200, "application/json", gpsGetCurrentPositionJson());
}

float degreesToRadians(float degrees) {
  return degrees * M_PI / 180;
}

float distanceInKmBetweenEarthCoordinates(float lat1,float lon1,float lat2,float lon2) {
  int earthRadiusKm = 6371;
  float dLat = degreesToRadians(lat2-lat1);
  float dLon = degreesToRadians(lon2-lon1);
  lat1 = degreesToRadians(lat1);
  lat2 = degreesToRadians(lat2);
  float a = sin(dLat/2) * sin(dLat/2) + sin(dLon/2) * sin(dLon/2) * cos(lat1) * cos(lat2); 
  float c = 2 * atan2(sqrt(a), sqrt(1-a)); 
  return earthRadiusKm * c;
}

void appendLineToFile(){
  if(gpsFileName == ""){
      Server.send(500, "application/json", "{\"error\":\"No GPS Fix Yet\"}");
      return;
    }
  if(SD.exists("/" + gpsFileName)){
    File file = SD.open("/" + gpsFileName);
  } 
}

void gpsSetup() {
  // 9600 NMEA is the default baud rate for Adafruit MTK GPS's- some use 4800
  GPS.begin(9600);
  // uncomment this line to turn on RMC (recommended minimum) and GGA (fix data) including altitude
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  // uncomment this line to turn on only the "minimum recommended" data
  //GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCONLY);
  // For parsing data, we don't suggest using anything but either RMC only or RMC+GGA since
  // the parser doesn't care about other sentences at this time
  // Set the update rate
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ); // 1 Hz update rate
  // For the parsing code to work nicely and have time to sort thru the data, and
  // print it out we don't suggest using anything higher than 1 Hz

  // Request updates on antenna status, comment out to keep quiet
  GPS.sendCommand(PGCMD_ANTENNA);
  delay(1000);
  // Ask for firmware version
  GPSSerial.println(PMTK_Q_RELEASE);
}

void gpsHttpSetup() {
   Server.on("/getGpsData", getGpsDataHandler);
   Server.on("/getCurrentPosition", gpsGetCurrentPosition);
}

String gpsGetCurrentPositionJson(){
  if(GPS.fix){
    return "{\"valid\":true, \"lat\": "  + String(GPS.latitudeDegrees, 6) + ", \"lon\": "  + String(GPS.longitudeDegrees, 6) + ", \"day\": " + String(GPS.day) + ", \"hour\": " + String(GPS.hour) + ", \"minute\": " + String(GPS.minute) + ", \"ang\": " + String(GPS.angle, 6) + "}";
  }else{
    return "{\"valid\":false, \"lat\": "  + String(GPS.latitudeDegrees, 6) + ", \"lon\": "  + String(GPS.longitudeDegrees, 6) + ", \"day\": " + String(GPS.day) + ", \"hour\": " + String(GPS.hour) + ", \"minute\": " + String(GPS.minute) + ", \"ang\": " + String(GPS.angle, 6) + "}";
 
  }
}



void gpsLoop(){
  // read data from the GPS in the 'main loop'
  char c = GPS.read();
  // if you want to debug, this is a good time to do it!
  //if (GPSECHO)
  // if (c) Serial.print(c);
  // if a sentence is received, we can check the checksum, parse it...
  if (GPS.newNMEAreceived()) {
    // a tricky thing here is if we print the NMEA sentence, or data
    // we end up not listening and catching other sentences!
    // so be very wary if using OUTPUT_ALLDATA and trying to print out data
    char* NmeaString = GPS.lastNMEA();    // this GPS.lastNMEA() sets the newNMEAreceived() flag to false
    GPS.parse(NmeaString); //return; // we can fail to parse a sentence in which case we should just wait for another
    if (GPSECHO){
      Serial.println(NmeaString); // this also sets the newNMEAreceived() flag to false
    }    
  }
  // if millis() or gpsTimer wraps around, we'll just reset it
  if (gpsTimer > millis()) gpsTimer = millis();
  // approximately every 10 seconds or so, print out the current stats
  if (millis() - gpsTimer > 10000) {
    gpsTimer = millis();  // reset the gpsTimer
   //if (  distanceInKmBetweenEarthCoordinates(last_latitude, last_longitude, (GPS.latitudeDegrees, 6), (GPS.longitudeDegrees, 6))>0.01){ // make sure that the function is working   
     if(GPS.fix && gpsFileName == ""){
        gpsFileName = String(GPS.year) + String(GPS.month) + String(GPS.day) + "_" + String(GPS.hour) + "_" + String(GPS.minute) + "_" + String(GPS.seconds) + ".txt"; 
        String positionJson = gpsGetCurrentPositionJson();
        char* jsonBuffer = 0;
        jsonBuffer =  (char*) malloc(positionJson.length());
        positionJson.toCharArray(jsonBuffer, sizeof(jsonBuffer));
        writeSDFile(SD, "/" + gpsFileName, jsonBuffer);
        free(jsonBuffer);
     }else if(GPS.fix){
        String positionJson = gpsGetCurrentPositionJson();
        char* jsonBuffer = 0;
        jsonBuffer =  (char*) malloc(positionJson.length());
        positionJson.toCharArray(jsonBuffer, sizeof(jsonBuffer));
        appendSDFile(SD, "/" + gpsFileName, jsonBuffer);
        free(jsonBuffer);
     }else{
        Serial.println("No GPS Fix!");
     }
  
  
   /*
    Serial.print("\nTime: ");
    if (GPS.hour < 10) { Serial.print('0'); }
    Serial.print(GPS.hour, DEC); Serial.print(':');
    if (GPS.minute < 10) { Serial.print('0'); }
    Serial.print(GPS.minute, DEC); Serial.print(':');
    if (GPS.seconds < 10) { Serial.print('0'); }
    Serial.print(GPS.seconds, DEC); Serial.print('.');
    if (GPS.milliseconds < 10) {
      Serial.print("00");
    } else if (GPS.milliseconds > 9 && GPS.milliseconds < 100) {
      Serial.print("0");
    }
    Serial.println(GPS.milliseconds);
    Serial.print("Date: ");
    Serial.print(GPS.day, DEC); Serial.print('/');
    Serial.print(GPS.month, DEC); Serial.print("/20");
    Serial.println(GPS.year, DEC);
    Serial.print("Fix: "); Serial.print((int)GPS.fix);
    Serial.print(" quality: "); Serial.println((int)GPS.fixquality);
    if (GPS.fix) {
      Serial.print("Location: ");
      Serial.print(GPS.latitude, 4); Serial.print(GPS.lat);
      Serial.print(", ");
      Serial.print(GPS.longitude, 4); Serial.println(GPS.lon);
      Serial.print("Speed (knots): "); Serial.println(GPS.speed);
      Serial.print("Angle: "); Serial.println(GPS.angle);
      Serial.print("Altitude: "); Serial.println(GPS.altitude);
      Serial.print("Satellites: "); Serial.println((int)GPS.satellites);
    }*/
  } 
}
