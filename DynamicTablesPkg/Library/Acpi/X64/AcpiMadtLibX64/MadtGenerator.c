/** @file
  This library provides the implementation of the ACPI Multiple APIC Description Table (MADT)
  for X64 architecture.

  Copyright (c) 2017 - 2023, Arm Limited. All rights reserved.
  Copyright (C) 2024 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <AcpiTableGenerator.h>
#include <ConfigurationManagerObject.h>
#include <ConfigurationManagerHelper.h>
#include <Library/TableHelperLib.h>
#include <Protocol/ConfigurationManagerProtocol.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <X64NameSpaceObjects.h>

/** The Creator ID for the ACPI tables generated using
  the standard ACPI table generators.
*/
#define TABLE_GENERATOR_CREATOR_ID_GENERIC  SIGNATURE_32('D', 'Y', 'N', 'T')

/// MACRO to create GET_OBJECT_LIST for EX64ObjMadtLocalInterruptInfo
GET_OBJECT_LIST (
  EObjNameSpaceX64,
  EX64ObjMadtLocalInterruptInfo,
  CM_X64_MADT_LOCAL_INTERRUPT_INFO
  );

/// MACRO to create GET_OBJECT_LIST for EX64ObjMadtProcessorLocalApicX2ApicInfo
GET_OBJECT_LIST (
  EObjNameSpaceX64,
  EX64ObjMadtProcessorLocalApicX2ApicInfo,
  CM_X64_MADT_INTERRUPT_CONTROLLER_TYPE_INFO
  );

/// MACRO to create GET_OBJECT_LIST for EX64ObjMadtIoApicInfo
GET_OBJECT_LIST (
  EObjNameSpaceX64,
  EX64ObjMadtIoApicInfo,
  CM_X64_MADT_INTERRUPT_CONTROLLER_TYPE_INFO
  );

/// MACRO to create GET_OBJECT_LIST for EX64ObjMadtLocalApicX2ApicNmiInfo
GET_OBJECT_LIST (
  EObjNameSpaceX64,
  EX64ObjMadtLocalApicX2ApicNmiInfo,
  CM_X64_MADT_INTERRUPT_CONTROLLER_TYPE_INFO
  );

/// MACRO to create GET_OBJECT_LIST for EX64ObjMadtInterruptSourceOverrideInfo
GET_OBJECT_LIST (
  EObjNameSpaceX64,
  EX64ObjMadtInterruptSourceOverrideInfo,
  CM_X64_MADT_INTERRUPT_CONTROLLER_TYPE_INFO
  );

/**
  Build the ACPI MADT table.

  This function builds the ACPI MADT table as per the input configuration.

  @param[in]  This              Pointer to the ACPI_TABLE_GENERATOR protocol.
  @param[in]  AcpiTableInfo     Pointer to the CM_STD_OBJ_ACPI_TABLE_INFO structure.
  @param[in]  CfgMgrProtocol    Pointer to the EDKII_CONFIGURATION_MANAGER_PROTOCOL protocol.
  @param[out] Table             Pointer to the ACPI table structure.

  @retval EFI_SUCCESS           The ACPI MADT table is built successfully.
  @retval EFI_INVALID_PARAMETER The input parameter is invalid.
**/
STATIC
EFI_STATUS
BuildMadtTable (
  IN  CONST ACPI_TABLE_GENERATOR                  *CONST  This,
  IN  CONST CM_STD_OBJ_ACPI_TABLE_INFO            *CONST  AcpiTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  OUT       EFI_ACPI_DESCRIPTION_HEADER          **CONST  Table
  )
{
  EFI_STATUS                                           Status;
  EFI_ACPI_6_5_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER  *Madt;
  UINTN                                                MadtSize;
  CM_X64_MADT_LOCAL_INTERRUPT_INFO                     *LocalInterruptInfo;
  CM_X64_MADT_INTERRUPT_CONTROLLER_TYPE_INFO           *ProcessorLocalApicX2ApicInfo;
  CM_X64_MADT_INTERRUPT_CONTROLLER_TYPE_INFO           *IoApicInfo;
  CM_X64_MADT_INTERRUPT_CONTROLLER_TYPE_INFO           *LocalApicX2ApicNmiInfo;
  CM_X64_MADT_INTERRUPT_CONTROLLER_TYPE_INFO           *InterruptSourceOverrideInfo;

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
      "ERROR: MADT: Requested table revision = %d, is not supported."
      "Supported table revision: Minimum = %d, Maximum = %d\n",
      AcpiTableInfo->AcpiTableRevision,
      This->MinAcpiTableRevision,
      This->AcpiTableRevision
      ));
    return EFI_INVALID_PARAMETER;
  }

  *Table   = NULL;
  MadtSize = sizeof (EFI_ACPI_6_5_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER);

  // Get Local Interrupt Info
  Status = GetEX64ObjMadtLocalInterruptInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &LocalInterruptInfo,
             NULL
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ERROR: MADT: Failed to get Local Interrupt Info\n"));
    return Status;
  }

  // Get Processor Local APIC/x2APIC Info
  Status = GetEX64ObjMadtProcessorLocalApicX2ApicInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &ProcessorLocalApicX2ApicInfo,
             NULL
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ERROR: MADT: Failed to get Processor Local APIC/x2APIC Info\n"));
    return Status;
  } else {
    if (ProcessorLocalApicX2ApicInfo->Size == 0) {
      DEBUG ((DEBUG_ERROR, "ERROR: MADT: Processor Local APIC/x2APIC Info size is 0\n"));
      return EFI_NOT_FOUND;
    }
  }

  MadtSize += ProcessorLocalApicX2ApicInfo->Size;

  // Get IO APIC Info
  Status = GetEX64ObjMadtIoApicInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &IoApicInfo,
             NULL
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ERROR: MADT: Failed to get IO APIC Info\n"));
    return Status;
  } else {
    if (IoApicInfo->Size == 0) {
      DEBUG ((DEBUG_ERROR, "ERROR: MADT: IO APIC Info size is 0\n"));
      return EFI_NOT_FOUND;
    }
  }

  MadtSize += IoApicInfo->Size;

  // Get Local APIC/x2APIC NMI Info
  Status = GetEX64ObjMadtLocalApicX2ApicNmiInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &LocalApicX2ApicNmiInfo,
             NULL
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ERROR: MADT: Failed to get Local APIC/x2APIC NMI Info\n"));
    return Status;
  } else {
    if (LocalApicX2ApicNmiInfo->Size == 0) {
      DEBUG ((DEBUG_ERROR, "ERROR: MADT: Local APIC/x2APIC NMI Info size is 0\n"));
      return EFI_NOT_FOUND;
    }
  }

  MadtSize += LocalApicX2ApicNmiInfo->Size;

  // Get Interrupt Source Override Info
  Status = GetEX64ObjMadtInterruptSourceOverrideInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &InterruptSourceOverrideInfo,
             NULL
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "WARNING: MADT: Failed to get Interrupt Source Override Info\n"));
  }

  if ((InterruptSourceOverrideInfo != NULL) && (InterruptSourceOverrideInfo->Size != 0)) {
    MadtSize += InterruptSourceOverrideInfo->Size;
  }

  // Allocate memory for Madt pointer
  Madt = (EFI_ACPI_6_5_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER *)AllocateZeroPool (MadtSize);
  if (Madt == NULL) {
    DEBUG ((DEBUG_ERROR, "ERROR: MADT: Failed to allocate memory for MADT\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  // Initialize the MADT header
  Status = AddAcpiHeader (
             CfgMgrProtocol,
             This,
             &Madt->Header,
             AcpiTableInfo,
             (UINT32)MadtSize
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ERROR: MADT: Failed to initialize MADT header\n"));
    FreePool (Madt);
    return Status;
  }

  Madt->LocalApicAddress = LocalInterruptInfo->LocalApicAddress;
  Madt->Flags            = LocalInterruptInfo->Flags;

  CopyMem (
    (UINT8 *)Madt + sizeof (EFI_ACPI_6_5_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER),
    ProcessorLocalApicX2ApicInfo->InterruptControllerTypeInfo,
    ProcessorLocalApicX2ApicInfo->Size
    );

  CopyMem (
    (UINT8 *)Madt + sizeof (EFI_ACPI_6_5_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER) + ProcessorLocalApicX2ApicInfo->Size,
    IoApicInfo->InterruptControllerTypeInfo,
    IoApicInfo->Size
    );

  CopyMem (
    (UINT8 *)Madt + sizeof (EFI_ACPI_6_5_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER) + ProcessorLocalApicX2ApicInfo->Size + IoApicInfo->Size,
    LocalApicX2ApicNmiInfo->InterruptControllerTypeInfo,
    LocalApicX2ApicNmiInfo->Size
    );

  if ((InterruptSourceOverrideInfo != NULL) && (InterruptSourceOverrideInfo->Size != 0)) {
    CopyMem (
      (UINT8 *)Madt + sizeof (EFI_ACPI_6_5_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER) + ProcessorLocalApicX2ApicInfo->Size + IoApicInfo->Size + LocalApicX2ApicNmiInfo->Size,
      InterruptSourceOverrideInfo->InterruptControllerTypeInfo,
      InterruptSourceOverrideInfo->Size
      );
  }

  *Table = (EFI_ACPI_DESCRIPTION_HEADER *)Madt;
  return EFI_SUCCESS;
}

/** Free any resources allocated for constructing the MADT

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
FreeMadtTableResources (
  IN      CONST ACPI_TABLE_GENERATOR                  *CONST  This,
  IN      CONST CM_STD_OBJ_ACPI_TABLE_INFO            *CONST  AcpiTableInfo,
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN OUT        EFI_ACPI_DESCRIPTION_HEADER          **CONST  Table
  )
{
  ASSERT (This != NULL);
  ASSERT (AcpiTableInfo != NULL);
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (AcpiTableInfo->TableGeneratorId == This->GeneratorID);
  ASSERT (AcpiTableInfo->AcpiTableSignature == This->AcpiTableSignature);

  if ((Table == NULL) || (*Table == NULL)) {
    DEBUG ((DEBUG_ERROR, "ERROR: MADT: Invalid Table Pointer\n"));
    ASSERT ((Table != NULL) && (*Table != NULL));
    return EFI_INVALID_PARAMETER;
  }

  *Table = NULL;
  return EFI_SUCCESS;
}

/// Macro for MADT Table generator revision
#define MADT_GENERATOR_REVISION  CREATE_REVISION (1, 0)

/// Interface for MADT table generator
STATIC
CONST
ACPI_TABLE_GENERATOR  mMadtTableGenerator = {
  // Generator ID
  CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdMadt),
  // Generator Description
  L"ACPI.STD.MADT.GENERATOR",
  // ACPI Table Signature
  EFI_ACPI_6_5_MULTIPLE_APIC_DESCRIPTION_TABLE_SIGNATURE,
  // ACPI Table Revision supported by this Generator
  EFI_ACPI_6_5_MULTIPLE_APIC_DESCRIPTION_TABLE_REVISION,
  // Minimum supported ACPI Table Revision
  EFI_ACPI_6_2_MULTIPLE_APIC_DESCRIPTION_TABLE_REVISION,
  // Creator ID
  TABLE_GENERATOR_CREATOR_ID_GENERIC,
  // Creator Revision
  MADT_GENERATOR_REVISION,
  // Build Table function
  BuildMadtTable,
  // Free Resource function
  FreeMadtTableResources,
  // Extended build function not needed
  NULL,
  // Extended build function not implemented by the generator.
  // Hence extended free resource function is not required.
  NULL
};

/**
  The constructor function registers the ACPI MADT table generator.

  The constructor function registers the ACPI MADT table generator.

  @param[in]  ImageHandle       The firmware allocated handle for the EFI image.
  @param[in]  SystemTable       A pointer to the EFI System Table.

  @retval EFI_SUCCESS           The constructor executed successfully.
  @retval EFI_INVALID_PARAMETER The input parameter is invalid.
**/
EFI_STATUS
EFIAPI
AcpiMadtLibX64Constructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = RegisterAcpiTableGenerator (&mMadtTableGenerator);
  DEBUG ((DEBUG_INFO, "MADT: Register Generator. Status = %r\n", Status));
  ASSERT_EFI_ERROR (Status);
  return Status;
}

/**
  The destructor function unregisters the ACPI MADT table generator.

  The destructor function unregisters the ACPI MADT table generator.

  @param[in]  ImageHandle       The firmware allocated handle for the EFI image.
  @param[in]  SystemTable       A pointer to the EFI System Table.

  @retval EFI_SUCCESS           The destructor executed successfully.
  @retval EFI_INVALID_PARAMETER The input parameter is invalid.
**/
EFI_STATUS
EFIAPI
AcpiMadtLibX64Destructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = DeregisterAcpiTableGenerator (&mMadtTableGenerator);
  DEBUG ((DEBUG_INFO, "MADT: Unregister Generator. Status = %r\n", Status));
  ASSERT_EFI_ERROR (Status);
  return Status;
}
