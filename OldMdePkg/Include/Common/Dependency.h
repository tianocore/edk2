/** @file
  This module contains data specific to dependency expressions.

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:  Dependency.h

  @par Revision Reference:
  These definitions are from DxeCis 0.91 spec.

**/

#ifndef __DEPENDENCY_H__
#define __DEPENDENCY_H__

/// EFI_DEP_BEFORE       - If present, this must be the first and only opcode
#define EFI_DEP_BEFORE        0x00

/// EFI_DEP_AFTER        - If present, this must be the first and only opcode
#define EFI_DEP_AFTER         0x01

#define EFI_DEP_PUSH          0x02
#define EFI_DEP_AND           0x03
#define EFI_DEP_OR            0x04
#define EFI_DEP_NOT           0x05
#define EFI_DEP_TRUE          0x06
#define EFI_DEP_FALSE         0x07
#define EFI_DEP_END           0x08

/// EFI_DEP_SOR          - If present, this must be the first opcode
#define EFI_DEP_SOR           0x09

#endif
