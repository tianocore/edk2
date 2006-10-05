/*++

Copyright (c)  2004-2006 Intel Corporation. All rights reserved
This program and the accompanying materials are licensed and made available 
under the terms and conditions of the BSD License which accompanies this 
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.


Module Name:

  CreateGuid.c  

Abstract:

  Library routine to create a GUID

--*/

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

void
CreateGuid (
  GUID *Guid
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  Guid  - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
{
  CoCreateGuid (Guid);
}
