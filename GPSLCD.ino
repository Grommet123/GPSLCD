/* GPS LCD

    By GK Grotsky
    8/30/16

  This program communicate with the u-blox NEO-6M GPS module
  through a software serial interface. It then displays certain GPS
  parameters on a I2C 16x2 (or 20x4) character LCD display module. The
  GPS messages are received by the on-board Arduino UART circuit using
  the SoftwareSerial library. It then uses the TinyGPS++ library to
  decode the GPS NMEA messages. The parameters are then displayed on a
  16x4 (or 20x4) LCD using the LiquidCrystal_I2C library.

  Note: Both the 16x2 and 20x4 LCDs are supported as these are the most popular
        LCDs available.
*/
#include "GPSLCD.h"
#include <TinyGPS++.h>
#include <SoftwareSerial.h>
#include <LiquidCrystal_I2C.h>

TinyGPSPlus gps; // This is the GPS object that will pretty much do all the grunt work with the NMEA data
SoftwareSerial GPSModule(10, 11); // RX, TX
LiquidCrystal_I2C lcd(I2C_ADDR, En_pin, Rw_pin, Rs_pin, D4_pin, D5_pin, D6_pin, D7_pin); // Initializes class variables and defines the I2C address of the LCD
bool dataValid; // Data is valid from the GPS module
GPSStruct GPSData; // Holds the GPS data coming from the GPS module

// The setup (runs once at start up)
void setup()
{
  pinMode(BACKLIGHT_SW, INPUT);

#ifdef _16x2
  // Selects either speed or heading to be displayed
  pinMode(SPEED_ALTITUDE_SW, INPUT);
#else
  // Selects either altitude or the date/time to be displayed
  pinMode(ALTITUDE_DATE_TIME_SW, INPUT);
#endif

  GPSModule.begin(9600); // This opens up communications to the software serial tx and rx lines
  GPSModule.flush();

  lcd.begin (COLUMN, ROW); // Defines LCD type
  lcd.setBacklightPin(BACKLIGHT_PIN, POSITIVE); // Sets the pin to control the backlight
  lcd.clear(); // Clear the LCD
  lcd.home (); // Go to the home position on the LCD
} // setup

// The loop (runs forever and a day :-))
void loop()
{
  static uint32_t pastSatellites = 0;
  static uint8_t initializingCounter = 0;
  static unsigned long prevInitializationTime = INITIALIZATION_INTERVAL;
  static unsigned long prevCreditTime = TOGGLETIME_INTERVAL;
  static unsigned long prevInvalidSatellitesTime = TOGGLETIME_INTERVAL;
  static bool leftInitialization = false;
  static bool creditToggle = true;
  static bool InvalidSatellitesToggle = true;
#ifndef _16x2
  static unsigned long prevDateTime = TOGGLETIME_INTERVAL;
  static unsigned long prevHeadingTime = TOGGLETIME_INTERVAL;
  static bool dateTimeToggle = true;
  static bool headingToggle = true;
#endif
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
  unsigned long now = millis();

  while (GPSModule.available()) // While there are characters to come from the GPS
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
#ifndef _16x2
    GPSData.heading    = gps.course.deg();
    GPSData.hdop       = gps.hdop.value();
    GPSData.year       = gps.date.year();
    GPSData.month      = gps.date.month();
    GPSData.day        = gps.date.day();
    GPSData.hour       = gps.time.hour();
    GPSData.minute     = gps.time.minute();
    GPSData.second     = gps.time.second();
#endif // #ifndef _16x2
#else // #ifndef DATA_VALID_OVERRIDE
    // Store the fake GPS data (for debugging)
    GPSData.satellites = 0; // Invalid satellites
    GPSData.lat        = 39.16d; // East coast
    GPSData.lon        = -77.07d; //   "
    GPSData.speed      = 55.0d;
    GPSData.altitude   = 555.0d;
#ifndef _16x2
    GPSData.heading    = 60.0d; // ENE or NE (16 cardinal points or 8 cardinal points)
    GPSData.hdop       = 240;
    GPSData.year       = 2016;
    GPSData.month      = 11; // Day before ST starts
    GPSData.day        = 5;  //      "
    GPSData.hour       = 23; // 7pm my local time
    GPSData.minute     = 10;
    GPSData.second     = 25;
#endif // #ifndef _16x2
#endif // #ifndef DATA_VALID_OVERRIDE
    // Clear the screen once when leaving initialization
    if (!leftInitialization) {
      leftInitialization = true;
      lcd.clear(); // Clear the LCD
    }
    // Display the latest info from the gps object which it derived from the data sent by the GPS unit
    // Send data to the LCD
#ifdef _16x2 //------------------------------------------------------
    lcd.home (); // Go to 1st line
    lcd.print("Lat: ");
    lcd.print(GPSData.lat);
    lcd.setCursor (11, 0);
    lcd.print("SV:");
    if ((pastSatellites != GPSData.satellites) || (GPSData.satellites == 0)) {
      pastSatellites = GPSData.satellites;
      if (GPSData.satellites == 0) {
        // Toggle invalid satellites indicator every TOGGLETIME_INTERVAL seconds
        if (now - prevInvalidSatellitesTime > TOGGLETIME_INTERVAL / 4) {
          prevInvalidSatellitesTime = now;
          if (InvalidSatellitesToggle) {
            InvalidSatellitesToggle = false;
            lcd.print("XX");
          }
          else {
            InvalidSatellitesToggle = true;
            lcd.print("  ");
          } // if (InvalidSatellitesToggle)
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
    lcd.setCursor (0, 1); // Go to 2nd line
    lcd.print("Lon: ");
    lcd.print(GPSData.lon);
    if ((digitalRead(SPEED_ALTITUDE_SW))) {
      lcd.setCursor (12, 1);
      if (GPSData.speed < SPEED_CUTOUT) {
        lcd.print("0  ");
      }
      else {
        lcd.print((int16_t)GPSData.speed);
        lcd.print(" "); // Clear the extra char
      }
    }
    else {
      lcd.print("     "); // Clear the extra chars
      if (GPSData.altitude <= 99.9) {
        lcd.setCursor (14, 1);
      }
      else {
        lcd.setCursor (13, 1);
      }
      lcd.print((int16_t)GPSData.altitude); // Altitude
      // Toggle credit every TOGGLETIME_INTERVAL seconds
      if (now - prevCreditTime > (TOGGLETIME_INTERVAL / 2)) {
        prevCreditTime = now;
        if (creditToggle) { // Display credit
          creditToggle = false;
          lcd.print("     "); // Clear the extra chars
          lcd.print(CREDIT); // Yours truly :-)
        }
        else {
          creditToggle = true;
          lcd.print("        "); // Clear the extra chars
        }
      } // if (now - prevCreditTime > TOGGLETIME_INTERVAL)
    } // if ((digitalRead(SPEED_ALTITUDE_SW)))
#else // 20x4 ----------------------------------------------------
    lcd.home (); // Go to 1st line
    lcd.print("Lat: ");
    lcd.print(GPSData.lat);
    lcd.setCursor (15, 0);
    lcd.print("SV:");
    if ((pastSatellites != GPSData.satellites) || (GPSData.satellites == 0)) {
      pastSatellites = GPSData.satellites;
      if (GPSData.satellites == 0) {
        // Toggle invalid satellites indicator every TOGGLETIME_INTERVAL seconds
        if (now - prevInvalidSatellitesTime > TOGGLETIME_INTERVAL / 3) {
          prevInvalidSatellitesTime = now;
          if (InvalidSatellitesToggle) {
            InvalidSatellitesToggle = false;
            lcd.print("XX");
          }
          else {
            InvalidSatellitesToggle = true;
            lcd.print("  ");
          } // if (InvalidSatellitesToggle)
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
    lcd.setCursor (0, 1); // Go to 2nd line
    lcd.print("Lon: ");
    lcd.print(GPSData.lon);
#ifndef CONVERT_TO_LOCAL
    // Only display if time/date is selected
    if (!(digitalRead(ALTITUDE_DATE_TIME_SW))) {
      lcd.print("      "); // Clear the extra chars
      uint8_t DOW = dayOfWeek(GPSData.year, GPSData.month, GPSData.day);
      if (IsDST(GPSData.day, GPSData.month, DOW)) {
        lcd.print("DST");
      }
      else {
        lcd.print(" ST");
      }
    }
    else {
      lcd.print("         "); // Clear the extra chars
    }
#else // #ifndef CONVERT_TO_LOCAL
    //Display Horizontal Dilution of Precision (Hdop) with the format X.X
    // It comes in from the GPS module as XXX
    uint32_t hdopInteger = GPSData.hdop / 100;
    uint32_t hdopFarction = (GPSData.hdop - (hdopInteger * 100)) / 10;
    lcd.print(" Hdop:");
    if (hdopInteger < 10) {
      lcd.print(hdopInteger); // Integer part
      lcd.print(".");
      lcd.print(hdopFarction); // Fraction part
    }
    else {
      lcd.print("XXX");
    }
#endif // #ifndef CONVERT_TO_LOCAL
    lcd.setCursor (0, 2); // Go to 3rd line
    lcd.print("Spd: ");
    if (GPSData.speed < SPEED_CUTOUT) {
      lcd.print("0  ");
    }
    else {
      lcd.print((int16_t)GPSData.speed);
      lcd.print(" "); // Clear the extra chars
    }
    lcd.setCursor (12, 2);
    lcd.print("Hdg: ");
    if (GPSData.speed < SPEED_CUTOUT) {
      lcd.print("0  ");
    }
    else {
      // Toggle heading every TOGGLETIME_INTERVAL seconds
      if (now - prevHeadingTime > TOGGLETIME_INTERVAL) {
        prevHeadingTime = now;
        if (headingToggle) { // Display cardinal heading
          headingToggle = false;
          lcd.print(cardinal(GPSData.heading));
        }
        else { // Display degrees heading
          headingToggle = true;
          lcd.print((int16_t)GPSData.heading);
          if (GPSData.heading <= 99.9) lcd.print(" "); // Clear the extra char
        } // if (headingToggle)
      } // if (now - prevHeadingTime > TOGGLETIME_INTERVAL)
    } // if (GPSData.speed < SPEED_CUTOUT)
    lcd.setCursor (0, 3); // Go to 4th line
    if ((digitalRead(ALTITUDE_DATE_TIME_SW))) {
      lcd.print("Alt: ");
      lcd.print((int16_t)GPSData.altitude);
      // Toggle credit every TOGGLETIME_INTERVAL seconds
      if (now - prevCreditTime > (TOGGLETIME_INTERVAL / 2)) {
        prevCreditTime = now;
        if (creditToggle) { // Display credit
          creditToggle = false;
          lcd.print("         "); // Clear the extra chars
          lcd.print(CREDIT); // Yours truly :-)
        }
        else {
          creditToggle = true;
          lcd.print("            "); // Clear the extra chars
        }
      } // if (now - prevCreditTime > TOGGLETIME_INTERVAL)
      prevDateTime = TOGGLETIME_INTERVAL;
      dateTimeToggle = true; // Default first to display date
    } // if ((digitalRead(ALTITUDE_DATE_TIME_SW)))
    else {
      // Toggle date/time every TOGGLETIME_INTERVAL seconds
      if (now - prevDateTime > TOGGLETIME_INTERVAL) {
        prevDateTime = now;
        if (dateTimeToggle) {
          // Display date
          dateTimeToggle = false;
          lcd.print("Date: ");
          uint8_t hour = GPSData.hour;
          uint8_t day  = GPSData.day;
          uint8_t month = GPSData.month;
          uint16_t year = GPSData.year;
#ifdef CONVERT_TO_LOCAL
          // Convert UTC date to local date
          bool DST = convertToLocal (&hour, &year, &month,
                                     &day, GPSData.lon, true); // true means date conversion
#endif // #ifdef CONVERT_TO_LOCAL
          if (month < 10) lcd.print("0");
          lcd.print(month);
          lcd.print("/");
          if (day < 10) lcd.print("0");
          lcd.print(day);
          lcd.print("/");
          lcd.print(year);
          printDay (dayOfWeek (year, month, day));
        } // if (dateTimeToggle)
        else {
          // Display time
          dateTimeToggle = true;
          lcd.print("Time: ");
          uint8_t hour = GPSData.hour;
          uint8_t day  = GPSData.day;
          uint8_t month = GPSData.month;
          uint16_t year = GPSData.year;
#ifdef CONVERT_TO_LOCAL
          // Convert UTC time to local time (no need to convert the date)
          bool DST = convertToLocal (&hour, &year, &month,
                                     &day, GPSData.lon, false); // false means no date conversion
#else // #ifdef CONVERT_TO_LOCAL
#ifdef DISPLAY_12_HOUR
          if (hour == 0) { // 12 hour clocks don't display 0
            hour = 12;
          }
#endif // #ifdef DISPLAY_12_HOUR
#endif // #ifdef CONVERT_TO_LOCAL
#ifdef DISPLAY_12_HOUR
          char AMPM[] = "am";
          if (hour >= 12) { // Convert to 12 hour format
            if (hour > 12) hour -= 12;
            strcpy (AMPM, "pm");
          }
#endif
          if (hour < 10) lcd.print("0");
          lcd.print(hour);
          lcd.print(":");
          if (GPSData.minute < 10) lcd.print("0");
          lcd.print(GPSData.minute);
          lcd.print(":");
          if (GPSData.second < 10) lcd.print("0");
          lcd.print(GPSData.second);
#ifdef DISPLAY_12_HOUR
          lcd.print(AMPM);
#endif // #ifdef DISPLAY_12_HOUR
#ifdef CONVERT_TO_LOCAL
#ifdef DISPLAY_12_HOUR
          (DST) ? lcd.print(" DST") : lcd.print("  ST");
#else // #ifdef DISPLAY_12_HOUR
          (DST) ? lcd.print("   DST") : lcd.print("    ST");
#endif // #ifdef DISPLAY_12_HOUR
#endif // #ifndef CONVERT_TO_LOCAL
#ifndef CONVERT_TO_LOCAL
#ifdef DISPLAY_12_HOUR
          lcd.print(" UTC");
#else // #ifdef DISPLAY_12_HOUR
          lcd.print("   UTC");
#endif // #ifdef DISPLAY_12_HOUR
#endif // #ifndef CONVERT_TO_LOCAL
        } // if (dateTimeToggle)
      } // if (now - prevDateTime > TOGGLETIME_INTERVAL)
    } // if ((digitalRead(ALTITUDE_DATE_TIME_SW)))
#endif //------------------------------------------------------------
  } // if (dataValid)
  else {
    // GPS data is not valid, so it must be initializing
#ifdef _16x2 //------------------------------------------------------
    // Display initialization screen once every INITIALIZATION_INTERVAL
    if (now - prevInitializationTime > INITIALIZATION_INTERVAL) {
      prevInitializationTime = now;
      lcd.setCursor (6, 0); // Go to 1st line
      lcd.print("GPS");
      lcd.setCursor (11, 0);
      lcd.print("v");
      lcd.print(VERSION);
      lcd.setCursor (2, 1); // Go to 2nd line
      lcd.print("Initializing");
      lcd.setCursor (15, 1);
      if (++initializingCounter > 9) initializingCounter = 1; // Limit 1 - 9
      lcd.print(initializingCounter);
      leftInitialization = false;
    }
#else // 20x4 -------------------------------------------------------
    // Display initialization screen once every INITIALIZATION_INTERVAL
    if (now - prevInitializationTime > INITIALIZATION_INTERVAL) {
      prevInitializationTime = now;
      lcd.setCursor (8, 0); // Go to 1st line
      lcd.print("GPS");
      lcd.setCursor (14, 0);
      lcd.print("v");
      lcd.print(VERSION);
      lcd.setCursor (4, 1); // Go to 2nd line
      lcd.print("Initializing");
      lcd.setCursor (6, 2); // Go to 3rd line
      lcd.print("Data Not");
      lcd.setCursor (7, 3); // Go to 4th line
      lcd.print("Valid");
      lcd.setCursor (19, 3);
      if (++initializingCounter > 9) initializingCounter = 1; // Limit 1 - 9
      lcd.print(initializingCounter);
      leftInitialization = false;
    }
#endif //------------------------------------------------------------
  } // GPS data is not valid
} // loop

#ifndef _16x2
/* Helper functions start here

  Ripped off from Electrical Engineering Stack Exchange
  http://electronics.stackexchange.com/questions/66285/how-to-calculate-day-of-the-week-for-rtc
  ------------------------------------------------------------
   Returns the number of days to the start of the specified year, taking leap
   years into account, but not the shift from the Julian calendar to the
   Gregorian calendar. Instead, it is as though the Gregorian calendar is
   extrapolated back in time to a hypothetical "year zero".
*/
uint16_t leap (uint16_t year)
{
  return year * 365 + (year / 4) - (year / 100) + (year / 400);
}
/* Returns a number representing the number of days since March 1 in the
   hypothetical year 0, not counting the change from the Julian calendar
   to the Gregorian calendar that occurred in the 16th century. This
   algorithm is loosely based on a function known as "Zeller's Congruence".
   This number MOD 7 gives the day of week, where 0 = Monday and 6 = Sunday.
*/
uint16_t zeller (uint16_t year, uint8_t month, uint8_t day)
{
  year += ((month + 9) / 12) - 1;
  month = (month + 9) % 12;
  return leap (year) + month * 30 + ((6 * month + 5) / 10) + day + 1;
}

// Returns the day of week for a given date.
uint8_t dayOfWeek (uint16_t year, uint8_t month, uint8_t day)
{
  return (zeller (year, month, day) % 7) + 1;
}
//----------------------------------------------------------

// Print the day of the week on the LCD
void printDay(uint8_t day)
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

// Compute the cardinal points of the compass
const char* cardinal(double course)
{
#ifdef _16CARDINAL
  const char* directions[] = {"N  ", "NNE", "NE ", "ENE", "E  ", "ESE", "SE ", "SSE",
                              "S  ", "SSW", "SW ", "WSW", "W  ", "WNW", "NW ", "NNW"
                             };
#else
  const char* directions[] = {"N  ", "N  ", "NE ", "NE ", "E  ", "E  ", "SE ", "SE ",
                              "S  ", "S  ", "SW ", "SW ", "W  ", "W  ", "NW ", "NW "
                             };
#endif // #ifdef _16CARDINAL
  uint8_t direction = (uint8_t)((course + 11.25d) / 22.5d);
  return directions[direction % 16];
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

#ifdef CONVERT_TO_LOCAL
/* Convert UTC time and date to local time and date
   Difference between UTC time/date (at Greenwich) and local time/date is 15 minutes
   per 1 degree of longitude. See the following:
   http://www.edaboard.com/thread101516.html
*/
bool convertToLocal (uint8_t* hour, uint16_t* year, uint8_t* month,
                     uint8_t* day, const double lon, bool convertDate) {

  uint8_t DaysAMonth [] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

  // Get Day of Week (DOW)
  uint8_t DOW = dayOfWeek(*year, *month, *day);
  // Get Daylight Saving Time (DST) or Standard Time (ST)
  bool DST = IsDST(*day, *month, DOW);
  // Compute local time (hours)
  int8_t UTCOffset = (int8_t)(lon / 15.0d); // UTC offset
  if (UTCOffset < 0) {
    // West of Greenwich, subtract
    UTCOffset = abs(UTCOffset); // Make offset positive
    if (DST) --UTCOffset; // Compensate for DST
    if (*hour <= UTCOffset) *hour += 24;
    *hour = *hour - UTCOffset; // Subtract offset
  }
  else {
    // East of Greenwich, add
    if (DST) ++UTCOffset; // Compensate for DST
    if (*hour <= UTCOffset) *hour += 24;
    *hour = *hour + UTCOffset; // Add offset
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
        day += 1;
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
#endif // #ifdef CONVERT_TO_LOCAL
#endif // #ifndef _16x2
