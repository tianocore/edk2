/*++

Copyright (c) 2004 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  FindFiles.c 
  
Abstract:

  OS-specific functions to assist in finding files in
  subdirectories.
  
--*/

#include <windows.h>
#include <direct.h>
//
// #include <io.h>         // for _chmod()
//
#include <sys/stat.h>
//
// #include <errno.h>      // for errno
//
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "HiiPack.h"

extern
void
Error (
  char    *Name,
  UINT32  LineNumber,
  UINT32  MessageCode,
  char    *Text,
  char    *MsgFmt,
  ...
  );

static
int
ProcessDirectory (
  char                *RootDirectory,
  char                *FileMask,
  FIND_FILE_CALLBACK  Callback
  );

/*****************************************************************************/
int
FindFiles (
  char                *RootDirectory,
  char                *FileMask,
  FIND_FILE_CALLBACK  Callback
  )
/*++

Routine Description:
  Find files of a given name under a root directory

Arguments:
  RootDirectory - base directory -- look in this directory and
                  all its subdirectories for files matching FileMask.
  FileMask      - file mask of files to find
  Callback      - function to call for each file found 

Returns:

--*/
{
  char  FullPath[MAX_PATH];
  //
  // If RootDirectory is relative, then append to cwd.
  //
  if (isalpha (RootDirectory[0]) && (RootDirectory[1] == ':')) {
    strcpy (FullPath, RootDirectory);
  } else {
    //
    // Get current working directory
    //
    if (_getcwd (FullPath, sizeof (FullPath)) == NULL) {
      Error (NULL, 0, 0, "failed to get current working directory", NULL);
      return 1;
    }
    //
    // Append the relative path they passed in
    //
    if (FullPath[strlen (FullPath) - 1] != '\\') {
      strcat (FullPath, "\\");
    }

    strcat (FullPath, RootDirectory);
  }

  if (FullPath[strlen (FullPath) - 1] == '\\') {
    FullPath[strlen (FullPath) - 1] = 0;
  }
  //
  // Process the directory
  //
  return ProcessDirectory (FullPath, FileMask, Callback);
}

static
int
ProcessDirectory (
  char                *RootDirectory,
  char                *FileMask,
  FIND_FILE_CALLBACK  Callback
  )
/*++

Routine Description:
  Process a directory to find all files matching a given file mask

Arguments:
  RootDirectory - base directory -- look in this directory and
                  all its subdirectories for files matching FileMask.
  FileMask      - file mask of files to find
  Callback      - function to call for each file found 

Returns:

--*/
{
  HANDLE          Handle;
  WIN32_FIND_DATA FindData;
  char            TempName[MAX_PATH];

  Handle = INVALID_HANDLE_VALUE;
  //
  // Concatenate the filemask to the directory to create the full
  // path\mask path name.
  //
  strcpy (TempName, RootDirectory);
  strcat (TempName, "\\");
  strcat (TempName, FileMask);
  memset (&FindData, 0, sizeof (FindData));
  Handle = FindFirstFile (TempName, &FindData);
  if (Handle != INVALID_HANDLE_VALUE) {
    do {
      if ((FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
        strcpy (TempName, RootDirectory);
        strcat (TempName, "\\");
        strcat (TempName, FindData.cFileName);
        if (Callback (TempName) != 0) {
          goto Done;
        }
      }
    } while (FindNextFile (Handle, &FindData));
  }

  if (Handle != INVALID_HANDLE_VALUE) {
    FindClose (Handle);
    Handle = INVALID_HANDLE_VALUE;
  }
  //
  // Now create a *.* file mask to get all subdirectories and recursive call this
  // function to handle each one found.
  //
  strcpy (TempName, RootDirectory);
  strcat (TempName, "\\*.*");
  memset (&FindData, 0, sizeof (FindData));
  Handle = FindFirstFile (TempName, &FindData);
  //
  // Loop until no more files/directories
  //
  if (Handle != INVALID_HANDLE_VALUE) {
    do {
      if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        //
        // Make sure it's not "." or ".."
        //
        if ((strcmp (FindData.cFileName, ".") != 0) && (strcmp (FindData.cFileName, "..") != 0)) {
          //
          // Found a valid directory. Put it all together and make a recursive call
          // to process it.
          //
          strcpy (TempName, RootDirectory);
          strcat (TempName, "\\");
          strcat (TempName, FindData.cFileName);
          if (ProcessDirectory (TempName, FileMask, Callback) != 0) {
            goto Done;
          }
        }
      }
    } while (FindNextFile (Handle, &FindData));
  }

Done:
  //
  // Free the handle
  //
  if (Handle != INVALID_HANDLE_VALUE) {
    FindClose (Handle);
  }

  return 0;
}
