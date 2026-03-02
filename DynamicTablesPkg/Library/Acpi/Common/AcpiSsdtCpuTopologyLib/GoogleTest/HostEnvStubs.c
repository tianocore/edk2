/** @file
  Host environment stubs for SSDT CPU Topology unit tests.

  These stub implementations provide the DXE driver functions that are not
  available in the host unit test environment. The stubs allow the tests
  to build and run without the full DXE runtime environment.

  Both ARM and X64 tests compile sources directly (not via library) to enable
  cross-architecture testing on x64 host. This requires stubbing:
  - GetMetadataRoot() - only implemented in DynamicTableFactoryDxe driver
  - MetadataHandlerGenerate() - from MetadataHandlerLib
  - CheckAcpiTablePresent() - from CmObjHelperLib

  Copyright (c) 2025, NVIDIA CORPORATION & AFFILIATES. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <ConfigurationManagerObject.h>
#include <AcpiTableGenerator.h>
#include <Library/MetadataObjLib.h>
#include <MetadataHelpers.h>
#include <Protocol/ConfigurationManagerProtocol.h>

/** Static mock metadata root for host tests.

  This is set to NULL since we don't need actual metadata functionality
  in the unit test environment. The stub functions handle this appropriately.
*/
STATIC METADATA_ROOT_HANDLE  mMockMetadataRoot = NULL;

/** Static counter for generating sequential UIDs in tests. */
STATIC UINT32  mNextUid = 0;

/** Stub implementation of GetMetadataRoot.

  In the actual firmware, this is implemented in DynamicTableFactoryDxe.
  For host tests, we return a mock handle.

  @return The mock Metadata Root handle (NULL).
**/
METADATA_ROOT_HANDLE
EFIAPI
GetMetadataRoot (
  VOID
  )
{
  return mMockMetadataRoot;
}

/** Stub implementation of MetadataHandlerGenerate.

  In the actual firmware, this queries or generates metadata and stores it
  in the metadata database. For host tests, we simply generate sequential UIDs.

  @param[in]       Root          Root of the Metadata information.
  @param[in]       Type          METADATA_TYPE of the entry to generate.
  @param[in]       Token         Token uniquely identifying an entry.
  @param[in]       Context       Optional context (unused in stub).
  @param[in, out]  Metadata      On output, contains generated Metadata.
  @param[in]       MetadataSize  Size of the input Metadata.

  @retval EFI_SUCCESS           Always succeeds for tests.
  @retval EFI_INVALID_PARAMETER If Metadata is NULL.
**/
EFI_STATUS
EFIAPI
MetadataHandlerGenerate (
  IN      METADATA_ROOT_HANDLE  Root,
  IN      METADATA_TYPE         Type,
  IN      CM_OBJECT_TOKEN       Token,
  IN      VOID                  *Context,
  IN OUT  VOID                  *Metadata,
  IN      UINT32                MetadataSize
  )
{
  if (Metadata == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // For MetadataTypeUid, assign a sequential UID
  if ((Type == MetadataTypeUid) && (MetadataSize >= sizeof (METADATA_OBJ_UID))) {
    METADATA_OBJ_UID  *Uid = (METADATA_OBJ_UID *)Metadata;
    Uid->Uid = mNextUid++;
  }
  // For MetadataTypeProximityDomain, assign a sequential domain ID
  else if ((Type == MetadataTypeProximityDomain) && (MetadataSize >= sizeof (METADATA_OBJ_PROXIMITY_DOMAIN))) {
    METADATA_OBJ_PROXIMITY_DOMAIN  *Domain = (METADATA_OBJ_PROXIMITY_DOMAIN *)Metadata;
    Domain->Id = mNextUid++;
  }

  return EFI_SUCCESS;
}

/** Stub implementation of CheckAcpiTablePresent.

  In the actual firmware, this checks if an ACPI table is in the
  Configuration Manager's table list. For host tests, we return FALSE
  to indicate no PPTT table is present, which affects topology generation.

  @param[in]  CfgMgrProtocol  Pointer to the Configuration Manager Protocol.
  @param[in]  AcpiTableId     ACPI Table Id to check.

  @retval FALSE  Always returns FALSE for host tests.
**/
BOOLEAN
EFIAPI
CheckAcpiTablePresent (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN        ESTD_ACPI_TABLE_ID                            AcpiTableId
  )
{
  // For unit tests, assume no PPTT or other tables are present
  // This simplifies test setup and validation
  return FALSE;
}

/** Reset the metadata UID counter.

  This function should be called between tests to ensure consistent
  UID generation across test runs.
**/
VOID
EFIAPI
ResetMetadataUidCounter (
  VOID
  )
{
  mNextUid = 0;
}
