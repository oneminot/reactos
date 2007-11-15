/*
 * complex.h
 *
 * This file is part of the Mingw32 package.
 *
 * Contributors:
 *  Created by Danny Smith <dannysmith@users.sourceforge.net>
 *
 *  THIS SOFTWARE IS NOT COPYRIGHTED
 *
 *  This source code is offered for use in the public domain. You may
 *  use, modify or distribute it freely.
 *
 *  This code is distributed in the hope that it will be useful but
 *  WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 *  DISCLAIMED. This includes but is not limited to warranties of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#ifndef _COMPLEX_H_
#define _COMPLEX_H_

/* All the headers include this file. */
#include <_mingw.h>

#if (defined (__STDC_VERSION__) && __STDC_VERSION__ >= 199901L) \
     || !defined __STRICT_ANSI__ 

/* These macros are specified by C99 standard */

#ifndef __cplusplus
#define complex _Complex
#endif

#define _Complex_I  (0.0F +  1.0iF)

/* GCC doesn't support _Imaginary type yet, so we don't
   define _Imaginary_I */

#define I _Complex_I

#ifdef __cplusplus
extern "C" {
#endif 

#ifndef RC_INVOKED

/* TODO: long double versions */ 
double __MINGW_ATTRIB_CONST creal (double _Complex);
double __MINGW_ATTRIB_CONST cimag (double _Complex);
double __MINGW_ATTRIB_CONST carg (double _Complex);
double __MINGW_ATTRIB_CONST cabs (double _Complex);
double _Complex __MINGW_ATTRIB_CONST conj (double _Complex);
double _Complex  cacos (double _Complex);
double _Complex  casin (double _Complex);
double _Complex  catan (double _Complex);
double _Complex  ccos (double _Complex);
double _Complex  csin (double _Complex);
double _Complex  ctan (double _Complex);
double _Complex  cacosh (double _Complex);
double _Complex  casinh (double _Complex);
double _Complex  catanh (double _Complex);
double _Complex  ccosh (double _Complex);
double _Complex  csinh (double _Complex);
double _Complex  ctanh (double _Complex);
double _Complex  cexp (double _Complex);
double _Complex  clog (double _Complex);
double _Complex  cpow (double _Complex, double _Complex);
double _Complex  csqrt (double _Complex);
double _Complex __MINGW_ATTRIB_CONST cproj (double _Complex);

float __MINGW_ATTRIB_CONST crealf (float _Complex);
float __MINGW_ATTRIB_CONST cimagf (float _Complex);
float __MINGW_ATTRIB_CONST cargf (float _Complex);
float __MINGW_ATTRIB_CONST cabsf (float _Complex);
float _Complex __MINGW_ATTRIB_CONST conjf (float _Complex);
float _Complex  cacosf (float _Complex);
float _Complex  casinf (float _Complex);
float _Complex  catanf (float _Complex);
float _Complex  ccosf (float _Complex);
float _Complex  csinf (float _Complex);
float _Complex  ctanf (float _Complex);
float _Complex  cacoshf (float _Complex);
float _Complex  casinhf (float _Complex);
float _Complex  catanhf (float _Complex);
float _Complex  ccoshf (float _Complex);
float _Complex  csinhf (float _Complex);
float _Complex  ctanhf (float _Complex);
float _Complex  cexpf (float _Complex);
float _Complex  clogf (float _Complex);
float _Complex  cpowf (float _Complex, float _Complex);
float _Complex  csqrtf (float _Complex);
float _Complex __MINGW_ATTRIB_CONST cprojf (float _Complex);

#ifdef __GNUC__

__CRT_INLINE double __MINGW_ATTRIB_CONST creal (double _Complex _Z)
{
  return __real__ _Z;
}

__CRT_INLINE double __MINGW_ATTRIB_CONST cimag (double _Complex _Z)
{
  return __imag__ _Z;
}

__CRT_INLINE double _Complex __MINGW_ATTRIB_CONST conj (double _Complex _Z)
{
  return __extension__ ~_Z;
}

__CRT_INLINE  double __MINGW_ATTRIB_CONST carg (double _Complex _Z)
{
  double res;
#ifdef _M_IX86
  __asm__  ("fpatan;"
	   : "=t" (res) : "0" (__real__ _Z), "u" (__imag__ _Z) : "st(1)");
#endif
  return res;
}


__CRT_INLINE float __MINGW_ATTRIB_CONST crealf (float _Complex _Z)
{
  return __real__ _Z;
}

__CRT_INLINE float __MINGW_ATTRIB_CONST cimagf (float _Complex _Z)
{
  return __imag__ _Z;
}

__CRT_INLINE float _Complex __MINGW_ATTRIB_CONST conjf (float _Complex _Z)
{
  return __extension__ ~_Z;
}

__CRT_INLINE  float __MINGW_ATTRIB_CONST cargf (float _Complex _Z)
{
  float res;
#ifdef _M_IX86
  __asm__  ("fpatan;"
	   : "=t" (res) : "0" (__real__ _Z), "u" (__imag__ _Z) : "st(1)");
#endif
  return res;
}
#endif /* __GNUC__ */


#endif /* RC_INVOKED */

#ifdef __cplusplus
}
#endif 

#endif /* __STDC_VERSION__ >= 199901L */


#endif /* _COMPLEX_H */
