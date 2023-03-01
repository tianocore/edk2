/** @file
  Function to create page talbe.
  Only create page table for x64, and leave the CreatePageTable empty for Ia32.
  Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Base.h>

/**
  Only create page table for x64, and leave the CreatePageTable empty for Ia32.
  @param[in]      LinearAddress  The start of the linear address range.
  @param[in]      Length         The length of the linear address range.
  @return The page table to be created.
**/
UINTN
CreatePageTable (
  IN UINTN  Address,
  IN UINTN  Length
  )
{
  return 0;
}
