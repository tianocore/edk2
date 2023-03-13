/*
Copyright (c) 2018-2023, MIPI Alliance, Inc.
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
 * Przemyslaw Romaniak (Intel Corporation) - SBD implementation
 */

/* SyS-T Instrumentation API defintions
 */

#ifndef MIPI_SYST_API_INCLUDED
#define MIPI_SYST_API_INCLUDED

#ifndef MIPI_SYST_H_INCLUDED
#include "mipi_syst.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Define away all instrumentation calls if one if the following global
 * disable switches is set
 */
#if !defined(MIPI_SYST_DISABLE_ALL)

/**
 * @defgroup ApiSets API Sets
 *
 * SyS-T provided Instrumentation API sets
 *
 * SyS-T provides different API sets. Most sets can be individually enabled
 * or disabled using the @ref PCFG_ApiSet platform feature defines.
 */

/**
 * @defgroup API_global State and Lifetime handling macros
 * @ingroup ApiSets
 *
 * State and handle lifetime related macros
 * @{
 */

/**
 * SyS-T platform initialization with user provided state structure
 *
 * This function must be called during system startup to initialize the SyS-T
 * execution environment. This function expects a user provided SyS-T state
 * structure pointer. This call supports environments with different
 * library states at the same time. To initialize SyS-T for using a global
 * shared state, call #MIPI_SYST_INIT(f,p) instead.
 *
 * @param s Pointer to SyS-T state header variable
 * @param f Pointer to platform initialization hook function
 * @param p Pointer value that gets passed to the initialization hook function
 */
#define MIPI_SYST_INIT_STATE(s, f, p) \
	mipi_syst_init((s), (f), (p))

/**
 * SyS-T platform shutdown with user provided state structure
 *
 * This function expects a user provided SyS-T state
 * structure pointer. This call supports environments with different
 * library states at the same time. To shutdown SyS-T using a global
 * shared state, call #MIPI_SYST_SHUTDOWN(f) instead.
 *
 * @param s Pointer to SyS-T state header variable
 * @param f pointer to platform resource destruction hook function
 */
#define MIPI_SYST_SHUTDOWN_STATE(s, f) \
	mipi_syst_destroy((s), (f))

/**
 * SyS-T global platform initialization
 *
 * This function must be called during system startup to initialize the SyS-T
 * execution environment.
 *
 * @param f pointer to platform initialization hook function
 * @param p pointer value that gets passed to the initialization hook function
 */
#define MIPI_SYST_INIT(f, p) \
	MIPI_SYST_INIT_STATE((struct mipi_syst_header*)0, (f), (p))

/**
 * SyS-T global platform shutdown
 *
 * @param f pointer to platform resource destruction hook function
 */
#define MIPI_SYST_SHUTDOWN(f) \
	MIPI_SYST_SHUTDOWN_STATE((struct mipi_syst_header*)0, (f))

/**
 * Initialize non-heap SyS-T handle with custom global state
 *
 * This function is used in platforms that don't support heap allocations.
 * The caller has to provide a pointer to a memory location that can hold
 * a mipi_syst_handle data structure. This function expect a user provided
 * SyS-T state structure pointer as its first parameter. To create a
 * handle for the shared global state, call #MIPI_SYST_INIT_HANDLE(h,p) instead.
 *
 * @param s Pointer to SyS-T state header variable
 * @param h Pointer to handle data structure on the stack or data segment.
 * @param p Pointer to data that get passed to the platform handle init hook
 *          function.
 *
 * Example
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 * extern struct mipi_syst_header systh_header;
 * static struct mipi_syst_handle systh_data;
 *
 * void foo()
 * {
 *      struct mipi_syst_handle* svh;
 *
 *      svh = MIPI_SYST_INIT_HANDLE_STATE(&systh_header, &systh_data, NULL);
 *
 *      ...
 *  }
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
#define MIPI_SYST_INIT_HANDLE_STATE(s, h, p) \
	mipi_syst_init_handle((s), (h), (p), 0)

/**
 * Initialize non-heap SyS-T handle
 *
 * This function is used in platforms that don't support heap allocations.
 * The caller has to provide a pointer to a memory location that can hold
 * a mipi_syst_handle data structure.
 *
 * @param h Pointer to handle data structure on the stack or data segment.
 * @param p Pointer to mipi_syst_origin structure with client
 *          identifying information, or NULL if not used
 *
 * Example
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 * static struct mipi_syst_handle systh_data;
 *
 * void foo()
 * {
 *      struct mipi_syst_handle* svh = MIPI_SYST_INIT_HANDLE(&systh_data, NULL);
 *
 *      ...
 *  }
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
#define MIPI_SYST_INIT_HANDLE(h, p) \
	MIPI_SYST_INIT_HANDLE_STATE((struct mipi_syst_header*)0, (h), (p))

#if defined(MIPI_SYST_PCFG_ENABLE_HEAP_MEMORY)
/**
 * Heap allocate and initialize a new SyS-T handle for a custom global state
 *
 * This function is only supported if the platform supports the
 * feature #MIPI_SYST_PCFG_ENABLE_HEAP_MEMORY. Use
 * #MIPI_SYST_INIT_HANDLE if no heap support is enabled.
 *
 * @param s Pointer to SyS-T state header variable
 * @param p Pointer to mipi_syst_origin structure with client
 *          identifying information, or NULL if not used
 *
 * Example:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 * extern struct mipi_syst_header systh_header;
 *
 * static const struct mipi_syst_origin origin =
 *    MIPI_SYST_GEN_ORIGIN_GUID(0x494E5443, 0xA2AE, 0x4C70, 0xABB5, 0xD1A79E9CEA35, 1);
 *
 * void foo()
 * {
 *      struct mipi_syst_handle* svh;
 *
 *      svh = MIPI_SYST_ALLOC_HANDLE_STATE(&systh_header, &origin);
 *
 *      ...
 *  }
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
#define MIPI_SYST_ALLOC_HANDLE_STATE(s, p) \
	mipi_syst_init_handle(\
		(s),\
		(struct mipi_syst_handle*)MIPI_SYST_HEAP_MALLOC(sizeof(struct mipi_syst_handle)),\
		(p), 1)

/**
 * Heap allocate and initialize a new SyS-T handle
 * This function is only supported if the platform supports the
 * feature #MIPI_SYST_PCFG_ENABLE_HEAP_MEMORY. Use
 * #MIPI_SYST_INIT_HANDLE if no heap support is enabled.
 *
  * @param p Pointer to mipi_syst_origin structure with client
 *          identifying information, or NULL if not used
 *
 * Example:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 *  * static const struct mipi_syst_origin origin =
 *    MIPI_SYST_GEN_ORIGIN_GUID(0x494E5443, 0xA2AE, 0x4C70, 0xABB5, 0xD1A79E9CEA35, 1);
 *
 *
 * void foo()
 * {
 *      struct mipi_syst_handle* svh = MIPI_SYST_ALLOC_HANDLE(&origin);
 *
 *      ...
 *  }
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
#define MIPI_SYST_ALLOC_HANDLE(p) \
	MIPI_SYST_ALLOC_HANDLE_STATE((struct mipi_syst_header*)0, (p))

#else
#define MIPI_SYST_ALLOC_HANDLE(p) \
CFG_ERROR_ALLOC_HANDLE_CALLED_WITHOUT_PCFG_ENABLE_HEAP_MEMORY
#define MIPI_SYST_ALLOC_HANDLE_STATE(s, p) \
CFG_ERROR_SYST_ALLOC_HANDLE_STATE_CALLED_WITHOUT_PCFG_ENABLE_HEAP_MEMORY
#endif


#if defined(MIPI_SYST_PCFG_LENGTH_FIELD)  || defined (_DOXYGEN_)
 /**
 * Enable or disable length generation over the given SyS-T handle
 *
 * @param h SyS-T handle from #MIPI_SYST_INIT_HANDLE or #MIPI_SYST_ALLOC_HANDLE
 * @param v 0 disable length field, otherwise enable length
 *
 * Example:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 * struct mipi_syst_handle* svh = MIPI_SYST_ALLOC_HANDLE(NULL);
 *
 * // enable checksums
 * //
 * MIPI_SYST_ENABLE_HANDLE_LENGTH(svh, 1);
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
#define MIPI_SYST_ENABLE_HANDLE_LENGTH(h, v) \
	((h) && ((h)->systh_tag.et_length = (v) ? 1 : 0))
#else
#define MIPI_SYST_ENABLE_HANDLE_LENGTH(h, v)
#endif

 /**
 * Get length field generation state from given SyS-T handle
 *
 * @param h SyS-T handle from #MIPI_SYST_INIT_HANDLE or #MIPI_SYST_ALLOC_HANDLE
 * @return 0 if disabled, otherwise enabled
 *
 * Example:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 * struct mipi_syst_handle* svh = MIPI_SYST_ALLOC_HANDLE(NULL);
 *
 *  if (MIPI_SYST_GET_HANDLE_LENGTH(svh)) {
 *      // length field enabled ...
 *  }
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
#define MIPI_SYST_GET_HANDLE_LENGTH(h) \
	(((h) &&(h)->systh_tag.et_length)  ? 1 : 0)

#if defined(MIPI_SYST_PCFG_ENABLE_CHECKSUM)
/**
 * Enable or disable checksum generation over the given SyS-T handle
 *
 * @param h SyS-T handle from #MIPI_SYST_INIT_HANDLE or #MIPI_SYST_ALLOC_HANDLE
 * @param v 0 disable checksum, otherwise enable checksum
 *
 * Example:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 * struct mipi_syst_handle* svh = MIPI_SYST_ALLOC_HANDLE(NULL);
 *
 * // enable checksums
 * //
 * MIPI_SYST_ENABLE_HANDLE_CHECKSUM(svh, 1);
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
#define MIPI_SYST_ENABLE_HANDLE_CHECKSUM(h, v) \
	((h) && ((h)->systh_tag.et_chksum = (v) ? 1 : 0))

 /**
 * Get checksum generation state from given SyS-T handle
 *
 * @param h SyS-T handle from #MIPI_SYST_INIT_HANDLE or #MIPI_SYST_ALLOC_HANDLE
 * @return 0 if disabled, otherwise enabled
 *
 * Example:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 * struct mipi_syst_handle* svh = MIPI_SYST_ALLOC_HANDLE(NULL);
 *
 *  if (MIPI_SYST_GET_HANDLE_CHECKSUM(svh)) {
 *      // checksums enabled ...
 *  }
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
#define MIPI_SYST_GET_HANDLE_CHECKSUM(h) \
	(((h) && (h)->systh_tag.et_chksum)  ? 1 : 0)
#endif

/**
 * Change module and unit ID of the given SyS-T handle.
 *
 * @param h SyS-T handle from #MIPI_SYST_INIT_HANDLE or #MIPI_SYST_ALLOC_HANDLE
 * @param module module id (0..0x7F)
 * @param unit unit id (0x0..0xF)
 *
 * @see #MIPI_SYST_SET_HANDLE_GUID_UNIT mipi_syst_msg_tag
 *
 * Example:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 * struct mipi_syst_handle* svh = MIPI_SYST_ALLOC_HANDLE(NULL);
 *
 * // tag message with 5:1 as module:unit id pair
 * //
 * MIPI_SYST_SET_MODULE_UNIT(svh, 5, 1);
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
#define MIPI_SYST_SET_HANDLE_MODULE_UNIT(h, module, unit) \
do { \
	if (h) { \
		(h)->systh_tag.et_guid = 0; \
		(h)->systh_tag.et_modunit = \
			_MIPI_SYST_MK_MODUNIT_ORIGIN((module), (unit)); \
	} \
} while (0)

#if defined(MIPI_SYST_PCFG_ENABLE_ORIGIN_GUID)

/**
 * Change GUID and unit ID of the given SyS-T handle.
 *
 * @param h SyS-T handle from #MIPI_SYST_INIT_HANDLE or #MIPI_SYST_ALLOC_HANDLE
 * @param guid mipi_syst_guid data structure
 * @param unit unit id (0x0..0xF)
 *
 * @see mipi_syst_msg_tag
 *
 * Example:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 * //
 * static const struct mipi_syst_guid guid =
 *   MIPI_SYST_GEN_GUID(0x494E5443, 0xA2AE, 0x4C70, 0xABB5, 0xD1A79E9CEA35);
 *
 * void foo()
 * {
 *      struct mipi_syst_handle* svh = MIPI_SYST_ALLOC_HANDLE(NULL);
 *
 *      // tag message with  GUID and 1 as unit id
 *      //
 *      MIPI_SYST_SET_HANDLE_GUID_UNIT(svh, guid, 1);
 *  }
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
#define MIPI_SYST_SET_HANDLE_GUID_UNIT(h, guid, unit) \
do { \
	if (h) { \
		(h)->systh_guid.u.ll[0] = MIPI_SYST_HTOLE64((guid).u.ll[0]); \
		(h)->systh_guid.u.ll[1] = MIPI_SYST_HTOLE64((guid).u.ll[1]); \
		(h)->systh_tag.et_modunit = (unit); \
		(h)->systh_tag.et_guid = 1;\
	} \
} while (0)

 /**
 * Change message origin using mipi_sys_t_origin structure
 *
 * @param h SyS-T handle from #MIPI_SYST_INIT_HANDLE or #MIPI_SYST_ALLOC_HANDLE
 * @param o origin structure
 *
 * @see #MIPI_SYST_SET_HANDLE_GUID_UNIT mipi_syst_origin
 *
 * Example:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 *  // Define origin used by this example as the message client ID
 *  //
 * static const struct mipi_syst_origin origin =
 * MIPI_SYST_GEN_ORIGIN_GUID(0x494E5443, 0xA2AE, 0x4C70, 0xABB5, 0xD1A79E9CEA35, 1);
  *
 * struct mipi_syst_handle* svh = MIPI_SYST_ALLOC_HANDLE(NULL);
 *
 * // tag message with  GUID and 1 as unit id
 * //
 * MIPI_SYST_SET_HANDLE_ORIGIN(svh, origin);
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
#define MIPI_SYST_SET_HANDLE_ORIGIN(h, o) \
do { \
	/* Guid byte 8 bit 7 indicates if full GUID or    \
	 * short module header is used. This bit is never \
	 * 0 for RFC4122 compliant guids.                 \
	 */                                               \
	if (0 != ((o).guid.u.b[8] & 0x80)) { \
		MIPI_SYST_SET_HANDLE_GUID_UNIT( \
			(h), \
			(o).guid, \
			(o).unit); \
	} else { \
		MIPI_SYST_SET_HANDLE_MODULE_UNIT( \
			(h), \
			(o).guid.u.b[8], \
			(o).unit); \
	} \
} while(0)

#else
#define MIPI_SYST_SET_HANDLE_GUID_UNIT(p, g, u) \
CFG_ERROR_SET_HANDLE_GUID_UNIT_WITHOUT_MIPI_SYST_PCFG_ENABLE_ORIGIN_GUID
#endif

#if defined(MIPI_SYST_PCFG_ENABLE_TIMESTAMP)
/**
 * Enable protocol specific time stamp support on this SyS-T handle.
 *
 * @param h SyS-T handle from #MIPI_SYST_INIT_HANDLE or #MIPI_SYST_ALLOC_HANDLE
 * @param v 0 disable, 1 enable
 *
 * Example:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 * void foo()
 * {
 *      struct mipi_syst_handle* svh = MIPI_SYST_ALLOC_HANDLE(NULL);

 *      MIPI_SYST_ENABLE_HANDLE_TIMESTAMP(svh, 1);
 *  }
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
#define MIPI_SYST_ENABLE_HANDLE_TIMESTAMP(h, v) \
	((h) && ((h)->systh_tag.et_timestamp = (v) ? 1 : 0))

/**
 * Get timestamp generation state from given SyS-T handle
 *
 * @param h SyS-T handle from #MIPI_SYST_INIT_HANDLE or #MIPI_SYST_ALLOC_HANDLE
 * @return 0 if disabled, otherwise enabled
 *
 * Example:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 * struct mipi_syst_handle* svh = MIPI_SYST_ALLOC_HANDLE(NULL);
 *
 *  if (MIPI_SYST_GET_HANDLE_TIMESTAMP(svh)) {
 *      // timestamp enabled ...
 *  }
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
#define MIPI_SYST_GET_HANDLE_TIMESTAMP(h) \
	(((h) && (h)->systh_tag.et_timestamp)  ? 1 : 0)

#else
#define MIPI_SYST_ENABLE_HANDLE_TIMESTAMP(h,v ) \
CFG_ERROR_MIPI_SYST_ENABLE_HANDLE_TIMESTAMP_WITHOUT_MIPI_SYST_PCFG_ENABLE_TIMESTAMP
#endif



/**
 * Delete a SyS-T handle
 *
 * @param h SyS-T handle from #MIPI_SYST_INIT_HANDLE or #MIPI_SYST_ALLOC_HANDLE
 *
 * Example:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 * void foo()
 * {
 *      struct mipi_syst_handle* svh = MIPI_SYST_ALLOC_HANDLE(NULL);
 *
 *      // ...
 *
 *      MIPI_SYST_DELETE_HANDLE(svh);
 *  }
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
#define MIPI_SYST_DELETE_HANDLE(h) \
	mipi_syst_delete_handle(h)

/** @} */

/**
 * Pass null to instrumentation API to skip location information generation.
 */
#define MIPI_SYST_NOLOCATION  (struct mipi_syst_msglocation *)0

/**
 * @defgroup WriteAPI Raw Data Writing Macros
 * @ingroup ApiSets
 *
 * Raw data writing macros:
 * @{
 */

/**
 * Send short data message with 28-bit payload.<BR>
 *
 * This API is indented for space and speed restricted environments that
 * cannot support the more complex message types.
 * @param h mipi_syst_handle* SyS-T handle
 * @param v 28-bit output value. The upper 4 bits  of the parameter value
 *          are discarded
 *
 * Example:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 * #define INIT_START 1
 * #define INIT_DONE  2
 *
 * foo()
 * {
 *      MIPI_SYST_SHORT32(svh, INIT_START);
 *      // processing ..
 *      //
 *
 *      MIPI_SYST_SHORT32(svh, INIT_DONE);
 *      // ..
 * }
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
#define MIPI_SYST_SHORT32(h, v) \
	MIPI_SYST_OUTPUT_D32MTS(h,\
		(((v)<<4) | (mipi_syst_u32)MIPI_SYST_TYPE_SHORT32))

/**
 * Send short data message with 60-bit payload.<BR>
 *
 * This API is indented for space and speed restricted environments that
 * cannot support the more complex message types.
 * @param h mipi_syst_handle* SyS-T handle
 * @param v 60-bit output value. The upper 4 bits of the parameter value
 *          are discarded.
 *
 * Example:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 * #define INIT_START 1
 * #define INIT_DONE  2
 *
 * foo()
 * {
 *      MIPI_SYST_SHORT64(svh, INIT_START);
 *      // processing ..
 *      //
 *
 *      MIPI_SYST_SHORT64(svh, INIT_DONE);
 *      // ..
 * }
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
#define MIPI_SYST_SHORT64(h, v) \
	MIPI_SYST_OUTPUT_D64MTS(h,\
		(((v)<<4) | (mipi_syst_u64)MIPI_SYST_TYPE_SHORT64))


#if defined(MIPI_SYST_PCFG_ENABLE_TIMESTAMP)

/**
 * Send a timesync Message that helps the decode Software to synchronize the
 * transport protocol clock value (e.g., the MIPI STP timestamp) with trace
 * hardware generated clocks. This API depends on the platform define
 * #MIPI_SYST_PCFG_ENABLE_TIMESTAMP.
 * @param h mipi_syst_handle* SyS-T handle
 * @param c 64-bit clock value from message timestamp clock
 * @param f 64-bit message timestamp clock frequency value in Herz
 */
#define MIPI_SYST_CLOCK_SYNC(h, c, f)\
	mipi_syst_write_clock((h),\
			MIPI_SYST_NOLOCATION,\
			MIPI_SYST_CLOCK_TRANSPORT_SYNC,\
			(c),\
			(f))

MIPI_SYST_EXPORT void MIPI_SYST_CALLCONV
mipi_syst_write_clock(struct mipi_syst_handle* svh,
		struct mipi_syst_msglocation* loc,
		enum mipi_syst_subtype_clock fmt,
		mipi_syst_u64 clock,
		mipi_syst_u64 freq);
#endif /* defined(MIPI_SYST_PCFG_ENABLE_TIMESTAMP) */

#if defined(MIPI_SYST_PCFG_ENABLE_WRITE_API)

/**
 * Send raw data message with user defined payload.<BR>
 *
 * @param h mipi_syst_handle* SyS-T handle
 * @param sev mipi_syst_severity severity level (0..7)
 * @param id 8-bit message protocol ID. This ID is used to identify
 *            the data protocol in the payload of this message for pretty
 *            printing in trace decoders. The protocol ID must uniquely
 *            identify the contents within the SyS-T handle's
 *            #mipi_syst_msg_tag.et_modunit id or
 *            #mipi_syst_handle.systh_guid value. The protocol decoder
 *            uses this value to route data from `MIPI_SYST_WRITE()` calls
 *            to other cascaded decoders to actually
 *            process the contents.
 * @param p pointer to raw data to send
 * @param len 16-bit length of data to send
 *
 * Example:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 *
 * // emit raw data under user defined decode ID 5
 * //
 * unsigned char data[] = { 0,1,2,3,4,5,6,7,8,9 };
 *
 * MIPI_SYST_WRITE(systh, MIPI_SYST_SEVERITY_INFO, 5, data, sizeof(data));
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
#define MIPI_SYST_WRITE(h, sev, id, p, len) \
	mipi_syst_write_raw_message((h), \
		MIPI_SYST_NOLOCATION, (sev), (id), (p), (len))

#if defined(MIPI_SYST_PCFG_ENABLE_LOCATION_RECORD)

#if defined(MIPI_SYST_PCFG_ENABLE_LOCATION_ADDRESS)

/** @copydoc MIPI_SYST_WRITE(h, sev, id, p, len) */
#define MIPI_SYST_WRITE_LOCADDR(h, sev, id, p, len) \
	mipi_syst_write_raw_message((h),\
		mipi_syst_make_address_location((h), mipi_syst_return_addr()),\
		(sev), (id), (p), (len))

#endif /* defined(MIPI_SYST_PCFG_ENABLE_LOCATION_ADDRESS) */

/**
* Send raw data message with user defined payload.<BR>
*
* @param h mipi_syst_handle* SyS-T handle
* @param sev mipi_syst_severity severity level (0..7)
* @param file file ID as 16-bit value
* @param id 8-bit message protocol ID. This ID is used to identify
*            the data protocol in the payload of this message for pretty
*            printing in trace decoders. The protocol ID must uniquely
*            identify the contents within the SyS-T handle's
*            #mipi_syst_msg_tag.et_modunit id or
*            #mipi_syst_handle.systh_guid value. The protocol decoder
*            uses this value to route data from `MIPI_SYST_WRITE()` calls
*            to other cascaded decoders to actually
*            process the contents.
* @param p pointer to raw data to send
* @param len 16-bit length of data to send
*
* Example:
* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
*
* #define THIS_FILE 100
*
* // emit raw data under user defined decode ID 5
* //
* unsigned char data[] = { 0,1,2,3,4,5,6,7,8,9 };
*
* MIPI_SYST_WRITE_LOC16(systh, MIPI_SYST_SEVERITY_INFO,
                        THIS_FILE, 5, data, sizeof(data));
* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
#define MIPI_SYST_WRITE_LOC16(h, sev, file, id,  p, len) \
	mipi_syst_write_raw_message((h),\
		mipi_syst_make_file_location32((h), \
				(mipi_syst_u16)(file), \
				(mipi_syst_u16)MIPI_SYST_LINE),\
				(sev), (id), (p), (len))

/**
* Send raw data message with user defined payload.<BR>
*
* @param h mipi_syst_handle* SyS-T handle
* @param sev mipi_syst_severity severity level (0..7)
* @param file file ID as 32-bit value
* @param id 8-bit message protocol ID. This ID is used to identify
*            the data protocol in the payload of this message for pretty
*            printing in trace decoders. The protocol ID must uniquely
*            identify the contents within the SyS-T handle's
*            #mipi_syst_msg_tag.et_modunit id or
*            #mipi_syst_handle.systh_guid value. The protocol decoder
*            uses this value to route data from `MIPI_SYST_WRITE()` calls
*            to other cascaded decoders to actually
*            process the contents.
* @param p pointer to raw data to send
* @param len 16-bit length of data to send
*
* Example:
* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
*
* #define THIS_FILE 100
*
* // emit raw data under user defined decode ID 5
* //
* unsigned char data[] = { 0,1,2,3,4,5,6,7,8,9 };
*
* MIPI_SYST_WRITE_LOC16(systh, MIPI_SYST_SEVERITY_INFO,
THIS_FILE, 5, data, sizeof(data));
* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
#define MIPI_SYST_WRITE_LOC32(h, sev, file, id, p, len) \
	mipi_syst_write_raw_message((h),\
		mipi_syst_make_file_location64((h), \
				(mipi_syst_u32)(file), \
				(mipi_syst_u32)MIPI_SYST_LINE),\
				(sev), (id), (p), (len))

#endif	/* defined(MIPI_SYST_PCFG_ENABLE_LOCATION_RECORD) */

/** @} */

MIPI_SYST_EXPORT void MIPI_SYST_CALLCONV
mipi_syst_write_raw_message(struct mipi_syst_handle* svh,
		struct mipi_syst_msglocation* loc,
		enum mipi_syst_severity severity,
		mipi_syst_u8 subtype,
		const void *data, mipi_syst_u16 length);

#endif	/* #if defined(MIPI_SYST_PCFG_ENABLE_WRITE_API) */

/**
* @defgroup BuildAPI Build Number Message Macros
* @ingroup ApiSets
*
* Build Number writing macros:
* @{
*/
/**

* Send compact client build number message with 22-bit payload.<BR>
*
* This API is indented for space and speed restricted environments that
* cannot support the more complex message types.
* @param h mipi_syst_handle* SyS-T handle
* @param n 22-bit output value. The upper 10 bits of the parameter value
*          are discarded.
*
* Example:
* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
* #define MY_BUILDID  0x3FFFFF
*
* foo()
* {
*      MIPI_SYST_BUILD_COMPACT32(svh, MY_BUILDID);
* }
* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
#define MIPI_SYST_BUILD_COMPACT32(h, n) \
	if (h) { MIPI_SYST_OUTPUT_D32MTS(h, (\
		(((n) & 0x000FFFFF) << 4)  | \
		((n) & 0xFFF00000) << 10));}

/**
* Send compact client build number message with 54-bit payload.<BR>
*
* This API is indented for space and speed restricted environments that
* cannot support the more complex message types.
* @param h mipi_syst_handle* SyS-T handle
* @param n 54-bit output value. The upper 10 bits of the parameter value
*          are discarded.
*
* Example:
* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
* #define MY_BUILDID  0x3FFFFFFFFFFFFFull
*
* foo()
* {
*      MIPI_SYST_BUILD_COMPACT64(svh, MY_BUILDID);
* }
* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
#define MIPI_SYST_BUILD_COMPACT64(h, n) \
	if (h) { MIPI_SYST_OUTPUT_D64MTS(h, (\
		(0x00000000001000000ull | \
		 ((n) & 0x00000000000FFFFFull) << 4)  | \
		 ((n) & 0xFFFFFFFFFFF00000ull) << 10));}


#if defined(MIPI_SYST_PCFG_ENABLE_BUILD_API)

/**
 * Send client build number data message
 *
 * @param h mipi_syst_handle* SyS-T handle
 * @param sev mipi_syst_severity severity level (0..7)
 * @param bld 64-bit build ID.
 * @param txt pointer to textual build description
 * @param len 16-bit length of text data to send
 *
 * Example:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 *
 * #define BUILD_NUMBER 0x1122334455667788ull
 * #define BUILD_BANNER "MyApp-v1.2.3 (April 16 2018)"
 *
 * MIPI_SYST_BUILD(systh, MIPI_SYST_SEVERITY_INFO,
 *      BUILD_NUMBER,
 *      BUILD_BANNER, sizeof(BUILD_BANNER)
 * );
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
#define MIPI_SYST_BUILD(h, sev, bld, txt, len) \
	mipi_syst_write_build_message((h), \
		MIPI_SYST_NOLOCATION, (sev), (bld), (txt), (len))

#if defined(MIPI_SYST_PCFG_ENABLE_LOCATION_RECORD)

#if defined(MIPI_SYST_PCFG_ENABLE_LOCATION_ADDRESS)

 /**
 * Send client build number data message
 *
 * @param h mipi_syst_handle* SyS-T handle
 * @param sev mipi_syst_severity severity level (0..7)
 * @param bld 64-bit build ID.
 * @param txt pointer to textual build description
 * @param len 16-bit length of text data to send
 *
 * Example:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 *
 * #define BUILD_NUMBER 0x1122334455667788ull
 * #define BUILD_BANNER "MyApp-v1.2.3 (April 16 2018)"
 *
 * MIPI_SYST_BUILD_LOCADDR(systh, MIPI_SYST_SEVERITY_INFO,
 *      BUILD_NUMBER,
 *      BUILD_BANNER, sizeof(BUILD_BANNER)
 * );
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
#define MIPI_SYST_BUILD_LOCADDR(h, sev, bld, txt, len) \
	mipi_syst_write_build_message((h),\
		mipi_syst_make_address_location((h), mipi_syst_return_addr()),\
		(sev), (bld), (txt), (len))

#endif /* defined(MIPI_SYST_PCFG_ENABLE_LOCATION_ADDRESS) */

 /**
 * Send client build number data message
 *
 * @param h mipi_syst_handle* SyS-T handle
 * @param sev mipi_syst_severity severity level (0..7)
 * @param file file ID as 16-bit value
 * @param bld 64-bit build ID.
 * @param txt pointer to textual textual build description
 * @param len 16-bit length of text data to send
 *
 * Example:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 *
 * #define BUILD_NUMBER 0x1122334455667788ull
 * #define BUILD_BANNER "MyApp-v1.2.3 (April 16 2018)"
 *
 * MIPI_SYST_BUILD_LOC16(systh, MIPI_SYST_SEVERITY_INFO, 10,
 *      BUILD_NUMBER,
 *      BUILD_BANNER, sizeof(BUILD_BANNER)
 * );
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
#define MIPI_SYST_BUILD_LOC16(h, sev, file, bld,  txt, len) \
	mipi_syst_write_build_message((h),\
		mipi_syst_make_file_location32((h), \
				(mipi_syst_u16)(file), \
				(mipi_syst_u16)MIPI_SYST_LINE),\
				(sev), (bld), (txt), (len))

 /**
 * Send client build number data message
 *
 * @param h mipi_syst_handle* SyS-T handle
 * @param sev mipi_syst_severity severity level (0..7)
 * @param file file ID as 32-bit value
 * @param bld 64-bit build ID.
 * @param txt pointer to textual build description
 * @param len 16-bit length of text data to send
 *
 * Example:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 *
 * #define BUILD_NUMBER 0x1122334455667788ull
 * #define BUILD_BANNER "MyApp-v1.2.3 (April 16 2018)"
 *
 * MIPI_SYST_BUILD_LOC32(systh, MIPI_SYST_SEVERITY_INFO, 10,
 *      BUILD_NUMBER,
 *      BUILD_BANNER, sizeof(BUILD_BANNER)
 * );
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
#define MIPI_SYST_BUILD_LOC32(h, sev, file, bld, txt, len) \
	mipi_syst_write_build_message((h),\
		mipi_syst_make_file_location64((h), \
				(mipi_syst_u32)(file), \
				(mipi_syst_u32)MIPI_SYST_LINE),\
				(sev), (bld), (txt), (len))

#endif	/* defined(MIPI_SYST_PCFG_ENABLE_LOCATION_RECORD) */

/** @} */

MIPI_SYST_EXPORT void MIPI_SYST_CALLCONV
mipi_syst_write_build_message(struct mipi_syst_handle* svh,
		struct mipi_syst_msglocation* loc,
		enum mipi_syst_severity severity,
		mipi_syst_u64 id,
		const char *text, mipi_syst_u16 length);
#endif /* defined(MIPI_SYST_PCFG_ENABLE_BUILD_API) */

/* public API prototypes */

MIPI_SYST_EXPORT void MIPI_SYST_CALLCONV
mipi_syst_init(struct mipi_syst_header* header, mipi_syst_inithook_t pfinit,
		const void * init_data);
MIPI_SYST_EXPORT void MIPI_SYST_CALLCONV mipi_syst_destroy(struct mipi_syst_header*
		header,
		mipi_syst_destroyhook_t
		pfdestroy);
MIPI_SYST_EXPORT struct mipi_syst_handle* MIPI_SYST_CALLCONV
mipi_syst_init_handle(struct mipi_syst_header* header, struct mipi_syst_handle* svh,
		const struct mipi_syst_origin *origin,
		mipi_syst_u32 heap);
MIPI_SYST_EXPORT void MIPI_SYST_CALLCONV mipi_syst_delete_handle(struct mipi_syst_handle*
		svh);

#if defined(MIPI_SYST_PCFG_ENABLE_LOCATION_ADDRESS)
MIPI_SYST_EXPORT void *MIPI_SYST_CALLCONV mipi_syst_return_addr(void);
#endif

/* Inline helper functions
 * These functions are only attempted to be inlined if the define
 * MIPI_SYST_PCFG_ENABLE_INLINE is defined.
 * Undefining it causes true functions calls to be used instead.
 * This is useful if saving code space matters more then speed.
 */
#if defined(MIPI_SYST_PCFG_ENABLE_INLINE)
#include "./inline.h"
#else
MIPI_SYST_INLINE struct mipi_syst_msglocation* MIPI_SYST_CALLCONV
mipi_syst_make_file_location32(struct mipi_syst_handle* h, mipi_syst_u16 f,
		mipi_syst_u16 l);
MIPI_SYST_INLINE struct mipi_syst_msglocation* MIPI_SYST_CALLCONV
mipi_syst_make_file_location64(struct mipi_syst_handle* h, mipi_syst_u32 f,
		mipi_syst_u32 l);
MIPI_SYST_INLINE struct mipi_syst_msglocation* MIPI_SYST_CALLCONV
mipi_syst_make_address_location(struct mipi_syst_handle* h, void *p);

MIPI_SYST_INLINE void MIPI_SYST_CALLCONV mipi_syst_make_param0(struct mipi_syst_handle* h);
MIPI_SYST_INLINE void MIPI_SYST_CALLCONV
mipi_syst_make_param1(struct mipi_syst_handle* h, mipi_syst_u32 p1);
MIPI_SYST_INLINE void MIPI_SYST_CALLCONV
mipi_syst_make_param2(struct mipi_syst_handle* h, mipi_syst_u32 p1,
		mipi_syst_u32 p2);
MIPI_SYST_INLINE void MIPI_SYST_CALLCONV mipi_syst_make_param3(struct mipi_syst_handle* h,
		mipi_syst_u32 p1,
		mipi_syst_u32 p2,
		mipi_syst_u32 p3);
MIPI_SYST_INLINE void MIPI_SYST_CALLCONV mipi_syst_make_param4(struct mipi_syst_handle* h,
		mipi_syst_u32 p1,
		mipi_syst_u32 p2,
		mipi_syst_u32 p3,
		mipi_syst_u32 p4);
MIPI_SYST_INLINE void MIPI_SYST_CALLCONV mipi_syst_make_param5(struct mipi_syst_handle* h,
		mipi_syst_u32 p1,
		mipi_syst_u32 p2,
		mipi_syst_u32 p3,
		mipi_syst_u32 p4,
		mipi_syst_u32 p5);
MIPI_SYST_INLINE void MIPI_SYST_CALLCONV mipi_syst_make_param6(struct mipi_syst_handle* h,
		mipi_syst_u32 p1,
		mipi_syst_u32 p2,
		mipi_syst_u32 p3,
		mipi_syst_u32 p4,
		mipi_syst_u32 p5,
		mipi_syst_u32 p6);
#endif


#if defined(MIPI_SYST_PCFG_ENABLE_STRING_API)

#define _MIPI_SYST_ASSERT_DEBUG_STRING(cond) \
	MIPI_SYST_FILE ":" _MIPI_SYST_CPP_TOSTR(MIPI_SYST_LINE) " " #cond

/**
 * @defgroup StringAPI String Generating Macros
 * @ingroup ApiSets
 *
 * String generating macros:
 * @{
 */

/**
 * Send UTF-8 character string with given severity and length.<BR>
 *
 * @param h mipi_syst_handle* SyS-T handle
 * @param severity mipi_syst_severity severity level (0..7)
 * @param str const mipi_syst_u8 * pointer to UTF-8 string bytes
 * @param len mipi_syst_u16 number of bytes to emit
 *
 * Example:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 * #define FILE_ID 0xabdc // this file id
 * char * str = "Hello World";
 *
 * MIPI_SYST_DEBUG(systh, MIPI_SYST_SEVERITY_INFO, str, strlen(str));
 * MIPI_SYST_DEBUG_LOCADDR(systh, MIPI_SYST_SEVERITY_INFO, str, strlen(str));
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
#define MIPI_SYST_DEBUG(h, severity, str, len) \
	mipi_syst_write_debug_string((h), MIPI_SYST_NOLOCATION, \
			MIPI_SYST_STRING_GENERIC, \
			(severity), (len), (str))

/**
 * Send function enter string message.
 *
 * @param h mipi_syst_handle* SyS-T handle
 * @param severity mipi_syst_severity severity level (0..7)
 *
 * Example:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 * void foo()
 * {
 *    MIPI_SYST_FUNC_ENTER(systh, MIPI_SYST_SEVERITY_INFO);
 *    // body
 *    MIPI_SYST_FUNC_EXIT(systh, MIPI_SYST_SEVERITY_INFO);
 * }
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
#define MIPI_SYST_FUNC_ENTER(h, severity) \
	mipi_syst_write_debug_string((h), MIPI_SYST_NOLOCATION, \
			MIPI_SYST_STRING_FUNCTIONENTER, \
			(severity), sizeof(MIPI_SYST_FUNCTION_NAME),\
			MIPI_SYST_FUNCTION_NAME)

/**
 * Send function exit string message.
 * The payload is the UTF-8 function name of the surrounding function
 *
 * @param h mipi_syst_handle* SyS-T handle
 * @param severity mipi_syst_severity severity level (0..7)
 *
 * Example:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 * void foo()
 * {
 *    MIPI_SYST_FUNC_ENTER(systh, MIPI_SYST_SEVERITY_INFO);
 *    // body
 *    MIPI_SYST_FUNC_EXIT(systh, MIPI_SYST_SEVERITY_INFO);
 * }
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
#define MIPI_SYST_FUNC_EXIT(h, severity) \
	mipi_syst_write_debug_string((h), MIPI_SYST_NOLOCATION, \
			MIPI_SYST_STRING_FUNCTIONEXIT, \
			(severity), sizeof(MIPI_SYST_FUNCTION_NAME),\
			MIPI_SYST_FUNCTION_NAME)

/**
 * Send assertion notice string message if the passed condition is false.
 * The notice includes the failing expression string as its payload.
 *
 * @param h mipi_syst_handle* SyS-T handle
 * @param severity mipi_syst_severity severity level (0..7)
 * @param cond boolean condition to verify
 *
 * Example:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 * void foo(void * p)
 * {
 *    MIPI_SYST_ASSERT(systh, MIPI_SYST_SEVERITY_ERROR, p != NULL);
 * }
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
#define MIPI_SYST_ASSERT(h, severity, cond) \
	{\
	if (!(cond)) {\
		mipi_syst_write_debug_string((h),\
			MIPI_SYST_NOLOCATION,\
			MIPI_SYST_STRING_ASSERT, \
			(severity), sizeof(_MIPI_SYST_ASSERT_DEBUG_STRING(cond)),\
			_MIPI_SYST_ASSERT_DEBUG_STRING(cond));\
		} \
	}


#if defined(MIPI_SYST_PCFG_ENABLE_LOCATION_RECORD)

 /**
 * Send UTF-8 character string with given severity and length.<BR>
 *
 * @param h mipi_syst_handle* SyS-T handle
 * @param severity mipi_syst_severity severity level (0..7)
 * @param file 16-bit user defined file ID
 * @param str const mipi_syst_u8 * pointer to UTF-8 string bytes
 * @param len mipi_syst_u16 number of bytes to emit
 *
 * Example:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 * #define FILE_ID 0xabdc // this file id
 * char * str = "Hello World";
 *
 * MIPI_SYST_DEBUG_LOC16(systh, MIPI_SYST_SEVERITY_INFO, FILE_ID, str, strlen(str));
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
#define MIPI_SYST_DEBUG_LOC16(h, severity, file, str, len) \
	mipi_syst_write_debug_string((h), \
		mipi_syst_make_file_location32((h), \
			(mipi_syst_u16)(file), \
			(mipi_syst_u16)MIPI_SYST_LINE),\
		MIPI_SYST_STRING_GENERIC, \
		(severity), (len), (str))
 /**
 * Send UTF-8 character string with given severity and length.<BR>
 *
 * @param h mipi_syst_handle* SyS-T handle
 * @param severity mipi_syst_severity severity level (0..7)
 * @param file 32-bit user defined file ID
 * @param str const mipi_syst_u8 * pointer to UTF-8 string bytes
 * @param len mipi_syst_u16 number of bytes to emit
 *
 * Example:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 * #define FILE_ID 0x0000abdc // this file id
 * char * str = "Hello World";
 *
 * MIPI_SYST_DEBUG_LOC32(systh, MIPI_SYST_SEVERITY_INFO, FILE_ID, str, strlen(str));
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
#define MIPI_SYST_DEBUG_LOC32(h, severity, file, str, len) \
	mipi_syst_write_debug_string((h), \
		mipi_syst_make_file_location64((h), \
			(mipi_syst_u32)(file), \
			(mipi_syst_u32)MIPI_SYST_LINE),\
		MIPI_SYST_STRING_GENERIC, \
		(severity), (len), (str))

 /**
 * Send function enter string message.
 * The payload is the UTF-8 function name of the surrounding function.
 *
 * @param h mipi_syst_handle* SyS-T handle
 * @param severity mipi_syst_severity severity level (0..7)
 * @param file 16-bit user defined file ID
 *
 * Example:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 * void foo()
 * {
 *    MIPI_SYST_FUNC_ENTER_LOC16(systh, 10, MIPI_SYST_SEVERITY_INFO);
 *    // body
 *    MIPI_SYST_FUNC_EXIT_LOC16(systh, 10, MIPI_SYST_SEVERITY_INFO);
 * }
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
#define MIPI_SYST_FUNC_ENTER_LOC16(h, severity, file) \
	mipi_syst_write_debug_string((h),\
		mipi_syst_make_file_location32((h), \
			(mipi_syst_u16)(file), \
			(mipi_syst_u16)MIPI_SYST_LINE),\
		MIPI_SYST_STRING_FUNCTIONENTER, \
		(severity), sizeof(MIPI_SYST_FUNCTION_NAME),\
		MIPI_SYST_FUNCTION_NAME)

 /**
 * Send function exit string message.
 * The payload is the UTF-8 function name of the surrounding function.
 *
 * @param h mipi_syst_handle* SyS-T handle
 * @param severity mipi_syst_severity severity level (0..7)
 * @param file 16-bit user defined file ID
 *
 * Example:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 * void foo()
 * {
 *    MIPI_SYST_FUNC_ENTER_LOC16(systh, 10, MIPI_SYST_SEVERITY_INFO);
 *    // body
 *    MIPI_SYST_FUNC_EXIT_LOC16(systh, 10, MIPI_SYST_SEVERITY_INFO);
 * }
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
#define MIPI_SYST_FUNC_EXIT_LOC16(h, severity, file) \
	mipi_syst_write_debug_string((h),\
		mipi_syst_make_file_location32((h), \
			(mipi_syst_u16)(file), \
			(mipi_syst_u16)MIPI_SYST_LINE),\
		MIPI_SYST_STRING_FUNCTIONEXIT, \
		(severity), sizeof(MIPI_SYST_FUNCTION_NAME),\
		MIPI_SYST_FUNCTION_NAME)

/**
* Send assertion notice string message if the passed condition is false.
* The notice includes the failing expression string as its payload.
*
* @param h mipi_syst_handle* SyS-T handle
* @param severity mipi_syst_severity severity level (0..7)
* @param file 16-bit  user defined file ID
* @param cond boolean condition to verify
*
* Example:
* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
* void foo(void * p)
* {
*    MIPI_SYST_ASSERT(systh, 1, MIPI_SYST_SEVERITY_ERROR, p != NULL);
* }
* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
#define MIPI_SYST_ASSERT_LOC16(h, severity, file, cond) \
	{\
	if (!(cond)) {\
		mipi_syst_write_debug_string((h),\
			mipi_syst_make_file_location32((h), \
					(mipi_syst_u16)(file), \
					(mipi_syst_u16)MIPI_SYST_LINE),\
			MIPI_SYST_STRING_ASSERT, \
			(severity), sizeof(_MIPI_SYST_ASSERT_DEBUG_STRING(cond)),\
			_MIPI_SYST_ASSERT_DEBUG_STRING(cond));\
		} \
	}
/**
* Send function enter string message.
* The payload is the UTF-8 function name of the surrounding function.
*
* @param h mipi_syst_handle* SyS-T handle
* @param severity mipi_syst_severity severity level (0..7)
* @param file 32-bit user defined file ID
*
* Example:
* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
* void foo()
* {
*    MIPI_SYST_FUNC_ENTER_LOC32(systh, 10, MIPI_SYST_SEVERITY_INFO);
*    // body
*    MIPI_SYST_FUNC_EXIT_LOC32(systh, 10, MIPI_SYST_SEVERITY_INFO);
* }
* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
#define MIPI_SYST_FUNC_ENTER_LOC32(h, severity, file) \
	mipi_syst_write_debug_string((h),\
		mipi_syst_make_file_location64((h), \
				(mipi_syst_u32)(file), \
				(mipi_syst_u32)MIPI_SYST_LINE),\
		MIPI_SYST_STRING_FUNCTIONENTER, \
		(severity), sizeof(MIPI_SYST_FUNCTION_NAME),\
		MIPI_SYST_FUNCTION_NAME)
/**
* Send function exit string message.
* The payload is the UTF-8 function name of the surrounding function.
*
* @param h mipi_syst_handle* SyS-T handle
* @param severity mipi_syst_severity severity level (0..7)
* @param file 32-bit user defined file ID
*
* Example:
* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
* void foo()
* {
*    MIPI_SYST_FUNC_ENTER_LOC32(systh, 10, MIPI_SYST_SEVERITY_INFO);
*    // body
*    MIPI_SYST_FUNC_EXIT_LOC32(systh, 10, MIPI_SYST_SEVERITY_INFO);
* }
* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
#define MIPI_SYST_FUNC_EXIT_LOC32(h, severity, file) \
	mipi_syst_write_debug_string((h),\
		mipi_syst_make_file_location64((h), \
				(mipi_syst_u32)(file), \
				(mipi_syst_u32)MIPI_SYST_LINE),\
		MIPI_SYST_STRING_FUNCTIONEXIT, \
		(severity), sizeof(MIPI_SYST_FUNCTION_NAME),\
		MIPI_SYST_FUNCTION_NAME)
/**
* Send assertion notice string message if the passed condition is false.
* The notice includes the failing expression string as its payload.
*
* @param h mipi_syst_handle* SyS-T handle
* @param severity mipi_syst_severity severity level (0..7)
* @param file 32-bit  user defined file ID
* @param cond boolean condition to verify
*
* Example:
* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
* void foo(void * p)
* {
*    MIPI_SYST_ASSERT(systh, 1, MIPI_SYST_SEVERITY_ERROR, p != NULL);
* }
* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
#define MIPI_SYST_ASSERT_LOC32(h, severity, file, cond) \
	{ \
	if (!(cond)) {\
		mipi_syst_write_debug_string((h),\
			mipi_syst_make_file_location64((h), \
					(mipi_syst_u32)(file), \
					(mipi_syst_u32)MIPI_SYST_LINE),\
			MIPI_SYST_STRING_ASSERT, \
			(severity), sizeof(_MIPI_SYST_ASSERT_DEBUG_STRING(cond)),\
			_MIPI_SYST_ASSERT_DEBUG_STRING(cond));\
		} \
	}

#if defined(MIPI_SYST_PCFG_ENABLE_LOCATION_ADDRESS)

/** @copydoc MIPI_SYST_DEBUG */
#define MIPI_SYST_DEBUG_LOCADDR(h, severity, str, len) \
	mipi_syst_write_debug_string((h), \
		mipi_syst_make_address_location((h),\
			mipi_syst_return_addr()),\
		MIPI_SYST_STRING_GENERIC, \
		(severity), (len), (str))

/**
* Send function enter string message.
* The payload is the UTF-8 function name of the surrounding function.

*
* @param h mipi_syst_handle* SyS-T handle
* @param severity mipi_syst_severity severity level (0..7)
*
* Example:
* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
* void foo()
* {
*    MIPI_SYST_FUNC_ENTER_LOCADDR(systh, MIPI_SYST_SEVERITY_INFO);
*    // body
*    MIPI_SYST_FUNC_EXIT_LOCADDR(systh, MIPI_SYST_SEVERITY_INFO);
* }
* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
#define MIPI_SYST_FUNC_ENTER_LOCADDR(h, severity) \
	mipi_syst_write_debug_string((h),\
		mipi_syst_make_address_location((h),\
			mipi_syst_return_addr()),\
		MIPI_SYST_STRING_FUNCTIONENTER, \
		(severity), sizeof(MIPI_SYST_FUNCTION_NAME),\
		MIPI_SYST_FUNCTION_NAME)

/**
* Send function exit string message. The payload is the UTF-8 function name
* of the surrounding function
*
* @param h mipi_syst_handle* SyS-T handle
* @param severity mipi_syst_severity severity level (0..7)
*
* Example:
* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
* void foo()
* {
*    MIPI_SYST_FUNC_ENTER_LOCADDR(systh, MIPI_SYST_SEVERITY_INFO);
*    // body
*    MIPI_SYST_FUNC_EXIT_LOCADDR(systh, MIPI_SYST_SEVERITY_INFO);
* }
* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/
#define MIPI_SYST_FUNC_EXIT_LOCADDR(h, severity) \
	mipi_syst_write_debug_string((h),\
		mipi_syst_make_address_location((h),\
			mipi_syst_return_addr()),\
		MIPI_SYST_STRING_FUNCTIONEXIT, \
		(severity), sizeof(MIPI_SYST_FUNCTION_NAME),\
		MIPI_SYST_FUNCTION_NAME)

/** @copydoc  MIPI_SYST_ASSERT(svh, severity) */
#define MIPI_SYST_ASSERT_LOCADDR(h, severity, cond) \
	{ \
	if (!(cond)) {\
		mipi_syst_write_debug_string((h),\
		mipi_syst_make_address_location((h),\
			mipi_syst_return_addr()),\
		MIPI_SYST_STRING_ASSERT, \
		(severity), sizeof(_MIPI_SYST_ASSERT_DEBUG_STRING(cond)),\
		 _MIPI_SYST_ASSERT_DEBUG_STRING(cond));\
		} \
	}

#endif		/* defined(MIPI_SYST_PCFG_ENABLE_LOCATION_ADDRESS) */

#endif		/* defined(MIPI_SYST_PCFG_ENABLE_LOCATION_RECORD) */

MIPI_SYST_EXPORT void MIPI_SYST_CALLCONV
mipi_syst_write_debug_string(struct mipi_syst_handle* svh,
		struct mipi_syst_msglocation* loc,
		enum mipi_syst_subtype_string type,
		enum mipi_syst_severity severity,
		mipi_syst_u16 len, const char *str);

#endif		/* defined (MIPI_SYST_PCFG_ENABLE_STRING_API) */

#if defined(MIPI_SYST_PCFG_ENABLE_PRINTF_API)

/**
 * Send UTF-8 character string in C99 printf format together with
 * the arguments to support printf() style output formatting.
 *
 * @param h mipi_syst_handle* SyS-T handle
 * @param severity mipi_syst_severity severity level (0..7)
 * @param str const mipi_syst_u8 * pointer to UTF-8 string bytes
 * @param ... optional format arguments
 *
 * Example:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 *
 * MIPI_SYST_PRINTF(systh, MIPI_SYST_SEVERITY_INFO,
 *             "The %s jumps over the %s %d times", "cow", "moon", 10);
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
#define MIPI_SYST_PRINTF(h, severity, str, ...) \
		mipi_syst_write_printf_string((h), MIPI_SYST_NOLOCATION, \
			(severity),\
			(str),\
			##__VA_ARGS__)

#if defined(MIPI_SYST_PCFG_ENABLE_LOCATION_RECORD)

 /**
 * Send UTF-8 character string in C99 printf format together with
 * the arguments to support printf() style output formatting.
 *
 * @param h mipi_syst_handle* SyS-T handle
 * @param severity mipi_syst_severity severity level (0..7)
 * @param file 16-bit  user defined file ID
 * @param str const mipi_syst_u8 * pointer to UTF-8 string bytes
 * @param ... optional format arguments
 *
 * Example:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 *
 * MIPI_SYST_PRINTF_LOC16(systh, MIPI_SYST_SEVERITY_INFO, 10,
 *             "The %s jumps over the %s %d times", "cow", "moon", 10);
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
#define MIPI_SYST_PRINTF_LOC16(h, severity, file, str, ...) \
		mipi_syst_write_printf_string((h), \
			mipi_syst_make_file_location32((h), \
					(mipi_syst_u16)(file), \
					(mipi_syst_u16)MIPI_SYST_LINE),\
			(severity),\
			(str),\
			##__VA_ARGS__)

 /**
 * Send UTF-8 character string in C99 printf format together with
 * the arguments to support printf() style output formatting.
 *
 * @param h mipi_syst_handle* SyS-T handle
 * @param severity mipi_syst_severity severity level (0..7)
 * @param file 32-bit  user defined file ID
 * @param str const mipi_syst_u8 * pointer to UTF-8 string bytes
 * @param ... optional format arguments
 *
 * Example:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 *
 * MIPI_SYST_PRINTF_LOC32(systh, MIPI_SYST_SEVERITY_INFO, 10,
 *             "The %s jumps over the %s %d times", "cow", "moon", 10);
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
#define MIPI_SYST_PRINTF_LOC32(h, severity, file, str, ...) \
		mipi_syst_write_printf_string((h), \
			mipi_syst_make_file_location64((h), \
					(mipi_syst_u32)(file), \
					(mipi_syst_u32)MIPI_SYST_LINE),\
			(severity),\
			(str),\
			##__VA_ARGS__)
#if defined(MIPI_SYST_PCFG_ENABLE_LOCATION_ADDRESS)

 /**
 * Send UTF-8 character string in C99 printf format together with
 * the arguments to support printf() style output formatting.
 *
 * @param h mipi_syst_handle* SyS-T handle
 * @param severity mipi_syst_severity severity level (0..7)
 * @param str const mipi_syst_u8 * pointer to UTF-8 string bytes
 * @param ... optional format arguments
 *
 * Example:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 *
 * MIPI_SYST_PRINTF_LOCADDR(systh, MIPI_SYST_SEVERITY_INFO,
 *             "The %s jumps over the %s %d times", "cow", "moon", 10);
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
#define MIPI_SYST_PRINTF_LOCADDR(h, severity, str, ...) \
		mipi_syst_write_printf_string((h), \
			mipi_syst_make_address_location((h),\
					mipi_syst_return_addr()),\
			(severity), \
			(str),\
			##__VA_ARGS__)

#endif /* defined(MIPI_SYST_PCFG_ENABLE_LOCATION_ADDRESS) */
#endif /* defined(MIPI_SYST_PCFG_ENABLE_LOCATION_RECORD) */

MIPI_SYST_EXPORT void MIPI_SYST_CALLCONV
mipi_syst_write_printf_string(struct mipi_syst_handle* svh,
		struct mipi_syst_msglocation* loc,
		enum mipi_syst_severity severity,
		const char *fmt,
		...);
#endif /* MIPI_SYST_PCFG_ENABLE_PRINTF_API */

/** @} */

/** Parameter encoding values for vararg style catalog APIs
  */
enum mipi_syst_catalog_parameter_types {
	_MIPI_SYST_CATARG_END  =  0,            /**< end of parameter list      */
	_MIPI_SYST_CATARG_D    =  1,            /**< int  like %d               */
	_MIPI_SYST_CATARG_LD   =  2,            /**< long like %ld              */
	_MIPI_SYST_CATARG_LLD  =  3,            /**< long long like %lld        */
	_MIPI_SYST_CATARG_ZD   =  4,            /**< size_t like %z             */
	_MIPI_SYST_CATARG_TD   =  5,            /**< ptrdiff_t like %t          */
	_MIPI_SYST_CATARG_F    =  6,            /**< double like %f             */
	_MIPI_SYST_CATARG_LF   =  7,            /**< long double like %lf       */
	_MIPI_SYST_CATARG_C    =  8,            /**< char like %c               */
	_MIPI_SYST_CATARG_HHD  =  9,            /**< short like %hhd            */
	_MIPI_SYST_CATARG_LC   = 10,            /**< long char like %lc         */
	_MIPI_SYST_CATARG_P    = 11,            /**< void * like %p or %n       */
	_MIPI_SYST_CATARG_CSTR = 12             /**< char * like %s             */
};
#define _MIPI_SYST_MK_PARAM_LIST(tag, p) _MIPI_SYST_CATARG_##tag, p

#define MIPI_SYST_PARAM_INT(p)         _MIPI_SYST_MK_PARAM_LIST(D, (p))        /**< int  like %d               */
#define MIPI_SYST_PARAM_LONG(p)        _MIPI_SYST_MK_PARAM_LIST(LD, (p))       /**< long like %ld              */
#define MIPI_SYST_PARAM_LONGLONG(p)    _MIPI_SYST_MK_PARAM_LIST(LLD, (p))      /**< long long like %lld        */
#define MIPI_SYST_PARAM_SIZE_T(p)      _MIPI_SYST_MK_PARAM_LIST(ZD, (p))       /**< size_t like %z             */
#define MIPI_SYST_PARAM_PTRDIFF_T(p)   _MIPI_SYST_MK_PARAM_LIST(TD, (p))       /**< ptrdiff_t like %t          */
#define MIPI_SYST_PARAM_FLOAT(p)       _MIPI_SYST_MK_PARAM_LIST(F, (p))        /**< float like %f              */
#define MIPI_SYST_PARAM_DOUBLE(p)      _MIPI_SYST_MK_PARAM_LIST(F, (p))        /**< double like %f             */
#define MIPI_SYST_PARAM_LONGDOUBLE(p)  _MIPI_SYST_MK_PARAM_LIST(LF, (p))       /**< long double like %lf       */
#define MIPI_SYST_PARAM_CHAR(p)        _MIPI_SYST_MK_PARAM_LIST(C, (p))        /**< char like %c               */
#define MIPI_SYST_PARAM_SHORT(p)       _MIPI_SYST_MK_PARAM_LIST(HHD, (p))      /**< short like %hhd            */
#define MIPI_SYST_PARAM_WCHAR(p)       _MIPI_SYST_MK_PARAM_LIST(LC, (p))       /**< long char like %lc         */
#define MIPI_SYST_PARAM_PTR(p)         _MIPI_SYST_MK_PARAM_LIST(P, (p))        /**< void * like %p or %n       */
#define MIPI_SYST_PARAM_CSTR(p)        _MIPI_SYST_MK_PARAM_LIST(CSTR, (p))     /**< char * like %s             */

/**
 * @defgroup CatAPI64 64-Bit Catalog Message Macros
 * @ingroup ApiSets
 *
 * Catalog message instrumentation API
 *
 * Note: This API set is enabled or disabled by the
 * #MIPI_SYST_PCFG_ENABLE_CATID64_API and/or #MIPI_SYST_PCFG_ENABLE_CATID32_API
 * platform feature defines.
 * @{
 */

#if defined(MIPI_SYST_PCFG_ENABLE_CATID64_API)

/**
 * Send catalog message with 0-6 parameters.<BR>
 * This family of Macros is used to send 32 or 64-bit wide catalog
 * message IDs with up to six 32-bit wide parameters into the trace stream.
 * The macro names are encoded in the following way:
 * MIPI_SYST_CATALOG{ID-WIDTH}_{PARAMETER-COUNT}
 *
 * Example: A 32-bit ID using catalog function with 3 parameters whould be
 * called MIPI_SYST_CATALOG32_3.
 *
 * @param svh mipi_syst_handle* SyS-T handle
 * @param sev mipi_syst_severity severity level (0..7)
 * @param id catalog ID
 *
 * Up to 6 32-Bit numeric parameter follow the catalog ID.
 *
 * Example:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 *
 * // emit plain catalog id (no parameter)
 * //
 * MIPI_SYST_CATALOG32_0(svh, MIPI_SYST_SEVERITY_ERROR, MSGID_INIT_FAIL);
 *
 * // catalog id with one parameter
 * //
 * MIPI_SYST_CATALOG32_1(svh, MIPI_SYST_SEVERITY_INFO, MSGID_SETLEVEL, 0x3);
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
#define MIPI_SYST_CATALOG64_0(svh, sev, id)\
	(mipi_syst_make_param0(svh),\
	mipi_syst_write_catalog64_message((svh), MIPI_SYST_NOLOCATION, (sev), (id)))
#define MIPI_SYST_CATALOG64_1(svh, sev, id, p1)\
	(mipi_syst_make_param1(svh, p1),\
	mipi_syst_write_catalog64_message((svh), MIPI_SYST_NOLOCATION, (sev), (id)))
#define MIPI_SYST_CATALOG64_2(svh, sev, id, p1, p2)\
	(mipi_syst_make_param2(svh, p1, p2),\
	mipi_syst_write_catalog64_message((svh), MIPI_SYST_NOLOCATION, (sev), (id)))
#define MIPI_SYST_CATALOG64_3(svh, sev, id, p1, p2, p3)\
	(mipi_syst_make_param3(svh, p1, p2, p3),\
	mipi_syst_write_catalog64_message((svh), MIPI_SYST_NOLOCATION, (sev), (id)))
#define MIPI_SYST_CATALOG64_4(svh, sev, id, p1, p2, p3, p4)\
	(mipi_syst_make_param4(svh, p1, p2, p3, p4),\
	mipi_syst_write_catalog64_message((svh), MIPI_SYST_NOLOCATION, (sev), (id)))
#define MIPI_SYST_CATALOG64_5(svh, sev, id, p1, p2, p3, p4, p5)\
	(mipi_syst_make_param5(svh, p1, p2, p3, p4, p5),\
	mipi_syst_write_catalog64_message((svh), MIPI_SYST_NOLOCATION, (sev), (id)))
#define MIPI_SYST_CATALOG64_6(svh, sev, id, p1, p2, p3, p4, p5, p6)\
	(mipi_syst_make_param6(svh, p1, p2, p3, p4, p5, p6),\
	mipi_syst_write_catalog64_message((svh), MIPI_SYST_NOLOCATION, (sev), (id)))

#if defined(MIPI_SYST_PCFG_ENABLE_LOCATION_RECORD)

#if defined(MIPI_SYST_PCFG_ENABLE_LOCATION_ADDRESS)

#define MIPI_SYST_CATALOG64_0_LOCADDR(svh, sev, id)\
	(mipi_syst_make_param0(svh),\
	mipi_syst_write_catalog64_message((svh),\
	       mipi_syst_make_address_location((svh), mipi_syst_return_addr()),\
	       (sev), (id)))
#define MIPI_SYST_CATALOG64_1_LOCADDR(svh, sev, id, p1)\
	(mipi_syst_make_param1(svh, p1),\
	mipi_syst_write_catalog64_message((svh),\
	       mipi_syst_make_address_location((svh), mipi_syst_return_addr()),\
	       (sev), (id)))
#define MIPI_SYST_CATALOG64_2_LOCADDR(svh, sev, id, p1, p2)\
	(mipi_syst_make_param2(svh, p1, p2),\
	mipi_syst_write_catalog64_message((svh),\
	       mipi_syst_make_address_location((svh), mipi_syst_return_addr()),\
	       (sev), (id)))
#define MIPI_SYST_CATALOG64_3_LOCADDR(svh, sev, id, p1, p2, p3)\
	(mipi_syst_make_param3(svh, p1, p2, p3),\
	mipi_syst_write_catalog64_message((svh),\
	       mipi_syst_make_address_location((svh), mipi_syst_return_addr()),\
	       (sev), (id)))
#define MIPI_SYST_CATALOG64_4_LOCADDR(svh, sev, id, p1, p2, p3, p4)\
	(mipi_syst_make_param4(svh, p1, p2, p3, p4),\
	mipi_syst_write_catalog64_message((svh),\
	       mipi_syst_make_address_location((svh), mipi_syst_return_addr()),\
	       (sev), (id)))
#define MIPI_SYST_CATALOG64_5_LOCADDR(svh, sev, id, p1, p2, p3, p4, p5)\
	(mipi_syst_make_param5(svh, p1, p2, p3, p4, p5),\
	mipi_syst_write_catalog64_message((svh),\
	       mipi_syst_make_address_location((svh), mipi_syst_return_addr()),\
	       (sev), (id)))
#define MIPI_SYST_CATALOG64_6_LOCADDR(svh, sev, id, p1, p2, p3, p4, p5, p6)\
	(mipi_syst_make_param6(svh, p1, p2, p3, p4, p5, p6),\
	mipi_syst_write_catalog64_message((svh),\
	       mipi_syst_make_address_location((svh), mipi_syst_return_addr()),\
	       (sev), (id)))

#endif /* defined(MIPI_SYST_PCFG_ENABLE_LOCATION_ADDRESS) */

#define MIPI_SYST_CATALOG64_0_LOC16(svh, sev, file, id)\
	(mipi_syst_make_param0(svh),\
	mipi_syst_write_catalog64_message((svh),\
	       mipi_syst_make_file_location32((svh), \
		   (mipi_syst_u16)(file), (mipi_syst_u16)MIPI_SYST_LINE),\
	       (sev), (id)))
#define MIPI_SYST_CATALOG64_1_LOC16(svh, sev, file, id, p1)\
	(mipi_syst_make_param1(svh, p1),\
	mipi_syst_write_catalog64_message((svh),\
	       mipi_syst_make_file_location32((svh), \
		   (mipi_syst_u16)(file), (mipi_syst_u16)MIPI_SYST_LINE),\
	       (sev), (id)))
#define MIPI_SYST_CATALOG64_2_LOC16(svh, sev, file, id, p1, p2)\
	(mipi_syst_make_param2(svh, p1, p2),\
	mipi_syst_write_catalog64_message((svh),\
	       mipi_syst_make_file_location32((svh), \
		   (mipi_syst_u16)(file), (mipi_syst_u16)MIPI_SYST_LINE),\
	       (sev), (id)))
#define MIPI_SYST_CATALOG64_3_LOC16(svh, sev, file, id, p1, p2, p3)\
	(mipi_syst_make_param3(svh, p1, p2, p3),\
	mipi_syst_write_catalog64_message((svh),\
	       mipi_syst_make_file_location32((svh), \
		   (mipi_syst_u16)(file), (mipi_syst_u16)MIPI_SYST_LINE),\
	       (sev), (id)))
#define MIPI_SYST_CATALOG64_4_LOC16(svh, sev, file, id, p1, p2, p3, p4)\
	(mipi_syst_make_param4(svh, p1, p2, p3, p4),\
	mipi_syst_write_catalog64_message((svh),\
	       mipi_syst_make_file_location32((svh), \
		   (mipi_syst_u16)(file), (mipi_syst_u16)MIPI_SYST_LINE),\
	       (sev), (id)))
#define MIPI_SYST_CATALOG64_5_LOC16(svh, sev, file, id, p1, p2, p3, p4, p5)\
	(mipi_syst_make_param5(svh, p1, p2, p3, p4, p5),\
	mipi_syst_write_catalog64_message((svh),\
	       mipi_syst_make_file_location32((svh), \
		   (mipi_syst_u16)(file), (mipi_syst_u16)MIPI_SYST_LINE),\
	       (sev), (id)))
#define MIPI_SYST_CATALOG64_6_LOC16(svh, sev, file, id, p1, p2, p3, p4, p5, p6)\
	(mipi_syst_make_param6(svh, p1, p2, p3, p4, p5, p6),\
	mipi_syst_write_catalog64_message((svh),\
	       mipi_syst_make_file_location32((svh), \
		   (mipi_syst_u16)(file), (mipi_syst_u16)MIPI_SYST_LINE),\
	       (sev), (id)))

#define MIPI_SYST_CATALOG64_0_LOC32(svh, sev, file, id)\
	(mipi_syst_make_param0(svh),\
	mipi_syst_write_catalog64_message((svh),\
	       mipi_syst_make_file_location64((svh), \
		   (mipi_syst_u32)(file), (mipi_syst_u32)MIPI_SYST_LINE),\
	       (sev), (id)))
#define MIPI_SYST_CATALOG64_1_LOC32(svh, sev, file, id, p1)\
	(mipi_syst_make_param1(svh, p1),\
	mipi_syst_write_catalog64_message((svh),\
	       mipi_syst_make_file_location64((svh), \
		   (mipi_syst_u32)(file), (mipi_syst_u32)MIPI_SYST_LINE),\
	       (sev), (id)))
#define MIPI_SYST_CATALOG64_2_LOC32(svh, sev, file, id, p1, p2)\
	(mipi_syst_make_param2(svh, p1, p2),\
	mipi_syst_write_catalog64_message((svh),\
	       mipi_syst_make_file_location64((svh), \
		   (mipi_syst_u32)(file), (mipi_syst_u32)MIPI_SYST_LINE),\
	       (sev), (id)))
#define MIPI_SYST_CATALOG64_3_LOC32(svh, sev, file, id, p1, p2, p3)\
	(mipi_syst_make_param3(svh, p1, p2, p3),\
	mipi_syst_write_catalog64_message((svh),\
	       mipi_syst_make_file_location64((svh), \
		   (mipi_syst_u32)(file), (mipi_syst_u32)MIPI_SYST_LINE),\
	       (sev), (id)))
#define MIPI_SYST_CATALOG64_4_LOC32(svh, sev, file, id, p1, p2, p3, p4)\
	(mipi_syst_make_param4(svh, p1, p2, p3, p4),\
	mipi_syst_write_catalog64_message((svh),\
	       mipi_syst_make_file_location64((svh), \
		   (mipi_syst_u32)(file), (mipi_syst_u32)MIPI_SYST_LINE),\
	       (sev), (id)))
#define MIPI_SYST_CATALOG64_5_LOC32(svh, sev, file, id, p1, p2, p3, p4, p5)\
	(mipi_syst_make_param5(svh, p1, p2, p3, p4, p5),\
	mipi_syst_write_catalog64_message((svh),\
	       mipi_syst_make_file_location64((svh), \
		   (mipi_syst_u32)(file), (mipi_syst_u32)MIPI_SYST_LINE),\
	       (sev), (id)))
#define MIPI_SYST_CATALOG64_6_LOC32(svh, sev, file, id, p1, p2, p3, p4, p5, p6)\
	(mipi_syst_make_param6(svh, p1, p2, p3, p4, p5, p6),\
	mipi_syst_write_catalog64_message((svh),\
	       mipi_syst_make_file_location64((svh), \
		   (mipi_syst_u32)(file), (mipi_syst_u32)MIPI_SYST_LINE),\
	       (sev), (id)))

#endif /* defined(MIPI_SYST_PCFG_ENABLE_LOCATION_RECORD) */

#if defined(MIPI_SYST_PCFG_ENABLE_PRINTF_API)

/**
 * Send vararg catalog message
 *
 * @param svh mipi_syst_handle* SyS-T handle
 * @param severity mipi_syst_severity severity level (0..7)
 * @param id  mipi_syst_u64 catalog ID
 * @param ... optional format arguments.
 *
 * The optional parameters are passed to the function as a sequence of pairs,
 * each containing a type tag with a value from the enumeration
 * #mipi_syst_catalog_parameter_types, and then a value of the specified type.
 *
 * Example:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 *
 * MIPI_SYST_CATALOG64(svh, MIPI_SYST_SEVERITY_INFO,
 *             MIPI_SYST_HASH("The %s jumps over the %s %d times",0),
 *             MIPI_SYST_PARAM_STRING("cow"),
 *             MIPI_SYST_PARAM_STRING("moon"),
 *             MIPI_SYST_PARAM_INT(10));
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
#define MIPI_SYST_CATALOG64(svh, severity, id, ...) \
		mipi_syst_write_printf_catalog64((svh), MIPI_SYST_NOLOCATION, \
			(severity),\
			(id),\
			##__VA_ARGS__,\
			_MIPI_SYST_CATARG_END)

 #if defined(MIPI_SYST_PCFG_ENABLE_LOCATION_RECORD)

 /**
 * Send vararg catalog message
 *
 * @param svh mipi_syst_handle* SyS-T handle
 * @param severity mipi_syst_severity severity level (0..7)
 * @param file file ID as 16-bit value
 * @param id  mipi_syst_u64 catalog ID
 * @param ... optional format arguments.
 *
 * The optional parameters are passed to the function as a sequence of pairs,
 * each containing a type tag with a value from the enumeration
 * #mipi_syst_catalog_parameter_types, and then a value of the specified type.
 *
 * Example:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 *
 * MIPI_SYST_CATALOG64_LOC16(svh, MIPI_SYST_SEVERITY_INFO, 10,
 *             MIPI_SYST_HASH("The %s jumps over the %s %d times",0),
 *             MIPI_SYST_PARAM_STRING("cow"),
 *             MIPI_SYST_PARAM_STRING("moon"),
 *             MIPI_SYST_PARAM_INT(10));
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
#define MIPI_SYST_CATALOG64_LOC16(svh, severity, file, id, ...) \
		mipi_syst_write_printf_catalog64((svh), \
			mipi_syst_make_file_location32((svh), \
					(mipi_syst_u16)(file), \
					(mipi_syst_u16)MIPI_SYST_LINE),\
			(severity),\
			(id),\
			##__VA_ARGS__,\
			_MIPI_SYST_CATARG_END))
 /**
 * Send vararg catalog message
 *
 * @param h mipi_syst_handle* SyS-T handle
 * @param severity mipi_syst_severity severity level (0..7)
 * @param file file ID as 32-bit value
 * @param id  mipi_syst_u64 catalog ID
 * @param ... optional format arguments.
 *
 * The optional parameters are passed to the function as a sequence of pairs,
 * each containing a type tag with a value from the enumeration
 * #mipi_syst_catalog_parameter_types, and then a value of the specified type.
 *
 *
 * Example:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 *
 * MIPI_SYST_CATALOG64_LOC32(systh, MIPI_SYST_SEVERITY_INFO, 10,
 *             MIPI_SYST_HASH("The %s jumps over the %s %d times",0),
 *             MIPI_SYST_PARAM_STRING("cow"),
 *             MIPI_SYST_PARAM_STRING("moon"),
 *             MIPI_SYST_PARAM_INT(10));
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
#define MIPI_SYST_CATALOG64_LOC32(h, severity, file, id, ...) \
		mipi_syst_write_printf_catalog64((h), \
			mipi_syst_make_file_location64((h), \
					(mipi_syst_u32)(file), \
					(mipi_syst_u32)MIPI_SYST_LINE),\
			(severity),\
			(id),\
			##__VA_ARGS__,\
			_MIPI_SYST_CATARG_END))

#if defined(MIPI_SYST_PCFG_ENABLE_LOCATION_ADDRESS)
 /**
 * Send vararg catalog message
 *
 * @param h mipi_syst_handle* SyS-T handle
 * @param severity mipi_syst_severity severity level (0..7)
 * @param id  mipi_syst_u64 catalog ID
 * @param ... optional format arguments.
 *
 * The optional parameters are passed to the function as a sequence of pairs,
 * each containing a type tag with a value from the enumeration
 * #mipi_syst_catalog_parameter_types, and then a value of the specified type.
 *
 *
 * Example:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 *
 * MIPI_SYST_CATALOG64_LOCADDR(systh, MIPI_SYST_SEVERITY_INFO,
 *             MIPI_SYST_HASH("The %s jumps over the %s %d times",0),
 *             MIPI_SYST_PARAM_STRING("cow"),
 *             MIPI_SYST_PARAM_STRING("moon"),
 *             MIPI_SYST_PARAM_INT(10));
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
#define MIPI_SYST_CATALOG64_LOCADDR(h, severity, id, ...) \
		mipi_syst_write_printf_catalog64((h), \
			mipi_syst_make_address_location((h),\
					mipi_syst_return_addr()),\
			(severity), \
			(id),\
			##__VA_ARGS__,\
			_MIPI_SYST_CATARG_END))

#endif /* defined(MIPI_SYST_PCFG_ENABLE_LOCATION_ADDRESS) */
#endif /* defined(MIPI_SYST_PCFG_ENABLE_LOCATION_RECORD) */


/**
 * Vararg style catalog message the contains the
 * fmt string in addition to the id. The fmt string is not
 * used by this API and removed during compilation. It is only
 * present to support source code scanning tools that compute
 * the ID to fmt string mapping. The API only sends the ID and
 * the arguments to support printf() style output formatting
 * by decoding software.
 *
 * @param svh mipi_syst_handle* SyS-T handle
 * @param severity mipi_syst_severity severity level (0..7)
 * @param id  mipi_syst_u64 catalog ID
 * @param fmt  UTF-8 printf style format string
 * @param ... optional format arguments
 *
 * Example:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 *
 * MIPI_SYST_CATPRINTF64(svh, MIPI_SYST_SEVERITY_INFO,
 *             0x1122334455667788ull,
 *             "The %s jumps over the %s %d times",
 *             MIPI_SYST_PARAM_STRING("cow"),
 *             MIPI_SYST_PARAM_STRING("moon"),
 *             MIPI_SYST_PARAM_INT(10));
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
#define MIPI_SYST_CATPRINTF64(svh, severity, id, fmt, ...) \
		MIPI_SYST_CATALOG64((svh),\
			(severity),\
			(id),\
			##__VA_ARGS__)

#if defined(MIPI_SYST_PCFG_ENABLE_LOCATION_RECORD)

 /**
 * Vararg style catalog message the contains the
 * fmt string in addition to the id. The fmt string is not
 * used by this API and removed during compilation. It is only
 * present to support source code scanning tools that compute
 * the ID to fmt string mapping. The API only sends the ID and
 * the arguments to support printf() style output formatting
 * by decoding software.
 *
 * @param h mipi_syst_handle* SyS-T handle
 * @param severity mipi_syst_severity severity level (0..7)
 * @param file file ID as 16-bit value
 * @param id  mipi_syst_u64 catalog ID
 * @param fmt  UTF-8 printf style format string
 * @param ... optional format arguments
 *
 * Example:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 *
 * MIPI_SYST_CATPRINTF64_LOC16(systh, MIPI_SYST_SEVERITY_INFO, 10
 *             0x1122334455667788ull,
 *             "The %s jumps over the %s %d times",
 *             MIPI_SYST_PARAM_STRING("cow"),
 *             MIPI_SYST_PARAM_STRING("moon"),
 *             MIPI_SYST_PARAM_INT(10));
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
#define MIPI_SYST_CATPRINTF64_LOC16(h, severity, file, id, fmt, ...) \
		MIPI_SYST_CATALOG64_LOC16((h),\
			(severity),\
			(file),\
			(id),\
			##__VA_ARGS__)
 /**
 * Vararg style catalog message the contains the
 * fmt string in addition to the id. The fmt string is not
 * used by this API and removed during compilation. It is only
 * present to support source code scanning tools that compute
 * the ID to fmt string mapping. The API only sends the ID and
 * the arguments to support printf() style output formatting
 * by decoding software.
 *
 * @param h mipi_syst_handle* SyS-T handle
 * @param severity mipi_syst_severity severity level (0..7)
 * @param file file ID as 32-bit value
 * @param id  mipi_syst_u64 catalog ID
 * @param fmt  UTF-8 printf style format string
 * @param ... optional format arguments
 *
 * Example:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 *
 * MIPI_SYST_CATPRINTF64_LOC32(systh, MIPI_SYST_SEVERITY_INFO, 10,
 *             0x1122334455667788ull,
 *             "The %s jumps over the %s %d times",
 *             MIPI_SYST_PARAM_STRING("cow"),
 *             MIPI_SYST_PARAM_STRING("moon"),
 *             MIPI_SYST_PARAM_INT(10));
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
#define MIPI_SYST_CATPRINTF64_LOC32(h, severity, file, id, fmt, ...) \
		MIPI_SYST_CATALOG64_LOC32((h),\
			(severity),\
			(file),\
			(id),\
			##__VA_ARGS__)

#if defined(MIPI_SYST_PCFG_ENABLE_LOCATION_ADDRESS)

 /**
 * Vararg style catalog message the contains the
 * fmt string in addition to the id. The fmt string is not
 * used by this API and removed during compilation. It is only
 * present to support source code scanning tools that compute
 * the ID to fmt string mapping. The API only sends the ID and
 * the arguments to support printf() style output formatting
 * by decoding software.
 *
 * @param h mipi_syst_handle* SyS-T handle
 * @param severity mipi_syst_severity severity level (0..7)
 * @param id  mipi_syst_u64 catalog ID
 * @param fmt  UTF-8 printf style format string
 * @param ... optional format arguments
 *
 * Example:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 *
 * MIPI_SYST_CATPRINTF64_LOCADDR(systh, MIPI_SYST_SEVERITY_INFO,
 *             0x1122334455667788ull,
 *             "The %s jumps over the %s %d times",
 *             MIPI_SYST_PARAM_STRING("cow"),
 *             MIPI_SYST_PARAM_STRING("moon"),
 *             MIPI_SYST_PARAM_INT(10));
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
#define MIPI_SYST_CATPRINTF64_LOCADDR(h, severity, id, fmt, ...) \
		MIPI_SYST_CATALOG64_LOCADDR((h),\
			(severity),\
			(id),\
			##__VA_ARGS__)

#endif /* defined(MIPI_SYST_PCFG_ENABLE_LOCATION_ADDRESS) */
#endif /* defined(MIPI_SYST_PCFG_ENABLE_LOCATION_RECORD) */

MIPI_SYST_EXPORT void MIPI_SYST_CALLCONV
mipi_syst_write_printf_catalog64(struct mipi_syst_handle* svh,
		struct mipi_syst_msglocation* loc,
		enum mipi_syst_severity severity,
		mipi_syst_u64 id,
		...);
#endif /* #if defined(MIPI_SYST_PCFG_ENABLE_PRINTF_API) */

/** @} */

/* API function prototypes */

MIPI_SYST_EXPORT void MIPI_SYST_CALLCONV
mipi_syst_write_catalog64_message(struct mipi_syst_handle* svh,
				 struct mipi_syst_msglocation* loc,
				 enum mipi_syst_severity severity,
				 mipi_syst_u64 catid);

#endif		/* defined(MIPI_SYST_PCFG_ENABLE_CATID64_API) */

#if defined(MIPI_SYST_PCFG_ENABLE_CATID32_API)
/**
 * @defgroup CatAPI32 32-Bit Catalog Message Macros
 * @ingroup ApiSets
 *
 * Catalog message instrumentation API
 * @{
 */

#if defined(MIPI_SYST_PCFG_ENABLE_PRINTF_API)
/**
 * Send vararg catalog message
 *
 * @param h mipi_syst_handle* SyS-T handle
 * @param severity mipi_syst_severity severity level (0..7)
 * @param id  mipi_syst_u32 catalog ID
 * @param ... optional format arguments
 *
 * The optional parameters are passed to the function as a sequence of pairs,
 * each containing a type tag with a value from the enumeration
 * #mipi_syst_catalog_parameter_types, and then a value of the specified type.
 *
 * Example:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 *
 * MIPI_SYST_CATALOG32(systh, MIPI_SYST_SEVERITY_INFO,
 *             MIPI_SYST_HASH("The %s jumps over the %s %d times",0),
 *             MIPI_SYST_PARAM_CSTR("cow"),
 *             MIPI_SYST_PARAM_CSTR("moon"),
 *             MIPI_SYST_PARAM_INT(10));
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
#define MIPI_SYST_CATALOG32(h, severity, id, ...) \
		mipi_syst_write_printf_catalog32((h), MIPI_SYST_NOLOCATION, \
			(severity),\
			(id),\
			##__VA_ARGS__,\
			_MIPI_SYST_CATARG_END)

 #if defined(MIPI_SYST_PCFG_ENABLE_LOCATION_RECORD)

 /**
 * Send vararg catalog message
 *
 * @param h mipi_syst_handle* SyS-T handle
 * @param severity mipi_syst_severity severity level (0..7)
 * @param file file ID as 16-bit value
 * @param id  mipi_syst_u32 catalog ID
 * @param ... optional format arguments
 *
 * The optional parameters are passed to the function as a sequence of pairs,
 * each containing a type tag with a value from the enumeration
 * #mipi_syst_catalog_parameter_types, and then a value of the specified type.
 *
 * Example:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 *
 * MIPI_SYST_CATALOG32_LOC16(systh, MIPI_SYST_SEVERITY_INFO, 10,
 *             MIPI_SYST_HASH("The %s jumps over the %s %d times",0),
 *             MIPI_SYST_PARAM_CSTR("cow"),
 *             MIPI_SYST_PARAM_CSTR("moon"),
 *             MIPI_SYST_PARAM_INT(10));
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
#define MIPI_SYST_CATALOG32_LOC16(h, severity, file, id, ...) \
		mipi_syst_write_printf_catalog32((h), \
			mipi_syst_make_file_location32((h), \
					(mipi_syst_u16)(file), \
					(mipi_syst_u16)MIPI_SYST_LINE),\
			(severity),\
			(id),\
			##__VA_ARGS__,\
			_MIPI_SYST_CATARG_END))

 /**
 * Send vararg catalog message
 *
 * @param h mipi_syst_handle* SyS-T handle
 * @param severity mipi_syst_severity severity level (0..7)
 * @param file file ID as 32-bit value
 * @param id  mipi_syst_u32 catalog ID
 * @param ... optional format arguments
 *
 * The optional parameters are passed to the function as a sequence of pairs,
 * each containing a type tag with a value from the enumeration
 * #mipi_syst_catalog_parameter_types, and then a value of the specified type.
 *
 * Example:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 *
 * MIPI_SYST_CATALOG32_LOC32(systh, MIPI_SYST_SEVERITY_INFO, 10,
 *             MIPI_SYST_HASH("The %s jumps over the %s %d times",0),
 *             MIPI_SYST_PARAM_CSTR("cow"),
 *             MIPI_SYST_PARAM_CSTR("moon"),
 *             MIPI_SYST_PARAM_INT(10));
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
#define MIPI_SYST_CATALOG32_LOC32(h, severity, file, id, ...) \
		mipi_syst_write_printf_catalog32((h), \
			mipi_syst_make_file_location64((h), \
					(mipi_syst_u32)(file), \
					(mipi_syst_u32)MIPI_SYST_LINE),\
			(severity),\
			(id),\
			##__VA_ARGS__,\
			_MIPI_SYST_CATARG_END))

#if defined(MIPI_SYST_PCFG_ENABLE_LOCATION_ADDRESS)

 /**
 * Send vararg catalog message
 *
 * @param h mipi_syst_handle* SyS-T handle
 * @param severity mipi_syst_severity severity level (0..7)
 * @param id  mipi_syst_u32 catalog ID
 * @param ... optional format arguments
 *
 * The optional parameters are passed to the function as a sequence of pairs,
 * each containing a type tag with a value from the enumeration
 * #mipi_syst_catalog_parameter_types, and then a value of the specified type.
 *
 * Example:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 *
 * MIPI_SYST_CATALOG32_LOCADDR(systh, MIPI_SYST_SEVERITY_INFO,
 *             MIPI_SYST_HASH("The %s jumps over the %s %d times",0),
 *             MIPI_SYST_PARAM_CSTR("cow"),
 *             MIPI_SYST_PARAM_CSTR("moon"),
 *             MIPI_SYST_PARAM_INT(10));
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
#define MIPI_SYST_CATALOG32_LOCADDR(h, severity, id, ...) \
		mipi_syst_write_printf_catalog32((h), \
			mipi_syst_make_address_location((h),\
					mipi_syst_return_addr()),\
			(severity), \
			(id),\
			##__VA_ARGS__,\
			_MIPI_SYST_CATARG_END))

#endif /* defined(MIPI_SYST_PCFG_ENABLE_LOCATION_ADDRESS) */
#endif /* defined(MIPI_SYST_PCFG_ENABLE_LOCATION_RECORD) */

/**
 * Send catalog message together with
 * the arguments to support printf() style output formatting.
 *
 * @param h mipi_syst_handle* SyS-T handle
 * @param sev mipi_syst_severity severity level (0..7)
 * @param id  mipi_syst_u32 catalog ID
 * @param fmt const mipi_syst_u8 * pointer to UTF-8 string bytes
 * @param ... optional format arguments
 *
 * Example:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 *
 * MIPI_SYST_CATALOG_PRINTF32(systh, MIPI_SYST_SEVERITY_INFO,
 *             0x1122334455667788ull,
 *             "The %s jumps over the %s %d times",
 *             MIPI_SYST_PARAM_CSTR("cow"),
 *             MIPI_SYST_PARAM_CSTR("moon"),
 *             MIPI_SYST_PARAM_INT(10));
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
#define MIPI_SYST_CATPRINTF32(h, sev, id, fmt, ...) \
		MIPI_SYST_CATALOG32((h),\
			(sev),\
			(id),\
			##__VA_ARGS__)

#if defined(MIPI_SYST_PCFG_ENABLE_LOCATION_RECORD)

 /**
 * Send catalog message together with
 * the arguments to support printf() style output formatting.
 *
 * @param h mipi_syst_handle* SyS-T handle
 * @param sev mipi_syst_severity severity level (0..7)
 * @param file file ID as 16-bit value
 * @param id  mipi_syst_u32 catalog ID
 * @param fmt const mipi_syst_u8 * pointer to UTF-8 string bytes
 * @param ... optional format arguments
 *
 * Example:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 *
 * MIPI_SYST_CATALOG_PRINTF32_LOC16(systh, MIPI_SYST_SEVERITY_INFO, 10,
 *             0x1122334455667788ull,
 *             "The %s jumps over the %s %d times",
 *             MIPI_SYST_PARAM_CSTR("cow"),
 *             MIPI_SYST_PARAM_CSTR("moon"),
 *             MIPI_SYST_PARAM_INT(10));
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
#define MIPI_SYST_CATPRINTF32_LOC16(h, sev, file, id, fmt, ...) \
		MIPI_SYST_CATALOG32_LOC16((h),\
			(sev),\
			(file),\
			(id),\
			##__VA_ARGS__)

 /**
 * Send catalog message together with
 * the arguments to support printf() style output formatting.
 *
 * @param h mipi_syst_handle* SyS-T handle
 * @param sev mipi_syst_severity severity level (0..7)
 * @param file file ID as 32-bit value
 * @param id  mipi_syst_u32 catalog ID
 * @param fmt const mipi_syst_u8 * pointer to UTF-8 string bytes
 * @param ... optional format arguments
 *
 * Example:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 *
 * MIPI_SYST_CATALOG_PRINTF32_LOC32(systh, MIPI_SYST_SEVERITY_INFO, 10,
 *             0x1122334455667788ull,
 *             "The %s jumps over the %s %d times",
 *             MIPI_SYST_PARAM_CSTR("cow"),
 *             MIPI_SYST_PARAM_CSTR("moon"),
 *             MIPI_SYST_PARAM_INT(10));
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
#define MIPI_SYST_CATPRINTF32_LOC32(h, sev, file, id, fmt, ...) \
	MIPI_SYST_CATALOG32_LOC32((h),\
			(sev),\
			(id),\
			(file),\
			##__VA_ARGS__)

#if defined(MIPI_SYST_PCFG_ENABLE_LOCATION_ADDRESS)

 /**
 * Send catalog message together with
 * the arguments to support printf() style output formatting.
 *
 * @param h mipi_syst_handle* SyS-T handle
 * @param sev mipi_syst_severity severity level (0..7)
 * @param id  mipi_syst_u32 catalog ID
 * @param fmt const mipi_syst_u8 * pointer to UTF-8 string bytes
 * @param ... optional format arguments
 *
 * Example:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 *
 * MIPI_SYST_CATALOG_PRINTF32_LOCADDR(systh, MIPI_SYST_SEVERITY_INFO,
 *             0x1122334455667788ull,
 *             "The %s jumps over the %s %d times",
 *             MIPI_SYST_PARAM_CSTR("cow"),
 *             MIPI_SYST_PARAM_CSTR("moon"),
 *             MIPI_SYST_PARAM_INT(10));
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
#define MIPI_SYST_CATPRINTF32_LOCADDR(h, sev, id, fmt, ...) \
		MIPI_SYST_CATALOG32_LOCADDR((h),\
			(sev),\
			(id),\
			##__VA_ARGS__)

#endif /* defined(MIPI_SYST_PCFG_ENABLE_LOCATION_ADDRESS) */
#endif /* defined(MIPI_SYST_PCFG_ENABLE_LOCATION_RECORD) */

MIPI_SYST_EXPORT void MIPI_SYST_CALLCONV
mipi_syst_write_printf_catalog32(struct mipi_syst_handle* svh,
		struct mipi_syst_msglocation* loc,
		enum mipi_syst_severity severity,
		mipi_syst_u32 id,
		...);

#endif /* defined(MIPI_SYST_PCFG_ENABLE_PRINTF_API) */

#if defined(MIPI_SYST_PCFG_ENABLE_SBD_API)

/**
 * Pass null to SBD API to skip optional blob address.
 */
#define MIPI_SYST_SBD_NO_BLOB_ADDRESS (mipi_syst_address)0

 /**
 * Create a Structured Binary Data (SBD) Message
 * of type MIPI_SYST_TYPE_SBD with 32-bit SBD-ID.
 *
 * @param h mipi_syst_handle * SyS-T handle
 * @param sev mipi_syst_severity severity level (0..7)
 * @param id  mipi_syst_u32 32-bit SBD-ID value
 * @param addr mipi_syst_u64 optional address for BLOB or MIPI_SYST_SBD_NO_BLOB_ADDRESS
 * @param name const mipi_syst_u8 * optional name for BLOB, UTF-8 string or NULL
 * @param len mipi_syst_u32 size of BLOB in bytes
 * @param blob const void * pointer to BLOB
 *
 * Example:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 *
 * struct MyVariable {
 *    int member1;
 *    int member2;
 * };
 *
 * MyVariable myVariable = {1, 2};

 * MIPI_SYST_SBD32(handle,
 *      MIPI_SYST_SEVERITY_INFO,
 *	    0x12345678,
 *	    (mipi_syst_address)&myVariable,
 *		"myVariable",
 *		sizeof(myVariable),
 *		&myVariable);
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
#define MIPI_SYST_SBD32(h, sev, id, addr, name, len, blob)\
	(mipi_syst_write_sbd32_message(h, MIPI_SYST_NOLOCATION, sev, id, addr, name, len, blob))

 /**
 * Create a Structured Binary Data (SBD) Message
 * of type MIPI_SYST_TYPE_SBD with 64-bit SBD-ID.
 *
 * @param h mipi_syst_handle * SyS-T handle
 * @param sev mipi_syst_severity severity level (0..7)
 * @param id  mipi_syst_u64 64-bit SBD-ID value
 * @param addr mipi_syst_u64 optional address for BLOB or MIPI_SYST_SBD_NO_BLOB_ADDRESS
 * @param name const mipi_syst_u8 * optional name for BLOB, UTF-8 string or NULL
 * @param len mipi_syst_u32 size of BLOB in bytes
 * @param blob const void * pointer to BLOB
 *
 * Example:
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~{.c}
 *
 * struct MyVariable {
 *    int member1;
 *    int member2;
 * };
 *
 * MyVariable myVariable = {1, 2};

 * MIPI_SYST_SBD64(handle,
 *      MIPI_SYST_SEVERITY_INFO,
 *	    0x1234567812345678,
 *	    (mipi_syst_address)&myVariable,
 *		"myVariable",
 *		sizeof(myVariable),
 *		&myVariable);
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
#define MIPI_SYST_SBD64(h, sev, id, addr, name, len, blob)\
	(mipi_syst_write_sbd64_message(h, MIPI_SYST_NOLOCATION, sev, id, addr, name, len, blob))

MIPI_SYST_EXPORT void MIPI_SYST_CALLCONV
mipi_syst_write_sbd32_message(struct mipi_syst_handle* svh,
			struct mipi_syst_msglocation* loc,
			enum mipi_syst_severity severity,
			mipi_syst_u32 sbd_id,
			mipi_syst_address addr,
			const mipi_syst_s8 *name,
			mipi_syst_u32 len,
			const void *blob);

MIPI_SYST_EXPORT void MIPI_SYST_CALLCONV
mipi_syst_write_sbd64_message(struct mipi_syst_handle* svh,
			struct mipi_syst_msglocation* loc,
			enum mipi_syst_severity severity,
			mipi_syst_u64 sbd_id,
			mipi_syst_address addr,
			const char* name,
			mipi_syst_u32 len,
			const void *blob);

#endif /* #if defined(MIPI_SYST_PCFG_ENABLE_SBD_API) */

/** @copydoc MIPI_SYST_CATALOG64_0 */
#define MIPI_SYST_CATALOG32_0(svh, sev, id)\
	(mipi_syst_make_param0(svh),\
	mipi_syst_write_catalog32_message((svh), MIPI_SYST_NOLOCATION, (sev), (id)))
#define MIPI_SYST_CATALOG32_1(svh, sev, id, p1)\
	(mipi_syst_make_param1(svh, p1),\
	mipi_syst_write_catalog32_message((svh), MIPI_SYST_NOLOCATION, (sev), (id)))
#define MIPI_SYST_CATALOG32_2(svh, sev, id, p1, p2)\
	(mipi_syst_make_param2(svh, p1, p2),\
	mipi_syst_write_catalog32_message((svh), MIPI_SYST_NOLOCATION, (sev), (id)))
#define MIPI_SYST_CATALOG32_3(svh, sev, id, p1, p2, p3)\
	(mipi_syst_make_param3(svh, p1, p2, p3),\
	mipi_syst_write_catalog32_message((svh), MIPI_SYST_NOLOCATION, (sev), (id)))
#define MIPI_SYST_CATALOG32_4(svh, sev, id, p1, p2, p3, p4)\
	(mipi_syst_make_param4(svh, p1, p2, p3, p4),\
	mipi_syst_write_catalog32_message((svh), MIPI_SYST_NOLOCATION, (sev), (id)))
#define MIPI_SYST_CATALOG32_5(svh, sev, id, p1, p2, p3, p4, p5)\
	(mipi_syst_make_param5(svh, p1, p2, p3, p4, p5),\
	mipi_syst_write_catalog32_message((svh), MIPI_SYST_NOLOCATION, (sev), (id)))
#define MIPI_SYST_CATALOG32_6(svh, sev, id, p1, p2, p3, p4, p5, p6)\
	(mipi_syst_make_param6(svh, p1, p2, p3, p4, p5, p6),\
	mipi_syst_write_catalog32_message((svh), MIPI_SYST_NOLOCATION, (sev), (id)))

#if defined(MIPI_SYST_PCFG_ENABLE_LOCATION_RECORD)

#if defined(MIPI_SYST_PCFG_ENABLE_LOCATION_ADDRESS)

#define MIPI_SYST_CATALOG32_0_LOCADDR(svh, sev, id)\
	(mipi_syst_make_param0(svh),\
	mipi_syst_write_catalog32_message((svh),\
	       mipi_syst_make_address_location((svh), mipi_syst_return_addr()),\
	       (sev), (id)))
#define MIPI_SYST_CATALOG32_1_LOCADDR(svh, sev, id, p1)\
	(mipi_syst_make_param1(svh, p1),\
	mipi_syst_write_catalog32_message((svh),\
	       mipi_syst_make_address_location((svh), mipi_syst_return_addr()),\
	       (sev), (id)))
#define MIPI_SYST_CATALOG32_2_LOCADDR(svh, sev, id, p1, p2)\
	(mipi_syst_make_param2(svh, p1, p2),\
	mipi_syst_write_catalog32_message((svh),\
	       mipi_syst_make_address_location((svh), mipi_syst_return_addr()),\
	       (sev), (id)))
#define MIPI_SYST_CATALOG32_3_LOCADDR(svh, sev, id, p1, p2, p3)\
	(mipi_syst_make_param3(svh, p1, p2, p3),\
	mipi_syst_write_catalog32_message((svh),\
	       mipi_syst_make_address_location((svh), mipi_syst_return_addr()),\
	       (sev), (id)))
#define MIPI_SYST_CATALOG32_4_LOCADDR(svh, sev, id, p1, p2, p3, p4)\
	(mipi_syst_make_param4(svh, p1, p2, p3, p4),\
	mipi_syst_write_catalog32_message((svh),\
	       mipi_syst_make_address_location((svh), mipi_syst_return_addr()),\
	       (sev), (id)))
#define MIPI_SYST_CATALOG32_5_LOCADDR(svh, sev, id, p1, p2, p3, p4, p5)\
	(mipi_syst_make_param5(svh, p1, p2, p3, p4, p5),\
	mipi_syst_write_catalog32_message((svh),\
	       mipi_syst_make_address_location((svh), mipi_syst_return_addr()),\
	       (sev), (id)))
#define MIPI_SYST_CATALOG32_6_LOCADDR(svh, sev, id, p1, p2, p3, p4, p5, p6)\
	(mipi_syst_make_param6(svh, p1, p2, p3, p4, p5, p6),\
	mipi_syst_write_catalog32_message((svh),\
	       mipi_syst_make_address_location((svh), mipi_syst_return_addr()),\
	       (sev), (id)))

#endif		/* defined(MIPI_SYST_PCFG_ENABLE_LOCATION_ADDRESS) */

#define MIPI_SYST_CATALOG32_0_LOC16(svh, sev, file, id)\
	(mipi_syst_make_param0(svh),\
	mipi_syst_write_catalog32_message((svh),\
	       mipi_syst_make_file_location32((svh), \
		   (mipi_syst_u16)(file), (mipi_syst_u16)MIPI_SYST_LINE),\
	       (sev), (id)))
#define MIPI_SYST_CATALOG32_1_LOC16(svh, sev, file, id, p1)\
	(mipi_syst_make_param1(svh, p1),\
	mipi_syst_write_catalog32_message((svh),\
	       mipi_syst_make_file_location32((svh), \
		   (mipi_syst_u16)(file), (mipi_syst_u16)MIPI_SYST_LINE),\
	       (sev), (id)))
#define MIPI_SYST_CATALOG32_2_LOC16(svh, sev, file, id, p1, p2)\
	(mipi_syst_make_param2(svh, p1, p2),\
	mipi_syst_write_catalog32_message((svh),\
	       mipi_syst_make_file_location32((svh), \
		   (mipi_syst_u16)(file), (mipi_syst_u16)MIPI_SYST_LINE),\
	       (sev), (id)))
#define MIPI_SYST_CATALOG32_3_LOC16(svh, sev, file, id, p1, p2, p3)\
	(mipi_syst_make_param3(svh, p1, p2, p3),\
	mipi_syst_write_catalog32_message((svh),\
	       mipi_syst_make_file_location32((svh), \
		   (mipi_syst_u16)(file), (mipi_syst_u16)MIPI_SYST_LINE),\
	       (sev), (id)))
#define MIPI_SYST_CATALOG32_4_LOC16(svh, sev, file, id, p1, p2, p3, p4)\
	(mipi_syst_make_param4(svh, p1, p2, p3, p4),\
	mipi_syst_write_catalog32_message((svh),\
	       mipi_syst_make_file_location32((svh), \
		   (mipi_syst_u16)(file), (mipi_syst_u16)MIPI_SYST_LINE),\
	       (sev), (id)))
#define MIPI_SYST_CATALOG32_5_LOC16(svh, sev, file, id, p1, p2, p3, p4, p5)\
	(mipi_syst_make_param5(svh, p1, p2, p3, p4, p5),\
	mipi_syst_write_catalog32_message((svh),\
	       mipi_syst_make_file_location32((svh), \
		   (mipi_syst_u16)(file), (mipi_syst_u16)MIPI_SYST_LINE),\
	       (sev), (id)))
#define MIPI_SYST_CATALOG32_6_LOC16(svh, sev, file, id, p1, p2, p3, p4, p5, p6)\
	(mipi_syst_make_param6(svh, p1, p2, p3, p4, p5, p6),\
	mipi_syst_write_catalog32_message((svh),\
	       mipi_syst_make_file_location32((svh), \
		   (mipi_syst_u16)(file), (mipi_syst_u16)MIPI_SYST_LINE),\
	       (sev), (id)))

#define MIPI_SYST_CATALOG32_0_LOC32(svh, sev, file, id)\
	(mipi_syst_make_param0(svh),\
	mipi_syst_write_catalog32_message((svh),\
	       mipi_syst_make_file_location64((svh), \
		   (mipi_syst_u32)(file), (mipi_syst_u32)MIPI_SYST_LINE),\
	       (sev), (id)))
#define MIPI_SYST_CATALOG32_1_LOC32(svh, sev, file, id, p1)\
	(mipi_syst_make_param1(svh, p1),\
	mipi_syst_write_catalog32_message((svh),\
	       mipi_syst_make_file_location64((svh), \
		   (mipi_syst_u32)(file), (mipi_syst_u32)MIPI_SYST_LINE),\
	       (sev), (id)))
#define MIPI_SYST_CATALOG32_2_LOC32(svh, sev, file, id, p1, p2)\
	(mipi_syst_make_param2(svh, p1, p2),\
	mipi_syst_write_catalog32_message((svh),\
	       mipi_syst_make_file_location64((svh), \
		   (mipi_syst_u32)(file), (mipi_syst_u32)MIPI_SYST_LINE),\
	       (sev), (id)))
#define MIPI_SYST_CATALOG32_3_LOC32(svh, sev, file, id, p1, p2, p3)\
	(mipi_syst_make_param3(svh, p1, p2, p3),\
	mipi_syst_write_catalog32_message((svh),\
	       mipi_syst_make_file_location64((svh), \
		   (mipi_syst_u32)(file), (mipi_syst_u32)MIPI_SYST_LINE),\
	       (sev), (id)))
#define MIPI_SYST_CATALOG32_4_LOC32(svh, sev, file, id, p1, p2, p3, p4)\
	(mipi_syst_make_param4(svh, p1, p2, p3, p4),\
	mipi_syst_write_catalog32_message((svh),\
	       mipi_syst_make_file_location64((svh), \
		   (mipi_syst_u32)(file), (mipi_syst_u32)MIPI_SYST_LINE),\
	       (sev), (id)))
#define MIPI_SYST_CATALOG32_5_LOC32(svh, sev, file, id, p1, p2, p3, p4, p5)\
	(mipi_syst_make_param5(svh, p1, p2, p3, p4, p5),\
	mipi_syst_write_catalog32_message((svh),\
	       mipi_syst_make_file_location64((svh), \
		   (mipi_syst_u32)(file), (mipi_syst_u32)MIPI_SYST_LINE),\
	       (sev), (id)))
#define MIPI_SYST_CATALOG32_6_LOC32(svh, sev, file, id, p1, p2, p3, p4, p5, p6)\
	(mipi_syst_make_param6(svh, p1, p2, p3, p4, p5, p6),\
	mipi_syst_write_catalog32_message((svh),\
	       mipi_syst_make_file_location64((svh), \
		   (mipi_syst_u32)(file), (mipi_syst_u32)MIPI_SYST_LINE),\
	       (sev), (id)))

#endif				/* defined(MIPI_SYST_PCFG_ENABLE_LOCATION_RECORD) */
/** @} */

MIPI_SYST_EXPORT void MIPI_SYST_CALLCONV
mipi_syst_write_catalog32_message(struct mipi_syst_handle* svh,
				 struct mipi_syst_msglocation* loc,
				 enum mipi_syst_severity severity,
				 mipi_syst_u32 catid);

#endif				/* defined(MIPI_SYST_PCFG_ENABLE_CATID32_API) */



#endif	/* !MIPI_SYST_DISABLE_ALL */

/* Define undefined API's to nothing. This ensures source compatibility
 * independent of the SyS-T feature configuration.
 */
#ifndef MIPI_SYST_INIT
#define MIPI_SYST_INIT(f, p)
#endif

#ifndef MIPI_SYST_SHUTDOWN
#define MIPI_SYST_SHUTDOWN(x)
#endif

#ifndef MIPI_SYST_INIT_HANDLE
#define MIPI_SYST_INIT_HANDLE(h, p) (struct mipi_syst_handle*)0
#endif

#ifndef MIPI_SYST_ENABLE_HANDLE_CHECKSUM
#define MIPI_SYST_ENABLE_HANDLE_CHECKSUM(h, v)
#endif
#ifndef MIPI_SYST_GET_HANDLE_CHECKSUM
#define MIPI_SYST_GET_HANDLE_CHECKSUM(h) 0
#endif
#ifndef MIPI_SYST_ENABLE_HANDLE_COUNTER
#define MIPI_SYST_ENABLE_HANDLE_COUNTER(h, v)
#endif
#ifndef MIPI_SYST_GET_HANDLE_COUNTER
#define MIPI_SYST_GET_HANDLE_COUNTER(h) 0
#endif
#ifndef MIPI_SYST_SET_HANDLE_MODULE_UNIT
#define MIPI_SYST_SET_HANDLE_MODULE_UNIT(p, m, u)
#endif

#ifndef MIPI_SYST_SET_HANDLE_GUID_UNIT
#define MIPI_SYST_SET_HANDLE_GUID_UNIT(p, g, u)
#endif

#ifndef MIPI_SYST_ENABLE_HANDLE_TIMESTAMP
#define MIPI_SYST_ENABLE_HANDLE_TIMESTAMP(h, v)
#endif
#ifndef MIPI_SYST_GET_HANDLE_TIMESTAMP
#define MIPI_SYST_GET_HANDLE_TIMESTAMP(h) 0
#endif

#ifndef MIPI_SYST_INIT
#define MIPI_SYST_INIT(f, p)
#endif

#ifndef MIPI_SYST_SHUTDOWN
#define MIPI_SYST_SHUTDOWN()
#endif

#ifndef MIPI_SYST_INIT_HANDLE
#define MIPI_SYST_INIT_HANDLE(h, p)
#endif

#ifndef MIPI_SYST_ALLOC_HANDLE
#define MIPI_SYST_ALLOC_HANDLE(p) (struct mipi_syst_handle*)0
#endif

#ifndef MIPI_SYST_ENABLE_HANDLE_CHECKSUM
#define MIPI_SYST_ENABLE_HANDLE_CHECKSUM(h,v)
#endif

#ifndef MIPI_SYST_ENABLE_HANDLE_COUNTER
#define MIPI_SYST_ENABLE_HANDLE_COUNTER(h,v)
#endif

#ifndef MIPI_SYST_ENABLE_HANDLE_LENGTH
#define MIPI_SYST_ENABLE_HANDLE_LENGTH(h,v)
#endif

#ifndef MIPI_SYST_SET_HANDLE_MODULE_UNIT
#define MIPI_SYST_SET_HANDLE_MODULE_UNIT(p, m, u)
#endif

#ifndef MIPI_SYST_SET_HANDLE_GUID_UNIT
#define MIPI_SYST_SET_HANDLE_GUID_UNIT(p, o, u)
#endif

#ifndef MIPI_SYST_SHORT32
#define MIPI_SYST_SHORT32(h, v)
#endif

#ifndef MIPI_SYST_WRITE
#define MIPI_SYST_WRITE(h, sev, id, p, len)
#endif
#ifndef MIPI_SYST_WRITE_LOCADDR
#define MIPI_SYST_WRITE_LOCADDR(h, sev, id,  p, len)
#endif
#ifndef MIPI_SYST_WRITE_LOC16
#define MIPI_SYST_WRITE_LOC16(h, sev, f, id,  p, len)
#endif
#ifndef MIPI_SYST_WRITE_LOC32
#define MIPI_SYST_WRITE_LOC32(h, sev, f, id,  p, len)
#endif

#ifndef MIPI_SYST_DELETE_HANDLE
#define MIPI_SYST_DELETE_HANDLE(h)
#endif

#ifndef MIPI_SYST_DEBUG
#define MIPI_SYST_DEBUG(svh, sev, str, len)
#endif
#ifndef MIPI_SYST_DEBUG_LOCADDR
#define MIPI_SYST_DEBUG_LOCADDR(svh, sev, str, len)
#endif
#ifndef MIPI_SYST_DEBUG_LOC16
#define MIPI_SYST_DEBUG_LOC16(svh, sev, file, str, len)
#endif
#ifndef MIPI_SYST_DEBUG_LOC32
#define MIPI_SYST_DEBUG_LOC32(svh, sev, file, str, len)
#endif
#ifndef MIPI_SYST_FUNC_ENTER
#define MIPI_SYST_FUNC_ENTER(svh, sev)
#endif
#ifndef MIPI_SYST_FUNC_ENTER_LOCADDR
#define MIPI_SYST_FUNC_ENTER_LOCADDR(svh, sev)
#endif
#ifndef MIPI_SYST_FUNC_ENTER_LOC16
#define MIPI_SYST_FUNC_ENTER_LOC16(svh, sev, file)
#endif
#ifndef MIPI_SYST_FUNC_ENTER_LOC32
#define MIPI_SYST_FUNC_ENTER_LOC32(svh, sev, file)
#endif
#ifndef MIPI_SYST_FUNC_EXIT
#define MIPI_SYST_FUNC_EXIT(svh, sev)
#endif
#ifndef MIPI_SYST_FUNC_EXIT_LOCADDR
#endif
#ifndef MIPI_SYST_FUNC_EXIT_LOCADDR
#define MIPI_SYST_FUNC_EXIT_LOCADDR(svh, sev)
#endif
#ifndef MIPI_SYST_FUNC_EXIT_LOC16
#define MIPI_SYST_FUNC_EXIT_LOC16(svh, sev, file)
#endif
#ifndef MIPI_SYST_FUNC_EXIT_LOC32
#define MIPI_SYST_FUNC_EXIT_LOC32(svh, sev, file)
#endif
#ifndef MIPI_SYST_ASSERT
#define MIPI_SYST_ASSERT(svh, sev, cond)
#endif
#ifndef MIPI_SYST_ASSERT_LOC16
#define MIPI_SYST_ASSERT_LOC16(svh, sev, file, cond)
#endif
#ifndef MIPI_SYST_ASSERT_LOC32
#define MIPI_SYST_ASSERT_LOC32(svh, sev, file, cond)
#endif
#ifndef MIPI_SYST_ASSERT_LOCADDR
#define MIPI_SYST_ASSERT_LOCADDR(svh, sev, cond)
#endif

#ifndef MIPI_SYST_CATALOG32_0
#define MIPI_SYST_CATALOG32_0(svh, sev, id)
#endif
#ifndef MIPI_SYST_CATALOG32_1
#define MIPI_SYST_CATALOG32_1(svh, sev, id, p1)
#endif
#ifndef MIPI_SYST_CATALOG32_2
#define MIPI_SYST_CATALOG32_2(svh, sev, id, p1, p2)
#endif
#ifndef MIPI_SYST_CATALOG32_3
#define MIPI_SYST_CATALOG32_3(svh, sev, id, p1, p2, p3)
#endif
#ifndef MIPI_SYST_CATALOG32_4
#define MIPI_SYST_CATALOG32_4(svh, sev, id, p1, p2, p3, p4)
#endif
#ifndef MIPI_SYST_CATALOG32_5
#define MIPI_SYST_CATALOG32_5(svh, sev, id, p1, p2, p3, p4, p5)
#endif
#ifndef MIPI_SYST_CATALOG32_6
#define MIPI_SYST_CATALOG32_6(svh, sev, id, p1, p2, p3, p4, p5, p6)
#endif

#ifndef MIPI_SYST_CATALOG32_0_LOCADDR
#define MIPI_SYST_CATALOG32_0_LOCADDR(svh, sev, id)
#endif
#ifndef MIPI_SYST_CATALOG32_1_LOCADDR
#define MIPI_SYST_CATALOG32_1_LOCADDR(svh, sev, id, p1)
#endif
#ifndef MIPI_SYST_CATALOG32_2_LOCADDR
#define MIPI_SYST_CATALOG32_2_LOCADDR(svh, sev, id, p1, p2)
#endif
#ifndef MIPI_SYST_CATALOG32_3_LOCADDR
#define MIPI_SYST_CATALOG32_3_LOCADDR(svh, sev, id, p1, p2, p3)
#endif
#ifndef MIPI_SYST_CATALOG32_4_LOCADDR
#define MIPI_SYST_CATALOG32_4_LOCADDR(svh, sev, id, p1, p2, p3, p4)
#endif
#ifndef MIPI_SYST_CATALOG32_5_LOCADDR
#define MIPI_SYST_CATALOG32_5_LOCADDR(svh, sev, id, p1, p2, p3, p4, p5)
#endif
#ifndef MIPI_SYST_CATALOG32_6_LOCADDR
#define MIPI_SYST_CATALOG32_6_LOCADDR(svh, sev, id, p1, p2, p3, p4, p5, p6)
#endif

#ifndef MIPI_SYST_CATALOG32_0_LOC16
#define MIPI_SYST_CATALOG32_0_LOC16(svh, sev, f, id)
#endif
#ifndef MIPI_SYST_CATALOG32_1_LOC16
#define MIPI_SYST_CATALOG32_1_LOC16(svh, sev, f, id, p1)
#endif
#ifndef MIPI_SYST_CATALOG32_2_LOC16
#define MIPI_SYST_CATALOG32_2_LOC16(svh, sev, f, id, p1, p2)
#endif
#ifndef MIPI_SYST_CATALOG32_3_LOC16
#define MIPI_SYST_CATALOG32_3_LOC16(svh, sev, f, id, p1, p2, p3)
#endif
#ifndef MIPI_SYST_CATALOG32_4_LOC16
#define MIPI_SYST_CATALOG32_4_LOC16(svh, sev, f, id, p1, p2, p3, p4)
#endif
#ifndef MIPI_SYST_CATALOG32_5_LOC16
#define MIPI_SYST_CATALOG32_5_LOC16(svh, sev, f, id, p1, p2, p3, p4, p5)
#endif
#ifndef MIPI_SYST_CATALOG32_6_LOC16
#define MIPI_SYST_CATALOG32_6_LOC16(svh, sev, f, id, p1, p2, p3, p4, p5, p6)
#endif

#ifndef MIPI_SYST_CATALOG32_0_LOC32
#define MIPI_SYST_CATALOG32_0_LOC32(svh, sev, f, id)
#endif
#ifndef MIPI_SYST_CATALOG32_1_LOC32
#define MIPI_SYST_CATALOG32_1_LOC32(svh, sev, f, id, p1)
#endif
#ifndef MIPI_SYST_CATALOG32_2_LOC32
#define MIPI_SYST_CATALOG32_2_LOC32(svh, sev, f, id, p1, p2)
#endif
#ifndef MIPI_SYST_CATALOG32_3_LOC32
#define MIPI_SYST_CATALOG32_3_LOC32(svh, sev, f, id, p1, p2, p3)
#endif
#ifndef MIPI_SYST_CATALOG32_4_LOC32
#define MIPI_SYST_CATALOG32_4_LOC32(svh, sev, f, id, p1, p2, p3, p4)
#endif
#ifndef MIPI_SYST_CATALOG32_5_LOC32
#define MIPI_SYST_CATALOG32_5_LOC32(svh, sev, f, id, p1, p2, p3, p4, p5)
#endif
#ifndef MIPI_SYST_CATALOG32_6_LOC32
#define MIPI_SYST_CATALOG32_6_LOC32(svh, sev, f, id, p1, p2, p3, p4, p5, p6)
#endif

#ifndef MIPI_SYST_CATALOG64_0
#define MIPI_SYST_CATALOG64_0(svh, sev, id)
#endif
#ifndef MIPI_SYST_CATALOG64_1
#define MIPI_SYST_CATALOG64_1(svh, sev, id, p1)
#endif
#ifndef MIPI_SYST_CATALOG64_2
#define MIPI_SYST_CATALOG64_2(svh, sev, id, p1, p2)
#endif
#ifndef MIPI_SYST_CATALOG64_3
#define MIPI_SYST_CATALOG64_3(svh, sev, id, p1, p2, p3)
#endif
#ifndef MIPI_SYST_CATALOG64_4
#define MIPI_SYST_CATALOG64_4(svh, sev, id, p1, p2, p3, p4)
#endif
#ifndef MIPI_SYST_CATALOG64_5
#define MIPI_SYST_CATALOG64_5(svh, sev, id, p1, p2, p3, p4, p5)
#endif
#ifndef MIPI_SYST_CATALOG64_6
#define MIPI_SYST_CATALOG64_6(svh, sev, id, p1, p2, p3, p4, p5, p6)
#endif

#ifndef MIPI_SYST_CATALOG64_0_LOCADDR
#define MIPI_SYST_CATALOG64_0_LOCADDR(svh, sev, id)
#endif
#ifndef MIPI_SYST_CATALOG64_1_LOCADDR
#define MIPI_SYST_CATALOG64_1_LOCADDR(svh, sev, id, p1)
#endif
#ifndef MIPI_SYST_CATALOG64_2_LOCADDR
#define MIPI_SYST_CATALOG64_2_LOCADDR(svh, sev, id, p1, p2)
#endif
#ifndef MIPI_SYST_CATALOG64_3_LOCADDR
#define MIPI_SYST_CATALOG64_3_LOCADDR(svh, sev, id, p1, p2, p3)
#endif
#ifndef MIPI_SYST_CATALOG64_4_LOCADDR
#define MIPI_SYST_CATALOG64_4_LOCADDR(svh, sev, id, p1, p2, p3, p4)
#endif
#ifndef MIPI_SYST_CATALOG64_5_LOCADDR
#define MIPI_SYST_CATALOG64_5_LOCADDR(svh, sev, id, p1, p2, p3, p4, p5)
#endif
#ifndef MIPI_SYST_CATALOG64_6_LOCADDR
#define MIPI_SYST_CATALOG64_6_LOCADDR(svh, sev, id, p1, p2, p3, p4, p5, p6)
#endif

#ifndef MIPI_SYST_CATALOG64_0_LOC16
#define MIPI_SYST_CATALOG64_0_LOC16(svh, sev, f, id)
#endif
#ifndef MIPI_SYST_CATALOG64_1_LOC16
#define MIPI_SYST_CATALOG64_1_LOC16(svh, sev, f, id, p1)
#endif
#ifndef MIPI_SYST_CATALOG64_2_LOC16
#define MIPI_SYST_CATALOG64_2_LOC16(svh, sev, f, id, p1, p2)
#endif
#ifndef MIPI_SYST_CATALOG64_3_LOC16
#define MIPI_SYST_CATALOG64_3_LOC16(svh, sev, f, id, p1, p2, p3)
#endif
#ifndef MIPI_SYST_CATALOG64_4_LOC16
#define MIPI_SYST_CATALOG64_4_LOC16(svh, sev, f, id, p1, p2, p3, p4)
#endif
#ifndef MIPI_SYST_CATALOG64_5_LOC16
#define MIPI_SYST_CATALOG64_5_LOC16(svh, sev, f, id, p1, p2, p3, p4, p5)
#endif
#ifndef MIPI_SYST_CATALOG64_6_LOC16
#define MIPI_SYST_CATALOG64_6_LOC16(svh, sev, f, id, p1, p2, p3, p4, p5, p6)
#endif

#ifndef MIPI_SYST_CATALOG64_0_LOC32
#define MIPI_SYST_CATALOG64_0_LOC32(svh, sev, f, id)
#endif
#ifndef MIPI_SYST_CATALOG64_1_LOC32
#define MIPI_SYST_CATALOG64_1_LOC32(svh, sev, f, id, p1)
#endif
#ifndef MIPI_SYST_CATALOG64_2_LOC32
#define MIPI_SYST_CATALOG64_2_LOC32(svh, sev, f, id, p1, p2)
#endif
#ifndef MIPI_SYST_CATALOG64_3_LOC32
#define MIPI_SYST_CATALOG64_3_LOC32(svh, sev, f, id, p1, p2, p3)
#endif
#ifndef MIPI_SYST_CATALOG64_4_LOC32
#define MIPI_SYST_CATALOG64_4_LOC32(svh, sev, f, id, p1, p2, p3, p4)
#endif
#ifndef MIPI_SYST_CATALOG64_5_LOC32
#define MIPI_SYST_CATALOG64_5_LOC32(svh, sev, f, id, p1, p2, p3, p4, p5)
#endif
#ifndef MIPI_SYST_CATALOG64_6_LOC32
#define MIPI_SYST_CATALOG64_6_LOC32(svh, sev, f, id, p1, p2, p3, p4, p5, p6)
#endif

#ifndef MIPI_SYST_CLOCK_SYNC
#define MIPI_SYST_CLOCK_SYNC(h,c,f)
#endif

#ifndef MIPI_SYST_PRINTF
#define MIPI_SYST_PRINTF(...)
#endif
#ifndef MIPI_SYST_PRINTF_LOC16
#define MIPI_SYST_PRINTF_LOC16(...)
#endif
#ifndef MIPI_SYST_PRINTF_LOC32
#define MIPI_SYST_PRINTF_LOC32(...)
#endif
#ifndef MIPI_SYST_PRINTF_LOCADDR
#define MIPI_SYST_PRINTF_LOCADDR(...)
#endif

#ifndef MIPI_SYST_CATPRINTF64
#define MIPI_SYST_CATPRINTF64(...)
#endif
#ifndef MIPI_SYST_CATPRINTF64_LOC16
#define MIPI_SYST_CATPRINTF64_LOC16(...)
#endif
#ifndef MIPI_SYST_CATPRINTF64_LOC32
#define MMIPI_SYST_CATPRINTF64_LOC32(...)
#endif
#ifndef MIPI_SYST_CATPRINTF64_LOCADDR
#define MIPI_SYST_CATPRINTF64_LOCADDR(...)
#endif
#ifndef MIPI_SYST_CATPRINTF32
#define MIPI_SYST_CATPRINTF32(...)
#endif
#ifndef MIPI_SYST_CATPRINTF32_LOC16
#define MIPI_SYST_CATPRINTF32_LOC16(...)
#endif
#ifndef MIPI_SYST_CATPRINTF32_LOC32
#define MMIPI_SYST_CATPRINTF32_LOC32(...)
#endif
#ifndef MIPI_SYST_CATPRINTF32_LOCADDR
#define MIPI_SYST_CATPRINTF32_LOCADDR(...)
#endif

#ifndef MIPI_SYST_SBD32
#define MIPI_SYST_SBD32(...)
#endif
#ifndef MIPI_SYST_SBD64
#define MIPI_SYST_SBD64(...)
#endif

/* Map CATPRINTF calls to their corresponding catalog APIs
 * by dropping the format string parameter.
 */
 /**
 * @defgroup PrintfApi Printf style catalog Message Macros
 * @ingroup ApiSets
 *
 * Printf style catalog message instrumentation API wrappers.
 * The  `MIPI_SYST_CATPRINTF{ID-WIDTH}_{PARAMETER-COUNT}`
 * macros call their corresponding catalog API macro by dropping
 * the format parameter. The API is intended to have
 * printf style instrumentation inside the sources, but drop the format
 * string during compilation and replace it with a catalog API call
 * instead. Source scanning tools are used to extract the format strings
 * from sources into catalog dictionary files. The dictionary files can
 * then be used to reconstruct the printf formatting during trace decode
 * time. This saves both space and execution time in the resulting
 * application, and bandwidth over a trace link.
 *
 *  This API set is enabled or disabled by the
 * #MIPI_SYST_PCFG_ENABLE_CATID64_API and/or #MIPI_SYST_PCFG_ENABLE_CATID32_API
 * platform feature defines.
 * @{
 */
#define MIPI_SYST_CATPRINTF32_0(svh, sev, id, fmt)\
	MIPI_SYST_CATALOG32_0(svh, sev, id)
#define MIPI_SYST_CATPRINTF32_1(svh, sev, id, fmt, p1)\
	MIPI_SYST_CATALOG32_1(svh, sev, id, p1)
#define MIPI_SYST_CATPRINTF32_2(svh, sev, id, fmt, p1, p2)\
	MIPI_SYST_CATALOG32_2(svh, sev, id, p1, p2)
#define MIPI_SYST_CATPRINTF32_3(svh, sev, id, fmt, p1, p2, p3)\
	MIPI_SYST_CATALOG32_3(svh, sev, id, p1, p2, p3)
#define MIPI_SYST_CATPRINTF32_4(svh, sev, id, fmt, p1, p2, p3, p4)\
	MIPI_SYST_CATALOG32_4(svh, sev, id, p1, p2, p3, p4)
#define MIPI_SYST_CATPRINTF32_5(svh, sev, id, fmt, p1, p2, p3, p4, p5)\
	MIPI_SYST_CATALOG32_5(svh, sev, id, p1, p2, p3, p4, p5)
#define MIPI_SYST_CATPRINTF32_6(svh, sev, id, fmt, p1, p2, p3, p4, p5, p6)\
	MIPI_SYST_CATALOG32_6(svh, sev, id, p1, p2, p3, p4, p5, p6)
#define MIPI_SYST_CATPRINTF32_0_LOCADDR(svh, sev, id, fmt)\
	MIPI_SYST_CATALOG32_0_LOCADDR(svh, sev, id)
#define MIPI_SYST_CATPRINTF32_1_LOCADDR(svh, sev, id, fmt, p1)\
	MIPI_SYST_CATALOG32_1_LOCADDR(svh, sev, id, p1)
#define MIPI_SYST_CATPRINTF32_2_LOCADDR(svh, sev, id, fmt, p1, p2)\
	MIPI_SYST_CATALOG32_2_LOCADDR(svh, sev, id, p1, p2)
#define MIPI_SYST_CATPRINTF32_3_LOCADDR(svh, sev, id, fmt, p1, p2, p3)\
	MIPI_SYST_CATALOG32_3_LOCADDR(svh, sev, id, p1, p2, p3)
#define MIPI_SYST_CATPRINTF32_4_LOCADDR(svh, sev, id, fmt, p1, p2, p3, p4)\
	MIPI_SYST_CATALOG32_4_LOCADDR(svh, sev, id, p1, p2, p3, p4)
#define MIPI_SYST_CATPRINTF32_5_LOCADDR(svh, sev, id, fmt, p1, p2, p3, p4, p5)\
	MIPI_SYST_CATALOG32_5_LOCADDR(svh, sev, id, p1, p2, p3, p4, p5)
#define MIPI_SYST_CATPRINTF32_6_LOCADDR(svh, sev, id, fmt, p1, p2, p3, p4, p5, p6)\
	MIPI_SYST_CATALOG32_6_LOCADDR(svh, sev, id, p1, p2, p3, p4, p5, p6)
#define MIPI_SYST_CATPRINTF32_0_LOC16(svh, sev, f, id, fmt)\
	MIPI_SYST_CATALOG32_0_LOC16(svh, sev, f, id)
#define MIPI_SYST_CATPRINTF32_1_LOC16(svh, sev, f, id, fmt, p1)\
	MIPI_SYST_CATALOG32_1_LOC16(svh, sev, f, id, p1)
#define MIPI_SYST_CATPRINTF32_2_LOC16(svh, sev, f, id, fmt, p1, p2)\
	MIPI_SYST_CATALOG32_2_LOC16(svh, sev, f, id, p1, p2)
#define MIPI_SYST_CATPRINTF32_3_LOC16(svh, sev, f, id, fmt, p1, p2, p3)\
	MIPI_SYST_CATALOG32_3_LOC16(svh, sev, f, id, p1, p2, p3)
#define MIPI_SYST_CATPRINTF32_4_LOC16(svh, sev, f, id, fmt, p1, p2, p3, p4)\
	MIPI_SYST_CATALOG32_4_LOC16(svh, sev, f, id, p1, p2, p3, p4)
#define MIPI_SYST_CATPRINTF32_5_LOC16(svh, sev, f, id, fmt, p1, p2, p3, p4, p5)\
	MIPI_SYST_CATALOG32_5_LOC16(svh, sev, f, id, p1, p2, p3, p4, p5)
#define MIPI_SYST_CATPRINTF32_6_LOC16(svh, sev, f, id, fmt, p1, p2, p3, p4, p5, p6)\
	MIPI_SYST_CATALOG32_6_LOC16(svh, sev, f, id, p1, p2, p3, p4, p5, p6)
#define MIPI_SYST_CATPRINTF32_0_LOC32(svh, sev, f, id, fmt)\
	MIPI_SYST_CATALOG32_0_LOC32(svh, sev, f, id)
#define MIPI_SYST_CATPRINTF32_1_LOC32(svh, sev, f, id, fmt, p1)\
	MIPI_SYST_CATALOG32_1_LOC32(svh, sev, f, id, p1)
#define MIPI_SYST_CATPRINTF32_2_LOC32(svh, sev, f, id, fmt, p1, p2)\
	MIPI_SYST_CATALOG32_2_LOC32(svh, sev, f, id, p1, p2)
#define MIPI_SYST_CATPRINTF32_3_LOC32(svh, sev, f, id, fmt, p1, p2, p3)\
	MIPI_SYST_CATALOG32_3_LOC32(svh, sev, f, id, p1, p2, p3)
#define MIPI_SYST_CATPRINTF32_4_LOC32(svh, sev, f, id, fmt, p1, p2, p3, p4)\
	MIPI_SYST_CATALOG32_4_LOC32(svh, sev, f, id, p1, p2, p3, p4)
#define MIPI_SYST_CATPRINTF32_5_LOC32(svh, sev, f, id, fmt, p1, p2, p3, p4, p5)\
	MIPI_SYST_CATALOG32_5_LOC32(svh, sev, f, id, p1, p2, p3, p4, p5)
#define MIPI_SYST_CATPRINTF32_6_LOC32(svh, sev, f, id, fmt, p1, p2, p3, p4, p5, p6)\
	MIPI_SYST_CATALOG32_6_LOC32(svh, sev, f, id, p1, p2, p3, p4, p5, p6)
#define MIPI_SYST_CATPRINTF64_0(svh, sev, id, fmt)\
	MIPI_SYST_CATALOG64_0(svh, sev, id)
#define MIPI_SYST_CATPRINTF64_1(svh, sev, id, fmt, p1)\
	MIPI_SYST_CATALOG64_1(svh, sev, id, p1)
#define MIPI_SYST_CATPRINTF64_2(svh, sev, id, fmt, p1, p2)\
	MIPI_SYST_CATALOG64_2(svh, sev, id, p1, p2)
#define MIPI_SYST_CATPRINTF64_3(svh, sev, id, fmt, p1, p2, p3)\
	MIPI_SYST_CATALOG64_3(svh, sev, id, p1, p2, p3)
#define MIPI_SYST_CATPRINTF64_4(svh, sev, id, fmt, p1, p2, p3, p4)\
	MIPI_SYST_CATALOG64_4(svh, sev, id, p1, p2, p3, p4)
#define MIPI_SYST_CATPRINTF64_5(svh, sev, id, fmt, p1, p2, p3, p4, p5)\
	MIPI_SYST_CATALOG64_5(svh, sev, id, p1, p2, p3, p4, p5)
#define MIPI_SYST_CATPRINTF64_6(svh, sev, id, fmt, p1, p2, p3, p4, p5, p6)\
	MIPI_SYST_CATALOG64_6(svh, sev, id, p1, p2, p3, p4, p5, p6)
#define MIPI_SYST_CATPRINTF64_0_LOCADDR(svh, sev, id, fmt)\
	MIPI_SYST_CATALOG64_0_LOCADDR(svh, sev, id)
#define MIPI_SYST_CATPRINTF64_1_LOCADDR(svh, sev, id, fmt, p1)\
	MIPI_SYST_CATALOG64_1_LOCADDR(svh, sev, id, p1)
#define MIPI_SYST_CATPRINTF64_2_LOCADDR(svh, sev, id, fmt, p1, p2)\
	MIPI_SYST_CATALOG64_2_LOCADDR(svh, sev, id, p1, p2)
#define MIPI_SYST_CATPRINTF64_3_LOCADDR(svh, sev, id, fmt, p1, p2, p3)\
	MIPI_SYST_CATALOG64_3_LOCADDR(svh, sev, id, p1, p2, p3)
#define MIPI_SYST_CATPRINTF64_4_LOCADDR(svh, sev, id, fmt, p1, p2, p3, p4)\
	MIPI_SYST_CATALOG64_4_LOCADDR(svh, sev, id, p1, p2, p3, p4)
#define MIPI_SYST_CATPRINTF64_5_LOCADDR(svh, sev, id, fmt, p1, p2, p3, p4, p5)\
	MIPI_SYST_CATALOG64_5_LOCADDR(svh, sev, id, p1, p2, p3, p4, p5)
#define MIPI_SYST_CATPRINTF64_6_LOCADDR(svh, sev, id, fmt, p1, p2, p3, p4, p5, p6)\
	MIPI_SYST_CATALOG64_6_LOCADDR(svh, sev, id, p1, p2, p3, p4, p5, p6)
#define MIPI_SYST_CATPRINTF64_0_LOC16(svh, sev, f, id, fmt)\
	MIPI_SYST_CATALOG64_0_LOC16(svh, sev, f, id)
#define MIPI_SYST_CATPRINTF64_1_LOC16(svh, sev, f, id, fmt, p1)\
	MIPI_SYST_CATALOG64_1_LOC16(svh, sev, f, id, p1)
#define MIPI_SYST_CATPRINTF64_2_LOC16(svh, sev, f, id, fmt, p1, p2)\
	MIPI_SYST_CATALOG64_2_LOC16(svh, sev, f, id, p1, p2)
#define MIPI_SYST_CATPRINTF64_3_LOC16(svh, sev, f, id, fmt, p1, p2, p3)\
	MIPI_SYST_CATALOG64_3_LOC16(svh, sev, f, id, p1, p2, p3)
#define MIPI_SYST_CATPRINTF64_4_LOC16(svh, sev, f, id, fmt, p1, p2, p3, p4)\
	MIPI_SYST_CATALOG64_4_LOC16(svh, sev, f, id, p1, p2, p3, p4)
#define MIPI_SYST_CATPRINTF64_5_LOC16(svh, sev, f, id, fmt, p1, p2, p3, p4, p5)\
	MIPI_SYST_CATALOG64_5_LOC16(svh, sev, f, id, p1, p2, p3, p4, p5)
#define MIPI_SYST_CATPRINTF64_6_LOC16(svh, sev, f, id, fmt, p1, p2, p3, p4, p5, p6)\
	MIPI_SYST_CATALOG64_6_LOC16(svh, sev, f, id, p1, p2, p3, p4, p5, p6)
#define MIPI_SYST_CATPRINTF64_0_LOC32(svh, sev, f, id, fmt)\
	MIPI_SYST_CATALOG64_0_LOC32(svh, sev, f, id)
#define MIPI_SYST_CATPRINTF64_1_LOC32(svh, sev, f, id, fmt, p1)\
	MIPI_SYST_CATALOG64_1_LOC32(svh, sev, f, id, p1)
#define MIPI_SYST_CATPRINTF64_2_LOC32(svh, sev, f, id, fmt, p1, p2)\
	MIPI_SYST_CATALOG64_2_LOC32(svh, sev, f, id, p1, p2)
#define MIPI_SYST_CATPRINTF64_3_LOC32(svh, sev, f, id, fmt, p1, p2, p3)\
	MIPI_SYST_CATALOG64_3_LOC32(svh, sev, f, id, p1, p2, p3)
#define MIPI_SYST_CATPRINTF64_4_LOC32(svh, sev, f, id, fmt, p1, p2, p3, p4)\
	MIPI_SYST_CATALOG64_4_LOC32(svh, sev, f, id, p1, p2, p3, p4)
#define MIPI_SYST_CATPRINTF64_5_LOC32(svh, sev, f, id, fmt, p1, p2, p3, p4, p5)\
	MIPI_SYST_CATALOG64_5_LOC32(svh, sev, f, id, p1, p2, p3, p4, p5)
#define MIPI_SYST_CATPRINTF64_6_LOC32(svh, sev, f, id, fmt, p1, p2, p3, p4, p5, p6)\
	MIPI_SYST_CATALOG64_6_LOC32(svh, sev, f, id, p1, p2, p3, p4, p5, p6)
/** @} */

#ifdef __cplusplus
}	/* extern C */
#endif
#endif
