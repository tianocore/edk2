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

#ifndef MIPI_SYST_MESSAGE_H_included
#define MIPI_SYST_MESSAGE_H_included

#include <cstdint>
#include <string>
#include <vector>
#include <ostream>

#include "mipi_syst_printer.h"
#include "mipi_syst_guid.h"

MIPI_SYST_NAMESPACE_BEGIN

class collateral;

/// Internal representation of a decoded SyS-T message
///
class message {
public:
	enum decode_state {
		OK = 0,
		UNKNOWN_TYPE = 1,
		TOO_SHORT = 2,
		TOO_LONG = 3,
		CHECKSUM_ERROR = 4,
		MISSING_COLLATERAL = 5
	};

	union header {
		public:
		struct fields {
#if defined(MIPI_SYST_BIG_ENDIAN)
			uint32_t res31 : 1;        ///< reserved, must be 0
			uint32_t res30 : 1;        ///< reserved, must be 0
			uint32_t subtype : 6;      ///< type dependent sub category
			uint32_t guid : 1;         ///< 128 bit GUID present
			uint32_t originUnit : 11;  ///< unit for GUID or module:unit pair
			uint32_t timestamp : 1;    ///< indicate 64 bit timestamp
			uint32_t chksum : 1;       ///< indicate 32bit CRC
			uint32_t length : 1;       ///< indicate variable length message
			uint32_t location : 1;     ///< indicate location information
			uint32_t res7 : 1;         ///<reserved, must be 0
			uint32_t severity : 3;     ///< severity level of message
			uint32_t type : 4;         ///< SyS-T message type ID
#else
			uint32_t type : 4;         ///< SyS-T message type ID
			uint32_t severity : 3;     ///< severity level of message
			uint32_t res7 : 1;         /// <reserved, must be 0
			uint32_t location : 1;     ///< indicate location information
			uint32_t length : 1;       ///< indicate variable length message
			uint32_t chksum : 1;       ///< indicate 32bit CRC
			uint32_t timestamp : 1;    ///< indicate 64 bit timestamp
			uint32_t originUnit : 11;  ///< unit for GUID or module:unit pair
			uint32_t guid : 1;         ///< 128 bit GUID present
			uint32_t subtype : 6;      ///< type dependent sub category
			uint32_t res30 : 1;        ///< reserved, must be 0
			uint32_t res31 : 1;        ///< reserved, must be 0
#endif
		} field;
		uint32_t val;

		header(uint32_t v = 0) : val(v) {};
	};

	enum location_type {
		NONE        = 0,
		ADDRESS32   = (1<<1),
		ADDRESS64   = (1<<2),
		IDANDLINE   = (1<<3)
	};

	struct location {
		uint32_t tag;
		uint64_t address;
		uint32_t file;
		uint32_t line;

		location() : tag(0), address(0), file(0), line(0) {}
	};

	enum type {
		BUILD = 0,           ///< client build version  messages
		SHORT32 = 1,         ///< tag only message
		STRING = 2,          ///< text message output
		CATALOG = 3,         ///< catalog message output
		RAW = 6,             ///< raw binary data
		SHORT64 = 7,         ///*< raw binary data
		CLOCK = 8,           ///< clock sync records
	};
	enum subtype_build {
		BUILD_COMPACT32 = 0, ///< 32 bit compressed version record
		BUILD_COMPACT64 = 1, ///< 64 bit compressed version record
		BUILD_LONG = 2       ///< complete version record
	};

	enum subtype_string {
		STRING_GENERIC = 1,       ///< string generic debug
		STRING_FUNCTIONENTER = 2, ///< string is function name
		STRING_FUNCTIONEXIT = 3,  ///< string is function name
		STRING_INVALIDPARAM = 5,  ///< invalid SyS-T APIcall
		STRING_ASSERT = 7,        ///< Software Assert: failure
		STRING_PRINTF_32 = 11,    ///< printf support 32bit packing
		STRING_PRINTF_64 = 12     ///< printf support 64bit packing
	};

	enum subtype_catalog {
		CATALOG_ID32_P32 = 1,   ///< 32 bit catalog ID, 32 bit packing
		CATALOG_ID64_P32 = 2,   ///< 64 bit catalog ID, 32 bit packing
		CATALOG_ID32_P64 = 5,   ///< 32 bit catalog ID, 64 bit packing
		CATALOG_ID64_P64 = 6    ///< 64 bit catalog ID, 64 bit packing
	};
	enum subtype_clock {
		CLOCK_SYNC = 1          ///< HW clock &frequency sync
	};

	enum severity {
		SEVERITY_MAX = 0,	///< no assigined severity
		SEVERITY_FATAL = 1,	///< critical error level
		SEVERITY_ERROR = 2,	///< error message level
		SEVERITY_WARNING = 3,	///< warning message level
		SEVERITY_INFO = 4,	///< information message level
		SEVERITY_USER1 = 5,	///< user defined level 5
		SEVERITY_USER2 = 6,	///< user defined level 6
		SEVERITY_DEBUG = 7	///< debug information level
	};


public:

	message() : m_state(UNKNOWN_TYPE), m_build(0) {}

	void setHeader(uint32_t val) { m_hdr.val = val; }
	void setStatus(const decode_state& s) { m_state = s; }
	void setPayload(const std::string& s) { m_payload = s; }
	void setType(uint32_t val) { m_hdr.field.type = val; }
	void setSubType(uint32_t val) { m_hdr.field.subtype = val; }
	void setGuid(const guid& g) { m_guid = g;  }
	void setBuild(uint64_t b) { m_build = b; }
	void setCrc(const uint32_t c) { m_crc = c; }
	void setLength(const uint32_t l) { m_length = l; }
	void setClientName(const std::string& s) { m_clientname = s; }
	void setCollateral(const collateral *c ) { m_coll = c; }

	void setLocAddr32(uint32_t addr);
	void setLocAddr64(uint64_t addr);
	void setLocFileLine(uint32_t file, uint32_t line);

	void setMessageTS(uint64_t ts) { m_msgTS = ts;  }
	void setContextTS(uint64_t ts) { m_ctxTS = ts; }

	const header& getHeader() const { return m_hdr; }
	const std::string& getPayload() const { return m_payload; }
	const decode_state& getState() const { return m_state; }
	const guid& getGuid() const { return m_guid; }
	const location& getLocation() const { return m_loc; }
	const uint32_t getCrc() const { return m_crc; }
	const uint32_t getLength() const { return m_length; }
	const std::string& getClientName() const { return m_clientname; }
	const collateral * getCollateral() const { return m_coll; }
	uint8_t getHeaderOrigin() const { return (uint8_t)(m_hdr.field.originUnit >> 4); }
	uint32_t getUnit() const {
		return m_hdr.field.guid ? m_hdr.field.originUnit : (m_hdr.field.originUnit & 0xF);
	}
	uint64_t getMessageTS() const{ return m_msgTS;  }
	uint64_t getContextTS()const { return m_ctxTS; }
	uint64_t getBuild() const    { return m_build; }
	bool isShort() const { return isShort(getHeader()); };

	static bool isShort(header hdr);
	static const char csvHeaderString[];
private:
	decode_state m_state;

	header       m_hdr;
	uint64_t     m_msgTS;
	uint64_t     m_ctxTS;
	uint64_t     m_build;
	guid         m_guid;
	location     m_loc;
	std::string  m_payload;
	std::string  m_clientname;
	uint32_t     m_length;
	uint32_t     m_crc;
	const collateral * m_coll;
};

std::ostream& operator<<(std::ostream& os, const message& msg);
MIPI_SYST_NAMESPACE_END

#endif // MIPI_SYST_MESSAGE_H_included
