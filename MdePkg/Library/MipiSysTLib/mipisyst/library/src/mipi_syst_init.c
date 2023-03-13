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

#include "mipi_syst.h"
#include "mipi_syst/message.h"

/**
 * SyS-T global state
 */
static struct mipi_syst_header syst_hdr = { 0 };

#if !defined(MIPI_SYST_PCFG_ENABLE_DEFAULT_SCATTER_WRITE)
/**
 * null-device style default output function
 */
static void nullWriter(struct mipi_syst_handle* systh,
		       struct mipi_syst_scatter_prog scatterprog, const void *pdesc)
{
}
#endif

/**
 * Initialize the SyS-T library.
 *
 * This function must be called during the start of the platform before any
 * other instrumentation library call. The function initializes the global
 * state data necessary for the library to execute. Passing NULL as state
 * means using the shared global state singleton. Passing a valid pointer
 * allows using multiple SyS-T state context structures in parallel.
 *
 * @param header Pointer to SyS-T global state structure or NULL for default.
 * @param pfinit Pointer to platform initialization function or 0 if not used.
 * @param init_param Value passed to the the platform init hook function.
 */
MIPI_SYST_EXPORT void MIPI_SYST_CALLCONV
mipi_syst_init(
	struct mipi_syst_header* header,
	mipi_syst_inithook_t pfinit,
	const void *init_param)
{
	struct mipi_syst_header zero_header = {0};
	if (0 == header) {
		/* No user supplied global state storage,
		 * use internal default state
		 */
		header = &syst_hdr;
	}

	*header = zero_header;
	header->systh_version = MIPI_SYST_VERSION_CODE;

#if MIPI_SYST_CONFORMANCE_LEVEL > 10
#if defined(MIPI_SYST_PCFG_ENABLE_DEFAULT_SCATTER_WRITE)
	header->systh_writer = mipi_syst_scatter_write;
#else
	header->systh_writer = nullWriter;
#endif
#endif

	/* call platform state initialization hook if defined
	 */
	if ((mipi_syst_inithook_t) 0 != pfinit)
		(*pfinit) (header, init_param);
}

/**
 * Destroy the SyS-T library state.
 *
 * This function must be called during shutdown of the platform to release
 * any SyS-T resources.
 *
 * @param header Pointer to library state or NULL to use shared default.
 * @param pfdestroy Pointer to platform state destroy function or 0
 *                  if not used.
 */
MIPI_SYST_EXPORT void MIPI_SYST_CALLCONV
mipi_syst_destroy(struct mipi_syst_header* header, mipi_syst_destroyhook_t pfdestroy)
{
	if (0 == header) {
		/* No user supplied global state storage,
		 * use internal default state
		 */
		header = &syst_hdr;
	}

	/* call platform state destroy hook first, if defined
	 */
	if ((mipi_syst_destroyhook_t) 0 != pfdestroy)
		(*pfdestroy) (header);
}

/**
 * Initialize a SyS-T handle.
 *
 * @param header Pointer to library state or NULL to use shared default.
 * @param svh Pointer to new/uninitialized SyS-T handle
 * @param origin Value passed to the the platform handle init function
 * @param fromHeap 1 of heap allocated handle, 0 otherwise
 */
MIPI_SYST_EXPORT struct mipi_syst_handle* MIPI_SYST_CALLCONV
mipi_syst_init_handle(
	struct mipi_syst_header* header,
	struct mipi_syst_handle* svh,
	const struct mipi_syst_origin *origin,
	mipi_syst_u32 fromHeap)
{
	struct mipi_syst_handle zero_handle = {0};
	if ((struct mipi_syst_handle*) 0 == svh)
		return svh;


	if (0 == header) {
		/* No user supplied global state storage,
		 * use internal default state
		 */
		header = &syst_hdr;
	}

	*svh = zero_handle;

	svh->systh_header = header;
	svh->systh_flags.shf_alloc = fromHeap ? 1 : 0;

#if defined(MIPI_SYST_PCFG_ENABLE_ORIGIN_GUID)

	if (0 != origin)
	{
		MIPI_SYST_SET_HANDLE_ORIGIN(svh, *origin);
	}
#endif

	/* call platform handle initialization hook if defined
	 */
#if defined(MIPI_SYST_PCFG_ENABLE_PLATFORM_HANDLE_DATA)
	if ((mipi_syst_inithandle_hook_t) 0 != svh->systh_header->systh_inith)
		svh->systh_header->systh_inith(svh);
#endif
	return svh;
}

/**
 *  Release a SyS-T handle.
 *
 * @param svh Pointer to initialized SyS-T handle
 */
MIPI_SYST_EXPORT void MIPI_SYST_CALLCONV mipi_syst_delete_handle(struct mipi_syst_handle* svh)
{
	struct mipi_syst_handle zero_handle = {0};
	if ((struct mipi_syst_handle*) 0 != svh) {
#if defined(MIPI_SYST_PCFG_ENABLE_PLATFORM_HANDLE_DATA)
		/* call platform handle release hook if defined
		 */
		if ((mipi_syst_releasehandle_hook_t) 0 !=
		    svh->systh_header->systh_releaseh)
			svh->systh_header->systh_releaseh(svh);
#endif

#if defined(MIPI_SYST_PCFG_ENABLE_HEAP_MEMORY)
		if (0 != svh->systh_flags.shf_alloc) {
			MIPI_SYST_HEAP_FREE(svh);
		} else
#endif
		{
			*svh = zero_handle;
		}
	}
}