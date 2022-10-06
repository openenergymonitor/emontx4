#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>

// #define DEBUG

const char* ssid = "SSID";
const char* password = "PSK";
String serverName = "http://emoncms.org/input/post";
String apikey = "APIKEY";

String line = "";
String data = "";
String sub = "";
int data_ready = 0;

int LEDpin = 2;

void setup() {
  pinMode(LEDpin,OUTPUT);
  led_state(1);
   
  Serial.begin(115200);
  
  WiFi.begin(ssid, password);
  #ifdef DEBUG
  Serial.println("Connecting");
  #endif
  
  led_state(0);
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    #ifdef DEBUG
    Serial.print(".");
    #endif
    led_flash(50);
  }
  
  #ifdef DEBUG
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
  #endif
  led_flash(500);
}

void loop() {

  while (Serial.available()) {
  
    char c = char(Serial.read());
  
    if (c=='\n') {
        data = line;
        data.trim();
        line = "";
        
        sub = data.substring(0, 3);
        if (sub=="MSG") {
            #ifdef DEBUG
            Serial.println(data);
            #endif
            data_ready = 1;
            led_flash(50);
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
      led_state(0);
      
      WiFiClient client;
      HTTPClient http;

      String serverPath = serverName + "?node=emontx4&data="+data+"&apikey="+apikey;
      #ifdef DEBUG
      Serial.println(serverPath);
      #endif
      // Your Domain name with URL path or IP address with path
      http.begin(client, serverPath.c_str());
      
      // Send HTTP GET request
      int httpResponseCode = http.GET();
      
      if (httpResponseCode>0) {
        #ifdef DEBUG
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
        String payload = http.getString();
        Serial.println(payload);
        #endif
        led_flash(5);
      }
      else {
        #ifdef DEBUG
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
        #endif
        led_flash(1000);
      }
      // Free resources
      http.end();
    }
    else {
      led_state(1);
      
      #ifdef DEBUG
      Serial.println("WiFi Disconnected");
      #endif
    }
  }
}

void led_flash(int ton) {
  led_state(1); delay(ton); led_state(0);
}

void led_state(int on) {
  if (on) {
    digitalWrite(LEDpin,LOW);
  } else {
    digitalWrite(LEDpin,HIGH); 
  }
}
