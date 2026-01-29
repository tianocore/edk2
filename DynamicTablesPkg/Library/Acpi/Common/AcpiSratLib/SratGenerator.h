/** @file
  SRAT Table Generator

  Copyright (c) 2019 - 2020, Arm Limited. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - ACPI 6.3 Specification, January 2019

  @par Glossary:
  - Cm or CM   - Configuration Manager
  - Obj or OBJ - Object
**/

#ifndef SRAT_GENERATOR_H_
#define SRAT_GENERATOR_H_

/** Reserve arch sub-tables space.

  @param [in] CfgMgrProtocol   Pointer to the Configuration Manager
  @param [in, out] ArchOffset  On input, contains the offset where arch specific
                               sub-tables can be written. It is expected that
                               there enough space to write all the arch specific
                               sub-tables from this offset onward.
                               On ouput, contains the ending offset of the arch
                               specific sub-tables.

  @retval EFI_SUCCESS           Table generated successfully.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         The required object information is not found.
  @retval EFI_BAD_BUFFER_SIZE   The size returned by the Configuration
                                Manager is less than the Object size for the
                                requested object.
**/
EFI_STATUS
EFIAPI
ArchReserveOffsets (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN OUT UINT32                                           *ArchOffset
  );

/** Add the arch specific sub-tables to the SRAT table.

  These sub-tables are written in the space reserved beforehand.

  @param [in]  CfgMgrProtocol   Pointer to the Configuration Manager
                                Protocol Interface.
  @param [in]  Srat             Pointer to the SRAT Table.

  @retval EFI_SUCCESS           Table generated successfully.
**/
EFI_STATUS
EFIAPI
AddArchObjects (
  IN CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL         *CONST  CfgMgrProtocol,
  IN EFI_ACPI_6_3_SYSTEM_RESOURCE_AFFINITY_TABLE_HEADER *CONST  Srat
  );

#endif // SRAT_GENERATOR_H_
