/** @file

  Generate ACPI FACS table for AMD platforms.

  Copyright (C) 2025 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier BSD-2-Clause-Patent
**/

#include <AcpiTableGenerator.h>
#include <ConfigurationManagerHelper.h>
#include <ConfigurationManagerObject.h>
#include <Library/DebugLib.h>
#include <Library/TableHelperLib.h>
#include <Protocol/AcpiTable.h>
#include <Protocol/ConfigurationManagerProtocol.h>
#include <X64NameSpaceObjects.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>

/** This macro expands to a function that retrieves the
    FACS information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceX64,
  EX64ObjFacsInfo,
  CM_X64_FACS_INFO
  );

/** The ACPI FACS Table.
*/
STATIC
EFI_ACPI_6_5_FIRMWARE_ACPI_CONTROL_STRUCTURE  AcpiFacs = {
  EFI_ACPI_6_5_FIRMWARE_ACPI_CONTROL_STRUCTURE_SIGNATURE,
  // Length
  sizeof (EFI_ACPI_6_5_FIRMWARE_ACPI_CONTROL_STRUCTURE),
  // Hardware Signature
  0,
  // Firmware Waking Vector
  0,
  // Global Lock
  0,
  // Flags
  0,
  // XFirmware Waking Vector
  0,
  // Version
  0,
  // Reserved0
  { 0,                                                   0,  0 },
  // OSPM Flags
  0,
  // Reserved1
  {
    0,                                                   0,  0, 0, 0, 0, 0, 0,
    0,                                                   0,  0, 0, 0, 0, 0, 0,
    0,                                                   0,  0, 0, 0, 0, 0, 0
  }
};

/**
  Update the hardware signature in the FACS table.
**/
VOID
AcpiFacsUpdateHardwareSignature (
  VOID
  )
{
  EFI_ACPI_6_5_FIRMWARE_ACPI_CONTROL_STRUCTURE  *Facs;
  EFI_ACPI_6_5_FIXED_ACPI_DESCRIPTION_TABLE     *Fadt;
  EFI_ACPI_6_5_ROOT_SYSTEM_DESCRIPTION_POINTER  *Rsdp;
  EFI_ACPI_DESCRIPTION_HEADER                   *Table;
  EFI_ACPI_DESCRIPTION_HEADER                   *Xsdt;
  EFI_ACPI_DESCRIPTION_HEADER                   *Dsdt;
  EFI_STATUS                                    Status;
  UINT32                                        CollectedCrc[2];
  UINT32                                        ComputedCrc;
  UINT32                                        TableCount;
  UINT64                                        XsdtTablePtr;
  UINTN                                         Index;
  UINTN                                         XsdtPtr;

  DEBUG ((DEBUG_INFO, "Updating hardware signature in FACS Table.\n"));

  Facs   = NULL;
  Fadt   = NULL;
  Dsdt   = NULL;
  Status = EfiGetSystemConfigurationTable (&gEfiAcpiTableGuid, (VOID **)&Rsdp);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to get RSDP. Status(%r)\n", Status));
    ASSERT_EFI_ERROR (Status);
    return;
  }

  if (  (Rsdp->Revision >= EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER_REVISION)
     && (Rsdp->XsdtAddress != 0))
  {
    CollectedCrc[0] = Rsdp->ExtendedChecksum;
    Xsdt            = (EFI_ACPI_DESCRIPTION_HEADER *)(UINTN)Rsdp->XsdtAddress;
    CollectedCrc[1] = Xsdt->Checksum;
    gBS->CalculateCrc32 (
           (UINT8 *)CollectedCrc,
           ARRAY_SIZE (CollectedCrc),
           &ComputedCrc
           );
    CollectedCrc[0] = ComputedCrc;

    TableCount = (Xsdt->Length - sizeof (EFI_ACPI_DESCRIPTION_HEADER)) / sizeof (UINT64);
    XsdtPtr    = (UINTN)(Xsdt + 1);
    for (Index = 0; Index < TableCount; Index++) {
      CopyMem (
        &XsdtTablePtr,
        (VOID *)(XsdtPtr + Index * sizeof (UINT64)),
        sizeof (UINT64)
        );
      Table = (EFI_ACPI_DESCRIPTION_HEADER *)((UINTN)(XsdtTablePtr));
      DEBUG ((
        DEBUG_INFO,
        "FACS: Collecting CRC of %c%c%c%c\n",
        Table->Signature & 0xFF,
        (Table->Signature >> 8) & 0xFF,
        (Table->Signature >> 16) & 0xFF,
        (Table->Signature >> 24) & 0xFF
        ));
      CollectedCrc[1] = Table->Checksum;
      gBS->CalculateCrc32 (
             (UINT8 *)CollectedCrc,
             ARRAY_SIZE (CollectedCrc),
             &ComputedCrc
             );
      CollectedCrc[0] = ComputedCrc;
      /// check if Table is FADT
      if (Table->Signature == EFI_ACPI_6_5_FIXED_ACPI_DESCRIPTION_TABLE_SIGNATURE) {
        Dsdt = NULL;
        Fadt = (EFI_ACPI_6_5_FIXED_ACPI_DESCRIPTION_TABLE *)Table;
        if (Fadt->XDsdt != 0) {
          Dsdt = (EFI_ACPI_DESCRIPTION_HEADER *)(UINTN)(Fadt->XDsdt);
        } else {
          Dsdt = (EFI_ACPI_DESCRIPTION_HEADER *)(UINTN)(Fadt->Dsdt);
        }

        if (Dsdt != NULL) {
          DEBUG ((
            DEBUG_INFO,
            "FACS: Collecting CRC of %c%c%c%c\n",
            Dsdt->Signature & 0xFF,
            (Dsdt->Signature >> 8) & 0xFF,
            (Dsdt->Signature >> 16) & 0xFF,
            (Dsdt->Signature >> 24) & 0xFF
            ));
          CollectedCrc[1] = Dsdt->Checksum;
          gBS->CalculateCrc32 (
                 (UINT8 *)CollectedCrc,
                 ARRAY_SIZE (CollectedCrc),
                 &ComputedCrc
                 );
          CollectedCrc[0] = ComputedCrc;
        }

        Facs = NULL;
        if (Fadt->XFirmwareCtrl != 0) {
          Facs = (EFI_ACPI_6_5_FIRMWARE_ACPI_CONTROL_STRUCTURE *)(UINTN)(Fadt->XFirmwareCtrl);
        } else {
          Facs = (EFI_ACPI_6_5_FIRMWARE_ACPI_CONTROL_STRUCTURE *)(UINTN)(Fadt->FirmwareCtrl);
        }
      }
    }
  } else {
    DEBUG ((DEBUG_ERROR, "ERROR: FACS Generator do not support RSDT table.\n"));
    return;
  }

  if (Facs != NULL) {
    /// Update FACS signature
    Facs->HardwareSignature = ComputedCrc;
  } else {
    DEBUG ((DEBUG_ERROR, "ERROR: FACS Table not found.\n"));
  }

  return;
}

/**
  Event notification function for AcpiFacsLib.

  @param[in] Event    Event whose notification function is being invoked.
  @param[in] Context  Pointer to the notification function's context, which is
                      implementation-dependent.
**/
STATIC
VOID
EFIAPI
AcpiFacsLibEvent (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  /// Close the Event
  gBS->CloseEvent (Event);

  /// Update the FACS Table
  AcpiFacsUpdateHardwareSignature ();
}

/** Update FACS table information.

  @param [in]  CfgMgrProtocol Pointer to the Configuration Manager
                              Protocol Interface.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         The required object was not found or
                                the FACS is not enabled.
  @retval EFI_BAD_BUFFER_SIZE   The size returned by the Configuration
                                Manager is less than the Object size for the
                                requested object.
  @retval EFI_UNSUPPORTED       If invalid protection and oem flags provided.
**/
STATIC
EFI_STATUS
EFIAPI
FacsUpdateTableInfo (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol
  )
{
  CM_X64_FACS_INFO  *FacsInfo;
  EFI_STATUS        Status;

  ASSERT (CfgMgrProtocol != NULL);

  // Get the FACS information from the Platform Configuration Manager
  Status = GetEX64ObjFacsInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &FacsInfo,
             NULL
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: FACS: Failed to get FACS information." \
      " Status = %r\n",
      Status
      ));
    return Status;
  }

  if ((FacsInfo->Flags & ~(EFI_ACPI_6_5_S4BIOS_F|EFI_ACPI_6_5_64BIT_WAKE_SUPPORTED_F)) != 0) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: FACS: Invalid Flags. Flags = 0x%x\n",
      FacsInfo->Flags
      ));
    return EFI_UNSUPPORTED;
  }

  if ((FacsInfo->OspmFlags & ~(EFI_ACPI_6_5_OSPM_64BIT_WAKE_F)) != 0) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: FACS: Invalid OSPM Flags. Flags = 0x%x\n",
      FacsInfo->OspmFlags
      ));
    return EFI_UNSUPPORTED;
  }

  if ((FacsInfo->OspmFlags & EFI_ACPI_6_5_OSPM_64BIT_WAKE_F) &&
      !(FacsInfo->Flags & EFI_ACPI_6_5_64BIT_WAKE_SUPPORTED_F))
  {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: FACS: Invalid Flags. Flags = 0x%x, OSPM Flags = 0x%x.\n"
      "       64-bit wake is not supported but 64-bit wake flag is set.\n",
      FacsInfo->Flags,
      FacsInfo->OspmFlags
      ));
    return EFI_UNSUPPORTED;
  }

  // Update the FACS table
  AcpiFacs.HardwareSignature     = 0;
  AcpiFacs.FirmwareWakingVector  = FacsInfo->FirmwareWakingVector;
  AcpiFacs.Flags                 = FacsInfo->Flags;
  AcpiFacs.XFirmwareWakingVector = FacsInfo->XFirmwareWakingVector;
  AcpiFacs.Version               = EFI_ACPI_6_5_FIRMWARE_ACPI_CONTROL_STRUCTURE_VERSION;
  AcpiFacs.OspmFlags             = FacsInfo->OspmFlags;

  return Status;
}

/** Construct the FACS table.

  This function invokes the Configuration Manager protocol interface
  to get the required information for generating the ACPI table.

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
BuildFacsTable (
  IN  CONST ACPI_TABLE_GENERATOR                  *CONST  This,
  IN  CONST CM_STD_OBJ_ACPI_TABLE_INFO            *CONST  AcpiTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  OUT       EFI_ACPI_DESCRIPTION_HEADER          **CONST  Table
  )
{
  EFI_STATUS  Status;
  EFI_EVENT   Event;

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
      "ERROR: FACS: Requested table revision = %d, is not supported."
      "Supported table revision: Minimum = %d, Maximum = %d\n",
      AcpiTableInfo->AcpiTableRevision,
      This->MinAcpiTableRevision,
      This->AcpiTableRevision
      ));
    return EFI_INVALID_PARAMETER;
  }

  *Table = NULL;

  // Update FACS table info
  Status = FacsUpdateTableInfo (CfgMgrProtocol);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: FACS: Failed to update table info. Status = %r\n",
      Status
      ));
    return Status;
  }

  //
  // Register notify function
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  AcpiFacsLibEvent,
                  NULL,
                  &gEfiEventReadyToBootGuid,
                  &Event
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: FACS: Failed to create event. Status = %r\n",
      Status
      ));
    return Status;
  }

  *Table = (EFI_ACPI_DESCRIPTION_HEADER *)&AcpiFacs;
  return Status;
}

/** This macro defines the FACS Table Generator revision.
*/
#define FACS_GENERATOR_REVISION  CREATE_REVISION (1, 0)

/** The interface for the FACS Table Generator.
*/
STATIC
CONST
ACPI_TABLE_GENERATOR  FacsGenerator = {
  // Generator ID
  CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdFacs),
  // Generator Description
  L"ACPI.STD.FACS.GENERATOR",
  // ACPI Table Signature
  EFI_ACPI_6_5_FIRMWARE_ACPI_CONTROL_STRUCTURE_SIGNATURE,
  // ACPI Table Revision supported by this Generator
  EFI_ACPI_6_5_FIRMWARE_ACPI_CONTROL_STRUCTURE_VERSION,
  // Minimum supported ACPI Table Revision
  EFI_ACPI_6_5_FIRMWARE_ACPI_CONTROL_STRUCTURE_VERSION,
  // Creator ID
  TABLE_GENERATOR_CREATOR_ID,
  // Creator Revision
  FACS_GENERATOR_REVISION,
  // Build Table function
  BuildFacsTable,
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
AcpiFacsLibConstructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = RegisterAcpiTableGenerator (&FacsGenerator);
  DEBUG ((DEBUG_INFO, "FACS: Register Generator. Status = %r\n", Status));
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
AcpiFacsLibDestructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = DeregisterAcpiTableGenerator (&FacsGenerator);
  DEBUG ((DEBUG_INFO, "FACS: Deregister Generator. Status = %r\n", Status));
  ASSERT_EFI_ERROR (Status);
  return Status;
}
