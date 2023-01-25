# pwl_compress
Piecewise linear (pwl) reduction of pixel bitwidth using
function defined by a number "numXY" of control points (pX[ii], pY[ii]):

```
If      pIn[ii] <= pX[0]         then pOut[ii] = pY[0]
else if pIn[ii] >= pX[numXY - 1] then pOut[ii] = pY[numXY - 1]
else pOut[ii] = linear interpolation between closest 2 control points
```

This is intended as a model for use in aiding FPGA design.
