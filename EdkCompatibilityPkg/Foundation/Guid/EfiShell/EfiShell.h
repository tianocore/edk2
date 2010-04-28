/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  
  EfiShell.h
    
Abstract:

  FFS Filename for EFI Shell

--*/

#ifndef _EFI_SHELL_H_
#define _EFI_SHELL_H_

#define EFI_SHELL_FILE_GUID  \
  { 0xc57ad6b7, 0x0515, 0x40a8, {0x9d, 0x21, 0x55, 0x16, 0x52, 0x85, 0x4e, 0x37} }

#define EFI_MINI_SHELL_FILE_GUID  \
  { 0x86ad232b, 0xd33a, 0x465c, {0xbf, 0x5f, 0x41, 0x37, 0xb, 0xa9, 0x2f, 0xe2} }

extern EFI_GUID gEfiShellFileGuid;
extern EFI_GUID gEfiMiniShellFileGuid;

#endif
