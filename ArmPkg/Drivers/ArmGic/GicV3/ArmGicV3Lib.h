/** @file
*
*  Copyright (c) 2014-2015, ARM Limited. All rights reserved.
*
*  This program and the accompanying materials are licensed and made available
*  under the terms and conditions of the BSD License which accompanies this
*  distribution. The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#ifndef _ARM_GIC_V3_H_
#define _ARM_GIC_V3_H_

#define ICC_SRE_EL2_SRE         (1 << 0)

#define ARM_GICD_IROUTER_IRM BIT31

UINT32
EFIAPI
ArmGicV3GetControlSystemRegisterEnable (
  VOID
  );

VOID
EFIAPI
ArmGicV3SetControlSystemRegisterEnable (
  IN UINT32         ControlSystemRegisterEnable
  );

VOID
EFIAPI
ArmGicV3EnableInterruptInterface (
  VOID
  );

VOID
EFIAPI
ArmGicV3DisableInterruptInterface (
  VOID
  );

UINTN
EFIAPI
ArmGicV3AcknowledgeInterrupt (
  VOID
  );

VOID
EFIAPI
ArmGicV3EndOfInterrupt (
  IN UINTN                  Source
  );

VOID
ArmGicV3SetBinaryPointer (
  IN UINTN                  BinaryPoint
  );

VOID
ArmGicV3SetPriorityMask (
  IN UINTN                  Priority
  );

#endif
