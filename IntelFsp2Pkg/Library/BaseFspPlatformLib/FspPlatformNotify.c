/** @file

  Copyright (c) 2014 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

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
extern EFI_GUID  gFspPerformanceDataGuid;

EFI_PEI_PPI_DESCRIPTOR  mPeiPostPciEnumerationPpi = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiPciEnumerationCompleteProtocolGuid,
  NULL
};

EFI_PEI_PPI_DESCRIPTOR  mPeiReadyToBootPpi = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiEventReadyToBootGuid,
  NULL
};

EFI_PEI_PPI_DESCRIPTOR  mPeiEndOfFirmwarePpi = {
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
  IN  UINT32  NotificationCode
  )
{
  EFI_STATUS  Status;

  Status = EFI_SUCCESS;

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

  @param[in] Status return status for the FspSiliconInit.

**/
VOID
EFIAPI
FspSiliconInitDone2 (
  IN EFI_STATUS  Status
  )
{
  volatile EFI_STATUS  FspStatus;

  FspStatus = Status;
  //
  // Convert to FSP EAS defined API return codes
  //
  switch (Status) {
    case EFI_SUCCESS:
    case EFI_INVALID_PARAMETER:
    case EFI_UNSUPPORTED:
    case EFI_DEVICE_ERROR:
      break;
    default:
      DEBUG ((DEBUG_INFO | DEBUG_INIT, "FspSiliconInitApi() Invalid Error - [Status: 0x%08X]\n", Status));
      Status = EFI_DEVICE_ERROR;  // Force to known error.
      break;
  }

  //
  // This is the end of the FspSiliconInit API
  // Give control back to the boot loader
  //
  SetFspMeasurePoint (FSP_PERF_ID_API_FSP_SILICON_INIT_EXIT);
  DEBUG ((DEBUG_INFO | DEBUG_INIT, "FspSiliconInitApi() - [Status: 0x%08X] - End\n", Status));
  PERF_END_EX (&gFspPerformanceDataGuid, "EventRec", NULL, 0, FSP_STATUS_CODE_SILICON_INIT | FSP_STATUS_CODE_COMMON_CODE | FSP_STATUS_CODE_API_EXIT);
  REPORT_STATUS_CODE (EFI_PROGRESS_CODE, FSP_STATUS_CODE_COMMON_CODE | FSP_STATUS_CODE_API_EXIT);
  if (GetFspGlobalDataPointer ()->FspMode == FSP_IN_API_MODE) {
    do {
      SetFspApiReturnStatus (Status);
      Pei2LoaderSwitchStack ();
      if (Status != EFI_SUCCESS) {
        DEBUG ((DEBUG_ERROR, "!!!ERROR: FspSiliconInitApi() - [Status: 0x%08X] - Error encountered during previous API and cannot proceed further\n", Status));
      }
    } while (FspStatus != EFI_SUCCESS);
  }
}

/**
  This function returns control to BootLoader after MemoryInitApi.

  @param[in] Status return status for the MemoryInitApi.
  @param[in,out] HobListPtr The address of HobList pointer, if NULL, will get value from GetFspApiParameter2 ()
**/
VOID
EFIAPI
FspMemoryInitDone2 (
  IN EFI_STATUS  Status,
  IN OUT VOID    **HobListPtr
  )
{
  FSP_GLOBAL_DATA      *FspData;
  volatile EFI_STATUS  FspStatus;

  FspStatus = Status;
  //
  // Calling use FspMemoryInit API
  // Update HOB and return the control directly
  //
  if (HobListPtr == NULL) {
    HobListPtr = (VOID **)GetFspApiParameter2 ();
  }

  if (HobListPtr != NULL) {
    *HobListPtr = (VOID *)GetHobList ();
  }

  //
  // Convert to FSP EAS defined API return codes
  //
  switch (Status) {
    case EFI_SUCCESS:
    case EFI_INVALID_PARAMETER:
    case EFI_UNSUPPORTED:
    case EFI_DEVICE_ERROR:
    case EFI_OUT_OF_RESOURCES:
      break;
    default:
      DEBUG ((DEBUG_INFO | DEBUG_INIT, "FspMemoryInitApi() Invalid Error [Status: 0x%08X]\n", Status));
      Status = EFI_DEVICE_ERROR;  // Force to known error.
      break;
  }

  //
  // This is the end of the FspMemoryInit API
  // Give control back to the boot loader
  //
  DEBUG ((DEBUG_INFO | DEBUG_INIT, "FspMemoryInitApi() - [Status: 0x%08X] - End\n", Status));
  SetFspMeasurePoint (FSP_PERF_ID_API_FSP_MEMORY_INIT_EXIT);
  FspData = GetFspGlobalDataPointer ();
  PERF_START_EX (&gFspPerformanceDataGuid, "EventRec", NULL, (FspData->PerfData[0] & FSP_PERFORMANCE_DATA_TIMER_MASK), FSP_STATUS_CODE_TEMP_RAM_INIT | FSP_STATUS_CODE_COMMON_CODE| FSP_STATUS_CODE_API_ENTRY);
  PERF_END_EX (&gFspPerformanceDataGuid, "EventRec", NULL, (FspData->PerfData[1] & FSP_PERFORMANCE_DATA_TIMER_MASK), FSP_STATUS_CODE_TEMP_RAM_INIT | FSP_STATUS_CODE_COMMON_CODE | FSP_STATUS_CODE_API_EXIT);
  PERF_START_EX (&gFspPerformanceDataGuid, "EventRec", NULL, (FspData->PerfData[2] & FSP_PERFORMANCE_DATA_TIMER_MASK), FSP_STATUS_CODE_MEMORY_INIT | FSP_STATUS_CODE_COMMON_CODE | FSP_STATUS_CODE_API_ENTRY);
  PERF_END_EX (&gFspPerformanceDataGuid, "EventRec", NULL, 0, FSP_STATUS_CODE_MEMORY_INIT | FSP_STATUS_CODE_COMMON_CODE | FSP_STATUS_CODE_API_EXIT);
  REPORT_STATUS_CODE (EFI_PROGRESS_CODE, FSP_STATUS_CODE_MEMORY_INIT | FSP_STATUS_CODE_COMMON_CODE | FSP_STATUS_CODE_API_EXIT);
  if (GetFspGlobalDataPointer ()->FspMode == FSP_IN_API_MODE) {
    do {
      SetFspApiReturnStatus (Status);
      Pei2LoaderSwitchStack ();
      if (Status != EFI_SUCCESS) {
        DEBUG ((DEBUG_ERROR, "!!!ERROR: FspMemoryInitApi() - [Status: 0x%08X] - Error encountered during previous API and cannot proceed further\n", Status));
      }
    } while (FspStatus != EFI_SUCCESS);
  }

  //
  // The TempRamExitApi is called
  //
  if (GetFspApiCallingIndex () == TempRamExitApiIndex) {
    SetPhaseStatusCode (FSP_STATUS_CODE_TEMP_RAM_EXIT);
    SetFspMeasurePoint (FSP_PERF_ID_API_TEMP_RAM_EXIT_ENTRY);
    PERF_START_EX (&gFspPerformanceDataGuid, "EventRec", NULL, 0, FSP_STATUS_CODE_TEMP_RAM_EXIT | FSP_STATUS_CODE_COMMON_CODE | FSP_STATUS_CODE_API_ENTRY);
    REPORT_STATUS_CODE (EFI_PROGRESS_CODE, FSP_STATUS_CODE_TEMP_RAM_EXIT | FSP_STATUS_CODE_COMMON_CODE | FSP_STATUS_CODE_API_ENTRY);
    DEBUG ((DEBUG_INFO | DEBUG_INIT, "TempRamExitApi() - Begin\n"));
  } else {
    SetPhaseStatusCode (FSP_STATUS_CODE_SILICON_INIT);
    SetFspMeasurePoint (FSP_PERF_ID_API_FSP_SILICON_INIT_ENTRY);
    PERF_START_EX (&gFspPerformanceDataGuid, "EventRec", NULL, 0, FSP_STATUS_CODE_SILICON_INIT | FSP_STATUS_CODE_COMMON_CODE | FSP_STATUS_CODE_API_ENTRY);
    REPORT_STATUS_CODE (EFI_PROGRESS_CODE, FSP_STATUS_CODE_SILICON_INIT | FSP_STATUS_CODE_COMMON_CODE | FSP_STATUS_CODE_API_ENTRY);
    DEBUG ((DEBUG_INFO | DEBUG_INIT, "FspSiliconInitApi() - Begin\n"));
  }
}

/**
  This function returns control to BootLoader after TempRamExitApi.

  @param[in] Status return status for the TempRamExitApi.

**/
VOID
EFIAPI
FspTempRamExitDone2 (
  IN EFI_STATUS  Status
  )
{
  //
  volatile EFI_STATUS  FspStatus;

  FspStatus = Status;
  // Convert to FSP EAS defined API return codes
  //
  switch (Status) {
    case EFI_SUCCESS:
    case EFI_INVALID_PARAMETER:
    case EFI_UNSUPPORTED:
    case EFI_DEVICE_ERROR:
      break;
    default:
      DEBUG ((DEBUG_INFO | DEBUG_INIT, "TempRamExitApi() Invalid Error - [Status: 0x%08X]\n", Status));
      Status = EFI_DEVICE_ERROR;  // Force to known error.
      break;
  }

  //
  // This is the end of the TempRamExit API
  // Give control back to the boot loader
  //
  DEBUG ((DEBUG_INFO | DEBUG_INIT, "TempRamExitApi() - [Status: 0x%08X] - End\n", Status));
  SetFspMeasurePoint (FSP_PERF_ID_API_TEMP_RAM_EXIT_EXIT);
  PERF_END_EX (&gFspPerformanceDataGuid, "EventRec", NULL, 0, FSP_STATUS_CODE_TEMP_RAM_EXIT | FSP_STATUS_CODE_COMMON_CODE | FSP_STATUS_CODE_API_EXIT);
  REPORT_STATUS_CODE (EFI_PROGRESS_CODE, FSP_STATUS_CODE_TEMP_RAM_EXIT | FSP_STATUS_CODE_COMMON_CODE | FSP_STATUS_CODE_API_EXIT);
  if (GetFspGlobalDataPointer ()->FspMode == FSP_IN_API_MODE) {
    do {
      SetFspApiReturnStatus (Status);
      Pei2LoaderSwitchStack ();
      if (Status != EFI_SUCCESS) {
        DEBUG ((DEBUG_ERROR, "!!!ERROR: TempRamExitApi() - [Status: 0x%08X] - Error encountered during previous API and cannot proceed further\n", Status));
      }
    } while (FspStatus != EFI_SUCCESS);
  }

  SetPhaseStatusCode (FSP_STATUS_CODE_SILICON_INIT);
  SetFspMeasurePoint (FSP_PERF_ID_API_FSP_SILICON_INIT_ENTRY);
  PERF_START_EX (&gFspPerformanceDataGuid, "EventRec", NULL, 0, FSP_STATUS_CODE_SILICON_INIT | FSP_STATUS_CODE_COMMON_CODE | FSP_STATUS_CODE_API_ENTRY);
  REPORT_STATUS_CODE (EFI_PROGRESS_CODE, FSP_STATUS_CODE_SILICON_INIT | FSP_STATUS_CODE_COMMON_CODE | FSP_STATUS_CODE_API_ENTRY);
  DEBUG ((DEBUG_INFO | DEBUG_INIT, "SiliconInitApi() - Begin\n"));
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
  EFI_STATUS           Status;
  UINT32               NotificationValue;
  UINT32               NotificationCount;
  UINT8                Count;
  volatile EFI_STATUS  FspStatus;

  NotificationCount = 0;
  while (NotificationCount < sizeof (mFspNotifySequence) / sizeof (UINT32)) {
    Count = (UINT8)((NotificationCount << 1) & 0x07);
    SetFspMeasurePoint (FSP_PERF_ID_API_NOTIFY_POST_PCI_ENTRY + Count);

    if (NotificationCount == 0) {
      SetPhaseStatusCode (FSP_STATUS_CODE_POST_PCIE_ENUM_NOTIFICATION);
      PERF_START_EX (&gFspPerformanceDataGuid, "EventRec", NULL, 0, FSP_STATUS_CODE_POST_PCIE_ENUM_NOTIFICATION | FSP_STATUS_CODE_COMMON_CODE | FSP_STATUS_CODE_API_ENTRY);
      REPORT_STATUS_CODE (EFI_PROGRESS_CODE, FSP_STATUS_CODE_POST_PCIE_ENUM_NOTIFICATION | FSP_STATUS_CODE_COMMON_CODE | FSP_STATUS_CODE_API_ENTRY);
    } else if (NotificationCount == 1) {
      SetPhaseStatusCode (FSP_STATUS_CODE_READY_TO_BOOT_NOTIFICATION);
      PERF_START_EX (&gFspPerformanceDataGuid, "EventRec", NULL, 0, FSP_STATUS_CODE_READY_TO_BOOT_NOTIFICATION | FSP_STATUS_CODE_COMMON_CODE | FSP_STATUS_CODE_API_ENTRY);
      REPORT_STATUS_CODE (EFI_PROGRESS_CODE, FSP_STATUS_CODE_READY_TO_BOOT_NOTIFICATION | FSP_STATUS_CODE_COMMON_CODE | FSP_STATUS_CODE_API_ENTRY);
    } else if (NotificationCount == 2) {
      SetPhaseStatusCode (FSP_STATUS_CODE_END_OF_FIRMWARE_NOTIFICATION);
      PERF_START_EX (&gFspPerformanceDataGuid, "EventRec", NULL, 0, FSP_STATUS_CODE_END_OF_FIRMWARE_NOTIFICATION | FSP_STATUS_CODE_COMMON_CODE | FSP_STATUS_CODE_API_ENTRY);
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
      if (!EFI_ERROR (Status)) {
        NotificationCount++;
      }
    }

    DEBUG ((DEBUG_INFO | DEBUG_INIT, "NotifyPhaseApi() - End  [Status: 0x%08X]\n", Status));
    SetFspMeasurePoint (FSP_PERF_ID_API_NOTIFY_POST_PCI_EXIT + Count);

    if ((NotificationCount - 1) == 0) {
      PERF_END_EX (&gFspPerformanceDataGuid, "EventRec", NULL, 0, FSP_STATUS_CODE_POST_PCIE_ENUM_NOTIFICATION | FSP_STATUS_CODE_COMMON_CODE | FSP_STATUS_CODE_API_EXIT);
      REPORT_STATUS_CODE (EFI_PROGRESS_CODE, FSP_STATUS_CODE_POST_PCIE_ENUM_NOTIFICATION | FSP_STATUS_CODE_COMMON_CODE | FSP_STATUS_CODE_API_EXIT);
    } else if ((NotificationCount - 1) == 1) {
      PERF_END_EX (&gFspPerformanceDataGuid, "EventRec", NULL, 0, FSP_STATUS_CODE_READY_TO_BOOT_NOTIFICATION | FSP_STATUS_CODE_COMMON_CODE | FSP_STATUS_CODE_API_EXIT);
      REPORT_STATUS_CODE (EFI_PROGRESS_CODE, FSP_STATUS_CODE_READY_TO_BOOT_NOTIFICATION | FSP_STATUS_CODE_COMMON_CODE | FSP_STATUS_CODE_API_EXIT);
    } else if ((NotificationCount - 1) == 2) {
      PERF_END_EX (&gFspPerformanceDataGuid, "EventRec", NULL, 0, FSP_STATUS_CODE_END_OF_FIRMWARE_NOTIFICATION | FSP_STATUS_CODE_COMMON_CODE | FSP_STATUS_CODE_API_EXIT);
      REPORT_STATUS_CODE (EFI_PROGRESS_CODE, FSP_STATUS_CODE_END_OF_FIRMWARE_NOTIFICATION | FSP_STATUS_CODE_COMMON_CODE | FSP_STATUS_CODE_API_EXIT);
    }

    if (GetFspGlobalDataPointer ()->FspMode == FSP_IN_API_MODE) {
      FspStatus = Status;
      do {
        SetFspApiReturnStatus (Status);
        Pei2LoaderSwitchStack ();
        if (Status != EFI_SUCCESS) {
          DEBUG ((DEBUG_ERROR, "!!!ERROR: NotifyPhaseApi() [Phase: %08X] - Failed - [Status: 0x%08X]\n", NotificationValue, Status));
        }
      } while (FspStatus != EFI_SUCCESS);
    }
  }

  //
  // Control goes back to the PEI Core and it dispatches further PEIMs.
  // DXEIPL is the final one to transfer control back to the boot loader.
  //
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
  FspSiliconInitDone2 (EFI_SUCCESS);
}

/**
  This function returns control to BootLoader after MemoryInitApi.

  @param[in,out] HobListPtr The address of HobList pointer.
**/
VOID
EFIAPI
FspMemoryInitDone (
  IN OUT VOID  **HobListPtr
  )
{
  FspMemoryInitDone2 (EFI_SUCCESS, HobListPtr);
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
  FspTempRamExitDone2 (EFI_SUCCESS);
}
