/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  LibGlobals.c

Abstract:

  Lib Globals

  gBS         - Pointer to the EFI Boot Services Table
  gST         - Pointer to EFI System Table
  gRtErrorLevel - Error level used with DEBUG () macro

--*/

#include "Tiano.h"
#include "EfiRuntimeLib.h"
#include EFI_GUID_DEFINITION (StatusCodeCallerId)
#include EFI_GUID_DEFINITION (StatusCodeDataTypeId)

//
// Lib globals that can ONLY be used at BootServices time!
//
EFI_BOOT_SERVICES *gBS;
EFI_SYSTEM_TABLE  *gST;
EFI_DXE_SERVICES  *gDS          = NULL;
UINTN             gRtErrorLevel = EFI_DBUG_MASK | EFI_D_LOAD;
