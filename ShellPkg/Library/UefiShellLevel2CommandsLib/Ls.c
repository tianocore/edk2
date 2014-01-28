/** @file
  Main file for ls shell level 2 function.

  Copyright (c) 2013 Hewlett-Packard Development Company, L.P.
  Copyright (c) 2009 - 2014, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "UefiShellLevel2CommandsLib.h"
#include <Guid/FileSystemInfo.h>

/**
  print out the standard format output volume entry.

  @param[in] TheList           a list of files from the volume.
**/
EFI_STATUS
EFIAPI
PrintSfoVolumeInfoTableEntry(
  IN CONST EFI_SHELL_FILE_INFO *TheList
  )
{
  EFI_STATUS            Status;
  EFI_SHELL_FILE_INFO   *Node;
  CHAR16                *DirectoryName;
  EFI_FILE_SYSTEM_INFO  *SysInfo;
  UINTN                 SysInfoSize;
  SHELL_FILE_HANDLE     ShellFileHandle;
  EFI_FILE_PROTOCOL     *EfiFpHandle;

  //
  // Get the first valid handle (directories)
  //
  for ( Node = (EFI_SHELL_FILE_INFO *)GetFirstNode(&TheList->Link)
      ; !IsNull(&TheList->Link, &Node->Link) && Node->Handle == NULL
      ; Node = (EFI_SHELL_FILE_INFO *)GetNextNode(&TheList->Link, &Node->Link)
     );

  if (Node->Handle == NULL) {
    DirectoryName = GetFullyQualifiedPath(((EFI_SHELL_FILE_INFO *)GetFirstNode(&TheList->Link))->FullName);

    //
    // We need to open something up to get system information
    //
    Status = gEfiShellProtocol->OpenFileByName(
      DirectoryName,
      &ShellFileHandle,
      EFI_FILE_MODE_READ
      );

    ASSERT_EFI_ERROR(Status);
    FreePool(DirectoryName);

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
      SysInfo
      );

    if (Status == EFI_BUFFER_TOO_SMALL) {
      SysInfo = AllocateZeroPool(SysInfoSize);
      Status = EfiFpHandle->GetInfo(
        EfiFpHandle,
        &gEfiFileSystemInfoGuid,
        &SysInfoSize,
        SysInfo
        );
    }

    ASSERT_EFI_ERROR(Status);

    gEfiShellProtocol->CloseFile(ShellFileHandle);
  } else {
    //
    // Get the Volume Info from Node->Handle
    //
    SysInfo = NULL;
    SysInfoSize = 0;
    EfiFpHandle = ConvertShellHandleToEfiFileProtocol(Node->Handle);
    Status = EfiFpHandle->GetInfo(
      EfiFpHandle,
      &gEfiFileSystemInfoGuid,
      &SysInfoSize,
      SysInfo
      );

    if (Status == EFI_BUFFER_TOO_SMALL) {
      SysInfo = AllocateZeroPool(SysInfoSize);
      Status = EfiFpHandle->GetInfo(
        EfiFpHandle,
        &gEfiFileSystemInfoGuid,
        &SysInfoSize,
        SysInfo
        );
    }

    ASSERT_EFI_ERROR(Status);
  }

  ShellPrintHiiEx (
    -1,
    -1,
    NULL,
    STRING_TOKEN (STR_GEN_SFO_HEADER),
    gShellLevel2HiiHandle,
    L"ls"
    );
  //
  // print VolumeInfo table
  //
  ASSERT(SysInfo != NULL);
  ShellPrintHiiEx (
    0,
    gST->ConOut->Mode->CursorRow,
    NULL,
    STRING_TOKEN (STR_LS_SFO_VOLINFO),
    gShellLevel2HiiHandle,
    SysInfo->VolumeLabel,
    SysInfo->VolumeSize,
    SysInfo->ReadOnly?L"TRUE":L"FALSE",
    SysInfo->FreeSpace,
    SysInfo->BlockSize
    );

  SHELL_FREE_NON_NULL(SysInfo);

  return (Status);
}

/**
  print out the info on a single file.

  @param[in] Sfo      TRUE if in SFO, false otherwise.
  @param[in] TheNode  the EFI_SHELL_FILE_INFO node to print out information on.
  @param[in] Files    incremented if a file is printed.
  @param[in] Size     incremented by file size.
  @param[in] Dirs     incremented if a directory is printed.

**/
VOID
EFIAPI
PrintFileInformation(
  IN CONST BOOLEAN              Sfo, 
  IN CONST EFI_SHELL_FILE_INFO  *TheNode, 
  IN UINT64                     *Files, 
  IN UINT64                     *Size, 
  IN UINT64                     *Dirs
  )
{
  ASSERT(Files    != NULL);
  ASSERT(Size     != NULL);
  ASSERT(Dirs     != NULL);
  ASSERT(TheNode  != NULL);

  if (Sfo) {
    //
    // Print the FileInfo Table
    //
    ShellPrintHiiEx (
      0,
      gST->ConOut->Mode->CursorRow,
      NULL,
      STRING_TOKEN (STR_LS_SFO_FILEINFO),
      gShellLevel2HiiHandle,
      TheNode->FullName,
      TheNode->Info->FileSize,
      TheNode->Info->PhysicalSize,
      (TheNode->Info->Attribute & EFI_FILE_ARCHIVE)   != 0?L"a":L"",
      (TheNode->Info->Attribute & EFI_FILE_DIRECTORY) != 0?L"d":L"",
      (TheNode->Info->Attribute & EFI_FILE_HIDDEN)    != 0?L"h":L"",
      (TheNode->Info->Attribute & EFI_FILE_READ_ONLY) != 0?L"r":L"",
      (TheNode->Info->Attribute & EFI_FILE_SYSTEM)    != 0?L"s":L"",
      TheNode->Info->CreateTime.Hour,
      TheNode->Info->CreateTime.Minute,
      TheNode->Info->CreateTime.Second,
      TheNode->Info->CreateTime.Day,
      TheNode->Info->CreateTime.Month,
      TheNode->Info->CreateTime.Year,
      TheNode->Info->LastAccessTime.Hour,
      TheNode->Info->LastAccessTime.Minute,
      TheNode->Info->LastAccessTime.Second,
      TheNode->Info->LastAccessTime.Day,
      TheNode->Info->LastAccessTime.Month,
      TheNode->Info->LastAccessTime.Year,
      TheNode->Info->ModificationTime.Hour,
      TheNode->Info->ModificationTime.Minute,
      TheNode->Info->ModificationTime.Second,
      TheNode->Info->ModificationTime.Day,
      TheNode->Info->ModificationTime.Month,
      TheNode->Info->ModificationTime.Year
      );
  } else {
    //
    // print this one out...
    // first print the universal start, next print the type specific name format, last print the CRLF
    //
    ShellPrintHiiEx (
      -1,
      -1,
      NULL,
      STRING_TOKEN (STR_LS_LINE_START_ALL),
      gShellLevel2HiiHandle,
      &TheNode->Info->ModificationTime,
      (TheNode->Info->Attribute & EFI_FILE_DIRECTORY) != 0?L"<DIR>":L"",
      (TheNode->Info->Attribute & EFI_FILE_READ_ONLY) != 0?L'r':L' ',
      TheNode->Info->FileSize
      );
    if (TheNode->Info->Attribute & EFI_FILE_DIRECTORY) {
      (*Dirs)++;
      ShellPrintHiiEx (
        -1,
        -1,
        NULL,
        STRING_TOKEN (STR_LS_LINE_END_DIR),
        gShellLevel2HiiHandle,
        TheNode->FileName
        );
    } else {
      (*Files)++;
      (*Size) += TheNode->Info->FileSize;
      if ( (gUnicodeCollation->StriColl(gUnicodeCollation, (CHAR16*)L".nsh", (CHAR16*)&(TheNode->FileName[StrLen (TheNode->FileName) - 4])) == 0)
        || (gUnicodeCollation->StriColl(gUnicodeCollation, (CHAR16*)L".efi", (CHAR16*)&(TheNode->FileName[StrLen (TheNode->FileName) - 4])) == 0)
       ){
        ShellPrintHiiEx (
          -1,
          -1,
          NULL,
          STRING_TOKEN (STR_LS_LINE_END_EXE),
          gShellLevel2HiiHandle,
          TheNode->FileName
          );
      } else {
        ShellPrintHiiEx (
          -1,
          -1,
          NULL,
          STRING_TOKEN (STR_LS_LINE_END_FILE),
          gShellLevel2HiiHandle,
          TheNode->FileName
          );
      }
    }
  }
}

/**
  print out the header when not using standard format output.

  @param[in] Path           String with starting path.
**/
VOID
EFIAPI
PrintNonSfoHeader(
  IN CONST CHAR16 *Path
  )
{
  CHAR16 *DirectoryName;

  //
  // get directory name from path...
  //
  DirectoryName = GetFullyQualifiedPath(Path);

  if (DirectoryName != NULL) {
    //
    // print header
    //
    ShellPrintHiiEx (
      0,
      gST->ConOut->Mode->CursorRow,
      NULL,
      STRING_TOKEN (STR_LS_HEADER_LINE1),
      gShellLevel2HiiHandle,
      DirectoryName
      );

    SHELL_FREE_NON_NULL(DirectoryName);
  }
}

/**
  print out the footer when not using standard format output.

  @param[in] Files            The number of files.
  @param[in] Size             The size of files in bytes.
  @param[in] Dirs             The number of directories.
**/
VOID
EFIAPI
PrintNonSfoFooter(
  IN UINT64                     Files, 
  IN UINT64                     Size, 
  IN UINT64                     Dirs
  )
{
  //
  // print footer
  //
  ShellPrintHiiEx (
    -1,
    -1,
    NULL,
    STRING_TOKEN (STR_LS_FOOTER_LINE),
    gShellLevel2HiiHandle,
    Files,
    Size,
    Dirs
   );
}

/**
  print out the list of files and directories from the LS command

  @param[in] Rec            TRUE to automatically recurse into each found directory
                            FALSE to only list the specified directory.
  @param[in] Attribs        List of required Attribute for display.
                            If 0 then all non-system and non-hidden files will be printed.
  @param[in] Sfo            TRUE to use Standard Format Output, FALSE otherwise
  @param[in] Path           String with starting path.
  @param[in] First          TRUE for the original and FALSE for any recursion spawned instances.
  @param[in] Count          The count of bits enabled in Attribs.
  @param[in] TimeZone       The current time zone offset.

  @retval SHELL_SUCCESS     the printing was sucessful.
**/
SHELL_STATUS
EFIAPI
PrintLsOutput(
  IN CONST BOOLEAN Rec,
  IN CONST UINT64  Attribs,
  IN CONST BOOLEAN Sfo,
  IN CONST CHAR16  *Path,
  IN CONST BOOLEAN First,
  IN CONST UINTN   Count,
  IN CONST INT16   TimeZone
  )
{
  EFI_STATUS            Status;
  EFI_SHELL_FILE_INFO   *ListHead;
  EFI_SHELL_FILE_INFO   *Node;
  SHELL_STATUS          ShellStatus;
  UINT64                FileCount;
  UINT64                DirCount;
  UINT64                FileSize;
  CHAR16                *DirectoryName;
  UINTN                 LongestPath;
  CHAR16                *CorrectedPath;

  FileCount     = 0;
  DirCount      = 0;
  FileSize      = 0;
  ListHead      = NULL;
  ShellStatus   = SHELL_SUCCESS;
  LongestPath   = 0;
  CorrectedPath = NULL;

  CorrectedPath = StrnCatGrow(&CorrectedPath, NULL, Path, 0);
  if (CorrectedPath == NULL) {
    return (SHELL_OUT_OF_RESOURCES);
  }

  PathCleanUpDirectories(CorrectedPath);

  if (!Sfo) {
    PrintNonSfoHeader(CorrectedPath);
  }

  Status = ShellOpenFileMetaArg((CHAR16*)CorrectedPath, EFI_FILE_MODE_READ, &ListHead);
  if (EFI_ERROR(Status)) {
    SHELL_FREE_NON_NULL(CorrectedPath);
    if(Status == EFI_NOT_FOUND){
      return (SHELL_NOT_FOUND);
    }
    return (SHELL_DEVICE_ERROR);
  }
  if (ListHead == NULL || IsListEmpty(&ListHead->Link)) {
    SHELL_FREE_NON_NULL(CorrectedPath);
    //
    // On the first one only we expect to find something...
    // do we find the . and .. directories otherwise?
    //
    if (First) {
      return (SHELL_NOT_FOUND);
    }
    return (SHELL_SUCCESS);
  }

  if (Sfo && First) {
    PrintSfoVolumeInfoTableEntry(ListHead);
  }

  for ( Node = (EFI_SHELL_FILE_INFO *)GetFirstNode(&ListHead->Link)
      ; !IsNull(&ListHead->Link, &Node->Link)
      ; Node = (EFI_SHELL_FILE_INFO *)GetNextNode(&ListHead->Link, &Node->Link)
      ){
    ASSERT(Node != NULL);
    if (LongestPath < StrSize(Node->FullName)) {
      LongestPath = StrSize(Node->FullName);
    }
    ASSERT(Node->Info != NULL);
    ASSERT((Node->Info->Attribute & EFI_FILE_VALID_ATTR) == Node->Info->Attribute);
    if (Attribs == 0) {
      //
      // NOT system & NOT hidden
      //
      if ( (Node->Info->Attribute & EFI_FILE_SYSTEM)
        || (Node->Info->Attribute & EFI_FILE_HIDDEN)
       ){
        continue;
      }
    } else if ((Attribs != EFI_FILE_VALID_ATTR) ||
               (Count == 5)) {
      //
      // Only matches the bits which "Attribs" contains, not
      // all files/directories with any of the bits.
      // Count == 5 is used to tell the difference between a user
      // specifying all bits (EX: -arhsda) and just specifying
      // -a (means display all files with any attribute).
      //
      if ( (Node->Info->Attribute & Attribs) != Attribs) {
        continue;
      }
    }

    PrintFileInformation(Sfo, Node, &FileCount, &FileSize, &DirCount);
  }

  if (!Sfo) {
    PrintNonSfoFooter(FileCount, FileSize, DirCount);
  }

  if (Rec){
    DirectoryName = AllocateZeroPool(LongestPath + 2*sizeof(CHAR16));
    if (DirectoryName == NULL) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_NO_MEM), gShellLevel2HiiHandle);
      ShellStatus = SHELL_OUT_OF_RESOURCES;
    } else {
      for ( Node = (EFI_SHELL_FILE_INFO *)GetFirstNode(&ListHead->Link)
          ; !IsNull(&ListHead->Link, &Node->Link) && ShellStatus == SHELL_SUCCESS
          ; Node = (EFI_SHELL_FILE_INFO *)GetNextNode(&ListHead->Link, &Node->Link)
         ){
        if (ShellGetExecutionBreakFlag ()) {
          ShellStatus = SHELL_ABORTED;
          break;
        }

        //
        // recurse on any directory except the traversing ones...
        //
        if (((Node->Info->Attribute & EFI_FILE_DIRECTORY) == EFI_FILE_DIRECTORY)
          && StrCmp(Node->FileName, L".") != 0
          && StrCmp(Node->FileName, L"..") != 0
         ){
          StrCpy(DirectoryName, Node->FullName);
          StrCat(DirectoryName, L"\\*");
          ShellStatus = PrintLsOutput(
            Rec,
            Attribs,
            Sfo,
            DirectoryName,
            FALSE,
            Count,
            TimeZone);
        }
      }
      FreePool(DirectoryName);
    }
  }

  FreePool(CorrectedPath);
  ShellCloseFileMetaArg(&ListHead);
  return (ShellStatus);
}

STATIC CONST SHELL_PARAM_ITEM LsParamList[] = {
  {L"-r", TypeFlag},
  {L"-a", TypeStart},
  {L"-sfo", TypeFlag},
  {NULL, TypeMax}
  };

/**
  Function for 'ls' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunLs (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS    Status;
  LIST_ENTRY    *Package;
  CHAR16        *ProblemParam;
  CONST CHAR16  *Attribs;
  SHELL_STATUS  ShellStatus;
  UINT64        RequiredAttributes;
  CONST CHAR16  *PathName;
  CONST CHAR16  *CurDir;
  UINTN         Count;
  CHAR16        *FullPath;
  UINTN         Size;
  EFI_TIME      TheTime;
  BOOLEAN       SfoMode;

  Size                = 0;
  FullPath            = NULL;
  ProblemParam        = NULL;
  Attribs             = NULL;
  ShellStatus         = SHELL_SUCCESS;
  RequiredAttributes  = 0;
  PathName            = NULL;
  CurDir              = NULL;
  Count               = 0;

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
  Status = ShellCommandLineParse (LsParamList, &Package, &ProblemParam, TRUE);
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
      //
      // check for -a
      //
      if (ShellCommandLineGetFlag(Package, L"-a")) {
        for ( Attribs = ShellCommandLineGetValue(Package, L"-a")
            ; Attribs != NULL && *Attribs != CHAR_NULL && ShellStatus == SHELL_SUCCESS
            ; Attribs++
           ){
          switch (*Attribs) {
            case L'a':
            case L'A':
              RequiredAttributes |= EFI_FILE_ARCHIVE;
              Count++;
              continue;
            case L's':
            case L'S':
              RequiredAttributes |= EFI_FILE_SYSTEM;
              Count++;
              continue;
            case L'h':
            case L'H':
              RequiredAttributes |= EFI_FILE_HIDDEN;
              Count++;
              continue;
            case L'r':
            case L'R':
              RequiredAttributes |= EFI_FILE_READ_ONLY;
              Count++;
              continue;
            case L'd':
            case L'D':
              RequiredAttributes |= EFI_FILE_DIRECTORY;
              Count++;
              continue;
            default:
              ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_ATTRIBUTE), gShellLevel2HiiHandle, ShellCommandLineGetValue(Package, L"-a"));
              ShellStatus = SHELL_INVALID_PARAMETER;
              break;
          } // switch
        } // for loop
        //
        // if nothing is specified all are specified
        //
        if (RequiredAttributes == 0) {
          RequiredAttributes = EFI_FILE_VALID_ATTR;
        }
      } // if -a present
      if (ShellStatus == SHELL_SUCCESS) {
        PathName = ShellCommandLineGetRawValue(Package, 1);
        if (PathName == NULL) {
          CurDir = gEfiShellProtocol->GetCurDir(NULL);
          if (CurDir == NULL) {
            ShellStatus = SHELL_NOT_FOUND;
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_NO_CWD), gShellLevel2HiiHandle);
          }
        }
        if (PathName != NULL) {
          if (StrStr(PathName, L":") == NULL && gEfiShellProtocol->GetCurDir(NULL) == NULL) {
            ShellStatus = SHELL_NOT_FOUND;
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_NO_CWD), gShellLevel2HiiHandle);
          } else {
            ASSERT((FullPath == NULL && Size == 0) || (FullPath != NULL));
            StrnCatGrow(&FullPath, &Size, PathName, 0);
            if  (ShellIsDirectory(PathName) == EFI_SUCCESS) {
              if (PathName[StrLen (PathName) - 1] == '\\') {
                //
                // For path ending with '\', just append '*'.
                //
                StrnCatGrow (&FullPath, &Size, L"*", 0);
              } else if (PathName[StrLen (PathName) - 1] == '*') {
                //
                // For path ending with '*', do nothing.
                //
              } else {
                //
                // Otherwise, append '\*' to directory name.
                //
                StrnCatGrow (&FullPath, &Size, L"\\*", 0);
              }
            }
          }
        } else {
          ASSERT(FullPath == NULL);
          StrnCatGrow(&FullPath, NULL, L"*", 0);
        }
        Status = gRT->GetTime(&TheTime, NULL);
        if (EFI_ERROR(Status)) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_UEFI_FUNC_WARN), gShellLevel2HiiHandle, L"gRT->GetTime", Status);
          TheTime.TimeZone = EFI_UNSPECIFIED_TIMEZONE;
        }

        SfoMode = ShellCommandLineGetFlag(Package, L"-sfo");
        if (ShellStatus == SHELL_SUCCESS) {
          ShellStatus = PrintLsOutput(
            ShellCommandLineGetFlag(Package, L"-r"),
            RequiredAttributes,
            SfoMode,
            FullPath,
            TRUE,
            Count,
            (INT16)(TheTime.TimeZone==EFI_UNSPECIFIED_TIMEZONE?0:TheTime.TimeZone)
           );
          if (ShellStatus == SHELL_NOT_FOUND) {
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_LS_FILE_NOT_FOUND), gShellLevel2HiiHandle);
          } else if (ShellStatus == SHELL_INVALID_PARAMETER) {
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_INV), gShellLevel2HiiHandle);
          } else if (ShellStatus == SHELL_ABORTED) {
            //
            // Ignore aborting.
            //
          } else if (ShellStatus != SHELL_SUCCESS) {
            ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_INV), gShellLevel2HiiHandle);
          }
        }
      }
    }
  }

  if (FullPath != NULL) {
    FreePool(FullPath);
  }
  //
  // free the command line package
  //
  ShellCommandLineFreeVarList (Package);

  return (ShellStatus);
}
