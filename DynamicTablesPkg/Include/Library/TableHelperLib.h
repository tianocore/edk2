/** @file

  Copyright (c) 2017 - 2021, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - PFN   - Pointer to a Function

**/

#ifndef TABLE_HELPER_LIB_H_
#define TABLE_HELPER_LIB_H_

#include <Library/AmlLib/AmlLib.h>

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

/** Build a RootNode containing SSDT ACPI header information using the AmlLib.

  The function utilizes the ACPI table Generator and the Configuration
  Manager protocol to obtain any information required for constructing the
  header. It then creates a RootNode. The SSDT ACPI header is part of the
  RootNode.

  This is essentially a wrapper around AmlCodeGenDefinitionBlock ()
  from the AmlLib.

  @param [in]   CfgMgrProtocol Pointer to the Configuration Manager
                               protocol interface.
  @param [in]   Generator      Pointer to the ACPI table Generator.
  @param [in]   AcpiTableInfo  Pointer to the ACPI table info structure.
  @param [out]  RootNode       If success, contains the created RootNode.
                               The SSDT ACPI header is part of the RootNode.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         The required object information is not found.
  @retval EFI_BAD_BUFFER_SIZE   The size returned by the Configuration
                                Manager is less than the Object size for the
                                requested object.
**/
EFI_STATUS
EFIAPI
AddSsdtAcpiHeader (
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  * CONST CfgMgrProtocol,
  IN      CONST ACPI_TABLE_GENERATOR                  * CONST Generator,
  IN      CONST CM_STD_OBJ_ACPI_TABLE_INFO            * CONST AcpiTableInfo,
      OUT       AML_ROOT_NODE_HANDLE                  *       RootNode
  );

/**
  Function prototype for testing if two arbitrary objects are equal.

  @param [in] Object1           Pointer to the first object to compare.
  @param [in] Object2           Pointer to the second object to compare.
  @param [in] Index1            Index of Object1. This value is optional and
                                can be ignored by the specified implementation.
  @param [in] Index2            Index of Object2. This value is optional and
                                can be ignored by the specified implementation.

  @retval TRUE                  Object1 and Object2 are equal.
  @retval FALSE                 Object1 and Object2 are NOT equal.
**/
typedef
BOOLEAN
(EFIAPI *PFN_IS_EQUAL)(
  IN CONST  VOID            * Object1,
  IN CONST  VOID            * Object2,
  IN        UINTN             Index1 OPTIONAL,
  IN        UINTN             Index2 OPTIONAL
  );

/**
  Test and report if a duplicate entry exists in the given array of comparable
  elements.

  @param [in] Array                 Array of elements to test for duplicates.
  @param [in] Count                 Number of elements in Array.
  @param [in] ElementSize           Size of an element in bytes
  @param [in] EqualTestFunction     The function to call to check if any two
                                    elements are equal.

  @retval TRUE                      A duplicate element was found or one of
                                    the input arguments is invalid.
  @retval FALSE                     Every element in Array is unique.
**/
BOOLEAN
EFIAPI
FindDuplicateValue (
  IN  CONST VOID          * Array,
  IN  CONST UINTN           Count,
  IN  CONST UINTN           ElementSize,
  IN        PFN_IS_EQUAL    EqualTestFunction
  );

/** Parse and print a CmObjDesc.

  @param [in]  CmObjDesc  The CmObjDesc to parse and print.
**/
VOID
EFIAPI
ParseCmObjDesc (
  IN  CONST CM_OBJ_DESCRIPTOR * CmObjDesc
  );

#endif // TABLE_HELPER_LIB_H_
