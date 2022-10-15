/* GPS LCD

  By GK Grotsky
  8/30/16

  This program communicate with the u-blox NEO-6M GPS module
  through a software serial interface. It then displays certain GPS
  parameters on a I2C 20x4 character LCD display module. The
  GPS messages are received by the on-board Arduino UART circuit using
  the SoftwareSerial library. It then uses the TinyGPSPlus library to
  decode the GPS NMEA messages. The parameters are then displayed on a
  20x4 LCD using the LiquidCrystal_I2C library.

  https://github.com/Grommet123/GPSLCD

  The MIT License (MIT)

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.

*/

#include "GPSLCD.h"
#include <TinyGPSPlus.h>
#include <SoftwareSerial.h>
#include <LiquidCrystal_I2C.h>

TinyGPSPlus gps; // This is the GPS object that will pretty much do all the grunt work with the NMEA data
SoftwareSerial GPSModule(RXPin, TXPin); // Defines the communications to the software serial tx and rx lines
LiquidCrystal_I2C lcd(I2C_ADDR, En_pin, Rw_pin, Rs_pin, D4_pin, D5_pin, D6_pin, D7_pin); // Defines the I2C address
// and pins of the LCD
GPSStruct GPSData; // Holds the local GPS data coming from the GPS module

// The setup (runs once at start up)
void setup()
{
#ifdef Debug
  Serial.begin(115200); // For debugging to the Serial Monitor (i.e. Serial.Println())
#endif
  pinMode(ENHANCE_SW, INPUT);
  pinMode(ENHANCE_2_SW, INPUT);
  pinMode(BACKLIGHT_SW, INPUT);
  // Selects either altitude or the date/time to be displayed
  pinMode(ALTITUDE_DATE_TIME_SW, INPUT);

  // Enable RGB LED
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(BLUE_LED_PIN, OUTPUT);

  pinMode(LED_BUILTIN, OUTPUT);   // Turn off on-board LED
  digitalWrite(LED_BUILTIN, LOW); //         "

  GPSModule.begin(9600); // This opens up communications to the software serial tx and rx lines (TXPin, RXPin)
  GPSModule.flush();

  lcd.begin(COLUMN, ROW); // Defines LCD type
  lcd.setBacklightPin(BACKLIGHT_PIN, POSITIVE); // Sets the pin to control the backlight
  lcd.clear(); // Clear the LCD
  lcd.home(); // Go to the home position on the LCD
} // setup

// The loop (runs forever and a day :-))
void loop()
{
  static uint32_t pastSatellites = 0;
  static uint8_t initializingCounter = 0;
  static uint8_t enhanceCounter2 = 0;
  static uint8_t altitudeDateTimeCounter = 0;
  static uint8_t initializingTimeoutCounter = 0;
  static unsigned long prevInitializationTime = INITIALIZATION_INTERVAL;
  static unsigned long prevCreditTime = TOGGLETIME_INTERVAL;
  static unsigned long prevInvalidSatellitesTime = TOGGLETIME_INTERVAL;
  static bool leftInitialization = false;
  static bool creditToggle = true;
  static bool invalidSatellitesToggle = true;
  static unsigned long prevDateTime = TOGGLETIME_INTERVAL;
  static unsigned long prevHeadingTime = TOGGLETIME_INTERVAL;
  static unsigned long prevHdopTime = TOGGLETIME_INTERVAL;
  static bool dateTimeToggle = true;
  static bool headingToggle = true;
  static bool hdopToggle = true;
  static bool OneTime = false;
  bool enhanceDisplay = digitalRead(ENHANCE_SW);
  bool enhanceDisplay2 = digitalRead(ENHANCE_2_SW);
  bool altitudeDateTime = digitalRead(ALTITUDE_DATE_TIME_SW);
  bool display12_24_Hour = !digitalRead(DISPLAY_12_HOUR_SW);
  bool localUTCTimeDate = !digitalRead(CONVERT_TO_LOCAL_SW);
  bool displayHdop = !digitalRead(DISPLAY_HDOP_SW);
  bool cardinal8_16 = !digitalRead(CARDINAL_SW);
  bool lowSpeedOverride = !digitalRead(LOW_SPEED_OVERRIDE);
  bool dataAvailable; // Data is available from the GPS module
  bool dataValid; // Data valid from the GPS module
  uint8_t SpeedCutout = SPEED_CUTOUT; // Value where the speed is set to 0
  uint8_t HeadingCutout = SPEED_CUTOUT; // Value where the heading is set to 0
  unsigned long now = millis(); // The time "now"

#ifdef DATA_VALID_OVERRIDE
  const bool dataValidOverride = true; // Set true to override data valid for debugging
#else
  const bool dataValidOverride = false; // Set false not to override data valid
#endif

#ifdef BACKLIGHT_OVERRIDE
  // Override LCD backlight switch
  // Force LCD backlight on
  lcd.setBacklight(BACKLIGHT_ON);
#else
  // Turn LCD backlight on/off
  lcd.setBacklight(digitalRead(BACKLIGHT_SW));
#endif

  // Enhance display mode
  if (enhanceDisplay) {
    pinMode(DISPLAY_12_HOUR_SW, INPUT_PULLUP);
    pinMode(CONVERT_TO_LOCAL_SW, INPUT_PULLUP);
    pinMode(DISPLAY_HDOP_SW, INPUT_PULLUP);
    pinMode(CARDINAL_SW, INPUT_PULLUP);
    pinMode(LOW_SPEED_OVERRIDE, INPUT_PULLUP);
    SpeedCutout = 0;
    HeadingCutout = 0;
  } else {
    pinMode(DISPLAY_12_HOUR_SW, INPUT);
    pinMode(CONVERT_TO_LOCAL_SW, INPUT);
    pinMode(DISPLAY_HDOP_SW, INPUT);
    pinMode(CARDINAL_SW, INPUT);
    pinMode(LOW_SPEED_OVERRIDE, INPUT);
    SpeedCutout = SPEED_CUTOUT;
    HeadingCutout = SPEED_CUTOUT;
  }
  // ************ GPS processing starts here ************
  dataAvailable = false;
  while (GPSModule.available()) // While there are characters coming from the GPS module (using the
    //                                                                                    SoftwareSerial library)
  {
    char c;
    bool b = gps.encode(c = GPSModule.read()); // This feeds the serial NMEA data into the GPS library one char at a time
#ifdef Debug
    Serial.print(c); // GPS data flow displaying the NMEA data
    delay (1); // 1 millisecond
#endif
    if (b) {
      dataAvailable = true; // Good read
    }
  }
#ifdef Serial_Debug
  Serial.flush();
  Serial.println();
#endif

  // Set (good read) valid flags for initialization
  if (dataAvailable) {
    GPSData.locationisValid    = gps.location.isValid();
    GPSData.speedisValid       = gps.speed.isValid();
    GPSData.altitudeisValid    = gps.altitude.isValid();
    GPSData.courseisValid      = gps.course.isValid();
    GPSData.dateisValid        = gps.date.isValid();
    GPSData.timeisValid        = gps.time.isValid();
    GPSData.satellitesisValid  = gps.satellites.isValid();
    GPSData.hdopisValid        = gps.hdop.isValid();
    GPSData.dataAvailable      = dataAvailable;
  }

#ifdef Serial_Debug // Print health data to the serial monitor
  Serial.print(GPSData.locationisValid);
  Serial.print(GPSData.speedisValid);
  Serial.print(GPSData.altitudeisValid);
  Serial.print(GPSData.courseisValid);
  Serial.print(GPSData.dateisValid);
  Serial.print(GPSData.timeisValid);
  Serial.print(GPSData.satellitesisValid);
  Serial.print(GPSData.hdopisValid);
  Serial.print("   ");
  Serial.print(GPSData.dataAvailable);
  Serial.print("   ");
  Serial.print(gps.satellites.value());
  Serial.print("   ");
  Serial.print(gps.hdop.value());
#endif

  // Get the GPS data valid flags
  dataValid =
    gps.location.isValid()   &&
    gps.speed.isValid()      &&
    gps.altitude.isValid()   &&
    gps.course.isValid()     &&
    gps.date.isValid()       &&
    gps.time.isValid()       &&
    gps.satellites.isValid() &&
    gps.hdop.isValid();

  // Check if the GPS data is valid or data valid override is set (for debugging)
  if (dataValid || dataValidOverride) {
#ifndef DATA_VALID_OVERRIDE
    // Store the real GPS data
    GPSData.satellites         = gps.satellites.value();
    GPSData.lat                = gps.location.lat();
    GPSData.lon                = gps.location.lng();
    GPSData.speed              = gps.speed.mph();
    GPSData.altitude           = gps.altitude.feet();
    GPSData.heading            = gps.course.deg();
    GPSData.hdop               = gps.hdop.value();
    GPSData.year               = gps.date.year();
    GPSData.month              = gps.date.month();
    GPSData.day                = gps.date.day();
    GPSData.hour               = gps.time.hour();
    GPSData.minute             = gps.time.minute();
    GPSData.second             = gps.time.second();
#else // #ifndef DATA_VALID_OVERRIDE
    // Store the fake GPS data (for debugging)
    GPSData.satellites = 5;
    GPSData.lat        = 40.71d;  // East coast (NYC)
    GPSData.lon        = -74.00d; //   "
    GPSData.speed      = 100.0d;
    GPSData.altitude   = 555.0d;
    GPSData.heading    = 60.0d; // ENE or NE (16 cardinal points or 8 cardinal points)
    GPSData.hdop       = 150; // 1.5
    GPSData.year       = 2017;
    GPSData.month      = 11; // Day before ST starts
    GPSData.day        = 5;  //      "
    GPSData.hour       = 23; // 7pm my local time
    GPSData.minute     = 10;
    GPSData.second     = 25;
#endif // #ifndef DATA_VALID_OVERRIDE
    // Turn on green LED (all is well)
    if ((digitalRead(BACKLIGHT_SW)) && (GPSData.satellites > 0)) {
      analogWrite(RED_LED_PIN, LED_OFF);
      analogWrite(GREEN_LED_PIN, 10);
      analogWrite(BLUE_LED_PIN, LED_OFF);
    } else if (GPSData.satellites > 0) {
      analogWrite(RED_LED_PIN, LED_OFF);
      analogWrite(GREEN_LED_PIN, LED_OFF);
      analogWrite(BLUE_LED_PIN, LED_OFF);
    }
#ifdef Serial_Debug // Print satellite health data to the serial monitor
    if (GPSData.satellites > 0) {
      Serial.println("   Green");
    }
#endif
    if ((enhanceDisplay2) && (!enhanceDisplay)) {
      OneTime = true;
      displayValidFlags(OneTime);
      lcd.setCursor(19, 3); // Display the counter
      if (++enhanceCounter2 > 9) enhanceCounter2 = 1; // Limit 1 - 9
      lcd.print(enhanceCounter2);
      delay(1000); // 1 seconds
      goto end; // I hate doing this, but somtimes a man has to do what a man has to do :-(
    } else if ((enhanceDisplay2) && (enhanceDisplay)) {
      uint16_t day  = GPSData.day;
      uint16_t month = GPSData.month;
      uint16_t year = GPSData.year;
      uint16_t hour = GPSData.hour;
      uint16_t riseI;
      uint16_t setI;
      uint16_t daylenI;
      double rise, set, daylen;

      // Convert UTC date to local date
      (void) convertToLocal(&hour, &year, &month,
                            &day, GPSData.lon, true); // true means date conversion

      // Get sunrise and sunset time in UTC. Don't need sun rises/sets this day (return value)
      (void) sun_rise_set(year, month, day,
                          GPSData.lon,
                          GPSData.lat,
                          &rise, &set);

      riseI = (uint16_t)round(rise);
      setI = (uint16_t)round(set);

      // Compute fraction.  UTC is the best I can do. "sun_rise_set" dose not return minutes and seconds.
      double riseF = rise - (double)riseI;
      double setF = set - (double)setI;
      String riseFS(abs(riseF));
      String setFS(abs(setF));
      riseFS = riseFS.substring(1, 6);
      setFS = setFS.substring(1, 6);

      // Convert UTC "sunrise" time to local "sunset" time
      bool DST = convertToLocal(&riseI, &year, &month,
                                &day, GPSData.lon, false); // false means no date and DST conversion
      // Convert UTC "sunset" time to local "sunset" time
      (void) convertToLocal(&setI, &year, &month,
                            &day, GPSData.lon, false, !DST); // false means no date and DST conversion, true means sunset
      Serial.println("");
      Serial.print("DST = ");
      Serial.println(DST);
      delay(1000);

      // Get day lenght hours in UTC.
      daylen = day_length(year, month, day, GPSData.lon, GPSData.lat);

      daylenI = (uint16_t)round(daylen);

      // Compute fraction.  UTC is the best I can do. "day_length" dose not return minutes and seconds.
      double daylenF = daylen - (double)daylenI;
      String daylenFS(abs(daylenF));
      daylenFS = daylenFS.substring(1, 6);

      lcd.clear(); // Clear the LCD
      lcd.setCursor(0, 0);
      if (month < 10) lcd.print("0");
      lcd.print(month);
      lcd.print("/");
      if (day < 10) lcd.print("0");
      lcd.print(day);
      lcd.print("/");
      lcd.print(year);
      displayDayOnLCD(dayOfWeek(year, month, day));
      lcd.setCursor(15, 0);
      lcd.print("SV:"); // Display the number of satellites
      lcd.print(gps.satellites.value());
      if (gps.satellites.value() >= 4) { // Need at least 4 satellites to navigate
        lcd.print("n"); // n = navigate
      } else {
        lcd.print("i"); // i = initializing
      }
      lcd.setCursor(0, 1);
      lcd.print("Sunrise: ");
      if (riseI > 12)  riseI -= 12;
      lcd.print(riseI);
      lcd.print(riseFS);
      lcd.print("am");
      lcd.setCursor(0, 2);
      lcd.print("Sunset:  ");
      if (setI > 12) setI -= 12;
      lcd.print(setI);
      lcd.print(setFS);
      lcd.print("pm");
      lcd.setCursor(0, 3);
      lcd.print("Day len: ");
      lcd.print(daylenI);
      lcd.print(daylenFS);
      lcd.print("hrs");
      lcd.setCursor(19, 3); // Display the counter
      if (++enhanceCounter2 > 9) enhanceCounter2 = 1; // Limit 1 - 9
      lcd.print(enhanceCounter2);
      delay(1500);
      goto end; // I hate doing this, but somtimes a man has to do what a man has to do :-(
    } // } else if ((enhanceDisplay2 && (enhanceDisplay)) {
    // Clear the screen once when leaving initialization
    if (!leftInitialization) {
      leftInitialization = true;
      lcd.clear(); // Clear the LCD
    }
    // Display the latest info from the gps object
    // which is derived from the data sent by the u-blox NEO-6M GPS module
    // Send data to the LCD
    if ((altitudeDateTime) && (GPSData.satellites == 0)) {
      OneTime = true;
      displayValidFlags(OneTime);
      lcd.setCursor(19, 3); // Display the hdop counter
      if (++altitudeDateTimeCounter > 9) altitudeDateTimeCounter = 1; // Limit 1 - 9
      lcd.print(altitudeDateTimeCounter);
      delay(1000); // 1 seconds
      goto end; // I hate doing this, but somtimes a man has to do what a man has to do :-(
    }
    lcd.home(); // Go to 1st line
    lcd.print("Lat: ");
    lcd.print(abs(GPSData.lat), 2);
    if (GPSData.lat >= 0) {
      lcd.print("N    ");
    }
    else {
      lcd.print("S  ");
    }
    lcd.setCursor(15, 0);
    lcd.print("SV:");
    if ((pastSatellites != GPSData.satellites) || (GPSData.satellites == 0)) {
      pastSatellites = GPSData.satellites;
      if (GPSData.satellites == 0) {
#ifdef Serial_Debug // Print satellite health data to the serial monitor
        Serial.println("   Red");
#endif
        // Toggle invalid satellites indicator every TOGGLETIME_INTERVAL seconds
        if (now - prevInvalidSatellitesTime > TOGGLETIME_INTERVAL / 4) {
          prevInvalidSatellitesTime = now;
          if (invalidSatellitesToggle) {
            invalidSatellitesToggle = false;
            lcd.print("XX");
            // Turn on red LED (all is not well)
            if (digitalRead(BACKLIGHT_SW)) {
              analogWrite(RED_LED_PIN, 10);
              analogWrite(GREEN_LED_PIN, LED_OFF);
              analogWrite(BLUE_LED_PIN, LED_OFF);
            } else {
              analogWrite(RED_LED_PIN, LED_OFF);
              analogWrite(GREEN_LED_PIN, LED_OFF);
              analogWrite(BLUE_LED_PIN, LED_OFF);
            }
          }
          else {
            invalidSatellitesToggle = true;
            lcd.print("  ");
            // Blink red LED
            analogWrite(RED_LED_PIN, LED_OFF);
            analogWrite(GREEN_LED_PIN, LED_OFF);
            analogWrite(BLUE_LED_PIN, LED_OFF);
          } // if (invalidSatellitesToggle)
        } // if (now - prevInvalidSatellitesTime > TOGGLETIME_INTERVAL)
      } // if (GPSData.satellites == 0)
      else if (GPSData.satellites <= 9) {
        lcd.print(" "); // Blank leading char
        lcd.print(GPSData.satellites);
      }
      else {
        lcd.print(GPSData.satellites);
      }
    } // if ((pastSatellites != GPSData.satellites) || (GPSData.satellites == 0))
    lcd.setCursor(0, 1); // Go to 2nd line
    lcd.print("Lon: ");
    lcd.print(abs(GPSData.lon), 2);
    if (GPSData.lon >= 0) {
      lcd.print("E");
    }
    else {
      lcd.print("W");
    }
    // Only display if time/date is selected
    if (altitudeDateTime) {
      if (!localUTCTimeDate) {
        // Display Hdop
        displayHdopOnLCD(GPSData.hdop, displayHdop, now,
                         &prevHdopTime, &hdopToggle);
      } // if (!localUTCTimeDate)
      else {
        if (localUTCTimeDate) {
          // Display Hdop
          displayHdopOnLCD(GPSData.hdop, displayHdop, now,
                           &prevHdopTime, &hdopToggle);
        }
      } // if (!localUTCTimeDate)
    } // if (altitudeDateTime)
    else {
      displayHdopOnLCD(GPSData.hdop, displayHdop, now,
                       &prevHdopTime, &hdopToggle);
    } // if (altitudeDateTime)
    lcd.setCursor(0, 2); // Go to 3rd line
    lcd.print("Spd: ");
    if ((GPSData.speed < SpeedCutout) && (lowSpeedOverride)) {
      lcd.print("0");
      lcd.print("m/h   "); // miles per hour
    }
    else {
      lcd.print((int16_t)GPSData.speed); // Miles
      if ((int16_t)GPSData.speed <= 99) {
        lcd.print("m/h   "); // Miles per hour
      } else {
        lcd.print("m/h ");
      }
      // Enhance display mode
      if (enhanceDisplay) {
        unsigned long KPH;
        KPH = (GPSData.speed * 1.61f); // Kilometers
        lcd.setCursor(5, 2);
        lcd.print((unsigned long)KPH);
        if ((int16_t)GPSData.speed <= 99) {
          lcd.print("k/h   "); // Kilometers per hour
        } else {
          lcd.print("k/h ");
        }
      } // if (enhanceDisplay) {
    } // if ((GPSData.speed < SPEED_CUTOUT) && (lowSpeedOverride))
    lcd.setCursor(12, 2);
    lcd.print("Hdg: ");
    if ((GPSData.speed < HeadingCutout) && (lowSpeedOverride)) {
      lcd.print("0  ");
    }
    else {
      // Toggle heading every TOGGLETIME_INTERVAL seconds
      if (now - prevHeadingTime > TOGGLETIME_INTERVAL) {
        prevHeadingTime = now;
        if (headingToggle) { // Display cardinal heading
          headingToggle = false;
          //          lcd.print(cardinal(GPSData.heading, cardinal8_16));
          lcd.print(cardinal(GPSData.heading, cardinal8_16));
        }
        else { // Display degrees heading
          headingToggle = true;
          lcd.print((int16_t)GPSData.heading);
          if (GPSData.heading <= 99.9) lcd.print(" "); // Clear the extra char
        } // if (headingToggle)
      } // if (now - prevHeadingTime > TOGGLETIME_INTERVAL)
    } // if ((GPSData.speed < SPEED_CUTOUT) && (lowSpeedOverride))
    lcd.setCursor(0, 3); // Go to 4th line
    if (altitudeDateTime) {
      lcd.print("Alt: ");
      if (enhanceDisplay) {
        lcd.print((int16_t)GPSData.altitude * 0.30f); // Convert to meters
        lcd.print ("mtrs");
        lcd.print("  "); // Clear the extra chars
      } else {
        lcd.print((int16_t)GPSData.altitude);
        lcd.print("ft");
        lcd.print("       "); // Clear the extra chars
      }
      // Toggle credit every TOGGLETIME_INTERVAL seconds
      if (now - prevCreditTime > (TOGGLETIME_INTERVAL / 2)) {
        prevCreditTime = now;
        if (creditToggle) { // Display credit
          creditToggle = false;
          lcd.print(CREDIT); // Yours truly :-)
        }
        else {
          creditToggle = true;
          lcd.print("            "); // Clear the extra chars
        }
      } // if (now - prevCreditTime > TOGGLETIME_INTERVAL)
    } // if (altitudeDateTime)
    else {
      // Toggle date/time every TOGGLETIME_INTERVAL seconds
      if (now - prevDateTime > TOGGLETIME_INTERVAL) {
        prevDateTime = now;
        if (dateTimeToggle) {
          // Display time
          dateTimeToggle = false;
          lcd.print("Time: ");
          uint16_t hour = GPSData.hour;
          uint16_t day  = GPSData.day;
          uint16_t month = GPSData.month;
          uint16_t year = GPSData.year;
          bool DST = false;
          if (!enhanceDisplay) {
            // Convert UTC time to local time (no need to convert the date)
            DST = convertToLocal(&hour, &year, &month,
                                 &day, GPSData.lon, false); // false means no date conversion
          }
          else { // if (localUTCTimeDate)
            if (display12_24_Hour) {
              if (hour == 0) { // 12 hour clocks don't display 0
                hour = 12;
              }
            } // if (display12_24_Hour)
          } // if (localUTCTimeDate)
          char AMPM[] = "am";
          if (display12_24_Hour) {
            if (hour >= 12) { // Convert to 12 hour format
              if (hour > 12) hour -= 12;
              strcpy(AMPM, "pm");
            }
          } // if (display12_24_Hour)
          if (hour < 10) lcd.print("0");
          lcd.print(hour);
          lcd.print(":");
          if (GPSData.minute < 10) lcd.print("0");
          lcd.print(GPSData.minute);
          lcd.print(":");
          if (GPSData.second < 10) lcd.print("0");
          lcd.print(GPSData.second);
          if (display12_24_Hour) {
            lcd.print(AMPM);
          }
          if (!enhanceDisplay) {
            if (display12_24_Hour) {
              (DST) ? lcd.print(" DST") : lcd.print("  ST");
            }
            else {
              (DST) ? lcd.print("   DST") : lcd.print("    ST");
            } // if (display12_24_Hour)
          } // if (!enhanceDisplay) {
          if (localUTCTimeDate) {
            if (display12_24_Hour) {
              lcd.print(" UTC");
            }
            else {
              lcd.print("   UTC");
            } // if (display12_24_Hour)
          } // if (!localUTCTimeDate)
        } // if (dateTimeToggle)
        else {
          // Display date
          dateTimeToggle = true;
          lcd.print("Date: ");
          uint16_t hour = GPSData.hour;
          uint16_t day  = GPSData.day;
          uint16_t month = GPSData.month;
          uint16_t year = GPSData.year;
          if (!enhanceDisplay) {
            // Convert UTC date to local date
            bool DST = convertToLocal(&hour, &year, &month,
                                      &day, GPSData.lon, true); // true means date conversion
          } // if (!enhanceDisplay)
          if (month < 10) lcd.print("0");
          lcd.print(month);
          lcd.print("/");
          if (day < 10) lcd.print("0");
          lcd.print(day);
          lcd.print("/");
          lcd.print(year);
          displayDayOnLCD(dayOfWeek(year, month, day));
        } // if (dateTimeToggle)
      } // if (now - prevDateTime > TOGGLETIME_INTERVAL)
    } // if (altitudeDateTime)
  } // if (dataValid || dataValidOverride)
  else {
    // GPS data is not valid, so it must be initializing
    // Turn on blue LED (Initializing)
    if (digitalRead(BACKLIGHT_SW)) {
      analogWrite(RED_LED_PIN, LED_OFF);
      analogWrite(GREEN_LED_PIN, LED_OFF);
      analogWrite(BLUE_LED_PIN, 50);
    } else {
      analogWrite(RED_LED_PIN, LED_OFF);
      analogWrite(GREEN_LED_PIN, LED_OFF);
      analogWrite(BLUE_LED_PIN, LED_OFF);
    }
#ifdef Serial_Debug // Print satellite health data to the serial monitor
    Serial.println("   Blue");
#endif
    // Display initialization screen once every INITIALIZATION_INTERVAL
    if (now - prevInitializationTime > INITIALIZATION_INTERVAL) {
      prevInitializationTime = now;
      if (altitudeDateTime) { // Display the valid flags
        OneTime = true;
        displayValidFlags(OneTime);
      } else {
        if (enhanceDisplay) {
          displayVersionInfo();
        } else {
          lcd.clear(); // Clear the LCD
          lcd.setCursor(8, 0); // Go to 1st line
          lcd.print("GPS");
          lcd.setCursor(15, 0);
          lcd.print("SV:"); // Display the number of satellites
          lcd.print(gps.satellites.value());
          if (gps.satellites.value() >= 4) { // Need at least 4 satellites to navigate
            lcd.print("n"); // n = navigate
          } else {
            lcd.print("i"); // i = initializing
          }
          lcd.setCursor(4, 1); // Go to 2nd line
          lcd.print("Initializing");
          lcd.setCursor(6, 2); // Go to 3rd line
          lcd.print("Data Not");
          lcd.setCursor(7, 3); // Go to 4th line
          lcd.print("Valid");
        } // if (enhanceDisplay)
      } // if (altitudeDateTime)
      lcd.setCursor(19, 3); // Display the initializing counter
      if (++initializingCounter > 9) initializingCounter = 1; // Limit 1 - 9
      lcd.print(initializingCounter);
      leftInitialization = false;
    } // if (now - prevInitializationTime > INITIALIZATION_INTERVAL)
  } // GPS data is not valid
end: (void)0; // goto end;
} // loop

// Helper functions start here

/* Convert UTC time, date, sunrise, and sunset to local time, date, sunrise, and sunset
   Difference between UTC time/date (at Greenwich) and local time/date is 15 minutes
   per 1 degree of longitude. See the following:
   http://www.edaboard.com/thread101516.html
*/
bool convertToLocal(uint16_t* hour, uint16_t* year, uint16_t* month,
                    uint16_t* day, const double lon, bool convertDate = true, bool sunset = false) {

  uint8_t DaysAMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

  // Get Day of Week (DOW)
  uint8_t DOW = dayOfWeek(*year, *month, *day);
  // Get Daylight Saving Time (DST) or Standard Time (ST)
  bool DST = IsDST(*day, *month, DOW);
  // Compute local time (hours)
  int8_t UTCOffset = (int8_t)round((lon / 15.0d)); // UTC offset
  if (UTCOffset < 0) {
    // West of Greenwich, subtract
    UTCOffset = abs(UTCOffset); // Make offset positive
    if (DST && !sunset) UTCOffset -= 1; // Compensate for DST
    if (*hour <= UTCOffset) *hour += 24;
    *hour -= UTCOffset; // Subtract offset
  }
  else {
    // East of Greenwich, add
    if (DST) - UTCOffset; // Compensate for DST
    if (*hour <= UTCOffset) *hour += 24;
    *hour += UTCOffset; // Add offset
  }
  // Convert date if convertDate flag is set
  // Portions of the following code (with some modifications) ripped off from Instructables
  // http://www.instructables.com/id/GPS-time-UTC-to-local-time-conversion-using-Arduin/?ALLSTEPS
  if (convertDate) {
    if ((24 - *hour) <= UTCOffset) { // A new UTC day started
      if (*year % 4 == 0) DaysAMonth[1] = 29; //leap year check (the simple method)
      if (*hour < 24) {
        *day -= 1;
        if (*day < 1) {
          if (*month == 1) {
            *month = 12;
            *year -= 1;
          } // if (*month == 1)
          else {
            *month -= 1;
          }
          *day = DaysAMonth[*month - 1];
        } // if (*day < 1)
      } // if (*hour < 24)
      else if (*hour >= 24) {
        *day += 1;
        if (*day > DaysAMonth[*month - 1]) {
          *day = 1;
          *month += 1;
          if (*month > 12) *year += 1;
        } // if (*day > DaysAMonth[*month - 1])
      } // if (*hour >= 24)
    } // if ((24 - *hour) <= UTCOffset)
  } // if (convertDate)
  return (DST);
}

// Display the Version info on the LCD
void displayVersionInfo(void) {
  lcd.clear(); // Clear the LCD
  lcd.setCursor(6, 0); // Center text (Row, column)
  lcd.print("Gary K.");
  lcd.setCursor(15, 0);
  lcd.print("SV:"); // Display the number of satellites
  lcd.print(gps.satellites.value());
  if (gps.satellites.value() >= 4) { // Need at least 4 satellites to navigate
    lcd.print("n"); // n = navigate
  } else {
    lcd.print("i"); // i = initializing
  }
  lcd.setCursor(6, 1); // Center text
  lcd.print("Grotsky");
  lcd.setCursor(5, 2); // Center text
  lcd.print("Version ");
  lcd.print(VERSION);
  lcd.setCursor(6, 3); // Center text
  lcd.print("IR ");
  lcd.print(DATE);
}
// Display the valid flags on the LCD
void displayValidFlags(bool OneTime) {
  if (OneTime) {
    lcd.clear(); // Clear the LCD
    OneTime = false;
  }
  lcd.setCursor(2, 0); // Go to 1st line
  lcd.print("Valid Flags");
  lcd.setCursor(15, 0);
  lcd.print("SV:");
  lcd.print(gps.satellites.value());
  if (gps.satellites.value() >= 4) { // Need at least 4 satellites to navigate
    lcd.print("n"); // n = navigate
  } else {
    lcd.print("i"); // i = initializing
  }
  if (GPSData.dataAvailable) {
    lcd.setCursor(0, 2); // Go to 3rd line
    lcd.print(GPSData.locationisValid);
    lcd.print(GPSData.speedisValid);
    lcd.print(GPSData.altitudeisValid);
    lcd.print(GPSData.courseisValid);
    lcd.print(GPSData.dateisValid);
    lcd.print(GPSData.timeisValid);
    lcd.print(GPSData.satellitesisValid);
    lcd.print(GPSData.hdopisValid);
    lcd.print("   ");
    lcd.print(GPSData.dataAvailable);
  } else {
    lcd.setCursor(11, 2); // Go to 3rd line
    lcd.print(GPSData.dataAvailable);
  }
  lcd.setCursor(0, 3); // Go to 4th line
  lcd.print("LSACDTSH   DA"); // Data valid flags
}

/*  The following three functions ripped off from Electrical Engineering Stack Exchange
  http://electronics.stackexchange.com/questions/66285/how-to-calculate-day-of-the-week-for-rtc

   Returns the number of days to the start of the specified year, taking leap
   years into account, but not the shift from the Julian calendar to the
   Gregorian calendar. Instead, it is as though the Gregorian calendar is
   extrapolated back in time to a hypothetical "year zero".
*/
uint16_t leap(uint16_t year)
{
  return year * 365 + (year / 4) - (year / 100) + (year / 400);
}
/* Returns a number representing the number of days since March 1 in the
   hypothetical year 0, not counting the change from the Julian calendar
   to the Gregorian calendar that occurred in the 16th century. This
   algorithm is loosely based on a function known as "Zeller's Congruence".
   This number MOD 7 gives the day of week, where 0 = Monday and 6 = Sunday.
*/
uint16_t zeller(uint16_t year, uint8_t month, uint8_t day)
{
  year += ((month + 9) / 12) - 1;
  month = (month + 9) % 12;
  return leap(year) + month * 30 + ((6 * month + 5) / 10) + day + 1;
}

// Returns the day of week for a given date.
uint8_t dayOfWeek(uint16_t year, uint8_t month, uint8_t day)
{
  return (zeller(year, month, day) % 7) + 1;
}

// Display the day of the week on the LCD
void displayDayOnLCD(uint8_t day)
{
  if (day == 7) day = 0;
  switch (day)
  {
    case 0:  lcd.print(" Mon"); break;
    case 1:  lcd.print(" Tue"); break;
    case 2:  lcd.print(" Wed"); break;
    case 3:  lcd.print(" Thu"); break;
    case 4:  lcd.print(" Fri"); break;
    case 5:  lcd.print(" Sat"); break;
    case 6:  lcd.print(" Sun"); break;
    default: lcd.print(" Err");
  }
}

// Compute the cardinal points of the compass (16 or 8 cardinal points)
const char* cardinal(double course, bool cardinalSelect)
{
  if (!cardinalSelect) {
    const char* directions[] = {"N  ", "NNE", "NE ", "ENE", "E  ", "ESE", "SE ", "SSE",
                                "S  ", "SSW", "SW ", "WSW", "W  ", "WNW", "NW ", "NNW"
                               };
    uint8_t direction = (uint8_t)((course + 11.25d) / 22.5d);
    return directions[direction % 16];
  }
  else {
    const char* directions[] = {"N  ", "N  ", "NE ", "NE ", "E  ", "E  ", "SE ", "SE ",
                                "S  ", "S  ", "SW ", "SW ", "W  ", "W  ", "NW ", "NW "
                               };
    uint8_t direction = (uint8_t)((course + 11.25d) / 22.5d);
    return directions[direction % 16];
  } // if (!cardinalSelect)
}

/* Ripped off from Stackoverflow
  http://stackoverflow.com/questions/5590429/calculating-daylight-saving-time-from-only-date

  Check to see if it's Daylight Savings Time (DST)
*/
bool IsDST(uint8_t day, uint8_t month , uint8_t DOW)
{
  // If and when permanent DST takes affect
#ifdef PERMANENT_DST
  return true;
#else
  // Make Day of Week (DOW) match with what Stackoverflow suggests
  // for DOW (Sunday = 0 to Saturday = 6)
  switch (DOW)
  {
    case 6:  DOW = 0; break; // Sun
    case 7:  DOW = 1; break; // Mon
    case 1:  DOW = 2; break; // Tue
    case 2:  DOW = 3; break; // Wed
    case 3:  DOW = 4; break; // Thu
    case 4:  DOW = 5; break; // Fri
    case 5:  DOW = 6; break; // Sat
    default: break;
  }
  // January, February, and December are out
  if (month < 3 || month > 11) {
    return false;
  }
  // April to October are in
  if (month > 3 && month < 11) {
    return true;
  }
  int8_t previousSunday = (int8_t)(day - DOW);
  // In march, we are DST if our previous Sunday was on or after the 8th
  if (month == 3) {
    return previousSunday >= 8;
  }
  // In November we must be before the first Sunday to be DST
  // That means the previous Sunday must be before the 1st
  return previousSunday <= 0;
#endif // #ifdef PERMANENT_DTS
}

// Dislpays Hdop or horizontal position error on the LCD
void displayHdopOnLCD(uint32_t Hdop, bool HdopSelect, unsigned long now,
                      unsigned long * prevHdopTime, bool * hdopToggle) {
  if (!HdopSelect) {
    // Display Horizontal Dilution of Precision (Hdop) with the format X.X
    // It comes in from the GPS module as XXX
    // If you care how Hdop is computed (I don't :-)), see the following:
    // http://www2.unb.ca/gge/Resources/gpsworld.may99.pdf
    float hdop = (float)Hdop / 100.0f; // Hdop in X.X format
    lcd.print(" Hdop:");
    if (hdop < 10.0f) {
      lcd.print((uint32_t)hdop);
      lcd.print(".");
      lcd.print((uint32_t)((hdop - (uint32_t)hdop) * 10.0f));
    }
    else {
      // Hdop value too large for my LCD, so cross it out
      // Toggle Hdop indicator every TOGGLETIME_INTERVAL seconds
      if (now - *prevHdopTime > TOGGLETIME_INTERVAL / 3) {
        *prevHdopTime = now;
        if (*hdopToggle) {
          *hdopToggle = false;
          lcd.print("XXX");
        }
        else {
          *hdopToggle = true;
          lcd.print("   ");
        } // if (hdopToggle)
      } // if (now - prevHdopTime > TOGGLETIME_INTERVAL)
    } // if (hdop < 10.0f)
  }
  else { // if (!HdopSelect)
    // Display horizontal position error in feet
    float hError = (float)Hdop / 100.0f; // Error in X.X format
    hError *= GPS_RECEIVER_ERROR; // Error in meters
    hError *= _GPS_FEET_PER_METER; // Error in feet
    lcd.print(" Err: ");
    if (hError < 100.0f) {
      lcd.print((uint8_t)hError);
      if (hError < 10.0f) {
        lcd.print("f "); // feet
      }
      else {
        lcd.print("f"); // feet
      }
    }
    else { // if (hError < 100.0f)
      // horizontal position error value too large for my LCD, so cross it out
      // Toggle Err indicator every TOGGLETIME_INTERVAL seconds
      if (now - *prevHdopTime > TOGGLETIME_INTERVAL / 3) {
        *prevHdopTime = now;
        if (*hdopToggle) {
          *hdopToggle = false;
          lcd.print("XXX");
        }
        else {
          *hdopToggle = true;
          lcd.print("   ");
        } // if (hdopToggle)
      } // if (now - prevHdopTime > TOGGLETIME_INTERVAL)
    } // if (hError < 100.0f)
  } // if (!HdopSelect)
}
/* Convert UTC sand sunset unrise time and date to local time and date
   Difference between UTC time/date (at Greenwich) and local time/date is 15 minutes
   per 1 degree of longitude. See the following:
   http://www.edaboard.com/thread101516.html
*/
bool convertSToLocal(int* hour, int* year, int* month,
                     int* day, double lon, bool convertDate) {

  uint8_t DaysAMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

  // Get Day of Week (DOW)
  uint8_t DOW = dayOfWeek(*year, *month, *day);
  // Get Daylight Saving Time (DST) or Standard Time (ST)
  bool DST = IsDST(*day, *month, DOW);
  // Compute local time (hours)
  int8_t UTCOffset = (int8_t)round((lon / 15.0d)); // UTC offset
  if (UTCOffset < 0) {
    // West of Greenwich, subtract
    UTCOffset = abs(UTCOffset); // Make offset positive
    if (DST && convertDate) --UTCOffset; // Compensate for DST
    if (*hour <= UTCOffset) *hour += 24;
    *hour -= UTCOffset; // Subtract offset
  }
  else {
    // East of Greenwich, add
    if (DST && convertDate) --UTCOffset; // Compensate for DST
    if (*hour <= UTCOffset) *hour += 24;
    *hour += UTCOffset; // Add offset
  }
  // Convert date if convertDate flag is set
  // Portions of the following code (with some modifications) ripped off from Instructables
  // http://www.instructables.com/id/GPS-time-UTC-to-local-time-conversion-using-Arduin/?ALLSTEPS
  if (convertDate) {
    if ((24 - *hour) <= UTCOffset) { // A new UTC day started
      if (*year % 4 == 0) DaysAMonth[1] = 29; //leap year check (the simple method)
      if (*hour < 24) {
        *day -= 1;
        if (*day < 1) {
          if (*month == 1) {
            *month = 12;
            *year -= 1;
          } // if (*month == 1)
          else {
            *month -= 1;
          }
          *day = DaysAMonth[*month - 1];
        } // if (*day < 1)
      } // if (*hour < 24)
      else if (*hour >= 24) {
        *day += 1;
        if (*day > DaysAMonth[*month - 1]) {
          *day = 1;
          *month += 1;
          if (*month > 12) *year += 1;
        } // if (*day > DaysAMonth[*month - 1])
      } // if (*hour >= 24)
    } // if ((24 - *hour) <= UTCOffset)
  } // if (convertDate)
  return (DST);
}

/*
  Computes Sun rise/set times, start/end of twilight, and
  the length of the day at any date and latitude
  Written as DAYLEN.C, 1989-08-16
  Modified to SUNRISET.C, 1992-12-01
  (c) Paul Schlyter, 1989, 1992
  Released to the public domain by Paul Schlyter, December 1992
  http://stjarnhimlen.se/comp/sunriset.c
*/

/* The "workhorse" function for sun rise/set times */
int __sunriset__( int year, int month, int day, double lon, double lat,
                  double altit, int upper_limb, double * trise, double * tset )
/***************************************************************************/
/* Note: year,month,date = calendar date, 1801-2099 only.             */
/*       Eastern longitude positive, Western longitude negative       */
/*       Northern latitude positive, Southern latitude negative       */
/*       The longitude value IS critical in this function!            */
/*       altit = the altitude which the Sun should cross              */
/*               Set to -35/60 degrees for rise/set, -6 degrees       */
/*               for civil, -12 degrees for nautical and -18          */
/*               degrees for astronomical twilight.                   */
/*         upper_limb: non-zero -> upper limb, zero -> center         */
/*               Set to non-zero (e.g. 1) when computing rise/set     */
/*               times, and to zero when computing start/end of       */
/*               twilight.                                            */
/*        *rise = where to store the rise time                        */
/*        *set  = where to store the set  time                        */
/*                Both times are relative to the specified altitude,  */
/*                and thus this function can be used to compute       */
/*                various twilight times, as well as rise/set times   */
/* Return value:  0 = sun rises/sets this day, times stored at        */
/*                    *trise and *tset.                               */
/*               +1 = sun above the specified "horizon" 24 hours.     */
/*                    *trise set to time when the sun is at south,    */
/*                    minus 12 hours while *tset is set to the south  */
/*                    time plus 12 hours. "Day" length = 24 hours     */
/*               -1 = sun is below the specified "horizon" 24 hours   */
/*                    "Day" length = 0 hours, *trise and *tset are    */
/*                    both set to the time when the sun is at south.  */
/*                                                                    */
/**********************************************************************/
{
  double  d,  /* Days since 2000 Jan 0.0 (negative before) */
          sr,         /* Solar distance, astronomical units */
          sRA,        /* Sun's Right Ascension */
          sdec,       /* Sun's declination */
          sradius,    /* Sun's apparent radius */
          t,          /* Diurnal arc */
          tsouth,     /* Time when Sun is at south */
          sidtime;    /* Local sidereal time */

  int rc = 0; /* Return cde from function - usually 0 */

  /* Compute d of 12h local mean solar time */
  d = days_since_2000_Jan_0(year, month, day) + 0.5 - lon / 360.0;

  /* Compute the local sidereal time of this moment */
  sidtime = revolution( GMST0(d) + 180.0 + lon );

  /* Compute Sun's RA, Decl and distance at this moment */
  sun_RA_dec( d, &sRA, &sdec, &sr );

  /* Compute time when Sun is at south - in hours UT */
  tsouth = 12.0 - rev180(sidtime - sRA) / 15.0;

  /* Compute the Sun's apparent radius in degrees */
  sradius = 0.2666 / sr;

  /* Do correction to upper limb, if necessary */
  if ( upper_limb )
    altit -= sradius;

  /* Compute the diurnal arc that the Sun traverses to reach */
  /* the specified altitude altit: */
  {
    double cost;
    cost = ( sind(altit) - sind(lat) * sind(sdec) ) /
           ( cosd(lat) * cosd(sdec) );
    if ( cost >= 1.0 )
      rc = -1, t = 0.0;       /* Sun always below altit */
    else if ( cost <= -1.0 )
      rc = +1, t = 12.0;      /* Sun always above altit */
    else
      t = acosd(cost) / 15.0; /* The diurnal arc, hours */
  }

  /* Store rise and set times - in hours UT */
  *trise = tsouth - t;
  *tset  = tsouth + t;

  return rc;
}  /* __sunriset__ */

/* The "workhorse" function */
double __daylen__( int year, int month, int day, double lon, double lat,
                   double altit, int upper_limb )
/**********************************************************************/
/* Note: year,month,date = calendar date, 1801-2099 only.             */
/*       Eastern longitude positive, Western longitude negative       */
/*       Northern latitude positive, Southern latitude negative       */
/*       The longitude value is not critical. Set it to the correct   */
/*       longitude if you're picky, otherwise set to to, say, 0.0     */
/*       The latitude however IS critical - be sure to get it correct */
/*       altit = the altitude which the Sun should cross              */
/*               Set to -35/60 degrees for rise/set, -6 degrees       */
/*               for civil, -12 degrees for nautical and -18          */
/*               degrees for astronomical twilight.                   */
/*         upper_limb: non-zero -> upper limb, zero -> center         */
/*               Set to non-zero (e.g. 1) when computing day length   */
/*               and to zero when computing day+twilight length.      */
/**********************************************************************/
{
  double  d,  /* Days since 2000 Jan 0.0 (negative before) */
          obl_ecl,    /* Obliquity (inclination) of Earth's axis */
          sr,         /* Solar distance, astronomical units */
          slon,       /* True solar longitude */
          sin_sdecl,  /* Sine of Sun's declination */
          cos_sdecl,  /* Cosine of Sun's declination */
          sradius,    /* Sun's apparent radius */
          t;          /* Diurnal arc */

  /* Compute d of 12h local mean solar time */
  d = days_since_2000_Jan_0(year, month, day) + 0.5 - lon / 360.0;

  /* Compute obliquity of ecliptic (inclination of Earth's axis) */
  obl_ecl = 23.4393 - 3.563E-7 * d;

  /* Compute Sun's ecliptic longitude and distance */
  sunpos( d, &slon, &sr );

  /* Compute sine and cosine of Sun's declination */
  sin_sdecl = sind(obl_ecl) * sind(slon);
  cos_sdecl = sqrt( 1.0 - sin_sdecl * sin_sdecl );

  /* Compute the Sun's apparent radius, degrees */
  sradius = 0.2666 / sr;

  /* Do correction to upper limb, if necessary */
  if ( upper_limb )
    altit -= sradius;

  /* Compute the diurnal arc that the Sun traverses to reach */
  /* the specified altitude altit: */
  {
    double cost;
    cost = ( sind(altit) - sind(lat) * sin_sdecl ) /
           ( cosd(lat) * cos_sdecl );
    if ( cost >= 1.0 )
      t = 0.0;                      /* Sun always below altit */
    else if ( cost <= -1.0 )
      t = 24.0;                     /* Sun always above altit */
    else  t = (2.0 / 15.0) * acosd(cost); /* The diurnal arc, hours */
  }
  return t;
}  /* __daylen__ */

/* This function computes the Sun's position at any instant */
void sunpos( double d, double * lon, double * r )
/******************************************************/
/* Computes the Sun's ecliptic longitude and distance */
/* at an instant given in d, number of days since     */
/* 2000 Jan 0.0.  The Sun's ecliptic latitude is not  */
/* computed, since it's always very near 0.           */
/******************************************************/
{
  double M,         /* Mean anomaly of the Sun */
         w,         /* Mean longitude of perihelion */
         /* Note: Sun's mean longitude = M + w */
         e,         /* Eccentricity of Earth's orbit */
         E,         /* Eccentric anomaly */
         x, y,      /* x, y coordinates in orbit */
         v;         /* True anomaly */

  /* Compute mean elements */
  M = revolution( 356.0470 + 0.9856002585 * d );
  w = 282.9404 + 4.70935E-5 * d;
  e = 0.016709 - 1.151E-9 * d;

  /* Compute true longitude and radius vector */
  E = M + e * RADEG * sind(M) * ( 1.0 + e * cosd(M) );
  x = cosd(E) - e;
  y = sqrt( 1.0 - e * e ) * sind(E);
  *r = sqrt( x * x + y * y );          /* Solar distance */
  v = atan2d( y, x );                  /* True anomaly */
  *lon = v + w;                        /* True solar longitude */
  if ( *lon >= 360.0 )
    *lon -= 360.0;                   /* Make it 0..360 degrees */
}

void sun_RA_dec( double d, double * RA, double * dec, double * r )
/******************************************************/
/* Computes the Sun's equatorial coordinates RA, Decl */
/* and also its distance, at an instant given in d,   */
/* the number of days since 2000 Jan 0.0.             */
/******************************************************/
{
  double lon, obl_ecl, x, y, z;

  /* Compute Sun's ecliptical coordinates */
  sunpos( d, &lon, r );

  /* Compute ecliptic rectangular coordinates (z=0) */
  x = *r * cosd(lon);
  y = *r * sind(lon);

  /* Compute obliquity of ecliptic (inclination of Earth's axis) */
  obl_ecl = 23.4393 - 3.563E-7 * d;

  /* Convert to equatorial rectangular coordinates - x is unchanged */
  z = y * sind(obl_ecl);
  y = y * cosd(obl_ecl);

  /* Convert to spherical coordinates */
  *RA = atan2d( y, x );
  *dec = atan2d( z, sqrt(x * x + y * y) );

}  /* sun_RA_dec */

/******************************************************************/
/* This function reduces any angle to within the first revolution */
/* by subtracting or adding even multiples of 360.0 until the     */
/* result is >= 0.0 and < 360.0                                   */
/******************************************************************/
#define INV360    ( 1.0 / 360.0 )

double revolution( double x )
/*****************************************/
/* Reduce angle to within 0..360 degrees */
/*****************************************/
{
  return ( x - 360.0 * floor( x * INV360 ) );
}  /* revolution */

double rev180( double x )
/*********************************************/
/* Reduce angle to within +180..+180 degrees */
/*********************************************/
{
  return ( x - 360.0 * floor( x * INV360 + 0.5 ) );
}  /* revolution */


/*******************************************************************/
/* This function computes GMST0, the Greenwich Mean Sidereal Time  */
/* at 0h UT (i.e. the sidereal time at the Greenwhich meridian at  */
/* 0h UT).  GMST is then the sidereal time at Greenwich at any     */
/* time of the day.  I've generalized GMST0 as well, and define it */
/* as:  GMST0 = GMST - UT  --  this allows GMST0 to be computed at */
/* other times than 0h UT as well.  While this sounds somewhat     */
/* contradictory, it is very practical:  instead of computing      */
/* GMST like:                                                      */
/*                                                                 */
/*  GMST = (GMST0) + UT * (366.2422/365.2422)                      */
/*                                                                 */
/* where (GMST0) is the GMST last time UT was 0 hours, one simply  */
/* computes:                                                       */
/*                                                                 */
/*  GMST = GMST0 + UT                                              */
/*                                                                 */
/* where GMST0 is the GMST "at 0h UT" but at the current moment!   */
/* Defined in this way, GMST0 will increase with about 4 min a     */
/* day.  It also happens that GMST0 (in degrees, 1 hr = 15 degr)   */
/* is equal to the Sun's mean longitude plus/minus 180 degrees!    */
/* (if we neglect aberration, which amounts to 20 seconds of arc   */
/* or 1.33 seconds of time)                                        */
/*                                                                 */
/*******************************************************************/

double GMST0( double d )
{
  double sidtim0;
  /* Sidtime at 0h UT = L (Sun's mean longitude) + 180.0 degr  */
  /* L = M + w, as defined in sunpos().  Since I'm too lazy to */
  /* add these numbers, I'll let the C compiler do it for me.  */
  /* Any decent C compiler will add the constants at compile   */
  /* time, imposing no runtime or code overhead.               */
  sidtim0 = revolution( ( 180.0 + 356.0470 + 282.9404 ) +
                        ( 0.9856002585 + 4.70935E-5 ) * d );
  return sidtim0;
}  /* GMST0 */
