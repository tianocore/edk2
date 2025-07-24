/** @file
  Configuration Manager Helper Library.

  Copyright (c) 2025, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef CM_OBJ_HELPER_LIB_H_
#define CM_OBJ_HELPER_LIB_H_

/** Check if an ACPI table is present in the Configuration manager's ACPI table list.

  @param [in]  CfgMgrProtocol         Pointer to the Configuration Manager
                                      Protocol Interface.
  @param [in]  AcpiTableId            Acpi Table Id.

  @retval TRUE if the ACPI table is in the list of ACPI tables to install.
          FALSE otherwise.
**/
BOOLEAN
EFIAPI
CheckAcpiTablePresent (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN        ESTD_ACPI_TABLE_ID                            AcpiTableId
  );

/** Get a Proximity Domain Id.

  Proximity Domain Id are now to be placed in
  CM_ARCH_COMMON_PROXIMITY_DOMAIN_INFO objects rather than in the various
  CmObj using them. This function handles the logic in the selection
  of the ProximityDomainId to use.

  Proximity Domain Id should be preferably placed in
  CM_ARCH_COMMON_PROXIMITY_DOMAIN_INFO objects now.

  @param [in]  CfgMgrProtocol   Pointer to the Configuration Manager
                                Protocol Interface.
  @param [in]  DefaultDomainId  Default per-CmObj Proximity Domain Id.
                                The CM_ARCH_COMMON_PROXIMITY_DOMAIN_INFO
                                should be preferably used.
  @param [in]  Token            Token referencing a
                                CM_ARCH_COMMON_PROXIMITY_DOMAIN_INFO object.
  @param [out] DomainId         If Success, contains DomainId to use.

  @retval EFI_SUCCESS           Table generated successfully.
  @retval EFI_INVALID_PARAMETER The table pointer is NULL or invalid.
**/
EFI_STATUS
EFIAPI
GetProximityDomainId (
  IN CONST  EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CfgMgrProtocol,
  IN        UINT32                                DefaultDomainId,
  IN        CM_OBJECT_TOKEN                       Token,
  OUT       UINT32                                *DomainId
  );

#endif // CM_OBJ_HELPER_LIB_H_
