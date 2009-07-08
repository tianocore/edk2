/** @file
  This file declares Boot Script Executer PPI.

  This PPI is published by a PEIM upon dispatch and provides an execution engine for the
  Framework boot script. This PEIM should be platform neutral and have no specific knowledge of
  platform instructions and other information. The ability to interpret the boot script depends on the
  abundance of other PPIs that are available. For example, if the script requests an SMBus command
  execution, the PEIM looks for a relevant PPI that is available to execute it, rather than executing it
  by issuing the native IA-32 instruction.

  Copyright (c) 2007 - 2009, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  Module Name:  BootScriptExecuter.h

  @par Revision Reference:
  This PPI is defined in Framework of EFI BootScript spec.
  Version 0.91.

**/

#ifndef _PEI_BOOT_SCRIPT_EXECUTER_PPI_H_
#define _PEI_BOOT_SCRIPT_EXECUTER_PPI_H_

#include <PiPei.h>

#define EFI_ACPI_S3_RESUME_SCRIPT_TABLE               0x00

//
// Boot Script Opcode Definitions
//

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

#define EFI_PEI_BOOT_SCRIPT_EXECUTER_PPI_GUID \
  { \
    0xabd42895, 0x78cf, 0x4872, {0x84, 0x44, 0x1b, 0x5c, 0x18, 0x0b, 0xfb, 0xff } \
  }

typedef struct _EFI_PEI_BOOT_SCRIPT_EXECUTER_PPI EFI_PEI_BOOT_SCRIPT_EXECUTER_PPI;

/**
  Executes the Framework boot script table.

  @param  PeiServices    A pointer to the system PEI Services Table.
  @param  This           A pointer to the EFI_PEI_BOOT_SCRIPT_EXECUTER_PPI instance.
  @param  Address        The physical memory address where the table is stored.
                         It must be zero if the table to be executed is stored in a firmware volume file.
  @param  FvFile         The firmware volume file name that contains the table to
                         be executed. It must be NULL if the table to be executed is stored in physical memory.

  @retval EFI_SUCCESS           The boot script table was executed successfully.
  @retval EFI_INVALID_PARAMETER Address is zero and FvFile is NULL.
  @retval EFI_NOT_FOUND         The file name specified in FvFile cannot be found.
  @retval EFI_UNSUPPORTED       The format of the boot script table is invalid.
                                Or An unsupported opcode occurred in the table.
                                Or There were opcode execution errors, such as an insufficient dependency.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PEI_BOOT_SCRIPT_EXECUTE)(
  IN     EFI_PEI_SERVICES                        **PeiServices,
  IN     EFI_PEI_BOOT_SCRIPT_EXECUTER_PPI        *This,
  IN     EFI_PHYSICAL_ADDRESS                    Address,
  IN     EFI_GUID                                *FvFile OPTIONAL
  );

///
/// EFI_PEI_BOOT_SCRIPT_EXECUTER_PPI produces the function which interprets and 
/// executes the Framework boot script table
///
struct _EFI_PEI_BOOT_SCRIPT_EXECUTER_PPI {
	///
	/// Executes a boot script table
	///
  EFI_PEI_BOOT_SCRIPT_EXECUTE Execute;  
};

extern EFI_GUID gEfiPeiBootScriptExecuterPpiGuid;

#endif
