/** @file
  The realization of EFI_RAM_DISK_PROTOCOL.

  Copyright (c) 2016 - 2019, Intel Corporation. All rights reserved.<BR>
  (C) Copyright 2016 Hewlett Packard Enterprise Development LP<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

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

BOOLEAN  mRamDiskSsdtTableKeyValid = FALSE;
UINTN    mRamDiskSsdtTableKey;


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
    (UINT64) PrivateData->StartingAddr + PrivateData->Size - 1
    );
  CopyGuid (&RamDiskDevNode->TypeGuid, &PrivateData->TypeGuid);
  RamDiskDevNode->Instance = PrivateData->InstanceNumber;
}


/**
  Initialize and publish NVDIMM root device SSDT in ACPI table.

  @retval EFI_SUCCESS        The NVDIMM root device SSDT is published.
  @retval Others             The NVDIMM root device SSDT is not published.

**/
EFI_STATUS
RamDiskPublishSsdt (
  VOID
  )
{
  EFI_STATUS                     Status;
  EFI_ACPI_DESCRIPTION_HEADER    *Table;
  UINTN                          SectionInstance;
  UINTN                          TableSize;

  Status          = EFI_SUCCESS;
  SectionInstance = 0;

  //
  // Scan all the EFI raw section instances in FV to find the NVDIMM root
  // device SSDT.
  //
  while (TRUE) {
    Status = GetSectionFromFv (
               &gEfiCallerIdGuid,
               EFI_SECTION_RAW,
               SectionInstance,
               (VOID **) &Table,
               &TableSize
               );
    if (EFI_ERROR (Status)) {
      break;
    }

    if (Table->OemTableId == SIGNATURE_64 ('R', 'a', 'm', 'D', 'i', 's', 'k', ' ')) {
      Status = mAcpiTableProtocol->InstallAcpiTable (
                                     mAcpiTableProtocol,
                                     Table,
                                     TableSize,
                                     &mRamDiskSsdtTableKey
                                     );
      ASSERT_EFI_ERROR (Status);

      if (!EFI_ERROR (Status)) {
        mRamDiskSsdtTableKeyValid = TRUE;
      }

      FreePool (Table);
      return Status;
    } else {
      FreePool (Table);
      SectionInstance++;
    }
  }

  return Status;
}


/**
  Publish the RAM disk NVDIMM Firmware Interface Table (NFIT) to the ACPI
  table.

  @param[in] PrivateData          Points to RAM disk private data.

  @retval EFI_SUCCESS             The RAM disk NFIT has been published.
  @retval others                  The RAM disk NFIT has not been published.

**/
EFI_STATUS
RamDiskPublishNfit (
  IN RAM_DISK_PRIVATE_DATA        *PrivateData
  )
{
  EFI_STATUS                                    Status;
  EFI_MEMORY_DESCRIPTOR                         *MemoryMap;
  EFI_MEMORY_DESCRIPTOR                         *MemoryMapEntry;
  EFI_MEMORY_DESCRIPTOR                         *MemoryMapEnd;
  UINTN                                         TableIndex;
  VOID                                          *TableHeader;
  EFI_ACPI_TABLE_VERSION                        TableVersion;
  UINTN                                         TableKey;
  EFI_ACPI_DESCRIPTION_HEADER                   *NfitHeader;
  EFI_ACPI_6_1_NFIT_SYSTEM_PHYSICAL_ADDRESS_RANGE_STRUCTURE
                                                *SpaRange;
  VOID                                          *Nfit;
  UINT32                                        NfitLen;
  UINTN                                         MemoryMapSize;
  UINTN                                         MapKey;
  UINTN                                         DescriptorSize;
  UINT32                                        DescriptorVersion;
  UINT64                                        CurrentData;
  UINT8                                         Checksum;
  BOOLEAN                                       MemoryFound;

  //
  // Get the EFI memory map.
  //
  MemoryMapSize = 0;
  MemoryMap     = NULL;
  MemoryFound   = FALSE;

  Status = gBS->GetMemoryMap (
                  &MemoryMapSize,
                  MemoryMap,
                  &MapKey,
                  &DescriptorSize,
                  &DescriptorVersion
                  );
  ASSERT (Status == EFI_BUFFER_TOO_SMALL);
  do {
    MemoryMap = (EFI_MEMORY_DESCRIPTOR *) AllocatePool (MemoryMapSize);
    ASSERT (MemoryMap != NULL);
    Status = gBS->GetMemoryMap (
                    &MemoryMapSize,
                    MemoryMap,
                    &MapKey,
                    &DescriptorSize,
                    &DescriptorVersion
                    );
    if (EFI_ERROR (Status)) {
      FreePool (MemoryMap);
    }
  } while (Status == EFI_BUFFER_TOO_SMALL);
  ASSERT_EFI_ERROR (Status);

  MemoryMapEntry = MemoryMap;
  MemoryMapEnd   = (EFI_MEMORY_DESCRIPTOR *) ((UINT8 *) MemoryMap + MemoryMapSize);
  while ((UINTN) MemoryMapEntry < (UINTN) MemoryMapEnd) {
    if ((MemoryMapEntry->Type == EfiReservedMemoryType) &&
        (MemoryMapEntry->PhysicalStart <= PrivateData->StartingAddr) &&
        (MemoryMapEntry->PhysicalStart +
         MultU64x32 (MemoryMapEntry->NumberOfPages, EFI_PAGE_SIZE)
         >= PrivateData->StartingAddr + PrivateData->Size)) {
      MemoryFound = TRUE;
      DEBUG ((
        EFI_D_INFO,
        "RamDiskPublishNfit: RAM disk with reserved meomry type, will publish to NFIT.\n"
        ));
      break;
    }
    MemoryMapEntry = NEXT_MEMORY_DESCRIPTOR (MemoryMapEntry, DescriptorSize);
  }
  FreePool (MemoryMap);

  if (!MemoryFound) {
    return EFI_NOT_FOUND;
  }

  //
  // Determine whether there is a NFIT already in the ACPI table.
  //
  Status      = EFI_SUCCESS;
  TableIndex  = 0;
  TableKey    = 0;
  TableHeader = NULL;

  while (!EFI_ERROR (Status)) {
    Status = mAcpiSdtProtocol->GetAcpiTable (
                                 TableIndex,
                                 (EFI_ACPI_SDT_HEADER **)&TableHeader,
                                 &TableVersion,
                                 &TableKey
                                 );
    if (!EFI_ERROR (Status)) {
      TableIndex++;

      if (((EFI_ACPI_SDT_HEADER *)TableHeader)->Signature ==
          EFI_ACPI_6_1_NVDIMM_FIRMWARE_INTERFACE_TABLE_STRUCTURE_SIGNATURE) {
        break;
      }
    }
  }

  if (!EFI_ERROR (Status)) {
    //
    // A NFIT is already in the ACPI table.
    //
    DEBUG ((
      EFI_D_INFO,
      "RamDiskPublishNfit: A NFIT is already exist in the ACPI Table.\n"
      ));

    NfitHeader = (EFI_ACPI_DESCRIPTION_HEADER *)TableHeader;
    NfitLen    = NfitHeader->Length + sizeof (EFI_ACPI_6_1_NFIT_SYSTEM_PHYSICAL_ADDRESS_RANGE_STRUCTURE);
    Nfit       = AllocateZeroPool (NfitLen);
    if (Nfit == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    CopyMem (Nfit, TableHeader, NfitHeader->Length);

    //
    // Update the NFIT head pointer.
    //
    NfitHeader = (EFI_ACPI_DESCRIPTION_HEADER *)Nfit;

    //
    // Uninstall the origin NFIT from the ACPI table.
    //
    Status = mAcpiTableProtocol->UninstallAcpiTable (
                                   mAcpiTableProtocol,
                                   TableKey
                                   );
    ASSERT_EFI_ERROR (Status);

    if (EFI_ERROR (Status)) {
      FreePool (Nfit);
      return Status;
    }

    //
    // Append the System Physical Address (SPA) Range Structure at the end
    // of the origin NFIT.
    //
    SpaRange   = (EFI_ACPI_6_1_NFIT_SYSTEM_PHYSICAL_ADDRESS_RANGE_STRUCTURE *)
                 ((UINT8 *)Nfit + NfitHeader->Length);

    //
    // Update the length field of the NFIT
    //
    NfitHeader->Length   = NfitLen;

    //
    // The checksum will be updated after the new contents are appended.
    //
    NfitHeader->Checksum = 0;
  } else {
    //
    // Assumption is made that if no NFIT is in the ACPI table, there is no
    // NVDIMM root device in the \SB scope.
    // Therefore, a NVDIMM root device will be reported via Secondary System
    // Description Table (SSDT).
    //
    Status = RamDiskPublishSsdt ();
    if (EFI_ERROR (Status)) {
      return Status;
    }

    //
    // No NFIT is in the ACPI table, we will create one here.
    //
    DEBUG ((
      EFI_D_INFO,
      "RamDiskPublishNfit: No NFIT is in the ACPI Table, will create one.\n"
      ));

    NfitLen = sizeof (EFI_ACPI_6_1_NVDIMM_FIRMWARE_INTERFACE_TABLE) +
              sizeof (EFI_ACPI_6_1_NFIT_SYSTEM_PHYSICAL_ADDRESS_RANGE_STRUCTURE);
    Nfit    = AllocateZeroPool (NfitLen);
    if (Nfit == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    SpaRange = (EFI_ACPI_6_1_NFIT_SYSTEM_PHYSICAL_ADDRESS_RANGE_STRUCTURE *)
               ((UINT8 *)Nfit + sizeof (EFI_ACPI_6_1_NVDIMM_FIRMWARE_INTERFACE_TABLE));

    NfitHeader                  = (EFI_ACPI_DESCRIPTION_HEADER *)Nfit;
    NfitHeader->Signature       = EFI_ACPI_6_1_NVDIMM_FIRMWARE_INTERFACE_TABLE_STRUCTURE_SIGNATURE;
    NfitHeader->Length          = NfitLen;
    NfitHeader->Revision        = EFI_ACPI_6_1_NVDIMM_FIRMWARE_INTERFACE_TABLE_REVISION;
    NfitHeader->Checksum        = 0;
    NfitHeader->OemRevision     = PcdGet32 (PcdAcpiDefaultOemRevision);
    NfitHeader->CreatorId       = PcdGet32 (PcdAcpiDefaultCreatorId);
    NfitHeader->CreatorRevision = PcdGet32 (PcdAcpiDefaultCreatorRevision);
    CurrentData                 = PcdGet64 (PcdAcpiDefaultOemTableId);
    CopyMem (NfitHeader->OemId, PcdGetPtr (PcdAcpiDefaultOemId), sizeof (NfitHeader->OemId));
    CopyMem (&NfitHeader->OemTableId, &CurrentData, sizeof (UINT64));
  }

  //
  // Fill in the content of the SPA Range Structure.
  //
  SpaRange->Type   = EFI_ACPI_6_1_NFIT_SYSTEM_PHYSICAL_ADDRESS_RANGE_STRUCTURE_TYPE;
  SpaRange->Length = sizeof (EFI_ACPI_6_1_NFIT_SYSTEM_PHYSICAL_ADDRESS_RANGE_STRUCTURE);
  SpaRange->SystemPhysicalAddressRangeBase   = PrivateData->StartingAddr;
  SpaRange->SystemPhysicalAddressRangeLength = PrivateData->Size;
  CopyGuid (&SpaRange->AddressRangeTypeGUID, &PrivateData->TypeGuid);

  Checksum             = CalculateCheckSum8((UINT8 *)Nfit, NfitHeader->Length);
  NfitHeader->Checksum = Checksum;

  //
  // Publish the NFIT to the ACPI table.
  // Note, since the NFIT might be modified by other driver, therefore, we
  // do not track the returning TableKey from the InstallAcpiTable().
  //
  Status = mAcpiTableProtocol->InstallAcpiTable (
                                 mAcpiTableProtocol,
                                 Nfit,
                                 NfitHeader->Length,
                                 &TableKey
                                 );
  ASSERT_EFI_ERROR (Status);

  FreePool (Nfit);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  PrivateData->InNfit = TRUE;

  return EFI_SUCCESS;
}


/**
  Unpublish the RAM disk NVDIMM Firmware Interface Table (NFIT) from the
  ACPI table.

  @param[in] PrivateData          Points to RAM disk private data.

  @retval EFI_SUCCESS             The RAM disk NFIT has been unpublished.
  @retval others                  The RAM disk NFIT has not been unpublished.

**/
EFI_STATUS
RamDiskUnpublishNfit (
  IN RAM_DISK_PRIVATE_DATA        *PrivateData
  )
{
  EFI_STATUS                                    Status;
  UINTN                                         TableIndex;
  VOID                                          *TableHeader;
  EFI_ACPI_TABLE_VERSION                        TableVersion;
  UINTN                                         TableKey;
  EFI_ACPI_DESCRIPTION_HEADER                   *NewNfitHeader;
  EFI_ACPI_6_1_NFIT_SYSTEM_PHYSICAL_ADDRESS_RANGE_STRUCTURE
                                                *SpaRange;
  VOID                                          *NewNfit;
  VOID                                          *NewNfitPtr;
  EFI_ACPI_6_1_NFIT_STRUCTURE_HEADER            *NfitStructHeader;
  UINT32                                        NewNfitLen;
  UINT32                                        RemainLen;
  UINT8                                         Checksum;

  //
  // Find the NFIT in the ACPI table.
  //
  Status      = EFI_SUCCESS;
  TableIndex  = 0;
  TableKey    = 0;
  TableHeader = NULL;

  while (!EFI_ERROR (Status)) {
    Status = mAcpiSdtProtocol->GetAcpiTable (
                                 TableIndex,
                                 (EFI_ACPI_SDT_HEADER **)&TableHeader,
                                 &TableVersion,
                                 &TableKey
                                 );
    if (!EFI_ERROR (Status)) {
      TableIndex++;

      if (((EFI_ACPI_SDT_HEADER *)TableHeader)->Signature ==
          EFI_ACPI_6_1_NVDIMM_FIRMWARE_INTERFACE_TABLE_STRUCTURE_SIGNATURE) {
        break;
      }
    }
  }

  if (EFI_ERROR (Status)) {
    //
    // No NFIT is found in the ACPI table.
    //
    return EFI_NOT_FOUND;
  }

  NewNfitLen    = ((EFI_ACPI_DESCRIPTION_HEADER *)TableHeader)->Length -
                  sizeof (EFI_ACPI_6_1_NFIT_SYSTEM_PHYSICAL_ADDRESS_RANGE_STRUCTURE);

  //
  // After removing this RAM disk from the NFIT, if no other structure is in
  // the NFIT, we just remove the NFIT and the SSDT which is used to report
  // the NVDIMM root device.
  //
  if (NewNfitLen == sizeof (EFI_ACPI_6_1_NVDIMM_FIRMWARE_INTERFACE_TABLE)) {
    //
    // Remove the NFIT.
    //
    Status = mAcpiTableProtocol->UninstallAcpiTable (
                                   mAcpiTableProtocol,
                                   TableKey
                                   );
    ASSERT_EFI_ERROR (Status);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    //
    // Remove the SSDT which is used by RamDiskDxe driver to report the NVDIMM
    // root device.
    // We do not care the return status since this SSDT might already be
    // uninstalled by other drivers to update the information of the NVDIMM
    // root device.
    //
    if (mRamDiskSsdtTableKeyValid) {
      mRamDiskSsdtTableKeyValid = FALSE;

      mAcpiTableProtocol->UninstallAcpiTable (
                            mAcpiTableProtocol,
                            mRamDiskSsdtTableKey
                            );
    }

    return EFI_SUCCESS;
  }

  NewNfit = AllocateZeroPool (NewNfitLen);
  if (NewNfit == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Get a copy of the old NFIT header content.
  //
  CopyMem (NewNfit, TableHeader, sizeof (EFI_ACPI_6_1_NVDIMM_FIRMWARE_INTERFACE_TABLE));
  NewNfitHeader           = (EFI_ACPI_DESCRIPTION_HEADER *)NewNfit;
  NewNfitHeader->Length   = NewNfitLen;
  NewNfitHeader->Checksum = 0;

  //
  // Copy the content of required NFIT structures.
  //
  NewNfitPtr       = (UINT8 *)NewNfit + sizeof (EFI_ACPI_6_1_NVDIMM_FIRMWARE_INTERFACE_TABLE);
  RemainLen        = NewNfitLen - sizeof (EFI_ACPI_6_1_NVDIMM_FIRMWARE_INTERFACE_TABLE);
  NfitStructHeader = (EFI_ACPI_6_1_NFIT_STRUCTURE_HEADER *)
                     ((UINT8 *)TableHeader + sizeof (EFI_ACPI_6_1_NVDIMM_FIRMWARE_INTERFACE_TABLE));
  while (RemainLen > 0) {
    if ((NfitStructHeader->Type == EFI_ACPI_6_1_NFIT_SYSTEM_PHYSICAL_ADDRESS_RANGE_STRUCTURE_TYPE) &&
        (NfitStructHeader->Length == sizeof (EFI_ACPI_6_1_NFIT_SYSTEM_PHYSICAL_ADDRESS_RANGE_STRUCTURE))) {
      SpaRange = (EFI_ACPI_6_1_NFIT_SYSTEM_PHYSICAL_ADDRESS_RANGE_STRUCTURE *)NfitStructHeader;

      if ((SpaRange->SystemPhysicalAddressRangeBase == PrivateData->StartingAddr) &&
          (SpaRange->SystemPhysicalAddressRangeLength == PrivateData->Size) &&
          (CompareGuid (&SpaRange->AddressRangeTypeGUID, &PrivateData->TypeGuid))) {
        //
        // Skip the SPA Range Structure for the RAM disk to be unpublished
        // from NFIT.
        //
        NfitStructHeader = (EFI_ACPI_6_1_NFIT_STRUCTURE_HEADER *)
                           ((UINT8 *)NfitStructHeader + NfitStructHeader->Length);
        continue;
      }
    }

    //
    // Copy the content of origin NFIT.
    //
    CopyMem (NewNfitPtr, NfitStructHeader, NfitStructHeader->Length);
    NewNfitPtr = (UINT8 *)NewNfitPtr + NfitStructHeader->Length;

    //
    // Move to the header of next NFIT structure.
    //
    RemainLen       -= NfitStructHeader->Length;
    NfitStructHeader = (EFI_ACPI_6_1_NFIT_STRUCTURE_HEADER *)
                       ((UINT8 *)NfitStructHeader + NfitStructHeader->Length);
  }

  Checksum                = CalculateCheckSum8((UINT8 *)NewNfit, NewNfitHeader->Length);
  NewNfitHeader->Checksum = Checksum;

  Status = mAcpiTableProtocol->UninstallAcpiTable (
                                 mAcpiTableProtocol,
                                 TableKey
                                 );
  ASSERT_EFI_ERROR (Status);

  if (EFI_ERROR (Status)) {
    FreePool (NewNfit);
    return Status;
  }

  //
  // Publish the NFIT to the ACPI table.
  // Note, since the NFIT might be modified by other driver, therefore, we
  // do not track the returning TableKey from the InstallAcpiTable().
  //
  Status = mAcpiTableProtocol->InstallAcpiTable (
                                 mAcpiTableProtocol,
                                 NewNfit,
                                 NewNfitLen,
                                 &TableKey
                                 );
  ASSERT_EFI_ERROR (Status);

  FreePool (NewNfit);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
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

  if ((0 == RamDiskSize) || (NULL == RamDiskType) || (NULL == DevicePath)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Add check to prevent data read across the memory boundary
  //
  if ((RamDiskSize > MAX_UINTN) ||
      (RamDiskBase > MAX_UINTN - RamDiskSize + 1)) {
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
  InsertTailList (&RegisteredRamDisks, &PrivateData->ThisInstance);

  gBS->ConnectController (PrivateData->Handle, NULL, NULL, TRUE);

  FreePool (RamDiskDevNode);

  if ((mAcpiTableProtocol != NULL) && (mAcpiSdtProtocol != NULL)) {
    RamDiskPublishNfit (PrivateData);
  }

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

  if (!IsListEmpty(&RegisteredRamDisks)) {
    EFI_LIST_FOR_EACH_SAFE (Entry, NextEntry, &RegisteredRamDisks) {
      PrivateData = RAM_DISK_PRIVATE_FROM_THIS (Entry);

      //
      // Unregister the RAM disk given by its starting address, ending address
      // and type guid.
      //
      if ((StartingAddr == PrivateData->StartingAddr) &&
          (EndingAddr == PrivateData->StartingAddr + PrivateData->Size - 1) &&
          (CompareGuid (&RamDiskDevNode->TypeGuid, &PrivateData->TypeGuid))) {
        //
        // Remove the content for this RAM disk in NFIT.
        //
        if (PrivateData->InNfit) {
          RamDiskUnpublishNfit (PrivateData);
        }

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
               (EFI_DEVICE_PATH_PROTOCOL *) PrivateData->DevicePath,
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

        FreePool (PrivateData->DevicePath);
        FreePool (PrivateData);
        Found = TRUE;

        break;
      }
    }
  }

  if (TRUE == Found) {
    return EFI_SUCCESS;
  } else {
    return EFI_NOT_FOUND;
  }
}
