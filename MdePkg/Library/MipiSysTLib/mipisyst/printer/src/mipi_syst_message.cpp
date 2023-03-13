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

#include<sstream>
#include <iomanip>
#include "mipi_syst_message.h"
#include "mipi_syst_collateral.h"
#include "mipi_syst_printf.h"

MIPI_SYST_NAMESPACE_BEGIN

// Code for storing and printing out decoded message content
//

void message::setLocAddr32(uint32_t addr)
{
	m_loc.tag |= location_type::ADDRESS32;
	m_loc.tag &= ~location_type::ADDRESS64;
	m_loc.address = addr;
}

void message::setLocAddr64(uint64_t addr)
{
	m_loc.tag |= location_type::ADDRESS64;
	m_loc.tag &= ~location_type::ADDRESS32;
	m_loc.address = addr;
}

void message::setLocFileLine(uint32_t file, uint32_t line)
{
	m_loc.tag |= location_type::IDANDLINE;
	m_loc.file = file;
	m_loc.line = line;
}

// Check type for short message
//
bool message::isShort(message::header hdr) {
	switch (hdr.field.type) {
	case message::type::SHORT32:
	case message::type::SHORT64:
		return true;

	case  message::type::BUILD:
		switch (hdr.field.subtype) {
		case message::subtype_build::BUILD_COMPACT32:
		case message::subtype_build::BUILD_COMPACT64:
			return true;
		}
	}
	return false;
}


// Pretty print decode status value
//
static const char * status2string(const message& msg)
{
	switch (msg.getState()) {
	case message::decode_state::OK:
		return "OK";
	case message::decode_state::UNKNOWN_TYPE:
		return "UNKNOWN_TYPE";
	case message::decode_state::TOO_SHORT:
		return "TOO_SHORT";
	case message::decode_state::CHECKSUM_ERROR:
		return "CHECKSUM_ERROR";
	case message::decode_state::MISSING_COLLATERAL:
		return "MISSING_COLLATERAL";
	default:
		return "UNKNOWN_STATE";
	}
}

// Pretty print sevrity values
//
static const char * severity2string[] = {
	"MAX",
	"FATAL",
	"ERROR",
	"WARNING",
	"INFO",
	"USER1",
	"USER2",
	"DEBUG"
};

// Pretty print type:subtype values
//
static const std::string type2string(
	message::header hdr,
	const collateral * coll)
{
	static const char * typenames[] = {
		"BUILD",
		"SHORT32",
		"STRING",
		"CATALOG",
		"UNKNOWN(4)",
		"UNKNOWN(5)",
		"RAW",
		"SHORT64",
		"CLOCK",
		"UNKNOWN(9)",
		"UNKNOWN(10)",
		"UNKNOWN(11)",
		"UNKNOWN(12)",
		"UNKNOWN(13)",
		"UNKNOWN(14)",
		"UNKNOWN(15)",
	};

	uint32_t type(hdr.field.type);
	std::stringstream sstr;

	sstr << typenames[type];

	// compute subtype string
	//
	std::string subtype("??");

	switch (type) {
	case message::type::SHORT32:
	case message::type::SHORT64:
		subtype.clear(); // no subtype
		break;

	case message::type::BUILD:
		switch (hdr.field.subtype) {
		case  message::subtype_build::BUILD_LONG:
			subtype = "LONG";
			break;
		case  message::subtype_build::BUILD_COMPACT32:
			subtype = "COMPACT32";
			break;
		case  message::subtype_build::BUILD_COMPACT64:
			subtype = "COMPACT64";
			break;
		}
		break;
	case  message::type::STRING:
		switch (hdr.field.subtype) {
		case  message::subtype_string::STRING_GENERIC:
			subtype = "GENERIC";
			break;
		case  message::subtype_string::STRING_FUNCTIONENTER:
			subtype = "ENTER";
			break;
		case  message::subtype_string::STRING_FUNCTIONEXIT:
			subtype = "EXIT";
			break;
		case  message::subtype_string::STRING_INVALIDPARAM:
			subtype = "INVPARAM";
			break;
		case  message::subtype_string::STRING_ASSERT:
			subtype = "ASSERT";
			break;
		case  message::subtype_string::STRING_PRINTF_32:
			subtype = "PRINTF32";
			break;
		case  message::subtype_string::STRING_PRINTF_64:
			subtype = "PRINTF64";
			break;
		}
		break;
	case  message::type::CATALOG:
		switch (hdr.field.subtype) {
		case  message::subtype_catalog::CATALOG_ID32_P32:
			subtype = "ID32P32";
			break;
		case  message::subtype_catalog::CATALOG_ID64_P32:
			subtype = "ID64P32";
			break;
		case  message::subtype_catalog::CATALOG_ID32_P64:
			subtype = "ID32P64";
			break;
		case  message::subtype_catalog::CATALOG_ID64_P64:
			subtype = "ID64P64";
			break;
		}
		break;
	case  message::type::RAW:
	{
		const std::string * name(nullptr);
		if (coll && (name = coll->getWriteType(hdr.field.subtype)) != nullptr)
		{
			std::stringstream dest;
			hostPrintf(dest, *name, hdr.field.subtype, std::vector<int>());
			subtype = dest.str();
		} else {
			subtype = std::to_string(hdr.field.subtype);
		}
	} break;

	case  message::type::CLOCK:
		switch (hdr.field.subtype) {
		case  message::subtype_clock::CLOCK_SYNC:
			subtype = "SYNC";
			break;
		}
		break;
	}

	if (!subtype.empty()) {
		sstr << ':' << subtype;
	}

	return sstr.str();
}

// CSV double-quote escaping, replace " with "" and eliminate newlines
//
void csvQuoteString(std::ostream& os, const std::string& s)
{
	for (auto c : s) {
		if (c == '\n') {
			os << ' ';   // newline not supported in CSV
		} else if (c == '"') {
			os << c << c;
		} else {
			os << c;
		}
	}
}

// location information printing
//
std::string location2string(
	const message::location& loc,
	const collateral * collateral)
{
	bool hasAddress(0!=(loc.tag & (message::ADDRESS32 | message::ADDRESS32)));
	bool hasFileLine(0 != (loc.tag & message::IDANDLINE));

	std::stringstream sstr;

	if (hasFileLine) {
		const std::string * fileName(
			collateral != nullptr ?
				collateral->getSourceFile(loc.file) : nullptr);

		if (fileName != nullptr) {
			sstr << (*fileName) << ':' << loc.line;
		} else {
			sstr << loc.file << ':' << loc.line;
		}
		if (hasAddress) sstr << ' ';
	}

	if (hasAddress) {
		if (loc.tag & message::ADDRESS32) {
			sstr << toHexValue((uint32_t)loc.address);
		} else {
			sstr << toHexValue(loc.address);
		}
	}

	return sstr.str();
}
const char message::csvHeaderString[] = "Decode Status,Payload,Type,Severity,Origin,Unit,Message TimeStamp,Context TimeStamp,Location,Raw Length,Checksum,Collateral";

// CSV line formatting of a message
//
std::ostream& operator << (std::ostream& os, const message& msg)
{
	char delimiter(',');
	const collateral * coll(msg.getCollateral());

	// turn volatile fields like timestamp/crc off when running unit testing
	//
	static bool unit_testing(getenv("SYST_UNITTESTING") != NULL);

	// status
	os << status2string(msg) << delimiter;

	// payload
	os << '"';
	csvQuoteString(os, msg.getPayload());
	os << '"' << delimiter;

	if (msg.getState() != message::OK && msg.getState() != message::MISSING_COLLATERAL)
	{
		// decode failed, report other fields as empty. The data can't be trusted.
		//
		os << ",,,,," << std::endl;
		return os;
	}

	// Type/Subtype
	//
	os << type2string(msg.getHeader(), msg.getCollateral()) << delimiter;

	// Severity
	//
	os << severity2string[msg.getHeader().field.severity] << delimiter;

	// client name and unit
	//
	os << msg.getClientName() << delimiter;
	os << msg.getUnit() << delimiter;

	// time stamps
	if (msg.getHeader().field.timestamp) {
		os << (unit_testing ?
			"<--UNITTEST-HIDE_TS-->" : toHexValue(msg.getMessageTS()));
	}
	os << delimiter;

	os << (unit_testing ?
		"<--UNITTEST-HIDE_TS-->" : toHexValue(msg.getContextTS())) << delimiter;

	// location
	//
	csvQuoteString(os, location2string(msg.getLocation(), coll));
  	os << delimiter;

	//  raw message length
	//
	os << std::to_string(msg.getLength()) << delimiter;

	// crc
	//
	if (msg.getHeader().field.chksum) {
		os << (unit_testing ? "<--UNITTEST-HIDE_CRC-->" : toHexValue(msg.getCrc()));
	}
	os << delimiter;

	if (coll != nullptr) {
		csvQuoteString(os, coll->getFileName());
	}
	os << std::endl;

	return os;
}

MIPI_SYST_NAMESPACE_END