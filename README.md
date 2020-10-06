# pwl_compress
Piecewise linear compression/decompression of pixel data

Piecewise linear (pwl) reduction of pixel bitwidth from uint32_t to uint16_t using
function defined by a number "numXY" of control points (pX[ii], pY[ii]):
```
If      pIn[ii] <= pX[0]         then pOut[ii] = pY[0]
else if pIn[ii] >= pX[numXY - 1] then pOut[ii] = pY[numXY - 1]
else pOut[ii] = linear interpolation between closest 2 control points
```
