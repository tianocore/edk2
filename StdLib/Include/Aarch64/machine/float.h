/*  $NetBSD: float.h,v 1.6 2005/12/11 12:16:47 christos Exp $   */
/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _AARCH64_FLOAT_H_
#define _AARCH64_FLOAT_H_

#ifndef __VFP_FP__
#define LDBL_MANT_DIG   64
#define LDBL_EPSILON    1.0842021724855044340E-19L
#define LDBL_DIG        18
#define LDBL_MIN_EXP    (-16381)
#define LDBL_MIN        1.6810515715560467531E-4932L
#define LDBL_MIN_10_EXP (-4931)
#define LDBL_MAX_EXP    16384
#define LDBL_MAX        1.1897314953572317650E+4932L
#define LDBL_MAX_10_EXP 4932
#endif

#include <sys/float_ieee754.h>

#ifndef __VFP_FP__
#if !defined(_ANSI_SOURCE) && !defined(_POSIX_C_SOURCE) && \
    !defined(_XOPEN_SOURCE) || \
    ((__STDC_VERSION__ - 0) >= 199901L) || \
    ((_POSIX_C_SOURCE - 0) >= 200112L) || \
    ((_XOPEN_SOURCE  - 0) >= 600) || \
    defined(_ISOC99_SOURCE) || defined(_NETBSD_SOURCE)
#define DECIMAL_DIG     21
#endif /* !defined(_ANSI_SOURCE) && ... */
#endif /* !__VFP_FP__ */

#endif /* !_AARCH64_FLOAT_H_ */
