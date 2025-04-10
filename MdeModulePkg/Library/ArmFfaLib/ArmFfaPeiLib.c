/** @file
  Arm Ffa library code for PEI Driver

  Copyright (c) 2024, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

   @par Glossary:
     - FF-A - Firmware Framework for Arm A-profile

   @par Reference(s):
     - Arm Firmware Framework for Arm A-Profile [https://developer.arm.com/documentation/den0077/latest]

**/

#include <Uefi.h>
#include <PiPei.h>
#include <Pi/PiPeiCis.h>
#include <Pi/PiMultiPhase.h>

#include <Library/ArmLib.h>
#include <Library/ArmSmcLib.h>
#include <Library/ArmFfaLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>

#include <IndustryStandard/ArmFfaSvc.h>

#include "ArmFfaCommon.h"
#include "ArmFfaRxTxMap.h"

/**
  ArmFfaLib Constructor.

  @param [in]   FileHandle        File Handle
  @param [in]   PeiServices       Pei Service Table

  @retval EFI_SUCCESS            Success
  @retval Others                 Error

**/
EFI_STATUS
EFIAPI
ArmFfaPeiLibConstructor (
  IN EFI_PEI_FILE_HANDLE     FileHandle,
  IN CONST EFI_PEI_SERVICES  **PeiServices
  )
{
  EFI_STATUS                 Status;
  EFI_HOB_GUID_TYPE          *RxTxBufferHob;
  ARM_FFA_RX_TX_BUFFER_INFO  *BufferInfo;

  Status = ArmFfaLibCommonInit ();
  if (EFI_ERROR (Status)) {
    if (Status == EFI_UNSUPPORTED) {
      /*
       * EFI_UNSUPPORTED return from ArmFfaLibCommonInit() means
       * FF-A interface doesn't support.
       * However, It doesn't make failure of loading driver/library instance
       * (i.e) ArmPkg's MmCommunication Dxe/PEI Driver uses as well as SpmMm.
       * So If FF-A is not supported the the MmCommunication Dxe/PEI falls
       * back to SpmMm.
       * For this case, return EFI_SUCCESS.

       */
      return EFI_SUCCESS;
    }

    return Status;
  }

  RxTxBufferHob = GetFirstGuidHob (&gArmFfaRxTxBufferInfoGuid);
  if (RxTxBufferHob == NULL) {
    Status = ArmFfaLibRxTxMap ();
    if (EFI_ERROR (Status)) {
      return Status;
    }

    BufferInfo = BuildGuidHob (
                   &gArmFfaRxTxBufferInfoGuid,
                   sizeof (ARM_FFA_RX_TX_BUFFER_INFO)
                   );
    if (BufferInfo == NULL) {
      ArmFfaLibRxTxUnmap ();
      return EFI_OUT_OF_RESOURCES;
    }

    BufferInfo->TxBufferAddr = (VOID *)(UINTN)PcdGet64 (PcdFfaTxBuffer);
    BufferInfo->TxBufferSize = PcdGet64 (PcdFfaTxRxPageCount) * EFI_PAGE_SIZE;
    BufferInfo->RxBufferAddr = (VOID *)(UINTN)PcdGet64 (PcdFfaRxBuffer);
    BufferInfo->RxBufferSize = PcdGet64 (PcdFfaTxRxPageCount) * EFI_PAGE_SIZE;
  }

  return EFI_SUCCESS;
}
