/*
  emonTxV4.0 Continuous Sampling
  using EmonLibCM https://github.com/openenergymonitor/EmonLibCM
  Authors: Robin Emley, Robert Wall, Trystan Lea
  
  -----------------------------------------
  Part of the openenergymonitor.org project
  Licence: GNU GPL V3



Change Log:
v1.0: First release of EmonTxV3 Continuous Monitoring Firmware.
v1.1: First stable release, Set default node to 15
v1.2: Enable RF startup test sequence (factory testing), Enable DEBUG by default to support EmonESP
v1.3: Inclusion of watchdog
v1.4: Error checking to EEPROM config
v1.5: Faster RFM factory test
v1.6: Removed reliance on full jeelib for RFM, minimal rfm_send function implemented instead, thanks to Robert Wall
v1.7: Check radio channel is clear before transmit
v1.8: PayloadTx.E1 etc were unsigned long. 
v1.9: Unused variables removed.

v1.0.0: First release for Jeelib "RFM69 Native" format.
*/
#define Serial Serial3

const char *firmware_version = {"1.0.0\n\r"};
/*

emonhub.conf node decoder (nodeid is 15 when switch is off, 16 when switch is on)
See: https://github.com/openenergymonitor/emonhub/blob/emon-pi/configuration.md
copy the following into emonhub.conf:

[[15]]
  nodename = emonTx4cm15
  [[[rx]]]
    names = MSG, Vrms, P1, P2, P3, P4, P5, P6, E1, E2, E3, E4, E5, E6, T1, T2, T3, pulse
    datacodes = L,h,h,h,h,h,h,h,l,l,l,l,l,l,h,h,h,L
    scales = 1,0.01,1,1,1,1,1,1,1,1,1,1,0.01,0.01,0.01,1
    units = n,V,W,W,W,W,W,W,Wh,Wh,Wh,Wh,Wh,Wh,C,C,C,p

*/
// Comment/Uncomment as applicable
#define DEBUG                                              // Debug level print out

// #define EEWL_DEBUG

#define OFF HIGH
#define ON LOW

#define RFM69CW

#include <Arduino.h>
#include <avr/wdt.h>
#include <rfm69nTxLib.h>                                   // OEM RFM69CW transmit-only library using "JeeLib RFM69 Native" message format
#include <emonEProm.h>                                     // OEM EEPROM library
#include <emonLibCM.h>                                     // OEM Continuous Monitoring library
// Include EmonTxV34CM_rfm69n_config.ino in the same directory for settings functions & data

// Radio - checks for traffic
const int busyThreshold = -97;                             // Signal level below which the radio channel is clear to transmit
const byte busyTimeout = 5;                               // Time in ms to wait for the channel to become clear, before transmitting anyway

typedef struct {
    unsigned long Msg;
    int Vrms,P1,P2,P3,P4,P5,P6; 
    long E1,E2,E3,E4,E5,E6; 
    int T1,T2,T3;
    unsigned long pulse;
} PayloadTX;                                                  // create a data packet for the RFM
PayloadTX emontx;
static void showString (PGM_P s);
 
#define MAX_TEMPS 3                                        // The maximum number of temperature sensors
 
//---------------------------- emonTx Settings - Stored in EEPROM and shared with config.ino ------------------------------------------------
#define DEFAULT_ICAL 60.06                                // 25A / 333mV output = 75.075
#define DEFAULT_LEAD 3.2

struct {
  byte RF_freq = RFM_433MHZ;                               // Frequency of radio module can be RFM_433MHZ, RFM_868MHZ or RFM_915MHZ. 
  byte networkGroup = 210;                                 // wireless network group, must be the same as emonBase / emonPi and emonGLCD. OEM default is 210
  byte nodeID = 15;                                        // node ID for this emonTx.
  byte rf_on = 1;                                          // RF - 0 = no RF, 1 = RF on.
  byte rfPower = 25;                                       // 7 = -10.5 dBm, 25 = +7 dBm for RFM12B; 0 = -18 dBm, 31 = +13 dBm for RFM69CW. Default = 25 (+7 dBm)
  float vCal  = 807.86;                                    // (6 x 10000) / 75 = 800.0
  float assumedVrms = 240.0;                               // Assumed Vrms when no a.c. is detected
  float lineFreq = 50;                                     // Line Frequency = 50 Hz

  float i1Cal =  300.30;
  float i1Lead = DEFAULT_LEAD;
  float i2Cal =  150.15;
  float i2Lead = DEFAULT_LEAD;
  float i3Cal =  150.15;
  float i3Lead = DEFAULT_LEAD;
  float i4Cal =  DEFAULT_ICAL;                                     
  float i4Lead = DEFAULT_LEAD;                                      
  float i5Cal =  DEFAULT_ICAL;
  float i5Lead = DEFAULT_LEAD;
  float i6Cal =  DEFAULT_ICAL;
  float i6Lead = DEFAULT_LEAD;
  
  float period = 9.8;                                      // datalogging period - should be fractionally less than the PHPFINA database period in emonCMS
  bool  pulse_enable = true;                               // pulse counting
  int   pulse_period = 100;                                // pulse min period - 0 = no de-bounce
  bool  temp_enable = true;                                // enable temperature measurement
  DeviceAddress allAddresses[MAX_TEMPS];                   // sensor address data
  bool  showCurrents = false;                              // Print to serial voltage, current & p.f. values  
} EEProm;

uint16_t eepromSig = 0x0015;                               // oemEProm signature - see oemEProm Library documentation for details.
 
#ifdef EEWL_DEBUG
  extern EEWL EVmem;
#endif

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

  #ifdef DEBUG
    Serial.print(F("emonTx V4CM Continuous Monitoring V")); Serial.write(firmware_version);
    Serial.println(F("OpenEnergyMonitor.org"));
  #else
    Serial.println(F("describe:EmonTX4CM"));
  #endif
 
  load_config(true);                                                   // Load RF config from EEPROM (if any exists)
  
  if (EEProm.rf_on)
  {
    #ifdef DEBUG
      #ifdef RFM12B
      Serial.print(F("RFM12B "));
      #endif
      #ifdef RFM69CW
      Serial.print(F("RFM69CW "));
      #endif
      Serial.print(F(" Freq: "));
      if (EEProm.RF_freq == RFM_433MHZ) Serial.print(F("433MHz"));
      if (EEProm.RF_freq == RFM_868MHZ) Serial.print(F("868MHz"));
      if (EEProm.RF_freq == RFM_915MHZ) Serial.print(F("915MHz"));
      Serial.print(F(" Group: ")); Serial.print(EEProm.networkGroup);
      Serial.print(F(" Node: ")); Serial.print(EEProm.nodeID);
      Serial.println(F(" "));
    #endif
  }

  // Sets expected frequency 50Hz/60Hz
  if (digitalRead(DIP_switch2)==ON) {
      USA=true; // 60 Hz
  }
  // ---------------------------------------------------------------------------------------

  if (EEProm.rf_on)
  {
    rfm_init();                                                        // initialize RFM
    /* Serial.println(F("Factory Test"));
    for (int i=10; i>=0; i--) {
      emontx.P1=i;
      rfm_send((byte *)&emontx, sizeof(emontx), 1, EEProm.nodeID, EEProm.RF_freq, EEProm.rfPower, busyThreshold, busyTimeout);
      delay(100);
    }
    emontx.P1=0;*/
    delay(random(EEProm.nodeID * 20));                                 // try to avoid r.f. collisions at start-up
  }
  
  // ---------------------------------------------------------------------------------------
      
#ifdef EEWL_DEBUG
  Serial.print("End of mem=");Serial.print(E2END);
  Serial.print("  Avail mem=");Serial.print((E2END>>2) * 3);
  Serial.print("  Start addr=");Serial.print(E2END - (((E2END>>2) * 3) / (sizeof(mem)+1))*(sizeof(mem)+1));
  Serial.print("  Num blocks=");Serial.println(((E2END>>2) * 3) / 21);
  EVmem.dump_buffer();
#endif

  // ----------------------------------------------------------------------------
  // EmonLibCM config
  // ----------------------------------------------------------------------------
  // 12 bit ADC = 4096 divisions
  // Time in microseconds for one ADC conversion: 40 us 
  EmonLibCM_setADC(12,29.5);

  // Using AVR-DB 1.024V internal voltage reference
  EmonLibCM_ADCCal(1.024);
  
  EmonLibCM_SetADC_VChannel(0, EEProm.vCal);                           // ADC Input channel, voltage calibration
  EmonLibCM_SetADC_IChannel(3, EEProm.i1Cal, EEProm.i1Lead);           // ADC Input channel, current calibration, phase calibration
  EmonLibCM_SetADC_IChannel(4, EEProm.i2Cal, EEProm.i2Lead);           // The current channels will be read in this order
  EmonLibCM_SetADC_IChannel(5, EEProm.i3Cal, EEProm.i3Lead);           
  EmonLibCM_SetADC_IChannel(6, EEProm.i4Cal, EEProm.i4Lead);           
  EmonLibCM_SetADC_IChannel(8, EEProm.i5Cal, EEProm.i5Lead);
  EmonLibCM_SetADC_IChannel(9, EEProm.i6Cal, EEProm.i6Lead);

  // mains frequency 50Hz
  if (USA) EmonLibCM_cycles_per_second(60);                            // mains frequency 60Hz
  EmonLibCM_datalog_period(EEProm.period);                             // period of readings in seconds - normal value for emoncms.org  

  EmonLibCM_setAssumedVrms(EEProm.assumedVrms);

  EmonLibCM_setPulseEnable(EEProm.pulse_enable);                       // Enable pulse counting
  EmonLibCM_setPulsePin(PIN_PA6);
  EmonLibCM_setPulseMinPeriod(EEProm.pulse_period);

  EmonLibCM_setTemperatureDataPin(PIN_PB4);                            // OneWire data pin (emonTx V3.4)
  EmonLibCM_setTemperaturePowerPin(PIN_PB3);                           // Temperature sensor Power Pin - 19 for emonTx V3.4  (-1 = Not used. No sensors, or sensor are permanently powered.)
  EmonLibCM_setTemperatureResolution(11);                              // Resolution in bits, allowed values 9 - 12. 11-bit resolution, reads to 0.125 degC
  EmonLibCM_setTemperatureAddresses(EEProm.allAddresses);              // Name of array of temperature sensors
  EmonLibCM_setTemperatureArray(allTemps);                             // Name of array to receive temperature measurements
  EmonLibCM_setTemperatureMaxCount(MAX_TEMPS);                         // Max number of sensors, limited by wiring and array size.
  
  long e0=0, e1=0, e2=0, e3=0, e4=0, e5=0;
  unsigned long p=0;
  
  recoverEValues(&e0,&e1,&e2,&e3,&e4,&e5,&p);
  EmonLibCM_setWattHour(0, e0);
  EmonLibCM_setWattHour(1, e1);
  EmonLibCM_setWattHour(2, e2);
  EmonLibCM_setWattHour(3, e3);
  EmonLibCM_setWattHour(4, e4);
  EmonLibCM_setWattHour(5, e5);
  EmonLibCM_setPulseCount(p);

#ifdef EEWL_DEBUG
  EVmem.dump_control();
  EVmem.dump_buffer();  
#endif

  EmonLibCM_TemperatureEnable(EEProm.temp_enable);  
  EmonLibCM_Init();                                                    // Start continuous monitoring.
  emontx.Msg = 0;
  printTemperatureSensorAddresses();
  // Speed up startup by making first reading 2s
  EmonLibCM_datalog_period(2.0);
}

void loop()             
{
  getSettings();
  
  if (EmonLibCM_Ready())   
  {
    #ifdef DEBUG
    if (emontx.Msg==0) 
    {
      digitalWrite(LEDpin,LOW);
      EmonLibCM_datalog_period(EEProm.period); 
      if (EmonLibCM_acPresent())
        Serial.println(F("AC present - Real Power calc enabled"));
      else
      {
        Serial.print(F("AC missing - Apparent Power calc enabled, assuming ")); Serial.print(EEProm.assumedVrms); Serial.println(F(" V"));
      }
    }
    delay(5);
    #endif

    emontx.Msg++;

    // Other options calculated by EmonLibCM
    // RMS Current:    EmonLibCM_getIrms(ch)
    // Apparent Power: EmonLibCM_getApparentPower(ch)
    // Power Factor:   EmonLibCM_getPF(ch)
    
    emontx.P1 = EmonLibCM_getRealPower(0); 
    emontx.E1 = EmonLibCM_getWattHour(0); 

    emontx.P2 = EmonLibCM_getRealPower(1); 
    emontx.E2 = EmonLibCM_getWattHour(1); 
    
    emontx.P3 = EmonLibCM_getRealPower(2); 
    emontx.E3 = EmonLibCM_getWattHour(2); 
  
    emontx.P4 = EmonLibCM_getRealPower(3); 
    emontx.E4 = EmonLibCM_getWattHour(3); 

    emontx.P5 = EmonLibCM_getRealPower(4); 
    emontx.E5 = EmonLibCM_getWattHour(4); 

    emontx.P6 = EmonLibCM_getRealPower(5); 
    emontx.E6 = EmonLibCM_getWattHour(5); 

    if (EmonLibCM_acPresent()) {
      emontx.Vrms = EmonLibCM_getVrms() * 100;
    } else {
      emontx.Vrms = EmonLibCM_getAssumedVrms() * 100;
    }
    
    emontx.T1 = allTemps[0];
    emontx.T2 = allTemps[1];
    emontx.T3 = allTemps[2];

    emontx.pulse = EmonLibCM_getPulseCount();
        
    if (EEProm.rf_on) {
      PayloadTX tmp = emontx;
      rfm_send((byte *)&tmp, sizeof(tmp), EEProm.networkGroup, EEProm.nodeID, EEProm.RF_freq, EEProm.rfPower, busyThreshold, busyTimeout);     //send data
      delay(50);
    }

    // ---------------------------------------------------------------------
    // Key:Value format, used by EmonESP & emonhub EmonHubOEMInterfacer
    // ---------------------------------------------------------------------
    Serial.print(F("MSG:")); Serial.print(emontx.Msg);
    Serial.print(F(",Vrms:")); Serial.print(emontx.Vrms*0.01);
    
    Serial.print(F(",P1:")); Serial.print(emontx.P1);
    Serial.print(F(",P2:")); Serial.print(emontx.P2);
    Serial.print(F(",P3:")); Serial.print(emontx.P3);
    Serial.print(F(",P4:")); Serial.print(emontx.P4);
    Serial.print(F(",P5:")); Serial.print(emontx.P5);
    Serial.print(F(",P6:")); Serial.print(emontx.P6);
       
    Serial.print(F(",E1:")); Serial.print(emontx.E1);
    Serial.print(F(",E2:")); Serial.print(emontx.E2);
    Serial.print(F(",E3:")); Serial.print(emontx.E3);
    Serial.print(F(",E4:")); Serial.print(emontx.E4);
    Serial.print(F(",E5:")); Serial.print(emontx.E5);
    Serial.print(F(",E6:")); Serial.print(emontx.E6);
     
    if (emontx.T1!=30000) { Serial.print(F(",T1:")); Serial.print(emontx.T1*0.01); }
    if (emontx.T2!=30000) { Serial.print(F(",T2:")); Serial.print(emontx.T2*0.01); }
    if (emontx.T3!=30000) { Serial.print(F(",T3:")); Serial.print(emontx.T3*0.01); }

    Serial.print(F(",pulse:")); Serial.print(emontx.pulse);
    
    if (!EEProm.showCurrents) {
      Serial.println();
      delay(40);
    } else {
      // to show voltage, current & power factor for calibration:
      Serial.print(F(",I1:")); Serial.print(EmonLibCM_getIrms(EmonLibCM_getLogicalChannel(1)),3);
      Serial.print(F(",I2:")); Serial.print(EmonLibCM_getIrms(EmonLibCM_getLogicalChannel(2)),3);
      Serial.print(F(",I3:")); Serial.print(EmonLibCM_getIrms(EmonLibCM_getLogicalChannel(3)),3);
      Serial.print(F(",I4:")); Serial.print(EmonLibCM_getIrms(EmonLibCM_getLogicalChannel(4)),3);
      Serial.print(F(",I5:")); Serial.print(EmonLibCM_getIrms(EmonLibCM_getLogicalChannel(5)),3);
      Serial.print(F(",I6:")); Serial.print(EmonLibCM_getIrms(EmonLibCM_getLogicalChannel(6)),3);
      
      Serial.print(F(",pf1:")); Serial.print(EmonLibCM_getPF(EmonLibCM_getLogicalChannel(1)),4);
      Serial.print(F(",pf2:")); Serial.print(EmonLibCM_getPF(EmonLibCM_getLogicalChannel(2)),4);
      Serial.print(F(",pf3:")); Serial.print(EmonLibCM_getPF(EmonLibCM_getLogicalChannel(3)),4);
      Serial.print(F(",pf4:")); Serial.print(EmonLibCM_getPF(EmonLibCM_getLogicalChannel(4)),4);
      Serial.print(F(",pf5:")); Serial.print(EmonLibCM_getPF(EmonLibCM_getLogicalChannel(5)),4);
      Serial.print(F(",pf6:")); Serial.println(EmonLibCM_getPF(EmonLibCM_getLogicalChannel(6)),4);
      delay(80);
    }
    digitalWrite(LEDpin,HIGH); delay(50);digitalWrite(LEDpin,LOW);
    // End of print out ----------------------------------------------------
    storeEValues(emontx.E1,emontx.E2,emontx.E3,emontx.E4,emontx.E5,emontx.E6,emontx.pulse);
  }
  wdt_reset();
  delay(20);
}
