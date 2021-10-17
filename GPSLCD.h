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
   
*/

#ifndef GPSLCD_h
#define GPSLCD_h

#define VERSION  "8.0"               // Version number
//#define DATA_VALID_OVERRIDE        // Override the data valid flag (comment out to turn off)
                                     // It also feed fake GPS data to the system for debugging

#define Rs_pin  0                    // Pin hook up for the LCD via the I2C interface
#define Rw_pin  1                    //                   "
#define En_pin  2                    //                   "
#define BACKLIGHT_PIN 3              //                   "
#define D4_pin  4                    //                   "
#define D5_pin  5                    //                   "
#define D6_pin  6                    //                   "
#define D7_pin  7                    //                   "
#define ON_BOARD_LED 13              // On-board LED
#define BACKLIGHT_SW 6               // LCD backlight select switch
#define INITIALIZATION_INTERVAL 1000 // Initialization display interval
#define SPEED_CUTOUT 10.0            // Value where the speed & heading are set to 0
#define TOGGLETIME_INTERVAL 6000     // Toggle display interval
#define CREDIT "GKG"                 // Yours truly :-)
//#define BACKLIGHT_OVERRIDE         // Override the LCD backlight switch (comment out to disable)
#ifdef BACKLIGHT_OVERRIDE
#define BACKLIGHT_ONOFF HIGH         // Works with BACKLIGHT_OVERRIDE to force on/off the LCD backlight
#endif

#define COLUMN 20                    // Number of Columns (On-board LED)
#define ROW 4                        // Number of rows (On-board LED)
#define ALTITUDE_DATE_TIME_SW 5      // Altitude/Date time select switch
#define I2C_ADDR    0x3F             // Address of my 20x4 LCD (yours might be different)
#define DISPLAY_12_HOUR_SW 2         // 12 or 24 hour time switch
#define CONVERT_TO_LOCAL_SW 3        // UTC time/date or local time/date switch
#define DISPLAY_HDOP_SW 4            // Hdop or horizontal position error switch
#define CARDINAL_SW 8                // 16 or 8 cardinal heading points switch
#define LOW_SPEED_OVERRIDE  7        // Low speed override switch
#define GPS_RECEIVER_ERROR 2.5f      // Receiver error (in meters) for the GPS module (Ublox NEO-6M)

struct GPSStruct {
  double   lat;           // Degrees
  double   lon;           // Degrees
  double   speed;         // MPH
  double   altitude;      // Feet
  uint32_t satellites;
  double   heading;       // Degrees
  uint32_t hdop;
  uint16_t year;
  uint8_t  month;
  uint8_t  day;
  uint8_t  hour;
  uint8_t  minute;
  uint8_t  second;
};

// Function Prototypes
uint16_t leap(uint16_t year);
uint16_t zeller(uint16_t year, uint8_t month, uint8_t day);
uint8_t dayOfWeek(uint16_t year, uint8_t month, uint8_t day);
void displayDayOnLCD(uint8_t day);
const char* cardinal(double course, bool cardinalSelect);
bool IsDST(uint8_t day, uint8_t month, uint8_t DOW);
bool convertToLocal(uint8_t* hour, uint16_t* year, uint8_t* month,
                    uint8_t* day, const double lon, bool convertDate, bool doDST = true);
void displayHdopOnLCD(uint32_t Hdop, bool HdopSelect, unsigned long now,
                      unsigned long* prevHdopTime, bool* hdopToggle);

#endif // #define GPSLCD_h
