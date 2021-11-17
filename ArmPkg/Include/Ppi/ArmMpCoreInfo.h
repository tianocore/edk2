/** @file
*
*  Copyright (c) 2011, ARM Limited. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#ifndef ARM_MP_CORE_INFO_PPI_H_
#define ARM_MP_CORE_INFO_PPI_H_

#include <Guid/ArmMpCoreInfo.h>

#define ARM_MP_CORE_INFO_PPI_GUID  \
  { 0x6847cc74, 0xe9ec, 0x4f8f, {0xa2, 0x9d, 0xab, 0x44, 0xe7, 0x54, 0xa8, 0xfc} }

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
(EFIAPI *ARM_MP_CORE_INFO_GET)(
  OUT UINTN                   *ArmCoreCount,
  OUT ARM_CORE_INFO           **ArmCoreTable
  );

///
/// This service abstracts the ability to migrate contents of the platform early memory store.
/// Note: The name EFI_PEI_TEMPORARY_RAM_SUPPORT_PPI is different from the current PI 1.2 spec.
///       This PPI was optional.
///
typedef struct {
  ARM_MP_CORE_INFO_GET    GetMpCoreInfo;
} ARM_MP_CORE_INFO_PPI;

extern EFI_GUID  gArmMpCoreInfoPpiGuid;
extern EFI_GUID  gArmMpCoreInfoGuid;

#endif // ARM_MP_CORE_INFO_PPI_H_
