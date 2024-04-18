---
github_url: "https://github.com/openenergymonitor/emontx4/blob/main/docs/firmware.md"
---

# Firmware

The emonTx v4 firmware can be edited and compiled using [PlatformIO](https://platfomrio.org/) (recommended) or [Arduino IDE](https://www.arduino.cc/) with [DxCore installed](https://github.com/SpenceKonde/DxCore). DxCore is an Arduino core for the AVR-DB microcontroller range, developed by SpenceKonde.

## Available Firmware

**[EmonTxV4:](https://github.com/openenergymonitor/emontx4/tree/main/firmware/EmonTxV4)** Single phase, 6 CT channel, continuous sampling, cumulative energy persisted to EEPROM, LowPowerLabs RFM69 radio format (JeeLib also supported via #define), 3x DS18B20 temperature sensors supported by default, serial configuration and data output.

Pre-compiled hex: [https://github.com/openenergymonitor/emontx4/releases/tag/1.5.7](https://github.com/openenergymonitor/emontx4/releases)

**[EmonTxV4_6x_temperature:](https://github.com/openenergymonitor/emontx4/tree/main/firmware/EmonTxV4_6x_temperature)** As above but configured to read and transmit 6x DS18B20 temperature sensor readings. Please see [EmonTx4 DS18B20 Temperature sensing & firmware release 1.5.7](https://community.openenergymonitor.org/t/emontx4-ds18b20-temperature-sensing-firmware-release-1-5-7/23496/2) for note on performance implications that also apply to the above 3x temperature sensor example.

**[EmonTxV4_heatpump:](https://github.com/openenergymonitor/emontx4/tree/main/firmware/EmonTxV4_heatpump)** Single phase, 3 CT channel, continuous sampling, cumulative energy persisted to EEPROM, LowPowerLabs RFM69 radio format (JeeLib also supported via #define), 4x DS18B20 temperature sensors supported by default, serial configuration and data output. Designed for use with Sika VFS flow meter, measures the analog voltage output to calculate flow rate, which combined with flow and return temperature measurements allow calculation of heat pump heat output.

**[EmonTxV4_DB_3phase_6CT:](https://github.com/openenergymonitor/emontx4/tree/main/firmware/EmonTxV4_DB_3phase_6CT)** 3-phase 6 CT channel firmware using the emonLibDB library, continuous sampling, cumulative energy persisted to EEPROM, LowPowerLabs RFM69 radio format (JeeLib also supported via #define). Serial configuration and data output. Please note that temperature sensing is not supported.

**[EmonTx4_DB_12CT_serial:](https://github.com/openenergymonitor/emontx4/tree/main/firmware/EmonTx4_DB_12CT_serial)** 12 CT channel firmware using the emonLibDB library, continuous sampling. Serial output only for use with EmonESP ESP8266 WiFi module or direct USB connection to an emonBase/emonPi.

## Updating firmware using an emonPi/emonBase (recommended)

The easiest way of updating the emonTx4 firmware is to connect it to an emonBase with a USB cable and then use the firmware upload tool available at `Setup > Admin > Update > Firmware`.

```{warning} 

**System update may be required:** If you don not see the latest firmware version as listed above in the firmware list a full system update is required first.
```

**Note: Upload via USB-C will only work if connected in the right orientation. Try turning the USB-C connector around if upload fails.** Some USB-C connectors have a smooth side on one side and jagged connection of the metal fold on the other. On the cables we have here, the smooth side should be facing up towards the top/front face of the emonTx4:

![usbc_orientation1.jpeg](img/usbc_orientation1.jpeg)

![usbc_orientation2.jpeg](img/usbc_orientation2.jpeg)

Refresh the update page after connecting the USB cable. You should now see port `ttyUSB0` appear in the 'Select port` list.

![emonsd_firmware_upload.png](img/emonsd_firmware_upload2.png)

Double check the serial port, this is likely to be 'ttyUSB0' when plugged in via USB device connected. Select 'emonTx4' from hardware.

The standard radio format is 'LowPowerLabs', if you wish to use the emonTx4 with an existing system running JeeLib classic radio format you can select the JeeLib classic radio format here.

## Upload pre-compiled using EmonScripts emonupload2 tool 

On the emonPi/emonBase ensure EmonScripts is updated to latest version then run emonupload2 tool 

    /opt/openenergymonitor/EmonScripts/emonupload2.py

Select hardware then firmware version

```
Select hardware:
  1. emonTx4
  2. emonPi
  3. emonTx3
  4. rfm69pi
  5. rfm12pi
  6. emonTH2
  7. emonTH1
Enter number:1

Select firmware:
1. emonTxV4_LPL
2. emonTxV4_JeeLib_Native
3. emonTxV4_JeeLib_Classic
4. EmonTxV4_DB_3phase_6CT
```

emonupload2 tool can also be run on any other linux computer by cloning the EmonScripts repo then running the emonupload2.py python script. Python3 required 

    git clone https://github.com/openenergymonitor/EmonScripts

## Upload pre-compiled manually using avrdude

Alternatively to upload the same pre-compiled firmware via command line on emonPi / emonBase: 

    avrdude -C/opt/openenergymonitor/EmonScripts/update/avrdude.conf -v -pavr128db48 -carduino -D -P/dev/ttyUSB0 -b115200 -Uflash:w:EmonTxV4_LPL.hex:i 

Or using differant computer, ensure `avrdude.conf` has `avr128db48` entry i.e DxCore see below instructions 

    avrdude -Cavrdude.conf -v -pavr128db48 -carduino -D -P/dev/ttyUSB0 -b115200 -Uflash:w:EmonTxV4_LPL.hex:i 
    
You will need avrdude installed (tested on version 6.3-2017) and the custom DxCore avrdude.conf. This can be downloaded here: [DxCore avrdude.conf](https://raw.githubusercontent.com/openenergymonitor/emontx4/main/firmware/avrdude.conf).

## How to compile and upload firmware

### Compile and Upload using PlatformIO (recommended)

Clone the emonTx V4 repo 

    git clone https://github.com/openenergymonitor/emontx4
    cd emontx4/firmware/EmonTxV4
    
Install PlatformIO core then to compile and upload  

    pio run -t upload

On first run PlatformIO will download automatically all the required libraries. You can also use the PlatformIO GUI. 

### Compile and Upload using Arduino IDE 

If you don’t already have the Arduino IDE it can be downloaded from here:<br>
[https://www.arduino.cc/en/software](https://www.arduino.cc/en/software)

Once you have the IDE installed, you then need to install [Spence Konde’s DxCore](https://github.com/SpenceKonde/DxCore). This can be done by first pasting the following board manager URL in Arduino IDE > File > Preferences:

    http://drazzy.com/package_drazzy.com_index.json

Then navigating to: *Tools > Boards > Boards Manager*, Select “DxCore by Spence Konde” and click Install. 

![install_dxcore.png](img/install_dxcore.png)

For more information on DxCore installation see: [https://github.com/SpenceKonde/DxCore/blob/master/Installation.md](https://github.com/SpenceKonde/DxCore/blob/master/Installation.md).

**Libraries**

Locate or create your Arduino Sketchbook directory (location found in Arduino preferences). If it doesnt already exist, create a directory called libraries in the Sketchbook directory and install the following libraries:


1\. Download EmonLibCM library (avrdb branch)<br>
[https://github.com/openenergymonitor/EmonLibCM/tree/avrdb](https://github.com/openenergymonitor/EmonLibCM/tree/avrdb)

2\. Download EmonLibDB library (main branch)<br>
[https://github.com/openenergymonitor/emonLibDB](https://github.com/openenergymonitor/emonLibDB)

2\. Download emonEProm library (avrdb branch)<br>
[https://github.com/openenergymonitor/emonEProm/tree/avrdb](https://github.com/openenergymonitor/emonEProm/tree/avrdb)

3\. Download RFM69_LPL library (main branch)<br>
[https://github.com/openenergymonitor/RFM69_LPL](https://github.com/openenergymonitor/RFM69_LPL)

4\. Download RFM69_JeeLib library (avrdb branch)<br>
[https://github.com/openenergymonitor/RFM69_JeeLib/tree/avrdb](https://github.com/openenergymonitor/RFM69_JeeLib/tree/avrdb)

5\. Download DxCore SpenceKonde OneWire library:<br>
[https://github.com/SpenceKonde/OneWire](https://github.com/SpenceKonde/OneWire)

**Compilation settings:**

With DxCore and the libraries installed the firmware should then compile. 

Under Tools, select the following configuration options:

- Select Board "AVR DB-series (Optiboot)"
- Select Chip: AVR128DB48
- Clock Speed: 24 MHz Crystal
- Bootloader serial port: UART3: TXPB0, RXPB1

Select Board "AVR DB-series (Optiboot)"

![firmware_dxcore_option.png](img/firmware_dxcore_option.png)

Select Chip: AVR128DB48

![firmware_core_option.png](img/firmware_core_option.png)

Bootloader serial port: UART3: TXPB0, RXPB1

![firmware_uart_option.png](img/firmware_uart_option.png)

