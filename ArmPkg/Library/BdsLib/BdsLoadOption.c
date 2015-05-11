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

EFI_STATUS
BootOptionParseLoadOption (
  IN     EFI_LOAD_OPTION *EfiLoadOption,
  IN     UINTN           EfiLoadOptionSize,
  IN OUT BDS_LOAD_OPTION **BdsLoadOption
  )
{
  BDS_LOAD_OPTION *LoadOption;
  UINTN           DescriptionLength;
  UINTN           EfiLoadOptionPtr;

  if (EfiLoadOption == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (EfiLoadOptionSize < sizeof(UINT32) + sizeof(UINT16) + sizeof(CHAR16) + sizeof(EFI_DEVICE_PATH_PROTOCOL)) {
    return EFI_BAD_BUFFER_SIZE;
  }

  if (*BdsLoadOption == NULL) {
    LoadOption = (BDS_LOAD_OPTION*)AllocateZeroPool (sizeof(BDS_LOAD_OPTION));
    if (LoadOption == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
  } else {
    LoadOption = *BdsLoadOption;
  }

  EfiLoadOptionPtr           = (UINTN)EfiLoadOption;
  LoadOption->LoadOption     = EfiLoadOption;
  LoadOption->LoadOptionSize = EfiLoadOptionSize;

  LoadOption->Attributes         = *(UINT32*)EfiLoadOptionPtr;
  LoadOption->FilePathListLength = *(UINT16*)(EfiLoadOptionPtr + sizeof(UINT32));
  LoadOption->Description        = (CHAR16*)(EfiLoadOptionPtr + sizeof(UINT32) + sizeof(UINT16));
  DescriptionLength              = StrSize (LoadOption->Description);
  LoadOption->FilePathList       = (EFI_DEVICE_PATH_PROTOCOL*)(EfiLoadOptionPtr + sizeof(UINT32) + sizeof(UINT16) + DescriptionLength);

  // If ((End of EfiLoadOptiony - Start of EfiLoadOption) == EfiLoadOptionSize) then No Optional Data
  if ((UINTN)((UINTN)LoadOption->FilePathList + LoadOption->FilePathListLength - EfiLoadOptionPtr) == EfiLoadOptionSize) {
    LoadOption->OptionalData     = NULL;
    LoadOption->OptionalDataSize = 0;
  } else {
    LoadOption->OptionalData     = (VOID*)((UINTN)(LoadOption->FilePathList) + LoadOption->FilePathListLength);
    LoadOption->OptionalDataSize = EfiLoadOptionSize - ((UINTN)LoadOption->OptionalData - EfiLoadOptionPtr);
  }

  if (*BdsLoadOption == NULL) {
    *BdsLoadOption = LoadOption;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
BootOptionFromLoadOptionVariable (
  IN  CHAR16*           BootVariableName,
  OUT BDS_LOAD_OPTION** BdsLoadOption
  )
{
  EFI_STATUS            Status;
  EFI_LOAD_OPTION       *EfiLoadOption;
  UINTN                 EfiLoadOptionSize;

  Status = GetGlobalEnvironmentVariable (BootVariableName, NULL, &EfiLoadOptionSize, (VOID**)&EfiLoadOption);
  if (!EFI_ERROR(Status)) {
    *BdsLoadOption = NULL;
    Status = BootOptionParseLoadOption (EfiLoadOption, EfiLoadOptionSize, BdsLoadOption);
  }

  return Status;
}

EFI_STATUS
BootOptionFromLoadOptionIndex (
  IN  UINT16            LoadOptionIndex,
  OUT BDS_LOAD_OPTION **BdsLoadOption
  )
{
  CHAR16        BootVariableName[9];
  EFI_STATUS    Status;

  UnicodeSPrint (BootVariableName, 9 * sizeof(CHAR16), L"Boot%04X", LoadOptionIndex);

  Status = BootOptionFromLoadOptionVariable (BootVariableName, BdsLoadOption);
  if (!EFI_ERROR(Status)) {
    (*BdsLoadOption)->LoadOptionIndex = LoadOptionIndex;
  }

  return Status;
}

EFI_STATUS
BootOptionToLoadOptionVariable (
  IN BDS_LOAD_OPTION*   BdsLoadOption
  )
{
  EFI_STATUS                    Status;
  UINTN                         DescriptionSize;
  //UINT16                        FilePathListLength;
  EFI_DEVICE_PATH_PROTOCOL*     DevicePathNode;
  UINTN                         NodeLength;
  UINT8*                        EfiLoadOptionPtr;
  VOID*                         OldLoadOption;
  CHAR16                        BootVariableName[9];
  UINTN                         BootOrderSize;
  UINT16*                       BootOrder;

  // If we are overwriting an existent Boot Option then we have to free previously allocated memory
  if (BdsLoadOption->LoadOptionSize > 0) {
    OldLoadOption = BdsLoadOption->LoadOption;
  } else {
    OldLoadOption = NULL;

    // If this function is called at the creation of the Boot Device entry (not at the update) the
    // BootOption->LoadOptionSize must be zero then we get a new BootIndex for this entry
    BdsLoadOption->LoadOptionIndex = BootOptionAllocateBootIndex ();

    //TODO: Add to the the Boot Entry List
  }

  DescriptionSize = StrSize(BdsLoadOption->Description);

  // Ensure the FilePathListLength information is correct
  ASSERT (GetDevicePathSize (BdsLoadOption->FilePathList) == BdsLoadOption->FilePathListLength);

  // Allocate the memory for the EFI Load Option
  BdsLoadOption->LoadOptionSize = sizeof(UINT32) + sizeof(UINT16) + DescriptionSize + BdsLoadOption->FilePathListLength + BdsLoadOption->OptionalDataSize;

  BdsLoadOption->LoadOption = (EFI_LOAD_OPTION *)AllocateZeroPool (BdsLoadOption->LoadOptionSize);
  if (BdsLoadOption->LoadOption == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  EfiLoadOptionPtr = (UINT8 *) BdsLoadOption->LoadOption;

  //
  // Populate the EFI Load Option and BDS Boot Option structures
  //

  // Attributes fields
  *(UINT32*)EfiLoadOptionPtr = BdsLoadOption->Attributes;
  EfiLoadOptionPtr += sizeof(UINT32);

  // FilePath List fields
  *(UINT16*)EfiLoadOptionPtr = BdsLoadOption->FilePathListLength;
  EfiLoadOptionPtr += sizeof(UINT16);

  // Boot description fields
  CopyMem (EfiLoadOptionPtr, BdsLoadOption->Description, DescriptionSize);
  EfiLoadOptionPtr += DescriptionSize;

  // File path fields
  DevicePathNode = BdsLoadOption->FilePathList;
  while (!IsDevicePathEndType (DevicePathNode)) {
    NodeLength = DevicePathNodeLength(DevicePathNode);
    CopyMem (EfiLoadOptionPtr, DevicePathNode, NodeLength);
    EfiLoadOptionPtr += NodeLength;
    DevicePathNode = NextDevicePathNode (DevicePathNode);
  }

  // Set the End Device Path Type
  SetDevicePathEndNode (EfiLoadOptionPtr);
  EfiLoadOptionPtr += sizeof(EFI_DEVICE_PATH);

  // Fill the Optional Data
  if (BdsLoadOption->OptionalDataSize > 0) {
    CopyMem (EfiLoadOptionPtr, BdsLoadOption->OptionalData, BdsLoadOption->OptionalDataSize);
  }

  // Case where the fields have been updated
  if (OldLoadOption) {
    // Now, the old data has been copied to the new allocated packed structure, we need to update the pointers of BdsLoadOption
    BootOptionParseLoadOption (BdsLoadOption->LoadOption, BdsLoadOption->LoadOptionSize, &BdsLoadOption);
    // Free the old packed structure
    FreePool (OldLoadOption);
  }

  // Create/Update Boot#### environment variable
  UnicodeSPrint (BootVariableName, 9 * sizeof(CHAR16), L"Boot%04X", BdsLoadOption->LoadOptionIndex);
  Status = gRT->SetVariable (
      BootVariableName,
      &gEfiGlobalVariableGuid,
      EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
      BdsLoadOption->LoadOptionSize,
      BdsLoadOption->LoadOption
      );

  // When it is a new entry we must add the entry to the BootOrder
  if (OldLoadOption == NULL) {
    // Add the new Boot Index to the list
    Status = GetGlobalEnvironmentVariable (L"BootOrder", NULL, &BootOrderSize, (VOID**)&BootOrder);
    if (!EFI_ERROR(Status)) {
      BootOrder = ReallocatePool (BootOrderSize, BootOrderSize + sizeof(UINT16), BootOrder);
      // Add the new index at the end
      BootOrder[BootOrderSize / sizeof(UINT16)] = BdsLoadOption->LoadOptionIndex;
      BootOrderSize += sizeof(UINT16);
    } else {
      // BootOrder does not exist. Create it
      BootOrderSize = sizeof(UINT16);
      BootOrder = &(BdsLoadOption->LoadOptionIndex);
    }

    // Update (or Create) the BootOrder environment variable
    gRT->SetVariable (
        L"BootOrder",
        &gEfiGlobalVariableGuid,
        EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
        BootOrderSize,
        BootOrder
        );
    DEBUG((EFI_D_ERROR,"Create %s\n",BootVariableName));

    // Free memory allocated by GetGlobalEnvironmentVariable
    if (!EFI_ERROR(Status)) {
      FreePool (BootOrder);
    }
  } else {
    DEBUG((EFI_D_ERROR,"Update %s\n",BootVariableName));
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
  Status = GetGlobalEnvironmentVariable (L"BootOrder", NULL, &BootOrderSize, (VOID**)&BootOrder);
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
    FreePool (BootOrder);
  }
  // Return the first index
  return 0;
}
