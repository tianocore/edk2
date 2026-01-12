/** @file
  Arm Ffa library code for PeilessSec

  Copyright (c) 2025, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

   @par Glossary:
     - FF-A - Firmware Framework for Arm A-profile

   @par Reference(s):
     - Arm Firmware Framework for Arm A-Profile [https://developer.arm.com/documentation/den0077/latest]

**/

#include <Uefi.h>
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

#include <Guid/ArmFfaRxTxBufferInfo.h>

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
ArmFfaSecLibConstructor (
  IN VOID
  )
{
  EFI_STATUS                 Status;
  ARM_FFA_RX_TX_BUFFER_INFO  *BufferInfo;
  EFI_HOB_MEMORY_ALLOCATION  *RxTxBufferAllocationHob;
  UINTN                      Property1;
  UINTN                      Property2;

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

  Status = ArmFfaLibRxTxMap ();
  if (EFI_ERROR (Status)) {
    if (Status != EFI_UNSUPPORTED) {
      return Status;
    }

    /*
     * When ARM_FID_FFA_PARTITION_INFO_GET_REGS is supported,
     * Rx/Tx buffer might not be required to request service to
     * secure partition.
     * So, consider EFI_UNSUPPORTED for Rx/Tx buffer as SUCCESS.
     */
    Status = ArmFfaLibGetFeatures (
               ARM_FID_FFA_PARTITION_INFO_GET_REGS,
               0x00,
               &Property1,
               &Property2
               );
    if (!EFI_ERROR (Status)) {
      DEBUG ((DEBUG_INFO, "%a Rx/Tx buffer doesn't support.\n", __func__));
    }

    return Status;
  }

  BufferInfo = BuildGuidHob (
                 &gArmFfaRxTxBufferInfoGuid,
                 sizeof (ARM_FFA_RX_TX_BUFFER_INFO)
                 );
  if (BufferInfo == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to create Rx/Tx Buffer Info Hob\n", __func__));
    ArmFfaLibRxTxUnmap ();
    return EFI_OUT_OF_RESOURCES;
  }

  RxTxBufferAllocationHob = FindRxTxBufferAllocationHob (FALSE);
  ASSERT (RxTxBufferAllocationHob != NULL);

  /*
   * Set then Name with gArmFfaRxTxBufferInfoGuid, so that ArmFfaPeiLib or
   * ArmFfaDxeLib can find the Rx/Tx buffer allocation area.
   */
  CopyGuid (
    &RxTxBufferAllocationHob->AllocDescriptor.Name,
    &gArmFfaRxTxBufferInfoGuid
    );

  UpdateRxTxBufferInfo (BufferInfo);
  BufferInfo->RemapOffset =
    (UINTN)(BufferInfo->TxBufferAddr -
            RxTxBufferAllocationHob->AllocDescriptor.MemoryBaseAddress);
  BufferInfo->RemapRequired = TRUE;

  return EFI_SUCCESS;
}
