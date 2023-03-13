/*
Copyright (c) 2018, MIPI Alliance, Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

* Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in
  the documentation and/or other materials provided with the
  distribution.

* Neither the name of the copyright holder nor the names of its
  contributors may be used to endorse or promote products derived
  from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*
 * Contributors:
 * Norbert Schulz (Intel Corporation) - Initial API and implementation
 */

/* Compiler specific defines */

#ifndef MIPI_SYST_COMPILER_INCLUDED
#define MIPI_SYST_COMPILER_INCLUDED

#if defined(__cplusplus)
extern "C" {
#endif

#if defined(__clang__)
#undef _WIN32
#endif

#if defined(_WIN32)		/* MSVC Compiler section */

/* basic integer types
 */
typedef __int8 mipi_syst_s8;
typedef __int16 mipi_syst_s16;
typedef __int32 mipi_syst_s32;
typedef __int64 mipi_syst_s64;

typedef unsigned __int8 mipi_syst_u8;
typedef unsigned __int16 mipi_syst_u16;
typedef unsigned __int32 mipi_syst_u32;
typedef unsigned __int64 mipi_syst_u64;

/* shared library import/export
 */
#if defined(MIPI_SYST_STATIC)
#define MIPI_SYST_EXPORT
#define MIPI_SYST_EXPORT
#else
#if defined(MIPI_SYST_EXPORTS)
#define MIPI_SYST_EXPORT   __declspec(dllexport)
#else
#define MIPI_SYST_EXPORT   __declspec(dllimport)
#endif
#endif

#define MIPI_SYST_CALLCONV __stdcall

/* Caution: Windows doesn't support attribute based shared library
 * life time functions. Add these calls into a dllmain routine
 * instead.
 */
#define MIPI_SYST_SHAREDLIB_CONSTRUCTOR
#define MIPI_SYST_SHAREDLIB_DESTRUCTOR

#define MIPI_SYST_FUNCTION_NAME __FUNCTION__
#define MIPI_SYST_LINE          __LINE__
#define MIPI_SYST_FILE          __FILE__

#if defined(NDEBUG)
#define _MIPI_SYST_OPTIMIZER_ON
#endif

#define MIPI_SYST_CC_INLINE     __inline

/* Macros for byte swapping to little endian
 *
 * Assume this compiler is always little endian
 */
#define MIPI_SYST_HTOLE16(v) (v)
#define MIPI_SYST_HTOLE32(v) (v)
#define MIPI_SYST_HTOLE64(v) (v)

/* HW CRC32C support ? */
#if defined(MIPI_SYST_CRC_INTRINSIC_ON)
#define MIPI_SYST_CRC_INTRINSIC

#include <intrin.h>
#define _MIPI_SYST_CPU_CRC8(crc, v) _mm_crc32_u8((crc), (v))
#define _MIPI_SYST_CPU_CRC16(crc, v) _mm_crc32_u16((crc), (v))
#define _MIPI_SYST_CPU_CRC32(crc, v) _mm_crc32_u32((crc), (v))
#if defined(_WIN64)
#define _MIPI_SYST_CPU_CRC64(crc, v) (mipi_syst_u32)_mm_crc32_u64((crc), (v))
#else
#define _MIPI_SYST_CPU_CRC64(crc, v)  \
	_mm_crc32_u32(\
		_mm_crc32_u32((crc), (mipi_syst_u32)(v)), \
		((mipi_syst_u32)((v)>> 32))\
	)
#endif
#endif

#elif defined(__GNUC__)	/* GNU-C Compiler section */

/* basic integer types
 */
typedef char mipi_syst_s8;
typedef short mipi_syst_s16;
typedef int mipi_syst_s32;
typedef long long mipi_syst_s64;

typedef unsigned char mipi_syst_u8;
typedef unsigned short mipi_syst_u16;
typedef unsigned int mipi_syst_u32;
typedef unsigned long long mipi_syst_u64;

/* shared library related settings
 */
#define MIPI_SYST_EXPORT
#define MIPI_SYST_CALLCONV

#define MIPI_SYST_SHAREDLIB_CONSTRUCTOR __attribute__((constructor))
#define MIPI_SYST_SHAREDLIB_DESTRUCTOR  __attribute__((destructor))

#define MIPI_SYST_FUNCTION_NAME __PRETTY_FUNCTION__
#define MIPI_SYST_LINE          __LINE__
#define MIPI_SYST_FILE          __FILE__
#define MIPI_SYST_CC_INLINE     inline

/* Macros for byte swapping to little endian
 */
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define MIPI_SYST_BIG_ENDIAN

#define MIPI_SYST_HTOLE16(v) \
	((((mipi_syst_u16)(v))>>8)|((((mipi_syst_u16)(v))&0xFF)<<8))
#define MIPI_SYST_HTOLE32(v) \
	 ((mipi_syst_u32)__builtin_bswap32((mipi_syst_u32)(v)))
#define MIPI_SYST_HTOLE64(v) \
	 ((mipi_syst_u64)__builtin_bswap64((mipi_syst_u64)(v)))
#else
#define MIPI_SYST_HTOLE16(v) (v)
#define MIPI_SYST_HTOLE32(v) (v)
#define MIPI_SYST_HTOLE64(v) (v)
#endif

#if defined(__OPTIMIZE__)
#define _MIPI_SYST_OPTIMIZER_ON
#endif

/* HW CRC32C support  ? */
#if defined(MIPI_SYST_CRC_INTRINSIC_ON)
#define MIPI_SYST_CRC_INTRINSIC

#define _MIPI_SYST_CPU_CRC8(crc, v) __builtin_ia32_crc32qi((crc), (v))
#define _MIPI_SYST_CPU_CRC16(crc, v) __builtin_ia32_crc32hi((crc), (v))
#define _MIPI_SYST_CPU_CRC32(crc, v) __builtin_ia32_crc32si((crc), (v))
#if defined(_WIN64) || defined(__x86_64__) || defined (__LP64__)
#define _MIPI_SYST_CPU_CRC64(crc, v) (mipi_syst_u32)__builtin_ia32_crc32di((crc), (v))
#else
#define _MIPI_SYST_CPU_CRC64(crc, v)  \
	__builtin_ia32_crc32si (\
		__builtin_ia32_crc32si((crc), (mipi_syst_u32)(v)), \
		((mipi_syst_u32)((v)>> 32))\
	)
#endif
#endif
#else
#error unknown compiler, copy and adapt one of the sections above
#endif

#ifdef __cplusplus
} /* extern C */
#endif
#endif
