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

/* Defines for CRC32C computation */

#ifndef MIPI_SYST_CRC32_INCLUDED
#define MIPI_SYST_CRC32_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif


#if !defined(MIPI_SYST_PCFG_ENABLE_CHECKSUM)

#define MIPI_SYST_CRC32_INIT(v)
#define MIPI_SYST_CRC32_GET(v)

#define MIPI_SYST_CRC32_U8(crc, v)
#define MIPI_SYST_CRC32_U16(crc, v)
#define MIPI_SYST_CRC32_U32(crc, v)
#define MIPI_SYST_CRC32_U64(crc, v)

#else

extern const mipi_syst_u32 mipi_syst_crc_table[256];

#define MIPI_SYST_CRC32_INIT(v) ((v) ^ 0xFFFFFFFF)
#define MIPI_SYST_CRC32_GET(v)  ((v) ^ 0xFFFFFFFF)

#define MIPI_SYST_CRC32_U8(crc, v)   { crc = mipi_syst_crc32_8((crc), (v));  }
#define MIPI_SYST_CRC32_U16(crc, v)  { crc = mipi_syst_crc32_16((crc), (v)); }
#define MIPI_SYST_CRC32_U32(crc, v)  { crc = mipi_syst_crc32_32((crc), (v)); }
#define MIPI_SYST_CRC32_U64(crc, v)  { crc = mipi_syst_crc32_64((crc), (v)); }

#if !defined(MIPI_SYST_PCFG_ENABLE_INLINE)

MIPI_SYST_INLINE mipi_syst_u32 mipi_syst_crc32_8(mipi_syst_u32 crc, mipi_syst_u8 b);
MIPI_SYST_INLINE mipi_syst_u32 mipi_syst_crc32_16(mipi_syst_u32 crc, mipi_syst_u16 hw);
MIPI_SYST_INLINE mipi_syst_u32 mipi_syst_crc32_32(mipi_syst_u32 crc, mipi_syst_u32 hw);
MIPI_SYST_INLINE mipi_syst_u32 mipi_syst_crc32_64(mipi_syst_u32 crc, mipi_syst_u64 hw);

#else

MIPI_SYST_INLINE mipi_syst_u32 mipi_syst_crc32_8(mipi_syst_u32 crc, mipi_syst_u8 b)
{
#if (defined(MIPI_SYST_CRC_INTRINSIC))
	return _MIPI_SYST_CPU_CRC8(crc, b);
#else
	return mipi_syst_crc_table[((int) crc ^ b) & 0xff] ^ (crc >> 8);
#endif
}

MIPI_SYST_INLINE mipi_syst_u32 mipi_syst_crc32_16(mipi_syst_u32 crc, mipi_syst_u16 s)
{
#if (defined(MIPI_SYST_CRC_INTRINSIC))
	return _MIPI_SYST_CPU_CRC16(crc, s);
#else
	crc = mipi_syst_crc32_8(crc, (mipi_syst_u8) s);
	crc = mipi_syst_crc32_8(crc, (mipi_syst_u8) (s >> 8));
	return crc;
#endif
}


MIPI_SYST_INLINE mipi_syst_u32 mipi_syst_crc32_32(mipi_syst_u32 crc, mipi_syst_u32 w)
{
#if (defined(MIPI_SYST_CRC_INTRINSIC))
	return _MIPI_SYST_CPU_CRC32(crc, w);
#else

	crc = mipi_syst_crc32_8(crc, (mipi_syst_u8) w);
	crc = mipi_syst_crc32_8(crc, (mipi_syst_u8) (w >> 8));
	crc = mipi_syst_crc32_8(crc, (mipi_syst_u8) (w >> 16));
	crc = mipi_syst_crc32_8(crc, (mipi_syst_u8) (w >> 24));
	return crc;
#endif
}

MIPI_SYST_INLINE mipi_syst_u32 mipi_syst_crc32_64(mipi_syst_u32 crc, mipi_syst_u64 l)
{
#if (defined(MIPI_SYST_CRC_INTRINSIC))
	return _MIPI_SYST_CPU_CRC64(crc, l);
#else
	crc = mipi_syst_crc32_8(crc, (mipi_syst_u8) l);
	crc = mipi_syst_crc32_8(crc, (mipi_syst_u8) (l >> 8));
	crc = mipi_syst_crc32_8(crc, (mipi_syst_u8) (l >> 16));
	crc = mipi_syst_crc32_8(crc, (mipi_syst_u8) (l >> 24));
	crc = mipi_syst_crc32_8(crc, (mipi_syst_u8) (l >> 32));
	crc = mipi_syst_crc32_8(crc, (mipi_syst_u8) (l >> 40));
	crc = mipi_syst_crc32_8(crc, (mipi_syst_u8) (l >> 48));
	crc = mipi_syst_crc32_8(crc, (mipi_syst_u8) (l >> 56));
	return crc;
#endif
}
#endif
#endif		/* defined(MIPI_SYST_PCFG_ENABLE_CHECKSUM) */

#ifdef __cplusplus
}		/* extern C */
#endif

#endif
