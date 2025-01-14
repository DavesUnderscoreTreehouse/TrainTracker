/******
 * David Thompson
 * Full project details at https://github.com/DavesUnderscoreTreehouse/TrainTracker
 * 
 * Web server code used to test from https://randomnerdtutorials.com/esp32-web-server-arduino-ide/
 ******/

// Load Arduino library
#include <Arduino.h>
// Load Wi-Fi library
#include <WiFi.h>

#include <WiFiManager.h>
// For configuring the Wifi credentials without re-programing
// Availalbe on library manager (WiFiManager)
// https://github.com/tzapu/WiFiManager

#include <ESP_DoubleResetDetector.h>
// For entering Config mode by pressing reset twice
// Available on the library manager (Double Reset Detector)
// https://github.com/datacute/DoubleResetDetector

// Network credentials
const char* ssid = "*ssid*";
const char* password = "*password";

// Web server port number
WiFiServer server(80);

// HTTP request holded
String header;

// Current output states
String output32State = "off";
String output33State = "off";

// Output pins
#define stripData 25
#define output32  32
#define output33  33

// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0; 
// Define timeout time in ms
const long timeoutTime = 2000;

// function declarations:
void flashTest();
void webServer();

void setup() {
  // Open serial port
  Serial.begin(115200);
  // Set pinmodes
  pinMode(stripData,OUTPUT);
  pinMode(output32, OUTPUT);
  pinMode(output33, OUTPUT);
  // Set outputs to LOW
  digitalWrite(output32, LOW);
  digitalWrite(output33, LOW);

  // Connect to WiFi with stored credentials
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();
}

void loop(){
  webServer();
}

// functions:
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