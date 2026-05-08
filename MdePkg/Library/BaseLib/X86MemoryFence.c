/** @file
  IA-32/x64 MemoryFence().

  Copyright (c) 2006 - 2008, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

/**
  Used to serialize load and store operations.

  This function impedes compiler reordering of loads and stores across it.

  It does not impede CPU reordering.

  It does not impede compiler reordering of other operations.
**/
VOID
EFIAPI
MemoryFence (
  VOID
  )
{
  _ReadWriteBarrier ();
}
