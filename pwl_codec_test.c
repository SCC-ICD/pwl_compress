// Test code for pwl_codec.c
//
// Developed using Windows subsystem for Linux (WSL 1) and Ubuntu 18.04.
// Compile via: gcc -o pwl_codec_test pwl_codec.c pwl_codec_test.c
// or use cmake (instructions at top of CMakeLists.txt)
//
// Author: Casey Miller, Microsoft (millerc@microsoft.com)
//   Date: 5/21/20

#include <math.h>    // fabsf()
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "pwl_codec.h"

////////////////////////////////////////////////////////////////////////////////
// Compute maximum absolute error and average error between two arrays.
// Returns non-zero on error.
void compute_err(
    const uint32_t* pIn1,       //  in: input data array
    const uint32_t* pIn2,       //  in: input data array
    uint32_t        numPix,     //  in: number of pixels in input data array
    uint32_t*       pMaxAbsErr, // out: max(abs(pIn1 - pIn2))
    float*          pAvgErr     // out:     avg(pIn1 - pIn2)
) {
    int32_t sum_err    = 0;
    uint32_t maxAbsErr = 0;
    const uint32_t* pEnd = pIn1 + numPix;
    while (pIn1 < pEnd) {  // loop thru elements of arrays
        int32_t err = (int)(*pIn1++) - (int)(*pIn2++);
        sum_err += err;

        uint32_t abs_err = (err < 0) ? -err : err;
        if (abs_err > maxAbsErr) { maxAbsErr = abs_err; }
    }
    *pMaxAbsErr = maxAbsErr;
    *pAvgErr    = (float)sum_err / numPix;
}

////////////////////////////////////////////////////////////////////////////////
void test_pwl_reduce(
    uint32_t*       pIn,        // in: input data to test with
    uint32_t        numPix,     // in: number of input pixels (size of pInput)
    const uint32_t* pX,         //  in: array of x-locations for control points:
                                //      pX[i] < pX[i+1],
    const uint32_t* pY,         //  in: array of y-locations for control points
    uint8_t         numXY,      //  in: number of elements in pX and pY arrays (<= 8)
    uint32_t*       pMaxAbsErr, // out: max(abs(pIn1 - pIn2))
    float*          pAvgErr     // out:     avg(pIn1 - pIn2)
) {
    // malloc temporary arrays
    uint32_t* pIn2 = (uint32_t*)malloc(numPix * sizeof(uint32_t));
    uint32_t* pOut = (uint32_t*)malloc(numPix * sizeof(uint32_t));
    assert(pIn2 != NULL);
    assert(pOut != NULL);

    // compress pIn into pOut
    assert(pwl_reduce(pIn, numPix, pX, pY, numXY, pOut) == 0);

    // decompress pOut into pIn2
    assert(pwl_expand(pOut, numPix, pX, pY, numXY, pIn2) == 0);

    // compute max and average error from input
    compute_err(pIn, pIn2, numPix, pMaxAbsErr, pAvgErr);
    free(pIn2);
    free(pOut);
}

////////////////////////////////////////////////////////////////////////////////
// Test lossy linear compression with 2 control points
//             out
//              ^
//              |
//    Y1=maxOut +            +------------>
//              |          .
//              |        .
//              |      .
//              |    .
//              |  .
//              |.
//    Y0=0      +------------+------------> in
//            X0=0           X1=maxIn
#define MAX_BPP_IN (16) // maximum input bits per pixel

void test_linear(void) {
    uint32_t pX[2];
    uint32_t pY[2];
    uint32_t* pIn = (uint32_t*)malloc((1 << MAX_BPP_IN) * sizeof(uint32_t));

    // test all possible input and output bit widths using 2 control points
    uint8_t  numXY = 2;
    for (uint8_t bppIn = 9; bppIn <= MAX_BPP_IN; ++bppIn) { // widths to compress to >= 8
        for (uint8_t bppOut = 8; bppOut <= bppIn; ++bppOut) {

            // setup control points
            uint32_t maxIn  = (1 << bppIn ) - 1;
            uint32_t maxOut = (1 << bppOut) - 1;
            pX[        0] =     0; pY[        0] =      0;
            pX[numXY - 1] = maxIn; pY[numXY - 1] = maxOut;

            // create all possible input values
            uint32_t numPix = 1 << bppIn;
            for (uint32_t ii = 0; ii < numPix; ++ii) { pIn[ii] = ii; }

            // compress input to output, then decompress and compute error stats
            uint32_t maxAbsErr;
            float    avgErr;
            test_pwl_reduce(pIn, numPix, pX, pY, numXY, &maxAbsErr, &avgErr);

            // determine maximum absolute error bound
            uint32_t maxAbsErrBound = (bppIn == bppOut) ? 0 :
                                      (1 << (bppIn - bppOut - 1));
#ifdef USE_HW_DIVIDE
            assert (maxAbsErr <= maxAbsErrBound);
            assert (avgErr == 0);
#else
            // approximation of divide using fixed-point multiplication
            // incurs slight additional error
            assert (maxAbsErr <= (maxAbsErrBound + 1));
            assert (fabsf(avgErr) < 0.51f);
#endif
        }
    }
    free(pIn);
    fprintf(stderr, "%s: PASSED\n", __func__);
}

////////////////////////////////////////////////////////////////////////////////
// Test "windowed" compression as done with 16-bit thermal data
//             out
//              ^
//              |
//    Y1=maxOut +                    +------------>
//              |                  .
//              |                .
//              |              .
//              |            .
//              |          .
//              |        .
//    Y0=0      +-------+------------+------------> in
//                      X0           X1
//                   start_in       stop_in
void test_windowed(void) {
    uint8_t  bppOut = 12;
    uint32_t maxOut = (1 << bppOut) - 1;

    // create input data that can be lossless compressed as it is within
    // the dynamic range of the output bit width
    uint32_t numPix = 1 << bppOut;
    uint32_t start_in = 2000; // arbitrary non-zero start in 16-bit range
    uint32_t stop_in  = start_in + numPix - 1;
    uint32_t* pIn = (uint32_t*)malloc(numPix * sizeof(uint32_t));
    assert(pIn != NULL);
    for (uint32_t ii = 0; ii < numPix; ++ii) { pIn[ii] = start_in + ii; }

    // setup control points for windowed compression
    uint8_t  numXY = 2;
    uint32_t pX[2];
    uint32_t pY[2];
    pX[0] = start_in; pY[0] =      0;
    pX[1] = stop_in;  pY[1] = maxOut;

    // compress input to output, then decompress and compute error stats
    uint32_t maxAbsErr;
    float    avgErr;
    test_pwl_reduce(pIn, numPix, pX, pY, numXY, &maxAbsErr, &avgErr);
    assert (maxAbsErr == 0);
    assert (avgErr == 0);
    free(pIn);
    fprintf(stderr, "%s: PASSED\n", __func__);
}

////////////////////////////////////////////////////////////////////////////////
int main(void) {
    test_linear();
    test_windowed();
    return 0;
}
