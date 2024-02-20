/** @file
  Ssdt Generic Device Generator

  Copyright (c) 2020 - 2021, Arm Limited. All rights reserved.<BR>
  Copyright (c) 2024 - 2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Protocol/AcpiTable.h>

// Module specific include files.
#include <AcpiTableGenerator.h>
#include <ConfigurationManagerObject.h>
#include <ConfigurationManagerHelper.h>
#include <Library/AcpiHelperLib.h>
#include <Library/AmlLib/AmlLib.h>
#include <Library/TableHelperLib.h>
#include <Protocol/ConfigurationManagerProtocol.h>

#define SB_SCOPE  "\\_SB_"

/** Standard SSDT Generic Device Table Generator

  Constructs SSDT tables describing generic devices

  This is designed to build devices of the format

    Device(DEV0) {
      Name (_HID, "TEST1080")
      Name (_UID, 0)
      Name (_CCA, ZERO)

      Name(_CRS, ResourceTemplate() {
        Memory32Fixed(ReadWrite, Base1, Size1)
        Memory32Fixed(ReadWrite, Base2, Size2)
        Interrupt(ResourceConsumer, Level, ActiveHigh, Exclusive) { Int1, Int2 }
      })
    }

Requirements:
  The following Configuration Manager Object(s) are required by
  this Generator:
  - EArchCommonObjGenericDeviceInfo
  - EArchCommonObjMemoryRangeDescriptor
  - EArchCommonObjGenericInterrupt
*/

/** This macro expands to a function that retrieves the generic device
    information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjGenericDeviceInfo,
  CM_ARCH_COMMON_GENERIC_DEVICE_INFO
  );

/** This macro expands to a function that retrieves the
    Memory Range Descriptor Array information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjMemoryRangeDescriptor,
  CM_ARCH_COMMON_MEMORY_RANGE_DESCRIPTOR
  );

/** This macro expands to a function that retrieves the
   Interrupt Array information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjGenericInterrupt,
  CM_ARCH_COMMON_GENERIC_INTERRUPT
  );

/** Free any resources allocated for constructing the tables.

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
EFIAPI
FreeSsdtGenericDeviceTable (
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
    DEBUG ((DEBUG_ERROR, "ERROR: SSDT-GENERIC-DEVICE: Invalid Table Pointer\n"));
    ASSERT ((Table != NULL) && (*Table != NULL));
    return EFI_INVALID_PARAMETER;
  }

  FreePool (*Table);
  *Table = NULL;
  return EFI_SUCCESS;
}

/** Add a device to SSDT.

  @param [in]  CfgMgrProtocol    Pointer to the Configuration Manager
                                 Protocol interface.
  @param [in]  ParentNode        Node where to add device.
  @param [in]  GenericDeviceInfo Device Info to add.

  @retval EFI_SUCCESS           The device was added successfully.
  @retval others                Error adding device.
**/
STATIC
EFI_STATUS
EFIAPI
AddGenericDeviceToSsdt (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN      AML_OBJECT_NODE_HANDLE                          ParentNode,
  IN      CM_ARCH_COMMON_GENERIC_DEVICE_INFO              *GenericDeviceInfo
  )
{
  EFI_STATUS                              Status;
  AML_OBJECT_NODE_HANDLE                  DeviceNode;
  AML_OBJECT_NODE_HANDLE                  CrsNode;
  BOOLEAN                                 DeviceAttached;
  UINT32                                  Index;
  CM_ARCH_COMMON_MEMORY_RANGE_DESCRIPTOR  *MemoryRanges;
  UINT32                                  MemoryRangesCount;
  CM_ARCH_COMMON_GENERIC_INTERRUPT        *Interrupts;
  UINT32                                  InterruptsCount;
  BOOLEAN                                 EdgeTriggered;
  BOOLEAN                                 ActiveLow;

  DeviceAttached = FALSE;

  Status = AmlCodeGenDevice (GenericDeviceInfo->Name, ParentNode, &DeviceNode);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ERROR: SSDT-GENERIC-DEVICE: Failed to create device node - %r\n", Status));
    ASSERT (0);
    goto exit_handler;
  }

  DeviceAttached = TRUE;

  Status = AmlCodeGenNameString (
             "_HID",
             GenericDeviceInfo->Hid,
             DeviceNode,
             NULL
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ERROR: SSDT-GENERIC-DEVICE: Failed to create HID node - %r\n", Status));
    ASSERT (0);
    goto exit_handler;
  }

  if (GenericDeviceInfo->CidValid) {
    Status = AmlCodeGenNameString (
               "_CID",
               GenericDeviceInfo->Cid,
               DeviceNode,
               NULL
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "ERROR: SSDT-GENERIC-DEVICE: Failed to create CID node - %r\n", Status));
      ASSERT (0);
      goto exit_handler;
    }
  }

  Status = AmlCodeGenNameInteger ("_UID", GenericDeviceInfo->Uid, DeviceNode, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ERROR: SSDT-GENERIC-DEVICE: Failed to create UID node - %r\n", Status));
    ASSERT (0);
    goto exit_handler;
  }

  if (GenericDeviceInfo->HrvValid) {
    Status = AmlCodeGenNameInteger (
               "_HRV",
               GenericDeviceInfo->Hrv,
               DeviceNode,
               NULL
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "ERROR: SSDT-GENERIC-DEVICE: Failed to create UID node - %r\n", Status));
      ASSERT (0);
      goto exit_handler;
    }
  }

  Status = AmlCodeGenNameInteger ("_CCA", GenericDeviceInfo->Cca, DeviceNode, NULL);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto exit_handler;
  }

  if ((GenericDeviceInfo->AddressResourceToken != CM_NULL_TOKEN) ||
      (GenericDeviceInfo->InterruptResourceToken != CM_NULL_TOKEN))
  {
    Status = AmlCodeGenNameResourceTemplate ("_CRS", DeviceNode, &CrsNode);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "ERROR: SSDT-GENERIC-DEVICE: Failed to create CRS node - %r\n", Status));
      ASSERT (0);
      goto exit_handler;
    }

    if (GenericDeviceInfo->AddressResourceToken != CM_NULL_TOKEN) {
      Status = GetEArchCommonObjMemoryRangeDescriptor (
                 CfgMgrProtocol,
                 GenericDeviceInfo->AddressResourceToken,
                 &MemoryRanges,
                 &MemoryRangesCount
                 );
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "ERROR: SSDT-GENERIC-DEVICE: Failed to get memory ranges - %r\n", Status));
        ASSERT (0);
        goto exit_handler;
      }

      for (Index = 0; Index < MemoryRangesCount; Index++) {
        Status = AmlCodeGenRdQWordMemory (
                   TRUE,
                   TRUE,
                   FALSE,
                   FALSE,
                   0,
                   TRUE,
                   0,
                   MemoryRanges[Index].BaseAddress,
                   MemoryRanges[Index].BaseAddress + MemoryRanges[Index].Length - 1,
                   0,
                   MemoryRanges[Index].Length,
                   0,
                   NULL,
                   0,
                   TRUE,
                   CrsNode,
                   NULL
                   );
        if (EFI_ERROR (Status)) {
          DEBUG ((DEBUG_ERROR, "ERROR: SSDT-GENERIC-DEVICE: Failed to create memory resource node - %r\n", Status));
          ASSERT (0);
          goto exit_handler;
        }
      }
    }

    if (GenericDeviceInfo->InterruptResourceToken != CM_NULL_TOKEN) {
      Status = GetEArchCommonObjGenericInterrupt (
                 CfgMgrProtocol,
                 GenericDeviceInfo->InterruptResourceToken,
                 &Interrupts,
                 &InterruptsCount
                 );
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "ERROR: SSDT-GENERIC-DEVICE: Failed to get interrupts - %r\n", Status));
        ASSERT (0);
        goto exit_handler;
      }

      for (Index = 0; Index < InterruptsCount; Index++) {
        EdgeTriggered = ((Interrupts[Index].Flags & BIT0) == BIT0);
        ActiveLow     = ((Interrupts[Index].Flags & BIT1) == BIT1);

        Status = AmlCodeGenRdInterrupt (
                   TRUE,
                   EdgeTriggered,
                   ActiveLow,
                   FALSE,
                   &Interrupts[Index].Interrupt,
                   1,
                   CrsNode,
                   NULL
                   );
        if (EFI_ERROR (Status)) {
          DEBUG ((DEBUG_ERROR, "ERROR: SSDT-GENERIC-DEVICE: Failed to create interrupt node - %r\n", Status));
          ASSERT (0);
          goto exit_handler;
        }
      }
    }
  }

exit_handler:
  if (EFI_ERROR (Status) && DeviceAttached) {
    AmlDetachNode (DeviceNode);
    AmlDeleteTree (DeviceNode);
  }

  return Status;
}

/** Construct SSDT tables describing generic devices.

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
BuildSsdtGenericDeviceTable (
  IN  CONST ACPI_TABLE_GENERATOR                  *CONST  This,
  IN  CONST CM_STD_OBJ_ACPI_TABLE_INFO            *CONST  AcpiTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  OUT       EFI_ACPI_DESCRIPTION_HEADER          **CONST  Table
  )
{
  EFI_STATUS                          Status;
  CM_ARCH_COMMON_GENERIC_DEVICE_INFO  *GenericDeviceInfo;
  UINT32                              GenericDeviceCount;
  UINTN                               Index;
  AML_ROOT_NODE_HANDLE                RootNode;
  AML_OBJECT_NODE_HANDLE              ScopeNode;

  ASSERT (This != NULL);
  ASSERT (AcpiTableInfo != NULL);
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (Table != NULL);
  ASSERT (AcpiTableInfo->TableGeneratorId == This->GeneratorID);
  ASSERT (AcpiTableInfo->AcpiTableSignature == This->AcpiTableSignature);

  *Table = NULL;

  Status = GetEArchCommonObjGenericDeviceInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &GenericDeviceInfo,
             &GenericDeviceCount
             );
  if (Status == EFI_NOT_FOUND) {
    DEBUG ((
      DEBUG_ERROR,
      "WARNING: SSDT-GENERIC-DEVICE: No generic device information present."
      " Status = %r\n",
      Status
      ));
    return Status;
  } else if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-GENERIC-DEVICE: Failed to get generic device information."
      " Status = %r\n",
      Status
      ));
    return Status;
  }

  Status = AddSsdtAcpiHeader (
             CfgMgrProtocol,
             This,
             AcpiTableInfo,
             &RootNode
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = AmlCodeGenScope (SB_SCOPE, RootNode, &ScopeNode);
  if (EFI_ERROR (Status)) {
    goto exit_handler;
  }

  for (Index = 0; Index < GenericDeviceCount; Index++) {
    Status = AddGenericDeviceToSsdt (CfgMgrProtocol, ScopeNode, &GenericDeviceInfo[Index]);
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-GENERIC-DEVICE: Failed to add generic device to SSDT."
        " Status = %r\n",
        Status
        ));
      goto exit_handler;
    }
  }

  Status = AmlSerializeDefinitionBlock (
             RootNode,
             Table
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-GENERIC-DEVICE: Failed to Serialize SSDT Table Data."
      " Status = %r\n",
      Status
      ));
    goto exit_handler;
  }

exit_handler:
  // Delete the RootNode and its attached children.
  AmlDeleteTree (RootNode);
  return Status;
}

/** This macro defines the SSDT Generic Device Table Generator revision.
*/
#define SSDT_GENERIC_DEVICE_GENERATOR_REVISION  CREATE_REVISION (1, 0)

/** The interface for the SSDT Generic Device Table Generator.
*/
STATIC
CONST
ACPI_TABLE_GENERATOR  SsdtGenericDeviceGenerator = {
  // Generator ID
  CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdSsdtGenericDevice),
  // Generator Description
  L"ACPI.STD.SSDT.GENERIC.DEVICE.GENERATOR",
  // ACPI Table Signature
  EFI_ACPI_6_4_SECONDARY_SYSTEM_DESCRIPTION_TABLE_SIGNATURE,
  // ACPI Table Revision - Unused
  0,
  // Minimum ACPI Table Revision - Unused
  0,
  // Creator ID
  TABLE_GENERATOR_CREATOR_ID_ARM,
  // Creator Revision
  SSDT_GENERIC_DEVICE_GENERATOR_REVISION,
  // Build Table function
  BuildSsdtGenericDeviceTable,
  // Free Resource function
  FreeSsdtGenericDeviceTable,
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
AcpiSsdtGenericDeviceLibConstructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = RegisterAcpiTableGenerator (&SsdtGenericDeviceGenerator);
  DEBUG ((
    DEBUG_INFO,
    "SSDT-GENERIC-DEVICE: Register Generator. Status = %r\n",
    Status
    ));
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
AcpiSsdtGenericDeviceLibDestructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = DeregisterAcpiTableGenerator (&SsdtGenericDeviceGenerator);
  DEBUG ((
    DEBUG_INFO,
    "SSDT-GENERIC-DEVICE: Deregister Generator. Status = %r\n",
    Status
    ));
  ASSERT_EFI_ERROR (Status);
  return Status;
}
