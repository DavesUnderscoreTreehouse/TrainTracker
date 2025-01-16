/******
 * David Thompson
 * Full project details at https://github.com/DavesUnderscoreTreehouse/TrainTracker
 * 
 * Web server code used to test from https://randomnerdtutorials.com/esp32-web-server-arduino-ide/
 ******/

//**** Standard Libraries ****//
#include <Arduino.h>
#include <WiFi.h>

//**** Other Libraries ****//
#include <WiFiManager.h>
// For configuring the Wifi credentials without re-programing
// https://github.com/tzapu/WiFiManager

#include <ESP_DoubleResetDetector.h>
// For entering Config mode by pressing reset twice
// https://github.com/datacute/DoubleResetDetector

#include <ArduinoHttpClient.h>
// For interacting with API
// https://github.com/arduino-libraries/ArduinoHttpClient

#include <ArduinoJson.h>
// For manipulating received JSONs
// https://github.com/bblanchon/ArduinoJson

#include <arduino_secrets.h>
// Contains #defines for SSID, WiFi password & API key
// NEED TO ADD YOU'RE OWN, NOT IN REPO


//**** Pins ****//
// Pin definitions
#define bootBtn   0
#define stripData 25
#define output32  32
#define output33  33

//**** Web Server ****//
// Web server port number
WiFiServer server(80);
// HTTP request holder
String header;
// Current output states
String output32State = "off";
String output33State = "off";
// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0; 
// Webserver timeout time in ms
const long timeoutTime = 2000;

//**** ISRs ****//
// ISR flags
bool ISR_bootBtnToggle = false;

//**** WiFiManager ****//

//**** Function declarations ****//
void flashTest();
void webServer();
void ISR_bootBtnFalling();

void setup() {
  // Open serial port
  Serial.begin(115200);
  Serial.setDebugOutput(true);  // enable debug output
  delay(5000);                  // Delay to open serial monitor
  Serial.println("\n Starting");
  
  // Set pinmodes
  pinMode(bootBtn,  INPUT);
  pinMode(stripData,OUTPUT);
  pinMode(output32, OUTPUT);
  pinMode(output33, OUTPUT);
  // Set interrupts
  attachInterrupt(bootBtn, ISR_bootBtnFalling, FALLING);
  // Set outputs to LOW
  digitalWrite(output32, LOW);
  digitalWrite(output33, LOW);

  // Initalise wifi manager library
  WiFiManager wm;
  wm.setClass("invert");          // dark theme
  wm.setWiFiAutoReconnect(true);  // set wifi to auto reconnect
  wm.setConnectTimeout(30);       // 10s wifi failed to connect timeout
  wm.setConfigPortalTimeout(180); // 180s config page timeout
  bool res = wm.autoConnect("ESP32 Trains", "ILikeTrain5"); // Network credentials of config network
  
  // Adding an additional config on the WIFI manager webpage for the API Key
  WiFiManagerParameter customApiKey("apiKey", "API Key", SECRET_APIKEY, 50);
  wm.addParameter(&customApiKey);

  // Print wifi connection status to serial
  Serial.println("");
  if(!res) {
    Serial.println("Failed to connect or hit timeout.");
    ESP.restart();      // If failed to connect at start-up reboot
  } 
  else
    Serial.println("WiFi connected.");

  // Print local IP address and start web server
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();

  // Reset flag after boot
  ISR_bootBtnToggle = false;
}

void loop(){
  if (ISR_bootBtnToggle == true){
    Serial.println("WiFi config portal launching...");
    WiFi.disconnect();
    WiFiManager wm;
    // Open WiFi config portal
    if (!wm.startConfigPortal("ESP32 Trains","ILikeTrain5")) {
        // Wifi failed to connect
        Serial.println("WiFi failed to connect or hit timeout");
        delay(3000);
        ESP.restart();
      } else {
        // WiFi connected
        Serial.println("WiFi connected");
      }
    ISR_bootBtnToggle = false;
  }
  webServer();
}

// Functions:
void flashTest(){
  digitalWrite(output32, HIGH);
  digitalWrite(output33, LOW);
  Serial.println("flash");
  delay(500);
  digitalWrite(output32, LOW);
  digitalWrite(output33, HIGH);
  Serial.println("flash");
  delay(500);
}

void webServer(){
  WiFiClient client = server.available();   // Listen for incoming clients

  if (client) {                             // If a new client connects,
    currentTime = millis();
    previousTime = currentTime;
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected() && currentTime - previousTime <= timeoutTime) {  // loop while the client's connected
      currentTime = millis();
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            
            // turns the GPIOs on and off
            if (header.indexOf("GET /32/on") >= 0) {
              Serial.println("GPIO 32 on");
              output32State = "on";
              digitalWrite(output32, HIGH);
            } else if (header.indexOf("GET /32/off") >= 0) {
              Serial.println("GPIO 32 off");
              output32State = "off";
              digitalWrite(output32, LOW);
            } else if (header.indexOf("GET /33/on") >= 0) {
              Serial.println("GPIO 33 on");
              output33State = "on";
              digitalWrite(output33, HIGH);
            } else if (header.indexOf("GET /33/off") >= 0) {
              Serial.println("GPIO 33 off");
              output33State = "off";
              digitalWrite(output33, LOW);
            }
            
            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style the on/off buttons 
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #555555;}</style></head>");
            
            // Web Page Heading
            client.println("<body><h1>ESP32 Web Server</h1>");
            
            // Display current state, and ON/OFF buttons for GPIO 32  
            client.println("<p>GPIO 32 - State " + output32State + "</p>");
            // If the output32State is off, it displays the ON button       
            if (output32State=="off") {
              client.println("<p><a href=\"/32/on\"><button class=\"button\">ON</button></a></p>");
            } else {
              client.println("<p><a href=\"/32/off\"><button class=\"button button2\">OFF</button></a></p>");
            } 
               
            // Display current state, and ON/OFF buttons for GPIO 33  
            client.println("<p>GPIO 33 - State " + output33State + "</p>");
            // If the output33State is off, it displays the ON button       
            if (output33State=="off") {
              client.println("<p><a href=\"/33/on\"><button class=\"button\">ON</button></a></p>");
            } else {
              client.println("<p><a href=\"/33/off\"><button class=\"button button2\">OFF</button></a></p>");
            }
            client.println("</body></html>");
            
            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}

void ISR_bootBtnFalling() {
  ISR_bootBtnToggle = true;
}