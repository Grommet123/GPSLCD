/* GPS LCD

    By GK Grotsky
    8/30/16
*/

#ifndef GPSLCD_h
#define GPSLCD_h

#define VERSION  "1.5"               // Version number
//#define _16x2                      // LCD type (20x4 or 16x2). Comment out for 20x4
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
#define BACKLIGHT_ONOFF LOW          // Works with BACKLIGHT_OVERRIDE to force on/off the LCD backlight
#endif

#ifdef _16x2 //------------------------------------------------------
#define COLUMN 16                    // Number of Columns
#define ROW 2                        // Number of rows
#define SPEED_ALTITUDE_SW 5          // Speed/Altitude select switch
#define I2C_ADDR    0x27             // Address of my 16x2 LCD (yours might be different)

// Function Prototypes
//     Nada
#else // _20x4 ------------------------------------------------------
#define COLUMN 20                    // Number of Columns
#define ROW 4                        // Number of rows
#define ALTITUDE_DATE_TIME_SW 5      // Altitude/Date time select switch
#define I2C_ADDR    0x3F             // Address of my 20x4 LCD (yours might be different)
#define DISPLAY_12_HOUR              // Display either 12 or 24 hour time (comment out for 24)
#define CONVERT_TO_LOCAL             // Convert the UTC time/date to local time/date (comment out for UTC)
//#define DISPLAY_HDOP               // Displays Hdop or horizontal position error (comment out for horizontal position error)
#ifndef DISPLAY_HDOP
#define GPS_RECEIVER_ERROR 2.5f      // Receiver error for the GPS module (Ublox NEO-6M)
#endif
//#define _16CARDINAL                // Displays 16 cardinal heading points (comment out for 8)

// Function Prototypes
uint16_t leap (uint16_t year);
uint16_t zeller (uint16_t year, uint8_t month, uint8_t day);
uint8_t dayOfWeek (uint16_t year, uint8_t month, uint8_t day);
void printDay(uint8_t day);
const char* cardinal(double course);
bool IsDST(uint8_t day, uint8_t month, uint8_t DOW);
#ifdef CONVERT_TO_LOCAL
bool convertToLocal (uint8_t* hour, uint16_t* year, uint8_t* month,
                     uint8_t* day, const double lon, bool convertDate);
#endif // #ifdef CONVERT_TO_LOCAL
#endif //------------------------------------------------------------

struct GPSStruct {
  double   lat;           // Degrees
  double   lon;           // Degrees
  double   speed;         // MPH
  double   altitude;      // Feet
  uint32_t satellites;
#ifndef _16x2
  double   heading;       // Degrees
  uint32_t hdop;
  uint16_t year;
  uint8_t  month;
  uint8_t  day;
  uint8_t  hour;
  uint8_t  minute;
  uint8_t  second;
#endif // #ifndef _16x2
};

#endif // #define GPSLCD_h
