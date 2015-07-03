/** @file
  This module sets default policy for attributes of EfiACPIMemoryNVS and EfiReservedMemoryType.

  This module sets EFI_MEMORY_XP for attributes of EfiACPIMemoryNVS and EfiReservedMemoryType
  in UEFI memory map, if and only of PropertiesTable is published and has BIT0 set.

Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Guid/EventGroup.h>
#include <Guid/PropertiesTable.h>

/**
  Converts a number of EFI_PAGEs to a size in bytes.

  NOTE: Do not use EFI_PAGES_TO_SIZE because it handles UINTN only.

  @param  Pages     The number of EFI_PAGES.

  @return  The number of bytes associated with the number of EFI_PAGEs specified
           by Pages.
**/
UINT64
EfiPagesToSize (
  IN UINT64 Pages
  )
{
  return LShiftU64 (Pages, EFI_PAGE_SHIFT);
}

/**
  Set memory attributes according to default policy.

  @param  MemoryMap        A pointer to the buffer in which firmware places the current memory map.
  @param  MemoryMapSize    Size, in bytes, of the MemoryMap buffer.
  @param  DescriptorSize   size, in bytes, of an individual EFI_MEMORY_DESCRIPTOR.
**/
VOID
SetMemorySpaceAttributesDefault (
  IN EFI_MEMORY_DESCRIPTOR  *MemoryMap,
  IN UINTN                  MemoryMapSize,
  IN UINTN                  DescriptorSize
  )
{
  EFI_MEMORY_DESCRIPTOR       *MemoryMapEntry;
  EFI_MEMORY_DESCRIPTOR       *MemoryMapEnd;
  EFI_STATUS                  Status;

  DEBUG ((EFI_D_INFO, "SetMemorySpaceAttributesDefault\n"));

  MemoryMapEntry = MemoryMap;
  MemoryMapEnd   = (EFI_MEMORY_DESCRIPTOR *) ((UINT8 *) MemoryMap + MemoryMapSize);
  while ((UINTN)MemoryMapEntry < (UINTN)MemoryMapEnd) {
    if (MemoryMapEntry->PhysicalStart < BASE_1MB) {
      //
      // Do not touch memory space below 1MB
      //
      MemoryMapEntry = NEXT_MEMORY_DESCRIPTOR (MemoryMapEntry, DescriptorSize);
      continue;
    }
    switch (MemoryMapEntry->Type) {
    case EfiRuntimeServicesCode:
    case EfiRuntimeServicesData:
      //
      // should be handled later;
      //
      break;
    case EfiReservedMemoryType:
    case EfiACPIMemoryNVS:
      //
      // Handle EfiReservedMemoryType and EfiACPIMemoryNVS, because there might be firmware executable there.
      //
      DEBUG ((EFI_D_INFO, "SetMemorySpaceAttributes - %016lx - %016lx (%016lx) ...\n",
        MemoryMapEntry->PhysicalStart,
        MemoryMapEntry->PhysicalStart + EfiPagesToSize (MemoryMapEntry->NumberOfPages),
        MemoryMapEntry->Attribute
        ));
      Status = gDS->SetMemorySpaceCapabilities (
                      MemoryMapEntry->PhysicalStart,
                      EfiPagesToSize (MemoryMapEntry->NumberOfPages),
                      MemoryMapEntry->Attribute | EFI_MEMORY_XP
                      );
      DEBUG ((EFI_D_INFO, "SetMemorySpaceCapabilities - %r\n", Status));
      break;
    }

    MemoryMapEntry = NEXT_MEMORY_DESCRIPTOR (MemoryMapEntry, DescriptorSize);
  }

  return ;
}

/**
  Update memory attributes according to default policy.

  @param[in]  Event     The Event this notify function registered to.
  @param[in]  Context   Pointer to the context data registered to the Event.
**/
VOID
EFIAPI
UpdateMemoryAttributesDefault (
  EFI_EVENT                               Event,
  VOID                                    *Context
  )
{
  EFI_STATUS                  Status;
  EFI_MEMORY_DESCRIPTOR       *MemoryMap;
  UINTN                       MemoryMapSize;
  UINTN                       MapKey;
  UINTN                       DescriptorSize;
  UINT32                      DescriptorVersion;
  EFI_PROPERTIES_TABLE        *PropertiesTable;

  DEBUG ((EFI_D_INFO, "UpdateMemoryAttributesDefault\n"));
  Status = EfiGetSystemConfigurationTable (&gEfiPropertiesTableGuid, (VOID **) &PropertiesTable);
  if (EFI_ERROR (Status)) {
    goto Done;
  }
  
  ASSERT (PropertiesTable != NULL);

  DEBUG ((EFI_D_INFO, "MemoryProtectionAttribute - 0x%016lx\n", PropertiesTable->MemoryProtectionAttribute));
  if ((PropertiesTable->MemoryProtectionAttribute & EFI_PROPERTIES_RUNTIME_MEMORY_PROTECTION_NON_EXECUTABLE_PE_DATA) == 0) {
    goto Done;
  }

  //
  // Get the EFI memory map.
  //
  MemoryMapSize  = 0;
  MemoryMap      = NULL;
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

  SetMemorySpaceAttributesDefault (MemoryMap, MemoryMapSize, DescriptorSize);

Done:
  gBS->CloseEvent (Event);

  return ;
}

/**
  The entrypoint of properties table attribute driver.

  @param  ImageHandle    The firmware allocated handle for the EFI image.
  @param  SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS    It always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
InitializePropertiesTableAttributesDxe (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_EVENT   ReadyToBootEvent;

  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  UpdateMemoryAttributesDefault,
                  NULL,
                  &gEfiEventReadyToBootGuid,
                  &ReadyToBootEvent
                  );
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}
