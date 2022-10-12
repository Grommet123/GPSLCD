/* GPS LCD

   By GK Grotsky
   8/30/16

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

  https://github.com/Grommet123/GPSLCD

*/

#ifndef GPSLCD_h
#define GPSLCD_h

#define VERSION  "19.5"              // Version number
#define DATE "8/30/16"               // Initial release date
#define Debug                        // Uncomment out for debug
#define Serial_Debug                 // Uncomment out for serial monitor display debug
//#define DATA_VALID_OVERRIDE        // Override the data valid flag (comment out to turn off)
// It also feeds fake GPS data to the system for debugging
#define Rs_pin  0                    // Pin hook up for the LCD via the I2C interface
#define Rw_pin  1                    //                   "
#define En_pin  2                    //                   "
#define BACKLIGHT_PIN 3              //                   "
#define D4_pin  4                    //                   "
#define D5_pin  5                    //                   "
#define D6_pin  6                    //                   "
#define D7_pin  7                    //                   "
#define INITIALIZATION_INTERVAL 1000 // Initialization display interval
#define TOGGLETIME_INTERVAL 6000     // Toggle display interval
#define CREDIT "GKG"                 // Yours truly :-)
//#define BACKLIGHT_OVERRIDE         // Override the LCD backlight switch (comment out to disable)
#ifdef  BACKLIGHT_OVERRIDE
#define BACKLIGHT_ON HIGH            // Works with BACKLIGHT_OVERRIDE to force on the LCD backlight on
#endif

#define COLUMN 20                    // Number of Columns (On-board LCD)
#define ROW 4                        // Number of rows (On-board LCD)
#define I2C_ADDR 0x27                // Address of my 20x4 On-board LCD (yours might be different)
#define ENHANCE_SW 2                 // Enables the enhance display switches
#define DISPLAY_12_HOUR_SW 2         // 12 or 24 hour time switch
#define ENHANCE_2_SW 3               // Enables the second enhance displays
#define CONVERT_TO_LOCAL_SW 3        // UTC time/date or local time/date switch
#define DISPLAY_HDOP_SW 4            // Hdop or horizontal position error switch
#define ALTITUDE_DATE_TIME_SW 5      // Altitude/Date time select switch
#define BACKLIGHT_SW 6               // LCD backlight select switch
#define LOW_SPEED_OVERRIDE  7        // Low speed override switch
#define CARDINAL_SW 8                // 16 or 8 cardinal heading points switch
#define RED_LED_PIN 9                // Red LED
#define GREEN_LED_PIN 10             // Green LED
#define BLUE_LED_PIN 11              // Blue LED
#define RXPin 12                     // Receive pin for the GPS SoftwareSerial interface
#define TXPin 13                     // Transmit pin for the GPS SoftwareSerial interface
#define GPS_RECEIVER_ERROR 2.5f      // Receiver error (in meters) for the GPS module (u-blox NEO-6M)
#define LED_OFF 0                    // Turns off the LEDs
#define SPEED_CUTOUT 10              // Value where the speed and heading are set to 0
//#define PERMANENT_DST              // Permanent DST (Comment out if and when this takes affect)
struct GPSStruct {
  double   lat;           // Degrees
  double   lon;           // Degrees
  double   speed;         // MPH
  double   altitude;      // Feet
  uint8_t satellites;
  double   heading;       // Degrees
  uint32_t hdop;          // Horizontal Dilution of Precision (Hdop)
  uint16_t year;
  uint8_t  month;
  uint8_t  day;
  uint8_t  hour;
  uint8_t  minute;
  uint8_t  second;
  bool     locationisValid; // Valid flags
  bool     speedisValid;
  bool     altitudeisValid;
  bool     courseisValid;
  bool     dateisValid;
  bool     timeisValid;
  bool     satellitesisValid;
  bool     hdopisValid;
  bool     dataAvailable;
};

/* A macro to compute the number of days elapsed since 2000 Jan 0.0 */
/* (which is equal to 1999 Dec 31, 0h UT)                           */
#define days_since_2000_Jan_0(y,m,d) \
  (367L*(y)-((7*((y)+(((m)+9)/12)))/4)+((275*(m))/9)+(d)-730530L)

/* Some conversion factors between radians and degrees */
#ifndef PI
#define PI        3.1415926535897932384
#endif

#define RADEG     ( 180.0 / PI )
#define DEGRAD    ( PI / 180.0 )

/* The trigonometric functions in degrees */
#define sind(x)  sin((x)*DEGRAD)
#define cosd(x)  cos((x)*DEGRAD)
#define tand(x)  tan((x)*DEGRAD)

#define atand(x)    (RADEG*atan(x))
#define asind(x)    (RADEG*asin(x))
#define acosd(x)    (RADEG*acos(x))
#define atan2d(y,x) (RADEG*atan2(y,x))

/* This macro computes times for sunrise/sunset.                      */
/* Sunrise/set is considered to occur when the Sun's upper limb is    */
/* 35 arc minutes below the horizon (this accounts for the refraction */
/* of the Earth's atmosphere).                                        */
#define sun_rise_set(year,month,day,lon,lat,rise,set)  \
  __sunriset__( year, month, day, lon, lat, -35.0/60.0, 1, rise, set )

/* This macro computes the length of the day, from sunrise to sunset. */
/* Sunrise/set is considered to occur when the Sun's upper limb is    */
/* 35 arc minutes below the horizon (this accounts for the refraction */
/* of the Earth's atmosphere).                                        */
#define day_length(year,month,day,lon,lat)  \
  __daylen__( year, month, day, lon, lat, -35.0/60.0, 1 )

// Function Prototypes
void displayVersionInfo(bool OneTime);
void displayValidFlags(void);
uint16_t leap(uint16_t year);
uint16_t zeller(uint16_t year, uint8_t month, uint8_t day);
uint8_t dayOfWeek(uint16_t year, uint8_t month, uint8_t day);
void displayDayOnLCD(uint8_t day);
const char* cardinal(double course, bool cardinalSelect);
bool IsDST(uint8_t day, uint8_t month, uint8_t DOW);
bool convertToLocal(uint16_t* hour, uint16_t* year, uint16_t* month,
                    uint16_t* day, const double lon, bool convertDate = true, bool sunset = false);
void displayHdopOnLCD(uint32_t Hdop, bool HdopSelect, unsigned long now,
                      unsigned long* prevHdopTime, bool* hdopToggle);

#endif // #define GPSLCD_h
