# GPSLCD

Display GPS data on a LCD.

This program communicate with the u-blox NEO-6M GPS module through a software serial interface. It then displays certain GPS parameters on a I2C 16x2 (or 20x4) character LCD display module. The GPS messages are received by the on-board Arduino UART circuit using the SoftwareSerial library. It then uses the TinyGPS++ library to decode the GPS NMEA messages. The parameters are then displayed on a 16x4 (or 20x4) LCD using the LiquidCrystal_I2C library.

Note: Both the 16x2 and 20x4 LCDs are supported as these are the most popular.
