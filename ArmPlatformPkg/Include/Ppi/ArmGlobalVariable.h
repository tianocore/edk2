/** @file
*
*  Copyright (c) 2011, ARM Limited. All rights reserved.
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

#ifndef __ARM_GLOBAL_VARIABLE_H__
#define __ARM_GLOBAL_VARIABLE_H__

#define ARM_GLOBAL_VARIABLE_PPI_GUID  \
  { 0xab1c1816, 0xd542, 0x4e6f, {0x9b, 0x1e, 0x8e, 0xcd, 0x92, 0x53, 0xe2, 0xe7} }


/**
  This service of the EFI_PEI_TEMPORARY_RAM_SUPPORT_PPI that migrates temporary RAM into
  permanent memory.

  @param PeiServices            Pointer to the PEI Services Table.
  @param TemporaryMemoryBase    Source Address in temporary memory from which the SEC or PEIM will copy the
                                Temporary RAM contents.
  @param PermanentMemoryBase    Destination Address in permanent memory into which the SEC or PEIM will copy the
                                Temporary RAM contents.
  @param CopySize               Amount of memory to migrate from temporary to permanent memory.

  @retval EFI_SUCCESS           The data was successfully returned.
  @retval EFI_INVALID_PARAMETER PermanentMemoryBase + CopySize > TemporaryMemoryBase when
                                TemporaryMemoryBase > PermanentMemoryBase.

**/
typedef
EFI_STATUS
(EFIAPI * ARM_GLOBAL_VARIABLE_GET_MEMORY) (
  OUT EFI_PHYSICAL_ADDRESS    *GlobalVariableBase
);

///
/// This service abstracts the ability to migrate contents of the platform early memory store.
/// Note: The name EFI_PEI_TEMPORARY_RAM_SUPPORT_PPI is different from the current PI 1.2 spec.
///       This PPI was optional.
///
typedef struct {
  ARM_GLOBAL_VARIABLE_GET_MEMORY   GetGlobalVariableMemory;
} ARM_GLOBAL_VARIABLE_PPI;

extern EFI_GUID gArmGlobalVariablePpiGuid;

#endif
