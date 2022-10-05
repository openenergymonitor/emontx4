#define REG_FIFO            0x00	
#define REG_OPMODE          0x01
#define MODE_SLEEP          0x00
#define MODE_TRANSMITTER    0x0C
#define MODE_RECEIVER       0x10
#define REG_DIOMAPPING1     0x25	
#define REG_IRQFLAGS1       0x27
#define MODE_READY          0x80
#define REG_IRQFLAGS2       0x28
#define IRQ2_FIFOFULL       0x80
#define IRQ2_FIFONOTEMPTY   0x40
#define IRQ2_PACKETSENT     0x08
#define IRQ2_FIFOOVERRUN    0x10
#define REG_PACKET_CONFIG2  0x3D
#define RESTART_RX          0x04
#define REG_RSSI_CONFIG     0x23
#define RSSI_START          0x01
#define RSSI_DONE           0x02
#define REG_RSSI_VALUE      0x24

#define RFM69CW
#define RFMSELPIN 27                                     // RFM pins
#define RFPWR 0x99                                      // RFM Power setting - see rfm.ino for more
#define FACTORYTESTGROUP 1                              // R.F. group for factory test only
#include <SPI.h>

SPIClass SPI1(HSPI);

enum rfband {RF12_433MHZ = 1, RF12_868MHZ, RF12_915MHZ }; // frequency band.

byte RF_freq = RF12_433MHZ;                             // Frequency of radio module can be RF12_433MHZ, RF12_868MHZ or RF12_915MHZ. 
byte nodeID = 1;                                        // node ID for this emonTx.
int networkGroup = 210;                                 // wireless network group, needs to be same as emonBase / emonPi and emonGLCD. OEM default is 210
const int busyThreshold = -97;                          // Signal level below which the radio channel is clear to transmit
const byte busyTimeout = 15;                            // Time in ms to wait for the channel to become clear, before transmitting anyway
int rf_whitening = 2;                                   // RF & data whitening - 0 = no RF, 1 = RF on, no whitening, default = 2: RF is ON with whitening.


typedef struct {
  int P1,P2,P3;
} PayloadTX;
PayloadTX emontx;

void setup() {	
  Serial.begin(115200);
  Serial.println("RFM TX Test");
  
	// Set up to drive the Radio Module
	pinMode(RFMSELPIN, OUTPUT);
	digitalWrite(RFMSELPIN, LOW);
  delay(5000);

  Serial.println("RFM Init start");
  rfm_init(RF_freq);
  Serial.println("RFM Init complete");
}

void loop() {
  emontx.P1 = 100;
  emontx.P2 = 200;
  emontx.P3 = 300;
  
  Serial.println("RFM send");
  rfm_send((byte *)&emontx, sizeof(emontx), networkGroup, nodeID, busyThreshold, busyTimeout);     //send data
  delay(2000);
}

void rfm_init(byte RF_freq)
{	
	// Set up to drive the Radio Module
	pinMode(RFMSELPIN, OUTPUT);
	digitalWrite(RFMSELPIN, HIGH);
	
	// SPI1.begin(18,19,23,5);
	SPI1.begin(14,12,13,27);
	
	SPI1.setBitOrder(MSBFIRST);
	SPI1.setDataMode(SPI_MODE0);
	SPI1.setClockDivider(SPI_CLOCK_DIV4); // decided to slow down from DIV2 after SPI stalling in some instances, especially visible on mega1284p when RFM69 and FLASH chip both present
	
	// Initialise RFM69CW
	do 
		writeReg(0x2F, 0xAA); // RegSyncValue1
	while (readReg(0x2F) != 0xAA) ;
	do
	  writeReg(0x2F, 0x55); 
	while (readReg(0x2F) != 0x55);
	
	writeReg(0x01, 0x04); // RegOpMode: RF_OPMODE_SEQUENCER_ON | RF_OPMODE_LISTEN_OFF | RF_OPMODE_STANDBY
	writeReg(0x02, 0x00); // RegDataModul: RF_DATAMODUL_DATAMODE_PACKET | RF_DATAMODUL_MODULATIONTYPE_FSK | RF_DATAMODUL_MODULATIONSHAPING_00 = no shaping
	writeReg(0x03, 0x02); // RegBitrateMsb  ~49.23k BPS
	writeReg(0x04, 0x8A); // RegBitrateLsb
	writeReg(0x05, 0x05); // RegFdevMsb: ~90 kHz 
	writeReg(0x06, 0xC3); // RegFdevLsb
	if (RF_freq == RF12_868MHZ)
  {
		writeReg(0x07, 0xD9); // RegFrfMsb: Frf = Rf Freq / 61.03515625 Hz = 0xD90000 = 868.00 MHz as used JeeLib  
		writeReg(0x08, 0x00); // RegFrfMid
		writeReg(0x09, 0x00); // RegFrfLsb
	}
  else if (RF_freq == RF12_915MHZ) // JeeLib uses 912.00 MHz
  {	
		writeReg(0x07, 0xE4); // RegFrfMsb: Frf = Rf Freq / 61.03515625 Hz = 0xE40000 = 912.00 MHz as used JeeLib 
		writeReg(0x08, 0x00); // RegFrfMid
		writeReg(0x09, 0x00); // RegFrfLsb
  }
	else // default to 433 MHz band
	{
    writeReg(0x07, 0x6C); // RegFrfMsb: Frf = Rf Freq / 61.03515625 Hz = 0x6C8000 = 434.00 MHz as used JeeLib 
		writeReg(0x08, 0x80); // RegFrfMid
		writeReg(0x09, 0x00); // RegFrfLsb
	}
//	writeReg(0x0B, 0x20); // RegAfcCtrl:
	writeReg(0x11, RFPWR); // RegPaLevel = 0x9F = PA0 on, +13 dBm  -- RFM12B equivalent: 0x99 | 0x88 (-10dBm) appears to be the max before the AC power supply fails @ 230 V mains. Min value is 0x80 (-18 dBm)
	writeReg(0x1E, 0x2C); //
	writeReg(0x25, 0x80); // RegDioMapping1: DIO0 is used as IRQ 
	writeReg(0x26, 0x03); // RegDioMapping2: ClkOut off
	writeReg(0x28, 0x00); // RegIrqFlags2: FifoOverrun

	// RegPreamble (0x2c, 0x2d): default 0x0003
	writeReg(0x2E, 0x88); // RegSyncConfig: SyncOn | FifoFillCondition | SyncSize = 2 bytes | SyncTol = 0
	writeReg(0x2F, 0x2D); // RegSyncValue1: Same as JeeLib
	writeReg(0x37, 0x00); // RegPacketConfig1: PacketFormat=fixed | !DcFree | !CrcOn | !CrcAutoClearOff | !AddressFiltering >> 0x00
}


// transmit data via the RFM69CW
bool rfm_send(const byte *data, const byte size, const byte group, const byte node, const int threshold, const byte timeout)   // *SEND RF DATA*             
{

  unsigned long t_start = millis();
  bool success = false;                                     // return false if timed out, else true

  writeReg(REG_OPMODE, (readReg(REG_OPMODE) & 0xE3) | MODE_RECEIVER);		// Receive mode - sniff for channel is busy
  while ((millis()-t_start)<(unsigned long)timeout)
  {
    while((readReg(REG_IRQFLAGS1) & MODE_READY) == 0)      
      ;
    writeReg(REG_RSSI_CONFIG, RSSI_START);
    while((readReg(REG_RSSI_CONFIG) & RSSI_DONE) == 0x00)
      ; 
    
    if (readReg(REG_RSSI_VALUE) > (threshold * -2))         // because REG_RSSI_VALUE is upside down!
    {
      success = true;
      break;                                                // Nothing heard - go ahead and transmit
    }
    writeReg(REG_PACKET_CONFIG2, (readReg(REG_PACKET_CONFIG2) & 0xFB) | RESTART_RX);  // Restart the receiver
  }                                                         // We have waited long enough - go ahead and transmit anyway
  
  writeReg(REG_OPMODE, (readReg(REG_OPMODE) & 0xE3) | MODE_SLEEP);	      	// Sleep

	while (readReg(REG_IRQFLAGS2) & (IRQ2_FIFONOTEMPTY | IRQ2_FIFOOVERRUN))		// Flush FIFO
        readReg(REG_FIFO);
	writeReg(0x30, group);                                    // RegSyncValue2

  writeReg(REG_DIOMAPPING1, 0x00); 										      // PacketSent
		
	volatile uint8_t txstate = 0;
	byte i = 0;
	uint16_t crc = _crc16_update(~0, group);	

	while(txstate < 7)
	{
		if ((readReg(REG_IRQFLAGS2) & IRQ2_FIFOFULL) == 0)			// FIFO !full
		{
			uint8_t next = 0xAA;
			switch(txstate)
			{
			  case 0: next=node & 0x1F; txstate++; break;    		  // Bits: CTL, DST, ACK, Node ID(5)
			  case 1: next=size; txstate++; break;				   	    // No. of payload bytes
			  case 2: next=data[i++]; if(i==size) txstate++; break;
			  case 3: next=(byte)crc; txstate++; break;
			  case 4: next=(byte)(crc>>8); txstate++; break;
			  case 5:
			  case 6: next=0xAA; txstate++; break; 					      // dummy bytes (if < 2, locks up)
			}
			if(txstate<4) crc = _crc16_update(crc, next);
			writeReg(REG_FIFO, next);								              // RegFifo(next);
		}
	}
    //transmit buffer is now filled, transmit it
	writeReg(REG_OPMODE, (readReg(REG_OPMODE) & 0xE3) | MODE_TRANSMITTER);		// Transmit mode - 56 Bytes max payload
   
  rfm_sleep();
  return success;
    
}

void rfm_sleep(void)
{   
    // Put into sleep mode when buffer is empty
	while (!(readReg(REG_IRQFLAGS2) & IRQ2_PACKETSENT))				// wait for transmission to complete (not present in JeeLib) 
	    delay(1);													//   

	writeReg(REG_OPMODE, (readReg(REG_OPMODE) & 0xE3) | 0x01); 		// Standby Mode	
} 



void writeReg(uint8_t addr, uint8_t value)
{
	select();
	SPI1.transfer(addr | 0x80);
	SPI1.transfer(value);
	unselect();
}

uint8_t readReg(uint8_t addr)
{
	select();
	SPI1.transfer(addr & 0x7F);
	uint8_t regval = SPI1.transfer(0);
	unselect();
	return regval;
}

// select the transceiver
void select() {
	noInterrupts();
	SPI1.setDataMode(SPI_MODE0);
	SPI1.setBitOrder(MSBFIRST);
	SPI1.setClockDivider(SPI_CLOCK_DIV4); // decided to slow down from DIV2 after SPI stalling in some instances, especially visible on mega1284p when RFM69 and FLASH chip both present
	digitalWrite(RFMSELPIN, LOW);
}

// UNselect the transceiver chip
void unselect() {
	digitalWrite(RFMSELPIN, HIGH);
	interrupts();
}

uint16_t _crc16_update(uint16_t crc, uint8_t a)
{
  int i;

  crc ^= a;
  for (i = 0; i < 8; ++i)
  {
    if (crc & 1)
  crc = (crc >> 1) ^ 0xA001;
    else
  crc = (crc >> 1);
  }

  return crc;
  }
