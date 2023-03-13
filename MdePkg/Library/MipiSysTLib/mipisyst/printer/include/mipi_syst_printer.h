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

#ifndef MIPI_SYST_PRINTER_H_included
#define MIPI_SYST_PRINTER_H_included

#include <cstdint>
#include <string>
#include <sstream>
#include <iomanip>


// Is Host Big Endian ?
//
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define MIPI_SYST_BIG_ENDIAN
#endif

#define MIPI_SYST_NAMESPACE ::mipi::syst
#define MIPI_SYST_NAMESPACE_BEGIN  namespace mipi { namespace syst {
#define MIPI_SYST_NAMESPACE_END    }}

#define MIPI_SYST_NAMESPACE_USE using namespace MIPI_SYST_NAMESPACE

MIPI_SYST_NAMESPACE_BEGIN

/// This function template attempts to convert a string into a numeric value.
///
/// Hexadecimal and decimal numbers are supported.
/// Hexadecimal values are expected to be prefixed with "0x"; without
/// this prefix the string is interpreted as a decimal value.
/// @param num_str the string to convert to a number
/// @param num the converted number will be stored in here
/// @return true if the conversion was successful, false otherwise
///
template<class Type>
bool stringToNum(const std::string& num_str, Type& num)
{
    const std::string terminator("@");
    std::string tail;
    std::stringstream sstr;

    sstr << num_str << terminator;
    auto base(num_str.find("0x") ? std::dec : std::hex);

    if (sizeof(Type) == 1) {
        // workaround for byte type assignment to use numbers not characters
        //
        uint32_t temp;
        sstr >> base >> temp >> tail;
        num = (Type)temp;
    } else {
        sstr >> base >> num >> tail;
    }
    if (tail != terminator)
    {
        return false;
    }
    return !sstr.fail();
}

template<class T> std::string toHexValue(T val)
{
	std::stringstream sstr;

	sstr << "0x"
		<< std::hex << std::uppercase
		<< std::setfill('0') << std::setw(sizeof(T) * 2)
		<< val;

	return sstr.str();
}

/// Helper to extract little endian values  of sizeof(T) from a byte array
///
template<class T> T bytes2ValLE(const void * p)
{
	const uint8_t* data((const uint8_t*)p);
	T result(0);

	for (size_t i(0); i < sizeof(T); ++i) {
		result |= (T)(*data++) << (i * 8);
	}
	return result;
}

/// Call the host libc printf for the formatting of one argument
/// to ensure compliance with C99 printf format rules.
/// The function needs to support 3 different call types.
/// One argument may use up to 2 star '*' format options
/// to dynamically change width and precisions.
///
template<class T> bool hostPrintf(
	std::stringstream& dest,
	const std::string& format, T value, const std::vector<int>& starArgs)
{
	int result(0);
	char buf[64 * 1024];

	// Map to native printf API on host system.
	// The "_set_printf_count_output() is a Windows special. Without it
	// a call the msvcrt runtime throws a SEH exception on %n usage as
	// part of security enhancements. We can safely allow %n
	// as the message printf treats it as a nop.
	//
#if defined(_WIN32)
	int(*nativeSprintf)(char * data, size_t buffer, const char * format, ...) = _snprintf;
	int saved_flag(_set_printf_count_output(1));
#else
	int(*nativeSprintf)(char * data, size_t buffer, const char * format, ...) = snprintf;
#endif

	switch (starArgs.size()) {
	case 0:
		result = nativeSprintf(buf, sizeof(buf), format.c_str(), value);
		break;
	case 1:
		result = nativeSprintf(buf, sizeof(buf), format.c_str(), starArgs[0], value);
		break;
	case 2:
		result = nativeSprintf(buf, sizeof(buf), format.c_str(), starArgs[0], starArgs[1], value);
		break;
	default:
		result = -1;  // format error, too many stars seen in argument format
		break;
	}

#if defined(_WIN32)
	_set_printf_count_output(saved_flag);
#endif

	if (result < 0) {
		dest << " - invalid format '" << format << "' in printf";
	}
	else if (result >= sizeof(buf)) {
		dest << " - printf result using format '" << format << "' is too large";
	}
	else {
		dest << buf;
		return true;
	}

	return false;
}

MIPI_SYST_NAMESPACE_END

#endif // MIPI_SYST_PRINTER_H_included
