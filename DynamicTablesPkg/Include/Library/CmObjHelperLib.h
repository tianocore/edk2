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

#endif // CM_OBJ_HELPER_LIB_H_
