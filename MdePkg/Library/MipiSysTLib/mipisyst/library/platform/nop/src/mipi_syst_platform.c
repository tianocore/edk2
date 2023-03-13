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
#endif
#include "mipi_syst.h"


/**
 * Platform specific SyS-T handle initialization hook function
 *
 * @param systh pointer to the new SyS-T handle structure
 */
static void platform_handle_init(struct mipi_syst_handle* systh)
{
}

/**
 * Platform specific SyS-T handle initialization hook function
 *
 * @param systh pointer to the new SyS-T handle structure
 */
static void platform_handle_release(struct mipi_syst_handle* systh)
{
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
	/* Set handle init hook that performs per SyS-T handle initialization
	* and destruction
	*/
#if defined(MIPI_SYST_PCFG_ENABLE_PLATFORM_HANDLE_DATA)
	systh->systh_inith = platform_handle_init;
	systh->systh_releaseh = platform_handle_release;
#endif
}

MIPI_SYST_EXPORT void MIPI_SYST_CALLCONV
mipi_syst_platform_destroy(struct mipi_syst_header* systh)
{
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


#if !defined(MIPI_SYST_STATIC)
/**
* This example platform uses SyS-T as a shared library inside an
* application. The platform init hook is called during a shared library
* constructor call.
*/
static MIPI_SYST_SHAREDLIB_CONSTRUCTOR void shared_library_init()
{
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
#endif /* defined(MIPI_SYST_PCFG_ENABLE_HEAP_MEMORY) */