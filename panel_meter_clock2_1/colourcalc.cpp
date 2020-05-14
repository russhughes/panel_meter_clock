/*
 * colourcalc.cpp
 *
 * Created: 19/10/2016 15:06:16
 *  Author: David Brown
 * Shared under the Creative Commons - Attribution - ShareAlike 3.0 license.
 */

#include <math.h>
#include "colourcalc.h"

/*this calculates the colour and intensity of the sky NOT the sun
so there needs to be a function to override some pixels for when the
sun comes over the horizon*/

//set values of CIE to RGB scaling constants
const float perez::rx = 2.28783849;
const float perez::ry = -0.83336768;
const float perez::rz = -0.4544708;
const float perez::gx = -0.51165138;
const float perez::gy = 1.42275838;
const float perez::gz = 0.08889300;
const float perez::bx = 0.00572041;
const float perez::by = -0.01590685;
const float perez::bz = 1.01018641;

perez_coefficient coeff; //single instance of coefficients

//function returns member of class perez_Yxy_coefficients for a given turbidity
void perez::generate_perez_coeff(float turbidity)
{

    coeff.A[0] = .17872f * turbidity - 1.46303f;
    coeff.B[0] = -.3554f * turbidity + .42749f;
    coeff.C[0] = -.02266f * turbidity + 5.32505f;
    coeff.D[0] = .12064f * turbidity - 2.57705f;
    coeff.E[0] = -.06696f * turbidity + .37027f;

    coeff.A[1] = -.01925f * turbidity - .25922f;
    coeff.B[1] = -.06651f * turbidity + .00081f;
    coeff.C[1] = -.00041f * turbidity + .21247f;
    coeff.D[1] = -.06409f * turbidity - .89887f;
    coeff.E[1] = -.00325f * turbidity + .04517f;

    coeff.A[2] = -.01669f * turbidity - .26078f;
    coeff.B[2] = -.09495f * turbidity + .00921f;
    coeff.C[2] = -.00792f * turbidity + .21023f;
    coeff.D[2] = -.04405f * turbidity - 1.65369f;
    coeff.E[2] = -.01092f * turbidity + .05291f;
}

//angle between sun and pixel
float perez::angle_sun_pixel(float theta_sun, float theta_pixel)
{
    float gamma;

    gamma = acos(sin(theta_sun) * sin(theta_pixel) + cos(theta_sun) * cos(theta_pixel));

    return gamma;
}

//calculate Perez luminosity
float perez::calc_perez_lum(float theta, float gamma, int n)
{
    theta = cos(theta);
    float lum;

    lum = (1 + coeff.A[n] * exp(coeff.B[n] / theta)) * (1 + coeff.C[n] * exp(coeff.D[n] * gamma) + coeff.E[n] * cos(gamma) * cos(gamma));

    return lum;
}

//calculate Perez colour
float perez::calc_Yz(float turbidity, float theta_sun)
{
    float Yz;

    Yz = ((4.0453 * turbidity - 4.9710) * tan((4 / 9 - turbidity / 120) * (M_PI - 2 * theta_sun)) - 0.2155 * turbidity + 2.4192) /
         ((4.0453 * turbidity - 4.9710) * tan((4 / 9 - turbidity / 120) * M_PI) - 0.2155 * turbidity + 2.4192);

    return Yz;
}

float perez::calc_xz(float turbidity, float theta_sun)
{
    float xz;
    float theta_sun2 = pow(theta_sun, 2);
    float theta_sun3 = pow(theta_sun, 3);

    xz = (0.00166 * theta_sun3 - 0.00375 * theta_sun2 + 0.00209 * theta_sun) * pow(turbidity, 2) +
         (-0.02903 * theta_sun3 + 0.06377 * theta_sun2 - 0.03202 * theta_sun + 0.00394) * turbidity +
         (0.11693 * theta_sun3 - 0.21196 * theta_sun2 + 0.06052 * theta_sun + 0.25886);

    return xz;
}

float perez::calc_yz(float turbidity, float theta_sun)
{
    float yz;
    float theta_sun2 = pow(theta_sun, 2);
    float theta_sun3 = pow(theta_sun, 3);

    yz = (0.00275 * theta_sun3 - 0.00610 * theta_sun2 + 0.00317 * theta_sun) * pow(turbidity, 2) +
         (-0.04214 * theta_sun3 + 0.08970 * theta_sun2 - 0.04153 * theta_sun + 0.00516) * turbidity +
         (0.15346 * theta_sun3 - 0.26756 * theta_sun2 + 0.06670 * theta_sun + 0.26688);

    return yz;
}

//calc CIE XYZ values
void perez::calc_XYZ(CIE_XYZ *temp_XYZ, float turbidity, float theta_sun, float theta_pix)
{

    CIE_Yxy temp_Yxy; //generate instance

    //calculate colour coefficients
    float Yz = calc_Yz(turbidity, theta_sun);
    float xz = calc_xz(turbidity, theta_sun);
    float yz = calc_yz(turbidity, theta_sun);

    //calculate CIE Yxy values
    temp_Yxy.Y = Yz * calc_perez_lum(theta_pix, theta_sun, 0) / calc_perez_lum(0, theta_sun, 0);
    temp_Yxy.x = xz * calc_perez_lum(theta_pix, theta_sun, 1) / calc_perez_lum(0, theta_sun, 1);
    temp_Yxy.y = yz * calc_perez_lum(theta_pix, theta_sun, 2) / calc_perez_lum(0, theta_sun, 2);

    //calculate CIE XYZ values
    temp_XYZ->X = (temp_Yxy.x / temp_Yxy.y) * temp_Yxy.Y;
    temp_XYZ->Y = temp_Yxy.Y;
    temp_XYZ->Z = (1 - temp_Yxy.x - temp_Yxy.y) / (temp_Yxy.y * temp_Yxy.Y);
}

//calc RGB colour values and scale
void perez::calc_RGB(RGB_value *temp_RGB, CIE_XYZ *temp_XYZ, float theta_sun)
{
    //convert CIE XYZ to RGB
    temp_RGB->R = (rx * temp_XYZ->X) + (ry * temp_XYZ->Y) + (rz * temp_XYZ->Z);
    temp_RGB->G = (gx * temp_XYZ->X) + (gy * temp_XYZ->Y) + (gz * temp_XYZ->Z);
    temp_RGB->B = (bx * temp_XYZ->X) + (by * temp_XYZ->Y) + (bz * temp_XYZ->Z);

    //normalise RGB
    float divisor = fmax(1, fmax(temp_RGB->R, fmax(temp_RGB->G, temp_RGB->B)));
    temp_RGB->R = temp_RGB->R / divisor;
    temp_RGB->G = temp_RGB->G / divisor;
    temp_RGB->B = temp_RGB->B / divisor;
}

RGB_value perez::calc_RGB_out(float theta_sun, float theta_pixel, float turbidity)
{
    CIE_XYZ temp_XYZ;
    RGB_value temp_RGB;

    calc_XYZ(&temp_XYZ, turbidity, theta_sun, theta_pixel);
    calc_RGB(&temp_RGB, &temp_XYZ, theta_sun);

    return temp_RGB; //scaling and gamma correction done before sending to the LEDs
}
