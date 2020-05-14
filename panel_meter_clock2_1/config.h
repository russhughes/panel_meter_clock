//
// Panel Meter Clock by Russ Hughes (russ@owt.com)
// April 2020
//
//
// Shared under the Creative Commons - Attribution - ShareAlike 3.0 license.
//

#ifndef __CONFIG_H__
#define __CONFIG_H__

// define pins usage

#define HOURPWM 5
#define MINPWM  6
#define HOURADJ 8
#define MINADJ 11
#define NEOPIXEL 10

// EEPROM config value offsets

#define EEPROM_SENTINEL 0
#define EEPROM_HOURS_CAL 3
#define EEPROM_MINUTES_CAL 16
#define EEPROM_COLOR_MODE 138
#define EEPROM_GLOB_SCALE 139
#define EEPROM_GMT_OFFSET 140
#define EEPROM_DST_OBS 141
#define EEPROM_NEOPIXEL_R 142
#define EEPROM_NEOPIXEL_G 143
#define EEPROM_NEOPIXEL_B 144
#define EEPROM_LATITUDE 145
#define EEPROM_LONGITUDE 155
#define EEPROM_AVAIL 165

// colorModes

#define MODE_NONE 0
#define MODE_FIXED 1
#define MODE_WHEEL 2
#define MODE_SKY 3
#define MODE_SUN 4

//
// BEGIN DEFAULT CONFIG VALUES
//

#define DEFAULT_COLOR_MODE MODE_WHEEL
#define DEFAULT_GMT_OFFSET -8
#define DEFAULT_DST_OBSERVED 1
#define DEFAULT_LATITUDE "46.2087"
#define DEFAULT_LONGITUDE "-119.1199"
#define DEFAULT_R 32
#define DEFAULT_G 0
#define DEFAULT_B 0

//
// END DEFAULT CONFIG VALUES
//

#define MAX_LOC_LEN 10

extern char aLatitude[MAX_LOC_LEN+1];
extern char aLongitude[MAX_LOC_LEN+1];

extern uint8_t HOURS_CAL[13];
extern uint16_t MINUTES_CAL[61];
extern uint8_t colorMode;
extern uint8_t globScale;
extern int8_t gmtOffset;
extern uint8_t dstObs;
extern float longitude;
extern float latitude;

extern int dstActive;
extern int lastDay;
extern int lastHour;
extern int lastMinute;
extern int colorStep;
extern uint8_t r, g, b;

extern float sky_angle;
extern DateTime theTime;

extern Adafruit_NeoPixel pixel;
extern RTC_DS3231 rtc;

void configCreate();
void configLoad();
void configMenu();

extern int isDST(int day, int month, int dow);
extern DateTime now(void);

extern void setColor(uint32_t color);
extern uint32_t Wheel(byte WheelPos);
extern void setPixelColor(float sky_angle, uint8_t glob_scale);

#endif
