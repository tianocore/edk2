/*++

Copyright (c)  1999 - 2002 Intel Corporation. All rights reserved
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.


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
PrintUtilityInfo (
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
PrintUsage (
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
