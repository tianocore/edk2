/** @file
  SMBIOS Type 8 Port Connector Information Table Generator.

  Copyright (c) 2023 - 2026, NVIDIA CORPORATION & AFFILIATES. All rights reserved.<BR>
  Copyright (c) 2020 - 2021, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/SmbiosStringTableLib.h>

#include <ConfigurationManagerObject.h>
#include <ConfigurationManagerHelper.h>
#include <Protocol/ConfigurationManagerProtocol.h>
#include <Protocol/DynamicTableFactoryProtocol.h>
#include <IndustryStandard/SmBios.h>

/** SMBIOS Type 8 Port Connector Information Generator

Requirements:
  The following Configuration Manager Object is required by this Generator:
  - EArchCommonObjPortConnectorInfo
*/

#define SMBIOS_TYPE8_MAX_STRINGS  2

GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjPortConnectorInfo,
  CM_ARCH_COMMON_PORT_CONNECTOR_INFO
  );

/** Check whether a connector type is defined by SMBIOS 3.9.0.

  @param [in] ConnectorType  Connector type to validate.

  @retval TRUE   The connector type is defined.
  @retval FALSE  The connector type is reserved.
**/
STATIC
BOOLEAN
IsConnectorTypeValid (
  IN UINT8  ConnectorType
  )
{
  return ((ConnectorType <= PortConnectorTypeUsbTypeC) ||
          ((ConnectorType >= PortConnectorTypePC98) &&
           (ConnectorType <= PortConnectorTypePC98Full)) ||
          (ConnectorType == PortConnectorTypeOther));
}

/** Check whether a port type is defined by SMBIOS 3.9.0.

  @param [in] PortType  Port type to validate.

  @retval TRUE   The port type is defined.
  @retval FALSE  The port type is reserved.
**/
STATIC
BOOLEAN
IsPortTypeValid (
  IN UINT8  PortType
  )
{
  return ((PortType <= PortTypeThunderbolt) ||
          ((PortType >= PortType8251Compatible) &&
           (PortType <= PortType8251FifoCompatible)) ||
          (PortType == PortTypeOther));
}

/** Validate Port Connector Information.

  Validation follows SMBIOS Specification v3.9.0, Sections 6.1.3 and 7.9.

  @param [in] PortConnectorInfo  Port Connector Information to validate.

  @retval EFI_SUCCESS            The Port Connector Information is valid.
  @retval EFI_INVALID_PARAMETER  The Port Connector Information is invalid.
**/
STATIC
EFI_STATUS
ValidatePortConnectorInfo (
  IN CONST CM_ARCH_COMMON_PORT_CONNECTOR_INFO  *PortConnectorInfo
  )
{
  UINTN  InternalDesignatorLength;
  UINTN  ExternalDesignatorLength;

  InternalDesignatorLength = AsciiStrnLenS (
                               PortConnectorInfo->InternalReferenceDesignator,
                               SMBIOS_MAX_STRING_SIZE
                               );
  ExternalDesignatorLength = AsciiStrnLenS (
                               PortConnectorInfo->ExternalReferenceDesignator,
                               SMBIOS_MAX_STRING_SIZE
                               );

  if ((PortConnectorInfo->PortConnectorInfoToken == CM_NULL_TOKEN) ||
      (InternalDesignatorLength == SMBIOS_MAX_STRING_SIZE) ||
      (ExternalDesignatorLength == SMBIOS_MAX_STRING_SIZE) ||
      !IsConnectorTypeValid (PortConnectorInfo->InternalConnectorType) ||
      !IsConnectorTypeValid (PortConnectorInfo->ExternalConnectorType) ||
      !IsPortTypeValid (PortConnectorInfo->PortType))
  {
    DEBUG ((DEBUG_ERROR, "%a: Invalid Port Connector Information\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  if (((PortConnectorInfo->InternalConnectorType == PortConnectorTypeNone) &&
       (InternalDesignatorLength != 0)) ||
      ((PortConnectorInfo->ExternalConnectorType == PortConnectorTypeNone) &&
       (ExternalDesignatorLength != 0)))
  {
    DEBUG ((DEBUG_ERROR, "%a: An unused connector must have an empty designator\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  if (((PortConnectorInfo->InternalConnectorType == PortConnectorTypeOther) &&
       (InternalDesignatorLength == 0)) ||
      ((PortConnectorInfo->ExternalConnectorType == PortConnectorTypeOther) &&
       (ExternalDesignatorLength == 0)))
  {
    DEBUG ((DEBUG_ERROR, "%a: Connector type Other requires a reference designator\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

/** Add an optional string to a string table.

  @param [in,out] StrTable   String table receiving the string.
  @param [in]     String     Null-terminated string to add.
  @param [out]    StringRef  SMBIOS string reference.

  @retval EFI_SUCCESS  The string was added or was empty.
  @return              Error status returned by StringTableAddString().
**/
STATIC
EFI_STATUS
AddOptionalString (
  IN OUT STRING_TABLE         *StrTable,
  IN     CONST CHAR8          *String,
  OUT    SMBIOS_TABLE_STRING  *StringRef
  )
{
  *StringRef = 0;
  if (String[0] == '\0') {
    return EFI_SUCCESS;
  }

  return StringTableAddString (StrTable, String, StringRef);
}

/** Construct SMBIOS Type 8 tables describing port connectors.

  @param [in]  This                  Pointer to the SMBIOS table generator.
  @param [in]  TableFactoryProtocol  Pointer to the SMBIOS table factory.
  @param [in]  SmbiosTableInfo       Pointer to the SMBIOS table information.
  @param [in]  CfgMgrProtocol        Pointer to the Configuration Manager.
  @param [out] Table                 Pointer to the generated SMBIOS tables.
  @param [out] CmObjectToken         Pointer to the CM object tokens.
  @param [out] TableCount            Number of generated SMBIOS tables.

  @retval EFI_SUCCESS            Tables generated successfully.
  @retval EFI_INVALID_PARAMETER  A parameter or CM object is invalid.
  @retval EFI_NOT_FOUND          No Port Connector Information objects were found.
  @retval EFI_OUT_OF_RESOURCES   Could not allocate memory.
**/
STATIC
EFI_STATUS
EFIAPI
BuildSmbiosType8TableEx (
  IN  CONST SMBIOS_TABLE_GENERATOR                         *This,
  IN  CONST EDKII_DYNAMIC_TABLE_FACTORY_PROTOCOL   *CONST  TableFactoryProtocol,
  IN        CM_STD_OBJ_SMBIOS_TABLE_INFO           *CONST  SmbiosTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL   *CONST  CfgMgrProtocol,
  OUT       SMBIOS_STRUCTURE                               ***Table,
  OUT       CM_OBJECT_TOKEN                                **CmObjectToken,
  OUT       UINTN                                  *CONST  TableCount
  )
{
  EFI_STATUS                          Status;
  CM_ARCH_COMMON_PORT_CONNECTOR_INFO  *PortConnectorInfo;
  UINT32                              PortConnectorCount;
  SMBIOS_STRUCTURE                    **TableList;
  CM_OBJECT_TOKEN                     *CmObjectList;
  SMBIOS_TABLE_TYPE8                  *SmbiosRecord;
  STRING_TABLE                        StrTable;
  BOOLEAN                             StringTableInitialized;
  SMBIOS_TABLE_STRING                 InternalDesignatorRef;
  SMBIOS_TABLE_STRING                 ExternalDesignatorRef;
  UINTN                               Index;

  ASSERT (This != NULL);
  ASSERT (TableFactoryProtocol != NULL);
  ASSERT (SmbiosTableInfo != NULL);
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (Table != NULL);
  ASSERT (CmObjectToken != NULL);
  ASSERT (TableCount != NULL);
  ASSERT (SmbiosTableInfo->TableGeneratorId == This->GeneratorID);

  if ((This == NULL) || (TableFactoryProtocol == NULL) ||
      (SmbiosTableInfo == NULL) || (CfgMgrProtocol == NULL) ||
      (Table == NULL) || (CmObjectToken == NULL) || (TableCount == NULL) ||
      (SmbiosTableInfo->TableGeneratorId != This->GeneratorID))
  {
    DEBUG ((DEBUG_ERROR, "%a: Invalid Parameter\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  *Table                 = NULL;
  *CmObjectToken         = NULL;
  *TableCount            = 0;
  TableList              = NULL;
  CmObjectList           = NULL;
  SmbiosRecord           = NULL;
  StringTableInitialized = FALSE;

  Status = GetEArchCommonObjPortConnectorInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &PortConnectorInfo,
             &PortConnectorCount
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to get Port Connector objects: %r\n", __func__, Status));
    return Status;
  }

  if (PortConnectorCount == 0) {
    return EFI_NOT_FOUND;
  }

  if ((PortConnectorInfo == NULL) ||
      (PortConnectorCount > (MAX_UINT32 / sizeof (*TableList))) ||
      (PortConnectorCount > (MAX_UINT32 / sizeof (*CmObjectList))))
  {
    DEBUG ((DEBUG_ERROR, "%a: Invalid Port Connector object list\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  TableList = AllocateZeroPool (sizeof (*TableList) * PortConnectorCount);
  if (TableList == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  CmObjectList = AllocateZeroPool (sizeof (*CmObjectList) * PortConnectorCount);
  if (CmObjectList == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ErrorExit;
  }

  for (Index = 0; Index < PortConnectorCount; Index++) {
    Status = ValidatePortConnectorInfo (&PortConnectorInfo[Index]);
    if (EFI_ERROR (Status)) {
      goto ErrorExit;
    }

    Status = StringTableInitialize (&StrTable, SMBIOS_TYPE8_MAX_STRINGS);
    if (EFI_ERROR (Status)) {
      goto ErrorExit;
    }

    StringTableInitialized = TRUE;
    Status                 = AddOptionalString (
                               &StrTable,
                               PortConnectorInfo[Index].InternalReferenceDesignator,
                               &InternalDesignatorRef
                               );
    if (!EFI_ERROR (Status)) {
      Status = AddOptionalString (
                 &StrTable,
                 PortConnectorInfo[Index].ExternalReferenceDesignator,
                 &ExternalDesignatorRef
                 );
    }

    if (EFI_ERROR (Status)) {
      goto ErrorExit;
    }

    SmbiosRecord = AllocateSmbiosRecord (sizeof (*SmbiosRecord), &StrTable);
    if (SmbiosRecord == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto ErrorExit;
    }

    SmbiosRecord->Hdr.Type                    = EFI_SMBIOS_TYPE_PORT_CONNECTOR_INFORMATION;
    SmbiosRecord->Hdr.Length                  = sizeof (*SmbiosRecord);
    SmbiosRecord->InternalReferenceDesignator = InternalDesignatorRef;
    SmbiosRecord->InternalConnectorType       = PortConnectorInfo[Index].InternalConnectorType;
    SmbiosRecord->ExternalReferenceDesignator = ExternalDesignatorRef;
    SmbiosRecord->ExternalConnectorType       = PortConnectorInfo[Index].ExternalConnectorType;
    SmbiosRecord->PortType                    = PortConnectorInfo[Index].PortType;

    Status = StringTablePublishStringSet (
               &StrTable,
               (CHAR8 *)(SmbiosRecord + 1),
               StringTableGetStringSetSize (&StrTable)
               );
    StringTableFree (&StrTable);
    StringTableInitialized = FALSE;
    if (EFI_ERROR (Status)) {
      goto ErrorExit;
    }

    TableList[Index]    = (SMBIOS_STRUCTURE *)SmbiosRecord;
    CmObjectList[Index] = PortConnectorInfo[Index].PortConnectorInfoToken;
    SmbiosRecord        = NULL;
  }

  *Table         = TableList;
  *CmObjectToken = CmObjectList;
  *TableCount    = PortConnectorCount;
  return EFI_SUCCESS;

ErrorExit:
  if (StringTableInitialized) {
    StringTableFree (&StrTable);
  }

  if (SmbiosRecord != NULL) {
    FreePool (SmbiosRecord);
  }

  if (TableList != NULL) {
    for (Index = 0; Index < PortConnectorCount; Index++) {
      if (TableList[Index] != NULL) {
        FreePool (TableList[Index]);
      }
    }

    FreePool (TableList);
  }

  if (CmObjectList != NULL) {
    FreePool (CmObjectList);
  }

  return Status;
}

/** Free resources allocated when installing SMBIOS Type 8 tables.

  @param [in] This                  Pointer to the SMBIOS table generator.
  @param [in] TableFactoryProtocol  Pointer to the SMBIOS Table Factory.
  @param [in] SmbiosTableInfo       Pointer to the SMBIOS table information.
  @param [in] CfgMgrProtocol        Pointer to the Configuration Manager.
  @param [in] Table                 Pointer to the SMBIOS tables.
  @param [in] CmObjectToken         Pointer to the CM ObjectToken array.
  @param [in] TableCount            Number of SMBIOS tables.

  @retval EFI_SUCCESS            Resources freed successfully.
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.
**/
STATIC
EFI_STATUS
EFIAPI
FreeSmbiosType8TableEx (
  IN      CONST SMBIOS_TABLE_GENERATOR                   *CONST  This,
  IN      CONST EDKII_DYNAMIC_TABLE_FACTORY_PROTOCOL     *CONST  TableFactoryProtocol,
  IN      CONST CM_STD_OBJ_SMBIOS_TABLE_INFO             *CONST  SmbiosTableInfo,
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL     *CONST  CfgMgrProtocol,
  IN      SMBIOS_STRUCTURE                             ***CONST  Table,
  IN      CM_OBJECT_TOKEN                                        **CmObjectToken,
  IN      CONST UINTN                                            TableCount
  )
{
  UINTN  Index;

  if ((This == NULL) || (TableFactoryProtocol == NULL) ||
      (SmbiosTableInfo == NULL) || (CfgMgrProtocol == NULL) ||
      (Table == NULL) || (CmObjectToken == NULL))
  {
    DEBUG ((DEBUG_ERROR, "%a: Invalid Parameter\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  if (*Table != NULL) {
    for (Index = 0; Index < TableCount; Index++) {
      if ((*Table)[Index] != NULL) {
        FreePool ((*Table)[Index]);
      }
    }

    FreePool (*Table);
    *Table = NULL;
  }

  if (*CmObjectToken != NULL) {
    FreePool (*CmObjectToken);
    *CmObjectToken = NULL;
  }

  return EFI_SUCCESS;
}

/** The SMBIOS Type 8 Table Generator.
*/
STATIC CONST SMBIOS_TABLE_GENERATOR  SmbiosType8Generator = {
  CREATE_STD_SMBIOS_TABLE_GEN_ID (EStdSmbiosTableIdType08),
  L"SMBIOS.TYPE8.GENERATOR",
  EFI_SMBIOS_TYPE_PORT_CONNECTOR_INFORMATION,
  NULL,
  NULL,
  BuildSmbiosType8TableEx,
  FreeSmbiosType8TableEx
};

/** Register the Generator with the SMBIOS Table Factory.

  @param [in] ImageHandle  The handle to the image.
  @param [in] SystemTable  Pointer to the System Table.

  @retval EFI_SUCCESS            The Generator is registered.
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.
  @retval EFI_ALREADY_STARTED    The Generator is already registered.
**/
EFI_STATUS
EFIAPI
SmbiosType8LibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = RegisterSmbiosTableGenerator (&SmbiosType8Generator);
  DEBUG ((DEBUG_INFO, "SMBIOS Type 8: Register Generator. Status = %r\n", Status));
  ASSERT_EFI_ERROR (Status);
  return Status;
}

/** Deregister the Generator from the SMBIOS Table Factory.

  @param [in] ImageHandle  The handle to the image.
  @param [in] SystemTable  Pointer to the System Table.

  @retval EFI_SUCCESS            The Generator is deregistered.
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.
  @retval EFI_NOT_FOUND          The Generator is not registered.
**/
EFI_STATUS
EFIAPI
SmbiosType8LibDestructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = DeregisterSmbiosTableGenerator (&SmbiosType8Generator);
  DEBUG ((DEBUG_INFO, "SMBIOS Type 8: Deregister Generator. Status = %r\n", Status));
  ASSERT_EFI_ERROR (Status);
  return Status;
}
