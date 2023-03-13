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

#ifndef MIPI_SYST_PRINTF_H_included
#define MIPI_SYST_PRINTF_H_included

#include <string>
#include <stdint.h>
#include <vector>

#include "mipi_syst_printer.h"

MIPI_SYST_NAMESPACE_BEGIN

/// Finite state machine parser to parse printf format strings used in catalog
/// and printf messages. It detects if we got a string embedded with formats like
/// this:
///            %[flags][width][.precision][length]specifier
///
///    flags:      - + (space) # 0
///    width:      (number)  *
///    precision:  .(number) .*
///    length:     hh h l ll j z t L
///    specifier:  d i u o x X f F e E g G a A c s p n
///
/// details: http://www.cplusplus.com/reference/cstdio/printf/
///
class fmtscanner
{
public:
	fmtscanner(bool is32Bit) :
		SIZEOF_PTR(is32Bit ? 4 : 8),
		SIZEOF_INT(4),
		SIZEOF_LONG(is32Bit ? 4 : 8),
		SIZEOF_SIZEOF(is32Bit ? 4 : 8),
		SIZEOF_LONGLONG(8)
	{}

	~fmtscanner() {}

	/// argument variants
	//
	enum ArgType {
		INTEGER,     ///< 32/64 bit integer
		POINTER,     ///< 32/64 bit pointer
		DOUBLE,      ///< floating point number
		TEXT,        ///< text portion in format
		STRING,      ///< pointer to string %s
		PERCENT_N,   ///< the odd %n format
		STAR,        ///< an int from a %* arg
		UNKNOWN      ///< invalid format
	};

	enum Result {
		OK = 0,
		UNKNOWN_FORMAT = (1 << 1),
		INCOMPLETE_FORMAT = (1 << 2)
	};

	/// Helper class to store information for one printf format fragment
	///
	class ArgFragment {
	public:
		ArgFragment(ArgType type, size_t size, const std::string& text);

		ArgType            type() const { return _type; }
		size_t             size() const { return _size; }
		const std::string& text() const { return _text; }

	private:
		ArgType _type;               ///< arg type
		size_t _size;                ///< arg buffer space usage
		std::string _text;           ///< argument string fragment
	};

	typedef std::vector<ArgFragment> Args;

protected:

	/// Finite state machine states of the argument parser
	///
	enum FmtScanState {
		START,
		PLAINTEXT,
		PERCENT,
		FLAGS,
		WIDTH, WIDTH_NUMBER,
		PRECISION_DOT, PRECISION_VAL, PRECISION_NUMBER,
		MODIFIER,
		MODIFIER_HALF,
		MODIFIER_LONG,
		SPECIFIER
	};

	/// Format modifier types
	///
	enum Modifier {
		MOD_NONE, MOD_HH, MOD_H, MOD_L, MOD_LL, MOD_J, MOD_Z, MOD_T, MOD_LD
	};

public:
	/// Parse a printf argument string into a vector of argument fragments
	/// @eturn OK if no error, or bitmask of errors from Result enumeration.
	///
	uint32_t parse(const std::string& fmt, Args& args);

	const uint32_t SIZEOF_PTR;
	const uint32_t SIZEOF_INT;
	const uint32_t SIZEOF_LONG;
	const uint32_t SIZEOF_SIZEOF;
	const uint32_t SIZEOF_LONGLONG;
};


/// Printf style formatter for events
///
/// This class does printf style formatting using a format string
/// and memory buffer for the printf arguments. The code assumes
/// that all arguments are stored in the buffer without any padding
/// bytes. Strings are embedded as byte sequences into the buffer,
/// including their terminating null byte.
///
/// Here is an example of how the buffer is expected
/// to be setup for the given printf call:
///
///  printf("the %s jumped over the %s, %d times", "cow", "moon", 2);
///
/// Argument buffer layout, following the format string:
///
/// +-----------------------------------------------------------------+
/// | 'c' | 'o' | 'w' | 0 | 'm' | 'o' | 'o' | 'n' | 0 | 2 | 0 | 0 | 0 |
/// +-----------------------------------------------------------------+
///   |<---    P1   --->|    |<---      P2       -->|   |<--- P3 -->|
///
/// It will print "the cow jumped over the moon, 2 times".
///
class msgprintf
{
public:
	msgprintf(bool is32bit) :
		_scanner(is32bit)
	{}

	///  Initialize a printf formatter based on fmt and argument buffer
	///
	/// @param fmt a C99 language compliant printf format string
	/// @param args the argument buffer
	/// @param size number of bytes in argument buffer
	/// @param result ougoing string result or error message
	///
	/// @ return true if formatting succeeded, false if not.
	///
	bool format(const std::string& fmt,
		const void * args, uint32_t size,
		std::string& result);

private:

	fmtscanner _scanner;
	fmtscanner::Args _args;

	msgprintf(const msgprintf&) = delete;
	msgprintf operator=(const msgprintf&) = delete;
};

MIPI_SYST_NAMESPACE_END

#endif // MIPI_SYST_PRINTER_H_included
