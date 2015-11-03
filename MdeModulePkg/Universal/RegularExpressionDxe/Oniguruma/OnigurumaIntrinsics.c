/** @file

  Provide intrinsics within Oniguruma

  (C) Copyright 2015 Hewlett Packard Enterprise Development LP<BR>

  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License that accompanies this
  distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, WITHOUT
  WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Library/BaseMemoryLib.h>

//
// From CryptoPkg/IntrinsicLib
//

/* Copies bytes between buffers */
#pragma function(memcpy)
void * memcpy (void *dest, const void *src, unsigned int count)
{
  return CopyMem (dest, src, (UINTN)count);
}

/* Sets buffers to a specified character */
#pragma function(memset)
void * memset (void *dest, char ch, unsigned int count)
{
  //
  // NOTE: Here we use one base implementation for memset, instead of the direct
  //       optimized SetMem() wrapper. Because the IntrinsicLib has to be built
  //       without whole program optimization option, and there will be some
  //       potential register usage errors when calling other optimized codes.
  //

  //
  // Declare the local variables that actually move the data elements as
  // volatile to prevent the optimizer from replacing this function with
  // the intrinsic memset()
  //
  volatile UINT8  *Pointer;

  Pointer = (UINT8 *)dest;
  while (count-- != 0) {
    *(Pointer++) = ch;
  }

  return dest;
}
