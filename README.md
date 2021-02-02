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
 - BH1750 GY-302 Lux sensor
   - [claws/BH1750](https://platformio.org/lib/show/439/BH1750)
 - VEML6070 UV sensor
   - [adafruit/Adafruit VEML6070 Library](https://platformio.org/lib/show/2929/Adafruit%20VEML6070%20Library)
 - MH-Sensor Series Flying Fish & Soil Sensor via ADS115
   - [adafruit/Adafruit ADS1X15](https://platformio.org/lib/show/342/Adafruit%20ADS1X15)
 - Small submergible water pumps
   - Digital output & power MOSFET
 - Small 5V PC Fans
   - PWM output & power MOSFET
 - MQTT client
   - [knolleary/PubSubClient](https://platformio.org/lib/show/89/PubSubClient)

## Home Assistant Example

Assuming you have set up a MQTT Broker like [Mosquitto](https://mosquitto.org/) and connected it to [Home Assistant](https://www.home-assistant.io/), you can create a simple integration like this:

```yaml
sensor greenhouse-air-temp-0:
  name: Greenhouse Air 0 Temperature
  platform: mqtt
  state_topic: "/greenhouse/air/temperature/0"
  unit_of_measurement: "°C"
  device_class: temperature

sensor greenhouse-air-hum-0:
  name: Greenhouse Air 0 Humidity
  platform: mqtt
  state_topic: "/greenhouse/air/humidity/0"
  unit_of_measurement: "%"
  device_class: humidity

sensor greenhouse-air-hidx-0:
  name: Greenhouse Air 0 Heat Index
  platform: mqtt
  state_topic: "/greenhouse/air/heatindex/0"
  unit_of_measurement: "°C"
  device_class: temperature

sensor greenhouse-soil-moist-0:
  name: Greenhouse Soil 0 Moisture
  platform: mqtt
  state_topic: "/greenhouse/soil/moisture/0"
  unit_of_measurement: "%"
  device_class: humidity

sensor greenhouse-air-temp-1:
  name: Greenhouse Air 1 Temperature
  platform: mqtt
  state_topic: "/greenhouse/air/temperature/1"
  unit_of_measurement: "°C"
  device_class: temperature

sensor greenhouse-air-hum-1:
  name: Greenhouse Air 1 Humidity
  platform: mqtt
  state_topic: "/greenhouse/air/humidity/1"
  unit_of_measurement: "%"
  device_class: humidity

sensor greenhouse-air-hidx-1:
  name: Greenhouse Air 1 Heat Index
  platform: mqtt
  state_topic: "/greenhouse/air/heatindex/1"
  unit_of_measurement: "°C"
  device_class: temperature

sensor greenhouse-soil-moist-1:
  name: Greenhouse Soil 1 Moisture
  platform: mqtt
  state_topic: "/greenhouse/soil/moisture/1"
  unit_of_measurement: "%"
  device_class: humidity

sensor greenhouse-sun-lux:
  name: Greenhouse Sun Brightness
  platform: mqtt
  state_topic: "/greenhouse/sun/lux"
  unit_of_measurement: "lx"
  device_class: illuminance

sensor greenhouse-sun-uv:
  name: Greenhouse Sun UV
  platform: mqtt
  state_topic: "/greenhouse/sun/uv"
  unit_of_measurement: "μW/cm²"

sensor greenhouse-sun-uvindex:
  name: Greenhouse Sun UV Index
  platform: mqtt
  state_topic: "/greenhouse/sun/uvindex"

switch greenhouse-pump-0:
  name: Greenhouse Pump 0 Power
  platform: mqtt
  command_topic: "/greenhouse/pump/power/0"
  qos: 2
  payload_off: 0
  payload_on: 1
  state_topic: "/greenhouse/pump/power/0"

switch greenhouse-pump-1:
  name: Greenhouse Pump 1 Power
  platform: mqtt
  command_topic: "/greenhouse/pump/power/1"
  qos: 2
  payload_off: 0
  payload_on: 1
  state_topic: "/greenhouse/pump/power/1"

fan greenhouse-fan:
  name: Greenhouse Fan
  platform: mqtt
  command_topic: "/greenhouse/fan/power"
  state_topic: "/greenhouse/fan/power"
  qos: 0
  speed_state_topic: "/greenhouse/fan/power"
  speed_command_topic: "/greenhouse/fan/power"
  payload_on: 1023
  payload_off: 0
  payload_low_speed: 800
  payload_medium_speed: 900
  payload_high_speed: 1023
  speeds:
    - low
    - medium
    - high
```