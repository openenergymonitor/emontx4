# EmonTx v4

See forum post: [https://community.openenergymonitor.org/t/avr-db-emontx-v4-new-hardware-in-progress/20209](https://community.openenergymonitor.org/t/avr-db-emontx-v4-new-hardware-in-progress/20209)

![emontx4.jpg](emontx4.jpg)

Key features

    Micro-controller: AVR128DB48 (VQFN 128 kb flash, 48-pin package)
    3 voltage channels (via RJ11 connector top-right, interfaces with separate voltage sensing board)
    5 CT current channels on the base board. [Edit: Subsequently extended to 6 in post no.50 (RW)]
    Option to extend the number of CT channels to 11 12 via an additional extender board.
    Precision voltage reference for the ADC to improve accuracy.
    RJ45 socket for pulse counter and DS18B20 temperature sensing.
    3x 3-WAY pluggable terminal blocks for selectable temperature sensing, pulse or additional analog input.
    RFM69CW 433MHz radio transceiver
    DIP Switch to select node id
    USB-C power and integrated CP2102 usb to serial chip
    Serial and SPI pin headers designed for ESP8266+I2C LCD based shield module (more details soon).


![schematic.png](hardware/schematic.png)

![board2.png](hardware/board2.png)
