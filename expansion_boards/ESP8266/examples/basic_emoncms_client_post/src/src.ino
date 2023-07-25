#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>

// #define DEBUG
const char* ssid = "";
const char* password = "";

#define PACKET_LEN 384

char serverName[] = "http://emoncms.org/input/post";
char nodestr[] = "node=emontx4&apikey=APIKEY&data=";
char packet[PACKET_LEN];
uint8_t packet_index = 0;

int LEDpin = 2;
unsigned long last_connection_attempt = 0;

bool data_ready = false;
unsigned long esp_msg_count = 0;
unsigned long esp_recconect_count = 0;
bool connected = false;

void setup() {
  pinMode(LEDpin,OUTPUT);
  led_state(1);
   
  Serial.begin(115200);
  
  WiFi.begin(ssid, password);
  last_connection_attempt = millis();

  // Reset packet to nodestr
  memset(packet, 0, PACKET_LEN);
  strcpy(packet,nodestr);
  packet_index = strlen(nodestr);
}

void loop() {

  while (Serial.available()) {
    char c = char(Serial.read());
    if (c=='\n') {
      if (strncmp(packet+strlen(nodestr),"MSG:",3)==0) {
        #ifdef DEBUG
        Serial.println(packet);
        #endif
        data_ready = true;
        led_flash(50);
      } else {
        memset(packet, 0, PACKET_LEN);
        strcpy(packet,nodestr);
        packet_index = strlen(nodestr);
      }
    } else {
      if (packet_index < PACKET_LEN) {
        packet[packet_index] = c;
        packet_index++;
      }
    }
  }

  // If WiFi has disconnected, reconnect
  if (WiFi.status() != WL_CONNECTED) {
    connected = false;
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
    data_ready = false;
    esp_msg_count++;
    //Check WiFi connection status
    if(WiFi.status()== WL_CONNECTED){
      // If WiFi has just connected, increment esp_recconect_count
      if (!connected) {
        connected = true;
        esp_recconect_count++;
      }

      led_state(0);

      // Add esp_msg_count to packet string
      char msg_count_str[10];
      sprintf(msg_count_str, "%lu", esp_msg_count);
      strcat(packet,",esp_msg:");
      strcat(packet,msg_count_str);

      // Add esp_recconect_count to packet string
      char recconect_count_str[10];
      sprintf(recconect_count_str, "%lu", esp_recconect_count);
      strcat(packet,",esp_con:");
      strcat(packet,recconect_count_str);

      WiFiClient client;
      HTTPClient http;

      // Your Domain name with URL path or IP address with path
      http.begin(client,serverName);

      // Set content type header
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");

      // Send HTTP POST request with URL-encoded data
      #ifdef DEBUG
      Serial.println(packet);
      #endif
      // uint8_t packetSize = strlen(packet);

      int httpResponseCode = http.POST(packet);
      
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
    memset(packet, 0, PACKET_LEN);
    strcpy(packet,nodestr);
    packet_index = strlen(nodestr);
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
