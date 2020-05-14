//
// Panel Meter Clock by Russ Hughes (russ@owt.com)
// April 2020
//
// Shared under the Creative Commons - Attribution - ShareAlike 3.0 license.
//

#ifndef __SUN_H__
#define __SUN_H__

extern float calcSolarNoon(float latitude, float longitude, int year, int month, int day, int timeZone);
extern float calcSolarZenithAngle(float latitude, float longitude, int year, int month, int day, int hour, int minute, int timeZone);
extern void decToHourMinute(float time, int *hour, int *minute);

#endif
