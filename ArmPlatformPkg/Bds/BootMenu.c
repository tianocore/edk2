/** @file
*
*  Copyright (c) 2011 - 2015, ARM Limited. All rights reserved.
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

#include <libfdt.h>

/**
  Worker function that displays the list of boot options that is passed in.

  The function loops over the entries of the list of boot options that is passed
  in. For each entry, the boot option description is displayed on a single line
  along with the position of the option in the list. In debug mode, the UEFI
  device path and the arguments of the boot option are displayed as well in
  subsequent lines.

  @param[in]  BootOptionsList  List of the boot options

**/
STATIC
VOID
DisplayBootOptions (
  IN  LIST_ENTRY*   BootOptionsList
  )
{
  EFI_STATUS        Status;
  UINTN             BootOptionCount;
  LIST_ENTRY       *Entry;
  BDS_LOAD_OPTION  *BdsLoadOption;
  BOOLEAN           IsUnicode;

  BootOptionCount = 0 ;
  for (Entry = GetFirstNode (BootOptionsList);
       !IsNull (BootOptionsList, Entry);
       Entry = GetNextNode (BootOptionsList, Entry)
      ) {

    BdsLoadOption = LOAD_OPTION_FROM_LINK (Entry);
    Print (L"[%d] %s\n", ++BootOptionCount, BdsLoadOption->Description);

    DEBUG_CODE_BEGIN ();
      CHAR16*                           DevicePathTxt;
      EFI_DEVICE_PATH_TO_TEXT_PROTOCOL* DevicePathToTextProtocol;

      Status = gBS->LocateProtocol (
                     &gEfiDevicePathToTextProtocolGuid,
                     NULL,
                     (VOID **)&DevicePathToTextProtocol
                     );
      ASSERT_EFI_ERROR (Status);
      DevicePathTxt = DevicePathToTextProtocol->ConvertDevicePathToText (
                                                  BdsLoadOption->FilePathList,
                                                  TRUE,
                                                  TRUE
                                                  );
      Print (L"\t- %s\n", DevicePathTxt);

      if (IsPrintableString (BdsLoadOption->OptionalData, &IsUnicode)) {
        if (IsUnicode) {
          Print (L"\t- Arguments: %s\n", BdsLoadOption->OptionalData);
        } else {
          AsciiPrint ("\t- Arguments: %a\n", BdsLoadOption->OptionalData);
        }
      }

      FreePool (DevicePathTxt);
    DEBUG_CODE_END ();
  }
}

/**
  Worker function that asks for a boot option to be selected and returns a
  pointer to the structure describing the selected boot option.

  @param[in]  BootOptionsList  List of the boot options

  @retval     EFI_SUCCESS      Selection succeeded
  @retval     !EFI_SUCCESS     Input error or input cancelled

**/
STATIC
EFI_STATUS
SelectBootOption (
  IN  LIST_ENTRY*               BootOptionsList,
  IN  CONST CHAR16*             InputStatement,
  OUT BDS_LOAD_OPTION_ENTRY**   BdsLoadOptionEntry
  )
{
  EFI_STATUS                    Status;
  UINTN                         BootOptionCount;
  UINT16                       *BootOrder;
  LIST_ENTRY*                   Entry;
  UINTN                         BootOptionSelected;
  UINTN                         Index;

  // Get the number of boot options
  Status = GetGlobalEnvironmentVariable (
            L"BootOrder", NULL, &BootOptionCount, (VOID**)&BootOrder
            );
  if (EFI_ERROR (Status)) {
    goto ErrorExit;
  }
  FreePool (BootOrder);
  BootOptionCount /= sizeof (UINT16);

  // Check if a valid boot option(s) is found
  if (BootOptionCount == 0) {
    if (StrCmp (InputStatement, DELETE_BOOT_ENTRY) == 0) {
      Print (L"Nothing to remove!\n");
    } else if (StrCmp (InputStatement, UPDATE_BOOT_ENTRY) == 0) {
      Print (L"Nothing to update!\n");
    } else if (StrCmp (InputStatement, MOVE_BOOT_ENTRY) == 0) {
      Print (L"Nothing to move!\n");
    } else {
      Print (L"No supported Boot Entry.\n");
    }
    return EFI_NOT_FOUND;
  }

  // Get the index of the boot device to delete
  BootOptionSelected = 0;
  while (BootOptionSelected == 0) {
    Print (InputStatement);
    Status = GetHIInputInteger (&BootOptionSelected);
    if (EFI_ERROR (Status)) {
      Print (L"\n");
      goto ErrorExit;
    } else if ((BootOptionSelected == 0) || (BootOptionSelected > BootOptionCount)) {
      Print (L"Invalid input (max %d)\n", BootOptionCount);
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
      *BdsLoadOptionEntry = LOAD_OPTION_ENTRY_FROM_LINK (Entry);
      break;
    }
    Index++;
  }

ErrorExit:
  return Status;
}

STATIC
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
  CHAR16                    BootDescription[BOOT_DEVICE_DESCRIPTION_MAX];
  CHAR16                    CmdLine[BOOT_DEVICE_OPTION_MAX];
  UINT32                    Attributes;
  BDS_LOAD_OPTION_ENTRY     *BdsLoadOptionEntry;
  EFI_DEVICE_PATH           *DevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePathNodes;
  UINT8*                    OptionalData;
  UINTN                     OptionalDataSize;
  BOOLEAN                   EfiBinary;
  CHAR16                    *LinuxDevicePath;

  Attributes                = 0;
  SupportedBootDevice = NULL;

  // List the Boot Devices supported
  Status = SelectBootDevice (&SupportedBootDevice);
  if (EFI_ERROR(Status)) {
    Status = EFI_ABORTED;
    goto EXIT;
  }

  // Create the specific device path node
  if (FeaturePcdGet (PcdBdsLinuxSupport) && mLinuxLoaderDevicePath) {
    Status = SupportedBootDevice->Support->CreateDevicePathNode (L"EFI Application or the kernel", &DevicePathNodes);
  } else {
    Status = SupportedBootDevice->Support->CreateDevicePathNode (L"EFI Application", &DevicePathNodes);
  }
  if (EFI_ERROR (Status)) {
    Status = EFI_ABORTED;
    goto EXIT;
  }
  // Append the Device Path to the selected device path
  DevicePath = AppendDevicePath (SupportedBootDevice->DevicePathProtocol, (CONST EFI_DEVICE_PATH_PROTOCOL *)DevicePathNodes);
  if (DevicePath == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto EXIT;
  }

  // Is it an EFI application?
  if (FeaturePcdGet (PcdBdsLinuxSupport) && mLinuxLoaderDevicePath) {
    Status = IsEfiBinary (DevicePath, &EfiBinary);
    if (EFI_ERROR (Status)) {
      Status = EFI_ABORTED;
      goto EXIT;
    }

    if (EfiBinary == FALSE) {
      Print (L"It is assumed the binary is a Linux kernel and the embedded Linux Loader is going to be used.\n");
      Print (L"Supported command line formats by the embedded Linux Loader:\n");
      Print (L"- <EFI device path of the Linux kernel> -c \"<Linux kernel command line>\"\n");
      Print (L"- <EFI device path of the Linux kernel> -c \"<Linux kernel command line>\" -f <EFI Device Path of the Linux initrd>\n");
      Print (L"- <EFI device path of the Linux kernel> -c \"<Linux kernel command line>\" -a <Machine Type for ATAG Linux kernel>\n");

      // Copy the Linux path into the command line
      LinuxDevicePath = ConvertDevicePathToText (DevicePath, FALSE, FALSE);
      CopyMem (CmdLine, LinuxDevicePath, MAX (sizeof (CmdLine), StrSize (LinuxDevicePath)));
      FreePool (LinuxDevicePath);

      // Free the generated Device Path
      FreePool (DevicePath);
      // and use the embedded Linux Loader as the EFI application
      DevicePath = mLinuxLoaderDevicePath;
    } else {
      CmdLine[0] = L'\0';
    }
  } else {
    CmdLine[0] = L'\0';
  }

  Print (L"Arguments to pass to the EFI Application: ");
  Status = EditHIInputStr (CmdLine, BOOT_DEVICE_OPTION_MAX);
  if (EFI_ERROR (Status)) {
    Status = EFI_ABORTED;
    goto EXIT;
  }

  OptionalData = (UINT8*)CmdLine;
  OptionalDataSize = StrSize (CmdLine);

  Print(L"Description for this new Entry: ");
  Status = GetHIInputStr (BootDescription, BOOT_DEVICE_DESCRIPTION_MAX);
  if (EFI_ERROR(Status)) {
    Status = EFI_ABORTED;
    goto FREE_DEVICE_PATH;
  }

  // Create new entry
  BdsLoadOptionEntry = (BDS_LOAD_OPTION_ENTRY*)AllocatePool (sizeof(BDS_LOAD_OPTION_ENTRY));
  Status = BootOptionCreate (Attributes, BootDescription, DevicePath, OptionalData, OptionalDataSize, &BdsLoadOptionEntry->BdsLoadOption);
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

EFI_STATUS
BootMenuRemoveBootOption (
  IN LIST_ENTRY *BootOptionsList
  )
{
  EFI_STATUS                    Status;
  BDS_LOAD_OPTION_ENTRY*        BootOptionEntry;

  DisplayBootOptions (BootOptionsList);
  Status = SelectBootOption (BootOptionsList, DELETE_BOOT_ENTRY, &BootOptionEntry);
  if (EFI_ERROR (Status)) {
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
  CHAR16                        BootDescription[BOOT_DEVICE_DESCRIPTION_MAX];
  CHAR8                         CmdLine[BOOT_DEVICE_OPTION_MAX];
  CHAR16                        UnicodeCmdLine[BOOT_DEVICE_OPTION_MAX];
  CHAR16                        *LinuxDevicePath;
  EFI_DEVICE_PATH               *DevicePath;
  UINT8*                        OptionalData;
  UINTN                         OptionalDataSize;
  BOOLEAN                       IsPrintable;
  BOOLEAN                       IsUnicode;
  BOOLEAN                       EfiBinary;

  DisplayBootOptions (BootOptionsList);
  Status = SelectBootOption (BootOptionsList, UPDATE_BOOT_ENTRY, &BootOptionEntry);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  BootOption = BootOptionEntry->BdsLoadOption;

  // Get the device support for this Boot Option
  Status = BootDeviceGetDeviceSupport (BootOption->FilePathList, &DeviceSupport);
  if (EFI_ERROR(Status)) {
    Print(L"Not possible to retrieve the supported device for the update\n");
    return EFI_UNSUPPORTED;
  }

  EfiBinary = TRUE;
  if (FeaturePcdGet (PcdBdsLinuxSupport) && mLinuxLoaderDevicePath) {
    Status = DeviceSupport->UpdateDevicePathNode (BootOption->FilePathList, L"EFI Application or the kernel", &DevicePath);
    if (EFI_ERROR (Status)) {
      Status = EFI_ABORTED;
      goto EXIT;
    }

    // Is it an EFI application?
    Status = IsEfiBinary (DevicePath, &EfiBinary);
    if (EFI_ERROR (Status)) {
      Status = EFI_ABORTED;
      goto EXIT;
    }

    if (EfiBinary == FALSE) {
      Print (L"It is assumed the binary is a Linux kernel and the embedded Linux Loader is going to be used.\n");
      Print (L"Supported command line formats by the embedded Linux Loader:\n");
      Print (L"- <EFI device path of the Linux kernel> -c \"<Linux kernel command line>\"\n");
      Print (L"- <EFI device path of the Linux kernel> -c \"<Linux kernel command line>\" -f <EFI Device Path of the Linux initrd>\n");
      Print (L"- <EFI device path of the Linux kernel> -c \"<Linux kernel command line>\" -a <Machine Type for ATAG Linux kernel>\n");

      // Copy the Linux path into the command line
      LinuxDevicePath = ConvertDevicePathToText (DevicePath, FALSE, FALSE);
      CopyMem (UnicodeCmdLine, LinuxDevicePath, MAX (sizeof (UnicodeCmdLine), StrSize (LinuxDevicePath)));
      FreePool (LinuxDevicePath);

      // Free the generated Device Path
      FreePool (DevicePath);
      // and use the embedded Linux Loader as the EFI application
      DevicePath = mLinuxLoaderDevicePath;

      // The command line is a unicode printable string
      IsPrintable = TRUE;
      IsUnicode = TRUE;
    }
  } else {
    Status = DeviceSupport->UpdateDevicePathNode (BootOption->FilePathList, L"EFI Application", &DevicePath);
    if (EFI_ERROR (Status)) {
      Status = EFI_ABORTED;
      goto EXIT;
    }
  }

  Print (L"Arguments to pass to the EFI Application: ");

  // When the command line has not been initialized by the embedded Linux loader earlier
  if (EfiBinary) {
    if (BootOption->OptionalDataSize > 0) {
      IsPrintable = IsPrintableString (BootOption->OptionalData, &IsUnicode);
      if (IsPrintable) {
          //
          // The size in bytes of the string, final zero included, should
          // be equal to or at least lower than "BootOption->OptionalDataSize"
          // and the "IsPrintableString()" has already tested that the length
          // in number of characters is smaller than BOOT_DEVICE_OPTION_MAX,
          // final '\0' included. We can thus copy the string for editing
          // using "CopyMem()". Furthermore, note that in the case of an Unicode
          // string "StrnCpy()" and "StrCpy()" can not be used to copy the
          // string because the data pointed to by "BootOption->OptionalData"
          // is not necessarily 2-byte aligned.
          //
        if (IsUnicode) {
          CopyMem (
            UnicodeCmdLine, BootOption->OptionalData,
            MIN (sizeof (UnicodeCmdLine),
                 BootOption->OptionalDataSize)
            );
        } else {
          CopyMem (
            CmdLine, BootOption->OptionalData,
            MIN (sizeof (CmdLine),
                 BootOption->OptionalDataSize)
            );
        }
      }
    } else {
      UnicodeCmdLine[0] = L'\0';
      IsPrintable = TRUE;
      IsUnicode = TRUE;
    }
  }

  // We do not request arguments for OptionalData that cannot be printed
  if (IsPrintable) {
    if (IsUnicode) {
      Status = EditHIInputStr (UnicodeCmdLine, BOOT_DEVICE_OPTION_MAX);
      if (EFI_ERROR (Status)) {
        Status = EFI_ABORTED;
        goto FREE_DEVICE_PATH;
      }

      OptionalData = (UINT8*)UnicodeCmdLine;
      OptionalDataSize = StrSize (UnicodeCmdLine);
    } else {
      Status = EditHIInputAscii (CmdLine, BOOT_DEVICE_OPTION_MAX);
      if (EFI_ERROR (Status)) {
        Status = EFI_ABORTED;
        goto FREE_DEVICE_PATH;
      }

      OptionalData = (UINT8*)CmdLine;
      OptionalDataSize = AsciiStrSize (CmdLine);
    }
  } else {
    // We keep the former OptionalData
    OptionalData = BootOption->OptionalData;
    OptionalDataSize = BootOption->OptionalDataSize;
  }

  Print(L"Description for this new Entry: ");
  StrnCpy (BootDescription, BootOption->Description, BOOT_DEVICE_DESCRIPTION_MAX);
  Status = EditHIInputStr (BootDescription, BOOT_DEVICE_DESCRIPTION_MAX);
  if (EFI_ERROR(Status)) {
    Status = EFI_ABORTED;
    goto FREE_DEVICE_PATH;
  }

  // Update the entry
  Status = BootOptionUpdate (BootOption, BootOption->Attributes, BootDescription, DevicePath, OptionalData, OptionalDataSize);

FREE_DEVICE_PATH:
  FreePool (DevicePath);

EXIT:
  if (Status == EFI_ABORTED) {
    Print(L"\n");
  }
  return Status;
}

/**
  Reorder boot options

  Ask for the boot option to move and then move it when up or down arrows
  are pressed. This function is called when the user selects the "Reorder Boot
  Device Entries" entry in the boot manager menu.
  The order of the boot options in BootOptionList and in the UEFI BootOrder
  global variable are kept coherent until the user confirm his reordering (ie:
  he does not exit by pressing escape).

  @param[in]  BootOptionsList  List of the boot devices constructed in
                               BootMenuMain()

  @retval  EFI_SUCCESS   No error encountered.
  @retval  !EFI_SUCCESS  An error has occured either in the selection of the
                         boot option to move or while interacting with the user.

**/
STATIC
EFI_STATUS
BootMenuReorderBootOptions (
  IN LIST_ENTRY *BootOptionsList
  )
{
  EFI_STATUS              Status;
  BDS_LOAD_OPTION_ENTRY  *BootOptionEntry;
  LIST_ENTRY             *SelectedEntry;
  LIST_ENTRY             *PrevEntry;
  BOOLEAN                 Move;
  BOOLEAN                 Save;
  BOOLEAN                 Cancel;
  UINTN                   WaitIndex;
  EFI_INPUT_KEY           Key;
  LIST_ENTRY             *SecondEntry;
  UINTN                   BootOrderSize;
  UINT16                 *BootOrder;
  LIST_ENTRY             *Entry;
  UINTN                   Index;

  DisplayBootOptions (BootOptionsList);

  // Ask to select the boot option to move
  while (TRUE) {
    Status = SelectBootOption (BootOptionsList, MOVE_BOOT_ENTRY, &BootOptionEntry);
    if (EFI_ERROR (Status)) {
      goto ErrorExit;
    }

    SelectedEntry = &BootOptionEntry->Link;
    SecondEntry = NULL;
    // Note down the previous entry in the list to be able to cancel changes
    PrevEntry = GetPreviousNode (BootOptionsList, SelectedEntry);

    //  Start of interaction
    while (TRUE) {
      Print (
        L"* Use up/down arrows to move the entry '%s'",
        BootOptionEntry->BdsLoadOption->Description
        );

      // Wait for a move, save or cancel request
      Move   = FALSE;
      Save   = FALSE;
      Cancel = FALSE;
      do {
        Status = gBS->WaitForEvent (1, &gST->ConIn->WaitForKey, &WaitIndex);
        if (!EFI_ERROR (Status)) {
          Status = gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);
        }
        if (EFI_ERROR (Status)) {
          Print (L"\n");
          goto ErrorExit;
        }

        switch (Key.ScanCode) {
        case SCAN_NULL:
          Save = (Key.UnicodeChar == CHAR_LINEFEED)        ||
                 (Key.UnicodeChar == CHAR_CARRIAGE_RETURN) ||
                 (Key.UnicodeChar == 0x7f);
          break;

        case SCAN_UP:
          SecondEntry = GetPreviousNode (BootOptionsList, SelectedEntry);
          Move = SecondEntry != BootOptionsList;
          break;

        case SCAN_DOWN:
          SecondEntry = GetNextNode (BootOptionsList, SelectedEntry);
          Move = SecondEntry != BootOptionsList;
          break;

        case SCAN_ESC:
          Cancel = TRUE;
          break;
        }
      } while ((!Move) && (!Save) && (!Cancel));

      if (Move) {
        if ((SelectedEntry != NULL) && (SecondEntry != NULL)) {
          SwapListEntries (SelectedEntry, SecondEntry);
        }
      } else {
        if (Save) {
          Status = GetGlobalEnvironmentVariable (
                    L"BootOrder", NULL, &BootOrderSize, (VOID**)&BootOrder
                    );
          BootOrderSize /= sizeof (UINT16);

          if (!EFI_ERROR (Status)) {
            // The order of the boot options in the 'BootOptionsList' is the
            // new order that has been just defined by the user. Save this new
            // order in "BootOrder" UEFI global variable.
            Entry = GetFirstNode (BootOptionsList);
            for (Index = 0; Index < BootOrderSize; Index++) {
              BootOrder[Index] = (LOAD_OPTION_FROM_LINK (Entry))->LoadOptionIndex;
              Entry = GetNextNode (BootOptionsList, Entry);
            }
            Status = gRT->SetVariable (
                           (CHAR16*)L"BootOrder",
                           &gEfiGlobalVariableGuid,
                           EFI_VARIABLE_NON_VOLATILE       |
                           EFI_VARIABLE_BOOTSERVICE_ACCESS |
                           EFI_VARIABLE_RUNTIME_ACCESS,
                           BootOrderSize * sizeof (UINT16),
                           BootOrder
                           );
            FreePool (BootOrder);
          }

          if (EFI_ERROR (Status)) {
            Print (L"\nAn error occurred, move not completed!\n");
            Cancel = TRUE;
          }
        }

        if (Cancel) {
          //
          // Restore initial position of the selected boot option
          //
          RemoveEntryList (SelectedEntry);
          InsertHeadList (PrevEntry, SelectedEntry);
        }
      }

      Print (L"\n");
      DisplayBootOptions (BootOptionsList);
      // Saved or cancelled, back to the choice of boot option to move
      if (!Move) {
        break;
      }
    }
  }

ErrorExit:
  return Status ;
}

EFI_STATUS
UpdateFdtPath (
  IN LIST_ENTRY *BootOptionsList
  )
{
  EFI_STATUS                Status;
  BDS_SUPPORTED_DEVICE      *SupportedBootDevice;
  EFI_DEVICE_PATH_PROTOCOL  *FdtDevicePathNodes;
  EFI_DEVICE_PATH_PROTOCOL  *FdtDevicePath;
  CHAR16                    *FdtTextDevicePath;
  EFI_PHYSICAL_ADDRESS      FdtBlobBase;
  UINTN                     FdtBlobSize;
  UINTN                     NumPages;
  EFI_PHYSICAL_ADDRESS      FdtConfigurationTableBase;

  SupportedBootDevice = NULL;

  Status = SelectBootDevice (&SupportedBootDevice);
  if (EFI_ERROR (Status)) {
    Status = EFI_ABORTED;
    goto EXIT;
  }

  // Create the specific device path node
  Status = SupportedBootDevice->Support->CreateDevicePathNode (L"FDT blob", &FdtDevicePathNodes);
  if (EFI_ERROR (Status)) {
    Status = EFI_ABORTED;
    goto EXIT;
  }

  if (FdtDevicePathNodes != NULL) {
    Status = EFI_OUT_OF_RESOURCES;

    FdtDevicePath = AppendDevicePath (SupportedBootDevice->DevicePathProtocol, FdtDevicePathNodes);
    FreePool (FdtDevicePathNodes);
    if (FdtDevicePath == NULL) {
      goto EXIT;
    }

    FdtTextDevicePath = ConvertDevicePathToText (FdtDevicePath, TRUE, TRUE);
    if (FdtTextDevicePath == NULL) {
      goto EXIT;
    }

    Status = gRT->SetVariable (
                    (CHAR16*)L"Fdt",
                    &gFdtVariableGuid,
                    EFI_VARIABLE_RUNTIME_ACCESS |
                    EFI_VARIABLE_NON_VOLATILE   |
                    EFI_VARIABLE_BOOTSERVICE_ACCESS,
                    StrSize (FdtTextDevicePath),
                    FdtTextDevicePath
                    );
    ASSERT_EFI_ERROR (Status);
    FreePool (FdtTextDevicePath);
  } else {
    Status = gRT->SetVariable (
           (CHAR16*)L"Fdt",
           &gFdtVariableGuid,
           EFI_VARIABLE_RUNTIME_ACCESS |
           EFI_VARIABLE_NON_VOLATILE   |
           EFI_VARIABLE_BOOTSERVICE_ACCESS,
           0,
           NULL
           );
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  //
  // Try to load FDT from the new EFI Device Path
  //

  //
  // Load the FDT given its device path.
  // This operation may fail if the device path is not supported.
  //
  FdtBlobBase = 0;
  NumPages    = 0;
  Status = BdsLoadImage (FdtDevicePath, AllocateAnyPages, &FdtBlobBase, &FdtBlobSize);
  FreePool (FdtDevicePath);

  if (EFI_ERROR (Status)) {
    goto EXIT_LOAD_FDT;
  }

  // Check the FDT header is valid. We only make this check in DEBUG mode in
  // case the FDT header change on production device and this ASSERT() becomes
  // not valid.
  ASSERT (fdt_check_header ((VOID*)(UINTN)FdtBlobBase) == 0);

  //
  // Ensure the Size of the Device Tree is smaller than the size of the read file
  //
  ASSERT ((UINTN)fdt_totalsize ((VOID*)(UINTN)FdtBlobBase) <= FdtBlobSize);

  //
  // Store the FDT as Runtime Service Data to prevent the Kernel from
  // overwritting its data.
  //
  NumPages = EFI_SIZE_TO_PAGES (FdtBlobSize);
  Status = gBS->AllocatePages (
                  AllocateAnyPages, EfiRuntimeServicesData,
                  NumPages, &FdtConfigurationTableBase
                  );
  if (EFI_ERROR (Status)) {
    goto EXIT_LOAD_FDT;
  }
  gBS->CopyMem (
    (VOID*)(UINTN)FdtConfigurationTableBase,
    (VOID*)(UINTN)FdtBlobBase,
    FdtBlobSize
    );

  //
  // Install the FDT into the Configuration Table
  //
  Status = gBS->InstallConfigurationTable (
                  &gFdtTableGuid,
                  (VOID*)(UINTN)FdtConfigurationTableBase
                  );
  if (EFI_ERROR (Status)) {
    gBS->FreePages (FdtConfigurationTableBase, NumPages);
  }

EXIT_LOAD_FDT:
  if (EFI_ERROR (Status)) {
    Print (L"\nWarning: Did not manage to install the new device tree. Try to restart the platform.\n");
  }

  if (FdtBlobBase != 0) {
    gBS->FreePages (FdtBlobBase, NumPages);
  }

EXIT:
  if (Status == EFI_ABORTED) {
    Print (L"\n");
  }

  if (SupportedBootDevice != NULL) {
    FreePool (SupportedBootDevice);
  }

  return Status;
}

/**
  Set boot timeout

  Ask for the boot timeout in seconds and if the input succeeds assign the
  input value to the UEFI global variable "Timeout". This function is called
  when the user selects the "Set Boot Timeout" of the boot manager menu.

  @param[in]  BootOptionsList  List of the boot devices, not used here

  @retval  EFI_SUCCESS   Boot timeout in second retrieved from the standard
                         input and assigned to the UEFI "Timeout" global
                         variable
  @retval  !EFI_SUCCESS  Either the input or the setting of the UEFI global
                         variable "Timeout" has failed.
**/
EFI_STATUS
STATIC
BootMenuSetBootTimeout (
  IN LIST_ENTRY *BootOptionsList
  )
{
  EFI_STATUS  Status;
  UINTN       Input;
  UINT16      Timeout;

  Print (L"Timeout duration (in seconds): ");
  Status = GetHIInputInteger (&Input);
  if (EFI_ERROR (Status)) {
    Print (L"\n");
    goto ErrorExit;
  }

  Timeout = Input;
  Status = gRT->SetVariable (
                 (CHAR16*)L"Timeout",
                 &gEfiGlobalVariableGuid,
                 EFI_VARIABLE_NON_VOLATILE       |
                 EFI_VARIABLE_BOOTSERVICE_ACCESS |
                 EFI_VARIABLE_RUNTIME_ACCESS,
                 sizeof (UINT16),
                 &Timeout
                 );
  ASSERT_EFI_ERROR (Status);

ErrorExit:
  return Status;
}

struct BOOT_MANAGER_ENTRY {
  CONST CHAR16* Description;
  EFI_STATUS (*Callback) (IN LIST_ENTRY *BootOptionsList);
} BootManagerEntries[] = {
    { L"Add Boot Device Entry", BootMenuAddBootOption },
    { L"Update Boot Device Entry", BootMenuUpdateBootOption },
    { L"Remove Boot Device Entry", BootMenuRemoveBootOption },
    { L"Reorder Boot Device Entries", BootMenuReorderBootOptions },
    { L"Update FDT path", UpdateFdtPath },
    { L"Set Boot Timeout", BootMenuSetBootTimeout },
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
  EFI_STATUS       Status;
  EFI_DEVICE_PATH* EfiShellDevicePath;

  // Find the EFI Shell
  Status = LocateEfiApplicationInFvByName (L"Shell", &EfiShellDevicePath);
  if (Status == EFI_NOT_FOUND) {
    Print (L"Error: EFI Application not found.\n");
    return Status;
  } else if (EFI_ERROR (Status)) {
    Print (L"Error: Status Code: 0x%X\n", (UINT32)Status);
    return Status;
  } else {
    // Need to connect every drivers to ensure no dependencies are missing for the application
    Status = BdsConnectAllDrivers ();
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR, "FAIL to connect all drivers\n"));
      return Status;
    }

    return BdsStartEfiApplication (gImageHandle, EfiShellDevicePath, 0, NULL);
  }
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
  BOOLEAN                       IsUnicode;

  BootOption         = NULL;
  BootMainEntryCount = sizeof(BootMainEntries) / sizeof(struct BOOT_MAIN_ENTRY);

  if (FeaturePcdGet (PcdBdsLinuxSupport)) {
    // Check Linux Loader is present
    Status = LocateEfiApplicationInFvByGuid (&mLinuxLoaderAppGuid, &mLinuxLoaderDevicePath);
    ASSERT_EFI_ERROR (Status);
  }

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

        Status = gBS->LocateProtocol (&gEfiDevicePathToTextProtocolGuid, NULL, (VOID **)&DevicePathToTextProtocol);
        if (EFI_ERROR(Status)) {
          // You must provide an implementation of DevicePathToTextProtocol in your firmware (eg: DevicePathDxe)
          DEBUG((EFI_D_ERROR,"Error: Bds requires DevicePathToTextProtocol\n"));
          return Status;
        }
        DevicePathTxt = DevicePathToTextProtocol->ConvertDevicePathToText (BootOption->FilePathList, TRUE, TRUE);

        Print(L"\t- %s\n",DevicePathTxt);

        if (BootOption->OptionalData != NULL) {
          if (IsPrintableString (BootOption->OptionalData, &IsUnicode)) {
            if (IsUnicode) {
              Print (L"\t- Arguments: %s\n", BootOption->OptionalData);
            } else {
              AsciiPrint ("\t- Arguments: %a\n", BootOption->OptionalData);
            }
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
