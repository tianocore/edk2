/** @file

  Copyright (c) 2017 - 2019, ARM Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef SMBIOS_TABLE_GENERATOR_H_
#define SMBIOS_TABLE_GENERATOR_H_

#include <IndustryStandard/SmBios.h>

#include <TableGenerator.h>

#pragma pack(1)

/** The SMBIOS_TABLE_GENERATOR_ID type describes SMBIOS table generator ID.
*/
typedef TABLE_GENERATOR_ID SMBIOS_TABLE_GENERATOR_ID;

/** The ESTD_SMBIOS_TABLE_ID enum describes the SMBIOS table IDs reserved for
  the standard generators.

  NOTE: The SMBIOS Generator IDs do not match the table type numbers!
          This allows 0 to be used to catch invalid parameters.
*/
typedef enum StdSmbiosTableGeneratorId {
  EStdSmbiosTableIdReserved = 0x0000,
  EStdSmbiosTableIdRAW,
  EStdSmbiosTableIdType00,
  EStdSmbiosTableIdType01,
  EStdSmbiosTableIdType02,
  EStdSmbiosTableIdType03,
  EStdSmbiosTableIdType04,
  EStdSmbiosTableIdType05,
  EStdSmbiosTableIdType06,
  EStdSmbiosTableIdType07,
  EStdSmbiosTableIdType08,
  EStdSmbiosTableIdType09,
  EStdSmbiosTableIdType10,
  EStdSmbiosTableIdType11,
  EStdSmbiosTableIdType12,
  EStdSmbiosTableIdType13,
  EStdSmbiosTableIdType14,
  EStdSmbiosTableIdType15,
  EStdSmbiosTableIdType16,
  EStdSmbiosTableIdType17,
  EStdSmbiosTableIdType18,
  EStdSmbiosTableIdType19,
  EStdSmbiosTableIdType20,
  EStdSmbiosTableIdType21,
  EStdSmbiosTableIdType22,
  EStdSmbiosTableIdType23,
  EStdSmbiosTableIdType24,
  EStdSmbiosTableIdType25,
  EStdSmbiosTableIdType26,
  EStdSmbiosTableIdType27,
  EStdSmbiosTableIdType28,
  EStdSmbiosTableIdType29,
  EStdSmbiosTableIdType30,
  EStdSmbiosTableIdType31,
  EStdSmbiosTableIdType32,
  EStdSmbiosTableIdType33,
  EStdSmbiosTableIdType34,
  EStdSmbiosTableIdType35,
  EStdSmbiosTableIdType36,
  EStdSmbiosTableIdType37,
  EStdSmbiosTableIdType38,
  EStdSmbiosTableIdType39,
  EStdSmbiosTableIdType40,
  EStdSmbiosTableIdType41,
  EStdSmbiosTableIdType42,

  // IDs 43 - 125 are reserved

  EStdSmbiosTableIdType126 = (EStdSmbiosTableIdType00 + 126),
  EStdSmbiosTableIdType127,
  EStdSmbiosTableIdMax
} ESTD_SMBIOS_TABLE_ID;

/** This macro checks if the Table Generator ID is for an SMBIOS Table
    Generator.

  @param [in] TableGeneratorId  The table generator ID.

  @return  TRUE if the table generator ID is for an SMBIOS Table
            Generator.
**/
#define IS_GENERATOR_TYPE_SMBIOS(TableGeneratorId) \
          (                                        \
          GET_TABLE_TYPE (TableGeneratorId) ==     \
          ETableGeneratorTypeSmbios                \
          )

/** This macro checks if the Table Generator ID is for a standard SMBIOS
    Table Generator.

  @param [in] TableGeneratorId  The table generator ID.

  @return  TRUE if the table generator ID is for a standard SMBIOS
            Table Generator.
**/
#define IS_VALID_STD_SMBIOS_GENERATOR_ID(TableGeneratorId)          \
          (                                                         \
          IS_GENERATOR_NAMESPACE_STD(TableGeneratorId) &&           \
          IS_GENERATOR_TYPE_SMBIOS(TableGeneratorId)   &&           \
          ((GET_TABLE_ID(GeneratorId) >= EStdSmbiosTableIdRaw) &&   \
           (GET_TABLE_ID(GeneratorId) < EStdSmbiosTableIdMax))      \
          )

/** This macro creates a standard SMBIOS Table Generator ID.

  @param [in] TableId  The table generator ID.

  @return a standard SMBIOS table generator ID.
**/
#define CREATE_STD_SMBIOS_TABLE_GEN_ID(TableId) \
          CREATE_TABLE_GEN_ID (                 \
            ETableGeneratorTypeSmbios,          \
            ETableGeneratorNameSpaceStd,        \
            TableId                             \
            )

/** Forward declarations.
*/
typedef struct ConfigurationManagerProtocol EDKII_CONFIGURATION_MANAGER_PROTOCOL;
typedef struct CmStdObjSmbiosTableInfo      CM_STD_OBJ_SMBIOS_TABLE_INFO;
typedef struct SmbiosTableGenerator         SMBIOS_TABLE_GENERATOR;

/** This function pointer describes the interface to SMBIOS table build
    functions provided by the SMBIOS table generator and called by the
    Table Manager to build an SMBIOS table.

  @param [in]  Generator       Pointer to the SMBIOS table generator.
  @param [in]  SmbiosTableInfo Pointer to the SMBIOS table information.
  @param [in]  CfgMgrProtocol  Pointer to the Configuration Manager
                               Protocol interface.
  @param [out] Table           Pointer to the generated SMBIOS table.

  @return EFI_SUCCESS  If the table is generated successfully or other
                        failure codes as returned by the generator.
**/
typedef EFI_STATUS (*SMBIOS_TABLE_GENERATOR_BUILD_TABLE) (
  IN  CONST SMBIOS_TABLE_GENERATOR                        *Generator,
  IN        CM_STD_OBJ_SMBIOS_TABLE_INFO          *CONST  SmbiosTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  OUT       SMBIOS_STRUCTURE                              **Table
  );

/** This function pointer describes the interface to used by the
    Table Manager to give the generator an opportunity to free
    any resources allocated for building the SMBIOS table.

  @param [in]  Generator       Pointer to the SMBIOS table generator.
  @param [in]  SmbiosTableInfo Pointer to the SMBIOS table information.
  @param [in]  CfgMgrProtocol  Pointer to the Configuration Manager
                               Protocol interface.
  @param [in]  Table           Pointer to the generated SMBIOS table.

  @return  EFI_SUCCESS If freed successfully or other failure codes
                        as returned by the generator.
**/
typedef EFI_STATUS (*SMBIOS_TABLE_GENERATOR_FREE_TABLE) (
  IN  CONST SMBIOS_TABLE_GENERATOR                        *Generator,
  IN  CONST CM_STD_OBJ_SMBIOS_TABLE_INFO          *CONST  SmbiosTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN        SMBIOS_STRUCTURE                              **Table
  );

/** The SMBIOS_TABLE_GENERATOR structure provides an interface that the
    Table Manager can use to invoke the functions to build SMBIOS tables.
*/
typedef struct SmbiosTableGenerator {
  /// The SMBIOS table generator ID.
  SMBIOS_TABLE_GENERATOR_ID             GeneratorID;

  /// String describing the DT table
  /// generator.
  CONST CHAR16                          *Description;

  /// The SMBIOS table type.
  SMBIOS_TYPE                           Type;

  /// SMBIOS table build function pointer.
  SMBIOS_TABLE_GENERATOR_BUILD_TABLE    BuildSmbiosTable;

  /** The function to free any resources
      allocated for building the SMBIOS table.
  */
  SMBIOS_TABLE_GENERATOR_FREE_TABLE     FreeTableResources;
} SMBIOS_TABLE_GENERATOR;

/** Register SMBIOS table factory generator.

  The SMBIOS table factory maintains a list of the Standard and OEM SMBIOS
  table generators.

  @param [in]  Generator       Pointer to the SMBIOS table generator.

  @retval EFI_SUCCESS           The Generator was registered
                                successfully.
  @retval EFI_INVALID_PARAMETER The Generator ID is invalid or
                                the Generator pointer is NULL.
  @retval EFI_ALREADY_STARTED   The Generator for the Table ID is
                                already registered.
**/
EFI_STATUS
EFIAPI
RegisterSmbiosTableGenerator (
  IN CONST SMBIOS_TABLE_GENERATOR                 *CONST  Generator
  );

/** Deregister SMBIOS generator.

  This function is called by the SMBIOS table generator to deregister itself
  from the SMBIOS table factory.

  @param [in]  Generator       Pointer to the SMBIOS table generator.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER The generator is invalid.
  @retval EFI_NOT_FOUND         The requested generator is not found
                                in the list of registered generators.
**/
EFI_STATUS
EFIAPI
DeregisterSmbiosTableGenerator (
  IN CONST SMBIOS_TABLE_GENERATOR                 *CONST  Generator
  );

#pragma pack()

#endif // SMBIOS_TABLE_GENERATOR_H_
