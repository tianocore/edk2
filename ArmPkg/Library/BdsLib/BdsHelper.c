/** @file
*
*  Copyright (c) 2011-2015, ARM Limited. All rights reserved.
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
