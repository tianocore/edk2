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

#ifndef MIPI_SYST_GUID_H_included
#define MIPI_SYST_GUID_H_included

#include <string>
#include <stdexcept>
#include "mipi_syst_printer.h"


MIPI_SYST_NAMESPACE_BEGIN

/// Container for 128 bit wide GUID (see https://www.ietf.org/rfc/rfc4122.txt)
//
class guid {
public:
	union {
		uint8_t b[16];
		uint64_t ll[2];
	} u;

	guid()
	{
		u.ll[0] = u.ll[1] = 0;
	}

	guid(const uint8_t * p)
	{
		u.ll[0] = *(uint64_t*)p;
		u.ll[1] = *(uint64_t*)(p+sizeof(uint64_t));
	}

	guid(const std::string& f)
	{
		if (!parse(f)) {
			throw std::invalid_argument(
				"invalid guid format " +
				f +
				" expected {xxxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx} notation."
			);
		}
	}

	/** Helper to parse a GUID "{...}" string into the binrary representation.
	* @param str GUID ascii string
	* @param guid resulting guid
	* @return true if guid could be parserd
	*/
	bool parse(const std::string& fmt);

	bool operator<(const guid& other) const
	{
		if (u.ll[0] == other.u.ll[0]) {
			return u.ll[1] < other.u.ll[1];
		} else {
			return u.ll[0] < other.u.ll[0];
		}
	}

	guid operator&(const guid& mask) const
	{
		guid result(*this);

		result.u.ll[0] &= mask.u.ll[0];
		result.u.ll[1] &= mask.u.ll[1];

		return result;
	}

	bool operator==(const guid& other) const
	{
		return u.ll[0] == other.u.ll[0] && u.ll[1] == other.u.ll[1];
	}
};

std::ostream& operator<<(std::ostream& os, const guid& guid);

MIPI_SYST_NAMESPACE_END

#endif // MIPI_SYST_COLLATERL_H_included
