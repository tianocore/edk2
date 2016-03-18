/** @file
  Floating-point Math functions and macros.

  Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunPro, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================

    NetBSD: math.h,v 1.44 2006/03/25 16:41:11 xtraeme Exp
    dlibm.h 5.1 93/09/24
**/
#ifndef _MATH_H_
#define _MATH_H_

#include  <sys/EfiCdefs.h>
#include  <sys/featuretest.h>

/** @{
    @brief    These are forward references to unions and macros used internaly
              by the implementation of the math functions and macros.
**/
union __float_u {
  unsigned char __dummy[sizeof(float)];
  float __val;
};

union __double_u {
  unsigned char __dummy[sizeof(double)];
  double __val;
};

union __long_double_u {
  unsigned char __dummy[sizeof(long double)];
  long double __val;
};

#include <machine/math.h>   /* may use __float_u, __double_u, or __long_double_u */

#ifdef __HAVE_LONG_DOUBLE
#define __fpmacro_unary_floating(__name, __arg0)      \
  /* LINTED */              \
  ((sizeof (__arg0) == sizeof (float))        \
  ? __ ## __name ## f (__arg0)        \
  : (sizeof (__arg0) == sizeof (double))        \
  ? __ ## __name ## d (__arg0)        \
  : __ ## __name ## l (__arg0))
#else
#define __fpmacro_unary_floating(__name, __arg0)      \
  /* LINTED */              \
  ((sizeof (__arg0) == sizeof (float))        \
  ? __ ## __name ## f (__arg0)        \
  : __ ## __name ## d (__arg0))
#endif /* __HAVE_LONG_DOUBLE */

extern const union __double_u       __infinity;
extern const union __float_u        __infinityf;
extern const union __long_double_u  __infinityl;

/* C99 7.12.3.1 int fpclassify(real-floating x) */
#define fpclassify(__x) __fpmacro_unary_floating(fpclassify, __x)

/* C99 7.12.3.3 int isinf(real-floating x) */
#ifdef __isinf
  #define isinf(__x)  __isinf(__x)
#else
  #define isinf(__x)  __fpmacro_unary_floating(isinf, __x)
#endif

/* C99 7.12.3.4 int isnan(real-floating x) */
#ifdef __isnan
  #define isnan(__x)  __isnan(__x)
#else
  #define isnan(__x)  __fpmacro_unary_floating(isnan, __x)
#endif
/*@)*/

/*#############################################################
 * ISO C95
 */

/**@{
    Double, float, and long double versions, respectively, of HUGE_VAL.
*/
#define HUGE_VAL  __infinity.__val
#define HUGE_VALF __infinityf.__val
#define HUGE_VALL __infinityl.__val
/*@)*/

__BEGIN_DECLS
/*
 * ANSI/POSIX
 */
/** Compute the principal value of the arc cosine of Arg.

    @param[in]    Arg   The value to compute the arc cosine of.

    @return   The computed value of the arc cosine of Arg in the interval [0,pi] radians.
              If Arg is not in the interval [-1,+1], errno is set to EDOM.
**/
double  acos(double Arg);

/** Compute the principal value of the arc sine of Arg.

    @param[in]    Arg   The value to compute the arc sine of.

    @return   The computed value of the arc sine of Arg in the interval [-pi/2,+pi/2] radians.
              If Arg is not in the interval [-1,+1], errno is set to EDOM.
**/
double  asin(double Arg);

/** Compute the principal value of the arc tangent of Arg.

    @param[in]    Arg   The value to compute the arc tangent of.

    @return   The computed value of the arc tangent of Arg in the interval [-pi/2,+pi/2] radians.
**/
double  atan(double Arg);

/** Compute the value of the arc tangent of (Num / Denom).
    The sign of both arguments is used to determine the quadrant of the return value.

    @param[in]    Num   The numerator of the value to compute the arc tangent of.
    @param[in]    Denom The denominator of the value to compute the arc tangent of.

    @return   The computed value of the arc tangent of (Num / Denom) in the interval [-pi,+pi] radians.
**/
double  atan2(double Num, double Denom);

/** Compute the value of the cosine of Arg, measured in radians.

    @param[in]    Arg   The value to compute the cosine of.

    @return   The computed value of the cosine of Arg.
**/
double  cos(double Arg);

/** Compute the value of the sine of Arg.

    @param[in]    Arg   The value to compute the sine of.

    @return   The computed value of the sine of Arg.
**/
double  sin(double Arg);

/** Compute the value of the tangent of Arg.

    @param[in]    Arg   The value to compute the tangent of.

    @return   The computed value of the tangent of Arg.
**/
double  tan(double Arg);


/** Compute the value of the hyperbolic cosine of Arg.

    @param[in]    Arg   The value to compute the hyperbolic cosine of.

    @return   The computed value of the hyperbolic cosine of Arg.
              If the magnitude of Arg is too large, errno is set to ERANGE.
**/
double  cosh(double Arg);

/** Compute the value of the hyperbolic sine of Arg.

    @param[in]    Arg   The value to compute the hyperbolic sine of.

    @return   The computed value of the hyperbolic sine of Arg.
              If the magnitude of Arg is too large, errno is set to ERANGE.
**/
double  sinh(double Arg);

/** Compute the value of the hyperbolic tangent of Arg.

    @param[in]    Arg   The value to compute the hyperbolic tangent of.

    @return   The computed value of the hyperbolic tangent of Arg.
**/
double  tanh(double Arg);


/** Compute the base-e exponential of Arg.

    @param[in]    Arg   The value to compute the base-e exponential of.

    @return   The computed value of e**Arg.
              If the magnitude of Arg is too large, errno is set to ERANGE.
**/
double  exp(double Arg);

/** Break a floating-point number into a normalized fraction and an integral power of 2.

    @param[in]    Value   The floating-point value to be broken down.
    @param[out]   Exp     A pointer to an integer object to receive the integral power of 2 exponent.

    @return   The frexp function returns a value R, such that Value == R**Exp.
              If Value is zero, both parts of the result are zero.
**/
double  frexp(double Value, int *Exp);

/** Multiply a floating-point number, Value, by an integral power of 2, Exp.

    @param[in]    Value   The floating-point value to be multiplied.
    @param[out]   Exp     The integral power of 2 to multiply Value by.

    @return   The ldexp function returns a value R, such that R = Value x 2**Exp.
              If a range error occurs, errno will be set to ERANGE.
**/
double  ldexp(double Value, int Exp);

/** Compute the natural logarithm of Arg.

    @param[in]    Arg   The value to compute the natural logarithm of.

    @return   The log function returns log base-e of Arg. If Arg is negative, errno is set to EDOM.
              Otherwise, errno will be set to ERANGE if a range error occurs.
**/
double  log(double Arg);

/** Compute the common (base-10) logarithm of Arg.

    @param[in]    Arg   The value to compute the common logarithm of.

    @return   The log10 function returns log base-10 of Arg. If Arg is negative, errno is set to EDOM.
              Otherwise, errno will be set to ERANGE if Arg is 0.
**/
double  log10(double Arg);

/** Compute the base-2 logarithm of Arg.

    @param[in]    Arg   The value to compute the base-2 logarithm of.

    @return   The log function returns log base-2 of Arg. If Arg is negative, errno is set to EDOM.
              Otherwise, errno will be set to ERANGE if Arg is 0.
**/
double  log2(double Arg);

/** Break Value into integral and fractional parts, each of which has the same type and sign
    as Value.  Store the integral part in the object pointed to by Integ and return the
    fractional part.

    @param[in]    Value   The value to compute the arc cosine of.
    @param[out]   Integ   Pointer to where the integral component is to be stored.

    @return   The fractional part of Value is returned directly while the integral part is
              returned in the location pointed to by Integ.
**/
double  modf(double Value, double *Integ);

/** Compute Value raised to the power Exp.

    @param[in]    Value   The value to be raised.
    @param[in]    Exp     The power Value is to be raised to.

    @return   The pow function returns Value**Exp.  If an error occurs, errno will be set as follows:
                - EDOM: Value is finite and negative and Exp is finite and not an integer.
                - EDOM: Both Value and Exp are zero.
                - EDOM: Value is zero and Exp is less than zero.
**/
double  pow(double Value, double Exp);

/** Compute the non-negative square root of Arg.

    @param[in]    Arg   The value to compute the square root of.

    @return   The square root of Arg.  If Arg is less than zero, errno is set to EDOM.
**/
double  sqrt(double Arg);


/** Compute the smallest integer value not less than Arg.

    @param[in]    Arg   The value to compute the ceiling of.

    @return   The ceiling of Arg expressed as a floating-point number.
**/
double  ceil(double Arg);

/** Compute the absolute value of Arg.

    @param[in]    Arg   The value to compute the absolute value of.

    @return   The absolute value of Arg.
**/
double  fabs(double Arg);

/** Compute the largest integer value not greater than Arg.

    @param[in]    Arg   The value to compute the floor of.

    @return   The largest integer value not greater than Arg, expressed as a floating-point number.
**/
double  floor(double);

/** Compute the floating-point remainder of A1 / A2.

    @param[in]    A1    The dividend.
    @param[in]    A2    The divisor.

    @return   The remainder of A1 / A2 with the same sign as A1.  If A2 is zero, the fmod function
              returns 0.
**/
double  fmod(double A1, double A2);


int finite(double);
double  expm1(double);

/**@{
    C99, Posix, or NetBSD functions that are not part of the C95 specification.
**/
/*
 * Functions callable from C, intended to support IEEE arithmetic.
 */
double  copysign(double, double);
double  scalbn(double, int);

/*
 * Library implementation
 */
int __fpclassifyf(float);
int __fpclassifyd(double);
int __isinff(float);
int __isinfd(double);
int __isnanf(float);
int __isnand(double);

#ifdef __HAVE_LONG_DOUBLE
  int __fpclassifyl(long double);
  int __isinfl(long double);
  int __isnanl(long double);
#endif  /* __HAVE_LONG_DOUBLE */
/*@}*/

__END_DECLS

/**@{
    Extensions provided by NetBSD but not required by the C95 standard.
**/
extern int signgam;

enum fdversion {fdlibm_ieee = -1, fdlibm_svid, fdlibm_xopen, fdlibm_posix};

#define _LIB_VERSION_TYPE enum fdversion
#define _LIB_VERSION _fdlib_version

/** If global variable _LIB_VERSION is not desirable, one may
 * change the following to be a constant by:
 *  #define _LIB_VERSION_TYPE const enum version
 * In that case, after one initializes the value _LIB_VERSION (see
 * s_lib_version.c) during compile time, it cannot be modified
 * in the middle of a program
 */
extern  _LIB_VERSION_TYPE  _LIB_VERSION;

#define _IEEE_  fdlibm_ieee
#define _SVID_  fdlibm_svid
#define _XOPEN_ fdlibm_xopen
#define _POSIX_ fdlibm_posix

#ifndef __cplusplus
struct exception {
  int type;
  char *name;
  double arg1;
  double arg2;
  double retval;
};
#endif

#define HUGE    MAXFLOAT

/** set X_TLOSS = pi*2**52 **/
#define X_TLOSS   1.41484755040568800000e+16

#define DOMAIN    1
#define SING      2
#define OVERFLOW  3
#define UNDERFLOW 4
#define TLOSS     5
#define PLOSS     6
/*@}*/

/* 7.12#4 INFINITY */
#ifdef __INFINITY
#define INFINITY  __INFINITY  /**< float constant which overflows */
#else
#define INFINITY  HUGE_VALF   /**< positive infinity */
#endif /* __INFINITY */

/* 7.12#5 NAN: a quiet NaN, if supported */
#ifdef __HAVE_NANF
extern const union __float_u __nanf;
#define NAN   __nanf.__val
#endif /* __HAVE_NANF */

/**@{
    C99 7.12#6 Number classification macros represent mutually exclusive kinds of floating-point
    values.
**/
#define FP_INFINITE   0x00
#define FP_NAN        0x01
#define FP_NORMAL     0x02
#define FP_SUBNORMAL  0x03
#define FP_ZERO       0x04
/* NetBSD extensions */
#define _FP_LOMD      0x80    /**< range for machine-specific classes */
#define _FP_HIMD      0xff
/*@)*/

/**@{
 * Constants ala XOPEN/SVID.
 */
#define M_E         2.7182818284590452354   /**< e */
#define M_LOG2E     1.4426950408889634074   /**< log 2e */
#define M_LOG10E    0.43429448190325182765  /**< log 10e */
#define M_LN2       0.69314718055994530942  /**< log e2 */
#define M_LN10      2.30258509299404568402  /**< log e10 */
#define M_PI        3.14159265358979323846  /**< pi */
#define M_PI_2      1.57079632679489661923  /**< pi/2 */
#define M_PI_4      0.78539816339744830962  /**< pi/4 */
#define M_1_PI      0.31830988618379067154  /**< 1/pi */
#define M_2_PI      0.63661977236758134308  /**< 2/pi */
#define M_2_SQRTPI  1.12837916709551257390  /**< 2/sqrt(pi) */
#define M_SQRT2     1.41421356237309504880  /**< sqrt(2) */
#define M_SQRT1_2   0.70710678118654752440  /**< 1/sqrt(2) */
#define MAXFLOAT  ((float)3.40282346638528860e+38)
/*@}*/

#endif /* _MATH_H_ */
