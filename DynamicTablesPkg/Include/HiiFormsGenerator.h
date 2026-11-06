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

/** The HII_FORM_GENERATOR_ID type describes the HII Form generator ID.
*/
typedef TABLE_GENERATOR_ID HII_FORM_GENERATOR_ID;

/** The ESTD_HII_FORMS_ID enum describes the Hii Form IDs reserved for
    the standard generators.
*/
typedef enum HiiFormsId {
  EStdHiiFormsIdRootForm = 0x0000,             ///< Reserved for the Root form generator.
  EStdHiiFormsIdCpuArm,                        ///< Cpu Generator for Arm Platforms.
  EStdHiiFormsIdMax
} ESTD_HII_FORMS_ID;

/** This structure is used for holding the HII handles of
    forms that have been added. The root form generator
    iterates through this list and creates references to
    all the forms that have been added to this list.
*/
typedef struct {
  /// Link to the list of all HII handles.
  LIST_ENTRY        List;

  /// HII handle of the form that has been
  /// added to the HII database.
  EFI_HII_HANDLE    HiiHandle;
} HII_FORM_HANDLE;

/** This macro checks if the Generator ID is for a HII form
    generator.

  @param [in] HiiFormGeneratorId  The form generator ID.

  @return TRUE if the table generator ID is for a HII form
            generator.
**/
#define IS_GENERATOR_TYPE_HII(HiiFormGeneratorId) \
          (GET_TABLE_TYPE(HiiFormGeneratorId) == ETableGeneratorTypeHii)

/** This macro creates an OEM HII Form Generator ID.

  @param [in] FormId  The HII form generator ID.

  @return OEM HII Form generator ID.
**/
#define CREATE_OEM_HII_FORM_GEN_ID(FormId) \
          CREATE_TABLE_GEN_ID (                 \
            ETableGeneratorTypeHii,             \
            ETableGeneratorNameSpaceOem,        \
            FormId                              \
            )

/** This macro creates a standard HII Form Generator ID.

  @param [in] FormId  The HII form generator ID.

  @return a standard HII form generator ID.
**/
#define CREATE_HII_FORM_GEN_ID(FormId)         \
          CREATE_TABLE_GEN_ID (                 \
            ETableGeneratorTypeHii,             \
            ETableGeneratorNameSpaceStd,        \
            FormId                              \
            )

/** This macro checks if the passed ID is for a HII
    Form Generator.

  @param [in] HiiFormGeneratorId  The HII form generator ID.

  @return  TRUE if the form generator ID is for a HII form
           generator.
**/
#define IS_VALID_HII_FORM_GENERATOR_ID(HiiFormGeneratorId)   \
        IS_GENERATOR_TYPE_HII(HiiFormGeneratorId)

/** Forward declarations.
*/
typedef struct ConfigurationManagerProtocol EDKII_CONFIGURATION_MANAGER_PROTOCOL;
typedef struct CmStdObjHiiFormInfo          CM_STD_OBJ_HII_FORM_INFO;
typedef struct HiiFormGenerator             HII_FORM_GENERATOR;
typedef struct DynamicTableFactoryProtocol  EDKII_DYNAMIC_TABLE_FACTORY_PROTOCOL;
typedef UINTN                               CM_OBJECT_TOKEN;

/** This function pointer describes the interface to HII form build
    function provided by the HII form generator and called by the
    table manager to build a HII form.

  @param [in]  Generator        Pointer to the HII form generator.
  @param [in]  HiiFormInfo      Pointer to the HII form information.
  @param [in]  CfgMgrProtocol   Pointer to the Configuration Manager
                                protocol interface.
  @param [in]  HiiHandleList    Optional list of HII handles
  @param [out] HiiHandle        Pointer to the generated form's HII
                                handle.
  @param [out] Priv             Pointer to the private structure
                                maintained by the generator.

  @return EFI_SUCCESS  If the table is generated successfully or other
                        failure codes as returned by the generator.
**/
typedef EFI_STATUS (EFIAPI *HII_FORM_GENERATOR_BUILD_FORM)(
  IN  CONST HII_FORM_GENERATOR                            *Generator,
  IN  CONST CM_STD_OBJ_HII_FORM_INFO              *CONST  HiiFormInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN  CONST LIST_ENTRY                                    *HiiHandleList OPTIONAL,
  OUT       EFI_HII_HANDLE                                *HiiHandle,
  OUT       VOID                                          **Priv
  );

/** This function pointer describes the interface to used by the
    table manager to give the generator an opportunity to free
    any resources allocated for building the HII form.

    When a generator defines this function, it is the generator's
    responsibility to free up resources that might be specific to
    the form generator, and also remove the HII package resources,
    which is done through a call to HiiRemovePackages().

  @param [in]  Generator      Pointer to the HII form's generator.
  @param [in]  HiiHandle      Pointer to the generated form's HII
                              handle.
  @param [in]  Priv           Pointer to the private structure
                              maintained by the generator.

  @return EFI_SUCCESS  If freed successfully or other failure codes
                        as returned by the generator.
**/
typedef EFI_STATUS (EFIAPI *HII_FORM_GENERATOR_FREE_RES)(
  IN  CONST HII_FORM_GENERATOR                        *Generator,
  IN  CONST CM_STD_OBJ_HII_FORM_INFO         *CONST   HiiFormInfo,
  IN        EFI_HII_HANDLE                            *HiiHandle,
  IN        VOID                                      *Priv
  );

/** The HII_FORM_GENERATOR structure provides an interface that the
    table manager can use to invoke the functions to build a HII
    Form.
*/
typedef struct HiiFormGenerator {
  /// The HII Form generator ID.
  HII_FORM_GENERATOR_ID            GeneratorID;

  /// String describing the HII Form
  /// generator.
  CONST CHAR16                     *Description;

  /// HII Form build function pointer.
  HII_FORM_GENERATOR_BUILD_FORM    BuildHiiForm;

  /// The function to free any resources allocated
  /// for building the HII Form.
  HII_FORM_GENERATOR_FREE_RES      FreeHiiFormResources;
} HII_FORM_GENERATOR;

/** Register HII Form factory generator.

  The HII forms factory maintains a list of the Standard and OEM
  HII form generators.

  @param [in]  Generator        Pointer to the HII form generator.

  @retval  EFI_SUCCESS          The generator was registered
                                successfully.
  @retval EFI_INVALID_PARAMETER The generator ID is invalid or
                                the generator pointer is NULL.
  @retval EFI_ALREADY_STARTED   The generator for the table ID is
                                already registered.
**/
EFI_STATUS
EFIAPI
RegisterHiiFormGenerator (
  IN  CONST HII_FORM_GENERATOR      *CONST  Generator
  );

/** Deregister HII form generator.

  This function is called by the HII form generator to deregister
  itself from the HII form factory.

  @param [in]  Generator        Pointer to the HII form generator.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER The generator is invalid.
  @retval EFI_NOT_FOUND         The requested generator is not found
                                in the list of registered generators.
**/
EFI_STATUS
EFIAPI
DeregisterHiiFormGenerator (
  IN  CONST HII_FORM_GENERATOR      *CONST  Generator
  );
