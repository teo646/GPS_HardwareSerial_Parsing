 #define FILESYSTEM SPIFFS
// You only need to format the filesystem once
#define FORMAT_FILESYSTEM false
#define useSDCard true
#if FILESYSTEM == FFat
#include <FFat.h>
#endif
#if FILESYSTEM == SPIFFS
#include <SPIFFS.h>
#endif
#include "FS.h"
#include "SD.h"

File fsUploadFile;
uint8_t sdCardType = CARD_NONE;

//format bytes
String formatBytes(size_t bytes) {
  if (bytes < 1024) {
    return String(bytes) + "B";
  } else if (bytes < (1024 * 1024)) {
    return String(bytes / 1024.0) + "KB";
  } else if (bytes < (1024 * 1024 * 1024)) {
    return String(bytes / 1024.0 / 1024.0) + "MB";
  } else {
    return String(bytes / 1024.0 / 1024.0 / 1024.0) + "GB";
  }
}

String getContentType(String filename) {
  if (Server.hasArg("download")) {
    return "application/octet-stream";
  } else if (filename.endsWith(".htm")) {
    return "text/html";
  } else if (filename.endsWith(".html")) {
    return "text/html";
  } else if (filename.endsWith(".css")) {
    return "text/css";
  } else if (filename.endsWith(".js")) {
    return "application/javascript";
  } else if (filename.endsWith(".png")) {
    return "image/png";
  } else if (filename.endsWith(".gif")) {
    return "image/gif";
  } else if (filename.endsWith(".jpg")) {
    return "image/jpeg";
  } else if (filename.endsWith(".ico")) {
    return "image/x-icon";
  } else if (filename.endsWith(".json")) {
    return "application/json";
  } else if (filename.endsWith(".xml")) {
    return "text/xml";
  } else if (filename.endsWith(".pdf")) {
    return "application/x-pdf";
  } else if (filename.endsWith(".zip")) {
    return "application/x-zip";
  } else if (filename.endsWith(".gz")) {
    return "application/x-gzip";
  }
  return "text/plain";
}

bool exists(fs::FS &fs, String path){
  bool yes = false;
  File file = fs.open(path, "r");
  if(!file.isDirectory()){
    yes = true;
  }
  file.close();
  return yes;
}

bool handleFileRead(String path) {
   if (path.startsWith("/sd/")==true){
    return handleFileReadFS(SD, path);
   }else{
    return handleFileReadFS(FILESYSTEM, path);
   }  
}

bool handleFileReadSD(String path) {
    return handleFileReadFS(SD, path);
}

bool handleFileReadFS(fs::FS &fs, String path) {
  Serial.println("handleFileRead: " + path);
  if (path.endsWith("/")) {
    path += "index.htm";
  }
  String contentType = getContentType(path);
  String pathWithGz = path + ".gz";
  if (exists(fs, pathWithGz) || exists(fs, path)) {
    if (exists(fs, pathWithGz)) {
      path += ".gz";
    }
    File file = fs.open(path, "r");
    Server.streamFile(file, contentType);
    file.close();
    return true;
  }
  return false;
}

void handleFileUpload() {
    handleFileUploadFS(FILESYSTEM);
}
void handleFileUploadSD() {
    handleFileUploadFS(SD);
}

void handleFileUploadFS(fs::FS &fs) {

  if (Server.uri() != "/edit") {
    return;
  }
  
  HTTPUpload& upload = Server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    String filename = upload.filename;
    if (!filename.startsWith("/")) {
      filename = "/" + filename;
    }
    Serial.print("handleFileUpload Name: "); Serial.println(filename);
    fsUploadFile = fs.open(filename, "w");
    filename = String();
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    //Serial.print("handleFileUpload Data: "); Serial.println(upload.currentSize);
    if (fsUploadFile) {
      fsUploadFile.write(upload.buf, upload.currentSize);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (fsUploadFile) {
      fsUploadFile.close();
    }
    Serial.print("handleFileUpload Size: "); Serial.println(upload.totalSize);
  }
}

void handleFileDelete() {
    handleFileDeleteFS(FILESYSTEM);
}

void handleFileDeleteSD() {
    handleFileDeleteFS(SD);
}

void handleFileDeleteFS(fs::FS &fs) {

  if (Server.args() == 0) {
    return Server.send(500, "text/plain", "BAD ARGS");
  }
  String path = Server.arg(0);
  Serial.println("handleFileDelete: " + path);
  if (path == "/") {
    return Server.send(500, "text/plain", "BAD PATH");
  }
  if (!exists(fs, path)) {
    return Server.send(404, "text/plain", "FileNotFound");
  }
  fs.remove(path);
  Server.send(200, "text/plain", "");
  path = String();
}

void handleFileCreate() {
    handleFileCreateFS(FILESYSTEM);
}

void handleFileCreateSD() {
    handleFileCreateFS(SD);
}

void handleFileCreateFS(fs::FS &fs) {

  if (Server.args() == 0) {
    return Server.send(500, "text/plain", "BAD ARGS");
  }
  String path = Server.arg(0);
  Serial.println("handleFileCreate: " + path);
  if (path == "/") {
    return Server.send(500, "text/plain", "BAD PATH");
  }
  if (exists(fs, path)) {
    return Server.send(500, "text/plain", "FILE EXISTS");
  }
  File file = fs.open(path, "w");
  if (file) {
    file.close();
  } else {
    return Server.send(500, "text/plain", "CREATE FAILED");
  }
  Server.send(200, "text/plain", "");
  path = String();
}

void handleFileList() {
    handleFileListFS(FILESYSTEM);
}

void handleFileListSD() {
    handleFileListFS(SD);
}

void handleFileListFS(fs::FS &fs) {
  
  if (!Server.hasArg("dir")) {
    Server.send(500, "text/plain", "BAD ARGS");
    return;
  }

  String path = Server.arg("dir");
  Serial.println("handleFileList: " + path);


  File root = fs.open(path);
  path = String();

  String output = "[";
  if(root.isDirectory()){
      File file = root.openNextFile();
      while(file){
          if (output != "[") {
            output += ',';
          }
          output += "{\"dev\":\"spiffs\", \"type\":\"";
          output += (file.isDirectory()) ? "dir" : "file";
          output += "\",\"name\":\"";
          output += String(file.name()).substring(1);
          output += "\"}";
          file = root.openNextFile();
      }
  }

  
  output += "]";
  Server.send(200, "text/json", output);
}



void listSDDir(fs::FS &fs, String dirname, uint8_t levels){
    Serial.println("Listing directory: " + dirname);
    if(sdCardType == CARD_NONE){
        Serial.println("No SD card attached");
        return;
    }
    File root = fs.open(dirname);
    if(!root){
        Serial.println("Failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        Serial.println("Not a directory");
        return;
    }

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if(levels){
                listSDDir(fs, file.name(), levels -1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("  SIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

void createSDDir(fs::FS &fs, String path){
    Serial.println("Creating Dir: " + path);
    if(sdCardType == CARD_NONE){
        Serial.println("No SD card attached");
        return;
    }
    if(fs.mkdir(path)){
        Serial.println("Dir created");
    } else {
        Serial.println("mkdir failed");
    }
}

void removeSDDir(fs::FS &fs, String path){
    Serial.println("Removing Dir: "  + path);
    if(sdCardType == CARD_NONE){
        Serial.println("No SD card attached");
        return;
    }
    if(fs.rmdir(path)){
        Serial.println("Dir removed");
    } else {
        Serial.println("rmdir failed");
    }
}

void readSDFile(fs::FS &fs, String path){
    Serial.println("Reading file: " + path);
    if(sdCardType == CARD_NONE){
        Serial.println("No SD card attached");
        return;
    }
    File file = fs.open(path);
    if(!file){
        Serial.println("Failed to open file for reading");
        return;
    }

    Serial.print("Read from file: ");
    while(file.available()){
        Serial.write(file.read());
    }
    file.close();
}

void writeSDFile(fs::FS &fs, String path, String message){
    Serial.println("Writing file: " +  path);
    if(sdCardType == CARD_NONE){
        Serial.println("No SD card attached");
        return;
    }
    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("Failed to open file for writing");
        return;
    }
    if(file.println(message)){
        Serial.println("File written");
    } else {
        Serial.println("Write failed");
    }
    file.close();
}

void appendSDFile(fs::FS &fs, String path, String message){
    Serial.println("Appending to SD file: " + path);
    if(sdCardType == CARD_NONE){
        Serial.println("No SD card attached");
        return;
    }
    File file = fs.open(path, FILE_APPEND);
    if(!file){
        Serial.println("Failed to open file for appending");
        return;
    }
    if(file.println("," + message)){
        Serial.println("Message appended");
    } else {
        Serial.println("Append failed");
    }
    file.close();
}

void renameSDFile(fs::FS &fs, String path1, String path2){
    Serial.println("Renaming file " +  path1 + " to " + path2);
    if(sdCardType == CARD_NONE){
        Serial.println("No SD card attached");
        return;
    }
    if (fs.rename(path1, path2)) {
        Serial.println("File renamed");
    } else {
        Serial.println("Rename failed");
    }
}

void deleteSDFile(fs::FS &fs, String path){
    Serial.println("Deleting file: " + path);
    if(sdCardType == CARD_NONE){
        Serial.println("No SD card attached");
        return;
    }
    if(fs.remove(path)){
        Serial.println("File deleted");
    } else {
        Serial.println("Delete failed");
    }
}




void setupFileSystem(){
  if (FORMAT_FILESYSTEM) FILESYSTEM.format();
  FILESYSTEM.begin();
  File root = FILESYSTEM.open("/");
  File file = root.openNextFile();
  while(file){
      String fileName = file.name();
      size_t fileSize = file.size();
      Serial.printf("SPIFFS File: %s, size: %s\n", fileName.c_str(), formatBytes(fileSize).c_str());
      file = root.openNextFile();
  }

  if(useSDCard){
    Serial.println("Checking for SD Card");
      if(SD.begin()){
        Serial.println("SD Card Adapter Found");
        sdCardType = SD.cardType();
        if(sdCardType == CARD_NONE){
            Serial.println("No SD card attached");
        }else{
          Serial.print("SD Card Type: ");
          if(sdCardType == CARD_MMC){
              Serial.println("MMC");
          } else if(sdCardType == CARD_SD){
              Serial.println("SDSC");
          } else if(sdCardType == CARD_SDHC){
              Serial.println("SDHC");
          } else {
              Serial.println("UNKNOWN");
          }
        
          Serial.printf("SD Card Size: %lluMB\n", SD.cardSize() / (1024 * 1024));
          Serial.printf("Total space: %lluMB\n", SD.totalBytes() / (1024 * 1024));
          Serial.printf("Used space: %lluMB\n", SD.usedBytes() / (1024 * 1024));
          listSDDir(SD, "/", 0);
        }
      }else{
        Serial.println("SD Card Adapter Not Found");
      }
  }
  
  Serial.printf("\n");
 
}
