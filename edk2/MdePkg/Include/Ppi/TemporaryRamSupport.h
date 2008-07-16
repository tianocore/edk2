/** @file
  This file declares Temporary RAM Support PPI.
  This Ppi provides the service that migrates temporary RAM into permanent memory.

  Copyright (c) 2006 - 2008, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  @par Revision Reference:
  This PPI is defined in PI.
  Version 1.00.

**/

#ifndef __TEMPORARY_RAM_SUPPORT_H__
#define __TEMPORARY_RAM_SUPPORT_H__

#define TEMPORARY_RAM_SUPPORT_PPI_GUID \
  { 0xdbe23aa9, 0xa345, 0x4b97, {0x85, 0xb6, 0xb2, 0x26, 0xf1, 0x61, 0x73, 0x89} }


/*
  This service of the TEMPORARY_RAM_SUPPORT_PPI that migrates temporary RAM into
  permanent memory.

  @param PeiServices            Pointer to the PEI Services Table.

  @param TemporaryMemoryBase    Source Address in temporary memory from which the SEC or PEIM will copy the
                                Temporary RAM contents.

  @param PermanentMemoryBase    Destination Address in permanent memory into which the SEC or PEIM will copy the
                                Temporary RAM contents.

  @param CopySize               Amount of memory to migrate from temporary to permanent memory.



  @retval EFI_SUCCESS           The data was successfully returned.

  @retval EFI_INVALID_PARAMETER PermanentMemoryBase + CopySize >
                                TemporaryMemoryBase when TemporaryMemoryBase >
                                PermanentMemoryBase.

**/
typedef
EFI_STATUS
(EFIAPI * TEMPORARY_RAM_MIGRATION)(
  IN CONST EFI_PEI_SERVICES   **PeiServices,
  IN EFI_PHYSICAL_ADDRESS     TemporaryMemoryBase,
  IN EFI_PHYSICAL_ADDRESS     PermanentMemoryBase,
  IN UINTN                    CopySize
);

/**
  This service abstracts the ability to migrate contents of the platform early memory store.

  @param ResetSystem
  Perform the migration of contents of Temporary RAM to Permanent RAM.
  Terminate the Temporary RAM if it cannot coexist with the Permanent RAM.

**/
typedef struct {
  TEMPORARY_RAM_MIGRATION   TemporaryRamMigration;
} TEMPORARY_RAM_SUPPORT_PPI;

extern EFI_GUID gEfiTemporaryRamSupportPpiGuid;

#endif
