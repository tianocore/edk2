/** @file
  Main file for NULL named library for debug1 profile shell command functions.

  Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "UefiShellDebug1CommandsLib.h"
#include <Library/BcfgCommandLib.h>

STATIC CONST CHAR16 mFileName[] = L"Debug1Commands";
EFI_HANDLE gShellDebug1HiiHandle = NULL;

/**
  Gets the debug file name.  This will be used if HII is not working.

  @retval NULL    No file is available.
  @return         The NULL-terminated filename to get help from.
**/
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
    return (EFI_SUCCESS);
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
  ShellCommandRegisterCommandName(L"setsize",       ShellCommandRunSetSize            , ShellCommandGetManFileNameDebug1, 0, L"Debug1", TRUE, gShellDebug1HiiHandle, STRING_TOKEN(STR_GET_HELP_SETSIZE)      );
  ShellCommandRegisterCommandName(L"comp",          ShellCommandRunComp               , ShellCommandGetManFileNameDebug1, 0, L"Debug1", TRUE, gShellDebug1HiiHandle, STRING_TOKEN(STR_GET_HELP_COMP)         );
  ShellCommandRegisterCommandName(L"mode",          ShellCommandRunMode               , ShellCommandGetManFileNameDebug1, 0, L"Debug1", TRUE, gShellDebug1HiiHandle, STRING_TOKEN(STR_GET_HELP_MODE)         );
  ShellCommandRegisterCommandName(L"memmap",        ShellCommandRunMemMap             , ShellCommandGetManFileNameDebug1, 0, L"Debug1", TRUE, gShellDebug1HiiHandle, STRING_TOKEN(STR_GET_HELP_MEMMAP)       );
  ShellCommandRegisterCommandName(L"eficompress",   ShellCommandRunEfiCompress        , ShellCommandGetManFileNameDebug1, 0, L"Debug1", TRUE, gShellDebug1HiiHandle, STRING_TOKEN(STR_GET_HELP_EFICOMPRESS)  );
  ShellCommandRegisterCommandName(L"efidecompress", ShellCommandRunEfiDecompress      , ShellCommandGetManFileNameDebug1, 0, L"Debug1", TRUE, gShellDebug1HiiHandle, STRING_TOKEN(STR_GET_HELP_EFIDCOMPRESS) );
  ShellCommandRegisterCommandName(L"dmem",          ShellCommandRunDmem               , ShellCommandGetManFileNameDebug1, 0, L"Debug1", TRUE, gShellDebug1HiiHandle, STRING_TOKEN(STR_GET_HELP_DMEM)         );
  ShellCommandRegisterCommandName(L"loadpcirom",    ShellCommandRunLoadPciRom         , ShellCommandGetManFileNameDebug1, 0, L"Debug1", TRUE, gShellDebug1HiiHandle, STRING_TOKEN(STR_GET_HELP_LOAD_PCI_ROM) );
  ShellCommandRegisterCommandName(L"mm",            ShellCommandRunMm                 , ShellCommandGetManFileNameDebug1, 0, L"Debug1", TRUE, gShellDebug1HiiHandle, STRING_TOKEN(STR_GET_HELP_MM)           );
  ShellCommandRegisterCommandName(L"setvar",        ShellCommandRunSetVar             , ShellCommandGetManFileNameDebug1, 0, L"Debug1", TRUE, gShellDebug1HiiHandle, STRING_TOKEN(STR_GET_HELP_SETVAR)       );
  ShellCommandRegisterCommandName(L"sermode",       ShellCommandRunSerMode            , ShellCommandGetManFileNameDebug1, 0, L"Debug1", TRUE, gShellDebug1HiiHandle, STRING_TOKEN(STR_GET_HELP_SERMODE)      );
  ShellCommandRegisterCommandName(L"pci",           ShellCommandRunPci                , ShellCommandGetManFileNameDebug1, 0, L"Debug1", TRUE, gShellDebug1HiiHandle, STRING_TOKEN(STR_GET_HELP_PCI)          );
  ShellCommandRegisterCommandName(L"smbiosview",    ShellCommandRunSmbiosView         , ShellCommandGetManFileNameDebug1, 0, L"Debug1", TRUE, gShellDebug1HiiHandle, STRING_TOKEN(STR_GET_HELP_SMBIOSVIEW)   );
  ShellCommandRegisterCommandName(L"dmpstore",      ShellCommandRunDmpStore           , ShellCommandGetManFileNameDebug1, 0, L"Debug1", TRUE, gShellDebug1HiiHandle, STRING_TOKEN(STR_GET_HELP_DMPSTORE)     );
  ShellCommandRegisterCommandName(L"dblk",          ShellCommandRunDblk               , ShellCommandGetManFileNameDebug1, 0, L"Debug1", TRUE, gShellDebug1HiiHandle, STRING_TOKEN(STR_GET_HELP_DBLK)         );
  ShellCommandRegisterCommandName(L"edit",          ShellCommandRunEdit               , ShellCommandGetManFileNameDebug1, 0, L"Debug1", TRUE, gShellDebug1HiiHandle, STRING_TOKEN(STR_GET_HELP_EDIT)         );
  ShellCommandRegisterCommandName(L"hexedit",       ShellCommandRunHexEdit            , ShellCommandGetManFileNameDebug1, 0, L"Debug1", TRUE, gShellDebug1HiiHandle, STRING_TOKEN(STR_GET_HELP_HEXEDIT)      );

  ShellCommandRegisterAlias(L"dmem", L"mem");

  BcfgLibraryRegisterBcfgCommand(ImageHandle, SystemTable, L"Debug1");

  return (EFI_SUCCESS);
}

/**
  Destructor for the library.  free any resources.

  @param ImageHandle            The image handle of the process.
  @param SystemTable            The EFI System Table pointer.
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

  BcfgLibraryUnregisterBcfgCommand(ImageHandle, SystemTable);
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

/**
  Dump some hexadecimal data to the screen.

  @param[in] Indent     How many spaces to indent the output.
  @param[in] Offset     The offset of the printing.
  @param[in] DataSize   The size in bytes of UserData.
  @param[in] UserData   The data to print out.
**/
VOID
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

  UINT8 TempByte;
  UINTN Size;
  UINTN Index;

  Data = UserData;
  while (DataSize != 0) {
    Size = 16;
    if (Size > DataSize) {
      Size = DataSize;
    }

    for (Index = 0; Index < Size; Index += 1) {
      TempByte            = Data[Index];
      Val[Index * 3 + 0]  = Hex[TempByte >> 4];
      Val[Index * 3 + 1]  = Hex[TempByte & 0xF];
      Val[Index * 3 + 2]  = (CHAR8) ((Index == 7) ? '-' : ' ');
      Str[Index]          = (CHAR8) ((TempByte < ' ' || TempByte > 'z') ? '.' : TempByte);
    }

    Val[Index * 3]  = 0;
    Str[Index]      = 0;
    ShellPrintEx(-1, -1, L"%*a%08X: %-48a *%a*\r\n", Indent, "", Offset, Val, Str);

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

  @param[in]  TableGuid     A pointer to the table's GUID type.
  @param[in, out] Table     On exit, a pointer to a system configuration table.

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
    if (CompareGuid (TableGuid, &(gST->ConfigurationTable[Index].VendorGuid))) {
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

/**
  Convert a string representation of a guid to a Guid value.

  @param[in] StringGuid    The pointer to the string of a guid.
  @param[in, out] Guid     The pointer to the GUID structure to populate.

  @retval EFI_INVALID_PARAMETER   A parameter was invalid.
  @retval EFI_SUCCESS             The conversion was successful.
**/
EFI_STATUS
EFIAPI
ConvertStringToGuid (
  IN CONST CHAR16 *StringGuid,
  IN OUT EFI_GUID *Guid
  )
{
  CHAR16  *TempCopy;
  CHAR16  *TempSpot;
  CHAR16  *Walker;
  UINT64  TempVal;
  EFI_STATUS Status;

  if (StringGuid == NULL) {
    return (EFI_INVALID_PARAMETER);
  } else if (StrLen(StringGuid) != 36) {
    return (EFI_INVALID_PARAMETER);
  } 
  TempCopy = NULL;
  TempCopy = StrnCatGrow(&TempCopy, NULL, StringGuid, 0);
  if (TempCopy == NULL) {
    return (EFI_OUT_OF_RESOURCES);
  }
  Walker   = TempCopy;
  TempSpot = StrStr(Walker, L"-");
  if (TempSpot != NULL) {
    *TempSpot = CHAR_NULL;
  }
  Status = ShellConvertStringToUint64(Walker, &TempVal, TRUE, FALSE);
  if (EFI_ERROR(Status)) {
    FreePool(TempCopy);
    return (Status);
  }
  Guid->Data1 = (UINT32)TempVal;
  Walker += 9;
  TempSpot = StrStr(Walker, L"-");
  if (TempSpot != NULL) {
    *TempSpot = CHAR_NULL;
  }
  Status = ShellConvertStringToUint64(Walker, &TempVal, TRUE, FALSE);
  if (EFI_ERROR(Status)) {
    FreePool(TempCopy);
    return (Status);
  }
  Guid->Data2 = (UINT16)TempVal;
  Walker += 5;
  TempSpot = StrStr(Walker, L"-");
  if (TempSpot != NULL) {
    *TempSpot = CHAR_NULL;
  }
  Status = ShellConvertStringToUint64(Walker, &TempVal, TRUE, FALSE);
  if (EFI_ERROR(Status)) {
    FreePool(TempCopy);
    return (Status);
  }
  Guid->Data3 = (UINT16)TempVal;
  Walker += 5;
  Guid->Data4[0] = (UINT8)(HexCharToUintn(Walker[0]) * 16);
  Guid->Data4[0] = (UINT8)(Guid->Data4[0]+ (UINT8)HexCharToUintn(Walker[1]));
  Walker += 2;
  Guid->Data4[1] = (UINT8)(HexCharToUintn(Walker[0]) * 16);
  Guid->Data4[1] = (UINT8)(Guid->Data4[1] + (UINT8)HexCharToUintn(Walker[1]));
  Walker += 3;
  Guid->Data4[2] = (UINT8)(HexCharToUintn(Walker[0]) * 16);
  Guid->Data4[2] = (UINT8)(Guid->Data4[2] + (UINT8)HexCharToUintn(Walker[1]));
  Walker += 2;
  Guid->Data4[3] = (UINT8)(HexCharToUintn(Walker[0]) * 16);
  Guid->Data4[3] = (UINT8)(Guid->Data4[3] + (UINT8)HexCharToUintn(Walker[1]));
  Walker += 2;
  Guid->Data4[4] = (UINT8)(HexCharToUintn(Walker[0]) * 16);
  Guid->Data4[4] = (UINT8)(Guid->Data4[4] + (UINT8)HexCharToUintn(Walker[1]));
  Walker += 2;
  Guid->Data4[5] = (UINT8)(HexCharToUintn(Walker[0]) * 16);
  Guid->Data4[5] = (UINT8)(Guid->Data4[5] + (UINT8)HexCharToUintn(Walker[1]));
  Walker += 2;
  Guid->Data4[6] = (UINT8)(HexCharToUintn(Walker[0]) * 16);
  Guid->Data4[6] = (UINT8)(Guid->Data4[6] + (UINT8)HexCharToUintn(Walker[1]));
  Walker += 2;
  Guid->Data4[7] = (UINT8)(HexCharToUintn(Walker[0]) * 16);
  Guid->Data4[7] = (UINT8)(Guid->Data4[7] + (UINT8)HexCharToUintn(Walker[1]));
  FreePool(TempCopy);
  return (EFI_SUCCESS);
}

/**
  Clear the line at the specified Row.
  
  @param[in] Row                The row number to be cleared ( start from 1 )
  @param[in] LastCol            The last printable column.
  @param[in] LastRow            The last printable row.
**/
VOID
EFIAPI
EditorClearLine (
  IN UINTN Row,
  IN UINTN LastCol,
  IN UINTN LastRow
  )
{
  CHAR16 Line[200];

  if (Row == 0) {
    Row = 1;
  }

  //
  // prepare a blank line
  //
  SetMem16(Line, LastCol*sizeof(CHAR16), L' ');

  if (Row == LastRow) {
    //
    // if CHAR_NULL is still at position 80, it will cause first line error
    //
    Line[LastCol - 1] = CHAR_NULL;
  } else {
    Line[LastCol] = CHAR_NULL;
  }

  //
  // print out the blank line
  //
  ShellPrintEx (0, ((INT32)Row) - 1, Line);
}

/**
  Determine if the character is valid for a filename.

  @param[in] Ch     The character to test.

  @retval TRUE      The character is valid.
  @retval FALSE     The character is not valid.
**/
BOOLEAN
EFIAPI
IsValidFileNameChar (
  IN CONST CHAR16 Ch
  )
{
  //
  // See if there are any illegal characters within the name
  //
  if (Ch < 0x20 || Ch == L'\"' || Ch == L'*' || Ch == L'/' || Ch == L'<' || Ch == L'>' || Ch == L'?' || Ch == L'|') {
    return FALSE;
  }

  return TRUE;
}

/**
  Check if file name has illegal characters.
  
  @param Name       The filename to check.

  @retval TRUE      The filename is ok.
  @retval FALSE     The filename is not ok.
**/
BOOLEAN
EFIAPI
IsValidFileName (
  IN CONST CHAR16 *Name
  )
{

  UINTN Index;
  UINTN Len;

  //
  // check the length of Name
  //
  for (Len = 0, Index = StrLen (Name) - 1; Index + 1 != 0; Index--, Len++) {
    if (Name[Index] == '\\' || Name[Index] == ':') {
      break;
    }
  }

  if (Len == 0 || Len > 255) {
    return FALSE;
  }
  //
  // check whether any char in Name not appears in valid file name char
  //
  for (Index = 0; Index < StrLen (Name); Index++) {
    if (!IsValidFileNameChar (Name[Index])) {
      return FALSE;
    }
  }

  return TRUE;
}

/**
  Find a filename that is valid (not taken) with the given extension.

  @param[in] Extension      The file extension.

  @retval NULL  Something went wrong.
  @return the valid filename.
**/
CHAR16 *
EFIAPI
EditGetDefaultFileName (
  IN CONST CHAR16 *Extension
  )
{
  EFI_STATUS         Status;
  UINTN              Suffix;
  CHAR16             *FileNameTmp;

  Suffix       = 0;

  do {
    FileNameTmp = CatSPrint (NULL, L"NewFile%d.%s", Suffix, Extension);

    //
    // after that filename changed to path
    //
    Status = ShellFileExists (FileNameTmp);

    if (Status == EFI_NOT_FOUND) {
      return FileNameTmp;
    }

    FreePool (FileNameTmp);
    FileNameTmp = NULL;
    Suffix++;
  } while (Suffix != 0);

  FreePool (FileNameTmp);
  return NULL;
}

/**
  Read a file into an allocated buffer.  The buffer is the responsibility 
  of the caller to free.

  @param[in]  FileName          The filename of the file to open.
  @param[out] Buffer            Upon successful return, the pointer to the 
                                address of the allocated buffer.                                  
  @param[out] BufferSize        If not NULL, then the pointer to the size
                                of the allocated buffer.
  @param[out] ReadOnly          Upon successful return TRUE if the file is
                                read only.  FALSE otherwise.

  @retval EFI_NOT_FOUND         The filename did not represent a file in the 
                                file system.
  @retval EFI_SUCCESS           The file was read into the buffer.
  @retval EFI_OUT_OF_RESOURCES  A memory allocation failed.
  @retval EFI_LOAD_ERROR        The file read operation failed.
  @retval EFI_INVALID_PARAMETER A parameter was invalid.
  @retval EFI_INVALID_PARAMETER FileName was NULL.
  @retval EFI_INVALID_PARAMETER FileName was a directory.
**/
EFI_STATUS
EFIAPI
ReadFileIntoBuffer (
  IN CONST CHAR16 *FileName,
  OUT VOID        **Buffer,
  OUT UINTN       *BufferSize OPTIONAL,
  OUT BOOLEAN     *ReadOnly
  )
{
  VOID              *InternalBuffer;
  UINTN             FileSize;
  SHELL_FILE_HANDLE FileHandle;
  BOOLEAN           CreateFile;
  EFI_STATUS        Status;
  EFI_FILE_INFO     *Info;

  InternalBuffer  = NULL;
  FileSize        = 0;
  FileHandle      = NULL;
  CreateFile      = FALSE;
  Status          = EFI_SUCCESS;
  Info            = NULL;

  if (FileName == NULL || Buffer == NULL || ReadOnly == NULL) {
    return (EFI_INVALID_PARAMETER);
  }

  //
  // try to open the file
  //
  Status = ShellOpenFileByName (FileName, &FileHandle, EFI_FILE_MODE_READ, 0);

  if (!EFI_ERROR(Status)) {
    ASSERT(CreateFile == FALSE);
    if (FileHandle == NULL) {
      return EFI_LOAD_ERROR;
    }

    Info = ShellGetFileInfo(FileHandle);
    
    if (Info->Attribute & EFI_FILE_DIRECTORY) {
      FreePool (Info);
      return EFI_INVALID_PARAMETER;
    }

    if (Info->Attribute & EFI_FILE_READ_ONLY) {
      *ReadOnly = TRUE;
    } else {
      *ReadOnly = FALSE;
    }
    //
    // get file size
    //
    FileSize = (UINTN) Info->FileSize;

    FreePool (Info);
  } else if (Status == EFI_NOT_FOUND) {
    //
    // file not exists.  add create and try again
    //
    Status = ShellOpenFileByName (FileName, &FileHandle, EFI_FILE_MODE_READ|EFI_FILE_MODE_WRITE|EFI_FILE_MODE_CREATE, 0);
    if (EFI_ERROR (Status)) {
      return Status;
    } else {
      //
      // it worked.  now delete it and move on with the name (now validated)
      //
      Status = ShellDeleteFile (&FileHandle);
      if (Status == EFI_WARN_DELETE_FAILURE) {
        Status = EFI_ACCESS_DENIED;
      }
      if (EFI_ERROR (Status)) {
        return Status;
      }
    }
    //
    // file doesn't exist, so set CreateFile to TRUE and can't be read-only
    //
    CreateFile = TRUE;
    *ReadOnly  = FALSE;
  }

  //
  // the file exists
  //
  if (!CreateFile) {
    //
    // allocate buffer to read file
    //
    InternalBuffer = AllocateZeroPool (FileSize);
    if (InternalBuffer == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    //
    // read file into InternalBuffer
    //
    Status = ShellReadFile (FileHandle, &FileSize, InternalBuffer);
    ShellCloseFile(&FileHandle);
    FileHandle = NULL;
    if (EFI_ERROR (Status)) {
      SHELL_FREE_NON_NULL (InternalBuffer);
      return EFI_LOAD_ERROR;
    }
  }
  *Buffer = InternalBuffer;
  if (BufferSize != NULL) {
    *BufferSize = FileSize;
  }
  return (EFI_SUCCESS);

}
