# Smart Gate Lock

## Introduction

This is a smart lock system that can be unlocked using either an RFID card or a smart home assistant such as Google Home. The system also includes a relay to control a gate or pillar light. The lock features an auto-lock function, automatically securing itself after 10 seconds thanks to its self-locking mechanism.

## Hardware

The hardware is based on an ESP8266 D1 Mini board and a servo motor. The servo motor is connected to the D1 Mini board through a 10kΩ resistor. The relay is connected to the D1 Mini board through a 220Ω resistor.

![hardware](hardware.png)

## Software

The software is written in Arduino and uses the following libraries:

-   Arduino ESP8266 WiFi
-   Arduino Servo
-   Arduino RFID
-   Arduino SinricPro

The software is based on the following example code:

-   [Arduino RFID](https://github.com/arduino-libraries/ArduinoRFID)
-   [Arduino Servo](https://github.com/arduino-libraries/Servo)
-   [Arduino SinricPro](https://github.com/sinricpro/arduino-sinricpro)

## Setup

1. Clone the repository to your computer.
2. Open the Arduino IDE and open the sketch.
3. Update the WiFi credentials in the `credentials.h` file.
4. Update the SinricPro credentials in the `credentials.h` file.
5. Update the authorized RFID card UIDs in the `credentials.h` file.
6. Upload the sketch to the ESP8266 D1 Mini board.
7. Open the SinricPro app and add the lock and switch devices.
8. Use the voice commands to control the lock and switch devices.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
