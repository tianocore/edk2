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

#include "../../src/mipi_syst_init.c"
#include "mipi_syst_platform.c"
#include "../../src/mipi_syst_writer.c"

std::stringstream MipiSysTFixtureOutput::sstr;

struct mipi_syst_origin MipiSysTFixtureBase::origin = MIPI_SYST_GEN_ORIGIN_MODULE(1, 2);

/* Test structure sizes used in binary output processing
*/
TEST_F(MipiSysTFixtureBase, syst_structure_sizes)
{
	EXPECT_EQ(1, sizeof(mipi_syst_u8)) << "mipi_syst_u8 not 1 byte long";
	EXPECT_EQ(2, sizeof(mipi_syst_u16)) << "mipi_syst_u16 not 2 byte long";
	EXPECT_EQ(4, sizeof(mipi_syst_u32)) << "mipi_syst_u32 not 4 byte long";
	EXPECT_EQ(8, sizeof(mipi_syst_u64)) << "mipi_syst_u64 not 8 byte long";

	EXPECT_EQ(1, sizeof(mipi_syst_s8)) << "mipi_syst_s8 not 1 byte long";
	EXPECT_EQ(2, sizeof(mipi_syst_s16)) << "mipi_syst_s16 not 2 byte long";
	EXPECT_EQ(4, sizeof(mipi_syst_s32)) << "mipi_syst_s32 not 4 byte long";
	EXPECT_EQ(8, sizeof(mipi_syst_s64)) << "mipi_syst_s64 not 8 byte long";

	EXPECT_EQ(16, sizeof(struct mipi_syst_guid)) << "struct mipi_syst_guid not 16 byte long";

	EXPECT_EQ(4, sizeof(struct mipi_syst_msg_tag)) << "struct mipi_syst_msg_tag must be 32bit";
	EXPECT_EQ(4, sizeof(struct mipi_syst_scatter_prog )) << "syst_scatter_prog_t must be 32bit";
	EXPECT_EQ(4, sizeof(union mipi_syst_msglocation32)) << "mipi_syst_msglocation32 must be 32bit";
	EXPECT_EQ(8, sizeof(union mipi_syst_msglocation64)) << "mipi_syst_msglocation64 must be 64bit";
}

TEST_F(MipiSysTFixtureBase, syst_little_endian_swap)
{
	union { mipi_syst_u16 v; mipi_syst_u8 b[2]; } v16 = { MIPI_SYST_HTOLE16(0x1234) };
	EXPECT_EQ(v16.b[0], 0x34);
	EXPECT_EQ(v16.b[1], 0x12);

	union { mipi_syst_u32 v; mipi_syst_u8 b[4]; } v32 = { MIPI_SYST_HTOLE32(0x12345678) };
	EXPECT_EQ(v32.b[0], 0x78);
	EXPECT_EQ(v32.b[1], 0x56);
	EXPECT_EQ(v32.b[2], 0x34);
	EXPECT_EQ(v32.b[3], 0x12);

	union { mipi_syst_u64 v; mipi_syst_u8 b[8]; } v64 = { MIPI_SYST_HTOLE64(0x12345678AABBCCDDull) };
	EXPECT_EQ(v64.b[0], 0xDD);
	EXPECT_EQ(v64.b[1], 0xCC);
	EXPECT_EQ(v64.b[2], 0xBB);
	EXPECT_EQ(v64.b[3], 0xAA);
	EXPECT_EQ(v64.b[4], 0x78);
	EXPECT_EQ(v64.b[5], 0x56);
	EXPECT_EQ(v64.b[6], 0x34);
	EXPECT_EQ(v64.b[7], 0x12);
}


/* test header bitfield alignment*/
TEST_F(MipiSysTFixtureBase, syst_header_bit_alignment)
{
	union {
		mipi_syst_msg_tag tag;
		mipi_syst_u32  val;
		mipi_syst_u8  b[4];
	} native, little;

	little.val = native.val = 0;
	native.tag.et_type = 0xF;
	little.b[0] = 0xF;
	EXPECT_EQ(little.val, MIPI_SYST_HTOLE32(native.val)) << "mipi_syst_msg_tag bit alignment wrong (type)";

	little.val = native.val = 0;
	native.tag.et_severity = 0x7;
	little.b[0] = 0x7<< 4;
	EXPECT_EQ(little.val, MIPI_SYST_HTOLE32(native.val)) << "mipi_syst_msg_tag bit alignment wrong (severity)";

	little.val = native.val = 0;
	native.tag.et_res7 = 1;
	little.b[0] = 0x80;
	EXPECT_EQ(little.val, MIPI_SYST_HTOLE32(native.val)) << "mipi_syst_msg_tag bit alignment wrong (res7)";

	little.val = native.val = 0;
	native.tag.et_location = 1;
	little.b[1] = 1;
	EXPECT_EQ(little.val, MIPI_SYST_HTOLE32(native.val)) << "mipi_syst_msg_tag bit alignment wrong (location)";

	little.val = native.val = 0;
	native.tag.et_length = 1;
	little.b[1] = 1<<1;
	EXPECT_EQ(little.val, MIPI_SYST_HTOLE32(native.val)) << "mipi_syst_msg_tag bit alignment wrong (length)";

	little.val = native.val = 0;
	native.tag.et_chksum = 1;
	little.b[1] = 1<<2;
	EXPECT_EQ(little.val, MIPI_SYST_HTOLE32(native.val)) << "mipi_syst_msg_tag bit alignment wrong (chksum)";

	little.val = native.val = 0;
	native.tag.et_timestamp = 1;
	little.b[1] = 1<<3;
	EXPECT_EQ(little.val, MIPI_SYST_HTOLE32(native.val)) << "mipi_syst_msg_tag bit alignment wrong (timestamp)";

	little.val = native.val = 0;
	native.tag.et_modunit = 0x7FF;
	little.b[1] = 0xF << 4;
	little.b[2] = 0x7F;
	EXPECT_EQ(little.val, MIPI_SYST_HTOLE32(native.val)) << "mipi_syst_msg_tag bit alignment wrong (origin)";

	little.val = native.val = 0;
	native.tag.et_guid = 1;
	little.b[2] = 0x80;
	EXPECT_EQ(little.val, MIPI_SYST_HTOLE32(native.val)) << "mipi_syst_msg_tag bit alignment wrong (guid)";

	little.val = native.val = 0;
	native.tag.et_subtype = 0x3F;
	little.b[3] = 0x3F;
	EXPECT_EQ(little.val, MIPI_SYST_HTOLE32(native.val)) << "mipi_syst_msg_tag bit alignment wrong (subtype)";

	little.val = native.val = 0;
	native.tag.et_res30 = 1;
	little.b[3] = 0x40;
	EXPECT_EQ(little.val, MIPI_SYST_HTOLE32(native.val)) << "mipi_syst_msg_tag bit alignment wrong (res30)";

	little.val = native.val = 0;
	native.tag.et_res31 = 1;
	little.b[3] = 0x80;
	EXPECT_EQ(little.val, MIPI_SYST_HTOLE32(native.val)) << "mipi_syst_msg_tag bit alignment wrong (res31)";
}

/* Check initialization of global state
*/
TEST_F(MipiSysTFixtureBase, syst_global_state_creation)
{
	struct mipi_syst_header* ph = &syst_hdr;

	EXPECT_EQ(ph->systh_version, MIPI_SYST_VERSION_CODE)  << "syst header has unexpected version";
#if defined(MIPI_SYST_PCFG_ENABLE_PLATFORM_HANDLE_DATA)
	EXPECT_EQ(ph->systh_inith, &platform_handle_init) << "syst header handle init hook wrong";
	EXPECT_EQ(ph->systh_releaseh, &platform_handle_release) << "syst header handle release hook wrong";
#endif
#if defined(MIPI_SYST_PCFG_ENABLE_DEFAULT_SCATTER_WRITE)
	EXPECT_EQ(ph->systh_writer, &mipi_syst_scatter_write) << "syst header writer function wrong ";
#endif
}

TEST_F(MipiSysTFixtureBase, syst_custom_state_creation)
{
	struct mipi_syst_header custom_hdr;

	MIPI_SYST_INIT_STATE(&custom_hdr, mipi_syst_platform_init, (void*)0);

	EXPECT_EQ(custom_hdr.systh_version, MIPI_SYST_VERSION_CODE)  << "syst header has unexpected version";
#if defined(MIPI_SYST_PCFG_ENABLE_PLATFORM_HANDLE_DATA)
	EXPECT_EQ(custom_hdr.systh_inith, &platform_handle_init) << "syst header handle init hook wrong";
	EXPECT_EQ(custom_hdr.systh_releaseh, &platform_handle_release) << "syst header handle release hook wrong";
#endif
#if defined(MIPI_SYST_PCFG_ENABLE_DEFAULT_SCATTER_WRITE)
	EXPECT_EQ(custom_hdr.systh_writer, &mipi_syst_scatter_write) << "syst header writer function wrong ";
#endif

	MIPI_SYST_SHUTDOWN_STATE(&custom_hdr, mipi_syst_platform_destroy);
}

/* Check initialization of handle state
*/
TEST_F(MipiSysTFixtureBase, syst_handle_static_creation)
{
	struct mipi_syst_handle sh;
	struct mipi_syst_handle* ph;

	ph = MIPI_SYST_INIT_HANDLE(&sh, NULL);
	MIPI_SYST_SET_HANDLE_MODULE_UNIT(ph, 1,2);

	ASSERT_EQ(ph, &sh) << "static allocation did not return passed pointer";

	EXPECT_EQ(&syst_hdr, ph->systh_header) << "header not set in handle";
	EXPECT_EQ(0, ph->systh_flags.shf_alloc) << "handle indicates allocation, but is static";
	EXPECT_EQ(0x012, ph->systh_tag.et_modunit) << "module id not set in handle";

	MIPI_SYST_DELETE_HANDLE(ph);
}

TEST_F(MipiSysTFixtureBase, syst_custom_handle_static_creation)
{
	struct mipi_syst_handle sh;
	struct mipi_syst_handle* ph;

	struct mipi_syst_header custom_hdr;
	MIPI_SYST_INIT_STATE(&custom_hdr, mipi_syst_platform_init, (void*)0);

	ph = MIPI_SYST_INIT_HANDLE_STATE(&custom_hdr, &sh, &origin);

	ASSERT_EQ(ph, &sh) << "static allocation did not return passed pointer";

	EXPECT_EQ(&custom_hdr, ph->systh_header) << "custom header not set in handle";
	EXPECT_EQ(0, ph->systh_flags.shf_alloc) << "handle indicates allocation, but is static";
#if defined(MIPI_SYST_PCFG_ENABLE_ORIGIN_GUID)
	EXPECT_EQ(0x012, ph->systh_tag.et_modunit) << "origin id not set in handle";
#endif
	MIPI_SYST_DELETE_HANDLE(ph);
	MIPI_SYST_SHUTDOWN_STATE(&custom_hdr, mipi_syst_platform_destroy);
}

#if defined(MIPI_SYST_PCFG_ENABLE_ORIGIN_GUID)
TEST_F(MipiSysTFixtureBase, syst_test_guid_origin_handle_creation)
{
	struct mipi_syst_handle sh;
	struct mipi_syst_handle* ph;
	struct mipi_syst_origin guid_origin =
		MIPI_SYST_GEN_ORIGIN_GUID(
			0x494E5443, 0xA2AE, 0x4C70, 0xABB5, 0xD1A79E9CEA35,
			0x7FF);

	ph = MIPI_SYST_INIT_HANDLE(&sh, &guid_origin);

	ASSERT_EQ(ph, &sh) << "static allocation did not return passed pointer";

	EXPECT_EQ(&syst_hdr, ph->systh_header) << "header not set in handle";
	EXPECT_EQ(0, ph->systh_flags.shf_alloc) << "handle indicates allocation, but is static";
	EXPECT_EQ(0x7FF, ph->systh_tag.et_modunit) << "module id not set in handle";
	EXPECT_EQ(ph->systh_guid.u.ll[0], MIPI_SYST_HTOLE64(guid_origin.guid.u.ll[0]));
	EXPECT_EQ(ph->systh_guid.u.ll[1], MIPI_SYST_HTOLE64(guid_origin.guid.u.ll[1]));
	MIPI_SYST_DELETE_HANDLE(ph);
}
#endif

#if defined(MIPI_SYST_PCFG_ENABLE_HEAP_MEMORY)
TEST_F(MipiSysTFixtureBase, syst_handle_dynamic_creation)
{
	struct mipi_syst_handle* ph;

	ph = MIPI_SYST_ALLOC_HANDLE(NULL);
	MIPI_SYST_SET_HANDLE_MODULE_UNIT(ph, 1,2);

	ASSERT_NE(ph, (struct mipi_syst_handle*)0) << "allocation failed";

	EXPECT_EQ(&syst_hdr, ph->systh_header) << "header not set in handle";
	EXPECT_EQ(1, ph->systh_flags.shf_alloc) << "handle indicates static, but is allocated";
	EXPECT_EQ(0x012, ph->systh_tag.et_modunit) << "origin id not set in handle";

	MIPI_SYST_DELETE_HANDLE(ph);
}

TEST_F(MipiSysTFixtureBase, syst_custom_handle_dynamic_creation)
{
	struct mipi_syst_handle* ph;
	struct mipi_syst_header custom_hdr;
	MIPI_SYST_INIT_STATE(&custom_hdr, mipi_syst_platform_init, (void*)0);

	ph = MIPI_SYST_ALLOC_HANDLE_STATE(&custom_hdr, NULL);
	MIPI_SYST_SET_HANDLE_MODULE_UNIT(ph, 1,2);

	ASSERT_NE(ph, (struct mipi_syst_handle*)0) << "allocation failed";

	EXPECT_EQ(&custom_hdr, ph->systh_header) << "custom header not set in handle";
	EXPECT_EQ(1, ph->systh_flags.shf_alloc) << "handle indicates static, but is allocated";
	EXPECT_EQ(0x012, ph->systh_tag.et_modunit) << "origin id not set in handle";

	MIPI_SYST_DELETE_HANDLE(ph);
}
#endif

TEST_F(MipiSysTFixtureBase, syst_message_flags)
{
	struct mipi_syst_handle* ph;
	struct mipi_syst_header custom_hdr;
	MIPI_SYST_INIT_STATE(&custom_hdr, mipi_syst_platform_init, (void*)0);

	ph = MIPI_SYST_ALLOC_HANDLE_STATE(&custom_hdr, &origin);

	ASSERT_NE(ph, (struct mipi_syst_handle*)0) << "allocation failed";


#if defined(MIPI_SYST_PCFG_ENABLE_CHECKSUM)
	EXPECT_EQ(0,MIPI_SYST_GET_HANDLE_CHECKSUM(ph));

	MIPI_SYST_ENABLE_HANDLE_CHECKSUM(ph, 1);
	EXPECT_NE(0, MIPI_SYST_GET_HANDLE_CHECKSUM(ph));

	MIPI_SYST_ENABLE_HANDLE_CHECKSUM(ph, 0);
	EXPECT_EQ(0, MIPI_SYST_GET_HANDLE_CHECKSUM(ph));
#endif
#if defined(MIPI_SYST_PCFG_ENABLE_TIMESTAMP)
	EXPECT_EQ(0,MIPI_SYST_GET_HANDLE_TIMESTAMP(ph));

	MIPI_SYST_ENABLE_HANDLE_TIMESTAMP(ph, 1);
	EXPECT_NE(0, MIPI_SYST_GET_HANDLE_TIMESTAMP(ph));

	MIPI_SYST_ENABLE_HANDLE_TIMESTAMP(ph, 0);
	EXPECT_EQ(0, MIPI_SYST_GET_HANDLE_TIMESTAMP(ph));
#endif
	MIPI_SYST_DELETE_HANDLE(ph);
}


TEST_F(MipiSysTFixtureBase, syst_version_macro)
{
	mipi_syst_u32 version = (MIPI_SYST_VERSION_MAJOR<<16) |(MIPI_SYST_VERSION_MINOR<<8)|MIPI_SYST_VERSION_PATCH;
	std::stringstream sstr;
	sstr << MIPI_SYST_VERSION_MAJOR << "." << MIPI_SYST_VERSION_MINOR << "." << MIPI_SYST_VERSION_PATCH;
	std::string verstr(sstr.str());

	EXPECT_STREQ(verstr.c_str(), MIPI_SYST_VERSION_STRING);

	EXPECT_EQ(MIPI_SYST_VERSION_CODE, version) << "unexpected version code";


	ASSERT_LT(MIPI_SYST_MAKE_VERSION_CODE(1,2,3),
		  MIPI_SYST_MAKE_VERSION_CODE(2,0,0));
	ASSERT_LT(MIPI_SYST_MAKE_VERSION_CODE(1,2,3),
		  MIPI_SYST_MAKE_VERSION_CODE(1,3,3));
	ASSERT_LT(MIPI_SYST_MAKE_VERSION_CODE(1,2,3),
		  MIPI_SYST_MAKE_VERSION_CODE(1,2,4));

	ASSERT_GT(MIPI_SYST_MAKE_VERSION_CODE(1,2,3),
		  MIPI_SYST_MAKE_VERSION_CODE(0,2,3));
	ASSERT_GT(MIPI_SYST_MAKE_VERSION_CODE(1,2,3),
		  MIPI_SYST_MAKE_VERSION_CODE(1,1,3));
	ASSERT_GT(MIPI_SYST_MAKE_VERSION_CODE(1,2,3),
		  MIPI_SYST_MAKE_VERSION_CODE(1,2,2));
}

TEST_F(MipiSysTFixtureBase, syst_handle_nullptr)
{
	MIPI_SYST_INIT_HANDLE(0,NULL);
	MIPI_SYST_DELETE_HANDLE(0);

	EXPECT_EQ(0,0);            // only reached if above calls don't crash
}

// {1DBBA102-DFFD-4A05-8CED-F744046715ED}

struct mipi_syst_guid guid1=
{{0x1d, 0xbb, 0xa1, 0x02, 0xdf, 0xfd, 0x4a, 0x05, 0x8c, 0xed, 0xf7, 0x44, 0x4, 0x67, 0x15, 0xed} };

struct mipi_syst_guid guid2 = MIPI_SYST_GEN_GUID(0x1DBBA102, 0xDFFD, 0x4A05, 0x8CED, 0xF744046715ED);

TEST_F(MipiSysTFixtureBase, syst_guid)
{
	union {
		struct mipi_syst_guid g; mipi_syst_u64 ll[2];
	} v;

	v.ll[0] = MIPI_SYST_HTOLE64(guid2.u.ll[0]);
	v.ll[1] = MIPI_SYST_HTOLE64(guid2.u.ll[1]);

	EXPECT_EQ(guid1.u.b[0], guid2.u.b[0]);
	EXPECT_EQ(guid1.u.b[1], guid2.u.b[1]);
	EXPECT_EQ(guid1.u.b[2], guid2.u.b[2]);
	EXPECT_EQ(guid1.u.b[3], guid2.u.b[3]);
	EXPECT_EQ(guid1.u.b[4], guid2.u.b[4]);
	EXPECT_EQ(guid1.u.b[5], guid2.u.b[5]);
	EXPECT_EQ(guid1.u.b[6], guid2.u.b[6]);
	EXPECT_EQ(guid1.u.b[7], guid2.u.b[7]);
	EXPECT_EQ(guid1.u.b[8], guid2.u.b[8]);
	EXPECT_EQ(guid1.u.b[9], guid2.u.b[9]);
	EXPECT_EQ(guid1.u.b[10], guid2.u.b[10]);
	EXPECT_EQ(guid1.u.b[11], guid2.u.b[11]);
	EXPECT_EQ(guid1.u.b[12], guid2.u.b[12]);
	EXPECT_EQ(guid1.u.b[13], guid2.u.b[13]);
	EXPECT_EQ(guid1.u.b[14], guid2.u.b[14]);
	EXPECT_EQ(guid1.u.b[15], guid2.u.b[15]);
}

#define LONG_STR  \
"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa" \
"bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb" \
"ccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc" \
"ddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd"

TEST_F(MipiSysTFixtureBase, syst_hash)
{
	mipi_syst_u32 val1, val2, val3;

	val1 = _MIPI_SYST_HASH_AT_CPP_TIME("", 0);
	val2 = _MIPI_SYST_HASH_AT_RUN_TIME("", 0);
	EXPECT_EQ(val1, val2);

	EXPECT_EQ(0x0, _MIPI_SYST_HASH_AT_RUN_TIME("", 0));
	EXPECT_EQ(0x19ae84c4, _MIPI_SYST_HASH_AT_RUN_TIME("hello world", 0));

	val1 = _MIPI_SYST_HASH_AT_CPP_TIME("hello world", 0);
	val2 = _MIPI_SYST_HASH_AT_RUN_TIME("hello world", 0);
	EXPECT_EQ(val1, val2);

	val1 = _MIPI_SYST_HASH_AT_CPP_TIME(LONG_STR, 0);
	val2 = _MIPI_SYST_HASH_AT_RUN_TIME(LONG_STR, 0);
	EXPECT_EQ(val1, val2);

	val1 = _MIPI_SYST_HASH_AT_CPP_TIME(LONG_STR, 0);
	val2 = _MIPI_SYST_HASH_AT_CPP_TIME(LONG_STR, 1);
	val3 = _MIPI_SYST_HASH_AT_CPP_TIME(LONG_STR, 3);
	EXPECT_NE(val1, val2);
	EXPECT_NE(val1, val3);
	EXPECT_NE(val2, val3);
}