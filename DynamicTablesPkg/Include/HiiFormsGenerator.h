/** @file

  Copyright (c) 2026, ARM Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Cm or CM   - Configuration Manager
    - Obj or OBJ - Object
    - Std or STD - Standard
    - HII        - Human Interface Infrastructure
**/

#pragma once

#include <TableGenerator.h>

/** The HII_FORMS_GENERATOR_ID type describes the HII Form generator ID.
*/
typedef TABLE_GENERATOR_ID HII_FORMS_GENERATOR_ID;

/** The ESTD_HII_FORMS_ID enum describes the Hii Form IDs reserved for
    the standard generators.
*/
typedef enum HiiFormsId {
  EStdHiiFormsIdPlatCfg = 0x0000,              ///< Reserved.
  EStdHiiFormsIdCpuArm,                        ///< Cpu Generator for Arm Platforms.
  EStdHiiFormsIdMax
} ESTD_HII_FORMS_ID;

/** This structure is used for holding the HII handles of
    forms that have been added.
*/
typedef struct {
  LIST_ENTRY        List;

  EFI_HII_HANDLE    HiiHandle;
} HII_FORM_HANDLE;

/** This macro checks if the Generator ID is for a HII Forms Generator.

  @param [in] HiiFormGeneratorId  The Form generator ID.

  @return TRUE if the table generator ID is for a HII Form
            Generator.
**/
#define IS_GENERATOR_TYPE_HII(HiiFormGeneratorId) \
          (GET_TABLE_TYPE(HiiFormGeneratorId) == ETableGeneratorTypeHii)

/** This macro creates a standard HII Form Generator ID.

  @param [in] TableId  The table generator ID.

  @return a standard HII Form generator ID.
**/
#define CREATE_HII_FORMS_GEN_ID(FormsId) \
          CREATE_TABLE_GEN_ID (                 \
            ETableGeneratorTypeHii,             \
            ETableGeneratorNameSpaceStd,        \
            FormsId                             \
            )

/** This macro checks if the Forms Generator ID is for a standard HII
    Forms Generator.

  @param [in] HiiFormsGeneratorId  The Hii Forms generator ID.

  @return  TRUE if the Forms generator ID is for a standard HII
            Forms Generator.
**/
#define IS_VALID_STD_HII_FORMS_GENERATOR_ID(HiiFormsGeneratorId)            \
          (                                                                 \
          IS_GENERATOR_NAMESPACE_STD(HiiFormsGeneratorId) &&                \
          IS_GENERATOR_TYPE_HII(HiiFormsGeneratorId)   &&                   \
          ((GET_TABLE_ID(HiiFormsGeneratorId) > EStdHiiFormsIdPlatCfg) &&   \
           (GET_TABLE_ID(HiiFormsGeneratorId) < EStdHiiFormsIdMax))              \
          )

/** Forward declarations.
*/
typedef struct ConfigurationManagerProtocol EDKII_CONFIGURATION_MANAGER_PROTOCOL;
typedef struct CmHiiFormsObjInfo            CM_HII_FORMS_OBJ_INFO;
typedef struct HiiFormsGenerator            HII_FORMS_GENERATOR;
typedef struct DynamicTableFactoryProtocol  EDKII_DYNAMIC_TABLE_FACTORY_PROTOCOL;
typedef UINTN                               CM_OBJECT_TOKEN;

/** This function pointer describes the interface to HII Form build
    function provided by the HII Forms generator and called by the
    Table Manager to build a HII Form.

  @param [in]  Generator       Pointer to the HII Form generator.
  @param [in]  HiiFormsInfo    Pointer to the HII Form information.
  @param [in]  CfgMgrProtocol  Pointer to the Configuration Manager
                               Protocol interface.
  @param [in]  HiiHandleList   Optional list of Hii Handles
  @param [out] HiiHandle       Pointer to the generated Form's Hii
                               Handle.

  @return EFI_SUCCESS  If the table is generated successfully or other
                        failure codes as returned by the generator.
**/
typedef EFI_STATUS (EFIAPI *HII_FORMS_GENERATOR_BUILD_FORM)(
  IN        HII_FORMS_GENERATOR                           *Generator,
  IN  CONST CM_HII_FORMS_OBJ_INFO                 *CONST  HiiFormsInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN  CONST LIST_ENTRY                                    *HiiHandleList OPTIONAL,
  OUT       EFI_HII_HANDLE                                *HiiHandle
  );

/** This function pointer describes the interface to used by the
    Table Manager to give the generator an opportunity to free
    any resources allocated for building the HII Form.

  @param [in]  Generator       Pointer to the HII Forms generator.
  @param [in]  HiiHandle       Pointer to the generated Form's HII
                               Handle.

  @return EFI_SUCCESS  If freed successfully or other failure codes
                        as returned by the generator.
**/
typedef EFI_STATUS (EFIAPI *HII_FORMS_GENERATOR_FREE_RES)(
  IN        HII_FORMS_GENERATOR                           *Generator,
  IN  CONST CM_HII_FORMS_OBJ_INFO                 *CONST  HiiFormsInfo,
  IN        EFI_HII_HANDLE                                *HiiHandle
  );

/** The HII_FORMS_GENERATOR structure provides an interface that the
    Table Manager can use to invoke the functions to build HII Forms.
*/
typedef struct HiiFormsGenerator {
  /// The HII Forms generator ID.
  HII_FORMS_GENERATOR_ID            GeneratorID;

  /// String describing the HII Form
  /// generator.
  CONST CHAR16                      *Description;

  /// Any private data associated with the
  /// generator.
  VOID                              *Priv;

  /// HII Form build function pointer.
  HII_FORMS_GENERATOR_BUILD_FORM    BuildHiiForm;

  /// The function to free any resources allocated
  /// for building the HII Form.
  HII_FORMS_GENERATOR_FREE_RES      FreeHiiFormResources;
} HII_FORMS_GENERATOR;

/** Register HII Forms factory generator.

  The HII Forms factory maintains a list of the Standard and OEM
  HII Forms generators.

  @param [in]  Generator       Pointer to the HII Forms generator.

  @retval  EFI_SUCCESS          The Generator was registered
                                successfully.
  @retval EFI_INVALID_PARAMETER The Generator ID is invalid or
                                the Generator pointer is NULL.
  @retval EFI_ALREADY_STARTED   The Generator for the Table ID is
                                already registered.
**/
EFI_STATUS
EFIAPI
RegisterHiiFormsGenerator (
  IN  HII_FORMS_GENERATOR                   *CONST  Generator
  );

/** Deregister HII Forms generator.

  This function is called by the HII Forms generator to deregister itself
  from the HII Forms factory.

  @param [in]  Generator       Pointer to the HII Forms generator.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER The generator is invalid.
  @retval EFI_NOT_FOUND         The requested generator is not found
                                in the list of registered generators.
**/
EFI_STATUS
EFIAPI
DeregisterHiiFormsGenerator (
  IN  HII_FORMS_GENERATOR                   *CONST  Generator
  );
