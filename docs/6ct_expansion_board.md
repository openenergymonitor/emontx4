# 6 CT Expansion board

The 6 CT Expansion board adds a further 6x CT inputs to the EmonTx4, giving a total of 12 CT inputs. These can be used with the new EmonLibDB based firmware that supports both single phase and 3 phase monitoring. Individual CT inputs can be configured for 12 single phase inputs or 4x 3 phase inputs. A custom mix of single and 3 phase inputs can also be configured by modifying the firmware and uploading firmware manual.

## Installation guide

**Upgrading:** If ordered seperately to the EmonTx4 the expansion board comes with a 12 CT fascia to replace the standard 6 CT version. A 9-way header is also included that needs to be soldered in place on the EmonTx4.

![](img/6CT/6ct_ext1.jpg)

Place the 9 way header on the top side of the board as circled below:

![](img/6CT/6ct_ext2.jpg)

Solder on the bottom side taking care not to bridge across the pins:

![](img/6CT/6ct_ext3.jpg)

**Note:** The mechanical attachment of the expansion board is not very secure. It relies fully on the 9 way PCB header connection. Care needs to be taken when plugging in expansion board CTs as the board can flex on the header. This is something we plan to fix on future versions.

We recommend connecting the CT sensors through the face plate, outside of the aluminium enclosure first and then attaching the face plate to the enclosure second. See second picture below.

![](img/6CT/6ct_ext4.jpg)

![](img/6CT/6ct_ext5.jpg)

![](img/6CT/6ct_ext6.jpg)
