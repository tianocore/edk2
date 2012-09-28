//
//  Copyright (c) 2011, ARM Limited. All rights reserved.
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

#include <AsmMacroIoLib.h>
#include <Base.h>
#include <Library/ArmPlatformLib.h>
#include <AutoGen.h>
#include <ArmPlatform.h>

  INCLUDE AsmMacroIoLib.inc

  EXPORT  ArmPlatformSecBootAction
  EXPORT  ArmPlatformSecBootMemoryInit

  PRESERVE8
  AREA    RTSMVExpressBootMode, CODE, READONLY

/**
  Call at the beginning of the platform boot up

  This function allows the firmware platform to do extra actions at the early
  stage of the platform power up.

  Note: This function must be implemented in assembler as there is no stack set up yet

**/
ArmPlatformSecBootAction
  bx  lr

/**
  Initialize the memory where the initial stacks will reside

  This memory can contain the initial stacks (Secure and Secure Monitor stacks).
  In some platform, this region is already initialized and the implementation of this function can
  do nothing. This memory can also represent the Secure RAM.
  This function is called before the satck has been set up. Its implementation must ensure the stack
  pointer is not used (probably required to use assembly language)

**/
ArmPlatformSecBootMemoryInit
  // The SMC does not need to be initialized for RTSM
  bx    lr

  END
