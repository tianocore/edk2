//
//  Copyright (c) 2012, ARM Limited. All rights reserved.
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

#include <Library/ArmLib.h>

  INCLUDE AsmMacroIoLib.inc

  EXPORT  ArmPlatformGetCorePosition

  PRESERVE8
  AREA    CTA15A7Helper, CODE, READONLY

//UINTN
//ArmPlatformGetCorePosition (
//  IN UINTN MpId
//  );
ArmPlatformGetCorePosition FUNCTION
  and	r1, r0, #ARM_CORE_MASK
  and	r0, r0, #ARM_CLUSTER_MASK
  add	r0, r1, r0, LSR #7
  bx 	lr
  ENDFUNC

  END
