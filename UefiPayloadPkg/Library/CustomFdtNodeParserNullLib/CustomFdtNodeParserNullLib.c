/** @file
  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include <Uefi.h>
#include <PiPei.h>
#include <Pi/PiHob.h>

/**
  Check the HOB and decide if it is need inside Payload
  Payload maintainer may make decision which HOB is need or needn't
  Then add the check logic in the function.
  @param[in] Hob The HOB to check
  @retval TRUE  If HOB is need inside Payload
  @retval FALSE If HOB is needn't inside Payload
**/
BOOLEAN
FitIsHobNeed (
  EFI_PEI_HOB_POINTERS  Hob
  )
{
  return FALSE;
}

/**
  It will Parse FDT -custom node based on information from bootloaders.
  @param[in]  FdtBase The starting memory address of FdtBase.
  @param[in]  HobList The starting memory address of New Hob list.
  @retval HobList   The base address of Hoblist.

**/
UINTN
CustomFdtNodeParser (
  IN VOID  *Fdt,
  IN VOID  *HobList
  )
{
  UINTN  CHobList;

  CHobList = 0;
  if (HobList != NULL) {
    CHobList = (UINTN)HobList;
  }

  return CHobList;
}
