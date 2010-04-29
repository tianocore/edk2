/** @file
  Compiler intrinsic to return the number of leading zeros, ported from LLVM code.

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

// Returns: the number of leading 0-bits

// Precondition: a != 0

INT32
__clzsi2(INT32 a)
{
    UINT32 x = (UINT32)a;
    INT32 t = ((x & 0xFFFF0000) == 0) << 4;  // if (x is small) t = 16 else 0
    x >>= 16 - t;      // x = [0 - 0xFFFF]
    UINT32 r = t;       // r = [0, 16]
    // return r + clz(x)
    t = ((x & 0xFF00) == 0) << 3;
    x >>= 8 - t;       // x = [0 - 0xFF]
    r += t;            // r = [0, 8, 16, 24]
    // return r + clz(x)
    t = ((x & 0xF0) == 0) << 2;
    x >>= 4 - t;       // x = [0 - 0xF]
    r += t;            // r = [0, 4, 8, 12, 16, 20, 24, 28]
    // return r + clz(x)
    t = ((x & 0xC) == 0) << 1;
    x >>= 2 - t;       // x = [0 - 3]
    r += t;            // r = [0 - 30] and is even
    // return r + clz(x)
//     switch (x)
//     {
//     case 0:
//         return r + 2;
//     case 1:
//         return r + 1;
//     case 2:
//     case 3:
//         return r;
//     }
    return r + ((2 - x) & -((x & 2) == 0));
}
