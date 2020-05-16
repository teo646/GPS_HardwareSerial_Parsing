
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

void getGpsDataHandler(){
  //String DateRequested;
  //Serial.println(portal.host().client().read());
  //char c;
   //   while(DateRequested.length() < 50) {
        //c = char(portal.host().client().read());
        //DateRequested += c;
    //    DateRequested += char(portal.host().client().read());
    //  }
  //    if(DateRequested == ""){
   //     Serial.println("no data");
//}else{
  //      Serial.println(DateRequested);
   //     }
  File root = SD.open("/");
   if(!root){
        Serial.println("Failed to open directory");
        Server.send(500, "text/json", "{\"error\":\"No GPS Fix Yet\"}");
        return;
    }
    if(!root.isDirectory()){
        Serial.println("Not a directory");
        Server.send(500, "text/json", "{\"error\":\"No GPS Fix Yet\"}");
        return;
    }
    

    File file = root.openNextFile();
    
    Serial.println("reading data");
    String Data;
    while(file){
     while(file.available()){
       Data +=  char(file.read());
     }    
     // SD.remove("/" + String(file.name()));
       Data += ",";
       file = root.openNextFile();
}   Data = Data.substring(1, (Data.length() - 2));
   // Serial.println("no empty space in the sd card");
    Server.send(200, "text/json", "[" + Data + "]");
    Serial.println("[" + Data + "]");
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


void gpsSetup() {
  // 9600 NMEA is the default baud rate for Adafruit MTK GPS's- some use 4800
  GPS.begin(9600);
  delay(100);
  //PMTK_SET_BAUD_115200
  GPS.sendCommand(PMTK_SET_BAUD_115200);
  delay(100);
  GPS.begin(115200);
  delay(100);
  // uncomment this line to turn on RMC (recommended minimum) and GGA (fix data) including altitude
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  // uncomment this line to turn on only the "minimum recommended" data
  //GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCONLY);
  // For parsing data, we don't suggest using anything but either RMC only or RMC+GGA since
  // the parser doesn't care about other sentences at this time
  // Set the update rate
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_200_MILLIHERTZ); // 200 miliHz update rate
  GPS.sendCommand(PMTK_API_SET_FIX_CTL_200_MILLIHERTZ);
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

String gpsGetGpsTimeStamp(String type){
  int differenceOfHours = 0;
  int Day;
  int Month;
  int Year;
  if((GPS.latitudeDegrees, 6) > 0){
    differenceOfHours = (GPS.latitudeDegrees, 6)/15;
  }else{
    differenceOfHours = (360 + (GPS.latitudeDegrees, 6))/15;
    }

  int Seconds = int(GPS.seconds);
  int Minutes = int(GPS.minute);
  int Hours = int(GPS.hour) + differenceOfHours;
  if(Hours > 24){
    Hours = Hours -24;
    Day = int(GPS.day) + 1;
  }
  else{
    Day = int(GPS.day);
  }
  if((GPS.month == 2) && Day ==30){
     Day = 1;
     Month = 3;
    }
  else if((GPS.month == 12) && Day ==32){
     Day = 1;
     Month = 1;
     Year = 2001 + int(GPS.year);
    }
  else if(((GPS.month == 1) || (GPS.month == 3) || (GPS.month == 5) || (GPS.month == 7) || (GPS.month == 8) || (GPS.month == 10)) && Day == 32){
      Day = 1;
      Month = int(GPS.month) + 1;
    }
  else if(((GPS.month == 4) || (GPS.month == 6) || (GPS.month == 9) || (GPS.month == 11)) && Day == 31){
      Day = 1;
      Month = int(GPS.month) + 1;
    }
  else{
     Month = int(GPS.month);
     Year = 20 + int(GPS.year);
  }


  if(type == "name"){
    return String(Year) +"."+ String(Month) +"."+ String(Day) +".json"; 
  }else if(type == "data"){
    return String(Hours) + "." + String(Minutes) + "." + String(Seconds); 
  }else{
    Serial.println("specify time stamp type");
    return "";
}}
String gpsGetCurrentPositionJson(){
    return "{\"lat\":"  + String(GPS.latitudeDegrees, 6) + ", \"lng\":"  + String(GPS.longitudeDegrees, 6) + ", \"time\":\"" + gpsGetGpsTimeStamp("data") + "\", \"ang\": " + String(GPS.angle, 6) + "}";

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
    if(GPS.fix){
  if (  distanceInKmBetweenEarthCoordinates(last_latitude, last_longitude, (GPS.latitudeDegrees, 6), (GPS.longitudeDegrees, 6))>0.01){ // make sure that the function is working   
     if (exists(SD, "/" + gpsGetGpsTimeStamp("name"))){ 
        String positionJson = gpsGetCurrentPositionJson();
        appendSDFile(SD, "/" + gpsGetGpsTimeStamp("name"), positionJson);
        
     }else{
        String positionJson = gpsGetCurrentPositionJson();
        writeSDFile(SD, "/" + gpsGetGpsTimeStamp("name"), positionJson);

     }
     last_latitude = (GPS.latitudeDegrees, 6);
     last_longitude = (GPS.longitudeDegrees, 6);
  }
    }else{
        Serial.println("No GPS Fix!~");
     }
   }
}
