#include <EEPROM.h>

char input[64];
byte idx = 0;

char nodestr[] = "node=emontx&data=";
char packet[128];
uint8_t packet_index = 0;
uint8_t data_ready = 0;

struct {
    char signature[6];
    char ssid[33];
    char pass[33];
    char apikey[33];
} conf;

const char signature[6] = "WSC01";

void setup() {
  // Start the Serial Monitor
  Serial.begin(115200);
  Serial1.begin(115200);

  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB
  } 
  Serial.println("WiFi Serial Config example");
  
  EEPROM.begin(128);

  eepromLoad((byte *)&conf,sizeof(conf));
  if (strncmp(conf.signature,signature,6)!=0) {
    Serial.println("Invalid signature");
    // Reset conf structure
    memset(&conf, 0, sizeof(conf));
    strncpy(conf.signature,signature,6);
  }
  printConfig();
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
  } else if (strncmp(ptr,"load",4)==0) {
    eepromLoad((byte *)&conf,sizeof(conf));
    printConfig();
  }
}

void loop() {

  // Receive data from connected emontx
  while (Serial1.available()) {
    char c = char(Serial1.read());
    if (c=='\n') {
      if (strncmp(packet+strlen(nodestr),"MSG:",3)==0) {
        // Serial.println(packet);
        data_ready = 1;
      } else {
        handle_conf(packet+strlen(nodestr));
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
    
    memset(packet, 0, 128);
    strcpy(packet,nodestr);
    packet_index = strlen(nodestr);
  }

  while (Serial.available()) {
    char c = char(Serial.read());
    if (c=='\n') {
      handle_conf(input);
      
      memset(input, 0, 64);
      idx = 0;
    } else {
      input[idx] = c;
      idx++;
    }
  }
}
