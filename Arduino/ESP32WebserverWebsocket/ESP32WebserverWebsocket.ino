// ---------------------------------------------------------------------------------------
//
// Code for a simple webserver on the ESP32 (device used for tests: ESP32-WROOM-32D).
// The code generates a random number on the ESP32 and uses Websockets to continuously
// update the web-clients. Please check out also the link below where this code is upgraded
// with JSON for more robust communication:
// https://github.com/mo-thunderz/Esp32WifiPart2/tree/main/Arduino/ESP32WebserverWebsocketJson
//
// For installation, the following libraries need to be installed:
// * Websockets by Markus Sattler (can be tricky to find -> search for "Arduino Websockets"
//
// Refer to https://youtu.be/15X0WvGaVg8
//
// Written by mo thunderz (last update: 27.08.2022)
//
// ---------------------------------------------------------------------------------------

#include <WiFi.h>              // needed to connect to WiFi
#include <WebServer.h>         // needed to create a simple webserver (make sure tools -> board is set to ESP32, otherwise you will get a "WebServer.h: No such file or directory" error)
#include <WebSocketsServer.h>  // needed for instant communication between client and server through Websockets
#include "DFRobot_LTR390UV.h"
DFRobot_LTR390UV ltr390(/*addr = */ LTR390UV_DEVICE_ADDR, /*pWire = */ &Wire);



// SSID and password of Wifi connection:
const char* ssid = "UVTester";
const char* password = "uvtester";

// The String below "webpage" contains the complete HTML code that is sent to the client whenever someone connects to the webserver
String website = "<!DOCTYPE html><html><head><title>UV measurement</title></head><body style='background-color: #EEEEEE;'><span style='color: #003366;'><h1>Let's measure the UV... </h1><p>The UV measurement is: <span id='rand'>-</span></p></span></body><script> var Socket; function init() { Socket = new WebSocket('ws://' + window.location.hostname + ':81/'); Socket.onmessage = function (event) { processCommand(event); }; } function processCommand(event) { document.getElementById('rand').innerHTML = event.data; console.log(event.data); } window.onload = function (event) { init(); }</script></html>";

// We want to periodically send values to the clients, so we need to define an "interval" and remember the last time we sent data to the client (with "previousMillis")
int interval = 1000;               // send data to the client every 1000ms -> 1s
unsigned long previousMillis = 0;  // we use the "millis()" command for time reference and this will output an unsigned long

// Initialization of webserver and websocket
WebServer server(80);                               // the server uses port 80 (standard port for websites
WebSocketsServer webSocket = WebSocketsServer(81);  // the websocket uses port 81 (standard port for websockets

void setup() {
  Serial.begin(115200);  // init serial port for debugging
  while (ltr390.begin() != 0) {
    Serial.println(" Sensor initialize failed!!");
    delay(1000);
  }
  Serial.println(" Sensor  initialize success!!");
  ltr390.setALSOrUVSMeasRate(ltr390.e18bit, ltr390.e100ms);  //18-bit data, sampling time of 100ms
  ltr390.setALSOrUVSGain(ltr390.eGain3);                     //Gain of 3
  ltr390.setMode(ltr390.eUVSMode);                           //Set UV mode

  Serial.println("\n[*] Creating AP");
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);  // start WiFi interface
  server.on("/", []() {                      // define here wat the webserver needs to do
    server.send(200, "text/html", website);  //    -> it needs to send out the HTML string "webpage" to the client
  });
  server.begin();  // start server

  webSocket.begin();                  // start websocket
  webSocket.onEvent(webSocketEvent);  // define a callback function -> what does the ESP32 need to do when an event from the websocket is received? -> run function "webSocketEvent()"
}

void loop() {

  server.handleClient();  // Needed for the webserver to handle all clients
  webSocket.loop();       // Update function for the webSockets

  //unsigned long now = millis();                             // read out the current "time" ("millis()" gives the time in ms since the Arduino started)
  // check if "interval" ms has passed since last time the clients were updated

  int data = 0;
  data = ltr390.readOriginalData();  //Get UV raw data
  Serial.print("data:");
  Serial.println(data);

  //String str = String(random(100));                 // create a random value
  //int str_len = data.length() + 1;
  //char char_array[data];
  //data.toCharArray(char_array);  // convert to char array
  //

  //
  String websocketStatusMessage = String(data);
  webSocket.broadcastTXT(websocketStatusMessage);  // send char_array to clients
  delay(1000);                                     // reset previousMillis
}

void webSocketEvent(byte num, WStype_t type, uint8_t* payload, size_t length) {  // the parameters of this callback function are always the same -> num: id of the client who send the event, type: type of message, payload: actual data sent and length: length of payload
  switch (type) {                                                                // switch on the type of information sent
    case WStype_DISCONNECTED:                                                    // if a client is disconnected, then type == WStype_DISCONNECTED
      Serial.println("Client " + String(num) + " disconnected");
      break;
    case WStype_CONNECTED:  // if a client is connected, then type == WStype_CONNECTED
      Serial.println("Client " + String(num) + " connected");
      // optionally you can add code here what to do when connected
      break;
    case WStype_TEXT:                     // if a client has sent data, then type == WStype_TEXT
      for (int i = 0; i < length; i++) {  // print received data from client
        Serial.print((char)payload[i]);
      }
      Serial.println("");
      break;
  }
}
