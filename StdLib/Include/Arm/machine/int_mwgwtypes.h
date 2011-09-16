/** @file
    Minimum and Greatest Width Integer types.

    Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
    This program and the accompanying materials are licensed and made available under
    the terms and conditions of the BSD License that accompanies this distribution.
    The full text of the license may be found at
    http://opensource.org/licenses/bsd-license.php.

    THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
    WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

    Portions Copyright (c) 2001 The NetBSD Foundation, Inc.
    All rights reserved.

    This code is derived from software contributed to The NetBSD Foundation
    by Klaus Klein.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
      1.  Redistributions of source code must retain the above copyright
          notice, this list of conditions and the following disclaimer.
      2.  Redistributions in binary form must reproduce the above copyright
          notice, this list of conditions and the following disclaimer in the
          documentation and/or other materials provided with the distribution.
      3.  All advertising materials mentioning features or use of this software
          must display the following acknowledgement:
            This product includes software developed by the NetBSD
            Foundation, Inc. and its contributors.
      4.  Neither the name of The NetBSD Foundation nor the names of its
          contributors may be used to endorse or promote products derived
          from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
    ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
    TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
    PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
    BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
    INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
    CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
    ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
    POSSIBILITY OF SUCH DAMAGE.

    NetBSD: int_mwgwtypes.h,v 1.5 2005/12/24 20:06:47 perry Exp
**/
#ifndef _ARM_INT_MWGWTYPES_H_
#define _ARM_INT_MWGWTYPES_H_

/*
 * 7.18.1 Integer types
 */

/* 7.18.1.2 Minimum-width integer types */

typedef CHAR8     int_least8_t;
typedef UINT8     uint_least8_t;
typedef INT16     int_least16_t;
typedef UINT16    uint_least16_t;
typedef INT32     int_least32_t;
typedef UINT32    uint_least32_t;
typedef INT64     int_least64_t;
typedef UINT64    uint_least64_t;

/* 7.18.1.3 Fastest minimum-width integer types */
typedef INT32     int_fast8_t;
typedef UINT32    uint_fast8_t;
typedef INT32     int_fast16_t;
typedef UINT32    uint_fast16_t;
typedef INT32     int_fast32_t;
typedef UINT32    uint_fast32_t;
typedef INT64     int_fast64_t;
typedef UINT64    uint_fast64_t;

/* 7.18.1.5 Greatest-width integer types */

typedef INT64     intmax_t;
typedef UINT64    uintmax_t;

#endif /* !_ARM_INT_MWGWTYPES_H_ */
