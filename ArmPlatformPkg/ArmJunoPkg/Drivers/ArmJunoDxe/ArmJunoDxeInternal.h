/** @file
*
*  Copyright (c) 2013-2015, ARM Limited. All rights reserved.
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

#ifndef __ARM_JUNO_DXE_INTERNAL_H__
#define __ARM_JUNO_DXE_INTERNAL_H__

#include <Uefi.h>

#include <Library/BaseLib.h>
#include <Library/ArmLib.h>
#include <Library/AcpiLib.h>
#include <Library/DebugLib.h>
#include <Library/DxeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

#include <Protocol/AcpiTable.h>

#include <IndustryStandard/Acpi.h>

#define ACPI_SPECFLAG_PREFETCHABLE    0x06
#define JUNO_MARVELL_YUKON_ID         0x438011AB /* Juno Marvell PCI Dev ID */
#define TST_CFG_WRITE_ENABLE          0x02       /* Enable Config Write */
#define TST_CFG_WRITE_DISABLE         0x00       /* Disable Config Write */
#define CS_RESET_CLR                  0x02       /* SW Reset Clear */
#define CS_RESET_SET                  0x00       /* SW Reset Set */
#define R_CONTROL_STATUS              0x0004     /* Control/Status Register */
#define R_MAC                         0x0100     /* MAC Address */
#define R_MAC_MAINT                   0x0110     /* MAC Address Maintenance */
#define R_MAC_LOW                     0x04       /* MAC Address Low Register Offset */
#define R_TST_CTRL_1                  0x0158     /* Test Control Register 1 */


/**
 * Callback called when ACPI Protocol is installed
 */
VOID
AcpiPciNotificationEvent (
  IN  EFI_EVENT                Event,
  IN  VOID                     *Context
  );

#endif // __ARM_JUNO_DXE_INTERNAL_H__
