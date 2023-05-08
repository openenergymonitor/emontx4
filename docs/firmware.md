---
github_url: "https://github.com/openenergymonitor/emontx4/blob/main/docs/firmware.md"
---

# Firmware

The emonTx v4 firmware can be edited and compiled using the [Arduino IDE](https://www.arduino.cc/) with [DxCore installed](https://github.com/SpenceKonde/DxCore).
DxCore is an Arduino core for the AVR-DB microcontroller range, developed by SpenceKonde.

## Available Firmware

**[EmonTxV4 (lastest v1.5.7):](https://github.com/openenergymonitor/emontx4/tree/main/firmware/EmonTxV4)** Single phase, 6 CT channel, continuous sampling, cumulative energy persisted to EEPROM, LowPowerLabs RFM69 radio format (JeeLib also supported via #define), 3x DS18B20 temperature sensors supported by default, serial configuration and data output.

Pre-compiled hex v1.5.7: [https://github.com/openenergymonitor/emontx4/releases/tag/1.5.7](https://github.com/openenergymonitor/emontx4/releases/tag/1.5.7)

**[EmonTxV4_6x_temperature:](https://github.com/openenergymonitor/emontx4/tree/main/firmware/EmonTxV4_6x_temperature)** As above but configured to read and transmit 6x DS18B20 temperature sensor readings. Please see [EmonTx4 DS18B20 Temperature sensing & firmware release 1.5.7](https://community.openenergymonitor.org/t/emontx4-ds18b20-temperature-sensing-firmware-release-1-5-7/23496/2) for note on performance implications that also apply to the above 3x temperature sensor example.

**[EmonTxV4_heatpump:](https://github.com/openenergymonitor/emontx4/tree/main/firmware/EmonTxV4_heatpump)** Single phase, 3 CT channel, continuous sampling, cumulative energy persisted to EEPROM, LowPowerLabs RFM69 radio format (JeeLib also supported via #define), 4x DS18B20 temperature sensors supported by default, serial configuration and data output. Designed for use with Sika VFS flow meter, measures the analog voltage output to calculate flow rate, which combined with flow and return temperature measurements allow calculation of heat pump heat output.

**[EmonTxV4_DB_3phase_6CT:](https://github.com/openenergymonitor/emontx4/tree/main/firmware/EmonTxV4_DB_3phase_6CT)** New 3-phase 6 CT channel firmware using the emonLibDB library, continuous sampling, cumulative energy persisted to EEPROM, LowPowerLabs RFM69 radio format (JeeLib also supported via #define). Serial configuration and data output. Please note that temperature sensing is not supported.

## Updating firmware using an emonPi/emonBase

The easiest way of updating the emonTx4 firmware is to connect it to an emonBase with a USB cable and then use the firmware upload tool available at `Setup > Admin > Update > Firmware`.

```{warning} 

**System update may be required:** If you don not see the latest firmware version as listed above in the firmware list a full system update is required first.
```

**Note: Upload via USB-C will only work if connected in the right orientation. Try turning the USB-C connector around if upload fails.** Some USB-C connectors have a smooth side on one side and jagged connection of the metal fold on the other. On the cables we have here, the smooth side should be facing up towards the top/front face of the emonTx4:

![usbc_orientation1.jpeg](img/usbc_orientation1.jpeg)

![usbc_orientation2.jpeg](img/usbc_orientation2.jpeg)

Refresh the update page after connecting the USB cable. You should now see port `ttyUSB0` appear in the 'Select port` list.

![emonsd_firmware_upload.png](img/emonsd_firmware_upload2.png)

Double check the serial port, this is likely to be 'ttyUSB0' when plugged in via USB. Select 'emonTx4' from hardware.

The standard radio format is 'LowPowerLabs', if you wish to use the emonTx4 with an existing system running JeeLib classic radio format you can select the JeeLib classic radio format here.

## Uploading pre-compiled firmware

Alternatively to upload the same pre-compiled firmware via command line using avrdude, the command to do this is:

    avrdude -Cavrdude.conf -v -pavr128db48 -carduino -D -P/dev/ttyUSB0 -b115200 -Uflash:w:EmonTxV4_LPL.hex:i 
    
You will need avrdude installed (tested on version 6.3-2017) and the custom DxCore avrdude.conf. This can be downloaded here: [DxCore avrdude.conf](https://raw.githubusercontent.com/openenergymonitor/emontx4/main/firmware/avrdude.conf).

## How to compile and upload firmware:

If you don’t already have the Arduino IDE it can be downloaded from here (As far as Im aware this needs to be the legacy version 1.8.19 for now):<br>
[https://www.arduino.cc/en/software](https://www.arduino.cc/en/software)

Once you have the IDE installed, you then need to install [Spence Konde’s DxCore](https://github.com/SpenceKonde/DxCore). This can be done by first pasting the following board manager URL in Arduino IDE > File > Preferences:

    http://drazzy.com/package_drazzy.com_index.json

Then navigating to: *Tools > Boards > Boards Manager*, Select “DxCore by Spence Konde” and click Install. 

![install_dxcore.png](img/install_dxcore.png)

For more information on DxCore installation see: [https://github.com/SpenceKonde/DxCore/blob/master/Installation.md](https://github.com/SpenceKonde/DxCore/blob/master/Installation.md).

**Libraries**

Next install the libraries used by the main firmware, download and place these in your Arduino libraries directory. 

The libraries folder is a folder that must be added to your Arduino Sketchbook directory (location found in Arduino preferences). Simply create the folder named libraries in that folder and place the libraries you need, there.

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

