/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:  

  FileSearch.h
  
Abstract:

  Header file to support file searching.
  
--*/

#ifndef _FILE_SEARCH_H_
#define _FILE_SEARCH_H_

//
// Since the file searching routines are OS dependent, put the
// necessary include paths in this header file so that the non-OS-dependent
// files don't need to include these windows-specific header files.
//
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <direct.h>
#include <windows.h>

//
// Return codes of some of the file search routines
//
#define STATUS_NOT_FOUND  0x1000

//
// Flags for what to search for. Also used in the FileFlags return field.
//
#define FILE_SEARCH_DIR   0x0001
#define FILE_SEARCH_FILE  0x0002

//
// Here's our class definition
//
typedef struct {
  HANDLE          Handle;
  WIN32_FIND_DATA FindData;
  UINT32          FileSearchFlags;    // DIRS, FILES, etc
  UINT32          FileFlags;
  INT8            FileName[MAX_PATH]; // for portability
  STRING_LIST     *ExcludeDirs;
  STRING_LIST     *ExcludeFiles;
  STRING_LIST     *ExcludeExtensions;
} FILE_SEARCH_DATA;

//
// Here's our member functions
//
STATUS
FileSearchInit (
  FILE_SEARCH_DATA    *FSData
  );

STATUS
FileSearchDestroy (
  FILE_SEARCH_DATA    *FSData
  );

STATUS
FileSearchStart (
  FILE_SEARCH_DATA    *FSData,
  char                *FileMask,
  UINT32              SearchFlags
  );

STATUS
FileSearchFindNext (
  FILE_SEARCH_DATA    *FSData
  );

STATUS
FileSearchExcludeDirs (
  FILE_SEARCH_DATA    *FSData,
  STRING_LIST         *StrList
  );
STATUS
FileSearchExcludeExtensions (
  FILE_SEARCH_DATA    *FSData,
  STRING_LIST         *StrList
  );
STATUS
FileSearchExcludeFiles (
  FILE_SEARCH_DATA    *FSData,
  STRING_LIST         *StrList
  );
#endif
