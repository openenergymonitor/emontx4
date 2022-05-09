# EmonTx v4

See forum post: [https://community.openenergymonitor.org/t/avr-db-emontx-v4-new-hardware-in-progress/20209](https://community.openenergymonitor.org/t/avr-db-emontx-v4-new-hardware-in-progress/20209)

The first set of emonTx v4 prototype PCB's have arrived. Here's a picture of the first assembled board, testing in progress!

![emontx4.jpg](emontx4.jpg)

**Key features**

* Micro-controller: AVR128DB48 (VQFN 128 kb flash, 48-pin package)
* 3 voltage channels (via RJ11 connector top-right, interfaces with separate voltage sensing board)
* 5 CT current channels on the base board. *[Edit: Subsequently extended to 6 in post no.50 (RW)]*
* Option to extend the number of CT channels to [s]11[/s] 12 via an additional extender board.
* Precision voltage reference for the ADC to improve accuracy.
* RJ45 socket for pulse counter and DS18B20 temperature sensing.
* 3x 3-WAY pluggable terminal blocks for selectable temperature sensing, pulse or additional analog input.
* RFM69CW 433MHz radio transceiver
* DIP Switch to select node id
* USB-C power and integrated CP2102 usb to serial chip
* Serial and SPI pin headers designed for ESP8266+I2C LCD based shield module (more details soon).

The dimensions of this board are slightly smaller than that of the emonTx v3 at 65mm long rather than 80mm this provides a proportional saving on the cost and material requirements for the aluminium enclosure, which goes part of the way to offset the additional cost of some of the additional components on the board.

The board is designed to support both single and full 3-phase electricity monitoring when used in conjunction with the relevant external voltage sensing board. And the CT extender pin headers should provide a cost effective way of gaining additional CT channels.

The on-board USB to serial chip will make it easier and neater to have a hard-wired installation linking the emonTx to a RaspberryPi based base station.

## Schematic and board file images:

![schematic.png](hardware/schematic.png)

![board2.png](hardware/board2.png)
