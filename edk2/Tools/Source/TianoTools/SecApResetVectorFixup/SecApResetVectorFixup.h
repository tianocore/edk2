/*++

Copyright (c)  2005 Intel Corporation. All rights reserved
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.


Module Name:
  
  SecApResetVectorFixup.h

Abstract:

  Definitions for the SecApResetVectorFixup utility.

--*/

#ifndef _SEC_AP_RESET_VECTOR_FIXUP_H
#define _SEC_AP_RESET_VECTOR_FIXUP_H

#include <stdio.h>
#include <stdlib.h>

#include <Common/UefiBaseTypes.h>
#include <Common/EfiImage.h>
#include <Common/FirmwareVolumeImageFormat.h>
#include <Common/FirmwareFileSystem.h>
#include <Common/FirmwareVolumeHeader.h>

#include "EfiUtilityMsgs.c"
#include "CommonLib.h"


//
// Utility Name
//
#define UTILITY_NAME  "SecApResetVectorFixup"

//
// Utility version information
//
#define UTILITY_MAJOR_VERSION 0
#define UTILITY_MINOR_VERSION 1
#define UTILITY_DATE          __DATE__

//
// The maximum number of arguments accepted from the command line.
//
#define MAX_ARGS        3
#define BUF_SIZE        (8 * 1024)

#define GETOCCUPIEDSIZE(ActualSize, Alignment) \
  (ActualSize) + (((Alignment) - ((ActualSize) & ((Alignment) - 1))) & ((Alignment) - 1))


VOID
PrintUtilityInfo (
  VOID
  )
/*++

Routine Description:

  Displays the standard utility information to SDTOUT

Arguments:

  None

Returns:

  None

--*/
;

VOID
PrintUsage (
  VOID
  )
/*++

Routine Description:

  Displays the utility usage syntax to STDOUT

Arguments:

  None

Returns:

  None

--*/
;


#endif
