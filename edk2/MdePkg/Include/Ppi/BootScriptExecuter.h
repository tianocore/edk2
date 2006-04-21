/** @file
  This file declares Boot Script Executer PPI.

  Copyright (c) 2006, Intel Corporation                                                         
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

#ifndef _PEI_BOOT_SCRIPT_EXECUTER_PPI_H
#define _PEI_BOOT_SCRIPT_EXECUTER_PPI_H

#define EFI_PEI_BOOT_SCRIPT_EXECUTER_PPI_GUID \
  { \
    0xabd42895, 0x78cf, 0x4872, {0x84, 0x44, 0x1b, 0x5c, 0x18, 0x0b, 0xfb, 0xff } \
  }

typedef struct _EFI_PEI_BOOT_SCRIPT_EXECUTER_PPI EFI_PEI_BOOT_SCRIPT_EXECUTER_PPI;

/**
  Executes the Framework boot script table.

  @param  PeiServices A pointer to the system PEI Services Table.
  
  @param  This A pointer to the EFI_PEI_BOOT_SCRIPT_EXECUTER_PPI instance.
  
  @param  Address The physical memory address where the table is stored. 
  It must be zero if the table to be executed is stored in a firmware volume file.
  
  @param  FvFile The firmware volume file name that contains the table to 
  be executed. It must be NULL if the table to be executed is stored in physical memory.

  @retval EFI_SUCCESS The boot script table was executed successfully.
  
  @retval EFI_INVALID_PARAMETER Address is zero and FvFile is NULL.
  
  @retval EFI_NOT_FOUND The file name specified in FvFile cannot be found.
  
  @retval EFI_UNSUPPORTED The format of the boot script table is invalid.
  Or An unsupported opcode occurred in the table.
  Or There were opcode execution errors, such as an insufficient dependency.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_PEI_BOOT_SCRIPT_EXECUTE) (
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
