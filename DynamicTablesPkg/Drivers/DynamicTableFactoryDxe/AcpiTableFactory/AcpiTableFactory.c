/** @file
  ACPI Table Factory

  Copyright (c) 2017 - 2019, ARM Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Std - Standard
**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Protocol/AcpiTable.h>

// Module specific include files.
#include <AcpiTableGenerator.h>
#include <ConfigurationManagerObject.h>
#include <Protocol/ConfigurationManagerProtocol.h>
#include <Protocol/DynamicTableFactoryProtocol.h>

#include "DynamicTableFactory.h"

extern EDKII_DYNAMIC_TABLE_FACTORY_INFO  TableFactoryInfo;

/** Return a pointer to the ACPI table generator.

  @param [in]  This         Pointer to the Dynamic Table Factory Protocol.
  @param [in]  GeneratorId  The ACPI table generator ID for the
                            requested generator.
  @param [out] Generator    Pointer to the requested ACPI table
                            generator.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         The requested generator is not found
                                in the list of registered generators.
**/
EFI_STATUS
EFIAPI
GetAcpiTableGenerator (
  IN  CONST EDKII_DYNAMIC_TABLE_FACTORY_PROTOCOL  *CONST  This,
  IN  CONST ACPI_TABLE_GENERATOR_ID                       GeneratorId,
  OUT CONST ACPI_TABLE_GENERATOR                 **CONST  Generator
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

  if (!IS_GENERATOR_TYPE_ACPI (GeneratorId)) {
    DEBUG ((DEBUG_ERROR, "ERROR: Generator Type is not ACPI\n"));
    return EFI_INVALID_PARAMETER;
  }

  *Generator = NULL;
  TableId    = GET_TABLE_ID (GeneratorId);
  if (IS_GENERATOR_NAMESPACE_STD (GeneratorId)) {
    if (TableId >= EStdAcpiTableIdMax) {
      ASSERT (TableId < EStdAcpiTableIdMax);
      return EFI_INVALID_PARAMETER;
    }

    if (FactoryInfo->StdAcpiTableGeneratorList[TableId] != NULL) {
      *Generator = FactoryInfo->StdAcpiTableGeneratorList[TableId];
    } else {
      return EFI_NOT_FOUND;
    }
  } else {
    if (TableId > FixedPcdGet16 (PcdMaxCustomACPIGenerators)) {
      ASSERT (TableId <= FixedPcdGet16 (PcdMaxCustomACPIGenerators));
      return EFI_INVALID_PARAMETER;
    }

    if (FactoryInfo->CustomAcpiTableGeneratorList[TableId] != NULL) {
      *Generator = FactoryInfo->CustomAcpiTableGeneratorList[TableId];
    } else {
      return EFI_NOT_FOUND;
    }
  }

  return EFI_SUCCESS;
}

/** Register ACPI table factory generator.

  The ACPI table factory maintains a list of the Standard and OEM ACPI
  table generators.

  @param [in]  Generator       Pointer to the ACPI table generator.

  @retval EFI_SUCCESS           The Generator was registered
                                successfully.
  @retval EFI_INVALID_PARAMETER The Generator ID is invalid or
                                the Generator pointer is NULL.
  @retval EFI_ALREADY_STARTED   The Generator for the Table ID is
                                already registered.
**/
EFI_STATUS
EFIAPI
RegisterAcpiTableGenerator (
  IN  CONST ACPI_TABLE_GENERATOR                *CONST  Generator
  )
{
  UINT16  TableId;

  if (Generator == NULL) {
    DEBUG ((DEBUG_ERROR, "ERROR: ACPI register - Invalid Generator\n"));
    return EFI_INVALID_PARAMETER;
  }

  if (!IS_GENERATOR_TYPE_ACPI (Generator->GeneratorID)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: ACPI register - Generator" \
      " Type is not ACPI\n"
      ));
    return EFI_INVALID_PARAMETER;
  }

  DEBUG ((DEBUG_INFO, "Registering %s\n", Generator->Description));

  TableId = GET_TABLE_ID (Generator->GeneratorID);
  if (IS_GENERATOR_NAMESPACE_STD (Generator->GeneratorID)) {
    if (TableId >= EStdAcpiTableIdMax) {
      ASSERT (TableId < EStdAcpiTableIdMax);
      return EFI_INVALID_PARAMETER;
    }

    if (TableFactoryInfo.StdAcpiTableGeneratorList[TableId] == NULL) {
      TableFactoryInfo.StdAcpiTableGeneratorList[TableId] = Generator;
    } else {
      return EFI_ALREADY_STARTED;
    }
  } else {
    if (TableId > FixedPcdGet16 (PcdMaxCustomACPIGenerators)) {
      ASSERT (TableId <= FixedPcdGet16 (PcdMaxCustomACPIGenerators));
      return EFI_INVALID_PARAMETER;
    }

    if (TableFactoryInfo.CustomAcpiTableGeneratorList[TableId] == NULL) {
      TableFactoryInfo.CustomAcpiTableGeneratorList[TableId] = Generator;
    } else {
      return EFI_ALREADY_STARTED;
    }
  }

  return EFI_SUCCESS;
}

/** Deregister ACPI generator.

  This function is called by the ACPI table generator to deregister itself
  from the ACPI table factory.

  @param [in]  Generator       Pointer to the ACPI table generator.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER The generator is invalid.
  @retval EFI_NOT_FOUND         The requested generator is not found
                                in the list of registered generators.
**/
EFI_STATUS
EFIAPI
DeregisterAcpiTableGenerator (
  IN  CONST ACPI_TABLE_GENERATOR                *CONST  Generator
  )
{
  UINT16  TableId;

  if (Generator == NULL) {
    DEBUG ((DEBUG_ERROR, "ERROR: ACPI deregister - Invalid Generator\n"));
    return EFI_INVALID_PARAMETER;
  }

  if (!IS_GENERATOR_TYPE_ACPI (Generator->GeneratorID)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: ACPI deregister - Generator" \
      " Type is not ACPI\n"
      ));
    return EFI_INVALID_PARAMETER;
  }

  TableId = GET_TABLE_ID (Generator->GeneratorID);
  if (IS_GENERATOR_NAMESPACE_STD (Generator->GeneratorID)) {
    if (TableId >= EStdAcpiTableIdMax) {
      ASSERT (TableId < EStdAcpiTableIdMax);
      return EFI_INVALID_PARAMETER;
    }

    if (TableFactoryInfo.StdAcpiTableGeneratorList[TableId] != NULL) {
      if (Generator != TableFactoryInfo.StdAcpiTableGeneratorList[TableId]) {
        return EFI_INVALID_PARAMETER;
      }

      TableFactoryInfo.StdAcpiTableGeneratorList[TableId] = NULL;
    } else {
      return EFI_NOT_FOUND;
    }
  } else {
    if (TableId > FixedPcdGet16 (PcdMaxCustomACPIGenerators)) {
      ASSERT (TableId <= FixedPcdGet16 (PcdMaxCustomACPIGenerators));
      return EFI_INVALID_PARAMETER;
    }

    if (TableFactoryInfo.CustomAcpiTableGeneratorList[TableId] != NULL) {
      if (Generator !=
          TableFactoryInfo.CustomAcpiTableGeneratorList[TableId])
      {
        return EFI_INVALID_PARAMETER;
      }

      TableFactoryInfo.CustomAcpiTableGeneratorList[TableId] = NULL;
    } else {
      return EFI_NOT_FOUND;
    }
  }

  DEBUG ((DEBUG_INFO, "Deregistering %s\n", Generator->Description));
  return EFI_SUCCESS;
}
