/** @file

  Copyright 2024 Google LLC

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef ARM_GIC_BASE_H_
#define ARM_GIC_BASE_H_

#include <Uefi/UefiBaseType.h>

#include <Library/ArmLib.h>

UINTN
ArmGicV3GetControlRegister (
  VOID
  );

VOID
ArmGicV3SetControlRegister (
  IN UINTN  Value
  );

#endif // ARM_GIC_BASE_H_
