/** @file
  Main file for SetVar shell Debug1 function.

  (C) Copyright 2015 Hewlett-Packard Development Company, L.P.<BR>
  Copyright (c) 2010 - 2014, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "UefiShellDebug1CommandsLib.h"

STATIC CONST SHELL_PARAM_ITEM ParamList[] = {
  {L"-guid", TypeValue},
  {L"-bs", TypeFlag},
  {L"-rt", TypeFlag},
  {L"-nv", TypeFlag},
  {NULL, TypeMax}
  };


/**
  Check if the input is a (potentially empty) string of hexadecimal nibbles.

  @param[in] String  The CHAR16 string to check.

  @retval FALSE  A character has been found in String for which
                 ShellIsHexaDecimalDigitCharacter() returned FALSE.

  @retval TRUE   Otherwise. (Note that this covers the case when String is
                 empty.)
**/
BOOLEAN
EFIAPI
IsStringOfHexNibbles (
  IN CONST CHAR16  *String
  )
{
  CONST CHAR16 *Pos;

  for (Pos = String; *Pos != L'\0'; ++Pos) {
    if (!ShellIsHexaDecimalDigitCharacter (*Pos)) {
      return FALSE;
    }
  }
  return TRUE;
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
  EFI_STATUS          Status;
  LIST_ENTRY          *Package;
  CHAR16              *ProblemParam;
  SHELL_STATUS        ShellStatus;
  CONST CHAR16        *VariableName;
  CONST CHAR16        *Data;
  EFI_GUID            Guid;
  CONST CHAR16        *StringGuid;
  UINT32              Attributes;
  VOID                *Buffer;
  UINTN               Size;
  UINTN               LoopVar;
  EFI_DEVICE_PATH_PROTOCOL           *DevPath;

  ShellStatus         = SHELL_SUCCESS;
  Status              = EFI_SUCCESS;
  Buffer              = NULL;
  Size                = 0;
  Attributes          = 0;
  DevPath             = NULL;

  //
  // initialize the shell lib (we must be in non-auto-init...)
  //
  Status = ShellInitialize();
  ASSERT_EFI_ERROR(Status);

  Status = CommandInit();
  ASSERT_EFI_ERROR(Status);

  //
  // parse the command line
  //
  Status = ShellCommandLineParse (ParamList, &Package, &ProblemParam, TRUE);
  if (EFI_ERROR(Status)) {
    if (Status == EFI_VOLUME_CORRUPTED && ProblemParam != NULL) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellDebug1HiiHandle, L"setvar", ProblemParam);  
      FreePool(ProblemParam);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      ASSERT(FALSE);
    }
  } else {
    if (ShellCommandLineGetCount(Package) < 2) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_FEW), gShellDebug1HiiHandle, L"setvar");  
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else if (ShellCommandLineGetCount(Package) > 3) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellDebug1HiiHandle, L"setvar");  
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      VariableName  = ShellCommandLineGetRawValue(Package, 1);
      Data          = ShellCommandLineGetRawValue(Package, 2);
      if (!ShellCommandLineGetFlag(Package, L"-guid")){
        CopyGuid(&Guid, &gEfiGlobalVariableGuid);
      } else {
        StringGuid = ShellCommandLineGetValue(Package, L"-guid");
        Status = ConvertStringToGuid(StringGuid, &Guid);
        if (EFI_ERROR(Status)) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_INV), gShellDebug1HiiHandle, L"setvar", StringGuid);  
          ShellStatus = SHELL_INVALID_PARAMETER;
        }
      }
      if (Data == NULL || Data[0] !=  L'=') {
        //
        // Display what's there
        //
        Status = gRT->GetVariable((CHAR16*)VariableName, &Guid, &Attributes, &Size, Buffer);
        if (Status == EFI_BUFFER_TOO_SMALL) {
          Buffer = AllocateZeroPool(Size);
          Status = gRT->GetVariable((CHAR16*)VariableName, &Guid, &Attributes, &Size, Buffer);
        }
        if (!EFI_ERROR(Status)&& Buffer != NULL) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_SETVAR_PRINT), gShellDebug1HiiHandle, &Guid, VariableName, Size);
          for (LoopVar = 0 ; LoopVar < Size ; LoopVar++) {
            ShellPrintEx(-1, -1, L"%02x ", ((UINT8*)Buffer)[LoopVar]);
          }
          ShellPrintEx(-1, -1, L"\r\n");
        } else {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_SETVAR_ERROR_GET), gShellDebug1HiiHandle, L"setvar", &Guid, VariableName);  
          ShellStatus = SHELL_ACCESS_DENIED;
        }
      } else if (StrCmp(Data, L"=") == 0) {
        //
        // Delete what's there!
        //
        Status = gRT->SetVariable((CHAR16*)VariableName, &Guid, Attributes, 0, NULL);
        if (EFI_ERROR(Status)) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_SETVAR_ERROR_SET), gShellDebug1HiiHandle, L"setvar", &Guid, VariableName);  
          ShellStatus = SHELL_ACCESS_DENIED;
        } else {
          ASSERT(ShellStatus == SHELL_SUCCESS);
        }
      } else {
        //
        // Change what's there or create a new one.
        //

        ASSERT(Data[0] == L'=');
        Data++;
        ASSERT(Data[0] != L'\0');

        //
        // Determine if the variable exists and get the attributes
        //
        Status = gRT->GetVariable((CHAR16*)VariableName, &Guid, &Attributes, &Size, Buffer);
        if (Status == EFI_BUFFER_TOO_SMALL) {
          Buffer = AllocateZeroPool(Size);
          Status = gRT->GetVariable((CHAR16*)VariableName, &Guid, &Attributes, &Size, Buffer);
        }

        if (EFI_ERROR(Status) || Buffer == NULL) {
          //
          // Creating a new variable.  determine attributes from command line.
          //
          Attributes = 0;
          if (ShellCommandLineGetFlag(Package, L"-bs")) {
            Attributes |= EFI_VARIABLE_BOOTSERVICE_ACCESS;
          }
          if (ShellCommandLineGetFlag(Package, L"-rt")) {
            Attributes |= EFI_VARIABLE_RUNTIME_ACCESS |
                          EFI_VARIABLE_BOOTSERVICE_ACCESS;
          }
          if (ShellCommandLineGetFlag(Package, L"-nv")) {
            Attributes |= EFI_VARIABLE_NON_VOLATILE;
          }
        }
        SHELL_FREE_NON_NULL(Buffer);

        //
        // What type is the new data.
        //
        if (IsStringOfHexNibbles(Data)) {
          if (StrLen(Data) % 2 != 0) {
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_INV), gShellDebug1HiiHandle, L"setvar", Data);  
            ShellStatus = SHELL_INVALID_PARAMETER;
          } else {
            //
            // arbitrary buffer
            //
            Buffer = AllocateZeroPool((StrLen(Data) / 2));
            if (Buffer == NULL) {
              Status = EFI_OUT_OF_RESOURCES;
            } else {
              for (LoopVar = 0 ; LoopVar < (StrLen(Data) / 2) ; LoopVar++) {
                ((UINT8*)Buffer)[LoopVar] = (UINT8)(HexCharToUintn(Data[LoopVar*2]) * 16);
                ((UINT8*)Buffer)[LoopVar] = (UINT8)(((UINT8*)Buffer)[LoopVar] + HexCharToUintn(Data[LoopVar*2+1]));
              }
              Status = gRT->SetVariable((CHAR16*)VariableName, &Guid, Attributes, StrLen(Data) / 2, Buffer);
            }
            if (EFI_ERROR(Status)) {
              ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_SETVAR_ERROR_SET), gShellDebug1HiiHandle, L"setvar", &Guid, VariableName);  
              ShellStatus = SHELL_ACCESS_DENIED;
            } else {
              ASSERT(ShellStatus == SHELL_SUCCESS);
            }
          }
        } else if (StrnCmp(Data, L"\"", 1) == 0) {
          //
          // ascii text
          //
          Data++;
          Buffer = AllocateZeroPool(StrSize(Data) / 2);
          if (Buffer == NULL) {
            Status = EFI_OUT_OF_RESOURCES;
          } else {
            AsciiSPrint(Buffer, StrSize(Data) / 2, "%s", Data);
            ((CHAR8*)Buffer)[AsciiStrLen(Buffer)-1] = CHAR_NULL;
            Status = gRT->SetVariable((CHAR16*)VariableName, &Guid, Attributes, AsciiStrSize(Buffer)-sizeof(CHAR8), Buffer);
          }
          if (EFI_ERROR(Status)) {
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_SETVAR_ERROR_SET), gShellDebug1HiiHandle, L"setvar", &Guid, VariableName);  
            ShellStatus = SHELL_ACCESS_DENIED;
          } else {
            ASSERT(ShellStatus == SHELL_SUCCESS);
          }
        } else if (StrnCmp(Data, L"L\"", 2) == 0) {
          //
          // ucs2 text
          //
          Data++;
          Data++;
          Buffer = AllocateZeroPool(StrSize(Data));
          if (Buffer == NULL) {
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_OUT_MEM), gShellDebug1HiiHandle, L"setvar");  
            ShellStatus = SHELL_OUT_OF_RESOURCES;
          } else {
            UnicodeSPrint(Buffer, StrSize(Data), L"%s", Data);
            ((CHAR16*)Buffer)[StrLen(Buffer)-1] = CHAR_NULL;

            Status = gRT->SetVariable((CHAR16*)VariableName, &Guid, Attributes, StrSize(Buffer)-sizeof(CHAR16), Buffer);
            if (EFI_ERROR(Status)) {
              ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_SETVAR_ERROR_SET), gShellDebug1HiiHandle, L"setvar", &Guid, VariableName);  
              ShellStatus = SHELL_ACCESS_DENIED;
            } else {
              ASSERT(ShellStatus == SHELL_SUCCESS);
            }
          }
        } else if (StrnCmp(Data, L"--", 2) == 0) {
          //
          // device path in text format
          //
          Data++;
          Data++;
          DevPath = ConvertTextToDevicePath(Data);
          if (DevPath == NULL) {
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_SETVAR_ERROR_DPFT), gShellDebug1HiiHandle, L"setvar");  
            ShellStatus = SHELL_INVALID_PARAMETER;
          } else {
            Status = gRT->SetVariable((CHAR16*)VariableName, &Guid, Attributes, GetDevicePathSize(DevPath), DevPath);
            if (EFI_ERROR(Status)) {
              ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_SETVAR_ERROR_SET), gShellDebug1HiiHandle, L"setvar", &Guid, VariableName);  
              ShellStatus = SHELL_ACCESS_DENIED;
            } else {
              ASSERT(ShellStatus == SHELL_SUCCESS);
            }
          }
        } else {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_INV), gShellDebug1HiiHandle, L"setvar", Data);  
          ShellStatus = SHELL_INVALID_PARAMETER;
        }
      }
    }
    ShellCommandLineFreeVarList (Package);
  }

  if (Buffer != NULL) {
    FreePool(Buffer);
  }

  if (DevPath != NULL) {
    FreePool(DevPath);
  }

  return (ShellStatus);
}
