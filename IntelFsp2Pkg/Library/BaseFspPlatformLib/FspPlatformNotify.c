/** @file

  Copyright (c) 2014 - 2016, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiPei.h>
#include <Library/PeiServicesLib.h>
#include <Library/PeiServicesTablePointerLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PcdLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/FspSwitchStackLib.h>
#include <Library/FspCommonLib.h>
#include <Guid/EventGroup.h>
#include <FspEas.h>
#include <FspStatusCode.h>
#include <Protocol/PciEnumerationComplete.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/PerformanceLib.h>
extern EFI_GUID gFspPerformanceDataGuid;

EFI_PEI_PPI_DESCRIPTOR      mPeiPostPciEnumerationPpi = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiPciEnumerationCompleteProtocolGuid,
  NULL
};

EFI_PEI_PPI_DESCRIPTOR      mPeiReadyToBootPpi = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiEventReadyToBootGuid,
  NULL
};

EFI_PEI_PPI_DESCRIPTOR      mPeiEndOfFirmwarePpi = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gFspEventEndOfFirmwareGuid,
  NULL
};

UINT32  mFspNotifySequence[] = {
  EnumInitPhaseAfterPciEnumeration,
  EnumInitPhaseReadyToBoot,
  EnumInitPhaseEndOfFirmware
};

/**
  Install FSP notification.

  @param[in] NotificationCode  FSP notification code

  @retval EFI_SUCCESS            Notify FSP successfully
  @retval EFI_INVALID_PARAMETER  NotificationCode is invalid

**/
EFI_STATUS
EFIAPI
FspNotificationHandler (
  IN  UINT32     NotificationCode
  )
{
  EFI_STATUS                Status;

  Status   = EFI_SUCCESS;

  switch (NotificationCode) {
  case EnumInitPhaseAfterPciEnumeration:
    //
    // Do POST PCI initialization if needed
    //
    DEBUG ((DEBUG_INFO | DEBUG_INIT, "FSP Post PCI Enumeration ...\n"));
    PeiServicesInstallPpi (&mPeiPostPciEnumerationPpi);
    break;

  case EnumInitPhaseReadyToBoot:
    //
    // Ready To Boot
    //
    DEBUG ((DEBUG_INFO| DEBUG_INIT, "FSP Ready To Boot ...\n"));
    PeiServicesInstallPpi (&mPeiReadyToBootPpi);
    break;

  case EnumInitPhaseEndOfFirmware:
    //
    // End of Firmware
    //
    DEBUG ((DEBUG_INFO| DEBUG_INIT, "FSP End of Firmware ...\n"));
    PeiServicesInstallPpi (&mPeiEndOfFirmwarePpi);
    break;

  default:
    Status = EFI_INVALID_PARAMETER;
    break;
  }

  return Status;
}

/**
  This function transfer control back to BootLoader after FspSiliconInit.

**/
VOID
EFIAPI
FspSiliconInitDone (
  VOID
  )
{
  //
  // This is the end of the FspSiliconInit API
  // Give control back to the boot loader
  //
  SetFspMeasurePoint (FSP_PERF_ID_API_FSP_SILICON_INIT_EXIT);
  DEBUG ((DEBUG_INFO | DEBUG_INIT, "FspSiliconInitApi() - End\n"));

  PERF_END_EX (&gFspPerformanceDataGuid, "EventRec", NULL, 0, 0x907F);

  REPORT_STATUS_CODE (EFI_PROGRESS_CODE, FSP_STATUS_CODE_COMMON_CODE | FSP_STATUS_CODE_API_EXIT);
  SetFspApiReturnStatus (EFI_SUCCESS);

  Pei2LoaderSwitchStack();

  PERF_START_EX (&gFspPerformanceDataGuid, "EventRec", NULL, 0, 0x6000);
}

/**
  This function returns control to BootLoader after MemoryInitApi.

  @param[in,out] HobListPtr The address of HobList pointer.
**/
VOID
EFIAPI
FspMemoryInitDone (
  IN OUT VOID   **HobListPtr
  )
{
  //
  // Calling use FspMemoryInit API
  // Update HOB and return the control directly
  //
  if (HobListPtr != NULL) {
    *HobListPtr = (VOID *) GetHobList ();
  }

  //
  // This is the end of the FspMemoryInit API
  // Give control back to the boot loader
  //
  SetFspMeasurePoint (FSP_PERF_ID_API_FSP_MEMORY_INIT_EXIT);
  DEBUG ((DEBUG_INFO | DEBUG_INIT, "FspMemoryInitApi() - End\n"));
  REPORT_STATUS_CODE (EFI_PROGRESS_CODE, FSP_STATUS_CODE_MEMORY_INIT | FSP_STATUS_CODE_COMMON_CODE | FSP_STATUS_CODE_API_EXIT);
  SetFspApiReturnStatus (EFI_SUCCESS);
  Pei2LoaderSwitchStack ();

  //
  // The TempRamExitApi is called
  //
  if (GetFspApiCallingIndex () == TempRamExitApiIndex) {
    SetPhaseStatusCode (FSP_STATUS_CODE_TEMP_RAM_EXIT);
    REPORT_STATUS_CODE (EFI_PROGRESS_CODE, FSP_STATUS_CODE_TEMP_RAM_EXIT | FSP_STATUS_CODE_COMMON_CODE | FSP_STATUS_CODE_API_ENTRY);
    SetFspMeasurePoint (FSP_PERF_ID_API_TEMP_RAM_EXIT_ENTRY);
    DEBUG ((DEBUG_INFO | DEBUG_INIT, "TempRamExitApi() - Begin\n"));
  } else {
    SetFspMeasurePoint (FSP_PERF_ID_API_FSP_SILICON_INIT_ENTRY);
    DEBUG ((DEBUG_INFO | DEBUG_INIT, "FspSiliconInitApi() - Begin\n"));
    SetPhaseStatusCode (FSP_STATUS_CODE_SILICON_INIT);
    REPORT_STATUS_CODE (EFI_PROGRESS_CODE, FSP_STATUS_CODE_SILICON_INIT | FSP_STATUS_CODE_COMMON_CODE | FSP_STATUS_CODE_API_ENTRY);
  }
}

/**
  This function returns control to BootLoader after TempRamExitApi.

**/
VOID
EFIAPI
FspTempRamExitDone (
  VOID
  )
{

  //
  // This is the end of the TempRamExit API
  // Give control back to the boot loader
  //
  SetFspMeasurePoint (FSP_PERF_ID_API_TEMP_RAM_EXIT_EXIT);
  DEBUG ((DEBUG_INFO | DEBUG_INIT, "TempRamExitApi() - End\n"));
  REPORT_STATUS_CODE (EFI_PROGRESS_CODE, FSP_STATUS_CODE_TEMP_RAM_EXIT | FSP_STATUS_CODE_COMMON_CODE | FSP_STATUS_CODE_API_EXIT);
  SetFspApiReturnStatus (EFI_SUCCESS);
  Pei2LoaderSwitchStack ();

  SetPhaseStatusCode (FSP_STATUS_CODE_SILICON_INIT);
  SetFspMeasurePoint (FSP_PERF_ID_API_FSP_SILICON_INIT_ENTRY);
  DEBUG ((DEBUG_INFO | DEBUG_INIT, "SiliconInitApi() - Begin\n"));
  REPORT_STATUS_CODE (EFI_PROGRESS_CODE, FSP_STATUS_CODE_SILICON_INIT | FSP_STATUS_CODE_COMMON_CODE | FSP_STATUS_CODE_API_ENTRY);
}

/**
  This function handle NotifyPhase API call from the BootLoader.
  It gives control back to the BootLoader after it is handled. If the
  Notification code is a ReadyToBoot event, this function will return
  and FSP continues the remaining execution until it reaches the DxeIpl.

**/
VOID
FspWaitForNotify (
  VOID
  )
{
  EFI_STATUS                 Status;
  UINT32                     NotificationValue;
  UINT32                     NotificationCount;
  UINT8                      Count;

  NotificationCount = 0;
  while (NotificationCount < sizeof(mFspNotifySequence) / sizeof(UINT32)) {

    Count = (UINT8)((NotificationCount << 1) & 0x07);
    SetFspMeasurePoint (FSP_PERF_ID_API_NOTIFY_POST_PCI_ENTRY + Count);

    if (NotificationCount == 0) {
      SetPhaseStatusCode (FSP_STATUS_CODE_POST_PCIE_ENUM_NOTIFICATION);
      REPORT_STATUS_CODE (EFI_PROGRESS_CODE, FSP_STATUS_CODE_POST_PCIE_ENUM_NOTIFICATION | FSP_STATUS_CODE_COMMON_CODE | FSP_STATUS_CODE_API_ENTRY);
    } else if (NotificationCount == 1) {
      SetPhaseStatusCode (FSP_STATUS_CODE_READY_TO_BOOT_NOTIFICATION);
      REPORT_STATUS_CODE (EFI_PROGRESS_CODE, FSP_STATUS_CODE_READY_TO_BOOT_NOTIFICATION | FSP_STATUS_CODE_COMMON_CODE | FSP_STATUS_CODE_API_ENTRY);
    } else if (NotificationCount == 2) {
      SetPhaseStatusCode (FSP_STATUS_CODE_END_OF_FIRMWARE_NOTIFICATION);
      REPORT_STATUS_CODE (EFI_PROGRESS_CODE, FSP_STATUS_CODE_END_OF_FIRMWARE_NOTIFICATION | FSP_STATUS_CODE_COMMON_CODE | FSP_STATUS_CODE_API_ENTRY);
    }

    NotificationValue = ((NOTIFY_PHASE_PARAMS *)(UINTN)GetFspApiParameter ())->Phase;
    DEBUG ((DEBUG_INFO | DEBUG_INIT, "NotifyPhaseApi() - Begin  [Phase: %08X]\n", NotificationValue));
    if (mFspNotifySequence[NotificationCount] != NotificationValue) {
      //
      // Notify code does not follow the predefined order
      //
      DEBUG ((DEBUG_INFO, "Unsupported FSP Notification Value\n"));
      Status = EFI_UNSUPPORTED;
    } else {
      //
      // Process Notification and Give control back to the boot loader framework caller
      //
      Status = FspNotificationHandler (NotificationValue);
      if (!EFI_ERROR(Status)) {
        NotificationCount++;
      }
    }

    SetFspApiReturnStatus(Status);
    DEBUG ((DEBUG_INFO | DEBUG_INIT, "NotifyPhaseApi() - End  [Status: 0x%08X]\n", Status));

    SetFspMeasurePoint (FSP_PERF_ID_API_NOTIFY_POST_PCI_EXIT + Count);

    if ((NotificationCount - 1) == 0) {
      REPORT_STATUS_CODE (EFI_PROGRESS_CODE, FSP_STATUS_CODE_POST_PCIE_ENUM_NOTIFICATION | FSP_STATUS_CODE_COMMON_CODE | FSP_STATUS_CODE_API_EXIT);
    } else if ((NotificationCount - 1) == 1) {
      REPORT_STATUS_CODE (EFI_PROGRESS_CODE, FSP_STATUS_CODE_READY_TO_BOOT_NOTIFICATION | FSP_STATUS_CODE_COMMON_CODE | FSP_STATUS_CODE_API_EXIT);
    } else if ((NotificationCount - 1) == 2) {
      REPORT_STATUS_CODE (EFI_PROGRESS_CODE, FSP_STATUS_CODE_END_OF_FIRMWARE_NOTIFICATION | FSP_STATUS_CODE_COMMON_CODE | FSP_STATUS_CODE_API_EXIT);
    }
    Pei2LoaderSwitchStack();
  }

  //
  // Control goes back to the PEI Core and it dispatches further PEIMs.
  // DXEIPL is the final one to transfer control back to the boot loader.
  //
}

