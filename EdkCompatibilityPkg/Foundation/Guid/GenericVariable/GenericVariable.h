/*++

Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  
    GenericVariable.h
    
Abstract:

    The variable space Guid to pair with a Unicode string name to tag an EFI variable

--*/

#ifndef _GENERIC_VARIABLE_H_
#define _GENERIC_VARIABLE_H_

#define EFI_GENERIC_VARIABLE_GUID \
  { \
    0x59d1c24f, 0x50f1, 0x401a, {0xb1, 0x01, 0xf3, 0x3e, 0x0d, 0xae, 0xd4, 0x43} \
  }

extern EFI_GUID gEfiGenericVariableGuid;

#endif
