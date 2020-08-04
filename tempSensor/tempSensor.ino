#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "ESPRotary.h";


#define ROTARY_PIN1 D0
#define ROTARY_PIN2 D1

ESPRotary r = ESPRotary(ROTARY_PIN1, ROTARY_PIN2);

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for SSD1306 display connected using software SPI (default case):
#define OLED_MOSI   D7
#define OLED_CLK   D5
#define OLED_DC    D2
#define OLED_CS    D8
#define OLED_RESET D3
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT,
  OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);

#ifndef STASSID
#define STASSID "jaineetp2"
#define STAPSK  "draguve1"
#endif

int status = 0;

const char* ssid = STASSID;
const char* password = STAPSK;

ESP8266WebServer server(80);

const int led = 13;
int lines = 8;

void handleNotFound() {
  digitalWrite(led, 1);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  digitalWrite(led, 0);
}

void handleDisplay(){
  String message = "<form action=\"/display\" method=\"POST\">";
  message += "<input type=\"text\" name=\"text\"><br>";
  message += "<input type=\"submit\" value=\"Submit\">";
  message += "</form>";
  if(server.method()==HTTP_POST){
     for (uint8_t i = 0; i < server.args(); i++) {
      if(server.argName(i)=="text"){
        scrollText(server.arg(i));
      }
    }
  }
  server.send(200, "text/html", message);
}

void setup(void) {
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  //setup rotary encoder;
  r.setChangedHandler(rotate);
  r.setStepsPerClick(4);

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();
  
  pinMode(led, OUTPUT);
  digitalWrite(led, 0);
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  display.clearDisplay();
  Serial.println(WiFi.localIP());

  if (MDNS.begin("draguve")) {
    Serial.println("MDNS responder started");
  }

  server.on("/", handleDisplay);

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
  scrollText(WiFi.localIP().toString());
  
}

void loop(void) {
  r.loop();
  server.handleClient();
  MDNS.update();

  //update loop
//  switch(getStatus()){
//    case 0:
//    text("Case 0");
//    break;
//    case 1:
//    text("Case 1");
//    break;
//    case 2:
//    text("Case 2");
//    break;
//  }
}

// on change

void rotate(ESPRotary& r) {
  int dir = r.getDirection();
  if(dir==1){
    status = (status+1)%3;
  }else{
    status = (status-1)%3;
  }

  //set loop
  switch(getStatus()){
    case 0:
    text("Case 0");
    break;
    case 1:
    text("Case 1");
    break;
    case 2:
    text("Case 2");
    break;
  }
}

void text(String data){
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println(data);
  display.display();
}

int getStatus(){
  return abs(status);
}

void scrollText(String input) {
  int thisLine = 1+input.length()/21;
  if(lines==0 || lines-thisLine<0 ){
    lines = 8;
    display.clearDisplay();
    display.setCursor(0, 0);
  }
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  //display.setCursor(0, 0);
  display.println(input);
  display.display();      // Show initial text
  lines -= thisLine;
}
