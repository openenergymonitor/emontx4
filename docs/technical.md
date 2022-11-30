# Technical Guide

## RFM69 Radio

The emonTx4 includes a 433 MHz RFM69CW radio on the board. This can be used to transmit data to an emonPi or emonBase base station. These radios provide a neat and simple way of sending data between devices.

The emonTx4 supports 3 different RFM69 radio formats:

1. LowPowerLabs (Default & Recommended)
2. JeeLib Classic (backwards compatibility mode)
3. JeeLib Native

There is a shop option to select between Standard (LowPowerLabs) and Backwards compatibility (JeeLib Classic) when ordering an emonTx4.

Pre-compiled hex files are available for all three of these. The format can also be chosen when compiling the firmware.

The LowPowerLabs and JeeLib Native formats both support encryption. Please see encryption section below for details on how to secure this.

### Background on libraries and formats

We have used the RFM69 radio now for quite a few years but the release of the emonTx4 brings with it an important change in the software implementation used for the radio. Up until this point we had been using the **JeeLib 'Classic' packet format.** This was built around compatibility with an earlier generation radio module called RFM12. The benefit of backwards compatibility however meant that some of the new features implemented on the RFM69 module could not be used. These included the FIFO data buffers which reduce the requirement to interrupt the main microcontroller (useful for energy monitoring) and hardware encryption. 

Jean Claude Wippler who originally developed the JeeLib library did develop a version of the library that made use of the new RFM69 features. He called the resulting packet format the **'Native' packet format**. There's a useful post on the differences [here](https://jeelabs.org/book/1522a/index.html). More recently Robert Wall adapted this JeeLib native library for use with the emonPi, to make continuous sampling possible.

The **LowPowerLabs packet format and library** is a separately developed project by [Felix Rusu of LowPowerLabs](https://github.com/LowPowerLab/RFM69). In terms of packet format and implementation it is in many ways similar to the JeeLib Native packet format library - in that it uses the native RFM69 module features. The radio settings such as bit rate are subtly different (55,555 bits/s vs 49,230 bits/s). The header section of the radio packet payload is also slightly differently enabling 1023 possible nodes, 256 possible networks and a control byte used for requesting acknowledgments.

**The emonTx4 uses the LowPowerLabs radio format by default.**<br>
The release of the emonTx4 was originally going to include the switch from the JeeLib Classic format to the JeeLib Native format, but in the end we've decided to make the switch to the LowPowerLabs library instead. 

This significant change was made primarily in order to make use of the packet acknowledgment and retry feature implemented in the LowPowerLabs library, which in testing was critical to bring packet loss down to the magic 0% level (with nodes within good signal range).

The second important benefit of this change is to have a radio format that is compatible with the wider actively developed LowPowerLabs RFM69 ecosystem. 

**Uncoordinated vs coordinated transmit**<br>
The present implementation can still be improved further, OpenEnergyMonitor nodes are still transmitting radio data in an uncoordinated way, which increases the likelihood of packet collisions and therefore packet loss. Nodes do listen briefly for a quiet radio window and the emonTx4 requests a packet acknowledgment, retrying if not heard, but there is a limitation to how many nodes that can practically be accommodated using this scheme alone.

The next step in development is to explore the option to either have the base station request data from each radio node in sequence or for the base station to broadcast some kind of timing coordination packet, say every 10s. Nodes would then offset the time at which they transmit by some delay relating to their unique node id's.

### Encryption

Encryption is enabled by default when using the LowPowerLabs or JeeLib Native packet formats.

**It is not however secure as the encryption key used is publicly available.** There is not yet a mechanism in place to generate and distribute the encryption key's to radio nodes in an automated way, or as part of an initial pairing process.

The default encryption key is currently hard coded in the firmware, e.g on line 230 of the emonTx4 firmware:

```
rf.encrypt("89txbe4p8aik5kt3");
```

In order to truly secure the radio network you can create your own secret 16 character encryption key. This encryption key needs to be copied to all radio nodes on the network, including the base-station.

The emonBase available for purchase with the emonTx4 in the OpenEnergyMonitor shop comes with an SPI RFM69 receiver board. The encryption key used for this is currently hard-coded in emonHub on line 51: [https://github.com/openenergymonitor/emonhub/blob/master/src/interfacers/EmonHubRFM69LPLInterfacer.py#L51](https://github.com/openenergymonitor/emonhub/blob/master/src/interfacers/EmonHubRFM69LPLInterfacer.py#L51).

This key needs to be modified here to match the secret key used on the emonTx4. The result will be a secure, encrypted radio network.





