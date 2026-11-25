/** @file
  Configuration Manager Helper Library.

  Copyright (c) 2025 - 2026, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>

// Module specific include files.
#include <AcpiTableGenerator.h>
#include <ConfigurationManagerObject.h>
#include <ConfigurationManagerHelper.h>
#include <MetadataHelpers.h>
#include <Library/MetadataHandlerLib.h>
#include <Protocol/ConfigurationManagerProtocol.h>

/**
  This macro expands to a function that retrieves the ACPI Table list
  information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceStandard,
  EStdObjAcpiTableList,
  CM_STD_OBJ_ACPI_TABLE_INFO
  );

/**
  This macro expands to a function that retrieves Proximity Domain
  information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjProximityDomainInfo,
  CM_ARCH_COMMON_PROXIMITY_DOMAIN_INFO
  );

/** Check if an ACPI table is present in the Configuration manager's ACPI table list.

  @param [in]  CfgMgrProtocol         Pointer to the Configuration Manager
                                      Protocol Interface.
  @param [in]  AcpiTableId            Acpi Table Id.

  @retval TRUE if the ACPI table is in the list of ACPI tables to install.
          FALSE otherwise.
**/
BOOLEAN
EFIAPI
CheckAcpiTablePresent (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN        ESTD_ACPI_TABLE_ID                            AcpiTableId
  )
{
  EFI_STATUS                  Status;
  CM_STD_OBJ_ACPI_TABLE_INFO  *AcpiTableList;
  UINT32                      AcpiTableListCount;
  UINT32                      Index;

  Status = GetEStdObjAcpiTableList (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &AcpiTableList,
             &AcpiTableListCount
             );
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return FALSE;
  }

  for (Index = 0; Index < AcpiTableListCount; Index++) {
    if (AcpiTableList[Index].TableGeneratorId ==
        CREATE_STD_ACPI_TABLE_GEN_ID (AcpiTableId))
    {
      return TRUE;
    }
  }

  return FALSE;
}

/** Get a Proximity Domain Id.

  Proximity Domain Id are now to be placed in
  CM_ARCH_COMMON_PROXIMITY_DOMAIN_INFO objects rather than in the various
  CmObj using them. This function handles the logic in the selection
  of the ProximityDomainId to use.

  Proximity Domain Id should be preferably placed in
  CM_ARCH_COMMON_PROXIMITY_DOMAIN_INFO objects now.

  DefaultDomainId is used only when Token is CM_NULL_TOKEN. For a non-null
  Token, any error retrieving the referenced object is returned without
  falling back to DefaultDomainId.

  @param [in]  CfgMgrProtocol   Pointer to the Configuration Manager
                                Protocol Interface.
  @param [in]  DefaultDomainId  Default per-CmObj Proximity Domain Id, used
                                only when Token is CM_NULL_TOKEN.
                                The CM_ARCH_COMMON_PROXIMITY_DOMAIN_INFO
                                should be preferably used.
  @param [in]  Token            Token referencing a
                                CM_ARCH_COMMON_PROXIMITY_DOMAIN_INFO object.
  @param [out] DomainId         If Success, contains DomainId to use.

  @retval EFI_SUCCESS           The Proximity Domain Id was returned.
  @retval EFI_INVALID_PARAMETER CfgMgrProtocol or DomainId is NULL, or a
                                parameter is invalid during retrieval
                                or metadata processing.
  @retval EFI_NOT_FOUND         A non-null Token could not be resolved.
  @return                       Other errors from object retrieval or
                                metadata processing are propagated.
**/
EFI_STATUS
EFIAPI
GetProximityDomainId (
  IN CONST  EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CfgMgrProtocol,
  IN        UINT32                                DefaultDomainId,
  IN        CM_OBJECT_TOKEN                       Token,
  OUT       UINT32                                *DomainId
  )
{
  EFI_STATUS                            Status;
  CM_ARCH_COMMON_PROXIMITY_DOMAIN_INFO  *ProximityDomain;
  METADATA_OBJ_PROXIMITY_DOMAIN         Metadata;

  if ((CfgMgrProtocol == NULL) ||
      (DomainId == NULL))
  {
    ASSERT (CfgMgrProtocol != NULL);
    ASSERT (DomainId != NULL);
    return EFI_INVALID_PARAMETER;
  }

  ProximityDomain = NULL;

  if (Token != CM_NULL_TOKEN) {
    Status = GetEArchCommonObjProximityDomainInfo (
               CfgMgrProtocol,
               Token,
               &ProximityDomain,
               NULL
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: CM_OBJ_HELPER_LIB: Failed to get Proximity Domain. Status=%r Token=%llx\n",
        Status,
        Token
        ));
      return Status;
    }
  }

  if ((ProximityDomain != NULL) &&
      (ProximityDomain->GenerateDomainId))
  {
    // A Proximity Domain was found and the Id must be generated.
    Status = MetadataHandlerGenerate (
               GetMetadataRoot (),
               MetadataTypeProximityDomain,
               Token,
               NULL,
               &Metadata,
               sizeof (METADATA_OBJ_PROXIMITY_DOMAIN)
               );
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      return Status;
    }

    *DomainId = Metadata.Id;
  } else {
    // A hard-coded DomainId is used.

    if (Token == CM_NULL_TOKEN) {
      // No ProximityDomain token was supplied.
      // Use the default Id.
      *DomainId = DefaultDomainId;

      DEBUG ((
        DEBUG_WARN,
        "CM_OBJ_HELPER_LIB: Proximity Domain Ids should be described using "
        "the new EArchCommonObjProximityDomainInfo object. "
        "The field currently used will be deprecated.\n"
        ));
    } else if (!ProximityDomain->GenerateDomainId) {
      // A ProximityDomain CmObj was found, but generation is disabled.
      // Use the Domain Id.
      *DomainId   = ProximityDomain->DomainId;
      Metadata.Id = *DomainId;

      // Add the Domain Id to the Metadata database to check duplicated values.
      Status = MetadataAdd (
                 GetMetadataRoot (),
                 MetadataTypeProximityDomain,
                 Token,
                 &Metadata,
                 sizeof (METADATA_OBJ_PROXIMITY_DOMAIN)
                 );
      if (EFI_ERROR (Status) && (Status != EFI_ALREADY_STARTED)) {
        ASSERT_EFI_ERROR (Status);
        return Status;
      }
    } else {
      ASSERT_EFI_ERROR (Status);
    }
  }

  return EFI_SUCCESS;
}
