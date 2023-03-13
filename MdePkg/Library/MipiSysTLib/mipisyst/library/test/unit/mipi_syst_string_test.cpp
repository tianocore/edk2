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
	defined(MIPI_SYST_PCFG_ENABLE_STRING_API)


class MipiSysTFixtureDebugString : public MipiSysTFixtureOutput
{
public:
	void SetUp() {
		MipiSysTFixtureOutput::SetUp();
	}

	void TearDown(){
		MipiSysTFixtureOutput::TearDown();
	}

	const char * mipi_syst_write_debug_string(struct mipi_syst_handle* svh,
	struct mipi_syst_msglocation* loc,
		enum mipi_syst_subtype_string type,
		enum mipi_syst_severity severity,
		mipi_syst_u16 len,
		const char * str)
	{
		sstr.str("");

		::mipi_syst_write_debug_string(svh, loc, type, severity, len, str);
		result = sstr.str();

		return result.c_str();
	}

	static std::string result;
};

std::string MipiSysTFixtureDebugString::result;

TEST_F(MipiSysTFixtureDebugString, syst_string_null)
{
	EXPECT_STREQ(
		xform(""),
		MIPI_SYST_DEBUG(NULL, MIPI_SYST_SEVERITY_WARNING, "", 1)
		);

	EXPECT_STREQ(
		xform("<D32TS>05012232[typ=2:5 mu=1:2 sev=3 len]<D16>0007<D32>6c756e28<D16>296c<D8>00<FLAG>"),
		MIPI_SYST_DEBUG(ph, MIPI_SYST_SEVERITY_WARNING, NULL, 1)
		);
}

TEST_F(MipiSysTFixtureDebugString, syst_func_enter_output)
{
	EXPECT_STREQ(
		xform("<D32TS>02012232[typ=2:2 mu=1:2 sev=3 len]<D16>000b<D64>3736353433323130<D16>3938<D8>00<FLAG>"),
		MIPI_SYST_FUNC_ENTER(ph, MIPI_SYST_SEVERITY_WARNING)
		);
}

TEST_F(MipiSysTFixtureDebugString, syst_func_exit_output)
{
	EXPECT_STREQ(
		xform("<D32TS>03012232[typ=2:3 mu=1:2 sev=3 len]<D16>000b<D64>3736353433323130<D16>3938<D8>00<FLAG>"),
		MIPI_SYST_FUNC_EXIT(ph, MIPI_SYST_SEVERITY_WARNING)
		);
}


TEST_F(MipiSysTFixtureDebugString, syst_debugstring_output)
{
	EXPECT_STREQ(
		xform("<D32TS>01012232[typ=2:1 mu=1:2 sev=3 len]<D16>000c<D64>6f77206f6c6c6548<D32>21646c72<FLAG>"),
		MIPI_SYST_DEBUG(ph, MIPI_SYST_SEVERITY_WARNING, "Hello world!" , 12)
		);

#if defined(MIPI_SYST_PCFG_ENABLE_CHECKSUM)
	MIPI_SYST_ENABLE_HANDLE_CHECKSUM(ph,1);
#if defined(MIPI_SYST_PCFG_LENGTH_FIELD)
	EXPECT_STREQ(
		xform("<D32TS>01012632[typ=2:1 mu=1:2 sev=3 len chk]<D16>000c<D64>6f77206f6c6c6548<D32>21646c72<D32>eab806d3<FLAG>"),
		MIPI_SYST_DEBUG(ph, MIPI_SYST_SEVERITY_WARNING, "Hello world!" , 12)
		);
#else
	EXPECT_STREQ(
		xform("<D32TS>01012432[typ=2:1 mu=1:2 sev=3 chk]<D64>6f77206f6c6c6548<D32>21646c72<D32>ff19ff90<FLAG>"),
		MIPI_SYST_DEBUG(ph, MIPI_SYST_SEVERITY_WARNING, "Hello world!", 12)
	);
#endif
	MIPI_SYST_ENABLE_HANDLE_CHECKSUM(ph,0);
#endif


#if defined(MIPI_SYST_PCFG_ENABLE_ORIGIN_GUID)
	// {8887160A-C965-463b-9F43-1EFE9FDFE3F9}
	const struct mipi_syst_guid aguid = MIPI_SYST_GEN_GUID(0x8887160A, 0xC965, 0x463b, 0x9F43, 0x1EFE9FDFE3F9);

	MIPI_SYST_SET_HANDLE_GUID_UNIT(ph, aguid, 7);
	EXPECT_STREQ(
		xform("<D32TS>01807252[typ=2:1 mu=0:7 sev=5 len]<D64>3b4665c90a168788<D64>f9e3df9ffe1e439f<D16>000c<D64>6f77206f6c6c6548<D32>21646c72<FLAG>"),
		MIPI_SYST_DEBUG(ph, MIPI_SYST_SEVERITY_USER1, "Hello world!" , 12)
		);
#endif
}

#if defined(MIPI_SYST_PCFG_ENABLE_LOCATION_RECORD)
TEST_F(MipiSysTFixtureDebugString, syst_debugstring_loc)
{
	EXPECT_STREQ(
		xform("<D32TS>01012332[typ=2:1 mu=1:2 sev=3 loc len]<D8>00<D32>5678abcd<D16>000c<D64>6f77206f6c6c6548<D32>21646c72<FLAG>"),
		MIPI_SYST_DEBUG_LOC16(ph, MIPI_SYST_SEVERITY_WARNING, 0xabcd, "Hello world!" , 12)
		);

	EXPECT_STREQ(
		xform("<D32TS>01012332[typ=2:1 mu=1:2 sev=3 loc len]<D8>01<D64>123456780000abcd<D16>000c<D64>6f77206f6c6c6548<D32>21646c72<FLAG>"),
		MIPI_SYST_DEBUG_LOC32(ph, MIPI_SYST_SEVERITY_WARNING,0xabcd,  "Hello world!" , 12)
		);
#if defined(MIPI_SYST_PCFG_ENABLE_LOCATION_ADDRESS)
#if defined(MIPI_SYST_PCFG_ENABLE_64BIT_ADDR)
	EXPECT_STREQ(
		xform("<D32TS>01012332[typ=2:1 mu=1:2 sev=3 loc len]<D8>03<D64>12345678aabbccdd<D16>000c<D64>6f77206f6c6c6548<D32>21646c72<FLAG>"),
		MIPI_SYST_DEBUG_LOCADDR(ph, MIPI_SYST_SEVERITY_WARNING, "Hello world!" , 12)
		);
#else
	EXPECT_STREQ(
		xform("<D32TS>01012332[typ=2:1 mu=1:2 sev=3 loc len]<D8>02<D32>12345678<D16>000c<D64>6f77206f6c6c6548<D32>21646c72<FLAG>"),
		MIPI_SYST_DEBUG_LOCADDR(ph, MIPI_SYST_SEVERITY_WARNING, "Hello world!" , 12)
		);
#endif
#endif //MIPI_SYST_PCFG_ENABLE_LOCATION_ADDRESS
}
#endif //MIPI_SYST_PCFG_ENABLE_LOCATION_RECORD

TEST_F(MipiSysTFixtureDebugString, syst_debugstring_nolength)
{
	MIPI_SYST_ENABLE_HANDLE_LENGTH(ph,0);
	EXPECT_STREQ(
		xform("<D32TS>01012032[typ=2:1 mu=1:2 sev=3]<D64>6f77206f6c6c6548<D32>21646c72<FLAG>"),
		MIPI_SYST_DEBUG(ph, MIPI_SYST_SEVERITY_WARNING, "Hello world!" , 12)
		);

	MIPI_SYST_ENABLE_HANDLE_LENGTH(ph,1);

	EXPECT_STREQ(
		xform("<D32TS>01012232[typ=2:1 mu=1:2 sev=3 len]<D16>000c<D64>6f77206f6c6c6548<D32>21646c72<FLAG>"),
		MIPI_SYST_DEBUG(ph, MIPI_SYST_SEVERITY_WARNING, "Hello world!" , 12)
		);
}

TEST_F(MipiSysTFixtureDebugString, syst_func_enter)
{
	EXPECT_STREQ(
		xform("<D32TS>02012242[typ=2:2 mu=1:2 sev=4 len]<D16>000b<D64>3736353433323130<D16>3938<D8>00<FLAG>"),
		MIPI_SYST_FUNC_ENTER(ph, MIPI_SYST_SEVERITY_INFO)
		);

#if defined(MIPI_SYST_PCFG_ENABLE_LOCATION_RECORD)
	EXPECT_STREQ(
		xform("<D32TS>02012342[typ=2:2 mu=1:2 sev=4 loc len]<D8>00<D32>5678abcd<D16>000b<D64>3736353433323130<D16>3938<D8>00<FLAG>"),
		MIPI_SYST_FUNC_ENTER_LOC16(ph, MIPI_SYST_SEVERITY_INFO,0xabcd)
		);

	EXPECT_STREQ(
		xform("<D32TS>02012342[typ=2:2 mu=1:2 sev=4 loc len]<D8>01<D64>123456780000abcd<D16>000b<D64>3736353433323130<D16>3938<D8>00<FLAG>"),
		MIPI_SYST_FUNC_ENTER_LOC32(ph, MIPI_SYST_SEVERITY_INFO,0xabcd)
		);
#if defined(MIPI_SYST_PCFG_ENABLE_LOCATION_ADDRESS)
#if defined(MIPI_SYST_PCFG_ENABLE_64BIT_ADDR)
	EXPECT_STREQ(
		xform("<D32TS>02012342[typ=2:2 mu=1:2 sev=4 loc len]<D8>03<D64>12345678aabbccdd<D16>000b<D64>3736353433323130<D16>3938<D8>00<FLAG>"),
		MIPI_SYST_FUNC_ENTER_LOCADDR(ph, MIPI_SYST_SEVERITY_INFO)
		);
#else
	EXPECT_STREQ(
		xform("<D32TS>02012342[typ=2:2 mu=1:2 sev=4 loc len]<D8>02<D32>12345678<D16>000b<D64>3736353433323130<D16>3938<D8>00<FLAG>"),
		MIPI_SYST_FUNC_ENTER_LOCADDR(ph, MIPI_SYST_SEVERITY_INFO)
		);
#endif //MIPI_SYST_PCFG_ENABLE_64BIT_ADDR
#endif //MIPI_SYST_PCFG_ENABLE_LOCATION_ADDRESS
#endif //MIPI_SYST_PCFG_ENABLE_LOCATION_RECORD
}

TEST_F(MipiSysTFixtureDebugString, syst_func_exit)
{
	EXPECT_STREQ(
		xform("<D32TS>03012242[typ=2:3 mu=1:2 sev=4 len]<D16>000b<D64>3736353433323130<D16>3938<D8>00<FLAG>"),
		MIPI_SYST_FUNC_EXIT(ph, MIPI_SYST_SEVERITY_INFO)
		);

#if defined(MIPI_SYST_PCFG_ENABLE_LOCATION_RECORD)
	EXPECT_STREQ(
		xform("<D32TS>03012342[typ=2:3 mu=1:2 sev=4 loc len]<D8>00<D32>5678abcd<D16>000b<D64>3736353433323130<D16>3938<D8>00<FLAG>"),
		MIPI_SYST_FUNC_EXIT_LOC16(ph, MIPI_SYST_SEVERITY_INFO,0xabcd)
		);

	EXPECT_STREQ(
		xform("<D32TS>03012342[typ=2:3 mu=1:2 sev=4 loc len]<D8>01<D64>123456780000abcd<D16>000b<D64>3736353433323130<D16>3938<D8>00<FLAG>"),
		MIPI_SYST_FUNC_EXIT_LOC32(ph, MIPI_SYST_SEVERITY_INFO,0xabcd)
		);
#if defined(MIPI_SYST_PCFG_ENABLE_LOCATION_ADDRESS)
#if defined(MIPI_SYST_PCFG_ENABLE_64BIT_ADDR)
	EXPECT_STREQ(
		xform("<D32TS>03012342[typ=2:3 mu=1:2 sev=4 loc len]<D8>03<D64>12345678aabbccdd<D16>000b<D64>3736353433323130<D16>3938<D8>00<FLAG>"),
		MIPI_SYST_FUNC_EXIT_LOCADDR(ph, MIPI_SYST_SEVERITY_INFO)
		);
#else
	EXPECT_STREQ(
		xform("<D32TS>03012342[typ=2:3 mu=1:2 sev=4 loc len]<D8>02<D32>12345678<D16>000b<D64>3736353433323130<D16>3938<D8>00<FLAG>"),
		MIPI_SYST_FUNC_EXIT_LOCADDR(ph, MIPI_SYST_SEVERITY_INFO)
		);
#endif //MIPI_SYST_PCFG_ENABLE_64BIT_ADDR
#endif //MIPI_SYST_PCFG_ENABLE_LOCATION_ADDRESS
#endif //MIPI_SYST_PCFG_ENABLE_LOCATION_RECORD
}

#if defined(MIPI_SYST_PCFG_ENABLE_LOCATION_RECORD)
TEST_F(MipiSysTFixtureDebugString, syst_debug_assert)
{
	MIPI_SYST_ASSERT(ph, MIPI_SYST_SEVERITY_ERROR, 0);

	EXPECT_STREQ(
		xform("<D32TS>07012222[typ=2:7 mu=1:2 sev=2 len]<D16>0018<D64>7473657474696e75<D64>33323178303a632e<D64>0030203837363534<FLAG>"),
		result.c_str()
		);

	result = "";
	MIPI_SYST_ASSERT(ph, MIPI_SYST_SEVERITY_ERROR, 1);

	EXPECT_STREQ(
		"",
		result.c_str()
		);

#if defined(MIPI_SYST_PCFG_ENABLE_LOCATION_ADDRESS)
	MIPI_SYST_ASSERT_LOCADDR(ph, MIPI_SYST_SEVERITY_ERROR, 0);
#if defined(MIPI_SYST_PCFG_ENABLE_64BIT_ADDR)
	EXPECT_STREQ(
		xform("<D32TS>07012322[typ=2:7 mu=1:2 sev=2 loc len]<D8>03<D64>12345678aabbccdd<D16>0018<D64>7473657474696e75<D64>33323178303a632e<D64>0030203837363534<FLAG>"),
		result.c_str()
		);
#else
	EXPECT_STREQ(
		xform("<D32TS>07012322[typ=2:7 mu=1:2 sev=2 loc len]<D8>02<D32>12345678<D16>0018<D64>7473657474696e75<D64>33323178303a632e<D64>0030203837363534<FLAG>"),
		result.c_str()
		);
#endif
	result = "";
	MIPI_SYST_ASSERT_LOCADDR(ph, MIPI_SYST_SEVERITY_ERROR, 1);

	EXPECT_STREQ(
		"",
		result.c_str()
		);
#endif //MIPI_SYST_PCFG_ENABLE_LOCATION_ADDRESS

	MIPI_SYST_ASSERT_LOC16(ph, MIPI_SYST_SEVERITY_ERROR, 0xabcd, 0);

	EXPECT_STREQ(
		xform("<D32TS>07012322[typ=2:7 mu=1:2 sev=2 loc len]<D8>00<D32>5678abcd<D16>0018<D64>7473657474696e75<D64>33323178303a632e<D64>0030203837363534<FLAG>"),
		result.c_str()
		);

	result = "";
	MIPI_SYST_ASSERT_LOC16(ph, MIPI_SYST_SEVERITY_ERROR,0xabcd, 1);

	EXPECT_STREQ(
		"",
		result.c_str()
		);

	MIPI_SYST_ASSERT_LOC32(ph, MIPI_SYST_SEVERITY_ERROR, 0xabcd, 0);

	EXPECT_STREQ(
		xform("<D32TS>07012322[typ=2:7 mu=1:2 sev=2 loc len]<D8>01<D64>123456780000abcd<D16>0018<D64>7473657474696e75<D64>33323178303a632e<D64>0030203837363534<FLAG>"),
		result.c_str()
		);

	result = "";
	MIPI_SYST_ASSERT_LOC32(ph, MIPI_SYST_SEVERITY_ERROR, 0xabcd, 1);

	EXPECT_STREQ(
		"",
		result.c_str()
		);
}
#endif //MIPI_SYST_PCFG_ENABLE_LOCATION_RECORD

#endif //MIPI_SYST_PCFG_ENABLE_PLATFORM_STATE_DATA && MIPI_SYST_PCFG_ENABLE_DEFAULT_SCATTER_WRITE && MIPI_SYST_PCFG_ENABLE_STRING_API