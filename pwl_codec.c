// Bit width compression using a piecewise linear function
//
// Developed using Windows subsystem for Linux (WSL 1) and Ubuntu 18.04.
// Compile via: gcc -o pwl_codec_test pwl_codec.c pwl_codec_test.c
//
// Author: Casey Miller, Microsoft (millerc@microsoft.com)
//   Date: 5/21/20
#include <assert.h>

#include "pwl_codec.h"

// define macro for "round(N / D)" where N and D are non-negative
#define DIVIDE_AND_ROUND(N, D) ((N) + ((D) / 2)) / (D)

#ifndef USE_HW_DIVIDE
#   define GAIN_FBITS (16)   // fractional bits in gain value (<= 16 for uint32_t math)
#   define GAIN_ROUND (1 << (GAIN_FBITS - 1))
#   define MAX_NUM_XY (4)    // most number of control points ever expected
#endif

////////////////////////////////////////////////////////////////////////////////
// piecewise linear (pwl) reduction of bitwidth from uint32_t to uint16_t using
// function defined by a number "numXY" of control points (pX[ii], pY[ii])
// If      pIn[ii] <= pX[0]         then pOut[ii] = pY[0]
// else if pIn[ii] >= pX[numXY - 1] then pOut[ii] = pY[numXY - 1]
// else pOut[ii] = linear interpolation between closest 2 control points
//
// The "X" values represent input  control point values and
// the "Y" values represent output control point values
int pwl_reduce(
    const uint32_t* pIn,    //  in: input data array
    uint32_t        numPix, //  in: number of pixels in input data array
    const uint32_t* pX,     //  in: array of x values for control points, pX[i] <  pX[i+1]
    const uint16_t* pY,     //  in: array of y values for control points, pY[i] <= pY[i+1]
    uint8_t         numXY,  //  in: number of elements in pX and pY arrays
    uint16_t*       pOut    // out: output data array (compressed)
) {
#ifndef USE_HW_DIVIDE
    // precompute fixed-point gain array to avoid division
    // Note: in the FPGA this is done between frames using vertical blank time
    // Note: this only needs to be done if pX or pY values have changed.
    assert(numXY <= MAX_NUM_XY);
    uint32_t pGain[MAX_NUM_XY - 1];
    for (uint32_t jj = 0; jj < (numXY - 1); ++jj) {
        uint32_t XL = pX[jj];
        uint32_t YL = pY[jj];
        uint32_t XR = pX[jj + 1];
        uint32_t YR = pY[jj + 1];
        assert(XL <  XR);
        assert(YL <= YR);
        pGain[jj] = DIVIDE_AND_ROUND((YR - YL) << GAIN_FBITS, XR - XL);
    }
#endif
    // loop thru pixels
    for (uint32_t ii = 0; ii < numPix; ++ii) {
        uint32_t input = *pIn++;
        uint32_t output;

        // Determine output value
        if      (input <= pX[0])         { output = pY[0]; }
        else if (input >= pX[numXY - 1]) { output = pY[numXY - 1]; }
        else {                          // else linear interpolation
            // find largest jj such that pX[jj] <= input
            for (int jj = numXY - 2; jj >= 0; --jj) {
                if (pX[jj] <= input) {
                    uint32_t XL = pX[jj];
                    uint16_t YL = pY[jj];
                    uint32_t XR = pX[jj + 1];
                    uint16_t YR = pY[jj + 1];

                    // linear interpolation between (XL, YL) and (XR, YR)
#ifdef USE_HW_DIVIDE
                    output = YL + DIVIDE_AND_ROUND((input - XL) * (YR - YL), (XR - XL));
#else
                    output = YL + (((input - XL) * pGain[jj] + GAIN_ROUND) >> GAIN_FBITS);
#endif
                    assert((XL <= input ) && (input  <  XR));
                    assert((YL <= output) && (output <= YR));
                    break;
                }
            }
        }
        *pOut++ = output; // set output and advance to next pixel
    }
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Reverse compression done by pwl_reduce()
int pwl_expand(
    uint32_t*       pIn,    // out: input data array (uncompressed)
    uint32_t        numPix, //  in: number of pixels in input data array
    const uint32_t* pX,     //  in: array of x-locations for control points, pX[i] <  pX[i+1]
    const uint16_t* pY,     //  in: array of y-locations for control points, pY[i] <= pY[i+1]
    uint8_t         numXY,  //  in: number of elements in pX and pY arrays
    const uint16_t* pOut    //  in: output data array (compressed)
) {
    // loop thru pixels
    for (uint32_t ii = 0; ii < numPix; ++ii) {
        uint32_t input;
        uint32_t output = *pOut++;

        if      (output <= pY[0])         { input = pX[0]; }
        else if (output >= pY[numXY - 1]) { input = pX[numXY - 1]; }
        else {                              // else no fixed point needed
            // find largest jj such that pY[jj] <= output
            for (int jj = numXY - 2; jj >= 0; --jj) {
                if (pY[jj] <= output) {
                    uint32_t XL = pX[jj];
                    uint16_t YL = pY[jj];
                    uint32_t XR = pX[jj + 1];
                    uint16_t YR = pY[jj + 1];

                    // linear interpolation between (YL, XL) and (YR, XR)
                    // Note: per-pixel divide assumed OK here since this is done by the
                    // GPU or SOC ARM and not the sensor FPGA.
                    input = (YR == YL) ? XR :
                            (XL + DIVIDE_AND_ROUND((output - YL) * (XR - XL), (YR - YL)));
                    assert((XL <= input ) && (input  <  XR));
                    assert((YL <= output) && (output <= YR));
                    break;
                }
            }
        }
        *pIn++ = input; // set input and advance to next pixel
    }
    return 0;
}
