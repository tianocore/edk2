/*++

Copyright (c) 2004 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  
  GenFvImageExe.h

Abstract:

  Definitions for the PeimFixup exe utility.

--*/

//
// Coded to Tiano Coding Standards
//
#ifndef _EFI_GEN_FV_IMAGE_EXE_H
#define _EFI_GEN_FV_IMAGE_EXE_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "GenFvImageLib.h"

//
// Utility Name
//
#define UTILITY_NAME  "GenFvImage"

//
// Utility version information
//
#define UTILITY_VERSION "v1.1"
#define UTILITY_DATE          __DATE__

//
// The maximum number of arguments accepted from the command line.
//
#define MAX_ARGS  3

//
// The function that displays general utility information
//
VOID
PrintUtilityInfo (
  VOID
  )
/*++

Routine Description:

  TODO: Add function description

Arguments:

  None

Returns:

  TODO: add return values

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

  TODO: Add function description

Arguments:

  None

Returns:

  TODO: add return values

--*/
;

#endif
