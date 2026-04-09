/** @file
  MADT Table Generator Implementation.

  This file implements the MADT Table Generator,
  which handles various APIC structures within a system,
  including Local APIC, IO APIC, Interrupt Source Override, and Local APIC NMI.

  Copyright (C) 2024 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <AcpiTableGenerator.h>
#include <ConfigurationManagerObject.h>
#include <ConfigurationManagerHelper.h>
#include <Library/DebugLib.h>
#include <Library/TableHelperLib.h>
#include <Protocol/ConfigurationManagerProtocol.h>
#include <X64NameSpaceObjects.h>
#include <Library/MemoryAllocationLib.h>

/** This macro expands to the total size of the MADT table.
  The size is calculated based on the number of IO APICs,
  Local APICs/X2APICs, Interrupt Source Overrides,
  and Local APIC/X2APIC NMIs.
*/
#define MADT_TOTAL_SIZE(ApicMode,                                                           \
                        IoApicCount,                                                        \
                        IntrSourceOverrideCount,                                            \
                        LocalApicCount,                                                     \
                        LocalApicNmiCount)                                                  \
  (                                                                                         \
    sizeof (EFI_ACPI_6_5_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER) +                          \
    (sizeof (EFI_ACPI_6_5_IO_APIC_STRUCTURE) * IoApicCount) +                               \
    (sizeof (EFI_ACPI_6_5_INTERRUPT_SOURCE_OVERRIDE_STRUCTURE) * IntrSourceOverrideCount) + \
    ((ApicMode == LocalApicModeXApic) ?                                                     \
      ((sizeof (EFI_ACPI_6_5_PROCESSOR_LOCAL_APIC_STRUCTURE) * LocalApicCount) +            \
       (sizeof (EFI_ACPI_6_5_LOCAL_APIC_NMI_STRUCTURE) * LocalApicNmiCount)) :              \
      ((sizeof (EFI_ACPI_6_5_PROCESSOR_LOCAL_X2APIC_STRUCTURE) * LocalApicCount) +          \
       (sizeof (EFI_ACPI_6_5_LOCAL_X2APIC_NMI_STRUCTURE) * LocalApicNmiCount))              \
    )                                                                                       \
  )

/** This macro expands to a function that retrieves the
  MADT local APIC address and flags information
  from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceX64,
  EX64ObjMadtInfo,
  CM_X64_MADT_INFO
  );

/** This macro defines a function to retrieve the
  MADT local APIC or X2APIC information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceX64,
  EX64ObjLocalApicX2ApicInfo,
  CM_X64_LOCAL_APIC_X2APIC_INFO
  );

/** This macro defines a function to retrieve the
  MADT IO APIC information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceX64,
  EX64ObjIoApicInfo,
  CM_X64_IO_APIC_INFO
  );

/** This macro defines a function to retrieve the
  MADT interrupt source override information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceX64,
  EX64ObjIntrSourceOverrideInfo,
  CM_X64_INTR_SOURCE_OVERRIDE_INFO
  );

/** This macro defines a function to retrieve the
  MADT local APIC NMI information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceX64,
  EX64ObjLocalApicX2ApicNmiInfo,
  CM_X64_LOCAL_APIC_X2APIC_NMI_INFO
  );

/** Construct the MADT table.

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
BuildMadtTable (
  IN  CONST ACPI_TABLE_GENERATOR                  *CONST  This,
  IN  CONST CM_STD_OBJ_ACPI_TABLE_INFO            *CONST  AcpiTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  OUT       EFI_ACPI_DESCRIPTION_HEADER          **CONST  Table
  )
{
  CM_X64_INTR_SOURCE_OVERRIDE_INFO                     *IntrSourceOverride;
  CM_X64_IO_APIC_INFO                                  *IoApic;
  CM_X64_LOCAL_APIC_X2APIC_INFO                        *LocalApic;
  CM_X64_LOCAL_APIC_X2APIC_NMI_INFO                    *LocalApicNmi;
  CM_X64_MADT_INFO                                     *MadtInfo;
  EFI_ACPI_6_5_INTERRUPT_SOURCE_OVERRIDE_STRUCTURE     *IntrSourceOverrideEntry;
  EFI_ACPI_6_5_IO_APIC_STRUCTURE                       *IoApicEntry;
  EFI_ACPI_6_5_LOCAL_APIC_NMI_STRUCTURE                *LocalApicNmiEntry;
  EFI_ACPI_6_5_LOCAL_X2APIC_NMI_STRUCTURE              *LocalX2ApicNmiEntry;
  EFI_ACPI_6_5_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER  *MadtHeader;
  EFI_ACPI_6_5_PROCESSOR_LOCAL_APIC_STRUCTURE          *LocalApicEntry;
  EFI_ACPI_6_5_PROCESSOR_LOCAL_X2APIC_STRUCTURE        *LocalX2ApicEntry;
  EFI_STATUS                                           Status;
  LOCAL_APIC_MODE                                      ApicMode;
  UINT32                                               AcpiMadtSize;
  UINT32                                               Index;
  UINT32                                               IntrSourceOverrideCount;
  UINT32                                               IoApicCount;
  UINT32                                               LocalApicCount;
  UINT32                                               LocalApicNmiCount;
  UINT32                                               NextPtrSize;
  UINT8                                                *AcpiMadtPtr;

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

  *Table = NULL;

  Status = GetEX64ObjMadtInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &MadtInfo,
             NULL
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: MADT: Failed to get MADT Info object. Status = %r\n",
      Status
      ));
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  if ((MadtInfo->Flags & ~(UINT32)EFI_ACPI_6_5_PCAT_COMPAT) != 0) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: MADT: Invalid MADT Info object. Flags = 0x%x\n",
      MadtInfo->Flags
      ));
    Status = EFI_INVALID_PARAMETER;
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  ApicMode = MadtInfo->ApicMode;
  if ((ApicMode != LocalApicModeXApic) && (ApicMode != LocalApicModeX2Apic)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: MADT: Invalid MADT Info object. ApicMode = 0x%x\n",
      ApicMode
      ));
    Status = EFI_INVALID_PARAMETER;
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  LocalApicCount = 0;
  Status         = GetEX64ObjLocalApicX2ApicInfo (
                     CfgMgrProtocol,
                     CM_NULL_TOKEN,
                     &LocalApic,
                     &LocalApicCount
                     );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: MADT: Failed to get Local APIC/X2APIC Info object. Status = %r\n",
      Status
      ));
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  if ((LocalApic == NULL) || (LocalApicCount == 0)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: MADT: Local APIC/X2APIC Info object not found.\n"
      ));
    Status = EFI_NOT_FOUND;
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  for (Index = 0; Index < LocalApicCount; Index++) {
    if ((LocalApic[Index].Flags &
         ~(UINT32)(EFI_ACPI_6_5_LOCAL_APIC_ENABLED|EFI_ACPI_6_5_LOCAL_APIC_ONLINE_CAPABLE)) != 0)
    {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: MADT: Invalid Local APIC/X2APIC Info object. Flags = 0x%x\n",
        LocalApic[Index].Flags
        ));
      Status = EFI_INVALID_PARAMETER;
      ASSERT_EFI_ERROR (Status);
      return Status;
    }

    /// If in APIC mode, check for AcpiProcessorUid and ApicId overflow against UINT8_MAX
    if (ApicMode == LocalApicModeXApic) {
      if ((LocalApic[Index].AcpiProcessorUid > MAX_UINT8) ||
          (LocalApic[Index].ApicId > MAX_UINT8))
      {
        DEBUG ((
          DEBUG_ERROR,
          "ERROR: MADT: Invalid Local XAPIC Info object. AcpiProcessorUid = 0x%x, ApicId = 0x%x\n",
          LocalApic[Index].AcpiProcessorUid,
          LocalApic[Index].ApicId
          ));
        Status = EFI_INVALID_PARAMETER;
        ASSERT_EFI_ERROR (Status);
        return Status;
      }
    }
  }

  IoApicCount = 0;
  Status      = GetEX64ObjIoApicInfo (
                  CfgMgrProtocol,
                  CM_NULL_TOKEN,
                  &IoApic,
                  &IoApicCount
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: MADT: Failed to get IO APIC Info object. Status = %r\n",
      Status
      ));
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  if ((IoApic == NULL) || (IoApicCount == 0)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: MADT: IO APIC Info object not found.\n"
      ));
    Status = EFI_NOT_FOUND;
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  IntrSourceOverrideCount = 0;
  Status                  = GetEX64ObjIntrSourceOverrideInfo (
                              CfgMgrProtocol,
                              CM_NULL_TOKEN,
                              &IntrSourceOverride,
                              &IntrSourceOverrideCount
                              );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: MADT: Failed to get Interrupt Source Override Info object. Status = %r\n",
      Status
      ));
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  if ((IntrSourceOverride == NULL) || (IntrSourceOverrideCount == 0)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: MADT: Interrupt Source Override Info object not found.\n"
      ));
    Status = EFI_NOT_FOUND;
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  for (Index = 0; Index < IntrSourceOverrideCount; Index++) {
    if ((IntrSourceOverride[Index].Flags &
         ~(UINT32)(EFI_ACPI_6_5_POLARITY|EFI_ACPI_6_5_TRIGGER_MODE)) != 0)
    {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: MADT: Invalid Interrupt Source Override Info object. Flags = 0x%x\n",
        IntrSourceOverride[Index].Flags
        ));
      Status = EFI_INVALID_PARAMETER;
      ASSERT_EFI_ERROR (Status);
      return Status;
    }
  }

  LocalApicNmiCount = 0;
  Status            = GetEX64ObjLocalApicX2ApicNmiInfo (
                        CfgMgrProtocol,
                        CM_NULL_TOKEN,
                        &LocalApicNmi,
                        &LocalApicNmiCount
                        );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: MADT: Failed to get Local APIC NMI Info object. Status = %r\n",
      Status
      ));
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  if ((LocalApicNmi == NULL) || (LocalApicNmiCount == 0)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: MADT: Local APIC NMI Info object not found.\n"
      ));
    Status = EFI_NOT_FOUND;
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  for (Index = 0; Index < LocalApicNmiCount; Index++) {
    if ((LocalApicNmi[Index].Flags & ~(UINT32)(EFI_ACPI_6_5_POLARITY|EFI_ACPI_6_5_TRIGGER_MODE)) != 0) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: MADT: Invalid Local APIC NMI Info object. Flags = 0x%x\n",
        LocalApicNmi[Index].Flags
        ));
      Status = EFI_INVALID_PARAMETER;
      ASSERT_EFI_ERROR (Status);
      return Status;
    }

    /// check for AcpiProcessorUid overflow against UINT8_MAX if in APIC mode
    if (ApicMode == LocalApicModeXApic) {
      if (LocalApicNmi[Index].AcpiProcessorUid > MAX_UINT8) {
        DEBUG ((
          DEBUG_ERROR,
          "ERROR: MADT: Invalid Local APIC NMI Info object. AcpiProcessorUid = 0x%x\n",
          LocalApicNmi[Index].AcpiProcessorUid
          ));
        Status = EFI_INVALID_PARAMETER;
        ASSERT_EFI_ERROR (Status);
        return Status;
      }
    }
  }

  if (MADT_TOTAL_SIZE (
        ApicMode,
        IoApicCount,
        IntrSourceOverrideCount,
        LocalApicCount,
        LocalApicNmiCount
        ) > MAX_UINT32)
  {
    DEBUG ((DEBUG_ERROR, "ERROR: MADT: MADT_TOTAL_SIZE overflow detected.\n"));
    Status = EFI_INVALID_PARAMETER;
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  AcpiMadtSize = (UINT32)MADT_TOTAL_SIZE (
                           ApicMode,
                           IoApicCount,
                           IntrSourceOverrideCount,
                           LocalApicCount,
                           LocalApicNmiCount
                           );
  AcpiMadtPtr = AllocateZeroPool (AcpiMadtSize);
  if (AcpiMadtPtr == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: MADT: Failed to allocate memory for MADT table.\n"
      ));
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  MadtHeader                   = (EFI_ACPI_6_5_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER *)(VOID *)AcpiMadtPtr;
  MadtHeader->LocalApicAddress = MadtInfo->LocalApicAddress;
  MadtHeader->Flags            = MadtInfo->Flags;

  NextPtrSize = sizeof (EFI_ACPI_6_5_MULTIPLE_APIC_DESCRIPTION_TABLE_HEADER);
  if (ApicMode == LocalApicModeXApic) {
    LocalApicEntry = (EFI_ACPI_6_5_PROCESSOR_LOCAL_APIC_STRUCTURE *)(AcpiMadtPtr + NextPtrSize);
    for (Index = 0; Index < LocalApicCount; Index++) {
      LocalApicEntry[Index].Type             = EFI_ACPI_6_5_PROCESSOR_LOCAL_APIC;
      LocalApicEntry[Index].Length           = sizeof (EFI_ACPI_6_5_PROCESSOR_LOCAL_APIC_STRUCTURE);
      LocalApicEntry[Index].AcpiProcessorUid = (UINT8)LocalApic[Index].AcpiProcessorUid;
      LocalApicEntry[Index].ApicId           = (UINT8)LocalApic[Index].ApicId;
      LocalApicEntry[Index].Flags            = LocalApic[Index].Flags;
    }

    NextPtrSize += (sizeof (EFI_ACPI_6_5_PROCESSOR_LOCAL_APIC_STRUCTURE) * LocalApicCount);
  } else {
    LocalX2ApicEntry = (EFI_ACPI_6_5_PROCESSOR_LOCAL_X2APIC_STRUCTURE *)(AcpiMadtPtr + NextPtrSize);
    for (Index = 0; Index < LocalApicCount; Index++) {
      LocalX2ApicEntry[Index].Type             = EFI_ACPI_6_5_PROCESSOR_LOCAL_X2APIC;
      LocalX2ApicEntry[Index].Length           = sizeof (EFI_ACPI_6_5_PROCESSOR_LOCAL_X2APIC_STRUCTURE);
      LocalX2ApicEntry[Index].Reserved[0]      = 0;
      LocalX2ApicEntry[Index].Reserved[1]      = 0;
      LocalX2ApicEntry[Index].X2ApicId         = LocalApic[Index].ApicId;
      LocalX2ApicEntry[Index].Flags            = LocalApic[Index].Flags;
      LocalX2ApicEntry[Index].AcpiProcessorUid = LocalApic[Index].AcpiProcessorUid;
    }

    NextPtrSize += (sizeof (EFI_ACPI_6_5_PROCESSOR_LOCAL_X2APIC_STRUCTURE) * LocalApicCount);
  }

  IoApicEntry = (EFI_ACPI_6_5_IO_APIC_STRUCTURE *)(AcpiMadtPtr + NextPtrSize);
  for (Index = 0; Index < IoApicCount; Index++) {
    IoApicEntry[Index].Type                      = EFI_ACPI_6_5_IO_APIC;
    IoApicEntry[Index].Length                    = sizeof (EFI_ACPI_6_5_IO_APIC_STRUCTURE);
    IoApicEntry[Index].IoApicId                  = IoApic[Index].IoApicId;
    IoApicEntry[Index].Reserved                  = 0;
    IoApicEntry[Index].IoApicAddress             = IoApic[Index].IoApicAddress;
    IoApicEntry[Index].GlobalSystemInterruptBase = IoApic[Index].GlobalSystemInterruptBase;
  }

  NextPtrSize += (sizeof (EFI_ACPI_6_5_IO_APIC_STRUCTURE) * IoApicCount);

  IntrSourceOverrideEntry = (EFI_ACPI_6_5_INTERRUPT_SOURCE_OVERRIDE_STRUCTURE *)(AcpiMadtPtr + NextPtrSize);
  for (Index = 0; Index < IntrSourceOverrideCount; Index++) {
    IntrSourceOverrideEntry[Index].Type                  = EFI_ACPI_6_5_INTERRUPT_SOURCE_OVERRIDE;
    IntrSourceOverrideEntry[Index].Length                = sizeof (EFI_ACPI_6_5_INTERRUPT_SOURCE_OVERRIDE_STRUCTURE);
    IntrSourceOverrideEntry[Index].Bus                   = IntrSourceOverride[Index].Bus;
    IntrSourceOverrideEntry[Index].Source                = IntrSourceOverride[Index].Source;
    IntrSourceOverrideEntry[Index].GlobalSystemInterrupt = IntrSourceOverride[Index].GlobalSystemInterrupt;
    IntrSourceOverrideEntry[Index].Flags                 = IntrSourceOverride[Index].Flags;
  }

  NextPtrSize += (sizeof (EFI_ACPI_6_5_INTERRUPT_SOURCE_OVERRIDE_STRUCTURE) * IntrSourceOverrideCount);
  if (ApicMode == LocalApicModeXApic) {
    LocalApicNmiEntry = (EFI_ACPI_6_5_LOCAL_APIC_NMI_STRUCTURE *)(AcpiMadtPtr + NextPtrSize);
    for (Index = 0; Index < LocalApicNmiCount; Index++) {
      LocalApicNmiEntry[Index].Type             = EFI_ACPI_6_5_LOCAL_APIC_NMI;
      LocalApicNmiEntry[Index].Length           = sizeof (EFI_ACPI_6_5_LOCAL_APIC_NMI_STRUCTURE);
      LocalApicNmiEntry[Index].AcpiProcessorUid = (UINT8)LocalApicNmi[Index].AcpiProcessorUid;
      LocalApicNmiEntry[Index].Flags            = LocalApicNmi[Index].Flags;
      LocalApicNmiEntry[Index].LocalApicLint    = LocalApicNmi[Index].LocalApicLint;
    }
  } else {
    LocalX2ApicNmiEntry = (EFI_ACPI_6_5_LOCAL_X2APIC_NMI_STRUCTURE *)(AcpiMadtPtr + NextPtrSize);
    for (Index = 0; Index < LocalApicNmiCount; Index++) {
      LocalX2ApicNmiEntry[Index].Type             = EFI_ACPI_6_5_LOCAL_X2APIC_NMI;
      LocalX2ApicNmiEntry[Index].Length           = sizeof (EFI_ACPI_6_5_LOCAL_X2APIC_NMI_STRUCTURE);
      LocalX2ApicNmiEntry[Index].AcpiProcessorUid = LocalApicNmi[Index].AcpiProcessorUid;
      LocalX2ApicNmiEntry[Index].Flags            = LocalApicNmi[Index].Flags;
      LocalX2ApicNmiEntry[Index].LocalX2ApicLint  = LocalApicNmi[Index].LocalApicLint;
    }
  }

  Status = AddAcpiHeader (
             CfgMgrProtocol,
             This,
             (EFI_ACPI_DESCRIPTION_HEADER *)AcpiMadtPtr,
             AcpiTableInfo,
             AcpiMadtSize
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: MADT: Failed to add ACPI header. Status = %r\n",
      Status
      ));
    ASSERT_EFI_ERROR (Status);
    FreePool (AcpiMadtPtr);
    return Status;
  }

  *Table = (EFI_ACPI_DESCRIPTION_HEADER *)AcpiMadtPtr;
  return Status;
}

/** This macro defines the MADT Table Generator revision.
*/
#define MADT_GENERATOR_REVISION  CREATE_REVISION (1, 0)

/** The interface for the MADT Table Generator.
*/
STATIC
CONST
ACPI_TABLE_GENERATOR  MadtGenerator = {
  // Generator ID
  CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdMadt),
  // Generator Description
  L"ACPI.STD.MADT.GENERATOR",
  // ACPI Table Signature
  EFI_ACPI_6_5_MULTIPLE_APIC_DESCRIPTION_TABLE_SIGNATURE,
  // ACPI Table Revision supported by this Generator
  EFI_ACPI_6_5_MULTIPLE_APIC_DESCRIPTION_TABLE_REVISION,
  // Minimum supported ACPI Table Revision
  EFI_ACPI_6_5_MULTIPLE_APIC_DESCRIPTION_TABLE_REVISION,
  // Creator ID
  TABLE_GENERATOR_CREATOR_ID,
  // Creator Revision
  MADT_GENERATOR_REVISION,
  // Build Table function
  BuildMadtTable,
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
AcpiMadtLibConstructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = RegisterAcpiTableGenerator (&MadtGenerator);
  DEBUG ((DEBUG_INFO, "MADT: Register Generator. Status = %r\n", Status));
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
AcpiMadtLibDestructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = DeregisterAcpiTableGenerator (&MadtGenerator);
  DEBUG ((DEBUG_INFO, "MADT: Deregister Generator. Status = %r\n", Status));
  ASSERT_EFI_ERROR (Status);
  return Status;
}
