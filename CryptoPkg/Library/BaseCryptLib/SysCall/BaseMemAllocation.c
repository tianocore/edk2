/** @file
  Base Memory Allocation Routines Wrapper for Crypto library over OpenSSL
  during PEI & DXE phases.

Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <OpenSslSupport.h>

//
// -- Memory-Allocation Routines --
//

/* Allocates memory blocks */
void *malloc (size_t size)
{
  return AllocatePool ((UINTN)size);
}

/* Reallocate memory blocks */
void *realloc (void *ptr, size_t size)
{
  //
  // BUG: hardcode OldSize == size! We have no any knowledge about
  // memory size of original pointer ptr.
  //
  return ReallocatePool ((UINTN)size, (UINTN)size, ptr);
}

/* De-allocates or frees a memory block */
void free (void *ptr)
{
  FreePool (ptr);
}
