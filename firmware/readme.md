## Firmware

The emonTx v4 firmware can be edited and compiled using the [Arduino IDE](https://www.arduino.cc/) with [DxCore installed](https://github.com/SpenceKonde/DxCore).
DxCore is an Arduino core for the AVR-DB microcontroller range, developed by SpenceKonde.

### Available Firmware

**[EmonTxV4CM_rfm69n:](EmonTxV4CM_rfm69n)** Single phase, 6 CT channel, continuous sampling, native RFM69 radio firmware for the emonTx v4.

**[EmonTxV4CM:](EmonTxV4CM)** Single phase, 6 CT channel, continuous sampling, jeelib format RFM69 radio firmware for the emonTx v4.

### Pre-compiled firmware:

Command line upload:

    avrdude -Cavrdude.conf -v -pavr128db32 -carduino -D -P/dev/ttyUSB0 -b115200 -Uflash:w:EmonTxV4CM_rfm69n.ino.hex:i 

### How to compile and upload firmware:

If you don’t already have the Arduino IDE it can be downloaded from here:<br>
[https://www.arduino.cc/en/software](https://www.arduino.cc/en/software)

Once you have the IDE installed, you then need to install [Spence Konde’s DxCore](https://github.com/SpenceKonde/DxCore). This can be done by first pasting the following board manager URL in Arduino IDE > File > Preferences:

    http://drazzy.com/package_drazzy.com_index.json

Then navigating to: *Tools > Boards > Boards Manager*, Select “DxCore by Spence Konde” and click Install. 

![install_dxcore.png](img/install_dxcore.png)

For more information on DxCore installation see: [https://github.com/SpenceKonde/DxCore/blob/master/Installation.md](https://github.com/SpenceKonde/DxCore/blob/master/Installation.md).

**Libraries**

Next install the libraries used by the main firmware, download and place these in your Arduino libraries folder.

1\. Download EmonLibCM library (avrdb branch)<br>
https://github.com/openenergymonitor/EmonLibCM/tree/avrdb

2\. Download emonEProm library (avrdb branch)<br>
https://github.com/openenergymonitor/emonEProm/tree/avrdb

3\. Download rfm69nTxLib library (avrdb branch)<br>
https://github.com/openenergymonitor/rfm69nTxLib/tree/avrdb

**Compilation settings:**

With DxCore and the libraries installed the firmware should then compile. 

Under Tools, select the following configuration options:

- Select Chip: AVR128DB48
- Clock Speed: 24 MHz Internal
- Bootloader serial port: UART3: TXPB0, RXPB1

![compile_settings.png](img/compile_settings.png)

