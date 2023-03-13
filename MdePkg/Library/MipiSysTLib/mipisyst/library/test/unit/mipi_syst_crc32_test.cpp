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
#include "mipi_syst_gtest.h"

#ifdef MIPI_SYST_PCFG_ENABLE_CHECKSUM

#include "../../include/mipi_syst/crc32.h"
#include "../../src/mipi_syst_crc32.c"

/* Test input data for crc calculation tests. The last value shown
* in the array name is the reference crc32C.
*/
static mipi_syst_u8 crc_input_u8_0xB5D83007[] = {
	'0', '1', '2', '3', '4', '5', '6', '7',
	'8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
};

static mipi_syst_u16 crc_input_u16_0xAA2741E5[] = {
	0x1,  0x2,  0x3,  0x4,	0x5,  0x6,  0x7,  0x8,  0x9,  0x10,
	0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x20
};

static mipi_syst_u32 crc_input_u32_0xCDBDE657[] = {
	0x00000000, 0xABCDEF12, 0x12345678, 0xFADEDBAD, 0xAA55AA55
};

static mipi_syst_u64 crc_input_u64_0x61A6DF0B[] = {
	0x1122334455667788ull, 0x1122334455667788ull, 0xABCDEFAABBCCDDEEull,
	0x0000000000000000ull, 0xFFFFFFFFFFFFFFFFull, 0x0101010101010101ull
};

class MipiSysTFixtureCrc32 : public testing::Test
{
public:
	void SetUp() {
		crc = MIPI_SYST_CRC32_INIT(0);
	}

	void TearDown(){
	}

	mipi_syst_u32 crc32_byte_array( mipi_syst_u8 *pData, mipi_syst_u32 bytes)
	{
		while (bytes--) {
			MIPI_SYST_CRC32_U8(crc, *pData++);
		}
		return MIPI_SYST_CRC32_GET(crc);
	}

	mipi_syst_u32 crc32_word_array( mipi_syst_u16 *pData, mipi_syst_u32 words)
	{
		while (words--) {
			MIPI_SYST_CRC32_U16(crc, *pData++);
		}
		return MIPI_SYST_CRC32_GET(crc);
	}

	mipi_syst_u32 crc32_dword_array( mipi_syst_u32 *pData, mipi_syst_u32 dwords)
	{
		while (dwords--) {
			MIPI_SYST_CRC32_U32(crc, *pData++);
		}
		return MIPI_SYST_CRC32_GET(crc);
	}

	mipi_syst_u32 crc32_qword_array( mipi_syst_u64 *pData, mipi_syst_u32 qwords)
	{
		while (qwords--) {
			MIPI_SYST_CRC32_U64(crc, *pData++);
		}
		return MIPI_SYST_CRC32_GET(crc);
	}
protected:
	mipi_syst_u32 crc;
};

/* Test structure sizes used in binary output processing
*/
TEST_F(MipiSysTFixtureCrc32, syst_crc32_bytes)
{
	EXPECT_EQ(0xB5D83007 ,crc32_byte_array(crc_input_u8_0xB5D83007 ,
		sizeof(crc_input_u8_0xB5D83007))) << "MIPI_SYST_CRC32_U8() test";
}

TEST_F(MipiSysTFixtureCrc32, syst_crc32_halfwords)
{
	EXPECT_EQ(0xAA2741E5,crc32_word_array((mipi_syst_u16*)crc_input_u16_0xAA2741E5,
		sizeof(crc_input_u16_0xAA2741E5) / sizeof(mipi_syst_u16))) << "MIPI_SYST_CRC32_U16() test";
}

TEST_F(MipiSysTFixtureCrc32, syst_crc32_dwords)
{
	EXPECT_EQ(0xCDBDE657,crc32_dword_array((mipi_syst_u32*)crc_input_u32_0xCDBDE657,
		sizeof(crc_input_u32_0xCDBDE657) / sizeof(mipi_syst_u32))) << "MIPI_SYST_CRC32_U32() test";
}

TEST_F(MipiSysTFixtureCrc32, syst_crc32_quadwords)
{
	EXPECT_EQ(0x61A6DF0B,crc32_qword_array((mipi_syst_u64*)crc_input_u64_0x61A6DF0B,
		sizeof(crc_input_u64_0x61A6DF0B) / sizeof(mipi_syst_u64))) << "MIPI_SYST_CRC32_U64() test";
}

#endif