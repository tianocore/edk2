/** @file
Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
See https://llvm.org/LICENSE.txt for license information.
SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

Portions of this file have been modified from the original (https://github.com/llvm/llvm-project/blob/main/compiler-rt/lib/builtins/udivmodti4.c)
under the following copyright and license.

Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>

typedef int si_int;
typedef unsigned su_int;

typedef          long long di_int;
typedef unsigned long long du_int;

typedef int ti_int;
typedef unsigned tu_int;

#ifndef CHAR_BIT
#define CHAR_BIT 8
#endif

typedef union
{
    tu_int all;
    struct
    {
#if _YUGA_LITTLE_ENDIAN
        du_int low;
        du_int high;
#else
        du_int high;
        du_int low;
#endif /* _YUGA_LITTLE_ENDIAN */
    }s;
} utwords;

static inline du_int udiv128by64to64default(du_int u1, du_int u0, du_int v,
                                            du_int *r) {
    const unsigned n_udword_bits = sizeof(du_int) * CHAR_BIT;
    const du_int b = (1ULL << (n_udword_bits / 2)); /*Number base (32 bits)*/
    du_int un1, un0;                                /*Norm. dividend LSD's*/
    du_int vn1, vn0;                                /*Norm. divisor digits*/
    du_int q1, q0;                                  /* Quotient digits*/
    du_int un64, un21, un10;                        /*Dividend digit pairs*/
    du_int rhat;                                    /*A remainder*/
    si_int s;                                       /*Shift amount for normalization*/
    s = __builtin_clzll(v);
    if (s > 0) {
        /* Normalize the divisor.*/
        v = v << s;
        un64 = (u1 << s) | (u0 >> (n_udword_bits - s));
        un10 = u0 << s; /*Shift dividend left*/
    } else {
        /* Avoid undefined behavior of (u0 >> 64).*/
        un64 = u1;
        un10 = u0;
    }
    /*Break divisor up into two 32-bit digits.*/
    vn1 = v >> (n_udword_bits / 2);
    vn0 = v & 0xFFFFFFFF;
    /*Break right half of dividend into two digits.*/
    un1 = un10 >> (n_udword_bits / 2);
    un0 = un10 & 0xFFFFFFFF;
    /*Compute the first quotient digit, q1.*/
    q1 = un64 / vn1;
    rhat = un64 - q1 * vn1;
    /*q1 has at most error 2. No more than 2 iterations.*/
    while (q1 >= b || q1 * vn0 > b * rhat + un1) {
        q1 = q1 - 1;
        rhat = rhat + vn1;
        if (rhat >= b) {
            break;
        }
    }
    un21 = un64 * b + un1 - q1 * v;
    /*Compute the second quotient digit.*/
    q0 = un21 / vn1;
    rhat = un21 - q0 * vn1;
    /*q0 has at most error 2. No more than 2 iterations.*/
    while (q0 >= b || q0 * vn0 > b * rhat + un0) {
        q0 = q0 - 1;
        rhat = rhat + vn1;
        if (rhat >= b) {
            break;
        }
    }
    *r = (un21 * b + un0 - q0 * v) >> s;
    return q1 * b + q0;
}
static inline du_int udiv128by64to64(du_int u1, du_int u0, du_int v,
                                     du_int *r) {
#if defined(__x86_64__)
    du_int result;
    __asm__ ("divq %[v]"
             : "=a" (result), "=d" (*r)
             : [ v ] "r" (v), "a" (u0), "d" (u1));
    return result;
#else
    return udiv128by64to64default(u1, u0, v, r);
#endif
}

tu_int __udivmodti4(tu_int a, tu_int b, tu_int *rem) {
    const unsigned n_utword_bits = sizeof(tu_int) * CHAR_BIT;
    utwords dividend;
    dividend.all = a;
    utwords divisor;
    divisor.all = b;
    utwords quotient;
    utwords remainder;
    if (divisor.all > dividend.all) {
        if (rem) {
            *rem = dividend.all;
        }
        return 0;
    }
    /* When the divisor fits in 64 bits, we can use an optimized path.*/
    if (divisor.s.high == 0) {
        remainder.s.high = 0;
        if (dividend.s.high < divisor.s.low) {
            /*The result fits in 64 bits.*/
            quotient.s.low = udiv128by64to64(dividend.s.high, dividend.s.low,
                                             divisor.s.low, &remainder.s.low);
            quotient.s.high = 0;
        } else {
            /* First, divide with the high part to get the remainder in dividend.s.high.
             * After that dividend.s.high < divisor.s.low.*/
            quotient.s.high = dividend.s.high / divisor.s.low;
            dividend.s.high = dividend.s.high % divisor.s.low;
            quotient.s.low = udiv128by64to64(dividend.s.high, dividend.s.low,
                                             divisor.s.low, &remainder.s.low);
        }
        if (rem) {
            *rem = remainder.all;
        }
        return quotient.all;
    }
    /*0 <= shift <= 63.*/
    si_int shift =
        __builtin_clzll(divisor.s.high) - __builtin_clzll(dividend.s.high);
    divisor.all <<= shift;
    quotient.s.high = 0;
    quotient.s.low = 0;
    for (; shift >= 0; --shift) {
        quotient.s.low <<= 1;
        /* Branch free version of.
         * if (dividend.all >= divisor.all)
         * {
         *  dividend.all -= divisor.all;
         *  carry = 1;
         * }
         * */
        const ti_int s =
            (ti_int)(divisor.all - dividend.all - 1) >> (n_utword_bits - 1);
        quotient.s.low |= s & 1;
        dividend.all -= divisor.all & s;
        divisor.all >>= 1;
    }
    if (rem) {
        *rem = dividend.all;
    }
    return quotient.all;
}

/* https://gcc.gnu.org/onlinedocs/gccint/Integer-library-routines.html */
__attribute__ ((__used__))
tu_int __udivti3 (tu_int a, tu_int b)
{
    return __udivmodti4(a, b, 0);
}
