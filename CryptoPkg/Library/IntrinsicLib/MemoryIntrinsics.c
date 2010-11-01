/** @file
  Intrinsic Memory Routines Wrapper Implementation for OpenSSL-based
  Cryptographic Library.

Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Base.h>
#include <Library/BaseMemoryLib.h>

/* OpenSSL will use floating point support, and C compiler produces the _fltused
   symbol by default. Simply define this symbol here to satisfy the linker. */
int _fltused = 1;

/* Sets buffers to a specified character */
void * memset (void *dest, char ch, unsigned int count)
{
  return SetMem (dest, (UINTN)count, (UINT8)ch);
}
