/** @file
  The realization of EFI_RAM_DISK_PROTOCOL.

  Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "RamDiskImpl.h"

RAM_DISK_PRIVATE_DATA mRamDiskPrivateDataTemplate = {
  RAM_DISK_PRIVATE_DATA_SIGNATURE,
  NULL
};

MEDIA_RAM_DISK_DEVICE_PATH  mRamDiskDeviceNodeTemplate = {
  {
    MEDIA_DEVICE_PATH,
    MEDIA_RAM_DISK_DP,
    {
      (UINT8) (sizeof (MEDIA_RAM_DISK_DEVICE_PATH)),
      (UINT8) ((sizeof (MEDIA_RAM_DISK_DEVICE_PATH)) >> 8)
    }
  }
};


/**
  Initialize the RAM disk device node.

  @param[in]      PrivateData     Points to RAM disk private data.
  @param[in, out] RamDiskDevNode  Points to the RAM disk device node.

**/
VOID
RamDiskInitDeviceNode (
  IN     RAM_DISK_PRIVATE_DATA         *PrivateData,
  IN OUT MEDIA_RAM_DISK_DEVICE_PATH    *RamDiskDevNode
  )
{
  WriteUnaligned64 (
    (UINT64 *) &(RamDiskDevNode->StartingAddr[0]),
    (UINT64) PrivateData->StartingAddr
    );
  WriteUnaligned64 (
    (UINT64 *) &(RamDiskDevNode->EndingAddr[0]),
    (UINT64) PrivateData->StartingAddr + PrivateData->Size
    );
  CopyGuid (&RamDiskDevNode->TypeGuid, &PrivateData->TypeGuid);
  RamDiskDevNode->Instance = PrivateData->InstanceNumber;
}


/**
  Register a RAM disk with specified address, size and type.

  @param[in]  RamDiskBase    The base address of registered RAM disk.
  @param[in]  RamDiskSize    The size of registered RAM disk.
  @param[in]  RamDiskType    The type of registered RAM disk. The GUID can be
                             any of the values defined in section 9.3.6.9, or a
                             vendor defined GUID.
  @param[in]  ParentDevicePath
                             Pointer to the parent device path. If there is no
                             parent device path then ParentDevicePath is NULL.
  @param[out] DevicePath     On return, points to a pointer to the device path
                             of the RAM disk device.
                             If ParentDevicePath is not NULL, the returned
                             DevicePath is created by appending a RAM disk node
                             to the parent device path. If ParentDevicePath is
                             NULL, the returned DevicePath is a RAM disk device
                             path without appending. This function is
                             responsible for allocating the buffer DevicePath
                             with the boot service AllocatePool().

  @retval EFI_SUCCESS             The RAM disk is registered successfully.
  @retval EFI_INVALID_PARAMETER   DevicePath or RamDiskType is NULL.
                                  RamDiskSize is 0.
  @retval EFI_ALREADY_STARTED     A Device Path Protocol instance to be created
                                  is already present in the handle database.
  @retval EFI_OUT_OF_RESOURCES    The RAM disk register operation fails due to
                                  resource limitation.

**/
EFI_STATUS
EFIAPI
RamDiskRegister (
  IN UINT64                       RamDiskBase,
  IN UINT64                       RamDiskSize,
  IN EFI_GUID                     *RamDiskType,
  IN EFI_DEVICE_PATH              *ParentDevicePath     OPTIONAL,
  OUT EFI_DEVICE_PATH_PROTOCOL    **DevicePath
  )
{
  EFI_STATUS                      Status;
  RAM_DISK_PRIVATE_DATA           *PrivateData;
  RAM_DISK_PRIVATE_DATA           *RegisteredPrivateData;
  MEDIA_RAM_DISK_DEVICE_PATH      *RamDiskDevNode;
  UINTN                           DevicePathSize;
  LIST_ENTRY                      *Entry;
  EFI_TPL                         OldTpl;

  if ((0 == RamDiskSize) || (NULL == RamDiskType) || (NULL == DevicePath)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Add check to prevent data read across the memory boundary
  //
  if (RamDiskBase + RamDiskSize > ((UINTN) -1) - RAM_DISK_BLOCK_SIZE + 1) {
    return EFI_INVALID_PARAMETER;
  }

  RamDiskDevNode = NULL;

  //
  // Create a new RAM disk instance and initialize its private data
  //
  PrivateData = AllocateCopyPool (
                  sizeof (RAM_DISK_PRIVATE_DATA),
                  &mRamDiskPrivateDataTemplate
                  );
  if (NULL == PrivateData) {
    return EFI_OUT_OF_RESOURCES;
  }

  PrivateData->StartingAddr = RamDiskBase;
  PrivateData->Size         = RamDiskSize;
  CopyGuid (&PrivateData->TypeGuid, RamDiskType);
  InitializeListHead (&PrivateData->ThisInstance);

  //
  // Generate device path information for the registered RAM disk
  //
  RamDiskDevNode = AllocateCopyPool (
                     sizeof (MEDIA_RAM_DISK_DEVICE_PATH),
                     &mRamDiskDeviceNodeTemplate
                     );
  if (NULL == RamDiskDevNode) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ErrorExit;
  }

  RamDiskInitDeviceNode (PrivateData, RamDiskDevNode);

  *DevicePath = AppendDevicePathNode (
                  ParentDevicePath,
                  (EFI_DEVICE_PATH_PROTOCOL *) RamDiskDevNode
                  );
  if (NULL == *DevicePath) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ErrorExit;
  }

  PrivateData->DevicePath = *DevicePath;

  //
  // Check whether the created device path is already present in the handle
  // database
  //
  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
  if (!IsListEmpty(&RegisteredRamDisks)) {
    DevicePathSize = GetDevicePathSize (PrivateData->DevicePath);

    EFI_LIST_FOR_EACH (Entry, &RegisteredRamDisks) {
      RegisteredPrivateData = RAM_DISK_PRIVATE_FROM_THIS (Entry);
      if (DevicePathSize == GetDevicePathSize (RegisteredPrivateData->DevicePath)) {
        //
        // Compare device path
        //
        if ((CompareMem (
               PrivateData->DevicePath,
               RegisteredPrivateData->DevicePath,
               DevicePathSize)) == 0) {
          *DevicePath = NULL;
          Status      = EFI_ALREADY_STARTED;
          goto ErrorExit;
        }
      }
    }
  }
  gBS->RestoreTPL (OldTpl);

  //
  // Fill Block IO protocol informations for the RAM disk
  //
  RamDiskInitBlockIo (PrivateData);

  //
  // Install EFI_DEVICE_PATH_PROTOCOL & EFI_BLOCK_IO(2)_PROTOCOL on a new
  // handle
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &PrivateData->Handle,
                  &gEfiBlockIoProtocolGuid,
                  &PrivateData->BlockIo,
                  &gEfiBlockIo2ProtocolGuid,
                  &PrivateData->BlockIo2,
                  &gEfiDevicePathProtocolGuid,
                  PrivateData->DevicePath,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    goto ErrorExit;
  }

  //
  // Insert the newly created one to the registered RAM disk list
  //
  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
  InsertTailList (&RegisteredRamDisks, &PrivateData->ThisInstance);
  ListEntryNum++;
  gBS->RestoreTPL (OldTpl);

  gBS->ConnectController (PrivateData->Handle, NULL, NULL, TRUE);

  FreePool (RamDiskDevNode);

  return EFI_SUCCESS;

ErrorExit:
  if (RamDiskDevNode != NULL) {
    FreePool (RamDiskDevNode);
  }

  if (PrivateData != NULL) {
    if (PrivateData->DevicePath) {
      FreePool (PrivateData->DevicePath);
    }

    FreePool (PrivateData);
  }

  return Status;
}


/**
  Unregister a RAM disk specified by DevicePath.

  @param[in] DevicePath      A pointer to the device path that describes a RAM
                             Disk device.

  @retval EFI_SUCCESS             The RAM disk is unregistered successfully.
  @retval EFI_INVALID_PARAMETER   DevicePath is NULL.
  @retval EFI_UNSUPPORTED         The device specified by DevicePath is not a
                                  valid ramdisk device path and not supported
                                  by the driver.
  @retval EFI_NOT_FOUND           The RAM disk pointed by DevicePath doesn't
                                  exist.

**/
EFI_STATUS
EFIAPI
RamDiskUnregister (
  IN  EFI_DEVICE_PATH_PROTOCOL    *DevicePath
  )
{
  LIST_ENTRY                      *Entry;
  LIST_ENTRY                      *NextEntry;
  BOOLEAN                         Found;
  UINT64                          StartingAddr;
  UINT64                          EndingAddr;
  EFI_DEVICE_PATH_PROTOCOL        *Header;
  MEDIA_RAM_DISK_DEVICE_PATH      *RamDiskDevNode;
  RAM_DISK_PRIVATE_DATA           *PrivateData;
  EFI_TPL                         OldTpl;

  if (NULL == DevicePath) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Locate the RAM disk device node.
  //
  RamDiskDevNode = NULL;
  Header         = DevicePath;
  do {
    //
    // Test if the current device node is a RAM disk.
    //
    if ((MEDIA_DEVICE_PATH == Header->Type) &&
      (MEDIA_RAM_DISK_DP == Header->SubType)) {
      RamDiskDevNode = (MEDIA_RAM_DISK_DEVICE_PATH *) Header;

      break;
    }

    Header = NextDevicePathNode (Header);
  } while ((Header->Type != END_DEVICE_PATH_TYPE));

  if (NULL == RamDiskDevNode) {
    return EFI_UNSUPPORTED;
  }

  Found          = FALSE;
  StartingAddr   = ReadUnaligned64 ((UINT64 *) &(RamDiskDevNode->StartingAddr[0]));
  EndingAddr     = ReadUnaligned64 ((UINT64 *) &(RamDiskDevNode->EndingAddr[0]));

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);
  if (!IsListEmpty(&RegisteredRamDisks)) {
    EFI_LIST_FOR_EACH_SAFE (Entry, NextEntry, &RegisteredRamDisks) {
      PrivateData = RAM_DISK_PRIVATE_FROM_THIS (Entry);

      //
      // Unregister the RAM disk given by its starting address, ending address
      // and type guid.
      //
      if ((StartingAddr == PrivateData->StartingAddr) &&
          (EndingAddr == PrivateData->StartingAddr + PrivateData->Size) &&
          (CompareGuid (&RamDiskDevNode->TypeGuid, &PrivateData->TypeGuid))) {
        //
        // Uninstall the EFI_DEVICE_PATH_PROTOCOL & EFI_BLOCK_IO(2)_PROTOCOL
        //
        gBS->UninstallMultipleProtocolInterfaces (
               PrivateData->Handle,
               &gEfiBlockIoProtocolGuid,
               &PrivateData->BlockIo,
               &gEfiBlockIo2ProtocolGuid,
               &PrivateData->BlockIo2,
               &gEfiDevicePathProtocolGuid,
               DevicePath,
               NULL
               );

        RemoveEntryList (&PrivateData->ThisInstance);

        if (RamDiskCreateHii == PrivateData->CreateMethod) {
          //
          // If a RAM disk is created within HII, then the RamDiskDxe driver
          // driver is responsible for freeing the allocated memory for the
          // RAM disk.
          //
          FreePool ((VOID *)(UINTN) PrivateData->StartingAddr);
        }

        gBS->DisconnectController (PrivateData->Handle, NULL, NULL);

        FreePool (PrivateData->DevicePath);
        FreePool (PrivateData);
        ListEntryNum--;
        Found = TRUE;

        break;
      }
    }
  }
  gBS->RestoreTPL (OldTpl);

  if (TRUE == Found) {
    return EFI_SUCCESS;
  } else {
    return EFI_NOT_FOUND;
  }
}
