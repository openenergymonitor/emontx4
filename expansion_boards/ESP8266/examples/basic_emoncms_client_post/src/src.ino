#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>

// #define DEBUG
const char* ssid = "SSID";
const char* password = "passkey";
String serverName = "http://emoncms.org/input/post";
String apikey = "APIKEY";
String node = "emontx4";

String line = "";
String data = "";
String sub = "";
int data_ready = 0;

int LEDpin = 2;
unsigned long last_connection_attempt = 0;

void setup() {
  pinMode(LEDpin,OUTPUT);
  led_state(1);
   
  Serial.begin(115200);
  
  WiFi.begin(ssid, password);
  last_connection_attempt = millis();
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

  // If WiFi has disconnected, reconnect
  if (WiFi.status() != WL_CONNECTED) {
    if (millis() - last_connection_attempt > 30000) {
        last_connection_attempt = millis();
        #ifdef DEBUG
        Serial.println("WiFi disconnected, reconnecting");
        #endif
        WiFi.begin(ssid, password);
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

      // Your Domain name with URL path or IP address with path
      http.begin(client,serverName.c_str());

      // Set content type header
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");

      // Send HTTP POST request with URL-encoded data
      String postData = "node=" + node + "&data=" + data + "&apikey=" + apikey;
      #ifdef DEBUG
      Serial.println(postData);
      #endif
      int httpResponseCode = http.POST(postData);
      
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
