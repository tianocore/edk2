/** @file
  Compiler intrinsic for 32-bit unsigned div, ported from LLVM code.

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
  
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
/**
  University of Illinois/NCSA
  Open Source License
  
  Copyright (c) 2003-2008 University of Illinois at Urbana-Champaign.
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


#include "Llvm_int_lib.h"


// Returns: n / d

// Translated from Figure 3-40 of The PowerPC Compiler Writer's Guide

UINT32
__udivsi3(UINT32 n, UINT32 d)
{
    const unsigned n_uword_bits = sizeof(UINT32) * CHAR_BIT;
    UINT32 q;
    UINT32 r;
    unsigned sr;

    // special cases
    if (d == 0) {
//        ASSERT (FALSE);
        return 0; // ?!
        }
    if (n == 0)
        return 0;

    sr = COUNT_LEADING_ZEROS(d) - COUNT_LEADING_ZEROS(n);
    // 0 <= sr <= n_uword_bits - 1 or sr large
    if (sr > n_uword_bits - 1)  // d > r
        return 0;
    if (sr == n_uword_bits - 1)  // d == 1
        return n;
    ++sr;
    // 1 <= sr <= n_uword_bits - 1
    // Not a special case
    q = n << (n_uword_bits - sr);
    r = n >> sr;
    UINT32 carry = 0;
    for (; sr > 0; --sr)
    {
        // r:q = ((r:q)  << 1) | carry
        r = (r << 1) | (q >> (n_uword_bits - 1));
        q = (q << 1) | carry;
        // carry = 0;
        // if (r.all >= d.all)
        // {
        //      r.all -= d.all;
        //      carry = 1;
        // }
        const INT32 s = (INT32)(d - r - 1) >> (n_uword_bits - 1);
        carry = s & 1;
        r -= d & s;
    }
    q = (q << 1) | carry;
    return q;
}
