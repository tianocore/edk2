/** @file
  SLIT Table Generator

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
#include <Library/CmObjHelperLib.h>

#define SLIT_DISTANCE_NORMALIZED   10
#define SLIT_DISTANCE_UNREACHABLE  0xFF

/** Structure to hold domain relation information.
 */
typedef struct {
  UINT32    DomainIdSrc;
  UINT32    DomainIdDst;
  UINT64    Relation;
} DOMAIN_RELATION_INFO;

/** Standard SLIT Generator

Requirements:
  The following Configuration Manager Object(s) are required by
  this Generator:
  - EArchCommonObjSystemLocalityInfo
  - EArchCommonObjProximityDomainRelationInfo
  - EArchCommonObjProximityDomainInfo
*/

/** Retrieve the System locality information. */
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjSystemLocalityInfo,
  CM_ARCH_COMMON_SYSTEM_LOCALITY_INFO
  );

/** Retrieve the Proximity Domain relation information. */
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjProximityDomainRelationInfo,
  CM_ARCH_COMMON_PROXIMITY_DOMAIN_RELATION_INFO
  );

/** Retrieve the System Locality domain information.

  This function fetches the System Locality data from the
  Configuration Manager.

  The caller is responsible for freeing the memory allocated for
  the System Locality data, i.e., SlitDomainRelationInfo.

  @param [in]  CfgMgrProtocol         Pointer to the Configuration Manager Protocol.
  @param [out] SlitDomainRelationInfoCount  Pointer to the count of System Locality
                                            domain information entries.
  @param [out] SlitDomainRelationInfo Pointer to the System Locality domain information.

  @retval EFI_SUCCESS           Successfully retrieved the System Locality domain information.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval retval                Errors returned by the Configuration Manager Protocol.
**/
STATIC
EFI_STATUS
EFIAPI
GetProximityDomainInfo (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  OUT UINT32                                              *SlitDomainRelationInfoCount,
  OUT DOMAIN_RELATION_INFO                                **SlitDomainRelationInfo
  )
{
  CM_ARCH_COMMON_PROXIMITY_DOMAIN_RELATION_INFO  *DomainRelationInfo;
  CM_ARCH_COMMON_SYSTEM_LOCALITY_INFO            *SystemLocalityInfo;
  DOMAIN_RELATION_INFO                           *RelationInfo;
  EFI_STATUS                                     Status;
  UINT32                                         DomainIdFirst;
  UINT32                                         DomainIdSecond;
  UINT32                                         DomainRelationInfoCount;
  UINT32                                         Index;

  if ((CfgMgrProtocol == NULL) ||
      (SlitDomainRelationInfoCount == NULL) ||
      (SlitDomainRelationInfo == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  Status = GetEArchCommonObjSystemLocalityInfo (
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

  Status = GetEArchCommonObjProximityDomainRelationInfo (
             CfgMgrProtocol,
             SystemLocalityInfo->RelativeDistanceArray,
             &DomainRelationInfo,
             &DomainRelationInfoCount
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SLIT: Failed to get domain relation info. Status = %r\n",
      Status
      ));
    return Status;
  }

  if ((DomainRelationInfo == NULL) || (DomainRelationInfoCount == 0)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SLIT: No domain relation info from config manager.\n"
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
    if ((DomainRelationInfo[Index].Relation < SLIT_DISTANCE_NORMALIZED) ||
        (DomainRelationInfo[Index].Relation > SLIT_DISTANCE_UNREACHABLE))
    {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SLIT: Invalid relation value %d\n",
        DomainRelationInfo[Index].Relation
        ));
      FreePool (RelationInfo);
      return EFI_INVALID_PARAMETER;
    }

    Status = GetProximityDomainId (
               CfgMgrProtocol,
               0,
               DomainRelationInfo[Index].FirstDomainToken,
               &DomainIdFirst
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SLIT: Failed to retrieve First Domain ID. Status = %r\n",
        Status
        ));
      FreePool (RelationInfo);
      return Status;
    }

    Status = GetProximityDomainId (
               CfgMgrProtocol,
               0,
               DomainRelationInfo[Index].SecondDomainToken,
               &DomainIdSecond
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SLIT: Failed to retrieve Second Domain ID. Status = %r\n",
        Status
        ));
      FreePool (RelationInfo);
      return Status;
    }

    if (DomainIdFirst == DomainIdSecond) {
      if (DomainRelationInfo[Index].Relation != SLIT_DISTANCE_NORMALIZED) {
        DEBUG ((
          DEBUG_ERROR,
          "ERROR: SLIT: Invalid relation value %d for same domain ID %d\n",
          DomainRelationInfo[Index].Relation,
          DomainIdFirst
          ));
        FreePool (RelationInfo);
        return EFI_INVALID_PARAMETER;
      }
    }

    RelationInfo[Index].DomainIdSrc = DomainIdFirst;
    RelationInfo[Index].DomainIdDst = DomainIdSecond;
    RelationInfo[Index].Relation    = DomainRelationInfo[Index].Relation;
  }

  *SlitDomainRelationInfoCount = DomainRelationInfoCount;
  *SlitDomainRelationInfo      = RelationInfo;
  return EFI_SUCCESS;
}

/** Get the number of System Localities.

  This function calculates the number of System Localities based on
  the maximum locality ID found in the SLIT domain relation information.

  @param [in]  SlitDomainRelationInfo       Pointer to the SLIT domain relation information.
  @param [in]  SlitDomainRelationInfoCount  The count of SLIT domain relation information entries.
  @param [out] NumberOfSystemLocalities    Pointer to the number of System Localities.

  @retval EFI_SUCCESS           Successfully retrieved the number of System Localities.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid
**/
STATIC
EFI_STATUS
EFIAPI
GetNumberOfSystemLocalities (
  IN DOMAIN_RELATION_INFO  *SlitDomainRelationInfo,
  IN UINT32                SlitDomainRelationInfoCount,
  OUT UINT32               *NumberOfSystemLocalities
  )
{
  UINT32  Index;
  UINT32  MaxLocality;

  if ((SlitDomainRelationInfo == NULL) ||
      (NumberOfSystemLocalities == NULL) ||
      (SlitDomainRelationInfoCount == 0))
  {
    return EFI_INVALID_PARAMETER;
  }

  MaxLocality = 0;
  for (Index = 0; Index < SlitDomainRelationInfoCount; Index++) {
    MaxLocality = MAX (
                    MaxLocality,
                    MAX (
                      SlitDomainRelationInfo[Index].DomainIdSrc,
                      SlitDomainRelationInfo[Index].DomainIdDst
                      )
                    );
  }

  *NumberOfSystemLocalities = MaxLocality + 1;
  return EFI_SUCCESS;
}

/** Get the SLIT entry.

  This function constructs the SLIT entry based on the SLIT domain relation
  information and the number of System Localities.

  @param [in]  SlitDomainRelationInfo       Pointer to the SLIT domain relation information.
  @param [in]  SlitDomainRelationInfoCount  The count of SLIT domain relation information entries.
  @param [in]  NumberOfSystemLocalities     The number of System Localities.
  @param [in, out] SlitEntry                Pointer to the SLIT entry.

  @retval EFI_SUCCESS           Successfully retrieved the SLIT entry.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
**/
STATIC
EFI_STATUS
EFIAPI
GetSlitEntry (
  IN DOMAIN_RELATION_INFO  *SlitDomainRelationInfo,
  IN UINT32                SlitDomainRelationInfoCount,
  IN UINT32                NumberOfSystemLocalities,
  IN OUT UINT8             *SlitEntry
  )
{
  UINT32  Index;
  UINT32  LocalitySrc;
  UINT32  LocalityDst;
  UINT32  SrcIndex;
  UINT32  DstIndex;

  if ((SlitDomainRelationInfo == NULL) ||
      (SlitEntry == NULL) ||
      (NumberOfSystemLocalities == 0) ||
      (SlitDomainRelationInfoCount == 0))
  {
    return EFI_INVALID_PARAMETER;
  }

  SetMem (
    SlitEntry,
    sizeof (UINT8) * NumberOfSystemLocalities * NumberOfSystemLocalities,
    SLIT_DISTANCE_UNREACHABLE
    );
  for (Index = 0; Index < NumberOfSystemLocalities; Index++) {
    SlitEntry[(Index * NumberOfSystemLocalities) + Index] = SLIT_DISTANCE_NORMALIZED;
  }

  for (Index = 0; Index < SlitDomainRelationInfoCount; Index++) {
    LocalitySrc = SlitDomainRelationInfo[Index].DomainIdSrc;
    LocalityDst = SlitDomainRelationInfo[Index].DomainIdDst;
    SrcIndex    = (LocalitySrc * NumberOfSystemLocalities) + LocalityDst;
    DstIndex    = (LocalityDst * NumberOfSystemLocalities) + LocalitySrc;

    /// compare with default normalized
    if (SlitDomainRelationInfo[Index].Relation == SLIT_DISTANCE_NORMALIZED) {
      if ((SlitEntry[SrcIndex] != SLIT_DISTANCE_NORMALIZED) ||
          (SlitEntry[DstIndex] != SLIT_DISTANCE_NORMALIZED))
      {
        DEBUG ((
          DEBUG_ERROR,
          "ERROR: SLIT: Invalid normalized value Src[%d] = %d, Dst[%d] = %d.\n",
          SrcIndex,
          SlitEntry[SrcIndex],
          DstIndex,
          SlitEntry[DstIndex]
          ));
        return EFI_INVALID_PARAMETER;
      }

      continue;
    }

    /// Compare with default unreachable and symmetric value
    if (SlitDomainRelationInfo[Index].Relation == SLIT_DISTANCE_UNREACHABLE) {
      if ((SlitEntry[SrcIndex] != SLIT_DISTANCE_UNREACHABLE) ||
          (SlitEntry[DstIndex] != SLIT_DISTANCE_UNREACHABLE))
      {
        DEBUG ((
          DEBUG_ERROR,
          "ERROR: SLIT: Invalid unreachable value Src[%d] = %d, Dst[%d] = %d\n",
          SrcIndex,
          SlitEntry[SrcIndex],
          DstIndex,
          SlitEntry[DstIndex]
          ));
        return EFI_INVALID_PARAMETER;
      }

      continue;
    }

    if ((SlitEntry[SrcIndex] != SLIT_DISTANCE_UNREACHABLE) &&
        (SlitEntry[SrcIndex] != SlitDomainRelationInfo[Index].Relation))
    {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SLIT: Invalid existing value %d for Src %d Dst %d\n",
        SlitEntry[SrcIndex],
        LocalitySrc,
        LocalityDst
        ));
      return EFI_INVALID_PARAMETER;
    }

    SlitEntry[SrcIndex] = (SlitDomainRelationInfo[Index].Relation & MAX_UINT8);

    if ((SlitEntry[DstIndex] != SLIT_DISTANCE_UNREACHABLE) &&
        (SlitEntry[DstIndex] != SlitDomainRelationInfo[Index].Relation))
    {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SLIT: Invalid existing value %d for Dst %d Src %d\n",
        SlitEntry[DstIndex],
        LocalityDst,
        LocalitySrc
        ));
      return EFI_INVALID_PARAMETER;
    }

    SlitEntry[DstIndex] = (SlitDomainRelationInfo[Index].Relation & MAX_UINT8);
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
  @param [in]  CfgMgrProtocol Pointer to the Configuration Manager
                                Protocol Interface.
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
  DOMAIN_RELATION_INFO  *SlitDomainRelationInfo;
  EFI_STATUS            Status;
  UINT32                NumberOfSystemLocalities;
  UINT32                SlitDomainRelationInfoCount;
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

  Status = GetProximityDomainInfo (
             CfgMgrProtocol,
             &SlitDomainRelationInfoCount,
             &SlitDomainRelationInfo
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SLIT: Failed to get domain info. Status = %r\n",
      Status
      ));
    return Status;
  }

  Status = GetNumberOfSystemLocalities (
             SlitDomainRelationInfo,
             SlitDomainRelationInfoCount,
             &NumberOfSystemLocalities
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SLIT: Failed to get NumberOfSystemLocalities. Status = %r\n",
      Status
      ));
    FreePool (SlitDomainRelationInfo);
    return Status;
  }

  AcpiSlitTable = AllocateZeroPool (
                    sizeof (EFI_ACPI_6_5_SYSTEM_LOCALITY_DISTANCE_INFORMATION_TABLE_HEADER) +
                    (sizeof (UINT8) * NumberOfSystemLocalities * NumberOfSystemLocalities)
                    );
  if (AcpiSlitTable == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SLIT: Failed to allocate memory for SLIT table.\n"
      ));
    FreePool (SlitDomainRelationInfo);
    return EFI_OUT_OF_RESOURCES;
  }

  AcpiSlitTable->NumberOfSystemLocalities = NumberOfSystemLocalities;

  SlitEntry = (UINT8 *)AcpiSlitTable;
  SlitEntry = SlitEntry + sizeof (EFI_ACPI_6_5_SYSTEM_LOCALITY_DISTANCE_INFORMATION_TABLE_HEADER);

  Status = GetSlitEntry (
             SlitDomainRelationInfo,
             SlitDomainRelationInfoCount,
             NumberOfSystemLocalities,
             SlitEntry
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SLIT: Failed to get SLIT entry. Status = %r\n",
      Status
      ));
    FreePool (AcpiSlitTable);
    FreePool (SlitDomainRelationInfo);
    return Status;
  }

  FreePool (SlitDomainRelationInfo);
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
