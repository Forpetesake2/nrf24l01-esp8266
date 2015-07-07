# nrf24l01-esp8266

This snippet uses the ESP8266 built-in AT commands to send received sensor data from the NRF24L01, trim the received response, and then send it to a server of your choice through a HTTP POST request. The script lends heavily from xesscorp's script to send commands to your ESP8266 module. An Arduino Pro Mini was used but should work on Uno/Mega as well.
