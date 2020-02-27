/*
 * Copyright (c) 2015 - 2019, Linaro Limited
 *
 * SPDX-License-Identifier: BSD-2-Clause-Patent
 */

#include "platform.h"
#include <softfloat.h>

/*
 * On ARM32 EABI defines both a soft-float ABI and a hard-float ABI,
 * hard-float is basically a super set of soft-float. Hard-float requires
 * all the support routines provided for soft-float, but the compiler may
 * choose to optimize to not use some of them.
 *
 * The AEABI functions uses soft-float calling convention even if the
 * functions are compiled for hard-float. So where float and double would
 * have been expected we use aeabi_float_t and aeabi_double_t respectively
 * instead.
 */
typedef uint32_t aeabi_float_t;
typedef uint64_t aeabi_double_t;

/*
 * Helpers to convert between float32 and aeabi_float_t, and float64 and
 * aeabi_double_t used by the AEABI functions below.
 */
static aeabi_float_t f32_to_f(float32_t val)
{
  return val.v;
}

static float32_t f32_from_f(aeabi_float_t val)
{
  float32_t res;

  res.v = val;

  return res;
}

static aeabi_double_t f64_to_d(float64_t val)
{
  return val.v;
}

static float64_t f64_from_d(aeabi_double_t val)
{
  float64_t res;

  res.v = val;

  return res;
}

/*
 * From ARM Run-time ABI for ARM Architecture
 * ARM IHI 0043D, current through ABI release 2.09
 *
 * 4.1.2 The floating-point helper functions
 */

/*
 * Table 2, Standard aeabi_double_t precision floating-point arithmetic helper
 * functions
 */

aeabi_double_t __aeabi_dadd(aeabi_double_t a, aeabi_double_t b)
{
  return f64_to_d(f64_add(f64_from_d(a), f64_from_d(b)));
}

aeabi_double_t __aeabi_ddiv(aeabi_double_t a, aeabi_double_t b)
{
  return f64_to_d(f64_div(f64_from_d(a), f64_from_d(b)));
}

aeabi_double_t __aeabi_dmul(aeabi_double_t a, aeabi_double_t b)
{
  return f64_to_d(f64_mul(f64_from_d(a), f64_from_d(b)));
}


aeabi_double_t __aeabi_drsub(aeabi_double_t a, aeabi_double_t b)
{
  return f64_to_d(f64_sub(f64_from_d(b), f64_from_d(a)));
}

aeabi_double_t __aeabi_dsub(aeabi_double_t a, aeabi_double_t b)
{
  return f64_to_d(f64_sub(f64_from_d(a), f64_from_d(b)));
}

/*
 * Table 3, double precision floating-point comparison helper functions
 */

int __aeabi_dcmpeq(aeabi_double_t a, aeabi_double_t b)
{
  return f64_eq(f64_from_d(a), f64_from_d(b));
}

int __aeabi_dcmplt(aeabi_double_t a, aeabi_double_t b)
{
  return f64_lt(f64_from_d(a), f64_from_d(b));
}

int __aeabi_dcmple(aeabi_double_t a, aeabi_double_t b)
{
  return f64_le(f64_from_d(a), f64_from_d(b));
}

int __aeabi_dcmpge(aeabi_double_t a, aeabi_double_t b)
{
  return f64_le(f64_from_d(b), f64_from_d(a));
}

int __aeabi_dcmpgt(aeabi_double_t a, aeabi_double_t b)
{
  return f64_lt(f64_from_d(b), f64_from_d(a));
}

/*
 * Table 4, Standard single precision floating-point arithmetic helper
 * functions
 */

aeabi_float_t __aeabi_fadd(aeabi_float_t a, aeabi_float_t b)
{
  return f32_to_f(f32_add(f32_from_f(a), f32_from_f(b)));
}

aeabi_float_t __aeabi_fdiv(aeabi_float_t a, aeabi_float_t b)
{
  return f32_to_f(f32_div(f32_from_f(a), f32_from_f(b)));
}

aeabi_float_t __aeabi_fmul(aeabi_float_t a, aeabi_float_t b)
{
  return f32_to_f(f32_mul(f32_from_f(a), f32_from_f(b)));
}

aeabi_float_t __aeabi_frsub(aeabi_float_t a, aeabi_float_t b)
{
  return f32_to_f(f32_sub(f32_from_f(b), f32_from_f(a)));
}

aeabi_float_t __aeabi_fsub(aeabi_float_t a, aeabi_float_t b)
{
  return f32_to_f(f32_sub(f32_from_f(a), f32_from_f(b)));
}

/*
 * Table 5, Standard single precision floating-point comparison helper
 * functions
 */

int __aeabi_fcmpeq(aeabi_float_t a, aeabi_float_t b)
{
  return f32_eq(f32_from_f(a), f32_from_f(b));
}

int __aeabi_fcmplt(aeabi_float_t a, aeabi_float_t b)
{
  return f32_lt(f32_from_f(a), f32_from_f(b));
}

int __aeabi_fcmple(aeabi_float_t a, aeabi_float_t b)
{
  return f32_le(f32_from_f(a), f32_from_f(b));
}

int __aeabi_fcmpge(aeabi_float_t a, aeabi_float_t b)
{
  return f32_le(f32_from_f(b), f32_from_f(a));
}

int __aeabi_fcmpgt(aeabi_float_t a, aeabi_float_t b)
{
  return f32_lt(f32_from_f(b), f32_from_f(a));
}

/*
 * Table 6, Standard floating-point to integer conversions
 */

int __aeabi_d2iz(aeabi_double_t a)
{
  return f64_to_i32_r_minMag(f64_from_d(a), false);
}

unsigned __aeabi_d2uiz(aeabi_double_t a)
{
  return f64_to_ui32_r_minMag(f64_from_d(a), false);
}

long long __aeabi_d2lz(aeabi_double_t a)
{
  return f64_to_i64_r_minMag(f64_from_d(a), false);
}

unsigned long long __aeabi_d2ulz(aeabi_double_t a)
{
  return f64_to_ui64_r_minMag(f64_from_d(a), false);
}

int __aeabi_f2iz(aeabi_float_t a)
{
  return f32_to_i32_r_minMag(f32_from_f(a), false);
}

unsigned __aeabi_f2uiz(aeabi_float_t a)
{
  return f32_to_ui32_r_minMag(f32_from_f(a), false);
}

long long __aeabi_f2lz(aeabi_float_t a)
{
  return f32_to_i64_r_minMag(f32_from_f(a), false);
}

unsigned long long __aeabi_f2ulz(aeabi_float_t a)
{
  return f32_to_ui64_r_minMag(f32_from_f(a), false);
}

/*
 * Table 7, Standard conversions between floating types
 */

aeabi_float_t __aeabi_d2f(aeabi_double_t a)
{
  return f32_to_f(f64_to_f32(f64_from_d(a)));
}

aeabi_double_t __aeabi_f2d(aeabi_float_t a)
{
  return f64_to_d(f32_to_f64(f32_from_f(a)));
}

/*
 * Table 8, Standard integer to floating-point conversions
 */

aeabi_double_t __aeabi_i2d(int a)
{
  return f64_to_d(i32_to_f64(a));
}

aeabi_double_t __aeabi_ui2d(unsigned a)
{
  return f64_to_d(ui32_to_f64(a));
}

aeabi_double_t __aeabi_l2d(long long a)
{
  return f64_to_d(i64_to_f64(a));
}

aeabi_double_t __aeabi_ul2d(unsigned long long a)
{
  return f64_to_d(ui64_to_f64(a));
}

aeabi_float_t __aeabi_i2f(int a)
{
  return f32_to_f(i32_to_f32(a));
}

aeabi_float_t __aeabi_ui2f(unsigned a)
{
  return f32_to_f(ui32_to_f32(a));
}

aeabi_float_t __aeabi_l2f(long long a)
{
  return f32_to_f(i64_to_f32(a));
}

aeabi_float_t __aeabi_ul2f(unsigned long long a)
{
  return f32_to_f(ui64_to_f32(a));
}
