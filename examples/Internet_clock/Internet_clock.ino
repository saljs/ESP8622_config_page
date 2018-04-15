#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <time.h>
#include <HTTP_config_page.h>

// Create an instance of the server
// specify the port to listen on as an argument
ESP8266WebServer server(80);

const char* ssid = "*****";
const char* password = "*****";

//create a config page
HTTP_config_page settings("Internet Clock");

//some global variables to set
bool DST = false;
short tzone = 0;

void handleSettings() {  
  // Send the response to the client
  settings.parse(&server);
  
  //update globals
  char temp[6];
  settings.getField("Use DST", temp);
  if(strcmp(temp, "true") == 0)
    DST = true;
  else
    DST = false;

  settings.getField("Time zone", temp);
  tzone = atoi(temp);
}
void handleNotFound() {
	server.send(404, "text/plain", "404 Not Found.");
}
void handleRoot() {
  time_t now;
  time(&now);
  server.send(200, "text/plain", String(ctime(&now)));
}
void handleSync() {
  getTime();
  handleRoot();
}

void setup() {
  Serial.begin(115200);
  delay(10);

  //set up EEPROM
  EEPROM.begin(512);

  //set up config page
  settings.addField("Time server", TEXTBOX, "time.nist.gov");
  settings.addField("Time zone", TEXTBOX, "0");
  settings.addField("Use DST", CHECKBOX);
  
  // Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  // Start the server
  server.on( "/", handleRoot);
  server.on("/settings", handleSettings);
  server.on("/sync", handleSync);
	server.onNotFound(handleNotFound);
	server.begin();
  Serial.println("Server started");

  // Print the IP address
  Serial.println(WiFi.localIP());
  getTime();
}

void loop() {
  server.handleClient();
}

void getTime() {
  char server[255];
  settings.getField("Time server", server);
  configTime(tzone*3600, 0, server);
  delay(500);
  time_t now;
  struct tm* timeinfo;
  time(&now);
  timeinfo = localtime(&now);  
  //quick and dirty DST
  if(DST && (timeinfo->tm_mon > 2 || (timeinfo->tm_mon == 2 && timeinfo->tm_mday - timeinfo->tm_wday >= 7))
      && (timeinfo->tm_mon < 10 || (timeinfo->tm_mon == 10 && (timeinfo->tm_mday >= 7 || timeinfo->tm_wday > 0))) ) {
        configTime(tzone*3600, 3600, server);
  }
}
