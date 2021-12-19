# arduino-weather-station-v2
An arduino based weather station.

* Both stations need to be in the same WiFi
* Indoor station runs an MQTT broker
* Outdoor station sends data to MQTT broker
* Indoor station subscribes to it's own topics and display the information
* Outdoor station uses deep sleep (3min) to conserve battery

## Indoor
* Wemos D1 mini
* SS1306 Display
* Two buttons
* USB powered
* Long pressing button 1 shows it's own temp/humidity values
* Button 2 is currently unused

## Outdoor
* Wemos D1 mini
* BME280
* 2x 4000mah Lithium Cells
* [Solarpanel](https://www.amazon.de/gp/product/B073XKPWY7)
* [TP4056 based 18650 charger](https://www.amazon.de/dp/B08VD83PR8)
* Wire from Bat+ with 100k Ohm resistor to A0 for battery voltage measurements
* Wire from D0 to RST for deep-sleep
