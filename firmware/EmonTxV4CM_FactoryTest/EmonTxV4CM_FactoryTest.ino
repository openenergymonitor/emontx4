/*
  emonTxV4.0 Factory Test
*/
#define Serial Serial3

// Comment/Uncomment as applicable
#define DEBUG                                              // Debug level print out

#define EMONHUBOEMEINTERFACER                              // Wired serial connection format: Use EMONHUBOEMEINTERFACER format if an ESP32 is used with emonhub. Default is space-separated values.

#define FACTORYTESTGROUP 1                                 // Transmit the Factory Test on Grp 1 
                                                           //   to avoid interference with recorded data at power-up.
#define OFF HIGH
#define ON LOW

#define RFM69CW

#include <Arduino.h>
#include <rfm69nTxLib.h>                                   // OEM RFM69CW transmit-only library using "JeeLib RFM69 Native" message format
#include <emonLibCM.h>                                     // OEM Continuous Monitoring library
// Include EmonTxV34CM_rfm69n_config.ino in the same directory for settings functions & data

// Radio - checks for traffic
const int busyThreshold = -97;                             // Signal level below which the radio channel is clear to transmit
const byte busyTimeout = 5;                                // Time in ms to wait for the channel to become clear, before transmitting anyway

typedef struct {
    unsigned long Msg;
    int Vrms,P1,P2,P3,P4,P5,P6;
    int T1,T2,T3;
    unsigned long pulse;
} PayloadTX;                                                  // create a data packet for the RFM
PayloadTX emontx;
static void showString (PGM_P s);
 
#define MAX_TEMPS 3                                        // The maximum number of temperature sensors
 
//---------------------------- emonTx Settings - Stored in EEPROM and shared with config.ino ------------------------------------------------
#define DEFAULT_VCAL 807.86
#define DEFAULT_ICAL 60.06                                // 25A / 333mV output = 75.075
#define DEFAULT_LEAD 3.2

struct {
  byte RF_freq = RFM_433MHZ;                               // Frequency of radio module can be RFM_433MHZ, RFM_868MHZ or RFM_915MHZ. 
  byte networkGroup = 210;                                 // wireless network group, must be the same as emonBase / emonPi and emonGLCD. OEM default is 210
  byte nodeID = 15;                                        // node ID for this emonTx.
  byte rf_on = 1;                                          // RF - 0 = no RF, 1 = RF on.
  byte rfPower = 25;                                       // 7 = -10.5 dBm, 25 = +7 dBm for RFM12B; 0 = -18 dBm, 31 = +13 dBm for RFM69CW. Default = 25 (+7 dBm)
  DeviceAddress allAddresses[MAX_TEMPS];                   // sensor address data
} EEProm;


DeviceAddress allAddresses[MAX_TEMPS];                     // Array to receive temperature sensor addresses
/*   Example - how to define temperature sensors, prevents an automatic search
DeviceAddress allAddresses[] = {       
    {0x28, 0x81, 0x43, 0x31, 0x7, 0x0, 0xFF, 0xD9}, 
    {0x28, 0x8D, 0xA5, 0xC7, 0x5, 0x0, 0x0, 0xD5},         // Use the actual addresses, as many as required
    {0x28, 0xC9, 0x58, 0x32, 0x7, 0x0, 0x0, 0x89}          // up to a maximum of 6    
};
*/

int allTemps[MAX_TEMPS];                                   // Array to receive temperature measurements

bool  USA=false;

bool calibration_enable = true;                           // Enable on-line calibration when running. 
                                                           // For safety, thus MUST default to false. (Required due to faulty ESP8266 software.)

//----------------------------emonTx V3 hard-wired connections-----------------------------------
const byte LEDpin      = PIN_PB2;  // emonTx V3 LED
const byte DIP_switch1 = PIN_PA4;  // RF node ID (default no change in node ID, switch on for nodeID + 1) switch off D8 is HIGH from internal pullup
const byte DIP_switch2 = PIN_PA5;  // Voltage selection 240 / 120 V AC (default switch off 240V)  - switch off D9 is HIGH from internal pullup

//---------------------------------CT availability status----------------------------------------
byte CT_count = 0;
bool CT1, CT2, CT3, CT4, CT5, CT6; // Record if CT present during startup


//----------------------------------------Setup--------------------------------------------------
void setup() 
{  
  //wdt_enable(WDTO_8S);
  
  pinMode(LEDpin, OUTPUT);
  digitalWrite(LEDpin,HIGH);
  
  pinMode(DIP_switch1, INPUT_PULLUP);
  pinMode(DIP_switch2, INPUT_PULLUP);
  
  // Serial---------------------------------------------------------------------------------
  Serial.begin(115200);

  // ---------------------------------------------------------------------------------------
  if (digitalRead(DIP_switch1)==ON) EEProm.nodeID++;                         // IF DIP switch 1 is switched on (LOW) then add 1 from nodeID

  Serial.println(F("emonTx V4 Factory Test"));

  rfm_init();                                                        // initialize RFM

  Serial.print(F("RFM69CW "));
  Serial.print(F(" Freq: "));
  Serial.print(F("433MHz"));
  Serial.print(F(" Group: ")); Serial.print(EEProm.networkGroup);
  Serial.print(F(" Node: ")); Serial.print(EEProm.nodeID);
  Serial.println(F(" "));
    
  digitalWrite(LEDpin,LOW);

  // ----------------------------------------------------------------------------
  // EmonLibCM config
  // ----------------------------------------------------------------------------
  // 12 bit ADC = 4096 divisions
  // Time in microseconds for one ADC conversion: 40 us 
  EmonLibCM_setADC(12,29.5);

  // Using AVR-DB 1.024V internal voltage reference
  EmonLibCM_ADCCal(1.024);
  
  EmonLibCM_SetADC_VChannel(0, DEFAULT_VCAL);                         // ADC Input channel, voltage calibration
  EmonLibCM_SetADC_IChannel(3, DEFAULT_ICAL, DEFAULT_LEAD);           // ADC Input channel, current calibration, phase calibration
  EmonLibCM_SetADC_IChannel(4, DEFAULT_ICAL, DEFAULT_LEAD);           // The current channels will be read in this order
  EmonLibCM_SetADC_IChannel(5, DEFAULT_ICAL, DEFAULT_LEAD);           
  EmonLibCM_SetADC_IChannel(6, DEFAULT_ICAL, DEFAULT_LEAD);           
  EmonLibCM_SetADC_IChannel(8, DEFAULT_ICAL, DEFAULT_LEAD);
  EmonLibCM_SetADC_IChannel(9, DEFAULT_ICAL, DEFAULT_LEAD);

  // mains frequency 50Hz
  EmonLibCM_datalog_period(1.8);                             // period of readings in seconds - normal value for emoncms.org  

  EmonLibCM_setAssumedVrms(240.0);

  EmonLibCM_setPulseEnable(true);                       // Enable pulse counting
  EmonLibCM_setPulsePin(PIN_PA6);
  EmonLibCM_setPulseMinPeriod(100);

  EmonLibCM_setTemperatureDataPin(PIN_PB4);                            // OneWire data pin (emonTx V3.4)
  EmonLibCM_setTemperaturePowerPin(PIN_PB3);                           // Temperature sensor Power Pin - 19 for emonTx V3.4  (-1 = Not used. No sensors, or sensor are permanently powered.)
  EmonLibCM_setTemperatureResolution(11);                              // Resolution in bits, allowed values 9 - 12. 11-bit resolution, reads to 0.125 degC
  EmonLibCM_setTemperatureAddresses(EEProm.allAddresses);              // Name of array of temperature sensors
  EmonLibCM_setTemperatureArray(allTemps);                             // Name of array to receive temperature measurements
  EmonLibCM_setTemperatureMaxCount(MAX_TEMPS);                         // Max number of sensors, limited by wiring and array size.
  
  long e0=0, e1=0, e2=0, e3=0, e4=0, e5=0;
  unsigned long p=0;

  EmonLibCM_TemperatureEnable(true);  
  EmonLibCM_Init();                                                    // Start continuous monitoring.
  emontx.Msg = 0;
  printTemperatureSensorAddresses();
}

void loop()             
{
  
  if (EmonLibCM_Ready())   
  {
    int acPresent_pass = 0;
    if (EmonLibCM_acPresent()) {
      acPresent_pass = 1;
    }

    emontx.Msg++;
    
    emontx.P1 = EmonLibCM_getMean(0);
    emontx.P2 = EmonLibCM_getMean(1);
    emontx.P3 = EmonLibCM_getMean(2);
    emontx.P4 = EmonLibCM_getMean(3);
    emontx.P5 = EmonLibCM_getMean(4); 
    emontx.P6 = EmonLibCM_getMean(5); 

    int ct_pass = 0;

    if (emontx.P1>1950 && emontx.P1<2100) {
      ct_pass++;
    }
    if (emontx.P2>1950 && emontx.P2<2100) {
      ct_pass++;
    }
    if (emontx.P3>1950 && emontx.P3<2100) {
      ct_pass++;
    }
    if (emontx.P4>1950 && emontx.P4<2100) {
      ct_pass++;
    }
    if (emontx.P5>1950 && emontx.P5<2100) {
      ct_pass++;
    }
    if (emontx.P6>1950 && emontx.P6<2100) {
      ct_pass++;
    }
    
    emontx.Vrms = EmonLibCM_getVrms() * 100;

    int voltage_pass = 0;
    if (emontx.Vrms>22000 && emontx.Vrms<26000) {
      voltage_pass = 1;
    }
    
    emontx.T1 = allTemps[0];
    emontx.T2 = allTemps[1];
    emontx.T3 = allTemps[2];

    emontx.pulse = EmonLibCM_getPulseCount();
        
    if (EEProm.rf_on)
    {
      PayloadTX tmp = emontx;
      rfm_send((byte *)&tmp, sizeof(tmp), EEProm.networkGroup, EEProm.nodeID, EEProm.RF_freq, EEProm.rfPower, busyThreshold, busyTimeout);     //send data
      delay(50);
    }
    Serial.print(F("MSG:")); Serial.print(emontx.Msg);
    Serial.print(F(",Vrms:")); Serial.print(emontx.Vrms*0.01);
    
    Serial.print(F(",P1:")); Serial.print(emontx.P1);
    Serial.print(F(",P2:")); Serial.print(emontx.P2);
    Serial.print(F(",P3:")); Serial.print(emontx.P3);
    Serial.print(F(",P4:")); Serial.print(emontx.P4);
    Serial.print(F(",P5:")); Serial.print(emontx.P5);
    Serial.print(F(",P6:")); Serial.print(emontx.P6);
     
    if (emontx.T1!=30000) { Serial.print(F(",T1:")); Serial.print(emontx.T1*0.01); }
    if (emontx.T2!=30000) { Serial.print(F(",T2:")); Serial.print(emontx.T2*0.01); }
    if (emontx.T3!=30000) { Serial.print(F(",T3:")); Serial.print(emontx.T3*0.01); }

    Serial.print(F(",pulse:")); Serial.print(emontx.pulse);

    Serial.print(F(",acpass:")); Serial.print(acPresent_pass);
    Serial.print(F(",vpass:")); Serial.print(voltage_pass);
    Serial.print(F(",ct_pass:")); Serial.print(ct_pass);


    Serial.println();
    delay(40);

    if (acPresent_pass && voltage_pass && ct_pass==6) {
        digitalWrite(LEDpin,LOW); delay(50);digitalWrite(LEDpin,HIGH);
    } else {
        digitalWrite(LEDpin,HIGH); delay(50);digitalWrite(LEDpin,LOW);
    }
  }
  delay(20);
}
