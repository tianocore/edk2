/** @file
  Main file for NULL named library for debug1 profile shell command functions.

  Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "UefiShellDebug1CommandsLib.h"

STATIC CONST CHAR16 mFileName[] = L"Debug1Commands";
EFI_HANDLE gShellDebug1HiiHandle = NULL;
CONST EFI_GUID gShellDebug1HiiGuid = \
  { \
    0x25f200aa, 0xd3cb, 0x470a, { 0xbf, 0x51, 0xe7, 0xd1, 0x62, 0xd2, 0x2e, 0x6f } \
  };

CONST CHAR16*
EFIAPI
ShellCommandGetManFileNameDebug1 (
  VOID
  )
{
  return (mFileName);
}

/**
  Constructor for the Shell Debug1 Commands library.

  @param ImageHandle    the image handle of the process
  @param SystemTable    the EFI System Table pointer

  @retval EFI_SUCCESS        the shell command handlers were installed sucessfully
  @retval EFI_UNSUPPORTED    the shell level required was not found.
**/
EFI_STATUS
EFIAPI
UefiShellDebug1CommandsLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  //
  // check our bit of the profiles mask
  //
  if ((PcdGet8(PcdShellProfileMask) & BIT1) == 0) {
    return (EFI_UNSUPPORTED);
  }

  //
  // install the HII stuff.
  //
  gShellDebug1HiiHandle = HiiAddPackages (&gShellDebug1HiiGuid, gImageHandle, UefiShellDebug1CommandsLibStrings, NULL);
  if (gShellDebug1HiiHandle == NULL) {
    return (EFI_DEVICE_ERROR);
  }

  //
  // install our shell command handlers that are always installed
  //
  ShellCommandRegisterCommandName(L"SetSize",       ShellCommandRunSetSize            , ShellCommandGetManFileNameDebug1, 0, L"Debug1", TRUE, gShellDebug1HiiHandle, STRING_TOKEN(STR_GET_HELP_SETSIZE)      );
  ShellCommandRegisterCommandName(L"comp",          ShellCommandRunComp               , ShellCommandGetManFileNameDebug1, 0, L"Debug1", TRUE, gShellDebug1HiiHandle, STRING_TOKEN(STR_GET_HELP_COMP)         );
  ShellCommandRegisterCommandName(L"mode",          ShellCommandRunMode               , ShellCommandGetManFileNameDebug1, 0, L"Debug1", TRUE, gShellDebug1HiiHandle, STRING_TOKEN(STR_GET_HELP_MODE)         );
  ShellCommandRegisterCommandName(L"memmap",        ShellCommandRunMemMap             , ShellCommandGetManFileNameDebug1, 0, L"Debug1", TRUE, gShellDebug1HiiHandle, STRING_TOKEN(STR_GET_HELP_MEMMAP)       );
  ShellCommandRegisterCommandName(L"eficompress",   ShellCommandRunEfiCompress        , ShellCommandGetManFileNameDebug1, 0, L"Debug1", TRUE, gShellDebug1HiiHandle, STRING_TOKEN(STR_GET_HELP_EFICOMPRESS)  );
  ShellCommandRegisterCommandName(L"efidecompress", ShellCommandRunEfiDecompress      , ShellCommandGetManFileNameDebug1, 0, L"Debug1", TRUE, gShellDebug1HiiHandle, STRING_TOKEN(STR_GET_HELP_EFIDCOMPRESS) );
  ShellCommandRegisterCommandName(L"dmem",          ShellCommandRunDmem               , ShellCommandGetManFileNameDebug1, 0, L"Debug1", TRUE, gShellDebug1HiiHandle, STRING_TOKEN(STR_GET_HELP_DMEM)         );
  ShellCommandRegisterCommandName(L"LoadPciRom",    ShellCommandRunLoadPciRom         , ShellCommandGetManFileNameDebug1, 0, L"Debug1", TRUE, gShellDebug1HiiHandle, STRING_TOKEN(STR_GET_HELP_LOAD_PCI_ROM) );
  ShellCommandRegisterCommandName(L"mm",            ShellCommandRunMm                 , ShellCommandGetManFileNameDebug1, 0, L"Debug1", TRUE, gShellDebug1HiiHandle, STRING_TOKEN(STR_GET_HELP_MM)           );
  ShellCommandRegisterCommandName(L"SetVar",        ShellCommandRunSetVar             , ShellCommandGetManFileNameDebug1, 0, L"Debug1", TRUE, gShellDebug1HiiHandle, STRING_TOKEN(STR_GET_HELP_SETVAR)       );
  ShellCommandRegisterCommandName(L"SerMode",       ShellCommandRunSerMode            , ShellCommandGetManFileNameDebug1, 0, L"Debug1", TRUE, gShellDebug1HiiHandle, STRING_TOKEN(STR_GET_HELP_SERMODE)      );
  ShellCommandRegisterCommandName(L"Pci",           ShellCommandRunPci                , ShellCommandGetManFileNameDebug1, 0, L"Debug1", TRUE, gShellDebug1HiiHandle, STRING_TOKEN(STR_GET_HELP_PCI)          );
  ShellCommandRegisterCommandName(L"smbiosview",    ShellCommandRunSmbiosView         , ShellCommandGetManFileNameDebug1, 0, L"Debug1", TRUE, gShellDebug1HiiHandle, STRING_TOKEN(STR_GET_HELP_SMBIOSVIEW)   );
  ShellCommandRegisterCommandName(L"dmpstore",      ShellCommandRunDmpStore           , ShellCommandGetManFileNameDebug1, 0, L"Debug1", TRUE, gShellDebug1HiiHandle, STRING_TOKEN(STR_GET_HELP_DMPSTORE)     );
  ShellCommandRegisterCommandName(L"dblk",          ShellCommandRunDblk               , ShellCommandGetManFileNameDebug1, 0, L"Debug1", TRUE, gShellDebug1HiiHandle, STRING_TOKEN(STR_GET_HELP_DBLK)         );

  //
  // check install profile bit of the profiles mask is set
  //
  if ((PcdGet8(PcdShellProfileMask) & BIT2) == 0) {
    ShellCommandRegisterCommandName(L"bcfg",        ShellCommandRunBcfg               , ShellCommandGetManFileNameDebug1, 0, L"Debug1", TRUE, gShellDebug1HiiHandle, STRING_TOKEN(STR_GET_HELP_BCFG)         );
  }

/*
  ShellCommandRegisterCommandName(L"hexedit",       ShellCommandRunHexEdit            , ShellCommandGetManFileNameDebug1, 0, L"Debug1", TRUE, gShellDebug1HiiHandle, STRING_TOKEN(STR_GET_HELP_HEXEDIT       );
  ShellCommandRegisterCommandName(L"edit",          ShellCommandRunEdit               , ShellCommandGetManFileNameDebug1, 0, L"Debug1", TRUE, gShellDebug1HiiHandle, STRING_TOKEN(STR_GET_HELP_EDIT)         );
*/

  ShellCommandRegisterAlias(L"dmem", L"mem");

  return (EFI_SUCCESS);
}

/**
  Destructor for the library.  free any resources.
**/
EFI_STATUS
EFIAPI
UefiShellDebug1CommandsLibDestructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  if (gShellDebug1HiiHandle != NULL) {
    HiiRemovePackages(gShellDebug1HiiHandle);
  }
  return (EFI_SUCCESS);
}

STATIC CONST CHAR8 Hex[] = {
  '0',
  '1',
  '2',
  '3',
  '4',
  '5',
  '6',
  '7',
  '8',
  '9',
  'A',
  'B',
  'C',
  'D',
  'E',
  'F'
};

VOID
EFIAPI
DumpHex (
  IN UINTN        Indent,
  IN UINTN        Offset,
  IN UINTN        DataSize,
  IN VOID         *UserData
  )
{
  UINT8 *Data;

  CHAR8 Val[50];

  CHAR8 Str[20];

  UINT8 c;
  UINTN Size;
  UINTN Index;

  ASSERT (UserData != NULL);

  Data = UserData;
  while (DataSize != 0) {
    Size = 16;
    if (Size > DataSize) {
      Size = DataSize;
    }

    for (Index = 0; Index < Size; Index += 1) {
      c                   = Data[Index];
      Val[Index * 3 + 0]  = Hex[c >> 4];
      Val[Index * 3 + 1]  = Hex[c & 0xF];
      Val[Index * 3 + 2]  = (CHAR8) ((Index == 7) ? '-' : ' ');
      Str[Index]          = (CHAR8) ((c < ' ' || c > 'z') ? '.' : c);
    }

    Val[Index * 3]  = 0;
    Str[Index]      = 0;
    ShellPrintEx(-1, -1, L"%*a%02X: %-.48a *%a*\r\n", Indent, "", Offset, Val, Str);

    Data += Size;
    Offset += Size;
    DataSize -= Size;
  }
}

/**
  Convert a Unicode character to upper case only if
  it maps to a valid small-case ASCII character.

  This internal function only deal with Unicode character
  which maps to a valid small-case ASCII character, i.e.
  L'a' to L'z'. For other Unicode character, the input character
  is returned directly.

  @param  Char  The character to convert.

  @retval LowerCharacter   If the Char is with range L'a' to L'z'.
  @retval Unchanged        Otherwise.


  //Stolen from MdePkg Baselib
**/
CHAR16
EFIAPI
CharToUpper (
  IN      CHAR16                    Char
  )
{
  if (Char >= L'a' && Char <= L'z') {
    return (CHAR16) (Char - (L'a' - L'A'));
  }

  return Char;
}

/**
  Function returns a system configuration table that is stored in the
  EFI System Table based on the provided GUID.

  @param[in]  TableGuid    A pointer to the table's GUID type.
  @param[out] Table        On exit, a pointer to a system configuration table.

  @retval EFI_SUCCESS      A configuration table matching TableGuid was found.
  @retval EFI_NOT_FOUND    A configuration table matching TableGuid was not found.
**/
EFI_STATUS
EFIAPI
GetSystemConfigurationTable (
  IN EFI_GUID *TableGuid,
  IN OUT VOID **Table
  )
{
  UINTN Index;
  ASSERT (Table != NULL);

  for (Index = 0; Index < gST->NumberOfTableEntries; Index++) {
    if (CompareGuid (TableGuid, &(gST->ConfigurationTable[Index].VendorGuid)) == 0) {
      *Table = gST->ConfigurationTable[Index].VendorTable;
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

/**
  Convert a Unicode character to numerical value.

  This internal function only deal with Unicode character
  which maps to a valid hexadecimal ASII character, i.e.
  L'0' to L'9', L'a' to L'f' or L'A' to L'F'. For other
  Unicode character, the value returned does not make sense.

  @param  Char  The character to convert.

  @return The numerical value converted.

**/
UINTN
EFIAPI
HexCharToUintn (
  IN      CHAR16                    Char
  )
{
  if (Char >= L'0' && Char <= L'9') {
    return Char - L'0';
  }

  return (UINTN) (10 + CharToUpper (Char) - L'A');
}

EFI_STATUS
EFIAPI
ConvertStringToGuid (
  IN CONST CHAR16 *StringGuid,
  IN OUT EFI_GUID *Guid
  )
{
  if (StrLen(StringGuid) != 35) {
    return (EFI_INVALID_PARAMETER);
  } else {
    Guid->Data1 = (UINT32)StrHexToUintn(StringGuid);
    StringGuid += 9;
    Guid->Data2 = (UINT16)StrHexToUintn(StringGuid);
    StringGuid += 5;
    Guid->Data3 = (UINT16)StrHexToUintn(StringGuid);
    StringGuid += 5;
    Guid->Data4[0] = (UINT8)(HexCharToUintn(StringGuid[0]) * 16);
    Guid->Data4[0] = (UINT8)(Guid->Data4[0]+ (UINT8)HexCharToUintn(StringGuid[1]));
    StringGuid += 2;
    Guid->Data4[1] = (UINT8)(HexCharToUintn(StringGuid[0]) * 16);
    Guid->Data4[1] = (UINT8)(Guid->Data4[1] + (UINT8)HexCharToUintn(StringGuid[1]));
    StringGuid += 2;
    Guid->Data4[2] = (UINT8)(HexCharToUintn(StringGuid[0]) * 16);
    Guid->Data4[2] = (UINT8)(Guid->Data4[2] + (UINT8)HexCharToUintn(StringGuid[1]));
    StringGuid += 2;
    Guid->Data4[3] = (UINT8)(HexCharToUintn(StringGuid[0]) * 16);
    Guid->Data4[3] = (UINT8)(Guid->Data4[3] + (UINT8)HexCharToUintn(StringGuid[1]));
    StringGuid += 2;
    Guid->Data4[4] = (UINT8)(HexCharToUintn(StringGuid[0]) * 16);
    Guid->Data4[4] = (UINT8)(Guid->Data4[4] + (UINT8)HexCharToUintn(StringGuid[1]));
    StringGuid += 2;
    Guid->Data4[5] = (UINT8)(HexCharToUintn(StringGuid[0]) * 16);
    Guid->Data4[5] = (UINT8)(Guid->Data4[5] + (UINT8)HexCharToUintn(StringGuid[1]));
    StringGuid += 2;
    Guid->Data4[6] = (UINT8)(HexCharToUintn(StringGuid[0]) * 16);
    Guid->Data4[6] = (UINT8)(Guid->Data4[6] + (UINT8)HexCharToUintn(StringGuid[1]));
    StringGuid += 2;
    Guid->Data4[7] = (UINT8)(HexCharToUintn(StringGuid[0]) * 16);
    Guid->Data4[7] = (UINT8)(Guid->Data4[7] = (UINT8)HexCharToUintn(StringGuid[1]));
    return (EFI_SUCCESS);
  }
}

