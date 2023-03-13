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

/* Internal message storage buffering */

#ifndef MIPI_syst_msg_INCLUDED
#define MIPI_syst_msg_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Catalog ID container
 */
union mipi_syst_catid {
	mipi_syst_u32 sci_32;
	mipi_syst_u64 sci_64;
};

#if defined(MIPI_SYST_PCFG_ENABLE_SBD_API)
/**
 * SBD union for 32bit/64bit ID
 */
union mipi_syst_sbd_id {
	mipi_syst_u32 sbd_id_32;
	mipi_syst_u64 sbd_id_64;
};
#endif

/**
 * SyS-T message descriptor
 *
 *  This structure stores a SyS-T message in "logical" memory format.
 *  Logical means that all optional fields are present but not necessarily
 *  used. Variable length payloads are addressed through a pointer and
 *  are not copied into the structure.
 */
 struct mipi_syst_msgdsc {
	struct mipi_syst_msg_tag ed_tag;
			     /**< 32-bit message tag  (mandatory)       */
#if defined(MIPI_SYST_PCFG_ENABLE_ORIGIN_GUID)
	struct mipi_syst_guid ed_guid; /**< origin GUID    (optional)   */
#endif

#if defined(MIPI_SYST_PCFG_ENABLE_LOCATION_RECORD)
	struct mipi_syst_msglocation ed_loc;
			     /**< message source location (optional)    */
#endif
#if defined(MIPI_SYST_PCFG_ENABLE_TIMESTAMP)
	mipi_syst_u64 ed_ts;  /**< protocol embedded time stamp         */
#endif

#if defined(MIPI_SYST_PCFG_LENGTH_FIELD)
	mipi_syst_u16 ed_len;  /**< variable payload length (optional)  */
#endif

	union {
		struct {
			mipi_syst_u64 id;
			const void *text;
		} data_version;

		struct {
			union mipi_syst_catid id;
			mipi_syst_u32 *param;
		} data_catid;

#if defined(MIPI_SYST_PCFG_ENABLE_SBD_API)
		struct {
			union mipi_syst_sbd_id id;
			mipi_syst_address address;
			const char *name;
			const void *blob;
		} data_sbd;
#endif

#if defined(MIPI_SYST_PCFG_ENABLE_TIMESTAMP)
		mipi_syst_u64 data_clock[2];
#endif
		const void *data_var;   /**< variable length payload        */
	} ed_pld;

#if defined(MIPI_SYST_PCFG_ENABLE_CHECKSUM)
	mipi_syst_u32 ed_chk;           /**< message checksum (optional)    */
#endif
};

#if defined(MIPI_SYST_PCFG_ENABLE_64BIT_ADDR)
#define MIPI_SYST_EVDSC_MEMBER_OFF(m)\
	((mipi_syst_u16)(mipi_syst_u64)&(((struct mipi_syst_msgdsc*)0)->m))
#else
#define MIPI_SYST_EVDSC_MEMBER_OFF(m)\
	((mipi_syst_u16)(mipi_syst_u32)&(((struct mipi_syst_msgdsc*)0)->m))
#endif

/**
 * message scatter write operations
 */
enum u_syst_scatter_op {
	MIPI_SYST_SCATTER_OP_SKIP = 0x00,
				/**< skip sso_length bytes          */

	MIPI_SYST_SCATTER_OP_8BIT = 0x01,
				/**< write sso_length times 8 bit   */
	MIPI_SYST_SCATTER_OP_16BIT = 0x02,
				/**< write sso_length times 16 bit  */
	MIPI_SYST_SCATTER_OP_32BIT = 0x04,
				/**< write sso_length times 32 bit  */
	MIPI_SYST_SCATTER_OP_64BIT = 0x08,
				/**< write sso_length times 64 bit  */

	MIPI_SYST_SCATTER_OP_BLOB = 0x10,
				/**< write sso_length bytes that are
				  *  accessed through a pointer   */
	MIPI_SYST_SCATTER_OP_END = 0xFF
				/**< end of scatter writer program  */
};

/**
 *  message scatter write instruction definition
 */
struct mipi_syst_scatter_prog {
	mipi_syst_u8 sso_opcode;	/**< scatter write operation
				 *   @see u_syst_scatter_op            */
	mipi_syst_u8 sso_offset;	/**< data offset in message descriptor    */
	mipi_syst_u16 sso_length;
				/**< repeat count for sso_opcode       */
};

#define MIPI_SYST_SCATTER_PROG_LEN   10    /**< maximum needed scatter prog size   */

#if defined(MIPI_SYST_PCFG_ENABLE_DEFAULT_SCATTER_WRITE)
/* default scatter write routine */
extern void mipi_syst_scatter_write(struct mipi_syst_handle* systh,
		struct mipi_syst_scatter_prog* scatterprog,
		const void *pdesc);
#endif

#ifdef __cplusplus
}	/* extern C */
#endif
#endif
