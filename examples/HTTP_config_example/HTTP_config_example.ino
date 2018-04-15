#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <HTTP_config_page.h>

// Create an instance of the server
// specify the port to listen on as an argument
ESP8266WebServer server(80);

const char* ssid = "*****";
const char* password = "*****";

//create a config page
HTTP_config_page settings("Example Sketch");

void handleSettings() {  
  // Send the response to the client
  settings.parse(&server);

  //print the settings
  char setting[100];
  settings.getField("test field 1", setting);
  Serial.print("test field 1: ");
  Serial.println(setting);
  settings.getField("test field 2", setting);
  Serial.print("test field 2: ");
  Serial.println(setting);
  Serial.print("Free Space: ");
  Serial.println(settings.getFreeBytes());
}
void handleNotFound() {
	server.send(404, "text/plain", "404 Not Found.");
}
void handleRoot() {
  handleSettings();
}

void setup() {
  Serial.begin(115200);
  delay(10);

  //set up EEPROM
  EEPROM.begin(512);

  //set up config page
  settings.addField("test field 1", TEXTBOX, "Default val");
  settings.addField("test field 2", CHECKBOX);
  
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
  server.on ( "/", handleRoot );
	server.onNotFound ( handleNotFound );
	server.begin();
  Serial.println("Server started");

  // Print the IP address
  Serial.println(WiFi.localIP());
}

void loop() {
  server.handleClient();
}
