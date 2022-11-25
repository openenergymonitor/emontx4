#include <WiFi.h>
#include <HTTPClient.h>

// Replace with your network credentials
const char* ssid = "SSID";
const char* password = "PASSKEY";

String serverName = "https://emoncms.org/input/post";

String line = "";
String data = "";
String sub = "";
int data_ready = 0;

void setup() {

  // Start the Serial Monitor
  Serial.begin(115200);

  Serial1.begin(115200);

  // Operate in WiFi Station mode
  WiFi.mode(WIFI_STA);

  // Start WiFi with supplied parameters
  WiFi.begin(ssid, password);

  // Print periods on monitor while establishing connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    delay(500);
  }

  // Connection established
  Serial.println("");
  Serial.print("Pico W is connected to WiFi network ");
  Serial.println(WiFi.SSID());

  // Print IP Address
  Serial.print("Assigned IP Address: ");
  Serial.println(WiFi.localIP());

}

void loop() {

  while (Serial1.available()) {
  
    char c = char(Serial1.read());
  
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

      String serverPath = serverName + "?node=pico2&data="+data+"&apikey=APIKEY";
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
