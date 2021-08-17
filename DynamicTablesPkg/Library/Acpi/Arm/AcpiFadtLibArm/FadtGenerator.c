/** @file
  FADT Table Generator

  Copyright (c) 2017 - 2021, ARM Limited. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - ACPI 6.4 Specification, January 2021

**/

#include <Library/AcpiLib.h>
#include <Library/DebugLib.h>
#include <Protocol/AcpiTable.h>

// Module specific include files.
#include <AcpiTableGenerator.h>
#include <ConfigurationManagerObject.h>
#include <ConfigurationManagerHelper.h>
#include <Library/TableHelperLib.h>
#include <Protocol/ConfigurationManagerProtocol.h>

/** ARM standard FADT Generator

Requirements:
  The following Configuration Manager Object(s) are required by
  this Generator:
  - EArmObjPowerManagementProfileInfo
  - EArmObjBootArchInfo
  - EArmObjHypervisorVendorIdentity (OPTIONAL)
*/

/** This macro defines the FADT flag options for ARM Platforms.
*/
#define FADT_FLAGS  (EFI_ACPI_6_4_HW_REDUCED_ACPI |          \
                     EFI_ACPI_6_4_LOW_POWER_S0_IDLE_CAPABLE)

/** This macro defines the valid mask for the FADT flag option
    if HW_REDUCED_ACPI flag in the table is set.

  Invalid bits are: 1, 2, 3,7, 8, 13, 14,16, 17 and
    22-31 (reserved).

  Valid bits are:
    EFI_ACPI_6_4_WBINVD                               BIT0
    EFI_ACPI_6_4_PWR_BUTTON                           BIT4
    EFI_ACPI_6_4_SLP_BUTTON                           BIT5
    EFI_ACPI_6_4_FIX_RTC                              BIT6
    EFI_ACPI_6_4_DCK_CAP                              BIT9
    EFI_ACPI_6_4_RESET_REG_SUP                        BIT10
    EFI_ACPI_6_4_SEALED_CASE                          BIT11
    EFI_ACPI_6_4_HEADLESS                             BIT12
    EFI_ACPI_6_4_USE_PLATFORM_CLOCK                   BIT15
    EFI_ACPI_6_4_FORCE_APIC_CLUSTER_MODEL             BIT18
    EFI_ACPI_6_4_FORCE_APIC_PHYSICAL_DESTINATION_MODE BIT19
    EFI_ACPI_6_4_HW_REDUCED_ACPI                      BIT20
    EFI_ACPI_6_4_LOW_POWER_S0_IDLE_CAPABLE            BIT21
*/
#define VALID_HARDWARE_REDUCED_FLAG_MASK  (                   \
          EFI_ACPI_6_4_WBINVD                               | \
          EFI_ACPI_6_4_PWR_BUTTON                           | \
          EFI_ACPI_6_4_SLP_BUTTON                           | \
          EFI_ACPI_6_4_FIX_RTC                              | \
          EFI_ACPI_6_4_DCK_CAP                              | \
          EFI_ACPI_6_4_RESET_REG_SUP                        | \
          EFI_ACPI_6_4_SEALED_CASE                          | \
          EFI_ACPI_6_4_HEADLESS                             | \
          EFI_ACPI_6_4_USE_PLATFORM_CLOCK                   | \
          EFI_ACPI_6_4_FORCE_APIC_CLUSTER_MODEL             | \
          EFI_ACPI_6_4_FORCE_APIC_PHYSICAL_DESTINATION_MODE | \
          EFI_ACPI_6_4_HW_REDUCED_ACPI                      | \
          EFI_ACPI_6_4_LOW_POWER_S0_IDLE_CAPABLE)

#pragma pack(1)

/** The AcpiFadt is a template EFI_ACPI_6_4_FIXED_ACPI_DESCRIPTION_TABLE
    structure used for generating the FADT Table.
  Note: fields marked with "{Template}" will be updated dynamically.
*/
STATIC
EFI_ACPI_6_4_FIXED_ACPI_DESCRIPTION_TABLE AcpiFadt = {
  ACPI_HEADER (
    EFI_ACPI_6_4_FIXED_ACPI_DESCRIPTION_TABLE_SIGNATURE,
    EFI_ACPI_6_4_FIXED_ACPI_DESCRIPTION_TABLE,
    EFI_ACPI_6_4_FIXED_ACPI_DESCRIPTION_TABLE_REVISION
    ),
  // UINT32     FirmwareCtrl
  0,
  // UINT32     Dsdt
  0,
  // UINT8      Reserved0
  EFI_ACPI_RESERVED_BYTE,
  // UINT8      PreferredPmProfile
  EFI_ACPI_6_4_PM_PROFILE_UNSPECIFIED,  // {Template}: Power Management Profile
  // UINT16     SciInt
  0,
  // UINT32     SmiCmd
  0,
  // UINT8      AcpiEnable
  0,
  // UINT8      AcpiDisable
  0,
  // UINT8      S4BiosReq
  0,
  // UINT8      PstateCnt
  0,
  // UINT32     Pm1aEvtBlk
  0,
  // UINT32     Pm1bEvtBlk
  0,
  // UINT32     Pm1aCntBlk
  0,
  // UINT32     Pm1bCntBlk
  0,
  // UINT32     Pm2CntBlk
  0,
  // UINT32     PmTmrBlk
  0,
  // UINT32     Gpe0Blk
  0,
  // UINT32     Gpe1Blk
  0,
  // UINT8      Pm1EvtLen
  0,
  // UINT8      Pm1CntLen
  0,
  // UINT8      Pm2CntLen
  0,
  // UINT8      PmTmrLen
  0,
  // UINT8      Gpe0BlkLen
  0,
  // UINT8      Gpe1BlkLen
  0,
  // UINT8      Gpe1Base
  0,
  // UINT8      CstCnt
  0,
  // UINT16     PLvl2Lat
  0,
  // UINT16     PLvl3Lat
  0,
  // UINT16     FlushSize
  0,
  // UINT16     FlushStride
  0,
  // UINT8      DutyOffset
  0,
  // UINT8      DutyWidth
  0,
  // UINT8      DayAlrm
  0,
  // UINT8      MonAlrm
  0,
  // UINT8      Century
  0,
  // UINT16     IaPcBootArch
  0,
  // UINT8      Reserved1
  0,
  // UINT32     Flags
  FADT_FLAGS,
  // EFI_ACPI_6_4_GENERIC_ADDRESS_STRUCTURE  ResetReg
  NULL_GAS,
  // UINT8      ResetValue
  0,
  // UINT16     ArmBootArch
  EFI_ACPI_6_4_ARM_PSCI_COMPLIANT,  // {Template}: ARM Boot Architecture Flags
  // UINT8      MinorRevision
  EFI_ACPI_6_4_FIXED_ACPI_DESCRIPTION_TABLE_MINOR_REVISION,
  // UINT64     XFirmwareCtrl
  0,
  // UINT64     XDsdt
  0,
  // EFI_ACPI_6_4_GENERIC_ADDRESS_STRUCTURE  XPm1aEvtBlk
  NULL_GAS,
  // EFI_ACPI_6_4_GENERIC_ADDRESS_STRUCTURE  XPm1bEvtBlk
  NULL_GAS,
  // EFI_ACPI_6_4_GENERIC_ADDRESS_STRUCTURE  XPm1aCntBlk
  NULL_GAS,
  // EFI_ACPI_6_4_GENERIC_ADDRESS_STRUCTURE  XPm1bCntBlk
  NULL_GAS,
  // EFI_ACPI_6_4_GENERIC_ADDRESS_STRUCTURE  XPm2CntBlk
  NULL_GAS,
  // EFI_ACPI_6_4_GENERIC_ADDRESS_STRUCTURE  XPmTmrBlk
  NULL_GAS,
  // EFI_ACPI_6_4_GENERIC_ADDRESS_STRUCTURE  XGpe0Blk
  NULL_GAS,
  // EFI_ACPI_6_4_GENERIC_ADDRESS_STRUCTURE  XGpe1Blk
  NULL_GAS,
  // EFI_ACPI_6_4_GENERIC_ADDRESS_STRUCTURE  SleepControlReg
  NULL_GAS,
  // EFI_ACPI_6_4_GENERIC_ADDRESS_STRUCTURE  SleepStatusReg
  NULL_GAS,
  // UINT64     HypervisorVendorIdentity
  EFI_ACPI_RESERVED_QWORD  // {Template}: Hypervisor Vendor ID
};

#pragma pack()

/** This macro expands to a function that retrieves the Power
    Management Profile Information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArm,
  EArmObjPowerManagementProfileInfo,
  CM_ARM_POWER_MANAGEMENT_PROFILE_INFO
  );

/** This macro expands to a function that retrieves the Boot
    Architecture Information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArm,
  EArmObjBootArchInfo,
  CM_ARM_BOOT_ARCH_INFO
  );

/** This macro expands to a function that retrieves the Hypervisor
    Vendor ID from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArm,
  EArmObjHypervisorVendorIdentity,
  CM_ARM_HYPERVISOR_VENDOR_ID
  );

/** This macro expands to a function that retrieves the Fixed
  feature flags for the platform from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArm,
  EArmObjFixedFeatureFlags,
  CM_ARM_FIXED_FEATURE_FLAGS
  );

/** Update the Power Management Profile information in the FADT Table.

  @param [in]  CfgMgrProtocol Pointer to the Configuration Manager
                              Protocol Interface.

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
FadtAddPmProfileInfo (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  * CONST CfgMgrProtocol
)
{
  EFI_STATUS                              Status;
  CM_ARM_POWER_MANAGEMENT_PROFILE_INFO  * PmProfile;

  ASSERT (CfgMgrProtocol != NULL);

  // Get the Power Management Profile from the Platform Configuration Manager
  Status = GetEArmObjPowerManagementProfileInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &PmProfile,
             NULL
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: FADT: Failed to get Power Management Profile information." \
      " Status = %r\n",
      Status
      ));
    goto error_handler;
  }

  DEBUG ((
    DEBUG_INFO,
    "FADT: PreferredPmProfile = 0x%x\n",
    PmProfile->PowerManagementProfile
    ));

  AcpiFadt.PreferredPmProfile = PmProfile->PowerManagementProfile;

error_handler:
  return Status;
}

/** Updates the Boot Architecture information in the FADT Table.

  @param [in]  CfgMgrProtocol Pointer to the Configuration Manager
                              Protocol Interface.

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
FadtAddBootArchInfo (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  * CONST CfgMgrProtocol
)
{
  EFI_STATUS               Status;
  CM_ARM_BOOT_ARCH_INFO  * BootArchInfo;

  ASSERT (CfgMgrProtocol != NULL);

  // Get the Boot Architecture flags from the Platform Configuration Manager
  Status = GetEArmObjBootArchInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &BootArchInfo,
             NULL
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: FADT: Failed to get Boot Architecture flags. Status = %r\n",
      Status
      ));
    goto error_handler;
  }

  DEBUG ((
    DEBUG_INFO,
    "FADT BootArchFlag = 0x%x\n",
    BootArchInfo->BootArchFlags
    ));

  AcpiFadt.ArmBootArch = BootArchInfo->BootArchFlags;

error_handler:
  return Status;
}

/** Update the Hypervisor Vendor ID in the FADT Table.

  @param [in]  CfgMgrProtocol Pointer to the Configuration Manager
                              Protocol Interface.

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
FadtAddHypervisorVendorId (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  * CONST CfgMgrProtocol
)
{
  EFI_STATUS                     Status;
  CM_ARM_HYPERVISOR_VENDOR_ID  * HypervisorVendorInfo;

  ASSERT (CfgMgrProtocol != NULL);

  // Get the Hypervisor Vendor ID from the Platform Configuration Manager
  Status = GetEArmObjHypervisorVendorIdentity (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &HypervisorVendorInfo,
             NULL
             );
  if (EFI_ERROR (Status)) {
    if (Status == EFI_NOT_FOUND) {
      DEBUG ((
        DEBUG_INFO,
        "INFO: FADT: Platform does not have a Hypervisor Vendor ID."
        "Status = %r\n",
        Status
        ));
    } else {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: FADT: Failed to get Hypervisor Vendor ID. Status = %r\n",
        Status
        ));
    }
    goto error_handler;
  }

  DEBUG ((
    DEBUG_INFO,
    "FADT: EArmObjHypervisorVendorIdentity = 0x%lx\n",
    HypervisorVendorInfo->HypervisorVendorId
    ));

  AcpiFadt.HypervisorVendorIdentity = HypervisorVendorInfo->HypervisorVendorId;

error_handler:
  return Status;
}

/** Update the Fixed Feature Flags in the FADT Table.

  @param [in]  CfgMgrProtocol Pointer to the Configuration Manager
                              Protocol Interface.

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
FadtAddFixedFeatureFlags (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  * CONST CfgMgrProtocol
)
{
  EFI_STATUS                    Status;
  CM_ARM_FIXED_FEATURE_FLAGS  * FixedFeatureFlags;

  ASSERT (CfgMgrProtocol != NULL);

  // Get the Fixed feature flags from the Platform Configuration Manager
  Status = GetEArmObjFixedFeatureFlags (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &FixedFeatureFlags,
             NULL
             );
  if (EFI_ERROR (Status)) {
    if (Status == EFI_NOT_FOUND) {
      DEBUG ((
        DEBUG_INFO,
        "INFO: FADT: Platform does not define additional Fixed feature flags."
        "Status = %r\n",
        Status
        ));
    } else {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: FADT: Failed to get Fixed feature flags. Status = %r\n",
        Status
        ));
    }
    goto error_handler;
  }

  DEBUG ((
    DEBUG_INFO,
    "FADT: EArmObjFixedFeatureFlags = 0x%x\n",
    FixedFeatureFlags->Flags
    ));

  if ((FixedFeatureFlags->Flags & ~(VALID_HARDWARE_REDUCED_FLAG_MASK)) != 0) {
    DEBUG ((
      DEBUG_WARN,
      "FADT: Invalid Fixed feature flags defined by platform,"
      "Invalid Flags bits are = 0x%x\n",
      (FixedFeatureFlags->Flags & ~(VALID_HARDWARE_REDUCED_FLAG_MASK))
      ));
  }

  AcpiFadt.Flags |= (FixedFeatureFlags->Flags &
                     VALID_HARDWARE_REDUCED_FLAG_MASK);

error_handler:
  return Status;
}

/** Construct the FADT table.

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
BuildFadtTable (
  IN  CONST ACPI_TABLE_GENERATOR                  * CONST This,
  IN  CONST CM_STD_OBJ_ACPI_TABLE_INFO            * CONST AcpiTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  * CONST CfgMgrProtocol,
  OUT       EFI_ACPI_DESCRIPTION_HEADER          ** CONST Table
  )
{
  EFI_STATUS  Status;

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
      "ERROR: FADT: Requested table revision = %d, is not supported."
      "Supported table revision: Minimum = %d, Maximum = %d\n",
      AcpiTableInfo->AcpiTableRevision,
      This->MinAcpiTableRevision,
      This->AcpiTableRevision
      ));
    return EFI_INVALID_PARAMETER;
  }

  *Table = NULL;

  Status = AddAcpiHeader (
             CfgMgrProtocol,
             This,
             (EFI_ACPI_DESCRIPTION_HEADER*)&AcpiFadt,
             AcpiTableInfo,
             sizeof (EFI_ACPI_6_4_FIXED_ACPI_DESCRIPTION_TABLE)
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: FADT: Failed to add ACPI header. Status = %r\n",
      Status
      ));
    goto error_handler;
  }

  // Update PmProfile Info
  Status = FadtAddPmProfileInfo (CfgMgrProtocol);
  if (EFI_ERROR (Status)) {
    goto error_handler;
  }

  // Update BootArch Info
  Status = FadtAddBootArchInfo (CfgMgrProtocol);
  if (EFI_ERROR (Status)) {
    goto error_handler;
  }

  // Add the Hypervisor Vendor Id if present
  // Note if no hypervisor is present the zero bytes
  // will be placed in this field.
  Status = FadtAddHypervisorVendorId (CfgMgrProtocol);
  if (EFI_ERROR (Status)) {
    if (Status == EFI_NOT_FOUND) {
      DEBUG ((
        DEBUG_INFO,
        "INFO: FADT: No Hypervisor Vendor ID found," \
        " assuming no Hypervisor is present in the firmware.\n"
        ));
    } else {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: FADT: Error reading Hypervisor Vendor ID, Status = %r",
        Status
        ));
      goto error_handler;
    }
  }

  Status = FadtAddFixedFeatureFlags (CfgMgrProtocol);
  if (EFI_ERROR (Status)) {
    if (Status == EFI_NOT_FOUND) {
      DEBUG ((
        DEBUG_INFO,
        "INFO: FADT: No Fixed feature flags found," \
        " assuming no additional flags are defined for the platform.\n"
        ));
      Status = EFI_SUCCESS;
    } else {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: FADT: Error reading Fixed feature flags, Status = %r",
        Status
        ));
      goto error_handler;
    }
  }

  *Table = (EFI_ACPI_DESCRIPTION_HEADER*)&AcpiFadt;
error_handler:
  return Status;
}

/** This macro defines the FADT Table Generator revision.
*/
#define FADT_GENERATOR_REVISION CREATE_REVISION (1, 0)

/** The interface for the FADT Table Generator.
*/
STATIC
CONST
ACPI_TABLE_GENERATOR FadtGenerator = {
  // Generator ID
  CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdFadt),
  // Generator Description
  L"ACPI.STD.FADT.GENERATOR",
  // ACPI Table Signature
  EFI_ACPI_6_4_FIXED_ACPI_DESCRIPTION_TABLE_SIGNATURE,
  // ACPI Table Revision supported by this Generator
  EFI_ACPI_6_4_FIXED_ACPI_DESCRIPTION_TABLE_REVISION,
  // Minimum supported ACPI Table Revision
  EFI_ACPI_6_2_FIXED_ACPI_DESCRIPTION_TABLE_REVISION,
  // Creator ID
  TABLE_GENERATOR_CREATOR_ID_ARM,
  // Creator Revision
  FADT_GENERATOR_REVISION,
  // Build Table function
  BuildFadtTable,
  // No additional resources are allocated by the generator.
  // Hence the Free Resource function is not required.
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
AcpiFadtLibConstructor (
  IN  EFI_HANDLE           ImageHandle,
  IN  EFI_SYSTEM_TABLE  *  SystemTable
  )
{
  EFI_STATUS  Status;
  Status = RegisterAcpiTableGenerator (&FadtGenerator);
  DEBUG ((DEBUG_INFO, "FADT: Register Generator. Status = %r\n", Status));
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
AcpiFadtLibDestructor (
  IN  EFI_HANDLE           ImageHandle,
  IN  EFI_SYSTEM_TABLE  *  SystemTable
  )
{
  EFI_STATUS  Status;
  Status = DeregisterAcpiTableGenerator (&FadtGenerator);
  DEBUG ((DEBUG_INFO, "FADT: Deregister Generator. Status = %r\n", Status));
  ASSERT_EFI_ERROR (Status);
  return Status;
}
