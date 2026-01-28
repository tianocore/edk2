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
  EFI_STATUS  Status;
  UINTN       Property1;
  UINTN       Property2;
  UINT16      PartId;
  BOOLEAN     IsFfaSupported;

  Status = ArmFfaLibCommonInit (&PartId, &IsFfaSupported);
  if (EFI_ERROR (Status)) {
    if (!IsFfaSupported) {
      /*
       * FF-A being unsupported doesn't mean a failure of loading the driver/library
       * instance (i.e) ArmPkg's MmCommunication Dxe/PEI Driver uses as well as SpmMm.
       * So If FF-A is not supported the the MmCommunication Dxe/PEI falls back to SpmMm.
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

    DEBUG ((DEBUG_INFO, "%a Rx/Tx buffer isn't supported.\n", __func__));

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
      DEBUG ((DEBUG_INFO, "%a PARTITION_INFO_GET_REGS is available as an alternative to Rx/Tx buffer.\n", __func__));
    }

    return Status;
  }

  return EFI_SUCCESS;
}

/**
  Return partition or VM ID

  @param[out] PartId  The partition or VM ID

  @retval EFI_SUCCESS  Partition ID or VM ID returned
  @retval Others       Errors

**/
EFI_STATUS
EFIAPI
ArmFfaLibGetPartId (
  OUT UINT16  *PartId
  )
{
  return ArmFfaLibPartitionIdGet (PartId);
}

/**
  Check FF-A support or not.

  @retval TRUE                   Supported
  @retval FALSE                  Not supported

**/
BOOLEAN
EFIAPI
IsFfaSupported (
  IN VOID
  )
{
  return ArmFfaLibIsFfaSupported ();
}

/**
  Callback for when Unmap is called to handle any post unmap
  functionality. In SEC, the Rx/Tx buffer HOB needs to be
  invalidated.

**/
VOID
EFIAPI
UnmapCallback (
  IN VOID
  )
{
  EFI_HOB_MEMORY_ALLOCATION  *RxTxBufferAllocationHob;

  /*
   * Rx/Tx buffers are allocated with continuous pages.
   * See ArmFfaLibRxTxMap(). If HOB is not found, the Rx/Tx
   * buffers were not successfully mapped.
   */
  RxTxBufferAllocationHob = FindRxTxBufferAllocationHob (TRUE);
  if (RxTxBufferAllocationHob == NULL) {
    return;
  }

  /*
   * Invalidate the HOB.
   */
  ZeroMem (RxTxBufferAllocationHob, sizeof (ARM_FFA_RX_TX_BUFFER_INFO));
}
