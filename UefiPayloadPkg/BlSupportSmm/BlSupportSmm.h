/** @file
  The header file of bootloader support SMM.

  Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#ifndef BL_SUPPORT_SMM_H_
#define BL_SUPPORT_SMM_H_

#include <PiDxe.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/HobLib.h>
#include <Library/MtrrLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/SmmServicesTableLib.h>
#include <Library/PciLib.h>
#include <Protocol/SmmSwDispatch2.h>
#include <Protocol/SmmAccess2.h>
#include <protocol/MpService.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Register/Intel/ArchitecturalMsr.h>
#include <Guid/SmmRegisterInfoGuid.h>
#include <Guid/SmmS3CommunicationInfoGuid.h>
#include <Guid/SmramMemoryReserve.h>

#define  EFI_MSR_SMRR_MASK              0xFFFFF000
#define  MSR_SMM_FEATURE_CONTROL        0x4E0
#define  SMRAM_SAVE_STATE_MAP_OFFSET    0xFC00  /// Save state offset from SMBASE

typedef struct {
  UINT32  Base;
  UINT32  Mask;
} SMRR_BASE_MASK;

#endif

