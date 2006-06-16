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
  
  GenFdImage.h

Abstract:

  This file contains the relevant declarations required
  to generate the Firmware Device

--*/

//
// Coded to EFI 2.0 Coding Standard
//
#ifndef _EFI_GEN_FD_IMAGE_H
#define _EFI_GEN_FD_IMAGE_H

//
// Included Header files
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "ParseInf.h"

//
// Defines
//
#define FILE_NAME_SIZE  256

//
// Type Definition
//
typedef struct {
  UINT64  FdSize;
  UINT64  FdBaseAddress;
  UINT8   PadValue;
  CHAR8   OutFileName[FILE_NAME_SIZE];
} FDINFO;

//
// Exported Function Prototype
//
EFI_STATUS
GenerateFdImage (
  IN UINT64  BaseAddress,
  IN UINT64  Size,
  IN UINT8   PadByte,
  IN CHAR8   *OutFile,
  IN CHAR8   **FileList
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  BaseAddress - GC_TODO: add argument description
  Size        - GC_TODO: add argument description
  PadByte     - GC_TODO: add argument description
  OutFile     - GC_TODO: add argument description
  FileList    - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
;

#endif
