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

/** Return a pointer to the HII form generator.

  @param [in]  This         Pointer to the dynamic table factory protocol.
  @param [in]  GeneratorId  The HII form generator ID for the
                            requested generator.
  @param [out] Generator    Pointer to the requested HII form
                            generator.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         The requested generator is not found
                                in the list of registered generators.
**/
EFI_STATUS
EFIAPI
GetHiiFormGenerator (
  IN  CONST EDKII_DYNAMIC_TABLE_FACTORY_PROTOCOL  *CONST   This,
  IN  CONST HII_FORM_GENERATOR_ID                          GeneratorId,
  OUT CONST HII_FORM_GENERATOR                    **CONST  Generator
  )
{
  UINT16                            FormId;
  EDKII_DYNAMIC_TABLE_FACTORY_INFO  *FactoryInfo;

  if (This == NULL) {
    DEBUG ((DEBUG_ERROR, "ERROR: Invalid factory protocol pointer\n"));
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  if (Generator == NULL) {
    DEBUG ((DEBUG_ERROR, "ERROR: Invalid Generator pointer\n"));
    return EFI_INVALID_PARAMETER;
  }

  if ((GeneratorId != 0) && !IS_GENERATOR_TYPE_HII (GeneratorId)) {
    DEBUG ((DEBUG_ERROR, "ERROR: Generator Type is not HII\n"));
    return EFI_INVALID_PARAMETER;
  }

  FactoryInfo = This->TableFactoryInfo;
  *Generator  = NULL;
  FormId      = GET_TABLE_ID (GeneratorId);
  if (IS_GENERATOR_NAMESPACE_STD (GeneratorId)) {
    if (FormId >= EStdHiiFormsIdMax) {
      ASSERT (FormId < EStdHiiFormsIdMax);
      return EFI_INVALID_PARAMETER;
    }

    if (FactoryInfo->StdHiiFormGeneratorList[FormId] != NULL) {
      *Generator = FactoryInfo->StdHiiFormGeneratorList[FormId];
    } else {
      return EFI_NOT_FOUND;
    }
  } else {
    if (FormId >= FixedPcdGet16 (PcdMaxCustomHiiGenerators)) {
      ASSERT (FormId <= FixedPcdGet16 (PcdMaxCustomHiiGenerators));
      return EFI_INVALID_PARAMETER;
    }

    if (FactoryInfo->CustomHiiFormGeneratorList[FormId] != NULL) {
      *Generator = FactoryInfo->CustomHiiFormGeneratorList[FormId];
    } else {
      return EFI_NOT_FOUND;
    }
  }

  return EFI_SUCCESS;
}

/** Register HII form factory generator.

  The HII form factory maintains a list of the Standard HII form
  generators.

  @param [in]  Generator        Pointer to the HII form generator.

  @retval EFI_SUCCESS           The Generator was registered
                                successfully.
  @retval EFI_INVALID_PARAMETER The Generator ID is invalid or
                                the Generator pointer is NULL.
  @retval EFI_ALREADY_STARTED   The Generator for the Table ID is
                                already registered.
**/
EFI_STATUS
EFIAPI
RegisterHiiFormGenerator (
  IN  CONST HII_FORM_GENERATOR  *CONST  Generator
  )
{
  UINT16  FormId;

  if (Generator == NULL) {
    DEBUG ((DEBUG_ERROR, "ERROR: HII form register - Invalid Generator\n"));
    return EFI_INVALID_PARAMETER;
  }

  if (!IS_GENERATOR_TYPE_HII (Generator->GeneratorID)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: HII form register - Generator" \
      " Type is not HII\n"
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

    if (TableFactoryInfo.StdHiiFormGeneratorList[FormId] == NULL) {
      TableFactoryInfo.StdHiiFormGeneratorList[FormId] = Generator;
    } else {
      return EFI_ALREADY_STARTED;
    }
  } else {
    if (FormId >= FixedPcdGet16 (PcdMaxCustomHiiGenerators)) {
      ASSERT (FormId <= FixedPcdGet16 (PcdMaxCustomHiiGenerators));
      return EFI_INVALID_PARAMETER;
    }

    if (TableFactoryInfo.CustomHiiFormGeneratorList[FormId] == NULL) {
      TableFactoryInfo.CustomHiiFormGeneratorList[FormId] = Generator;
    } else {
      return EFI_ALREADY_STARTED;
    }
  }

  return EFI_SUCCESS;
}

/** Deregister HII form generator.

  This function is called by the HII form generator to deregister itself
  from the HII forms factory.

  @param [in]  Generator        Pointer to the HII form generator.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER The generator is invalid.
  @retval EFI_NOT_FOUND         The requested generator is not found
                                in the list of registered generators.
**/
EFI_STATUS
EFIAPI
DeregisterHiiFormGenerator (
  IN  CONST HII_FORM_GENERATOR    *CONST  Generator
  )
{
  UINT16  FormId;

  if (Generator == NULL) {
    DEBUG ((DEBUG_ERROR, "ERROR: HII form deregister - Invalid Generator\n"));
    return EFI_INVALID_PARAMETER;
  }

  if (!IS_GENERATOR_TYPE_HII (Generator->GeneratorID)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: HII form deregister - Generator" \
      " Type is not HII\n"
      ));
    return EFI_INVALID_PARAMETER;
  }

  FormId = GET_TABLE_ID (Generator->GeneratorID);
  if (IS_GENERATOR_NAMESPACE_STD (Generator->GeneratorID)) {
    if (FormId >= EStdHiiFormsIdMax) {
      ASSERT (FormId < EStdHiiFormsIdMax);
      return EFI_INVALID_PARAMETER;
    }

    if (TableFactoryInfo.StdHiiFormGeneratorList[FormId] != NULL) {
      if (Generator != TableFactoryInfo.StdHiiFormGeneratorList[FormId]) {
        return EFI_INVALID_PARAMETER;
      }

      TableFactoryInfo.StdHiiFormGeneratorList[FormId] = NULL;
    } else {
      return EFI_NOT_FOUND;
    }
  } else {
    if (FormId >= FixedPcdGet16 (PcdMaxCustomHiiGenerators)) {
      ASSERT (FormId <= FixedPcdGet16 (PcdMaxCustomHiiGenerators));
      return EFI_INVALID_PARAMETER;
    }

    if (TableFactoryInfo.CustomHiiFormGeneratorList[FormId] != NULL) {
      if (Generator != TableFactoryInfo.CustomHiiFormGeneratorList[FormId]) {
        return EFI_INVALID_PARAMETER;
      }

      TableFactoryInfo.CustomHiiFormGeneratorList[FormId] = NULL;
    } else {
      return EFI_NOT_FOUND;
    }
  }

  DEBUG ((DEBUG_INFO, "Deregistering %s\n", Generator->Description));
  return EFI_SUCCESS;
}
