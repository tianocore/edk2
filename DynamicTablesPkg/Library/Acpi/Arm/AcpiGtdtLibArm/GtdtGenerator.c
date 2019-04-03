/** @file
  GTDT Table Generator

  Copyright (c) 2017 - 2019, ARM Limited. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - ACPI 6.2 Specification - Errata A, September 2017

**/

#include <Library/AcpiLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Protocol/AcpiTable.h>

// Module specific include files.
#include <AcpiTableGenerator.h>
#include <ConfigurationManagerObject.h>
#include <ConfigurationManagerHelper.h>
#include <Library/TableHelperLib.h>
#include <Protocol/ConfigurationManagerProtocol.h>

/** ARM standard GTDT Generator

Requirements:
  The following Configuration Manager Object(s) are required by
  this Generator:
  - EArmObjGenericTimerInfo
  - EArmObjPlatformGenericWatchdogInfo (OPTIONAL)
  - EArmObjPlatformGTBlockInfo (OPTIONAL)
  - EArmObjGTBlockTimerFrameInfo (OPTIONAL)
*/

/** This macro expands to a function that retrieves the Generic
    Timer Information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArm,
  EArmObjGenericTimerInfo,
  CM_ARM_GENERIC_TIMER_INFO
  );

/** This macro expands to a function that retrieves the SBSA Generic
    Watchdog Timer Information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArm,
  EArmObjPlatformGenericWatchdogInfo,
  CM_ARM_GENERIC_WATCHDOG_INFO
  );

/** This macro expands to a function that retrieves the Platform Generic
    Timer Block Information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArm,
  EArmObjPlatformGTBlockInfo,
  CM_ARM_GTBLOCK_INFO
  );

/** This macro expands to a function that retrieves the Generic
  Timer Block Timer Frame Information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArm,
  EArmObjGTBlockTimerFrameInfo,
  CM_ARM_GTBLOCK_TIMER_FRAME_INFO
  );

/** Add the Generic Timer Information to the GTDT table.

  Also update the Platform Timer offset information if the platform
  implements platform timers.

  @param [in]  CfgMgrProtocol     Pointer to the Configuration Manager
                                  Protocol Interface.
  @param [in]  Gtdt               Pointer to the GTDT Table.
  @param [in]  PlatformTimerCount Platform timer count.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         The required object was not found.
  @retval EFI_BAD_BUFFER_SIZE   The size returned by the Configuration
                                Manager is less than the Object size for the
                                requested object.
**/
STATIC
EFI_STATUS
EFIAPI
AddGenericTimerInfo (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL         * CONST CfgMgrProtocol,
  IN        EFI_ACPI_6_2_GENERIC_TIMER_DESCRIPTION_TABLE * CONST Gtdt,
  IN  CONST UINT32                                               PlatformTimerCount
)
{
  EFI_STATUS                   Status;
  CM_ARM_GENERIC_TIMER_INFO  * GenericTimerInfo;

  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (Gtdt != NULL);

  Status = GetEArmObjGenericTimerInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &GenericTimerInfo,
             NULL
             );

  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: GTDT: Failed to get GenericTimerInfo. Status = %r\n",
      Status
      ));
    return Status;
  }

  Gtdt->CntControlBasePhysicalAddress =
    GenericTimerInfo->CounterControlBaseAddress;
  Gtdt->Reserved = EFI_ACPI_RESERVED_DWORD;
  Gtdt->SecurePL1TimerGSIV = GenericTimerInfo->SecurePL1TimerGSIV;
  Gtdt->SecurePL1TimerFlags = GenericTimerInfo->SecurePL1TimerFlags;
  Gtdt->NonSecurePL1TimerGSIV = GenericTimerInfo->NonSecurePL1TimerGSIV;
  Gtdt->NonSecurePL1TimerFlags = GenericTimerInfo->NonSecurePL1TimerFlags;
  Gtdt->VirtualTimerGSIV = GenericTimerInfo->VirtualTimerGSIV;
  Gtdt->VirtualTimerFlags = GenericTimerInfo->VirtualTimerFlags;
  Gtdt->NonSecurePL2TimerGSIV = GenericTimerInfo->NonSecurePL2TimerGSIV;
  Gtdt->NonSecurePL2TimerFlags = GenericTimerInfo->NonSecurePL2TimerFlags;
  Gtdt->CntReadBasePhysicalAddress =
    GenericTimerInfo->CounterReadBaseAddress;
  Gtdt->PlatformTimerCount = PlatformTimerCount;
  Gtdt->PlatformTimerOffset = (PlatformTimerCount == 0) ? 0 :
    sizeof (EFI_ACPI_6_2_GENERIC_TIMER_DESCRIPTION_TABLE);

  return EFI_SUCCESS;
}

/** Add the SBSA Generic Watchdog Timers to the GTDT table.

  @param [in]  Gtdt             Pointer to the GTDT Table.
  @param [in]  WatchdogOffset   Offset to the watchdog information in the
                                GTDT Table.
  @param [in]  WatchdogInfoList Pointer to the watchdog information list.
  @param [in]  WatchdogCount    Platform timer count.
**/
STATIC
VOID
AddGenericWatchdogList (
  IN EFI_ACPI_6_2_GENERIC_TIMER_DESCRIPTION_TABLE  * CONST Gtdt,
  IN CONST UINT32                                          WatchdogOffset,
  IN CONST CM_ARM_GENERIC_WATCHDOG_INFO            *       WatchdogInfoList,
  IN       UINT32                                          WatchdogCount
  )
{
  EFI_ACPI_6_2_GTDT_SBSA_GENERIC_WATCHDOG_STRUCTURE  * Watchdog;

  ASSERT (Gtdt != NULL);
  ASSERT (WatchdogInfoList != NULL);

  Watchdog = (EFI_ACPI_6_2_GTDT_SBSA_GENERIC_WATCHDOG_STRUCTURE *)
             ((UINT8*)Gtdt + WatchdogOffset);

  while (WatchdogCount-- != 0) {
    // Add watchdog entry
    DEBUG ((DEBUG_INFO, "GTDT: Watchdog = 0x%p\n", Watchdog));
    Watchdog->Type = EFI_ACPI_6_2_GTDT_SBSA_GENERIC_WATCHDOG;
    Watchdog->Length =
      sizeof (EFI_ACPI_6_2_GTDT_SBSA_GENERIC_WATCHDOG_STRUCTURE);
    Watchdog->Reserved = EFI_ACPI_RESERVED_BYTE;
    Watchdog->RefreshFramePhysicalAddress =
      WatchdogInfoList->RefreshFrameAddress;
    Watchdog->WatchdogControlFramePhysicalAddress =
      WatchdogInfoList->ControlFrameAddress;
    Watchdog->WatchdogTimerGSIV = WatchdogInfoList->TimerGSIV;
    Watchdog->WatchdogTimerFlags = WatchdogInfoList->Flags;
    Watchdog++;
    WatchdogInfoList++;
  } // for
}

/** Update the GT Block Timer Frame lists in the GTDT Table.

  @param [in]  GtBlockFrame           Pointer to the GT Block Frames
                                      list to be updated.
  @param [in]  GTBlockTimerFrameList  Pointer to the GT Block Frame
                                      Information List.
  @param [in]  GTBlockFrameCount      Number of GT Block Frames.

  @retval EFI_SUCCESS           Table generated successfully.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
**/
STATIC
EFI_STATUS
AddGTBlockTimerFrames (
  IN       EFI_ACPI_6_2_GTDT_GT_BLOCK_TIMER_STRUCTURE *       GtBlockFrame,
  IN CONST CM_ARM_GTBLOCK_TIMER_FRAME_INFO            *       GTBlockTimerFrameList,
  IN       UINT32                                             GTBlockFrameCount
)
{
  ASSERT (GtBlockFrame != NULL);
  ASSERT (GTBlockTimerFrameList != NULL);

  if (GTBlockFrameCount > 8) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: GTDT: GT Block Frame Count %d is greater than 8\n",
      GTBlockFrameCount
      ));
    ASSERT (GTBlockFrameCount <= 8);
    return EFI_INVALID_PARAMETER;
  }

  while (GTBlockFrameCount-- != 0) {
    DEBUG ((
      DEBUG_INFO,
      "GTDT: GtBlockFrame = 0x%p\n",
      GtBlockFrame
      ));

    GtBlockFrame->GTFrameNumber = GTBlockTimerFrameList->FrameNumber;
    GtBlockFrame->Reserved[0] = EFI_ACPI_RESERVED_BYTE;
    GtBlockFrame->Reserved[1] = EFI_ACPI_RESERVED_BYTE;
    GtBlockFrame->Reserved[2] = EFI_ACPI_RESERVED_BYTE;

    GtBlockFrame->CntBaseX = GTBlockTimerFrameList->PhysicalAddressCntBase;
    GtBlockFrame->CntEL0BaseX =
      GTBlockTimerFrameList->PhysicalAddressCntEL0Base;

    GtBlockFrame->GTxPhysicalTimerGSIV =
      GTBlockTimerFrameList->PhysicalTimerGSIV;
    GtBlockFrame->GTxPhysicalTimerFlags =
      GTBlockTimerFrameList->PhysicalTimerFlags;

    GtBlockFrame->GTxVirtualTimerGSIV = GTBlockTimerFrameList->VirtualTimerGSIV;
    GtBlockFrame->GTxVirtualTimerFlags =
      GTBlockTimerFrameList->VirtualTimerFlags;

    GtBlockFrame->GTxCommonFlags = GTBlockTimerFrameList->CommonFlags;
    GtBlockFrame++;
    GTBlockTimerFrameList++;
  } // for
  return EFI_SUCCESS;
}

/** Add the GT Block Timers in the GTDT Table.

  @param [in]  CfgMgrProtocol   Pointer to the Configuration Manager
                                Protocol Interface.
  @param [in]  Gtdt             Pointer to the GTDT Table.
  @param [in]  GTBlockOffset    Offset of the GT Block
                                information in the GTDT Table.
  @param [in]  GTBlockInfo      Pointer to the GT Block
                                Information List.
  @param [in]  BlockTimerCount  Number of GT Block Timers.

  @retval EFI_SUCCESS           Table generated successfully.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
**/
STATIC
EFI_STATUS
AddGTBlockList (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL     * CONST CfgMgrProtocol,
  IN EFI_ACPI_6_2_GENERIC_TIMER_DESCRIPTION_TABLE    * CONST Gtdt,
  IN CONST UINT32                                            GTBlockOffset,
  IN CONST CM_ARM_GTBLOCK_INFO                       *       GTBlockInfo,
  IN       UINT32                                            BlockTimerCount
)
{
  EFI_STATUS                                    Status;
  EFI_ACPI_6_2_GTDT_GT_BLOCK_STRUCTURE        * GTBlock;
  EFI_ACPI_6_2_GTDT_GT_BLOCK_TIMER_STRUCTURE  * GtBlockFrame;
  CM_ARM_GTBLOCK_TIMER_FRAME_INFO             * GTBlockTimerFrameList;
  UINT32                                        GTBlockTimerFrameCount;

  ASSERT (Gtdt != NULL);
  ASSERT (GTBlockInfo != NULL);

  GTBlock = (EFI_ACPI_6_2_GTDT_GT_BLOCK_STRUCTURE *)((UINT8*)Gtdt +
              GTBlockOffset);

  while (BlockTimerCount-- != 0) {
    DEBUG ((DEBUG_INFO, "GTDT: GTBlock = 0x%p\n", GTBlock));

    Status = GetEArmObjGTBlockTimerFrameInfo (
               CfgMgrProtocol,
               GTBlockInfo->GTBlockTimerFrameToken,
               &GTBlockTimerFrameList,
               &GTBlockTimerFrameCount
               );
    if (EFI_ERROR (Status) ||
        (GTBlockTimerFrameCount != GTBlockInfo->GTBlockTimerFrameCount)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: GTDT: Failed to get Generic Timer Frames. Status = %r\n",
        Status
        ));
      return Status;
    }

    GTBlock->Type = EFI_ACPI_6_2_GTDT_GT_BLOCK;
    GTBlock->Length = sizeof (EFI_ACPI_6_2_GTDT_GT_BLOCK_STRUCTURE) +
                        (sizeof (EFI_ACPI_6_2_GTDT_GT_BLOCK_TIMER_STRUCTURE) *
                          GTBlockInfo->GTBlockTimerFrameCount);

    GTBlock->Reserved = EFI_ACPI_RESERVED_BYTE;
    GTBlock->CntCtlBase = GTBlockInfo->GTBlockPhysicalAddress;
    GTBlock->GTBlockTimerCount = GTBlockInfo->GTBlockTimerFrameCount;
    GTBlock->GTBlockTimerOffset =
      sizeof (EFI_ACPI_6_2_GTDT_GT_BLOCK_STRUCTURE);

    GtBlockFrame = (EFI_ACPI_6_2_GTDT_GT_BLOCK_TIMER_STRUCTURE*)
      ((UINT8*)GTBlock + GTBlock->GTBlockTimerOffset);

    // Add GT Block Timer frames
    Status = AddGTBlockTimerFrames (
               GtBlockFrame,
               GTBlockTimerFrameList,
               GTBlockTimerFrameCount
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: GTDT: Failed to add Generic Timer Frames. Status = %r\n",
        Status
        ));
      return Status;
    }

    // Next GTBlock
    GTBlock = (EFI_ACPI_6_2_GTDT_GT_BLOCK_STRUCTURE *)((UINT8*)GTBlock +
               GTBlock->Length);
    GTBlockInfo++;
  }// for
  return EFI_SUCCESS;
}

/** Construct the GTDT ACPI table.

  Called by the Dynamic Table Manager, this function invokes the
  Configuration Manager protocol interface to get the required hardware
  information for generating the ACPI table.

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
  @retval EFI_OUT_OF_RESOURCES  Memory allocation failed.
**/
STATIC
EFI_STATUS
EFIAPI
BuildGtdtTable (
  IN  CONST ACPI_TABLE_GENERATOR                  * CONST This,
  IN  CONST CM_STD_OBJ_ACPI_TABLE_INFO            * CONST AcpiTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  * CONST CfgMgrProtocol,
  OUT       EFI_ACPI_DESCRIPTION_HEADER          ** CONST Table
  )
{
  EFI_STATUS                                      Status;
  UINT32                                          TableSize;
  UINT32                                          PlatformTimerCount;
  UINT32                                          WatchdogCount;
  UINT32                                          BlockTimerCount;
  CM_ARM_GENERIC_WATCHDOG_INFO                  * WatchdogInfoList;
  CM_ARM_GTBLOCK_INFO                           * GTBlockInfo;
  EFI_ACPI_6_2_GENERIC_TIMER_DESCRIPTION_TABLE  * Gtdt;
  UINT32                                          Idx;
  UINT32                                          GTBlockOffset;
  UINT32                                          WatchdogOffset;

  ASSERT (This != NULL);
  ASSERT (AcpiTableInfo != NULL);
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (Table != NULL);
  ASSERT (AcpiTableInfo->TableGeneratorId == This->GeneratorID);
  ASSERT (AcpiTableInfo->AcpiTableSignature == This->AcpiTableSignature);

  if ((AcpiTableInfo->AcpiTableRevision < This->MinAcpiTableRevision) ||
      (AcpiTableInfo->AcpiTableRevision > This->AcpiTableRevision)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: GTDT: Requested table revision = %d, is not supported."
      "Supported table revision: Minimum = %d, Maximum = %d\n",
      AcpiTableInfo->AcpiTableRevision,
      This->MinAcpiTableRevision,
      This->AcpiTableRevision
      ));
    return EFI_INVALID_PARAMETER;
  }

  *Table = NULL;
  Status = GetEArmObjPlatformGTBlockInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &GTBlockInfo,
             &BlockTimerCount
             );
  if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: GTDT: Failed to Get Platform GT Block Information." \
      " Status = %r\n",
      Status
      ));
    goto error_handler;
  }

  Status = GetEArmObjPlatformGenericWatchdogInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &WatchdogInfoList,
             &WatchdogCount
             );
  if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: GTDT: Failed to Get Platform Generic Watchdog Information." \
      " Status = %r\n",
      Status
      ));
    goto error_handler;
  }

  DEBUG ((
    DEBUG_INFO,
    "GTDT: BlockTimerCount = %d, WatchdogCount = %d\n",
    BlockTimerCount,
    WatchdogCount
    ));

  // Calculate the GTDT Table Size
  PlatformTimerCount = 0;
  TableSize = sizeof (EFI_ACPI_6_2_GENERIC_TIMER_DESCRIPTION_TABLE);
  if (BlockTimerCount != 0) {
    GTBlockOffset = TableSize;
    PlatformTimerCount += BlockTimerCount;
    TableSize += (sizeof (EFI_ACPI_6_2_GTDT_GT_BLOCK_STRUCTURE) *
                  BlockTimerCount);

    for (Idx = 0; Idx < BlockTimerCount; Idx++) {
      if (GTBlockInfo[Idx].GTBlockTimerFrameCount > 8) {
        Status = EFI_INVALID_PARAMETER;
        DEBUG ((
          DEBUG_ERROR,
          "GTDT: GTBockFrameCount cannot be more than 8." \
          " GTBockFrameCount = %d, Status = %r\n",
          GTBlockInfo[Idx].GTBlockTimerFrameCount,
          Status
          ));
        goto error_handler;
      }
      TableSize += (sizeof (EFI_ACPI_6_2_GTDT_GT_BLOCK_TIMER_STRUCTURE) *
        GTBlockInfo[Idx].GTBlockTimerFrameCount);
    }

    DEBUG ((
      DEBUG_INFO,
      "GTDT: GTBockOffset = 0x%x, PLATFORM_TIMER_COUNT = %d\n",
      GTBlockOffset,
      PlatformTimerCount
      ));
  }

  WatchdogOffset = 0;
  if (WatchdogCount != 0) {
    WatchdogOffset = TableSize;
    PlatformTimerCount += WatchdogCount;
    TableSize += (sizeof (EFI_ACPI_6_2_GTDT_SBSA_GENERIC_WATCHDOG_STRUCTURE) *
                  WatchdogCount);
    DEBUG ((
      DEBUG_INFO,
      "GTDT: WatchdogOffset = 0x%x, PLATFORM_TIMER_COUNT = %d\n",
      WatchdogOffset,
      PlatformTimerCount
      ));
  }

  *Table = (EFI_ACPI_DESCRIPTION_HEADER*)AllocateZeroPool (TableSize);
  if (*Table == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: GTDT: Failed to allocate memory for GTDT Table, Size = %d," \
      " Status = %r\n",
      TableSize,
      Status
      ));
    goto error_handler;
  }

  Gtdt = (EFI_ACPI_6_2_GENERIC_TIMER_DESCRIPTION_TABLE*)*Table;
  DEBUG ((
    DEBUG_INFO,
    "GTDT: Gtdt = 0x%p TableSize = 0x%x\n",
    Gtdt,
    TableSize
    ));

  Status = AddAcpiHeader (
             CfgMgrProtocol,
             This,
             &Gtdt->Header,
             AcpiTableInfo,
             TableSize
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: GTDT: Failed to add ACPI header. Status = %r\n",
      Status
      ));
    goto error_handler;
  }

  Status = AddGenericTimerInfo (
             CfgMgrProtocol,
             Gtdt,
             PlatformTimerCount
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: GTDT: Failed to add Generic Timer Info. Status = %r\n",
      Status
      ));
    goto error_handler;
  }

  if (BlockTimerCount != 0) {
    Status = AddGTBlockList (
               CfgMgrProtocol,
               Gtdt,
               GTBlockOffset,
               GTBlockInfo,
               BlockTimerCount
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: GTDT: Failed to add GT Block timers. Status = %r\n",
        Status
        ));
      goto error_handler;
    }
  }

  if (WatchdogCount != 0) {
    AddGenericWatchdogList (
      Gtdt,
      WatchdogOffset,
      WatchdogInfoList,
      WatchdogCount
      );
  }

  return Status;

error_handler:
  if (*Table != NULL) {
    FreePool (*Table);
    *Table = NULL;
  }
  return Status;
}

/** Free any resources allocated for constructing the GTDT.

  @param [in]      This           Pointer to the table generator.
  @param [in]      AcpiTableInfo  Pointer to the ACPI Table Info.
  @param [in]      CfgMgrProtocol Pointer to the Configuration Manager
                                  Protocol Interface.
  @param [in, out] Table          Pointer to the ACPI Table.

  @retval EFI_SUCCESS           The resources were freed successfully.
  @retval EFI_INVALID_PARAMETER The table pointer is NULL or invalid.
**/
STATIC
EFI_STATUS
FreeGtdtTableResources (
  IN      CONST ACPI_TABLE_GENERATOR                  * CONST This,
  IN      CONST CM_STD_OBJ_ACPI_TABLE_INFO            * CONST AcpiTableInfo,
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  * CONST CfgMgrProtocol,
  IN OUT        EFI_ACPI_DESCRIPTION_HEADER          ** CONST Table
)
{
  ASSERT (This != NULL);
  ASSERT (AcpiTableInfo != NULL);
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (AcpiTableInfo->TableGeneratorId == This->GeneratorID);
  ASSERT (AcpiTableInfo->AcpiTableSignature == This->AcpiTableSignature);

  if ((Table == NULL) || (*Table == NULL)) {
    DEBUG ((DEBUG_ERROR, "ERROR: GTDT: Invalid Table Pointer\n"));
    ASSERT ((Table != NULL) && (*Table != NULL));
    return EFI_INVALID_PARAMETER;
  }

  FreePool (*Table);
  *Table = NULL;
  return EFI_SUCCESS;
}

/** This macro defines the GTDT Table Generator revision.
*/
#define GTDT_GENERATOR_REVISION CREATE_REVISION (1, 0)

/** The interface for the GTDT Table Generator.
*/
STATIC
CONST
ACPI_TABLE_GENERATOR GtdtGenerator = {
  // Generator ID
  CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdGtdt),
  // Generator Description
  L"ACPI.STD.GTDT.GENERATOR",
  // ACPI Table Signature
  EFI_ACPI_6_2_GENERIC_TIMER_DESCRIPTION_TABLE_SIGNATURE,
  // ACPI Table Revision supported by this Generator
  EFI_ACPI_6_2_GENERIC_TIMER_DESCRIPTION_TABLE_REVISION,
  // Minimum ACPI Table Revision supported by this Generator
  EFI_ACPI_6_2_GENERIC_TIMER_DESCRIPTION_TABLE_REVISION,
  // Creator ID
  TABLE_GENERATOR_CREATOR_ID_ARM,
  // Creator Revision
  GTDT_GENERATOR_REVISION,
  // Build Table function
  BuildGtdtTable,
  // Free Resource function
  FreeGtdtTableResources,
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
AcpiGtdtLibConstructor (
  IN CONST EFI_HANDLE                ImageHandle,
  IN       EFI_SYSTEM_TABLE  * CONST SystemTable
  )
{
  EFI_STATUS  Status;
  Status = RegisterAcpiTableGenerator (&GtdtGenerator);
  DEBUG ((DEBUG_INFO, "GTDT: Register Generator. Status = %r\n", Status));
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
AcpiGtdtLibDestructor (
  IN CONST EFI_HANDLE                ImageHandle,
  IN       EFI_SYSTEM_TABLE  * CONST SystemTable
  )
{
  EFI_STATUS  Status;
  Status = DeregisterAcpiTableGenerator (&GtdtGenerator);
  DEBUG ((DEBUG_INFO, "GTDT: Deregister Generator. Status = %r\n", Status));
  ASSERT_EFI_ERROR (Status);
  return Status;
}
