# EmonTx4, emonVs & emonBase Install Guide

The following guide covers installation of the EmonTx4 6x input energy monitor in combination with an emonBase (RaspberryPi base-station). Using an EmonTx with an emonBase provides full local emoncms data logging and visualisation capabilities as well as the option to expand data input from other devices.

The following EmonTx4, emonVs and emonBase bundle will soon be available in the OpenEnergyMonitor shop with all the parts needed to build this configuration:

![emontx4emonbase.jpg](img/emontx4emonbase.jpg)

**Included in the bundle**

- EmonTx v4: 6 input electricity monitor
- EmonVs: Precision voltage sensor and power supply
- EmonBase: RaspberryPi base-station
- Up to 6 CT sensors
- RJ11 voltage sensor cable
- USB-C data cable for wired connection between the EmonTx4 and emonBase

**Steps to install**

1. Plug CT sensors into emonTx via 3.5mm jack plugs

2. Clip CT sensors around Live OR Neutral cable of the AC circuit to be measured (not both)

3. Plug emonTx into Raspberry Pi base-station USB-A port using USB-C to USB-A cable

4. Plug emonVS RJ11 cable into emonTx 

5. Plug emonVS USB-C cable into  Raspberry Pi base-station power input connector

6. Plug emonVS into mains power via a domestic wall socket

7. (optionally) connect Raspberry Pi base-station to Ethernet 

8. Switch on mains socket and verify that the green LED on the emonTx and the red LED on the Raspberry Pi illuminates

9. After a few moments the Raspberry Pi will create a WiFi Access Point called ‘emonpi’, connect to this using password ‘emonpi2016’

10. Browse the IP address http://192.168.42.1 and follow the setup wizard to connect the device to your local WiFi network

9. Once connected to your local WiFi network the base-station can be accessed via http://emonpi or http://emonpi.local

**Instructions for safe use:**

    • Clip-on CT sensors are non-invasive and should not have direct contact with the AC mains. As a precaution, we recommend ensuring all cables are fully isolated prior to installing. If in doubt seek professional assistance.
    • Do not expose to water or moisture 
    • Do not expose to temperate above rated operating limits 
    • Indoor use only
    • Do not connect unapproved accessories 
    • Please contact us if you have any questions 
