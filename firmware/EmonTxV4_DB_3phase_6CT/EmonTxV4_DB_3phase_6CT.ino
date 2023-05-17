/*
  emonTxV4.0 Continuous Sampling
  using EmonLibCM https://github.com/openenergymonitor/EmonLibCM
  Authors: Robin Emley, Robert Wall, Trystan Lea
  
  -----------------------------------------
  Part of the openenergymonitor.org project
  Licence: GNU GPL V3



Change Log:
v1.0.0: First release of EmonTxV4 Continuous Monitoring Firmware (based on EmonTx v3 CM Firmware)
v1.1.0: Fixed emonEProm implementation for AVR-DB & new serial config implementation
v1.2.0: LowPowerLabs radio format, with option to switch to JeeLib classic or native.
v1.3.0: Read and calibrate reference voltage at startup
v1.4.0: Option to output serial data as JSON (Brian Orpin)
v1.5.0: emonEProm fixed pulse count data type issue
v1.5.1: default node id set to 17, swap nodeid DIP, zero all 6 energy values (
v1.5.2: emonEProm fixed EEWL overlap
v1.5.3: Slightly slower sample rate to improve zero power performance
        temperature sensing disabled if no temperature sensors detected at startup
v1.5.4: Fix emonEProm EEWL overlap properly
v1.5.5: RFM69_LPL library update use setPins
v2.0.0: Single phase 6CT energy monitor based on EmonLibDB library
v2.0.1: Default nodeid set to 27
v2.0.2: Change default phase allocation to 1-2-3 1-2-3

*/
#define Serial Serial3

#define RFM69_JEELIB_CLASSIC 1
#define RFM69_JEELIB_NATIVE 2
#define RFM69_LOW_POWER_LABS 3

#define RadioFormat RFM69_LOW_POWER_LABS

const char *firmware_version = {"2.0.2\n\r"};
/*

emonhub.conf node decoder (nodeid is 27 when switch is off, 18 when switch is on)
See: https://github.com/openenergymonitor/emonhub/blob/emon-pi/configuration.md
copy the following into emonhub.conf:

[[27]]
  nodename = EmonTx4_DB
  [[[rx]]]
    names = MSG, V1, V2, V3, P1, P2, P3, P4, P5, P6, E1, E2, E3, E4, E5, E6, pulse
    datacodes = L, h,h,h, h,h,h,h,h,h, l,l,l,l,l,l ,L
    scales = 1,0.01,0.01,0.01,1,1,1,1,1,1,1,1,1,1,1
    units = n,V,V,V,W,W,W,W,W,W,Wh,Wh,Wh,Wh,Wh,Wh,p

*/
// Comment/Uncomment as applicable
#define DEBUG                                              // Debug level print out
#define EMONTX4

// #define EEWL_DEBUG

#define OFF HIGH
#define ON LOW

#define RFM69CW

#include <Arduino.h>
#include <avr/wdt.h>

#if RadioFormat == RFM69_LOW_POWER_LABS
  #include "RFM69_LPL.h"
#else
  #include "RFM69_JeeLib.h"                                        // Minimal radio library that supports both original JeeLib format and later native format
#endif

// EEWL_START = 102, Start EEPROM wear leveling section after config section which takes up first 99 bytes, a few bytes of padding here
// EEWL_BLOCKS = 14, 14 x 29 byte blocks = 406 bytes. EEWL_END = 102+406 = 508 a few bytes short of 512 byte limit
// EEWL is only updated if values change by more than 200Wh, this means for a typical house consuming ~4000kWh/year
// 20,000 writes per channel to EEPROM, there's a 100,000 write lifetime for any individual EEPROM byte.
// With a circular buffer of 14 blocks, this extends the lifetime from 5 years to 70 years.
// IMPORTANT! If adding to config section change EEWL_START and check EEWL_END Implications.

#include <emonEProm.h>                                     // OEM EEPROM library
#include <emonLibDB.h>                                     // OEM Continuous Monitoring library DB

// Include EmonTxV4_config.ino in the same directory for settings functions & data

RFM69 rf;

#define NUM_V_CHANNELS 3                                   // SET TO 1 FOR SINGLE PHASE
#define NUM_I_CHANNELS 6

// 50 Bytes
typedef struct {
    unsigned long Msg;
    int V[NUM_V_CHANNELS];
    int P[NUM_I_CHANNELS];
    long E[NUM_I_CHANNELS]; 
    // int T1,T2,T3;
    unsigned long pulse;
} PayloadTX;                                               // create a data packet for the RFM
PayloadTX emontx;

static void showString (PGM_P s);
 
//---------------------------- emonTx Settings - Stored in EEPROM and shared with config.ino ------------------------------------------------
struct {
  byte RF_freq = RF69_433MHZ;                              // Frequency of radio module can be RFM_433MHZ, RFM_868MHZ or RFM_915MHZ. 
  byte networkGroup = 210;                                 // wireless network group, must be the same as emonBase / emonPi and emonGLCD. OEM default is 210
  byte nodeID = 27;                                        // node ID for this emonTx.
  byte rf_on = 1;                                          // RF - 0 = no RF, 1 = RF on.
  byte rfPower = 25;                                       // 7 = -10.5 dBm, 25 = +7 dBm for RFM12B; 0 = -18 dBm, 31 = +13 dBm for RFM69CW. Default = 25 (+7 dBm)
  float vCal  = 100.0;                                     // Only single vCal for three phase in this firmware
  // float assumedVrms = 240.0;                            // Assumed Vrms when no a.c. is detected
  float lineFreq = 50;                                     // Line Frequency = 50 Hz

  float iCal[NUM_I_CHANNELS];
  float iLead[NUM_I_CHANNELS];
  
  float period = 9.8;                                      // datalogging period - should be fractionally less than the PHPFINA database period in emonCMS
  bool  pulse_enable = true;                               // pulse counting
  int   pulse_period = 100;                                // pulse min period - 0 = no de-bounce
  bool  showCurrents = false;                              // Print to serial voltage, current & p.f. values
  bool  json_enabled = false;                              // JSON Enabled - false = key,Value pair, true = JSON, default = false: Key,Value pair.  
} EEProm;

uint16_t eepromSig = 0x0020;                               // oemEProm signature - see oemEProm Library documentation for details.
 
#ifdef EEWL_DEBUG
  extern EEWL EVmem;
#endif

bool  USA=false;

bool calibration_enable = true;                           // Enable on-line calibration when running. 
                                                           // For safety, thus MUST default to false. (Required due to faulty ESP8266 software.)

//----------------------------emonTx V4 hard-wired connections-----------------------------------
const byte LEDpin      = PIN_PB2;  // emonTx V4 LED
const byte DIP_switch1 = PIN_PA4;  // RF node ID (default no change in node ID, switch on for nodeID + 1) switch off D8 is HIGH from internal pullup
const byte DIP_switch2 = PIN_PA5;  // Voltage selection 240 / 120 V AC (default switch off 240V)  - switch off D9 is HIGH from internal pullup

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
  if (digitalRead(DIP_switch2)==ON) EEProm.nodeID++;                         // IF DIP switch 1 is switched on (LOW) then add 1 from nodeID

  #ifdef DEBUG
    Serial.print(F("emonTx V4 DB Continuous Monitoring V")); Serial.write(firmware_version);
    Serial.println(F("OpenEnergyMonitor.org"));
  #else
    Serial.println(F("describe:EmonTX4DB"));
  #endif

  for (byte ch=0; ch<NUM_I_CHANNELS; ch++) {
    EEProm.iCal[ch] = 20.0;
    EEProm.iLead[ch] = 3.2;
  }
 
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
      if (EEProm.RF_freq == RF69_433MHZ) Serial.print(F("433MHz"));
      if (EEProm.RF_freq == RF69_868MHZ) Serial.print(F("868MHz"));
      if (EEProm.RF_freq == RF69_915MHZ) Serial.print(F("915MHz"));
      Serial.print(F(" Group: ")); Serial.print(EEProm.networkGroup);
      Serial.print(F(" Node: ")); Serial.print(EEProm.nodeID);
      Serial.println(F(" "));
    #endif

    #if RadioFormat == RFM69_LOW_POWER_LABS
      Serial.println("RadioFormat: LowPowerLabs");
    #elif RadioFormat == RFM69_JEELIB_CLASSIC
      Serial.println("RadioFormat: JeeLib Classic");
    #elif RadioFormat == RFM69_JEELIB_NATIVE
      Serial.println("RadioFormat: JeeLib Native");
    #endif
  }

  // Sets expected frequency 50Hz/60Hz
  if (digitalRead(DIP_switch1)==ON) {
      USA=true; // 60 Hz
  }
  // ---------------------------------------------------------------------------------------

  if (EEProm.rf_on)
  {
    #if RadioFormat == RFM69_JEELIB_CLASSIC
      rf.format(RFM69_JEELIB_CLASSIC);
    #endif
    
    // Frequency is currently hardcoded to 433Mhz in library
    #if RadioFormat == RFM69_LOW_POWER_LABS
    rf.setPins(PIN_PB5,PIN_PC0,PIN_PC1,PIN_PC2);
    #endif
    rf.initialize(RF69_433MHZ, EEProm.nodeID, EEProm.networkGroup); 
    rf.encrypt("89txbe4p8aik5kt3");                                    // ignored if jeelib classic
    delay(random(EEProm.nodeID * 20));                                 // try to avoid r.f. collisions at start-up
  }
  
  // ---------------------------------------------------------------------------------------
      
#ifdef EEWL_DEBUG
  EVmem.dump_buffer();
#endif

  double reference = read_reference();
  Serial.print(F("Reference voltage calibration: "));
  Serial.println(reference,4);

  /****************************************************************************
  *                                                                           *
  * Set the properties of the physical sensors                                *
  *                                                                           *
  ****************************************************************************/
  
  EmonLibDB_set_vInput(1, EEProm.vCal, 0.16);         // emonVS Input channel 1, voltage calibration 100, phase error 0.16°
  #if NUM_V_CHANNELS == 3
    EmonLibDB_set_vInput(2, EEProm.vCal, 0.16);       // emonVS Input channel 2, voltage calibration 100, phase error 0.16°
    EmonLibDB_set_vInput(3, EEProm.vCal, 0.16);       // emonVS Input channel 3, voltage calibration 100, phase error 0.16°
  #endif
                                                      // (All 3 may be set, even if those inputs are unused)

  for (byte ch=0; ch<NUM_I_CHANNELS; ch++) {
    EmonLibDB_set_cInput(ch+1, EEProm.iCal[ch], EEProm.iLead[ch]);         
  }

  /****************************************************************************
  *                                                                           *
  * Link voltage and current sensors to define the power                      *
  *  & energy measurements                                                    *
  *                                                                           *
  * For best precision and performance, include only the following lines      *
  *    that apply to current/power inputs being used.                         *
  ****************************************************************************/

  #if NUM_V_CHANNELS == 3
    
    EmonLibDB_set_pInput(1, 1); // Phase 1
    EmonLibDB_set_pInput(2, 2); // Phase 2
    EmonLibDB_set_pInput(3, 3); // Phase 3
    EmonLibDB_set_pInput(4, 1); // Phase 1
    EmonLibDB_set_pInput(5, 2); // Phase 2
    EmonLibDB_set_pInput(6, 3); // Phase 3
    /*
    EmonLibDB_set_pInput(1, 1, 2);               // CT1 between V1 & V2    
    EmonLibDB_set_pInput(2, 2, 3);               // CT2 between V2 & V3  (etc)
    EmonLibDB_set_pInput(3, 3, 1);  
    EmonLibDB_set_pInput(4, 1, 2);  
    EmonLibDB_set_pInput(5, 2, 3);
    EmonLibDB_set_pInput(6, 3, 1);
    */
  #else
    for (byte ch=0; ch<NUM_I_CHANNELS; ch++) {
      EmonLibDB_set_pInput(ch+1, 1);
    }
  #endif
  

  /****************************************************************************
  *                                                                           *
  * Values for timing & calibration of the ADC                                *
  *                                                                           *
  *   Users in the 60 Hz world must change "cycles_per_second"                *
  *                                                                           *
  ****************************************************************************/

  // mains frequency 50Hz
  if (!USA) {
    EmonLibDB_cyclesPerSecond(50);                            // mains frequency 50Hz
  } else {
    EmonLibDB_cyclesPerSecond(60);                            // mains frequency 60Hz
  }
  EmonLibDB_minStartupCycles(10);                             // number of cycles to let ADC run before starting first actual measurement
  EmonLibDB_datalogPeriod(EEProm.period);                     // period of readings in seconds - normal value for emoncms.org
  EmonLibDB_ADCCal(1.024);                                    // ADC Reference voltage, (1.024 V)


  /****************************************************************************
  *                                                                           *
  * Pulse Counting                                                            *
  *                                                                           *
  * Pulse counting on any channel is only available if the appropriate        *
  * solder link is made on the hardware, and the related "TMP" link is        *
  * broken.                                                                   *
  * The 'Analogue' input is not available if an extender card is fitted.      *
  ****************************************************************************/
 
  EmonLibDB_setPulseEnable(true);              // Enable counting on "Pulse" input
  EmonLibDB_setPulseMinPeriod(20);             // Contact bounce must not last longer than 20 ms

  EmonLibDB_setPulseEnable(2, false);           // Enable counting on "Digital" input
  // EmonLibDB_setPulseMinPeriod(2, 20, FALLING); // Contact bounce must not last longer than 20 ms, trigger on the falling edge

  EmonLibDB_setPulseEnable(3, false);          // Disable counting on "Analog" input
  // EmonLibDB_setPulseMinPeriod(3, 0, RISING);   // No contact bounce expected, trigger on the rising edge
  // EmonLibDB_setPulseCount(3, 123);             // Initialise the pulse count for this to 123 counts

  /****************************************************************************
  *                                                                           *
  * Pre-set Energy counters                                                   *
  *                                                                           *
  ****************************************************************************/

  long e0=0, e1=0, e2=0, e3=0, e4=0, e5=0;
  unsigned long p=0;
  
  recoverEValues(&e0,&e1,&e2,&e3,&e4,&e5,&p);
  EmonLibDB_setWattHour(0, e0);
  EmonLibDB_setWattHour(1, e1);
  EmonLibDB_setWattHour(2, e2);
  EmonLibDB_setWattHour(3, e3);
  EmonLibDB_setWattHour(4, e4);
  EmonLibDB_setWattHour(5, e5);
  EmonLibDB_setPulseCount(p);

#ifdef EEWL_DEBUG
  EVmem.dump_control();
  EVmem.dump_buffer();  
#endif
  EmonLibDB_Init();                                                    // Start continuous monitoring.
  emontx.Msg = 0;
    
  // Speed up startup by making first reading 2s
  EmonLibDB_datalogPeriod(2.0);
}

void loop()             
{
  getSettings();
  
  if (EmonLibDB_Ready())   
  {
    #ifdef DEBUG
    if (emontx.Msg==0) 
    {
      digitalWrite(LEDpin,LOW);
      EmonLibDB_datalogPeriod(EEProm.period);
      /*
      if (EmonLibCM_acPresent()) {
        Serial.println(F("AC present - Real Power calc enabled"));
      } else {
        Serial.print(F("AC missing - Apparent Power calc enabled, assuming ")); Serial.print(EEProm.assumedVrms); Serial.println(F(" V"));
      }*/
    }
    delay(5);
    #endif

    emontx.Msg++;

    // Other options calculated by EmonLibCM
    // RMS Current:    EmonLibCM_getIrms(ch)
    // Apparent Power: EmonLibCM_getApparentPower(ch)
    // Power Factor:   EmonLibCM_getPF(ch)
    
    for (byte ch=0; ch<NUM_V_CHANNELS; ch++) {
        emontx.V[ch] = EmonLibDB_getVrms(ch+1) * 100;
    }
    
    for (byte ch=0; ch<NUM_I_CHANNELS; ch++) {
        emontx.P[ch] = EmonLibDB_getRealPower(ch+1);
        emontx.E[ch] = EmonLibDB_getWattHour(ch+1);
    }
    
    emontx.pulse = EmonLibDB_getPulseCount(1);
        
    if (EEProm.rf_on) {
      PayloadTX tmp = emontx;

      #if RadioFormat == RFM69_LOW_POWER_LABS
        rf.sendWithRetry(5,(byte *)&tmp, sizeof(tmp));
      #else
        rf.send(0, (byte *)&tmp, sizeof(tmp));
      #endif
      
      delay(50);
    }

    if (EEProm.json_enabled) {
      // ---------------------------------------------------------------------
      // JSON Format
      // ---------------------------------------------------------------------
      Serial.print(F("{\"MSG\":")); Serial.print(emontx.Msg);

      for (byte ch=0; ch<NUM_V_CHANNELS; ch++) {
        Serial.print(F(",\"V")); Serial.print(ch+1); Serial.print("\":"); Serial.print(emontx.V[ch]*0.01);     
      }
      for (byte ch=0; ch<NUM_I_CHANNELS; ch++) {
        Serial.print(F(",\"P")); Serial.print(ch+1); Serial.print("\":"); Serial.print(emontx.P[ch]);
      }
      for (byte ch=0; ch<NUM_I_CHANNELS; ch++) {
        Serial.print(F(",\"E")); Serial.print(ch+1); Serial.print("\":"); Serial.print(emontx.E[ch]);
      }

      //if (emontx.T1!=30000) { Serial.print(F(",\"T1\":")); Serial.print(emontx.T1*0.01); }
      //if (emontx.T2!=30000) { Serial.print(F(",\"T2\":")); Serial.print(emontx.T2*0.01); }
      //if (emontx.T3!=30000) { Serial.print(F(",\"T3\":")); Serial.print(emontx.T3*0.01); }

      Serial.print(F(",\"pulse\":")); Serial.print(emontx.pulse);
      Serial.println(F("}"));
      delay(60);
      
    } else {
  
      // ---------------------------------------------------------------------
      // Key:Value format, used by EmonESP & emonhub EmonHubOEMInterfacer
      // ---------------------------------------------------------------------
      Serial.print(F("MSG:")); Serial.print(emontx.Msg);
      
      for (byte ch=0; ch<NUM_V_CHANNELS; ch++) {
        Serial.print(F(",V")); Serial.print(ch+1); Serial.print(":"); Serial.print(emontx.V[ch]*0.01);     
      }
      for (byte ch=0; ch<NUM_I_CHANNELS; ch++) {
        Serial.print(F(",P")); Serial.print(ch+1); Serial.print(":"); Serial.print(emontx.P[ch]);
      }
      for (byte ch=0; ch<NUM_I_CHANNELS; ch++) {
        Serial.print(F(",E")); Serial.print(ch+1); Serial.print(":"); Serial.print(emontx.E[ch]);
      }
       
      //if (emontx.T1!=30000) { Serial.print(F(",T1:")); Serial.print(emontx.T1*0.01); }
      //if (emontx.T2!=30000) { Serial.print(F(",T2:")); Serial.print(emontx.T2*0.01); }
      //if (emontx.T3!=30000) { Serial.print(F(",T3:")); Serial.print(emontx.T3*0.01); }
  
      Serial.print(F(",pulse:")); Serial.print(emontx.pulse);
      
      if (!EEProm.showCurrents) {
        Serial.println();
        delay(40);
      } else {
        // to show voltage, current & power factor for calibration:
        for (byte ch=0; ch<NUM_I_CHANNELS; ch++) {
          Serial.print(F(",I")); Serial.print(ch+1); Serial.print(":"); Serial.print(EmonLibDB_getIrms(ch+1),3);
        }
        for (byte ch=0; ch<NUM_I_CHANNELS; ch++) {
          Serial.print(F(",pf")); Serial.print(ch+1); Serial.print(":"); Serial.print(EmonLibDB_getPF(ch+1),4);
        }
        delay(80);
      }
    }
    digitalWrite(LEDpin,HIGH); delay(50);digitalWrite(LEDpin,LOW);
    // End of print out ----------------------------------------------------
    storeEValues(emontx.E[0],emontx.E[1],emontx.E[2],emontx.E[3],emontx.E[4],emontx.E[5],emontx.pulse);
  }
  wdt_reset();
  delay(20);
}


double read_reference() {
  ADC0.SAMPCTRL = 14;
  ADC0.CTRLD |= 0x0;
  VREF.ADC0REF = VREF_REFSEL_1V024_gc;
  ADC0.CTRLC = ADC_PRESC_DIV24_gc;
  ADC0.CTRLA = ADC_ENABLE_bm;
  ADC0.CTRLA |= ADC_RESSEL_12BIT_gc;

  ADC0.MUXPOS = 7;
  unsigned long sum = 0;
  for (int i=0; i<10010; i++) {
    ADC0.COMMAND = ADC_STCONV_bm;
    while (!(ADC0.INTFLAGS & ADC_RESRDY_bm));
    if (i>9) {
      sum += ADC0.RES;
    }
  }
  double mean = sum / 10000.0;
  double reference = 0.9 / (mean/4095.0);
  return reference;
}
