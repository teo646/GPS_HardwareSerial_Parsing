void handleMenuHtml(void){
   Server.send(200, "text/html", AUTOCONNECT_LINK(BAR_32)); 
}

void setupHttpServer(){
  Server.on("/", HTTP_GET, handleRoot); 
  //SERVER INIT
  //get all GPIO statuses in one json call
  Server.on("/autoconnectMenu", HTTP_GET, handleMenuHtml);
  //list directory
  Server.on("/list", HTTP_GET, handleFileList);
  //load editor
  Server.on("/edit", HTTP_GET, []() {
    if (!handleFileRead("/edit.htm")) {
      Server.send(404, "text/plain", "FileNotFound");
    }
  });
  
  //create file
  Server.on("/edit", HTTP_PUT, handleFileCreate);
  //delete file
  Server.on("/edit", HTTP_DELETE, handleFileDelete);
  //first callback is called after the request has ended with all parsed arguments
  //second callback handles file uploads at that location
  Server.on("/edit", HTTP_POST, []() {
    Server.send(200, "text/plain", "");
  }, handleFileUpload);


 //SD list directory
  Server.on("/sd/list", HTTP_GET, handleFileListSD);
  //load SD editor
  Server.on("/sd/edit", HTTP_GET, []() {
    if (!handleFileRead("/edit.htm")) {
      Server.send(404, "text/plain", "FileNotFound");
    }
  });
  //create file
  Server.on("/sd/edit", HTTP_PUT, handleFileCreateSD);
  //delete file
  Server.on("/sd/edit", HTTP_DELETE, handleFileDeleteSD);
  //first callback is called after the request has ended with all parsed arguments
  //second callback handles file uploads at that location
  Server.on("/sd/edit", HTTP_POST, []() {
    Server.send(200, "text/plain", "");
  }, handleFileUploadSD);
  
  //called when the url is not defined here
  //use it to load content from FILESYSTEM
  Server.onNotFound([]() {
    Serial.println("File Not Found See if it is on SPI");
    if (!handleFileRead(Server.uri())) {
      Server.send(404, "text/plain", "File Not Found Andy");
    }
  });

  
}
