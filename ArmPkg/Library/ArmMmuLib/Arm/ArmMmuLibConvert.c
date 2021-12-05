/** @file
*  File managing the MMU for ARMv7 architecture
*
*  Copyright (c) 2011-2016, ARM Limited. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#include <Uefi.h>

#include <Library/ArmLib.h>

#include <Chipset/ArmV7.h>

UINT32
ConvertSectionAttributesToPageAttributes (
  IN UINT32   SectionAttributes,
  IN BOOLEAN  IsLargePage
  )
{
  UINT32  PageAttributes;

  PageAttributes  = 0;
  PageAttributes |= TT_DESCRIPTOR_CONVERT_TO_PAGE_CACHE_POLICY (SectionAttributes, IsLargePage);
  PageAttributes |= TT_DESCRIPTOR_CONVERT_TO_PAGE_AP (SectionAttributes);
  PageAttributes |= TT_DESCRIPTOR_CONVERT_TO_PAGE_XN (SectionAttributes, IsLargePage);
  PageAttributes |= TT_DESCRIPTOR_CONVERT_TO_PAGE_NG (SectionAttributes);
  PageAttributes |= TT_DESCRIPTOR_CONVERT_TO_PAGE_S (SectionAttributes);

  return PageAttributes;
}
