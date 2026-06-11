/** @file
  HII Forms Factory

  Copyright (c) 2026, ARM Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - HII - Human Interface Infrastructure
    - Std - Standard
**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>

// Module specific include files.
#include <HiiFormsGenerator.h>
#include <ConfigurationManagerObject.h>
#include <Protocol/ConfigurationManagerProtocol.h>
#include <Protocol/DynamicTableFactoryProtocol.h>

#include "DynamicTableFactory.h"

extern EDKII_DYNAMIC_TABLE_FACTORY_INFO  TableFactoryInfo;

/** Return a pointer to the Hii Forms generator.

  @param [in]  This         Pointer to the Dynamic Table Factory Protocol.
  @param [in]  GeneratorId  The Hii Forms generator ID for the
                            requested generator.
  @param [out] Generator    Pointer to the requested Hii Forms
                            generator.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         The requested generator is not found
                                in the list of registered generators.
**/
EFI_STATUS
EFIAPI
GetHiiFormsGenerator (
  IN  CONST EDKII_DYNAMIC_TABLE_FACTORY_PROTOCOL  *CONST   This,
  IN  CONST HII_FORMS_GENERATOR_ID                         GeneratorId,
  OUT       HII_FORMS_GENERATOR                   **CONST  Generator
  )
{
  UINT16                            FormId;
  EDKII_DYNAMIC_TABLE_FACTORY_INFO  *FactoryInfo;

  ASSERT (This != NULL);

  FactoryInfo = This->TableFactoryInfo;

  if (Generator == NULL) {
    DEBUG ((DEBUG_ERROR, "ERROR: Invalid Generator pointer\n"));
    return EFI_INVALID_PARAMETER;
  }

  if ((GeneratorId != 0) && !IS_GENERATOR_TYPE_HII (GeneratorId)) {
    DEBUG ((DEBUG_ERROR, "ERROR: Generator Type is not Hii\n"));
    return EFI_INVALID_PARAMETER;
  }

  *Generator = NULL;
  FormId     = GET_TABLE_ID (GeneratorId);
  if (IS_GENERATOR_NAMESPACE_STD (GeneratorId)) {
    if (FormId >= EStdHiiFormsIdMax) {
      ASSERT (FormId < EStdHiiFormsIdMax);
      return EFI_INVALID_PARAMETER;
    }

    if (FactoryInfo->StdHiiFormsGeneratorList[FormId] != NULL) {
      *Generator = FactoryInfo->StdHiiFormsGeneratorList[FormId];
    } else {
      return EFI_NOT_FOUND;
    }
  }

  return EFI_SUCCESS;
}

/** Register Hii Forms factory generator.

  The Hii Forms factory maintains a list of the Standard Hii Forms
  generators.

  @param [in]  Generator        Pointer to the Hii Forms generator.

  @retval EFI_SUCCESS           The Generator was registered
                                successfully.
  @retval EFI_INVALID_PARAMETER The Generator ID is invalid or
                                the Generator pointer is NULL.
  @retval EFI_ALREADY_STARTED   The Generator for the Table ID is
                                already registered.
**/
EFI_STATUS
EFIAPI
RegisterHiiFormsGenerator (
  IN  HII_FORMS_GENERATOR                *CONST  Generator
  )
{
  UINT16  FormId;

  if (Generator == NULL) {
    DEBUG ((DEBUG_ERROR, "ERROR: Hii Forms register - Invalid Generator\n"));
    return EFI_INVALID_PARAMETER;
  }

  if (!IS_GENERATOR_TYPE_HII (Generator->GeneratorID)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: Hii Forms register - Generator" \
      " Type is not Hii\n"
      ));
    return EFI_INVALID_PARAMETER;
  }

  DEBUG ((DEBUG_INFO, "Registering %s\n", Generator->Description));

  FormId = GET_TABLE_ID (Generator->GeneratorID);
  if (IS_GENERATOR_NAMESPACE_STD (Generator->GeneratorID)) {
    if (FormId >= EStdHiiFormsIdMax) {
      ASSERT (FormId < EStdHiiFormsIdMax);
      return EFI_INVALID_PARAMETER;
    }

    if (TableFactoryInfo.StdHiiFormsGeneratorList[FormId] == NULL) {
      TableFactoryInfo.StdHiiFormsGeneratorList[FormId] = Generator;
    } else {
      return EFI_ALREADY_STARTED;
    }
  }

  return EFI_SUCCESS;
}

/** Deregister Hii Forms generator.

  This function is called by the Hii Forms generator to deregister itself
  from the Hii Forms factory.

  @param [in]  Generator        Pointer to the Hii Forms generator.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER The generator is invalid.
  @retval EFI_NOT_FOUND         The requested generator is not found
                                in the list of registered generators.
**/
EFI_STATUS
EFIAPI
DeregisterHiiFormsGenerator (
  IN  HII_FORMS_GENERATOR                *CONST  Generator
  )
{
  UINT16  FormId;

  if (Generator == NULL) {
    DEBUG ((DEBUG_ERROR, "ERROR: Hii Forms deregister - Invalid Generator\n"));
    return EFI_INVALID_PARAMETER;
  }

  if (!IS_GENERATOR_TYPE_HII (Generator->GeneratorID)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: Hii Forms deregister - Generator" \
      " Type is not Hii\n"
      ));
    return EFI_INVALID_PARAMETER;
  }

  FormId = GET_TABLE_ID (Generator->GeneratorID);
  if (IS_GENERATOR_NAMESPACE_STD (Generator->GeneratorID)) {
    if (FormId >= EStdHiiFormsIdMax) {
      ASSERT (FormId < EStdHiiFormsIdMax);
      return EFI_INVALID_PARAMETER;
    }

    if (TableFactoryInfo.StdHiiFormsGeneratorList[FormId] != NULL) {
      if (Generator != TableFactoryInfo.StdHiiFormsGeneratorList[FormId]) {
        return EFI_INVALID_PARAMETER;
      }

      TableFactoryInfo.StdHiiFormsGeneratorList[FormId] = NULL;
    } else {
      return EFI_NOT_FOUND;
    }
  }

  DEBUG ((DEBUG_INFO, "Deregistering %s\n", Generator->Description));
  return EFI_SUCCESS;
}
