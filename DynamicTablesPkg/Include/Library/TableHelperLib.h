/** @file

  Copyright (c) 2017 - 2019, ARM Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef TABLE_HELPER_LIB_H_
#define TABLE_HELPER_LIB_H_

/** The GetCgfMgrInfo function gets the CM_STD_OBJ_CONFIGURATION_MANAGER_INFO
    object from the Configuration Manager.

  @param [in]  CfgMgrProtocol Pointer to the Configuration Manager protocol
                              interface.
  @param [out] CfgMfrInfo     Pointer to the Configuration Manager Info
                              object structure.

  @retval EFI_SUCCESS           The object is returned.
  @retval EFI_INVALID_PARAMETER The Object ID is invalid.
  @retval EFI_NOT_FOUND         The requested Object is not found.
  @retval EFI_BAD_BUFFER_SIZE   The size returned by the Configuration
                                Manager is less than the Object size.
**/
EFI_STATUS
EFIAPI
GetCgfMgrInfo (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL      * CONST  CfgMgrProtocol,
  OUT       CM_STD_OBJ_CONFIGURATION_MANAGER_INFO    **        CfgMfrInfo
  );

/** The AddAcpiHeader function updates the ACPI header structure. It uses the
    ACPI table Generator and the Configuration Manager protocol to obtain the
    information required for constructing the header.

  @param [in]     CfgMgrProtocol Pointer to the Configuration Manager
                                 protocol interface.
  @param [in]     Generator      Pointer to the ACPI table Generator.
  @param [in,out] AcpiHeader     Pointer to the ACPI table header to be
                                 updated.
  @param [in]     AcpiTableInfo  Pointer to the ACPI table info structure.
  @param [in]     Length         Length of the ACPI table.

  @retval EFI_SUCCESS           The ACPI table is updated successfully.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         The required object information is not found.
  @retval EFI_BAD_BUFFER_SIZE   The size returned by the Configuration
                                Manager is less than the Object size for the
                                requested object.
**/
EFI_STATUS
EFIAPI
AddAcpiHeader (
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  * CONST CfgMgrProtocol,
  IN      CONST ACPI_TABLE_GENERATOR                  * CONST Generator,
  IN OUT  EFI_ACPI_DESCRIPTION_HEADER                 * CONST AcpiHeader,
  IN      CONST CM_STD_OBJ_ACPI_TABLE_INFO            * CONST AcpiTableInfo,
  IN      CONST UINT32                                        Length
  );

#endif // TABLE_HELPER_LIB_H_
