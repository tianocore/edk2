/** @file
  This driver implements TPM 2.0 definition block in ACPI table and
  populates registered MMI callback functions for Tcg2 physical presence
  to handle the requests for ACPI method. It needs to be used together with
  Tcg2 MM drivers to handle the physical presence requests.

  Note: The use of this driver is not to be conflicted with the TPM2 table
  produced through the DynamicTablesPkg. Platform should only choose one of
  them to use.

Copyright (c) 2015 - 2018, Intel Corporation. All rights reserved.<BR>
Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>

#include <IndustryStandard/Tpm2Acpi.h>

#include <Guid/TpmInstance.h>
#include <Guid/TpmNvsMm.h>

#include <Protocol/AcpiTable.h>
#include <Protocol/Tcg2Protocol.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DxeServicesLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/PrintLib.h>
#include <Library/TpmMeasurementLib.h>
#include <Library/Tpm2DeviceLib.h>
#include <Library/Tpm2CommandLib.h>
#include <Library/UefiLib.h>

//
// Physical Presence Interface Version supported by Platform
//
#define PHYSICAL_PRESENCE_VERSION_TAG   "$PV"
#define PHYSICAL_PRESENCE_VERSION_SIZE  4

//
// PNP _HID for TPM2 device
//
#define TPM_HID_TAG        "NNNN0000"
#define TPM_HID_PNP_SIZE   8
#define TPM_HID_ACPI_SIZE  9

//
// Minimum PRS resource template size
//  1 byte    for  BufferOp
//  1 byte    for  PkgLength
//  2 bytes   for  BufferSize
//  12 bytes  for  Memory32Fixed descriptor
//  5 bytes   for  Interrupt descriptor
//  2 bytes   for  END Tag
//
#define TPM_POS_RES_TEMPLATE_MIN_SIZE  (1 + 1 + 2 + 12 + 5 + 2)

//
// Max Interrupt buffer size for PRS interrupt resource
// Now support 15 interrupts in maxmum
//
#define MAX_PRS_INT_BUF_SIZE  (15*4)

/**
  Patch version string of Physical Presence interface supported by platform. The initial string tag in TPM
ACPI table is "$PV".

  @param[in, out] Table          The TPM item in ACPI table.
  @param[in]      PPVer          Version string of Physical Presence interface supported by platform.

  @return                        The allocated address for the found region.

**/
EFI_STATUS
UpdatePPVersion (
  EFI_ACPI_DESCRIPTION_HEADER  *Table,
  CHAR8                        *PPVer
  )
{
  EFI_STATUS  Status;
  UINT8       *DataPtr;

  //
  // Patch some pointers for the ASL code before loading the SSDT.
  //
  for (DataPtr  = (UINT8 *)(Table + 1);
       DataPtr <= (UINT8 *)((UINT8 *)Table + Table->Length - PHYSICAL_PRESENCE_VERSION_SIZE);
       DataPtr += 1)
  {
    if (AsciiStrCmp ((CHAR8 *)DataPtr, PHYSICAL_PRESENCE_VERSION_TAG) == 0) {
      Status = AsciiStrCpyS ((CHAR8 *)DataPtr, PHYSICAL_PRESENCE_VERSION_SIZE, PPVer);
      DEBUG ((DEBUG_INFO, "TPM2 Physical Presence Interface Version update status 0x%x\n", Status));
      return Status;
    }
  }

  return EFI_NOT_FOUND;
}

/**
  Patch TPM2 device HID string.  The initial string tag in TPM2 ACPI table is "NNN0000".

  @param[in, out] Table          The TPM2 SSDT ACPI table.

  @return                               HID Update status.

**/
EFI_STATUS
UpdateHID (
  EFI_ACPI_DESCRIPTION_HEADER  *Table
  )
{
  EFI_STATUS  Status;
  UINT8       *DataPtr;
  CHAR8       Hid[TPM_HID_ACPI_SIZE];
  UINT32      ManufacturerID;
  UINT32      FirmwareVersion1;
  UINT32      FirmwareVersion2;
  BOOLEAN     PnpHID;

  PnpHID = TRUE;

  //
  // Initialize HID with Default PNP string
  //
  ZeroMem (Hid, TPM_HID_ACPI_SIZE);

  //
  // Get Manufacturer ID
  //
  Status = Tpm2GetCapabilityManufactureID (&ManufacturerID);
  if (!EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "TPM_PT_MANUFACTURER 0x%08x\n", ManufacturerID));
    //
    // ManufacturerID defined in TCG Vendor ID Registry
    // may tailed with 0x00 or 0x20
    //
    if (((ManufacturerID >> 24) == 0x00) || ((ManufacturerID >> 24) == 0x20)) {
      //
      //  HID containing PNP ID "NNN####"
      //   NNN is uppercase letter for Vendor ID specified by manufacturer
      //
      CopyMem (Hid, &ManufacturerID, 3);
    } else {
      //
      //  HID containing ACP ID "NNNN####"
      //   NNNN is uppercase letter for Vendor ID specified by manufacturer
      //
      CopyMem (Hid, &ManufacturerID, 4);
      PnpHID = FALSE;
    }
  } else {
    DEBUG ((DEBUG_ERROR, "Get TPM_PT_MANUFACTURER failed %x!\n", Status));
    ASSERT (FALSE);
    return Status;
  }

  Status = Tpm2GetCapabilityFirmwareVersion (&FirmwareVersion1, &FirmwareVersion2);
  if (!EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "TPM_PT_FIRMWARE_VERSION_1 0x%x\n", FirmwareVersion1));
    DEBUG ((DEBUG_INFO, "TPM_PT_FIRMWARE_VERSION_2 0x%x\n", FirmwareVersion2));
    //
    //   #### is Firmware Version 1
    //
    if (PnpHID) {
      AsciiSPrint (Hid + 3, TPM_HID_PNP_SIZE - 3, "%02d%02d", ((FirmwareVersion1 & 0xFFFF0000) >> 16), (FirmwareVersion1 & 0x0000FFFF));
    } else {
      AsciiSPrint (Hid + 4, TPM_HID_ACPI_SIZE - 4, "%02d%02d", ((FirmwareVersion1 & 0xFFFF0000) >> 16), (FirmwareVersion1 & 0x0000FFFF));
    }
  } else {
    DEBUG ((DEBUG_ERROR, "Get TPM_PT_FIRMWARE_VERSION_X failed %x!\n", Status));
    ASSERT (FALSE);
    return Status;
  }

  //
  // Patch HID in ASL code before loading the SSDT.
  //
  for (DataPtr  = (UINT8 *)(Table + 1);
       DataPtr <= (UINT8 *)((UINT8 *)Table + Table->Length - TPM_HID_PNP_SIZE);
       DataPtr += 1)
  {
    if (AsciiStrCmp ((CHAR8 *)DataPtr, TPM_HID_TAG) == 0) {
      if (PnpHID) {
        CopyMem (DataPtr, Hid, TPM_HID_PNP_SIZE);
        //
        // if HID is PNP ID, patch the last byte in HID TAG to Noop
        //
        *(DataPtr + TPM_HID_PNP_SIZE) = AML_NOOP_OP;
      } else {
        CopyMem (DataPtr, Hid, TPM_HID_ACPI_SIZE);
      }

      DEBUG ((DEBUG_INFO, "TPM2 ACPI _HID is patched to %a\n", Hid));

      return Status;
    }
  }

  DEBUG ((DEBUG_ERROR, "TPM2 ACPI HID TAG for patch not found!\n"));
  return EFI_NOT_FOUND;
}

/**
  Initialize and publish TPM items in ACPI table.

  @retval   EFI_SUCCESS     The TCG ACPI table is published successfully.
  @retval   Others          The TCG ACPI table is not published.

**/
EFI_STATUS
PublishAcpiTable (
  VOID
  )
{
  EFI_STATUS                   Status;
  EFI_ACPI_TABLE_PROTOCOL      *AcpiTable;
  UINTN                        TableKey;
  EFI_ACPI_DESCRIPTION_HEADER  *Table;
  UINTN                        TableSize;

  Status = GetSectionFromFv (
             &gEfiCallerIdGuid,
             EFI_SECTION_RAW,
             0,
             (VOID **)&Table,
             &TableSize
             );
  ASSERT_EFI_ERROR (Status);

  //
  // Measure to PCR[0] with event EV_POST_CODE ACPI DATA.
  // The measurement has to be done before any update.
  // Otherwise, the PCR record would be different after TPM FW update
  // or the PCD configuration change.
  //
  TpmMeasureAndLogData (
    0,
    EV_POST_CODE,
    EV_POSTCODE_INFO_ACPI_DATA,
    ACPI_DATA_LEN,
    Table,
    TableSize
    );

  //
  // Update Table version before measuring it to PCR
  //
  Status = UpdatePPVersion (Table, (CHAR8 *)PcdGetPtr (PcdTcgPhysicalPresenceInterfaceVer));
  ASSERT_EFI_ERROR (Status);

  DEBUG ((
    DEBUG_INFO,
    "Current physical presence interface version - %a\n",
    (CHAR8 *)PcdGetPtr (PcdTcgPhysicalPresenceInterfaceVer)
    ));

  //
  // Update TPM2 HID after measuring it to PCR
  //
  Status = UpdateHID (Table);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  ASSERT (Table->OemTableId == SIGNATURE_64 ('T', 'p', 'm', '2', 'T', 'a', 'b', 'l'));
  CopyMem (Table->OemId, PcdGetPtr (PcdAcpiDefaultOemId), sizeof (Table->OemId));

  //
  // Publish the TPM ACPI table. Table is re-checksummed.
  //
  Status = gBS->LocateProtocol (&gEfiAcpiTableProtocolGuid, NULL, (VOID **)&AcpiTable);
  ASSERT_EFI_ERROR (Status);

  TableKey = 0;
  Status   = AcpiTable->InstallAcpiTable (
                          AcpiTable,
                          Table,
                          TableSize,
                          &TableKey
                          );
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/**
  Publish TPM2 ACPI table

  @retval   EFI_SUCCESS     The TPM2 ACPI table is published successfully.
  @retval   Others          The TPM2 ACPI table is not published.

**/
EFI_STATUS
PublishTpm2 (
  VOID
  )
{
  EFI_STATUS                    Status;
  EFI_ACPI_TABLE_PROTOCOL       *AcpiTable;
  UINTN                         TableKey;
  UINT64                        OemTableId;
  EFI_TPM2_ACPI_CONTROL_AREA    *ControlArea;
  TPM2_PTP_INTERFACE_TYPE       InterfaceType;
  EFI_TPM2_ACPI_TABLE_V5        *Tpm2AcpiTableV5;
  EFI_TPM2_ACPI_TABLE_TEMPLATE  Tpm2AcpiTemplate;

  STATIC_ASSERT ((FixedPcdGet64 (PcdTpmMaxAddress) - FixedPcdGet64 (PcdTpmBaseAddress)) == (FixedPcdGet32 (PcdTpmCrbRegionSize) - 1), "TPM CRB region size mismatch");

  // Zero the template so its measured contents are deterministic.
  ZeroMem (&Tpm2AcpiTemplate, sizeof (Tpm2AcpiTemplate));

  //
  // Measure to PCR[0] with event EV_POST_CODE ACPI DATA.
  // The measurement has to be done before any update.
  // Otherwise, the PCR record would be different after event log update
  // or the PCD configuration change.
  //
  TpmMeasureAndLogData (
    0,
    EV_POST_CODE,
    EV_POSTCODE_INFO_ACPI_DATA,
    ACPI_DATA_LEN,
    &Tpm2AcpiTemplate,
    sizeof (EFI_TPM2_ACPI_TABLE_TEMPLATE)
    );

  Tpm2AcpiTemplate.Header.Signature = EFI_ACPI_5_0_TRUSTED_COMPUTING_PLATFORM_2_TABLE_SIGNATURE;
  Tpm2AcpiTemplate.Header.Length    = sizeof (EFI_TPM2_ACPI_TABLE_V5);
  Tpm2AcpiTemplate.Header.Revision  = PcdGet8 (PcdTpm2AcpiTableRev);
  DEBUG ((DEBUG_INFO, "Tpm2 ACPI table revision is %d\n", Tpm2AcpiTemplate.Header.Revision));

  // FF-A is only supported in revisions 5 and up.
  if (Tpm2AcpiTemplate.Header.Revision != EFI_TPM2_ACPI_TABLE_REVISION_5) {
    DEBUG ((DEBUG_ERROR, "%a The minimum revision supported for TPM over FF-A table is 5 not %d\n", __func__, Tpm2AcpiTemplate.Header.Revision));
    ASSERT (FALSE);
    return EFI_UNSUPPORTED;
  }

  // CRB over FF-A only supports the CRB interface type.
  InterfaceType = PcdGet8 (PcdActiveTpmInterfaceType);
  DEBUG ((DEBUG_INFO, "Tpm Active Interface Type %d\n", InterfaceType));
  if (InterfaceType != Tpm2PtpInterfaceCrb) {
    DEBUG ((DEBUG_ERROR, "TPM over FF-A only supports CRB interface\n"));
    return EFI_UNSUPPORTED;
  }

  Tpm2AcpiTemplate.Flags = (Tpm2AcpiTemplate.Flags & ~EFI_TPM2_ACPI_TABLE_FLAGS_PLATFORM_CLASS_MASK) | PcdGet8 (PcdTpmPlatformClass);
  DEBUG ((DEBUG_INFO, "Tpm2 ACPI table PlatformClass is %d\n", (Tpm2AcpiTemplate.Flags & EFI_TPM2_ACPI_TABLE_FLAGS_PLATFORM_CLASS_MASK)));

  Tpm2AcpiTableV5       = (EFI_TPM2_ACPI_TABLE_V5 *)&Tpm2AcpiTemplate;
  Tpm2AcpiTableV5->Laml = PcdGet32 (PcdTpm2AcpiTableLaml);
  Tpm2AcpiTableV5->Lasa = PcdGet64 (PcdTpm2AcpiTableLasa);
  if ((Tpm2AcpiTableV5->Laml == 0) || (Tpm2AcpiTableV5->Lasa == 0)) {
    // Remove LAML/LASA from the length if either is 0.
    Tpm2AcpiTemplate.Header.Length = OFFSET_OF (EFI_TPM2_ACPI_TABLE_V5, Laml);
  }

  DEBUG ((DEBUG_INFO, "Tpm2 ACPI table size %d\n", Tpm2AcpiTemplate.Header.Length));

  Tpm2AcpiTemplate.StartMethod          = EFI_TPM2_ACPI_TABLE_START_METHOD_COMMAND_RESPONSE_BUFFER_INTERFACE_WITH_FFA;
  Tpm2AcpiTemplate.AddressOfControlArea = PcdGet64 (PcdTpmBaseAddress) + 0x40;
  ControlArea                           = (EFI_TPM2_ACPI_CONTROL_AREA *)(UINTN)Tpm2AcpiTemplate.AddressOfControlArea;
  ControlArea->CommandSize              = 0xF80;
  ControlArea->ResponseSize             = 0xF80;
  ControlArea->Command                  = PcdGet64 (PcdTpmBaseAddress) + 0x80;
  ControlArea->Response                 = PcdGet64 (PcdTpmBaseAddress) + 0x80;

  // Set the FF-A specific parameters.
  Tpm2AcpiTableV5->StartMethodSpecificParameters.FfaParameters.Flags      = 0x00;  // Notifications Not Supported
  Tpm2AcpiTableV5->StartMethodSpecificParameters.FfaParameters.Attributes = (EFI_TPM2_ACPI_TABLE_ARM_FFA_PARAMETER_ATTR_CRB_REGION_SIZE_4KB << EFI_TPM2_ACPI_TABLE_ARM_FFA_PARAMETER_ATTR_CRB_REGION_SIZE_SHIFT) |
                                                                            (EFI_TPM2_ACPI_TABLE_ARM_FFA_PARAMETER_ATTR_MEM_TYPE_NOT_CACHABLE << EFI_TPM2_ACPI_TABLE_ARM_FFA_PARAMETER_ATTR_MEM_TYPE_SHIFT);
  Tpm2AcpiTableV5->StartMethodSpecificParameters.FfaParameters.PartitionId = PcdGet16 (PcdTpmServiceFfaPartitionId);
  ASSERT (Tpm2AcpiTableV5->StartMethodSpecificParameters.FfaParameters.PartitionId != 0);

  CopyMem (Tpm2AcpiTemplate.Header.OemId, PcdGetPtr (PcdAcpiDefaultOemId), sizeof (Tpm2AcpiTemplate.Header.OemId));
  OemTableId = PcdGet64 (PcdAcpiDefaultOemTableId);
  CopyMem (&Tpm2AcpiTemplate.Header.OemTableId, &OemTableId, sizeof (UINT64));
  Tpm2AcpiTemplate.Header.OemRevision     = PcdGet32 (PcdAcpiDefaultOemRevision);
  Tpm2AcpiTemplate.Header.CreatorId       = PcdGet32 (PcdAcpiDefaultCreatorId);
  Tpm2AcpiTemplate.Header.CreatorRevision = PcdGet32 (PcdAcpiDefaultCreatorRevision);

  //
  // Construct ACPI table
  //
  Status = gBS->LocateProtocol (&gEfiAcpiTableProtocolGuid, NULL, (VOID **)&AcpiTable);
  ASSERT_EFI_ERROR (Status);

  Status = AcpiTable->InstallAcpiTable (
                        AcpiTable,
                        &Tpm2AcpiTemplate,
                        Tpm2AcpiTemplate.Header.Length,
                        &TableKey
                        );
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/**
  The driver's entry point.

  It patches and installs ACPI tables used for handling TPM physical presence
  and Memory Clear requests through ACPI method.

  @param[in] ImageHandle  The firmware allocated handle for the EFI image.
  @param[in] SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS     The entry point is executed successfully.
  @retval Others          Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
InitializeTcgAcpiFfa (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  VOID        *TpmGuid;

  DEBUG ((DEBUG_INFO, "TCG ACPI FFA Entry Point!\n"));
  TpmGuid = PcdGetPtr (PcdTpmInstanceGuid);
  if (TpmGuid == NULL) {
    DEBUG ((DEBUG_ERROR, "%a - Driver failed due to no TPM2 instance configured!\n", __func__));
    return EFI_UNSUPPORTED;
  }

  if (!CompareGuid (PcdGetPtr (PcdTpmInstanceGuid), &gTpm2ServiceFfaGuid)) {
    DEBUG ((DEBUG_ERROR, "%a - Driver failed due to no the system does not have a TPM2 FFA instance configured, not supported!!\n", __func__));
    return EFI_UNSUPPORTED;
  }

  Status = PublishAcpiTable ();
  ASSERT_EFI_ERROR (Status);

  //
  // Set TPM2 ACPI table
  //
  Status = PublishTpm2 ();
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}
