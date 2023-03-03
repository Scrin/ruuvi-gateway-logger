# ruuvi-gateway-logger

ESP8266 based NodeMCU remote logger for Ruuvi Gateway.
Ruuvi Gateway prints all kinds of useful logs over the serial, but unless you have plugged the gateway directly into a computer or other usb host capable device, you can't really get to the logs.
So, instead of that, you can solder some wires onto the gateway PCB and connect them to a ESP8266 based NodeMCU which reads the serial and forwards the messages over to a syslog server and/or MQTT. More info in [this](https://f.ruuvi.com/t/ruuvi-gateway-logs-to-a-remote-syslog-server-the-diy-way/6130) forum thread.

## Setup

- Clone/download this repository
- Copy `include/config.h.example` as `include/config.h`
- Set your configuration in `config.h`
- Build/flash like any other PlatformIO project

## Over The Air update

(for the logger, not the gateway)

Documentation: https://arduino-esp8266.readthedocs.io/en/latest/ota_updates/readme.html#web-browser

Basic steps:

- Use PlatformIO: Build
- Browse to http://IP_ADDRESS/update
- Select .pio/build/nodemcuv2/firmware.bin from work directory as Firmware and press Update Firmware
