// Bit width compression using a piecewise linear function
//
// Developed using Windows subsystem for Linux (WSL 1) and Ubuntu 18.04.
//
// Author: Casey Miller, Microsoft (millerc@microsoft.com)
//   Date: 5/21/20

#include <stdint.h>

// Uncomment the following line to use per-pixel divide
// (per-pixel divide uses alot of FPGA resources and is normally avoided)
//#define USE_HW_DIVIDE (1)

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
);

////////////////////////////////////////////////////////////////////////////////
// Reverse compression done by pwl_reduce()
int pwl_expand(
    uint32_t*       pIn,    // out: input data array (uncompressed)
    uint32_t        numPix, //  in: number of pixels in input data array
    const uint32_t* pX,     //  in: array of x-locations for control points, pX[i] <  pX[i+1]
    const uint16_t* pY,     //  in: array of y-locations for control points, pY[i] <= pY[i+1]
    uint8_t         numXY,  //  in: number of elements in pX and pY arrays
    const uint16_t* pOut    //  in: output data array (compressed)
);
