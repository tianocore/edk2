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

#ifndef _ARM_GIC_V2_H_
#define _ARM_GIC_V2_H_

// Interrupts from 1020 to 1023 are considered as special interrupts (eg: spurious interrupts)
#define ARM_GIC_IS_SPECIAL_INTERRUPTS(Interrupt) (((Interrupt) >= 1020) && ((Interrupt) <= 1023))

VOID
EFIAPI
ArmGicV2SetupNonSecure (
  IN  UINTN         MpId,
  IN  INTN          GicDistributorBase,
  IN  INTN          GicInterruptInterfaceBase
  );

VOID
EFIAPI
ArmGicV2EnableInterruptInterface (
  IN  INTN          GicInterruptInterfaceBase
  );

VOID
EFIAPI
ArmGicV2DisableInterruptInterface (
  IN  INTN          GicInterruptInterfaceBase
  );

UINTN
EFIAPI
ArmGicV2AcknowledgeInterrupt (
  IN  UINTN          GicInterruptInterfaceBase
  );

VOID
EFIAPI
ArmGicV2EndOfInterrupt (
  IN UINTN                  GicInterruptInterfaceBase,
  IN UINTN                  Source
  );

#endif
