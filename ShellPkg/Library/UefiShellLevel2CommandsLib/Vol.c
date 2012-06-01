/** @file
  Main file for vol shell level 2 function.

  Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "UefiShellLevel2CommandsLib.h"
#include <Guid/FileSystemInfo.h>
#include <Guid/FileSystemVolumeLabelInfo.h>

/**
  Print the info or change the volume info.

  @param[in] Path           String with starting path.
  @param[in] Delete         TRUE to delete the volume label. FALSE otherwise.
  @param[in] Name           New name to set to the volume label.

  @retval SHELL_SUCCESS     The operation was sucessful.
**/
SHELL_STATUS
EFIAPI
HandleVol(
  IN CONST CHAR16  *Path,
  IN CONST BOOLEAN Delete,
  IN CONST CHAR16  *Name OPTIONAL
  )
{
  EFI_STATUS            Status;
  SHELL_STATUS          ShellStatus;
  EFI_FILE_SYSTEM_INFO  *SysInfo;
  UINTN                 SysInfoSize;
  SHELL_FILE_HANDLE     ShellFileHandle;
  EFI_FILE_PROTOCOL     *EfiFpHandle;
  UINTN                 Size1;
  UINTN                 Size2;

  ShellStatus   = SHELL_SUCCESS;

  if (
      Name != NULL && (
      StrStr(Name, L"%") != NULL ||
      StrStr(Name, L"^") != NULL ||
      StrStr(Name, L"*") != NULL ||
      StrStr(Name, L"+") != NULL ||
      StrStr(Name, L"=") != NULL ||
      StrStr(Name, L"[") != NULL ||
      StrStr(Name, L"]") != NULL ||
      StrStr(Name, L"|") != NULL ||
      StrStr(Name, L":") != NULL ||
      StrStr(Name, L";") != NULL ||
      StrStr(Name, L"\"") != NULL ||
      StrStr(Name, L"<") != NULL ||
      StrStr(Name, L">") != NULL ||
      StrStr(Name, L"?") != NULL ||
      StrStr(Name, L"/") != NULL ||
      StrStr(Name, L" ") != NULL )
      ){
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellLevel2HiiHandle, Name);
    return (SHELL_INVALID_PARAMETER);
  }

  Status = gEfiShellProtocol->OpenFileByName(
    Path,
    &ShellFileHandle,
    Name != NULL?EFI_FILE_MODE_READ|EFI_FILE_MODE_WRITE:EFI_FILE_MODE_READ);

  if (EFI_ERROR(Status) || ShellFileHandle == NULL) {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_FILE_OPEN_FAIL), gShellLevel2HiiHandle, Path);
    return (SHELL_ACCESS_DENIED);
  }

  //
  // Get the Volume Info from ShellFileHandle
  //
  SysInfo     = NULL;
  SysInfoSize = 0;
  EfiFpHandle = ConvertShellHandleToEfiFileProtocol(ShellFileHandle);
  Status = EfiFpHandle->GetInfo(
    EfiFpHandle,
    &gEfiFileSystemInfoGuid,
    &SysInfoSize,
    SysInfo);

  if (Status == EFI_BUFFER_TOO_SMALL) {
    SysInfo = AllocateZeroPool(SysInfoSize);
    Status = EfiFpHandle->GetInfo(
      EfiFpHandle,
      &gEfiFileSystemInfoGuid,
      &SysInfoSize,
      SysInfo);
  }

  ASSERT(SysInfo != NULL);

  if (Delete) {
    StrCpy ((CHAR16 *) SysInfo->VolumeLabel, L"");
    SysInfo->Size = SIZE_OF_EFI_FILE_SYSTEM_INFO + StrSize(SysInfo->VolumeLabel);
    Status = EfiFpHandle->SetInfo(
      EfiFpHandle,
      &gEfiFileSystemInfoGuid,
      (UINTN)SysInfo->Size,
      SysInfo);
  } else if (Name != NULL) {
    Size1 = StrSize(Name);
    Size2 = StrSize(SysInfo->VolumeLabel);
    if (Size1 > Size2) {
      SysInfo = ReallocatePool((UINTN)SysInfo->Size, (UINTN)SysInfo->Size + Size1 - Size2, SysInfo);
      if (SysInfo == NULL) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_OUT_MEM), gShellLevel2HiiHandle);
        ShellStatus = SHELL_OUT_OF_RESOURCES;
      } 
    }
    if (SysInfo != NULL) {
      StrCpy ((CHAR16 *) SysInfo->VolumeLabel, Name);
      SysInfo->Size = SIZE_OF_EFI_FILE_SYSTEM_INFO + Size1;
      Status = EfiFpHandle->SetInfo(
        EfiFpHandle,
        &gEfiFileSystemInfoGuid,
        (UINTN)SysInfo->Size,
        SysInfo);
    }
  }  

  FreePool(SysInfo);

  if (Delete || Name != NULL) {
    if (EFI_ERROR(Status)) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_FILE_AD), gShellLevel2HiiHandle, Path);
      ShellStatus = SHELL_ACCESS_DENIED;
    }
  }

  SysInfoSize = 0;
  SysInfo = NULL;

  Status = EfiFpHandle->GetInfo(
    EfiFpHandle,
    &gEfiFileSystemInfoGuid,
    &SysInfoSize,
    SysInfo);

  if (Status == EFI_BUFFER_TOO_SMALL) {
    SysInfo = AllocateZeroPool(SysInfoSize);
    Status = EfiFpHandle->GetInfo(
      EfiFpHandle,
      &gEfiFileSystemInfoGuid,
      &SysInfoSize,
      SysInfo);
  }

  gEfiShellProtocol->CloseFile(ShellFileHandle);
  
  ASSERT(SysInfo != NULL);

  if (SysInfo != NULL) {
    //
    // print VolumeInfo table
    //
    ShellPrintHiiEx (
      0,
      gST->ConOut->Mode->CursorRow,
      NULL,
      STRING_TOKEN (STR_VOL_VOLINFO),
      gShellLevel2HiiHandle,
      SysInfo->VolumeLabel,
      SysInfo->ReadOnly?L"r":L"rw",
      SysInfo->VolumeSize,
      SysInfo->FreeSpace,
      SysInfo->BlockSize
     );
    SHELL_FREE_NON_NULL(SysInfo);
  }

  return (ShellStatus);
}

STATIC CONST SHELL_PARAM_ITEM ParamList[] = {
  {L"-d", TypeFlag},
  {L"-n", TypeValue},
  {NULL, TypeMax}
  };

/**
  Function for 'Vol' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunVol (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS    Status;
  LIST_ENTRY    *Package;
  CHAR16        *ProblemParam;
  SHELL_STATUS  ShellStatus;
  CONST CHAR16  *PathName;
  CONST CHAR16  *CurDir;
  BOOLEAN       DeleteMode;
  CHAR16        *FullPath;
  CHAR16        *TempSpot;
  UINTN         Length;
  CONST CHAR16  *NewName;

  Length              = 0;
  ProblemParam        = NULL;
  ShellStatus         = SHELL_SUCCESS;
  PathName            = NULL;
  CurDir              = NULL;
  FullPath            = NULL;

  //
  // initialize the shell lib (we must be in non-auto-init...)
  //
  Status = ShellInitialize();
  ASSERT_EFI_ERROR(Status);

  //
  // Fix local copies of the protocol pointers
  //
  Status = CommandInit();
  ASSERT_EFI_ERROR(Status);

  //
  // parse the command line
  //
  Status = ShellCommandLineParse (ParamList, &Package, &ProblemParam, TRUE);
  if (EFI_ERROR(Status)) {
    if (Status == EFI_VOLUME_CORRUPTED && ProblemParam != NULL) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellLevel2HiiHandle, ProblemParam);
      FreePool(ProblemParam);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      ASSERT(FALSE);
    }
  } else {
    //
    // check for "-?"
    //
    if (ShellCommandLineGetFlag(Package, L"-?")) {
      ASSERT(FALSE);
    }

    if (ShellCommandLineGetCount(Package) > 2) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellLevel2HiiHandle);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      PathName = ShellCommandLineGetRawValue(Package, 1);
      if (PathName == NULL) {
        CurDir = gEfiShellProtocol->GetCurDir(NULL);
        if (CurDir == NULL) {
          ShellStatus = SHELL_NOT_FOUND;
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_NO_CWD), gShellLevel2HiiHandle);
        } else {
          PathName = CurDir;
        }
      }
      if (PathName != NULL) {
        TempSpot = StrStr(PathName, L":");
        if (TempSpot != NULL) {
          *TempSpot = CHAR_NULL;
        }
        TempSpot = StrStr(PathName, L"\\");
        if (TempSpot != NULL) {
          *TempSpot = CHAR_NULL;
        }
        StrnCatGrow(&FullPath, &Length, PathName, 0);
        StrnCatGrow(&FullPath, &Length, L":\\", 0);
        DeleteMode = ShellCommandLineGetFlag(Package, L"-d");
        NewName    = ShellCommandLineGetValue(Package, L"-n");
        if (DeleteMode && ShellCommandLineGetFlag(Package, L"-n")) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_CON), gShellLevel2HiiHandle);
          ShellStatus = SHELL_INVALID_PARAMETER;
        } else if (ShellCommandLineGetFlag(Package, L"-n") && NewName == NULL) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_NO_VALUE), gShellLevel2HiiHandle, L"-n");
          ShellStatus = SHELL_INVALID_PARAMETER;
        } else if (NewName != NULL && StrLen(NewName) > 11) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM_VAL), gShellLevel2HiiHandle, L"-n");
          ShellStatus = SHELL_INVALID_PARAMETER;
        } else if (ShellStatus == SHELL_SUCCESS) {
          ShellStatus = HandleVol(
            FullPath,
            DeleteMode,
            NewName
           );
        }
      }
    }
  }

  SHELL_FREE_NON_NULL(FullPath);

  //
  // free the command line package
  //
  ShellCommandLineFreeVarList (Package);

  return (ShellStatus);
}
