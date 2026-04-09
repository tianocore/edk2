/** @file
  Main file of the MMC Dxe driver. The driver entrypoint is defined into this file.

  Copyright (c) 2011-2013, ARM Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Protocol/DevicePath.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DevicePathLib.h>

#include "Mmc.h"

EFI_BLOCK_IO_MEDIA  mMmcMediaTemplate = {
  SIGNATURE_32 ('m', 'm', 'c', 'o'), // MediaId
  TRUE,                              // RemovableMedia
  FALSE,                             // MediaPresent
  FALSE,                             // LogicalPartition
  FALSE,                             // ReadOnly
  FALSE,                             // WriteCaching
  512,                               // BlockSize
  4,                                 // IoAlign
  0,                                 // Pad
  0                                  // LastBlock
};

//
// This device structure is serviced as a header.
// Its next field points to the first root bridge device node.
//
LIST_ENTRY  mMmcHostPool;

/**
  Event triggered by the timer to check if any cards have been removed
  or if new ones have been plugged in
**/

EFI_EVENT  gCheckCardsEvent;

/**
  Initialize the MMC Host Pool to support multiple MMC devices
**/
VOID
InitializeMmcHostPool (
  VOID
  )
{
  InitializeListHead (&mMmcHostPool);
}

/**
  Insert a new Mmc Host controller to the pool
**/
VOID
InsertMmcHost (
  IN MMC_HOST_INSTANCE  *MmcHostInstance
  )
{
  InsertTailList (&mMmcHostPool, &(MmcHostInstance->Link));
}

/*
  Remove a new Mmc Host controller to the pool
*/
VOID
RemoveMmcHost (
  IN MMC_HOST_INSTANCE  *MmcHostInstance
  )
{
  RemoveEntryList (&(MmcHostInstance->Link));
}

MMC_HOST_INSTANCE *
CreateMmcHostInstance (
  IN EFI_MMC_HOST_PROTOCOL  *MmcHost
  )
{
  EFI_STATUS                Status;
  MMC_HOST_INSTANCE         *MmcHostInstance;
  EFI_DEVICE_PATH_PROTOCOL  *NewDevicePathNode;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;

  MmcHostInstance = AllocateZeroPool (sizeof (MMC_HOST_INSTANCE));
  if (MmcHostInstance == NULL) {
    return NULL;
  }

  MmcHostInstance->Signature = MMC_HOST_INSTANCE_SIGNATURE;

  MmcHostInstance->State = MmcHwInitializationState;

  MmcHostInstance->BlockIo.Media = AllocateCopyPool (sizeof (EFI_BLOCK_IO_MEDIA), &mMmcMediaTemplate);
  if (MmcHostInstance->BlockIo.Media == NULL) {
    goto FREE_INSTANCE;
  }

  MmcHostInstance->BlockIo.Revision    = EFI_BLOCK_IO_INTERFACE_REVISION;
  MmcHostInstance->BlockIo.Reset       = MmcReset;
  MmcHostInstance->BlockIo.ReadBlocks  = MmcReadBlocks;
  MmcHostInstance->BlockIo.WriteBlocks = MmcWriteBlocks;
  MmcHostInstance->BlockIo.FlushBlocks = MmcFlushBlocks;

  MmcHostInstance->MmcHost = MmcHost;

  // Create DevicePath for the new MMC Host
  Status = MmcHost->BuildDevicePath (MmcHost, &NewDevicePathNode);
  if (EFI_ERROR (Status)) {
    goto FREE_MEDIA;
  }

  DevicePath = (EFI_DEVICE_PATH_PROTOCOL *)AllocatePool (END_DEVICE_PATH_LENGTH);
  if (DevicePath == NULL) {
    goto FREE_MEDIA;
  }

  SetDevicePathEndNode (DevicePath);
  MmcHostInstance->DevicePath = AppendDevicePathNode (DevicePath, NewDevicePathNode);

  // Publish BlockIO protocol interface
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &MmcHostInstance->MmcHandle,
                  &gEfiBlockIoProtocolGuid,
                  &MmcHostInstance->BlockIo,
                  &gEfiDevicePathProtocolGuid,
                  MmcHostInstance->DevicePath,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    goto FREE_DEVICE_PATH;
  }

  return MmcHostInstance;

FREE_DEVICE_PATH:
  FreePool (DevicePath);

FREE_MEDIA:
  FreePool (MmcHostInstance->BlockIo.Media);

FREE_INSTANCE:
  FreePool (MmcHostInstance);

  return NULL;
}

EFI_STATUS
DestroyMmcHostInstance (
  IN MMC_HOST_INSTANCE  *MmcHostInstance
  )
{
  EFI_STATUS  Status;

  // Uninstall Protocol Interfaces
  Status = gBS->UninstallMultipleProtocolInterfaces (
                  MmcHostInstance->MmcHandle,
                  &gEfiBlockIoProtocolGuid,
                  &(MmcHostInstance->BlockIo),
                  &gEfiDevicePathProtocolGuid,
                  MmcHostInstance->DevicePath,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  // Free Memory allocated for the instance
  if (MmcHostInstance->BlockIo.Media) {
    FreePool (MmcHostInstance->BlockIo.Media);
  }

  if (MmcHostInstance->CardInfo.ECSDData) {
    FreePages (MmcHostInstance->CardInfo.ECSDData, EFI_SIZE_TO_PAGES (sizeof (ECSD)));
  }

  FreePool (MmcHostInstance);

  return Status;
}

/**
  This function checks if the controller implement the Mmc Host and the Device Path Protocols
**/
EFI_STATUS
EFIAPI
MmcDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS  Status;
  // EFI_DEVICE_PATH_PROTOCOL        *ParentDevicePath;
  EFI_MMC_HOST_PROTOCOL  *MmcHost;
  EFI_DEV_PATH_PTR       Node;

  //
  // Check RemainingDevicePath validation
  //
  if (RemainingDevicePath != NULL) {
    //
    // Check if RemainingDevicePath is the End of Device Path Node,
    // if yes, go on checking other conditions
    //
    if (!IsDevicePathEnd (RemainingDevicePath)) {
      //
      // If RemainingDevicePath isn't the End of Device Path Node,
      // check its validation
      //
      Node.DevPath = RemainingDevicePath;
      if ((Node.DevPath->Type != HARDWARE_DEVICE_PATH) ||
          (Node.DevPath->SubType != HW_VENDOR_DP) ||
          (DevicePathNodeLength (Node.DevPath) != sizeof (VENDOR_DEVICE_PATH)))
      {
        return EFI_UNSUPPORTED;
      }
    }
  }

  //
  // Check if Mmc Host protocol is installed by platform
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEmbeddedMmcHostProtocolGuid,
                  (VOID **)&MmcHost,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (Status == EFI_ALREADY_STARTED) {
    return EFI_SUCCESS;
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Close the Mmc Host used to perform the supported test
  //
  gBS->CloseProtocol (
         Controller,
         &gEmbeddedMmcHostProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  return EFI_SUCCESS;
}

/**

**/
EFI_STATUS
EFIAPI
MmcDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS             Status;
  MMC_HOST_INSTANCE      *MmcHostInstance;
  EFI_MMC_HOST_PROTOCOL  *MmcHost;

  //
  // Check RemainingDevicePath validation
  //
  if (RemainingDevicePath != NULL) {
    //
    // Check if RemainingDevicePath is the End of Device Path Node,
    // if yes, return EFI_SUCCESS
    //
    if (IsDevicePathEnd (RemainingDevicePath)) {
      return EFI_SUCCESS;
    }
  }

  //
  // Get the Mmc Host protocol
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEmbeddedMmcHostProtocolGuid,
                  (VOID **)&MmcHost,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    if (Status == EFI_ALREADY_STARTED) {
      return EFI_SUCCESS;
    }

    return Status;
  }

  MmcHostInstance = CreateMmcHostInstance (MmcHost);
  if (MmcHostInstance != NULL) {
    // Add the handle to the pool
    InsertMmcHost (MmcHostInstance);

    MmcHostInstance->Initialized = FALSE;

    // Detect card presence now
    CheckCardsCallback (NULL, NULL);
  }

  return EFI_SUCCESS;
}

/**

**/
EFI_STATUS
EFIAPI
MmcDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Controller,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  )
{
  EFI_STATUS         Status = EFI_SUCCESS;
  LIST_ENTRY         *CurrentLink;
  MMC_HOST_INSTANCE  *MmcHostInstance;

  MMC_TRACE ("MmcDriverBindingStop()");

  // For each MMC instance
  CurrentLink = mMmcHostPool.ForwardLink;
  while (CurrentLink != NULL && CurrentLink != &mMmcHostPool && (Status == EFI_SUCCESS)) {
    MmcHostInstance = MMC_HOST_INSTANCE_FROM_LINK (CurrentLink);
    ASSERT (MmcHostInstance != NULL);

    // Close gEmbeddedMmcHostProtocolGuid
    Status = gBS->CloseProtocol (
                    Controller,
                    &gEmbeddedMmcHostProtocolGuid,
                    This->DriverBindingHandle,
                    Controller
                    );

    // Remove MMC Host Instance from the pool
    RemoveMmcHost (MmcHostInstance);

    // Destroy MmcHostInstance
    DestroyMmcHostInstance (MmcHostInstance);
  }

  return Status;
}

VOID
EFIAPI
CheckCardsCallback (
  IN  EFI_EVENT  Event,
  IN  VOID       *Context
  )
{
  LIST_ENTRY         *CurrentLink;
  MMC_HOST_INSTANCE  *MmcHostInstance;
  EFI_STATUS         Status;

  CurrentLink = mMmcHostPool.ForwardLink;
  while (CurrentLink != NULL && CurrentLink != &mMmcHostPool) {
    MmcHostInstance = MMC_HOST_INSTANCE_FROM_LINK (CurrentLink);
    ASSERT (MmcHostInstance != NULL);

    if (MmcHostInstance->MmcHost->IsCardPresent (MmcHostInstance->MmcHost) == !MmcHostInstance->Initialized) {
      MmcHostInstance->State                       = MmcHwInitializationState;
      MmcHostInstance->BlockIo.Media->MediaPresent = !MmcHostInstance->Initialized;
      MmcHostInstance->Initialized                 = !MmcHostInstance->Initialized;

      if (MmcHostInstance->BlockIo.Media->MediaPresent) {
        InitializeMmcDevice (MmcHostInstance);
      }

      Status = gBS->ReinstallProtocolInterface (
                      (MmcHostInstance->MmcHandle),
                      &gEfiBlockIoProtocolGuid,
                      &(MmcHostInstance->BlockIo),
                      &(MmcHostInstance->BlockIo)
                      );

      if (EFI_ERROR (Status)) {
        Print (L"MMC Card: Error reinstalling BlockIo interface\n");
      }
    }

    CurrentLink = CurrentLink->ForwardLink;
  }
}

EFI_DRIVER_BINDING_PROTOCOL  gMmcDriverBinding = {
  MmcDriverBindingSupported,
  MmcDriverBindingStart,
  MmcDriverBindingStop,
  0xa,
  NULL,
  NULL
};

/**

**/
EFI_STATUS
EFIAPI
MmcDxeInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  //
  // Initializes MMC Host pool
  //
  InitializeMmcHostPool ();

  //
  // Install driver model protocol(s).
  //
  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &gMmcDriverBinding,
             ImageHandle,
             &gMmcComponentName,
             &gMmcComponentName2
             );
  ASSERT_EFI_ERROR (Status);

  // Install driver diagnostics
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &ImageHandle,
                  &gEfiDriverDiagnostics2ProtocolGuid,
                  &gMmcDriverDiagnostics2,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  // Use a timer to detect if a card has been plugged in or removed
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL | EVT_TIMER,
                  TPL_CALLBACK,
                  CheckCardsCallback,
                  NULL,
                  &gCheckCardsEvent
                  );
  ASSERT_EFI_ERROR (Status);

  Status = gBS->SetTimer (
                  gCheckCardsEvent,
                  TimerPeriodic,
                  (UINT64)(10*1000*200)
                  );                    // 200 ms
  ASSERT_EFI_ERROR (Status);

  return Status;
}
