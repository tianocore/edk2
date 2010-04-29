/** @file
  Compiler intrinsic for 64-bit compare, ported from LLVM code.

  Copyright (c) 2008-2009, Apple Inc. All rights reserved.<BR>
  
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

// Effects: if rem != 0, *rem = a % b
// Returns: a / b

// Translated from Figure 3-40 of The PowerPC Compiler Writer's Guide

UINT64
__udivmoddi4 (UINT64 a, UINT64 b, UINT64* rem)
{
    const unsigned n_uword_bits = sizeof(UINT32) * CHAR_BIT;
    const unsigned n_udword_bits = sizeof(UINT64) * CHAR_BIT;
    udwords n;
    n.all = a;
    udwords d;
    d.all = b;
    udwords q;
    udwords r;
    unsigned sr;
    
    if (b == 0) {
//      ASSERT (FALSE);
      return 0;
    }
    
    // special cases, X is unknown, K != 0
    if (n.high == 0)
    {
        if (d.high == 0)
        {
            // 0 X
            // ---
            // 0 X
            if (rem)
                *rem = n.low % d.low;
            return n.low / d.low;
        }
        // 0 X
        // ---
        // K X
        if (rem)
            *rem = n.low;
        return 0;
    }
    // n.high != 0
    if (d.low == 0)
    {
        if (d.high == 0)
        {
            // K X
            // ---
            // 0 0
            if (rem)
                *rem = n.high % d.low;
            return n.high / d.low;
        }
        // d.high != 0
        if (n.low == 0)
        {
            // K 0
            // ---
            // K 0
            if (rem)
            {
                r.high = n.high % d.high;
                r.low = 0;
                *rem = r.all;
            }
            return n.high / d.high;
        }
        // K K
        // ---
        // K 0
        if ((d.high & (d.high - 1)) == 0)     // if d is a power of 2
        {
            if (rem)
            {
                r.low = n.low;
                r.high = n.high & (d.high - 1);
                *rem = r.all;
            }
            return n.high >> COUNT_TRAILING_ZEROS(d.high);
        }
        // K K
        // ---
        // K 0
        sr = COUNT_LEADING_ZEROS(d.high) - COUNT_LEADING_ZEROS(n.high);
        // 0 <= sr <= n_uword_bits - 2 or sr large
        if (sr > n_uword_bits - 2)
        {
           if (rem)
                *rem = n.all;
            return 0;
        }
        ++sr;
        // 1 <= sr <= n_uword_bits - 1
        // q.all = n.all << (n_udword_bits - sr);
        q.low = 0;
        q.high = n.low << (n_uword_bits - sr);
        // r.all = n.all >> sr;
        r.high = n.high >> sr;
        r.low = (n.high << (n_uword_bits - sr)) | (n.low >> sr);
    }
    else  // d.low != 0
    {
        if (d.high == 0)
        {
            // K X
            // ---
            // 0 K
            if ((d.low & (d.low - 1)) == 0)     // if d is a power of 2
            {
                if (rem)
                    *rem = n.low & (d.low - 1);
                if (d.low == 1)
                    return n.all;
                unsigned sr = COUNT_TRAILING_ZEROS(d.low);
                q.high = n.high >> sr;
                q.low = (n.high << (n_uword_bits - sr)) | (n.low >> sr);
                return q.all;
            }
            // K X
            // ---
            // 0 K
            sr = 1 + n_uword_bits + COUNT_LEADING_ZEROS(d.low) - COUNT_LEADING_ZEROS(n.high);
            // 2 <= sr <= n_udword_bits - 1
            // q.all = n.all << (n_udword_bits - sr);
            // r.all = n.all >> sr;
            // if (sr == n_uword_bits)
            // {
            //     q.low = 0;
            //     q.high = n.low;
            //     r.high = 0;
            //     r.low = n.high;
            // }
            // else if (sr < n_uword_bits)  // 2 <= sr <= n_uword_bits - 1
            // {
            //     q.low = 0;
            //     q.high = n.low << (n_uword_bits - sr);
            //     r.high = n.high >> sr;
            //     r.low = (n.high << (n_uword_bits - sr)) | (n.low >> sr);
            // }
            // else              // n_uword_bits + 1 <= sr <= n_udword_bits - 1
            // {
            //     q.low = n.low << (n_udword_bits - sr);
            //     q.high = (n.high << (n_udword_bits - sr)) |
            //              (n.low >> (sr - n_uword_bits));
            //     r.high = 0;
            //     r.low = n.high >> (sr - n_uword_bits);
            // }
            q.low =  (n.low << (n_udword_bits - sr)) &
                     ((INT32)(n_uword_bits - sr) >> (n_uword_bits-1));
            q.high = ((n.low << ( n_uword_bits - sr))                       &
                     ((INT32)(sr - n_uword_bits - 1) >> (n_uword_bits-1))) |
                     (((n.high << (n_udword_bits - sr))                     |
                     (n.low >> (sr - n_uword_bits)))                        &
                     ((INT32)(n_uword_bits - sr) >> (n_uword_bits-1)));
            r.high = (n.high >> sr) &
                     ((INT32)(sr - n_uword_bits) >> (n_uword_bits-1));
            r.low =  ((n.high >> (sr - n_uword_bits))                       &
                     ((INT32)(n_uword_bits - sr - 1) >> (n_uword_bits-1))) |
                     (((n.high << (n_uword_bits - sr))                      |
                     (n.low >> sr))                                         &
                     ((INT32)(sr - n_uword_bits) >> (n_uword_bits-1)));
        }
        else
        {
            // K X
            // ---
            // K K
            sr = COUNT_LEADING_ZEROS(d.high) - COUNT_LEADING_ZEROS(n.high);
            // 0 <= sr <= n_uword_bits - 1 or sr large
            if (sr > n_uword_bits - 1)
            {
               if (rem)
                    *rem = n.all;
                return 0;
            }
            ++sr;
            // 1 <= sr <= n_uword_bits
            // q.all = n.all << (n_udword_bits - sr);
            q.low = 0;
            q.high = n.low << (n_uword_bits - sr);
            // r.all = n.all >> sr;
            // if (sr < n_uword_bits)
            // {
            //     r.high = n.high >> sr;
            //     r.low = (n.high << (n_uword_bits - sr)) | (n.low >> sr);
            // }
            // else
            // {
            //     r.high = 0;
            //     r.low = n.high;
            // }
            r.high = (n.high >> sr) &
                     ((INT32)(sr - n_uword_bits) >> (n_uword_bits-1));
            r.low = (n.high << (n_uword_bits - sr)) |
                    ((n.low >> sr)                  &
                    ((INT32)(sr - n_uword_bits) >> (n_uword_bits-1)));
        }
    }
    // Not a special case
    // q and r are initialized with:
    // q.all = n.all << (n_udword_bits - sr);
    // r.all = n.all >> sr;
    // 1 <= sr <= n_udword_bits - 1
    UINT32 carry = 0;
    for (; sr > 0; --sr)
    {
        // r:q = ((r:q)  << 1) | carry
        r.high = (r.high << 1) | (r.low  >> (n_uword_bits - 1));
        r.low  = (r.low  << 1) | (q.high >> (n_uword_bits - 1));
        q.high = (q.high << 1) | (q.low  >> (n_uword_bits - 1));
        q.low  = (q.low  << 1) | carry;
        // carry = 0;
        // if (r.all >= d.all)
        // {
        //      r.all -= d.all;
        //      carry = 1;
        // }
        const INT64 s = (INT64)(d.all - r.all - 1) >> (n_udword_bits - 1);
        carry = s & 1;
        r.all -= d.all & s;
    }
    q.all = (q.all << 1) | carry;
    if (rem)
        *rem = r.all;
    return q.all;
}
