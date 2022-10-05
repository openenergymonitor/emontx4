#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include "SSD1306Wire.h"

SSD1306Wire display(0x3c, 21, 22, GEOMETRY_128_32);

const char* ssid = "";
const char* password = "";

String serverName = "http://emoncms.org/input/post";

String line = "";
String data = "";
String sub = "";
int data_ready = 0;

void setup() {
  Serial.begin(115200); 
  Serial2.begin(115200, SERIAL_8N1, 16, 17);

  display.init();
  display.flipScreenVertically();

  display_print("EmonTx v4");
  
  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  display_print("WiFi Connecting");
  
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
  
  display_print("WiFi Connected");
  display_print(WiFi.localIP().toString().c_str());
  
}

void loop() {

  while (Serial2.available()) {
  
    char c = char(Serial2.read());
  
    if (c=='\n') {
        data = line;
        data.trim();
        line = "";
        
        sub = data.substring(0, 3);
        if (sub=="MSG") {
            Serial.println(data);
            data_ready = 1;
        }
    } else {
        line = line + c; 
    }
  }

  //Send an HTTP POST request every 10 minutes
  if (data_ready) {
    data_ready = 0;
    //Check WiFi connection status
    if(WiFi.status()== WL_CONNECTED){
      HTTPClient http;

      String serverPath = serverName + "?node=emontx4&data="+data+"&apikey=APIKEY";
      Serial.println(serverPath);
      
      // Your Domain name with URL path or IP address with path
      http.begin(serverPath.c_str());
      
      // Send HTTP GET request
      int httpResponseCode = http.GET();
      
      if (httpResponseCode>0) {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
        String payload = http.getString();
        Serial.println(payload);
      }
      else {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
      }
      // Free resources
      http.end();
    }
    else {
      Serial.println("WiFi Disconnected");
    }
  }
}

void display_print(String str) {
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_16);
  display.drawString(0, 0, str);
  display.display();
}

