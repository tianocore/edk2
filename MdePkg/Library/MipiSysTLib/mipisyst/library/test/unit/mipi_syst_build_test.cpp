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
	defined(MIPI_SYST_PCFG_ENABLE_TIMESTAMP) &&\
	defined(MIPI_SYST_PCFG_ENABLE_BUILD_API)

class MipiSysTFixtureBuild : public MipiSysTFixtureOutput
{
public:
	void SetUp() {
		MipiSysTFixtureOutput::SetUp();
	}

	void TearDown(){
		MipiSysTFixtureOutput::TearDown();
	}

	const char * mipi_syst_write_build_message(
	struct mipi_syst_handle* svh,
	struct mipi_syst_msglocation* loc,
		enum mipi_syst_severity severity,
		mipi_syst_u64    subtype,
		const char * data,
		mipi_syst_u16 length)
	{
		sstr.str("");

		::mipi_syst_write_build_message(svh, loc, severity, subtype, data, length);
		result = sstr.str();

		return result.c_str();
	}

	static std::string result;
};

std::string MipiSysTFixtureBuild::result;


#if defined(MIPI_SYST_PCFG_ENABLE_BUILD_API)
TEST_F(MipiSysTFixtureBuild, syst_build_long)
{
	EXPECT_STREQ(
		xform(""),
		MIPI_SYST_BUILD(NULL, MIPI_SYST_SEVERITY_MAX, 0x1122334455667788ull, "some text", 10)
		);

	EXPECT_STREQ(
		xform("<D32TS>02012200[typ=0:2 mu=1:2 sev=0 len]<D16>0012<D64>1122334455667788<D64>78657420656d6f73<D16>0074<FLAG>"),
		MIPI_SYST_BUILD(ph, MIPI_SYST_SEVERITY_MAX, 0x1122334455667788ull, "some text", 10)
		);
	EXPECT_STREQ(
		xform("<D32TS>02012200[typ=0:2 mu=1:2 sev=0 len]<D16>0008<D64>1122334455667788<FLAG>"),
		MIPI_SYST_BUILD(ph, MIPI_SYST_SEVERITY_MAX, 0x1122334455667788ull, NULL, 0)
		);

#if defined(MIPI_SYST_PCFG_ENABLE_LOCATION_RECORD)
#if defined(MIPI_SYST_PCFG_ENABLE_LOCATION_ADDRESS)
#if defined(MIPI_SYST_PCFG_ENABLE_64BIT_ADDR)
	EXPECT_STREQ(
		xform("<D32TS>02012300[typ=0:2 mu=1:2 sev=0 loc len]<D8>03<D64>12345678aabbccdd<D16>0012<D64>1122334455667788<D64>78657420656d6f73<D16>0074<FLAG>"),
		MIPI_SYST_BUILD_LOCADDR(ph, MIPI_SYST_SEVERITY_MAX, 0x1122334455667788ull, "some text", 10)
	);
#else
	EXPECT_STREQ(
		xform("<D32TS>02012300[typ=0:2 mu=1:2 sev=0 loc len]<D8>02<D32>12345678<D16>0012<D64>1122334455667788<D64>78657420656d6f73<D16>0074<FLAG>"),
		MIPI_SYST_BUILD_LOCADDR(ph, MIPI_SYST_SEVERITY_MAX, 0x1122334455667788ull, "some text", 10)
	);

#endif
#endif

	EXPECT_STREQ(
		xform("<D32TS>02012300[typ=0:2 mu=1:2 sev=0 loc len]<D8>00<D32>5678abcd<D16>0012<D64>1122334455667788<D64>78657420656d6f73<D16>0074<FLAG>"),
		MIPI_SYST_BUILD_LOC16(ph, MIPI_SYST_SEVERITY_MAX, 0xabcd, 0x1122334455667788ull, "some text", 10)
	);

	EXPECT_STREQ(
		xform("<D32TS>02012300[typ=0:2 mu=1:2 sev=0 loc len]<D8>01<D64>12345678aabbccdd<D16>0012<D64>1122334455667788<D64>78657420656d6f73<D16>0074<FLAG>"),
		MIPI_SYST_BUILD_LOC32(ph, MIPI_SYST_SEVERITY_MAX, 0xaabbccdd, 0x1122334455667788ull, "some text", 10)
	);
#endif
}

#endif /* defined(MIPI_SYST_PCFG_ENABLE_BUILD_API)*/

#endif

TEST_F(MipiSysTFixtureOutput, syst_build_compact_null)
{
	std::string str;

	MIPI_SYST_BUILD_COMPACT64(0, 0x0000000000000000ull);
	str = sstr.str();

	EXPECT_STREQ(
		xform(""),
		str.c_str()
	);

	MIPI_SYST_BUILD_COMPACT32(0, 0);
	str = sstr.str();

	EXPECT_STREQ(
		xform(""),
		str.c_str()
	);
}

TEST_F(MipiSysTFixtureOutput, syst_build_compact64)
{
	std::string str;

	MIPI_SYST_BUILD_COMPACT64(ph, 0x0000000000000000ull);
	str = sstr.str();

	EXPECT_STREQ(
		xform("<D64MTS>0000000001000000"),
		str.c_str()
	);
	sstr.str("");

	MIPI_SYST_BUILD_COMPACT64(ph, 0x3FFFFFFFFFFFFFull);
	str = sstr.str();

	EXPECT_STREQ(
		xform("<D64MTS>ffffffffc1fffff0"),
		str.c_str()
	);
	sstr.str("");
}

TEST_F(MipiSysTFixtureOutput, syst_build_compact32)
{
	std::string str;

	MIPI_SYST_BUILD_COMPACT32(ph, 0x00000000ull);
	str = sstr.str();

	EXPECT_STREQ(
		xform("<D32MTS>00000000"),
		str.c_str()
	);
	sstr.str("");

	MIPI_SYST_BUILD_COMPACT32(ph, 0x3FFFFF);
	str = sstr.str();

	EXPECT_STREQ(
		xform("<D32MTS>c0fffff0"),
		str.c_str()
	);
	sstr.str("");
}