//
// Panel Meter Clock by Russ Hughes (russ@owt.com)
// April 2020
//
// Shared under the Creative Commons - Attribution - ShareAlike 3.0 license.
//

#include <avr/pgmspace.h>
#include <EEPROM.h>
#include <RTClib.h>
#include <Adafruit_NeoPixel.h>
#include <stdint.h>
#include <math.h>

#include "config.h"

int dosetdate = 0;	// set to 1 to always set date/time to compiled time.

//
// Solar Calculations based on formulas from the NOAA Sunrise/Sunset and Solar Position
// Calculator Excel spreadsheet availabe from:
//
// https://www.esrl.noaa.gov/gmd/grad/solcalc/NOAA_Solar_Calculations_day.xls
//

#include "sun.h"

/*
 * colourcalc.h
 *
 * Created: 19/10/2016 15:06:02
 *  Author: David Brown
 * Shared under the Creative Commons - Attribution - ShareAlike 3.0 license.
 */

#include "colourcalc.h"
perez colour;

// Prototypes
float level(float in);

// Globals

float sky_angle = 1.309;
float theta_max;
float theta_sun;
float scalar = 3;
float turbidity = 1.8;

int dstActive = 0;
int	lastDay = -1;
int lastHour = -1;
int lastMinute = -1;
int colorStep = 0;

Adafruit_NeoPixel pixel = Adafruit_NeoPixel(1, NEOPIXEL, NEO_GRB + NEO_KHZ800);
RTC_DS3231 rtc;

//
// Returns 1 if DST should be active today
//

int isDST(int day, int month, int dow) {
   if (month < 3 || month > 11) {
      return 0;
   }
   if (month > 3 && month < 11) {
      return 1;
   }
   int previousSunday = day - dow;
   if (month == 3) {
      return previousSunday >= 8;
   }
   return previousSunday <= 0;
}

//
// Returns the current time as a DateTime object adjusting for
// US daylight saving time as needed if dstObs is set to a true value
//

DateTime now(void) {

    // get the time from the RTC
    DateTime theTime = rtc.now();

    // check if DST is observed in this location
    if (dstObs) {
        // check if DST should be in effect today
        if (isDST(theTime.day(), theTime.month(), theTime.dayOfTheWeek())) {
            // if DST is not currently active & it is after 02:00
            if (!dstActive && theTime.hour() >= 2)
                dstActive = 1;          // make DST active
        } else {
            // if DST is currently active & it is after 02:00
        if (dstActive && theTime.hour() >= 2)
            dstActive = 0;  // make DST not active
        }
    } else {
		dstActive = 0;	// DST not observed so not active
	}

	// if DST is active spring forward one hour
    if (dstActive)
        return theTime + TimeSpan(0, 1, 0, 0);

	// DST is not active use RTC time directly
	return theTime;
}

//
// Calculates the solar max using the height of the sun
// in radians at solar noon for the current day.
//

float calcSolarMax() {
	int solarNoonHour;
	int solarNoonMinute;

	theTime = now();
	float solarNoon = calcSolarNoon(
		latitude,
		longitude,
		theTime.year(),
		theTime.month(),
		theTime.day(),
		gmtOffset
	);

	decToHourMinute(solarNoon, &solarNoonHour, &solarNoonMinute);

	return radians(
		calcSolarZenithAngle(
			latitude,
			longitude,
			theTime.year(),
			theTime.month(),
			theTime.day(),
			solarNoonHour,
			solarNoonMinute,
			gmtOffset
		)
	);
}

//
// Setup IO pins, PWN, RTC and NeoPixel.
//

void setup() {
    Serial.begin(9600);

	// Check EEPROM for calibration data

	if (EEPROM[EEPROM_SENTINEL] != 'P' && EEPROM[EEPROM_SENTINEL+1] != 'M' &&EEPROM[EEPROM_SENTINEL+2] != 'C') {
		configCreate();
	}

	//configCreate();  // uncomment to override EEPROM settings with defaults
	configLoad();

    pinMode(HOURPWM, OUTPUT);	// hour pwm pin
    pinMode(MINPWM, OUTPUT);	// minute pwm pin
    pinMode(HOURADJ, INPUT);	// hour adjust pin
    pinMode(MINADJ, INPUT);		// minute adjust pin

	//
	// Set D6 to 10 bit PWM
	//

    TCCR4E |= (1<<ENHC4);
    TCCR4B &= ~(1<<CS41);
    TCCR4B |= (1<<CS42)|(1<<CS40);
    TCCR4D |= (1<<WGM40);
    TC4H = 0x3;
    OCR4C = 0xFF;
    TCCR4C |= (1<<COM4D1)|(1<<PWM4D);

	//
    // Start the clock
	//

    if (! rtc.begin()) {
		while(1) {
        	Serial.println(F("Could not find RTC!"));
			delay(1000);
		}
    }

	//
	// if the RTC lost power set the RTC to the date & time this sketch was compiled
	//

    if (dosetdate || rtc.lostPower()) {
        Serial.println(F("RTC was NOT running!"));
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }

	//
	// Get the solar max for today
	//

	theta_max = calcSolarMax();

	//
	// Start the NeoPixel
	//

    pixel.begin();
	if (colorMode == MODE_FIXED) {
		pixel.setPixelColor(0, pixel.Color(r, g, b));
		pixel.show();
	}

	//
	// Initialize the color class & coefficients
	//

	colour.generate_perez_coeff(turbidity);
}

//
//	Update minute meter 10 bit PWM
//

void updateMinute(uint16_t value) {
	TC4H = value>>8;
    OCR4D = (value&0xFF);
}

//
//  Sets the NeoPixel color
//

void setColor(uint32_t color) {
   	pixel.setPixelColor(0, color);
   	pixel.show();
}

//
// Return the color for the given WheelPos
//

uint32_t Wheel(byte WheelPos) {
    WheelPos = 255 - WheelPos;
    if (WheelPos < 85) {
        return pixel.Color(255 - WheelPos * 3, 0, WheelPos * 3);
    }
    if (WheelPos < 170) {
        WheelPos -= 85;
        return pixel.Color(0, WheelPos * 3, 255 - WheelPos * 3);
    }
    WheelPos -= 170;
    return pixel.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

//
// Main Loop
//

void loop() {

	int day;
    int	hour;
    int minute;

	//
	//	Bring up the configure menu if the
	//	escape key is pressed
	//

	if (Serial.available()) {
		char ch = Serial.read();
		if (ch == 27)
			configMenu();
	}

	//
	//	Advance the hour if the hour adjust button
	// 	is pressed and released
	//

    if (digitalRead(HOURADJ) == LOW)  {
        while (digitalRead(HOURADJ) == LOW)
            delay(50);

        rtc.adjust(rtc.now() + TimeSpan(0, 1, 0, 0));
    }

	//
	//	Advance the minute if the minute adjust button
	// 	is pressed and released
	//

    if (digitalRead(MINADJ) == LOW)  {
        while (digitalRead(MINADJ) == LOW)
            delay(50);

        rtc.adjust(rtc.now() + TimeSpan(0, 0, 1, 0));
    }

	//
	// Get the current time
	//

    theTime = now();

    hour = theTime.twelveHour();
    if (hour == 12)
    	hour = 0;

    minute = theTime.minute();

    //
    // if the day has changed
    //  calculate the solar max for the day
    //

	day = theTime.day();
	if (day != lastDay) {
		theta_max = calcSolarMax();
		lastDay = day;
	}

	//
	// if the hour has changed update the hour meter,
	//		sweep the meter back if last hour was 12
	//

    if (hour != lastHour) {
        if (lastHour == 12) {
            for (int i = lastHour; i != 0; i--) {
                analogWrite(HOURPWM, HOURS_CAL[i]);
                delay(1000 / 12);
            }
        }
        else
            analogWrite(HOURPWM, HOURS_CAL[hour]);

        lastHour = hour;
    }

	//
	// if the minute has changed update the minute meter,
	//		sweep the meter back if last minute was 59
	//

    if (minute != lastMinute) {
        if (lastMinute == 59) {
            for (int i = lastMinute; i != 0; i--) {
                updateMinute(MINUTES_CAL[i]);
                delay(1000 / 59);
            }
        }
        else
            updateMinute(MINUTES_CAL[minute]);

        lastMinute = minute;

        // Update the NeoPixel color

    	if (colorMode == MODE_SUN) {
    		setPixelColor(sky_angle, 255);
    	} else if (colorMode == MODE_SKY) {
    		setPixelColor(sky_angle-(M_PI/2), 255);
    	}
    }

	//
	//	Step the NeoPixel color to the next color
	//		and wait 100ms
	//

	if (colorMode == MODE_WHEEL) {
    	colorStep++;
    	colorStep %= 255;
    	pixel.setPixelColor(0, Wheel(colorStep));
    	pixel.show();
	}

    delay(100);
}

//
// calculate and set the NeoPixel color
//
// color calculation code based on:
//
// Sunrise_v10-AVR.cpp
// Created: 19/10/2016 10:17:11
// Author : David Brown
// Shared under the Creative Commons - Attribution - ShareAlike 3.0 license.
//

float level(float in)
{
	if (in <= 0)
		return 0.0;

	return in;
}

void setPixelColor(float sky_angle, uint8_t glob_scale)
{
	RGB_value f_value;
    float theta_sun;
    float gamma = 1/1.8;

	theta_sun = radians(
		calcSolarZenithAngle(
			latitude,
			longitude,
			theTime.year(),
			theTime.month(),
			theTime.day(),
			theTime.hour(),
			theTime.minute(),
			gmtOffset
		)
	);

	//check theta sun is valid, cant be less than max theta and 360 deg - max theta
	if (theta_sun < theta_max || theta_sun > ((2*M_PI)-theta_max)) {
		theta_sun = 3;
	}

	// scalar function pulled output to zero when sun below civic twilight and then uses
	// cosine distribution to scale the intensity from current to maximum sun angle (midday)

	scalar = level(cos((theta_sun-theta_max)*1.5));
	f_value = colour.calc_RGB_out(theta_sun*0.01745329252, sky_angle, turbidity);
	f_value.R = (pow(level(f_value.R),(gamma))*scalar);
	f_value.G = (pow(level(f_value.G),(gamma))*scalar);
	f_value.B = (pow(level(f_value.B),(gamma))*scalar);

	r = (uint8_t)(f_value.R*glob_scale);
	g = (uint8_t)(f_value.G*glob_scale);
	b = (uint8_t)(f_value.B*glob_scale);

  	pixel.setPixelColor(0, pixel.Color(r, g, b));
    pixel.show();
}
