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

#include <stdio.h>
#if defined(_WIN32)
#include <windows.h>
#else
#include <sys/time.h>
#endif

#include "mipi_syst.h"

/* "driver" data output routines */
static void write_d32mts(struct mipi_syst_handle* systh, mipi_syst_u32 v);
static void write_d64mts(struct mipi_syst_handle* systh, mipi_syst_u64 v);
static void write_d32ts(struct mipi_syst_handle* systh, mipi_syst_u32 v);
static void write_d8(struct mipi_syst_handle* systh, mipi_syst_u8 v);
static void write_d16(struct mipi_syst_handle* systh, mipi_syst_u16 v);
static void write_d32(struct mipi_syst_handle* systh, mipi_syst_u32 v);
#if defined(MIPI_SYST_PCFG_ENABLE_64BIT_IO)
static void write_d64(struct mipi_syst_handle* systh, mipi_syst_u64 vp);
#endif
static void write_flag(struct mipi_syst_handle* systh);

/**
 * Platform specific SyS-T handle initialization hook function
 *
 * @param systh pointer to the new SyS-T handle structure
 */
static void platform_handle_init(struct mipi_syst_handle* systh)
{
#if !defined(MIPI_SYST_UNIT_TEST)
	printf(
		"  in SyS-T platform handle init hook: systh = %p\n\n",
		systh
	);
#endif

	/* Initialize platform specific data in global state
	 * This example just stores the platform_data, a real implementation
	 * would put data into the handle structure that enables the output
	 * routines to execute efficiently (pointer for MMIO for example).
	 */
#if defined(MIPI_SYST_PCFG_ENABLE_PLATFORM_HANDLE_DATA)

#endif
}

/**
 * Platform specific SyS-T handle initialization hook function
 *
 * @param systh pointer to the new SyS-T handle structure
 */
static void platform_handle_release(struct mipi_syst_handle* systh)
{
#if !defined(MIPI_SYST_UNIT_TEST) && defined(MIPI_SYST_PCFG_ENABLE_PLATFORM_HANDLE_DATA)
	printf(
		"  in SyS-T platform handle release hook:systh = %p\n",
		systh
	);
#endif

	/* Release any handle specific data or resources here.*/
}

/**
 * Platform specific global state initialization hook function
 *
 * @param systh pointer to the new SyS-T handle structure
 * @param platform_data user defined data for the init function.
 */
MIPI_SYST_EXPORT void MIPI_SYST_CALLCONV
mipi_syst_platform_init(struct mipi_syst_header* systh, const void * platform_data)
{
#if !defined(MIPI_SYST_UNIT_TEST)
	printf(
		"  in SyS-T platform init hook: \"mipi_syst_platform_state_init()\"\n"
		"                              systh = %p, platform_data = %p\n",
		systh,
		platform_data
	);
#endif

	/* Set handle init hook that performs per SyS-T handle initialization
	 * and destruction
	 */
#if defined(MIPI_SYST_PCFG_ENABLE_PLATFORM_HANDLE_DATA)
	systh->systh_inith = platform_handle_init;
	systh->systh_releaseh = platform_handle_release;
#endif

	/* Initialize platform specific data in global state
	 * This platform example puts its low level output function
	 * pointers here. A real implementation may have these "inlined"
	 * for performance reasons.
	 */
#if defined(MIPI_SYST_PCFG_ENABLE_PLATFORM_STATE_DATA)
	systh->systh_platform.write_d32ts = write_d32ts;
	systh->systh_platform.write_d32mts = write_d32mts;
	systh->systh_platform.write_d64mts = write_d64mts;
	systh->systh_platform.write_d8 = write_d8;
	systh->systh_platform.write_d16 = write_d16;
	systh->systh_platform.write_d32 = write_d32;
#if defined(MIPI_SYST_PCFG_ENABLE_64BIT_IO)
	systh->systh_platform.write_d64 = write_d64;
#endif
	systh->systh_platform.write_flag = write_flag;
#endif
}

MIPI_SYST_EXPORT void MIPI_SYST_CALLCONV
mipi_syst_platform_destroy(struct mipi_syst_header* systh)
{
	(void)systh;
}

#if !defined(MIPI_SYST_STATIC)
/**
 * This example platform uses SyS-T as a shared library inside an
 * application. The platform init hook is called during a shared library
 * constructor call.
 */
static MIPI_SYST_SHAREDLIB_CONSTRUCTOR void shared_library_init()
{
	/* Initialize SyS-T infrastructure
	 * This must be done once at platform startup.
	 * The parameters are the platform specific initialization function and
	 * the data that gets passed to it.
	 */
	printf("calling MIPI_SYST_INIT() from platform code  \n");
	MIPI_SYST_INIT(mipi_syst_platform_init, (void*)42);
}

/**
 * This example platform  uses SyS-T as a shared library inside an
 * application. The platform destroy hook is called during a shared library
 * destructor call.
 */
static MIPI_SYST_SHAREDLIB_DESTRUCTOR void shared_library_exit()
{
	/* run platform shutdown code */
	MIPI_SYST_SHUTDOWN(mipi_syst_platform_destroy);
	printf("\nexecuted MIPI_SYST_SHUTDOWN() from platform code  \n");
}

#if defined(_WIN32)
/**
 * Windows DLL main routine, needed to run the global initialization and
 * destruction handlers.
 */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved)
{
	(void)lpReserved;
	(void)hinstDLL;

	switch (fdwReason) {
	case DLL_PROCESS_ATTACH:
		shared_library_init();
		break;
	case DLL_PROCESS_DETACH:
		shared_library_exit();
		break;
	}
	return TRUE;
}
#endif
#endif /* !defined(MIPI_SYST_STATIC) */

#if defined(MIPI_SYST_PCFG_ENABLE_PLATFORM_HANDLE_DATA)

/* Save a message bytes for printing the raw message when completed.
 */
static void append_raw(
	struct mipi_syst_handle* systh,
	const void * p,
	int n)
{
	int i;

#if defined(MIPI_SYST_BIG_ENDIAN)
	for (i = n-1; i >= 0; --i) {
#else
	for (i = 0; i < n; ++i) {
#endif
		mipi_syst_u32 index = systh->systh_platform.sph_raw_count++;
		if (index < sizeof(systh->systh_platform.sph_raw)) {
			systh->systh_platform.sph_raw[index] =
				((const mipi_syst_u8*)p)[i];
		}
		else {
			fprintf(stderr, "Internal Error: Record buffer overflow\n");
			break;
		}
	}
}
#endif

/* dump contents for raw message bytes
 */
static void write_rawdata(struct mipi_syst_handle* systh)
{
#if defined(MIPI_SYST_PCFG_ENABLE_PLATFORM_HANDLE_DATA)
	static char valToHex[] = "0123456789ABCDEF";
	const mipi_syst_u8 *p;

	printf("SYS-T RAW DATA: ");
	for (p = systh->systh_platform.sph_raw;
		systh->systh_platform.sph_raw_count;
		++p, --systh->systh_platform.sph_raw_count)
	{
		putc(valToHex[(*p) >> 0x4], stdout);
		putc(valToHex[(*p) & 0xF], stdout);
	}
#endif
}

/*  Dummy driver output routines that just print their operation.
 */
static void write_d32mts(struct mipi_syst_handle* systh, mipi_syst_u32 v)
{
	/* d32mts == Short SyS-T header, reset io count
	 */
#if defined(MIPI_SYST_PCFG_ENABLE_PLATFORM_HANDLE_DATA)
	systh->systh_platform.sph_io_count = 0;
	systh->systh_platform.sph_raw_count = 0;


	printf("STP Protocol Output:\n");

	printf("    %2d <D32MTS> %08x\n",
		systh->systh_platform.sph_io_count++, v);

	append_raw(systh, &v, sizeof(v));
	write_rawdata(systh);
	printf("\n\n");
#endif
}
static void write_d64mts(struct mipi_syst_handle* systh, mipi_syst_u64 v)
{
	/* d64mts == 64-Bit Short SyS-T header, reset io count
	 */
#if defined(MIPI_SYST_PCFG_ENABLE_PLATFORM_HANDLE_DATA)
	systh->systh_platform.sph_io_count = 0;

	printf("STP Protocol Output:\n");

	printf("    %2d <D64MTS> %08llx\n",
		systh->systh_platform.sph_io_count++, v);

	append_raw(systh, &v, sizeof(v));
	write_rawdata(systh);
	printf("\n\n");

#endif
}

static void write_d32ts(struct mipi_syst_handle* systh, mipi_syst_u32 v)
{
	/* d32ts == SyS-T header, reset io count
	 */
#if defined(MIPI_SYST_PCFG_ENABLE_PLATFORM_HANDLE_DATA)
	systh->systh_platform.sph_io_count = 0;

	printf("STP Protocol Output:\n");

	printf("    %2d <D32TS>  %08x\n",
		systh->systh_platform.sph_io_count++, v);

	append_raw(systh, &v, sizeof(v));
#endif
}
static void write_d8(struct mipi_syst_handle* systh, mipi_syst_u8 v)
{
#if defined(MIPI_SYST_PCFG_ENABLE_PLATFORM_HANDLE_DATA)
	printf("    %2d <D8>     %02x\n",
		systh->systh_platform.sph_io_count++, v);

	append_raw(systh, &v, sizeof(v));
#endif
}

static void write_d16(struct mipi_syst_handle* systh, mipi_syst_u16 v)
{
#if defined(MIPI_SYST_PCFG_ENABLE_PLATFORM_HANDLE_DATA)
	printf("    %2d <D16>    %04x\n",
		systh->systh_platform.sph_io_count++, v);

	append_raw(systh, &v, sizeof(v));
#endif
}

static void write_d32(struct mipi_syst_handle* systh, mipi_syst_u32 v)
{
#if defined(MIPI_SYST_PCFG_ENABLE_PLATFORM_HANDLE_DATA)
	printf("    %2d <D32>    %08x\n",
		systh->systh_platform.sph_io_count++, v);

	append_raw(systh, &v, sizeof(v));
#endif
}

#if defined(MIPI_SYST_PCFG_ENABLE_64BIT_IO)
static void write_d64(struct mipi_syst_handle* systh, mipi_syst_u64 v)
{
#if defined(MIPI_SYST_PCFG_ENABLE_PLATFORM_HANDLE_DATA)
	printf("    %2d <D64>    %08x%08x\n",
		systh->systh_platform.sph_io_count++,
#if defined(MIPI_SYST_BIG_ENDIAN)
		(mipi_syst_u32)v, (mipi_syst_u32)(v >> 32);
#else
		(mipi_syst_u32)(v >> 32), (mipi_syst_u32)v);
#endif
#endif
	append_raw(systh, &v, sizeof(v));
}
#endif

static void write_flag(struct mipi_syst_handle* systh)
{
#if defined(MIPI_SYST_PCFG_ENABLE_PLATFORM_HANDLE_DATA)
	printf("    %2d <FLAG>\n", systh->systh_platform.sph_io_count++);

	write_rawdata(systh);
	printf("\n\n");

#endif
}

MIPI_SYST_EXPORT
mipi_syst_u64  MIPI_SYST_CALLCONV mipi_syst_get_epoch_us()
{
	mipi_syst_u64 epoch;
#if defined(MIPI_SYST_UNIT_TEST)
	epoch = 0x12345678aabbccddull;
#elif defined(_WIN32)
	// Windows does not offer epoch time API directly.
	// Search for the 116444... constant below on
	// MSDN for an explanation of the computation:
	//
	FILETIME ft;
	GetSystemTimeAsFileTime(&ft);
	epoch = ft.dwHighDateTime;
	epoch = ((epoch << 32) | ft.dwLowDateTime) / 10 - 11644473600000000ULL;

#else
	struct timeval    tv;

	gettimeofday(&tv, NULL);
	epoch = tv.tv_sec;
	epoch *= 1000000;
	epoch += tv.tv_usec;
#endif
	return epoch;
}

#if defined(MIPI_SYST_PCFG_ENABLE_HEAP_MEMORY)
MIPI_SYST_EXPORT
void * MIPI_SYST_CALLCONV  mipi_syst_platform_alloc(size_t s)
{
	return malloc(s);
}

MIPI_SYST_EXPORT
void MIPI_SYST_CALLCONV mipi_syst_platform_free(void * p)
{
	free(p);
}

#endif /* defined(MIPI_SYST_PCFG_ENABLE_HEAP_MEMORY) */