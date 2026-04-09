/** @file
  Device Tree Table Factory

  Copyright (c) 2017 - 2019, ARM Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Std - Standard
**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>

// Module specific include files.
#include <DeviceTreeTableGenerator.h>
#include <ConfigurationManagerObject.h>
#include <Protocol/ConfigurationManagerProtocol.h>
#include <Protocol/DynamicTableFactoryProtocol.h>

#include "DynamicTableFactory.h"

extern EDKII_DYNAMIC_TABLE_FACTORY_INFO  TableFactoryInfo;

/** Return a pointer to the DT table generator.

  @param [in]  This         Pointer to the Dynamic Table Factory Protocol.
  @param [in]  GeneratorId  The DT table generator ID for the
                            requested generator.
  @param [out] Generator    Pointer to the requested DT table
                            generator.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         The requested generator is not found
                                in the list of registered generators.
**/
EFI_STATUS
EFIAPI
GetDtTableGenerator (
  IN  CONST EDKII_DYNAMIC_TABLE_FACTORY_PROTOCOL  *CONST  This,
  IN  CONST DT_TABLE_GENERATOR_ID                         GeneratorId,
  OUT CONST DT_TABLE_GENERATOR                   **CONST  Generator
  )
{
  UINT16                            TableId;
  EDKII_DYNAMIC_TABLE_FACTORY_INFO  *FactoryInfo;

  ASSERT (This != NULL);

  FactoryInfo = This->TableFactoryInfo;

  if (Generator == NULL) {
    DEBUG ((DEBUG_ERROR, "ERROR: Invalid Generator pointer\n"));
    return EFI_INVALID_PARAMETER;
  }

  if (!IS_GENERATOR_TYPE_DT (GeneratorId)) {
    DEBUG ((DEBUG_ERROR, "ERROR: Generator Type is not DT\n"));
    return EFI_INVALID_PARAMETER;
  }

  *Generator = NULL;
  TableId    = GET_TABLE_ID (GeneratorId);
  if (IS_GENERATOR_NAMESPACE_STD (GeneratorId)) {
    if (TableId >= EStdDtTableIdMax) {
      ASSERT (TableId < EStdDtTableIdMax);
      return EFI_INVALID_PARAMETER;
    }

    if (FactoryInfo->StdDtTableGeneratorList[TableId] != NULL) {
      *Generator = FactoryInfo->StdDtTableGeneratorList[TableId];
    } else {
      return EFI_NOT_FOUND;
    }
  } else {
    if (TableId > FixedPcdGet16 (PcdMaxCustomDTGenerators)) {
      ASSERT (TableId <= FixedPcdGet16 (PcdMaxCustomDTGenerators));
      return EFI_INVALID_PARAMETER;
    }

    if (FactoryInfo->CustomDtTableGeneratorList[TableId] != NULL) {
      *Generator = FactoryInfo->CustomDtTableGeneratorList[TableId];
    } else {
      return EFI_NOT_FOUND;
    }
  }

  return EFI_SUCCESS;
}

/** Register DT table factory generator.

  The DT table factory maintains a list of the Standard and OEM DT
  table generators.

  @param [in]  Generator       Pointer to the DT table generator.

  @retval EFI_SUCCESS           The Generator was registered
                                successfully.
  @retval EFI_INVALID_PARAMETER The Generator ID is invalid or
                                the Generator pointer is NULL.
  @retval EFI_ALREADY_STARTED   The Generator for the Table ID is
                                already registered.
**/
EFI_STATUS
EFIAPI
RegisterDtTableGenerator (
  IN  CONST DT_TABLE_GENERATOR                *CONST  Generator
  )
{
  UINT16  TableId;

  if (Generator == NULL) {
    DEBUG ((DEBUG_ERROR, "ERROR: DT register - Invalid Generator\n"));
    return EFI_INVALID_PARAMETER;
  }

  if (!IS_GENERATOR_TYPE_DT (Generator->GeneratorID)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: DT register - Generator" \
      " Type is not DT\n"
      ));
    return EFI_INVALID_PARAMETER;
  }

  DEBUG ((DEBUG_INFO, "Registering %s\n", Generator->Description));

  TableId = GET_TABLE_ID (Generator->GeneratorID);
  if (IS_GENERATOR_NAMESPACE_STD (Generator->GeneratorID)) {
    if (TableId >= EStdDtTableIdMax) {
      ASSERT (TableId < EStdDtTableIdMax);
      return EFI_INVALID_PARAMETER;
    }

    if (TableFactoryInfo.StdDtTableGeneratorList[TableId] == NULL) {
      TableFactoryInfo.StdDtTableGeneratorList[TableId] = Generator;
    } else {
      return EFI_ALREADY_STARTED;
    }
  } else {
    if (TableId > FixedPcdGet16 (PcdMaxCustomDTGenerators)) {
      ASSERT (TableId <= FixedPcdGet16 (PcdMaxCustomDTGenerators));
      return EFI_INVALID_PARAMETER;
    }

    if (TableFactoryInfo.CustomDtTableGeneratorList[TableId] == NULL) {
      TableFactoryInfo.CustomDtTableGeneratorList[TableId] = Generator;
    } else {
      return EFI_ALREADY_STARTED;
    }
  }

  return EFI_SUCCESS;
}

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
  IN  CONST DT_TABLE_GENERATOR                *CONST  Generator
  )
{
  UINT16  TableId;

  if (Generator == NULL) {
    DEBUG ((DEBUG_ERROR, "ERROR: DT deregister - Invalid Generator\n"));
    return EFI_INVALID_PARAMETER;
  }

  if (!IS_GENERATOR_TYPE_DT (Generator->GeneratorID)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: DT deregister - Generator" \
      " Type is not DT\n"
      ));
    return EFI_INVALID_PARAMETER;
  }

  TableId = GET_TABLE_ID (Generator->GeneratorID);
  if (IS_GENERATOR_NAMESPACE_STD (Generator->GeneratorID)) {
    if (TableId >= EStdDtTableIdMax) {
      ASSERT (TableId < EStdDtTableIdMax);
      return EFI_INVALID_PARAMETER;
    }

    if (TableFactoryInfo.StdDtTableGeneratorList[TableId] != NULL) {
      if (Generator != TableFactoryInfo.StdDtTableGeneratorList[TableId]) {
        return EFI_INVALID_PARAMETER;
      }

      TableFactoryInfo.StdDtTableGeneratorList[TableId] = NULL;
    } else {
      return EFI_NOT_FOUND;
    }
  } else {
    if (TableId > FixedPcdGet16 (PcdMaxCustomDTGenerators)) {
      ASSERT (TableId <= FixedPcdGet16 (PcdMaxCustomDTGenerators));
      return EFI_INVALID_PARAMETER;
    }

    if (TableFactoryInfo.CustomDtTableGeneratorList[TableId] != NULL) {
      if (Generator !=
          TableFactoryInfo.CustomDtTableGeneratorList[TableId])
      {
        return EFI_INVALID_PARAMETER;
      }

      TableFactoryInfo.CustomDtTableGeneratorList[TableId] = NULL;
    } else {
      return EFI_NOT_FOUND;
    }
  }

  DEBUG ((DEBUG_INFO, "Deregistering %s\n", Generator->Description));
  return EFI_SUCCESS;
}
