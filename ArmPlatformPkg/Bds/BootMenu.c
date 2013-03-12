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

#include <Guid/ArmGlobalVariableHob.h>

extern EFI_HANDLE mImageHandle;
extern BDS_LOAD_OPTION_SUPPORT *BdsLoadOptionSupportList;


EFI_STATUS
SelectBootDevice (
  OUT BDS_SUPPORTED_DEVICE** SupportedBootDevice
  )
{
  EFI_STATUS  Status;
  LIST_ENTRY  SupportedDeviceList;
  UINTN       SupportedDeviceCount;
  LIST_ENTRY* Entry;
  UINTN       SupportedDeviceSelected;
  UINTN       Index;

  //
  // List the Boot Devices supported
  //

  // Start all the drivers first
  BdsConnectAllDrivers ();

  // List the supported devices
  Status = BootDeviceListSupportedInit (&SupportedDeviceList);
  ASSERT_EFI_ERROR(Status);

  SupportedDeviceCount = 0;
  for (Entry = GetFirstNode (&SupportedDeviceList);
       !IsNull (&SupportedDeviceList,Entry);
       Entry = GetNextNode (&SupportedDeviceList,Entry)
       )
  {
    *SupportedBootDevice = SUPPORTED_BOOT_DEVICE_FROM_LINK(Entry);
    Print(L"[%d] %s\n",SupportedDeviceCount+1,(*SupportedBootDevice)->Description);

    DEBUG_CODE_BEGIN();
      CHAR16*                           DevicePathTxt;
      EFI_DEVICE_PATH_TO_TEXT_PROTOCOL* DevicePathToTextProtocol;

      Status = gBS->LocateProtocol (&gEfiDevicePathToTextProtocolGuid, NULL, (VOID **)&DevicePathToTextProtocol);
      ASSERT_EFI_ERROR(Status);
      DevicePathTxt = DevicePathToTextProtocol->ConvertDevicePathToText ((*SupportedBootDevice)->DevicePathProtocol,TRUE,TRUE);

      Print(L"\t- %s\n",DevicePathTxt);

      FreePool(DevicePathTxt);
    DEBUG_CODE_END();

    SupportedDeviceCount++;
  }

  if (SupportedDeviceCount == 0) {
    Print(L"There is no supported device.\n");
    Status = EFI_ABORTED;
    goto EXIT;
  }

  //
  // Select the Boot Device
  //
  SupportedDeviceSelected = 0;
  while (SupportedDeviceSelected == 0) {
    Print(L"Select the Boot Device: ");
    Status = GetHIInputInteger (&SupportedDeviceSelected);
    if (EFI_ERROR(Status)) {
      Status = EFI_ABORTED;
      goto EXIT;
    } else if ((SupportedDeviceSelected == 0) || (SupportedDeviceSelected > SupportedDeviceCount)) {
      Print(L"Invalid input (max %d)\n",SupportedDeviceCount);
      SupportedDeviceSelected = 0;
    }
  }

  //
  // Get the Device Path for the selected boot device
  //
  Index = 1;
  for (Entry = GetFirstNode (&SupportedDeviceList);
       !IsNull (&SupportedDeviceList,Entry);
       Entry = GetNextNode (&SupportedDeviceList,Entry)
       )
  {
    if (Index == SupportedDeviceSelected) {
      *SupportedBootDevice = SUPPORTED_BOOT_DEVICE_FROM_LINK(Entry);
      break;
    }
    Index++;
  }

EXIT:
  BootDeviceListSupportedFree (&SupportedDeviceList, *SupportedBootDevice);
  return Status;
}

EFI_STATUS
BootMenuAddBootOption (
  IN LIST_ENTRY *BootOptionsList
  )
{
  EFI_STATUS                Status;
  BDS_SUPPORTED_DEVICE*     SupportedBootDevice;
  ARM_BDS_LOADER_ARGUMENTS* BootArguments;
  CHAR16                    BootDescription[BOOT_DEVICE_DESCRIPTION_MAX];
  CHAR8                     CmdLine[BOOT_DEVICE_OPTION_MAX];
  UINT32                    Attributes;
  ARM_BDS_LOADER_TYPE       BootType;
  BDS_LOAD_OPTION_ENTRY     *BdsLoadOptionEntry;
  EFI_DEVICE_PATH           *DevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePathNodes;
  EFI_DEVICE_PATH_PROTOCOL  *InitrdPathNodes;
  EFI_DEVICE_PATH_PROTOCOL  *InitrdPath;
  UINTN                     CmdLineSize;
  BOOLEAN                   InitrdSupport;
  UINTN                     InitrdSize;

  Attributes                = 0;
  SupportedBootDevice = NULL;

  // List the Boot Devices supported
  Status = SelectBootDevice (&SupportedBootDevice);
  if (EFI_ERROR(Status)) {
    Status = EFI_ABORTED;
    goto EXIT;
  }

  // Create the specific device path node
  Status = SupportedBootDevice->Support->CreateDevicePathNode (L"EFI Application or the kernel", &DevicePathNodes, &BootType, &Attributes);
  if (EFI_ERROR(Status)) {
    Status = EFI_ABORTED;
    goto EXIT;
  }
  // Append the Device Path to the selected device path
  DevicePath = AppendDevicePath (SupportedBootDevice->DevicePathProtocol, (CONST EFI_DEVICE_PATH_PROTOCOL *)DevicePathNodes);
  if (DevicePath == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }

  if ((BootType == BDS_LOADER_KERNEL_LINUX_ATAG) || (BootType == BDS_LOADER_KERNEL_LINUX_FDT)) {
    Print(L"Add an initrd: ");
    Status = GetHIInputBoolean (&InitrdSupport);
    if (EFI_ERROR(Status)) {
      Status = EFI_ABORTED;
      goto EXIT;
    }

    if (InitrdSupport) {
      // Create the specific device path node
      Status = SupportedBootDevice->Support->CreateDevicePathNode (L"initrd", &InitrdPathNodes, NULL, NULL);
      if (EFI_ERROR(Status) && Status != EFI_NOT_FOUND) { // EFI_NOT_FOUND is returned on empty input string, but we can boot without an initrd
        Status = EFI_ABORTED;
        goto EXIT;
      }

      if (InitrdPathNodes != NULL) {
        // Append the Device Path to the selected device path
        InitrdPath = AppendDevicePath (SupportedBootDevice->DevicePathProtocol, (CONST EFI_DEVICE_PATH_PROTOCOL *)InitrdPathNodes);
        if (InitrdPath == NULL) {
          Status = EFI_OUT_OF_RESOURCES;
          goto EXIT;
        }
      } else {
        InitrdPath = NULL;
      }
    } else {
      InitrdPath = NULL;
    }

    Print(L"Arguments to pass to the binary: ");
    Status = GetHIInputAscii (CmdLine,BOOT_DEVICE_OPTION_MAX);
    if (EFI_ERROR(Status)) {
      Status = EFI_ABORTED;
      goto FREE_DEVICE_PATH;
    }

    CmdLineSize = AsciiStrSize (CmdLine);
    InitrdSize = GetDevicePathSize (InitrdPath);

    BootArguments = (ARM_BDS_LOADER_ARGUMENTS*)AllocatePool (sizeof(ARM_BDS_LOADER_ARGUMENTS) + CmdLineSize + InitrdSize);

    BootArguments->LinuxArguments.CmdLineSize = CmdLineSize;
    BootArguments->LinuxArguments.InitrdSize = InitrdSize;
    CopyMem ((VOID*)(&BootArguments->LinuxArguments + 1), CmdLine, CmdLineSize);
    CopyMem ((VOID*)((UINTN)(&BootArguments->LinuxArguments + 1) + CmdLineSize), InitrdPath, InitrdSize);
  } else {
    BootArguments = NULL;
  }

  Print(L"Description for this new Entry: ");
  Status = GetHIInputStr (BootDescription, BOOT_DEVICE_DESCRIPTION_MAX);
  if (EFI_ERROR(Status)) {
    Status = EFI_ABORTED;
    goto FREE_DEVICE_PATH;
  }

  // Create new entry
  BdsLoadOptionEntry = (BDS_LOAD_OPTION_ENTRY*)AllocatePool (sizeof(BDS_LOAD_OPTION_ENTRY));
  Status = BootOptionCreate (Attributes, BootDescription, DevicePath, BootType, BootArguments, &BdsLoadOptionEntry->BdsLoadOption);
  if (!EFI_ERROR(Status)) {
    InsertTailList (BootOptionsList, &BdsLoadOptionEntry->Link);
  }

FREE_DEVICE_PATH:
  FreePool (DevicePath);

EXIT:
  if (Status == EFI_ABORTED) {
    Print(L"\n");
  }
  FreePool(SupportedBootDevice);
  return Status;
}

STATIC
EFI_STATUS
BootMenuSelectBootOption (
  IN  LIST_ENTRY*               BootOptionsList,
  IN  CONST CHAR16*             InputStatement,
  IN  BOOLEAN                   OnlyArmBdsBootEntry,
  OUT BDS_LOAD_OPTION_ENTRY**   BdsLoadOptionEntry
  )
{
  EFI_STATUS                    Status;
  LIST_ENTRY*                   Entry;
  BDS_LOAD_OPTION*              BdsLoadOption;
  UINTN                         BootOptionSelected;
  UINTN                         BootOptionCount;
  UINTN                         Index;

  // Display the list of supported boot devices
  BootOptionCount = 0;
  for (Entry = GetFirstNode (BootOptionsList);
       !IsNull (BootOptionsList,Entry);
       Entry = GetNextNode (BootOptionsList, Entry)
       )
  {
    BdsLoadOption = LOAD_OPTION_FROM_LINK(Entry);

    if (OnlyArmBdsBootEntry && !IS_ARM_BDS_BOOTENTRY (BdsLoadOption)) {
      continue;
    }

    Print (L"[%d] %s\n", (BootOptionCount + 1), BdsLoadOption->Description);

    DEBUG_CODE_BEGIN();
      CHAR16*                           DevicePathTxt;
      EFI_DEVICE_PATH_TO_TEXT_PROTOCOL* DevicePathToTextProtocol;
      ARM_BDS_LOADER_TYPE               LoaderType;
      ARM_BDS_LOADER_OPTIONAL_DATA*     OptionalData;

      Status = gBS->LocateProtocol(&gEfiDevicePathToTextProtocolGuid, NULL, (VOID **)&DevicePathToTextProtocol);
      ASSERT_EFI_ERROR(Status);
      DevicePathTxt = DevicePathToTextProtocol->ConvertDevicePathToText(BdsLoadOption->FilePathList,TRUE,TRUE);

      Print(L"\t- %s\n",DevicePathTxt);
      OptionalData = BdsLoadOption->OptionalData;
      LoaderType = (ARM_BDS_LOADER_TYPE)ReadUnaligned32 ((CONST UINT32*)&OptionalData->Header.LoaderType);
      if ((LoaderType == BDS_LOADER_KERNEL_LINUX_ATAG) || (LoaderType == BDS_LOADER_KERNEL_LINUX_FDT)) {
        Print (L"\t- Arguments: %a\n",&OptionalData->Arguments.LinuxArguments + 1);
      }

      FreePool(DevicePathTxt);
    DEBUG_CODE_END();

    BootOptionCount++;
  }

  // Check if a valid boot option(s) is found
  if (BootOptionCount == 0) {
    if (StrCmp (InputStatement, DELETE_BOOT_ENTRY) == 0) {
      Print (L"Nothing to remove!\n");
    } else if (StrCmp (InputStatement, UPDATE_BOOT_ENTRY) == 0) {
      Print (L"Couldn't find valid boot entries\n");
    } else{
      Print (L"No supported Boot Entry.\n");
    }

    return EFI_NOT_FOUND;
  }

  // Get the index of the boot device to delete
  BootOptionSelected = 0;
  while (BootOptionSelected == 0) {
    Print(InputStatement);
    Status = GetHIInputInteger (&BootOptionSelected);
    if (EFI_ERROR(Status)) {
      return Status;
    } else if ((BootOptionSelected == 0) || (BootOptionSelected > BootOptionCount)) {
      Print(L"Invalid input (max %d)\n",BootOptionCount);
      BootOptionSelected = 0;
    }
  }

  // Get the structure of the Boot device to delete
  Index = 1;
  for (Entry = GetFirstNode (BootOptionsList);
       !IsNull (BootOptionsList, Entry);
       Entry = GetNextNode (BootOptionsList,Entry)
       )
  {
    if (Index == BootOptionSelected) {
      *BdsLoadOptionEntry = LOAD_OPTION_ENTRY_FROM_LINK(Entry);
      break;
    }
    Index++;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
BootMenuRemoveBootOption (
  IN LIST_ENTRY *BootOptionsList
  )
{
  EFI_STATUS                    Status;
  BDS_LOAD_OPTION_ENTRY*        BootOptionEntry;

  Status = BootMenuSelectBootOption (BootOptionsList, DELETE_BOOT_ENTRY, FALSE, &BootOptionEntry);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  // If the Boot Option was attached to a list remove it
  if (!IsListEmpty (&BootOptionEntry->Link)) {
    // Remove the entry from the list
    RemoveEntryList (&BootOptionEntry->Link);
  }

  // Delete the BDS Load option structures
  BootOptionDelete (BootOptionEntry->BdsLoadOption);

  return EFI_SUCCESS;
}

EFI_STATUS
BootMenuUpdateBootOption (
  IN LIST_ENTRY *BootOptionsList
  )
{
  EFI_STATUS                    Status;
  BDS_LOAD_OPTION_ENTRY         *BootOptionEntry;
  BDS_LOAD_OPTION               *BootOption;
  BDS_LOAD_OPTION_SUPPORT*      DeviceSupport;
  ARM_BDS_LOADER_ARGUMENTS*     BootArguments;
  CHAR16                        BootDescription[BOOT_DEVICE_DESCRIPTION_MAX];
  CHAR8                         CmdLine[BOOT_DEVICE_OPTION_MAX];
  EFI_DEVICE_PATH               *DevicePath;
  EFI_DEVICE_PATH               *TempInitrdPath;
  ARM_BDS_LOADER_TYPE           BootType;
  ARM_BDS_LOADER_OPTIONAL_DATA* OptionalData;
  ARM_BDS_LINUX_ARGUMENTS*      LinuxArguments;
  EFI_DEVICE_PATH               *InitrdPathNodes;
  EFI_DEVICE_PATH               *InitrdPath;
  UINTN                         InitrdSize;
  UINTN                         CmdLineSize;
  BOOLEAN                       InitrdSupport;

  Status = BootMenuSelectBootOption (BootOptionsList, UPDATE_BOOT_ENTRY, TRUE, &BootOptionEntry);
  if (EFI_ERROR(Status)) {
    return Status;
  }
  BootOption = BootOptionEntry->BdsLoadOption;

  // Get the device support for this Boot Option
  Status = BootDeviceGetDeviceSupport (BootOption->FilePathList, &DeviceSupport);
  if (EFI_ERROR(Status)) {
    Print(L"Not possible to retrieve the supported device for the update\n");
    return EFI_UNSUPPORTED;
  }

  Status = DeviceSupport->UpdateDevicePathNode (BootOption->FilePathList, L"EFI Application or the kernel", &DevicePath, NULL, NULL);
  if (EFI_ERROR(Status)) {
    Status = EFI_ABORTED;
    goto EXIT;
  }

  OptionalData = BootOption->OptionalData;
  BootType = (ARM_BDS_LOADER_TYPE)ReadUnaligned32 ((UINT32 *)(&OptionalData->Header.LoaderType));

  if ((BootType == BDS_LOADER_KERNEL_LINUX_ATAG) || (BootType == BDS_LOADER_KERNEL_LINUX_FDT)) {
    LinuxArguments = &OptionalData->Arguments.LinuxArguments;

    CmdLineSize = ReadUnaligned16 ((CONST UINT16*)&LinuxArguments->CmdLineSize);

    InitrdSize = ReadUnaligned16 ((CONST UINT16*)&LinuxArguments->InitrdSize);
    if (InitrdSize > 0) {
      Print(L"Keep the initrd: ");
    } else {
      Print(L"Add an initrd: ");
    }
    Status = GetHIInputBoolean (&InitrdSupport);
    if (EFI_ERROR(Status)) {
      Status = EFI_ABORTED;
      goto EXIT;
    }

    if (InitrdSupport) {
      if (InitrdSize > 0) {
        // Case we update the initrd device path
        Status = DeviceSupport->UpdateDevicePathNode ((EFI_DEVICE_PATH*)((UINTN)(LinuxArguments + 1) + CmdLineSize), L"initrd", &InitrdPath, NULL, NULL);
        if (EFI_ERROR(Status) && Status != EFI_NOT_FOUND) {// EFI_NOT_FOUND is returned on empty input string, but we can boot without an initrd
          Status = EFI_ABORTED;
          goto EXIT;
        }
        InitrdSize = GetDevicePathSize (InitrdPath);
      } else {
        // Case we create the initrd device path

        Status = DeviceSupport->CreateDevicePathNode (L"initrd", &InitrdPathNodes, NULL, NULL);
        if (EFI_ERROR(Status) && Status != EFI_NOT_FOUND) { // EFI_NOT_FOUND is returned on empty input string, but we can boot without an initrd
          Status = EFI_ABORTED;
          goto EXIT;
        }

        if (InitrdPathNodes != NULL) {
          // Duplicate Linux kernel Device Path
          TempInitrdPath = DuplicateDevicePath (BootOption->FilePathList);
          // Replace Linux kernel Node by EndNode
          SetDevicePathEndNode (GetLastDevicePathNode (TempInitrdPath));
          // Append the Device Path to the selected device path
          InitrdPath = AppendDevicePath (TempInitrdPath, (CONST EFI_DEVICE_PATH_PROTOCOL *)InitrdPathNodes);
          FreePool (TempInitrdPath);
          if (InitrdPath == NULL) {
            Status = EFI_OUT_OF_RESOURCES;
            goto EXIT;
          }
          InitrdSize = GetDevicePathSize (InitrdPath);
        } else {
          InitrdPath = NULL;
        }
      }
    } else {
      InitrdSize = 0;
    }

    Print(L"Arguments to pass to the binary: ");
    if (CmdLineSize > 0) {
      AsciiStrnCpy(CmdLine, (CONST CHAR8*)(LinuxArguments + 1), CmdLineSize);
    } else {
      CmdLine[0] = '\0';
    }
    Status = EditHIInputAscii (CmdLine, BOOT_DEVICE_OPTION_MAX);
    if (EFI_ERROR(Status)) {
      Status = EFI_ABORTED;
      goto FREE_DEVICE_PATH;
    }

    CmdLineSize = AsciiStrSize (CmdLine);

    BootArguments = (ARM_BDS_LOADER_ARGUMENTS*)AllocatePool(sizeof(ARM_BDS_LOADER_ARGUMENTS) + CmdLineSize + InitrdSize);
    BootArguments->LinuxArguments.CmdLineSize = CmdLineSize;
    BootArguments->LinuxArguments.InitrdSize = InitrdSize;
    CopyMem (&BootArguments->LinuxArguments + 1, CmdLine, CmdLineSize);
    CopyMem ((VOID*)((UINTN)(&BootArguments->LinuxArguments + 1) + CmdLineSize), InitrdPath, InitrdSize);
  } else {
    BootArguments = NULL;
  }

  Print(L"Description for this new Entry: ");
  StrnCpy (BootDescription, BootOption->Description, BOOT_DEVICE_DESCRIPTION_MAX);
  Status = EditHIInputStr (BootDescription, BOOT_DEVICE_DESCRIPTION_MAX);
  if (EFI_ERROR(Status)) {
    Status = EFI_ABORTED;
    goto FREE_DEVICE_PATH;
  }

  // Update the entry
  Status = BootOptionUpdate (BootOption, BootOption->Attributes, BootDescription, DevicePath, BootType, BootArguments);

FREE_DEVICE_PATH:
  FreePool (DevicePath);

EXIT:
  if (Status == EFI_ABORTED) {
    Print(L"\n");
  }
  return Status;
}

EFI_STATUS
UpdateFdtPath (
  IN LIST_ENTRY *BootOptionsList
  )
{
  EFI_STATUS                Status;
  UINTN                     FdtDevicePathSize;
  BDS_SUPPORTED_DEVICE      *SupportedBootDevice;
  EFI_DEVICE_PATH_PROTOCOL  *FdtDevicePathNodes;
  EFI_DEVICE_PATH_PROTOCOL  *FdtDevicePath;

  Status = SelectBootDevice (&SupportedBootDevice);
  if (EFI_ERROR(Status)) {
    Status = EFI_ABORTED;
    goto EXIT;
  }

  // Create the specific device path node
  Status = SupportedBootDevice->Support->CreateDevicePathNode (L"FDT blob", &FdtDevicePathNodes, NULL, NULL);
  if (EFI_ERROR(Status)) {
    Status = EFI_ABORTED;
    goto EXIT;
  }

  if (FdtDevicePathNodes != NULL) {
    // Append the Device Path node to the select device path
    FdtDevicePath = AppendDevicePath (SupportedBootDevice->DevicePathProtocol, FdtDevicePathNodes);
    FdtDevicePathSize = GetDevicePathSize (FdtDevicePath);
    Status = gRT->SetVariable (
                    (CHAR16*)L"Fdt",
                    &gArmGlobalVariableGuid,
                    EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                    FdtDevicePathSize,
                    FdtDevicePath
                    );
    ASSERT_EFI_ERROR(Status);
  } else {
    gRT->SetVariable (
           (CHAR16*)L"Fdt",
           &gArmGlobalVariableGuid,
           EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
           0,
           NULL
           );
    ASSERT_EFI_ERROR(Status);
  }

EXIT:
  if (Status == EFI_ABORTED) {
    Print(L"\n");
  }
  FreePool(SupportedBootDevice);
  return Status;
}

struct BOOT_MANAGER_ENTRY {
  CONST CHAR16* Description;
  EFI_STATUS (*Callback) (IN LIST_ENTRY *BootOptionsList);
} BootManagerEntries[] = {
    { L"Add Boot Device Entry", BootMenuAddBootOption },
    { L"Update Boot Device Entry", BootMenuUpdateBootOption },
    { L"Remove Boot Device Entry", BootMenuRemoveBootOption },
    { L"Update FDT path", UpdateFdtPath },
};

EFI_STATUS
BootMenuManager (
  IN LIST_ENTRY *BootOptionsList
  )
{
  UINTN Index;
  UINTN OptionSelected;
  UINTN BootManagerEntryCount;
  EFI_STATUS Status;

  BootManagerEntryCount = sizeof(BootManagerEntries) / sizeof(struct BOOT_MANAGER_ENTRY);

  while (TRUE) {
    // Display Boot Manager menu
    for (Index = 0; Index < BootManagerEntryCount; Index++) {
      Print(L"[%d] %s\n",Index+1,BootManagerEntries[Index]);
    }
    Print(L"[%d] Return to main menu\n",Index+1);

    // Select which entry to call
    Print(L"Choice: ");
    Status = GetHIInputInteger (&OptionSelected);
    if (EFI_ERROR(Status) || (OptionSelected == (BootManagerEntryCount+1))) {
      if (EFI_ERROR(Status)) {
        Print(L"\n");
      }
      return EFI_SUCCESS;
    } else if ((OptionSelected > 0) && (OptionSelected <= BootManagerEntryCount))  {
      BootManagerEntries[OptionSelected-1].Callback (BootOptionsList);
    }
  }
  // Should never go here
}

EFI_STATUS
BootShell (
  IN LIST_ENTRY *BootOptionsList
  )
{
  EFI_STATUS Status;

  // Start EFI Shell
  Status = BdsLoadApplication (mImageHandle, L"Shell", 0, NULL);
  if (Status == EFI_NOT_FOUND) {
    Print (L"Error: EFI Application not found.\n");
  } else if (EFI_ERROR(Status)) {
    Print (L"Error: Status Code: 0x%X\n",(UINT32)Status);
  }

  return Status;
}

struct BOOT_MAIN_ENTRY {
  CONST CHAR16* Description;
  EFI_STATUS (*Callback) (IN LIST_ENTRY *BootOptionsList);
} BootMainEntries[] = {
    { L"Shell", BootShell },
    { L"Boot Manager", BootMenuManager },
};


EFI_STATUS
BootMenuMain (
  VOID
  )
{
  LIST_ENTRY                    BootOptionsList;
  UINTN                         OptionCount;
  UINTN                         BootOptionCount;
  EFI_STATUS                    Status;
  LIST_ENTRY*                   Entry;
  BDS_LOAD_OPTION*              BootOption;
  UINTN                         BootOptionSelected;
  UINTN                         Index;
  UINTN                         BootMainEntryCount;

  BootOption              = NULL;
  BootMainEntryCount = sizeof(BootMainEntries) / sizeof(struct BOOT_MAIN_ENTRY);

  while (TRUE) {
    // Get Boot#### list
    BootOptionList (&BootOptionsList);

    OptionCount = 1;

    // Display the Boot options
    for (Entry = GetFirstNode (&BootOptionsList);
         !IsNull (&BootOptionsList,Entry);
         Entry = GetNextNode (&BootOptionsList,Entry)
         )
    {
      BootOption = LOAD_OPTION_FROM_LINK(Entry);

      Print(L"[%d] %s\n", OptionCount, BootOption->Description);

      DEBUG_CODE_BEGIN();
        CHAR16*                           DevicePathTxt;
        EFI_DEVICE_PATH_TO_TEXT_PROTOCOL* DevicePathToTextProtocol;
        ARM_BDS_LOADER_OPTIONAL_DATA*     OptionalData;
        UINTN                             CmdLineSize;
        ARM_BDS_LOADER_TYPE               LoaderType;

        Status = gBS->LocateProtocol (&gEfiDevicePathToTextProtocolGuid, NULL, (VOID **)&DevicePathToTextProtocol);
        if (EFI_ERROR(Status)) {
          // You must provide an implementation of DevicePathToTextProtocol in your firmware (eg: DevicePathDxe)
          DEBUG((EFI_D_ERROR,"Error: Bds requires DevicePathToTextProtocol\n"));
          return Status;
        }
        DevicePathTxt = DevicePathToTextProtocol->ConvertDevicePathToText (BootOption->FilePathList, TRUE, TRUE);

        Print(L"\t- %s\n",DevicePathTxt);

        // If it is a supported BootEntry then print its details
        if (IS_ARM_BDS_BOOTENTRY (BootOption)) {
          OptionalData = BootOption->OptionalData;
          LoaderType = (ARM_BDS_LOADER_TYPE)ReadUnaligned32 ((CONST UINT32*)&OptionalData->Header.LoaderType);
          if ((LoaderType == BDS_LOADER_KERNEL_LINUX_ATAG) || (LoaderType == BDS_LOADER_KERNEL_LINUX_FDT)) {
            if (ReadUnaligned16 (&OptionalData->Arguments.LinuxArguments.InitrdSize) > 0) {
              CmdLineSize = ReadUnaligned16 (&OptionalData->Arguments.LinuxArguments.CmdLineSize);
              DevicePathTxt = DevicePathToTextProtocol->ConvertDevicePathToText (
                  GetAlignedDevicePath ((EFI_DEVICE_PATH*)((UINTN)(&OptionalData->Arguments.LinuxArguments + 1) + CmdLineSize)), TRUE, TRUE);
              Print(L"\t- Initrd: %s\n", DevicePathTxt);
            }
            Print(L"\t- Arguments: %a\n", (&OptionalData->Arguments.LinuxArguments + 1));
          }

          switch (LoaderType) {
            case BDS_LOADER_EFI_APPLICATION:
              Print(L"\t- LoaderType: EFI Application\n");
              break;

            case BDS_LOADER_KERNEL_LINUX_ATAG:
              Print(L"\t- LoaderType: Linux kernel with ATAG support\n");
              break;

            case BDS_LOADER_KERNEL_LINUX_FDT:
              Print(L"\t- LoaderType: Linux kernel with FDT support\n");
              break;

            default:
              Print(L"\t- LoaderType: Not recognized (%d)\n", LoaderType);
          }
        }
        FreePool(DevicePathTxt);
      DEBUG_CODE_END();

      OptionCount++;
    }
    BootOptionCount = OptionCount-1;

    // Display the hardcoded Boot entries
    for (Index = 0; Index < BootMainEntryCount; Index++) {
      Print(L"[%d] %s\n",OptionCount,BootMainEntries[Index]);
      OptionCount++;
    }

    // Request the boot entry from the user
    BootOptionSelected = 0;
    while (BootOptionSelected == 0) {
      Print(L"Start: ");
      Status = GetHIInputInteger (&BootOptionSelected);
      if (EFI_ERROR(Status) || (BootOptionSelected == 0) || (BootOptionSelected > OptionCount)) {
        Print(L"Invalid input (max %d)\n",(OptionCount-1));
        BootOptionSelected = 0;
      }
    }

    // Start the selected entry
    if (BootOptionSelected > BootOptionCount) {
      // Start the hardcoded entry
      Status = BootMainEntries[BootOptionSelected - BootOptionCount - 1].Callback (&BootOptionsList);
    } else {
      // Find the selected entry from the Boot#### list
      Index = 1;
      for (Entry = GetFirstNode (&BootOptionsList);
           !IsNull (&BootOptionsList,Entry);
           Entry = GetNextNode (&BootOptionsList,Entry)
           )
      {
        if (Index == BootOptionSelected) {
          BootOption = LOAD_OPTION_FROM_LINK(Entry);
          break;
        }
        Index++;
      }

      Status = BootOptionStart (BootOption);
    }
  }
  // Should never go here
}
