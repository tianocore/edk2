/*++

Copyright (c) 2004 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  HiiPack.h

Abstract:

  Common defines and prototypes for the HII pack tool.
  
--*/

#ifndef _HII_PACK_H_
#define _HII_PACK_H_

#define DEFAULT_VARIABLE_NAME_L L"Setup"
#define DEFAULT_VARIABLE_NAME   "Setup"

#define MAX_VARIABLE_NAME       256
#define FIRST_HII_PACK_HANDLE   1

typedef
int
(*FIND_FILE_CALLBACK) (
  char *FileName
  );

extern
int
FindFiles (
  char                *RootDirectory,
  char                *FileMask,
  FIND_FILE_CALLBACK  Callback
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  RootDirectory - GC_TODO: add argument description
  FileMask      - GC_TODO: add argument description
  Callback      - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

#endif // #ifndef _HII_PACK_H_
