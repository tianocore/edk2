/** @file

  Memory Debug Log PEIM

  Copyright (C) 2025, Oracle and/or its affiliates.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/PcdLib.h>
#include <Library/MemDebugLogLib.h>

EFI_STATUS
EFIAPI
MemDebugLogMemAvailCB (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  );

CONST EFI_PEI_NOTIFY_DESCRIPTOR  mMemAvailNotifyList[] = {
  {
    (EFI_PEI_PPI_DESCRIPTOR_NOTIFY_DISPATCH | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
    &gEfiPeiMemoryDiscoveredPpiGuid,
    MemDebugLogMemAvailCB
  }
};

EFI_STATUS
EFIAPI
MemDebugLogMemAvailCB (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  )
{
  UINT32                  MemDebugLogBufPages;
  EFI_PHYSICAL_ADDRESS    MemDebugLogBufAddr;
  EFI_HOB_GUID_TYPE       *GuidHob;
  MEM_DEBUG_LOG_HOB_DATA  *HobData;
  EFI_STATUS              Status;

  MemDebugLogBufPages = MemDebugLogPages ();

  //
  // Buffer size of 0 disables memory debug logging.
  //
  if (!MemDebugLogBufPages) {
    MemDebugLogBufAddr = 0;
    Status             = EFI_SUCCESS;
    goto done;
  }

  //
  // Allocate the memory debug log buffer.
  // NOTE: We allocate the buffer as type EfiRuntimeServicesData
  // as this seems to allow the buffer to persist and be
  // accessible/modifiable throughout runtime (i.e. and avoid
  // being locked down by the MemoryAttributesTable code).
  //
  Status = PeiServicesAllocatePages (
             EfiRuntimeServicesData,
             MemDebugLogBufPages,
             &MemDebugLogBufAddr
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to allocate Memory Debug Log buffer: %r. Logging disabled\n", __func__, Status));
    MemDebugLogBufAddr = 0;
    goto done;
  }

  //
  // Init the debug log buffer
  //
  Status = MemDebugLogInit (MemDebugLogBufAddr, (UINT32)EFI_PAGES_TO_SIZE ((UINTN)MemDebugLogBufPages));
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to init Memory Debug Log buffer: %r. Logging disabled\n", __func__, Status));
    PeiServicesFreePages (MemDebugLogBufAddr, MemDebugLogBufPages);
    MemDebugLogBufAddr = 0;
    goto done;
  }

  //
  // Copy over the messages from the Early Debug Log buffer.
  //
  if (FixedPcdGet32 (PcdOvmfEarlyMemDebugLogBase) != 0) {
    MemDebugLogCopy (MemDebugLogBufAddr, (EFI_PHYSICAL_ADDRESS)(UINTN)FixedPcdGet32 (PcdOvmfEarlyMemDebugLogBase));
  }

done:
  //
  // Zero the early buffer if we successfully
  // created the main memory log buffer.
  //
  if ((Status == EFI_SUCCESS) && (FixedPcdGet32 (PcdOvmfEarlyMemDebugLogBase) != 0)) {
    ZeroMem (
      (VOID *)(UINTN)FixedPcdGet32 (PcdOvmfEarlyMemDebugLogBase),
      (UINT32)FixedPcdGet32 (PcdOvmfEarlyMemDebugLogSize)
      );
  }

  //
  // Create HOB to pass mem debug log buffer addr
  //
  Status = PeiServicesCreateHob (
             EFI_HOB_TYPE_GUID_EXTENSION,
             (UINT16)(sizeof (EFI_HOB_GUID_TYPE) + sizeof (MEM_DEBUG_LOG_HOB_DATA)),
             (VOID **)&GuidHob
             );
  if (EFI_ERROR (Status)) {
    if (MemDebugLogBufAddr) {
      PeiServicesFreePages (MemDebugLogBufAddr, MemDebugLogBufPages);
    }
  } else {
    //
    // Populate the HOB
    //
    CopyGuid (&GuidHob->Name, &gMemDebugLogHobGuid);
    HobData                     = (MEM_DEBUG_LOG_HOB_DATA *)GET_GUID_HOB_DATA (GuidHob);
    HobData->MemDebugLogBufAddr = MemDebugLogBufAddr;
  }

  return Status;
}

EFI_STATUS
EFIAPI
MemDebugLogEntry (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS  Status;

  //
  // Setup callback for memory available notification
  //
  Status = PeiServicesNotifyPpi (mMemAvailNotifyList);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to create MemDebugLog PEIM Callback: %r. Logging disabled\n", __func__, Status));
  }

  return Status;
}
