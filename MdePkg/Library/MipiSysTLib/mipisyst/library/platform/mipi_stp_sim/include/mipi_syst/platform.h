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

/* Example platform adaptation
 * This "platform" shows how to implement a SyS-T library platform module
 * to simulate MIPI STP data protocol generation for SyS-T messages.
 */
#ifndef MIPI_SYST_PLATFORM_INCLUDED
#define MIPI_SYST_PLATFORM_INCLUDED


/* Uncomment to turn code inlining off.
 *
 * #undef MIPI_SYST_PCFG_ENABLE_INLINE
 */

#if defined(MIPI_SYST_PCFG_ENABLE_HEAP_MEMORY)
#include <stdlib.h>
#endif

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

	struct MipiStpWriter;

	struct mipi_syst_platform_state {
		void (*write_d32ts)(struct mipi_syst_handle * systh, mipi_syst_u32 v);
		void (*write_d32mts)(struct mipi_syst_handle * systh, mipi_syst_u32 v);
		void (*write_d64mts)(struct mipi_syst_handle * systh, mipi_syst_u64 v);

		void (*write_d8)(struct mipi_syst_handle * systh, mipi_syst_u8 v);
		void (*write_d16)(struct mipi_syst_handle * systh, mipi_syst_u16 v);
		void (*write_d32)(struct mipi_syst_handle * systh, mipi_syst_u32 v);
#if defined(MIPI_SYST_PCFG_ENABLE_64BIT_IO)
		void (*write_d64)(struct mipi_syst_handle * systh, mipi_syst_u64 v);
#endif
		void (*write_flag)(struct mipi_syst_handle * systh);

		struct stp_writer_data * stpWriter;
	};

	extern MIPI_SYST_EXPORT void MIPI_SYST_CALLCONV
		mipi_syst_platform_init(struct mipi_syst_header* , const void *);
	extern MIPI_SYST_EXPORT void MIPI_SYST_CALLCONV
		mipi_syst_platform_destroy(struct mipi_syst_header* systh);

	struct mipi_syst_platform_handle {
		mipi_syst_u32 master;
		mipi_syst_u32 channel;
	};



#if defined(MIPI_SYST_PCFG_ENABLE_HEAP_MEMORY)

/**
* Map heap memory allocation to platform malloc() implementation.
*
* This function is used for handle allocations if heap usage
* is supported by the platform.
*
* @param s number of bytes to allocate
* @see MIPI_SYST_HEAP_FREE
*/
#define MIPI_SYST_HEAP_MALLOC(s) mipi_syst_platform_alloc(s)

/**
* Map heap memory free function  to platform free() implementation.
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

#if defined(MIPI_SYST_PCFG_ENABLE_TIMESTAMP)
/* This example uses UNIX epoch time in micro second resolution
 * as own clock.
 */
#define MIPI_SYST_PLATFORM_CLOCK() mipi_syst_get_epoch_us()
#define MIPI_SYST_PLATFORM_FREQ()  1000000

MIPI_SYST_EXPORT mipi_syst_u64  MIPI_SYST_CALLCONV mipi_syst_get_epoch_us();
#endif /* defined(MIPI_SYST_PCFG_ENABLE_TIMESTAMP) */

/* IO output routine mapping
 * Call the function pointers in the global SyS-T state
 */
#define MIPI_SYST_OUTPUT_D32TS(syst_handle, data) \
	(syst_handle)->systh_header->systh_platform.write_d32ts((syst_handle), (data))
#define MIPI_SYST_OUTPUT_D32MTS(syst_handle, data) \
	(syst_handle)->systh_header->systh_platform.write_d32mts((syst_handle), (data))
#define MIPI_SYST_OUTPUT_D64MTS(syst_handle, data) \
	(syst_handle)->systh_header->systh_platform.write_d64mts((syst_handle), (data))
#define MIPI_SYST_OUTPUT_D8(syst_handle, data) \
	(syst_handle)->systh_header->systh_platform.write_d8((syst_handle), (data))
#define MIPI_SYST_OUTPUT_D16(syst_handle, data) \
	(syst_handle)->systh_header->systh_platform.write_d16((syst_handle), (data))
#define MIPI_SYST_OUTPUT_D32(syst_handle, data) \
	(syst_handle)->systh_header->systh_platform.write_d32((syst_handle), (data))
#if defined(MIPI_SYST_PCFG_ENABLE_64BIT_IO)
#define MIPI_SYST_OUTPUT_D64(syst_handle, data) \
	(syst_handle)->systh_header->systh_platform.write_d64((syst_handle), (data))
#endif
#define MIPI_SYST_OUTPUT_FLAG(syst_handle) \
	(syst_handle)->systh_header->systh_platform.write_flag((syst_handle))

#ifdef __cplusplus
} /* extern C */
#endif

#endif
