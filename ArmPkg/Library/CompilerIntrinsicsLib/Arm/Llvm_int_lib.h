/** @file

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

#include <Base.h>
#include <Library/DebugLib.h>

#define CHAR_BIT  8

typedef union {
    INT64   all;
    struct {
        UINT32 low;
        INT32  high;
    };
} dwords;

typedef union {
    UINT64  all;
    struct {
        UINT32 low;
        UINT32 high;
    };
} udwords;

// __aeabi_ return values
typedef struct {
  UINT64  Quotent;
  UINT64  Remainder;
} ulldiv_t;

typedef struct {
  INT64   Quotent;
  INT64   Remainder;
} lldiv_t;

typedef struct {
  UINT32  Quotent;
  UINT32  Remainder;
} uidiv_return;

#if __GNUC__
  #define COUNT_LEADING_ZEROS(_a)   __builtin_clz((_a))
  #define COUNT_TRAILING_ZEROS(_a)  __builtin_ctz((_a))
#else
#error COUNT_LEADING_ZEROS() and COUNT_TRAILING_ZEROS() macros not ported to your compiler
#endif
