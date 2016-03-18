/** @file
  System Management System Table Services SmmInstallConfigurationTable service

  Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials are licensed and made available 
  under the terms and conditions of the BSD License which accompanies this 
  distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#include "PiSmmCore.h"

#define CONFIG_TABLE_SIZE_INCREASED 0x10

UINTN  mSmmSystemTableAllocateSize = 0;

/**
  The SmmInstallConfigurationTable() function is used to maintain the list
  of configuration tables that are stored in the System Management System
  Table.  The list is stored as an array of (GUID, Pointer) pairs.  The list
  must be allocated from pool memory with PoolType set to EfiRuntimeServicesData.

  @param  SystemTable      A pointer to the SMM System Table (SMST).
  @param  Guid             A pointer to the GUID for the entry to add, update, or remove.
  @param  Table            A pointer to the buffer of the table to add.
  @param  TableSize        The size of the table to install.

  @retval EFI_SUCCESS           The (Guid, Table) pair was added, updated, or removed.
  @retval EFI_INVALID_PARAMETER Guid is not valid.
  @retval EFI_NOT_FOUND         An attempt was made to delete a non-existent entry.
  @retval EFI_OUT_OF_RESOURCES  There is not enough memory available to complete the operation.

**/
EFI_STATUS
EFIAPI
SmmInstallConfigurationTable (
  IN  CONST EFI_SMM_SYSTEM_TABLE2  *SystemTable,
  IN  CONST EFI_GUID               *Guid,
  IN  VOID                         *Table,
  IN  UINTN                        TableSize
  )
{
  UINTN                    Index;
  EFI_CONFIGURATION_TABLE  *ConfigurationTable;

  //
  // If Guid is NULL, then this operation cannot be performed
  //
  if (Guid == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  ConfigurationTable = gSmmCoreSmst.SmmConfigurationTable;

  //
  // Search all the table for an entry that matches Guid
  //
  for (Index = 0; Index < gSmmCoreSmst.NumberOfTableEntries; Index++) {
    if (CompareGuid (Guid, &(ConfigurationTable[Index].VendorGuid))) {
      break;
    }
  }

  if (Index < gSmmCoreSmst.NumberOfTableEntries) {
    //
    // A match was found, so this is either a modify or a delete operation
    //
    if (Table != NULL) {
      //
      // If Table is not NULL, then this is a modify operation.
      // Modify the table enty and return.
      //
      ConfigurationTable[Index].VendorTable = Table;
      return EFI_SUCCESS;
    }

    //
    // A match was found and Table is NULL, so this is a delete operation.
    //
    gSmmCoreSmst.NumberOfTableEntries--;

    //
    // Copy over deleted entry
    //
    CopyMem (
      &(ConfigurationTable[Index]),
      &(ConfigurationTable[Index + 1]),
      (gSmmCoreSmst.NumberOfTableEntries - Index) * sizeof (EFI_CONFIGURATION_TABLE)
      );

  } else {
    //
    // No matching GUIDs were found, so this is an add operation.
    //
    if (Table == NULL) {
      //
      // If Table is NULL on an add operation, then return an error.
      //
      return EFI_NOT_FOUND;
    }

    //
    // Assume that Index == gSmmCoreSmst.NumberOfTableEntries
    //
    if ((Index * sizeof (EFI_CONFIGURATION_TABLE)) >= mSmmSystemTableAllocateSize) {
      //
      // Allocate a table with one additional entry.
      //
      mSmmSystemTableAllocateSize += (CONFIG_TABLE_SIZE_INCREASED * sizeof (EFI_CONFIGURATION_TABLE));
      ConfigurationTable = AllocatePool (mSmmSystemTableAllocateSize);
      if (ConfigurationTable == NULL) {
        //
        // If a new table could not be allocated, then return an error.
        //
        return EFI_OUT_OF_RESOURCES;
      }

      if (gSmmCoreSmst.SmmConfigurationTable != NULL) {
        //
        // Copy the old table to the new table.
        //
        CopyMem (
          ConfigurationTable,
          gSmmCoreSmst.SmmConfigurationTable,
          Index * sizeof (EFI_CONFIGURATION_TABLE)
          );

        //
        // Free Old Table
        //
        FreePool (gSmmCoreSmst.SmmConfigurationTable);
      }

      //
      // Update System Table
      //
      gSmmCoreSmst.SmmConfigurationTable = ConfigurationTable;
    }

    //
    // Fill in the new entry
    //
    CopyGuid ((VOID *)&ConfigurationTable[Index].VendorGuid, Guid);
    ConfigurationTable[Index].VendorTable = Table;

    //
    // This is an add operation, so increment the number of table entries
    //
    gSmmCoreSmst.NumberOfTableEntries++;
  }

  //
  // CRC-32 field is ignorable for SMM System Table and should be set to zero
  //

  return EFI_SUCCESS;
}
