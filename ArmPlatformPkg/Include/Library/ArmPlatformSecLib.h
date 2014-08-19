/** @file
*
*  Copyright (c) 2011-2012, ARM Limited. All rights reserved.
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

#ifndef _ARMPLATFORMSECLIB_H_
#define _ARMPLATFORMSECLIB_H_

#define ARM_SEC_BOOT_MASK                 ~0
#define ARM_SEC_COLD_BOOT                 (1 << 0)
#define ARM_SEC_SECONDARY_COLD_BOOT       (1 << 1)

/**
  Initialize the memory where the initial stacks will reside

  This memory can contain the initial stacks (Secure and Secure Monitor stacks).
  In some platform, this region is already initialized and the implementation of this function can
  do nothing. This memory can also represent the Secure RAM.
  This function is called before the satck has been set up. Its implementation must ensure the stack
  pointer is not used (probably required to use assembly language)

**/
VOID
ArmPlatformSecBootMemoryInit (
  VOID
  );

/**
  Call at the beginning of the platform boot up

  This function allows the firmware platform to do extra actions at the early
  stage of the platform power up.

  Note: This function must be implemented in assembler as there is no stack set up yet

**/
VOID
ArmPlatformSecBootAction (
  VOID
  );

/**
  Initialize controllers that must setup at the early stage

  Some peripherals must be initialized in Secure World.
  For example: Some L2 controller, interconnect, clock, DMC, etc

**/
RETURN_STATUS
ArmPlatformSecInitialize (
  IN  UINTN                     MpId
  );

/**
  Call before jumping to Normal World

  This function allows the firmware platform to do extra actions before
  jumping to the Normal World

**/
VOID
ArmPlatformSecExtraAction (
  IN  UINTN         MpId,
  OUT UINTN*        JumpAddress
  );

/**
  Initialize the Secure peripherals and memory regions

  If Trustzone is supported by your platform then this function makes the required initialization
  of the secure peripherals and memory regions.

**/
VOID
ArmPlatformSecTrustzoneInit (
  IN  UINTN                     MpId
  );

#endif
