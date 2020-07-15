/** @file
  Main file for attrib shell level 2 function.

  (C) Copyright 2016 Hewlett Packard Enterprise Development LP<BR>
  (C) Copyright 2015 Hewlett-Packard Development Company, L.P.<BR>
  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2018, Dell Technologies. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UefiShellLevel2CommandsLib.h"

/**
  Function will replace drive identifier with CWD.

  If FullPath begining with ':' is invalid path, then ASSERT.
  If FullPath not include dirve identifier , then do nothing.
  If FullPath likes "fs0:\xx" or "fs0:/xx" , then do nothing.
  If FullPath likes "fs0:xxx" or "fs0:", the drive replaced by CWD.

  @param[in, out]   FullPath    The pointer to the string containing the path.
  @param[in]        Cwd         Current directory.

  @retval   EFI_SUCCESS         Success.
  @retval   EFI_OUT_OF_SOURCES  A memory allocation failed.
**/
EFI_STATUS
ReplaceDriveWithCwd (
  IN OUT    CHAR16  **FullPath,
  IN CONST  CHAR16  *Cwd
  )
{
  CHAR16        *Splitter;
  CHAR16        *TempBuffer;
  UINTN         TotalSize;

  Splitter   = NULL;
  TempBuffer = NULL;
  TotalSize  = 0;

  if (FullPath == NULL || *FullPath == NULL) {
    return EFI_SUCCESS;
  }

  Splitter = StrStr (*FullPath, L":");
  ASSERT(Splitter != *FullPath);

  if (Splitter != NULL && *(Splitter + 1) != L'\\' && *(Splitter + 1) != L'/') {
    TotalSize = StrSize (Cwd) + StrSize (Splitter + 1);
    TempBuffer = AllocateZeroPool (TotalSize);
    if (TempBuffer == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    StrCpyS (TempBuffer, TotalSize / sizeof(CHAR16), Cwd);
    StrCatS (TempBuffer, TotalSize / sizeof(CHAR16), L"\\");
    StrCatS (TempBuffer, TotalSize / sizeof(CHAR16), Splitter + 1);

    FreePool(*FullPath);
    *FullPath = TempBuffer;
  }

  return EFI_SUCCESS;
}

/**
  function to determine if FullPath is under current filesystem.

  @param[in]    FullPath    The target location to determine.
  @param[in]    Cwd         Current directory.

  @retval       TRUE        The FullPath is in the current filesystem.
  @retval       FALSE       The FullPaht isn't in the current filesystem.
**/
BOOLEAN
IsCurrentFileSystem (
  IN CONST CHAR16   *FullPath,
  IN CONST CHAR16   *Cwd
  )
{
  CHAR16 *Splitter1;
  CHAR16 *Splitter2;

  Splitter1 = NULL;
  Splitter2 = NULL;

  ASSERT(FullPath != NULL);

  Splitter1 = StrStr (FullPath, L":");
  if (Splitter1 == NULL) {
    return TRUE;
  }

  Splitter2 = StrStr (Cwd, L":");

  if (((UINTN) Splitter1 - (UINTN) FullPath) != ((UINTN) Splitter2 - (UINTN) Cwd)) {
    return FALSE;
  } else {
    if (StrniCmp (FullPath, Cwd, ((UINTN) Splitter1 - (UINTN) FullPath) / sizeof (CHAR16)) == 0) {
      return TRUE;
    } else {
      return FALSE;
    }
  }
}

/**
  Extract drive string and path string from FullPath.

  The caller must be free Drive and Path.

  @param[in]    FullPath    A path to be extracted.
  @param[out]   Drive       Buffer to save drive identifier.
  @param[out]   Path        Buffer to save path.

  @retval       EFI_SUCCESS           Success.
  @retval       EFI_OUT_OF_RESOUCES   A memory allocation failed.
**/
EFI_STATUS
ExtractDriveAndPath (
  IN CONST CHAR16   *FullPath,
  OUT CHAR16        **Drive,
  OUT CHAR16        **Path
  )
{
  CHAR16 *Splitter;

  ASSERT (FullPath != NULL);

  Splitter = StrStr (FullPath, L":");

  if (Splitter == NULL) {
    *Drive = NULL;
    *Path = AllocateCopyPool (StrSize (FullPath), FullPath);
    if (*Path == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
  } else {
    if (*(Splitter + 1) == CHAR_NULL) {
      *Drive = AllocateCopyPool (StrSize (FullPath), FullPath);
      *Path = NULL;
      if (*Drive == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }
    } else {
      *Drive = AllocateCopyPool ((Splitter - FullPath + 2) * sizeof(CHAR16), FullPath);
      if (*Drive == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }
      (*Drive)[Splitter - FullPath + 1] = CHAR_NULL;

      *Path = AllocateCopyPool (StrSize (Splitter + 1), Splitter + 1);
      if (*Path == NULL) {
        FreePool (*Drive);
        return EFI_OUT_OF_RESOURCES;
      }
    }
  }

  return EFI_SUCCESS;
}

/**
  Function for 'cd' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunCd (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS        Status;
  LIST_ENTRY        *Package;
  CONST CHAR16      *Cwd;
  CHAR16            *Path;
  CHAR16            *Drive;
  CHAR16            *ProblemParam;
  SHELL_STATUS      ShellStatus;
  CONST CHAR16      *Param1;
  CHAR16            *Param1Copy;
  CHAR16            *Walker;
  CHAR16            *Splitter;
  CHAR16            *TempBuffer;
  UINTN             TotalSize;

  ProblemParam  = NULL;
  ShellStatus   = SHELL_SUCCESS;
  Cwd           = NULL;
  Path          = NULL;
  Drive         = NULL;
  Splitter      = NULL;
  TempBuffer    = NULL;
  TotalSize     = 0;

  Status = CommandInit();
  ASSERT_EFI_ERROR(Status);

  //
  // initialize the shell lib (we must be in non-auto-init...)
  //
  Status = ShellInitialize();
  ASSERT_EFI_ERROR(Status);

  //
  // parse the command line
  //
  Status = ShellCommandLineParse (EmptyParamList, &Package, &ProblemParam, TRUE);
  if (EFI_ERROR(Status)) {
    if (Status == EFI_VOLUME_CORRUPTED && ProblemParam != NULL) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellLevel2HiiHandle, L"cd", ProblemParam);
      FreePool(ProblemParam);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      ASSERT(FALSE);
    }
  }

  //
  // check for "-?"
  //
  if (ShellCommandLineGetFlag(Package, L"-?")) {
    ASSERT(FALSE);
  } else if (ShellCommandLineGetRawValue(Package, 2) != NULL) {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellLevel2HiiHandle, L"cd");
    ShellStatus = SHELL_INVALID_PARAMETER;
  } else {
    //
    // remember that param 0 is the command name
    // If there are 0 value parameters, then print the current directory
    // else If there are 2 value parameters, then print the error message
    // else If there is  1 value paramerer , then change the directory
    //
    Cwd = ShellGetCurrentDir (NULL);
    if (Cwd == NULL) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN(STR_GEN_NO_CWD), gShellLevel2HiiHandle, L"cd");
      ShellStatus = SHELL_NOT_FOUND;
    } else {
      Param1 = ShellCommandLineGetRawValue (Package, 1);
      if (Param1 == NULL) {
        //
        // display the current directory
        //
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN(STR_CD_PRINT), gShellLevel2HiiHandle, Cwd);
      } else {
        Param1Copy = CatSPrint (NULL, L"%s", Param1, NULL);
        for (Walker = Param1Copy; Walker != NULL && *Walker != CHAR_NULL; Walker++) {
          if (*Walker == L'\"') {
            CopyMem (Walker, Walker + 1, StrSize(Walker) - sizeof(Walker[0]));
          }
        }

        if (Param1Copy != NULL && IsCurrentFileSystem (Param1Copy, Cwd)) {
          Status = ReplaceDriveWithCwd (&Param1Copy,Cwd);
        } else {
          //
          // Can't use cd command to change filesystem.
          //
          ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_CD_NF), gShellLevel2HiiHandle, L"cd");
          Status = EFI_NOT_FOUND;
        }

        if (!EFI_ERROR(Status) && Param1Copy != NULL) {
          Splitter = StrStr (Cwd, L":");
          if (Param1Copy[0] == L'\\') {
            //
            // Absolute Path on current drive letter.
            //
            TotalSize = ((Splitter - Cwd + 1) * sizeof(CHAR16)) + StrSize(Param1Copy);
            TempBuffer = AllocateZeroPool (TotalSize);
            if (TempBuffer == NULL) {
              Status = EFI_OUT_OF_RESOURCES;
            } else {
              StrnCpyS (TempBuffer, TotalSize / sizeof(CHAR16), Cwd, (Splitter - Cwd + 1));
              StrCatS (TempBuffer, TotalSize / sizeof(CHAR16), Param1Copy);

              FreePool (Param1Copy);
              Param1Copy = TempBuffer;
              TempBuffer = NULL;
            }
          } else {
            if (StrStr (Param1Copy,L":") == NULL) {
              TotalSize = StrSize (Cwd) + StrSize (Param1Copy);
              TempBuffer = AllocateZeroPool (TotalSize);
              if (TempBuffer == NULL) {
                Status = EFI_OUT_OF_RESOURCES;
              } else {
                StrCpyS (TempBuffer, TotalSize / sizeof (CHAR16), Cwd);
                StrCatS (TempBuffer, TotalSize / sizeof (CHAR16), L"\\");
                StrCatS (TempBuffer, TotalSize / sizeof (CHAR16), Param1Copy);

                FreePool (Param1Copy);
                Param1Copy = TempBuffer;
                TempBuffer = NULL;
              }
            }
          }
        }

        if (!EFI_ERROR(Status)) {
          Param1Copy = PathCleanUpDirectories (Param1Copy);
          Status = ExtractDriveAndPath (Param1Copy, &Drive, &Path);
        }

        if (!EFI_ERROR (Status) && Drive != NULL && Path != NULL) {
          if (EFI_ERROR(ShellIsDirectory (Param1Copy))) {
            ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN(STR_GEN_NOT_DIR), gShellLevel2HiiHandle, L"cd", Param1Copy);
            ShellStatus = SHELL_NOT_FOUND;
          } else {
            Status = gEfiShellProtocol->SetCurDir (Drive, Path + 1);
            if (EFI_ERROR (Status)) {
              ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN(STR_GEN_DIR_NF), gShellLevel2HiiHandle, L"cd", Param1Copy);
              ShellStatus = SHELL_NOT_FOUND;
            }
          }
        }

        if (Drive != NULL) {
          FreePool (Drive);
        }

        if (Path != NULL) {
          FreePool (Path);
        }

        FreePool (Param1Copy);
      }
    }
  }

  //
  // free the command line package
  //
  ShellCommandLineFreeVarList (Package);

  //
  // return the status
  //
  return (ShellStatus);
}

