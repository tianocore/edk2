/*++

Copyright (c)  1999-2006 Intel Corporation. All rights reserved
This program and the accompanying materials are licensed and made available 
under the terms and conditions of the BSD License which accompanies this 
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


Module Name:
  
  SecFixup.h

Abstract:

  Definitions for the SecFixup utility.

--*/

#ifndef _SEC_FIXUP_H
#define _SEC_FIXUP_H

//
// Utility Name
//
#define UTILITY_NAME  "SecFixup"

//
// Utility version information
//
#define UTILITY_MAJOR_VERSION 0
#define UTILITY_MINOR_VERSION 1
#define UTILITY_DATE          __DATE__

//
// The maximum number of arguments accepted from the command line.
//
#define MAX_ARGS        4

#define DEST_REL_OFFSET 13
#define BUF_SIZE        (8 * 1024)

//
// The function that displays general utility information
//
VOID
Version (
  VOID
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  None

Returns:

  GC_TODO: add return values

--*/
;

//
// The function that displays the utility usage message.
//
VOID
Usage (
  VOID
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  None

Returns:

  GC_TODO: add return values

--*/
;

//
// The function that gets the entry point of a PE/TE file.
//
STATUS
GetEntryPoint (
  IN  FILE   *ExeFile,
  OUT UINT32 *EntryPoint
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  ExeFile     - GC_TODO: add argument description
  EntryPoint  - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

//
// The function that copies a file.
//
STATUS
CopyFile (
  FILE    *FpIn,
  FILE    *FpOut
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  FpIn  - GC_TODO: add argument description
  FpOut - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

#endif
