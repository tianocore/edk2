/** @file
*
*  Copyright (c) 2011-2013, ARM Limited. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#include "BdsInternal.h"

#include <Library/HobLib.h>
#include <Library/TimerLib.h>
#include <Library/PrintLib.h>
#include <Library/SerialPortLib.h>

STATIC CHAR8 *mTokenList[] = {
  /*"SEC",*/
  "PEI",
  "DXE",
  "BDS",
  NULL
};

EFI_STATUS
ShutdownUefiBootServices (
  VOID
  )
{
  EFI_STATUS              Status;
  UINTN                   MemoryMapSize;
  EFI_MEMORY_DESCRIPTOR   *MemoryMap;
  UINTN                   MapKey;
  UINTN                   DescriptorSize;
  UINT32                  DescriptorVersion;
  UINTN                   Pages;

  MemoryMap = NULL;
  MemoryMapSize = 0;
  Pages = 0;

  do {
    Status = gBS->GetMemoryMap (
                    &MemoryMapSize,
                    MemoryMap,
                    &MapKey,
                    &DescriptorSize,
                    &DescriptorVersion
                    );
    if (Status == EFI_BUFFER_TOO_SMALL) {

      Pages = EFI_SIZE_TO_PAGES (MemoryMapSize) + 1;
      MemoryMap = AllocatePages (Pages);

      //
      // Get System MemoryMap
      //
      Status = gBS->GetMemoryMap (
                      &MemoryMapSize,
                      MemoryMap,
                      &MapKey,
                      &DescriptorSize,
                      &DescriptorVersion
                      );
    }

    // Don't do anything between the GetMemoryMap() and ExitBootServices()
    if (!EFI_ERROR(Status)) {
      Status = gBS->ExitBootServices (gImageHandle, MapKey);
      if (EFI_ERROR(Status)) {
        FreePages (MemoryMap, Pages);
        MemoryMap = NULL;
        MemoryMapSize = 0;
      }
    }
  } while (EFI_ERROR(Status));

  return Status;
}

/**
  Connect all DXE drivers

  @retval EFI_SUCCESS           All drivers have been connected
  @retval EFI_NOT_FOUND         No handles match the search.
  @retval EFI_OUT_OF_RESOURCES  There is not resource pool memory to store the matching results.

**/
EFI_STATUS
BdsConnectAllDrivers (
  VOID
  )
{
  UINTN                     HandleCount, Index;
  EFI_HANDLE                *HandleBuffer;
  EFI_STATUS                Status;

  do {
    // Locate all the driver handles
    Status = gBS->LocateHandleBuffer (
                AllHandles,
                NULL,
                NULL,
                &HandleCount,
                &HandleBuffer
                );
    if (EFI_ERROR (Status)) {
      break;
    }

    // Connect every handles
    for (Index = 0; Index < HandleCount; Index++) {
      gBS->ConnectController (HandleBuffer[Index], NULL, NULL, TRUE);
    }

    if (HandleBuffer != NULL) {
      FreePool (HandleBuffer);
    }

    // Check if new handles have been created after the start of the previous handles
    Status = gDS->Dispatch ();
  } while (!EFI_ERROR(Status));

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
InsertSystemMemoryResources (
  LIST_ENTRY *ResourceList,
  EFI_HOB_RESOURCE_DESCRIPTOR *ResHob
  )
{
  BDS_SYSTEM_MEMORY_RESOURCE  *NewResource;
  LIST_ENTRY                  *Link;
  LIST_ENTRY                  *NextLink;
  LIST_ENTRY                  AttachedResources;
  BDS_SYSTEM_MEMORY_RESOURCE  *Resource;
  EFI_PHYSICAL_ADDRESS        NewResourceEnd;

  if (IsListEmpty (ResourceList)) {
    NewResource = AllocateZeroPool (sizeof(BDS_SYSTEM_MEMORY_RESOURCE));
    NewResource->PhysicalStart = ResHob->PhysicalStart;
    NewResource->ResourceLength = ResHob->ResourceLength;
    InsertTailList (ResourceList, &NewResource->Link);
    return EFI_SUCCESS;
  }

  InitializeListHead (&AttachedResources);

  Link = ResourceList->ForwardLink;
  ASSERT (Link != NULL);
  while (Link != ResourceList) {
    Resource = (BDS_SYSTEM_MEMORY_RESOURCE*)Link;

    // Sanity Check. The resources should not overlapped.
    ASSERT(!((ResHob->PhysicalStart >= Resource->PhysicalStart) && (ResHob->PhysicalStart < (Resource->PhysicalStart + Resource->ResourceLength))));
    ASSERT(!((ResHob->PhysicalStart + ResHob->ResourceLength - 1 >= Resource->PhysicalStart) &&
        ((ResHob->PhysicalStart + ResHob->ResourceLength - 1) < (Resource->PhysicalStart + Resource->ResourceLength))));

    // The new resource is attached after this resource descriptor
    if (ResHob->PhysicalStart == Resource->PhysicalStart + Resource->ResourceLength) {
      Resource->ResourceLength =  Resource->ResourceLength + ResHob->ResourceLength;

      NextLink = RemoveEntryList (&Resource->Link);
      InsertTailList (&AttachedResources, &Resource->Link);
      Link = NextLink;
    }
    // The new resource is attached before this resource descriptor
    else if (ResHob->PhysicalStart + ResHob->ResourceLength == Resource->PhysicalStart) {
      Resource->PhysicalStart = ResHob->PhysicalStart;
      Resource->ResourceLength =  Resource->ResourceLength + ResHob->ResourceLength;

      NextLink = RemoveEntryList (&Resource->Link);
      InsertTailList (&AttachedResources, &Resource->Link);
      Link = NextLink;
    } else {
      Link = Link->ForwardLink;
    }
  }

  if (!IsListEmpty (&AttachedResources)) {
    // See if we can merge the attached resource with other resources

    NewResource = (BDS_SYSTEM_MEMORY_RESOURCE*)GetFirstNode (&AttachedResources);
    Link = RemoveEntryList (&NewResource->Link);
    while (!IsListEmpty (&AttachedResources)) {
      // Merge resources
      Resource = (BDS_SYSTEM_MEMORY_RESOURCE*)Link;

      // Ensure they overlap each other
      ASSERT(
          ((NewResource->PhysicalStart >= Resource->PhysicalStart) && (NewResource->PhysicalStart < (Resource->PhysicalStart + Resource->ResourceLength))) ||
          (((NewResource->PhysicalStart + NewResource->ResourceLength) >= Resource->PhysicalStart) && ((NewResource->PhysicalStart + NewResource->ResourceLength) < (Resource->PhysicalStart + Resource->ResourceLength)))
      );

      NewResourceEnd = MAX (NewResource->PhysicalStart + NewResource->ResourceLength, Resource->PhysicalStart + Resource->ResourceLength);
      NewResource->PhysicalStart = MIN (NewResource->PhysicalStart, Resource->PhysicalStart);
      NewResource->ResourceLength = NewResourceEnd - NewResource->PhysicalStart;

      Link = RemoveEntryList (Link);
    }
  } else {
    // None of the Resource of the list is attached to this ResHob. Create a new entry for it
    NewResource = AllocateZeroPool (sizeof(BDS_SYSTEM_MEMORY_RESOURCE));
    NewResource->PhysicalStart = ResHob->PhysicalStart;
    NewResource->ResourceLength = ResHob->ResourceLength;
  }
  InsertTailList (ResourceList, &NewResource->Link);
  return EFI_SUCCESS;
}

EFI_STATUS
GetSystemMemoryResources (
  IN  LIST_ENTRY *ResourceList
  )
{
  EFI_HOB_RESOURCE_DESCRIPTOR *ResHob;

  InitializeListHead (ResourceList);

  // Find the first System Memory Resource Descriptor
  ResHob = (EFI_HOB_RESOURCE_DESCRIPTOR *)GetFirstHob (EFI_HOB_TYPE_RESOURCE_DESCRIPTOR);
  while ((ResHob != NULL) && (ResHob->ResourceType != EFI_RESOURCE_SYSTEM_MEMORY)) {
    ResHob = (EFI_HOB_RESOURCE_DESCRIPTOR *)GetNextHob (EFI_HOB_TYPE_RESOURCE_DESCRIPTOR,(VOID *)((UINTN)ResHob + ResHob->Header.HobLength));
  }

  // Did not find any
  if (ResHob == NULL) {
    return EFI_NOT_FOUND;
  } else {
    InsertSystemMemoryResources (ResourceList, ResHob);
  }

  ResHob = (EFI_HOB_RESOURCE_DESCRIPTOR *)GetNextHob (EFI_HOB_TYPE_RESOURCE_DESCRIPTOR,(VOID *)((UINTN)ResHob + ResHob->Header.HobLength));
  while (ResHob != NULL) {
    if (ResHob->ResourceType == EFI_RESOURCE_SYSTEM_MEMORY) {
      InsertSystemMemoryResources (ResourceList, ResHob);
    }
    ResHob = (EFI_HOB_RESOURCE_DESCRIPTOR *)GetNextHob (EFI_HOB_TYPE_RESOURCE_DESCRIPTOR,(VOID *)((UINTN)ResHob + ResHob->Header.HobLength));
  }

  return EFI_SUCCESS;
}

VOID
PrintPerformance (
  VOID
  )
{
  UINTN       Key;
  CONST VOID  *Handle;
  CONST CHAR8 *Token, *Module;
  UINT64      Start, Stop, TimeStamp;
  UINT64      Delta, TicksPerSecond, Milliseconds;
  UINTN       Index;
  CHAR8       Buffer[100];
  UINTN       CharCount;
  BOOLEAN     CountUp;

  TicksPerSecond = GetPerformanceCounterProperties (&Start, &Stop);
  if (Start < Stop) {
    CountUp = TRUE;
  } else {
    CountUp = FALSE;
  }

  TimeStamp = 0;
  Key       = 0;
  do {
    Key = GetPerformanceMeasurement (Key, (CONST VOID **)&Handle, &Token, &Module, &Start, &Stop);
    if (Key != 0) {
      for (Index = 0; mTokenList[Index] != NULL; Index++) {
        if (AsciiStriCmp (mTokenList[Index], Token) == 0) {
          Delta = CountUp?(Stop - Start):(Start - Stop);
          TimeStamp += Delta;
          Milliseconds = DivU64x64Remainder (MultU64x32 (Delta, 1000), TicksPerSecond, NULL);
          CharCount = AsciiSPrint (Buffer,sizeof (Buffer),"%6a %6ld ms\n", Token, Milliseconds);
          SerialPortWrite ((UINT8 *) Buffer, CharCount);
          break;
        }
      }
    }
  } while (Key != 0);

  CharCount = AsciiSPrint (Buffer,sizeof (Buffer),"Total Time = %ld ms\n\n", DivU64x64Remainder (MultU64x32 (TimeStamp, 1000), TicksPerSecond, NULL));
  SerialPortWrite ((UINT8 *) Buffer, CharCount);
}

EFI_STATUS
GetGlobalEnvironmentVariable (
  IN     CONST CHAR16*   VariableName,
  IN     VOID*           DefaultValue,
  IN OUT UINTN*          Size,
  OUT    VOID**          Value
  )
{
  return GetEnvironmentVariable (VariableName, &gEfiGlobalVariableGuid,
           DefaultValue, Size, Value);
}

EFI_STATUS
GetEnvironmentVariable (
  IN     CONST CHAR16*   VariableName,
  IN     EFI_GUID*       VendorGuid,
  IN     VOID*           DefaultValue,
  IN OUT UINTN*          Size,
  OUT    VOID**          Value
  )
{
  EFI_STATUS  Status;
  UINTN       VariableSize;

  // Try to get the variable size.
  *Value = NULL;
  VariableSize = 0;
  Status = gRT->GetVariable ((CHAR16 *) VariableName, VendorGuid, NULL, &VariableSize, *Value);
  if (Status == EFI_NOT_FOUND) {
    if ((DefaultValue != NULL) && (Size != NULL) && (*Size != 0)) {
      // If the environment variable does not exist yet then set it with the default value
      Status = gRT->SetVariable (
                    (CHAR16*)VariableName,
                    VendorGuid,
                    EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                    *Size,
                    DefaultValue
                    );
      *Value = AllocateCopyPool (*Size, DefaultValue);
    } else {
      return EFI_NOT_FOUND;
    }
  } else if (Status == EFI_BUFFER_TOO_SMALL) {
    // Get the environment variable value
    *Value = AllocatePool (VariableSize);
    if (*Value == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    Status = gRT->GetVariable ((CHAR16 *)VariableName, VendorGuid, NULL, &VariableSize, *Value);
    if (EFI_ERROR (Status)) {
      FreePool(*Value);
      return EFI_INVALID_PARAMETER;
    }

    if (Size) {
      *Size = VariableSize;
    }
  } else {
    *Value = AllocateCopyPool (*Size, DefaultValue);
    return Status;
  }

  return EFI_SUCCESS;
}
