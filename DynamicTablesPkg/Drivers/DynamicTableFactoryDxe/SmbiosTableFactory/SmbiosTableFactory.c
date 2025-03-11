/** @file
  SMBIOS Table Factory

  Copyright (c) 2017 - 2019, ARM Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Std - Standard
**/

#include <IndustryStandard/SmBios.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>

// Module specific include files.
#include <SmbiosTableGenerator.h>
#include <ConfigurationManagerObject.h>
#include <Protocol/ConfigurationManagerProtocol.h>
#include <Protocol/DynamicTableFactoryProtocol.h>

#include "DynamicTableFactory.h"

extern EDKII_DYNAMIC_TABLE_FACTORY_INFO  TableFactoryInfo;

/** Add a new entry to the SMBIOS table Map.

  @param [in]  Smbios         SMBIOS Protocol pointer.
  @param [in]  SmbiosHandle   SMBIOS Handle to be added.
  @param [in]  CmObjectToken  CmObjectToken of the CM_OBJECT used to build the SMBIOS Table
  @param [in]  GeneratorId    Smbios Table Generator Id.

  @retval EFI_SUCCESS               Successfully added/generated the handle.
  @retval EFI_OUT_OF_RESOURCES      Failure to add/generate the handle.
**/
EFI_STATUS
EFIAPI
AddSmbiosHandle (
  IN EFI_SMBIOS_PROTOCOL        *Smbios,
  IN SMBIOS_HANDLE              *SmbiosHandle,
  IN CM_OBJECT_TOKEN            CmObjectToken,
  IN SMBIOS_TABLE_GENERATOR_ID  GeneratorId
  )
{
  EFI_STATUS  Status;
  UINTN       Index;

  Status = EFI_OUT_OF_RESOURCES;

  for (Index = 0; Index < FixedPcdGet16 (PcdMaxSmbiosHandleMapEntries); Index++) {
    if (TableFactoryInfo.SmbiosHandleMap[Index].SmbiosTblHandle == SMBIOS_HANDLE_PI_RESERVED) {
      TableFactoryInfo.SmbiosHandleMap[Index].SmbiosTblHandle   = *SmbiosHandle;
      TableFactoryInfo.SmbiosHandleMap[Index].SmbiosCmToken     = CmObjectToken;
      TableFactoryInfo.SmbiosHandleMap[Index].SmbiosGeneratorId = GeneratorId;
      Status                                                    = EFI_SUCCESS;
      break;
    }
  }

  return Status;
}

/** Return a pointer to the SMBIOS table Map.

  @param [in]  GeneratorId  The CmObjectToken to look up an SMBIOS Handle.

  @retval SMBIOS_HANDLE_MAP  Pointer to the SMBIOS Handle Map if the
                             CmObjectToken is found.
  @retval NULL               if CmObjectToken is not found.
**/
SMBIOS_HANDLE_MAP *
EFIAPI
FindSmbiosHandle (
  CM_OBJECT_TOKEN  CmObjectToken
  )
{
  UINTN              Index;
  SMBIOS_HANDLE_MAP  *SmbiosHandleMap;

  SmbiosHandleMap = NULL;
  for (Index = 0; Index < FixedPcdGet16 (PcdMaxSmbiosHandleMapEntries); Index++) {
    if (TableFactoryInfo.SmbiosHandleMap[Index].SmbiosCmToken == CmObjectToken) {
      SmbiosHandleMap = &TableFactoryInfo.SmbiosHandleMap[Index];
      break;
    }
  }

  return SmbiosHandleMap;
}

/** Find and return SMBIOS handle based on associated CM object token.

  @param [in]  GeneratorId     SMBIOS generator ID used to build the SMBIOS Table.
  @param [in]  CmObjectToken   Token of the CM_OBJECT used to build the SMBIOS Table.

  @return  SMBIOS handle of the table associated with SmbiosGeneratorId and
           CmObjectToken if found. Otherwise, returns SMBIOS_HANDLE_INVALID.
**/
SMBIOS_HANDLE
EFIAPI
FindSmbiosHandleEx (
  IN  SMBIOS_TABLE_GENERATOR_ID  GeneratorId,
  IN  CM_OBJECT_TOKEN            CmObjToken
  )
{
  SMBIOS_HANDLE_MAP  *HandleMap;

  if (CmObjToken == CM_NULL_TOKEN) {
    return SMBIOS_HANDLE_INVALID;
  }

  HandleMap = FindSmbiosHandle (CmObjToken);
  if (HandleMap == NULL) {
    return SMBIOS_HANDLE_INVALID;
  }

  if (HandleMap->SmbiosGeneratorId != GeneratorId) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Expect ID %d but get %d\n",
      __func__,
      GeneratorId,
      HandleMap->SmbiosGeneratorId
      ));
    ASSERT (FALSE);
    return SMBIOS_HANDLE_INVALID;
  }

  return HandleMap->SmbiosTblHandle;
}

/** Return a pointer to the SMBIOS table generator.

  @param [in]  This         Pointer to the Dynamic Table Factory Protocol.
  @param [in]  GeneratorId  The SMBIOS table generator ID for the
                            requested generator.
  @param [out] Generator    Pointer to the requested SMBIOS table
                            generator.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         The requested generator is not found
                                in the list of registered generators.
**/
EFI_STATUS
EFIAPI
GetSmbiosTableGenerator (
  IN  CONST EDKII_DYNAMIC_TABLE_FACTORY_PROTOCOL  *CONST  This,
  IN  CONST SMBIOS_TABLE_GENERATOR_ID                     GeneratorId,
  OUT CONST SMBIOS_TABLE_GENERATOR               **CONST  Generator
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

  if (!IS_GENERATOR_TYPE_SMBIOS (GeneratorId)) {
    DEBUG ((DEBUG_ERROR, "ERROR: Generator Type is not SMBIOS\n"));
    return EFI_INVALID_PARAMETER;
  }

  *Generator = NULL;
  TableId    = GET_TABLE_ID (GeneratorId);
  if (IS_GENERATOR_NAMESPACE_STD (GeneratorId)) {
    if (TableId >= EStdSmbiosTableIdMax) {
      ASSERT (TableId < EStdSmbiosTableIdMax);
      return EFI_INVALID_PARAMETER;
    }

    if (FactoryInfo->StdSmbiosTableGeneratorList[TableId] != NULL) {
      *Generator = FactoryInfo->StdSmbiosTableGeneratorList[TableId];
    } else {
      return EFI_NOT_FOUND;
    }
  } else {
    if (TableId > FixedPcdGet16 (PcdMaxCustomSMBIOSGenerators)) {
      ASSERT (TableId <= FixedPcdGet16 (PcdMaxCustomSMBIOSGenerators));
      return EFI_INVALID_PARAMETER;
    }

    if (FactoryInfo->CustomSmbiosTableGeneratorList[TableId] != NULL) {
      *Generator = FactoryInfo->CustomSmbiosTableGeneratorList[TableId];
    } else {
      return EFI_NOT_FOUND;
    }
  }

  return EFI_SUCCESS;
}

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
  IN  CONST SMBIOS_TABLE_GENERATOR              *CONST  Generator
  )
{
  UINT16  TableId;

  if (Generator == NULL) {
    DEBUG ((DEBUG_ERROR, "ERROR: SMBIOS register - Invalid Generator\n"));
    return EFI_INVALID_PARAMETER;
  }

  if (!IS_GENERATOR_TYPE_SMBIOS (Generator->GeneratorID)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SMBIOS register - Generator" \
      " Type is not SMBIOS\n"
      ));
    return EFI_INVALID_PARAMETER;
  }

  DEBUG ((DEBUG_INFO, "Registering %s\n", Generator->Description));

  TableId = GET_TABLE_ID (Generator->GeneratorID);
  if (IS_GENERATOR_NAMESPACE_STD (Generator->GeneratorID)) {
    if (TableId >= EStdSmbiosTableIdMax) {
      ASSERT (TableId < EStdSmbiosTableIdMax);
      return EFI_INVALID_PARAMETER;
    }

    if (TableFactoryInfo.StdSmbiosTableGeneratorList[TableId] == NULL) {
      TableFactoryInfo.StdSmbiosTableGeneratorList[TableId] = Generator;
    } else {
      return EFI_ALREADY_STARTED;
    }
  } else {
    if (TableId > FixedPcdGet16 (PcdMaxCustomSMBIOSGenerators)) {
      ASSERT (TableId <= FixedPcdGet16 (PcdMaxCustomSMBIOSGenerators));
      return EFI_INVALID_PARAMETER;
    }

    if (TableFactoryInfo.CustomSmbiosTableGeneratorList[TableId] == NULL) {
      TableFactoryInfo.CustomSmbiosTableGeneratorList[TableId] = Generator;
    } else {
      return EFI_ALREADY_STARTED;
    }
  }

  return EFI_SUCCESS;
}

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
  IN  CONST SMBIOS_TABLE_GENERATOR              *CONST  Generator
  )
{
  UINT16  TableId;

  if (Generator == NULL) {
    DEBUG ((DEBUG_ERROR, "ERROR: SMBIOS deregister - Invalid Generator\n"));
    return EFI_INVALID_PARAMETER;
  }

  if (!IS_GENERATOR_TYPE_SMBIOS (Generator->GeneratorID)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SMBIOS deregister - Generator" \
      " Type is not SMBIOS\n"
      ));
    return EFI_INVALID_PARAMETER;
  }

  TableId = GET_TABLE_ID (Generator->GeneratorID);
  if (IS_GENERATOR_NAMESPACE_STD (Generator->GeneratorID)) {
    if (TableId >= EStdSmbiosTableIdMax) {
      ASSERT (TableId < EStdSmbiosTableIdMax);
      return EFI_INVALID_PARAMETER;
    }

    if (TableFactoryInfo.StdSmbiosTableGeneratorList[TableId] != NULL) {
      if (Generator != TableFactoryInfo.StdSmbiosTableGeneratorList[TableId]) {
        return EFI_INVALID_PARAMETER;
      }

      TableFactoryInfo.StdSmbiosTableGeneratorList[TableId] = NULL;
    } else {
      return EFI_NOT_FOUND;
    }
  } else {
    if (TableId > FixedPcdGet16 (PcdMaxCustomSMBIOSGenerators)) {
      ASSERT (TableId <= FixedPcdGet16 (PcdMaxCustomSMBIOSGenerators));
      return EFI_INVALID_PARAMETER;
    }

    if (TableFactoryInfo.CustomSmbiosTableGeneratorList[TableId] != NULL) {
      if (Generator !=
          TableFactoryInfo.CustomSmbiosTableGeneratorList[TableId])
      {
        return EFI_INVALID_PARAMETER;
      }

      TableFactoryInfo.CustomSmbiosTableGeneratorList[TableId] = NULL;
    } else {
      return EFI_NOT_FOUND;
    }
  }

  DEBUG ((DEBUG_INFO, "Deregistering %s\n", Generator->Description));
  return EFI_SUCCESS;
}
