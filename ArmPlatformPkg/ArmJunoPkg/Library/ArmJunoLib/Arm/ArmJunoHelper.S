/** @file
*
*  Copyright (c) 2013-2014, ARM Limited. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#include <AsmMacroIoLib.h>
#include <Library/ArmLib.h>

//
// Return the core position from the value of its MpId register
//
// This function returns the core position from the position 0 in the processor.
// This function might be called from assembler before any stack is set.
//
// @return   Return the core position
//
//UINTN
//ArmPlatformGetCorePosition (
//  IN UINTN MpId
//  );
// With this function: CorePos = (ClusterId * 2) + CoreId
ASM_FUNC(ArmPlatformGetCorePosition)
  and   r1, r0, #ARM_CORE_MASK
  and   r0, r0, #ARM_CLUSTER_MASK
  add   r0, r1, r0, LSR #7
  bx    lr

//
// Return the MpId of the primary core
//
// This function returns the MpId of the primary core.
// This function might be called from assembler before any stack is set.
//
// @return   Return the MpId of the primary core
//
//UINTN
//ArmPlatformGetPrimaryCoreMpId (
//  VOID
//  );
ASM_FUNC(ArmPlatformGetPrimaryCoreMpId)
  LDRL  (r0, PrimaryCoreMpid)
  bx    lr

//
// Return a non-zero value if the callee is the primary core
//
// This function returns a non-zero value if the callee is the primary core.
// The primary core is the core responsible to initialize the hardware and run UEFI.
// This function might be called from assembler before any stack is set.
//
//  @return   Return a non-zero value if the callee is the primary core.
//
//UINTN
//ArmPlatformIsPrimaryCore (
//  IN UINTN MpId
//  );
ASM_FUNC(ArmPlatformIsPrimaryCore)
  MOV32 (r1, FixedPcdGet32 (PcdArmPrimaryCoreMask))
  and   r0, r0, r1

  LDRL  (r1, PrimaryCoreMpid)

  cmp   r0, r1
  moveq r0, #1
  movne r0, #0
  bx    lr

//
// First platform specific function to be called in the PEI phase
//
// This function is actually the first function called by the PrePi
// or PrePeiCore modules. It allows to retrieve arguments passed to
// the UEFI firmware through the CPU registers.
//
ASM_FUNC(ArmPlatformPeiBootAction)
  // The trusted firmware passes the primary CPU MPID through r0 register.
  // Save it in a variable.
  adr  r1, PrimaryCoreMpid
  str  r0, [r1]
  bx   lr

PrimaryCoreMpid:  .word    0x0
