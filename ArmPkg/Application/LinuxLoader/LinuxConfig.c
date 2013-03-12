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

#include "LinuxInternal.h"

#define DEFAULT_BOOT_ENTRY_DESCRIPTION  L"Linux"
#define MAX_STR_INPUT                   300
#define MAX_ASCII_INPUT                 300

typedef enum {
  LINUX_LOADER_NEW = 1,
  LINUX_LOADER_UPDATE
} LINUX_LOADER_ACTION;

STATIC
EFI_STATUS
EditHIInputStr (
  IN OUT CHAR16  *CmdLine,
  IN     UINTN   MaxCmdLine
  )
{
  UINTN           CmdLineIndex;
  UINTN           WaitIndex;
  CHAR8           Char;
  EFI_INPUT_KEY   Key;
  EFI_STATUS      Status;

  Print (CmdLine);

  for (CmdLineIndex = StrLen (CmdLine); CmdLineIndex < MaxCmdLine; ) {
    Status = gBS->WaitForEvent (1, &gST->ConIn->WaitForKey, &WaitIndex);
    ASSERT_EFI_ERROR (Status);

    Status = gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);
    ASSERT_EFI_ERROR (Status);

    // Unicode character is valid when Scancode is NUll
    if (Key.ScanCode == SCAN_NULL) {
      // Scan code is NUll, hence read Unicode character
      Char = (CHAR8)Key.UnicodeChar;
    } else {
      Char = CHAR_NULL;
    }

    if ((Char == CHAR_LINEFEED) || (Char == CHAR_CARRIAGE_RETURN) || (Char == 0x7f)) {
      CmdLine[CmdLineIndex] = '\0';
      Print (L"\n\r");

      return EFI_SUCCESS;
    } else if ((Key.UnicodeChar == L'\b') || (Key.ScanCode == SCAN_LEFT) || (Key.ScanCode == SCAN_DELETE)){
      if (CmdLineIndex != 0) {
        CmdLineIndex--;
        Print (L"\b \b");
      }
    } else if ((Key.ScanCode == SCAN_ESC) || (Char == 0x1B) || (Char == 0x0)) {
      return EFI_INVALID_PARAMETER;
    } else {
      CmdLine[CmdLineIndex++] = Key.UnicodeChar;
      Print (L"%c", Key.UnicodeChar);
    }
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EditHIInputAscii (
  IN OUT CHAR8   *CmdLine,
  IN     UINTN   MaxCmdLine
  )
{
  CHAR16*     Str;
  EFI_STATUS  Status;

  Str = (CHAR16*)AllocatePool (MaxCmdLine * sizeof(CHAR16));
  AsciiStrToUnicodeStr (CmdLine, Str);

  Status = EditHIInputStr (Str, MaxCmdLine);

  UnicodeStrToAsciiStr (Str, CmdLine);
  FreePool (Str);

  return Status;
}

STATIC
EFI_STATUS
GetHIInputInteger (
  OUT UINTN   *Integer
  )
{
  CHAR16      CmdLine[255];
  EFI_STATUS  Status;

  CmdLine[0] = '\0';
  Status = EditHIInputStr (CmdLine, 255);
  if (!EFI_ERROR(Status)) {
    *Integer = StrDecimalToUintn (CmdLine);
  }

  return Status;
}

#if 0
EFI_STATUS
GenerateDeviceDescriptionName (
  IN  EFI_HANDLE  Handle,
  IN OUT CHAR16*  Description
  )
{
  EFI_STATUS                        Status;
  EFI_COMPONENT_NAME_PROTOCOL*      ComponentName2Protocol;
  EFI_DEVICE_PATH_TO_TEXT_PROTOCOL* DevicePathToTextProtocol;
  EFI_DEVICE_PATH_PROTOCOL*         DevicePathProtocol;
  CHAR16*                           DriverName;
  CHAR16*                           DevicePathTxt;
  EFI_DEVICE_PATH*                  DevicePathNode;

  ComponentName2Protocol = NULL;
  Status = gBS->HandleProtocol (Handle, &gEfiComponentName2ProtocolGuid, (VOID **)&ComponentName2Protocol);
  if (!EFI_ERROR(Status)) {
    //TODO: Fixme. we must find the best langague
    Status = ComponentName2Protocol->GetDriverName (ComponentName2Protocol,"en",&DriverName);
    if (!EFI_ERROR(Status)) {
      StrnCpy (Description,DriverName,BOOT_DEVICE_DESCRIPTION_MAX);
    }
  }

  if (EFI_ERROR(Status)) {
    // Use the lastest non null entry of the Device path as a description
    Status = gBS->HandleProtocol (Handle, &gEfiDevicePathProtocolGuid, (VOID **)&DevicePathProtocol);
    if (EFI_ERROR(Status)) {
      return Status;
    }

    // Convert the last non end-type Device Path Node in text for the description
    DevicePathNode = GetLastDevicePathNode (DevicePathProtocol);
    Status = gBS->LocateProtocol (&gEfiDevicePathToTextProtocolGuid, NULL, (VOID **)&DevicePathToTextProtocol);
    ASSERT_EFI_ERROR(Status);
    DevicePathTxt = DevicePathToTextProtocol->ConvertDevicePathToText(DevicePathNode,TRUE,TRUE);
    StrnCpy (Description, DevicePathTxt, BOOT_DEVICE_DESCRIPTION_MAX);
    FreePool (DevicePathTxt);
  }

  return EFI_SUCCESS;
}
#endif

EFI_STATUS
LinuxLoaderConfig (
  IN EFI_LOADED_IMAGE_PROTOCOL   *LoadedImage
  )
{
  EFI_STATUS                   Status;
  LINUX_LOADER_ACTION          Choice;
  UINTN                        BootOrderSize;
  UINT16*                      BootOrder;
  UINTN                        BootOrderCount;
  UINTN                        Index;
  CHAR16                       Description[MAX_ASCII_INPUT];
  CHAR8                        CmdLine[MAX_ASCII_INPUT];
  CHAR16                       Initrd[MAX_STR_INPUT];
  UINT16                       InitrdPathListLength;
  UINT16                       CmdLineLength;
  BDS_LOAD_OPTION*             BdsLoadOption;
  BDS_LOAD_OPTION**            SupportedBdsLoadOptions;
  UINTN                        SupportedBdsLoadOptionCount;
  LINUX_LOADER_OPTIONAL_DATA*  LinuxOptionalData;
  EFI_DEVICE_PATH*             DevicePathRoot;

  Choice = (LINUX_LOADER_ACTION)0;
  SupportedBdsLoadOptions = NULL;
  SupportedBdsLoadOptionCount = 0;

  do {
    Print (L"[%d] Create new Linux Boot Entry\n",LINUX_LOADER_NEW);
    Print (L"[%d] Update Linux Boot Entry\n",LINUX_LOADER_UPDATE);

    Print (L"Option: ");
    Status = GetHIInputInteger ((UINTN*)&Choice);
    if (Status == EFI_INVALID_PARAMETER) {
      Print (L"\n");
      return Status;
    } else if ((Choice != LINUX_LOADER_NEW) && (Choice != LINUX_LOADER_UPDATE)) {
      Print (L"Error: the option should be either '%d' or '%d'\n",LINUX_LOADER_NEW,LINUX_LOADER_UPDATE);
      Status = EFI_INVALID_PARAMETER;
    }
  } while (EFI_ERROR(Status));

  if (Choice == LINUX_LOADER_UPDATE) {
    // If no compatible entry then we just create a new entry
    Choice = LINUX_LOADER_NEW;

    // Scan the OptionalData of every entry for the correct signature
    Status = GetGlobalEnvironmentVariable (L"BootOrder", NULL, &BootOrderSize, (VOID**)&BootOrder);
    if (!EFI_ERROR(Status)) {
      BootOrderCount = BootOrderSize / sizeof(UINT16);

      // Allocate an array to handle maximum number of supported Boot Entry
      SupportedBdsLoadOptions = (BDS_LOAD_OPTION**)AllocatePool(sizeof(BDS_LOAD_OPTION*) * BootOrderCount);

      SupportedBdsLoadOptionCount = 0;

      // Check if the signature is present in the list of the current Boot entries
      for (Index = 0; Index < BootOrderCount; Index++) {
        Status = BootOptionFromLoadOptionIndex (BootOrder[Index], &BdsLoadOption);
        if (!EFI_ERROR(Status)) {
          if ((BdsLoadOption->OptionalDataSize >= sizeof(UINT32)) &&
              (*(UINT32*)BdsLoadOption->OptionalData == LINUX_LOADER_SIGNATURE)) {
            SupportedBdsLoadOptions[SupportedBdsLoadOptionCount++] = BdsLoadOption;
            Choice = LINUX_LOADER_UPDATE;
          }
        }
      }
    }
    FreePool (BootOrder);
  }

  if (Choice == LINUX_LOADER_NEW) {
    Description[0] = '\0';
    CmdLine[0]     = '\0';
    Initrd[0]      = '\0';

    BdsLoadOption = (BDS_LOAD_OPTION*)AllocateZeroPool (sizeof(BDS_LOAD_OPTION));

    DEBUG_CODE_BEGIN();
      CHAR16*                           DevicePathTxt;
      EFI_DEVICE_PATH_TO_TEXT_PROTOCOL* DevicePathToTextProtocol;

      Status = gBS->LocateProtocol (&gEfiDevicePathToTextProtocolGuid, NULL, (VOID **)&DevicePathToTextProtocol);
      ASSERT_EFI_ERROR(Status);
      DevicePathTxt = DevicePathToTextProtocol->ConvertDevicePathToText (LoadedImage->FilePath, TRUE, TRUE);

      Print(L"EFI OS Loader: %s\n",DevicePathTxt);

      FreePool(DevicePathTxt);
    DEBUG_CODE_END();

    //
    // Fill the known fields of BdsLoadOption
    //

    BdsLoadOption->Attributes = LOAD_OPTION_ACTIVE | LOAD_OPTION_CATEGORY_BOOT;

    // Get the full Device Path for this file
    Status = gBS->HandleProtocol (LoadedImage->DeviceHandle, &gEfiDevicePathProtocolGuid, (VOID **)&DevicePathRoot);
    ASSERT_EFI_ERROR(Status);

    BdsLoadOption->FilePathList = AppendDevicePath (DevicePathRoot, LoadedImage->FilePath);
    BdsLoadOption->FilePathListLength = GetDevicePathSize (BdsLoadOption->FilePathList);
  } else {
    if (SupportedBdsLoadOptionCount > 1) {
      for (Index = 0; Index < SupportedBdsLoadOptionCount; Index++) {
        Print (L"[%d] %s\n",Index + 1,SupportedBdsLoadOptions[Index]->Description);
      }

      do {
        Print (L"Update Boot Entry: ");
        Status = GetHIInputInteger ((UINTN*)&Choice);
        if (Status == EFI_INVALID_PARAMETER) {
          Print (L"\n");
          return Status;
        } else if ((Choice < 1) && (Choice > SupportedBdsLoadOptionCount)) {
          Print (L"Choose entry from 1 to %d\n",SupportedBdsLoadOptionCount);
          Status = EFI_INVALID_PARAMETER;
        }
      } while (EFI_ERROR(Status));
      BdsLoadOption = SupportedBdsLoadOptions[Choice-1];
    }
    StrnCpy (Description, BdsLoadOption->Description, MAX_STR_INPUT);

    LinuxOptionalData = (LINUX_LOADER_OPTIONAL_DATA*)BdsLoadOption->OptionalData;
    if (LinuxOptionalData->CmdLineLength > 0) {
      CopyMem (CmdLine, (CHAR8*)LinuxOptionalData + sizeof(LINUX_LOADER_OPTIONAL_DATA), LinuxOptionalData->CmdLineLength);
    } else {
      CmdLine[0] = '\0';
    }

    if (LinuxOptionalData->InitrdPathListLength > 0) {
      CopyMem (Initrd, (CHAR8*)LinuxOptionalData + sizeof(LINUX_LOADER_OPTIONAL_DATA) + LinuxOptionalData->CmdLineLength, LinuxOptionalData->InitrdPathListLength);
    } else {
      Initrd[0] = L'\0';
    }
    DEBUG((EFI_D_ERROR,"L\n"));
  }

  // Description
  Print (L"Description: ");
  Status = EditHIInputStr (Description, MAX_STR_INPUT);
  if (EFI_ERROR(Status)) {
    return Status;
  }
  if (StrLen (Description) == 0) {
    StrnCpy (Description, DEFAULT_BOOT_ENTRY_DESCRIPTION, MAX_STR_INPUT);
  }
  BdsLoadOption->Description = Description;

  // CmdLine
  Print (L"Command Line: ");
  Status = EditHIInputAscii (CmdLine, MAX_ASCII_INPUT);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  // Initrd
  Print (L"Initrd name: ");
  Status = EditHIInputStr (Initrd, MAX_STR_INPUT);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  CmdLineLength = AsciiStrLen (CmdLine);
  if (CmdLineLength > 0) {
    CmdLineLength += sizeof(CHAR8);
  }

  InitrdPathListLength = StrLen (Initrd) * sizeof(CHAR16);
  if (InitrdPathListLength > 0) {
    InitrdPathListLength += sizeof(CHAR16);
  }

  BdsLoadOption->OptionalDataSize = sizeof(LINUX_LOADER_OPTIONAL_DATA) + CmdLineLength + InitrdPathListLength;

  LinuxOptionalData = (LINUX_LOADER_OPTIONAL_DATA*)AllocatePool (BdsLoadOption->OptionalDataSize);
  BdsLoadOption->OptionalData = LinuxOptionalData;

  LinuxOptionalData->Signature = LINUX_LOADER_SIGNATURE;
  LinuxOptionalData->CmdLineLength = CmdLineLength;
  LinuxOptionalData->InitrdPathListLength = InitrdPathListLength;

  if (CmdLineLength > 0) {
    CopyMem (LinuxOptionalData + 1, CmdLine, CmdLineLength);
  }
  if (InitrdPathListLength > 0) {
    CopyMem ((UINT8*)(LinuxOptionalData + 1) + CmdLineLength, Initrd, InitrdPathListLength);
  }

  // Create or Update the boot entry
  Status = BootOptionToLoadOptionVariable (BdsLoadOption);

  return Status;
}
