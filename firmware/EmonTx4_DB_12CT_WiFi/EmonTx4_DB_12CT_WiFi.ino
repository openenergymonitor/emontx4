// EmonTx4_DB_12CT_WiFi 
// This default example prints 1 voltage and 12 power values to the serial port
// this is then either picked up by an EmonESP 8266 WiFi module or can be used
// for direct USB link to a emonBase/emonPi via emonHub.

#define Serial Serial3
#include <Arduino.h>
#include "emonLibDB.h"
#define EMONTX4          // Must be above "#include <RFM69_LPL.h>

#define DATALOG 9.8   

#define EXPANSION_BOARD  // Can be commented out for 6 CT only

const uint8_t LEDpin = PIN_PB2;                   // emonTx V4 LED
uint32_t counter = 0;
void setup() 
{  
  pinMode(LEDpin, OUTPUT);

  Serial.begin(9600);
  Serial.println("Set baud=115200");
  Serial.end();
  Serial.begin(115200);
  
  Serial.println("\nEmonTx4_DB_12CT_WIFI"); 

  
  /****************************************************************************
  *                                                                           *
  * Set the properties of the physical sensors                                *
  *                                                                           *
  ****************************************************************************/
  
  EmonLibDB_set_vInput(1, 100.0, 0.16);  
  
  /* Include the next two lines if you have a 3-phase emonVS */
 
  // EmonLibDB_set_vInput(2, 100.0, 0.16); 
  // EmonLibDB_set_vInput(3, 100.0, 0.16); 
  
  EmonLibDB_set_cInput(1, 20.0, 0.3);         // 0.3° @ 20 A for 100 A CT
  EmonLibDB_set_cInput(2, 20.0, 0.3);
  EmonLibDB_set_cInput(3, 20.0, 0.3);
  EmonLibDB_set_cInput(4, 20.0, 0.3);
  EmonLibDB_set_cInput(5, 20.0, 0.3);
  EmonLibDB_set_cInput(6, 20.0, 0.3);
  
#ifdef EXPANSION_BOARD

  EmonLibDB_set_cInput(7, 20.0, 0.3);
  EmonLibDB_set_cInput(8, 20.0, 0.3);
  EmonLibDB_set_cInput(9, 20.0, 0.3);
  EmonLibDB_set_cInput(10, 20.0, 0.3);
  EmonLibDB_set_cInput(11, 20.0, 0.3);
  EmonLibDB_set_cInput(12, 20.0, 0.3);
  
#endif  

  /****************************************************************************
  *                                                                           *
  * Link voltage and current sensors to define the power                      *
  *  & energy measurements                                                    *
  *                                                                           *
  * For best precision and performance, include only the following lines      *
  *    that apply to current/power inputs being used.                         *
  ****************************************************************************/

/*
  EmonLibDB_set_pInput(1, 1);                  // CT1, V1
*/
  EmonLibDB_set_pInput(1, 1);                  // CT2, V2 (etc)
  EmonLibDB_set_pInput(2, 1);
  EmonLibDB_set_pInput(3, 1);
  EmonLibDB_set_pInput(4, 1);  
  EmonLibDB_set_pInput(5, 1);
  EmonLibDB_set_pInput(6, 1);

#ifdef EXPANSION_BOARD

  EmonLibDB_set_pInput(7, 1);                  // CT7, V1  
  EmonLibDB_set_pInput(8, 1);                  // CT8, V2 (etc)
  EmonLibDB_set_pInput(9, 1);
  EmonLibDB_set_pInput(10, 1);  
  EmonLibDB_set_pInput(11, 1);
  EmonLibDB_set_pInput(12, 1);

#endif

  /* How to measure Line-Line loads: */
/*
  EmonLibDB_set_pInput(3, 1, 2);               // CT1 between V1 & V2    
  EmonLibDB_set_pInput(2, 2, 3);               // CT2 between V2 & V3
  EmonLibDB_set_pInput(3, 3, 1);               // CT2 between V3 & V1  (etc)  
  EmonLibDB_set_pInput(4, 1, 2);  
  EmonLibDB_set_pInput(5, 2, 3);
  EmonLibDB_set_pInput(6, 3, 1);

  EmonLibDB_set_pInput(7, 1, 2);  
  EmonLibDB_set_pInput(8, 2, 3);
  EmonLibDB_set_pInput(9, 3, 1);
  EmonLibDB_set_pInput(10, 1, 2);  
  EmonLibDB_set_pInput(11, 2, 3);
  EmonLibDB_set_pInput(12, 3, 1);
*/

/*
    Pulse counting on any channel is only available if the appropriate solder link is made on the hardware,
      and the related "TMP" link is broken.
*/      
  EmonLibDB_setPulseEnable(true);              // Enable counting on the "Pulse" input
  EmonLibDB_setPulseMinPeriod(20);             // Contact bounce must not last longer than 20 ms
  // EmonLibDB_setPulseEnable(Dig, true);         // Enable counting on the "Pulse" input
  // EmonLibDB_setPulseMinPeriod(Dig, 20);        // Contact bounce must not last longer than 20 ms

  EmonLibDB_datalogPeriod(DATALOG);            // Report every 9.8 s (approx)
  Serial.print("Starting ");
  EmonLibDB_Init();                            // Start continuous monitoring.
  Serial.print("reports every ");
  Serial.print(EmonLibDB_getDatalogPeriod());
  Serial.println(" seconds approx");
  
}

void loop()             
{
  static uint32_t counter = 0;

  if (EmonLibDB_Ready())   
  {
    counter++;
    Serial.print("MSG:"); Serial.print(counter);
    Serial.print(",V1:"); Serial.print(EmonLibDB_getVrms(1));
    
    // Uncomment for further phases as required
    // Serial.print(",V2:"); Serial.print(EmonLibDB_getVrms(2));
    // Serial.print(",V3:"); Serial.print(EmonLibDB_getVrms(3));
    // Serial.print(",FR:"); Serial.print(EmonLibDB_getLineFrequency());
    
#ifndef EXPANSION_BOARD
    for (uint8_t ch=1; ch<=6; ch++)
#else
    for (uint8_t ch=1; ch<=12; ch++)
#endif
    {
      Serial.print(",P"); Serial.print(ch); Serial.print(":"); Serial.print(EmonLibDB_getRealPower(ch)); 

      // Uncomment for energy values
      // Serial.print(",E"); Serial.print(ch); Serial.print(":"); Serial.println(EmonLibDB_getWattHour(ch));
    }
    Serial.println();
    delay(200);
  }
}
