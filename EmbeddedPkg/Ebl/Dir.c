/** @file
  Dir for EBL (Embedded Boot Loader)

  Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
  Portions copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>


  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  Module Name:  CmdTemplate.c

  Search/Replace Dir with the name of your new command

**/

#include "Ebl.h"


GLOBAL_REMOVE_IF_UNREFERENCED   CHAR8 *gFvFileType[] = {
  "All",
  "Bin",
  "section",
  "SEC",
  "PeiCore",
  "DxeCore",
  "PEIM",
  "Driver",
  "Combo",
  "App",
  "NULL",
  "FV"
};


/**
  Perform a dir on a device. The device must support Simple File System Protocol
  or the FV protocol.

  Argv[0] - "dir"
  Argv[1] - Device Name:path. Path is optional
  Argv[2] - Optional filename to match on. A leading * means match substring
  Argv[3] - Optional FV file type

  dir fs1:\efi      ; perform a dir on fs1: device in the efi directory
  dir fs1:\efi *.efi; perform a dir on fs1: device in the efi directory but
                      only print out files that contain the string *.efi
  dir fv1:\         ; perform a dir on fv1: device in the efi directory
                    NOTE: fv devices do not contain subdirs
  dir fv1:\ * PEIM  ; will match all files of type PEIM

  @param  Argc   Number of command arguments in Argv
  @param  Argv   Array of strings that represent the parsed command line.
                 Argv[0] is the command name

  @return EFI_SUCCESS

**/
EFI_STATUS
EblDirCmd (
  IN UINTN  Argc,
  IN CHAR8  **Argv
  )
{
  EFI_STATUS                    Status;
  EFI_OPEN_FILE                 *File;
  EFI_FILE_INFO                 *DirInfo;
  UINTN                         ReadSize;
  UINTN                         CurrentRow;
  CHAR16                        *MatchSubString;
  EFI_STATUS                    GetNextFileStatus;
  UINTN                         Key;
  EFI_FV_FILETYPE               SearchType;
  EFI_FV_FILETYPE               Type;
  EFI_FV_FILE_ATTRIBUTES        Attributes;
  UINTN                         Size;
  EFI_GUID                      NameGuid;
  EFI_FIRMWARE_VOLUME2_PROTOCOL *Fv;
  UINT32                        AuthenticationStatus;
  VOID                          *Section;
  UINTN                         SectionSize;
  EFI_FV_FILETYPE               Index;
  UINTN                         Length;
  UINTN                         BestMatchCount;
  CHAR16                        UnicodeFileName[MAX_CMD_LINE];
  CHAR8                         *Path;
  CHAR8                         *TypeStr;
  UINTN                         TotalSize;


  if (Argc <= 1) {
    Path = EfiGetCwd ();
    if (Path == NULL) {
      return EFI_SUCCESS;
    }
  } else {
    Path = Argv[1];
  }

  File = EfiOpen (Path, EFI_FILE_MODE_READ, 0);
  if (File == NULL) {
    return EFI_SUCCESS;
  }

  if (File->Type == EfiOpenFirmwareVolume) {
    // FV Dir

    SearchType = EFI_FV_FILETYPE_ALL;
    UnicodeFileName[0] = '\0';
    MatchSubString = &UnicodeFileName[0];
    if (Argc > 2) {
      AsciiStrToUnicodeStr (Argv[2], UnicodeFileName);
      if (UnicodeFileName[0] == '*') {
        // Handle *Name substring matching
        MatchSubString = &UnicodeFileName[1];
      }

      // Handle file type matchs
      if (Argc > 3) {
        // match a specific file type, always last argument
        Length = AsciiStrLen (Argv[3]);
        for (Index = 1, BestMatchCount = 0; Index < sizeof (gFvFileType)/sizeof (CHAR8 *); Index++) {
          if (AsciiStriCmp (gFvFileType[Index], Argv[3]) == 0) {
            // exact match
            SearchType = Index;
            break;
          }

          if (AsciiStrniCmp (Argv[3], gFvFileType[Index], Length) == 0) {
            // partial match, so keep looking to make sure there is only one partial match
            BestMatchCount++;
            SearchType = Index;
          }
        }

        if (BestMatchCount > 1) {
          SearchType = EFI_FV_FILETYPE_ALL;
        }
      }
    }

    TotalSize = 0;
    Fv = File->Fv;
    Key = 0;
    CurrentRow = 0;
    do {
      Type = SearchType;
      GetNextFileStatus = Fv->GetNextFile (
                                Fv,
                                &Key,
                                &Type,
                                &NameGuid,
                                &Attributes,
                                &Size
                                );
      if (!EFI_ERROR (GetNextFileStatus)) {
        TotalSize += Size;
        // Calculate size of entire file
        Section = NULL;
        Size = 0;
        Status = Fv->ReadFile (
                      Fv,
                      &NameGuid,
                      Section,
                      &Size,
                      &Type,
                      &Attributes,
                      &AuthenticationStatus
                      );
        if (!((Status == EFI_BUFFER_TOO_SMALL) || !EFI_ERROR (Status))) {
          // EFI_SUCCESS or EFI_BUFFER_TOO_SMALL mean size is valid
          Size = 0;
        }

        TypeStr = (Type <= EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE) ? gFvFileType[Type] : "UNKNOWN";

        // read the UI seciton to do a name match.
        Section = NULL;
        Status = Fv->ReadSection (
                        Fv,
                        &NameGuid,
                        EFI_SECTION_USER_INTERFACE,
                        0,
                        &Section,
                        &SectionSize,
                        &AuthenticationStatus
                        );
        if (!EFI_ERROR (Status)) {
          if (StrStr (Section, MatchSubString) != NULL) {
            AsciiPrint ("%,9d %7a %g %s\n", Size, TypeStr, &NameGuid, Section);
            if (EblAnyKeyToContinueQtoQuit (&CurrentRow, FALSE)) {
              break;
            }
          }
          FreePool (Section);
        } else {
          if (*MatchSubString == '\0') {
            AsciiPrint ("%,9d %7a %g\n", Size, TypeStr, &NameGuid);
            if (EblAnyKeyToContinueQtoQuit (&CurrentRow, FALSE)) {
              break;
            }
          }
        }
      }
    } while (!EFI_ERROR (GetNextFileStatus));

    if (SearchType == EFI_FV_FILETYPE_ALL) {
      AsciiPrint ("%,20d bytes in files %,d bytes free\n", TotalSize, File->FvSize - File->FvHeaderSize - TotalSize);
    }


  } else if ((File->Type == EfiOpenFileSystem) || (File->Type == EfiOpenBlockIo)) {
    // Simple File System DIR

    if (File->FsFileInfo ==  NULL) {
      return EFI_SUCCESS;
    }

    if (!(File->FsFileInfo->Attribute & EFI_FILE_DIRECTORY)) {
      return EFI_SUCCESS;
    }

    // Handle *Name substring matching
    MatchSubString = NULL;
    UnicodeFileName[0] = '\0';
    if (Argc > 2) {
      AsciiStrToUnicodeStr (Argv[2], UnicodeFileName);
      if (UnicodeFileName[0] == '*') {
        MatchSubString = &UnicodeFileName[1];
      }
    }

    File->FsFileHandle->SetPosition (File->FsFileHandle, 0);
    for (CurrentRow = 0;;) {
      // First read gets the size
      DirInfo = NULL;
      ReadSize = 0;
      Status = File->FsFileHandle->Read (File->FsFileHandle, &ReadSize, DirInfo);
      if (Status == EFI_BUFFER_TOO_SMALL) {
        // Allocate the buffer for the real read
        DirInfo = AllocatePool (ReadSize);
        if (DirInfo == NULL) {
          goto Done;
        }

        // Read the data
        Status = File->FsFileHandle->Read (File->FsFileHandle, &ReadSize, DirInfo);
        if ((EFI_ERROR (Status)) || (ReadSize == 0)) {
          break;
        }
      } else {
        break;
      }

      if (MatchSubString != NULL) {
        if (StrStr (&DirInfo->FileName[0], MatchSubString) == NULL) {
          // does not match *name argument, so skip
          continue;
        }
      } else if (UnicodeFileName[0] != '\0') {
        // is not an exact match for name argument, so skip
        if (StrCmp (&DirInfo->FileName[0], UnicodeFileName) != 0) {
          continue;
        }
      }

      if (DirInfo->Attribute & EFI_FILE_DIRECTORY) {
        AsciiPrint ("         <DIR> %s\n", &DirInfo->FileName[0]);
      } else {
        AsciiPrint ("%,14ld %s\n", DirInfo->FileSize, &DirInfo->FileName[0]);
      }

      if (EblAnyKeyToContinueQtoQuit (&CurrentRow, FALSE)) {
        break;
      }

      FreePool (DirInfo);
    }

Done:
    if (DirInfo != NULL) {
      FreePool (DirInfo);
    }
  }

  EfiClose (File);

  return EFI_SUCCESS;
}

/**
  Change the Current Working Directory

  Argv[0] - "cd"
  Argv[1] - Device Name:path. Path is optional

  @param  Argc   Number of command arguments in Argv
  @param  Argv   Array of strings that represent the parsed command line.
                 Argv[0] is the command name

  @return EFI_SUCCESS

**/
EFI_STATUS
EblCdCmd (
  IN UINTN  Argc,
  IN CHAR8  **Argv
  )
{
  if (Argc <= 1) {
    return EFI_SUCCESS;
  }

  return EfiSetCwd (Argv[1]);
}



GLOBAL_REMOVE_IF_UNREFERENCED const EBL_COMMAND_TABLE mCmdDirTemplate[] =
{
  {
    "dir",
    " dirdev [*match]; directory listing of dirdev. opt match a substring",
    NULL,
    EblDirCmd
  },
  {
    "cd",
    " device - set the current working directory",
    NULL,
    EblCdCmd
  }
};


/**
  Initialize the commands in this in this file
**/
VOID
EblInitializeDirCmd (
  VOID
  )
{
  if (FeaturePcdGet (PcdEmbeddedDirCmd)) {
    EblAddCommands (mCmdDirTemplate, sizeof (mCmdDirTemplate)/sizeof (EBL_COMMAND_TABLE));
  }
}

