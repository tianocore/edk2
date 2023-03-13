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

#ifndef MIPI_SYST_DECODE_H_included
#define MIPI_SYST_DECODE_H_included

#include <vector>
#include "mipi_syst_printer.h"
#include "mipi_syst_collateral.h"
#include "mipi_syst_message.h"

MIPI_SYST_NAMESPACE_BEGIN

class message;
class collateral;

/// Base class for decoding exceptions
//
class decode_exception {
public:
	decode_exception(message::decode_state s,
		const std::string& what = std::string())
		: state(s), msg(what)
	{}
	virtual ~decode_exception() {};

	virtual const std::string& what() const { return msg; }
	message::decode_state get_state() const { return state; }

protected:
	message::decode_state state;
	std::string msg;
};


class tooshort_exception : public decode_exception
{
public:
	tooshort_exception() :
		decode_exception(message::decode_state::TOO_SHORT) {}
};
class toolong_exception : public decode_exception
{
public:
	toolong_exception() :
		decode_exception(message::decode_state::TOO_LONG) {}
};
class unknown_type_exception : public decode_exception
{
public:
	unknown_type_exception() :
		decode_exception(message::decode_state::UNKNOWN_TYPE) {}
};

class crc_error_exception : public decode_exception
{
public:
	crc_error_exception() :
		decode_exception(message::decode_state::CHECKSUM_ERROR) {}
};

class missing_collateral_exception : public decode_exception
{
public:
	missing_collateral_exception() :
		decode_exception(message::decode_state::MISSING_COLLATERAL) {}
};

/// Decode context information.
//
//  This class is delivering timstamp and identifcation information as
//  side-band data.
//  Context based message identification is needed for client identification
//  if messages  don't carry GUID information as part of the message data.
//  Short messages are one example for such messages. In this case
//  collateral matching relies on side-band information. In systems using
//  MIPI System Trace Protocol (STP), the master and channel 16-bit values
//  are typically used for this purpose.
//  A timestamp is often also provided through a transport protocol and not
//  embedded into SyS-T messages.
//
class decode_context {
public:
	virtual const guid& getGuid() const = 0;  ///< get context based guid
	virtual  uint64_t getTS() const = 0;      ///< get context timestamp
	virtual ~decode_context() {}
};

///  MIPI SyS-T data protocol decoder
//
// This class is decoding a sequence of bytes that represent one SyS-T message into
// a message data structure for post processing.
//
class decoder {
public:
	/// Parse given file as a SyS-T collateral XML file
	//
	// Parse and add provided file as SyS-T collateral data. The function
	// throws an exception if the XML is malformed.
	//
	// @param file filename to load
	//
	void loadCollateral(const std::string& file);

	/// Store Build ID for given GUID
	//
	// Cache build ID for given GUID to make collateral matching "build number"
	// aware. If a build ID was seen, the decode looks for collateral matching this
	// one.
	//
	// @param id 64-Bit build number to remember
	// @param g  client guid associated with this build number
	//
	void setBuildNumber(uint64_t id, const guid& g) const;

	/// Decode single SyS-T raw binary message
	//
	// @param dest message data structed holding decode result
	// @param data raw data bytes representing one SyS-T message
	//
	// @return true on success
	//
	bool decode(message& dest, const std::vector<uint8_t>& data, const decode_context * ctx) const;

private:
	bool decodeShortMessage(message& dest, const uint8_t *data, uint32_t length, const decode_context * ctx) const;
	bool decodeNormalMessage(message& dest,  const uint8_t *data, uint32_t length, const decode_context * ctx) const;

	void decodeBuildPayload(message& dest, const uint8_t * data, uint32_t len) const;
	void decodeStringPayload(message& dest, const uint8_t * data, uint32_t len) const;
	void decodeCatalogPayload(message& dest, const uint8_t * data, uint32_t len) const;
	void decodeRawPayload(message& dest, const uint8_t * data, uint32_t len) const;
	void decodeInvalidType(message& dest, const uint8_t * data, uint32_t len) const;


	collateral * findCollateral(const guid& m) const;

	static guid generatePseudoGuid(uint8_t origin);
	static uint32_t getCrc32(const uint8_t * data, size_t len);

	std::vector<collateral*> collaterals;
	mutable std::map<guid, collateral*> collateral_by_guid;
	mutable std::map<guid, uint64_t> build_by_guid;
	typedef void (decoder::*decode_payload_f)(message&, const uint8_t *, uint32_t len) const;

	static decode_payload_f payloadDecode[16];
	static const uint32_t crc32c_table[256];
};

MIPI_SYST_NAMESPACE_END

#endif // MIPI_SYST_PRINTER_H_included
