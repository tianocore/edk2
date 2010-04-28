/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  LibGlobalErrorLevel.c

Abstract:

  Globals used in EFI Driver Lib. They are initialized in EfiDriverLib.c.
  Each seperatly linked module has it's own copy of these globals.

  gBS       - Boot Services table pointer
  gRT       - Runt Time services table pointer
  gST       - System Table pointer
  
  gErrorLevel     - Debug error level.

--*/

#include "Tiano.h"
#include "EfiDriverLib.h"
#include EFI_GUID_DEFINITION (StatusCodeCallerId)
#include EFI_GUID_DEFINITION (StatusCodeDataTypeId)

UINTN gErrorLevel = EFI_DBUG_MASK | EFI_D_LOAD;
