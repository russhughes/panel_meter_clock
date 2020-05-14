//
// Panel Meter Clock by Russ Hughes (russ@owt.com)
// April 2020
//
// Shared under the Creative Commons - Attribution - ShareAlike 3.0 license.
//

#include <Arduino.h>
#include <avr/pgmspace.h>
#include <EEPROM.h>
#include <RTClib.h>
#include <Adafruit_NeoPixel.h>

#include "config.h"

extern void updateMinute(uint16_t value);

//
// global configuration values
//

uint8_t HOURS_CAL[13] = {
    8, 27, 45, 64, 82, 102, 119, 136, 154, 171, 190, 207, 227
};

uint16_t MINUTES_CAL[61] = {
      96,  129,  162,  195,  228,  264,  296,  328,  360,  392,
     428,  460,  492,  524,  556,  590,  622,  654,  686,  718,
     754,  785,  816,  847,  878,  911,  943,  975, 1007, 1039,
    1074, 1105, 1136, 1167, 1198, 1232, 1262, 1292, 1322, 1352,
    1386, 1415, 1444, 1473, 1502, 1535, 1564, 1593, 1622, 1651,
    1684, 1714, 1744, 1774, 1804, 1838, 1868, 1898, 1928, 1958,
    1988
};

uint8_t globScale = 255;

uint8_t colorMode = DEFAULT_COLOR_MODE;
int8_t gmtOffset = DEFAULT_GMT_OFFSET;
uint8_t dstObs = DEFAULT_DST_OBSERVED;

uint8_t r = DEFAULT_R;
uint8_t g = DEFAULT_G;
uint8_t b = DEFAULT_B;

char aLatitude[MAX_LOC_LEN+1];
char aLongitude[MAX_LOC_LEN+1];

float longitude;
float latitude;

DateTime theTime;

//
// resetTimeFlags:
//	used when time or settings are changed
//

void resetTimeFlags() {
	lastDay = -1;
	lastHour = -1;
	lastMinute = -1;
	colorStep = 0;
}

//
// Load calibration data from EEPROM
//

void configLoad() {
	for (uint8_t val = 0; val < 13; val++) {
		EEPROM.get(val + EEPROM_HOURS_CAL, HOURS_CAL[val]);
	}

	for (uint8_t val = 0; val < 61; val++) {
		EEPROM.get(val*2 + EEPROM_MINUTES_CAL, MINUTES_CAL[val]);
	}

	EEPROM.get(EEPROM_COLOR_MODE, colorMode);
	EEPROM.get(EEPROM_GLOB_SCALE, globScale);
    EEPROM.get(EEPROM_GMT_OFFSET, gmtOffset);
    EEPROM.get(EEPROM_DST_OBS, dstObs);
    EEPROM.get(EEPROM_NEOPIXEL_R, r);
	EEPROM.get(EEPROM_NEOPIXEL_G, g);
	EEPROM.get(EEPROM_NEOPIXEL_B, b);

	for (int addr=0; addr < MAX_LOC_LEN; addr++) {
		aLatitude[addr] = EEPROM[EEPROM_LATITUDE+addr];
		aLongitude[addr] = EEPROM[EEPROM_LONGITUDE+addr];
	}

	aLatitude[EEPROM_LATITUDE+MAX_LOC_LEN+1] = 0;
	aLongitude[EEPROM_LONGITUDE+MAX_LOC_LEN+1] = 0;

	latitude = atof(aLatitude);
	longitude = atof(aLongitude);
}

//
// Save calibration data to EEPROM after calculating values between 5 minute marks
// and print C code for the default values that can be used in this program.
//

void configSave() {
	// calculate values between 5 minute marks

	for (int val = 5; val < 61; val +=5) {
		int first = MINUTES_CAL[val-5];
		int difference = MINUTES_CAL[val] - first;
		int step = difference / 5;

		for (int tick = 1; tick < 5; tick++) {
			MINUTES_CAL[val+tick-5] = first + step * tick;
		}
	}

	EEPROM[EEPROM_SENTINEL]   = 'P';
	EEPROM[EEPROM_SENTINEL+1] = 'M';
	EEPROM[EEPROM_SENTINEL+2] = 'C';

	Serial.println();
	Serial.println(F("// Your calibration values are:"));
	Serial.println();
	Serial.println(F("uint8_t HOURS_CAL[13] = {"));
	Serial.print(F("    "));

	for (int val = 0; val < 13; val++) {
		EEPROM.put(val + EEPROM_HOURS_CAL, HOURS_CAL[val]);
		Serial.print(HOURS_CAL[val]);
		if (val < 12)
			Serial.print(", ");
	}


	Serial.println();
	Serial.println("};");
	Serial.println();
	Serial.println(F("int MINUTES_CAL[61] = {"));
	Serial.print(F("    "));

	for (int val = 0; val < 61; val++) {
		EEPROM.put(EEPROM_MINUTES_CAL + val*2, MINUTES_CAL[val]);


		if (MINUTES_CAL[val] < 1000)
			Serial.print(" ");
		if (MINUTES_CAL[val] < 100)
			Serial.print(" ");

		Serial.print(MINUTES_CAL[val]);
		if (val < 60)
			Serial.print(", ");

		if (val && (val+1) % 10 == 0) {
			Serial.println();
			Serial.print(F("    "));
		}

	}

	Serial.println();
	Serial.println("};");

	EEPROM.put(EEPROM_COLOR_MODE, colorMode);
	EEPROM.put(EEPROM_GLOB_SCALE, globScale);
    EEPROM.put(EEPROM_GMT_OFFSET, gmtOffset);
    EEPROM.put(EEPROM_DST_OBS, dstObs);
    EEPROM.put(EEPROM_NEOPIXEL_R, r);
	EEPROM.put(EEPROM_NEOPIXEL_G, g);
	EEPROM.put(EEPROM_NEOPIXEL_B, b);

	for (int addr=0; addr < MAX_LOC_LEN; addr++) {
		EEPROM[EEPROM_LATITUDE+addr] = aLatitude[addr];
		EEPROM[EEPROM_LONGITUDE+addr] = aLongitude[addr];
	}
}

//
// Create EEPROM calibration data from constants.
//

void configCreate() {
//	Serial.println(F("Creating EEPROM calibration data using default data."));

	memset(aLatitude, 0, MAX_LOC_LEN+1);
	memset(aLongitude, 0, MAX_LOC_LEN+1);

	strncpy(aLatitude, DEFAULT_LATITUDE, MAX_LOC_LEN);
	strncpy(aLongitude, DEFAULT_LONGITUDE, MAX_LOC_LEN);

	configSave();
}

//
//	helper routine to calculate EEPROM offsets.
//

void configPrint() {
/*
	int address = 0;

	Serial.print(F("#define EEPROM_SENTINEL "));
	Serial.println(address);
	address += 3;

	Serial.print(F("#define EEPROM_HOURS_CAL "));
	Serial.println(address);
	address += sizeof(HOURS_CAL);

	Serial.print(F("#define EEPROM_MINUTES_CAL "));
	Serial.println(address);
	address += sizeof(MINUTES_CAL);

	Serial.print(F("#define EEPROM_COLOR_MODE "));
	Serial.println(address);
	address += sizeof(colorMode);

	Serial.print(F("#define EEPROM_GLOB_SCALE "));
	Serial.println(address);
	address += sizeof(globScale);

    Serial.print(F("#define EEPROM_GMT_OFFSET "));
    Serial.println(address);
    address += sizeof(gmtOffset);

    Serial.print(F("#define EEPROM_DST_OBS "));
    Serial.println(address);
    address += sizeof(dstObs);

	Serial.print(F("#define EEPROM_LATITUDE "));
	Serial.println(address);
	address += MAX_LOC_LEN;

    Serial.print(F("#define EEPROM_LONGITUDE "));
    Serial.println(address);
    address += MAX_LOC_LEN;
	Serial.println();
	*/
}

//
// Show Configuration Help
//

void configHelp() {

	configPrint();
	Serial.println();
	Serial.println(F("Configuration Menu"));
	Serial.println(F("========================"));
	Serial.println(F("'?' Show help again"));
	Serial.println(F("'h' Hour meter adjust"));
	Serial.println(F("'m' Minute meter adjust"));
	Serial.println(F("'i' Increase PWM"));
	Serial.println(F("'d' Decrease PWM"));
	Serial.println(F("'n' Next hour/minute PWM"));
	Serial.println(F("'p' Previous hour/minute PWM"));
	Serial.println(F("'c' change colormode"));
	Serial.println(F("'s' Sweep minutes"));
	Serial.println(F("'t' Set time"));
	Serial.println(F("'l' Set location"));
	Serial.println(F("'w' Write to EEPROM"));
	Serial.println(F("'q' Quit menu"));
	Serial.println();
}

//
// demo wheel mode
//

void demoWheel() {
	for (byte i = 0; i < 255; i++) {
		setColor(Wheel(i));
		delay(50);
	}
}

//
// demo sky mode
//

void demoSky() {
	int year = theTime.year();
	int month = theTime.month();
	int day = theTime.day();

	for (int hour = 0; hour < 24; hour++) {
		analogWrite(HOURPWM, HOURS_CAL[hour % 13]);
		for (int minute = 0; minute < 60; minute += 10) {
			theTime = DateTime(year, month, day, hour, minute);
            setPixelColor(sky_angle-(M_PI/2), 255);
			delay(50);
		}
	}
}

//
// demo sun mode
//

void demoSun() {
	int year = theTime.year();
	int month = theTime.month();
	int day = theTime.day();

	for (int hour = 0; hour < 24; hour++) {
		analogWrite(HOURPWM, HOURS_CAL[hour % 13]);
		for (int minute = 0; minute < 60; minute += 15) {
			theTime = DateTime(year, month, day, hour, minute);
			setPixelColor(sky_angle, 255);
			delay(50);
		}
	}
}

//
// char getValue(char *buffer, int maxlength)
//  Edit char string in buffer with maxLength
//	Returns:
//		ESC if escape key pressed,
//		CR if return key pressed,
//		0 if buffer is NULL
//

char getValue(char *buffer, int maxLength) {
	char erase[4] = "\x08 \x08";
	char c = 0;

	if (buffer) {
		Serial.print(buffer);
		int l = strlen(buffer);

		while (c != 0x0d && c !=0x1b) {
			if (Serial.available()) {
				c = Serial.read();

				switch (c) {
					case 0x08:
					case 0x7f:
						if (l) {
							Serial.print(erase);
							buffer[--l] = 0;
						}
					break;

					case '0':
					case '1':
					case '2':
					case '3':
					case '4':
					case '5':
					case '6':
					case '7':
					case '8':
					case '9':
					case '.':
						if (l < maxLength-1) {
							buffer[l++] = c;
							buffer[l] = 0;
							Serial.print(c);
						}
					break;

					case '-':
						if (l == 0) {
							buffer[l++] = '-';
							buffer[1] = 0;
							Serial.print(c);
						}
					break;
				}
			}
		}
	}
	Serial.println();
	return c;
}

//
// char getInt(int *value)
//  Edit integer value
//	Returns:
//		ESC if escape key pressed,
//		CR if return key pressed,
//		0 if buffer is NULL
//

#define BUFFER_SIZE 10
char getInt(int *value) {
	char buffer[BUFFER_SIZE+1];
	char c = 0;

	if (value) {
		itoa(*value, buffer, 10);
		c = getValue(buffer, BUFFER_SIZE);
		if (c == 0x0d)
			*value = (int) atoi(buffer);
	}
	return c;
}

//
// Set r, g, b color values for MODE_FIXED
//

void configColor(void) {
	int new_r = r;
	int new_g = g;
	int new_b = b;

	bool again = true;
	int count = 0;
	char ch;

	while (again) {
		switch (count) {
			case 0:
				Serial.print(F("Value for Red (0-255) ? "));
				ch = getInt(&new_r);
			break;

			case 1:
				Serial.print(F("Value for Green (0-255) ? "));
				ch = getInt(&new_g);
			break;

			case 2:
				Serial.print(F("Value for Blue (0-255) ? "));
				ch = getInt(&new_b);
			break;
		}

		if (ch == 0x0d) {
			if (count == 2) {
				r = new_r;
				g = new_g;
				b = new_b;
			  	setColor(pixel.Color(r, g, b));
				again = false;
			}
			else
				count++;
		}

		if (ch == 0x1b) {
			again = false;
		}
	}
}

//
// change color mode and demo it.
//

void configColorMode(void) {
    char ch = Serial.read();
    bool again = true;

    Serial.println(F("Select color mode: "));
    Serial.println(F("0 - None."));
    Serial.println(F("1 - Fixed Color."));
    Serial.println(F("2 - Color Wheel."));
    Serial.println(F("3 - Sky."));
    Serial.println(F("4 - Sun."));
    Serial.print(F("? "));

    while (again) {
        if (Serial.available()) {
            ch = Serial.read();
            switch(ch) {

				case 0x1b:
					again = false;
				break;

                case '0':
                    colorMode = MODE_NONE;
                    again = false;
                    Serial.println(F("None"));
                    setColor(0);
                break;

                case '1':
                    colorMode = MODE_FIXED;
                    again = false;
                    Serial.println(F("Fixed"));
					configColor();
                break;

                case '2':
                    colorMode = MODE_WHEEL;
                    again = false;
                    Serial.println(F("Wheel"));
                    demoWheel();
                break;

                case '3':
                    colorMode = MODE_SKY;
                    again = false;
                    Serial.println(F("Sky"));
                    demoSky();
                break;

                case '4':
                    colorMode = MODE_SUN;
                    again = false;
                    Serial.println(F("Sun"));
                    demoSun();
                break;
            }
        }
        delay(100);
    }
}

//
// set the time
//

void configTime(void) {
	theTime = rtc.now();
	int hour = theTime.hour();
	int minute = theTime.minute();
	int offset = gmtOffset;
	int year = theTime.year();
	int month = theTime.month();
	int day = theTime.day();
	int observed = dstObs;
	bool again = true;
	int count = 0;
	char ch;

	Serial.println(theTime.timestamp());
	Serial.println(F("Enter new time or press ESC to quit."));
	while (again) {

		switch (count) {
			case 0:
				Serial.print(F("Hour (0-23) ? "));
				ch = getInt(&hour);
			break;

			case 1:
				Serial.print(F("Minute (0-59) ? "));
				ch = getInt(&minute);
			break;

			case 2:
				Serial.print(F("GMT offset (-West) ? "));
				ch = getInt(&offset);
			break;

			case 3:
				Serial.print(F("DST Observed (0-No,1-Yes) ? "));
				ch = getInt(&observed);
			break;

			case 4:
				Serial.print(F("4 digit year ? "));
				ch = getInt(&year);
			break;

			case 5:
				Serial.print(F("month ? "));
				ch = getInt(&month);
			break;

			case 6:
				Serial.print(F("day ? "));
				ch = getInt(&day);
			break;
		}

		if (ch == 0x0d) {
			if (count == 6) {
				DateTime newTime = DateTime(year, month, day, hour, minute);

				if (dstObs) {	// if DST observed
					if (isDST(newTime.day(), newTime.month(), newTime.dayOfTheWeek())) {
						if (newTime.hour() >= 2) {
							newTime = newTime - TimeSpan(0, 1, 0, 0);
							dstActive = 1;
						}
					}
				}

				rtc.adjust(newTime);
				Serial.println(now().timestamp());
				gmtOffset = offset;
			    dstObs = observed;
				again = false;
			}
			else
				count++;
		}

		if (ch == 0x1b) {
			again = false;
		}
	}
}

//
// set the location
//

void configLocation(void) {
	bool again = true;
	int count = 0;
	char ch;

	Serial.println(theTime.timestamp());
	Serial.println(F("Enter new location or press ESC to quit."));
	while (again) {
		switch (count) {
			case 0:
				Serial.print(F("Latitude (+N)? "));
				ch = getValue(aLatitude, 10);
			break;

			case 1:
				Serial.print(F("Longitude (-W)? "));
				ch = getValue(aLongitude, 10);
			break;
		}

		if (ch == 0x0d) {
			if (count == 1) {
				latitude = atof(aLatitude);
				longitude = atof(aLongitude);
				again = false;
			}
			else
				count++;
		}

		if (ch == 0x1b) {
			again = false;
		}
	}
}

//
// configure menu
//

void configMenu() {
	enum meter{hours, minutes};
	meter adjust = hours;
	bool again = true;

	uint8_t hour = 0;
	uint8_t minute = 0;
	uint8_t hour_pwm = 0;
	uint8_t minute_pwm = 0;

	configHelp();

	Serial.println(F("Adjusting Hours"));
	Serial.print(">");

	while(again) {
		if (Serial.available()) {
			char ch = Serial.read();
			if (isPrintable(ch))
				Serial.println(ch);

			switch (tolower(ch)) {

				case '?':
					configHelp();
				break;

				case 'h':
					Serial.println(F("Adjusting Hours"));
					adjust = hours;
				break;

				case 'm':
					Serial.println(F("Adjusting Minutes"));
					adjust = minutes;
				break;

				case 'i':
					if (adjust == hours)
						HOURS_CAL[hour]++;

					if (adjust == minutes)
						MINUTES_CAL[minute]++;
				break;

				case 'd':
					if (adjust == hours)
						HOURS_CAL[hour]--;

					if (adjust == minutes)
						MINUTES_CAL[minute]--;
				break;

				case 'n':
					if (adjust == hours) {
						if (hour < 12)
							hour++;
					}

					if (adjust == minutes) {
						if (minute < 60)
						minute += 5;
					}
				break;

				case 'p':
					if (adjust == hours) {
						if (hour)
							hour--;
					}

					if (adjust == minutes) {
						if (minute)
							minute -=5;
					}
				break;

				case 'c':
                    configColorMode();
				break;

				case 's':
					for (int val = 0; val < 61; val++) {
						Serial.print(F("Minute: "));
						Serial.print(val);
						Serial.print(F("pwm: "));
						Serial.println(MINUTES_CAL[val]);
						updateMinute(MINUTES_CAL[val]);
						delay(500);
					}
				break;

				case 't':
					configTime();
				break;

				case 'l':
					configLocation();
				break;

				case 'w':
					configSave();
				break;

				case 'q':
				case 0x1b:
					Serial.println(F("Exiting menu..."));
					resetTimeFlags();
					again = false;
				break;
			}

			if (again) {
				if (adjust == hours) {
					Serial.print(F("Adjusting hour: "));
					Serial.print(hour);
					Serial.print(F(" pwm: "));
					Serial.println(HOURS_CAL[hour]);
				}

				if (adjust == minutes) {
					Serial.print(F("Adjusting Minute: "));
					Serial.print(minute);
					Serial.print(F(" pwm: "));
					Serial.println(MINUTES_CAL[minute]);
				}

				analogWrite(HOURPWM, HOURS_CAL[hour]);
				updateMinute(MINUTES_CAL[minute]);
				Serial.print(">");
			}
		}
	}
}
