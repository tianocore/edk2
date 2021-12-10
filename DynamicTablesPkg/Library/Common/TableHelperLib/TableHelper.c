/** @file
  Table Helper

  Copyright (c) 2017 - 2021, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Protocol/AcpiTable.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>

// Module specific include files.
#include <AcpiTableGenerator.h>
#include <ConfigurationManagerObject.h>
#include <Library/TableHelperLib.h>
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
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL      *CONST  CfgMgrProtocol,
  OUT       CM_STD_OBJ_CONFIGURATION_MANAGER_INFO             **CfgMfrInfo
  )
{
  EFI_STATUS         Status;
  CM_OBJ_DESCRIPTOR  CmObjectDesc;

  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (CfgMfrInfo != NULL);

  *CfgMfrInfo = NULL;
  Status      = CfgMgrProtocol->GetObject (
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
      (sizeof (CM_STD_OBJ_CONFIGURATION_MANAGER_INFO) * CmObjectDesc.Count))
  {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: EStdObjCfgMgrInfo: Buffer too small, size  = 0x%x\n",
      CmObjectDesc.Size
      ));
    ASSERT (FALSE);
    return EFI_BAD_BUFFER_SIZE;
  }

  *CfgMfrInfo = (CM_STD_OBJ_CONFIGURATION_MANAGER_INFO *)CmObjectDesc.Data;
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
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN      CONST ACPI_TABLE_GENERATOR                  *CONST  Generator,
  IN OUT  EFI_ACPI_DESCRIPTION_HEADER                 *CONST  AcpiHeader,
  IN      CONST CM_STD_OBJ_ACPI_TABLE_INFO            *CONST  AcpiTableInfo,
  IN      CONST UINT32                                        Length
  )
{
  EFI_STATUS                             Status;
  CM_STD_OBJ_CONFIGURATION_MANAGER_INFO  *CfgMfrInfo;

  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (Generator != NULL);
  ASSERT (AcpiHeader != NULL);
  ASSERT (AcpiTableInfo != NULL);
  ASSERT (Length >= sizeof (EFI_ACPI_DESCRIPTION_HEADER));

  if ((CfgMgrProtocol == NULL) ||
      (Generator == NULL) ||
      (AcpiHeader == NULL) ||
      (AcpiTableInfo == NULL) ||
      (Length < sizeof (EFI_ACPI_DESCRIPTION_HEADER))
      )
  {
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

/** Build a RootNode containing SSDT ACPI header information using the AmlLib.

  The function utilizes the ACPI table Generator and the Configuration
  Manager protocol to obtain any information required for constructing the
  header. It then creates a RootNode. The SSDT ACPI header is part of the
  RootNode.

  This is essentially a wrapper around AmlCodeGenDefinitionBlock ()
  from the AmlLib.

  @param [in]   CfgMgrProtocol Pointer to the Configuration Manager
                               protocol interface.
  @param [in]   Generator      Pointer to the ACPI table Generator.
  @param [in]   AcpiTableInfo  Pointer to the ACPI table info structure.
  @param [out]  RootNode       If success, contains the created RootNode.
                               The SSDT ACPI header is part of the RootNode.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         The required object information is not found.
  @retval EFI_BAD_BUFFER_SIZE   The size returned by the Configuration
                                Manager is less than the Object size for the
                                requested object.
**/
EFI_STATUS
EFIAPI
AddSsdtAcpiHeader (
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN      CONST ACPI_TABLE_GENERATOR                  *CONST  Generator,
  IN      CONST CM_STD_OBJ_ACPI_TABLE_INFO            *CONST  AcpiTableInfo,
  OUT       AML_ROOT_NODE_HANDLE                              *RootNode
  )
{
  EFI_STATUS                             Status;
  UINT64                                 OemTableId;
  UINT32                                 OemRevision;
  CM_STD_OBJ_CONFIGURATION_MANAGER_INFO  *CfgMfrInfo;

  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (Generator != NULL);
  ASSERT (AcpiTableInfo != NULL);

  if ((CfgMgrProtocol == NULL)  ||
      (Generator == NULL)       ||
      (AcpiTableInfo == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  Status = GetCgfMgrInfo (CfgMgrProtocol, &CfgMfrInfo);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: Failed to get Configuration Manager info. Status = %r\n",
      Status
      ));
    return Status;
  }

  if (AcpiTableInfo->OemTableId != 0) {
    OemTableId = AcpiTableInfo->OemTableId;
  } else {
    OemTableId = SIGNATURE_32 (
                   CfgMfrInfo->OemId[0],
                   CfgMfrInfo->OemId[1],
                   CfgMfrInfo->OemId[2],
                   CfgMfrInfo->OemId[3]
                   ) |
                 ((UINT64)Generator->AcpiTableSignature << 32);
  }

  if (AcpiTableInfo->OemRevision != 0) {
    OemRevision = AcpiTableInfo->OemRevision;
  } else {
    OemRevision = CfgMfrInfo->Revision;
  }

  Status = AmlCodeGenDefinitionBlock (
             "SSDT",
             (CONST CHAR8 *)&CfgMfrInfo->OemId,
             (CONST CHAR8 *)&OemTableId,
             OemRevision,
             RootNode
             );
  ASSERT_EFI_ERROR (Status);
  return Status;
}

/**
  Test and report if a duplicate entry exists in the given array of comparable
  elements.

  @param [in] Array                 Array of elements to test for duplicates.
  @param [in] Count                 Number of elements in Array.
  @param [in] ElementSize           Size of an element in bytes
  @param [in] EqualTestFunction     The function to call to check if any two
                                    elements are equal.

  @retval TRUE                      A duplicate element was found or one of
                                    the input arguments is invalid.
  @retval FALSE                     Every element in Array is unique.
**/
BOOLEAN
EFIAPI
FindDuplicateValue (
  IN  CONST VOID          *Array,
  IN  CONST UINTN         Count,
  IN  CONST UINTN         ElementSize,
  IN        PFN_IS_EQUAL  EqualTestFunction
  )
{
  UINTN  Index1;
  UINTN  Index2;
  UINT8  *Element1;
  UINT8  *Element2;

  if (Array == NULL) {
    DEBUG ((DEBUG_ERROR, "ERROR: FindDuplicateValues: Array is NULL.\n"));
    return TRUE;
  }

  if (ElementSize == 0) {
    DEBUG ((DEBUG_ERROR, "ERROR: FindDuplicateValues: ElementSize is 0.\n"));
    return TRUE;
  }

  if (EqualTestFunction == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: FindDuplicateValues: EqualTestFunction is NULL.\n"
      ));
    return TRUE;
  }

  if (Count < 2) {
    return FALSE;
  }

  for (Index1 = 0; Index1 < Count - 1; Index1++) {
    for (Index2 = Index1 + 1; Index2 < Count; Index2++) {
      Element1 = (UINT8 *)Array + (Index1 * ElementSize);
      Element2 = (UINT8 *)Array + (Index2 * ElementSize);

      if (EqualTestFunction (Element1, Element2, Index1, Index2)) {
        return TRUE;
      }
    }
  }

  return FALSE;
}
