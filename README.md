# Smart Greenhouse Firmware
Firmware for a smart greenhouse based around a [ESP-12F](https://www.espressif.com/) [NodeMCU](https://www.nodemcu.com/index_en.html). Implements the MQTT protocol.

## Setup

Clone this repository using `git clone --recurse-submodules` and open it up in the [PlatformIO IDE](https://platformio.org/).

Copy `include/wifi_config.h.example` to `include/wifi_config.h` and configure your network. You can also edit `include/hardware_config.h` if your pin layout differs.

For debugging connect the board via USB and open a serial console at 9600 BAUD.

## Hardware and Software Components
 - ESP-12F NodeMCU controller
   - [Arduino framework](https://www.arduino.cc/)
 - KY-015 temperature and humidity sensor
   - [adafruit/DHT sensor library](https://platformio.org/lib/show/19/DHT%20sensor%20library)
   - [adafruit/Adafruit Unified Sensor](https://platformio.org/lib/show/31/Adafruit%20Unified%20Sensor)
 - MH-Sensor Series Flying Fish & Soil Sensor
   - Analog input
 - Small submergible water pump
   - Digital output & power MOSFET
 - MQTT client
   - [ottowinter/AsyncMqttClient-esphome](https://platformio.org/lib/show/6759/AsyncMqttClient-esphome)
   - Fork of [me-no-dev/ESPAsyncTCP](https://platformio.org/lib/show/305/ESPAsyncTCP): [mhightower83/ESPAsyncTCP](https://github.com/mhightower83/ESPAsyncTCP/tree/correct-ssl-_recv), contains SSL patches

## Home Assistant Example

Assuming you have set up a MQTT Broker like [Mosquitto](https://mosquitto.org/) and connected it to [Home Assistant](https://www.home-assistant.io/), you can create a simple integration like this:

```yaml
sensor greenhouse-air-temp:
  name: Greenhouse Air Temperature
  platform: mqtt
  state_topic: "/greenhouse/air/temperature"
  unit_of_measurement: "°C"
  device_class: temperature

sensor greenhouse-air-hum:
  name: Greenhouse Air Humidity
  platform: mqtt
  state_topic: "/greenhouse/air/humidity"
  unit_of_measurement: "%"
  device_class: humidity

sensor greenhouse-air-hidx:
  name: Greenhouse Air Heat Index
  platform: mqtt
  state_topic: "/greenhouse/air/heatindex"
  unit_of_measurement: "°C"
  device_class: temperature

sensor greenhouse-soil-moist:
  name: Greenhouse Soil Moisture
  platform: mqtt
  state_topic: "/greenhouse/soil/moisture"
  unit_of_measurement: "%"
  device_class: humidity

switch greenhouse-pump:
  name: Greenhouse Pump Power
  platform: mqtt
  command_topic: "/greenhouse/pump/power"
  qos: 2
  payload_off: 0
  payload_on: 1
  state_topic: "/greenhouse/pump/power"
```