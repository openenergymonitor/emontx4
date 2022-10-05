# EmonTx v4 Pi Zero Extension Board

![pizero_ext.jpeg](pizero_ext.jpeg)

Starting with an SD card running emonSD:<br>
https://github.com/openenergymonitor/emonpi/wiki/emonSD-pre-built-SD-card-Download-&-Change-Log

Configure EmonHub to use the EmonHubOEMInterfacer:<br>
https://github.com/openenergymonitor/emonhub/tree/master/conf/interfacer_examples/OEM

    [[OEM]]
        Type = EmonHubOEMInterfacer
        [[[init_settings]]]
            com_port = /dev/ttyAMA0
            com_baud = 115200
        [[[runtimesettings]]]
            pubchannels = ToEmonCMS,
