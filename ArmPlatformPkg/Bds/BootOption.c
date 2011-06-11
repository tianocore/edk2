/** @file
*
*  Copyright (c) 2011, ARM Limited. All rights reserved.
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

extern EFI_HANDLE mImageHandle;

EFI_STATUS
BootOptionStart (
  IN BDS_LOAD_OPTION *BootOption
  )
{
  EFI_STATUS Status;
  EFI_DEVICE_PATH* FdtDevicePath;
  EFI_DEVICE_PATH_FROM_TEXT_PROTOCOL  *EfiDevicePathFromTextProtocol;

  Status = EFI_UNSUPPORTED;

  if (BootOption->OptionalData->LoaderType == BDS_LOADER_EFI_APPLICATION) {
    // Need to connect every drivers to ensure no dependencies are missing for the application
    BdsConnectAllDrivers();

    Status = BdsStartEfiApplication (mImageHandle, BootOption->FilePathList);
  } else if (BootOption->OptionalData->LoaderType == BDS_LOADER_KERNEL_LINUX_ATAG) {
    Status = BdsBootLinux (BootOption->FilePathList, BootOption->OptionalData->Arguments, NULL);
  } else if (BootOption->OptionalData->LoaderType == BDS_LOADER_KERNEL_LINUX_FDT) {
    // Convert the FDT path into a Device Path
    Status = gBS->LocateProtocol (&gEfiDevicePathFromTextProtocolGuid, NULL, (VOID **)&EfiDevicePathFromTextProtocol);
    ASSERT_EFI_ERROR(Status);
    FdtDevicePath = EfiDevicePathFromTextProtocol->ConvertTextToDevicePath ((CHAR16*)PcdGetPtr(PcdFdtDevicePath));

    Status = BdsBootLinux (BootOption->FilePathList, BootOption->OptionalData->Arguments, FdtDevicePath);
    FreePool(FdtDevicePath);
  }

  return Status;
}

EFI_STATUS
BootOptionParseLoadOption (
  IN  EFI_LOAD_OPTION EfiLoadOption,
  IN  UINTN           EfiLoadOptionSize,
  OUT BDS_LOAD_OPTION **BdsLoadOption
  )
{
  BDS_LOAD_OPTION *LoadOption;
  UINTN           FilePathListLength;
  UINTN           DescriptionLength;

  if (EfiLoadOption == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (EfiLoadOptionSize < sizeof(UINT32) + sizeof(UINT16) + sizeof(CHAR16) + sizeof(EFI_DEVICE_PATH_PROTOCOL)) {
    return EFI_BAD_BUFFER_SIZE;
  }

  LoadOption = (BDS_LOAD_OPTION*)AllocatePool(sizeof(BDS_LOAD_OPTION));
  if (LoadOption == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  LoadOption->LoadOption = EfiLoadOption;
  LoadOption->LoadOptionSize = EfiLoadOptionSize;

  LoadOption->Attributes    = *(UINT32*)EfiLoadOption;
  FilePathListLength        = *(UINT16*)(EfiLoadOption + sizeof(UINT32));
  LoadOption->Description   = (CHAR16*)(EfiLoadOption + sizeof(UINT32) + sizeof(UINT16));
  DescriptionLength         = StrSize (LoadOption->Description);
  LoadOption->FilePathList  = (EFI_DEVICE_PATH_PROTOCOL*)(EfiLoadOption + sizeof(UINT32) + sizeof(UINT16) + DescriptionLength);

  if ((UINTN)((UINT8*)LoadOption->FilePathList + FilePathListLength - EfiLoadOption) == EfiLoadOptionSize) {
    LoadOption->OptionalData = NULL;
  } else {
    LoadOption->OptionalData = (BDS_LOADER_OPTIONAL_DATA *)((UINT8*)LoadOption->FilePathList + FilePathListLength);
  }

  *BdsLoadOption = LoadOption;
  return EFI_SUCCESS;
}

EFI_STATUS
BootOptionFromLoadOptionVariable (
  IN  UINT16            LoadOptionIndex,
  OUT BDS_LOAD_OPTION **BdsLoadOption
  )
{
  EFI_STATUS  Status;
  CHAR16      BootVariableName[9];
  EFI_LOAD_OPTION     EfiLoadOption;
  UINTN               EfiLoadOptionSize;

  UnicodeSPrint (BootVariableName, 9 * sizeof(CHAR16), L"Boot%04X", LoadOptionIndex);

  Status = GetEnvironmentVariable (BootVariableName, NULL, &EfiLoadOptionSize, (VOID**)&EfiLoadOption);
  if (!EFI_ERROR(Status)) {
    Status = BootOptionParseLoadOption (EfiLoadOption,EfiLoadOptionSize,BdsLoadOption);
    if (!EFI_ERROR(Status)) {
      (*BdsLoadOption)->LoadOptionIndex = LoadOptionIndex;
    }
  }

  return Status;
}

EFI_STATUS
BootOptionList (
  IN OUT LIST_ENTRY *BootOptionList
  )
{
  EFI_STATUS        Status;
  UINTN             Index;
  UINT16            *BootOrder;
  UINTN             BootOrderSize;
  BDS_LOAD_OPTION   *BdsLoadOption;

  InitializeListHead (BootOptionList);

  // Get the Boot Option Order from the environment variable
  Status = GetEnvironmentVariable (L"BootOrder", NULL, &BootOrderSize, (VOID**)&BootOrder);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  for (Index = 0; Index < BootOrderSize / sizeof (UINT16); Index++) {
    Status = BootOptionFromLoadOptionVariable (BootOrder[Index],&BdsLoadOption);
    if (!EFI_ERROR(Status)) {
      InsertTailList (BootOptionList,&BdsLoadOption->Link);
    }
  }

  return EFI_SUCCESS;
}

UINT16
BootOptionAllocateBootIndex (
  VOID
  )
{
  EFI_STATUS        Status;
  UINTN             Index;
  UINT32            BootIndex;
  UINT16            *BootOrder;
  UINTN             BootOrderSize;
  BOOLEAN           Found;

  // Get the Boot Option Order from the environment variable
  Status = GetEnvironmentVariable (L"BootOrder", NULL, &BootOrderSize, (VOID**)&BootOrder);
  if (!EFI_ERROR(Status)) {
    for (BootIndex = 0; BootIndex <= 0xFFFF; BootIndex++) {
      Found = FALSE;
      for (Index = 0; Index < BootOrderSize / sizeof (UINT16); Index++) {
        if (BootOrder[Index] == BootIndex) {
          Found = TRUE;
          break;
        }
      }
      if (!Found) {
        return BootIndex;
      }
    }
  }
  // Return the first index
  return 0;
}

STATIC
EFI_STATUS
BootOptionSetFields (
  IN BDS_LOAD_OPTION *BootOption,
  IN UINT32 Attributes,
  IN CHAR16* BootDescription,
  IN EFI_DEVICE_PATH_PROTOCOL* DevicePath,
  IN  BDS_LOADER_TYPE   BootType,
  IN  CHAR8*            BootArguments
  )
{
  EFI_LOAD_OPTION EfiLoadOption;
  UINTN           EfiLoadOptionSize;
  UINTN           BootDescriptionSize;
  UINTN           BootOptionalDataSize;
  UINT16          FilePathListLength;
  EFI_DEVICE_PATH_PROTOCOL* DevicePathNode;
  UINTN           NodeLength;
  UINT8*          EfiLoadOptionPtr;

  // If we are overwriting an existent Boot Option then we have to free previously allocated memory
  if (BootOption->LoadOption) {
    FreePool(BootOption->LoadOption);
  }

  BootDescriptionSize = StrSize(BootDescription);
  BootOptionalDataSize = sizeof(BDS_LOADER_OPTIONAL_DATA) +
      (BootArguments == NULL ? 0 : AsciiStrSize(BootArguments));

  // Compute the size of the FilePath list
  FilePathListLength = 0;
  DevicePathNode = DevicePath;
  while (!IsDevicePathEndType (DevicePathNode)) {
    FilePathListLength += DevicePathNodeLength (DevicePathNode);
    DevicePathNode = NextDevicePathNode (DevicePathNode);
  }
  // Add the length of the DevicePath EndType
  FilePathListLength += DevicePathNodeLength (DevicePathNode);

  // Allocate the memory for the EFI Load Option
  EfiLoadOptionSize = sizeof(UINT32) + sizeof(UINT16) + BootDescriptionSize + FilePathListLength + BootOptionalDataSize;
  EfiLoadOption = (EFI_LOAD_OPTION)AllocatePool(EfiLoadOptionSize);
  EfiLoadOptionPtr = EfiLoadOption;

  //
  // Populate the EFI Load Option and BDS Boot Option structures
  //

  // Attributes fields
  BootOption->Attributes = Attributes;
  *(UINT32*)EfiLoadOptionPtr = Attributes;
  EfiLoadOptionPtr += sizeof(UINT32);

  // FilePath List fields
  BootOption->FilePathListLength = FilePathListLength;
  *(UINT16*)EfiLoadOptionPtr = FilePathListLength;
  EfiLoadOptionPtr += sizeof(UINT16);

  // Boot description fields
  BootOption->Description = (CHAR16*)EfiLoadOptionPtr;
  CopyMem (EfiLoadOptionPtr, BootDescription, BootDescriptionSize);
  EfiLoadOptionPtr += BootDescriptionSize;

  // File path fields
  BootOption->FilePathList = (EFI_DEVICE_PATH_PROTOCOL*)EfiLoadOptionPtr;
  DevicePathNode = DevicePath;
  while (!IsDevicePathEndType (DevicePathNode)) {
    NodeLength = DevicePathNodeLength(DevicePathNode);
    CopyMem (EfiLoadOptionPtr, DevicePathNode, NodeLength);
    EfiLoadOptionPtr += NodeLength;
    DevicePathNode = NextDevicePathNode (DevicePathNode);
  }

  // Set the End Device Path Type
  SetDevicePathEndNode (EfiLoadOptionPtr);
  EfiLoadOptionPtr = (UINT8 *)EfiLoadOptionPtr + sizeof(EFI_DEVICE_PATH);

  // Optional Data fields, Do unaligned writes
  WriteUnaligned32 ((UINT32 *)EfiLoadOptionPtr, BootType);

  CopyMem (&((BDS_LOADER_OPTIONAL_DATA*)EfiLoadOptionPtr)->Arguments, BootArguments, AsciiStrSize(BootArguments));
  BootOption->OptionalData = (BDS_LOADER_OPTIONAL_DATA *)EfiLoadOptionPtr;

  // Fill the EFI Load option fields
  BootOption->LoadOptionIndex = BootOptionAllocateBootIndex();
  BootOption->LoadOption = EfiLoadOption;
  BootOption->LoadOptionSize = EfiLoadOptionSize;

  return EFI_SUCCESS;
}

EFI_STATUS
BootOptionCreate (
  IN  UINT32 Attributes,
  IN  CHAR16* BootDescription,
  IN  EFI_DEVICE_PATH_PROTOCOL* DevicePath,
  IN  BDS_LOADER_TYPE   BootType,
  IN  CHAR8*            BootArguments,
  OUT BDS_LOAD_OPTION **BdsLoadOption
  )
{
  EFI_STATUS      Status;
  BDS_LOAD_OPTION *BootOption;
  CHAR16          BootVariableName[9];
  UINT16            *BootOrder;
  UINTN             BootOrderSize;

  //
  // Allocate and fill the memory for the BDS Load Option structure
  //
  BootOption = (BDS_LOAD_OPTION*)AllocateZeroPool(sizeof(BDS_LOAD_OPTION));

  InitializeListHead (&BootOption->Link);
  BootOptionSetFields (BootOption, Attributes, BootDescription, DevicePath, BootType, BootArguments);

  //
  // Set the related environment variables
  //

  // Create Boot#### environment variable
  UnicodeSPrint (BootVariableName, 9 * sizeof(CHAR16), L"Boot%04X", BootOption->LoadOptionIndex);
  Status = gRT->SetVariable (
      BootVariableName,
      &gEfiGlobalVariableGuid,
      EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
      BootOption->LoadOptionSize,
      BootOption->LoadOption
      );

  // Add the new Boot Index to the list
  Status = GetEnvironmentVariable (L"BootOrder", NULL, &BootOrderSize, (VOID**)&BootOrder);
  if (!EFI_ERROR(Status)) {
    BootOrder = ReallocatePool (BootOrderSize, BootOrderSize + sizeof(UINT16), BootOrder);
    // Add the new index at the end
    BootOrder[BootOrderSize / sizeof(UINT16)] = BootOption->LoadOptionIndex;
    BootOrderSize += sizeof(UINT16);
  } else {
    // BootOrder does not exist. Create it
    BootOrderSize = sizeof(UINT16);
    BootOrder = &(BootOption->LoadOptionIndex);
  }

  // Update (or Create) the BootOrder environment variable
  Status = gRT->SetVariable (
      L"BootOrder",
      &gEfiGlobalVariableGuid,
      EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
      BootOrderSize,
      BootOrder
      );

  *BdsLoadOption = BootOption;
  return Status;
}

EFI_STATUS
BootOptionUpdate (
  IN  BDS_LOAD_OPTION *BdsLoadOption,
  IN  UINT32 Attributes,
  IN  CHAR16* BootDescription,
  IN  EFI_DEVICE_PATH_PROTOCOL* DevicePath,
  IN  BDS_LOADER_TYPE   BootType,
  IN  CHAR8*            BootArguments
  )
{
  EFI_STATUS      Status;
  BDS_LOAD_OPTION *BootOption;
  CHAR16          BootVariableName[9];

  // Update the BDS Load Option structure
  BootOptionSetFields (BdsLoadOption, Attributes, BootDescription, DevicePath, BootType, BootArguments);

  // Update the related environment variables
  UnicodeSPrint (BootVariableName, 9 * sizeof(CHAR16), L"Boot%04X", BootOption->LoadOptionIndex);
  Status = gRT->SetVariable (
      BootVariableName,
      &gEfiGlobalVariableGuid,
      EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
      BootOption->LoadOptionSize,
      BootOption->LoadOption
      );

  return Status;
}

EFI_STATUS
BootOptionDelete (
  IN  BDS_LOAD_OPTION *BootOption
  )
{
  UINTN Index;
  UINTN BootOrderSize;
  UINT16* BootOrder;
  UINTN BootOrderCount;
  EFI_STATUS  Status;

  // If the Boot Optiono was attached to a list remove it
  if (!IsListEmpty (&BootOption->Link)) {
    // Remove the entry from the list
    RemoveEntryList (&BootOption->Link);
  }

  // Remove the entry from the BootOrder environment variable
  Status = GetEnvironmentVariable (L"BootOrder", NULL, &BootOrderSize, (VOID**)&BootOrder);
  if (!EFI_ERROR(Status)) {
    BootOrderCount = BootOrderSize / sizeof(UINT16);

    // Find the index of the removed entry
    for (Index = 0; Index < BootOrderCount; Index++) {
      if (BootOrder[Index] == BootOption->LoadOptionIndex) {
        // If it the last entry we do not need to rearrange the BootOrder list
        if (Index + 1 != BootOrderCount) {
          CopyMem (&BootOrder[Index],&BootOrder[Index+1], BootOrderCount - (Index + 1));
        }
        break;
      }
    }

    // Update the BootOrder environment variable
    Status = gRT->SetVariable (
        L"BootOrder",
        &gEfiGlobalVariableGuid,
        EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
        BootOrderSize - sizeof(UINT16),
        BootOrder
        );
  }

  return EFI_SUCCESS;
}
