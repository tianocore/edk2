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

#include <Guid/ArmFfaRxTxBufferInfo.h>

#include "ArmFfaCommon.h"
#include "ArmFfaRxTxMap.h"

/**
  Update Rx/TX buffer information.

  @param  BufferInfo            Rx/Tx buffer information.

**/
STATIC
VOID
EFIAPI
UpdateRxTxBufferInfo (
  OUT ARM_FFA_RX_TX_BUFFER_INFO  *BufferInfo
  )
{
  BufferInfo->TxBufferAddr = (VOID *)(UINTN)PcdGet64 (PcdFfaTxBuffer);
  BufferInfo->TxBufferSize = PcdGet64 (PcdFfaTxRxPageCount) * EFI_PAGE_SIZE;
  BufferInfo->RxBufferAddr = (VOID *)(UINTN)PcdGet64 (PcdFfaRxBuffer);
  BufferInfo->RxBufferSize = PcdGet64 (PcdFfaTxRxPageCount) * EFI_PAGE_SIZE;
}

/**
  Notification service to be called when gEfiPeiMemoryDiscoveredPpiGuid is installed.
  This function change reamp Rx/Tx buffer with permanent memory from
  temporary Rx/Tx buffer.

  Since, the Rx/Tx buffer is chanaged after gEfiPeiMemoryDiscoveredPpiGuid is installed,
  the Rx/Tx buffer should be gotten in each PEIM entrypoint
  via "ArmFfaGetRxTxBuffers()" for PEIM registered as shadow and
  call that function always then, it always gets proper Rx/Tx buffer.

  @param  PeiServices                 Indirect reference to the PEI Services Table.
  @param  NotifyDescriptor            Address of the notification descriptor data structure.
                                      Type EFI_PEI_NOTIFY_DESCRIPTOR is defined above.
  @param  Ppi                         Address of the PPI that was installed.

  @retval EFI_STATUS                  This function will install a PPI to PPI database.
                                      The status code will be the code for (*PeiServices)->InstallPpi.

**/
STATIC
EFI_STATUS
EFIAPI
PeiServicesMemoryDiscoveredNotifyCallback (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  )
{
  EFI_STATUS                 Status;
  EFI_HOB_GUID_TYPE          *RxTxBufferHob;
  ARM_FFA_RX_TX_BUFFER_INFO  *BufferInfo;

  RxTxBufferHob = GetFirstGuidHob (&gArmFfaRxTxBufferInfoGuid);
  ASSERT (RxTxBufferHob != NULL);
  BufferInfo = GET_GUID_HOB_DATA (RxTxBufferHob);

  /*
   * Temporary memory doesn't need to be free.
   * otherwise PEI memory manager using permanent memory will be confused.
   */
  PcdSet64S (PcdFfaTxBuffer, 0x00);
  PcdSet64S (PcdFfaRxBuffer, 0x00);

  Status = ArmFfaLibRxTxUnmap ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = ArmFfaLibRxTxMap ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  UpdateRxTxBufferInfo (BufferInfo);

  return EFI_SUCCESS;
}

STATIC EFI_PEI_NOTIFY_DESCRIPTOR  mNotifyOnPeiMemoryDiscovered = {
  (EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiPeiMemoryDiscoveredPpiGuid,
  PeiServicesMemoryDiscoveredNotifyCallback
};

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
  VOID                       *Dummy;

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

    UpdateRxTxBufferInfo (BufferInfo);

    Status = (*PeiServices)->LocatePpi (
                               PeiServices,
                               &gEfiPeiMemoryDiscoveredPpiGuid,
                               0,
                               NULL,
                               &Dummy
                               );
    if (EFI_ERROR (Status)) {
      Status = (*PeiServices)->NotifyPpi (PeiServices, &mNotifyOnPeiMemoryDiscovered);
      ASSERT_EFI_ERROR (Status);
    }
  }

  return EFI_SUCCESS;
}
