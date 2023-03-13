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
 */

#include "mipi_syst.h"
#include "mipi_syst/message.h"
#include "mipi_syst/crc32.h"

#if defined(MIPI_SYST_PCFG_ENABLE_DEFAULT_SCATTER_WRITE)

 /** scatter write routine
  *
  * This function implements the scatter write algorithm that translates
  * the logical SyS-T memory descriptor representation into output
  * requests. The actual output routines are defined through
  * the MIPI_SYST_OUTPUT_* definitions from the platform header file.
  *
  * @param systh used syst handle
  * @param scatterprog message content write instructions
  * @param pdesc pointer to memory area with message data
  */
void mipi_syst_scatter_write(struct mipi_syst_handle* systh,
	struct mipi_syst_scatter_prog *scatterprog, const void *pdesc)
{
	unsigned int repeat;

	/* Define an "any" size integer pointer to avoid casts and to simplify
	 * type based incrementing
	 */
	union {
		const void *vp;
		const mipi_syst_u8 *bp;
		const mipi_syst_u16 *hp;
		const mipi_syst_u32 *wp;
		const mipi_syst_u64 *dp;
	} data;

#if defined(MIPI_SYST_PCFG_ENABLE_CHECKSUM)
#define IFDO(a, b)  { if (a) do { b; } while (0); }

	mipi_syst_u32 crc;
	int use_crc;

	use_crc = systh->systh_tag.et_chksum;
	crc = MIPI_SYST_CRC32_INIT(0);
#else

#define IFDO(a, b)
	/* no checksump computation support */
#endif

	/* Write the "always" present tag field as a time-stamped D32 */
	MIPI_SYST_OUTPUT_D32TS(systh, *(mipi_syst_u32 *)pdesc);

	IFDO(use_crc, MIPI_SYST_CRC32_U32(crc, *(mipi_syst_u32 *)pdesc));

	/* Run the message scatter write program to dump the message contents
	 */
	while (scatterprog->sso_opcode != MIPI_SYST_SCATTER_OP_END) {
		repeat = scatterprog->sso_length;
		data.vp = pdesc;
		data.bp += scatterprog->sso_offset;

		switch (scatterprog->sso_opcode) {
		case MIPI_SYST_SCATTER_OP_8BIT:
			do {
				MIPI_SYST_OUTPUT_D8(systh, *data.bp);
				IFDO(use_crc,
					MIPI_SYST_CRC32_U8(crc, *data.bp));
				++data.bp;
			} while (--repeat);
			break;

		case MIPI_SYST_SCATTER_OP_16BIT:
			do {
				MIPI_SYST_OUTPUT_D16(systh, *data.hp);
				IFDO(use_crc,
					MIPI_SYST_CRC32_U16(crc, *data.hp));
				++data.hp;
			} while (--repeat);
			break;

		case MIPI_SYST_SCATTER_OP_32BIT:
			do {
				MIPI_SYST_OUTPUT_D32(systh, *data.wp);
				IFDO(use_crc,
					MIPI_SYST_CRC32_U32(crc, *data.wp));
				++data.wp;
			} while (--repeat);
			break;


		case MIPI_SYST_SCATTER_OP_64BIT:
			do {
#if defined(MIPI_SYST_PCFG_ENABLE_64BIT_IO)
				MIPI_SYST_OUTPUT_D64(systh, *data.dp);
#else
#if defined(MIPI_SYST_BIG_ENDIAN)
				MIPI_SYST_OUTPUT_D32(systh, data.wp[1]);
				MIPI_SYST_OUTPUT_D32(systh, data.wp[0]);
#else
				MIPI_SYST_OUTPUT_D32(systh, data.wp[0]);
				MIPI_SYST_OUTPUT_D32(systh, data.wp[1]);
#endif

#endif /* ! defined(MIPI_SYST_PCFG_ENABLE_64BIT_IO) */
				IFDO(use_crc,
					MIPI_SYST_CRC32_U64(crc, *data.dp));
				++data.dp;
			} while (--repeat);
			break;

		case MIPI_SYST_SCATTER_OP_BLOB:
			/* data location is pointer to real data,
			 * not data itself
			 */
			data.vp = *(void **)data.vp;

#if defined(MIPI_SYST_PCFG_ENABLE_64BIT_IO)

			while (repeat >= sizeof(mipi_syst_u64)) {
				mipi_syst_u64 v;
				v = MIPI_SYST_HTOLE64(*data.dp);
				MIPI_SYST_OUTPUT_D64(systh, v);
				IFDO(use_crc,
					MIPI_SYST_CRC32_U64(crc, v));
				++data.dp;
				repeat -= sizeof(mipi_syst_u64);
			}

			if (repeat >= sizeof(mipi_syst_u32)) {
				mipi_syst_u32 v;
				v = MIPI_SYST_HTOLE32(*data.wp);
				MIPI_SYST_OUTPUT_D32(systh, v);
				IFDO(use_crc,
					MIPI_SYST_CRC32_U32(crc,
						*data.wp));
				++data.wp;
				repeat -= sizeof(mipi_syst_u32);
			}
#else
			while (repeat >= sizeof(mipi_syst_u32)) {
				mipi_syst_u32 v;
				v = MIPI_SYST_HTOLE32(*data.wp);
				MIPI_SYST_OUTPUT_D32(systh, v);

				IFDO(use_crc,
					MIPI_SYST_CRC32_U32(crc, v));
				++data.wp;
				repeat -= sizeof(mipi_syst_u32);
			}
#endif
			if (repeat >= sizeof(mipi_syst_u16)) {
				mipi_syst_u16 v;
				v = MIPI_SYST_HTOLE16(*data.hp);
				MIPI_SYST_OUTPUT_D16(systh, v);

				IFDO(use_crc,
					MIPI_SYST_CRC32_U16(crc, v));
				++data.hp;
				repeat -= sizeof(mipi_syst_u16);
			}

			if (repeat) {
				MIPI_SYST_OUTPUT_D8(systh,
					*data.bp);
				IFDO(use_crc,
					MIPI_SYST_CRC32_U8(crc,	*data.bp));
			}
			break;
		}
		++scatterprog;
	}

#if defined(MIPI_SYST_PCFG_ENABLE_CHECKSUM)
	if (use_crc) {
		crc = MIPI_SYST_CRC32_GET(crc);
		MIPI_SYST_OUTPUT_D32(systh, crc);
	}
#endif

	/* EVENT end of record mark */
	MIPI_SYST_OUTPUT_FLAG(systh);
}

#endif /* defined(MIPI_SYST_PCFG_ENABLE_DEFAULT_SCATTER_WRITE) */