/** @file
  This file declares Boot Script Executer PPI.

  This PPI is published by a PEIM upon dispatch and provides an execution engine for the
  Framework boot script. This PEIM should be platform neutral and have no specific knowledge of
  platform instructions and other information. The ability to interpret the boot script depends on the
  abundance of other PPIs that are available. For example, if the script requests an SMBus command
  execution, the PEIM looks for a relevant PPI that is available to execute it, rather than executing it
  by issuing the native IA-32 instruction.

  Copyright (c) 2007, Intel Corporation
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

#define EFI_BOOT_SCRIPT_IO_WRITE_OPCODE               0x00
#define EFI_BOOT_SCRIPT_IO_READ_WRITE_OPCODE          0x01
#define EFI_BOOT_SCRIPT_MEM_WRITE_OPCODE              0x02
#define EFI_BOOT_SCRIPT_MEM_READ_WRITE_OPCODE         0x03
#define EFI_BOOT_SCRIPT_PCI_CONFIG_WRITE_OPCODE       0x04
#define EFI_BOOT_SCRIPT_PCI_CONFIG_READ_WRITE_OPCODE  0x05
#define EFI_BOOT_SCRIPT_SMBUS_EXECUTE_OPCODE          0x06
#define EFI_BOOT_SCRIPT_STALL_OPCODE                  0x07
#define EFI_BOOT_SCRIPT_DISPATCH_OPCODE               0x08

//
// Extensions to boot script definitions
//
#define EFI_BOOT_SCRIPT_MEM_POLL_OPCODE               0x09
#define EFI_BOOT_SCRIPT_INFORMATION_OPCODE            0x0A
#define EFI_BOOT_SCRIPT_PCI_CONFIG2_WRITE_OPCODE      0x0B
#define EFI_BOOT_SCRIPT_PCI_CONFIG2_READ_WRITE_OPCODE 0x0C
#define EFI_BOOT_SCRIPT_DISPATCH_2_OPCODE             0x0D
#define EFI_BOOT_SCRIPT_TABLE_OPCODE                  0xAA
#define EFI_BOOT_SCRIPT_TERMINATE_OPCODE              0xFF

//
// EFI Boot Script Width
//
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

/**
  @par Ppi Description:
  This PPI produces functions to interpret and execute the Framework boot script table.

  @param Execute
  Executes a boot script table.

**/
struct _EFI_PEI_BOOT_SCRIPT_EXECUTER_PPI {
  EFI_PEI_BOOT_SCRIPT_EXECUTE Execute;
};

extern EFI_GUID gEfiPeiBootScriptExecuterPpiGuid;

#endif
