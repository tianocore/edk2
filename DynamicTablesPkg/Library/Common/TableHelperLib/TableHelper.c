/** @file
  Table Helper

Copyright (c) 2017 - 2019, ARM Limited. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Protocol/AcpiTable.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>

// Module specific include files.
#include <AcpiTableGenerator.h>
#include <ConfigurationManagerObject.h>
#include <Protocol/ConfigurationManagerProtocol.h>

/** The GetCgfMgrInfo function gets the CM_STD_OBJ_CONFIGURATION_MANAGER_INFO
    object from the Configuration Manager.

  @param [in]  CfgMgrProtocol Pointer to the Configuration Manager protocol
                              interface.
  @param [out] CfgMfrInfo     Pointer to the Configuration Manager Info
                              object structure.

  @retval EFI_SUCCESS           The object is returned.
  @retval EFI_INVALID_PARAMETER The Object ID is invalid.
  @retval EFI_NOT_FOUND         The requested Object is not found.
  @retval EFI_BAD_BUFFER_SIZE   The size returned by the Configuration
                                Manager is less than the Object size.
**/
EFI_STATUS
EFIAPI
GetCgfMgrInfo (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL      * CONST  CfgMgrProtocol,
  OUT       CM_STD_OBJ_CONFIGURATION_MANAGER_INFO    **        CfgMfrInfo
  )
{
  EFI_STATUS         Status;
  CM_OBJ_DESCRIPTOR  CmObjectDesc;

  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (CfgMfrInfo != NULL);

  *CfgMfrInfo = NULL;
  Status = CfgMgrProtocol->GetObject (
                             CfgMgrProtocol,
                             CREATE_CM_STD_OBJECT_ID (EStdObjCfgMgrInfo),
                             CM_NULL_TOKEN,
                             &CmObjectDesc
                             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: Failed to Get Configuration Manager Info. Status = %r\n",
      Status
      ));
    return Status;
  }

  if (CmObjectDesc.ObjectId != CREATE_CM_STD_OBJECT_ID (EStdObjCfgMgrInfo)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: EStdObjCfgMgrInfo: Invalid ObjectId = 0x%x, expected Id = 0x%x\n",
      CmObjectDesc.ObjectId,
      CREATE_CM_STD_OBJECT_ID (EStdObjCfgMgrInfo)
      ));
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  if (CmObjectDesc.Size <
      (sizeof (CM_STD_OBJ_CONFIGURATION_MANAGER_INFO) * CmObjectDesc.Count)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: EStdObjCfgMgrInfo: Buffer too small, size  = 0x%x\n",
      CmObjectDesc.Size
      ));
    ASSERT (FALSE);
    return EFI_BAD_BUFFER_SIZE;
  }

  *CfgMfrInfo = (CM_STD_OBJ_CONFIGURATION_MANAGER_INFO*)CmObjectDesc.Data;
  return Status;
}

/** The AddAcpiHeader function updates the ACPI header structure pointed by
    the AcpiHeader. It utilizes the ACPI table Generator and the Configuration
    Manager protocol to obtain any information required for constructing the
    header.

  @param [in]     CfgMgrProtocol Pointer to the Configuration Manager
                                 protocol interface.
  @param [in]     Generator      Pointer to the ACPI table Generator.
  @param [in,out] AcpiHeader     Pointer to the ACPI table header to be
                                 updated.
  @param [in]     AcpiTableInfo  Pointer to the ACPI table info structure.
  @param [in]     Length         Length of the ACPI table.

  @retval EFI_SUCCESS           The ACPI table is updated successfully.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         The required object information is not found.
  @retval EFI_BAD_BUFFER_SIZE   The size returned by the Configuration
                                Manager is less than the Object size for the
                                requested object.
**/
EFI_STATUS
EFIAPI
AddAcpiHeader (
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  * CONST CfgMgrProtocol,
  IN      CONST ACPI_TABLE_GENERATOR                  * CONST Generator,
  IN OUT  EFI_ACPI_DESCRIPTION_HEADER                 * CONST AcpiHeader,
  IN      CONST CM_STD_OBJ_ACPI_TABLE_INFO            * CONST AcpiTableInfo,
  IN      CONST UINT32                                        Length
  )
{
  EFI_STATUS                               Status;
  CM_STD_OBJ_CONFIGURATION_MANAGER_INFO  * CfgMfrInfo;

  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (Generator != NULL);
  ASSERT (AcpiHeader != NULL);
  ASSERT (Length >= sizeof (EFI_ACPI_DESCRIPTION_HEADER));

  if ((CfgMgrProtocol == NULL) ||
      (Generator == NULL) ||
      (AcpiHeader == NULL) ||
      (Length < sizeof (EFI_ACPI_DESCRIPTION_HEADER))
    ) {
    return EFI_INVALID_PARAMETER;
  }

  Status = GetCgfMgrInfo (CfgMgrProtocol, &CfgMfrInfo);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: Failed to get Configuration Manager info. Status = %r\n",
      Status
      ));
    goto error_handler;
  }

  // UINT32  Signature
  AcpiHeader->Signature = Generator->AcpiTableSignature;
  // UINT32  Length
  AcpiHeader->Length = Length;
  // UINT8   Revision
  AcpiHeader->Revision = AcpiTableInfo->AcpiTableRevision;
  // UINT8   Checksum
  AcpiHeader->Checksum = 0;

  // UINT8   OemId[6]
  CopyMem (AcpiHeader->OemId, CfgMfrInfo->OemId, sizeof (AcpiHeader->OemId));

  // UINT64  OemTableId
  if (AcpiTableInfo->OemTableId != 0) {
    AcpiHeader->OemTableId = AcpiTableInfo->OemTableId;
  } else {
    AcpiHeader->OemTableId = SIGNATURE_32 (
                               CfgMfrInfo->OemId[0],
                               CfgMfrInfo->OemId[1],
                               CfgMfrInfo->OemId[2],
                               CfgMfrInfo->OemId[3]
                               ) |
                             ((UINT64)Generator->AcpiTableSignature << 32);
  }

  // UINT32  OemRevision
  if (AcpiTableInfo->OemRevision != 0) {
    AcpiHeader->OemRevision = AcpiTableInfo->OemRevision;
  } else {
    AcpiHeader->OemRevision = CfgMfrInfo->Revision;
  }

  // UINT32  CreatorId
  AcpiHeader->CreatorId = Generator->CreatorId;
  // UINT32  CreatorRevision
  AcpiHeader->CreatorRevision = Generator->CreatorRevision;

error_handler:
  return Status;
}
