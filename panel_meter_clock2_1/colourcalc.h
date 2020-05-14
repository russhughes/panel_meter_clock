/*
 * colourcalc.h
 *
 * Created: 19/10/2016 15:06:02
 *  Author: David Brown
 * Shared under the Creative Commons - Attribution - ShareAlike 3.0 license.
 */

#ifndef COLOURCALC_H_
#define COLOURCALC_H_

#include <math.h> //include math functions
#include <stdint.h>

//data structure for coefficients
struct perez_coefficient
{
    float A[3];
    float B[3];
    float C[3];
    float D[3];
    float E[3]; //structure for coefficients
};

//data structure for CIE Yxy values
struct CIE_Yxy
{
    float Y, x, y;
};

//data structure for CIE XYZ values
struct CIE_XYZ
{
    float X, Y, Z;
};

//data structure for RGB values
struct RGB_value
{
    float R, G, B;
};

class perez
{

public:
    //function prototypes
    void generate_perez_coeff(float turbidity); //only needed once at initiation
    RGB_value calc_RGB_out(float theta_sun, float theta_pixel, float turbidity);

private:
    //define CIE to RGB scaling constants, set actual values in the body .cpp file
    static const float rx;
    static const float ry;
    static const float rz;
    static const float gx;
    static const float gy;
    static const float gz;
    static const float bx;
    static const float by;
    static const float bz;

    //function prototypes
    float angle_sun_pixel(float theta_sun, float theta_pixel);
    float calc_perez_lum(float theta, float gamma, int n);
    float calc_Yz(float turbidity, float theta_sun);
    float calc_xz(float turbidity, float theta_sun);
    float calc_yz(float turbidity, float theta_sun);
    void calc_XYZ(CIE_XYZ *temp_XYZ, float turbidity, float theta_sun, float theta_pix);
    void calc_RGB(RGB_value *temp_RGB, CIE_XYZ *temp_XYZ, float theta_sun);
};

#endif /* COLOURCALC_H_ */