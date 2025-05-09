/** @file
  SLIT Table Generator.

  Copyright (C) 2025 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Protocol/AcpiTable.h>

// Module specific include files.
#include <AcpiTableGenerator.h>
#include <ConfigurationManagerObject.h>
#include <ConfigurationManagerHelper.h>
#include <Library/TableHelperLib.h>
#include <Protocol/ConfigurationManagerProtocol.h>

/** Standard SLIT Generator.
Requirements:
  The following Configuration Manager Object(s) are required by
  this Generator:
  - EArchCommonObjSystemLocalityInfo
  - EArchCommonObjProxDomainRelationInfo
  - EArchCommonObjProxDomainInfo
*/

/** Structure representing information about domain relationships. */
typedef struct {
  UINT32    DomainIdSrc;
  UINT32    DomainIdDst;
  UINT32    Relation;
} DOMAIN_RELATION_INFO;

/** Retrieve the System locality information. */
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjSystemLocalityInfo,
  CM_ARCH_COMMON_SYSTEM_LOCALITY_INFO
  );

/** Retrieve the Proximity Domain relation information. */
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjProxDomainRelationInfo,
  CM_ARCH_COMMON_PROX_DOMAIN_RELATION_INFO
  );

/** Retrieve the Proximity Domain information. */
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjProxDomainInfo,
  CM_ARCH_COMMON_PROX_DOMAIN_INFO
  );

/** Retrieve the proximity domain information for System Locality.

  This function fetches the System Locality data from the
  Configuration Manager.

  The caller is responsible for freeing the memory allocated for
  the System Locality data, i.e., PxmDomainRelationInfo.

  @param [in]  CfgMgrProtocol         Pointer to the Configuration Manager
                                        Protocol.
  @param [out] RelationInfoCount      Pointer to the count of proximity
                                        domain information entries.
  @param [out] PxmDomainRelationInfo  Pointer to the proximity domain
                                        information for System Locality.
  @retval EFI_SUCCESS           Successfully retrieved proximity domain information.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval retval                Errors returned by the Configuration Manager Protocol.
**/
STATIC
EFI_STATUS
EFIAPI
GetProximityDomainInfo (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  OUT UINT32                                              *RelationInfoCount,
  OUT DOMAIN_RELATION_INFO                                **PxmDomainRelationInfo
  )
{
  CM_ARCH_COMMON_PROX_DOMAIN_INFO           *DomainInfoFirst;
  CM_ARCH_COMMON_PROX_DOMAIN_INFO           *DomainInfoSecond;
  CM_ARCH_COMMON_PROX_DOMAIN_RELATION_INFO  *DomainRelationInfo;
  CM_ARCH_COMMON_SYSTEM_LOCALITY_INFO       *SystemLocalityInfo;
  DOMAIN_RELATION_INFO                      *RelationInfo;
  EFI_STATUS                                Status;
  UINT32                                    DomainRelationInfoCount;
  UINT32                                    Index;

  if ((CfgMgrProtocol == NULL) ||
      (RelationInfoCount == NULL) ||
      (PxmDomainRelationInfo == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  SystemLocalityInfo = NULL;
  Status             = GetEArchCommonObjSystemLocalityInfo (
                         CfgMgrProtocol,
                         CM_NULL_TOKEN,
                         &SystemLocalityInfo,
                         NULL
                         );

  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SLIT: Failed to retrieve SLIT information. Status = %r\n",
      Status
      ));
    return Status;
  }

  if (SystemLocalityInfo == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SLIT: No SLIT information provided by configuration manager.\n"
      ));
    return EFI_NOT_FOUND;
  }

  DomainRelationInfo      = NULL;
  DomainRelationInfoCount = 0;
  Status                  = GetEArchCommonObjProxDomainRelationInfo (
                              CfgMgrProtocol,
                              SystemLocalityInfo->RelativeDistanceArray,
                              &DomainRelationInfo,
                              &DomainRelationInfoCount
                              );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SLIT: Failed to get Domain relation info. Status = %r\n",
      Status
      ));
    return Status;
  }

  if ((DomainRelationInfo == NULL) || (DomainRelationInfoCount == 0)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SLIT: No Domain relation info from config manager.\n"
      ));
    return EFI_NOT_FOUND;
  }

  RelationInfo = AllocateZeroPool (
                   sizeof (DOMAIN_RELATION_INFO) * DomainRelationInfoCount
                   );
  if (RelationInfo == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SLIT: Failed to allocate memory for SLIT relation info.\n"
      ));
    return EFI_OUT_OF_RESOURCES;
  }

  for (Index = 0; Index < DomainRelationInfoCount; Index++) {
    if ((DomainRelationInfo[Index].Relation < 10) ||
        (DomainRelationInfo[Index].Relation > MAX_UINT8))
    {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SLIT: Invalid relation value %d\n",
        DomainRelationInfo[Index].Relation
        ));
      FreePool (RelationInfo);
      return EFI_INVALID_PARAMETER;
    }

    Status = GetEArchCommonObjProxDomainInfo (
               CfgMgrProtocol,
               DomainRelationInfo[Index].FirstDomainToken,
               &DomainInfoFirst,
               NULL
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SLIT: Failed to get First Domain info. Status = %r\n",
        Status
        ));
      FreePool (RelationInfo);
      return Status;
    }

    Status = GetEArchCommonObjProxDomainInfo (
               CfgMgrProtocol,
               DomainRelationInfo[Index].SecondDomainToken,
               &DomainInfoSecond,
               NULL
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SLIT: Failed to get Second Domain info. Status = %r\n",
        Status
        ));
      FreePool (RelationInfo);
      return Status;
    }

    if ((DomainInfoFirst == NULL) || (DomainInfoSecond == NULL)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SLIT: Missing Proximity Domain info from config manager.\n"
        ));
      FreePool (RelationInfo);
      return EFI_INVALID_PARAMETER;
    }

    if (DomainInfoFirst->DomainId == DomainInfoSecond->DomainId) {
      if (DomainRelationInfo[Index].Relation != 10) {
        DEBUG ((
          DEBUG_ERROR,
          "ERROR: SLIT: Invalid relation value %d for same domain ID %d\n",
          DomainRelationInfo[Index].Relation,
          DomainInfoFirst->DomainId
          ));
        FreePool (RelationInfo);
        return EFI_INVALID_PARAMETER;
      }
    }

    RelationInfo[Index].DomainIdSrc = DomainInfoFirst->DomainId;
    RelationInfo[Index].DomainIdDst = DomainInfoSecond->DomainId;
    RelationInfo[Index].Relation    = DomainRelationInfo[Index].Relation;
  }

  *RelationInfoCount     = DomainRelationInfoCount;
  *PxmDomainRelationInfo = RelationInfo;
  return EFI_SUCCESS;
}

/** Get the number of System Localities.

  This function calculates the number of System Localities based on
  the maximum locality ID found in the SLIT domain relation information.

  @param [in]  RelationInfo       Pointer to the SLIT domain relation information.
  @param [in]  RelationInfoCount  The count of SLIT domain relation information entries.
  @param [out] NumberOfSystemLocalities    Pointer to the number of System Localities.

  @retval EFI_SUCCESS           Successfully retrieved the number of System Localities.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
**/
STATIC
EFI_STATUS
EFIAPI
GetNumberOfSystemLocalities (
  IN DOMAIN_RELATION_INFO  *RelationInfo,
  IN UINT32                RelationInfoCount,
  OUT UINT32               *NumberOfSystemLocalities
  )
{
  UINT32  Index;
  UINT32  Locality;

  if ((RelationInfo == NULL) ||
      (NumberOfSystemLocalities == NULL) ||
      (RelationInfoCount == 0))
  {
    return EFI_INVALID_PARAMETER;
  }

  Locality = 0;
  for (Index = 0; Index < RelationInfoCount; Index++) {
    Locality = MAX (
                 Locality,
                 MAX (
                   RelationInfo[Index].DomainIdSrc,
                   RelationInfo[Index].DomainIdDst
                   )
                 );
  }

  *NumberOfSystemLocalities = Locality + 1;
  return EFI_SUCCESS;
}

/** Update the SLIT entry.

  This function constructs the SLIT entry based on the SLIT domain relation
  information and the number of System Localities.

  @param [in]  DomainRelationInfo Pointer to the SLIT domain relation information.
  @param [in]  RelationInfoCount  The count of SLIT domain relation information entries.
  @param [in]  LocalityCount The number of System Localities.
  @param [in, out] SlitEntry  Pointer to the SLIT table entry.

  @retval EFI_SUCCESS           Successfully retrieved the SLIT entry.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
**/
STATIC
EFI_STATUS
EFIAPI
UpdateSlitEntry (
  IN DOMAIN_RELATION_INFO  *DomainRelationInfo,
  IN UINT32                RelationInfoCount,
  IN UINT32                LocalityCount,
  IN OUT UINT8             *SlitEntry
  )
{
  UINT32  Index;
  UINT8   LocalitySrc;
  UINT8   LocalityDst;

  if ((DomainRelationInfo == NULL) ||
      (SlitEntry == NULL) ||
      (LocalityCount == 0) ||
      (RelationInfoCount == 0))
  {
    return EFI_INVALID_PARAMETER;
  }

  SetMem (
    SlitEntry,
    sizeof (UINT8) * LocalityCount * LocalityCount,
    0xFF
    );

  for (Index = 0; Index < LocalityCount; Index++) {
    SlitEntry[(Index * LocalityCount) + Index] = 10;
  }

  for (Index = 0; Index < RelationInfoCount; Index++) {
    LocalitySrc = DomainRelationInfo[Index].DomainIdSrc;
    LocalityDst = DomainRelationInfo[Index].DomainIdDst;

    SlitEntry[(LocalitySrc * LocalityCount) + LocalityDst] = DomainRelationInfo[Index].Relation;
    if (SlitEntry[(LocalityDst * LocalityCount) + LocalitySrc] != 0xFF) {
      SlitEntry[(LocalityDst * LocalityCount) + LocalitySrc] = DomainRelationInfo[Index].Relation;
    }
  }

  return EFI_SUCCESS;
}

/** Construct the SLIT ACPI table.

  This function invokes the Configuration Manager protocol interface
  to get the required hardware information for generating the ACPI
  table.

  If this function allocates any resources then they must be freed
  in the FreeXXXXTableResources function.

  @param [in]  This           Pointer to the table generator.
  @param [in]  AcpiTableInfo  Pointer to the ACPI Table Info.
  @param [in]  CfgMgrProtocol Configuration Manager Protocol Interface pointer.
  @param [out] Table          Pointer to the constructed ACPI Table.

  @retval EFI_SUCCESS           Table generated successfully.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         The required object was not found.
  @retval EFI_BAD_BUFFER_SIZE   The size returned by the Configuration
                                Manager is less than the Object size for the
                                requested object.
**/
STATIC
EFI_STATUS
EFIAPI
BuildSlitTable (
  IN  CONST ACPI_TABLE_GENERATOR                  *CONST  This,
  IN  CONST CM_STD_OBJ_ACPI_TABLE_INFO            *CONST  AcpiTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  OUT       EFI_ACPI_DESCRIPTION_HEADER          **CONST  Table
  )
{
  DOMAIN_RELATION_INFO  *RelationInfo;
  EFI_STATUS            Status;
  UINT32                NumberOfSystemLocalities;
  UINT32                RelationInfoCount;
  UINT8                 *SlitEntry;

  EFI_ACPI_6_5_SYSTEM_LOCALITY_DISTANCE_INFORMATION_TABLE_HEADER  *AcpiSlitTable;

  ASSERT (This != NULL);
  ASSERT (AcpiTableInfo != NULL);
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (Table != NULL);
  ASSERT (AcpiTableInfo->TableGeneratorId == This->GeneratorID);
  ASSERT (AcpiTableInfo->AcpiTableSignature == This->AcpiTableSignature);

  if ((AcpiTableInfo->AcpiTableRevision < This->MinAcpiTableRevision) ||
      (AcpiTableInfo->AcpiTableRevision > This->AcpiTableRevision))
  {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SLIT: Requested table revision = %d, is not supported."
      "Supported table revision: Minimum = %d, Maximum = %d\n",
      AcpiTableInfo->AcpiTableRevision,
      This->MinAcpiTableRevision,
      This->AcpiTableRevision
      ));
    return EFI_INVALID_PARAMETER;
  }

  *Table = NULL;

  RelationInfoCount = 0;
  RelationInfo      = NULL;
  Status            = GetProximityDomainInfo (
                        CfgMgrProtocol,
                        &RelationInfoCount,
                        &RelationInfo
                        );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SLIT: Failed to get proximity domain relation information. Status = %r\n",
      Status
      ));
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  if ((RelationInfoCount == 0) ||
      (RelationInfo == NULL))
  {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SLIT: No Proximity Domain relation information provided by configuration manager.\n"
      ));
    ASSERT_EFI_ERROR (EFI_NOT_FOUND);
    return EFI_NOT_FOUND;
  }

  NumberOfSystemLocalities = 0;
  Status                   = GetNumberOfSystemLocalities (
                               RelationInfo,
                               RelationInfoCount,
                               &NumberOfSystemLocalities
                               );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SLIT: Failed to get NumberOfSystemLocalities. Status = %r\n",
      Status
      ));
    FreePool (RelationInfo);
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  if ((NumberOfSystemLocalities == 0) ||
      (NumberOfSystemLocalities > MAX_UINT8))
  {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SLIT: No System Locality information provided by configuration manager.\n"
      ));
    FreePool (RelationInfo);
    ASSERT_EFI_ERROR (EFI_NOT_FOUND);
    return EFI_NOT_FOUND;
  }

  AcpiSlitTable = AllocateZeroPool (
                    sizeof (EFI_ACPI_6_5_SYSTEM_LOCALITY_DISTANCE_INFORMATION_TABLE_HEADER) +
                    (sizeof (UINT8) * NumberOfSystemLocalities * NumberOfSystemLocalities)
                    );
  if (AcpiSlitTable == NULL) {
    DEBUG (
      (DEBUG_ERROR, "ERROR: SLIT: Failed to allocate memory for SLIT table.\n"));
    FreePool (RelationInfo);
    ASSERT_EFI_ERROR (EFI_OUT_OF_RESOURCES);
    return EFI_OUT_OF_RESOURCES;
  }

  AcpiSlitTable->NumberOfSystemLocalities = NumberOfSystemLocalities;

  SlitEntry = (UINT8 *)AcpiSlitTable;
  SlitEntry = SlitEntry + sizeof (EFI_ACPI_6_5_SYSTEM_LOCALITY_DISTANCE_INFORMATION_TABLE_HEADER);

  Status = UpdateSlitEntry (
             RelationInfo,
             RelationInfoCount,
             NumberOfSystemLocalities,
             SlitEntry
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SLIT: Failed to update SLIT entry. Status = %r\n",
      Status
      ));
    FreePool (AcpiSlitTable);
    FreePool (RelationInfo);
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  FreePool (RelationInfo);
  Status = AddAcpiHeader (
             CfgMgrProtocol,
             This,
             (EFI_ACPI_DESCRIPTION_HEADER *)AcpiSlitTable,
             AcpiTableInfo,
             (sizeof (EFI_ACPI_6_5_SYSTEM_LOCALITY_DISTANCE_INFORMATION_TABLE_HEADER) +
              (sizeof (UINT8) * NumberOfSystemLocalities * NumberOfSystemLocalities))
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SLIT: Failed to add ACPI header. Status = %r\n",
      Status
      ));
    FreePool (AcpiSlitTable);
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  *Table = (EFI_ACPI_DESCRIPTION_HEADER *)AcpiSlitTable;
  return Status;
}

/** This macro defines the SLIT Table Generator revision.
*/
#define SLIT_GENERATOR_REVISION  CREATE_REVISION (1, 0)

/** The interface for the SLIT Table Generator.
*/
STATIC
CONST
ACPI_TABLE_GENERATOR  SpmiGenerator = {
  // Generator ID
  CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdSlit),
  // Generator Description
  L"ACPI.STD.SLIT.GENERATOR",
  // ACPI Table Signature
  EFI_ACPI_6_5_SYSTEM_LOCALITY_INFORMATION_TABLE_SIGNATURE,
  // ACPI Table Revision supported by this Generator
  EFI_ACPI_6_5_SYSTEM_LOCALITY_DISTANCE_INFORMATION_TABLE_REVISION,
  // Minimum supported ACPI Table Revision
  EFI_ACPI_6_5_SYSTEM_LOCALITY_DISTANCE_INFORMATION_TABLE_REVISION,
  // Creator ID
  TABLE_GENERATOR_CREATOR_ID,
  // Creator Revision
  SLIT_GENERATOR_REVISION,
  // Build Table function
  BuildSlitTable,
  // Free Resource function
  NULL,
  // Extended build function not needed
  NULL,
  // Extended build function not implemented by the generator.
  // Hence extended free resource function is not required.
  NULL
};

/** Register the Generator with the ACPI Table Factory.

  @param [in]  ImageHandle  The handle to the image.
  @param [in]  SystemTable  Pointer to the System Table.

  @retval EFI_SUCCESS           The Generator is registered.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_ALREADY_STARTED   The Generator for the Table ID
                                is already registered.
**/
EFI_STATUS
EFIAPI
AcpiSlitLibConstructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = RegisterAcpiTableGenerator (&SpmiGenerator);
  DEBUG ((DEBUG_INFO, "SLIT: Register Generator. Status = %r\n", Status));
  ASSERT_EFI_ERROR (Status);
  return Status;
}

/** Deregister the Generator from the ACPI Table Factory.

  @param [in]  ImageHandle  The handle to the image.
  @param [in]  SystemTable  Pointer to the System Table.

  @retval EFI_SUCCESS           The Generator is deregistered.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         The Generator is not registered.
**/
EFI_STATUS
EFIAPI
AcpiSlitLibDestructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = DeregisterAcpiTableGenerator (&SpmiGenerator);
  DEBUG ((DEBUG_INFO, "SLIT: Deregister Generator. Status = %r\n", Status));
  ASSERT_EFI_ERROR (Status);
  return Status;
}
