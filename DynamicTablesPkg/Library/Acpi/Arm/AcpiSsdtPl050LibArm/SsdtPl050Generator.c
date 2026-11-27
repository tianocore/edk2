/** @file
  SSDT PL050 AML Table Generator.

  Copyright (c) 2026, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - ARM PrimeCell PS/2 Keyboard/Mouse Interface (PL050) Technical Reference
    Manual, ARM DDI 0143C
**/

#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>

// Module specific include files.
#include <AcpiTableGenerator.h>
#include <ConfigurationManagerHelper.h>
#include <IndustryStandard/Acpi.h>
#include <Library/AcpiHelperLib.h>
#include <Library/AmlLib/AmlLib.h>
#include <Protocol/ConfigurationManagerProtocol.h>
#include <Library/TableHelperLib.h>
#include "SsdtPl050Generator.h"

/** SSDT PL050 Table Generator.

  Requirements:
  The following Configuration Manager Object is required by this Generator:
  - EArmObjPl050Info
*/

/** This macro expands to a function that retrieves the PL050 information from
    the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArm,
  EArmObjPl050Info,
  CM_ARM_PL050_INFO
  );

/** Validate the PL050 Configuration Manager information.

  @param [in] Pl050Info   Array of PL050 information structures.
  @param [in] Pl050Count  Number of entries in Pl050Info.

  @retval EFI_SUCCESS            The PL050 information is valid.
  @retval EFI_INVALID_PARAMETER  A parameter or address is invalid.
**/
STATIC
EFI_STATUS
EFIAPI
ValidatePl050Info (
  IN  CONST CM_ARM_PL050_INFO  *CONST  Pl050Info,
  IN        UINT32                     Pl050Count
  )
{
  BOOLEAN                  KeyboardFound;
  BOOLEAN                  MouseFound;
  UINT32                   Index;
  CONST CM_ARM_PL050_INFO  *Info;

  if ((Pl050Info == NULL) || (Pl050Count == 0)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-PL050: Invalid device list or count %u.\n",
      Pl050Count
      ));
    return EFI_INVALID_PARAMETER;
  }

  KeyboardFound = FALSE;
  MouseFound    = FALSE;

  for (Index = 0; Index < Pl050Count; Index++) {
    Info = &Pl050Info[Index];

    if (Info->IsMouse) {
      if (MouseFound) {
        DEBUG ((
          DEBUG_ERROR,
          "ERROR: SSDT-PL050: Duplicate mouse interface at index %u.\n",
          Index
          ));
        return EFI_INVALID_PARAMETER;
      }

      MouseFound = TRUE;
    } else {
      if (KeyboardFound) {
        DEBUG ((
          DEBUG_ERROR,
          "ERROR: SSDT-PL050: Duplicate keyboard interface at index %u.\n",
          Index
          ));
        return EFI_INVALID_PARAMETER;
      }

      KeyboardFound = TRUE;
    }

    // KMIDATA has the highest register offset described by this generator.
    if (Info->BaseAddress >
        (MAX_UINT64 - PL050_KMIDATA_OFFSET - (PL050_REGISTER_LENGTH - 1)))
    {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-PL050: Register address overflow for device %u, "
        "BaseAddress = 0x%llx.\n",
        Index,
        Info->BaseAddress
        ));
      return EFI_INVALID_PARAMETER;
    }

    if ((Info->Interrupt.Flags &
         EFI_ACPI_EXTENDED_INTERRUPT_FLAG_PRODUCER_CONSUMER_MASK) == 0)
    {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-PL050: Interrupt must be a resource consumer "
        "for device %u.\n",
        Index
        ));
      return EFI_INVALID_PARAMETER;
    }
  }

  return EFI_SUCCESS;
}

/** Append a fixed memory resource for a PL050 register.

  A Memory32Fixed descriptor is used when the complete register range fits in
  the 32-bit address space. Otherwise, a QWordMemory descriptor is used.

  @param [in] BaseAddress     PL050 controller base address.
  @param [in] RegisterOffset  Register offset from BaseAddress.
  @param [in] IsReadWrite     TRUE for ReadWrite, FALSE for ReadOnly.
  @param [in] CrsNode         AML _CRS Name object node.

  @retval EFI_SUCCESS            The resource was appended.
  @retval EFI_INVALID_PARAMETER  The address range or parameter is invalid.
  @retval EFI_OUT_OF_RESOURCES   Memory allocation failed.
**/
STATIC
EFI_STATUS
EFIAPI
AddPl050RegisterResource (
  IN  UINT64                  BaseAddress,
  IN  UINT64                  RegisterOffset,
  IN  BOOLEAN                 IsReadWrite,
  IN  AML_OBJECT_NODE_HANDLE  CrsNode
  )
{
  UINT64  AddressMaximum;
  UINT64  AddressMinimum;

  AddressMinimum = BaseAddress + RegisterOffset;
  AddressMaximum = AddressMinimum + PL050_REGISTER_LENGTH - 1;

  if (AddressMaximum <= MAX_UINT32) {
    return AmlCodeGenRdMemory32Fixed (
             IsReadWrite,
             (UINT32)AddressMinimum,
             PL050_REGISTER_LENGTH,
             CrsNode,
             NULL
             );
  }

  return AmlCodeGenRdQWordMemory (
           TRUE,
           TRUE,
           TRUE,
           TRUE,
           AmlMemoryNonCacheable,
           IsReadWrite,
           0,
           AddressMinimum,
           AddressMaximum,
           0,
           PL050_REGISTER_LENGTH,
           0,
           NULL,
           AmlAddressRangeMemory,
           TRUE,
           CrsNode,
           NULL
           );
}

/** Create the _CRS object for a PL050 device.

  @param [in] Pl050Info  PL050 Configuration Manager information.
  @param [in] DeviceNode AML PL050 Device object node.

  @retval EFI_SUCCESS            The _CRS object was created.
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.
  @retval EFI_OUT_OF_RESOURCES   Memory allocation failed.
**/
STATIC
EFI_STATUS
EFIAPI
CreatePl050Crs (
  IN  CONST CM_ARM_PL050_INFO  *CONST  Pl050Info,
  IN        AML_OBJECT_NODE_HANDLE     DeviceNode
  )
{
  AML_OBJECT_NODE_HANDLE  CrsNode;
  UINT32                  InterruptNumber;
  UINT32                  InterruptFlags;
  EFI_STATUS              Status;

  Status = AmlCodeGenNameResourceTemplate ("_CRS", DeviceNode, &CrsNode);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-PL050: Failed to create AML _CRS object. Status = %r\n",
      Status
      ));
    return Status;
  }

  // The PL050 register resources must be described in this order.
  Status = AddPl050RegisterResource (
             Pl050Info->BaseAddress,
             PL050_KMIDATA_OFFSET,
             TRUE,
             CrsNode
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-PL050: Failed to create KMIDATA memory resource. "
      "Status = %r\n",
      Status
      ));
    return Status;
  }

  Status = AddPl050RegisterResource (
             Pl050Info->BaseAddress,
             PL050_KMICR_OFFSET,
             TRUE,
             CrsNode
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-PL050: Failed to create KMICR memory resource. "
      "Status = %r\n",
      Status
      ));
    return Status;
  }

  Status = AddPl050RegisterResource (
             Pl050Info->BaseAddress,
             PL050_KMISTAT_OFFSET,
             FALSE,
             CrsNode
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-PL050: Failed to create KMISTAT memory resource. "
      "Status = %r\n",
      Status
      ));
    return Status;
  }

  InterruptNumber = Pl050Info->Interrupt.Interrupt;
  InterruptFlags  = Pl050Info->Interrupt.Flags;
  Status          = AmlCodeGenRdInterrupt (
                      ((InterruptFlags &
                        EFI_ACPI_EXTENDED_INTERRUPT_FLAG_PRODUCER_CONSUMER_MASK) != 0),
                      ((InterruptFlags &
                        EFI_ACPI_EXTENDED_INTERRUPT_FLAG_MODE_MASK) != 0),
                      ((InterruptFlags &
                        EFI_ACPI_EXTENDED_INTERRUPT_FLAG_POLARITY_MASK) != 0),
                      ((InterruptFlags &
                        EFI_ACPI_EXTENDED_INTERRUPT_FLAG_SHARABLE_MASK) != 0),
                      &InterruptNumber,
                      1,
                      CrsNode,
                      NULL
                      );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-PL050: Failed to create AML Interrupt resource. "
      "Status = %r\n",
      Status
      ));
  }

  return Status;
}

/** Create one PL050 AML Device object.

  @param [in] Pl050Info  PL050 Configuration Manager information.
  @param [in] Index      Index used to form the KMIx device name.
  @param [in] ScopeNode  AML System Bus Scope object node.

  @retval EFI_SUCCESS            The device object was created.
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.
  @retval EFI_OUT_OF_RESOURCES   Memory allocation failed.
**/
STATIC
EFI_STATUS
EFIAPI
BuildPl050Device (
  IN  CONST CM_ARM_PL050_INFO  *CONST  Pl050Info,
  IN        UINT32                     Index,
  IN        AML_OBJECT_NODE_HANDLE     ScopeNode
  )
{
  CHAR8                   Name[AML_NAME_SEG_SIZE + 1];
  CONST CHAR8             *Cid;
  AML_OBJECT_NODE_HANDLE  DeviceNode;
  CONST CHAR8             *Hid;
  EFI_STATUS              Status;

  Name[0] = 'K';
  Name[1] = 'M';
  Name[2] = 'I';
  Name[3] = AsciiFromHex ((UINT8)Index);
  Name[4] = '\0';

  if (Pl050Info->IsMouse) {
    Hid = PL050_MOUSE_HID;
    Cid = PL050_MOUSE_CID;
  } else {
    Hid = PL050_KEYBOARD_HID;
    Cid = PL050_KEYBOARD_CID;
  }

  Status = AmlCodeGenDevice (Name, ScopeNode, &DeviceNode);
  if (!EFI_ERROR (Status)) {
    Status = AmlCodeGenNameString ("_HID", Hid, DeviceNode, NULL);
  }

  if (!EFI_ERROR (Status)) {
    Status = AmlCodeGenNameString ("_CID", Cid, DeviceNode, NULL);
  }

  if (!EFI_ERROR (Status)) {
    Status = CreatePl050Crs (Pl050Info, DeviceNode);
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-PL050: Failed to create AML device %a. Status = %r\n",
      Name,
      Status
      ));
  }

  return Status;
}

/** Construct an SSDT describing PL050 keyboard and mouse interfaces.

  @param [in]  This            Pointer to the ACPI table generator.
  @param [in]  AcpiTableInfo   Pointer to the ACPI table information.
  @param [in]  CfgMgrProtocol  Pointer to the Configuration Manager protocol.
  @param [out] Table           Pointer to the generated ACPI table.

  @retval EFI_SUCCESS            The table was generated successfully.
  @retval EFI_INVALID_PARAMETER  A parameter or CM object is invalid.
  @retval Others                 Failed to build the table.
**/
STATIC
EFI_STATUS
EFIAPI
BuildSsdtPl050Table (
  IN  CONST ACPI_TABLE_GENERATOR                   *CONST  This,
  IN  CONST CM_STD_OBJ_ACPI_TABLE_INFO             *CONST  AcpiTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL   *CONST  CfgMgrProtocol,
  OUT       EFI_ACPI_DESCRIPTION_HEADER           **CONST  Table
  )
{
  EFI_STATUS              CleanupStatus;
  UINT32                  Index;
  CM_ARM_PL050_INFO       *Pl050Info;
  UINT32                  Pl050Count;
  AML_ROOT_NODE_HANDLE    RootNode;
  AML_OBJECT_NODE_HANDLE  ScopeNode;
  EFI_STATUS              Status;

  ASSERT (This != NULL);
  ASSERT (AcpiTableInfo != NULL);
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (Table != NULL);
  ASSERT (AcpiTableInfo->TableGeneratorId == This->GeneratorID);
  ASSERT (AcpiTableInfo->AcpiTableSignature == This->AcpiTableSignature);

  *Table   = NULL;
  RootNode = NULL;

  Status = GetEArmObjPl050Info (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &Pl050Info,
             &Pl050Count
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-PL050: Failed to get PL050 information. Status = %r\n",
      Status
      ));
    return Status;
  }

  Status = ValidatePl050Info (Pl050Info, Pl050Count);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = AddSsdtAcpiHeader (
             CfgMgrProtocol,
             This,
             AcpiTableInfo,
             &RootNode
             );
  if (EFI_ERROR (Status)) {
    goto exit_handler;
  }

  Status = AmlCodeGenScope ("\\_SB_", RootNode, &ScopeNode);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-PL050: Failed to create AML system bus scope. "
      "Status = %r\n",
      Status
      ));
    goto exit_handler;
  }

  for (Index = 0; Index < Pl050Count; Index++) {
    Status = BuildPl050Device (&Pl050Info[Index], Index, ScopeNode);
    if (EFI_ERROR (Status)) {
      goto exit_handler;
    }
  }

  Status = AmlSerializeDefinitionBlock (RootNode, Table);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-PL050: Failed to serialize SSDT. Status = %r\n",
      Status
      ));
  }

exit_handler:
  if (RootNode != NULL) {
    CleanupStatus = AmlDeleteTree (RootNode);
    if (EFI_ERROR (CleanupStatus)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-PL050: Failed to delete AML tree. Status = %r\n",
        CleanupStatus
        ));
      if (!EFI_ERROR (Status)) {
        Status = CleanupStatus;
      }
    }
  }

  if (EFI_ERROR (Status) && (*Table != NULL)) {
    FreePool (*Table);
    *Table = NULL;
  }

  return Status;
}

/** Free the generated SSDT PL050 table buffer.

  @param [in]      This            Pointer to the ACPI table generator.
  @param [in]      AcpiTableInfo   Pointer to the ACPI table information.
  @param [in]      CfgMgrProtocol  Pointer to the Configuration Manager protocol.
  @param [in, out] Table           Pointer to the generated ACPI table.

  @retval EFI_SUCCESS            The table buffer was freed.
  @retval EFI_INVALID_PARAMETER  The table pointer is NULL or invalid.
**/
STATIC
EFI_STATUS
EFIAPI
FreeSsdtPl050TableResources (
  IN      CONST ACPI_TABLE_GENERATOR                   *CONST  This,
  IN      CONST CM_STD_OBJ_ACPI_TABLE_INFO             *CONST  AcpiTableInfo,
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL   *CONST  CfgMgrProtocol,
  IN OUT        EFI_ACPI_DESCRIPTION_HEADER           **CONST  Table
  )
{
  ASSERT (This != NULL);
  ASSERT (AcpiTableInfo != NULL);
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (AcpiTableInfo->TableGeneratorId == This->GeneratorID);
  ASSERT (AcpiTableInfo->AcpiTableSignature == This->AcpiTableSignature);

  if ((Table == NULL) || (*Table == NULL)) {
    DEBUG ((DEBUG_ERROR, "ERROR: SSDT-PL050: Invalid table pointer.\n"));
    ASSERT ((Table != NULL) && (*Table != NULL));
    return EFI_INVALID_PARAMETER;
  }

  FreePool (*Table);
  *Table = NULL;
  return EFI_SUCCESS;
}

#define SSDT_PL050_GENERATOR_REVISION  CREATE_REVISION (1, 0)

/** SSDT PL050 table generator registration. */
STATIC CONST ACPI_TABLE_GENERATOR  SsdtPl050Generator = {
  // Generator ID
  CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdSsdtPl050),
  // Generator Description
  L"ACPI.STD.SSDT.PL050.GENERATOR",
  // ACPI Table Signature
  EFI_ACPI_6_6_SECONDARY_SYSTEM_DESCRIPTION_TABLE_SIGNATURE,
  // ACPI Table Revision - Unused
  0,
  // Minimum ACPI Table Revision - Unused
  0,
  // Creator ID
  TABLE_GENERATOR_CREATOR_ID_ARM,
  // Creator Revision
  SSDT_PL050_GENERATOR_REVISION,
  // Build table function
  BuildSsdtPl050Table,
  // Free table function
  FreeSsdtPl050TableResources,
  // Build Table function. Extended version not needed.
  NULL,
  // Free Resource function. Extended version not needed.
  NULL
};

/** Register the SSDT PL050 generator with the ACPI table factory.

  @param [in] ImageHandle  The image handle.
  @param [in] SystemTable  Pointer to the system table.

  @retval EFI_SUCCESS           The generator was registered.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_ALREADY_STARTED   The generator is already registered.
**/
EFI_STATUS
EFIAPI
AcpiSsdtPl050LibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = RegisterAcpiTableGenerator (&SsdtPl050Generator);
  DEBUG ((
    DEBUG_INFO,
    "SSDT-PL050: Register generator. Status = %r\n",
    Status
    ));
  ASSERT_EFI_ERROR (Status);
  return Status;
}

/** Deregister the SSDT PL050 generator from the ACPI table factory.

  @param [in] ImageHandle  The image handle.
  @param [in] SystemTable  Pointer to the system table.

  @retval EFI_SUCCESS           The generator was deregistered.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval EFI_NOT_FOUND         The generator is not registered.
**/
EFI_STATUS
EFIAPI
AcpiSsdtPl050LibDestructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = DeregisterAcpiTableGenerator (&SsdtPl050Generator);
  DEBUG ((
    DEBUG_INFO,
    "SSDT-PL050: Deregister generator. Status = %r\n",
    Status
    ));
  ASSERT_EFI_ERROR (Status);
  return Status;
}
