// Bit width compression using a piecewise linear function
//
// Author: Casey Miller, Microsoft (millerc@microsoft.com)

#include <stdint.h>

// Uncomment the following line to use per-pixel divide
// (per-pixel divide uses a lot of FPGA resources and is normally avoided)
//#define USE_HW_DIVIDE (1)

////////////////////////////////////////////////////////////////////////////////
// piecewise linear (pwl) reduction of bitwidth
// function defined by a number "numXY" of control points (pX[ii], pY[ii])
// If      pIn[ii] <= pX[0]         then pOut[ii] = pY[0]
// else if pIn[ii] >= pX[numXY - 1] then pOut[ii] = pY[numXY - 1]
// else pOut[ii] = linear interpolation between closest 2 control points
//
// The "X" values represent input  control point values and
// the "Y" values represent output control point values
int pwl_reduce(
    uint32_t*       pIn,    //  in: input data array (uncompressed)
    uint32_t        numPix, //  in: number of pixels in input data array
    const uint32_t* pX,     //  in: array of x values for control points, pX[i] <  pX[i+1]
    const uint32_t* pY,     //  in: array of y values for control points, pY[i] <= pY[i+1]
    uint8_t         numXY,  //  in: number of elements in pX and pY arrays
    uint32_t*       pOut    // out: output data array (compressed)
);

////////////////////////////////////////////////////////////////////////////////
// Reverse compression done by pwl_reduce()
int pwl_expand(
    uint32_t*       pIn,    //  in: input data array (compressed)
    uint32_t        numPix, //  in: number of pixels in input data array
    const uint32_t* pX,     //  in: array of x-locations for control points, pX[i] <  pX[i+1]
    const uint32_t* pY,     //  in: array of y-locations for control points, pY[i] <= pY[i+1]
    uint8_t         numXY,  //  in: number of elements in pX and pY arrays
    uint32_t*       pOut    // out: output data array (uncompressed)
);
