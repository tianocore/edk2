/** @file
  This file contains the boot script defintions that are shared between the 
  Boot Script Executor PPI and the Boot Script Save Protocol.

  Copyright (c) 2009, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _FRAMEWORK_BOOT_SCRIPT_H_
#define _FRAMEWORK_BOOT_SCRIPT_H_

///
/// S3 Boot Script Table identifier
///
#define EFI_ACPI_S3_RESUME_SCRIPT_TABLE               0x00

///
/// The opcode is to add a record for an I/O write operation into a specified boot script table.
///
#define EFI_BOOT_SCRIPT_IO_WRITE_OPCODE               0x00
///
/// The opcode is to add a record for an I/O modify operation into a specified boot script table.
///
#define EFI_BOOT_SCRIPT_IO_READ_WRITE_OPCODE          0x01
///
/// The opcode is to add a record for a memory write operation into a specified boot script table.
///
#define EFI_BOOT_SCRIPT_MEM_WRITE_OPCODE              0x02
///
/// The opcode is to add a record for a memory modify operation into a specified boot script table.
///
#define EFI_BOOT_SCRIPT_MEM_READ_WRITE_OPCODE         0x03
///
/// The opcode is to adds a record for a PCI configuration space write operation into a specified boot 
/// script table.
///
#define EFI_BOOT_SCRIPT_PCI_CONFIG_WRITE_OPCODE       0x04
///
/// The opcode is to add a record for a PCI configuration space modify operation into a specified 
/// boot script table.
///
#define EFI_BOOT_SCRIPT_PCI_CONFIG_READ_WRITE_OPCODE  0x05
///
/// The opcode is to add a record for an SMBus command execution into a specified boot script table.
///
#define EFI_BOOT_SCRIPT_SMBUS_EXECUTE_OPCODE          0x06
///
/// The opcode is to adds a record for an execution stall on the processor into a specified
/// boot script table.
///
#define EFI_BOOT_SCRIPT_STALL_OPCODE                  0x07
///
/// The opcode is to add a record for dispatching specified arbitrary code into a specified 
/// boot script table.
///
#define EFI_BOOT_SCRIPT_DISPATCH_OPCODE               0x08

//
// Extensions to boot script definitions 
//
///
/// Inconsistent with specification here: 
/// Follow OPCODEs are not defined in Framework Spec BootScript_0.91, but in
/// PI1.0 Spec. And OPCODEs which are needed in the implementation
///
///
/// The opcode is to add a record for memory reads of the memory location and continues when the 
/// exit criteria is satisfied or after a defined duration.
///
#define EFI_BOOT_SCRIPT_MEM_POLL_OPCODE               0x09
///
/// The opcode is to store arbitrary information in the boot script table which is a no-op on dispatch 
/// and is only used for debugging script issues.
///
#define EFI_BOOT_SCRIPT_INFORMATION_OPCODE            0x0A
///
/// The opcode is to add a record for a PCI configuration space write operation into a 
/// specified boot script table.
/// 
#define EFI_BOOT_SCRIPT_PCI_CONFIG2_WRITE_OPCODE      0x0B
///
/// The opcode is to add a record for a PCI configuration space modify operation into a specified
/// boot script table.
///
#define EFI_BOOT_SCRIPT_PCI_CONFIG2_READ_WRITE_OPCODE 0x0C
///
/// The opcode is to add a record for dispatching specified arbitrary code into a specified
/// boot script table.
///
#define EFI_BOOT_SCRIPT_DISPATCH_2_OPCODE             0x0D

///
/// The opcode indicate the start of the boot script table.
///
#define EFI_BOOT_SCRIPT_TABLE_OPCODE                  0xAA
///
/// The opcode indicate the end of the boot script table.
///
#define EFI_BOOT_SCRIPT_TERMINATE_OPCODE              0xFF

///
/// EFI Boot Script Width
///
typedef enum {
  EfiBootScriptWidthUint8,
  EfiBootScriptWidthUint16,
  EfiBootScriptWidthUint32,
  EfiBootScriptWidthUint64,
  EfiBootScriptWidthFifoUint8,
  EfiBootScriptWidthFifoUint16,
  EfiBootScriptWidthFifoUint32,
  EfiBootScriptWidthFifoUint64,
  EfiBootScriptWidthFillUint8,
  EfiBootScriptWidthFillUint16,
  EfiBootScriptWidthFillUint32,
  EfiBootScriptWidthFillUint64,
  EfiBootScriptWidthMaximum
} EFI_BOOT_SCRIPT_WIDTH;

#endif
