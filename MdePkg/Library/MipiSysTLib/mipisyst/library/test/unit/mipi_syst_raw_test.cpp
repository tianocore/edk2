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

#if defined(MIPI_SYST_PCFG_ENABLE_PLATFORM_STATE_DATA) &&\
	defined(MIPI_SYST_PCFG_ENABLE_DEFAULT_SCATTER_WRITE) &&\
	defined(MIPI_SYST_PCFG_ENABLE_WRITE_API)

class MipiSysTFixtureRaw : public MipiSysTFixtureOutput
{
public:
	void SetUp() {
		MipiSysTFixtureOutput::SetUp();
	}

	void TearDown(){
		MipiSysTFixtureOutput::TearDown();
	}

	const char * mipi_syst_write_raw_message(
	struct mipi_syst_handle* svh,
	struct mipi_syst_msglocation* loc,
		enum mipi_syst_severity severity,
		mipi_syst_u8    subtype,
		const void * data,
		mipi_syst_u16 length)
	{
		sstr.str("");

		::mipi_syst_write_raw_message(svh, loc, severity, subtype, data, length);
		result = sstr.str();

		return result.c_str();
	}

	static std::string result;
};

std::string MipiSysTFixtureRaw::result;

TEST_F(MipiSysTFixtureRaw, syst_raw_nullptr)
{
	EXPECT_STREQ(
		"",
		MIPI_SYST_WRITE(NULL, MIPI_SYST_SEVERITY_INFO, 0x3f, NULL, 0)
		);

	EXPECT_STREQ(
		xform("<D32TS>3f012246[typ=6:3f mu=1:2 sev=4 len]<D16>0000<FLAG>"),
		MIPI_SYST_WRITE(ph, MIPI_SYST_SEVERITY_INFO, 0x3f, NULL, 0)
		);
}

TEST_F(MipiSysTFixtureRaw, syst_raw_output)
{
	mipi_syst_u8 byte = 0xab;
	mipi_syst_u16 word =MIPI_SYST_HTOLE16(0xabcd);
	mipi_syst_u32 dword =MIPI_SYST_HTOLE32(0xaabbccdd);
	mipi_syst_u64 qword =MIPI_SYST_HTOLE64(0x1122aabbccddeeffull);

	EXPECT_STREQ(
		xform("<D32TS>3f012246[typ=6:3f mu=1:2 sev=4 len]<D16>0001<D8>ab<FLAG>"),
		MIPI_SYST_WRITE(ph, MIPI_SYST_SEVERITY_INFO, 0x3f, &byte, 1)
		);
	EXPECT_STREQ(
		xform("<D32TS>3f012246[typ=6:3f mu=1:2 sev=4 len]<D16>0002<D16>abcd<FLAG>"),
		MIPI_SYST_WRITE(ph, MIPI_SYST_SEVERITY_INFO, 0x3f, &word, 2)
		);
	EXPECT_STREQ(
		xform("<D32TS>3f012246[typ=6:3f mu=1:2 sev=4 len]<D16>0004<D32>aabbccdd<FLAG>"),
		MIPI_SYST_WRITE(ph, MIPI_SYST_SEVERITY_INFO, 0x3f, &dword, 4)
		);
	EXPECT_STREQ(
		xform("<D32TS>3f012246[typ=6:3f mu=1:2 sev=4 len]<D16>0008<D64>1122aabbccddeeff<FLAG>"),
		MIPI_SYST_WRITE(ph, MIPI_SYST_SEVERITY_INFO, 0x3f, &qword, 8)
		);

#if defined(MIPI_SYST_PCFG_ENABLE_LOCATION_RECORD)
	EXPECT_STREQ(
		xform("<D32TS>3f012346[typ=6:3f mu=1:2 sev=4 loc len]<D8>00<D32>5678abcd<D16>0001<D8>ab<FLAG>"),
		MIPI_SYST_WRITE_LOC16(ph, MIPI_SYST_SEVERITY_INFO, 0xabcd, 0x3f,  &byte, 1)
		);

	EXPECT_STREQ(
		xform("<D32TS>3f012346[typ=6:3f mu=1:2 sev=4 loc len]<D8>01<D64>12345678aabbccdd<D16>0001<D8>ab<FLAG>"),
		MIPI_SYST_WRITE_LOC32(ph, MIPI_SYST_SEVERITY_INFO, 0x12345678aabbccddull, 0x3f, &byte, 1)
		);
#if defined(MIPI_SYST_PCFG_ENABLE_LOCATION_ADDRESS)
#if defined(MIPI_SYST_PCFG_ENABLE_64BIT_ADDR)
	EXPECT_STREQ(
		xform("<D32TS>3f012346[typ=6:3f mu=1:2 sev=4 loc len]<D8>03<D64>12345678aabbccdd<D16>0001<D8>ab<FLAG>"),
		MIPI_SYST_WRITE_LOCADDR(ph, MIPI_SYST_SEVERITY_INFO, 0x3f, &byte, 1)
		);
#else
	EXPECT_STREQ(
		xform("<D32TS>3f012346[typ=6:3f mu=1:2 sev=4 loc len]<D8>02<D32>12345678<D16>0001<D8>ab<FLAG>"),
		MIPI_SYST_WRITE_LOCADDR(ph, MIPI_SYST_SEVERITY_INFO, 0x3f, &byte, 1)
		);
#endif //MIPI_SYST_PCFG_ENABLE_64BIT_ADDR
#endif //MIPI_SYST_PCFG_ENABLE_LOCATION_ADDRESS
#endif //MIPI_SYST_PCFG_ENABLE_LOCATION_RECORD
}

#endif //MIPI_SYST_PCFG_ENABLE_PLATFORM_STATE_DATA && MIPI_SYST_PCFG_ENABLE_DEFAULT_SCATTER_WRITE && MIPI_SYST_PCFG_ENABLE_WRITE_API

TEST_F(MipiSysTFixtureOutput, syst_short)
{
	std::string str;

	MIPI_SYST_SHORT32(ph, 0x02345678);
	str = sstr.str();

	EXPECT_STREQ(
		xform("<D32MTS>23456781"),
		str.c_str()
	);

	sstr.str("");
	MIPI_SYST_SHORT64(ph, 0x0122334455667788);
	str = sstr.str();

	EXPECT_STREQ(
		xform("<D64MTS>1223344556677887"),
		str.c_str()
	);
}