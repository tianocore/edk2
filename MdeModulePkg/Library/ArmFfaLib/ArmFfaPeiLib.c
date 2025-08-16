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
  EFI_HOB_GUID_TYPE          *RxTxBufferHob;
  ARM_FFA_RX_TX_BUFFER_INFO  *BufferInfo;

  RxTxBufferHob = GetFirstGuidHob (&gArmFfaRxTxBufferInfoGuid);
  ASSERT (RxTxBufferHob != NULL);
  BufferInfo = GET_GUID_HOB_DATA (RxTxBufferHob);

  return RemapFfaRxTxBuffer (BufferInfo);
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

  RxTxBufferHob = GetFirstGuidHob (&gArmFfaRxTxBufferInfoGuid);
  if (RxTxBufferHob == NULL) {
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
      ArmFfaLibRxTxUnmap ();
      return EFI_OUT_OF_RESOURCES;
    }

    UpdateRxTxBufferInfo (BufferInfo);

    /*
     * When permanent memory is used, gEfiPeiMemoryDiscoveredPpiGuid
     * is installed. If gEfiPeiMemoryDiscoveredPpiGuid is found,
     * It doesn't need to remap Rx/Tx buffer.
     */
    Status = (*PeiServices)->LocatePpi (
                               PeiServices,
                               &gEfiPeiMemoryDiscoveredPpiGuid,
                               0,
                               NULL,
                               &Dummy
                               );
    BufferInfo->RemapRequired = EFI_ERROR (Status);
  } else {
    BufferInfo = GET_GUID_HOB_DATA (RxTxBufferHob);
  }

  if (BufferInfo->RemapRequired) {
    /*
     * If RxTxBufferAllocationHob can be found with gArmFfaRxTxBufferInfoGuid,
     * This Rx/Tx buffer is mapped by ArmFfaSecLib.
     */
    RxTxBufferAllocationHob = FindRxTxBufferAllocationHob (TRUE);

    /*
     * Below case Rx/Tx buffer mapped by ArmPeiLib but in temporary memory.
     */
    if (RxTxBufferAllocationHob == NULL) {
      RxTxBufferAllocationHob = FindRxTxBufferAllocationHob (FALSE);
      ASSERT (RxTxBufferAllocationHob != NULL);
      BufferInfo->RemapOffset =
        (UINTN)(BufferInfo->TxBufferAddr -
                RxTxBufferAllocationHob->AllocDescriptor.MemoryBaseAddress);

      CopyGuid (
        &RxTxBufferAllocationHob->AllocDescriptor.Name,
        &gArmFfaRxTxBufferInfoGuid
        );
    }

    Status = (*PeiServices)->NotifyPpi (PeiServices, &mNotifyOnPeiMemoryDiscovered);

    /*
     * Failed to register NotifyPpi.
     * In this case, return ERROR to make failure of load for PPI
     * and postpone to remap to other PEIM.
     */
    if (EFI_ERROR (Status)) {
      return Status;
    }

    /*
     * Change RemapRequired to FALSE here to prevent other PEIM from
     * registering notification again.
     */
    BufferInfo->RemapRequired = FALSE;
  }

  return EFI_SUCCESS;
}
