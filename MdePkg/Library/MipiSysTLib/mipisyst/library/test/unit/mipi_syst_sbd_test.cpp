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
 * Przemyslaw Romaniak (Intel Corporation) - SBD implementation
 */

#include "mipi_syst_gtest.h"

#if defined(MIPI_SYST_PCFG_ENABLE_PLATFORM_STATE_DATA) &&\
	defined(MIPI_SYST_PCFG_ENABLE_DEFAULT_SCATTER_WRITE) &&\
	defined(MIPI_SYST_PCFG_ENABLE_SBD_API)

class MipiSysTFixtureSBD : public MipiSysTFixtureOutput
{
public:
	struct test_struct
	{
		uint32_t first_32bit_field;
		uint64_t second_64bit_field;
		float third_float_field;
		uint8_t fourth_8bit_field;
	} test;

	struct test_struct_long
	{
		test_struct sub[40];
	} test_long;

	void SetUp() {
		MipiSysTFixtureOutput::SetUp();

		test.first_32bit_field = 0xAABBCCDD;
		test.second_64bit_field = 0x0011223344556677;
		test.third_float_field = 1.123f;
		test.fourth_8bit_field = 0x55;
		for (size_t i=0; i<sizeof(test_long.sub)/sizeof(test_long.sub[0]); i++)
		{
			test_struct & current = test_long.sub[i];
			current.first_32bit_field = 0xAABBCCDD;
			current.second_64bit_field = 0x0011223344556677;
			current.third_float_field = 1.123f;
			current.fourth_8bit_field = static_cast<uint8_t>(i);
		}
	}

	void TearDown(){
		MipiSysTFixtureOutput::TearDown();
	}

	std::string mipi_syst_write_sbd64_message(struct mipi_syst_handle* svh,
			struct mipi_syst_msglocation* loc,
			enum mipi_syst_severity severity,
			mipi_syst_u64 sbd_id,
			mipi_syst_address addr,
			const char* name,
			mipi_syst_u32 len,
			const void *blob)
	{
		std::string result;

		::mipi_syst_write_sbd64_message(svh, loc, severity, sbd_id, addr, name, len, blob);
		result = sstr.str();
		sstr.str("");

		return result;
	}

	std::string mipi_syst_write_sbd32_message(struct mipi_syst_handle* svh,
			struct mipi_syst_msglocation* loc,
			enum mipi_syst_severity severity,
			mipi_syst_u32 sbd_id,
			mipi_syst_address addr,
			const mipi_syst_s8 *name,
			mipi_syst_u32 len,
			const void *blob)
	{
		std::string result;

		::mipi_syst_write_sbd32_message(svh, loc, severity, sbd_id, addr, name, len, blob);
		result = sstr.str();
		sstr.str("");

		return result;
	}

};

TEST_F(MipiSysTFixtureSBD, syst_sbd32_no_handle)
{

	EXPECT_EQ(
		xform(""),
		MIPI_SYST_SBD32(0, MIPI_SYST_SEVERITY_WARNING, 0xaabbccdd, MIPI_SYST_SBD_NO_BLOB_ADDRESS, NULL, sizeof(test), &test)
	);

	EXPECT_EQ(
		xform(""),
		MIPI_SYST_SBD32(0, MIPI_SYST_SEVERITY_WARNING, 0xaabbccdd, MIPI_SYST_SBD_NO_BLOB_ADDRESS, NULL, sizeof(test), &test)
	);

	EXPECT_EQ(
		xform(""),
		MIPI_SYST_SBD32(0, MIPI_SYST_SEVERITY_WARNING, 0xaabbccdd, MIPI_SYST_SBD_NO_BLOB_ADDRESS, "Structure description", sizeof(test), &test)
	);

	EXPECT_EQ(
		xform(""),
		MIPI_SYST_SBD32(0, MIPI_SYST_SEVERITY_WARNING, 0xaabbccdd, MIPI_SYST_SBD_NO_BLOB_ADDRESS, "Structure description no address", sizeof(test), &test)
	);
}

TEST_F(MipiSysTFixtureSBD, syst_sbd64_no_handle)
{

	EXPECT_EQ(
		xform(""),
		MIPI_SYST_SBD64(0, MIPI_SYST_SEVERITY_WARNING, 0xaabbccdd11223344, MIPI_SYST_SBD_NO_BLOB_ADDRESS, NULL, sizeof(test), &test)
	);

	EXPECT_EQ(
		xform(""),
		MIPI_SYST_SBD64(0, MIPI_SYST_SEVERITY_WARNING, 0xaabbccdd11223344, MIPI_SYST_SBD_NO_BLOB_ADDRESS, NULL, sizeof(test), &test)
	);

	EXPECT_EQ(
		xform(""),
		MIPI_SYST_SBD64(0, MIPI_SYST_SEVERITY_WARNING, 0xaabbccdd11223344, MIPI_SYST_SBD_NO_BLOB_ADDRESS, "Structure description", sizeof(test), &test)
	);

	EXPECT_EQ(
		xform(""),
		MIPI_SYST_SBD64(0, MIPI_SYST_SEVERITY_WARNING, 0xaabbccdd11223344, MIPI_SYST_SBD_NO_BLOB_ADDRESS, "Structure description no address", sizeof(test), &test)
	);
}

TEST_F(MipiSysTFixtureSBD, syst_sbd32_reference)
{

	EXPECT_EQ(
		xform("<D32TS>00012039[typ=9:0 mu=1:2 sev=3]<D32>aabbccdd<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd553f8fbe77<FLAG>"),
		MIPI_SYST_SBD32(ph, MIPI_SYST_SEVERITY_WARNING, 0xaabbccdd, MIPI_SYST_SBD_NO_BLOB_ADDRESS, NULL, sizeof(test), &test)
	);

	EXPECT_EQ(
		xform("<D32TS>08012039[typ=9:8 mu=1:2 sev=3]<D32>aabbccdd<D32>abcd1234<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd553f8fbe77<FLAG>"),
		MIPI_SYST_SBD32(ph, MIPI_SYST_SEVERITY_WARNING, 0xaabbccdd, 0xabcd1234, NULL, sizeof(test), &test)
	);

	EXPECT_EQ(
		xform("<D32TS>0a012039[typ=9:a mu=1:2 sev=3]<D32>aabbccdd<D32>abcd1234<D64>7275746375727453<D64>6972637365642065<D32>6f697470<D16>006e<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd553f8fbe77<FLAG>"),
		MIPI_SYST_SBD32(ph, MIPI_SYST_SEVERITY_WARNING, 0xaabbccdd, 0xabcd1234, "Structure description", sizeof(test), &test)
	);

	EXPECT_EQ(
		xform("<D32TS>02012039[typ=9:2 mu=1:2 sev=3]<D32>aabbccdd<D64>7275746375727453<D64>6972637365642065<D64>6f6e206e6f697470<D64>7373657264646120<D8>00<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd553f8fbe77<FLAG>"),
		MIPI_SYST_SBD32(ph, MIPI_SYST_SEVERITY_WARNING, 0xaabbccdd, MIPI_SYST_SBD_NO_BLOB_ADDRESS, "Structure description no address", sizeof(test), &test)
	);
}

TEST_F(MipiSysTFixtureSBD, syst_sbd64_reference)
{

	EXPECT_EQ(
		xform("<D32TS>01012039[typ=9:1 mu=1:2 sev=3]<D64>aabbccdd11223344<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd553f8fbe77<FLAG>"),
		MIPI_SYST_SBD64(ph, MIPI_SYST_SEVERITY_WARNING, 0xaabbccdd11223344, MIPI_SYST_SBD_NO_BLOB_ADDRESS, NULL, sizeof(test), &test)
	);

	EXPECT_EQ(
		xform("<D32TS>09012039[typ=9:9 mu=1:2 sev=3]<D64>aabbccdd11223344<D32>abcd1234<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd553f8fbe77<FLAG>"),
		MIPI_SYST_SBD64(ph, MIPI_SYST_SEVERITY_WARNING, 0xaabbccdd11223344, 0xabcd1234, NULL, sizeof(test), &test)
	);

	EXPECT_EQ(
		xform("<D32TS>0b012039[typ=9:b mu=1:2 sev=3]<D64>aabbccdd11223344<D32>abcd1234<D64>7275746375727453<D64>6972637365642065<D32>6f697470<D16>006e<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd553f8fbe77<FLAG>"),
		MIPI_SYST_SBD64(ph, MIPI_SYST_SEVERITY_WARNING, 0xaabbccdd11223344, 0xabcd1234, "Structure description", sizeof(test), &test)
	);

	EXPECT_EQ(
		xform("<D32TS>03012039[typ=9:3 mu=1:2 sev=3]<D64>aabbccdd11223344<D64>7275746375727453<D64>6972637365642065<D64>6f6e206e6f697470<D64>7373657264646120<D8>00<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd553f8fbe77<FLAG>"),
		MIPI_SYST_SBD64(ph, MIPI_SYST_SEVERITY_WARNING, 0xaabbccdd11223344, MIPI_SYST_SBD_NO_BLOB_ADDRESS, "Structure description no address", sizeof(test), &test)
	);
}

TEST_F(MipiSysTFixtureSBD, syst_sbd_long_struct)
{
	EXPECT_EQ(
		xform("<D32TS>00012039[typ=9:0 mu=1:2 sev=3]<D32>aabbccdd<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd003f8fbe77<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd013f8fbe77<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd023f8fbe77<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd033f8fbe77<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd043f8fbe77<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd053f8fbe77<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd063f8fbe77<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd073f8fbe77<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd083f8fbe77<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd093f8fbe77<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd0a3f8fbe77<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd0b3f8fbe77<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd0c3f8fbe77<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd0d3f8fbe77<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd0e3f8fbe77<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd0f3f8fbe77<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd103f8fbe77<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd113f8fbe77<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd123f8fbe77<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd133f8fbe77<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd143f8fbe77<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd153f8fbe77<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd163f8fbe77<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd173f8fbe77<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd183f8fbe77<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd193f8fbe77<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd1a3f8fbe77<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd1b3f8fbe77<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd1c3f8fbe77<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd1d3f8fbe77<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd1e3f8fbe77<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd1f3f8fbe77<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd203f8fbe77<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd213f8fbe77<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd223f8fbe77<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd233f8fbe77<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd243f8fbe77<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd253f8fbe77<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd263f8fbe77<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd273f8fbe77<FLAG>"),
		MIPI_SYST_SBD32(ph, MIPI_SYST_SEVERITY_WARNING, 0xaabbccdd, MIPI_SYST_SBD_NO_BLOB_ADDRESS, NULL, sizeof(test_long), &test_long)
	);

	EXPECT_EQ(
		xform("<D32TS>01012039[typ=9:1 mu=1:2 sev=3]<D64>aabbccdd11223344<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd003f8fbe77<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd013f8fbe77<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd023f8fbe77<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd033f8fbe77<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd043f8fbe77<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd053f8fbe77<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd063f8fbe77<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd073f8fbe77<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd083f8fbe77<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd093f8fbe77<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd0a3f8fbe77<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd0b3f8fbe77<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd0c3f8fbe77<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd0d3f8fbe77<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd0e3f8fbe77<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd0f3f8fbe77<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd103f8fbe77<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd113f8fbe77<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd123f8fbe77<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd133f8fbe77<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd143f8fbe77<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd153f8fbe77<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd163f8fbe77<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd173f8fbe77<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd183f8fbe77<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd193f8fbe77<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd1a3f8fbe77<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd1b3f8fbe77<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd1c3f8fbe77<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd1d3f8fbe77<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd1e3f8fbe77<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd1f3f8fbe77<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd203f8fbe77<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd213f8fbe77<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd223f8fbe77<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd233f8fbe77<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd243f8fbe77<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd253f8fbe77<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd263f8fbe77<D64>cdcdcdcdaabbccdd<D64>0011223344556677<D64>cdcdcd273f8fbe77<FLAG>"),
		MIPI_SYST_SBD64(ph, MIPI_SYST_SEVERITY_WARNING, 0xaabbccdd11223344, MIPI_SYST_SBD_NO_BLOB_ADDRESS, NULL, sizeof(test_long), &test_long)
	);
}

#endif
