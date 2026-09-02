/** @file
  Capsule library runtime support.

  Copyright (c) 2016 - 2024, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2024, Ampere Computing LLC. All rights reserved.<BR>
  Copyright (c) 2026, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>

#include <Guid/CapsuleReport.h>
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

#include "DxeCapsuleRuntime.h"

STATIC EFI_EVENT  mDxeRuntimeCapsuleLibEndOfDxeEvent             = NULL;
STATIC EFI_EVENT  mDxeRuntimeCapsuleLibVirtualAddressChangeEvent = NULL;
STATIC EFI_EVENT  mDxeRuntimeCapsuleLibSystemResourceTableEvent  = NULL;
STATIC EFI_EVENT  mDxeRuntimeCapsuleLibExitBootServiceEvent      = NULL;

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
  EFI_STATUS                 Status;
  UINTN                      Idx;
  UINTN                      RtFmpIdx;
  UINTN                      NumberOfHandles;
  EFI_HANDLE                 *HandleBuffer;
  EFI_LOADED_IMAGE_PROTOCOL  *LoadedImage;
  CHAR16                     *DevicePathStr;
  UINTN                      MaxImageInfoSize;
  UINTN                      MaxCapsuleResultVariableSize;
  UINTN                      ImageInfoSize;
  UINTN                      CapsuleResultVariableSize;

  HandleBuffer                 = NULL;
  DevicePathStr                = NULL;
  MaxImageInfoSize             = 0;
  MaxCapsuleResultVariableSize = 0;
  RtFmpIdx                     = 0;

  if (!FeaturePcdGet (PcdSupportProcessCapsuleAtRuntime)) {
    return;
  }

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
    Status = EFI_SUCCESS;
    DEBUG ((DEBUG_INFO, "%a: No Runtime FmpDxe is found!\n", __func__));
    goto ErrorHandler;
  }

  mRuntimeMatchedHandleBuffer = AllocateRuntimeZeroPool (mRuntimeFmpCount * sizeof (EFI_HANDLE));
  if (mRuntimeMatchedHandleBuffer == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    DEBUG ((DEBUG_ERROR, "%a: Failed to allocate Runtime Matched Handle Buffer...\n", __func__));
    goto ErrorHandler;
  }

  mRuntimeMatchedResetRequiredBuffer = AllocateRuntimeZeroPool (mRuntimeFmpCount * sizeof (BOOLEAN));
  if (mRuntimeMatchedResetRequiredBuffer == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    DEBUG ((DEBUG_ERROR, "%a: Failed to allocate Runtime Matched Reset Required Buffer...\n", __func__));
    goto ErrorHandler;
  }

  mRuntimeFmpList = AllocateRuntimeZeroPool (mRuntimeFmpCount * sizeof (RUNTIME_FIRMWARE_MANAGEMENT_INFO));
  if (mRuntimeFmpList == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    DEBUG ((DEBUG_ERROR, "%a: Failed to allocate Runtime FmpDxe list...\n", __func__));
    goto ErrorHandler;
  }

  for (Idx = 0; Idx < NumberOfHandles; Idx++) {
    if (HandleBuffer[Idx] == NULL) {
      continue;
    }

    Status = gBS->HandleProtocol (
                    HandleBuffer[Idx],
                    &gEfiFirmwareManagementProtocolGuid,
                    (VOID **)&mRuntimeFmpList[RtFmpIdx].Fmp
                    );
    if (EFI_ERROR (Status)) {
      goto ErrorHandler;
    }

    ImageInfoSize = 0;
    Status        = (mRuntimeFmpList[RtFmpIdx].Fmp)->GetImageInfo (
                                                       mRuntimeFmpList[RtFmpIdx].Fmp,
                                                       &ImageInfoSize,
                                                       NULL,
                                                       NULL,
                                                       NULL,
                                                       NULL,
                                                       NULL,
                                                       NULL
                                                       );
    if (Status == EFI_BUFFER_TOO_SMALL) {
      MaxImageInfoSize = MAX (MaxImageInfoSize, ImageInfoSize);
    }

    Status = gBS->HandleProtocol (
                    HandleBuffer[Idx],
                    &gEdkiiFirmwareManagementProgressProtocolGuid,
                    (VOID **)&mRuntimeFmpList[RtFmpIdx].FmpProgress
                    );
    if (EFI_ERROR (Status)) {
      goto ErrorHandler;
    }

    Status = gBS->HandleProtocol (
                    HandleBuffer[Idx],
                    &gEfiDevicePathProtocolGuid,
                    (VOID **)&mRuntimeFmpList[RtFmpIdx].FmpDevicePath
                    );
    if (EFI_ERROR (Status)) {
      if (Status != EFI_UNSUPPORTED) {
        goto ErrorHandler;
      }

      mRuntimeFmpList[RtFmpIdx].FmpDevicePath  = NULL;
      mRuntimeFmpList[RtFmpIdx].DevicePathSize = sizeof (CHAR16);
      mRuntimeFmpList[RtFmpIdx].DevicePathStr  = NULL;
    } else {
      DevicePathStr = ConvertDevicePathToText (mRuntimeFmpList[RtFmpIdx].FmpDevicePath, FALSE, FALSE);
      if (DevicePathStr != NULL) {
        mRuntimeFmpList[RtFmpIdx].DevicePathSize = StrSize (DevicePathStr);
        mRuntimeFmpList[RtFmpIdx].DevicePathStr  = AllocateRuntimeZeroPool (
                                                     mRuntimeFmpList[RtFmpIdx].DevicePathSize
                                                     );
        if (mRuntimeFmpList[RtFmpIdx].DevicePathStr == NULL) {
          Status = EFI_OUT_OF_RESOURCES;
          FreePool (DevicePathStr);
          goto ErrorHandler;
        }

        CopyMem (
          mRuntimeFmpList[RtFmpIdx].DevicePathStr,
          DevicePathStr,
          mRuntimeFmpList[RtFmpIdx].DevicePathSize
          );

        FreePool (DevicePathStr);
        DevicePathStr = NULL;
      } else {
        mRuntimeFmpList[RtFmpIdx].DevicePathSize = sizeof (CHAR16);
        mRuntimeFmpList[RtFmpIdx].DevicePathStr  = NULL;
      }
    }

    CapsuleResultVariableSize =
      sizeof (EFI_CAPSULE_RESULT_VARIABLE_HEADER) +
      sizeof (EFI_CAPSULE_RESULT_VARIABLE_FMP) +
      mRuntimeFmpList[RtFmpIdx].DevicePathSize;

    MaxCapsuleResultVariableSize = MAX (MaxCapsuleResultVariableSize, CapsuleResultVariableSize);

    mRuntimeFmpList[RtFmpIdx].Handle = HandleBuffer[Idx];
  }

  mRuntimeImageInfoBuffer = AllocateRuntimeZeroPool (MaxImageInfoSize);
  if (mRuntimeImageInfoBuffer == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ErrorHandler;
  }

  mRuntimeCapsuleResultVariable = AllocateRuntimeZeroPool (MaxCapsuleResultVariableSize);
  if (mRuntimeCapsuleResultVariable == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ErrorHandler;
  }

  Status = EFI_SUCCESS;

ErrorHandler:
  if (HandleBuffer != NULL) {
    FreePool (HandleBuffer);
  }

  if (!EFI_ERROR (Status)) {
    return;
  }

  if (mRuntimeImageInfoBuffer != NULL) {
    FreePool (mRuntimeImageInfoBuffer);
    mRuntimeImageInfoBuffer = NULL;
  }

  if (mRuntimeFmpList != NULL) {
    for (Idx = 0; Idx < RtFmpIdx; Idx++) {
      if (mRuntimeFmpList[Idx].DevicePathStr != NULL) {
        FreePool (mRuntimeFmpList[Idx].DevicePathStr);
      }
    }

    FreePool (mRuntimeFmpList);
    mRuntimeFmpList  = NULL;
    mRuntimeFmpCount = 0;
  }

  if (mRuntimeMatchedHandleBuffer != NULL) {
    FreePool (mRuntimeMatchedHandleBuffer);
    mRuntimeMatchedHandleBuffer = NULL;
  }

  if (mRuntimeMatchedResetRequiredBuffer != NULL) {
    FreePool (mRuntimeMatchedResetRequiredBuffer);
    mRuntimeMatchedResetRequiredBuffer = NULL;
  }
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
    gRT->ConvertPointer (0x00, (VOID **)&mRuntimeFmpList[Idx].Fmp);
    gRT->ConvertPointer (0x00, (VOID **)&mRuntimeFmpList[Idx].FmpProgress);
    gRT->ConvertPointer (0x00, (VOID **)&mRuntimeFmpList[Idx].FmpDevicePath);
    gRT->ConvertPointer (0x00, (VOID **)&mRuntimeFmpList[Idx].DevicePathStr);
  }

  gRT->ConvertPointer (0x00, (VOID **)&mRuntimeFmpList);
  gRT->ConvertPointer (0x00, (VOID **)&mRuntimeImageInfoBuffer);
  gRT->ConvertPointer (0x00, (VOID **)&mRuntimeCapsuleResultVariable);
  gRT->ConvertPointer (0x00, (VOID **)&mRuntimeMatchedHandleBuffer);
  gRT->ConvertPointer (0x00, (VOID **)&mRuntimeMatchedResetRequiredBuffer);
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
