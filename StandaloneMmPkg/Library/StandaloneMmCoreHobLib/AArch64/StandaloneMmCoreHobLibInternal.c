/** @file
  HOB Library implementation for Standalone MM Core.

Copyright (c) 2006 - 2014, Intel Corporation. All rights reserved.<BR>
Copyright (c) 2017 - 2018, ARM Limited. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiMm.h>

#include <Library/HobLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>

#include <Guid/MemoryAllocationHob.h>

//
// Cache copy of HobList pointer.
//
extern VOID *gHobList;

EFI_HOB_HANDOFF_INFO_TABLE*
HobConstructor (
  IN VOID   *EfiMemoryBegin,
  IN UINTN  EfiMemoryLength,
  IN VOID   *EfiFreeMemoryBottom,
  IN VOID   *EfiFreeMemoryTop
  )
{
  EFI_HOB_HANDOFF_INFO_TABLE  *Hob;
  EFI_HOB_GENERIC_HEADER      *HobEnd;

  Hob    = EfiFreeMemoryBottom;
  HobEnd = (EFI_HOB_GENERIC_HEADER *)(Hob+1);

  Hob->Header.HobType     = EFI_HOB_TYPE_HANDOFF;
  Hob->Header.HobLength   = sizeof (EFI_HOB_HANDOFF_INFO_TABLE);
  Hob->Header.Reserved    = 0;

  HobEnd->HobType     = EFI_HOB_TYPE_END_OF_HOB_LIST;
  HobEnd->HobLength   = sizeof (EFI_HOB_GENERIC_HEADER);
  HobEnd->Reserved    = 0;

  Hob->Version             = EFI_HOB_HANDOFF_TABLE_VERSION;
  Hob->BootMode            = BOOT_WITH_FULL_CONFIGURATION;

  Hob->EfiMemoryTop        = (UINTN)EfiMemoryBegin + EfiMemoryLength;
  Hob->EfiMemoryBottom     = (UINTN)EfiMemoryBegin;
  Hob->EfiFreeMemoryTop    = (UINTN)EfiFreeMemoryTop;
  Hob->EfiFreeMemoryBottom = (EFI_PHYSICAL_ADDRESS)(UINTN)(HobEnd+1);
  Hob->EfiEndOfHobList     = (EFI_PHYSICAL_ADDRESS)(UINTN)HobEnd;

  gHobList = Hob;

  return Hob;
}
