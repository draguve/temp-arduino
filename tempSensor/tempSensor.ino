#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

//ntp stuff
#include <NTPClient.h>
#include <WiFiUdp.h>

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "ESPRotary.h";

#include "DHT.h"

#define DHTPIN D6   

#define DHTTYPE DHT11

#define ROTARY_PIN1 D0
#define ROTARY_PIN2 D1

String arr_days[]={"Sun","Mon","Tues","Wed","Thurs","Fri","Sat"};
String date_time;

ESPRotary r = ESPRotary(ROTARY_PIN1, ROTARY_PIN2);
DHT dht(DHTPIN, DHTTYPE);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 19800, 60000);


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

unsigned long last = 0;
const long waitTime = 1000;

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
  Serial.println(WiFi.localIP());

  if (MDNS.begin("draguve")) {
    Serial.println("MDNS responder started");
  }

  server.on("/", handleDisplay);

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
  //scrollText(WiFi.localIP().toString());

  dht.begin();
  timeClient.begin();
  last = millis();
}

void loop(void) {
  
  r.loop();
  server.handleClient();
  MDNS.update();

  //update loop
  if(millis()-last >= waitTime){
    timeClient.update();
    switch(getStatus()){
      case 0:
      tempPage();
      break;
      case 1:
      timeScreen("IN");
      break;
      case 2:
      timeScreen("US");
      break;
    }
    last=millis();
  }
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
    tempPage();
    break;
    case 1:
    timeClient.setTimeOffset(19800);
    timeScreen("IN");
    break;
    case 2:
    timeClient.setTimeOffset(-25200);
    timeScreen("US");
    break;
  }
  last=millis();
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

void tempPage(){
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float f = dht.readTemperature(true);

  // Check if any reads failed.
  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println("Error: couldn't get temprature and humidity");
  } else{
    // routine for converting temp/hum floats to char arrays
    char temp_buff[5]; char hum_buff[5];
    char temp_disp_buff[11] = "Tmp:";
    char hum_disp_buff[11] = "Hum:";
    
    // appending temp/hum to buffers
    dtostrf(t,2,1,temp_buff);
    strcat(temp_disp_buff,temp_buff);
    dtostrf(h,2,1,hum_buff);
    strcat(hum_disp_buff,hum_buff);
    
    // routine for displaying text for temp/hum readout
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(WHITE);
    display.setCursor(0,0);
    display.println(temp_disp_buff);
    display.println(hum_disp_buff);
    display.display();
  }
}

void timeScreen(String place){
  timeClient.update();
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0,0);
  int hh = timeClient.getHours();
  int mm = timeClient.getMinutes();
  int ss = timeClient.getSeconds();
  
  if(hh>12)
  {
    hh=hh-12;
    display.print(hh);
    display.print(":");
    display.print(mm);
    display.print(":");
    display.print(ss);
    display.println(" PM");
  }
  else
  {
    display.print(hh);
    display.print(":");
    display.print(mm);
    display.print(":");
    display.print(ss);
    display.println(" AM");   
  }

  int day = timeClient.getDay();
  display.println(arr_days[day] + " " + place);
  
  display.println(getDateString());
  display.display();
}

String getDateString() {
   time_t rawtime = timeClient.getEpochTime();
   struct tm * ti;
   ti = localtime (&rawtime);

   uint16_t year = ti->tm_year + 1900;
   String yearStr = String(year);

   uint8_t month = ti->tm_mon + 1;
   String monthStr = month < 10 ? "0" + String(month) : String(month);

   uint8_t day = ti->tm_mday;
   String dayStr = day < 10 ? "0" + String(day) : String(day);
   return dayStr+"/"+monthStr+"/"+yearStr;
}
