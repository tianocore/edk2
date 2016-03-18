/** @file
  Wrapper Implementation of Helper Routines produced by the C Compiler
  for the OpenSSL-based Cryptographic Library.

Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <OpenSslSupport.h>

//---------------------------------------------------------
// Helper Routines Wrapper
//---------------------------------------------------------

/* Divides a 64-bit signed value with a 64-bit signed value and returns
   a 64-bit signed quotient and reminder */
void _aulldvrm ()
{
  //
  // Null _aulldvrm() Math function implementation to satisfy the linker, since
  // there is no direct functionality logic dependency in present UEFI cases.
  //
  return;
}


/* Converts a scalar double-precision floating point value to a 32-bit integer */
long _ftol2_sse (double dblSource)
{
  //
  // OpenSSL uses this function due to using floating-point inside it.
  // It is only present in 32-bit versions of the compiler.
  // Null _ftol2_sse() function implementation to satisfy the linker, since
  // there is no direct functionality logic dependency in present UEFI cases.
  //
  return 0;
}

/* Converts a scalar double-precision floating point value to a 32-bit integer */
long _ftol2 (double dblSource)
{
  //
  // Null _ftol2() function implementation to satisfy the linker, since
  // there is no direct functionality logic dependency in present UEFI cases.
  //
  return 0;
}
