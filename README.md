# greenhouse-firmware
Firmware for a smart greenhouse based around a ESP-12F NodeMCU.

## Setup

Clone this repository using `git clone --recurse-submodules` and open it up in the PlatformIO IDE (vscode).

Copy `include/wifi_config.h.example` to `include/wifi_config.h` and configure your network.

## Hardware and Software Components
 - ESP-12F NodeMCU controller
   - Arduino framework
 - KY-015 temperature and humidity sensor
   - adafruit/DHT sensor library
   - adafruit/Adafruit Unified Sensor
 - MQTT client
   - ottowinter/AsyncMqttClient-esphome
   - Fork of me-no-dev/ESPAsyncTCP: [mhightower83/ESPAsyncTCP](https://github.com/mhightower83/ESPAsyncTCP/tree/correct-ssl-_recv), contains SSL patches
