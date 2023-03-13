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

#include "mipi_syst_decode.h"
#include "mipi_syst_printf.h"

MIPI_SYST_NAMESPACE_BEGIN

/// Helper to extract string from byte array,
//
// @param dest where to write bytes to
// @param offset where to start from
// @param data byte array
// @param len number of bytes in array
//
// @return bytes consumed or < 0 if no null byte found
//
static int32_t bytes2String(std::string& dest, const uint8_t * data, uint32_t len)
{
	uint32_t i(0);

	while(i <len)
	{
		if (!data[i]) {
			break;  // null byte found
		}
		dest += data[i++];
	}

	if (i == len && data[i - 1]) {
		return -1;
	}

	return (uint32_t)dest.size();
}

// Helper for printf formating
//
static std::string printfFormat(
	const std::string& fmt,
	bool is32Bit,
	const void * data,
	uint32_t len)
{
	msgprintf printer(is32Bit);
	std::string result;
	if (len > 0) {
		printer.format(fmt, data, len, result);
	}
	else {
		printer.format(fmt, NULL, 0, result);
	}
	return result;
}

void decoder::setBuildNumber(uint64_t id, const guid& g) const
{
	// cache guid <-> id association
	// TODO: A production quality implementation should add a sanity check here
	//      and issue a warning if the ID changes compared to an earlier one.
	//
	if (build_by_guid.find(g) != build_by_guid.end()) {
		build_by_guid[g] = id;
	}

	// Remove an eventually earlier cached collateral to guid mapping. We recompute
	// next time with the now known build id.
	//
	auto cache_hit(collateral_by_guid.find(g));
	if (cache_hit != collateral_by_guid.end()) {
		collateral_by_guid.erase(cache_hit);
	}
}


// Decode single SyS-T message
//
bool decoder::decode(message& dest, const std::vector<uint8_t>& data, const decode_context * ctx) const
{
	bool rt(false);

	try {
		// header
		//
		if (data.size() < sizeof(uint32_t)) throw tooshort_exception();
		message::header hdr(bytes2ValLE<uint32_t>(&data[0]));

		// short message ?
		//
		if (message::isShort(hdr)) {
			rt =  decodeShortMessage(dest, &data[0], (uint32_t)data.size(), ctx);
		} else {
			rt = decodeNormalMessage(dest, &data[0], (uint32_t)data.size(), ctx);
		}

		if (rt  && dest.getHeader().field.type == message::BUILD) {
			// found build message, store build number for later
			//
			setBuildNumber(dest.getBuild(), dest.getGuid());
		}
	} catch (decode_exception& de) {
		dest.setStatus(de.get_state());

		// decode failed, show raw byte dump as payload
		//
		std::stringstream sstr;
		sstr << std::hex << std::uppercase << std::setfill('0') << std::setw(2);
		for (auto uc : data) {
			sstr << (uint16_t)uc;
		}
		dest.setPayload(sstr.str());
	}
	return rt;
}


template<class T> void decodeShortPayload(message & m, T v)
{
	const collateral * coll(m.getCollateral());
	const collateral::catalogentry<T> * entry(
		coll ? coll->getShortEntry(v) : nullptr);

	if (entry) {
		std::stringstream dest;
		hostPrintf(dest, entry->msg, v & (~entry->mask), std::vector<int>());
		m.setPayload(dest.str());
	} else {
		m.setPayload(toHexValue(v));
	}
}

bool decoder::decodeShortMessage(
	message& dest,
	const uint8_t *data, uint32_t length,
	const decode_context * ctx) const
{
	if (length < sizeof(uint32_t)) throw tooshort_exception();

	uint32_t type(data[0] & 0x0F); // Bit 0..3 = type
	uint32_t subtype(data[3] & 0x3F); // Bit 24..29 = subtype

	dest.setHeader(0);  // all optional fields off
	dest.setType(type);
	dest.setLength(length);
	dest.setCollateral(ctx ? findCollateral(ctx->getGuid()) : nullptr);
	dest.setContextTS(ctx ? ctx->getTS() : 0);

	switch (type) {
	case message::type::SHORT32:
		decodeShortPayload(dest, bytes2ValLE<uint32_t>(data) >> 4);
		break;

	case message::type::SHORT64:
		if (length < sizeof(uint64_t)) throw tooshort_exception();
		decodeShortPayload(dest, bytes2ValLE<uint64_t>(data) >> 4);
		break;

	case message::type::BUILD:
		dest.setSubType(subtype);
		switch (subtype) {
		case message::subtype_build::BUILD_COMPACT32:
		{
			uint32_t val(bytes2ValLE<uint32_t>(data));
			val = ((val & 0x00FFFFF0) >> 4) | ((val & 0x30000000) >> 10);
			dest.setBuild(val);
			dest.setPayload(toHexValue(val));
			break;
		}
		case message::subtype_build::BUILD_COMPACT64:
		{
			if (length < sizeof(uint64_t)) throw tooshort_exception();

			uint64_t val(bytes2ValLE<uint64_t>(data));
			val = ((val & 0x0000000000FFFFF0ull) >> 4) |
				((val & 0xFFFFFFFF30000000ull) >> 10);
			dest.setBuild(val);
			dest.setPayload(toHexValue(val));

			break;
		}
		default:
			throw unknown_type_exception();
		}
		break;
	default:
		throw unknown_type_exception();
	}

	dest.setStatus(message::decode_state::OK);

	return true;
}

// Decode "normal" SyS-T message data
//
bool decoder::decodeNormalMessage(message& dest, const uint8_t * data, uint32_t length, const decode_context * ctx) const
{
	uint32_t offset(0);

	dest.setContextTS(ctx ? ctx->getTS() : 0);
	dest.setLength(length);

	// header
	//
	if (length < (offset + sizeof(uint32_t))) throw tooshort_exception();
	dest.setHeader(bytes2ValLE<uint32_t>(data+offset));
	offset += sizeof(uint32_t);

	// checksum at the end ?
	//
	if (dest.getHeader().field.chksum) {
		if (length < offset + sizeof(uint32_t)) throw tooshort_exception();
		length -= sizeof(uint32_t);
		dest.setCrc(bytes2ValLE<uint32_t>(data+length));
	}

	// guid
	//
	if (dest.getHeader().field.guid) {
		if (length < (offset + sizeof(uint64_t[2]))) throw tooshort_exception();

		dest.setGuid(guid(data+offset));
		offset += sizeof(uint64_t[2]);
	} else {
		dest.setGuid(generatePseudoGuid(dest.getHeaderOrigin()));
	}

	// location
	//
	if (dest.getHeader().field.location) {
		if (length < offset + sizeof(uint8_t)) throw tooshort_exception();
		uint32_t tag(data[offset++]);
		if (tag & 0x1) {
			// 64 bit
			if (length < offset + sizeof(uint64_t)) throw tooshort_exception();
			if (tag & 0x2) { // addr
				uint64_t addr(bytes2ValLE<uint64_t>(data+offset));
				dest.setLocAddr64(addr);
			} else {
				uint32_t file(bytes2ValLE<uint32_t>(data + offset));
				uint32_t line(bytes2ValLE<uint32_t>(data + offset + 4));
				dest.setLocFileLine(file, line);
			}
			offset += sizeof(uint64_t);
		} else {
			// 32 bit
			if (length < offset + sizeof(uint32_t)) throw tooshort_exception();
			if (tag & 0x2) { // addr
				uint32_t addr(bytes2ValLE<uint32_t>(data + offset));
				dest.setLocAddr32(addr);
			} else {
				uint16_t file(bytes2ValLE<uint16_t>(data + offset));
				uint16_t line(bytes2ValLE<uint16_t>(data + offset + 2));
				dest.setLocFileLine(file, line);
			}
			offset += sizeof(uint32_t);
		}
	}

	// length
	//
	uint32_t payload_len(length - offset);

	if (dest.getHeader().field.length) {
		if (length < offset + sizeof(uint16_t)) throw tooshort_exception();
		payload_len = bytes2ValLE<uint16_t>(data + offset);
		offset += sizeof(uint16_t);
	}

	// timestamp
	//
	if (dest.getHeader().field.timestamp) {
		if (length < offset + sizeof(uint64_t)) throw tooshort_exception();
		dest.setMessageTS(bytes2ValLE<uint64_t>(data + offset));
		offset += sizeof(uint64_t);
	}

	if (dest.getHeader().field.length) {
		// length given, compare with remaining payload length
		//
		int diff(length - offset - payload_len);
		if (diff < 0) {
			throw tooshort_exception();
		} else if (diff > 0) {
			throw toolong_exception();
		}
	}

	// check CRC32 if set
	//
	if (dest.getHeader().field.chksum) {
		uint32_t crc = getCrc32(data, length);
		if (crc != dest.getCrc()) {
			throw crc_error_exception();
		}
	}

	// Check for collateral information
	//
	const collateral* coll = findCollateral(dest.getGuid());
	dest.setCollateral(coll);

	// Set client name based on collateral (or use raw guid/origin from message)
	//
	if (coll != nullptr) {
		dest.setClientName(coll->getName());
	}
	else {
		std::stringstream sstr;
		if (dest.getHeader().field.guid) {
			sstr << dest.getGuid();
		} else {
			sstr << dest.getHeaderOrigin();
		}
		dest.setClientName(sstr.str());
	}

	// Payload decode depening on type
	//
	(this->*payloadDecode[dest.getHeader().field.type])(dest, data+offset, length - offset);

	dest.setStatus(message::decode_state::OK);
	return true;
}

void decoder::decodeInvalidType(message& dest, const uint8_t * data, uint32_t len) const
{
	(void)dest;
	(void)data;
	(void)len;

	throw unknown_type_exception();
}

void decoder::decodeBuildPayload(message& dest, const uint8_t * data, uint32_t len) const
{
	std::stringstream sstr;

	switch (dest.getHeader().field.subtype) {
	case message::subtype_build::BUILD_LONG:
		if (len < sizeof(uint64_t)) throw tooshort_exception();

		dest.setBuild(bytes2ValLE<uint64_t>(data));
		sstr << toHexValue(dest.getBuild());

		data += sizeof(uint64_t);
		len  -= sizeof(uint64_t);

		if (0 != len) { // string follows ....
			std::string payload;
			if (bytes2String(payload, data, len) > 0) {
				sstr << " " << payload;
			} else {
				throw tooshort_exception();
			}
		}
		break;
	default:
		throw unknown_type_exception();
		break;
	}

	dest.setPayload(sstr.str());
}

void decoder::decodeStringPayload(message& dest, const uint8_t * data, uint32_t len) const
{
	std::string format;
	int fmtlen;

	if (!len || (fmtlen = bytes2String(format, data, len)) <= 0) {
		throw tooshort_exception();
	}

	std::stringstream sstr;

	switch (dest.getHeader().field.subtype) {
	case message::subtype_string::STRING_PRINTF_32:
	case message::subtype_string::STRING_PRINTF_64:
		sstr << printfFormat(
			format,
			dest.getHeader().field.subtype ==
			message::subtype_string::STRING_PRINTF_32,
			&data[fmtlen+1],
			len - fmtlen - 1);
		break;

	case message::subtype_string::STRING_ASSERT:
	case message::subtype_string::STRING_FUNCTIONENTER:
	case message::subtype_string::STRING_FUNCTIONEXIT:
	case message::subtype_string::STRING_GENERIC:
	case message::subtype_string::STRING_INVALIDPARAM:
		sstr << format;
		break;
	default:
		throw unknown_type_exception();
		break;
	}

	dest.setPayload(sstr.str());
}

void decoder::decodeCatalogPayload(message& dest, const uint8_t * data, uint32_t len) const
{
	const collateral * coll(dest.getCollateral());
	if (coll == nullptr) throw missing_collateral_exception();

	std::stringstream sstr;
	bool is32Bit(false);
	const collateral::sourcepos * srcpos(nullptr);
	const std::string * fmt(nullptr);

	switch (dest.getHeader().field.subtype) {
	case message::subtype_catalog::CATALOG_ID32_P32:
		is32Bit = true;
		// fall through

	case message::subtype_catalog::CATALOG_ID32_P64:
	{
		if (len < sizeof(uint32_t)) throw tooshort_exception();
		const collateral::catalogentry<uint32_t> * entry(nullptr);

		if ((entry = coll->getCatalogEntry(bytes2ValLE<uint32_t>(data))) == nullptr) {
			sstr
				<< "Undefined catalog id 0x"
				<< toHexValue(bytes2ValLE<uint32_t>(data));
		} else {
			fmt = &entry->msg;
			srcpos = entry;
		}
		data += sizeof(uint32_t);
		len -= sizeof(uint32_t);
		break;
	}
	case message::subtype_catalog::CATALOG_ID64_P32:
		is32Bit = true;
		// fall through
	case message::subtype_catalog::CATALOG_ID64_P64:
	{
		if (len < sizeof(uint64_t)) throw tooshort_exception();
		const collateral::catalogentry<uint64_t> * entry(nullptr);

		if ((entry = coll->getCatalogEntry(bytes2ValLE<uint64_t>(data))) == nullptr) {
			sstr
				<< "Undefined catalog id 0x"
				<< toHexValue(bytes2ValLE<uint64_t>(data));
		} else {
			fmt = &entry->msg;
			srcpos = entry;
		}
		data += sizeof(uint64_t);
		len -= sizeof(uint64_t);
		break;
	}
	default:
		throw unknown_type_exception();
		break;
	}

	if (fmt != nullptr) {
		sstr << printfFormat(*fmt, is32Bit, data, len);

		// if current message has no file attribute from payload, but the catalog entry
		// does provides one, use the one from the catalog
		//
		if (0 != srcpos->m_file && 0 == (dest.getLocation().tag & message::IDANDLINE))
		{
			dest.setLocFileLine(srcpos->m_file, srcpos->m_line);
		}
	}

	dest.setPayload(sstr.str());
}

void decoder::decodeRawPayload(message& dest, const uint8_t * data, uint32_t len) const
{
	std::stringstream sstr;

	while (len--) {
		sstr << std::setfill('0') << std::setw(2) << std::hex << (uint16_t)*data++;
	}

	dest.setPayload(sstr.str());
}

void decoder::loadCollateral(const std::string& filename)
{
	for (auto items : collateral::parseXml(filename)) {
		collaterals.insert(collaterals.end(), items);
	}
}

guid decoder::generatePseudoGuid(uint8_t origin)
{
	// A pseudo-guid is defined by storing the origin value into
	// bits [7..0] of the guid[8] byte. The highest bit is forced
	// to zero, which distiquishes it from an RFC4122 compliant
	// GUID.
	// This example code has no additional transport data for
	// identifying the message client. All other values are
	// set to 0.
	//
	uint8_t pseudo_guid_data[16] = {
		0,0,0,0,
		0,0,0,origin,
		0,0,0,0,
		0,0,0,0
	};
	return guid(pseudo_guid_data);
}

collateral * decoder::findCollateral(const guid& g) const
{
	// do we already know the catalog for this GUID ?
	//
	auto cache_hit(collateral_by_guid.find(g));
	if (cache_hit != collateral_by_guid.end()) {
		return cache_hit->second;
	}

	uint64_t build(0);
	auto  build_hit(build_by_guid.find(g));
	if (build_hit != build_by_guid.end()) {
		build = build_hit->second;
	}

	// search all catalogs
	//
	auto match = std::find_if(collaterals.begin(), collaterals.end(),
		[&g,build](const collateral* v)
	{
		return (v->match(g, build));
	});

	if (match != collaterals.end()) {
		collateral_by_guid[g] = *match;
		return *match;
	}
	return nullptr;
}

// Message type dependend payload decode handlers
//
decoder::decode_payload_f decoder::payloadDecode[] = {
	&decoder::decodeBuildPayload,		// [0]
	&decoder::decodeInvalidType,		// [1]
	&decoder::decodeStringPayload,		// [2]
	&decoder::decodeCatalogPayload,		// [3]
	&decoder::decodeInvalidType,		// [4]
	&decoder::decodeInvalidType,		// [5]
	&decoder::decodeRawPayload,		// [6]
	&decoder::decodeInvalidType,		// [7]
	&decoder::decodeInvalidType,		// [8]
	&decoder::decodeInvalidType,		// [9]
	&decoder::decodeInvalidType,		// [10]
	&decoder::decodeInvalidType,		// [11]
	&decoder::decodeInvalidType,		// [12]
	&decoder::decodeInvalidType,		// [13]
	&decoder::decodeInvalidType,		// [14]
	&decoder::decodeInvalidType		// [15]
};

// Build CRC over a byte buffer (Castagnoli CRC-32C)
//
uint32_t decoder::getCrc32(const uint8_t * data, size_t len)
{
	uint32_t crc(0xFFFFFFFF);

	while (len--) {
		crc = crc32c_table[((int)crc ^ (*data++)) & 0xFF] ^ (crc >> 8);
	}
	return crc ^ 0xFFFFFFFF;
}

// Pre-computed CRC-C values for all byte values using the
// polynomial 0x1EDC6F41 (Castagnoli).
//
const uint32_t decoder::crc32c_table[256] = {
	0x00000000,
	0xF26B8303,
	0xE13B70F7,
	0x1350F3F4,
	0xC79A971F,
	0x35F1141C,
	0x26A1E7E8,
	0xD4CA64EB,
	0x8AD958CF,
	0x78B2DBCC,
	0x6BE22838,
	0x9989AB3B,
	0x4D43CFD0,
	0xBF284CD3,
	0xAC78BF27,
	0x5E133C24,
	0x105EC76F,
	0xE235446C,
	0xF165B798,
	0x030E349B,
	0xD7C45070,
	0x25AFD373,
	0x36FF2087,
	0xC494A384,
	0x9A879FA0,
	0x68EC1CA3,
	0x7BBCEF57,
	0x89D76C54,
	0x5D1D08BF,
	0xAF768BBC,
	0xBC267848,
	0x4E4DFB4B,
	0x20BD8EDE,
	0xD2D60DDD,
	0xC186FE29,
	0x33ED7D2A,
	0xE72719C1,
	0x154C9AC2,
	0x061C6936,
	0xF477EA35,
	0xAA64D611,
	0x580F5512,
	0x4B5FA6E6,
	0xB93425E5,
	0x6DFE410E,
	0x9F95C20D,
	0x8CC531F9,
	0x7EAEB2FA,
	0x30E349B1,
	0xC288CAB2,
	0xD1D83946,
	0x23B3BA45,
	0xF779DEAE,
	0x05125DAD,
	0x1642AE59,
	0xE4292D5A,
	0xBA3A117E,
	0x4851927D,
	0x5B016189,
	0xA96AE28A,
	0x7DA08661,
	0x8FCB0562,
	0x9C9BF696,
	0x6EF07595,
	0x417B1DBC,
	0xB3109EBF,
	0xA0406D4B,
	0x522BEE48,
	0x86E18AA3,
	0x748A09A0,
	0x67DAFA54,
	0x95B17957,
	0xCBA24573,
	0x39C9C670,
	0x2A993584,
	0xD8F2B687,
	0x0C38D26C,
	0xFE53516F,
	0xED03A29B,
	0x1F682198,
	0x5125DAD3,
	0xA34E59D0,
	0xB01EAA24,
	0x42752927,
	0x96BF4DCC,
	0x64D4CECF,
	0x77843D3B,
	0x85EFBE38,
	0xDBFC821C,
	0x2997011F,
	0x3AC7F2EB,
	0xC8AC71E8,
	0x1C661503,
	0xEE0D9600,
	0xFD5D65F4,
	0x0F36E6F7,
	0x61C69362,
	0x93AD1061,
	0x80FDE395,
	0x72966096,
	0xA65C047D,
	0x5437877E,
	0x4767748A,
	0xB50CF789,
	0xEB1FCBAD,
	0x197448AE,
	0x0A24BB5A,
	0xF84F3859,
	0x2C855CB2,
	0xDEEEDFB1,
	0xCDBE2C45,
	0x3FD5AF46,
	0x7198540D,
	0x83F3D70E,
	0x90A324FA,
	0x62C8A7F9,
	0xB602C312,
	0x44694011,
	0x5739B3E5,
	0xA55230E6,
	0xFB410CC2,
	0x092A8FC1,
	0x1A7A7C35,
	0xE811FF36,
	0x3CDB9BDD,
	0xCEB018DE,
	0xDDE0EB2A,
	0x2F8B6829,
	0x82F63B78,
	0x709DB87B,
	0x63CD4B8F,
	0x91A6C88C,
	0x456CAC67,
	0xB7072F64,
	0xA457DC90,
	0x563C5F93,
	0x082F63B7,
	0xFA44E0B4,
	0xE9141340,
	0x1B7F9043,
	0xCFB5F4A8,
	0x3DDE77AB,
	0x2E8E845F,
	0xDCE5075C,
	0x92A8FC17,
	0x60C37F14,
	0x73938CE0,
	0x81F80FE3,
	0x55326B08,
	0xA759E80B,
	0xB4091BFF,
	0x466298FC,
	0x1871A4D8,
	0xEA1A27DB,
	0xF94AD42F,
	0x0B21572C,
	0xDFEB33C7,
	0x2D80B0C4,
	0x3ED04330,
	0xCCBBC033,
	0xA24BB5A6,
	0x502036A5,
	0x4370C551,
	0xB11B4652,
	0x65D122B9,
	0x97BAA1BA,
	0x84EA524E,
	0x7681D14D,
	0x2892ED69,
	0xDAF96E6A,
	0xC9A99D9E,
	0x3BC21E9D,
	0xEF087A76,
	0x1D63F975,
	0x0E330A81,
	0xFC588982,
	0xB21572C9,
	0x407EF1CA,
	0x532E023E,
	0xA145813D,
	0x758FE5D6,
	0x87E466D5,
	0x94B49521,
	0x66DF1622,
	0x38CC2A06,
	0xCAA7A905,
	0xD9F75AF1,
	0x2B9CD9F2,
	0xFF56BD19,
	0x0D3D3E1A,
	0x1E6DCDEE,
	0xEC064EED,
	0xC38D26C4,
	0x31E6A5C7,
	0x22B65633,
	0xD0DDD530,
	0x0417B1DB,
	0xF67C32D8,
	0xE52CC12C,
	0x1747422F,
	0x49547E0B,
	0xBB3FFD08,
	0xA86F0EFC,
	0x5A048DFF,
	0x8ECEE914,
	0x7CA56A17,
	0x6FF599E3,
	0x9D9E1AE0,
	0xD3D3E1AB,
	0x21B862A8,
	0x32E8915C,
	0xC083125F,
	0x144976B4,
	0xE622F5B7,
	0xF5720643,
	0x07198540,
	0x590AB964,
	0xAB613A67,
	0xB831C993,
	0x4A5A4A90,
	0x9E902E7B,
	0x6CFBAD78,
	0x7FAB5E8C,
	0x8DC0DD8F,
	0xE330A81A,
	0x115B2B19,
	0x020BD8ED,
	0xF0605BEE,
	0x24AA3F05,
	0xD6C1BC06,
	0xC5914FF2,
	0x37FACCF1,
	0x69E9F0D5,
	0x9B8273D6,
	0x88D28022,
	0x7AB90321,
	0xAE7367CA,
	0x5C18E4C9,
	0x4F48173D,
	0xBD23943E,
	0xF36E6F75,
	0x0105EC76,
	0x12551F82,
	0xE03E9C81,
	0x34F4F86A,
	0xC69F7B69,
	0xD5CF889D,
	0x27A40B9E,
	0x79B737BA,
	0x8BDCB4B9,
	0x988C474D,
	0x6AE7C44E,
	0xBE2DA0A5,
	0x4C4623A6,
	0x5F16D052,
	0xAD7D5351
};

MIPI_SYST_NAMESPACE_END