//
//  Copyright (c) 2013, ARM Limited. All rights reserved.
//
//  This program and the accompanying materials
//  are licensed and made available under the terms and conditions of the BSD License
//  which accompanies this distribution.  The full text of the license may be found at
//  http://opensource.org/licenses/bsd-license.php
//
//  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
//  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
//
//

  INCLUDE AsmMacroIoLib.inc

  EXPORT  InitializeGicV3

  PRESERVE8
  AREA    GicV3, CODE, READONLY

/* Initialize GICv3 to expose it as a GICv2 as UEFI does not support GICv3 yet */
InitializeGicV3 FUNCTION
  // GICv3 Initialization not Supported yet
  bx  lr
  ENDFUNC

  END
