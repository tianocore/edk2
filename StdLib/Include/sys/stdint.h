/*  $NetBSD: stdint.h,v 1.5 2005/12/11 12:25:21 christos Exp $  */

/*-
 * Copyright (c) 2001, 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Klaus Klein.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#ifndef _SYS_STDINT_H_
#define _SYS_STDINT_H_

#include  <sys/EfiCdefs.h>
#include  <machine/int_types.h>

#ifndef int8_t
typedef __int8_t  int8_t;
#define int8_t    __int8_t
#endif

#ifndef uint8_t
typedef __uint8_t uint8_t;
#define uint8_t   __uint8_t
#endif

#ifndef int16_t
typedef __int16_t int16_t;
#define int16_t   __int16_t
#endif

#ifndef uint16_t
typedef __uint16_t  uint16_t;
#define uint16_t  __uint16_t
#endif

#ifndef int32_t
typedef __int32_t int32_t;
#define int32_t   __int32_t
#endif

#ifndef uint32_t
typedef __uint32_t  uint32_t;
#define uint32_t  __uint32_t
#endif

#ifndef int64_t
typedef __int64_t int64_t;
#define int64_t   __int64_t
#endif

#ifndef uint64_t
typedef __uint64_t  uint64_t;
#define uint64_t  __uint64_t
#endif

#ifndef intptr_t
typedef __intptr_t  intptr_t;
#define intptr_t  __intptr_t
#endif

#ifndef uintptr_t
typedef __uintptr_t uintptr_t;
#define uintptr_t __uintptr_t
#endif

#include  <machine/int_mwgwtypes.h>

#if !defined(__cplusplus) || defined(__STDC_LIMIT_MACROS)
#include  <machine/int_limits.h>
#endif

#if !defined(__cplusplus) || defined(__STDC_CONSTANT_MACROS)
#include  <machine/int_const.h>
#endif

//#include  <machine/wchar_limits.h>

#endif /* !_SYS_STDINT_H_ */
