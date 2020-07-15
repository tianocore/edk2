/** @file
  Support functions for managing debug image info table when loading and unloading
  images.

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "DxeMain.h"


EFI_DEBUG_IMAGE_INFO_TABLE_HEADER  mDebugInfoTableHeader = {
  0,          // volatile UINT32                 UpdateStatus;
  0,          // UINT32                          TableSize;
  NULL        // EFI_DEBUG_IMAGE_INFO            *EfiDebugImageInfoTable;
};

UINTN mMaxTableEntries = 0;

EFI_SYSTEM_TABLE_POINTER  *mDebugTable = NULL;

#define EFI_DEBUG_TABLE_ENTRY_SIZE       (sizeof (VOID *))

/**
  Creates and initializes the DebugImageInfo Table.  Also creates the configuration
  table and registers it into the system table.

**/
VOID
CoreInitializeDebugImageInfoTable (
  VOID
  )
{
  EFI_STATUS            Status;
  UINTN                 Pages;
  EFI_PHYSICAL_ADDRESS  Memory;
  UINTN                 AlignedMemory;
  UINTN                 AlignmentMask;
  UINTN                 UnalignedPages;
  UINTN                 RealPages;

  //
  // Allocate 4M aligned page for the structure and fill in the data.
  // Ideally we would update the CRC now as well, but the service may not yet be available.
  // See comments in the CoreUpdateDebugTableCrc32() function below for details.
  //
  Pages          = EFI_SIZE_TO_PAGES (sizeof (EFI_SYSTEM_TABLE_POINTER));
  AlignmentMask  = SIZE_4MB - 1;
  RealPages      = Pages + EFI_SIZE_TO_PAGES (SIZE_4MB);

  //
  // Attempt to allocate memory below PcdMaxEfiSystemTablePointerAddress
  // If PcdMaxEfiSystemTablePointerAddress is 0, then allocate memory below
  // MAX_ADDRESS
  //
  Memory = PcdGet64 (PcdMaxEfiSystemTablePointerAddress);
  if (Memory == 0) {
    Memory = MAX_ADDRESS;
  }
  Status = CoreAllocatePages (
             AllocateMaxAddress,
             EfiBootServicesData,
             RealPages,
             &Memory
             );
  if (EFI_ERROR (Status)) {
    if (PcdGet64 (PcdMaxEfiSystemTablePointerAddress) != 0) {
      DEBUG ((EFI_D_INFO, "Allocate memory for EFI_SYSTEM_TABLE_POINTER below PcdMaxEfiSystemTablePointerAddress failed. \
                          Retry to allocate memroy as close to the top of memory as feasible.\n"));
    }
    //
    // If the initial memory allocation fails, then reattempt allocation
    // as close to the top of memory as feasible.
    //
    Status = CoreAllocatePages (
               AllocateAnyPages,
               EfiBootServicesData,
               RealPages,
               &Memory
               );
    ASSERT_EFI_ERROR (Status);
    if (EFI_ERROR (Status)) {
      return;
    }
  }

  //
  // Free overallocated pages
  //
  AlignedMemory  = ((UINTN) Memory + AlignmentMask) & ~AlignmentMask;
  UnalignedPages = EFI_SIZE_TO_PAGES (AlignedMemory - (UINTN)Memory);
  if (UnalignedPages > 0) {
    //
    // Free first unaligned page(s).
    //
    Status = CoreFreePages (Memory, UnalignedPages);
    ASSERT_EFI_ERROR (Status);
  }
  Memory         = AlignedMemory + EFI_PAGES_TO_SIZE (Pages);
  UnalignedPages = RealPages - Pages - UnalignedPages;
  if (UnalignedPages > 0) {
    //
    // Free last unaligned page(s).
    //
    Status = CoreFreePages (Memory, UnalignedPages);
    ASSERT_EFI_ERROR (Status);
  }

  //
  // Set mDebugTable to the 4MB aligned allocated pages
  //
  mDebugTable = (EFI_SYSTEM_TABLE_POINTER  *)(AlignedMemory);
  ASSERT (mDebugTable != NULL);

  //
  // Initialize EFI_SYSTEM_TABLE_POINTER structure
  //
  mDebugTable->Signature          = EFI_SYSTEM_TABLE_SIGNATURE;
  mDebugTable->EfiSystemTableBase = (EFI_PHYSICAL_ADDRESS) (UINTN) gDxeCoreST;
  mDebugTable->Crc32              = 0;

  //
  // Install the EFI_SYSTEM_TABLE_POINTER structure in the EFI System
  // Configuration Table
  //
  Status = CoreInstallConfigurationTable (&gEfiDebugImageInfoTableGuid, &mDebugInfoTableHeader);
  ASSERT_EFI_ERROR (Status);
}


/**
  Update the CRC32 in the Debug Table.
  Since the CRC32 service is made available by the Runtime driver, we have to
  wait for the Runtime Driver to be installed before the CRC32 can be computed.
  This function is called elsewhere by the core when the runtime architectural
  protocol is produced.

**/
VOID
CoreUpdateDebugTableCrc32 (
  VOID
  )
{
  ASSERT(mDebugTable != NULL);
  mDebugTable->Crc32 = 0;
  gBS->CalculateCrc32 ((VOID *)mDebugTable, sizeof (EFI_SYSTEM_TABLE_POINTER), &mDebugTable->Crc32);
}


/**
  Adds a new DebugImageInfo structure to the DebugImageInfo Table.  Re-Allocates
  the table if it's not large enough to accomidate another entry.

  @param  ImageInfoType  type of debug image information
  @param  LoadedImage    pointer to the loaded image protocol for the image being
                         loaded
  @param  ImageHandle    image handle for the image being loaded

**/
VOID
CoreNewDebugImageInfoEntry (
  IN  UINT32                      ImageInfoType,
  IN  EFI_LOADED_IMAGE_PROTOCOL   *LoadedImage,
  IN  EFI_HANDLE                  ImageHandle
  )
{
  EFI_DEBUG_IMAGE_INFO      *Table;
  EFI_DEBUG_IMAGE_INFO      *NewTable;
  UINTN                     Index;
  UINTN                     TableSize;

  //
  // Set the flag indicating that we're in the process of updating the table.
  //
  mDebugInfoTableHeader.UpdateStatus |= EFI_DEBUG_IMAGE_INFO_UPDATE_IN_PROGRESS;

  Table = mDebugInfoTableHeader.EfiDebugImageInfoTable;

  if (mDebugInfoTableHeader.TableSize < mMaxTableEntries) {
    //
    // We still have empty entires in the Table, find the first empty entry.
    //
    Index = 0;
    while (Table[Index].NormalImage != NULL) {
      Index++;
    }
    //
    // There must be an empty entry in the in the table.
    //
    ASSERT (Index < mMaxTableEntries);
  } else {
    //
    //  Table is full, so re-allocate another page for a larger table...
    //
    TableSize = mMaxTableEntries * EFI_DEBUG_TABLE_ENTRY_SIZE;
    NewTable = AllocateZeroPool (TableSize + EFI_PAGE_SIZE);
    if (NewTable == NULL) {
      mDebugInfoTableHeader.UpdateStatus &= ~EFI_DEBUG_IMAGE_INFO_UPDATE_IN_PROGRESS;
      return;
    }
    //
    // Copy the old table into the new one
    //
    CopyMem (NewTable, Table, TableSize);
    //
    // Free the old table
    //
    CoreFreePool (Table);
    //
    // Update the table header
    //
    Table = NewTable;
    mDebugInfoTableHeader.EfiDebugImageInfoTable = NewTable;
    //
    // Enlarge the max table entries and set the first empty entry index to
    // be the original max table entries.
    //
    Index             = mMaxTableEntries;
    mMaxTableEntries += EFI_PAGE_SIZE / EFI_DEBUG_TABLE_ENTRY_SIZE;
  }

  //
  // Allocate data for new entry
  //
  Table[Index].NormalImage = AllocateZeroPool (sizeof (EFI_DEBUG_IMAGE_INFO_NORMAL));
  if (Table[Index].NormalImage != NULL) {
    //
    // Update the entry
    //
    Table[Index].NormalImage->ImageInfoType               = (UINT32) ImageInfoType;
    Table[Index].NormalImage->LoadedImageProtocolInstance = LoadedImage;
    Table[Index].NormalImage->ImageHandle                 = ImageHandle;
    //
    // Increase the number of EFI_DEBUG_IMAGE_INFO elements and set the mDebugInfoTable in modified status.
    //
    mDebugInfoTableHeader.TableSize++;
    mDebugInfoTableHeader.UpdateStatus |= EFI_DEBUG_IMAGE_INFO_TABLE_MODIFIED;
  }
  mDebugInfoTableHeader.UpdateStatus &= ~EFI_DEBUG_IMAGE_INFO_UPDATE_IN_PROGRESS;
}



/**
  Removes and frees an entry from the DebugImageInfo Table.

  @param  ImageHandle    image handle for the image being unloaded

**/
VOID
CoreRemoveDebugImageInfoEntry (
  EFI_HANDLE ImageHandle
  )
{
  EFI_DEBUG_IMAGE_INFO  *Table;
  UINTN                 Index;

  mDebugInfoTableHeader.UpdateStatus |= EFI_DEBUG_IMAGE_INFO_UPDATE_IN_PROGRESS;

  Table = mDebugInfoTableHeader.EfiDebugImageInfoTable;

  for (Index = 0; Index < mMaxTableEntries; Index++) {
    if (Table[Index].NormalImage != NULL && Table[Index].NormalImage->ImageHandle == ImageHandle) {
      //
      // Found a match. Free up the record, then NULL the pointer to indicate the slot
      // is free.
      //
      CoreFreePool (Table[Index].NormalImage);
      Table[Index].NormalImage = NULL;
      //
      // Decrease the number of EFI_DEBUG_IMAGE_INFO elements and set the mDebugInfoTable in modified status.
      //
      mDebugInfoTableHeader.TableSize--;
      mDebugInfoTableHeader.UpdateStatus |= EFI_DEBUG_IMAGE_INFO_TABLE_MODIFIED;
      break;
    }
  }
  mDebugInfoTableHeader.UpdateStatus &= ~EFI_DEBUG_IMAGE_INFO_UPDATE_IN_PROGRESS;
}


