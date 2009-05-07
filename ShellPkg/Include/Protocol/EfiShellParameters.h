/** @file
  EFI Shell protocol as defined in the UEFI Shell 2.0 specification.
  
  Copyright (c) 2006 - 2009, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef __EFI_SHELL_PARAMETERS_PROTOCOL__
#define __EFI_SHELL_PARAMETERS_PROTOCOL__

#define EFI_SHELL_PARAMETERS_PROTOCOL_GUID \
  { \
  0x752f3136, 0x4e16, 0x4fdc, { 0xa2, 0x2a, 0xe5, 0xf4, 0x68, 0x12, 0xf4, 0xca } \
  }

typedef struct _EFI_SHELL_PARAMETERS_PROTOCOL {
  CHAR16 **Argv;
  UINTN Argc;
  EFI_FILE_HANDLE StdIn;
  EFI_FILE_HANDLE StdOut;
  EFI_FILE_HANDLE StdErr;
} EFI_SHELL_PARAMETERS_PROTOCOL;

extern EFI_GUID gEfiShellParametersProtocolGuid;

#endif
