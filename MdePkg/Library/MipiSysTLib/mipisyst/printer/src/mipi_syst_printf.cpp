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

#include <sstream>
#include "mipi_syst_printf.h"

MIPI_SYST_NAMESPACE_BEGIN

template<> double bytes2ValLE<double>(const void * data)
{
	union { uint64_t ll; double d; } u = { bytes2ValLE<uint64_t>(data) };
	return u.d;
}

// Parse printf style format string into a sequence of text and format fragments
//
uint32_t
fmtscanner::parse(const std::string& fmt, Args& args)
{
	uint32_t     result(OK);

	const char * p(fmt.c_str());
	const char * eof(p + fmt.size());
	FmtScanState state(START);
	Modifier     modifier(MOD_NONE);
	size_t       fmtPos = 0;           // curremt position in format
	size_t       fragmentStart = 0;    // where current fragment started
	size_t       lastPercentPos = 0;   // where the last % fmt start was seen

	args.clear();

	while (p < eof) {
		switch (state) {
		case START:
			fragmentStart = fmtPos;
			modifier = MOD_NONE;

			state = PLAINTEXT;
			; // deliberate fall through

		case PLAINTEXT:
			if (*p == '%') {
				lastPercentPos = fmtPos;
				state = PERCENT;
			}
			break;

		case PERCENT:
			if (*p == '%') {
				lastPercentPos = 0;   // '%%' is not a format, but the % char
				state = PLAINTEXT;
			} else {
				// arg fmt definition is starting
				//
				if (lastPercentPos != fragmentStart) {
					args.push_back(
						ArgFragment(TEXT, 0,
							fmt.substr(fragmentStart,
								lastPercentPos - fragmentStart)));
				}
				fragmentStart = lastPercentPos;
				state = FLAGS;
				continue;
			}
			break;

		case FLAGS:
			switch (*p) {
			case '-':
			case '+':
			case ' ':
			case '#':
			case '0':
				break;
			default:
				state = WIDTH;
				continue;
				break;
			}
			break;

		case WIDTH:
			if (*p == '*') {
				args.push_back(ArgFragment(STAR, SIZEOF_INT, ""));
				state = PRECISION_DOT;
			} else {
				state = WIDTH_NUMBER;
				continue;
			}
			break;

		case WIDTH_NUMBER:
			if (!isdigit(*p)) {
				state = PRECISION_DOT;
				continue;
			}
			break;

		case PRECISION_DOT:
			if (*p == '.') {
				state = PRECISION_VAL;
			} else {
				state = MODIFIER;
				continue;
			}
			break;

		case PRECISION_VAL:
			if (*p == '*') {
				args.push_back(ArgFragment(STAR, SIZEOF_INT, std::string()));
				state = MODIFIER;
			} else {
				state = PRECISION_NUMBER;
				continue;
			}
			break;

		case PRECISION_NUMBER:
			if (!isdigit(*p)) {
				state = MODIFIER;
				continue;
			}
			break;

		case MODIFIER:
			state = SPECIFIER;

			switch (*p) {
			case 'h':
				modifier = MOD_H;
				state = MODIFIER_HALF;
				break;
			case 'l':
				modifier = MOD_L;
				state = MODIFIER_LONG;
				break;
			case 'j':
				modifier = MOD_J;
				break;
			case 'z':
				modifier = MOD_Z;
				break;
			case 't':
				modifier = MOD_T;
				break;
			case 'L':
				modifier = MOD_LD;
				break;
			default:
				continue;
			}
			break;

		case MODIFIER_HALF:
			state = SPECIFIER;
			if (*p == 'h') {
				modifier = MOD_HH;
				break;
			} else {
				continue;
			}
			break;


		case MODIFIER_LONG:
			state = SPECIFIER;
			if (*p == 'l') {
				modifier = MOD_LL;
				break;
			} else {
				continue;
			}

		case SPECIFIER:
		{
			std::string fragment = fmt.substr(fragmentStart,
				fmtPos - fragmentStart + 1);

			switch (*p) {
			case 'd':
			case 'i':
			case 'u':
			case 'o':
			case 'x':
			case 'X':
				switch (modifier) {
				case MOD_L:
					// convert long to longlong if client is 64bit, but host 32bit
					//
					if ((SIZEOF_LONG == 8) && (sizeof(long) != 8)) {
						size_t pos(fragment.find('l'));
						if (pos != std::string::npos) {
							fragment = fragment.insert(pos, "l");
						}
					}
					args.push_back(ArgFragment(INTEGER, SIZEOF_LONG, fragment));
					break;
				case MOD_LL:
				case MOD_J:
					args.push_back(ArgFragment(INTEGER, SIZEOF_LONGLONG, fragment));
					break;
				case MOD_Z:
					args.push_back(ArgFragment(INTEGER, SIZEOF_SIZEOF, fragment));
					break;
				default:
					args.push_back(ArgFragment(INTEGER, SIZEOF_INT, fragment));
					break;
				}
				state = START;
				break;
			case 'f':
			case 'F':
			case 'e':
			case 'E':
			case 'g':
			case 'G':
			case 'a':
			case 'A':
				args.push_back(ArgFragment(DOUBLE, SIZEOF_LONGLONG, fragment));
				break;
			case 'c':
				args.push_back(ArgFragment(INTEGER, SIZEOF_INT, fragment));
				break;
			case 'p':
				args.push_back(ArgFragment(POINTER, SIZEOF_PTR, fragment));
				break;
			case 's':
				args.push_back(ArgFragment(STRING, SIZEOF_PTR, fragment));
				break;
			case 'n':
				args.push_back(ArgFragment(PERCENT_N, SIZEOF_PTR, fragment));
				break;
			default:  // unsupported format
				args.push_back(ArgFragment(UNKNOWN, 0, fragment));
				result |= UNKNOWN_FORMAT;
				break;
			}
			state = START;
		}
		break;

		default:
			break;
		}
		++p;
		++fmtPos;
	}

	// check if we have a tail plain text string at the end of the format
	// and add it to arg list.
	//
	if (state == PLAINTEXT) {
		args.push_back(ArgFragment(TEXT, 0,
			fmt.substr(fragmentStart, fmtPos - fragmentStart + 1)));
		state = START;
	}

	if (state != START) {
		result |= INCOMPLETE_FORMAT;
	}

	return result;
}


fmtscanner::ArgFragment::ArgFragment(
	ArgType type, size_t size, const std::string& text) :
	_type(type), _size(size), _text(text)
{}



bool msgprintf::format(
	const std::string& fmt,
	const void * args, uint32_t size,
	std::string& result)
{
	std::stringstream sstr;    // where the result is build

	 // Parse the format string for the number of needed parameters
	 //
	uint32_t parseResult;
	if ((parseResult = _scanner.parse(fmt, _args)) != fmtscanner::OK) {
		sstr << "invalid format string '" << fmt << "'";
		result = sstr.str();
		return false;
	}

	const char * bufPtr((const char *)args);     //current byte from arg buffer
	const void * eob((const char *)args + size); // end of buffer address
	std::vector<int> starArgs;                   // '*' args like in "%*.*f"
	bool        success(true);

	// Loop over the argument fragments to format each using local printf
	// and then add it to the result string.
	//
	for (fmtscanner::Args::const_iterator it(_args.begin());
		it != _args.end();
		++it)
	{
		size_t need(it->size());  // number of bytes needed for format
		fmtscanner::ArgType type(it->type());

		switch (type) {
		case fmtscanner::INTEGER:
		case fmtscanner::POINTER:
			if (bufPtr + need <= eob) {
				if (need == sizeof(uint64_t)) {
					uint64_t val(bytes2ValLE<uint64_t>(bufPtr));
					success = hostPrintf(sstr, it->text(), val, starArgs);
				} else {
					uint32_t val(bytes2ValLE<uint32_t>(bufPtr));
					success = hostPrintf(sstr, it->text(), val, starArgs);
				}
			}
			break;

		case fmtscanner::DOUBLE:
			if (bufPtr + need <= eob) {
				double val(bytes2ValLE<double>(bufPtr));
				success = hostPrintf(sstr, it->text(), val, starArgs);
			}
			break;

		case fmtscanner::TEXT:
			// copy text, but "%%" means "%" in printf format
			//
			for (const char *c = it->text().c_str(); *c; ++c) {
			  if ('%' == *c && '%' ==  *(c+1)) continue;
			  sstr << *c;
			}

			break;

		case fmtscanner::STRING:
			// String embedded in message buffer
			// check if it is valid (zero terminated inside message) and
			// then format it into result
			//
		{
			const char * str(bufPtr);

			// Hunt the null byte
			//
			for (need = 1; str <= eob && *str; ++str, ++need)
				;

			if (str > eob) {
				result = " - corrupt printf payload, missing argument string termination";
				return false;
			}

			success = hostPrintf(sstr, it->text(), bufPtr, starArgs);
		}
		break;
		case fmtscanner::PERCENT_N:
			// %n is a nop . There is no usefull place to return the
			// output to.
			//
			break;

		case fmtscanner::STAR:
			// A star argument was seen in fmt, put on the *-stack (can be up to 2)
			//
			if (bufPtr + need <= eob) {
				starArgs.push_back(bytes2ValLE<uint32_t>(bufPtr));
			}
			break;

		default:
			sstr << "- printf internal error, unkown argtype in format '"
				<< fmt << "'";
			success = false;
		}

		// Check for overflow in input buffer
		//
		if ((bufPtr + need) > eob) {
			sstr << "- insufficient argument bytes for format '"
				<< fmt << "'";
			success = false;
		}

		if (!success) {
			result = sstr.str();
			return false;
		}

		if (type != fmtscanner::STAR) {
			starArgs.clear();
		}
		bufPtr += need;
	}

	result = sstr.str();

	return true;
}

MIPI_SYST_NAMESPACE_END