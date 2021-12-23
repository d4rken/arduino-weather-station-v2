# arduino-weather-station-v2
An arduino based weather station.


<img src="https://github.com/d4rken/arduino-weather-station-v2/blob/main/Images/stations-frontal.jpg" height="200"><img src="https://github.com/d4rken/arduino-weather-station-v2/raw/main/Images/eink-indoor-station.jpg" height="200">

* Both stations need to be in the same WiFi
* Indoor station runs an MQTT broker
* Outdoor station sends data to MQTT broker
* Indoor station subscribes to it's own topics and display the information
* Outdoor station uses deep sleep (3min) to conserve battery
* 🍰 approximate uptime is stored in RTC memory during deep sleep

## Indoor
* Wemos D1 mini
* Waveshare 1.54" e-Paper Display
* USB powered

## Outdoor
* Wemos D1 mini
* BME280
* 2x 4000mah Lithium Cells
* [Solarpanel](https://www.amazon.de/gp/product/B073XKPWY7)
* [TP4056 based 18650 charger](https://www.amazon.de/dp/B08VD83PR8)
* Wire from Bat+ with 100k Ohm resistor to A0 for battery voltage measurements
* Wire from D0 to RST for deep-sleep

## More Images
Wiring could be cleaner, but lets first see if it survives the winter ¯\\\_(ツ)_/¯

<img src="https://github.com/d4rken/arduino-weather-station-v2/blob/main/Images/cube-display.jpg" width="200"><img src="https://github.com/d4rken/arduino-weather-station-v2/blob/main/Images/stations-frontal.jpg" width="200"><img src="https://github.com/d4rken/arduino-weather-station-v2/blob/main/Images/stations-side.jpg" width="200"><img src="https://github.com/d4rken/arduino-weather-station-v2/blob/main/Images/station-guts.jpg" width="200">
