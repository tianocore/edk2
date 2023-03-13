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

/* Minimal platform example (which is acutally is a NOP)
* This "platform" shows how to use the SyS-T platform modules
* to implement platform specific customizations.
*/

#ifndef MIPI_SYST_PLATFORM_INCLUDED
#define MIPI_SYST_PLATFORM_INCLUDED

/* Uncomment to turn  code inlining off.
 *
 * #undef MIPI_SYST_PCFG_ENABLE_INLINE
 */

#if defined(MIPI_SYST_PCFG_ENABLE_HEAP_MEMORY)
#include <stdlib.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
* Platform specific SyS-T global state extension
*
* The contents of this structure can be freely defined to
* match platform specific data needs. It can later be
* accessed through the mipi_syst_header systh_platform member.
*
* This platform example puts low-level output function pointers
* here. Real implementations may have them "inlined" for performance
* reasons.
*/
struct mipi_syst_platform_state {
	volatile void * mmio;
};

extern MIPI_SYST_EXPORT void MIPI_SYST_CALLCONV
	mipi_syst_platform_init(struct mipi_syst_header *, const void *);
extern MIPI_SYST_EXPORT void MIPI_SYST_CALLCONV
	mipi_syst_platform_destroy(struct mipi_syst_header * systh);

#if defined(MIPI_SYST_PCFG_ENABLE_TIMESTAMP)
/* this example uses  UNIX epoch time in micro-second resolution
* as own clock.
*/
#if defined(MIPI_SYST_UNIT_TEST)
#define MIPI_SYST_PLATFORM_CLOCK() 0x12345678aabbccdd
#else
#define MIPI_SYST_PLATFORM_CLOCK() 0 /* replace  with real clock provider */
#endif
#define MIPI_SYST_PLATFORM_FREQ()  1000*1000

#endif /* defined(MIPI_SYST_PCFG_ENABLE_TIMESTAMP) */

/**
* Platform specific SyS-T handle state extension
*
* The contents of this structure can be freely defined to
* match platform specific data needs. It can later be
* accessed through the syst_handles's systh_platform member.
*
* @see MIPI_SYST_PCFG_ENABLE_PLATFORM_HANDLE_DATA struct mipi_syst_handle
*/
struct mipi_syst_platform_handle {
	volatile void * mmio_addr;
};


#if defined(MIPI_SYST_PCFG_ENABLE_HEAP_MEMORY)
/**
* Map heap memory allocation to platfrom malloc() implementation.
*
* This function is used for handle allocations if heap usage
* is supported by the platform.
*
* @param s number of bytes to allocate
* @see MIPI_SYST_HEAP_FREE
*/
#define MIPI_SYST_HEAP_MALLOC(s) mipi_syst_platform_alloc(s)

/**
* Map heap memory free function  to platfrom free() implementation.
*
* This function is used for handle release if heap usage
* is supported by the platform.
*
* @param p pointer previously returned from MIPI_SYST_HEAP_MALLOC or NULL.
* @see MIPI_SYST_HEAP_MALLOC
*/
#define MIPI_SYST_HEAP_FREE(p)   mipi_syst_platform_free(p)

extern MIPI_SYST_EXPORT void * MIPI_SYST_CALLCONV  mipi_syst_platform_alloc(size_t s);
extern MIPI_SYST_EXPORT void MIPI_SYST_CALLCONV mipi_syst_platform_free(void *);
#endif

/* IO output routine mapping
 * Define these to generate SyS-T data protocol output
 */
#define MIPI_SYST_OUTPUT_D32TS(syst_handle, data)
#define MIPI_SYST_OUTPUT_D32MTS(syst_handle, data)
#define MIPI_SYST_OUTPUT_D64MTS(syst_handle, data)
#define MIPI_SYST_OUTPUT_D8(syst_handle, data)
#define MIPI_SYST_OUTPUT_D16(syst_handle, data)
#define MIPI_SYST_OUTPUT_D32(syst_handle, data)
#if defined(MIPI_SYST_PCFG_ENABLE_64BIT_IO)
#define MIPI_SYST_OUTPUT_D64(syst_handle, data)
#endif
#define MIPI_SYST_OUTPUT_FLAG(syst_handle)

#endif


#ifdef __cplusplus
} /* extern C */
#endif
