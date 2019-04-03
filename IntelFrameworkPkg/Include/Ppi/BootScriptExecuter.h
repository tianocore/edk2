/** @file
  This file declares the Boot Script Executer PPI.

  This PPI is published by a PEIM upon dispatch and provides an execution engine for the
  Framework boot script. This PEIM should be platform neutral and have no specific knowledge of
  platform instructions or other information. The ability to interpret the boot script depends on the
  abundance of other PPIs that are available. For example, if the script requests an SMBus command
  execution, the PEIM looks for a relevant PPI that is available to execute it, rather than executing it
  by issuing the native IA-32 instruction.

Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Revision Reference:
  This PPI is defined in Framework of EFI BootScript spec.
  Version 0.91.

**/

#ifndef _PEI_BOOT_SCRIPT_EXECUTER_PPI_H_
#define _PEI_BOOT_SCRIPT_EXECUTER_PPI_H_

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
                         It must be zero if the table to be executed is stored in
                         a firmware volume file.
  @param  FvFile         The firmware volume file name that contains the table to
                         be executed. It must be NULL if the table to be executed
                         is stored in physical memory.

  @retval EFI_SUCCESS           The boot script table was executed successfully.
  @retval EFI_INVALID_PARAMETER Address is zero and FvFile is NULL.
  @retval EFI_NOT_FOUND         The file name specified in FvFile cannot be found.
  @retval EFI_UNSUPPORTED       The format of the boot script table is invalid.
                                Or, an unsupported opcode occurred in the table.
                                Or there were opcode execution errors, such as an
                                insufficient dependency.

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
/// executes the Framework boot script table.
///
struct _EFI_PEI_BOOT_SCRIPT_EXECUTER_PPI {
  ///
  /// Executes a boot script table.
  ///
  EFI_PEI_BOOT_SCRIPT_EXECUTE Execute;
};

extern EFI_GUID gEfiPeiBootScriptExecuterPpiGuid;

#endif
