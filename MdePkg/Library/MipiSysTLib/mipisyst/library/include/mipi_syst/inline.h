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

/*  Instrumentation API inline code definitions */

#ifndef MIPI_SYST_INLINE_INCLUDED
#define MIPI_SYST_INLINE_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#if defined(MIPI_SYST_PCFG_ENABLE_LOCATION_RECORD)

/**
 * Update File Location in syst handle
 * @param h syst handle pointer
 * @param f file id (16 bit)
 * @param l line number in file (16 bit)
 */
MIPI_SYST_INLINE struct mipi_syst_msglocation* MIPI_SYST_CALLCONV
mipi_syst_make_file_location32(struct mipi_syst_handle* h, mipi_syst_u16 f, mipi_syst_u16 l)
{
	if (h) {
		h->systh_location.el_format = 0;
		h->systh_location.el_u.loc32.etls_source_location.etls_fileID = f;
		h->systh_location.el_u.loc32.etls_source_location.etls_lineNo = l;
	}

	return &h->systh_location;
}
/**
 * Update File Location in syst handle
 * @param h syst handle pointer
 * @param f file id (32 bit)
 * @param l line number in file (32 bit)
 */
MIPI_SYST_INLINE struct mipi_syst_msglocation* MIPI_SYST_CALLCONV
mipi_syst_make_file_location64(struct mipi_syst_handle* h, mipi_syst_u32 f, mipi_syst_u32 l)
{
	if (h) {
		h->systh_location.el_format = 1;
		h->systh_location.el_u.loc64.etls_source_location.etls_fileID = f;
		h->systh_location.el_u.loc64.etls_source_location.etls_lineNo = l;
	}

	return &h->systh_location;
}

/**
 * Update address Location in syst handle
 * @param h syst handle pointer
 * @param p address at instrumentation point
 */
MIPI_SYST_INLINE struct mipi_syst_msglocation* MIPI_SYST_CALLCONV
mipi_syst_make_address_location(struct mipi_syst_handle* h, void *p)
{
	if (h) {
#if defined(MIPI_SYST_PCFG_ENABLE_64BIT_ADDR)

		h->systh_location.el_format = 3;
		h->systh_location.el_u.loc64.etls_code_location = (mipi_syst_u64) p;
#else
		h->systh_location.el_format = 2;
		h->systh_location.el_u.loc32.etls_code_location = (mipi_syst_u32) p;
#endif
	}
	return &h->systh_location;
}

#endif	/* defined(MIPI_SYST_PCFG_ENABLE_LOCATION_RECORD) */

/**
 * Setup handle for 0 parameters passed to catid message.
 */
MIPI_SYST_INLINE void MIPI_SYST_CALLCONV mipi_syst_make_param0(struct mipi_syst_handle* h)
{
	if (h)
		h->systh_param_count = 0;
}
/**
 * Setup handle for 1 parameter passed to catid message.
 */
MIPI_SYST_INLINE void MIPI_SYST_CALLCONV mipi_syst_make_param1(struct mipi_syst_handle* h,
						 mipi_syst_u32 p1)
{
	if (h) {
		h->systh_param_count = 1;
		h->systh_param[0] = p1;
	}
}

/**
 * Setup handle for 2 parameters passed to catid message.
 */
MIPI_SYST_INLINE void MIPI_SYST_CALLCONV mipi_syst_make_param2(struct mipi_syst_handle* h,
						 mipi_syst_u32 p1,
						 mipi_syst_u32 p2)
{
	if (h) {
		h->systh_param_count = 2;
		h->systh_param[0] = p1;
		h->systh_param[1] = p2;
	}
}

/**
 * Setup handle for 3 parameters passed to catid message.
 */
MIPI_SYST_INLINE void MIPI_SYST_CALLCONV mipi_syst_make_param3(struct mipi_syst_handle* h,
						 mipi_syst_u32 p1,
						 mipi_syst_u32 p2,
						 mipi_syst_u32 p3)
{
	if (h) {
		h->systh_param_count = 3;
		h->systh_param[0] = p1;
		h->systh_param[1] = p2;
		h->systh_param[2] = p3;
	}
}

/**
 * Setup handle for 4 parameters passed to catid message.
 */
MIPI_SYST_INLINE void MIPI_SYST_CALLCONV mipi_syst_make_param4(struct mipi_syst_handle* h,
						 mipi_syst_u32 p1,
						 mipi_syst_u32 p2,
						 mipi_syst_u32 p3,
						 mipi_syst_u32 p4)
{
	if (h) {
		h->systh_param_count = 4;
		h->systh_param[0] = p1;
		h->systh_param[1] = p2;
		h->systh_param[2] = p3;
		h->systh_param[3] = p4;
	}
}

/**
 * Setup handle for 5 parameters passed to catid message.
 */
MIPI_SYST_INLINE void MIPI_SYST_CALLCONV mipi_syst_make_param5(struct mipi_syst_handle* h,
						 mipi_syst_u32 p1,
						 mipi_syst_u32 p2,
						 mipi_syst_u32 p3,
						 mipi_syst_u32 p4,
						 mipi_syst_u32 p5)
{
	if (h) {
		h->systh_param_count = 5;
		h->systh_param[0] = p1;
		h->systh_param[1] = p2;
		h->systh_param[2] = p3;
		h->systh_param[3] = p4;
		h->systh_param[4] = p5;
	}
}

/**
 * Setup handle for 6 parameters passed to catid message.
 */
MIPI_SYST_INLINE void MIPI_SYST_CALLCONV mipi_syst_make_param6(struct mipi_syst_handle* h,
						 mipi_syst_u32 p1,
						 mipi_syst_u32 p2,
						 mipi_syst_u32 p3,
						 mipi_syst_u32 p4,
						 mipi_syst_u32 p5,
						 mipi_syst_u32 p6)
{
	if (h) {
		h->systh_param_count = 6;
		h->systh_param[0] = p1;
		h->systh_param[1] = p2;
		h->systh_param[2] = p3;
		h->systh_param[3] = p4;
		h->systh_param[4] = p5;
		h->systh_param[5] = p6;
	}
}

/**
 * Runtime computation of hash values for catalog message IDs.
 * This function is used in debug builds to avoid a code explosion
 * by the preprocessor method ( @see mipi_syst_hash_x65599 ). The
 * preprocesssor methods does compute this value at compile time only
 * if const expression optimisations are enabled.
 */
MIPI_SYST_INLINE  mipi_syst_u32 mipi_syst_hash_x65599(
	const char * p, mipi_syst_u32 length)
{
	mipi_syst_u32 hash;
	mipi_syst_u8 c;
	hash = 0;

	p += (length > 0x3F) ? (length-0x40): 0;

	while (0 != (c = *p++))
	{
		hash = hash * 65599U + c;
	}

	return hash;
}
#ifdef __cplusplus
}	/* extern C */
#endif

#endif
