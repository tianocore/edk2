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

 /* Internal C-language API implementation */

#include "mipi_syst.h"
#include "mipi_syst/message.h"

#if defined(MIPI_SYST_UNIT_TEST)
#include <assert.h>
#define ASSERT_CHECK(x) assert(x)
#else
#define ASSERT_CHECK(x)
#endif

#if  MIPI_SYST_CONFORMANCE_LEVEL > 10

#if !defined(MIPI_SYST_SCATTER_WRITE)
/**
 * Default writer access is to call global state systh_writer pointer.
 * Redefine this if you can avoid the function pointer overhead.
 */
#define MIPI_SYST_SCATTER_WRITE(syst_handle, scatter_prog, data_ptr) \
	{ \
		(syst_handle)->systh_header->systh_writer( \
				(syst_handle), (scatter_prog), (data_ptr));\
	}
#endif

/**
 * predefined write scatter instructions defined in scatter_ops table
 */
enum syst_scatter_ops {
#if defined(MIPI_SYST_PCFG_ENABLE_ORIGIN_GUID)
	SCATTER_OP_GUID,
#endif

#if defined(MIPI_SYST_PCFG_ENABLE_LOCATION_RECORD)
	SCATTER_OP_LOC_FMT,
	SCATTER_OP_LOC_32,
	SCATTER_OP_LOC_64,
#endif
#if defined(MIPI_SYST_PCFG_LENGTH_FIELD)
	SCATTER_OP_LENGTH,
#endif
	SCATTER_OP_PAYLD_VAR,
	SCATTER_OP_CHECKSUM,
	SCATTER_OP_CATID_32,
	SCATTER_OP_CATID_64,
	SCATTER_OP_CATID_ARGS,
	SCATTER_OP_CLOCK,
#if defined(MIPI_SYST_PCFG_ENABLE_TIMESTAMP)
	SCATTER_OP_TS,
#endif
#if defined(MIPI_SYST_PCFG_ENABLE_BUILD_API)
	SCATTER_OP_VER_ID,
	SCATTER_OP_VER_TXT,
#endif
#if defined(MIPI_SYST_PCFG_ENABLE_SBD_API)
	SCATTER_OP_SBD_ID_32,
	SCATTER_OP_SBD_ID_64,
	SCATTER_OP_SBD_ADDR,
	SCATTER_OP_SBD_NAME,
	SCATTER_OP_SBD_BLOB,
#endif
	SCATTER_OP_END
};

/**
 * Scatter instruction that describe the writing of SyS-T message descriptor
 * members by the scatter write algorithm.
 */
static const struct mipi_syst_scatter_prog scatter_ops[] = {
#if defined(MIPI_SYST_PCFG_ENABLE_ORIGIN_GUID)

	{			/* SCATTER_OP_GUID */
	 MIPI_SYST_SCATTER_OP_64BIT,
	 MIPI_SYST_EVDSC_MEMBER_OFF(ed_guid),
	 2},
#endif

#if defined(MIPI_SYST_PCFG_ENABLE_LOCATION_RECORD)
	{			/* SCATTER_OP_LOC_FMT */
	 MIPI_SYST_SCATTER_OP_8BIT,
	 MIPI_SYST_EVDSC_MEMBER_OFF(ed_loc.el_format),
	 1},
	{			/* SCATTER_OP_LOC_32 */
	 MIPI_SYST_SCATTER_OP_32BIT,
	 MIPI_SYST_EVDSC_MEMBER_OFF(ed_loc.el_u),
	 1},
	{			/* SCATTER_OP_LOC_64 */
	 MIPI_SYST_SCATTER_OP_64BIT,
	 MIPI_SYST_EVDSC_MEMBER_OFF(ed_loc.el_u),
	 1},
#endif
#if defined(MIPI_SYST_PCFG_LENGTH_FIELD)
	{			/* SCATTER_OP_LENGTH */
	 MIPI_SYST_SCATTER_OP_16BIT,
	 MIPI_SYST_EVDSC_MEMBER_OFF(ed_len),
	 1},
#endif
	{			/* SCATTER_OP_PAYLD_VAR */
	 MIPI_SYST_SCATTER_OP_BLOB,
	 MIPI_SYST_EVDSC_MEMBER_OFF(ed_pld.data_var),
	 0}
	,
	{			/* SCATTER_OP_PAYLD_CHECKSUM */
	 MIPI_SYST_SCATTER_OP_32BIT,
	 MIPI_SYST_EVDSC_MEMBER_OFF(ed_chk),
	 1}
	,
	{			/* SCATTER_OP_CATID_32 */
	 MIPI_SYST_SCATTER_OP_32BIT,
	 MIPI_SYST_EVDSC_MEMBER_OFF(ed_pld.data_catid.id.sci_32),
	 1}
	,
	{			/* SCATTER_OP_CATID_64 */
	 MIPI_SYST_SCATTER_OP_64BIT,
	 MIPI_SYST_EVDSC_MEMBER_OFF(ed_pld.data_catid.id.sci_64),
	 1}
	,
	{			/* SCATTER_OP_CATID_ARGS */
	 MIPI_SYST_SCATTER_OP_BLOB,
	 MIPI_SYST_EVDSC_MEMBER_OFF(ed_pld.data_catid.param),
	 0}
	,
	{		/* SCATTER_OP_CLOCK */
	 MIPI_SYST_SCATTER_OP_64BIT,
	 MIPI_SYST_EVDSC_MEMBER_OFF(ed_pld.data_clock),
	 2}
	,
#if defined(MIPI_SYST_PCFG_ENABLE_TIMESTAMP)		/* SCATTER_OP_TS */
	{
	 MIPI_SYST_SCATTER_OP_64BIT,
	 MIPI_SYST_EVDSC_MEMBER_OFF(ed_ts),
	 1}
	,
#endif /* defined(MIPI_SYST_PCFG_ENABLE_TIMESTAMP) */
#if defined(MIPI_SYST_PCFG_ENABLE_BUILD_API)
	{ 			/* SCATTER_OP_VER_ID */
	 MIPI_SYST_SCATTER_OP_64BIT,
	 MIPI_SYST_EVDSC_MEMBER_OFF(ed_pld.data_version.id),
	 1}
	,
	{			/* SCATTER_OP_VER_TXT */
	 MIPI_SYST_SCATTER_OP_BLOB,
	 MIPI_SYST_EVDSC_MEMBER_OFF(ed_pld.data_version.text),
	 0}
	,
#endif /* defined(MIPI_SYST_PCFG_ENABLE_BUILD_API) */
#if defined(MIPI_SYST_PCFG_ENABLE_SBD_API)

	{			/* SCATTER_OP_SBD_ID_32 */
	 MIPI_SYST_SCATTER_OP_32BIT,
	 MIPI_SYST_EVDSC_MEMBER_OFF(ed_pld.data_sbd.id.sbd_id_32),
	 1}
	 ,
	{			/* SCATTER_OP_SBD_ID_64 */
	 MIPI_SYST_SCATTER_OP_64BIT,
	 MIPI_SYST_EVDSC_MEMBER_OFF(ed_pld.data_sbd.id.sbd_id_64),
	 1}
	 ,
	 {			/* SCATTER_OP_SBD_ADDR */
#ifdef MIPI_SYST_PTR_SIZE_16BIT
	 MIPI_SYST_SCATTER_OP_16BIT,
#elif defined(MIPI_SYST_PTR_SIZE_32BIT)
	 MIPI_SYST_SCATTER_OP_32BIT,
#elif defined(MIPI_SYST_PTR_SIZE_64BIT)
	 MIPI_SYST_SCATTER_OP_64BIT,
#else
	#error unsupported pointer size for Structured Binary Data (SBD)
#endif
	 MIPI_SYST_EVDSC_MEMBER_OFF(ed_pld.data_sbd.address),
	 1}
	 ,
	{			/* SCATTER_OP_SBD_NAME */
	 MIPI_SYST_SCATTER_OP_BLOB,
	 MIPI_SYST_EVDSC_MEMBER_OFF(ed_pld.data_sbd.name),
	 0}
	 ,
	{			/* SCATTER_OP_SBD_BLOB */
	 MIPI_SYST_SCATTER_OP_BLOB,
	 MIPI_SYST_EVDSC_MEMBER_OFF(ed_pld.data_sbd.blob),
	 0}
	 ,
#endif /* #if defined(MIPI_SYST_PCFG_ENABLE_SBD_API) */

	{ /* SCATTER_OP_END */
	 MIPI_SYST_SCATTER_OP_END,
	 0,
	 0}
};


/**
 * Add optional message components to the message descriptor
 */
static
#if defined(MIPI_SYST_PCFG_ENABLE_INLINE)
MIPI_SYST_CC_INLINE
#endif
void
insert_optional_msg_components(struct mipi_syst_handle* svh,
				 struct mipi_syst_msglocation* loc,
				 mipi_syst_u16 len,
				 struct mipi_syst_msgdsc *desc,
				 struct mipi_syst_scatter_prog **prog_ptr)
{
	struct mipi_syst_scatter_prog *prog = *prog_ptr;

#if defined(MIPI_SYST_PCFG_ENABLE_ORIGIN_GUID)
	/* origin GUID  ? */
    if (0 != desc->ed_tag.et_guid) {
		desc->ed_guid = svh->systh_guid;
		*prog++ = scatter_ops[SCATTER_OP_GUID];
	}
#endif

#if defined(MIPI_SYST_PCFG_ENABLE_LOCATION_RECORD)
	/* location information ? */
	if ((struct mipi_syst_msglocation*) 0 != loc) {
		desc->ed_loc = *loc;
		desc->ed_tag.et_location = 1;

		*prog++ = scatter_ops[SCATTER_OP_LOC_FMT];
		if (desc->ed_loc.el_format & 0x1)
			*prog++ = scatter_ops[SCATTER_OP_LOC_64];
		else
			*prog++ = scatter_ops[SCATTER_OP_LOC_32];
	}
#endif

#if defined(MIPI_SYST_PCFG_LENGTH_FIELD)
	/* pay load length */
	if(0 != desc->ed_tag.et_length) {
		desc->ed_len = len;
		*prog++ = scatter_ops[SCATTER_OP_LENGTH];
	}
#endif

#if defined(MIPI_SYST_PCFG_ENABLE_TIMESTAMP)
	if (desc->ed_tag.et_timestamp) {
		/* timestamp present */
		desc->ed_ts = MIPI_SYST_PLATFORM_CLOCK();
		*prog++ = scatter_ops[SCATTER_OP_TS];
	}
#endif /* defined(MIPI_SYST_PCFG_ENABLE_TIMESTAMP) */

	*prog_ptr = prog;
}
#endif

#if defined(MIPI_SYST_PCFG_ENABLE_STRING_API)

/**
 * Write a string output message
 *
 * @param svh SyS-T handle
 * @param loc Pointer to instrumentation location or null if no location
 * @param type string message subtype
 * @param severity severity level (0..7)
 * @param len number of bytes to emit or 0 to send fixed size 32bytes
 * @param str  pointer to UTF-8 string bytes
 */
MIPI_SYST_EXPORT void MIPI_SYST_CALLCONV
mipi_syst_write_debug_string(struct mipi_syst_handle* svh,
			  struct mipi_syst_msglocation* loc,
			  enum mipi_syst_subtype_string type,
			  enum mipi_syst_severity severity,
			  mipi_syst_u16 len, const char *str)
{
	struct mipi_syst_msgdsc desc;
	struct mipi_syst_scatter_prog prog[MIPI_SYST_SCATTER_PROG_LEN];
	struct mipi_syst_scatter_prog *prog_ptr = prog;
	mipi_syst_u64 errmsg;

	if ((struct mipi_syst_handle*)0 == svh)
		return;

	/* assign tag */
	desc.ed_tag = svh->systh_tag;
	desc.ed_tag.et_type = MIPI_SYST_TYPE_STRING;
	desc.ed_tag.et_subtype = type;
	desc.ed_tag.et_severity = severity;

	if ((const char *)0 == str) {
		desc.ed_tag.et_subtype = MIPI_SYST_STRING_INVALIDPARAM;
		errmsg =
#if defined(MIPI_SYST_BIG_ENDIAN)
			0x286e756c6c290000ull; /* == "(null)\0\0" */
#else
			0x0000296c6c756e28ull; /* == "(null)\0\0" */
#endif
		str = (char*)&errmsg;
		len = 7;
	}
	insert_optional_msg_components(svh, loc, len, &desc, &prog_ptr);

	desc.ed_pld.data_var = (const mipi_syst_u8 *) str;
	*prog_ptr = scatter_ops[SCATTER_OP_PAYLD_VAR];
	prog_ptr->sso_length = len;
	++prog_ptr;

	*prog_ptr = scatter_ops[SCATTER_OP_END];

	ASSERT_CHECK(prog_ptr < &prog[MIPI_SYST_SCATTER_PROG_LEN]);

	/* call IO routine to dump out the message */
	MIPI_SYST_SCATTER_WRITE(svh, prog, &desc);
}
#endif				/* #if defined(MIPI_SYST_PCFG_ENABLE_STRING_API) */

#if defined(MIPI_SYST_PCFG_ENABLE_CATID64_API)

/**
 * Write 64-bit catalog message
 *
 * @param svh  SyS-T handle
 * @param loc  Pointer to instrumentation location or null
 * @param severity message severity level (0..7)
 * @param catid catalog ID
 */
MIPI_SYST_EXPORT void MIPI_SYST_CALLCONV
mipi_syst_write_catalog64_message(struct mipi_syst_handle* svh,
			       struct mipi_syst_msglocation* loc,
			       enum mipi_syst_severity severity, mipi_syst_u64 catid)
{
	struct mipi_syst_msgdsc desc;
	struct mipi_syst_scatter_prog prog[MIPI_SYST_SCATTER_PROG_LEN];
	struct mipi_syst_scatter_prog *prog_ptr = prog;
	mipi_syst_u16 paramlen;

	if ((struct mipi_syst_handle*)0 == svh)
		return;

	/* assign tag */
	desc.ed_tag = svh->systh_tag;
	desc.ed_tag.et_type = MIPI_SYST_TYPE_CATALOG;
	desc.ed_tag.et_severity = severity;
#if defined(MIPI_SYST_PCFG_ENABLE_64BIT_ADDR)
	desc.ed_tag.et_subtype = MIPI_SYST_CATALOG_ID64_P64;
#else
	desc.ed_tag.et_subtype = MIPI_SYST_CATALOG_ID64_P32;
#endif

	paramlen = (mipi_syst_u16)
		(svh->systh_param_count *  sizeof(mipi_syst_u32));

	insert_optional_msg_components(
			svh, loc,
			sizeof(catid) + paramlen,
			&desc, &prog_ptr);

	/* cat ID */
	desc.ed_pld.data_catid.id.sci_64 = catid;
	*prog_ptr++ = scatter_ops[SCATTER_OP_CATID_64];

	/* parameters (if any) */

	if (0 != paramlen) {
		mipi_syst_u32 *param;
		param = svh->systh_param;
		desc.ed_pld.data_catid.param = param;
		*prog_ptr = scatter_ops[SCATTER_OP_CATID_ARGS];
		prog_ptr->sso_length = paramlen;
		++prog_ptr;
#if defined(MIPI_SYST_BIG_ENDIAN)
		while(paramlen) {
			*param = MIPI_SYST_HTOLE32(*param);
			param++;
			paramlen-=sizeof(mipi_syst_u32);
		}
#endif
	}

	*prog_ptr = scatter_ops[SCATTER_OP_END];

	ASSERT_CHECK(prog_ptr < &prog[MIPI_SYST_SCATTER_PROG_LEN]);

	/* call IO routine to dump out the message */
	MIPI_SYST_SCATTER_WRITE(svh, prog, &desc);
}

#endif /* #if defined(MIPI_SYST_PCFG_ENABLE_CATID64_API) */

#if defined(MIPI_SYST_PCFG_ENABLE_CATID32_API)

/**
 * Write 32-Bit catalog message
 *
 * @param svh  SyS-T handle
 * @param loc  Pointer to instrumentation location or null
 * @param severity message severity level (0..7)
 * @param catid catalog ID
 */
MIPI_SYST_EXPORT void MIPI_SYST_CALLCONV
mipi_syst_write_catalog32_message(struct mipi_syst_handle* svh,
			       struct mipi_syst_msglocation* loc,
			       enum mipi_syst_severity severity, mipi_syst_u32 catid)
{
	struct mipi_syst_msgdsc desc;
	struct mipi_syst_scatter_prog prog[MIPI_SYST_SCATTER_PROG_LEN];
	struct mipi_syst_scatter_prog *prog_ptr = prog;
	mipi_syst_u16 paramlen;

	if ((struct mipi_syst_handle*)0 == svh)
		return;

	/* assign tag */
	desc.ed_tag = svh->systh_tag;
	desc.ed_tag.et_type = MIPI_SYST_TYPE_CATALOG;
	desc.ed_tag.et_severity = severity;
#if defined(MIPI_SYST_PCFG_ENABLE_64BIT_ADDR)
	desc.ed_tag.et_subtype = MIPI_SYST_CATALOG_ID32_P64;
#else
	desc.ed_tag.et_subtype = MIPI_SYST_CATALOG_ID32_P32;
#endif

	paramlen = (mipi_syst_u16)
		(svh->systh_param_count *  sizeof(mipi_syst_u32));

	insert_optional_msg_components(
			svh, loc,
			sizeof(catid) + paramlen,
			&desc, &prog_ptr);

	/* cat ID */
	desc.ed_pld.data_catid.id.sci_32 = catid;
	*prog_ptr++ = scatter_ops[SCATTER_OP_CATID_32];

	/* parameters (if any) */

	if (0 != paramlen) {
		mipi_syst_u32 * param;
		param = svh->systh_param;
		desc.ed_pld.data_catid.param = param;
		*prog_ptr = scatter_ops[SCATTER_OP_CATID_ARGS];
		prog_ptr->sso_length = paramlen;
		++prog_ptr;
#if defined(MIPI_SYST_BIG_ENDIAN)
		while(paramlen) {
			*param = MIPI_SYST_HTOLE32(*param);
			param++;
			paramlen-=sizeof(mipi_syst_u32);
		}
#endif
	}

	*prog_ptr = scatter_ops[SCATTER_OP_END];

	ASSERT_CHECK(prog_ptr < &prog[MIPI_SYST_SCATTER_PROG_LEN]);

	/* call IO routine to dump out the message */
	MIPI_SYST_SCATTER_WRITE(svh, prog, &desc);
}

#endif /* #if defined(MIPI_SYST_PCFG_ENABLE_CATID32_API) */

#if defined(MIPI_SYST_PCFG_ENABLE_SBD_API)

/**
 * Write 64bit SBD message
 *
 * @param svh  SyS-T handle
 * @param loc  Pointer to instrumentation location or null
 * @param severity message severity level (0..7)
 * @param sbd_id 64bit SBD ID
 * @param addr optional address of BLOB structure or NULL
 * @param name optional null-terminated UTF-8 string describing BLOB or NULL
 * @param len size of provided BLOB
 * @param blob pointer to BLOB to be captured in SBD message
 */
MIPI_SYST_EXPORT void MIPI_SYST_CALLCONV
mipi_syst_write_sbd64_message(struct mipi_syst_handle* svh,
			struct mipi_syst_msglocation* loc,
			enum mipi_syst_severity severity,
			mipi_syst_u64 sbd_id,
			mipi_syst_address addr,
			const char* name,
			mipi_syst_u32 len,
			const void *blob)
{
	struct mipi_syst_msgdsc desc;
	struct mipi_syst_scatter_prog prog[MIPI_SYST_SCATTER_PROG_LEN];
	struct mipi_syst_scatter_prog *prog_ptr = prog;

	if ((struct mipi_syst_handle*)0 == svh)
		return;

	/* assign tag */
	desc.ed_tag = svh->systh_tag;
	desc.ed_tag.et_type = MIPI_SYST_TYPE_SBD;
	desc.ed_tag.et_severity = severity;
	desc.ed_tag.et_subtype = MIPI_SYST_SBD_ID_64BIT;
	mipi_syst_u32 total_len = sizeof(sbd_id);
	mipi_syst_u32 name_len = 0;

	if (NULL != name)
	{
		desc.ed_tag.et_subtype |= MIPI_SYST_SBD_WITH_NAME;
		while(name[name_len] != 0)
		{
			++name_len; // calculate name length
		}

		//count null terminator too
		++name_len;

		total_len += name_len;
	}
	if (0 != addr)
	{
#ifdef MIPI_SYST_PTR_SIZE_16BIT
		desc.ed_tag.et_subtype |= MIPI_SYST_SBD_16BIT_ADDRESS;
#elif defined(MIPI_SYST_PTR_SIZE_32BIT)
		desc.ed_tag.et_subtype |= MIPI_SYST_SBD_32BIT_ADDRESS;
#elif defined(MIPI_SYST_PTR_SIZE_64BIT)
		desc.ed_tag.et_subtype |= MIPI_SYST_SBD_64BIT_ADDRESS;
#endif
		total_len += sizeof(addr);
	}

	total_len += len; // add BLOB length

	insert_optional_msg_components(svh, loc, total_len, &desc, &prog_ptr);

	// insert SBD ID
	desc.ed_pld.data_sbd.id.sbd_id_64 = sbd_id;
	*prog_ptr++ = scatter_ops[SCATTER_OP_SBD_ID_64];

	// add optional address (if any)
	if (0 != addr)
	{
		desc.ed_pld.data_sbd.address = addr;
		*prog_ptr++ = scatter_ops[SCATTER_OP_SBD_ADDR];
	}

	// add optional name (if any)
	if (NULL != name)
	{
		desc.ed_pld.data_sbd.name = name;
		*prog_ptr = scatter_ops[SCATTER_OP_SBD_NAME];
		prog_ptr->sso_length = name_len;
		++prog_ptr;
	}

	// add BLOB bytes
	desc.ed_pld.data_sbd.blob = blob;
	*prog_ptr = scatter_ops[SCATTER_OP_SBD_BLOB];
	prog_ptr->sso_length = len;
	++prog_ptr;

	*prog_ptr = scatter_ops[SCATTER_OP_END];

	ASSERT_CHECK(prog_ptr < &prog[MIPI_SYST_SCATTER_PROG_LEN]);

	/* call IO routine to dump out the message */
	MIPI_SYST_SCATTER_WRITE(svh, prog, &desc);
}

/**
 * Write 32bit SBD message
 *
 * @param svh  SyS-T handle
 * @param loc  Pointer to instrumentation location or null
 * @param severity message severity level (0..7)
 * @param sbd_id 32bit SBD ID
 * @param addr optional address of BLOB structure or NULL
 * @param name optional null-terminated UTF-8 string describing BLOB or NULL
 * @param len size of provided BLOB
 * @param blob pointer to BLOB to be captured in SBD message
 */
MIPI_SYST_EXPORT void MIPI_SYST_CALLCONV
mipi_syst_write_sbd32_message(struct mipi_syst_handle* svh,
			struct mipi_syst_msglocation* loc,
			enum mipi_syst_severity severity,
			mipi_syst_u32 sbd_id,
			mipi_syst_address addr,
			const char *name,
			mipi_syst_u32 len,
			const void *blob)
{
	struct mipi_syst_msgdsc desc;
	struct mipi_syst_scatter_prog prog[MIPI_SYST_SCATTER_PROG_LEN];
	struct mipi_syst_scatter_prog *prog_ptr = prog;

	if ((struct mipi_syst_handle*)0 == svh)
		return;

	/* assign tag */
	desc.ed_tag = svh->systh_tag;
	desc.ed_tag.et_type = MIPI_SYST_TYPE_SBD;
	desc.ed_tag.et_severity = severity;
	desc.ed_tag.et_subtype = MIPI_SYST_SBD_ID_32BIT;
	mipi_syst_u32 total_len = sizeof(sbd_id);
	mipi_syst_u32 name_len = 0;

	if (NULL != name)
	{
		desc.ed_tag.et_subtype |= MIPI_SYST_SBD_WITH_NAME;
		while(name[name_len] != 0)
		{
			++name_len; // calculate name length
		}

		//count null terminator too
		++name_len;

		total_len += name_len;
	}
	if (0 != addr)
	{
#ifdef MIPI_SYST_PTR_SIZE_16BIT
		desc.ed_tag.et_subtype |= MIPI_SYST_SBD_16BIT_ADDRESS;
#elif defined(MIPI_SYST_PTR_SIZE_32BIT)
		desc.ed_tag.et_subtype |= MIPI_SYST_SBD_32BIT_ADDRESS;
#elif defined(MIPI_SYST_PTR_SIZE_64BIT)
		desc.ed_tag.et_subtype |= MIPI_SYST_SBD_64BIT_ADDRESS;
#endif
		total_len += sizeof(addr);
	}

	total_len += len; // add BLOB length

	insert_optional_msg_components(svh, loc, total_len, &desc, &prog_ptr);

	// insert SBD ID
	desc.ed_pld.data_sbd.id.sbd_id_32 = sbd_id;
	*prog_ptr++ = scatter_ops[SCATTER_OP_SBD_ID_32];

	// add optional address (if any)
	if (0 != addr)
	{
		desc.ed_pld.data_sbd.address = addr;
		*prog_ptr++ = scatter_ops[SCATTER_OP_SBD_ADDR];
	}

	// add optional name (if any)
	if (NULL != name)
	{
		desc.ed_pld.data_sbd.name = name;
		*prog_ptr = scatter_ops[SCATTER_OP_SBD_NAME];
		prog_ptr->sso_length = name_len;
		++prog_ptr;
	}

	// add BLOB bytes
	desc.ed_pld.data_sbd.blob = blob;
	*prog_ptr = scatter_ops[SCATTER_OP_SBD_BLOB];
	prog_ptr->sso_length = len;
	++prog_ptr;

	*prog_ptr = scatter_ops[SCATTER_OP_END];

	ASSERT_CHECK(prog_ptr < &prog[MIPI_SYST_SCATTER_PROG_LEN]);

	/* call IO routine to dump out the message */
	MIPI_SYST_SCATTER_WRITE(svh, prog, &desc);
}

#endif /* #if defined(MIPI_SYST_PCFG_ENABLE_SBD_API) */

#if defined(MIPI_SYST_PCFG_ENABLE_WRITE_API)

/**
 * Write raw data message
 *
 * @param svh  SyS-T handle
 * @param loc  pointer to instrumentation location or null
 * @param severity message severity level (0..7)
 * @param protocol content protocol ID
 * @param data pointer to raw data
 * @param length number of bytes to send
 */
MIPI_SYST_EXPORT void MIPI_SYST_CALLCONV
mipi_syst_write_raw_message(struct mipi_syst_handle* svh,
			struct mipi_syst_msglocation* loc,
			enum mipi_syst_severity severity,
			mipi_syst_u8 protocol,
			const void *data, mipi_syst_u16 length)
{
	struct mipi_syst_msgdsc desc;
	struct mipi_syst_scatter_prog prog[MIPI_SYST_SCATTER_PROG_LEN];
	struct mipi_syst_scatter_prog *prog_ptr = prog;

	if ((struct mipi_syst_handle*)0 == svh)
		return;

	/* assign tag */
	desc.ed_tag = svh->systh_tag;
	desc.ed_tag.et_type = MIPI_SYST_TYPE_RAW;
	desc.ed_tag.et_severity = severity;
	desc.ed_tag.et_subtype = protocol;

	insert_optional_msg_components(svh, loc, length, &desc, &prog_ptr);

	desc.ed_pld.data_var = data;
	*prog_ptr = scatter_ops[SCATTER_OP_PAYLD_VAR];
	prog_ptr->sso_length = length;
	++prog_ptr;

	*prog_ptr = scatter_ops[SCATTER_OP_END];

	ASSERT_CHECK(prog_ptr < &prog[MIPI_SYST_SCATTER_PROG_LEN]);

	/* call IO routine to dump out the message */
	MIPI_SYST_SCATTER_WRITE(svh, prog, &desc);
}
#endif /* defined(MIPI_SYST_PCFG_ENABLE_WRITE_API) */

#if defined(MIPI_SYST_PCFG_ENABLE_BUILD_API)

/**
 * Write client build version message
 *
 * @param svh  SyS-T handle
 * @param loc  pointer to instrumentation location or null
 * @param severity message severity level (0..7)
 * @param id 64-Bit version ID
 * @param text pointer to UTF-8 version text
 * @param length number of bytes to send
 */
MIPI_SYST_EXPORT void MIPI_SYST_CALLCONV
mipi_syst_write_build_message(struct mipi_syst_handle* svh,
			struct mipi_syst_msglocation* loc,
			enum mipi_syst_severity severity,
			mipi_syst_u64 id,
			const char *text, mipi_syst_u16 length)
{
	struct mipi_syst_msgdsc desc;
	struct mipi_syst_scatter_prog prog[MIPI_SYST_SCATTER_PROG_LEN];
	struct mipi_syst_scatter_prog *prog_ptr = prog;

	if ((struct mipi_syst_handle*)0 == svh)
		return;

	/* assign tag */
	desc.ed_tag = svh->systh_tag;
	desc.ed_tag.et_type = MIPI_SYST_TYPE_BUILD;
	desc.ed_tag.et_severity = severity;
	desc.ed_tag.et_subtype = MIPI_SYST_BUILD_ID_LONG;

	insert_optional_msg_components(
		svh, loc, length + sizeof(id), &desc, &prog_ptr);

	desc.ed_pld.data_version.id = id;
	*prog_ptr = scatter_ops[SCATTER_OP_VER_ID];
	++prog_ptr;
	if (0 != length) {
		desc.ed_pld.data_version.text = text;
		*prog_ptr = scatter_ops[SCATTER_OP_VER_TXT];
		prog_ptr->sso_length = length;
		++prog_ptr;
	}
	*prog_ptr = scatter_ops[SCATTER_OP_END];

	ASSERT_CHECK(prog_ptr < &prog[MIPI_SYST_SCATTER_PROG_LEN]);

	/* call IO routine to dump out the message */
	MIPI_SYST_SCATTER_WRITE(svh, prog, &desc);
}
#endif /* defined(MIPI_SYST_PCFG_ENABLE_BUILD_API) */

#if defined(MIPI_SYST_PCFG_ENABLE_TIMESTAMP)
/**
* Write clock sync message
*
* @param svh SyS-T handle
* @param loc pointer to instrumentation location or null
* @param fmt clock sync message subtype
* @param clock_value 64-Bit clock value
* @param clock_freq 64-Bit clock frequency in herz
*/
MIPI_SYST_EXPORT void MIPI_SYST_CALLCONV
mipi_syst_write_clock(struct mipi_syst_handle* svh,
		struct mipi_syst_msglocation* loc,
		enum mipi_syst_subtype_clock fmt,
		mipi_syst_u64 clock_value,
		mipi_syst_u64 clock_freq)
{
	struct mipi_syst_msgdsc desc;
	struct mipi_syst_scatter_prog prog[MIPI_SYST_SCATTER_PROG_LEN];
	struct mipi_syst_scatter_prog *prog_ptr;

	if ((struct mipi_syst_handle*)0 == svh)
		return;

	prog_ptr = prog;

	/* assign tag */
	desc.ed_tag = svh->systh_tag;
	desc.ed_tag.et_subtype = fmt;
	desc.ed_tag.et_type = MIPI_SYST_TYPE_CLOCK;
	desc.ed_tag.et_severity = MIPI_SYST_SEVERITY_MAX;

	insert_optional_msg_components(
			svh, loc,
			2*sizeof(mipi_syst_u64),
			&desc, &prog_ptr);

	desc.ed_pld.data_clock[0] = clock_value;
	desc.ed_pld.data_clock[1] = clock_freq;
	*prog_ptr++ = scatter_ops[SCATTER_OP_CLOCK];

	*prog_ptr = scatter_ops[SCATTER_OP_END];
	ASSERT_CHECK(prog_ptr < &prog[MIPI_SYST_SCATTER_PROG_LEN]);

	MIPI_SYST_SCATTER_WRITE(svh, prog, &desc);
}

#endif /* defined(MIPI_SYST_PCFG_ENABLE_TIMESTAMP) */

#if defined(MIPI_SYST_PCFG_ENABLE_PRINTF_API)

/* printf requires stdarg from the compiler in use. It is a varargs function*/

#include <stdarg.h>
#include <stddef.h>
#include <wchar.h>

#if !defined(MIPI_SYST_PCFG_PRINTF_ARGBUF_SIZE)
#define MIPI_SYST_PCFG_PRINTF_ARGBUF_SIZE 1024  /* Default 1Kb arg buffer */
#endif

/** state IDs of the printf format string parser's finite state machine
 */
enum FmtScanState {
	START,
	PLAINTEXT,
	PERCENT,
	FLAGS,
	WIDTH, WIDTH_NUMBER,
	PRECISION_DOT, PRECISION_VAL, PRECISION_NUMBER,
	MODIFIER,
	MODIFIER_HALF,
	MODIFIER_LONG,
	SPECIFIER
};

/** format modifier types
 */
enum Modifier {
	MOD_NONE, MOD_HH, MOD_H, MOD_L, MOD_LL, MOD_J, MOD_Z, MOD_T, MOD_LD
};

/** parser result codes
 *
 */
enum ReturnCodes {
	FMT_PARSE_OK = 1,
	FMT_PARSE_ARG_BUFFER_TOO_SMALL = -1,
	FMT_PARSE_UNSUPPORTED_FORMAT   = -2
};

/* Helper macro to copy an argument from the arguments into the
 * payload buffer.
 *
 *  TOTYPE is the data type that gets stored in the messages's arg buffer
 *  FROMTYPE is the data type taken from the printf stack via varags
 */
#define COPY_ARG_DOUBLE(TOTYPE, FROMTYPE)				\
  do {									\
    union {mipi_syst_u64 v; TOTYPE d;} val;				\
    val.d = (TOTYPE)va_arg(args, FROMTYPE);				\
    if (argp + sizeof(TOTYPE) < argEob) {				\
      val.v = MIPI_SYST_HTOLE64(val.v);					\
      *((TOTYPE *)argp) = val.d;					\
      argp += sizeof(TOTYPE);						\
    } else {								\
      return FMT_PARSE_ARG_BUFFER_TOO_SMALL;				\
    }									\
  } while(0)

#define COPY_ARG32(TOTYPE, FROMTYPE)					\
  do {									\
    if (argp + sizeof(TOTYPE) < argEob) {				\
      *((TOTYPE *)argp) = (TOTYPE)MIPI_SYST_HTOLE32(va_arg(args, FROMTYPE)); \
      argp += sizeof(TOTYPE);						\
    } else {								\
      return FMT_PARSE_ARG_BUFFER_TOO_SMALL;				\
    }									\
  } while(0)

#define COPY_ARG64(TOTYPE, FROMTYPE)					\
  do {									\
    if (argp + sizeof(TOTYPE) < argEob) {				\
      *((TOTYPE *)argp) = (TOTYPE)MIPI_SYST_HTOLE64(va_arg(args, FROMTYPE));\
      argp += sizeof(TOTYPE);						\
    } else {								\
      return FMT_PARSE_ARG_BUFFER_TOO_SMALL;				\
    }									\
  } while(0)

/** Create the payload buffer for a printf format
 *
 * The payload of a printf message, starts with an UTF-8 string,
 * which becomes the printf format. Depending on the format string, further
 * parameters follow the format string using the following rules:
 *
 * All printf arguments follow the format string without alignment bytes added.
 * Strings for the '%s' format specifier are embedded as UTF-8 strings with
 * 0-byte termination into the payload.
 * printf arguments use the binary data layout as defined in the table below.
 *
 * Data Type                 |  32 Bit Platform   |   64 Bit Platform
 * --------------------------|--------------------|---------------------------
 * int, unsigned int         |   32 Bits          |   32 Bits
 * long, unsigned long       |   32 Bits          |   64 Bits
 * long long, unsinged long long|64 Bits          |   64 Bits
 * size_t                    |   32 Bits          |   64 Bits
 * ptrdiff_t                 |   32 Bits          |   64 Bits
 * float double, long double |   64 Bits          |   64 Bits
 * char, unsigned char       |   32 Bits          |   32 Bits
 * wchar_t                   |   32 Bits          |   32 Bits
 * Addresses(pointers)       |   32 Bits          |   64 Bits
 * Strings (char *)          |UTF-8, 0-terminated |  UTF-8, 0-terminated
 *
 * The format string follows the C99 definition which can contain format
 * specifier following this pattern:
 *
 *             %[flags][width][.precision][length]specifier
 *
 * This function 'only' converts the fmt and arguments into a message payload.
 * The actual formatting of the data into a string is done by the receiving
 * trace decoder.
 *
 * @param buffer memory buffer filled with argument data
 * @param size   # of bytes in buffer
 * @param fmt    printf format string
 * @param args   printf varags

 */
static int buildPrintfPayload(
	char * buffer,
	mipi_syst_u32 size,
	const char * fmt,
	va_list args)
{
	char * argEob;
	char * argp;
	const char * fmtp;
	enum FmtScanState state;
	enum Modifier     modifier;

	if (0 == fmt) return FMT_PARSE_UNSUPPORTED_FORMAT;

	argp   = buffer;
	argEob = buffer + size;  /* end of buffer address */
	fmtp   = fmt;

	/* copy argument string to start of payload buffer
	*/
	while (argp < argEob) {
		if( 0 == (*argp++ = *fmtp++)) break;
	}
	if (argp == argEob) return  FMT_PARSE_ARG_BUFFER_TOO_SMALL;

	fmtp     = fmt;
	state    = START;
	modifier = MOD_NONE;

	/* loop over the arguments in the format and full the arg buffer
	*/
	while( *fmtp != 0 )
	{
		switch(state) {
		case START:
			modifier      = MOD_NONE;

			state = PLAINTEXT;
			; /* deliberate fall through */

		case PLAINTEXT:
			if (*fmtp == '%') {
				state = PERCENT;
			}
			break;

		case PERCENT:
			if (*fmtp == '%') { /* '%%' is not a format, but the % char */
				state = PLAINTEXT;
			} else {
				/* arg fmt definition is starting */
				state = FLAGS;
				continue;
			}
			break;

		case FLAGS:
			switch(*fmtp) {
			case '-':
			case '+':
			case ' ':
			case '#':
			case '0':
				break;
			default:
				state = WIDTH;
				continue;
			}
			break;

		case WIDTH:
			if (*fmtp == '*') {
				COPY_ARG32(mipi_syst_s32, int);
				state = PRECISION_DOT;
			} else {
				state = WIDTH_NUMBER;
				continue;
			}
			break;

		case WIDTH_NUMBER:
			if (*fmtp < '0' || *fmtp > '9') {  /* !isdigit */
				state = PRECISION_DOT;
				continue;
			}
			break;

		case PRECISION_DOT:
			if (*fmtp == '.') {
				state = PRECISION_VAL;
			} else {
				state = MODIFIER;
				continue;
			}
			break;

		case PRECISION_VAL:
			if (*fmtp == '*') {
				COPY_ARG32(mipi_syst_s32, int);
				state = MODIFIER;
			} else {
				state = PRECISION_NUMBER;
				continue;
			}
			break;

		case PRECISION_NUMBER:
			if (*fmtp < '0' || *fmtp > '9') {  /* !isdigit */
				state = MODIFIER;
				continue;
			}
			break;

		case MODIFIER:
			state = SPECIFIER;

			switch(*fmtp){
			case 'h':
				modifier = MOD_H;
				state = MODIFIER_HALF;
				break;
			case 'l':
				modifier = MOD_L;
				state = MODIFIER_LONG;
				break;
			case 'j':
				modifier = MOD_J;
				break;
			case 'z':
				modifier = MOD_Z;
				break;
			case 't':
				modifier = MOD_T;
				break;
			case 'L':
				modifier = MOD_LD;
				break;
			default:
				continue;
			}
			break;

		case MODIFIER_HALF:
			state = SPECIFIER;
			if (*fmtp == 'h') {
				modifier = MOD_HH;
				break;
			} else {
				continue;
			}
			break;


		case MODIFIER_LONG:
			state = SPECIFIER;
			if (*fmtp == 'l') {
				modifier = MOD_LL;
				break;
			} else {
				continue;
			}

		case SPECIFIER:
			{
				switch(*fmtp) {
				case 'd':
				case 'i':
				case 'u':
				case 'o':
				case 'x':
				case 'X':
					switch(modifier) {
					case MOD_L:
#if defined(MIPI_SYST_PCFG_ENABLE_64BIT_ADDR)
						COPY_ARG64(mipi_syst_u64, long);
#else
						COPY_ARG32(mipi_syst_u32, unsigned long);
#endif
						break;
					case MOD_LL:
					case MOD_J:
						COPY_ARG64(mipi_syst_u64, unsigned long long);
						break;
					case MOD_Z:
#if defined(MIPI_SYST_PCFG_ENABLE_64BIT_ADDR)
						COPY_ARG64(mipi_syst_u64, size_t);
#else
						COPY_ARG32(mipi_syst_u32, size_t);
#endif
						break;
					case MOD_T:
#if defined(MIPI_SYST_PCFG_ENABLE_64BIT_ADDR)
						COPY_ARG64(mipi_syst_s64, ptrdiff_t);
#else
						COPY_ARG32(mipi_syst_s32, ptrdiff_t);
#endif
						break;
					default:
						COPY_ARG32(mipi_syst_u32, unsigned int);
						break;
					}
					state = START;
					break;
				case 'f':
				case 'F':
				case 'e':
				case 'E':
				case 'g':
				case 'G':
				case 'a':
				case 'A':
					if (modifier == MOD_LD) {
						COPY_ARG_DOUBLE(double, long double); /* only double*/
					} else {
						COPY_ARG_DOUBLE(double, double);
					}
					break;
				case 'c':
					if (modifier == MOD_L) {
					  COPY_ARG32(mipi_syst_u32, wchar_t);
					} else {
					  COPY_ARG32(mipi_syst_u32, int);
					}
					break;
				case 'p':
				case 'n':
#if defined(MIPI_SYST_PCFG_ENABLE_64BIT_ADDR)
				  COPY_ARG64(mipi_syst_u64, void *);
#else
				  COPY_ARG32(mipi_syst_u32, void *);
#endif
				  break;
				case 's':
					{
						/* Embed string with 0-byte into arg buffer */
						const char * p;
						p = va_arg(args, char *);
						while (argp < argEob) {
							if (0 == (*argp++ = *p++)) {
								break;
							}
							if (argp == argEob) {
								return  FMT_PARSE_ARG_BUFFER_TOO_SMALL;
							}
						}
					}
					break;

				default:
					return FMT_PARSE_UNSUPPORTED_FORMAT;
					break;
				}
				state = START;
			}
			break;
		}
		++fmtp;
	}

	if (state == START || state == PLAINTEXT) {
		return (int)(argp - buffer);
	} else {
		return FMT_PARSE_UNSUPPORTED_FORMAT;
	}
}
#if defined(MIPI_SYST_PCFG_ENABLE_CATID64_API) || defined(MIPI_SYST_PCFG_ENABLE_CATID32_API)
static int buildCatalogPayload(
	mipi_syst_u8 * buffer,
	mipi_syst_u32 size,
	va_list args)
{
	mipi_syst_u8 * argEob;
	mipi_syst_u8 * argp;
	int    argType;

	argp   = buffer;
	argEob = buffer + size;  /* end of buffer address */


	/* loop over the argument types
	*/
	for(argType = va_arg(args, int);
		argType != 0;
		argType = va_arg(args, int))
	{
		switch(argType) {
		case _MIPI_SYST_CATARG_D:
			COPY_ARG32(mipi_syst_u32, unsigned int);
			break;
		case _MIPI_SYST_CATARG_LD:
#if defined(MIPI_SYST_PCFG_ENABLE_64BIT_ADDR)
			COPY_ARG64(mipi_syst_u64, long);
#else
			COPY_ARG32(mipi_syst_u32, unsigned long);
#endif
			break;

		case _MIPI_SYST_CATARG_LLD:
			COPY_ARG64(mipi_syst_u64, unsigned long long);
			break;

		case _MIPI_SYST_CATARG_ZD:
#if defined(MIPI_SYST_PCFG_ENABLE_64BIT_ADDR)
			COPY_ARG64(mipi_syst_u64, size_t);
#else
			COPY_ARG32(mipi_syst_u32, size_t);
#endif
			break;

		case _MIPI_SYST_CATARG_TD:
#if defined(MIPI_SYST_PCFG_ENABLE_64BIT_ADDR)
			COPY_ARG64(mipi_syst_s64, ptrdiff_t);
#else
			COPY_ARG32(mipi_syst_s32, ptrdiff_t);
#endif
			break;

		case _MIPI_SYST_CATARG_F:
			COPY_ARG_DOUBLE(double, double);
			break;
		case _MIPI_SYST_CATARG_LF:
			COPY_ARG_DOUBLE(double, long double);
			break;
		case _MIPI_SYST_CATARG_C:
			COPY_ARG32(mipi_syst_u32, int);
			break;
		case _MIPI_SYST_CATARG_HHD:
			COPY_ARG32(mipi_syst_u32, int);
			break;
		case _MIPI_SYST_CATARG_LC:
			COPY_ARG32(mipi_syst_u32, wint_t);
			break;

		case _MIPI_SYST_CATARG_P:
#if defined(MIPI_SYST_PCFG_ENABLE_64BIT_ADDR)
			COPY_ARG64(mipi_syst_u64, void *);
#else
			COPY_ARG32(mipi_syst_u32, void *);
#endif
			break;

		case _MIPI_SYST_CATARG_CSTR:
			{
				const char * p;
				p = va_arg(args, char *);
				while (argp < argEob) {
					if (0 == (*argp++ = *p++)) {
						break;
					}
					if (argp == argEob) {
						return  FMT_PARSE_ARG_BUFFER_TOO_SMALL;
					}
				}
			}
			break;

		default:
			return FMT_PARSE_UNSUPPORTED_FORMAT;
			break;
		}
	}

	return (int)(argp - buffer);
}

#endif // #if defined(MIPI_SYST_PCFG_ENABLE_CATIDxx_API)

/**
 * Write a printf message
 *
 * @param svh SyS-T handle
 * @param loc Pointer to instrumentation location or null if no location
 * @param severity severity level (0..7)
 * @param fmt pointer to UTF-8 string bytes
 * @param ... optional format arguments
 */
MIPI_SYST_EXPORT void MIPI_SYST_CALLCONV
	mipi_syst_write_printf_string(struct mipi_syst_handle* svh,
	struct mipi_syst_msglocation* loc,
	enum mipi_syst_severity severity,
	const char *fmt,
	...)
{
	char argBuf[MIPI_SYST_PCFG_PRINTF_ARGBUF_SIZE];
	int len;
	va_list args;
	struct mipi_syst_msgdsc desc;
	struct mipi_syst_scatter_prog prog[MIPI_SYST_SCATTER_PROG_LEN];
	struct mipi_syst_scatter_prog *prog_ptr = prog;
	mipi_syst_u64 errmsg;

	if ((struct mipi_syst_handle*)0 == svh)
		return;

	/* assign tag */
	desc.ed_tag = svh->systh_tag;
	desc.ed_tag.et_type = MIPI_SYST_TYPE_STRING;
#if defined(MIPI_SYST_PCFG_ENABLE_64BIT_ADDR)
	desc.ed_tag.et_subtype = MIPI_SYST_STRING_PRINTF_64;
#else
	desc.ed_tag.et_subtype = MIPI_SYST_STRING_PRINTF_32;
#endif
	desc.ed_tag.et_severity = severity;

	va_start(args, fmt);
	len = buildPrintfPayload(argBuf, sizeof(argBuf), fmt, args);
	va_end(args);

	if (len <= 0 ) {
		/* Invalid format, send up to 32 bytes from the offending format string
		 *  as string message with tag "invalid parameter" instead
		*/
		desc.ed_tag.et_subtype = MIPI_SYST_STRING_INVALIDPARAM;
		errmsg =
#if defined(MIPI_SYST_BIG_ENDIAN)
		 0x286e756c6c290000ull; /* == "(null)\0\0" */
#else
		 0x0000296c6c756e28ull; /* == "(null)\0\0" */
#endif
		fmt = fmt ? fmt : (char*)&errmsg;

		for (len = 0; len < 32;)
			if (0 == fmt[len++]) break;
	} else {
		fmt = argBuf;
	}
	insert_optional_msg_components(svh, loc, (mipi_syst_u16)len, &desc, &prog_ptr);

	*prog_ptr = scatter_ops[SCATTER_OP_PAYLD_VAR];
	desc.ed_pld.data_var = (const mipi_syst_u8 *) fmt;
	prog_ptr->sso_length = (mipi_syst_u16)len;
	++prog_ptr;
	*prog_ptr = scatter_ops[SCATTER_OP_END];

	ASSERT_CHECK(prog_ptr < &prog[MIPI_SYST_SCATTER_PROG_LEN]);

	/* call IO routine to dump out the message */
	MIPI_SYST_SCATTER_WRITE(svh, prog, &desc);
}

#if defined(MIPI_SYST_PCFG_ENABLE_CATID64_API)

/**
 * Write a printf catalog message with 64bit ID
 *
 * @param svh SyS-T handle
 * @param loc Pointer to instrumentation location or null if no location
 * @param severity severity level (0..7)
 * @param id  catalog id
 */
MIPI_SYST_EXPORT void MIPI_SYST_CALLCONV
	mipi_syst_write_printf_catalog64(struct mipi_syst_handle* svh,
	struct mipi_syst_msglocation* loc,
	enum mipi_syst_severity severity,
	mipi_syst_u64 id,
	...)
{
	mipi_syst_u8 argBuf[MIPI_SYST_PCFG_PRINTF_ARGBUF_SIZE+sizeof(id)];
	int len;
	va_list args;
	struct mipi_syst_msgdsc desc;
	struct mipi_syst_scatter_prog prog[MIPI_SYST_SCATTER_PROG_LEN];
	struct mipi_syst_scatter_prog *prog_ptr = prog;
	mipi_syst_u64 errmsg;

	if ((struct mipi_syst_handle*)0 == svh)
		return;

	/* assign tag */
	desc.ed_tag = svh->systh_tag;
	desc.ed_tag.et_type = MIPI_SYST_TYPE_CATALOG;
#if defined(MIPI_SYST_PCFG_ENABLE_64BIT_ADDR)
	desc.ed_tag.et_subtype = MIPI_SYST_CATALOG_ID64_P64;
#else
	desc.ed_tag.et_subtype = MIPI_SYST_CATALOG_ID64_P32;
#endif
	desc.ed_tag.et_severity = severity;

	va_start(args, id);
	len = buildCatalogPayload(argBuf+sizeof(id),
	                          MIPI_SYST_PCFG_PRINTF_ARGBUF_SIZE,
	                          args);
	va_end(args);

	if (len < 0 ) {
		char * msg;
		errmsg =
#if defined(MIPI_SYST_BIG_ENDIAN)
			0x6361746172670000ull; /* = "catarg\0\0" */
#else
			0x0000677261746163ull; /* = "catarg\0\0" */
#endif
		/* Invalid parameter list */
		desc.ed_tag.et_type = MIPI_SYST_TYPE_STRING;
		desc.ed_tag.et_subtype = MIPI_SYST_STRING_INVALIDPARAM;
		len = 0;
		msg = (char*)&errmsg;
		while (0 != (argBuf[len++] = *msg++));
	} else {
	  *(mipi_syst_u64*)argBuf = MIPI_SYST_HTOLE64(id);
		len += sizeof(id);
	}

	insert_optional_msg_components(svh, loc, (mipi_syst_u16)len, &desc, &prog_ptr);

	*prog_ptr = scatter_ops[SCATTER_OP_PAYLD_VAR];
	desc.ed_pld.data_var = (const mipi_syst_u8 *) argBuf;
	prog_ptr->sso_length = (mipi_syst_u16)len;
	++prog_ptr;
	*prog_ptr = scatter_ops[SCATTER_OP_END];

	ASSERT_CHECK(prog_ptr < &prog[MIPI_SYST_SCATTER_PROG_LEN]);

	/* call IO routine to dump out the message */
	MIPI_SYST_SCATTER_WRITE(svh, prog, &desc);
}
#endif // #if defined(MIPI_SYST_PCFG_ENABLE_CATID64_API)
#if defined(MIPI_SYST_PCFG_ENABLE_CATID32_API)

/**
 * Write a printf catalog message with 32bit ID
 *
 * @param svh SyS-T handle
 * @param loc Pointer to instrumentation location or null if no location
 * @param severity severity level (0..7)
 * @param id  catalog id
 */
MIPI_SYST_EXPORT void MIPI_SYST_CALLCONV
	mipi_syst_write_printf_catalog32(struct mipi_syst_handle* svh,
	struct mipi_syst_msglocation* loc,
	enum mipi_syst_severity severity,
	mipi_syst_u32 id,
	...)
{
	mipi_syst_u8 argBuf[MIPI_SYST_PCFG_PRINTF_ARGBUF_SIZE+sizeof(id)];
	int len;
	va_list args;
	struct mipi_syst_msgdsc desc;
	struct mipi_syst_scatter_prog prog[MIPI_SYST_SCATTER_PROG_LEN];
	struct mipi_syst_scatter_prog *prog_ptr = prog;
	mipi_syst_u64 errmsg;

	if ((struct mipi_syst_handle*)0 == svh)
		return;

	/* assign tag */
	desc.ed_tag = svh->systh_tag;
	desc.ed_tag.et_type = MIPI_SYST_TYPE_CATALOG;
#if defined(MIPI_SYST_PCFG_ENABLE_64BIT_ADDR)
	desc.ed_tag.et_subtype = MIPI_SYST_CATALOG_ID32_P64;
#else
	desc.ed_tag.et_subtype = MIPI_SYST_CATALOG_ID32_P32;
#endif
	desc.ed_tag.et_severity = severity;

	va_start(args, id);
	len = buildCatalogPayload(argBuf+sizeof(id),
			MIPI_SYST_PCFG_PRINTF_ARGBUF_SIZE,
			args);
	va_end(args);

	if (len < 0 ) {
		char * msg;
		errmsg =
#if defined(MIPI_SYST_BIG_ENDIAN)
			0x6361746172670000ull; /* = "catarg\0\0" */
#else
			0x0000677261746163ull; /* = "catarg\0\0" */
#endif
		/* Invalid parameter list */
		desc.ed_tag.et_type = MIPI_SYST_TYPE_STRING;
		desc.ed_tag.et_subtype = MIPI_SYST_STRING_INVALIDPARAM;
		len = 0;
		msg = (char*)&errmsg;
		while (0 != (argBuf[len++] = *msg++));
	} else {
	  *(mipi_syst_u32*)argBuf = MIPI_SYST_HTOLE32(id);
		len += sizeof(id);
	}

	insert_optional_msg_components(svh, loc, (mipi_syst_u16)len, &desc, &prog_ptr);

	*prog_ptr = scatter_ops[SCATTER_OP_PAYLD_VAR];
	desc.ed_pld.data_var = (const mipi_syst_u8 *) argBuf;
	prog_ptr->sso_length = (mipi_syst_u16)len;
	++prog_ptr;
	*prog_ptr = scatter_ops[SCATTER_OP_END];

	ASSERT_CHECK(prog_ptr < &prog[MIPI_SYST_SCATTER_PROG_LEN]);

	/* call IO routine to dump out the message */
	MIPI_SYST_SCATTER_WRITE(svh, prog, &desc);
}
#endif // #if defined(MIPI_SYST_PCFG_ENABLE_CATID32_API)
#endif	/* #if defined(MIPI_SYST_PCFG_ENABLE_PRINTF_API) */