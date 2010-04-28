/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  EfiDependency.h

Abstract:
   
  This module contains data specific to dependency expressions
  and local function prototypes.
        
--*/

#ifndef _DEPENDENCY_H_
#define _DEPENDENCY_H_

//
//
// EFI_DEP_BEFORE       - If present, it must be the first and only opcode
// EFI_DEP_AFTER        - If present, it must be the first and only opcode
// EFI_DEP_SOR          - If present, it must be the first opcode
// EFI_DEP_REPLACE_TRUE - Used to dynamically patch the dependecy expression
//                        to save time.  A EFI_DEP_PUSH is evauated one an
//                        replaced with EFI_DEP_REPLACE_TRUE
//
#define EFI_DEP_BEFORE        0x00
#define EFI_DEP_AFTER         0x01
#define EFI_DEP_PUSH          0x02
#define EFI_DEP_AND           0x03
#define EFI_DEP_OR            0x04
#define EFI_DEP_NOT           0x05
#define EFI_DEP_TRUE          0x06
#define EFI_DEP_FALSE         0x07
#define EFI_DEP_END           0x08
#define EFI_DEP_SOR           0x09
#define EFI_DEP_REPLACE_TRUE  0xff

//
// Define the initial size of the dependency expression evaluation stack
//
#define DEPEX_STACK_SIZE_INCREMENT  0x1000

#endif
