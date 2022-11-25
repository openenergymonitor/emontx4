#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>

// #define DEBUG

#include <Crypto.h>
#include <base64.hpp>       // https://github.com/Densaugeo/base64_arduino

const char *ssid = "";
const char *pass = "";
const char *apikey = "";

#define AES_KEY_LENGTH 16

uint8_t key[AES_KEY_LENGTH];

// This needs to be randomly generated
uint8_t iv[AES_KEY_LENGTH] = { 0x28,0x4E,0x3A,0xBE,0xE3,0x8E,0x88,0x0E,0x2D,0x4F,0x78,0xE3,0xA7,0xB8,0x9D,0x48 };

SHA256 sha256;

char nodestr[] = "node=emontx&data=";
char packet[128];
uint8_t packet_index = 0;
uint8_t data_ready = 0;

int LEDpin = 2;

void bin2hex(char*out, uint8_t* block, int length) {
  for (int i=0; i<length; i++) {
    sprintf(out,"%02x",*block);
    out += 2;
    block++;
  }
}

byte asc2byte(char chr) {
  byte rVal = 0;
  if (isdigit(chr)) {
    rVal = chr - '0';
  } else if (chr >= 'a' && chr <= 'f') {
    rVal = chr + 10 - 'a';
  }
  return rVal;
}

void hex2bin(const char* inP, byte* outP, size_t len) {
  for (; len > 1; len -= 2) {
    byte val = asc2byte(*inP++) << 4;
    *outP++ = val | asc2byte(*inP++);
  }
}

void setup() {
  pinMode(LEDpin,OUTPUT);
  led_state(1);
  
  Serial.begin(115200);
  #ifdef DEBUG
  Serial.println("ESP8266 emoncms client with AES128CBC encryption");
  #endif
  
  hex2bin(apikey, key, strlen(apikey));
  
  WiFi.begin(ssid, pass);
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
  // Reset packet to nodestr
  memset(packet, 0, 128);
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
        data_ready = 1;
        led_flash(50);
      } else {
        memset(packet, 0, 128);
        strcpy(packet,nodestr);
        packet_index = strlen(nodestr);
      }
    } else {
      packet[packet_index] = c;
      packet_index++;
    }
  }
  
  if (data_ready) {
    data_ready = 0;

    uint8_t packetSize = strlen(packet);
    
    if(WiFi.status()== WL_CONNECTED){
      led_state(0);
      
      WiFiClient client;
      HTTPClient http;
            
      RNG::fill(iv, AES_KEY_LENGTH);
    
      AES aes(key, iv, AES::AES_MODE_128, AES::CIPHER_ENCRYPT);
    
      int encryptedSize = aes.calcSizeAndPad(packetSize);
      int ivEncryptedSize = AES_KEY_LENGTH + encryptedSize;
      uint8_t ivEncrypted[ivEncryptedSize];
      // copy in the iv
      memcpy(ivEncrypted, iv, AES_KEY_LENGTH);
      // pointer that starts at end of iv
      uint8_t* encrypted = ivEncrypted + AES_KEY_LENGTH;
      // AES 128 CBC and pkcs7 padding
      aes.process((uint8_t*)packet, encrypted, packetSize);
      // base64 encoded
      int encodedSize = encode_base64_length(ivEncryptedSize); 
      uint8_t encoded[encodedSize];
      encode_base64(ivEncrypted, ivEncryptedSize, encoded);
    
      // hmac
      uint8_t computedHmac[32];
      SHA256HMAC hmac(key, AES_KEY_LENGTH);
      hmac.doUpdate(packet, packetSize+1); // +1 probably not needed but used for \0
      hmac.doFinal(computedHmac);
      // convert to hex  
      char auth_header[71] = "45448:";
      bin2hex(auth_header+6,computedHmac,32);
      // -----------------------------------
  
      String serverPath = "http://emoncms.org/input/post";
      #ifdef DEBUG
      Serial.println(serverPath);
      #endif
      // Your Domain name with URL path or IP address with path
      http.begin(serverPath.c_str());
      http.addHeader(F("Authorization"),auth_header);
      http.addHeader(F("Content-Type"),"aes128cbc");
      
      // Send HTTP GET request
      int httpResponseCode = http.POST(encoded,encodedSize);
      
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
    memset(packet, 0, 128);
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
