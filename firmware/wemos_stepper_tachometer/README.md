# Wemos D1 mini Arduino firmware

Open `wemos_stepper_tachometer.ino` in Arduino IDE.

Install:

- ESP8266 board support
- `PubSubClient`
- `AccelStepper`

Configure `WIFI_SSID`, `WIFI_PASSWORD`, and `MQTT_BROKER` before uploading.
Do not commit real credentials.

The firmware subscribes to `tachometer/angle` and publishes status on
`tachometer/status`. The Python application must use the same broker and topic.

Wiring for a Wemos D1 mini:

| Wemos | ULN2003 |
| --- | --- |
| D1 / GPIO5 | IN1 |
| D2 / GPIO4 | IN2 |
| D5 / GPIO14 | IN3 |
| D6 / GPIO12 | IN4 |

Power the 28BYJ-48 from an appropriate external 5 V supply and connect its
ground to Wemos ground. Set the mechanical needle to 90° before resetting the
board because this design has no homing sensor.
