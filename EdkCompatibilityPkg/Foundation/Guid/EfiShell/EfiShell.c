/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  
  EfiShell.c
    
Abstract:

  FFS Filename for EFI Shell

--*/

#include "Tiano.h"

#include EFI_GUID_DEFINITION (EfiShell)

EFI_GUID gEfiShellFileGuid = EFI_SHELL_FILE_GUID;
EFI_GUID gEfiMiniShellFileGuid = EFI_MINI_SHELL_FILE_GUID;

EFI_GUID_STRING (&gEfiShellFileGuid, "EfiShell", "Efi Shell FFS file name GUID")
EFI_GUID_STRING (&gEfiMiniShellFileGuid, "EfiMiniShell", "Efi Mini-Shell FFS file name GUID")

