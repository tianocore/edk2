/** @file
  This file contains the boot script definitions that are shared between the
  Boot Script Executor PPI and the Boot Script Save Protocol.

Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _BOOT_SCRIPT_H_
#define _BOOT_SCRIPT_H_

#include <PiDxe.h>
///
/// The framework implementation defines follow opcode that are different from the PI specification:
/// Add FRAMEWORK_ prefix to avoid naming conflict.
///
/// S3 Boot Script Table identifier.
///
#define FRAMEWORK_EFI_ACPI_S3_RESUME_SCRIPT_TABLE  0x00
///
/// The opcode is used to add a record for memory reads of the memory location and continues when the
/// exit criteria is satisfied, or after a defined duration.
///
#define FRAMEWORK_EFI_BOOT_SCRIPT_MEM_POLL_OPCODE  0x09
///
/// The opcode is used to add a record for dispatching specified arbitrary code into a specified
/// boot script table.
///
#define FRAMEWORK_EFI_BOOT_SCRIPT_DISPATCH_2_OPCODE  0x0D
///
/// The opcode indicates the start of the boot script table.
///
#define FRAMEWORK_EFI_BOOT_SCRIPT_TABLE_OPCODE  0xAA
///
/// The opcode indicates the end of the boot script table.
///
#define FRAMEWORK_EFI_BOOT_SCRIPT_TERMINATE_OPCODE  0xFF

#endif
