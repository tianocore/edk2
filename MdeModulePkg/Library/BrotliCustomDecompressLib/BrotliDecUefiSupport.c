/** @file
  Implements for functions declared in BrotliDecUefiSupport.h

  Copyright (c) 2020, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <BrotliDecUefiSupport.h>

/**
  Dummy malloc function for compiler.
**/
VOID *
BrDummyMalloc (
  IN size_t  Size
  )
{
  ASSERT (FALSE);
  return NULL;
}

/**
  Dummy free function for compiler.
**/
VOID
BrDummyFree (
  IN VOID  *Ptr
  )
{
  ASSERT (FALSE);
}
