/** @file
  Main file for attrib shell level 2 function.

  (C) Copyright 2014-2015 Hewlett-Packard Development Company, L.P.<BR>
  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UefiShellLevel2CommandsLib.h"

STATIC CONST CHAR16  AllFiles[] = L"*";

STATIC CONST SHELL_PARAM_ITEM  AttribParamList[] = {
  { L"-a", TypeFlag },
  { L"+a", TypeFlag },
  { L"-s", TypeFlag },
  { L"+s", TypeFlag },
  { L"-h", TypeFlag },
  { L"+h", TypeFlag },
  { L"-r", TypeFlag },
  { L"+r", TypeFlag },
  { NULL,  TypeMax  }
};

/**
  Function for 'attrib' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunAttrib (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  UINT64               FileAttributesToAdd;
  UINT64               FileAttributesToRemove;
  EFI_STATUS           Status;
  LIST_ENTRY           *Package;
  CHAR16               *ProblemParam;
  SHELL_STATUS         ShellStatus;
  UINTN                ParamNumberCount;
  CONST CHAR16         *FileName;
  EFI_SHELL_FILE_INFO  *ListOfFiles;
  EFI_SHELL_FILE_INFO  *FileNode;
  EFI_FILE_INFO        *FileInfo;

  ListOfFiles  = NULL;
  ShellStatus  = SHELL_SUCCESS;
  ProblemParam = NULL;

  //
  // initialize the shell lib (we must be in non-auto-init...)
  //
  Status = ShellInitialize ();
  ASSERT_EFI_ERROR (Status);

  //
  // parse the command line
  //
  Status = ShellCommandLineParse (AttribParamList, &Package, &ProblemParam, TRUE);
  if (EFI_ERROR (Status)) {
    if ((Status == EFI_VOLUME_CORRUPTED) && (ProblemParam != NULL)) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellLevel2HiiHandle, L"attrib", ProblemParam);
      FreePool (ProblemParam);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      ASSERT (FALSE);
    }
  } else {
    //
    // check for "-?"
    //
    if (ShellCommandLineGetFlag (Package, L"-?")) {
      ASSERT (FALSE);
    } else {
      FileAttributesToAdd    = 0;
      FileAttributesToRemove = 0;

      //
      // apply or remove each flag
      //
      if (ShellCommandLineGetFlag (Package, L"+a")) {
        FileAttributesToAdd |= EFI_FILE_ARCHIVE;
      }

      if (ShellCommandLineGetFlag (Package, L"-a")) {
        FileAttributesToRemove |= EFI_FILE_ARCHIVE;
      }

      if (ShellCommandLineGetFlag (Package, L"+s")) {
        FileAttributesToAdd |= EFI_FILE_SYSTEM;
      }

      if (ShellCommandLineGetFlag (Package, L"-s")) {
        FileAttributesToRemove |= EFI_FILE_SYSTEM;
      }

      if (ShellCommandLineGetFlag (Package, L"+h")) {
        FileAttributesToAdd |= EFI_FILE_HIDDEN;
      }

      if (ShellCommandLineGetFlag (Package, L"-h")) {
        FileAttributesToRemove |= EFI_FILE_HIDDEN;
      }

      if (ShellCommandLineGetFlag (Package, L"+r")) {
        FileAttributesToAdd |= EFI_FILE_READ_ONLY;
      }

      if (ShellCommandLineGetFlag (Package, L"-r")) {
        FileAttributesToRemove |= EFI_FILE_READ_ONLY;
      }

      if ((FileAttributesToRemove == 0) && (FileAttributesToAdd == 0)) {
        //
        // Do display as we have no attributes to change
        //
        for ( ParamNumberCount = 1
              ;
              ; ParamNumberCount++
              )
        {
          FileName = ShellCommandLineGetRawValue (Package, ParamNumberCount);
          // if we dont have anything left, move on...
          if ((FileName == NULL) && (ParamNumberCount == 1)) {
            FileName = (CHAR16 *)AllFiles;
          } else if (FileName == NULL) {
            break;
          }

          ASSERT (ListOfFiles == NULL);
          Status = ShellOpenFileMetaArg ((CHAR16 *)FileName, EFI_FILE_MODE_READ, &ListOfFiles);
          if (EFI_ERROR (Status)) {
            ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_FILE_OPEN_FAIL), gShellLevel2HiiHandle, L"attrib", ShellCommandLineGetRawValue (Package, ParamNumberCount));
            ShellStatus = SHELL_NOT_FOUND;
          } else {
            for (FileNode = (EFI_SHELL_FILE_INFO *)GetFirstNode (&ListOfFiles->Link)
                 ; !IsNull (&ListOfFiles->Link, &FileNode->Link)
                 ; FileNode = (EFI_SHELL_FILE_INFO *)GetNextNode (&ListOfFiles->Link, &FileNode->Link)
                 )
            {
              ShellPrintHiiEx (
                -1,
                -1,
                NULL,
                STRING_TOKEN (STR_ATTRIB_OUTPUT_LINE),
                gShellLevel2HiiHandle,
                FileNode->Info->Attribute&EFI_FILE_DIRECTORY ? L'D' : L' ',
                FileNode->Info->Attribute&EFI_FILE_ARCHIVE ?   L'A' : L' ',
                FileNode->Info->Attribute&EFI_FILE_SYSTEM ?    L'S' : L' ',
                FileNode->Info->Attribute&EFI_FILE_HIDDEN ?    L'H' : L' ',
                FileNode->Info->Attribute&EFI_FILE_READ_ONLY ? L'R' : L' ',
                FileNode->FileName
                );

              if (ShellGetExecutionBreakFlag ()) {
                ShellStatus = SHELL_ABORTED;
                break;
              }
            }

            Status      = ShellCloseFileMetaArg (&ListOfFiles);
            ListOfFiles = NULL;
            if (EFI_ERROR (Status)) {
              ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_FILE_CLOSE_FAIL), gShellLevel2HiiHandle, L"attrib", ShellCommandLineGetRawValue (Package, ParamNumberCount));
              ShellStatus = SHELL_NOT_FOUND;
            }
          } // for loop for handling wildcard filenames
        } // for loop for printing out the info
      } else if ((FileAttributesToRemove & FileAttributesToAdd) != 0) {
        //
        // fail as we have conflcting params.
        //
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_CON), gShellLevel2HiiHandle, L"attrib");
        ShellStatus = SHELL_INVALID_PARAMETER;
      } else {
        //
        // enumerate through all the files/directories and apply the attributes
        //
        for ( ParamNumberCount = 1
              ;
              ; ParamNumberCount++
              )
        {
          FileName = ShellCommandLineGetRawValue (Package, ParamNumberCount);
          // if we dont have anything left, move on...
          if (FileName == NULL) {
            //
            // make sure we are not failing on the first one we do... if yes that's an error...
            //
            if (ParamNumberCount == 1) {
              ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_FEW), gShellLevel2HiiHandle, L"attrib");
              ShellStatus = SHELL_INVALID_PARAMETER;
            }

            break;
          }

          //
          // OpenFileByName / GetFileInfo / Change attributes / SetFileInfo / CloseFile / free memory
          // for each file or directory on the line.
          //

          //
          // Open the file(s)
          //
          ASSERT (ListOfFiles == NULL);
          Status = ShellOpenFileMetaArg ((CHAR16 *)FileName, EFI_FILE_MODE_READ, &ListOfFiles);
          if (EFI_ERROR (Status)) {
            ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_FILE_OPEN_FAIL), gShellLevel2HiiHandle, L"attrib", ShellCommandLineGetRawValue (Package, ParamNumberCount));
            ShellStatus = SHELL_NOT_FOUND;
          } else {
            for (FileNode = (EFI_SHELL_FILE_INFO *)GetFirstNode (&ListOfFiles->Link)
                 ; !IsNull (&ListOfFiles->Link, &FileNode->Link)
                 ; FileNode = (EFI_SHELL_FILE_INFO *)GetNextNode (&ListOfFiles->Link, &FileNode->Link)
                 )
            {
              //
              // skip the directory traversing stuff...
              //
              if ((StrCmp (FileNode->FileName, L".") == 0) || (StrCmp (FileNode->FileName, L"..") == 0)) {
                continue;
              }

              FileInfo = gEfiShellProtocol->GetFileInfo (FileNode->Handle);

              //
              // if we are removing Read-Only we need to do that alone
              //
              if ((FileAttributesToRemove & EFI_FILE_READ_ONLY) == EFI_FILE_READ_ONLY) {
                FileInfo->Attribute &= ~EFI_FILE_READ_ONLY;
                //
                // SetFileInfo
                //
                Status = ShellSetFileInfo (FileNode->Handle, FileInfo);
                if (EFI_ERROR (Status)) {
                  ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_FILE_AD), gShellLevel2HiiHandle, L"attrib", ShellCommandLineGetRawValue (Package, ParamNumberCount));
                  ShellStatus = SHELL_ACCESS_DENIED;
                }
              }

              //
              // change the attribute
              //
              FileInfo->Attribute &= ~FileAttributesToRemove;
              FileInfo->Attribute |= FileAttributesToAdd;

              //
              // SetFileInfo
              //
              Status = ShellSetFileInfo (FileNode->Handle, FileInfo);
              if (EFI_ERROR (Status)) {
                ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_FILE_AD), gShellLevel2HiiHandle, L"attrib", ShellCommandLineGetRawValue (Package, ParamNumberCount));
                ShellStatus = SHELL_ACCESS_DENIED;
              }

              SHELL_FREE_NON_NULL (FileInfo);
            }

            Status      = ShellCloseFileMetaArg (&ListOfFiles);
            ListOfFiles = NULL;
            if (EFI_ERROR (Status)) {
              ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_FILE_CLOSE_FAIL), gShellLevel2HiiHandle, L"attrib", ShellCommandLineGetRawValue (Package, ParamNumberCount));
              ShellStatus = SHELL_NOT_FOUND;
            }
          } // for loop for handling wildcard filenames
        }
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
