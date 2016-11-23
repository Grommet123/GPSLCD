# GPSLCD

  This program communicate with the u-blox NEO-6M GPS module
  through a software serial interface. It then displays certain GPS
  parameters on a I2C 20x4 character LCD display module. The
  GPS messages are received by the on-board Arduino UART circuit using
  the SoftwareSerial library. It then uses the TinyGPS++ library to
  decode the GPS NMEA messages. The parameters are then displayed on a
  20x4 LCD using the LiquidCrystal_I2C library.