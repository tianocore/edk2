/** @file

  Copyright (c) 2017 - 2019, ARM Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Cm or CM   - Configuration Manager
    - Obj or OBJ - Object
    - Std or STD - Standard
**/

#ifndef DEVICETREE_TABLE_GENERATOR_H_
#define DEVICETREE_TABLE_GENERATOR_H_

#include <TableGenerator.h>

#pragma pack(1)

/** The DT_TABLE_GENERATOR_ID type describes Device Tree table generator ID.
*/
typedef TABLE_GENERATOR_ID DT_TABLE_GENERATOR_ID;

/** The ESTD_DT_TABLE_ID enum describes the DT table IDs reserved for
  the standard generators.
*/
typedef enum StdDtTableId {
  EStdDtTableIdReserved = 0x0000,             ///< Reserved.
  EStdDtTableIdRaw,                           ///< RAW Generator.
  EStdDtTableIdMax
} ESTD_DT_TABLE_ID;

/** This macro checks if the Table Generator ID is for an DT Table Generator.

  @param [in] TableGeneratorId  The table generator ID.

  @return TRUE if the table generator ID is for an DT Table
            Generator.
**/
#define IS_GENERATOR_TYPE_DT(TableGeneratorId) \
          (GET_TABLE_TYPE(TableGeneratorId) == ETableGeneratorTypeDt)

/** This macro checks if the Table Generator ID is for a standard DT
    Table Generator.

  @param [in] TableGeneratorId  The table generator ID.

  @return TRUE if the table generator ID is for a standard DT
            Table Generator.
**/
#define IS_VALID_STD_DT_GENERATOR_ID(TableGeneratorId)           \
          (                                                      \
          IS_GENERATOR_NAMESPACE_STD(TableGeneratorId) &&        \
          IS_GENERATOR_TYPE_DT(TableGeneratorId)       &&        \
          ((GET_TABLE_ID(GeneratorId) >= EStdDtTableIdRaw) &&    \
           (GET_TABLE_ID(GeneratorId) < EStdDtTableIdMax))       \
          )

/** This macro creates a standard DT Table Generator ID.

  @param [in] TableId  The table generator ID.

  @return a standard DT table generator ID.
**/
#define CREATE_STD_DT_TABLE_GEN_ID(TableId) \
          CREATE_TABLE_GEN_ID (             \
            ETableGeneratorTypeDt,          \
            ETableGeneratorNameSpaceStd,    \
            TableId                         \
            )

/** Forward declarations.
*/
typedef struct ConfigurationManagerProtocol EDKII_CONFIGURATION_MANAGER_PROTOCOL;
typedef struct CmAStdObjDtTableInfo         CM_STD_OBJ_DT_TABLE_INFO;
typedef struct DtTableGenerator             DT_TABLE_GENERATOR;

/** This function pointer describes the interface to DT table build
    functions provided by the DT table generator and called by the
    Table Manager to build an DT table.

  @param [in]  Generator       Pointer to the DT table generator.
  @param [in]  DtTableInfo     Pointer to the DT table information.
  @param [in]  CfgMgrProtocol  Pointer to the Configuration Manager
                               Protocol interface.
  @param [out] Table           Pointer to the generated DT table.

  @return EFI_SUCCESS  If the table is generated successfully or other
                        failure codes as returned by the generator.
**/
typedef EFI_STATUS (*DT_TABLE_GENERATOR_BUILD_TABLE) (
  IN  CONST DT_TABLE_GENERATOR                    *       Generator,
  IN  CONST CM_STD_OBJ_DT_TABLE_INFO              * CONST DtTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  * CONST CfgMgrProtocol,
  OUT       VOID                                 **       Table
  );

/** This function pointer describes the interface to used by the
    Table Manager to give the generator an opportunity to free
    any resources allocated for building the DT table.

  @param [in]  Generator       Pointer to the DT table generator.
  @param [in]  DtTableInfo     Pointer to the DT table information.
  @param [in]  CfgMgrProtocol  Pointer to the Configuration Manager
                               Protocol interface.
  @param [in]  Table           Pointer to the generated DT table.

  @return EFI_SUCCESS  If freed successfully or other failure codes
                        as returned by the generator.
**/
typedef EFI_STATUS (*DT_TABLE_GENERATOR_FREE_TABLE) (
  IN  CONST DT_TABLE_GENERATOR                    *       Generator,
  IN  CONST CM_STD_OBJ_DT_TABLE_INFO              * CONST DtTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  * CONST CfgMgrProtocol,
  IN        VOID                                 **       Table
  );

/** The DT_TABLE_GENERATOR structure provides an interface that the
    Table Manager can use to invoke the functions to build DT tables.
*/
typedef struct DtTableGenerator {
  /// The DT table generator ID.
  DT_TABLE_GENERATOR_ID                  GeneratorID;

  /// String describing the DT table generator.
  CONST CHAR16                         * Description;

  /// DT table build function pointer.
  DT_TABLE_GENERATOR_BUILD_TABLE         BuildDtTable;

  /// The function to free any resources allocated for building the DT table.
  DT_TABLE_GENERATOR_FREE_TABLE          FreeTableResources;
} DT_TABLE_GENERATOR;

/** Register DT table factory generator.

  The DT table factory maintains a list of the Standard and OEM DT
  table generators.

  @param [in]  Generator       Pointer to the DT table generator.

  @retval  EFI_SUCCESS          The Generator was registered
                                successfully.
  @retval EFI_INVALID_PARAMETER The Generator ID is invalid or
                                the Generator pointer is NULL.
  @retval EFI_ALREADY_STARTED   The Generator for the Table ID is
                                already registered.
**/
EFI_STATUS
EFIAPI
RegisterDtTableGenerator (
  IN CONST DT_TABLE_GENERATOR                   * CONST Generator
  );

/** Deregister DT generator.

  This function is called by the DT table generator to deregister itself
  from the DT table factory.

  @param [in]  Generator       Pointer to the DT table generator.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER The generator is invalid.
  @retval EFI_NOT_FOUND         The requested generator is not found
                                in the list of registered generators.
**/
EFI_STATUS
EFIAPI
DeregisterDtTableGenerator (
  IN CONST DT_TABLE_GENERATOR                   * CONST Generator
  );

#pragma pack()

#endif // DEVICETREE_TABLE_GENERATOR_H_

