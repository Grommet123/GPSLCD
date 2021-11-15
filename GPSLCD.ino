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
SoftwareSerial GPSModule(RXPin, TXPin);
// Initializes class variables and defines the I2C address of the LCD
LiquidCrystal_I2C lcd(I2C_ADDR, En_pin, Rw_pin, Rs_pin, D4_pin, D5_pin, D6_pin, D7_pin);

// The setup (runs once at start up)
void setup()
{
  Serial.begin(115200); // For debugging to the Serial Monitor (i.e. Serial.Println())
  pinMode(BACKLIGHT_SW, INPUT);
  // Selects either altitude or the date/time to be displayed
  pinMode(ALTITUDE_DATE_TIME_SW, INPUT);
  // Select switches
  pinMode(DISPLAY_12_HOUR_SW, INPUT);
  pinMode(CONVERT_TO_LOCAL_SW, INPUT);
  pinMode(DISPLAY_HDOP_SW, INPUT);
  pinMode(CARDINAL_SW, INPUT);
  pinMode(LOW_SPEED_OVERRIDE, INPUT);
  pinMode(ON_BOARD_LED, OUTPUT); // Turn off on-board LED
  digitalWrite(ON_BOARD_LED, LOW); //       "

  GPSModule.begin(9600); // This opens up communications to the software serial tx and rx lines
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
  bool display12_24_Hour = !digitalRead(DISPLAY_12_HOUR_SW);
  bool localUTCTimeDate = !digitalRead(CONVERT_TO_LOCAL_SW);
  bool displayHdop = !digitalRead(DISPLAY_HDOP_SW);
  bool cardinal8_16 = !digitalRead(CARDINAL_SW);
#ifdef DATA_VALID_OVERRIDE
  bool lowSpeedOverride = HIGH;
#else
  bool lowSpeedOverride = !digitalRead(LOW_SPEED_OVERRIDE);
#endif
  bool dataValid; // Data valid from the GPS module
  GPSStruct GPSData; // Holds the GPS data coming from the GPS module
  unsigned long now = millis(); // The time "now"

#ifdef DATA_VALID_OVERRIDE
  const bool dataValidOverride = true; // Set true to override data valid for debugging
#else
  const bool dataValidOverride = false; // Set false not to override data valid
#endif

#ifdef BACKLIGHT_OVERRIDE
  // Override LCD backlight switch
  // Force LCD backlight on/off
  lcd.setBacklight(BACKLIGHT_ONOFF);
#else
  // Turn LCD backlight on/off
  lcd.setBacklight(digitalRead(BACKLIGHT_SW));
#endif

  // ************ GPS processing starts here ************

  while (GPSModule.available()) // While there are characters coming from the GPS module (using the SoftwareSerial library)
  {
    bool b = gps.encode(GPSModule.read()); // This feeds the serial NMEA data into the GPS library one char at a time
  }
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
    GPSData.satellites = gps.satellites.value();
    GPSData.lat        = gps.location.lat();
    GPSData.lon        = gps.location.lng();
    GPSData.speed      = gps.speed.mph();
    GPSData.altitude   = gps.altitude.feet();
    GPSData.heading    = gps.course.deg();
    GPSData.hdop       = gps.hdop.value();
    GPSData.year       = gps.date.year();
    GPSData.month      = gps.date.month();
    GPSData.day        = gps.date.day();
    GPSData.hour       = gps.time.hour();
    GPSData.minute     = gps.time.minute();
    GPSData.second     = gps.time.second();
#else // #ifndef DATA_VALID_OVERRIDE
    // Store the fake GPS data (for debugging)
    GPSData.satellites = 5;
    GPSData.lat        = 40.71d;  // East coast (NYC)
    GPSData.lon        = -74.00d; //   "
    GPSData.speed      = 5.0d;
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

    // Clear the screen once when leaving initialization
    if (!leftInitialization) {
      leftInitialization = true;
      lcd.clear(); // Clear the LCD
    }

    // Display the latest info from the gps object
    // which is derived from the data sent by the GPS unit
    // Send data to the LCD
    lcd.home(); // Go to 1st line
    lcd.print("Lat: ");
    lcd.print(GPSData.lat);
    lcd.setCursor(15, 0);
    lcd.print("SV:");
    if ((pastSatellites != GPSData.satellites) || (GPSData.satellites == 0)) {
      pastSatellites = GPSData.satellites;
      if (GPSData.satellites == 0) {
        // Toggle invalid satellites indicator every TOGGLETIME_INTERVAL seconds
        if (now - prevInvalidSatellitesTime > TOGGLETIME_INTERVAL / 4) {
          prevInvalidSatellitesTime = now;
          if (invalidSatellitesToggle) {
            invalidSatellitesToggle = false;
            lcd.print("XX");
          }
          else {
            invalidSatellitesToggle = true;
            lcd.print("  ");
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
    lcd.print(GPSData.lon);
    // Only display if time/date is selected
    if ((digitalRead(ALTITUDE_DATE_TIME_SW))) {
      if (!localUTCTimeDate) {
        lcd.print("      "); // Clear the extra chars
        uint8_t DOW = dayOfWeek(GPSData.year, GPSData.month, GPSData.day);
        if (IsDST(GPSData.day, GPSData.month, DOW)) {
          lcd.print("DST");
        }
        else {
          lcd.print(" ST");
        }
      }
      else { // if (!localUTCTimeDate)
        // Display Hdop
        displayHdopOnLCD(GPSData.hdop, displayHdop, now,
                         &prevHdopTime, &hdopToggle);
      } // if (!localUTCTimeDate)
    } // if ((digitalRead(ALTITUDE_DATE_TIME_SW)))
    else {
      // Display Hdop
      displayHdopOnLCD(GPSData.hdop, displayHdop, now,
                       &prevHdopTime, &hdopToggle);
    } // if ((digitalRead(ALTITUDE_DATE_TIME_SW)))
    lcd.setCursor(0, 2); // Go to 3rd line
    lcd.print("Spd: ");
    if ((GPSData.speed < SPEED_CUTOUT) && (lowSpeedOverride)) {
      lcd.print("0  ");
    }
    else {
      lcd.print((int16_t)GPSData.speed);
      lcd.print(" "); // Clear the extra chars
    } // if ((GPSData.speed < SPEED_CUTOUT) && (lowSpeedOverride))
    lcd.setCursor(12, 2);
    lcd.print("Hdg: ");
    if ((GPSData.speed < SPEED_CUTOUT) && (lowSpeedOverride)) {
      lcd.print("0  ");
    }
    else {
      // Toggle heading every TOGGLETIME_INTERVAL seconds
      if (now - prevHeadingTime > TOGGLETIME_INTERVAL) {
        prevHeadingTime = now;
        if (headingToggle) { // Display cardinal heading
          headingToggle = false;
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
    if ((digitalRead(ALTITUDE_DATE_TIME_SW))) {
      lcd.print("Alt: ");
      lcd.print((int16_t)GPSData.altitude);
      lcd.print("ft");
      // Toggle credit every TOGGLETIME_INTERVAL seconds
      if (now - prevCreditTime > (TOGGLETIME_INTERVAL / 2)) {
        prevCreditTime = now;
        if (creditToggle) { // Display credit
          creditToggle = false;
          lcd.print("       "); // Clear the extra chars
          lcd.print(CREDIT); // Yours truly :-)
        }
        else {
          creditToggle = true;
          lcd.print("            "); // Clear the extra chars
        }
      } // if (now - prevCreditTime > TOGGLETIME_INTERVAL)
    } // if ((digitalRead(ALTITUDE_DATE_TIME_SW)))
    else {
      // Toggle date/time every TOGGLETIME_INTERVAL seconds
      if (now - prevDateTime > TOGGLETIME_INTERVAL) {
        prevDateTime = now;
        if (dateTimeToggle) {
          // Display time
          dateTimeToggle = false;
          lcd.print("Time: ");
          uint8_t hour = GPSData.hour;
          uint8_t day  = GPSData.day;
          uint8_t month = GPSData.month;
          uint16_t year = GPSData.year;
          bool DST = false;
          if (localUTCTimeDate) {
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
          if (localUTCTimeDate) {
            if (display12_24_Hour) {
              (DST) ? lcd.print(" DST") : lcd.print("  ST");
            }
            else {
              (DST) ? lcd.print("   DST") : lcd.print("    ST");
            } // if (display12_24_Hour)
          } // if (localUTCTimeDate)
          if (!localUTCTimeDate) {
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
          uint8_t hour = GPSData.hour;
          uint8_t day  = GPSData.day;
          uint8_t month = GPSData.month;
          uint16_t year = GPSData.year;
          if (localUTCTimeDate) {
            // Convert UTC date to local date
            bool DST = convertToLocal(&hour, &year, &month,
                                      &day, GPSData.lon, true); // true means date conversion
          } // if (localUTCTimeDate)
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
    } // if ((digitalRead(ALTITUDE_DATE_TIME_SW)))
  } // if (dataValid)
  else {
    // GPS data is not valid, so it must be initializing
    // Display initialization screen once every INITIALIZATION_INTERVAL
    if (now - prevInitializationTime > INITIALIZATION_INTERVAL) {
      prevInitializationTime = now;
      if ((digitalRead(ALTITUDE_DATE_TIME_SW))) { // Display the valid flags
        lcd.clear(); // Clear the LCD
        lcd.home(); // Go to the home position on the LCD
        lcd.print("    Valid Flags");
        lcd.setCursor(0, 2); // Go to 3rd line
        lcd.print(gps.location.isValid());
        lcd.print(gps.speed.isValid());
        lcd.print(gps.altitude.isValid());
        lcd.print(gps.course.isValid());
        lcd.print(gps.date.isValid());
        lcd.print(gps.time.isValid());
        lcd.print(gps.satellites.isValid());
        lcd.print(gps.hdop.isValid());
        lcd.setCursor(0, 3); // Go to 4th line
        lcd.print("LSACDTSH"); // Data valid flags
      } else {
        lcd.clear(); // Clear the LCD
        lcd.setCursor(0, 0); // Go to 1st line
        lcd.setCursor(8, 0);
        lcd.print("GPS");
        lcd.setCursor(16, 0);
        lcd.print("v");
        lcd.print(VERSION);
        lcd.setCursor(4, 1); // Go to 2nd line
        lcd.print("Initializing");
        lcd.setCursor(6, 2); // Go to 3rd line
        lcd.print("Data Not");
        lcd.setCursor(7, 3); // Go to 4th line
        lcd.print("Valid");
        lcd.setCursor(0, 3);
        lcd.print("SV:"); // Display the number of satellites
        if (gps.satellites.value() > 2) { // Need at least 3 satellites to navigate
          lcd.print(gps.satellites.value());
          lcd.print("n"); // n = navigate
          delay(1000); // 1 second
        } else {
          lcd.print(gps.satellites.value());
          lcd.print("i"); // i = initializing
        }
      } // if ((digitalRead(ALTITUDE_DATE_TIME_SW)))
      lcd.setCursor(19, 3); // Display the initializing counter
      if (++initializingCounter > 9) initializingCounter = 1; // Limit 1 - 9
      lcd.print(initializingCounter);
      leftInitialization = false;
    } // if (now - prevInitializationTime > INITIALIZATION_INTERVAL)
  } // GPS data is not valid
} // loop

/* Helper functions start here

  The following three functions ripped off from Electrical Engineering Stack Exchange
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
}

/* Convert UTC time and date to local time and date
   Difference between UTC time/date (at Greenwich) and local time/date is 15 minutes
   per 1 degree of longitude. See the following:
   http://www.edaboard.com/thread101516.html
*/
bool convertToLocal(uint8_t* hour, uint16_t* year, uint8_t* month,
                    uint8_t* day, const double lon, bool convertDate, bool doDST) {

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
    if (DST && convertDate && doDST) --UTCOffset; // Compensate for DST
    if (*hour <= UTCOffset) *hour += 24;
    if (DST) {
      *hour -= (UTCOffset - 1); // Subtract offset
    } else {
      *hour -= (UTCOffset); // Subtract offset
    }
  }
  else {
    // East of Greenwich, add
    if (DST && convertDate && doDST) --UTCOffset; // Compensate for DST
    if (*hour <= UTCOffset) *hour += 24;
    if (DST) {
      *hour += (UTCOffset + 1); // add offset
    } else {
      *hour += (UTCOffset); // add offset
    }
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
