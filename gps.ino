
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
  float last_latitude = 0;
  float last_longitude = 0;
  unsigned int previousMinutes = minutes_time();
  uint32_t gpsTimer = millis();

/*[
       {"valid":true, "lat":37.727, "lng":-96.707, "date":"2020-05-12T19:21:26.000Z", "ang": 182},
       {"valid":true, "lat":38.971, "lng":-92.713, "date":"2020-05-12T19:21:26.000Z", "ang": 182},
             {"valid":true, "lat":36.172, "lng":-89.415, "date":"2020-05-12T19:21:26.000Z", "ang": 182}
       ];*/
void getGpsDataHandler(){
  String Date;
  //String Data;
  if (Server.arg("Date")== ""){     //Parameter not found
  File today = SD.open("/" + gpsGetGpsTimeStamp());
    if(!today){
        Serial.println("No Data for " + Date);
        Server.send(200, "text/json", "{\"valid\":true, \"exist\":false}");
        return;
    }

    Serial.println("Read from file");
   // while(file.available()){
     //   Data += char(file.read());
   // }
   while(today.available()){
        Serial.write(today.read());
    }
  
  Server.streamFile(today, "text/json");
  Server.send(200, "text/json", "]");
    today.close();
    //Server.send(200, "text/json", "[" + Data + "]");
   // Serial.println("[" + Data + "]");
  return;
  }else{     //Parameter found
  
  Date = Server.arg("Date");     //Gets the value of the query parameter
  
  }

    
     //File append = SD.open("/" + Date + ".json", FILE_APPEND);
     //append.println("]");
     //append.close();
     File file = SD.open("/" + Date + ".json");
    if(!file){
        Serial.println("No Data for " + Date);
        Server.send(200, "text/json", "{\"valid\":true, \"exist\":false}");
        return;
    }

    Serial.println("Read from file");
   // while(file.available()){
     //   Data += char(file.read());
   // }
   while(file.available()){
        Serial.write(file.read());
    }
  
  Server.streamFile(file, "text/json");
  //Server.send(200, "text/json", "]");
    file.close();
    //Server.send(200, "text/json", "[" + Data + "]");
   // Serial.println("[" + Data + "]");
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
}

String gpsGetGpsTimeStamp(){
  int differenceOfHours = 0;
  String Day;
  String Month;
  String Year = "20" + String(GPS.year);
  if(GPS.latitudeDegrees >= 0){
    differenceOfHours = GPS.latitudeDegrees/15;
  }else{
    differenceOfHours = 24 + GPS.latitudeDegrees/15;
    }

  int Hours = int(GPS.hour) - differenceOfHours;
  if(Hours > 0){
    Day = String(GPS.day);
  }
  else{
    Hours = Hours + 24;
    Day = String(GPS.day - 1);
  }

  if(((GPS.month == 2) || (GPS.month == 4) || (GPS.month == 6) || (GPS.month == 8) || (GPS.month == 9) || (GPS.month == 11) || (GPS.month == 1)) && Day == "0"){
      Day = "31";
      Month = String(GPS.month - 1);

  }else if(((GPS.month == 5) || (GPS.month == 7) || (GPS.month == 10) || (GPS.month == 12)) && Day == "0"){
      Day = "30";
      Month = String(GPS.month - 1);
  }else if((GPS.month == 3) && Day == "0"){
      Day = "28";
      Month = String(GPS.month - 1);
  }else{Month = String(GPS.month);
      }


  if (Month.length() == 1){
    Month = "0" + Month;      
  }

  if (Day.length() == 1){
    Day = "0" + Day;      
  }

    return Year +"-"+ Month +"-"+ Day +".json"; 
  }

unsigned int minutes_time(){
  int hours = int(GPS.hour);
  int minutes = int(GPS.minute);

  return hours*60 + minutes;
  
  }
String gpsGetCurrentPositionJson(unsigned int DifferenceOfTime){
    return "{\"valid\":true, \"exist\":true, \"lat\":"  + String(GPS.latitudeDegrees, 6) + ", \"lng\":"  + String(GPS.longitudeDegrees, 6) + ", \"time\":" + DifferenceOfTime + "}";
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
  if (millis() - gpsTimer > 8000) {
    gpsTimer = millis();  // reset the gpsTimer
    if(GPS.fix){
  if (  distanceInKmBetweenEarthCoordinates(last_latitude, last_longitude, GPS.latitudeDegrees, GPS.longitudeDegrees)>0.05){   
     if (!SD.open("/" + gpsGetGpsTimeStamp())){ 
        unsigned int DifferenceOfTime = minutes_time() - previousMinutes;
        previousMinutes = minutes_time();
        String positionJson = gpsGetCurrentPositionJson(DifferenceOfTime);
        writeSDFile(SD, "/" + gpsGetGpsTimeStamp(), positionJson);
     }else{
       unsigned int DifferenceOfTime = minutes_time() - previousMinutes;
       previousMinutes = minutes_time();
       String positionJson = gpsGetCurrentPositionJson(DifferenceOfTime);
       appendSDFile(SD, "/" + gpsGetGpsTimeStamp(), positionJson);
     }
     last_latitude = GPS.latitudeDegrees;
     last_longitude = GPS.longitudeDegrees;
  }else{
    Serial.println("didn't move a lot");
  }
    }else{
        Serial.println("No GPS Fix!~");
     }
   }
}
