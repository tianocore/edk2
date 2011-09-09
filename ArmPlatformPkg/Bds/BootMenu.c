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

      Status = gBS->LocateProtocol(&gEfiDevicePathToTextProtocolGuid, NULL, (VOID **)&DevicePathToTextProtocol);
      ASSERT_EFI_ERROR(Status);
      DevicePathTxt = DevicePathToTextProtocol->ConvertDevicePathToText((*SupportedBootDevice)->DevicePathProtocol,TRUE,TRUE);

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
      Print(L"Invalid input (max %d)\n",SupportedDeviceSelected);
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
  EFI_STATUS               Status;
  BDS_SUPPORTED_DEVICE*    SupportedBootDevice;
  BDS_LOADER_ARGUMENTS     BootArguments;
  CHAR16                    BootDescription[BOOT_DEVICE_DESCRIPTION_MAX];
  UINT32                   Attributes;
  BDS_LOADER_TYPE          BootType;
  BDS_LOAD_OPTION          *BdsLoadOption;
  EFI_DEVICE_PATH          *DevicePath;
  EFI_DEVICE_PATH_PROTOCOL *DevicePathNode;
  EFI_DEVICE_PATH_PROTOCOL *InitrdPathNode;

  Attributes                = 0;
  SupportedBootDevice = NULL;

  // List the Boot Devices supported
  Status = SelectBootDevice(&SupportedBootDevice);
  if (EFI_ERROR(Status)) {
    Status = EFI_ABORTED;
    goto EXIT;
  }

  // Create the specific device path node
  Print(L"File path of the EFI Application or the kernel: ");
  Status = SupportedBootDevice->Support->CreateDevicePathNode (SupportedBootDevice, &DevicePathNode, &BootType, &Attributes);
  if (EFI_ERROR(Status)) {
    Status = EFI_ABORTED;
    goto EXIT;
  }
  // Append the Device Path node to the select device path
  DevicePath = AppendDevicePathNode (SupportedBootDevice->DevicePathProtocol, (CONST EFI_DEVICE_PATH_PROTOCOL *)DevicePathNode);

  if (BootType == BDS_LOADER_KERNEL_LINUX_ATAG) {
    // Create the specific device path node
    Print(L"File path of the initrd: ");
    Status = SupportedBootDevice->Support->CreateDevicePathNode (SupportedBootDevice, &InitrdPathNode, NULL, NULL);
    if (EFI_ERROR(Status) && Status != EFI_NOT_FOUND) { // EFI_NOT_FOUND is returned on empty input string, but we can boot without an initrd
      Status = EFI_ABORTED;
      goto EXIT;
    }

    if (InitrdPathNode != NULL) {
      // Append the Device Path node to the select device path
      BootArguments.LinuxAtagArguments.InitrdPathList = AppendDevicePathNode (SupportedBootDevice->DevicePathProtocol, (CONST EFI_DEVICE_PATH_PROTOCOL *)InitrdPathNode);
    } else {
      BootArguments.LinuxAtagArguments.InitrdPathList = NULL;
    }

    Print(L"Arguments to pass to the binary: ");
    Status = GetHIInputAscii (BootArguments.LinuxAtagArguments.CmdLine,BOOT_DEVICE_OPTION_MAX);
    if (EFI_ERROR(Status)) {
      Status = EFI_ABORTED;
      goto FREE_DEVICE_PATH;
    }
    BootArguments.LinuxAtagArguments.CmdLine[BOOT_DEVICE_OPTION_MAX]= '\0';
  }

  Print(L"Description for this new Entry: ");
  Status = GetHIInputStr (BootDescription, BOOT_DEVICE_DESCRIPTION_MAX);
  if (EFI_ERROR(Status)) {
    Status = EFI_ABORTED;
    goto FREE_DEVICE_PATH;
  }

  // Create new entry
  Status = BootOptionCreate (Attributes, BootDescription, DevicePath, BootType, &BootArguments, &BdsLoadOption);
  if (!EFI_ERROR(Status)) {
    InsertTailList (BootOptionsList,&BdsLoadOption->Link);
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
  IN  LIST_ENTRY *BootOptionsList,
  IN  CONST CHAR16* InputStatement,
  OUT BDS_LOAD_OPTION **BdsLoadOption
  )
{
  EFI_STATUS    Status;
  LIST_ENTRY*   Entry;
  BDS_LOAD_OPTION *BootOption;
  UINTN         BootOptionSelected;
  UINTN         BootOptionCount;
  UINTN         Index;

  // Display the list of supported boot devices
  BootOptionCount = 1;
  for (Entry = GetFirstNode (BootOptionsList);
       !IsNull (BootOptionsList,Entry);
       Entry = GetNextNode (BootOptionsList,Entry)
       )
  {
    BootOption = LOAD_OPTION_FROM_LINK(Entry);
    Print(L"[%d] %s\n",BootOptionCount,BootOption->Description);

    DEBUG_CODE_BEGIN();
      CHAR16*                           DevicePathTxt;
      EFI_DEVICE_PATH_TO_TEXT_PROTOCOL* DevicePathToTextProtocol;

      Status = gBS->LocateProtocol(&gEfiDevicePathToTextProtocolGuid, NULL, (VOID **)&DevicePathToTextProtocol);
      ASSERT_EFI_ERROR(Status);
      DevicePathTxt = DevicePathToTextProtocol->ConvertDevicePathToText(BootOption->FilePathList,TRUE,TRUE);

      Print(L"\t- %s\n",DevicePathTxt);
      if ((BDS_LOADER_TYPE)ReadUnaligned32(&BootOption->OptionalData->LoaderType) == BDS_LOADER_KERNEL_LINUX_ATAG) {
        Print(L"\t- Arguments: %a\n",BootOption->OptionalData->Arguments.LinuxAtagArguments.CmdLine);
      }

      FreePool(DevicePathTxt);
    DEBUG_CODE_END();

    BootOptionCount++;
  }

  // Get the index of the boot device to delete
  BootOptionSelected = 0;
  while (BootOptionSelected == 0) {
    Print(InputStatement);
    Status = GetHIInputInteger (&BootOptionSelected);
    if (EFI_ERROR(Status)) {
      return Status;
    } else if ((BootOptionSelected == 0) || (BootOptionSelected >= BootOptionCount)) {
      Print(L"Invalid input (max %d)\n",BootOptionCount-1);
      BootOptionSelected = 0;
    }
  }

  // Get the structure of the Boot device to delete
  Index = 1;
  for (Entry = GetFirstNode (BootOptionsList);
       !IsNull (BootOptionsList,Entry);
       Entry = GetNextNode (BootOptionsList,Entry)
       )
  {
    if (Index == BootOptionSelected) {
      *BdsLoadOption = LOAD_OPTION_FROM_LINK(Entry);
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
  EFI_STATUS    Status;
  BDS_LOAD_OPTION *BootOption;

  Status = BootMenuSelectBootOption (BootOptionsList,L"Delete entry: ",&BootOption);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  // Delete the BDS Load option structures
  BootOptionDelete (BootOption);

  return EFI_SUCCESS;
}

EFI_STATUS
BootMenuUpdateBootOption (
  IN LIST_ENTRY *BootOptionsList
  )
{
  EFI_STATUS                Status;
  BDS_LOAD_OPTION           *BootOption;
  BDS_LOAD_OPTION_SUPPORT   *DeviceSupport;
  BDS_LOADER_ARGUMENTS      BootArguments;
  CHAR16                        BootDescription[BOOT_DEVICE_DESCRIPTION_MAX];
  EFI_DEVICE_PATH*          DevicePath;
  BDS_LOADER_TYPE           BootType;

  Status = BootMenuSelectBootOption (BootOptionsList,L"Update entry: ",&BootOption);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  // Get the device support for this Boot Option
  Status = BootDeviceGetDeviceSupport (BootOption,&DeviceSupport);
  if (EFI_ERROR(Status)) {
    Print(L"Impossible to retrieve the supported device for the update\n");
    return EFI_UNSUPPORTED;
  }

  Print(L"File path of the EFI Application or the kernel: ");
  Status = DeviceSupport->UpdateDevicePathNode (BootOption->FilePathList, &DevicePath, NULL, NULL);
  if (EFI_ERROR(Status)) {
    Status = EFI_ABORTED;
    goto EXIT;
  }

  BootType = (BDS_LOADER_TYPE)ReadUnaligned32((UINT32 *)(&BootOption->OptionalData->LoaderType));

  // TODO: Allow adding an initrd to a boot entry without one
  if (BootType == BDS_LOADER_KERNEL_LINUX_ATAG) {
    if (ReadUnaligned16(&BootOption->OptionalData->Arguments.LinuxAtagArguments.InitrdPathListLength) > 0
        && (EFI_DEVICE_PATH_PROTOCOL *)ReadUnaligned32((UINT32 *)(&BootOption->OptionalData->Arguments.LinuxAtagArguments.InitrdPathList)) != NULL) {

      Print(L"File path of the initrd: ");
      Status = DeviceSupport->UpdateDevicePathNode (
                (EFI_DEVICE_PATH_PROTOCOL *)ReadUnaligned32((UINT32 *)(&BootOption->OptionalData->Arguments.LinuxAtagArguments.InitrdPathList)),
                &BootArguments.LinuxAtagArguments.InitrdPathList,
                NULL,
                NULL);
      if (EFI_ERROR(Status) && Status != EFI_NOT_FOUND) {// EFI_NOT_FOUND is returned on empty input string, but we can boot without an initrd
        Status = EFI_ABORTED;
        goto EXIT;
      }
    } else {
      BootArguments.LinuxAtagArguments.InitrdPathList = NULL;
      BootArguments.LinuxAtagArguments.InitrdPathListLength = 0;
    }

    Print(L"Arguments to pass to the binary: ");
    if (ReadUnaligned32((CONST UINT32*)&BootOption->OptionalData->Arguments.LinuxAtagArguments.CmdLine)) {
      AsciiStrnCpy(BootArguments.LinuxAtagArguments.CmdLine,
                   BootOption->OptionalData->Arguments.LinuxAtagArguments.CmdLine,
                   BOOT_DEVICE_OPTION_MAX+1);
    } else {
      BootArguments.LinuxAtagArguments.CmdLine[0] = '\0';
    }
    Status = EditHIInputAscii (BootArguments.LinuxAtagArguments.CmdLine, BOOT_DEVICE_OPTION_MAX);
    if (EFI_ERROR(Status)) {
      Status = EFI_ABORTED;
      goto FREE_DEVICE_PATH;
    }
  }

  Print(L"Description for this new Entry: ");
  Status = EditHIInputStr (BootDescription, BOOT_DEVICE_DESCRIPTION_MAX);
  if (EFI_ERROR(Status)) {
    Status = EFI_ABORTED;
    goto FREE_DEVICE_PATH;
  }

  // Update the entry
  Status = BootOptionUpdate (BootOption, BootOption->Attributes, BootDescription, DevicePath, BootType, &BootArguments);

FREE_DEVICE_PATH:
  FreePool (DevicePath);

EXIT:
  if (Status == EFI_ABORTED) {
    Print(L"\n");
  }
  return Status;
}

struct BOOT_MANAGER_ENTRY {
  CONST CHAR16* Description;
  EFI_STATUS (*Callback) (IN LIST_ENTRY *BootOptionsList);
} BootManagerEntries[] = {
    { L"Add Boot Device Entry", BootMenuAddBootOption },
    { L"Update Boot Device Entry", BootMenuUpdateBootOption },
    { L"Remove Boot Device Entry", BootMenuRemoveBootOption },
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
      Status = BootManagerEntries[OptionSelected-1].Callback (BootOptionsList);
    }
  }

  return EFI_SUCCESS;
}

EFI_STATUS
BootEBL (
  IN LIST_ENTRY *BootOptionsList
  )
{
  EFI_STATUS Status;

  // Start EFI Shell
  Status = BdsLoadApplication(mImageHandle, L"Ebl");
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
    { L"EBL", BootEBL },
    { L"Boot Manager", BootMenuManager },
};


EFI_STATUS
BootMenuMain (
  VOID
  )
{
  LIST_ENTRY BootOptionsList;
  UINTN       OptionCount;
  UINTN       BootOptionCount;
  EFI_STATUS  Status;
  LIST_ENTRY  *Entry;
  BDS_LOAD_OPTION *BootOption;
  UINTN   BootOptionSelected;
  UINTN   Index;
  UINTN   BootMainEntryCount;

  BootOption              = NULL;
  BootMainEntryCount = sizeof(BootMainEntries) / sizeof(struct BOOT_MAIN_ENTRY);

  // Get Boot#### list
  BootOptionList (&BootOptionsList);

  while (TRUE) {
    OptionCount = 1;

    // Display the Boot options
    for (Entry = GetFirstNode (&BootOptionsList);
         !IsNull (&BootOptionsList,Entry);
         Entry = GetNextNode (&BootOptionsList,Entry)
         )
    {
      BootOption = LOAD_OPTION_FROM_LINK(Entry);

      Print(L"[%d] %s\n",OptionCount,BootOption->Description);

      DEBUG_CODE_BEGIN();
        CHAR16*                           DevicePathTxt;
        EFI_DEVICE_PATH_TO_TEXT_PROTOCOL* DevicePathToTextProtocol;

        Status = gBS->LocateProtocol(&gEfiDevicePathToTextProtocolGuid, NULL, (VOID **)&DevicePathToTextProtocol);
        if (EFI_ERROR(Status)) {
          // You must provide an implementation of DevicePathToTextProtocol in your firmware (eg: DevicePathDxe)
          DEBUG((EFI_D_ERROR,"Error: Bds requires DevicePathToTextProtocol\n"));
          return Status;
        }
        DevicePathTxt = DevicePathToTextProtocol->ConvertDevicePathToText(BootOption->FilePathList,TRUE,TRUE);

        Print(L"\t- %s\n",DevicePathTxt);
        if (ReadUnaligned32(&BootOption->OptionalData->LoaderType) == BDS_LOADER_KERNEL_LINUX_ATAG) {
          if (ReadUnaligned16(&BootOption->OptionalData->Arguments.LinuxAtagArguments.InitrdPathListLength) > 0
              && (EFI_DEVICE_PATH_PROTOCOL *)ReadUnaligned32((UINT32 *)(&BootOption->OptionalData->Arguments.LinuxAtagArguments.InitrdPathList)) != NULL) {
            DevicePathTxt = DevicePathToTextProtocol->ConvertDevicePathToText((EFI_DEVICE_PATH_PROTOCOL *)ReadUnaligned32((UINT32 *)(&BootOption->OptionalData->Arguments.LinuxAtagArguments.InitrdPathList)),TRUE,TRUE);
            Print(L"\t- Initrd: %s\n", DevicePathTxt);
          }
        
          Print(L"\t- Arguments: %a\n", BootOption->OptionalData->Arguments.LinuxAtagArguments.CmdLine);
        }

        Print(L"\t- LoaderType: %d\n", ReadUnaligned32 (&BootOption->OptionalData->LoaderType));

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

  return Status;
}
