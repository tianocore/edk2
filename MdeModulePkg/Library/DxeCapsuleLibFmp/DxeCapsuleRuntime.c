/** @file
  Capsule library runtime support.

  Copyright (c) 2016 - 2024, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2024, Ampere Computing LLC. All rights reserved.<BR>
  Copyright (c) 2025, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>

#include <Guid/FmpCapsule.h>
#include <Guid/SystemResourceTable.h>
#include <Guid/EventGroup.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>

#include <Protocol/LoadedImage.h>
#include <Protocol/FirmwareManagement.h>

extern EFI_SYSTEM_RESOURCE_TABLE         *mEsrtTable;
extern BOOLEAN                           mDxeCapsuleLibIsExitBootService;
EFI_EVENT                                mDxeRuntimeCapsuleLibEndOfDxeEvent             = NULL;
EFI_EVENT                                mDxeRuntimeCapsuleLibVirtualAddressChangeEvent = NULL;
EFI_EVENT                                mDxeRuntimeCapsuleLibSystemResourceTableEvent  = NULL;
EFI_EVENT                                mDxeRuntimeCapsuleLibExitBootServiceEvent      = NULL;
extern EFI_FIRMWARE_MANAGEMENT_PROTOCOL  **mRuntimeFmpList;
extern UINTN                             mRuntimeFmpCount;

/**
  Get runtime FmpDxe list at EndOfDxe event.

  @param[in] Event      Event whose notification function is being invoked.
  @param[in] Context    The pointer to the notification function's context, which
                        is implementation-dependent.
**/
VOID
EFIAPI
DxeCapsuleLibEndOfDxeEventNotify (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  )
{
  EFI_STATUS                        Status;
  UINTN                             Idx;
  UINTN                             RtFmpIdx = 0;
  UINTN                             NumberOfHandles;
  EFI_HANDLE                        *HandleBuffer;
  EFI_LOADED_IMAGE_PROTOCOL         *LoadedImage;
  EFI_FIRMWARE_MANAGEMENT_PROTOCOL  *Fmp;

  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiFirmwareManagementProtocolGuid,
                  NULL,
                  &NumberOfHandles,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status)) {
    return;
  }

  for (Idx = 0; Idx < NumberOfHandles; Idx++) {
    Status = gBS->HandleProtocol (
                    HandleBuffer[Idx],
                    &gEfiLoadedImageProtocolGuid,
                    (VOID **)&LoadedImage
                    );
    ASSERT_EFI_ERROR (Status);

    if (LoadedImage->ImageCodeType == EfiRuntimeServicesCode) {
      mRuntimeFmpCount++;
    } else {
      HandleBuffer[Idx] = NULL;
    }
  }

  if (mRuntimeFmpCount == 0) {
    DEBUG ((DEBUG_INFO, "%a: No Runtime FmpDxe is found!\n", __func__));
    goto ErrorHandle;
  }

  mRuntimeFmpList = AllocateRuntimePool (mRuntimeFmpCount * sizeof (EFI_FIRMWARE_MANAGEMENT_PROTOCOL *));
  if (mRuntimeFmpList == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to allocate Runtime FmpDxe list...\n", __func__));
    mRuntimeFmpCount = 0;
    goto ErrorHandle;
  }

  for (Idx = 0; Idx < NumberOfHandles; Idx++) {
    if (HandleBuffer[Idx] == NULL) {
      continue;
    }

    Status = gBS->HandleProtocol (
                    HandleBuffer[Idx],
                    &gEfiFirmwareManagementProtocolGuid,
                    (VOID **)&Fmp
                    );
    ASSERT_EFI_ERROR (Status);

    mRuntimeFmpList[RtFmpIdx++] = Fmp;
  }

ErrorHandle:
  FreePool (HandleBuffer);
}

/**
  Convert EsrtTable and Fmp Runtime Dxe driver physical address to virtual address.

  @param[in] Event      Event whose notification function is being invoked.
  @param[in] Context    The pointer to the notification function's context, which
                        is implementation-dependent.
**/
VOID
EFIAPI
DxeCapsuleLibVirtualAddressChangeEvent (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  )
{
  UINTN  Idx;

  gRT->ConvertPointer (EFI_OPTIONAL_PTR, (VOID **)&mEsrtTable);

  for (Idx = 0; Idx < mRuntimeFmpCount; Idx++) {
    gRT->ConvertPointer (0x00, (VOID **)&mRuntimeFmpList[Idx]);
  }

  gRT->ConvertPointer (0x00, (VOID **)&mRuntimeFmpList);
}

/**
  Notify function for event of system resource table installation.

  @param[in]  Event    The Event that is being processed.
  @param[in]  Context  The Event Context.

**/
STATIC
VOID
EFIAPI
DxeCapsuleLibSystemResourceTableInstallEventNotify (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  UINTN                      Index;
  EFI_CONFIGURATION_TABLE    *ConfigEntry;
  EFI_SYSTEM_RESOURCE_TABLE  *EsrtTable;

  //
  // Get Esrt table first
  //
  ConfigEntry = gST->ConfigurationTable;
  for (Index = 0; Index < gST->NumberOfTableEntries; Index++) {
    if (CompareGuid (&gEfiSystemResourceTableGuid, &ConfigEntry->VendorGuid)) {
      break;
    }

    ConfigEntry++;
  }

  //
  // If no Esrt table installed in Configure Table
  //
  if (Index < gST->NumberOfTableEntries) {
    //
    // Free the pool to remove the cached ESRT table.
    //
    if (mEsrtTable != NULL) {
      FreePool ((VOID *)mEsrtTable);
      mEsrtTable = NULL;
    }

    //
    // Search Esrt to check given capsule is qualified
    //
    EsrtTable = (EFI_SYSTEM_RESOURCE_TABLE *)ConfigEntry->VendorTable;

    mEsrtTable = AllocateRuntimeCopyPool (
                   sizeof (EFI_SYSTEM_RESOURCE_TABLE) +
                   EsrtTable->FwResourceCount * sizeof (EFI_SYSTEM_RESOURCE_ENTRY),
                   EsrtTable
                   );
    ASSERT (mEsrtTable != NULL);

    //
    // Set FwResourceCountMax to a sane value.
    //
    mEsrtTable->FwResourceCountMax = mEsrtTable->FwResourceCount;
  }
}

/**
  Notify function for event of exit boot service.

  @param[in]  Event    The Event that is being processed.
  @param[in]  Context  The Event Context.

**/
STATIC
VOID
EFIAPI
DxeCapsuleLibExitBootServiceEventNotify (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  mDxeCapsuleLibIsExitBootService = TRUE;
}

/**
  The constructor function for the file of DxeCapsuleRuntime.

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   The constructor successfully .
**/
EFI_STATUS
EFIAPI
DxeRuntimeCapsuleLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  //
  // Make sure we can handle virtual address changes.
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  DxeCapsuleLibVirtualAddressChangeEvent,
                  NULL,
                  &gEfiEventVirtualAddressChangeGuid,
                  &mDxeRuntimeCapsuleLibVirtualAddressChangeEvent
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Register notify function to cache the FMP capsule GUIDs when system resource table installed.
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  DxeCapsuleLibSystemResourceTableInstallEventNotify,
                  NULL,
                  &gEfiSystemResourceTableGuid,
                  &mDxeRuntimeCapsuleLibSystemResourceTableEvent
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Register notify function to indicate the event is signaled at ExitBootService.
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  DxeCapsuleLibExitBootServiceEventNotify,
                  NULL,
                  &gEfiEventExitBootServicesGuid,
                  &mDxeRuntimeCapsuleLibExitBootServiceEvent
                  );
  ASSERT_EFI_ERROR (Status);

  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  DxeCapsuleLibEndOfDxeEventNotify,
                  NULL,
                  &gEfiEndOfDxeEventGroupGuid,
                  &mDxeRuntimeCapsuleLibEndOfDxeEvent
                  );
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}

/**
  The destructor function for the file of DxeCapsuleRuntime.

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   The destructor completed successfully.
**/
EFI_STATUS
EFIAPI
DxeRuntimeCapsuleLibDestructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  //
  // Close the VirtualAddressChange event.
  //
  Status = gBS->CloseEvent (mDxeRuntimeCapsuleLibVirtualAddressChangeEvent);
  ASSERT_EFI_ERROR (Status);

  //
  // Close the system resource table installed event.
  //
  Status = gBS->CloseEvent (mDxeRuntimeCapsuleLibSystemResourceTableEvent);
  ASSERT_EFI_ERROR (Status);

  //
  // Close the ExitBootService event.
  //
  Status = gBS->CloseEvent (mDxeRuntimeCapsuleLibExitBootServiceEvent);
  ASSERT_EFI_ERROR (Status);

  //
  // Close the EndOfDxe event.
  //
  Status = gBS->CloseEvent (mDxeRuntimeCapsuleLibEndOfDxeEvent);
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}
