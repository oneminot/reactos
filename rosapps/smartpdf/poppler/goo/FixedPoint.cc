//========================================================================
//
// FixedPoint.cc
//
// Fixed point type, with C++ operators.
//
// Copyright 2004 Glyph & Cog, LLC
//
//========================================================================

#include <config.h>

#if USE_FIXEDPOINT

#ifdef USE_GCC_PRAGMAS
#pragma implementation
#endif

#include "FixedPoint.h"

FixedPoint FixedPoint::sqrt(FixedPoint x) {
  FixedPoint y0, y1, z;

  if (x.val <= 0) {
    y1.val = 0;
  } else {
    y1.val = x.val >> 1;
    do {
      y0.val = y1.val;
      z = x / y0;
      y1.val = (y0.val + z.val) >> 1;
    } while (::abs(y0.val - y1.val) > 1);
  }
  return y1;
}

//~ this is not very accurate
FixedPoint FixedPoint::pow(FixedPoint x, FixedPoint y) {
  FixedPoint t, t2, lnx0, lnx, z0, z;
  int d, i;

  if (y.val <= 0) {
    z.val = 0;
  } else {
    // y * ln(x)
    t = (x - 1) / (x + 1);
    t2 = t * t;
    d = 1;
    lnx = 0;
    do {
      lnx0 = lnx;
      lnx += t / d;
      t *= t2;
      d += 2;
    } while (::abs(lnx.val - lnx0.val) > 2);
    lnx.val <<= 1;
    t = y * lnx;
    // exp(y * ln(x))
    t2 = t;
    d = 1;
    i = 1;
    z = 1;
    do {
      z0 = z;
      z += t2 / d;
      t2 *= t;
      ++i;
      d *= i;
    } while (::abs(z.val - z0.val) > 2 && d < (1 << fixptShift));
  }
  return z;
}

int FixedPoint::mul(int x, int y) {
#if 1 //~tmp
  return ((FixPtInt64)x * y) >> fixptShift;
#else
  int ah0, ah, bh, al, bl;
  ah0 = x & fixptMaskH;
  ah = x >> fixptShift;
  al = x - ah0;
  bh = y >> fixptShift;
  bl = y - (bh << fixptShift);
  return ah0 * bh + ah * bl + al * bh + ((al * bl) >> fixptShift);
#endif
}

int FixedPoint::div(int x, int y) {
#if 1 //~tmp
  return ((FixPtInt64)x << fixptShift) / y;
#else
#endif
}

#endif // USE_FIXEDPOINT
