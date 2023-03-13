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
	defined(MIPI_SYST_PCFG_ENABLE_TIMESTAMP)

class MipiSysTFixtureClock : public MipiSysTFixtureOutput
{
public:
	void SetUp() {
		MipiSysTFixtureOutput::SetUp();
	}

	void TearDown(){
		MipiSysTFixtureOutput::TearDown();
	}

	const char * mipi_syst_write_clock(struct mipi_syst_handle* svh,
	struct mipi_syst_msglocation* loc,
		enum mipi_syst_subtype_clock fmt,
		mipi_syst_u64 clock,
		mipi_syst_u64 freq)
	{
		static std::string result;

		::mipi_syst_write_clock(svh, loc,fmt, clock, freq);
		result = sstr.str();
		sstr.str("");

		return result.c_str();
	}
};

#if defined(MIPI_SYST_PCFG_ENABLE_TIMESTAMP)
TEST_F(MipiSysTFixtureClock, syst_clock_sync_null)
{
	EXPECT_STREQ(
		xform(""),
		MIPI_SYST_CLOCK_SYNC(NULL, MIPI_SYST_PLATFORM_CLOCK(), MIPI_SYST_PLATFORM_FREQ())
		);
}
TEST_F(MipiSysTFixtureClock, syst_clock_sync_output)
{
	MIPI_SYST_ENABLE_HANDLE_TIMESTAMP(ph, 0);

	EXPECT_STREQ(
		xform("<D32TS>01012208[typ=8:1 mu=1:2 sev=0 len]<D16>0010<D64>12345678aabbccdd<D64>00000000000f4240<FLAG>"),
		MIPI_SYST_CLOCK_SYNC(ph, MIPI_SYST_PLATFORM_CLOCK(), MIPI_SYST_PLATFORM_FREQ())
		);
}

#endif /* defined(MIPI_SYST_PCFG_ENABLE_TIMESTAMP)*/

#endif