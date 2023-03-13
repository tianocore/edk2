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

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <memory>

#include "mipi_syst_decode.h"
#include "mipi_syst_guid.h"

// print program usage banner
//
static void usage(const std::string& details)
{
	std::cerr << "usage: systprint [-p] [-c <collateralXML>...] [-g short-message-guid] [-o output] [inputfile(s)...]" << std::endl;
	std::cerr << "    -p / --payload_only         only print payload (default is CSV)" << std::endl;
	std::cerr << "    -c / --colateral filename   load given SyS-T collateral XML" << std::endl;
	std::cerr << "    -g / --short_guid guid      guid value for short messages" << std::endl;
	std::cerr << "    -o / --output file          output file name (default stdout)" << std::endl;
	std::cerr << "    inputfile(s)...             file(s) with example library platform output or '-' for stdin" << std::endl;
	std::cerr << std::endl;

	if (!details.empty()) {
		std::cerr << details << std::endl;
	}
	exit(1);
}


// ASCII char -> HEX nibble value, or 0xFF if invalid
//
static const uint8_t hexCharVal[] =
{
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0,       1,    2,    3,    4,    5,    6,    7,    8,    9, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF,   10,   11,   12,   13,   14,   15, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF,   10,   11,   12,   13,   14,   15, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};


// Convert ASCII hex string into byte array, fail if string is not a
// sequence of 0..(2*n) hex characters
//
static bool hex2bin(const std::string& hexStr, std::vector<uint8_t>& dest)
{
	size_t nibbles(hexStr.size());

	if (nibbles % 2) {         // 2 chars needed for 1 value
		return false;
	}
	dest.clear();

	for(auto hexChar(hexStr.c_str()); nibbles ; nibbles -= 2)
	{
		auto high(hexCharVal[(uint8_t)*hexChar++]);
		auto low(hexCharVal[(uint8_t)*hexChar++]);
		if (high == 0xFF || low == 0xFF) {
			return false;
		}
		dest.push_back((high << 4) | low);
	}
	return true;
}


// Scan input file that is assumed to come from the SyS-T library example platform.
// Extract the raw event HEX data lines that look like this
//
//  SYS-T RAW DATA: 4202000B0...
//
// Then parse the hex portion of the line and try to decode it as a SyS-T message.
//
static void readAndPrint(std::istream& is, std::ostream& os,
	bool payload_only,
	const  mipi::syst::decode_context * ctx,
	 const mipi::syst::decoder& decoder)
{
	static const std::string pattern("SYS-T RAW DATA: ");
	std::string line;

	while (getline(is, line)) {
		if (line.compare(0, pattern.size(), pattern)) {
			continue;  // not a raw dump of a SyS-T message
		}

		// sanitize line endings
		//
		while (line.size() &&
		       (line[line.size()-1] == '\r' ||
			line[line.size()-1] == '\n'))
		{
			line.erase(line.size()-1);
		}
		std::vector<uint8_t> bytes;
		if (!hex2bin(line.substr(pattern.size()), bytes)) {
			continue;
		}
		mipi::syst::message msg;

		decoder.decode(msg, bytes, ctx);
		if (payload_only) {
			os << msg.getPayload() << std::endl;
		} else {
			os << msg;
		}
	}
}

/// Simple GUID context wrapper
//
// This class implements a trivial decode context provider by just
// wrapping a guid set from the command line. A real implementation would
// encapsulate transport information here to build client identifying
// information. @see mipi::syst::decode_context
//
class short_guid_context : public mipi::syst::decode_context {
public:
	short_guid_context(const mipi::syst::guid& g) : m_guid(g), m_fakeTS(0) {}

	const mipi::syst::guid& getGuid() const {
		return m_guid;
	}

	uint64_t getTS() const {
		return m_fakeTS++; // replace with transport TS
	}
private:
	mipi::syst::guid m_guid;
	mutable uint64_t m_fakeTS;
};


int main(int argc, char ** argv)
{
	std::vector<std::string> inputs;
	std::string output;
	bool payload_only(false);
	std::shared_ptr<short_guid_context> short_ctx(nullptr);   // guid to use for short messages

	mipi::syst::decoder decoder;

	// parse & check arguments
	//
	for (auto i = 1; i < argc; ++i) {
		const std::string arg(argv[i]);
		if (arg == "-c" || arg == "--collateral") {
			if (++i >= argc) {
				usage("missing collateral file argument");
			}
			try {
				decoder.loadCollateral(argv[i]);
			}
			catch (std::exception& e) {
				std::cerr << "error loading '" << argv[i] << "': " << e.what();
				exit(1);
			}
		} else if (arg == "-p" || arg == "--payload_only") {
			payload_only = true;
		} else if (arg == "-g" || arg == "--short_guid") {
			if (++i >= argc) {
				usage("missing guid argument for -g/--short_guid option");
			}
			try {
				short_ctx = std::make_shared<short_guid_context>(
					mipi::syst::guid(argv[i]));
			} catch (std::exception& e) {
				std::cerr << e.what();
				exit(1);
			}
		} else if (arg == "-o" || arg == "--output") {
			if (++i >= argc) {
				usage("missing filename argument for -o/--output option");
			}
			output = argv[i];
		} else if (arg == "-") {   //stdin
			inputs.push_back(arg);
		} else if (arg[0] == '-') {
			usage(std::string("unknown argument : ") + arg);
		} else {
			inputs.push_back(arg);
		}
	}

	if (inputs.empty()) {
		usage("no input provided");
	}

	// set output destination (default stdout)
	//
	std::ostream * os(&std::cout);
	std::ofstream  ofs;

	if (!output.empty()) {
		ofs.open(output.c_str(), std::ofstream::out);
		if (!ofs.is_open()) {
			std::cerr << "unable to open output file : " <<  output << std::endl;
			exit(1);
		}
		os = &ofs;
	}

	if (!payload_only) {
		*os << mipi::syst::message::csvHeaderString << std::endl;
	}
	for (auto input : inputs) {
		if (input == "-") {
			readAndPrint(std::cin, *os, payload_only, short_ctx.get(), decoder);
		} else {
			std::ifstream ifs(input);
			if (!ifs.is_open()) {
				std::cerr << "unable to open input file : " << input << std::endl;
				exit(1);
			}
			readAndPrint(ifs, *os, payload_only, short_ctx.get(), decoder);
			ifs.close();
		}
	}

	if (ofs.is_open()) {
		ofs.close();
	}

	return 0;
}