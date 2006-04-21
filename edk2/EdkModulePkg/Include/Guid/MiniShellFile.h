/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
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

#ifndef _MINISHELLFILE_H_
#define _MINISHELLFILE_H_

#define EFI_MINI_SHELL_FILE_GUID  \
  { 0x86ad232b, 0xd33a, 0x465c, {0xbf, 0x5f, 0x41, 0x37, 0xb, 0xa9, 0x2f, 0xe2 } }
  
extern EFI_GUID gEfiMiniShellFileGuid;

#endif
