import network
import socket
from time import sleep
import machine
from machine import UART, Pin

import urequests
import uselect
import sys

ssid = 'SSID'
password = 'PASSKEY'

def connect():
    #Connect to WLAN
    wlan = network.WLAN(network.STA_IF)
    wlan.active(True)
    wlan.connect(ssid, password)
    while wlan.isconnected() == False:
        print('Waiting for connection...')
        sleep(1)
    ip = wlan.ifconfig()[0]
    print(f'Connected on {ip}')
    
try:
    connect()
except Exception as e:
    machine.reset()

buffered_input = []

uart0 = UART(0) # defaults 115200 baud

while(True):
    
    while uart0.any() > 0:
        c = chr(uart0.read(1)[0])
        buffered_input.append(c)

    if '\n' in buffered_input:
        line_ending_index = buffered_input.index('\n')
        data = "".join(buffered_input[:line_ending_index]).strip()
        buffered_input = []
        
        if data[0:3]=="MSG":
            req = "https://emoncms.org/input/post?apikey=apikey&node=emontx4pico&data="+data
            print(req)
            r = urequests.get(req)
            print(r.content)
            
    sleep(0.1)
