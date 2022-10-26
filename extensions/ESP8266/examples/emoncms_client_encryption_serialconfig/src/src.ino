// Emoncms client with encryption and serial configuration

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <Crypto.h>
#include <base64.hpp>       // https://github.com/Densaugeo/base64_arduino
#include <EEPROM.h>

// Used for serial configuration
char input[64];
byte idx = 0;

// Config struct
struct {
    char signature[6];
    char ssid[33];
    char pass[33];
    char apikey[33];
} conf;

// Signature used to check EEPROM
const char signature[6] = "ECE01";

#define AES_KEY_LENGTH 16

uint8_t key[AES_KEY_LENGTH];

// This needs to be randomly generated
uint8_t iv[AES_KEY_LENGTH] = { 0x28,0x4E,0x3A,0xBE,0xE3,0x8E,0x88,0x0E,0x2D,0x4F,0x78,0xE3,0xA7,0xB8,0x9D,0x48 };

SHA256 sha256;

char nodestr[] = "node=emontx&data=";
char packet[128];
uint8_t packet_index = 0;

uint8_t data_ready = 0;

bool last_wifi_connected = false;

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

void eepromLoad(byte *dest, byte len) {
  for (byte i=0; i<len; i++) {
    dest[i] = EEPROM.read(i);
  }
}

void eepromSave(byte *dest, byte len) {
  for (byte i=0; i<len; i++) {
    EEPROM.write(i,dest[i]);
  }
  if (EEPROM.commit()) {
    Serial.println("Saved");
  }
}

void printConfig() {
  Serial.print("signature:");
  Serial.println(conf.signature);
  Serial.print("ssid:");
  Serial.println(conf.ssid);
  Serial.print("psk:");
  Serial.println(conf.pass);
  Serial.print("apikey:");
  Serial.println(conf.apikey);
}

bool set_conf(char* input, char *tag, char *conf_var, byte max_len) {
  byte tag_len = strlen(tag);
  if (strncmp(input,tag,tag_len)==0) {
        byte len = idx-tag_len;
        if (len>max_len) len = max_len;
        memset(conf_var, 0, max_len);
        strncpy(conf_var,input+tag_len,len);
        conf_var[len] = '\0';
        return true;
  }
  return false;      
}

void handle_conf(char* ptr) {
  if (set_conf(ptr,"ssid:",conf.ssid,32)) {
    Serial.print("Set:");
    Serial.println(conf.ssid);
  } else if (set_conf(ptr,"psk:",conf.pass,32)) {
    Serial.print("Set:");
    Serial.println(conf.pass); 
  } else if (set_conf(ptr,"apikey:",conf.apikey,32)) {
    Serial.print("Set:");
    Serial.println(conf.apikey);   
  } else if (strncmp(ptr,"save",4)==0) {
    eepromSave((byte *)&conf,sizeof(conf));

    // Only start WiFi if configuration is valid
    if (strlen(conf.ssid)>=4 && strlen(conf.pass)>=4 && strlen(conf.apikey)==32) {
      last_wifi_connected = false;
      Serial.println("Attempting to connect to WiFi");
      // Start WiFi with supplied parameters
      WiFi.begin(conf.ssid, conf.pass);
      // Convert apikey to binary
      hex2bin(conf.apikey, key, strlen(conf.apikey));
    } 
    
  } else if (strncmp(ptr,"load",4)==0) {
    eepromLoad((byte *)&conf,sizeof(conf));
    printConfig();
  }
}

void setup() {
  pinMode(LEDpin,OUTPUT);
  led_state(1);
  
  Serial.begin(115200);
  delay(1000);
  Serial.println("ESP8266 emoncms client with AES128CBC encryption");
  
  EEPROM.begin(128);

  eepromLoad((byte *)&conf,sizeof(conf));
  if (strncmp(conf.signature,signature,6)!=0) {
    Serial.println("Invalid signature");
    // Reset conf structure
    memset(&conf, 0, sizeof(conf));
    strncpy(conf.signature,signature,6);
  }
  printConfig();
  
  // Operate in WiFi Station mode
  WiFi.mode(WIFI_STA);

  // Only start WiFi if configuration is valid
  if (strlen(conf.ssid)>=4 && strlen(conf.pass)>=4 && strlen(conf.apikey)==32) {
    Serial.println("Attempting to connect to WiFi");  
    // Start WiFi with supplied parameters
    WiFi.begin(conf.ssid, conf.pass);
    // Convert apikey to binary
    hex2bin(conf.apikey, key, strlen(conf.apikey));
  }
  
  // Reset packet to nodestr
  memset(packet, 0, 128);
  strcpy(packet,nodestr);
  packet_index = strlen(nodestr);

  led_state(0);
}

void loop() {
 
  // Receive data from connected emontx
  while (Serial.available()) {
    char c = char(Serial.read());
    if (c=='\n') {
      if (strncmp(packet+strlen(nodestr),"MSG:",3)==0) {
        Serial.println(packet);
        data_ready = 1;
        led_flash(50);
      } else {
        handle_conf(packet+strlen(nodestr));
        memset(packet, 0, 128);
        strcpy(packet,nodestr);
        packet_index = strlen(nodestr);
        led_flash(500);
      }
    } else {
      packet[packet_index] = c;
      packet_index++;
    }
  }

  // Print WiFi connection status
  if (!last_wifi_connected) {
    if(WiFi.status()== WL_CONNECTED) {
      last_wifi_connected = true;
      
      // Connection established
      Serial.print("Connected to WiFi network ");
      Serial.println(WiFi.SSID());

      // Print IP Address
      Serial.print("Assigned IP Address: ");
      Serial.println(WiFi.localIP());

      // Test packet
      Serial.println("Sending test packet");
      memset(packet, 0, 128);
      strcpy(packet,nodestr);
      packet_index = strlen(nodestr);
      strcpy(packet+packet_index,"start:123");
      data_ready = 1;
      led_flash(1000);
    } else {
      led_flash(20);
      delay(100);
    }
  }
  
  // Send data if data is ready
  if (data_ready) {
    data_ready = 0;

    uint8_t packetSize = strlen(packet);
    if(WiFi.status()== WL_CONNECTED){
      
      WiFiClient client;
      HTTPClient http;
      
      // RNG::fill(iv, AES_KEY_LENGTH);
    
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
      Serial.println(serverPath);
      
      // Your Domain name with URL path or IP address with path
      http.begin(client,serverPath.c_str());
      http.addHeader(F("Authorization"),auth_header);
      http.addHeader(F("Content-Type"),"aes128cbc");
      
      // Send HTTP GET request
      int httpResponseCode = http.POST(encoded,encodedSize);
      
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
