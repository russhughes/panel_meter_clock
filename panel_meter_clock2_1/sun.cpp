//
// Panel Meter Clock by Russ Hughes (russ@owt.com)
// April 2020
//
// Shared under the Creative Commons - Attribution - ShareAlike 3.0 license.
//

//
// Solar Calculations based on formulas from the NOAA Sunrise/Sunset and Solar
// Position Calculator Excel spreadsheet availabe from:
//
// https://www.esrl.noaa.gov/gmd/grad/solcalc/NOAA_Solar_Calculations_day.xls
//

#include <Arduino.h>
#include <math.h>
#include <stdint.h>

#include "sun.h"

//
// calculate the julian date for a given year, month and day
//

static long julianDate(int year, int month, int day) {
  return day-32075+1461L*(year+4800+(month-14)/12)/4+367* \
  (month-2-(month-14)/12*12)/12-3*((year+4900+(month-14)/12)/100)/4;
}

//
// calculate the solar noon for the given location, date and timeZone
//

float calcSolarNoon(float latitude, float longitude, int year, int month, int day, int timeZone) {
    float julianDay = julianDate(year, month, day) -(float)timeZone/24;
    float julianCentury = (julianDay-2451545)/36525;
    float geomMeanLongSun = fmod(280.46646+julianCentury*(36000.76983+julianCentury*0.0003032),360);
    float geomMeanAnomSun = 357.52911+julianCentury*(35999.05029-0.0001537*julianCentury);
    float eccentEarthOrbit = 0.016708634-julianCentury*(0.000042037+0.0000001267*julianCentury);
    float meanObliqEcliptic = 23+(26+((21.448-julianCentury*(46.815+julianCentury*(0.00059-julianCentury*0.001813))))/60)/60;
    float obliqCorr = meanObliqEcliptic+0.00256*cos(radians(125.04-1934.136*julianCentury));
    float varY = tan(radians(obliqCorr/2))*tan(radians(obliqCorr/2));

    float eqOfTime = 4*degrees(varY*sin(2*radians(geomMeanLongSun))-2*\
        eccentEarthOrbit*sin(radians(geomMeanAnomSun))+4*\
        eccentEarthOrbit*varY*sin(radians(geomMeanAnomSun))*\
        cos(2*radians(geomMeanLongSun))-0.5*\
        varY*varY*sin(4*radians(geomMeanLongSun))-1.25*\
        eccentEarthOrbit*eccentEarthOrbit*sin(2*radians(geomMeanAnomSun)));

    return (720-4*longitude-eqOfTime+timeZone*60)/1440;
}

//
// calculate the solar zenith angle for the given location, date and timeZone
//

float calcSolarZenithAngle(float latitude, float longitude,
    int year, int month, int day, int hour, int minute, int timeZone) {

    float localPastMidnight = (float)hour/24+((float)minute/1440);
    float julianDay = julianDate(year, month, day)-0.5+localPastMidnight-(float)timeZone/24;
    float julianCentury = (julianDay-2451545)/36525;
    float geomMeanLongSun = fmod(280.46646+julianCentury*(36000.76983+julianCentury*0.0003032),360);
    float geomMeanAnomSun = 357.52911+julianCentury*(35999.05029-0.0001537*julianCentury);
    float eccentEarthOrbit = 0.016708634-julianCentury*(0.000042037+0.0000001267*julianCentury);

    float sunEqofCtr = sin(radians(geomMeanAnomSun))*\
        (1.914602-julianCentury*(0.004817+0.000014*julianCentury))+\
        sin(radians(2*geomMeanAnomSun))*(0.019993-0.000101*julianCentury)+\
        sin(radians(3*geomMeanAnomSun))*0.000289;

    float sunTrueLong = geomMeanLongSun+sunEqofCtr;
    float sunAppLong = sunTrueLong-0.00569-0.00478*sin(radians(125.04-1934.136*julianCentury));
    float meanObliqEcliptic = 23+(26+((21.448-julianCentury*(46.815+julianCentury*(0.00059-julianCentury*0.001813))))/60)/60;
    float obliqCorr = meanObliqEcliptic+0.00256*cos(radians(125.04-1934.136*julianCentury));
    float sunDeclin = degrees(asin(sin(radians(obliqCorr))*sin(radians(sunAppLong))));
    float varY = tan(radians(obliqCorr/2))*tan(radians(obliqCorr/2));

    float eqOfTime = 4*degrees(varY*sin(2*radians(geomMeanLongSun))-2*\
        eccentEarthOrbit*sin(radians(geomMeanAnomSun))+4*\
        eccentEarthOrbit*varY*sin(radians(geomMeanAnomSun))*\
        cos(2*radians(geomMeanLongSun))-0.5*\
        varY*varY*sin(4*radians(geomMeanLongSun))-1.25*\
        eccentEarthOrbit*eccentEarthOrbit*sin(2*radians(geomMeanAnomSun)));

    float trueSolarTime = fmod(localPastMidnight*1440+eqOfTime+4*longitude-60.0*timeZone,1440);
    float hourAngle = (trueSolarTime/4<0 ) ? trueSolarTime/4+180 : trueSolarTime/4-180;
    float solarZenithAngle = degrees(acos(sin(radians(latitude))*sin(radians(sunDeclin))+cos(radians(latitude))* \
        cos(radians(sunDeclin))*cos(radians(hourAngle))));

   return solarZenithAngle;
}

//
// return hour and minute from given decimal time
//

void decToHourMinute(float time, int *hour, int *minute) {
    time *= 24;
    *hour = floor(time);
    *minute = floor((time - *hour)*60);
}
