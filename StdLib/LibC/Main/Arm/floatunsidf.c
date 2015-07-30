/**
University of Illinois/NCSA
Open Source License

Copyright (c) 2009-2014 by the contributors listed in CREDITS.TXT

All rights reserved.

Developed by:

    LLVM Team

    University of Illinois at Urbana-Champaign

    http://llvm.org

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal with
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimers.

    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimers in the
      documentation and/or other materials provided with the distribution.

    * Neither the names of the LLVM Team, University of Illinois at
      Urbana-Champaign, nor the names of its contributors may be used to
      endorse or promote products derived from this Software without specific
      prior written permission.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS WITH THE
SOFTWARE.
**/

#define DOUBLE_PRECISION
#include "fp_lib.h"

#include "int_lib.h"

ARM_EABI_FNALIAS(ui2d, floatunsidf)

COMPILER_RT_ABI fp_t
__floatunsidf(unsigned int a) {

    const int aWidth = sizeof a * CHAR_BIT;

    // Handle zero as a special case to protect clz
    if (a == 0) return fromRep(0);

    // Exponent of (fp_t)a is the width of abs(a).
    const int exponent = (aWidth - 1) - __builtin_clz(a);
    rep_t result;

    // Shift a into the significand field and clear the implicit bit.
    const int shift = significandBits - exponent;
    result = (rep_t)a << shift ^ implicitBit;

    // Insert the exponent
    result += (rep_t)(exponent + exponentBias) << significandBits;
    return fromRep(result);
}
