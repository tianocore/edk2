/** @file
  Arm Ffa library code for Dxe Driver

  Copyright (c) 2024, Arm Limited. All rights reserved.<BR>
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
#include <Library/UefiBootServicesTableLib.h>

#include <IndustryStandard/ArmFfaSvc.h>

#include <Guid/ArmFfaRxTxBufferInfo.h>

#include "ArmFfaCommon.h"
#include "ArmFfaRxTxMap.h"

STATIC EFI_EVENT  mFfaExitBootServiceEvent;

/**
  Unmap RX/TX buffer on Exit Boot Service.

  @param [in]   Event      Registered exit boot service event.
  @param [in]   Context    Additional data.

**/
STATIC
VOID
EFIAPI
ArmFfaLibExitBootServiceEvent (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  ArmFfaLibRxTxUnmap ();
}

/**
  ArmFfaLib Constructor.

  @param [in]   ImageHandle      Image Handle
  @param [in]   SystemTable      System Table

  @retval EFI_SUCCESS            Success
  @retval EFI_INVALID_PARAMETER  Invalid alignment of Rx/Tx buffer
  @retval EFI_OUT_OF_RESOURCES   Out of memory
  @retval Others                 Error

**/
EFI_STATUS
EFIAPI
ArmFfaDxeLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                 Status;
  EFI_HOB_GUID_TYPE          *RxTxBufferHob;
  ARM_FFA_RX_TX_BUFFER_INFO  *BufferInfo;
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

  if (PcdGetBool (PcdFfaExitBootEventRegistered)) {
    return EFI_SUCCESS;
  }

  RxTxBufferHob = GetFirstGuidHob (&gArmFfaRxTxBufferInfoGuid);
  if (RxTxBufferHob != NULL) {
    BufferInfo = GET_GUID_HOB_DATA (RxTxBufferHob);
    if (!BufferInfo->RemapRequired) {
      /*
       * ArmFfaPeiLib handles the Rx/Tx buffer Remap and update the
       * BufferInfo with permanant memory. So use it as it is.
       */
      PcdSet64S (PcdFfaTxBuffer, (UINTN)BufferInfo->TxBufferAddr);
      PcdSet64S (PcdFfaRxBuffer, (UINTN)BufferInfo->RxBufferAddr);
    } else {
      /*
       * SEC maps Rx/Tx buffer, But no PEIM module doesn't use
       * ArmFfaPeiLib. In this case, the BufferInfo includes
       * temporary Rx/Tx buffer address.
       *
       * Therefore, remap Rx/Tx buffer with migrated address again.
       */
      Status = RemapFfaRxTxBuffer (BufferInfo);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "%a: Failed to remap Rx/Tx buffer... Status: %r\n", __func__, Status));
        return Status;
      }

      BufferInfo->RemapRequired = FALSE;
    }
  } else {
    Status = ArmFfaLibRxTxMap ();
    if (Status == EFI_UNSUPPORTED) {
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
    }

    /*
     * When first Dxe instance (library or driver) which uses ArmFfaLib loaded,
     * It already maps Rx/Tx buffer.
     * From Next Dxe instance which uses ArmFfaLib it doesn't need to map Rx/Tx
     * buffer again but it uses the mapped one.
     * ArmFfaLibRxTxMap() returns EFI_ALREADY_STARTED when the Rx/Tx buffers
     * already maps.
     */
    if ((Status != EFI_SUCCESS) && (Status != EFI_ALREADY_STARTED)) {
      DEBUG ((
        DEBUG_ERROR,
        "%a: Failed to Map Rx/Tx buffer. Status: %r\n",
        __func__,
        Status
        ));
      return Status;
    }
  }

  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  ArmFfaLibExitBootServiceEvent,
                  NULL,
                  &gEfiEventExitBootServicesGuid,
                  &mFfaExitBootServiceEvent
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to register ExitBootService event. Status: %r\n",
      __func__,
      Status
      ));
    goto ErrorHandler;
  }

  PcdSetBoolS (PcdFfaExitBootEventRegistered, TRUE);

  return EFI_SUCCESS;

ErrorHandler:
  if (RxTxBufferHob != NULL) {
    ArmFfaLibRxTxUnmap ();
  }

  return Status;
}
