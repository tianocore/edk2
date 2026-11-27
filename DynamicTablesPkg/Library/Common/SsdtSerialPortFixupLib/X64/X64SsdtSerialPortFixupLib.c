/** @file
  SSDT Serial Port Fixup Library.

  Copyright (c) 2026, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - Arm Server Base Boot Requirements (SBBR), s4.2.1.8 "SPCR".
  - Microsoft Debug Port Table 2 (DBG2) Specification - December 10, 2015.
  - ACPI for Arm Components 1.0 - 2020
  - Arm Generic Interrupt Controller Architecture Specification,
    Issue H, January 2022.
    (https://developer.arm.com/documentation/ihi0069/)
**/
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>

#include <ArchCommonNameSpaceObjects.h>

/** Validate the Serial Port Architecture Information.

  @param [in]  SerialPortInfoTable    Table of CM_ARCH_COMMON_SERIAL_PORT_INFO.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
ArchValidateSerialPortInfo (
  IN  CONST CM_ARCH_COMMON_SERIAL_PORT_INFO  *SerialPortInfo
  )
{
  /// Nothing to do.
  return EFI_SUCCESS;
}
