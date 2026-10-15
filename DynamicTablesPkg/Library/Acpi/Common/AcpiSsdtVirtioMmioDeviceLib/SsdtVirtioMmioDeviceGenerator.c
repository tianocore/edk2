/** @file
  SSDT Virtio MMIO Device AML Table Generator.

  Copyright (c) 2026, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - ACPI Specification Version 6.5
    https://uefi.org/specs/ACPI/6.5/

  - Linux Virtio MMIO Driver
    https://github.com/torvalds/linux/blob/master/drivers/virtio/virtio_mmio.c

**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>

#include <AcpiTableGenerator.h>
#include <ConfigurationManagerHelper.h>
#include <ConfigurationManagerObject.h>
#include <Library/AcpiHelperLib.h>
#include <Library/AmlLib/AmlLib.h>
#include <Library/TableHelperLib.h>
#include <Protocol/ConfigurationManagerProtocol.h>

#define VIRTIO_MMIO_DEVICE_HID   "LNRO0005"
#define MAX_VIRTIO_MMIO_DEVICES  0x1000
#define SB_SCOPE                 "\\_SB_"

GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjVirtioMmioDeviceInfo,
  CM_ARCH_COMMON_VIRTIO_MMIO_DEVICE_INFO
  );

/** Validate the Virtio MMIO device register ranges and interrupts.

  @param [in]       VirtioMmioDevices         Pointer to the Virtio MMIO device information array.
  @param [in]       VirtioMmioDeviceCount     Number of device information entries.

  @retval EFI_SUCCESS             All device information entries are valid.
  @retval EFI_INVALID_PARAMETER   A device pointer, register range or interrupt is invalid.
**/
STATIC
EFI_STATUS
ValidateVirtioMmioDeviceInfo (
  IN CONST CM_ARCH_COMMON_VIRTIO_MMIO_DEVICE_INFO  *VirtioMmioDevices,
  IN       UINT32                                  VirtioMmioDeviceCount
  )
{
  CONST CM_ARCH_COMMON_VIRTIO_MMIO_DEVICE_INFO  *VirtioMmioDeviceInfo;
  UINT32                                        Index;

  if ((VirtioMmioDevices == NULL) || (VirtioMmioDeviceCount == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  for (Index = 0; Index < VirtioMmioDeviceCount; Index++) {
    VirtioMmioDeviceInfo = &VirtioMmioDevices[Index];

    if ((VirtioMmioDeviceInfo->BaseAddress == 0) ||
        (VirtioMmioDeviceInfo->BaseAddressLength == 0) ||
        (VirtioMmioDeviceInfo->BaseAddress >
         MAX_UINT64 - (VirtioMmioDeviceInfo->BaseAddressLength - 1)))
    {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-VIRTIO-MMIO: Invalid MMIO range at index %u.\n",
        Index
        ));
      return EFI_INVALID_PARAMETER;
    }

    if ((VirtioMmioDeviceInfo->Interrupt.Interrupt == 0) ||
        ((VirtioMmioDeviceInfo->Interrupt.Flags & ~(BIT0 | BIT1)) != 0))
    {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-VIRTIO-MMIO: Invalid interrupt at index %u.\n",
        Index
        ));
      return EFI_INVALID_PARAMETER;
    }
  }

  return EFI_SUCCESS;
}

/** Create the _CRS resource template for a Virtio MMIO device.

  @param [in]       VirtioMmioDeviceInfo      Pointer to the Virtio MMIO device information.
  @param [in]       DeviceNode                AML device node to receive the resource template.

  @retval EFI_SUCCESS             The memory and interrupt resources were created.
  @return Other errors from the Configuration Manager or AML library.
**/
STATIC
EFI_STATUS
CreateVirtioMmioDeviceCrs (
  IN CONST CM_ARCH_COMMON_VIRTIO_MMIO_DEVICE_INFO  *VirtioMmioDeviceInfo,
  IN       AML_OBJECT_NODE_HANDLE                  DeviceNode
  )
{
  AML_OBJECT_NODE_HANDLE  CrsNode;
  EFI_STATUS              Status;
  UINT32                  Interrupt;
  UINT64                  AddressMaximum;

  // ASL: Name (_CRS, ResourceTemplate () {})
  Status = AmlCodeGenNameResourceTemplate ("_CRS", DeviceNode, &CrsNode);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-VIRTIO-MMIO: Failed to create AML _CRS node."
      " Status = %r\n",
      Status
      ));
    return Status;
  }

  // The complete range and length must fit the selected descriptor.
  AddressMaximum = VirtioMmioDeviceInfo->BaseAddress +
                   (VirtioMmioDeviceInfo->BaseAddressLength - 1);
  if ((AddressMaximum <= MAX_UINT32) &&
      (VirtioMmioDeviceInfo->BaseAddressLength <= MAX_UINT32))
  {
    Status = AmlCodeGenRdMemory32Fixed (
               TRUE,
               (UINT32)VirtioMmioDeviceInfo->BaseAddress,
               (UINT32)VirtioMmioDeviceInfo->BaseAddressLength,
               CrsNode,
               NULL
               );
  } else {
    Status = AmlCodeGenRdQWordMemory (
               TRUE, // ResourceConsumer
               TRUE, // PosDecode
               TRUE, // MinFixed
               TRUE, // MaxFixed
               AmlMemoryNonCacheable,
               TRUE, // ReadWrite
               0,    // AddressGranularity
               VirtioMmioDeviceInfo->BaseAddress,
               AddressMaximum,
               0,    // AddressTranslation
               VirtioMmioDeviceInfo->BaseAddressLength,
               0,
               NULL,
               AmlAddressRangeMemory,
               TRUE, // TypeStatic
               CrsNode,
               NULL
               );
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-VIRTIO-MMIO: Failed to create MMIO resource."
      " Status = %r\n",
      Status
      ));
    return Status;
  }

  Interrupt = VirtioMmioDeviceInfo->Interrupt.Interrupt;

  // ASL: Interrupt (ResourceConsumer, <Mode>, <Polarity>, Exclusive) {}
  Status = AmlCodeGenRdInterrupt (
             TRUE,
             ((VirtioMmioDeviceInfo->Interrupt.Flags & BIT0) != 0),
             ((VirtioMmioDeviceInfo->Interrupt.Flags & BIT1) != 0),
             FALSE,
             &Interrupt,
             1,
             CrsNode,
             NULL
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-VIRTIO-MMIO: Failed to create interrupt resource."
      " Status = %r\n",
      Status
      ));
  }

  return Status;
}

/** Build an SSDT describing a Virtio MMIO device.

  The caller owns the generated table and must release it with the
  registered free callback.

  @param [in]       This                      Pointer to the ACPI table generator.
  @param [in]       AcpiTableInfo             Pointer to the ACPI table information.
  @param [in]       CfgMgrProtocol            Pointer to the Configuration Manager protocol.
  @param [in]       VirtioMmioDeviceInfo      Pointer to the Virtio MMIO device information.
  @param [in]       DeviceIndex               Device index used to construct the AML name and UID.
  @param [out]      Table                     Pointer to the generated table.

  @retval EFI_SUCCESS             The requested SSDT was generated.
  @retval EFI_INVALID_PARAMETER   A parameter or device configuration is invalid.
  @retval EFI_OUT_OF_RESOURCES    Memory allocation failed.
  @return Other errors from the Configuration Manager or AML library.
**/
STATIC
EFI_STATUS
BuildSsdtVirtioMmioDeviceTable (
  IN  CONST ACPI_TABLE_GENERATOR                                 *This,
  IN  CONST CM_STD_OBJ_ACPI_TABLE_INFO           *CONST          AcpiTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL *CONST          CfgMgrProtocol,
  IN  CONST CM_ARCH_COMMON_VIRTIO_MMIO_DEVICE_INFO       *CONST  VirtioMmioDeviceInfo,
  IN        UINT32                                               DeviceIndex,
  OUT       EFI_ACPI_DESCRIPTION_HEADER                          **Table
  )
{
  EFI_STATUS              Status;
  EFI_STATUS              CleanupStatus;
  AML_OBJECT_NODE_HANDLE  DeviceNode;
  AML_OBJECT_NODE_HANDLE  ScopeNode;
  AML_ROOT_NODE_HANDLE    RootNode;
  CHAR8                   AslName[AML_NAME_SEG_SIZE + 1];

  RootNode   = NULL;
  ScopeNode  = NULL;
  DeviceNode = NULL;
  *Table     = NULL;

  Status = AddSsdtAcpiHeader (
             CfgMgrProtocol,
             This,
             AcpiTableInfo,
             &RootNode
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // ASL: Scope (\_SB) {}
  Status = AmlCodeGenScope (SB_SCOPE, RootNode, &ScopeNode);
  if (EFI_ERROR (Status)) {
    goto Cleanup;
  }

  // ASL: Device (Vxxx) {}
  CopyMem (AslName, "V000", sizeof (AslName));
  AslName[3] = AsciiFromHex (DeviceIndex & 0xF);
  AslName[2] = AsciiFromHex ((DeviceIndex >> 4) & 0xF);
  AslName[1] = AsciiFromHex ((DeviceIndex >> 8) & 0xF);

  Status = AmlCodeGenDevice (AslName, ScopeNode, &DeviceNode);
  if (EFI_ERROR (Status)) {
    goto Cleanup;
  }

  // ASL: Name (_HID, "LNRO0005")
  Status = AmlCodeGenNameString (
             "_HID",
             VIRTIO_MMIO_DEVICE_HID,
             DeviceNode,
             NULL
             );
  if (EFI_ERROR (Status)) {
    goto Cleanup;
  }

  // ASL: Name (_UID, <DeviceIndex>)
  Status = AmlCodeGenNameInteger (
             "_UID",
             DeviceIndex,
             DeviceNode,
             NULL
             );
  if (EFI_ERROR (Status)) {
    goto Cleanup;
  }

  Status = CreateVirtioMmioDeviceCrs (VirtioMmioDeviceInfo, DeviceNode);
  if (EFI_ERROR (Status)) {
    goto Cleanup;
  }

  Status = AmlSerializeDefinitionBlock (RootNode, Table);

Cleanup:
  CleanupStatus = AmlDeleteTree (RootNode);
  if (EFI_ERROR (CleanupStatus)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-VIRTIO-MMIO: Failed to delete AML tree."
      " Status = %r\n",
      CleanupStatus
      ));
  }

  return Status;
}

/** Free resources for the generated Virtio MMIO SSDTs.

  An empty output after build failure requires no cleanup and succeeds.

  @param [in]       This                      Pointer to the ACPI table generator.
  @param [in]       AcpiTableInfo             Pointer to the ACPI table information.
  @param [in]       CfgMgrProtocol            Pointer to the Configuration Manager protocol.
  @param [in, out]  Table                     Pointer to the generated table list.
  @param [in]       TableCount                Number of generated ACPI tables.

  @retval EFI_SUCCESS             The resources were freed, or the output was empty.
  @retval EFI_INVALID_PARAMETER   An argument or table signature is invalid.
**/
STATIC
EFI_STATUS
EFIAPI
FreeSsdtVirtioMmioDeviceTableEx (
  IN      CONST ACPI_TABLE_GENERATOR                   *CONST  This,
  IN      CONST CM_STD_OBJ_ACPI_TABLE_INFO             *CONST  AcpiTableInfo,
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL   *CONST  CfgMgrProtocol,
  IN OUT        EFI_ACPI_DESCRIPTION_HEADER          ***CONST  Table,
  IN      CONST UINTN                                          TableCount
  )
{
  EFI_ACPI_DESCRIPTION_HEADER  **TableList;
  UINTN                        Index;

  if ((This == NULL) || (AcpiTableInfo == NULL) || (CfgMgrProtocol == NULL) ||
      (Table == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  if ((AcpiTableInfo->TableGeneratorId != This->GeneratorID) ||
      (AcpiTableInfo->AcpiTableSignature != This->AcpiTableSignature))
  {
    return EFI_INVALID_PARAMETER;
  }

  // The framework also invokes this callback after build failure.
  if (*Table == NULL) {
    return (TableCount == 0) ? EFI_SUCCESS : EFI_INVALID_PARAMETER;
  }

  TableList = *Table;
  for (Index = 0; Index < TableCount; Index++) {
    if ((TableList[Index] == NULL) ||
        (TableList[Index]->Signature != EFI_ACPI_6_3_SECONDARY_SYSTEM_DESCRIPTION_TABLE_SIGNATURE))
    {
      return EFI_INVALID_PARAMETER;
    }
  }

  for (Index = 0; Index < TableCount; Index++) {
    FreePool (TableList[Index]);
  }

  FreePool (TableList);
  *Table = NULL;
  return EFI_SUCCESS;
}

/** Build SSDTs describing Virtio MMIO devices.

  On failure, release allocated tables and leave Table NULL and
  TableCount zero.

  @param [in]       This                      Pointer to the ACPI table generator.
  @param [in]       AcpiTableInfo             Pointer to the ACPI table information.
  @param [in]       CfgMgrProtocol            Pointer to the Configuration Manager protocol.
  @param [out]      Table                     Pointer to the generated table list.
  @param [out]      TableCount                Number of tables returned; zero on failure.

  @retval EFI_SUCCESS             The requested SSDTs were generated.
  @retval EFI_INVALID_PARAMETER   A parameter or device configuration is invalid.
  @retval EFI_OUT_OF_RESOURCES    Memory allocation failed.
  @return Other errors from the Configuration Manager or AML library.
**/
STATIC
EFI_STATUS
EFIAPI
BuildSsdtVirtioMmioDeviceTableEx (
  IN  CONST ACPI_TABLE_GENERATOR                         *This,
  IN  CONST CM_STD_OBJ_ACPI_TABLE_INFO           *CONST  AcpiTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL *CONST  CfgMgrProtocol,
  OUT       EFI_ACPI_DESCRIPTION_HEADER                  ***Table,
  OUT       UINTN                                *CONST  TableCount
  )
{
  EFI_STATUS                              Status;
  CM_ARCH_COMMON_VIRTIO_MMIO_DEVICE_INFO  *VirtioMmioDeviceInfo;
  UINT32                                  VirtioMmioDeviceCount;
  UINT32                                  Index;
  UINT32                                  CleanupIndex;
  EFI_ACPI_DESCRIPTION_HEADER             **TableList;

  if ((This == NULL) || (AcpiTableInfo == NULL) || (CfgMgrProtocol == NULL) ||
      (Table == NULL) || (TableCount == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  if ((AcpiTableInfo->TableGeneratorId != This->GeneratorID) ||
      (AcpiTableInfo->AcpiTableSignature != This->AcpiTableSignature))
  {
    return EFI_INVALID_PARAMETER;
  }

  *Table      = NULL;
  *TableCount = 0;

  Status = GetEArchCommonObjVirtioMmioDeviceInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &VirtioMmioDeviceInfo,
             &VirtioMmioDeviceCount
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-VIRTIO-MMIO: Failed to get Virtio MMIO Device information."
      " Status = %r\n",
      Status
      ));
    return Status;
  }

  if (VirtioMmioDeviceCount > MAX_VIRTIO_MMIO_DEVICES) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-VIRTIO-MMIO: Too many Virtio MMIO devices: %u."
      " Maximum supported = %u.\n",
      VirtioMmioDeviceCount,
      MAX_VIRTIO_MMIO_DEVICES
      ));
    return EFI_INVALID_PARAMETER;
  }

  Status = ValidateVirtioMmioDeviceInfo (VirtioMmioDeviceInfo, VirtioMmioDeviceCount);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-VIRTIO-MMIO: Invalid Virtio MMIO Device information."
      " Status = %r\n",
      Status
      ));
    return Status;
  }

  // Allocate a table to store pointers to the SSDT tables.
  TableList = (EFI_ACPI_DESCRIPTION_HEADER **)
              AllocateZeroPool (
                sizeof (EFI_ACPI_DESCRIPTION_HEADER *) * VirtioMmioDeviceCount
                );
  if (TableList == NULL) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-VIRTIO-MMIO: Failed to allocate table list.\n"
      ));
    return EFI_OUT_OF_RESOURCES;
  }

  for (Index = 0; Index < VirtioMmioDeviceCount; Index++) {
    Status = BuildSsdtVirtioMmioDeviceTable (
               This,
               AcpiTableInfo,
               CfgMgrProtocol,
               &VirtioMmioDeviceInfo[Index],
               Index,
               &TableList[Index]
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-VIRTIO-MMIO: Failed to build SSDT for device %u."
        " Status = %r\n",
        Index,
        Status
        ));
      // Keep ownership local until every SSDT has been built.
      // Include the failing slot in case serialization allocated a table.
      for (CleanupIndex = 0; CleanupIndex <= Index; CleanupIndex++) {
        if (TableList[CleanupIndex] != NULL) {
          FreePool (TableList[CleanupIndex]);
        }
      }

      FreePool (TableList);
      return Status;
    }
  }

  *Table      = TableList;
  *TableCount = VirtioMmioDeviceCount;
  return EFI_SUCCESS;
}

#define SSDT_VIRTIO_MMIO_DEVICE_GENERATOR_REVISION  CREATE_REVISION (1, 0)

STATIC
CONST
ACPI_TABLE_GENERATOR  SsdtVirtioMmioDeviceGenerator = {
  CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdSsdtVirtioMmioDevice),
  L"ACPI.STD.SSDT.VIRTIO.MMIO.DEVICE.GENERATOR",
  EFI_ACPI_6_3_SECONDARY_SYSTEM_DESCRIPTION_TABLE_SIGNATURE,
  0,
  0,
  TABLE_GENERATOR_CREATOR_ID,
  SSDT_VIRTIO_MMIO_DEVICE_GENERATOR_REVISION,
  NULL,
  NULL,
  BuildSsdtVirtioMmioDeviceTableEx,
  FreeSsdtVirtioMmioDeviceTableEx
};

/** Register the Virtio MMIO SSDT generator with the ACPI table factory.

  @param [in]       ImageHandle               Handle of the image containing this library.
  @param [in]       SystemTable               Pointer to the UEFI system table.

  @retval EFI_SUCCESS             The generator was registered.
  @retval EFI_INVALID_PARAMETER   The generator identifier is invalid.
  @retval EFI_ALREADY_STARTED     The generator is already registered.
**/
EFI_STATUS
EFIAPI
AcpiSsdtVirtioMmioDeviceLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = RegisterAcpiTableGenerator (&SsdtVirtioMmioDeviceGenerator);
  DEBUG ((DEBUG_INFO, "SSDT-VIRTIO-MMIO: Register Generator. Status = %r\n", Status));
  ASSERT_EFI_ERROR (Status);
  return Status;
}

/** Deregister the Virtio MMIO SSDT generator from the ACPI table factory.

  @param [in]       ImageHandle               Handle of the image containing this library.
  @param [in]       SystemTable               Pointer to the UEFI system table.

  @retval EFI_SUCCESS             The generator was deregistered.
  @retval EFI_INVALID_PARAMETER   The generator identifier is invalid.
  @retval EFI_NOT_FOUND           The generator is not registered.
**/
EFI_STATUS
EFIAPI
AcpiSsdtVirtioMmioDeviceLibDestructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = DeregisterAcpiTableGenerator (&SsdtVirtioMmioDeviceGenerator);
  DEBUG ((DEBUG_INFO, "SSDT-VIRTIO-MMIO: Deregister Generator. Status = %r\n", Status));
  ASSERT_EFI_ERROR (Status);
  return Status;
}
