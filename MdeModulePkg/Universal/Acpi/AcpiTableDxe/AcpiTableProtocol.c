/** @file
  ACPI Table Protocol Implementation

  Copyright (c) 2006 - 2021, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2016, Linaro Ltd. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

//
// Includes
//
#include "AcpiTable.h"
//
// The maximum number of tables that pre-allocated.
//
UINTN  mEfiAcpiMaxNumTables = EFI_ACPI_MAX_NUM_TABLES;

//
// Allocation strategy to use for AllocatePages ().
// Runtime value depends on PcdExposedAcpiTableVersions.
//
STATIC EFI_ALLOCATE_TYPE  mAcpiTableAllocType;

/**
  This function adds an ACPI table to the table list.  It will detect FACS and
  allocate the correct type of memory and properly align the table.

  @param  AcpiTableInstance         Instance of the protocol.
  @param  Table                     Table to add.
  @param  Checksum                  Does the table require checksumming.
  @param  Version                   The version of the list to add the table to.
  @param  IsFromHob                 True, if add Apci Table from Hob List.
  @param  Handle                    Pointer for returning the handle.

  @return EFI_SUCCESS               The function completed successfully.
  @return EFI_OUT_OF_RESOURCES      Could not allocate a required resource.
  @return EFI_ABORTED               The table is a duplicate of a table that is required
                                    to be unique.

**/
EFI_STATUS
AddTableToList (
  IN EFI_ACPI_TABLE_INSTANCE  *AcpiTableInstance,
  IN VOID                     *Table,
  IN BOOLEAN                  Checksum,
  IN EFI_ACPI_TABLE_VERSION   Version,
  IN BOOLEAN                  IsFromHob,
  OUT UINTN                   *Handle
  );

/**
  This function finds and removes the table specified by the handle.

  @param  AcpiTableInstance  Instance of the protocol.
  @param  Version            Bitmask of which versions to remove.
  @param  Handle             Table to remove.

  @return EFI_SUCCESS    The function completed successfully.
  @return EFI_ABORTED    An error occurred.
  @return EFI_NOT_FOUND  Handle not found in table list.

**/
EFI_STATUS
RemoveTableFromList (
  IN EFI_ACPI_TABLE_INSTANCE  *AcpiTableInstance,
  IN EFI_ACPI_TABLE_VERSION   Version,
  IN UINTN                    Handle
  );

/**
  This function calculates and updates an UINT8 checksum.

  @param  Buffer          Pointer to buffer to checksum
  @param  Size            Number of bytes to checksum
  @param  ChecksumOffset  Offset to place the checksum result in

  @return EFI_SUCCESS             The function completed successfully.
**/
EFI_STATUS
AcpiPlatformChecksum (
  IN VOID   *Buffer,
  IN UINTN  Size,
  IN UINTN  ChecksumOffset
  );

/**
  Checksum all versions of the common tables, RSDP, RSDT, XSDT.

  @param  AcpiTableInstance  Protocol instance private data.

  @return EFI_SUCCESS        The function completed successfully.

**/
EFI_STATUS
ChecksumCommonTables (
  IN OUT EFI_ACPI_TABLE_INSTANCE  *AcpiTableInstance
  );

//
// Protocol function implementations.
//

/**
  This function publishes the specified versions of the ACPI tables by
  installing EFI configuration table entries for them.  Any combination of
  table versions can be published.

  @param  AcpiTableInstance  Instance of the protocol.
  @param  Version            Version(s) to publish.

  @return EFI_SUCCESS  The function completed successfully.
  @return EFI_ABORTED  The function could not complete successfully.

**/
EFI_STATUS
EFIAPI
PublishTables (
  IN EFI_ACPI_TABLE_INSTANCE  *AcpiTableInstance,
  IN EFI_ACPI_TABLE_VERSION   Version
  )
{
  EFI_STATUS  Status;
  UINT32      *CurrentRsdtEntry;
  VOID        *CurrentXsdtEntry;
  UINT64      Buffer64;

  //
  // Reorder tables as some operating systems don't seem to find the
  // FADT correctly if it is not in the first few entries
  //

  //
  // Add FADT as the first entry
  //
  if ((Version & EFI_ACPI_TABLE_VERSION_1_0B) != 0) {
    CurrentRsdtEntry  = (UINT32 *)((UINT8 *)AcpiTableInstance->Rsdt1 + sizeof (EFI_ACPI_DESCRIPTION_HEADER));
    *CurrentRsdtEntry = (UINT32)(UINTN)AcpiTableInstance->Fadt1;

    CurrentRsdtEntry  = (UINT32 *)((UINT8 *)AcpiTableInstance->Rsdt3 + sizeof (EFI_ACPI_DESCRIPTION_HEADER));
    *CurrentRsdtEntry = (UINT32)(UINTN)AcpiTableInstance->Fadt3;
  }

  if ((Version & ACPI_TABLE_VERSION_GTE_2_0) != 0) {
    CurrentXsdtEntry = (VOID *)((UINT8 *)AcpiTableInstance->Xsdt + sizeof (EFI_ACPI_DESCRIPTION_HEADER));
    //
    // Add entry to XSDT, XSDT expects 64 bit pointers, but
    // the table pointers in XSDT are not aligned on 8 byte boundary.
    //
    Buffer64 = (UINT64)(UINTN)AcpiTableInstance->Fadt3;
    CopyMem (
      CurrentXsdtEntry,
      &Buffer64,
      sizeof (UINT64)
      );
  }

  //
  // Do checksum again because Dsdt/Xsdt is updated.
  //
  ChecksumCommonTables (AcpiTableInstance);

  //
  // Add the RSD_PTR to the system table and store that we have installed the
  // tables.
  //
  if ((Version & EFI_ACPI_TABLE_VERSION_1_0B) != 0) {
    Status = gBS->InstallConfigurationTable (&gEfiAcpi10TableGuid, AcpiTableInstance->Rsdp1);
    if (EFI_ERROR (Status)) {
      return EFI_ABORTED;
    }
  }

  if ((Version & ACPI_TABLE_VERSION_GTE_2_0) != 0) {
    Status = gBS->InstallConfigurationTable (&gEfiAcpiTableGuid, AcpiTableInstance->Rsdp3);
    if (EFI_ERROR (Status)) {
      return EFI_ABORTED;
    }
  }

  return EFI_SUCCESS;
}

/**
  Installs an ACPI table into the RSDT/XSDT.
  Note that the ACPI table should be checksumed before installing it.
  Otherwise it will assert.

  @param  This                 Protocol instance pointer.
  @param  AcpiTableBuffer      A pointer to a buffer containing the ACPI table to be installed.
  @param  AcpiTableBufferSize  Specifies the size, in bytes, of the AcpiTableBuffer buffer.
  @param  TableKey             Reurns a key to refer to the ACPI table.

  @return EFI_SUCCESS            The table was successfully inserted.
  @return EFI_INVALID_PARAMETER  Either AcpiTableBuffer is NULL, TableKey is NULL, or AcpiTableBufferSize
                                 and the size field embedded in the ACPI table pointed to by AcpiTableBuffer
                                 are not in sync.
  @return EFI_OUT_OF_RESOURCES   Insufficient resources exist to complete the request.
  @retval EFI_ACCESS_DENIED      The table signature matches a table already
                                 present in the system and platform policy
                                 does not allow duplicate tables of this type.

**/
EFI_STATUS
EFIAPI
InstallAcpiTable (
  IN   EFI_ACPI_TABLE_PROTOCOL  *This,
  IN   VOID                     *AcpiTableBuffer,
  IN   UINTN                    AcpiTableBufferSize,
  OUT  UINTN                    *TableKey
  )
{
  EFI_ACPI_TABLE_INSTANCE  *AcpiTableInstance;
  EFI_STATUS               Status;
  VOID                     *AcpiTableBufferConst;
  EFI_ACPI_TABLE_VERSION   Version;

  //
  // Check for invalid input parameters
  //
  if (  (AcpiTableBuffer == NULL) || (TableKey == NULL)
     || (((EFI_ACPI_DESCRIPTION_HEADER *)AcpiTableBuffer)->Length != AcpiTableBufferSize))
  {
    return EFI_INVALID_PARAMETER;
  }

  Version = PcdGet32 (PcdAcpiExposedTableVersions);

  //
  // Get the instance of the ACPI table protocol
  //
  AcpiTableInstance = EFI_ACPI_TABLE_INSTANCE_FROM_THIS (This);

  //
  // Install the ACPI table
  //
  AcpiTableBufferConst = AllocateCopyPool (AcpiTableBufferSize, AcpiTableBuffer);
  *TableKey            = 0;
  Status               = AddTableToList (
                           AcpiTableInstance,
                           AcpiTableBufferConst,
                           TRUE,
                           Version,
                           FALSE,
                           TableKey
                           );
  if (!EFI_ERROR (Status)) {
    Status = PublishTables (
               AcpiTableInstance,
               Version
               );
  }

  FreePool (AcpiTableBufferConst);

  //
  // Add a new table successfully, notify registed callback
  //
  if (FeaturePcdGet (PcdInstallAcpiSdtProtocol)) {
    if (!EFI_ERROR (Status)) {
      SdtNotifyAcpiList (
        AcpiTableInstance,
        Version,
        *TableKey
        );
    }
  }

  return Status;
}

/**
  Removes an ACPI table from the RSDT/XSDT.

  @param  This      Protocol instance pointer.
  @param  TableKey  Specifies the table to uninstall.  The key was returned from InstallAcpiTable().

  @return EFI_SUCCESS    The table was successfully uninstalled.
  @return EFI_NOT_FOUND  TableKey does not refer to a valid key for a table entry.

**/
EFI_STATUS
EFIAPI
UninstallAcpiTable (
  IN  EFI_ACPI_TABLE_PROTOCOL  *This,
  IN  UINTN                    TableKey
  )
{
  EFI_ACPI_TABLE_INSTANCE  *AcpiTableInstance;
  EFI_STATUS               Status;
  EFI_ACPI_TABLE_VERSION   Version;

  //
  // Get the instance of the ACPI table protocol
  //
  AcpiTableInstance = EFI_ACPI_TABLE_INSTANCE_FROM_THIS (This);

  Version = PcdGet32 (PcdAcpiExposedTableVersions);

  //
  // Uninstall the ACPI table
  //
  Status = RemoveTableFromList (
             AcpiTableInstance,
             Version,
             TableKey
             );
  if (!EFI_ERROR (Status)) {
    Status = PublishTables (
               AcpiTableInstance,
               Version
               );
  }

  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  } else {
    return EFI_SUCCESS;
  }
}

/**
  If the number of APCI tables exceeds the preallocated max table number, enlarge the table buffer.

  @param  AcpiTableInstance       ACPI table protocol instance data structure.

  @return EFI_SUCCESS             reallocate the table beffer successfully.
  @return EFI_OUT_OF_RESOURCES    Unable to allocate required resources.

**/
EFI_STATUS
ReallocateAcpiTableBuffer (
  IN EFI_ACPI_TABLE_INSTANCE  *AcpiTableInstance
  )
{
  UINTN                    NewMaxTableNumber;
  UINTN                    TotalSize;
  UINT8                    *Pointer;
  EFI_PHYSICAL_ADDRESS     PageAddress;
  EFI_ACPI_TABLE_INSTANCE  TempPrivateData;
  EFI_STATUS               Status;
  UINT64                   CurrentData;
  EFI_MEMORY_TYPE          AcpiAllocateMemoryType;

  CopyMem (&TempPrivateData, AcpiTableInstance, sizeof (EFI_ACPI_TABLE_INSTANCE));
  //
  // Enlarge the max table number from mEfiAcpiMaxNumTables to mEfiAcpiMaxNumTables + EFI_ACPI_MAX_NUM_TABLES
  //
  NewMaxTableNumber = mEfiAcpiMaxNumTables + EFI_ACPI_MAX_NUM_TABLES;
  //
  // Create RSDT, XSDT structures and allocate buffers.
  //
  TotalSize = sizeof (EFI_ACPI_DESCRIPTION_HEADER) +         // for ACPI 2.0/3.0 XSDT
              NewMaxTableNumber * sizeof (UINT64);

  if ((PcdGet32 (PcdAcpiExposedTableVersions) & EFI_ACPI_TABLE_VERSION_1_0B) != 0) {
    TotalSize += sizeof (EFI_ACPI_DESCRIPTION_HEADER) +      // for ACPI 1.0 RSDT
                 NewMaxTableNumber * sizeof (UINT32) +
                 sizeof (EFI_ACPI_DESCRIPTION_HEADER) +      // for ACPI 2.0/3.0 RSDT
                 NewMaxTableNumber * sizeof (UINT32);
  }

  if (PcdGetBool (PcdNoACPIReclaimMemory)) {
    AcpiAllocateMemoryType = EfiACPIMemoryNVS;
  } else {
    AcpiAllocateMemoryType = EfiACPIReclaimMemory;
  }

  if (mAcpiTableAllocType != AllocateAnyPages) {
    //
    // Allocate memory in the lower 32 bit of address range for
    // compatibility with ACPI 1.0 OS.
    //
    // This is done because ACPI 1.0 pointers are 32 bit values.
    // ACPI 2.0 OS and all 64 bit OS must use the 64 bit ACPI table addresses.
    // There is no architectural reason these should be below 4GB, it is purely
    // for convenience of implementation that we force memory below 4GB.
    //
    PageAddress = 0xFFFFFFFF;
    Status      = gBS->AllocatePages (
                         mAcpiTableAllocType,
                         AcpiAllocateMemoryType,
                         EFI_SIZE_TO_PAGES (TotalSize),
                         &PageAddress
                         );
  } else {
    Status = gBS->AllocatePool (
                    AcpiAllocateMemoryType,
                    TotalSize,
                    (VOID **)&Pointer
                    );
  }

  if (EFI_ERROR (Status)) {
    return EFI_OUT_OF_RESOURCES;
  }

  if (mAcpiTableAllocType != AllocateAnyPages) {
    Pointer = (UINT8 *)(UINTN)PageAddress;
  }

  ZeroMem (Pointer, TotalSize);

  AcpiTableInstance->Rsdt1 = (EFI_ACPI_DESCRIPTION_HEADER *)Pointer;
  if ((PcdGet32 (PcdAcpiExposedTableVersions) & EFI_ACPI_TABLE_VERSION_1_0B) != 0) {
    Pointer                 += (sizeof (EFI_ACPI_DESCRIPTION_HEADER) + NewMaxTableNumber * sizeof (UINT32));
    AcpiTableInstance->Rsdt3 = (EFI_ACPI_DESCRIPTION_HEADER *)Pointer;
    Pointer                 += (sizeof (EFI_ACPI_DESCRIPTION_HEADER) + NewMaxTableNumber * sizeof (UINT32));
  }

  AcpiTableInstance->Xsdt = (EFI_ACPI_DESCRIPTION_HEADER *)Pointer;

  //
  // Update RSDP to point to the new Rsdt and Xsdt address.
  //
  if ((PcdGet32 (PcdAcpiExposedTableVersions) & EFI_ACPI_TABLE_VERSION_1_0B) != 0) {
    AcpiTableInstance->Rsdp1->RsdtAddress = (UINT32)(UINTN)AcpiTableInstance->Rsdt1;
    AcpiTableInstance->Rsdp3->RsdtAddress = (UINT32)(UINTN)AcpiTableInstance->Rsdt3;
  }

  CurrentData = (UINT64)(UINTN)AcpiTableInstance->Xsdt;
  CopyMem (&AcpiTableInstance->Rsdp3->XsdtAddress, &CurrentData, sizeof (UINT64));

  //
  // copy the original Rsdt1, Rsdt3 and Xsdt structure to new buffer
  //
  if ((PcdGet32 (PcdAcpiExposedTableVersions) & EFI_ACPI_TABLE_VERSION_1_0B) != 0) {
    CopyMem (AcpiTableInstance->Rsdt1, TempPrivateData.Rsdt1, (sizeof (EFI_ACPI_DESCRIPTION_HEADER) + mEfiAcpiMaxNumTables * sizeof (UINT32)));
    CopyMem (AcpiTableInstance->Rsdt3, TempPrivateData.Rsdt3, (sizeof (EFI_ACPI_DESCRIPTION_HEADER) + mEfiAcpiMaxNumTables * sizeof (UINT32)));
  }

  CopyMem (AcpiTableInstance->Xsdt, TempPrivateData.Xsdt, (sizeof (EFI_ACPI_DESCRIPTION_HEADER) + mEfiAcpiMaxNumTables * sizeof (UINT64)));

  if (mAcpiTableAllocType != AllocateAnyPages) {
    //
    // Calculate orignal ACPI table buffer size
    //
    TotalSize = sizeof (EFI_ACPI_DESCRIPTION_HEADER) +         // for ACPI 2.0/3.0 XSDT
                mEfiAcpiMaxNumTables * sizeof (UINT64);

    if ((PcdGet32 (PcdAcpiExposedTableVersions) & EFI_ACPI_TABLE_VERSION_1_0B) != 0) {
      TotalSize += sizeof (EFI_ACPI_DESCRIPTION_HEADER) +         // for ACPI 1.0 RSDT
                   mEfiAcpiMaxNumTables * sizeof (UINT32) +
                   sizeof (EFI_ACPI_DESCRIPTION_HEADER) +         // for ACPI 2.0/3.0 RSDT
                   mEfiAcpiMaxNumTables * sizeof (UINT32);
    }

    gBS->FreePages (
           (EFI_PHYSICAL_ADDRESS)(UINTN)TempPrivateData.Rsdt1,
           EFI_SIZE_TO_PAGES (TotalSize)
           );
  } else {
    gBS->FreePool (TempPrivateData.Rsdt1);
  }

  //
  // Update the Max ACPI table number
  //
  mEfiAcpiMaxNumTables = NewMaxTableNumber;
  return EFI_SUCCESS;
}

/**
  Free the memory associated with the provided EFI_ACPI_TABLE_LIST instance.

  @param  TableEntry                EFI_ACPI_TABLE_LIST instance pointer

**/
STATIC
VOID
FreeTableMemory (
  EFI_ACPI_TABLE_LIST  *TableEntry
  )
{
  if (TableEntry->PoolAllocation) {
    gBS->FreePool (TableEntry->Table);
  } else {
    gBS->FreePages (
           (EFI_PHYSICAL_ADDRESS)(UINTN)TableEntry->Table,
           EFI_SIZE_TO_PAGES (TableEntry->TableSize)
           );
  }
}

/**
  This function adds an ACPI table to the table list.  It will detect FACS and
  allocate the correct type of memory and properly align the table.

  @param  AcpiTableInstance         Instance of the protocol.
  @param  Table                     Table to add.
  @param  Checksum                  Does the table require checksumming.
  @param  Version                   The version of the list to add the table to.
  @param  IsFromHob                 True, if add Apci Table from Hob List.
  @param  Handle                    Pointer for returning the handle.

  @return EFI_SUCCESS               The function completed successfully.
  @return EFI_OUT_OF_RESOURCES      Could not allocate a required resource.
  @retval EFI_ACCESS_DENIED         The table signature matches a table already
                                    present in the system and platform policy
                                    does not allow duplicate tables of this type.

**/
EFI_STATUS
AddTableToList (
  IN EFI_ACPI_TABLE_INSTANCE  *AcpiTableInstance,
  IN VOID                     *Table,
  IN BOOLEAN                  Checksum,
  IN EFI_ACPI_TABLE_VERSION   Version,
  IN BOOLEAN                  IsFromHob,
  OUT UINTN                   *Handle
  )
{
  EFI_STATUS            Status;
  EFI_ACPI_TABLE_LIST   *CurrentTableList;
  UINT32                CurrentTableSignature;
  UINT32                CurrentTableSize;
  UINT32                *CurrentRsdtEntry;
  VOID                  *CurrentXsdtEntry;
  EFI_PHYSICAL_ADDRESS  AllocPhysAddress;
  UINT64                Buffer64;
  BOOLEAN               AddToRsdt;
  EFI_MEMORY_TYPE       AcpiAllocateMemoryType;

  //
  // Check for invalid input parameters
  //
  ASSERT (AcpiTableInstance);
  ASSERT (Table);
  ASSERT (Handle);

  //
  // Init locals
  //
  AddToRsdt = TRUE;

  //
  // Create a new list entry
  //
  CurrentTableList = AllocatePool (sizeof (EFI_ACPI_TABLE_LIST));
  ASSERT (CurrentTableList);

  //
  // Determine table type and size
  //
  CurrentTableSignature = ((EFI_ACPI_COMMON_HEADER *)Table)->Signature;
  CurrentTableSize      = ((EFI_ACPI_COMMON_HEADER *)Table)->Length;

  //
  // Allocate a buffer for the table.  All tables are allocated in the lower 32 bits of address space
  // for backwards compatibility with ACPI 1.0 OS.
  //
  // This is done because ACPI 1.0 pointers are 32 bit values.
  // ACPI 2.0 OS and all 64 bit OS must use the 64 bit ACPI table addresses.
  // There is no architectural reason these should be below 4GB, it is purely
  // for convenience of implementation that we force memory below 4GB.
  //
  AllocPhysAddress                 = 0xFFFFFFFF;
  CurrentTableList->TableSize      = CurrentTableSize;
  CurrentTableList->PoolAllocation = FALSE;

  if (PcdGetBool (PcdNoACPIReclaimMemory)) {
    AcpiAllocateMemoryType = EfiACPIMemoryNVS;
  } else {
    AcpiAllocateMemoryType = EfiACPIReclaimMemory;
  }

  //
  // Allocation memory type depends on the type of the table
  //
  if ((CurrentTableSignature == EFI_ACPI_2_0_FIRMWARE_ACPI_CONTROL_STRUCTURE_SIGNATURE) ||
      (CurrentTableSignature == EFI_ACPI_4_0_UEFI_ACPI_DATA_TABLE_SIGNATURE))
  {
    //
    // Allocate memory for the FACS.  This structure must be aligned
    // on a 64 byte boundary and must be ACPI NVS memory.
    // Using AllocatePages should ensure that it is always aligned.
    // Do not change signature for new ACPI version because they are same.
    //
    // UEFI table also need to be in ACPI NVS memory, because some data field
    // could be updated by OS present agent. For example, BufferPtrAddress in
    // SMM communication ACPI table.
    //
    ASSERT ((EFI_PAGE_SIZE % 64) == 0);
    if (IsFromHob) {
      AllocPhysAddress = (UINTN)Table;
      Status           = EFI_SUCCESS;
    } else {
      Status = gBS->AllocatePages (
                      AllocateMaxAddress,
                      EfiACPIMemoryNVS,
                      EFI_SIZE_TO_PAGES (CurrentTableList->TableSize),
                      &AllocPhysAddress
                      );
    }
  } else if (mAcpiTableAllocType == AllocateAnyPages) {
    //
    // If there is no allocation limit, there is also no need to use page
    // based allocations for ACPI tables, which may be wasteful on platforms
    // such as AArch64 that allocate multiples of 64 KB
    //
    Status = gBS->AllocatePool (
                    AcpiAllocateMemoryType,
                    CurrentTableList->TableSize,
                    (VOID **)&CurrentTableList->Table
                    );
    CurrentTableList->PoolAllocation = TRUE;
  } else {
    //
    // All other tables are ACPI reclaim memory, no alignment requirements.
    //
    Status = gBS->AllocatePages (
                    mAcpiTableAllocType,
                    AcpiAllocateMemoryType,
                    EFI_SIZE_TO_PAGES (CurrentTableList->TableSize),
                    &AllocPhysAddress
                    );
    CurrentTableList->Table = (EFI_ACPI_COMMON_HEADER *)(UINTN)AllocPhysAddress;
  }

  //
  // Check return value from memory alloc.
  //
  if (EFI_ERROR (Status)) {
    gBS->FreePool (CurrentTableList);
    return EFI_OUT_OF_RESOURCES;
  }

  if (!CurrentTableList->PoolAllocation) {
    CurrentTableList->Table = (EFI_ACPI_COMMON_HEADER *)(UINTN)AllocPhysAddress;
  }

  //
  // Initialize the table contents
  //
  CurrentTableList->Signature = EFI_ACPI_TABLE_LIST_SIGNATURE;
  CopyMem (CurrentTableList->Table, Table, CurrentTableSize);
  CurrentTableList->Handle  = AcpiTableInstance->CurrentHandle++;
  *Handle                   = CurrentTableList->Handle;
  CurrentTableList->Version = Version;

  //
  // Update internal pointers if this is a required table.  If it is a required
  // table and a table of that type already exists, return an error.
  //
  // Calculate the checksum if the table is not FACS.
  //
  switch (CurrentTableSignature) {
    case EFI_ACPI_1_0_FIXED_ACPI_DESCRIPTION_TABLE_SIGNATURE:
      //
      // We don't add the FADT in the standard way because some
      // OS expect the FADT to be early in the table list.
      // So we always add it as the first element in the list.
      //
      AddToRsdt = FALSE;

      //
      // Check that the table has not been previously added.
      //
      if ((((Version & EFI_ACPI_TABLE_VERSION_1_0B) != 0) && (AcpiTableInstance->Fadt1 != NULL)) ||
          (((Version & ACPI_TABLE_VERSION_GTE_2_0)  != 0) && (AcpiTableInstance->Fadt3 != NULL))
          )
      {
        FreeTableMemory (CurrentTableList);
        gBS->FreePool (CurrentTableList);
        return EFI_ACCESS_DENIED;
      }

      //
      // Add the table to the appropriate table version
      //
      if ((Version & EFI_ACPI_TABLE_VERSION_1_0B) != 0) {
        //
        // Save a pointer to the table
        //
        AcpiTableInstance->Fadt1 = (EFI_ACPI_1_0_FIXED_ACPI_DESCRIPTION_TABLE *)CurrentTableList->Table;

        //
        // Update pointers in FADT.  If tables don't exist this will put NULL pointers there.
        //
        AcpiTableInstance->Fadt1->FirmwareCtrl = (UINT32)(UINTN)AcpiTableInstance->Facs1;
        AcpiTableInstance->Fadt1->Dsdt         = (UINT32)(UINTN)AcpiTableInstance->Dsdt1;

        //
        // RSDP OEM information is updated to match the FADT OEM information
        //
        CopyMem (
          &AcpiTableInstance->Rsdp1->OemId,
          &AcpiTableInstance->Fadt1->Header.OemId,
          6
          );

        //
        // RSDT OEM information is updated to match the FADT OEM information.
        //
        CopyMem (
          &AcpiTableInstance->Rsdt1->OemId,
          &AcpiTableInstance->Fadt1->Header.OemId,
          6
          );

        CopyMem (
          &AcpiTableInstance->Rsdt1->OemTableId,
          &AcpiTableInstance->Fadt1->Header.OemTableId,
          sizeof (UINT64)
          );
        AcpiTableInstance->Rsdt1->OemRevision = AcpiTableInstance->Fadt1->Header.OemRevision;
      }

      if ((Version & ACPI_TABLE_VERSION_GTE_2_0) != 0) {
        //
        // Save a pointer to the table
        //
        AcpiTableInstance->Fadt3 = (EFI_ACPI_3_0_FIXED_ACPI_DESCRIPTION_TABLE *)CurrentTableList->Table;

        //
        // Update pointers in FADT.  If tables don't exist this will put NULL pointers there.
        // Note: If the FIRMWARE_CTRL is non-zero, then X_FIRMWARE_CTRL must be zero, and
        // vice-versa.
        //
        if ((UINT64)(UINTN)AcpiTableInstance->Facs3 < BASE_4GB) {
          AcpiTableInstance->Fadt3->FirmwareCtrl = (UINT32)(UINTN)AcpiTableInstance->Facs3;
          ZeroMem (&AcpiTableInstance->Fadt3->XFirmwareCtrl, sizeof (UINT64));
        } else {
          Buffer64 = (UINT64)(UINTN)AcpiTableInstance->Facs3;
          CopyMem (
            &AcpiTableInstance->Fadt3->XFirmwareCtrl,
            &Buffer64,
            sizeof (UINT64)
            );
          AcpiTableInstance->Fadt3->FirmwareCtrl = 0;
        }

        if ((UINT64)(UINTN)AcpiTableInstance->Dsdt3 < BASE_4GB) {
          AcpiTableInstance->Fadt3->Dsdt = (UINT32)(UINTN)AcpiTableInstance->Dsdt3;
          //
          // Comment block "the caller installs the tables in "DSDT, FADT" order"
          // The below comments are also in "the caller installs the tables in "FADT, DSDT" order" comment block.
          //
          // The ACPI specification, up to and including revision 5.1 Errata A,
          // allows the DSDT and X_DSDT fields to be both set in the FADT.
          // (Obviously, this only makes sense if the DSDT address is representable in 4 bytes.)
          // Starting with 5.1 Errata B, specifically for Mantis 1393 <https://mantis.uefi.org/mantis/view.php?id=1393>,
          // the spec requires at most one of DSDT and X_DSDT fields to be set to a nonzero value,
          // but strangely an exception is 6.0 that has no this requirement.
          //
          // Here we do not make the DSDT and X_DSDT fields mutual exclusion conditionally
          // by checking FADT revision, but always set both DSDT and X_DSDT fields in the FADT
          // to have better compatibility as some OS may have assumption to only consume X_DSDT
          // field even the DSDT address is < 4G.
          //
          Buffer64 = AcpiTableInstance->Fadt3->Dsdt;
        } else {
          AcpiTableInstance->Fadt3->Dsdt = 0;
          Buffer64                       = (UINT64)(UINTN)AcpiTableInstance->Dsdt3;
        }

        CopyMem (&AcpiTableInstance->Fadt3->XDsdt, &Buffer64, sizeof (UINT64));

        //
        // RSDP OEM information is updated to match the FADT OEM information
        //
        CopyMem (
          &AcpiTableInstance->Rsdp3->OemId,
          &AcpiTableInstance->Fadt3->Header.OemId,
          6
          );

        if ((PcdGet32 (PcdAcpiExposedTableVersions) & EFI_ACPI_TABLE_VERSION_1_0B) != 0) {
          //
          // RSDT OEM information is updated to match FADT OEM information.
          //
          CopyMem (
            &AcpiTableInstance->Rsdt3->OemId,
            &AcpiTableInstance->Fadt3->Header.OemId,
            6
            );
          CopyMem (
            &AcpiTableInstance->Rsdt3->OemTableId,
            &AcpiTableInstance->Fadt3->Header.OemTableId,
            sizeof (UINT64)
            );
          AcpiTableInstance->Rsdt3->OemRevision = AcpiTableInstance->Fadt3->Header.OemRevision;
        }

        //
        // XSDT OEM information is updated to match FADT OEM information.
        //
        CopyMem (
          &AcpiTableInstance->Xsdt->OemId,
          &AcpiTableInstance->Fadt3->Header.OemId,
          6
          );
        CopyMem (
          &AcpiTableInstance->Xsdt->OemTableId,
          &AcpiTableInstance->Fadt3->Header.OemTableId,
          sizeof (UINT64)
          );
        AcpiTableInstance->Xsdt->OemRevision = AcpiTableInstance->Fadt3->Header.OemRevision;
      }

      //
      // Checksum the table
      //
      if (Checksum) {
        AcpiPlatformChecksum (
          CurrentTableList->Table,
          CurrentTableList->Table->Length,
          OFFSET_OF (
            EFI_ACPI_DESCRIPTION_HEADER,
            Checksum
            )
          );
      }

      break;

    case EFI_ACPI_1_0_FIRMWARE_ACPI_CONTROL_STRUCTURE_SIGNATURE:
      //
      // Check that the table has not been previously added.
      //
      if ((((Version & EFI_ACPI_TABLE_VERSION_1_0B) != 0) && (AcpiTableInstance->Facs1 != NULL)) ||
          (((Version & ACPI_TABLE_VERSION_GTE_2_0)  != 0) && (AcpiTableInstance->Facs3 != NULL))
          )
      {
        FreeTableMemory (CurrentTableList);
        gBS->FreePool (CurrentTableList);
        return EFI_ACCESS_DENIED;
      }

      //
      // FACS is referenced by FADT and is not part of RSDT
      //
      AddToRsdt = FALSE;

      //
      // Add the table to the appropriate table version
      //
      if ((Version & EFI_ACPI_TABLE_VERSION_1_0B) != 0) {
        //
        // Save a pointer to the table
        //
        AcpiTableInstance->Facs1 = (EFI_ACPI_1_0_FIRMWARE_ACPI_CONTROL_STRUCTURE *)CurrentTableList->Table;

        //
        // If FADT already exists, update table pointers.
        //
        if (AcpiTableInstance->Fadt1 != NULL) {
          AcpiTableInstance->Fadt1->FirmwareCtrl = (UINT32)(UINTN)AcpiTableInstance->Facs1;

          //
          // Checksum FADT table
          //
          AcpiPlatformChecksum (
            AcpiTableInstance->Fadt1,
            AcpiTableInstance->Fadt1->Header.Length,
            OFFSET_OF (
              EFI_ACPI_DESCRIPTION_HEADER,
              Checksum
              )
            );
        }
      }

      if ((Version & ACPI_TABLE_VERSION_GTE_2_0) != 0) {
        //
        // Save a pointer to the table
        //
        AcpiTableInstance->Facs3 = (EFI_ACPI_3_0_FIRMWARE_ACPI_CONTROL_STRUCTURE *)CurrentTableList->Table;

        //
        // If FADT already exists, update table pointers.
        //
        if (AcpiTableInstance->Fadt3 != NULL) {
          //
          // Note: If the FIRMWARE_CTRL is non-zero, then X_FIRMWARE_CTRL must be zero, and
          // vice-versa.
          //
          if ((UINT64)(UINTN)AcpiTableInstance->Facs3 < BASE_4GB) {
            AcpiTableInstance->Fadt3->FirmwareCtrl = (UINT32)(UINTN)AcpiTableInstance->Facs3;
            ZeroMem (&AcpiTableInstance->Fadt3->XFirmwareCtrl, sizeof (UINT64));
          } else {
            Buffer64 = (UINT64)(UINTN)AcpiTableInstance->Facs3;
            CopyMem (
              &AcpiTableInstance->Fadt3->XFirmwareCtrl,
              &Buffer64,
              sizeof (UINT64)
              );
            AcpiTableInstance->Fadt3->FirmwareCtrl = 0;
          }

          //
          // Checksum FADT table
          //
          AcpiPlatformChecksum (
            AcpiTableInstance->Fadt3,
            AcpiTableInstance->Fadt3->Header.Length,
            OFFSET_OF (
              EFI_ACPI_DESCRIPTION_HEADER,
              Checksum
              )
            );
        }
      }

      break;

    case EFI_ACPI_1_0_DIFFERENTIATED_SYSTEM_DESCRIPTION_TABLE_SIGNATURE:
      //
      // Check that the table has not been previously added.
      //
      if ((((Version & EFI_ACPI_TABLE_VERSION_1_0B) != 0) && (AcpiTableInstance->Dsdt1 != NULL)) ||
          (((Version & ACPI_TABLE_VERSION_GTE_2_0)  != 0) && (AcpiTableInstance->Dsdt3 != NULL))
          )
      {
        FreeTableMemory (CurrentTableList);
        gBS->FreePool (CurrentTableList);
        return EFI_ACCESS_DENIED;
      }

      //
      // DSDT is referenced by FADT and is not part of RSDT
      //
      AddToRsdt = FALSE;

      //
      // Add the table to the appropriate table version
      //
      if ((Version & EFI_ACPI_TABLE_VERSION_1_0B) != 0) {
        //
        // Save a pointer to the table
        //
        AcpiTableInstance->Dsdt1 = (EFI_ACPI_DESCRIPTION_HEADER *)CurrentTableList->Table;

        //
        // If FADT already exists, update table pointers.
        //
        if (AcpiTableInstance->Fadt1 != NULL) {
          AcpiTableInstance->Fadt1->Dsdt = (UINT32)(UINTN)AcpiTableInstance->Dsdt1;

          //
          // Checksum FADT table
          //
          AcpiPlatformChecksum (
            AcpiTableInstance->Fadt1,
            AcpiTableInstance->Fadt1->Header.Length,
            OFFSET_OF (
              EFI_ACPI_DESCRIPTION_HEADER,
              Checksum
              )
            );
        }
      }

      if ((Version & ACPI_TABLE_VERSION_GTE_2_0) != 0) {
        //
        // Save a pointer to the table
        //
        AcpiTableInstance->Dsdt3 = (EFI_ACPI_DESCRIPTION_HEADER *)CurrentTableList->Table;

        //
        // If FADT already exists, update table pointers.
        //
        if (AcpiTableInstance->Fadt3 != NULL) {
          if ((UINT64)(UINTN)AcpiTableInstance->Dsdt3 < BASE_4GB) {
            AcpiTableInstance->Fadt3->Dsdt = (UINT32)(UINTN)AcpiTableInstance->Dsdt3;
            //
            // Comment block "the caller installs the tables in "FADT, DSDT" order"
            // The below comments are also in "the caller installs the tables in "DSDT, FADT" order" comment block.
            //
            // The ACPI specification, up to and including revision 5.1 Errata A,
            // allows the DSDT and X_DSDT fields to be both set in the FADT.
            // (Obviously, this only makes sense if the DSDT address is representable in 4 bytes.)
            // Starting with 5.1 Errata B, specifically for Mantis 1393 <https://mantis.uefi.org/mantis/view.php?id=1393>,
            // the spec requires at most one of DSDT and X_DSDT fields to be set to a nonzero value,
            // but strangely an exception is 6.0 that has no this requirement.
            //
            // Here we do not make the DSDT and X_DSDT fields mutual exclusion conditionally
            // by checking FADT revision, but always set both DSDT and X_DSDT fields in the FADT
            // to have better compatibility as some OS may have assumption to only consume X_DSDT
            // field even the DSDT address is < 4G.
            //
            Buffer64 = AcpiTableInstance->Fadt3->Dsdt;
          } else {
            AcpiTableInstance->Fadt3->Dsdt = 0;
            Buffer64                       = (UINT64)(UINTN)AcpiTableInstance->Dsdt3;
          }

          CopyMem (&AcpiTableInstance->Fadt3->XDsdt, &Buffer64, sizeof (UINT64));

          //
          // Checksum FADT table
          //
          AcpiPlatformChecksum (
            AcpiTableInstance->Fadt3,
            AcpiTableInstance->Fadt3->Header.Length,
            OFFSET_OF (
              EFI_ACPI_DESCRIPTION_HEADER,
              Checksum
              )
            );
        }
      }

      //
      // Checksum the table
      //
      if (Checksum) {
        AcpiPlatformChecksum (
          CurrentTableList->Table,
          CurrentTableList->Table->Length,
          OFFSET_OF (
            EFI_ACPI_DESCRIPTION_HEADER,
            Checksum
            )
          );
      }

      break;

    default:
      //
      // Checksum the table
      //
      if (Checksum) {
        AcpiPlatformChecksum (
          CurrentTableList->Table,
          CurrentTableList->Table->Length,
          OFFSET_OF (
            EFI_ACPI_DESCRIPTION_HEADER,
            Checksum
            )
          );
      }

      break;
  }

  //
  // Add the table to the current list of tables
  //
  InsertTailList (&AcpiTableInstance->TableList, &CurrentTableList->Link);

  //
  // Add the table to RSDT and/or XSDT table entry lists.
  //
  //
  // Add to ACPI 1.0b table tree
  //
  if ((Version & EFI_ACPI_TABLE_VERSION_1_0B) != 0) {
    if (AddToRsdt) {
      //
      // If the table number exceed the gEfiAcpiMaxNumTables, enlarge the table buffer
      //
      if (AcpiTableInstance->NumberOfTableEntries1 >= mEfiAcpiMaxNumTables) {
        Status = ReallocateAcpiTableBuffer (AcpiTableInstance);
        ASSERT_EFI_ERROR (Status);
      }

      CurrentRsdtEntry = (UINT32 *)
                         (
                          (UINT8 *)AcpiTableInstance->Rsdt1 +
                          sizeof (EFI_ACPI_DESCRIPTION_HEADER) +
                          AcpiTableInstance->NumberOfTableEntries1 *
                          sizeof (UINT32)
                         );

      //
      // Add entry to the RSDT unless its the FACS or DSDT
      //
      *CurrentRsdtEntry = (UINT32)(UINTN)CurrentTableList->Table;

      //
      // Update RSDT length
      //
      AcpiTableInstance->Rsdt1->Length = AcpiTableInstance->Rsdt1->Length + sizeof (UINT32);

      AcpiTableInstance->NumberOfTableEntries1++;
    }
  }

  //
  // Add to ACPI 2.0/3.0  table tree
  //
  if ((Version & ACPI_TABLE_VERSION_GTE_2_0) != 0) {
    if (AddToRsdt) {
      //
      // If the table number exceed the gEfiAcpiMaxNumTables, enlarge the table buffer
      //
      if (AcpiTableInstance->NumberOfTableEntries3 >= mEfiAcpiMaxNumTables) {
        Status = ReallocateAcpiTableBuffer (AcpiTableInstance);
        ASSERT_EFI_ERROR (Status);
      }

      if ((PcdGet32 (PcdAcpiExposedTableVersions) & EFI_ACPI_TABLE_VERSION_1_0B) != 0) {
        //
        // At this time, it is assumed that RSDT and XSDT maintain parallel lists of tables.
        // If it becomes necessary to maintain separate table lists, changes will be required.
        //
        CurrentRsdtEntry = (UINT32 *)
                           (
                            (UINT8 *)AcpiTableInstance->Rsdt3 +
                            sizeof (EFI_ACPI_DESCRIPTION_HEADER) +
                            AcpiTableInstance->NumberOfTableEntries3 *
                            sizeof (UINT32)
                           );

        //
        // Add entry to the RSDT
        //
        *CurrentRsdtEntry = (UINT32)(UINTN)CurrentTableList->Table;

        //
        // Update RSDT length
        //
        AcpiTableInstance->Rsdt3->Length = AcpiTableInstance->Rsdt3->Length + sizeof (UINT32);
      }

      //
      // This pointer must not be directly dereferenced as the XSDT entries may not
      // be 64 bit aligned resulting in a possible fault.  Use CopyMem to update.
      //
      CurrentXsdtEntry = (VOID *)
                         (
                          (UINT8 *)AcpiTableInstance->Xsdt +
                          sizeof (EFI_ACPI_DESCRIPTION_HEADER) +
                          AcpiTableInstance->NumberOfTableEntries3 *
                          sizeof (UINT64)
                         );

      //
      // Add entry to XSDT, XSDT expects 64 bit pointers, but
      // the table pointers in XSDT are not aligned on 8 byte boundary.
      //
      Buffer64 = (UINT64)(UINTN)CurrentTableList->Table;
      CopyMem (
        CurrentXsdtEntry,
        &Buffer64,
        sizeof (UINT64)
        );

      //
      // Update length
      //
      AcpiTableInstance->Xsdt->Length = AcpiTableInstance->Xsdt->Length + sizeof (UINT64);

      AcpiTableInstance->NumberOfTableEntries3++;
    }
  }

  ChecksumCommonTables (AcpiTableInstance);
  return EFI_SUCCESS;
}

/**
  This function finds the table specified by the handle and returns a pointer to it.
  If the handle is not found, EFI_NOT_FOUND is returned and the contents of Table are
  undefined.

  @param  Handle      Table to find.
  @param  TableList   Table list to search
  @param  Table       Pointer to table found.

  @return EFI_SUCCESS    The function completed successfully.
  @return EFI_NOT_FOUND  No table found matching the handle specified.

**/
EFI_STATUS
FindTableByHandle (
  IN UINTN                 Handle,
  IN LIST_ENTRY            *TableList,
  OUT EFI_ACPI_TABLE_LIST  **Table
  )
{
  LIST_ENTRY           *CurrentLink;
  EFI_ACPI_TABLE_LIST  *CurrentTable;

  //
  // Check for invalid input parameters
  //
  ASSERT (Table);

  //
  // Find the table
  //
  CurrentLink = TableList->ForwardLink;

  while (CurrentLink != TableList) {
    CurrentTable = EFI_ACPI_TABLE_LIST_FROM_LINK (CurrentLink);
    if (CurrentTable->Handle == Handle) {
      //
      // Found handle, so return this table.
      //
      *Table = CurrentTable;
      return EFI_SUCCESS;
    }

    CurrentLink = CurrentLink->ForwardLink;
  }

  //
  // Table not found
  //
  return EFI_NOT_FOUND;
}

/**
  This function removes a basic table from the RSDT and/or XSDT.
  For Acpi 1.0 tables, pass in the Rsdt.
  For Acpi 2.0 tables, pass in both Rsdt and Xsdt.

  @param  Table                 Pointer to table found.
  @param  NumberOfTableEntries  Current number of table entries in the RSDT/XSDT
  @param  Rsdt                  Pointer to the RSDT to remove from
  @param  Xsdt                  Pointer to the Xsdt to remove from

  @return EFI_SUCCESS            The function completed successfully.
  @return EFI_INVALID_PARAMETER  The table was not found in both Rsdt and Xsdt.

**/
EFI_STATUS
RemoveTableFromRsdt (
  IN OUT EFI_ACPI_TABLE_LIST          *Table,
  IN OUT UINTN                        *NumberOfTableEntries,
  IN OUT EFI_ACPI_DESCRIPTION_HEADER  *Rsdt OPTIONAL,
  IN OUT EFI_ACPI_DESCRIPTION_HEADER  *Xsdt OPTIONAL
  )
{
  UINT32  *CurrentRsdtEntry;
  VOID    *CurrentXsdtEntry;
  UINT64  CurrentTablePointer64;
  UINTN   Index;

  //
  // Check for invalid input parameters
  //
  ASSERT (Table);
  ASSERT (NumberOfTableEntries);
  ASSERT (Rsdt || Xsdt);

  //
  // Find the table entry in the RSDT and XSDT
  //
  for (Index = 0; Index < *NumberOfTableEntries; Index++) {
    //
    // At this time, it is assumed that RSDT and XSDT maintain parallel lists of tables.
    // If it becomes necessary to maintain separate table lists, changes will be required.
    //
    if (Rsdt != NULL) {
      CurrentRsdtEntry = (UINT32 *)((UINT8 *)Rsdt + sizeof (EFI_ACPI_DESCRIPTION_HEADER) + Index * sizeof (UINT32));
    } else {
      CurrentRsdtEntry = NULL;
    }

    if (Xsdt != NULL) {
      //
      // This pointer must not be directly dereferenced as the XSDT entries may not
      // be 64 bit aligned resulting in a possible fault.  Use CopyMem to update.
      //
      CurrentXsdtEntry = (VOID *)((UINT8 *)Xsdt + sizeof (EFI_ACPI_DESCRIPTION_HEADER) + Index * sizeof (UINT64));

      //
      // Read the entry value out of the XSDT
      //
      CopyMem (&CurrentTablePointer64, CurrentXsdtEntry, sizeof (UINT64));
    } else {
      //
      // Initialize to NULL
      //
      CurrentXsdtEntry      = 0;
      CurrentTablePointer64 = 0;
    }

    //
    // Check if we have found the corresponding entry in both RSDT and XSDT
    //
    if (((Rsdt == NULL) || (*CurrentRsdtEntry == (UINT32)(UINTN)Table->Table)) &&
        ((Xsdt == NULL) || (CurrentTablePointer64 == (UINT64)(UINTN)Table->Table))
        )
    {
      //
      // Found entry, so copy all following entries and shrink table
      //
      if (Rsdt != NULL) {
        CopyMem (CurrentRsdtEntry, CurrentRsdtEntry + 1, (*NumberOfTableEntries - Index - 1) * sizeof (UINT32));
        ZeroMem ((UINT8 *)Rsdt + sizeof (EFI_ACPI_DESCRIPTION_HEADER) + ((*NumberOfTableEntries - 1) * sizeof (UINT32)), sizeof (UINT32));
        Rsdt->Length = Rsdt->Length - sizeof (UINT32);
      }

      if (Xsdt != NULL) {
        CopyMem (CurrentXsdtEntry, ((UINT64 *)CurrentXsdtEntry) + 1, (*NumberOfTableEntries - Index - 1) * sizeof (UINT64));
        ZeroMem ((UINT8 *)Xsdt + sizeof (EFI_ACPI_DESCRIPTION_HEADER) + ((*NumberOfTableEntries - 1) * sizeof (UINT64)), sizeof (UINT64));
        Xsdt->Length = Xsdt->Length - sizeof (UINT64);
      }

      break;
    } else if (Index + 1 == *NumberOfTableEntries) {
      //
      // At the last entry, and table not found
      //
      return EFI_INVALID_PARAMETER;
    }
  }

  //
  // Checksum the tables
  //
  if (Rsdt != NULL) {
    AcpiPlatformChecksum (
      Rsdt,
      Rsdt->Length,
      OFFSET_OF (
        EFI_ACPI_DESCRIPTION_HEADER,
        Checksum
        )
      );
  }

  if (Xsdt != NULL) {
    AcpiPlatformChecksum (
      Xsdt,
      Xsdt->Length,
      OFFSET_OF (
        EFI_ACPI_DESCRIPTION_HEADER,
        Checksum
        )
      );
  }

  //
  // Decrement the number of tables
  //
  (*NumberOfTableEntries)--;

  return EFI_SUCCESS;
}

/**
  This function removes a table and frees any associated memory.

  @param  AcpiTableInstance  Instance of the protocol.
  @param  Version            Version(s) to delete.
  @param  Table              Pointer to table found.

  @return EFI_SUCCESS  The function completed successfully.

**/
EFI_STATUS
DeleteTable (
  IN EFI_ACPI_TABLE_INSTANCE  *AcpiTableInstance,
  IN EFI_ACPI_TABLE_VERSION   Version,
  IN OUT EFI_ACPI_TABLE_LIST  *Table
  )
{
  UINT32   CurrentTableSignature;
  BOOLEAN  RemoveFromRsdt;

  //
  // Check for invalid input parameters
  //
  ASSERT (AcpiTableInstance);
  ASSERT (Table);

  //
  // Init locals
  //
  RemoveFromRsdt = TRUE;
  //
  // Check for Table->Table
  //
  ASSERT (Table->Table != NULL);
  CurrentTableSignature = ((EFI_ACPI_COMMON_HEADER *)Table->Table)->Signature;

  //
  // Basic tasks to accomplish delete are:
  //   Determine removal requirements (in RSDT/XSDT or not)
  //   Remove entry from RSDT/XSDT
  //   Remove any table references to the table
  //   If no one is using the table
  //      Free the table (removing pointers from private data and tables)
  //      Remove from list
  //      Free list structure
  //
  //
  // Determine if this table is in the RSDT or XSDT
  //
  if ((CurrentTableSignature == EFI_ACPI_1_0_FIRMWARE_ACPI_CONTROL_STRUCTURE_SIGNATURE) ||
      (CurrentTableSignature == EFI_ACPI_2_0_DIFFERENTIATED_SYSTEM_DESCRIPTION_TABLE_SIGNATURE) ||
      (CurrentTableSignature == EFI_ACPI_3_0_DIFFERENTIATED_SYSTEM_DESCRIPTION_TABLE_SIGNATURE)
      )
  {
    RemoveFromRsdt = FALSE;
  }

  //
  // We don't remove the FADT in the standard way because some
  // OS expect the FADT to be early in the table list.
  // So we always put it as the first element in the list.
  //
  if (CurrentTableSignature == EFI_ACPI_1_0_FIXED_ACPI_DESCRIPTION_TABLE_SIGNATURE) {
    RemoveFromRsdt = FALSE;
  }

  //
  // Remove the table from RSDT and XSDT
  //
  if (Table->Table != NULL) {
    //
    // This is a basic table, remove it from any lists and the Rsdt and/or Xsdt
    //
    if (Version & EFI_ACPI_TABLE_VERSION_NONE & Table->Version) {
      //
      // Remove this version from the table
      //
      Table->Version = Table->Version &~EFI_ACPI_TABLE_VERSION_NONE;
    }

    if (Version & EFI_ACPI_TABLE_VERSION_1_0B & Table->Version) {
      //
      // Remove this version from the table
      //
      Table->Version = Table->Version &~EFI_ACPI_TABLE_VERSION_1_0B;

      //
      // Remove from Rsdt.  We don't care about the return value because it is
      // acceptable for the table to not exist in Rsdt.
      // We didn't add some tables so we don't remove them.
      //
      if (RemoveFromRsdt) {
        RemoveTableFromRsdt (
          Table,
          &AcpiTableInstance->NumberOfTableEntries1,
          AcpiTableInstance->Rsdt1,
          NULL
          );
      }
    }

    if (Version & ACPI_TABLE_VERSION_GTE_2_0 & Table->Version) {
      //
      // Remove this version from the table
      //
      Table->Version = Table->Version &~(Version & ACPI_TABLE_VERSION_GTE_2_0);

      //
      // Remove from Rsdt and Xsdt.  We don't care about the return value
      // because it is acceptable for the table to not exist in Rsdt/Xsdt.
      // We didn't add some tables so we don't remove them.
      //
      if (RemoveFromRsdt) {
        RemoveTableFromRsdt (
          Table,
          &AcpiTableInstance->NumberOfTableEntries3,
          AcpiTableInstance->Rsdt3,
          AcpiTableInstance->Xsdt
          );
      }
    }

    //
    // Free the table, clean up any dependent tables and our private data pointers.
    //
    switch (Table->Table->Signature) {
      case EFI_ACPI_3_0_FIXED_ACPI_DESCRIPTION_TABLE_SIGNATURE:
        if ((Version & EFI_ACPI_TABLE_VERSION_1_0B) != 0) {
          AcpiTableInstance->Fadt1 = NULL;
        }

        if ((Version & ACPI_TABLE_VERSION_GTE_2_0) != 0) {
          AcpiTableInstance->Fadt3 = NULL;
        }

        break;

      case EFI_ACPI_3_0_FIRMWARE_ACPI_CONTROL_STRUCTURE_SIGNATURE:
        if ((Version & EFI_ACPI_TABLE_VERSION_1_0B) != 0) {
          AcpiTableInstance->Facs1 = NULL;

          //
          // Update FADT table pointers
          //
          if (AcpiTableInstance->Fadt1 != NULL) {
            AcpiTableInstance->Fadt1->FirmwareCtrl = 0;

            //
            // Checksum table
            //
            AcpiPlatformChecksum (
              AcpiTableInstance->Fadt1,
              AcpiTableInstance->Fadt1->Header.Length,
              OFFSET_OF (
                EFI_ACPI_DESCRIPTION_HEADER,
                Checksum
                )
              );
          }
        }

        if ((Version & ACPI_TABLE_VERSION_GTE_2_0) != 0) {
          AcpiTableInstance->Facs3 = NULL;

          //
          // Update FADT table pointers
          //
          if (AcpiTableInstance->Fadt3 != NULL) {
            AcpiTableInstance->Fadt3->FirmwareCtrl = 0;
            ZeroMem (&AcpiTableInstance->Fadt3->XFirmwareCtrl, sizeof (UINT64));

            //
            // Checksum table
            //
            AcpiPlatformChecksum (
              AcpiTableInstance->Fadt3,
              AcpiTableInstance->Fadt3->Header.Length,
              OFFSET_OF (
                EFI_ACPI_DESCRIPTION_HEADER,
                Checksum
                )
              );
          }
        }

        break;

      case EFI_ACPI_3_0_DIFFERENTIATED_SYSTEM_DESCRIPTION_TABLE_SIGNATURE:
        if ((Version & EFI_ACPI_TABLE_VERSION_1_0B) != 0) {
          AcpiTableInstance->Dsdt1 = NULL;

          //
          // Update FADT table pointers
          //
          if (AcpiTableInstance->Fadt1 != NULL) {
            AcpiTableInstance->Fadt1->Dsdt = 0;

            //
            // Checksum table
            //
            AcpiPlatformChecksum (
              AcpiTableInstance->Fadt1,
              AcpiTableInstance->Fadt1->Header.Length,
              OFFSET_OF (
                EFI_ACPI_DESCRIPTION_HEADER,
                Checksum
                )
              );
          }
        }

        if ((Version & ACPI_TABLE_VERSION_GTE_2_0) != 0) {
          AcpiTableInstance->Dsdt3 = NULL;

          //
          // Update FADT table pointers
          //
          if (AcpiTableInstance->Fadt3 != NULL) {
            AcpiTableInstance->Fadt3->Dsdt = 0;
            ZeroMem (&AcpiTableInstance->Fadt3->XDsdt, sizeof (UINT64));

            //
            // Checksum table
            //
            AcpiPlatformChecksum (
              AcpiTableInstance->Fadt3,
              AcpiTableInstance->Fadt3->Header.Length,
              OFFSET_OF (
                EFI_ACPI_DESCRIPTION_HEADER,
                Checksum
                )
              );
          }
        }

        break;

      default:
        //
        // Do nothing
        //
        break;
    }
  }

  //
  // If no version is using this table anymore, remove and free list entry.
  //
  if (Table->Version == 0) {
    //
    // Free the Table
    //
    FreeTableMemory (Table);
    RemoveEntryList (&(Table->Link));
    gBS->FreePool (Table);
  }

  //
  // Done
  //
  return EFI_SUCCESS;
}

/**
  This function finds and removes the table specified by the handle.

  @param  AcpiTableInstance  Instance of the protocol.
  @param  Version            Bitmask of which versions to remove.
  @param  Handle             Table to remove.

  @return EFI_SUCCESS    The function completed successfully.
  @return EFI_ABORTED    An error occurred.
  @return EFI_NOT_FOUND  Handle not found in table list.

**/
EFI_STATUS
RemoveTableFromList (
  IN EFI_ACPI_TABLE_INSTANCE  *AcpiTableInstance,
  IN EFI_ACPI_TABLE_VERSION   Version,
  IN UINTN                    Handle
  )
{
  EFI_ACPI_TABLE_LIST  *Table;
  EFI_STATUS           Status;

  Table = (EFI_ACPI_TABLE_LIST *)NULL;

  //
  // Check for invalid input parameters
  //
  ASSERT (AcpiTableInstance);

  //
  // Find the table
  //
  Status = FindTableByHandle (
             Handle,
             &AcpiTableInstance->TableList,
             &Table
             );
  if (EFI_ERROR (Status)) {
    return EFI_NOT_FOUND;
  }

  //
  // Remove the table
  //
  Status = DeleteTable (AcpiTableInstance, Version, Table);
  if (EFI_ERROR (Status)) {
    return EFI_ABORTED;
  }

  //
  // Completed successfully
  //
  return EFI_SUCCESS;
}

/**
  This function calculates and updates an UINT8 checksum.

  @param  Buffer          Pointer to buffer to checksum
  @param  Size            Number of bytes to checksum
  @param  ChecksumOffset  Offset to place the checksum result in

  @return EFI_SUCCESS             The function completed successfully.

**/
EFI_STATUS
AcpiPlatformChecksum (
  IN VOID   *Buffer,
  IN UINTN  Size,
  IN UINTN  ChecksumOffset
  )
{
  UINT8  Sum;
  UINT8  *Ptr;

  Sum = 0;
  //
  // Initialize pointer
  //
  Ptr = Buffer;

  //
  // set checksum to 0 first
  //
  Ptr[ChecksumOffset] = 0;

  //
  // add all content of buffer
  //
  while ((Size--) != 0) {
    Sum = (UINT8)(Sum + (*Ptr++));
  }

  //
  // set checksum
  //
  Ptr                 = Buffer;
  Ptr[ChecksumOffset] = (UINT8)(0xff - Sum + 1);

  return EFI_SUCCESS;
}

/**
  Checksum all versions of the common tables, RSDP, RSDT, XSDT.

  @param  AcpiTableInstance  Protocol instance private data.

  @return EFI_SUCCESS        The function completed successfully.

**/
EFI_STATUS
ChecksumCommonTables (
  IN OUT EFI_ACPI_TABLE_INSTANCE  *AcpiTableInstance
  )
{
  //
  // RSDP ACPI 1.0 checksum for 1.0 table.  This is only the first 20 bytes of the structure
  //
  if ((PcdGet32 (PcdAcpiExposedTableVersions) & EFI_ACPI_TABLE_VERSION_1_0B) != 0) {
    AcpiPlatformChecksum (
      AcpiTableInstance->Rsdp1,
      sizeof (EFI_ACPI_1_0_ROOT_SYSTEM_DESCRIPTION_POINTER),
      OFFSET_OF (
        EFI_ACPI_1_0_ROOT_SYSTEM_DESCRIPTION_POINTER,
        Checksum
        )
      );
  }

  //
  // RSDP ACPI 1.0 checksum for 2.0/3.0 table.  This is only the first 20 bytes of the structure
  //
  AcpiPlatformChecksum (
    AcpiTableInstance->Rsdp3,
    sizeof (EFI_ACPI_1_0_ROOT_SYSTEM_DESCRIPTION_POINTER),
    OFFSET_OF (
      EFI_ACPI_1_0_ROOT_SYSTEM_DESCRIPTION_POINTER,
      Checksum
      )
    );

  //
  // RSDP ACPI 2.0/3.0 checksum, this is the entire table
  //
  AcpiPlatformChecksum (
    AcpiTableInstance->Rsdp3,
    sizeof (EFI_ACPI_3_0_ROOT_SYSTEM_DESCRIPTION_POINTER),
    OFFSET_OF (
      EFI_ACPI_3_0_ROOT_SYSTEM_DESCRIPTION_POINTER,
      ExtendedChecksum
      )
    );

  if ((PcdGet32 (PcdAcpiExposedTableVersions) & EFI_ACPI_TABLE_VERSION_1_0B) != 0) {
    //
    // RSDT checksums
    //
    AcpiPlatformChecksum (
      AcpiTableInstance->Rsdt1,
      AcpiTableInstance->Rsdt1->Length,
      OFFSET_OF (
        EFI_ACPI_DESCRIPTION_HEADER,
        Checksum
        )
      );

    AcpiPlatformChecksum (
      AcpiTableInstance->Rsdt3,
      AcpiTableInstance->Rsdt3->Length,
      OFFSET_OF (
        EFI_ACPI_DESCRIPTION_HEADER,
        Checksum
        )
      );
  }

  //
  // XSDT checksum
  //
  AcpiPlatformChecksum (
    AcpiTableInstance->Xsdt,
    AcpiTableInstance->Xsdt->Length,
    OFFSET_OF (
      EFI_ACPI_DESCRIPTION_HEADER,
      Checksum
      )
    );

  return EFI_SUCCESS;
}

/**
  This function will find gUniversalPayloadAcpiTableGuid Guid Hob, and install Acpi table from it.

  @param  AcpiTableInstance  Protocol instance private data.

  @return EFI_SUCCESS        The function completed successfully.
  @return EFI_NOT_FOUND      The function doesn't find the gEfiAcpiTableGuid Guid Hob.
  @return EFI_ABORTED        The function could not complete successfully.

**/
EFI_STATUS
InstallAcpiTableFromHob (
  EFI_ACPI_TABLE_INSTANCE  *AcpiTableInstance
  )
{
  EFI_HOB_GUID_TYPE                             *GuidHob;
  EFI_ACPI_TABLE_VERSION                        Version;
  EFI_ACPI_3_0_ROOT_SYSTEM_DESCRIPTION_POINTER  *Rsdp;
  EFI_ACPI_DESCRIPTION_HEADER                   *Rsdt;
  EFI_ACPI_DESCRIPTION_HEADER                   *ChildTable;
  UINT64                                        ChildTableAddress;
  UINTN                                         Count;
  UINTN                                         Index;
  UINTN                                         TableKey;
  EFI_STATUS                                    Status;
  UINTN                                         EntrySize;
  UNIVERSAL_PAYLOAD_ACPI_TABLE                  *AcpiTableAdress;
  VOID                                          *TableToInstall;
  EFI_ACPI_SDT_HEADER                           *Table;
  UNIVERSAL_PAYLOAD_GENERIC_HEADER              *GenericHeader;

  TableKey = 0;
  Version  = PcdGet32 (PcdAcpiExposedTableVersions);
  Status   = EFI_SUCCESS;
  //
  // HOB only contains the ACPI table in 2.0+ format.
  //
  GuidHob = GetFirstGuidHob (&gUniversalPayloadAcpiTableGuid);
  if (GuidHob == NULL) {
    return EFI_NOT_FOUND;
  }

  GenericHeader = (UNIVERSAL_PAYLOAD_GENERIC_HEADER *)GET_GUID_HOB_DATA (GuidHob);
  if ((sizeof (UNIVERSAL_PAYLOAD_GENERIC_HEADER) > GET_GUID_HOB_DATA_SIZE (GuidHob)) || (GenericHeader->Length > GET_GUID_HOB_DATA_SIZE (GuidHob))) {
    return EFI_NOT_FOUND;
  }

  if (GenericHeader->Revision == UNIVERSAL_PAYLOAD_ACPI_TABLE_REVISION) {
    //
    // UNIVERSAL_PAYLOAD_ACPI_TABLE structure is used when Revision equals to UNIVERSAL_PAYLOAD_ACPI_TABLE_REVISION
    //
    AcpiTableAdress = (UNIVERSAL_PAYLOAD_ACPI_TABLE *)GET_GUID_HOB_DATA (GuidHob);
    if (AcpiTableAdress->Header.Length < UNIVERSAL_PAYLOAD_SIZEOF_THROUGH_FIELD (UNIVERSAL_PAYLOAD_ACPI_TABLE, Rsdp)) {
      //
      // Retrun if can't find the ACPI Info Hob with enough length
      //
      return EFI_NOT_FOUND;
    }

    Rsdp = (EFI_ACPI_3_0_ROOT_SYSTEM_DESCRIPTION_POINTER *)(UINTN)(AcpiTableAdress->Rsdp);

    //
    // An ACPI-compatible OS must use the XSDT if present.
    // It shouldn't happen that XsdtAddress points beyond 4G range in 32-bit environment.
    //
    ASSERT ((UINTN)Rsdp->XsdtAddress == Rsdp->XsdtAddress);

    EntrySize = sizeof (UINT64);
    Rsdt      = (EFI_ACPI_DESCRIPTION_HEADER *)(UINTN)Rsdp->XsdtAddress;
    if (Rsdt == NULL) {
      //
      // XsdtAddress is zero, then we use Rsdt which has 32 bit entry
      //
      Rsdt      = (EFI_ACPI_DESCRIPTION_HEADER *)(UINTN)Rsdp->RsdtAddress;
      EntrySize = sizeof (UINT32);
    }

    if (Rsdt->Length <= sizeof (EFI_ACPI_DESCRIPTION_HEADER)) {
      return EFI_ABORTED;
    }

    Count = (Rsdt->Length - sizeof (EFI_ACPI_DESCRIPTION_HEADER)) / EntrySize;

    for (Index = 0; Index < Count; Index++) {
      ChildTableAddress = 0;
      CopyMem (&ChildTableAddress, (UINT8 *)(Rsdt + 1) + EntrySize * Index, EntrySize);
      //
      // If the address is of UINT64 while this module runs at 32 bits,
      // make sure the upper bits are all-zeros.
      //
      ASSERT (ChildTableAddress == (UINTN)ChildTableAddress);
      if (ChildTableAddress != (UINTN)ChildTableAddress) {
        Status = EFI_ABORTED;
        break;
      }

      ChildTable = (EFI_ACPI_DESCRIPTION_HEADER *)(UINTN)ChildTableAddress;
      Status     = AddTableToList (AcpiTableInstance, ChildTable, TRUE, Version, TRUE, &TableKey);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "InstallAcpiTableFromHob: Fail to add ACPI table at 0x%p\n", ChildTable));
        ASSERT_EFI_ERROR (Status);
        break;
      }

      if (ChildTable->Signature == EFI_ACPI_3_0_FIXED_ACPI_DESCRIPTION_TABLE_SIGNATURE) {
        //
        // Add the FACS and DSDT tables if it is not NULL.
        //
        if (((EFI_ACPI_3_0_FIXED_ACPI_DESCRIPTION_TABLE *)ChildTable)->FirmwareCtrl != 0) {
          TableToInstall = (VOID *)(UINTN)((EFI_ACPI_3_0_FIXED_ACPI_DESCRIPTION_TABLE *)ChildTable)->FirmwareCtrl;
          Status         = AddTableToList (AcpiTableInstance, TableToInstall, TRUE, Version, TRUE, &TableKey);
          if (EFI_ERROR (Status)) {
            DEBUG ((DEBUG_ERROR, "InstallAcpiTableFromHob: Fail to add ACPI table FACS\n"));
            ASSERT_EFI_ERROR (Status);
            break;
          }
        }

        //
        // First check if xDSDT is available, as that is preferred as per
        // ACPI Spec 6.5+ Table 5-9 X_DSDT definition
        //
        if (((EFI_ACPI_3_0_FIXED_ACPI_DESCRIPTION_TABLE *)ChildTable)->XDsdt != 0) {
          TableToInstall = (VOID *)(UINTN)((EFI_ACPI_3_0_FIXED_ACPI_DESCRIPTION_TABLE *)ChildTable)->XDsdt;
        } else if (((EFI_ACPI_3_0_FIXED_ACPI_DESCRIPTION_TABLE *)ChildTable)->Dsdt != 0) {
          TableToInstall = (VOID *)(UINTN)((EFI_ACPI_3_0_FIXED_ACPI_DESCRIPTION_TABLE *)ChildTable)->Dsdt;
        } else {
          DEBUG ((DEBUG_ERROR, "DSDT table not found\n"));
          continue;
        }

        Status = AddTableToList (AcpiTableInstance, TableToInstall, TRUE, Version, TRUE, &TableKey);
        if (EFI_ERROR (Status)) {
          DEBUG ((DEBUG_ERROR, "InstallAcpiTableFromHob: Fail to add ACPI table DSDT\n"));
          ASSERT_EFI_ERROR (Status);
          break;
        }
      }
    }
  } else {
    return EFI_NOT_FOUND;
  }

  if (EFI_ERROR (Status)) {
    //
    // Error happens when trying to add ACPI table to the list.
    // Remove all of them from list because at this time, no other tables except from HOB are in the list
    //
    while (SdtGetAcpiTable (AcpiTableInstance, 0, &Table, &Version, &TableKey) == EFI_SUCCESS) {
      RemoveTableFromList (AcpiTableInstance, Version, TableKey);
    }
  } else {
    Status = PublishTables (AcpiTableInstance, Version);
  }

  ASSERT_EFI_ERROR (Status);
  return Status;
}

/**
  This function is updating the instance with RSDP and RSDT, these are steps in the constructor that will be skipped if this HOB is available.

  @param  AcpiTableInstance  Protocol instance private data.
  @param  GuidHob            GUID HOB header.

  @return EFI_SUCCESS        The function completed successfully.
  @return EFI_NOT_FOUND      The function doesn't find the Rsdp from AcpiSiliconHob.
  @return EFI_ABORTED        The function could not complete successfully.

**/
EFI_STATUS
InstallAcpiTableFromAcpiSiliconHob (
  EFI_ACPI_TABLE_INSTANCE  *AcpiTableInstance,
  EFI_HOB_GUID_TYPE        *GuidHob
  )
{
  ACPI_SILICON_HOB                              *AcpiSiliconHob;
  EFI_ACPI_3_0_ROOT_SYSTEM_DESCRIPTION_POINTER  *SiAcpiHobRsdp;
  EFI_ACPI_DESCRIPTION_HEADER                   *SiCommonAcpiTable;
  EFI_STATUS                                    Status;
  UINT8                                         *TempBuffer;
  UINTN                                         NumOfTblEntries;

  DEBUG ((DEBUG_INFO, "InstallAcpiTableFromAcpiSiliconHob\n"));
  //
  // Initial variable.
  //
  SiAcpiHobRsdp     = NULL;
  SiCommonAcpiTable = NULL;
  AcpiSiliconHob    = GET_GUID_HOB_DATA (GuidHob);
  Status            = EFI_SUCCESS;
  //
  // Got RSDP table from ACPI Silicon Hob.
  //
  SiAcpiHobRsdp = (EFI_ACPI_3_0_ROOT_SYSTEM_DESCRIPTION_POINTER *)(UINTN)(AcpiSiliconHob->Rsdp);
  if (SiAcpiHobRsdp == NULL) {
    DEBUG ((DEBUG_ERROR, "InstallAcpiTableFromAcpiSiliconHob: Fail to locate RSDP Acpi table!!\n"));
    return EFI_NOT_FOUND;
  }

  DEBUG ((DEBUG_INFO, "Silicon ACPI RSDP address : 0x%lx\n", SiAcpiHobRsdp));
  AcpiTableInstance->Rsdp3 = SiAcpiHobRsdp;

  if (SiAcpiHobRsdp->RsdtAddress != 0x00000000) {
    //
    // Initial RSDT.
    //
    TempBuffer               = (UINT8 *)(UINTN)(SiAcpiHobRsdp->RsdtAddress);
    SiCommonAcpiTable        = (EFI_ACPI_DESCRIPTION_HEADER *)TempBuffer;
    AcpiTableInstance->Rsdt3 = SiCommonAcpiTable;

    if (SiCommonAcpiTable->Length <= sizeof (EFI_ACPI_DESCRIPTION_HEADER)) {
      DEBUG ((DEBUG_ERROR, "RSDT length is incorrect\n"));
      return EFI_ABORTED;
    }

    //
    // Calcaue 32bit Acpi table number.
    //
    NumOfTblEntries                          = (SiCommonAcpiTable->Length - sizeof (EFI_ACPI_DESCRIPTION_HEADER)) / sizeof (UINT32);
    AcpiTableInstance->NumberOfTableEntries1 = NumOfTblEntries;
    DEBUG ((DEBUG_INFO, "32bit NumOfTblEntries : 0x%x\n", NumOfTblEntries));
    //
    // Enlarge the max table number from mEfiAcpiMaxNumTables to current ACPI tables + EFI_ACPI_MAX_NUM_TABLES
    //
    if (AcpiTableInstance->NumberOfTableEntries1 >= EFI_ACPI_MAX_NUM_TABLES) {
      mEfiAcpiMaxNumTables = AcpiTableInstance->NumberOfTableEntries1 + EFI_ACPI_MAX_NUM_TABLES;
      DEBUG ((DEBUG_ERROR, "mEfiAcpiMaxNumTables : 0x%x\n", mEfiAcpiMaxNumTables));
    }
  } else {
    //
    // Initial XSDT.
    //
    TempBuffer              = (UINT8 *)(UINTN)(SiAcpiHobRsdp->XsdtAddress);
    SiCommonAcpiTable       = (EFI_ACPI_DESCRIPTION_HEADER *)TempBuffer;
    AcpiTableInstance->Xsdt = SiCommonAcpiTable;

    if (SiCommonAcpiTable->Length <= sizeof (EFI_ACPI_DESCRIPTION_HEADER)) {
      DEBUG ((DEBUG_ERROR, "XSDT length is incorrect\n"));
      return EFI_ABORTED;
    }

    //
    // Calcaue 64bit Acpi table number.
    //
    NumOfTblEntries                          = (SiCommonAcpiTable->Length - sizeof (EFI_ACPI_DESCRIPTION_HEADER)) / sizeof (UINT64);
    AcpiTableInstance->NumberOfTableEntries3 = NumOfTblEntries;
    DEBUG ((DEBUG_ERROR, "64bit NumOfTblEntries : 0x%x\n", NumOfTblEntries));
    //
    // Enlarge the max table number from mEfiAcpiMaxNumTables to current ACPI tables + EFI_ACPI_MAX_NUM_TABLES
    //
    if (AcpiTableInstance->NumberOfTableEntries3 >= EFI_ACPI_MAX_NUM_TABLES) {
      mEfiAcpiMaxNumTables = AcpiTableInstance->NumberOfTableEntries3 + EFI_ACPI_MAX_NUM_TABLES;
      DEBUG ((DEBUG_ERROR, "mEfiAcpiMaxNumTables : 0x%x\n", mEfiAcpiMaxNumTables));
    }
  }

  return Status;
}

/**
  Constructor for the ACPI table protocol.  Initializes instance
  data.

  @param  AcpiTableInstance   Instance to construct

  @return EFI_SUCCESS             Instance initialized.
  @return EFI_OUT_OF_RESOURCES    Unable to allocate required resources.

**/
EFI_STATUS
AcpiTableAcpiTableConstructor (
  EFI_ACPI_TABLE_INSTANCE  *AcpiTableInstance
  )
{
  EFI_STATUS            Status;
  UINT64                CurrentData;
  UINTN                 TotalSize;
  UINTN                 RsdpTableSize;
  UINT8                 *Pointer;
  EFI_PHYSICAL_ADDRESS  PageAddress;
  EFI_MEMORY_TYPE       AcpiAllocateMemoryType;
  EFI_HOB_GUID_TYPE     *GuidHob;

  //
  // Check for invalid input parameters
  //
  ASSERT (AcpiTableInstance);

  //
  // If ACPI v1.0b is among the ACPI versions we aim to support, we have to
  // ensure that all memory allocations are below 4 GB.
  //
  if ((PcdGet32 (PcdAcpiExposedTableVersions) & EFI_ACPI_TABLE_VERSION_1_0B) != 0) {
    mAcpiTableAllocType = AllocateMaxAddress;
  } else {
    mAcpiTableAllocType = AllocateAnyPages;
  }

  InitializeListHead (&AcpiTableInstance->TableList);
  AcpiTableInstance->CurrentHandle = 1;

  AcpiTableInstance->AcpiTableProtocol.InstallAcpiTable   = InstallAcpiTable;
  AcpiTableInstance->AcpiTableProtocol.UninstallAcpiTable = UninstallAcpiTable;

  if (FeaturePcdGet (PcdInstallAcpiSdtProtocol)) {
    SdtAcpiTableAcpiSdtConstructor (AcpiTableInstance);
  }

  //
  // Check Silicon ACPI Hob.
  //
  GuidHob = GetFirstGuidHob (&gAcpiTableHobGuid);
  if (GuidHob != NULL) {
    Status = InstallAcpiTableFromAcpiSiliconHob (AcpiTableInstance, GuidHob);
    if (Status == EFI_SUCCESS) {
      DEBUG ((DEBUG_INFO, "Installed ACPI Table from AcpiSiliconHob.\n"));
      return EFI_SUCCESS;
    } else {
      DEBUG ((DEBUG_ERROR, "Fail to Installed ACPI Table from AcpiSiliconHob!!\n"));
      ASSERT (Status != EFI_SUCCESS);
    }
  } else {
    DEBUG ((DEBUG_INFO, "Fail to locate AcpiSiliconHob!!\n"));
  }

  //
  // Create RSDP table
  //
  RsdpTableSize = sizeof (EFI_ACPI_3_0_ROOT_SYSTEM_DESCRIPTION_POINTER);
  if ((PcdGet32 (PcdAcpiExposedTableVersions) & EFI_ACPI_TABLE_VERSION_1_0B) != 0) {
    RsdpTableSize += sizeof (EFI_ACPI_1_0_ROOT_SYSTEM_DESCRIPTION_POINTER);
  }

  if (PcdGetBool (PcdNoACPIReclaimMemory)) {
    AcpiAllocateMemoryType = EfiACPIMemoryNVS;
  } else {
    AcpiAllocateMemoryType = EfiACPIReclaimMemory;
  }

  if (mAcpiTableAllocType != AllocateAnyPages) {
    PageAddress = 0xFFFFFFFF;
    Status      = gBS->AllocatePages (
                         mAcpiTableAllocType,
                         AcpiAllocateMemoryType,
                         EFI_SIZE_TO_PAGES (RsdpTableSize),
                         &PageAddress
                         );
  } else {
    Status = gBS->AllocatePool (
                    AcpiAllocateMemoryType,
                    RsdpTableSize,
                    (VOID **)&Pointer
                    );
  }

  if (EFI_ERROR (Status)) {
    return EFI_OUT_OF_RESOURCES;
  }

  if (mAcpiTableAllocType != AllocateAnyPages) {
    Pointer = (UINT8 *)(UINTN)PageAddress;
  }

  ZeroMem (Pointer, RsdpTableSize);

  AcpiTableInstance->Rsdp1 = (EFI_ACPI_1_0_ROOT_SYSTEM_DESCRIPTION_POINTER *)Pointer;
  if ((PcdGet32 (PcdAcpiExposedTableVersions) & EFI_ACPI_TABLE_VERSION_1_0B) != 0) {
    Pointer += sizeof (EFI_ACPI_1_0_ROOT_SYSTEM_DESCRIPTION_POINTER);
  }

  AcpiTableInstance->Rsdp3 = (EFI_ACPI_3_0_ROOT_SYSTEM_DESCRIPTION_POINTER *)Pointer;

  //
  // Create RSDT, XSDT structures
  //
  TotalSize = sizeof (EFI_ACPI_DESCRIPTION_HEADER) +         // for ACPI 2.0/3.0 XSDT
              mEfiAcpiMaxNumTables * sizeof (UINT64);

  if ((PcdGet32 (PcdAcpiExposedTableVersions) & EFI_ACPI_TABLE_VERSION_1_0B) != 0) {
    TotalSize += sizeof (EFI_ACPI_DESCRIPTION_HEADER) +      // for ACPI 1.0 RSDT
                 mEfiAcpiMaxNumTables * sizeof (UINT32) +
                 sizeof (EFI_ACPI_DESCRIPTION_HEADER) +      // for ACPI 2.0/3.0 RSDT
                 mEfiAcpiMaxNumTables * sizeof (UINT32);
  }

  if (mAcpiTableAllocType != AllocateAnyPages) {
    //
    // Allocate memory in the lower 32 bit of address range for
    // compatibility with ACPI 1.0 OS.
    //
    // This is done because ACPI 1.0 pointers are 32 bit values.
    // ACPI 2.0 OS and all 64 bit OS must use the 64 bit ACPI table addresses.
    // There is no architectural reason these should be below 4GB, it is purely
    // for convenience of implementation that we force memory below 4GB.
    //
    PageAddress = 0xFFFFFFFF;
    Status      = gBS->AllocatePages (
                         mAcpiTableAllocType,
                         AcpiAllocateMemoryType,
                         EFI_SIZE_TO_PAGES (TotalSize),
                         &PageAddress
                         );
  } else {
    Status = gBS->AllocatePool (
                    AcpiAllocateMemoryType,
                    TotalSize,
                    (VOID **)&Pointer
                    );
  }

  if (EFI_ERROR (Status)) {
    if (mAcpiTableAllocType != AllocateAnyPages) {
      gBS->FreePages (
             (EFI_PHYSICAL_ADDRESS)(UINTN)AcpiTableInstance->Rsdp1,
             EFI_SIZE_TO_PAGES (RsdpTableSize)
             );
    } else {
      gBS->FreePool (AcpiTableInstance->Rsdp1);
    }

    return EFI_OUT_OF_RESOURCES;
  }

  if (mAcpiTableAllocType != AllocateAnyPages) {
    Pointer = (UINT8 *)(UINTN)PageAddress;
  }

  ZeroMem (Pointer, TotalSize);

  AcpiTableInstance->Rsdt1 = (EFI_ACPI_DESCRIPTION_HEADER *)Pointer;
  if ((PcdGet32 (PcdAcpiExposedTableVersions) & EFI_ACPI_TABLE_VERSION_1_0B) != 0) {
    Pointer                 += (sizeof (EFI_ACPI_DESCRIPTION_HEADER) + EFI_ACPI_MAX_NUM_TABLES * sizeof (UINT32));
    AcpiTableInstance->Rsdt3 = (EFI_ACPI_DESCRIPTION_HEADER *)Pointer;
    Pointer                 += (sizeof (EFI_ACPI_DESCRIPTION_HEADER) + EFI_ACPI_MAX_NUM_TABLES * sizeof (UINT32));
  }

  AcpiTableInstance->Xsdt = (EFI_ACPI_DESCRIPTION_HEADER *)Pointer;

  //
  // Initialize RSDP
  //
  if ((PcdGet32 (PcdAcpiExposedTableVersions) & EFI_ACPI_TABLE_VERSION_1_0B) != 0) {
    CurrentData = EFI_ACPI_1_0_ROOT_SYSTEM_DESCRIPTION_POINTER_SIGNATURE;
    CopyMem (&AcpiTableInstance->Rsdp1->Signature, &CurrentData, sizeof (UINT64));
    CopyMem (AcpiTableInstance->Rsdp1->OemId, PcdGetPtr (PcdAcpiDefaultOemId), sizeof (AcpiTableInstance->Rsdp1->OemId));
    AcpiTableInstance->Rsdp1->Reserved    = EFI_ACPI_RESERVED_BYTE;
    AcpiTableInstance->Rsdp1->RsdtAddress = (UINT32)(UINTN)AcpiTableInstance->Rsdt1;
  }

  CurrentData = EFI_ACPI_3_0_ROOT_SYSTEM_DESCRIPTION_POINTER_SIGNATURE;
  CopyMem (&AcpiTableInstance->Rsdp3->Signature, &CurrentData, sizeof (UINT64));
  CopyMem (AcpiTableInstance->Rsdp3->OemId, PcdGetPtr (PcdAcpiDefaultOemId), sizeof (AcpiTableInstance->Rsdp3->OemId));
  AcpiTableInstance->Rsdp3->Revision = EFI_ACPI_3_0_ROOT_SYSTEM_DESCRIPTION_POINTER_REVISION;
  AcpiTableInstance->Rsdp3->Length   = sizeof (EFI_ACPI_3_0_ROOT_SYSTEM_DESCRIPTION_POINTER);
  if ((PcdGet32 (PcdAcpiExposedTableVersions) & EFI_ACPI_TABLE_VERSION_1_0B) != 0) {
    AcpiTableInstance->Rsdp3->RsdtAddress = (UINT32)(UINTN)AcpiTableInstance->Rsdt3;
  }

  CurrentData = (UINT64)(UINTN)AcpiTableInstance->Xsdt;
  CopyMem (&AcpiTableInstance->Rsdp3->XsdtAddress, &CurrentData, sizeof (UINT64));
  SetMem (AcpiTableInstance->Rsdp3->Reserved, 3, EFI_ACPI_RESERVED_BYTE);

  if ((PcdGet32 (PcdAcpiExposedTableVersions) & EFI_ACPI_TABLE_VERSION_1_0B) != 0) {
    //
    // Initialize Rsdt
    //
    // Note that we "reserve" one entry for the FADT so it can always be
    // at the beginning of the list of tables.  Some OS don't seem
    // to find it correctly if it is too far down the list.
    //
    AcpiTableInstance->Rsdt1->Signature = EFI_ACPI_1_0_ROOT_SYSTEM_DESCRIPTION_TABLE_SIGNATURE;
    AcpiTableInstance->Rsdt1->Length    = sizeof (EFI_ACPI_DESCRIPTION_HEADER);
    AcpiTableInstance->Rsdt1->Revision  = EFI_ACPI_1_0_ROOT_SYSTEM_DESCRIPTION_TABLE_REVISION;
    CopyMem (AcpiTableInstance->Rsdt1->OemId, PcdGetPtr (PcdAcpiDefaultOemId), sizeof (AcpiTableInstance->Rsdt1->OemId));
    CurrentData = PcdGet64 (PcdAcpiDefaultOemTableId);
    CopyMem (&AcpiTableInstance->Rsdt1->OemTableId, &CurrentData, sizeof (UINT64));
    AcpiTableInstance->Rsdt1->OemRevision     = PcdGet32 (PcdAcpiDefaultOemRevision);
    AcpiTableInstance->Rsdt1->CreatorId       = PcdGet32 (PcdAcpiDefaultCreatorId);
    AcpiTableInstance->Rsdt1->CreatorRevision = PcdGet32 (PcdAcpiDefaultCreatorRevision);
    //
    // We always reserve first one for FADT
    //
    AcpiTableInstance->NumberOfTableEntries1 = 1;
    AcpiTableInstance->Rsdt1->Length         = AcpiTableInstance->Rsdt1->Length + sizeof (UINT32);

    AcpiTableInstance->Rsdt3->Signature = EFI_ACPI_3_0_ROOT_SYSTEM_DESCRIPTION_TABLE_SIGNATURE;
    AcpiTableInstance->Rsdt3->Length    = sizeof (EFI_ACPI_DESCRIPTION_HEADER);
    AcpiTableInstance->Rsdt3->Revision  = EFI_ACPI_3_0_ROOT_SYSTEM_DESCRIPTION_TABLE_REVISION;
    CopyMem (AcpiTableInstance->Rsdt3->OemId, PcdGetPtr (PcdAcpiDefaultOemId), sizeof (AcpiTableInstance->Rsdt3->OemId));
    CurrentData = PcdGet64 (PcdAcpiDefaultOemTableId);
    CopyMem (&AcpiTableInstance->Rsdt3->OemTableId, &CurrentData, sizeof (UINT64));
    AcpiTableInstance->Rsdt3->OemRevision     = PcdGet32 (PcdAcpiDefaultOemRevision);
    AcpiTableInstance->Rsdt3->CreatorId       = PcdGet32 (PcdAcpiDefaultCreatorId);
    AcpiTableInstance->Rsdt3->CreatorRevision = PcdGet32 (PcdAcpiDefaultCreatorRevision);
    //
    // We always reserve first one for FADT
    //
    AcpiTableInstance->Rsdt3->Length = AcpiTableInstance->Rsdt3->Length + sizeof (UINT32);
  }

  AcpiTableInstance->NumberOfTableEntries3 = 1;

  //
  // Initialize Xsdt
  //
  AcpiTableInstance->Xsdt->Signature = EFI_ACPI_3_0_EXTENDED_SYSTEM_DESCRIPTION_TABLE_SIGNATURE;
  AcpiTableInstance->Xsdt->Length    = sizeof (EFI_ACPI_DESCRIPTION_HEADER);
  AcpiTableInstance->Xsdt->Revision  = EFI_ACPI_3_0_EXTENDED_SYSTEM_DESCRIPTION_TABLE_REVISION;
  CopyMem (AcpiTableInstance->Xsdt->OemId, PcdGetPtr (PcdAcpiDefaultOemId), sizeof (AcpiTableInstance->Xsdt->OemId));
  CurrentData = PcdGet64 (PcdAcpiDefaultOemTableId);
  CopyMem (&AcpiTableInstance->Xsdt->OemTableId, &CurrentData, sizeof (UINT64));
  AcpiTableInstance->Xsdt->OemRevision     = PcdGet32 (PcdAcpiDefaultOemRevision);
  AcpiTableInstance->Xsdt->CreatorId       = PcdGet32 (PcdAcpiDefaultCreatorId);
  AcpiTableInstance->Xsdt->CreatorRevision = PcdGet32 (PcdAcpiDefaultCreatorRevision);
  //
  // We always reserve first one for FADT
  //
  AcpiTableInstance->Xsdt->Length = AcpiTableInstance->Xsdt->Length + sizeof (UINT64);

  ChecksumCommonTables (AcpiTableInstance);

  InstallAcpiTableFromHob (AcpiTableInstance);

  //
  // Completed successfully
  //
  return EFI_SUCCESS;
}
