/** @file

  Copyright (c) 2017 - 2019, ARM Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef SMBIOS_TABLE_GENERATOR_H_
#define SMBIOS_TABLE_GENERATOR_H_

#include <IndustryStandard/SmBios.h>
#include <Protocol/Smbios.h>
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
typedef struct DynamicTableFactoryProtocol  EDKII_DYNAMIC_TABLE_FACTORY_PROTOCOL;
typedef UINTN                               CM_OBJECT_TOKEN;

/** An array for holding a map of SMBIOS handles and the CM Object
    token used to build the SMBIOS record for a given SMBIOS Table Generator.
    This array is maintained by the Dynamic Table Factory Protocol and is
    used by SMBIOS table generators to look up other SMBIOS table handles that
    the Table being generated needs to reference.
**/
typedef struct SmbiosHandleCmObjMap {
  SMBIOS_TABLE_GENERATOR_ID    SmbiosGeneratorId;
  SMBIOS_HANDLE                SmbiosTblHandle;
  CM_OBJECT_TOKEN              SmbiosCmToken;
} SMBIOS_HANDLE_MAP;

/** This function pointer describes the interface to SMBIOS table build
    functions provided by the SMBIOS table generator and called by the
    Table Manager to build an SMBIOS table.
    The function will return a pointer to the SMBIOS table and a pointer
    to the CM_OBJECT_TOKEN of the CM_OBJECT that was used to build the
    SMBIOS table.

  @param [in]  Generator             Pointer to the SMBIOS table generator.
  @param [in]  TableFactoryProtocol  Pointer to the Dynamic TableFactory Protocol.
  @param [in]  SmbiosTableInfo       Pointer to the SMBIOS table information.
  @param [in]  CfgMgrProtocol        Pointer to the Configuration Manager
                                     Protocol interface.
  @param [out] Table                 Pointer to the generated SMBIOS table.
  @param [out] CmObjToken            Pointer to the CM_OBJECT_TOKEN
                                     for the SMBIOS table.

  @return EFI_SUCCESS  If the table is generated successfully or other
                        failure codes as returned by the generator.
**/
typedef EFI_STATUS (*SMBIOS_TABLE_GENERATOR_BUILD_TABLE) (
  IN  CONST SMBIOS_TABLE_GENERATOR                        *Generator,
  IN  CONST EDKII_DYNAMIC_TABLE_FACTORY_PROTOCOL  *CONST  TableFactoryProtocol,
  IN        CM_STD_OBJ_SMBIOS_TABLE_INFO          *CONST  SmbiosTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  OUT       SMBIOS_STRUCTURE                              **Table,
  OUT       CM_OBJECT_TOKEN                               *CmObjToken
  );

/** This function pointer describes the interface to used by the
    Table Manager to give the generator an opportunity to free
    any resources allocated for building the SMBIOS table.

  @param [in]  Generator             Pointer to the SMBIOS table generator.
  @param [in]  TableFactoryProtocol  Pointer to the Dynamic TableFactory Protocol.
  @param [in]  SmbiosTableInfo       Pointer to the SMBIOS table information.
  @param [in]  CfgMgrProtocol        Pointer to the Configuration Manager
                                     Protocol interface.
  @param [in]  Table                 Pointer to the generated SMBIOS table.

  @return  EFI_SUCCESS If freed successfully or other failure codes
                        as returned by the generator.
**/
typedef EFI_STATUS (*SMBIOS_TABLE_GENERATOR_FREE_TABLE) (
  IN  CONST SMBIOS_TABLE_GENERATOR                        *Generator,
  IN  CONST EDKII_DYNAMIC_TABLE_FACTORY_PROTOCOL  *CONST  TableFactoryProtocol,
  IN  CONST CM_STD_OBJ_SMBIOS_TABLE_INFO          *CONST  SmbiosTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN        SMBIOS_STRUCTURE                              **Table
  );

/** This function pointer describes an extended interface to build SMBIOS
    tables. The SMBIOS Table Generator can build multiple tables and returns
    a pointer to the list of SMBIOS tables and to a list of the CM_OBJECT_TOKEN
    of the CM_OBJECTS that were used to generate the SMBIOS tables.
    Use te FreeTableResourcesEx to free any resources that may have been
    allocated using this interface.

  @param [in]  Generator             Pointer to the SMBIOS table generator.
  @param [in]  TableFactoryProtocol  Pointer to the Dynamic TableFactory Protocol.
  @param [in]  SmbiosTableInfo       Pointer to the SMBIOS table information.
  @param [in]  CfgMgrProtocol        Pointer to the Configuration Manager
                                     Protocol interface.
  @param [out] Table                 Pointer to a list of generated SMBIOS table(s).
  @param [out] CmObjToken            Pointer to the CM_OBJECT_TOKEN for the
                                     SMBIOS table.
  @param [out] TableCount            Number of generated SMBIOS table(s).

  @return EFI_SUCCESS  If the table is generated successfully or other
                        failure codes as returned by the generator.
**/
typedef EFI_STATUS (*SMBIOS_TABLE_GENERATOR_BUILD_TABLEEX) (
  IN  CONST SMBIOS_TABLE_GENERATOR                         *Generator,
  IN  CONST EDKII_DYNAMIC_TABLE_FACTORY_PROTOCOL   *CONST  TableFactoryProtocol,
  IN        CM_STD_OBJ_SMBIOS_TABLE_INFO          *CONST   SmbiosTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST   CfgMgrProtocol,
  OUT       SMBIOS_STRUCTURE                               ***Table,
  OUT       CM_OBJECT_TOKEN                                **CmObjectToken,
  OUT       UINTN                                 *CONST   TableCount
  );

/** This function pointer describes the interface used by the
    Table Manager to give the generator an opportunity to free
    any resources allocated for building the SMBIOS table. This interface
    must be used in conjunction with BuildSmbiosTableEx.

  @param [in]  Generator             Pointer to the SMBIOS table generator.
  @param [in]  TableFactoryProtocol  Pointer to the Dynamic TableFactory Protocol.
  @param [in]  SmbiosTableInfo       Pointer to the SMBIOS table information.
  @param [in]  CfgMgrProtocol        Pointer to the Configuration Manager
                                     Protocol interface.
  @param [in]  Table                 Pointer to the generated SMBIOS table.
  @param [in]  CmObjToken            Pointer to the CM_OBJECT_TOKEN for the
                                     SMBIOS table.
  @param [in]  TableCount            Number of generated SMBIOS table(s).

  @return  EFI_SUCCESS If freed successfully or other failure codes
                        as returned by the generator.
**/
typedef EFI_STATUS (*SMBIOS_TABLE_GENERATOR_FREE_TABLEEX) (
  IN  CONST SMBIOS_TABLE_GENERATOR                        *Generator,
  IN  CONST EDKII_DYNAMIC_TABLE_FACTORY_PROTOCOL  *CONST  TableFactoryProtocol,
  IN  CONST CM_STD_OBJ_SMBIOS_TABLE_INFO          *CONST  SmbiosTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN        SMBIOS_STRUCTURE                              ***Table,
  IN        CM_OBJECT_TOKEN                               **CmObjectToken,
  IN  CONST UINTN                                         TableCount
  );

/** This function pointer describes the interface to used by the
    Table Manager to give the generator an opportunity to add
    an SMBIOS Handle.

  This function is called by the Dynamic Table Manager to add a newly added
  SMBIOS Table OR it can be called by an SMBIOS Table generator to create
  and add a new SMBIOS Handle if there is a reference to another table and
  if there is a generator for that table that hasn't been called yet.

  @param [in]  Smbios         Pointer to the SMBIOS protocol.
  @param [in]  SmbiosHandle   Pointer to an SMBIOS handle.
  @param [in]  CmObjectToken  The CM Object token for that is used to generate
                              SMBIOS record.
  @param [in]  GeneratorId    The SMBIOS table generator Id.

  @retval EFI_SUCCESS           Success.
  @retval EFI_OUT_OF_RESOURCES  Unable to add the handle.
  @retval EFI_NOT_FOUND         The requested generator is not found
                                in the list of registered generators.
**/
typedef EFI_STATUS (EFIAPI *EDKII_DYNAMIC_TABLE_FACTORY_SMBIOS_TABLE_ADD_HANDLE)(
  IN  EFI_SMBIOS_PROTOCOL        *Smbios,
  IN  SMBIOS_HANDLE              *SmbiosHandle,
  IN  CM_OBJECT_TOKEN            CmObjectToken,
  IN  SMBIOS_TABLE_GENERATOR_ID  GeneratorId
  );

/** This function pointer describes the interface to used by the
    Table Manager to give the generator an opportunity to find
    an SMBIOS Handle.

  This function is called by the SMBIOS table generator to find an SMBIOS
  handle needed as part of generating an SMBIOS Table.

  @param [in]  CmObjectToken    The CM Object token used to generate the SMBIOS
                                record.

  @retval SMBIOS_HANDLE_MAP  Pointer to the SMBIOS Handle Map if the
                             CmObjectToken is found.
  @retval NULL               if CmObjectToken is not found.
**/
typedef SMBIOS_HANDLE_MAP *(EFIAPI *EDKII_DYNAMIC_TABLE_FACTORY_SMBIOS_TABLE_GET_HANDLE)(
  IN  CM_OBJECT_TOKEN      CmObjectToken
  );

/** Find and return SMBIOS handle based on associated CM object token.

  @param [in]  GeneratorId     SMBIOS generator ID used to build the SMBIOS Table.
  @param [in]  CmObjectToken   Token of the CM_OBJECT used to build the SMBIOS Table.

  @return  SMBIOS handle of the table associated with SmbiosTableId and
           CmObjectToken if found. Otherwise, returns SMBIOS_HANDLE_INVALID.
**/
typedef SMBIOS_HANDLE (EFIAPI *EDKII_DYNAMIC_TABLE_FACTORY_SMBIOS_TABLE_GET_HANDLE_EX)(
  IN  SMBIOS_TABLE_GENERATOR_ID  GeneratorId,
  IN  CM_OBJECT_TOKEN            CmObjToken
  );

/** The SMBIOS_TABLE_GENERATOR structure provides an interface that the
    Table Manager can use to invoke the functions to build SMBIOS tables.
*/
typedef struct SmbiosTableGenerator {
  /// The SMBIOS table generator ID.
  SMBIOS_TABLE_GENERATOR_ID               GeneratorID;

  /// String describing the DT table
  /// generator.
  CONST CHAR16                            *Description;

  /// The SMBIOS table type.
  SMBIOS_TYPE                             Type;

  /// SMBIOS table build function pointer.
  SMBIOS_TABLE_GENERATOR_BUILD_TABLE      BuildSmbiosTable;

  /** The function to free any resources
      allocated for building the SMBIOS table.
  */
  SMBIOS_TABLE_GENERATOR_FREE_TABLE       FreeTableResources;

  /// SMBIOS table extended build function pointer.
  SMBIOS_TABLE_GENERATOR_BUILD_TABLEEX    BuildSmbiosTableEx;

  /** The function to free any resources
      allocated for building the SMBIOS table.
  */
  SMBIOS_TABLE_GENERATOR_FREE_TABLEEX     FreeTableResourcesEx;
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

/** Add SMBIOS Handle.

  This function is called by the Dynamic Table Manager to add an entry for
  the newly created SMBIOS Table handle and the CM_OBJECT_TOKEN of the CM_OBJECT
  used to generate the table for a given SMBIOS generator to the SMBIOS Handle
  Map maintained by the DynamicTable Factory.

  @param [in]  Smbios         Pointer to the SMBIOS protocol.
  @param [in]  SmbiosHandle   Pointer to an SMBIOS handle.
  @param [in]  CmObjectToken  The CM Object token for that is used to generate
                              SMBIOS record.
  @param [in]  GeneratorId    The SMBIOS table generator Id.

  @retval EFI_SUCCESS           Success.
  @retval EFI_OUT_OF_RESOURCES  Unable to add the handle.
  @retval EFI_NOT_FOUND         The requested generator is not found
                                in the list of registered generators.
**/
EFI_STATUS
EFIAPI
AddSmbiosHandle (
  IN EFI_SMBIOS_PROTOCOL        *Smbios,
  IN SMBIOS_HANDLE              *SmbiosHandle,
  IN CM_OBJECT_TOKEN            CmObjectToken,
  IN SMBIOS_TABLE_GENERATOR_ID  GeneratorId
  );

/** Find SMBIOS Handle given the CM Object token used to generate the SMBIOS
    record.

  This function is called by the SMBIOS table generator to find an SMBIOS
  handle map to reference a different SMBIOS table.

  @param [in]  CmObjectToken    The CM Object token used to generate the SMBIOS
                                record.

  @retval SMBIOS_HANDLE_MAP  Pointer to the SMBIOS Handle Map if the
                             CmObjectToken is found.
  @retval NULL               if CmObjectToken is not found.
**/
SMBIOS_HANDLE_MAP *
EFIAPI
FindSmbiosHandle (
  IN CM_OBJECT_TOKEN  CmObjectToken
  );

/** Find and return SMBIOS handle for a given associated CM object token for
    an SMBIOS Table Generator.

  @param [in]  GeneratorId     SMBIOS generator ID used to build the SMBIOS Table.
  @param [in]  CmObjectToken   Token of the CM_OBJECT used to build the SMBIOS Table.

  @return  SMBIOS handle of the table associated with SmbiosTableId and
           CmObjectToken if found. Otherwise, returns SMBIOS_HANDLE_INVALID.
**/
SMBIOS_HANDLE
EFIAPI
FindSmbiosHandleEx (
  IN  SMBIOS_TABLE_GENERATOR_ID  GeneratorId,
  IN  CM_OBJECT_TOKEN            CmObjToken
  );

#pragma pack()

#endif // SMBIOS_TABLE_GENERATOR_H_
