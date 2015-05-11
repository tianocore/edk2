/** @file
*
*  Copyright (c) 2011-2014, ARM Limited. All rights reserved.
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

#include <Guid/ArmGlobalVariableHob.h>
#include "BdsInternal.h"

EFI_STATUS
BootOptionStart (
  IN BDS_LOAD_OPTION *BootOption
  )
{
  EFI_STATUS                            Status;
  UINT32                                LoaderType;
  ARM_BDS_LOADER_OPTIONAL_DATA*         OptionalData;
  ARM_BDS_LINUX_ARGUMENTS*              LinuxArguments;
  UINTN                                 CmdLineSize;
  UINTN                                 InitrdSize;
  EFI_DEVICE_PATH*                      Initrd;
  UINT16                                LoadOptionIndexSize;

  if (IS_ARM_BDS_BOOTENTRY (BootOption)) {
    Status = EFI_UNSUPPORTED;
    OptionalData = BootOption->OptionalData;
    LoaderType = ReadUnaligned32 ((CONST UINT32*)&OptionalData->Header.LoaderType);

    if (LoaderType == BDS_LOADER_EFI_APPLICATION) {
      if ((BootOption->Attributes & LOAD_OPTION_CATEGORY) == LOAD_OPTION_CATEGORY_APP) {
        // Need to connect every drivers to ensure no dependencies are missing for the application
        BdsConnectAllDrivers ();
      }

      Status = BdsStartEfiApplication (gImageHandle, BootOption->FilePathList, 0, NULL);
    } else if (LoaderType == BDS_LOADER_KERNEL_LINUX_ATAG) {
      LinuxArguments = &(OptionalData->Arguments.LinuxArguments);
      CmdLineSize = ReadUnaligned16 ((CONST UINT16*)&LinuxArguments->CmdLineSize);
      InitrdSize = ReadUnaligned16 ((CONST UINT16*)&LinuxArguments->InitrdSize);

      if (InitrdSize > 0) {
        Initrd = GetAlignedDevicePath ((EFI_DEVICE_PATH*)((UINTN)(LinuxArguments + 1) + CmdLineSize));
      } else {
        Initrd = NULL;
      }

      Status = BdsBootLinuxAtag (BootOption->FilePathList,
                                 Initrd, // Initrd
                                 (CHAR8*)(LinuxArguments + 1)); // CmdLine
    } else if (LoaderType == BDS_LOADER_KERNEL_LINUX_FDT) {
      LinuxArguments = &(OptionalData->Arguments.LinuxArguments);
      CmdLineSize = ReadUnaligned16 ((CONST UINT16*)&LinuxArguments->CmdLineSize);
      InitrdSize = ReadUnaligned16 ((CONST UINT16*)&LinuxArguments->InitrdSize);

      if (InitrdSize > 0) {
        Initrd = GetAlignedDevicePath ((EFI_DEVICE_PATH*)((UINTN)(LinuxArguments + 1) + CmdLineSize));
      } else {
        Initrd = NULL;
      }
      Status = BdsBootLinuxFdt (
                 BootOption->FilePathList,
                 Initrd,
                 (CHAR8*)(LinuxArguments + 1)
                 );
    }
  } else {
    // Connect all the drivers if the EFI Application is not a EFI OS Loader
    if ((BootOption->Attributes & LOAD_OPTION_CATEGORY) == LOAD_OPTION_CATEGORY_APP) {
      BdsConnectAllDrivers ();
    }

    // Set BootCurrent variable
    LoadOptionIndexSize = sizeof(UINT16);
    gRT->SetVariable (L"BootCurrent", &gEfiGlobalVariableGuid,
              EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
              LoadOptionIndexSize, &(BootOption->LoadOptionIndex));

    Status = BdsStartEfiApplication (gImageHandle, BootOption->FilePathList, BootOption->OptionalDataSize, BootOption->OptionalData);

    // Clear BootCurrent variable
    LoadOptionIndexSize = sizeof(UINT16);
    gRT->SetVariable (L"BootCurrent", &gEfiGlobalVariableGuid,
              EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
              0, NULL);
  }

  return Status;
}

EFI_STATUS
BootOptionList (
  IN OUT LIST_ENTRY *BootOptionList
  )
{
  EFI_STATUS                    Status;
  UINTN                         Index;
  UINT16*                       BootOrder;
  UINTN                         BootOrderSize;
  BDS_LOAD_OPTION*              BdsLoadOption;
  BDS_LOAD_OPTION_ENTRY*        BdsLoadOptionEntry;

  InitializeListHead (BootOptionList);

  // Get the Boot Option Order from the environment variable
  Status = GetGlobalEnvironmentVariable (L"BootOrder", NULL, &BootOrderSize, (VOID**)&BootOrder);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  for (Index = 0; Index < BootOrderSize / sizeof (UINT16); Index++) {
    Status = BootOptionFromLoadOptionIndex (BootOrder[Index], &BdsLoadOption);
    if (!EFI_ERROR(Status)) {
      BdsLoadOptionEntry = (BDS_LOAD_OPTION_ENTRY*)AllocatePool(sizeof(BDS_LOAD_OPTION_ENTRY));
      BdsLoadOptionEntry->BdsLoadOption = BdsLoadOption;
      InsertTailList (BootOptionList,&BdsLoadOptionEntry->Link);
    }
  }

  FreePool (BootOrder);

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
BootOptionSetFields (
  IN BDS_LOAD_OPTION*           BootOption,
  IN UINT32                     Attributes,
  IN CHAR16*                    BootDescription,
  IN EFI_DEVICE_PATH_PROTOCOL*  DevicePath,
  IN ARM_BDS_LOADER_TYPE        BootType,
  IN UINT8*                     OptionalData,
  IN UINTN                      OptionalDataSize
  )
{
  EFI_LOAD_OPTION               *EfiLoadOption;
  UINTN                         EfiLoadOptionSize;
  UINTN                         BootDescriptionSize;
  UINT16                        FilePathListLength;
  UINT8*                        EfiLoadOptionPtr;
  UINT8*                        InitrdPathListPtr;
  ARM_BDS_LINUX_ARGUMENTS*      DestLinuxArguments;
  ARM_BDS_LINUX_ARGUMENTS*      SrcLinuxArguments;
  ARM_BDS_LOADER_ARGUMENTS*     BootArguments;

  // If we are overwriting an existent Boot Option then we have to free previously allocated memory
  if (BootOption->LoadOption) {
    FreePool (BootOption->LoadOption);
  }

  BootDescriptionSize = StrSize (BootDescription);

  // Fixup the size in case of entry specific to ArmPlatformPkg/Bds
  if ((BootType == BDS_LOADER_KERNEL_LINUX_ATAG) || (BootType == BDS_LOADER_KERNEL_LINUX_FDT)) {
    OptionalDataSize += sizeof(ARM_BDS_LOADER_OPTIONAL_DATA_HEADER);
  }

  // Compute the size of the FilePath list
  FilePathListLength = GetUnalignedDevicePathSize (DevicePath);

  // Allocate the memory for the EFI Load Option
  EfiLoadOptionSize = sizeof(UINT32) + sizeof(UINT16) + BootDescriptionSize + FilePathListLength + OptionalDataSize;
  EfiLoadOption = (EFI_LOAD_OPTION *)AllocatePool(EfiLoadOptionSize);
  EfiLoadOptionPtr = (UINT8 *)EfiLoadOption;

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
  CopyMem (EfiLoadOptionPtr, DevicePath, FilePathListLength);
  EfiLoadOptionPtr += FilePathListLength;

  // Optional Data fields, Do unaligned writes
  BootOption->OptionalData = EfiLoadOptionPtr;

  if ((BootType == BDS_LOADER_KERNEL_LINUX_ATAG) || (BootType == BDS_LOADER_KERNEL_LINUX_FDT)) {
    // Write the header
    WriteUnaligned32 ((UINT32 *)EfiLoadOptionPtr, ARM_BDS_OPTIONAL_DATA_SIGNATURE);
    WriteUnaligned32 ((UINT32 *)(EfiLoadOptionPtr + 4), BootType);

    // OptionalData should have been initialized by the caller of this function
    ASSERT (OptionalData != NULL);
    BootArguments = (ARM_BDS_LOADER_ARGUMENTS*)OptionalData;
    SrcLinuxArguments = &(BootArguments->LinuxArguments);
    DestLinuxArguments = &((ARM_BDS_LOADER_OPTIONAL_DATA*)EfiLoadOptionPtr)->Arguments.LinuxArguments;

    WriteUnaligned16 ((UINT16 *)&(DestLinuxArguments->CmdLineSize), SrcLinuxArguments->CmdLineSize);
    WriteUnaligned16 ((UINT16 *)&(DestLinuxArguments->InitrdSize), SrcLinuxArguments->InitrdSize);

    if (SrcLinuxArguments->CmdLineSize > 0) {
      CopyMem ((VOID*)(DestLinuxArguments + 1), (VOID*)(SrcLinuxArguments + 1), SrcLinuxArguments->CmdLineSize);
    }

    if (SrcLinuxArguments->InitrdSize > 0) {
      InitrdPathListPtr = (UINT8*)((UINTN)(DestLinuxArguments + 1) + SrcLinuxArguments->CmdLineSize);
      CopyMem (InitrdPathListPtr, (VOID*)((UINTN)(SrcLinuxArguments + 1) + SrcLinuxArguments->CmdLineSize), SrcLinuxArguments->InitrdSize);
    }
  } else {
    if (OptionalData != NULL) {
      CopyMem (BootOption->OptionalData, OptionalData, OptionalDataSize);
    }
  }
  BootOption->OptionalDataSize = OptionalDataSize;

  // If this function is called at the creation of the Boot Device entry (not at the update) the
  // BootOption->LoadOptionSize must be zero then we get a new BootIndex for this entry
  if (BootOption->LoadOptionSize == 0) {
    BootOption->LoadOptionIndex = BootOptionAllocateBootIndex ();
  }

  // Fill the EFI Load option fields
  BootOption->LoadOption = EfiLoadOption;
  BootOption->LoadOptionSize = EfiLoadOptionSize;

  return EFI_SUCCESS;
}

EFI_STATUS
BootOptionCreate (
  IN  UINT32                    Attributes,
  IN  CHAR16*                   BootDescription,
  IN  EFI_DEVICE_PATH_PROTOCOL* DevicePath,
  IN  ARM_BDS_LOADER_TYPE       BootType,
  IN  UINT8*                    OptionalData,
  IN  UINTN                     OptionalDataSize,
  OUT BDS_LOAD_OPTION**         BdsLoadOption
  )
{
  EFI_STATUS                    Status;
  BDS_LOAD_OPTION_ENTRY*        BootOptionEntry;
  BDS_LOAD_OPTION*              BootOption;
  CHAR16                        BootVariableName[9];
  UINT16*                       BootOrder;
  UINTN                         BootOrderSize;

  //
  // Allocate and fill the memory for the BDS Load Option structure
  //
  BootOptionEntry = (BDS_LOAD_OPTION_ENTRY*)AllocatePool (sizeof (BDS_LOAD_OPTION_ENTRY));
  InitializeListHead (&BootOptionEntry->Link);
  BootOptionEntry->BdsLoadOption = (BDS_LOAD_OPTION*)AllocateZeroPool (sizeof(BDS_LOAD_OPTION));

  BootOption = BootOptionEntry->BdsLoadOption;
  BootOptionSetFields (BootOption, Attributes, BootDescription, DevicePath, BootType, OptionalData, OptionalDataSize);

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
  Status = GetGlobalEnvironmentVariable (L"BootOrder", NULL, &BootOrderSize, (VOID**)&BootOrder);
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

  // We only free it if the UEFI Variable 'BootOrder' was already existing
  if (BootOrderSize > sizeof(UINT16)) {
    FreePool (BootOrder);
  }

  *BdsLoadOption = BootOption;
  return Status;
}

EFI_STATUS
BootOptionUpdate (
  IN  BDS_LOAD_OPTION*          BdsLoadOption,
  IN  UINT32                    Attributes,
  IN  CHAR16*                   BootDescription,
  IN  EFI_DEVICE_PATH_PROTOCOL* DevicePath,
  IN  ARM_BDS_LOADER_TYPE       BootType,
  IN UINT8*                     OptionalData,
  IN UINTN                      OptionalDataSize
  )
{
  EFI_STATUS      Status;
  CHAR16          BootVariableName[9];

  // Update the BDS Load Option structure
  BootOptionSetFields (BdsLoadOption, Attributes, BootDescription, DevicePath, BootType, OptionalData, OptionalDataSize);

  // Update the related environment variables
  UnicodeSPrint (BootVariableName, 9 * sizeof(CHAR16), L"Boot%04X", BdsLoadOption->LoadOptionIndex);

  Status = gRT->SetVariable (
      BootVariableName,
      &gEfiGlobalVariableGuid,
      EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
      BdsLoadOption->LoadOptionSize,
      BdsLoadOption->LoadOption
      );

  return Status;
}

EFI_STATUS
BootOptionDelete (
  IN  BDS_LOAD_OPTION *BootOption
  )
{
  UINTN         Index;
  UINTN         BootOrderSize;
  UINT16*       BootOrder;
  UINTN         BootOrderCount;
  CHAR16        BootVariableName[9];
  EFI_STATUS    Status;

  // Remove the entry from the BootOrder environment variable
  Status = GetGlobalEnvironmentVariable (L"BootOrder", NULL, &BootOrderSize, (VOID**)&BootOrder);
  if (!EFI_ERROR(Status)) {
    BootOrderCount = BootOrderSize / sizeof(UINT16);

    // Find the index of the removed entry
    for (Index = 0; Index < BootOrderCount; Index++) {
      if (BootOrder[Index] == BootOption->LoadOptionIndex) {
        // If it the last entry we do not need to rearrange the BootOrder list
        if (Index + 1 != BootOrderCount) {
          CopyMem (
            &BootOrder[Index],
            &BootOrder[Index + 1],
            (BootOrderCount - (Index + 1)) * sizeof(UINT16)
            );
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

  // Delete Boot#### environment variable
  UnicodeSPrint (BootVariableName, 9 * sizeof(CHAR16), L"Boot%04X", BootOption->LoadOptionIndex);
  Status = gRT->SetVariable (
      BootVariableName,
      &gEfiGlobalVariableGuid,
      EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
      0,
      NULL
      );

  FreePool (BootOrder);

  return Status;
}
