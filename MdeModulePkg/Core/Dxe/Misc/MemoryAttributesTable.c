/** @file
  UEFI MemoryAttributesTable support

Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiDxe.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiLib.h>

#include <Guid/EventGroup.h>

#include <Guid/MemoryAttributesTable.h>
#include <Guid/PropertiesTable.h>

#include "DxeMain.h"

/**
  This function for GetMemoryMap() with properties table capability.

  It calls original GetMemoryMap() to get the original memory map information. Then
  plus the additional memory map entries for PE Code/Data seperation.

  @param  MemoryMapSize          A pointer to the size, in bytes, of the
                                 MemoryMap buffer. On input, this is the size of
                                 the buffer allocated by the caller.  On output,
                                 it is the size of the buffer returned by the
                                 firmware  if the buffer was large enough, or the
                                 size of the buffer needed  to contain the map if
                                 the buffer was too small.
  @param  MemoryMap              A pointer to the buffer in which firmware places
                                 the current memory map.
  @param  MapKey                 A pointer to the location in which firmware
                                 returns the key for the current memory map.
  @param  DescriptorSize         A pointer to the location in which firmware
                                 returns the size, in bytes, of an individual
                                 EFI_MEMORY_DESCRIPTOR.
  @param  DescriptorVersion      A pointer to the location in which firmware
                                 returns the version number associated with the
                                 EFI_MEMORY_DESCRIPTOR.

  @retval EFI_SUCCESS            The memory map was returned in the MemoryMap
                                 buffer.
  @retval EFI_BUFFER_TOO_SMALL   The MemoryMap buffer was too small. The current
                                 buffer size needed to hold the memory map is
                                 returned in MemoryMapSize.
  @retval EFI_INVALID_PARAMETER  One of the parameters has an invalid value.

**/
EFI_STATUS
EFIAPI
CoreGetMemoryMapWithSeparatedImageSection (
  IN OUT UINTN                  *MemoryMapSize,
  IN OUT EFI_MEMORY_DESCRIPTOR  *MemoryMap,
  OUT UINTN                     *MapKey,
  OUT UINTN                     *DescriptorSize,
  OUT UINT32                    *DescriptorVersion
  );

extern EFI_PROPERTIES_TABLE  mPropertiesTable;
EFI_MEMORY_ATTRIBUTES_TABLE  *mMemoryAttributesTable = NULL;
BOOLEAN                      mMemoryAttributesTableReadyToBoot = FALSE;

/**
  Install MemoryAttributesTable.

**/
VOID
InstallMemoryAttributesTable (
  VOID
  )
{
  UINTN                          MemoryMapSize;
  EFI_MEMORY_DESCRIPTOR          *MemoryMap;
  EFI_MEMORY_DESCRIPTOR          *MemoryMapStart;
  UINTN                          MapKey;
  UINTN                          DescriptorSize;
  UINT32                         DescriptorVersion;
  UINTN                          Index;
  EFI_STATUS                     Status;
  UINT32                         RuntimeEntryCount;
  EFI_MEMORY_ATTRIBUTES_TABLE    *MemoryAttributesTable;
  EFI_MEMORY_DESCRIPTOR          *MemoryAttributesEntry;

  if (gMemoryMapTerminated) {
    //
    // Directly return after MemoryMap terminated.
    //
    return;
  }

  if ((mPropertiesTable.MemoryProtectionAttribute & EFI_PROPERTIES_RUNTIME_MEMORY_PROTECTION_NON_EXECUTABLE_PE_DATA) == 0) {
    DEBUG ((EFI_D_VERBOSE, "MemoryProtectionAttribute NON_EXECUTABLE_PE_DATA is not set, "));
    DEBUG ((EFI_D_VERBOSE, "because Runtime Driver Section Alignment is not %dK.\n", RUNTIME_PAGE_ALLOCATION_GRANULARITY >> 10));
    return ;
  }

  if (mMemoryAttributesTable == NULL) {
    //
    // InstallConfigurationTable here to occupy one entry for MemoryAttributesTable
    // before GetMemoryMap below, as InstallConfigurationTable may allocate runtime
    // memory for the new entry.
    //
    Status = gBS->InstallConfigurationTable (&gEfiMemoryAttributesTableGuid, (VOID *) (UINTN) MAX_ADDRESS);
    ASSERT_EFI_ERROR (Status);
  }

  MemoryMapSize = 0;
  MemoryMap = NULL;
  Status = CoreGetMemoryMapWithSeparatedImageSection (
             &MemoryMapSize,
             MemoryMap,
             &MapKey,
             &DescriptorSize,
             &DescriptorVersion
             );
  ASSERT (Status == EFI_BUFFER_TOO_SMALL);

  do {
    MemoryMap = AllocatePool (MemoryMapSize);
    ASSERT (MemoryMap != NULL);

    Status = CoreGetMemoryMapWithSeparatedImageSection (
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

  MemoryMapStart = MemoryMap;
  RuntimeEntryCount = 0;
  for (Index = 0; Index < MemoryMapSize/DescriptorSize; Index++) {
    switch (MemoryMap->Type) {
    case EfiRuntimeServicesCode:
    case EfiRuntimeServicesData:
      RuntimeEntryCount ++;
      break;
    }
    MemoryMap = NEXT_MEMORY_DESCRIPTOR(MemoryMap, DescriptorSize);
  }

  //
  // Allocate MemoryAttributesTable
  //
  MemoryAttributesTable = AllocatePool (sizeof(EFI_MEMORY_ATTRIBUTES_TABLE) + DescriptorSize * RuntimeEntryCount);
  ASSERT (MemoryAttributesTable != NULL);
  MemoryAttributesTable->Version         = EFI_MEMORY_ATTRIBUTES_TABLE_VERSION;
  MemoryAttributesTable->NumberOfEntries = RuntimeEntryCount;
  MemoryAttributesTable->DescriptorSize  = (UINT32)DescriptorSize;
  MemoryAttributesTable->Reserved        = 0;
  DEBUG ((EFI_D_VERBOSE, "MemoryAttributesTable:\n"));
  DEBUG ((EFI_D_VERBOSE, "  Version              - 0x%08x\n", MemoryAttributesTable->Version));
  DEBUG ((EFI_D_VERBOSE, "  NumberOfEntries      - 0x%08x\n", MemoryAttributesTable->NumberOfEntries));
  DEBUG ((EFI_D_VERBOSE, "  DescriptorSize       - 0x%08x\n", MemoryAttributesTable->DescriptorSize));
  MemoryAttributesEntry = (EFI_MEMORY_DESCRIPTOR *)(MemoryAttributesTable + 1);
  MemoryMap = MemoryMapStart;
  for (Index = 0; Index < MemoryMapSize/DescriptorSize; Index++) {
    switch (MemoryMap->Type) {
    case EfiRuntimeServicesCode:
    case EfiRuntimeServicesData:
      CopyMem (MemoryAttributesEntry, MemoryMap, DescriptorSize);
      MemoryAttributesEntry->Attribute &= (EFI_MEMORY_RO|EFI_MEMORY_XP|EFI_MEMORY_RUNTIME);
      DEBUG ((EFI_D_VERBOSE, "Entry (0x%x)\n", MemoryAttributesEntry));
      DEBUG ((EFI_D_VERBOSE, "  Type              - 0x%x\n", MemoryAttributesEntry->Type));
      DEBUG ((EFI_D_VERBOSE, "  PhysicalStart     - 0x%016lx\n", MemoryAttributesEntry->PhysicalStart));
      DEBUG ((EFI_D_VERBOSE, "  VirtualStart      - 0x%016lx\n", MemoryAttributesEntry->VirtualStart));
      DEBUG ((EFI_D_VERBOSE, "  NumberOfPages     - 0x%016lx\n", MemoryAttributesEntry->NumberOfPages));
      DEBUG ((EFI_D_VERBOSE, "  Attribute         - 0x%016lx\n", MemoryAttributesEntry->Attribute));
      MemoryAttributesEntry = NEXT_MEMORY_DESCRIPTOR(MemoryAttributesEntry, DescriptorSize);
      break;
    }
    MemoryMap = NEXT_MEMORY_DESCRIPTOR(MemoryMap, DescriptorSize);
  }
  MemoryMap = MemoryMapStart;
  FreePool (MemoryMap);

  //
  // Update configuratoin table for MemoryAttributesTable.
  //
  Status = gBS->InstallConfigurationTable (&gEfiMemoryAttributesTableGuid, MemoryAttributesTable);
  ASSERT_EFI_ERROR (Status);

  if (mMemoryAttributesTable != NULL) {
    FreePool (mMemoryAttributesTable);
  }
  mMemoryAttributesTable = MemoryAttributesTable; 
}

/**
  Install MemoryAttributesTable on memory allocation.

  @param[in] MemoryType EFI memory type.
**/
VOID
InstallMemoryAttributesTableOnMemoryAllocation (
  IN EFI_MEMORY_TYPE    MemoryType
  )
{
  //
  // Install MemoryAttributesTable after ReadyToBoot on runtime memory allocation.
  //
  if (mMemoryAttributesTableReadyToBoot &&
      ((MemoryType == EfiRuntimeServicesCode) || (MemoryType == EfiRuntimeServicesData))) {
    InstallMemoryAttributesTable ();
  }
}

/**
  Install MemoryAttributesTable on ReadyToBoot.

  @param[in] Event      The Event this notify function registered to.
  @param[in] Context    Pointer to the context data registered to the Event.
**/
VOID
EFIAPI
InstallMemoryAttributesTableOnReadyToBoot (
  IN EFI_EVENT          Event,
  IN VOID               *Context
  )
{
  InstallMemoryAttributesTable ();
  mMemoryAttributesTableReadyToBoot = TRUE; 
}

/**
  Initialize MemoryAttrubutesTable support.
**/
VOID
EFIAPI
CoreInitializeMemoryAttributesTable (
  VOID
  )
{
  EFI_STATUS  Status;
  EFI_EVENT   ReadyToBootEvent;

  //
  // Construct the table at ReadyToBoot.
  //
  Status = CoreCreateEventInternal (
             EVT_NOTIFY_SIGNAL,
             TPL_CALLBACK - 1,
             InstallMemoryAttributesTableOnReadyToBoot,
             NULL,
             &gEfiEventReadyToBootGuid,
             &ReadyToBootEvent
             );
  ASSERT_EFI_ERROR (Status);
  return ;
}
