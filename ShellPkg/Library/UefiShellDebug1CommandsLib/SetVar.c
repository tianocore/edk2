/** @file
  Main file for SetVar shell Debug1 function.

  (C) Copyright 2015 Hewlett-Packard Development Company, L.P.<BR>
  Copyright (c) 2010 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UefiShellDebug1CommandsLib.h"

STATIC CONST SHELL_PARAM_ITEM  ParamList[] = {
  { L"-guid", TypeValue },
  { L"-bs",   TypeFlag  },
  { L"-rt",   TypeFlag  },
  { L"-nv",   TypeFlag  },
  { NULL,     TypeMax   }
};

typedef enum {
  DataTypeHexNumber  = 0,
  DataTypeHexArray   = 1,
  DataTypeAscii      = 2,
  DataTypeUnicode    = 3,
  DataTypeDevicePath = 4,
  DataTypeUnKnow     = 5
} DATA_TYPE;

typedef union {
  UINT8     HexNumber8;
  UINT16    HexNumber16;
  UINT32    HexNumber32;
  UINT64    HexNumber64;
} HEX_NUMBER;

/**
  Check if the input is a (potentially empty) string of hexadecimal nibbles.

  @param[in] String  The CHAR16 string to check.

  @retval FALSE  A character has been found in String for which
                 ShellIsHexaDecimalDigitCharacter() returned FALSE.

  @retval TRUE   Otherwise. (Note that this covers the case when String is
                 empty.)
**/
BOOLEAN
IsStringOfHexNibbles (
  IN CONST CHAR16  *String
  )
{
  CONST CHAR16  *Pos;

  for (Pos = String; *Pos != L'\0'; ++Pos) {
    if (!ShellIsHexaDecimalDigitCharacter (*Pos)) {
      return FALSE;
    }
  }

  return TRUE;
}

/**
  Function to check the TYPE of Data.

  @param[in]    Data          The Data to be check.

  @retval       DATA_TYPE     The TYPE of Data.
**/
DATA_TYPE
TestDataType (
  IN CONST CHAR16  *Data
  )
{
  if ((Data[0] == L'0') && ((Data[1] == L'x') || (Data[1] == L'X'))) {
    if (IsStringOfHexNibbles (Data+2) && (StrLen (Data + 2) <= 16)) {
      return DataTypeHexNumber;
    } else {
      return DataTypeUnKnow;
    }
  } else if (Data[0] == L'H') {
    if (IsStringOfHexNibbles (Data + 1) && (StrLen (Data + 1) % 2 == 0)) {
      return DataTypeHexArray;
    } else {
      return DataTypeUnKnow;
    }
  } else if (Data[0] == L'S') {
    return DataTypeAscii;
  } else if (Data[0] == L'L') {
    return DataTypeUnicode;
  } else if ((Data[0] == L'P') || (StrnCmp (Data, L"--", 2) == 0)) {
    return DataTypeDevicePath;
  }

  if (IsStringOfHexNibbles (Data) && (StrLen (Data) % 2 == 0)) {
    return DataTypeHexArray;
  }

  return DataTypeAscii;
}

/**
  Function to parse the Data by the type of Data, and save in the Buffer.

  @param[in]      Data                A pointer to a buffer to be parsed.
  @param[out]     Buffer              A pointer to a buffer to hold the return data.
  @param[in,out]  BufferSize          On input, indicates the size of Buffer in bytes.
                                      On output,indicates the size of data return in Buffer.
                                      Or the size in bytes of the buffer needed to obtain.

  @retval   EFI_INVALID_PARAMETER     The Buffer or BufferSize is NULL.
  @retval   EFI_BUFFER_TOO_SMALL      The Buffer is too small to hold the data.
  @retval   EFI_OUT_OF_RESOURCES      A memory allcation failed.
  @retval   EFI_SUCCESS               The Data parsed successful and save in the Buffer.
**/
EFI_STATUS
ParseParameterData (
  IN CONST CHAR16  *Data,
  OUT VOID         *Buffer,
  IN OUT UINTN     *BufferSize
  )
{
  UINT64                    HexNumber;
  UINTN                     HexNumberLen;
  UINTN                     Size;
  CHAR8                     *AsciiBuffer;
  DATA_TYPE                 DataType;
  EFI_DEVICE_PATH_PROTOCOL  *DevPath;
  EFI_STATUS                Status;

  HexNumber    = 0;
  HexNumberLen = 0;
  Size         = 0;
  AsciiBuffer  = NULL;
  DevPath      = NULL;
  Status       = EFI_SUCCESS;

  if ((Data == NULL) || (BufferSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  DataType = TestDataType (Data);
  if (DataType == DataTypeHexNumber) {
    //
    // hex number
    //
    StrHexToUint64S (Data + 2, NULL, &HexNumber);
    HexNumberLen = StrLen (Data + 2);
    if ((HexNumberLen >= 1) && (HexNumberLen <= 2)) {
      Size = 1;
    } else if ((HexNumberLen >= 3) && (HexNumberLen <= 4)) {
      Size = 2;
    } else if ((HexNumberLen >= 5) && (HexNumberLen <= 8)) {
      Size = 4;
    } else if ((HexNumberLen >= 9) && (HexNumberLen <= 16)) {
      Size = 8;
    }

    if ((Buffer != NULL) && (*BufferSize >= Size)) {
      CopyMem (Buffer, (VOID *)&HexNumber, Size);
    } else {
      Status = EFI_BUFFER_TOO_SMALL;
    }

    *BufferSize = Size;
  } else if (DataType == DataTypeHexArray) {
    //
    // hex array
    //
    if (*Data == L'H') {
      Data = Data + 1;
    }

    Size = StrLen (Data) / 2;
    if ((Buffer != NULL) && (*BufferSize >= Size)) {
      StrHexToBytes (Data, StrLen (Data), (UINT8 *)Buffer, Size);
    } else {
      Status = EFI_BUFFER_TOO_SMALL;
    }

    *BufferSize = Size;
  } else if (DataType == DataTypeAscii) {
    //
    // ascii text
    //
    if (*Data == L'S') {
      Data = Data + 1;
    }

    AsciiBuffer = AllocateZeroPool (StrSize (Data) / 2);
    if (AsciiBuffer == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
    } else {
      AsciiSPrint (AsciiBuffer, StrSize (Data) / 2, "%s", (CHAR8 *)Data);

      Size = StrSize (Data) / 2 - 1;
      if ((Buffer != NULL) && (*BufferSize >= Size)) {
        CopyMem (Buffer, AsciiBuffer, Size);
      } else {
        Status = EFI_BUFFER_TOO_SMALL;
      }

      *BufferSize = Size;
    }

    SHELL_FREE_NON_NULL (AsciiBuffer);
  } else if (DataType == DataTypeUnicode) {
    //
    // unicode text
    //
    if (*Data == L'L') {
      Data = Data + 1;
    }

    Size = StrSize (Data) - sizeof (CHAR16);
    if ((Buffer != NULL) && (*BufferSize >= Size)) {
      CopyMem (Buffer, Data, Size);
    } else {
      Status = EFI_BUFFER_TOO_SMALL;
    }

    *BufferSize = Size;
  } else if (DataType == DataTypeDevicePath) {
    if (*Data == L'P') {
      Data = Data + 1;
    } else if (StrnCmp (Data, L"--", 2) == 0) {
      Data = Data + 2;
    }

    DevPath = ConvertTextToDevicePath (Data);
    if (DevPath == NULL) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SETVAR_ERROR_DPFT), gShellDebug1HiiHandle, L"setvar");
      Status = EFI_INVALID_PARAMETER;
    } else {
      Size = GetDevicePathSize (DevPath);
      if ((Buffer != NULL) && (*BufferSize >= Size)) {
        CopyMem (Buffer, DevPath, Size);
      } else {
        Status = EFI_BUFFER_TOO_SMALL;
      }

      *BufferSize = Size;
    }

    SHELL_FREE_NON_NULL (DevPath);
  } else {
    Status = EFI_INVALID_PARAMETER;
  }

  return Status;
}

/**
  Function to get each data from parameters.

  @param[in]    Package               The package of checked values.
  @param[out]   Buffer                A pointer to a buffer to hold the return data.
  @param[out]   BufferSize            Indicates the size of data in bytes return in Buffer.

  @retval   EFI_INVALID_PARAMETER     Buffer or BufferSize is NULL.
  @retval   EFI_OUT_OF_RESOURCES      A memory allcation failed.
  @retval   EFI_SUCCESS               Get each parameter data was successful.
**/
EFI_STATUS
GetVariableDataFromParameter (
  IN CONST LIST_ENTRY  *Package,
  OUT UINT8            **Buffer,
  OUT UINTN            *BufferSize
  )
{
  CONST CHAR16  *TempData;
  UINTN         Index;
  UINTN         TotalSize;
  UINTN         Size;
  UINT8         *BufferWalker;
  EFI_STATUS    Status;

  TotalSize = 0;
  Size      = 0;
  Status    = EFI_SUCCESS;

  if ((BufferSize == NULL) || (Buffer == NULL) || (ShellCommandLineGetCount (Package) < 3)) {
    return EFI_INVALID_PARAMETER;
  }

  for (Index = 2; Index < ShellCommandLineGetCount (Package); Index++) {
    TempData = ShellCommandLineGetRawValue (Package, Index);
    ASSERT (TempData != NULL);

    if (TempData[0] != L'=') {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_INV), gShellDebug1HiiHandle, L"setvar", TempData);
      return EFI_INVALID_PARAMETER;
    }

    TempData = TempData + 1;
    Size     = 0;
    Status   = ParseParameterData (TempData, NULL, &Size);
    if (EFI_ERROR (Status)) {
      if (Status == EFI_BUFFER_TOO_SMALL) {
        //
        // We expect return EFI_BUFFER_TOO_SMALL when pass 'NULL' as second parameter to the function ParseParameterData.
        //
        TotalSize += Size;
      } else {
        if (Status == EFI_INVALID_PARAMETER) {
          ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_INV), gShellDebug1HiiHandle, L"setvar", TempData);
        } else if (Status == EFI_NOT_FOUND) {
          ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SETVAR_ERROR_DPFT), gShellDebug1HiiHandle, L"setvar");
        }

        return Status;
      }
    }
  }

  *BufferSize = TotalSize;
  *Buffer     = AllocateZeroPool (TotalSize);

  if (*Buffer == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
  } else {
    BufferWalker = *Buffer;
    for (Index = 2; Index < ShellCommandLineGetCount (Package); Index++) {
      TempData = ShellCommandLineGetRawValue (Package, Index);
      TempData = TempData + 1;

      Size   = TotalSize;
      Status = ParseParameterData (TempData, (VOID *)BufferWalker, &Size);
      if (!EFI_ERROR (Status)) {
        BufferWalker = BufferWalker + Size;
        TotalSize    = TotalSize - Size;
      } else {
        return Status;
      }
    }
  }

  return EFI_SUCCESS;
}

/**
  Function for 'setvar' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunSetVar (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS     Status;
  RETURN_STATUS  RStatus;
  LIST_ENTRY     *Package;
  CHAR16         *ProblemParam;
  SHELL_STATUS   ShellStatus;
  CONST CHAR16   *VariableName;
  EFI_GUID       Guid;
  CONST CHAR16   *StringGuid;
  UINT32         Attributes;
  VOID           *Buffer;
  UINTN          Size;
  UINTN          LoopVar;

  ShellStatus = SHELL_SUCCESS;
  Status      = EFI_SUCCESS;
  Buffer      = NULL;
  Size        = 0;
  Attributes  = 0;

  //
  // initialize the shell lib (we must be in non-auto-init...)
  //
  Status = ShellInitialize ();
  ASSERT_EFI_ERROR (Status);

  Status = CommandInit ();
  ASSERT_EFI_ERROR (Status);

  //
  // parse the command line
  //
  Status = ShellCommandLineParse (ParamList, &Package, &ProblemParam, TRUE);
  if (EFI_ERROR (Status)) {
    if ((Status == EFI_VOLUME_CORRUPTED) && (ProblemParam != NULL)) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellDebug1HiiHandle, L"setvar", ProblemParam);
      FreePool (ProblemParam);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      ASSERT (FALSE);
    }
  } else if (ShellCommandLineCheckDuplicate (Package, &ProblemParam) != EFI_SUCCESS) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_DUPLICATE), gShellDebug1HiiHandle, L"setvar", ProblemParam);
    FreePool (ProblemParam);
    ShellStatus = SHELL_INVALID_PARAMETER;
  } else {
    if (ShellCommandLineGetCount (Package) < 2) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_FEW), gShellDebug1HiiHandle, L"setvar");
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      VariableName = ShellCommandLineGetRawValue (Package, 1);
      if (!ShellCommandLineGetFlag (Package, L"-guid")) {
        CopyGuid (&Guid, &gEfiGlobalVariableGuid);
      } else {
        StringGuid = ShellCommandLineGetValue (Package, L"-guid");
        RStatus    = StrToGuid (StringGuid, &Guid);
        if (RETURN_ERROR (RStatus) || (StringGuid[GUID_STRING_LENGTH] != L'\0')) {
          ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_INV), gShellDebug1HiiHandle, L"setvar", StringGuid);
          ShellStatus = SHELL_INVALID_PARAMETER;
        }
      }

      if (ShellCommandLineGetCount (Package) == 2) {
        //
        // Display
        //
        Status = gRT->GetVariable ((CHAR16 *)VariableName, &Guid, &Attributes, &Size, Buffer);
        if (Status == EFI_BUFFER_TOO_SMALL) {
          Buffer = AllocateZeroPool (Size);
          Status = gRT->GetVariable ((CHAR16 *)VariableName, &Guid, &Attributes, &Size, Buffer);
        }

        if (!EFI_ERROR (Status) && (Buffer != NULL)) {
          ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SETVAR_PRINT), gShellDebug1HiiHandle, &Guid, VariableName, Size);
          for (LoopVar = 0; LoopVar < Size; LoopVar++) {
            ShellPrintEx (-1, -1, L"%02x ", ((UINT8 *)Buffer)[LoopVar]);
          }

          ShellPrintEx (-1, -1, L"\r\n");
        } else {
          ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SETVAR_ERROR_GET), gShellDebug1HiiHandle, L"setvar", &Guid, VariableName);
          ShellStatus = SHELL_ACCESS_DENIED;
        }
      } else {
        //
        // Create, Delete or Modify.
        //
        Status = gRT->GetVariable ((CHAR16 *)VariableName, &Guid, &Attributes, &Size, Buffer);
        if (Status == EFI_BUFFER_TOO_SMALL) {
          Buffer = AllocateZeroPool (Size);
          Status = gRT->GetVariable ((CHAR16 *)VariableName, &Guid, &Attributes, &Size, Buffer);
        }

        if (EFI_ERROR (Status) || (Buffer == NULL)) {
          //
          // Creating a new variable.  determine attributes from command line.
          //
          Attributes = 0;
          if (ShellCommandLineGetFlag (Package, L"-bs")) {
            Attributes |= EFI_VARIABLE_BOOTSERVICE_ACCESS;
          }

          if (ShellCommandLineGetFlag (Package, L"-rt")) {
            Attributes |= EFI_VARIABLE_RUNTIME_ACCESS |
                          EFI_VARIABLE_BOOTSERVICE_ACCESS;
          }

          if (ShellCommandLineGetFlag (Package, L"-nv")) {
            Attributes |= EFI_VARIABLE_NON_VOLATILE;
          }
        }

        SHELL_FREE_NON_NULL (Buffer);

        Size   = 0;
        Status = GetVariableDataFromParameter (Package, (UINT8 **)&Buffer, &Size);
        if (!EFI_ERROR (Status)) {
          Status = gRT->SetVariable ((CHAR16 *)VariableName, &Guid, Attributes, Size, Buffer);
        }

        if (EFI_ERROR (Status)) {
          ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SETVAR_ERROR_SET), gShellDebug1HiiHandle, L"setvar", &Guid, VariableName);
          ShellStatus = SHELL_ACCESS_DENIED;
        } else {
          ASSERT (ShellStatus == SHELL_SUCCESS);
        }
      }
    }

    ShellCommandLineFreeVarList (Package);
  }

  if (Buffer != NULL) {
    FreePool (Buffer);
  }

  return (ShellStatus);
}
