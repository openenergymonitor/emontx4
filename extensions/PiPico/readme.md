# EmonTx v4 Pi Pico Extension Board

![pipico_ext.jpeg](pipico_ext.jpeg)

### Software Examples

- [MicroPython: Basic emoncms client](https://github.com/openenergymonitor/emontx4/tree/main/extensions/PiPico/examples/micropython)
- [Arduino: Basic emoncms client](https://github.com/openenergymonitor/emontx4/tree/main/extensions/PiPico/examples/arduino/basic_emoncms_client)

### Uploading MicroPython

1. Download the latest MicroPython Pi Pico W build from here: https://micropython.org/download/rp2-pico-w/ and drop it onto the drive created by the Pi Pico when plugged in via USB to your computer.

2. Install Adafruit Ampy, see: https://pypi.org/project/adafruit-ampy/

3. Upload the micropython example using ampy, e.g:

    ampy --port /dev/ttyACM0 put basic_emoncms_client.py main.py
