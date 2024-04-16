/** @file

  Copyright (c) 2021 - 2022, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Cm or CM   - Configuration Manager
    - Obj or OBJ - Object
**/

#ifndef CONFIGURATION_MANAGER_H_
#define CONFIGURATION_MANAGER_H_

///
/// C array containing the compiled AML template.
/// This symbol is defined in the auto generated C file
/// containing the AML bytecode array.
///
extern CHAR8  dsdt_aml_code[];

///
/// The configuration manager version.
///
#define CONFIGURATION_MANAGER_REVISION  CREATE_REVISION (1, 0)

///
/// The OEM ID
///
#define CFG_MGR_OEM_ID  { 'A', 'R', 'M', 'L', 'T', 'D' }

///
/// Memory address size limit. Assume the whole address space.
///
#define MEMORY_ADDRESS_SIZE_LIMIT  64

/** A function that prepares Configuration Manager Objects for returning.

  @param [in]  This        Pointer to the Configuration Manager Protocol.
  @param [in]  CmObjectId  The Configuration Manager Object ID.
  @param [in]  Token       A token for identifying the object.
  @param [out] CmObject    Pointer to the Configuration Manager Object
                           descriptor describing the requested Object.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         The required object information is not found.
**/
typedef EFI_STATUS (*CM_OBJECT_HANDLER_PROC) (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  This,
  IN  CONST CM_OBJECT_ID                                  CmObjectId,
  IN  CONST CM_OBJECT_TOKEN                               Token,
  IN  OUT   CM_OBJ_DESCRIPTOR                     *CONST  CmObject
  );

///
/// A helper macro for mapping a reference token.
///
#define REFERENCE_TOKEN(Field)                                     \
          (CM_OBJECT_TOKEN)((UINT8*)&mKvmtoolPlatRepositoryInfo +  \
           OFFSET_OF (EDKII_PLATFORM_REPOSITORY_INFO, Field))

///
/// The number of ACPI tables to install
///
#define PLAT_ACPI_TABLE_COUNT  10

///
/// A structure describing the platform configuration
/// manager repository information
///
typedef struct PlatformRepositoryInfo {
  ///
  /// Configuration Manager Information.
  ///
  CM_STD_OBJ_CONFIGURATION_MANAGER_INFO           CmInfo;

  ///
  /// List of ACPI tables
  ///
  CM_STD_OBJ_ACPI_TABLE_INFO                      CmAcpiTableList[PLAT_ACPI_TABLE_COUNT];

  ///
  /// Power management profile information
  ///
  CM_ARCH_COMMON_POWER_MANAGEMENT_PROFILE_INFO    PmProfileInfo;

  ///
  /// ITS Group node
  ///
  CM_ARM_ITS_GROUP_NODE                           ItsGroupInfo;

  ///
  /// ITS Identifier array
  ///
  CM_ARM_ITS_IDENTIFIER                           ItsIdentifierArray[1];

  ///
  /// PCI Root complex node
  ///
  CM_ARM_ROOT_COMPLEX_NODE                        RootComplexInfo;

  ///
  /// Array of DeviceID mapping
  ///
  CM_ARM_ID_MAPPING                               DeviceIdMapping[1];

  ///
  /// Dynamic platform repository.
  /// CmObj created by parsing the Kvmtool device tree are stored here.
  ///
  DYNAMIC_PLATFORM_REPOSITORY_INFO                *DynamicPlatformRepo;

  ///
  /// Base address of the FDT.
  ///
  VOID                                            *FdtBase;

  ///
  /// A handle to the FDT HwInfoParser.
  ///
  HW_INFO_PARSER_HANDLE                           FdtParserHandle;
} EDKII_PLATFORM_REPOSITORY_INFO;

#endif // CONFIGURATION_MANAGER_H_
