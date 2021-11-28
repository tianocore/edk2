/** @file

  Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef TDX_STARTUP_INTERNAL_LIB_H_
#define TDX_STARTUP_INTERNAL_LIB_H_

#include <PiPei.h>
#include <Library/BaseLib.h>
#include <Uefi/UefiSpec.h>
#include <Uefi/UefiBaseType.h>
#include <IndustryStandard/Tpm20.h>
#include <IndustryStandard/UefiTcgPlatform.h>
#include <IndustryStandard/IntelTdx.h>

#pragma pack (1)

#define HANDOFF_TABLE_DESC  "TdxTable"
typedef struct {
  UINT8                      TableDescriptionSize;
  UINT8                      TableDescription[sizeof (HANDOFF_TABLE_DESC)];
  UINT64                     NumberOfTables;
  EFI_CONFIGURATION_TABLE    TableEntry[1];
} TDX_HANDOFF_TABLE_POINTERS2;
#pragma pack()

#define LOOPIT(X)  do {\
  volatile int foo = (X); \
  while (foo) ; \
} while(0)

EFI_STATUS
EFIAPI
DxeLoadCore (
  IN INTN  FvInstance
  );

EFI_STATUS
EFIAPI
InitPcdPeim (
  IN INTN  FvInstance
  );

VOID
EFIAPI
TransferHobList (
  IN CONST VOID  *HobStart
  );

/**
 * This function is to find a memory region which is the largest one below 4GB.
 * It will be used as the firmware hoblist.
 *
 * @param VmmHobList        Vmm passed hoblist which constains the memory information.
 * @return EFI_SUCCESS      Successfully construct the firmware hoblist.
 * @return EFI_NOT_FOUND    Cannot find a memory region to be the fw hoblist.
 */
EFI_STATUS
EFIAPI
ConstructFwHobList (
  IN CONST VOID  *VmmHobList
  );

#endif
