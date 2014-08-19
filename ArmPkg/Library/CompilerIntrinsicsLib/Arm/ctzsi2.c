/** @file
  Compiler intrinsic to return the number of trailing zeros, ported from LLVM code.

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

// Returns: the number of trailing 0-bits

// Precondition: a != 0

INT32
__ctzsi2(INT32 a)
{
    UINT32 x = (UINT32)a;
    INT32 t = ((x & 0x0000FFFF) == 0) << 4;  // if (x has no small bits) t = 16 else 0
    x >>= t;           // x = [0 - 0xFFFF] + higher garbage bits
    UINT32 r = t;       // r = [0, 16]
    // return r + ctz(x)
    t = ((x & 0x00FF) == 0) << 3;
    x >>= t;           // x = [0 - 0xFF] + higher garbage bits
    r += t;            // r = [0, 8, 16, 24]
    // return r + ctz(x)
    t = ((x & 0x0F) == 0) << 2;
    x >>= t;           // x = [0 - 0xF] + higher garbage bits
    r += t;            // r = [0, 4, 8, 12, 16, 20, 24, 28]
    // return r + ctz(x)
    t = ((x & 0x3) == 0) << 1;
    x >>= t;
    x &= 3;            // x = [0 - 3]
    r += t;            // r = [0 - 30] and is even
    // return r + ctz(x)
//  The branch-less return statement below is equivalent
//  to the following switch statement:
//     switch (x)
//     {
//     case 0:
//         return r + 2;
//     case 2:
//         return r + 1;
//     case 1:
//     case 3:
//         return r;
//     }
    return r + ((2 - (x >> 1)) & -((x & 1) == 0));
}
