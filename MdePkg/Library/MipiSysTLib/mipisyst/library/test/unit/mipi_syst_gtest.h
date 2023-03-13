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

#include <gtest/gtest.h>
#include <string>

#include "mipi_syst.h"

/* hard code compiler dependend standard defines to fixed values for
* unit test support. Regression tests expect to see these values.
*/
#undef MIPI_SYST_FUNCTION_NAME
#define MIPI_SYST_FUNCTION_NAME "0123456789"

#undef MIPI_SYST_LINE
#define MIPI_SYST_LINE          0x12345678

#undef MIPI_SYST_FILE
#define MIPI_SYST_FILE          "unittest.c"


#if defined(MIPI_SYST_PCFG_ENABLE_64BIT_ADDR)
#define mipi_syst_return_addr() (void*)0x12345678aabbccdd
#else
#define mipi_syst_return_addr() (void*)0x12345678
#endif


/* replace output handlers with unit test versions
*/
#undef MIPI_SYST_OUTPUT_D32MTS
#undef MIPI_SYST_OUTPUT_D64MTS
#undef MIPI_SYST_OUTPUT_D32TS
#undef MIPI_SYST_OUTPUT_D8
#undef MIPI_SYST_OUTPUT_D16
#undef MIPI_SYST_OUTPUT_D32
#if defined(MIPI_SYST_PCFG_ENABLE_64BIT_IO)
#undef MIPI_SYST_OUTPUT_D64
#endif
#undef MIPI_SYST_OUTPUT_FLAG

#if defined(MIPI_SYST_PCFG_ENABLE_PLATFORM_STATE_DATA)
#define MIPI_SYST_OUTPUT_D32MTS(syst_handle, data) \
	MipiSysTFixtureOutput::d32mts((syst_handle), (data))
#define MIPI_SYST_OUTPUT_D64MTS(syst_handle, data) \
	MipiSysTFixtureOutput::d64mts((syst_handle), (data))
#define MIPI_SYST_OUTPUT_D32TS(syst_handle, data) \
	MipiSysTFixtureOutput::d32ts((syst_handle), (data))
#define MIPI_SYST_OUTPUT_D8(syst_handle, data) \
	MipiSysTFixtureOutput::d8((syst_handle), (data))
#define MIPI_SYST_OUTPUT_D16(syst_handle, data) \
	MipiSysTFixtureOutput::d16((syst_handle), (data))
#define MIPI_SYST_OUTPUT_D32(syst_handle, data) \
	MipiSysTFixtureOutput::d32((syst_handle), (data))
#if defined(MIPI_SYST_PCFG_ENABLE_64BIT_IO)
#define MIPI_SYST_OUTPUT_D64(syst_handle, data) \
	MipiSysTFixtureOutput::d64((syst_handle), (data))
#endif
#define MIPI_SYST_OUTPUT_FLAG(syst_handle) \
	MipiSysTFixtureOutput::flag((syst_handle))
#else
#define MIPI_SYST_OUTPUT_D32MTS(syst_handle, data)
#define MIPI_SYST_OUTPUT_D32TS(syst_handle, data)
#define MIPI_SYST_OUTPUT_D8(syst_handle, data)
#define MIPI_SYST_OUTPUT_D16(syst_handle, data)
#define MIPI_SYST_OUTPUT_D32(syst_handle, data)
#if defined(MIPI_SYST_PCFG_ENABLE_64BIT_IO)
#define MIPI_SYST_OUTPUT_D64(syst_handle, data)
#endif
#define MIPI_SYST_OUTPUT_FLAG(syst_handle)
#endif // MIPI_SYST_PCFG_ENABLE_PLATFORM_STATE_DATA

class MipiSysTFixtureBase : public testing::Test
{
public:
	void SetUp() {
		MIPI_SYST_INIT(mipi_syst_platform_init, (void*)0);}

	void TearDown(){
		MIPI_SYST_SHUTDOWN(mipi_syst_platform_destroy);
	}

	static struct mipi_syst_origin origin;
};

class MipiSysTFixtureOutput : public MipiSysTFixtureBase
{
public:
	void SetUp() {
		MipiSysTFixtureBase::SetUp();

		ph = MIPI_SYST_INIT_HANDLE(&sh, &origin);
		MIPI_SYST_SET_HANDLE_MODULE_UNIT(ph, 1,2);
		MIPI_SYST_ENABLE_HANDLE_LENGTH(ph, 1);

		sstr.str("");
	}

	void TearDown(){
		MIPI_SYST_DELETE_HANDLE(ph);

		MipiSysTFixtureBase::TearDown();
	}

public:
	struct mipi_syst_handle sh;
	struct mipi_syst_handle* ph;

	static std::stringstream sstr;
	std::string x4m;

	/* unit test IO driver function to print the access output into local string */
	static void d64(struct mipi_syst_handle* systh, mipi_syst_u64 v)
	{ sstr << "<D64>"<< std::hex << std::setfill('0') << std::setw(16) << v; }
	static void d32(struct mipi_syst_handle* systh, mipi_syst_u32 v)
	{ sstr << "<D32>"<< std::hex << std::setfill('0') << std::setw(8) << v; }
	static void d16(struct mipi_syst_handle* systh, mipi_syst_u16 v)
	{ sstr << "<D16>"<< std::hex << std::setfill('0') << std::setw(4) << v; }
	static void d8(struct mipi_syst_handle* systh, mipi_syst_u8 v)
	{ sstr << "<D8>"<< std::hex << std::setfill('0') << std::setw(2) << (mipi_syst_u16)v; }
	static void d32ts(struct mipi_syst_handle* systh, mipi_syst_u32 v)
	{
		sstr << "<D32TS>"<< std::hex << std::setfill('0') << std::setw(8) << v;
		union {
			mipi_syst_u32 val;
			struct mipi_syst_msg_tag tag;
		}u = { v };

		sstr << "[typ=" << u.tag.et_type << ":"   << u.tag.et_subtype
			<< " mu=" << (u.tag.et_modunit >> 4) << ":" << (u.tag.et_modunit & 0xF)
			<< " sev=" << u.tag.et_severity;

		if (u.tag.et_res7) sstr << " res7";
		if (u.tag.et_location) sstr << " loc";
		if (u.tag.et_res30) sstr << " res30";
		if (u.tag.et_length) sstr << " len";
		if (u.tag.et_chksum) sstr << " chk";
		sstr << "]";
	}
	static void d32mts(struct mipi_syst_handle* systh, mipi_syst_u32 v)
	{ sstr << "<D32MTS>"<< std::hex << std::setfill('0') << std::setw(8) << v; }
	static void d64mts(struct mipi_syst_handle* systh, mipi_syst_u64 v)
	{ sstr << "<D64MTS>"<< std::hex << std::setfill('0') << std::setw(16) << v; }
	static void flag(struct mipi_syst_handle* systh)
	{ sstr << "<FLAG>"; }

	/* output string transformation to match platform properties */
	const char* xform(const char* input)
	{
		size_t pos(0);
		x4m=input;
#if !defined(MIPI_SYST_PCFG_ENABLE_64BIT_IO)
		pos = std::string::npos;
		while((pos=x4m.find("<D64>")) != std::string::npos){
			std::string val = x4m.substr(pos+5,16);
			std::string upperVal = val.substr(8,8);
			std::string lowerVal = val.substr(0,8);
			std::string replacement = "<D32>";
			replacement += upperVal;
			replacement += "<D32>";
			replacement += lowerVal;
			x4m.erase(pos,21);
			x4m.insert(pos,replacement);
		}
#endif
#if defined(MIPI_SYST_PCFG_ENABLE_64BIT_ADDR)
		/* need to adapt 32bit catalog type for 64bit packing settings */
		pos = x4m.find("[typ=3:1 ");
		if (pos != std::string::npos) {
			x4m.replace(pos, 9, std::string("[typ=3:5 "));
			x4m.replace(0, 9, std::string("<D32TS>05"));
		}

		/* need to adapt 64bit catalog type for 64bit packing settings */
		pos = x4m.find("[typ=3:2 ");
		if (pos != std::string::npos) {
			x4m.replace(pos, 9, std::string("[typ=3:6 "));
			x4m.replace(0, 9, std::string("<D32TS>06"));
		}

		pos = x4m.find("[typ=2:b ");
		if (pos != std::string::npos) {
			x4m.replace(pos, 9, std::string("[typ=2:c "));
			x4m.replace(0, 9, std::string("<D32TS>0c"));
		}
#endif
#if !defined(MIPI_SYST_PCFG_LENGTH_FIELD)
		/* take out len parts from comparsion string */
		pos = x4m.find(" len");
		if (pos != std::string::npos) {
			x4m.replace(pos, 4, "");

			std::stringstream sstr(x4m.substr(7, 8));
			mipi_syst_u32 tag;
			sstr >> std::hex >> tag;
			tag &= ~(1 << 9);
			sstr.str(std::string());
			sstr.clear();
			sstr << std::hex << std::setfill('0') << std::setw(8)
			     << std::noshowbase << tag;
			x4m.replace(7, 8, sstr.str());

			pos = x4m.find("<D16>");
			if (pos != std::string::npos) {
				x4m.replace(pos, 9, "");
			}
		}
#endif

		return x4m.c_str();
	}
};
