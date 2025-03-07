/** @file
  DBG2 Table Generator

  Copyright (c) 2017 - 2022, Arm Limited. All rights reserved.<BR>
  Copyright (c) 2024 - 2025, NVIDIA CORPORATION & AFFILIATES. All rights reserved. <BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - Microsoft Debug Port Table 2 (DBG2) Specification - December 10, 2015.

**/

#include <IndustryStandard/AcpiAml.h>
#include <IndustryStandard/DebugPort2Table.h>
#include <Library/AcpiLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Protocol/AcpiTable.h>
#include <Protocol/SerialIo.h>

// Module specific include files.
#include <AcpiTableGenerator.h>
#include <ConfigurationManagerObject.h>
#include <ConfigurationManagerHelper.h>
#include <Library/SsdtSerialPortFixupLib.h>
#include <Library/TableHelperLib.h>
#include <Protocol/ConfigurationManagerProtocol.h>

#include "Dbg2Generator.h"

/** ARM standard DBG2 Table Generator

  Constructs the DBG2 table for corresponding DBG2 peripheral.

Requirements:
  The following Configuration Manager Object(s) are required by
  this Generator:
  - EArchCommonObjSerialDebugPortInfo
  - EArchCommonObjGenericDbg2DeviceInfo
*/

/** A string representing the name of the serial debug port 0.
*/
#define NAME_STR_DBG_PORT0  "COM0"

// _SB scope of the AML namespace.
#define SB_SCOPE  "\\_SB_."

/** An UID representing the serial debug port 0.
*/
#define UID_DBG_PORT0  0

/** The length of the namespace string.
*/
#define DBG2_NAMESPACESTRING_FIELD_SIZE  (sizeof (SB_SCOPE) + AML_NAME_SEG_SIZE)

/** This macro expands to a function that retrieves the Serial
    debug port information from the Configuration Manager
*/
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjSerialDebugPortInfo,
  CM_ARCH_COMMON_SERIAL_PORT_INFO
  );

/** This macro expands to a function that retrieves the DBG2
    device information from the Configuration Manager
*/
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjGenericDbg2DeviceInfo,
  CM_ARCH_COMMON_DBG2_DEVICE_INFO
  );

/** This macro expands to a function that retrieves the
    Memory Range Descriptor Array information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjMemoryRangeDescriptor,
  CM_ARCH_COMMON_MEMORY_RANGE_DESCRIPTOR
  );

/** Initialize the DBG2 UART with the parameters obtained from
    the Configuration Manager.

  @param [in]  SerialPortInfo Pointer to the Serial Port Information.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER The parameters for serial port initialization
                                are invalid.
**/
STATIC
EFI_STATUS
SetupDebugUart (
  IN  CONST CM_ARCH_COMMON_SERIAL_PORT_INFO  *CONST  SerialPortInfo
  )
{
  EFI_STATUS          Status;
  UINT64              BaudRate;
  UINT32              ReceiveFifoDepth;
  EFI_PARITY_TYPE     Parity;
  UINT8               DataBits;
  EFI_STOP_BITS_TYPE  StopBits;

  ASSERT (SerialPortInfo != NULL);

  // Initialize the Serial Debug UART
  DEBUG ((DEBUG_INFO, "Initializing Serial Debug UART...\n"));
  ReceiveFifoDepth = 0; // Use the default value for FIFO depth
  Parity           = (EFI_PARITY_TYPE)FixedPcdGet8 (PcdUartDefaultParity);
  DataBits         = FixedPcdGet8 (PcdUartDefaultDataBits);
  StopBits         = (EFI_STOP_BITS_TYPE)FixedPcdGet8 (PcdUartDefaultStopBits);

  BaudRate = SerialPortInfo->BaudRate;
  Status   = Dbg2InitializePort (
               SerialPortInfo,
               &BaudRate,
               &ReceiveFifoDepth,
               &Parity,
               &DataBits,
               &StopBits
               );

  ASSERT_EFI_ERROR (Status);
  return Status;
}

/** Free any resources allocated for constructing the tables.

  @param [in]      This           Pointer to the ACPI table generator.
  @param [in]      AcpiTableInfo  Pointer to the ACPI Table Info.
  @param [in]      CfgMgrProtocol Pointer to the Configuration Manager
                                  Protocol Interface.
  @param [in, out] Table          Pointer to an array of pointers
                                  to ACPI Table(s).
  @param [in]      TableCount     Number of ACPI table(s).

  @retval EFI_SUCCESS           The resources were freed successfully.
  @retval EFI_INVALID_PARAMETER The table pointer is NULL or invalid.
**/
STATIC
EFI_STATUS
EFIAPI
FreeDbg2TableEx (
  IN      CONST ACPI_TABLE_GENERATOR                   *CONST  This,
  IN      CONST CM_STD_OBJ_ACPI_TABLE_INFO             *CONST  AcpiTableInfo,
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL   *CONST  CfgMgrProtocol,
  IN OUT        EFI_ACPI_DESCRIPTION_HEADER          ***CONST  Table,
  IN      CONST UINTN                                          TableCount
  )
{
  EFI_STATUS                   Status;
  UINTN                        Index;
  EFI_ACPI_DESCRIPTION_HEADER  **TableList;

  ASSERT (This != NULL);
  ASSERT (AcpiTableInfo != NULL);
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (AcpiTableInfo->TableGeneratorId == This->GeneratorID);
  ASSERT (AcpiTableInfo->AcpiTableSignature == This->AcpiTableSignature);

  if ((Table == NULL)   ||
      (*Table == NULL))
  {
    DEBUG ((DEBUG_ERROR, "ERROR: DBG2: Invalid Table Pointer\n"));
    return EFI_INVALID_PARAMETER;
  }

  TableList = *Table;

  // Free the DBG2 table
  if (TableCount != 0) {
    FreePool (TableList[0]);
  }

  // Free the SSDT tables if any exist
  for (Index = 1; Index < TableCount; Index++) {
    if ((TableList[Index] == NULL) ||
        (TableList[Index]->Signature !=
         EFI_ACPI_6_3_SECONDARY_SYSTEM_DESCRIPTION_TABLE_SIGNATURE))
    {
      DEBUG ((DEBUG_ERROR, "ERROR: DBG2: Invalid SSDT table pointer.\n"));
      return EFI_INVALID_PARAMETER;
    }

    Status = FreeSsdtSerialPortTable (TableList[Index]);
    ASSERT_EFI_ERROR (Status);
  }

  // Free the table list.
  FreePool (*Table);

  return Status;
}

/** Populates the DBG2 device info structure.

  @param [in, out] AcpiDbg2Device     Pointer to the DBG2 ACPI table to add device to.
                                      Pointer will be updated to point to after the new DBG2 device.
  @param [in]      DeviceInfo         Pointer to the Device Info structure.
  @param [in]      MemoryRanges       The memory ranges of the device.
  @param [in]      MemoryRangesCount  The number of memory ranges in the device.

  @retval EFI_SUCCESS           The structure was populated correctly.
  @retval EFI_INVALID_PARAMETER The parameters are invalid.
  @retval EFI_BUFFER_TOO_SMALL  The namespace string is too long.
**/
STATIC
EFI_STATUS
EFIAPI
PopulateDbg2Device (
  IN OUT EFI_ACPI_DBG2_DEBUG_DEVICE_INFORMATION_STRUCT  **AcpiDbg2Device,
  IN CM_ARCH_COMMON_DBG2_DEVICE_INFO                    *DeviceInfo,
  IN CM_ARCH_COMMON_MEMORY_RANGE_DESCRIPTOR             *MemoryRanges,
  IN UINT32                                             MemoryRangesCount
  )
{
  UINTN                                          Index;
  UINT16                                         Dbg2DeviceSize;
  EFI_ACPI_DBG2_DEBUG_DEVICE_INFORMATION_STRUCT  *CurrentDbg2Device;
  EFI_ACPI_6_3_GENERIC_ADDRESS_STRUCTURE         *BaseAddressRegister;
  UINT32                                         *AddressSize;
  UINT64                                         TotalSize;
  CHAR8                                          *NamespaceString;

  // Parameter validation
  if ((AcpiDbg2Device == NULL) || (DeviceInfo == NULL) || (MemoryRanges == NULL)) {
    DEBUG ((DEBUG_ERROR, "ERROR: DBG2: Invalid parameters to PopulateDbg2Device\n"));
    return EFI_INVALID_PARAMETER;
  }

  if (MemoryRangesCount == 0) {
    DEBUG ((DEBUG_ERROR, "ERROR: DBG2: Memory ranges count cannot be zero\n"));
    return EFI_INVALID_PARAMETER;
  }

  // Check string length before concatenation
  if ((DeviceInfo->ObjectName[0] != '\0') &&
      (AsciiStrLen (SB_SCOPE) + AsciiStrLen (DeviceInfo->ObjectName) >= DBG2_NAMESPACESTRING_FIELD_SIZE))
  {
    DEBUG ((DEBUG_ERROR, "ERROR: DBG2: Namespace string too long\n"));
    return EFI_BUFFER_TOO_SMALL;
  }

  if (MemoryRangesCount > MAX_UINT8) {
    DEBUG ((DEBUG_ERROR, "ERROR: DBG2: Too many memory ranges. Count = %u\n", MemoryRangesCount));
    return EFI_INVALID_PARAMETER;
  }

  // Validate all memory ranges
  for (Index = 0; Index < MemoryRangesCount; Index++) {
    if (MemoryRanges[Index].BaseAddress == 0) {
      DEBUG ((DEBUG_ERROR, "ERROR: DBG2: Memory range base address is 0. Index = %u\n", Index));
      return EFI_INVALID_PARAMETER;
    }

    if (MemoryRanges[Index].Length == 0) {
      DEBUG ((DEBUG_ERROR, "ERROR: DBG2: Memory range length is 0. Index = %u\n", Index));
      return EFI_INVALID_PARAMETER;
    }

    if (MemoryRanges[Index].Length > MAX_UINT32) {
      DEBUG ((DEBUG_ERROR, "ERROR: DBG2: Memory range length too large. Length = %u\n", MemoryRanges[Index].Length));
      return EFI_INVALID_PARAMETER;
    }
  }

  // Calculate total size with overflow check
  TotalSize = (UINT64)sizeof (EFI_ACPI_DBG2_DEBUG_DEVICE_INFORMATION_STRUCT) +
              ((UINT64)sizeof (EFI_ACPI_6_3_GENERIC_ADDRESS_STRUCTURE) + sizeof (UINT32)) * MemoryRangesCount +
              (UINT64)sizeof (CHAR8) * DBG2_NAMESPACESTRING_FIELD_SIZE;

  if (TotalSize > MAX_UINT16) {
    DEBUG ((DEBUG_ERROR, "ERROR: DBG2: Device size too large\n"));
    return EFI_INVALID_PARAMETER;
  }

  Dbg2DeviceSize    = (UINT16)TotalSize;
  CurrentDbg2Device = *AcpiDbg2Device;

  // Initialize device structure
  CurrentDbg2Device->Revision                        = EFI_ACPI_DBG2_DEBUG_DEVICE_INFORMATION_STRUCT_REVISION;
  CurrentDbg2Device->Length                          = Dbg2DeviceSize;
  CurrentDbg2Device->NumberofGenericAddressRegisters = (UINT8)MemoryRangesCount;
  CurrentDbg2Device->OemDataLength                   = 0;
  CurrentDbg2Device->OemDataOffset                   = 0;
  CurrentDbg2Device->PortType                        = DeviceInfo->PortType;
  CurrentDbg2Device->PortSubtype                     = DeviceInfo->PortSubtype;
  CurrentDbg2Device->BaseAddressRegisterOffset       = sizeof (EFI_ACPI_DBG2_DEBUG_DEVICE_INFORMATION_STRUCT);
  CurrentDbg2Device->AddressSizeOffset               = CurrentDbg2Device->BaseAddressRegisterOffset +
                                                       ((UINT16)sizeof (EFI_ACPI_6_3_GENERIC_ADDRESS_STRUCTURE) * ((UINT8)MemoryRangesCount));
  CurrentDbg2Device->NameSpaceStringLength = DBG2_NAMESPACESTRING_FIELD_SIZE;
  CurrentDbg2Device->NameSpaceStringOffset = CurrentDbg2Device->AddressSizeOffset +
                                             ((UINT16)sizeof (UINT32) * ((UINT8)MemoryRangesCount));
  BaseAddressRegister = (EFI_ACPI_6_3_GENERIC_ADDRESS_STRUCTURE *)((UINT8 *)CurrentDbg2Device + CurrentDbg2Device->BaseAddressRegisterOffset);
  AddressSize         = (UINT32 *)((UINT8 *)CurrentDbg2Device + CurrentDbg2Device->AddressSizeOffset);
  for (Index = 0; Index < MemoryRangesCount; Index++) {
    BaseAddressRegister->AddressSpaceId    = EFI_ACPI_6_3_SYSTEM_MEMORY;
    BaseAddressRegister->RegisterBitWidth  = 32;
    BaseAddressRegister->RegisterBitOffset = 0;
    BaseAddressRegister->AccessSize        = DeviceInfo->AccessSize;
    BaseAddressRegister->Address           = MemoryRanges[Index].BaseAddress;
    *AddressSize                           = (UINT32)(MemoryRanges[Index].Length);
    BaseAddressRegister++;
    AddressSize++;
  }

  NamespaceString = (CHAR8 *)CurrentDbg2Device + CurrentDbg2Device->NameSpaceStringOffset;
  if (DeviceInfo->ObjectName[0] == '\0') {
    // If device string is empty then use "." as the name per the DBG2 specification.
    AsciiSPrint (
      NamespaceString,
      DBG2_NAMESPACESTRING_FIELD_SIZE,
      "."
      );
  } else {
    // Construct the namespace string for the device (e.g. \_SB_.COM1)
    AsciiSPrint (
      NamespaceString,
      DBG2_NAMESPACESTRING_FIELD_SIZE,
      "%a%a",
      SB_SCOPE,
      DeviceInfo->ObjectName
      );
  }

  // Update the pointer to point to the next device
  *AcpiDbg2Device = (EFI_ACPI_DBG2_DEBUG_DEVICE_INFORMATION_STRUCT *)((UINT8 *)CurrentDbg2Device + Dbg2DeviceSize);

  return EFI_SUCCESS;
}

/** Builds the serial port device entry.

  @param [in]       AcpiTableInfo   Pointer to the ACPI table information.
  @param [in, out]  AcpiDbg2Device  Pointer to the DBG2 ACPI table to add device to.
                                      Pointer will be updated to point to after the new DBG2 device.
  @param [in]       SerialPortInfo  Pointer to the serial port information.
  @param [in]       ObjectName      Pointer to the object name.
  @param [in]       Uid             The UID of the device.
  @param [out]      Table           Pointer to the table to add the device to.

  @retval EFI_SUCCESS           The serial port device was built successfully.
  @retval EFI_INVALID_PARAMETER The parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES  Could not allocate memory.
**/
STATIC
EFI_STATUS
EFIAPI
BuildDbg2SerialPortEntry  (
  IN  CONST CM_STD_OBJ_ACPI_TABLE_INFO             *CONST  AcpiTableInfo,
  IN OUT EFI_ACPI_DBG2_DEBUG_DEVICE_INFORMATION_STRUCT     **AcpiDbg2Device,
  IN  CONST CM_ARCH_COMMON_SERIAL_PORT_INFO                *SerialPortInfo,
  IN  CONST CHAR8                                          *ObjectName,
  IN  UINT32                                               Uid,
  OUT EFI_ACPI_DESCRIPTION_HEADER                          **Table
  )
{
  EFI_STATUS                              Status;
  CM_ARCH_COMMON_MEMORY_RANGE_DESCRIPTOR  SerialMemoryRange;
  CM_ARCH_COMMON_DBG2_DEVICE_INFO         SerialPortDeviceInfo;

  SerialMemoryRange.BaseAddress             = SerialPortInfo->BaseAddress;
  SerialMemoryRange.Length                  = SerialPortInfo->BaseAddressLength;
  SerialPortDeviceInfo.AddressResourceToken = CM_NULL_TOKEN;
  SerialPortDeviceInfo.PortType             = EFI_ACPI_DBG2_PORT_TYPE_SERIAL;
  SerialPortDeviceInfo.PortSubtype          = SerialPortInfo->PortSubtype;
  // Set the access size
  if (SerialPortInfo->AccessSize >= EFI_ACPI_6_3_QWORD) {
    Status = EFI_INVALID_PARAMETER;
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: DBG2: Access size must be <= 3 (DWORD). Status = %r\n",
      Status
      ));
    goto error_handler;
  } else if (SerialPortInfo->AccessSize == EFI_ACPI_6_3_UNDEFINED) {
    // 0 Undefined (legacy reasons)
    // Default to DWORD access size as the access
    // size field was introduced at a later date
    // and some ConfigurationManager implementations
    // may not be providing this field data
    SerialPortDeviceInfo.AccessSize = EFI_ACPI_6_3_DWORD;
  } else {
    SerialPortDeviceInfo.AccessSize = SerialPortInfo->AccessSize;
  }

  AsciiSPrint (
    SerialPortDeviceInfo.ObjectName,
    sizeof (SerialPortDeviceInfo.ObjectName),
    ObjectName
    );

  // Populate serial port device
  Status = PopulateDbg2Device (
             AcpiDbg2Device,
             &SerialPortDeviceInfo,
             &SerialMemoryRange,
             1
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ERROR: DBG2: Failed to populate serial port device. Status = %r\n", Status));
    goto error_handler;
  }

  if ((SerialPortInfo->PortSubtype ==
       EFI_ACPI_DBG2_PORT_SUBTYPE_SERIAL_ARM_PL011_UART)           ||
      (SerialPortInfo->PortSubtype ==
       EFI_ACPI_DBG2_PORT_SUBTYPE_SERIAL_ARM_SBSA_GENERIC_UART_2X) ||
      (SerialPortInfo->PortSubtype ==
       EFI_ACPI_DBG2_PORT_SUBTYPE_SERIAL_ARM_SBSA_GENERIC_UART))
  {
    // Initialize the serial port
    Status = SetupDebugUart (SerialPortInfo);
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: DBG2: Failed to configure debug serial port. Status = %r\n",
        Status
        ));
      goto error_handler;
    }
  }

  // Build a SSDT table describing the serial port.
  Status = BuildSsdtSerialPortTable (
             AcpiTableInfo,
             SerialPortInfo,
             SerialPortDeviceInfo.ObjectName,
             Uid,
             Table
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: DBG2: Failed to build associated SSDT table. Status = %r\n",
      Status
      ));
    goto error_handler;
  }

error_handler:
  return Status;
}

/** Construct the DBG2 ACPI table and its associated SSDT table.

  This function invokes the Configuration Manager protocol interface
  to get the required hardware information for generating the ACPI
  table.

  If this function allocates any resources then they must be freed
  in the FreeXXXXTableResourcesEx function.

  @param [in]  This            Pointer to the ACPI table generator.
  @param [in]  AcpiTableInfo   Pointer to the ACPI table information.
  @param [in]  CfgMgrProtocol  Pointer to the Configuration Manager
                               Protocol interface.
  @param [out] Table           Pointer to a list of generated ACPI table(s).
  @param [out] TableCount      Number of generated ACPI table(s).

  @retval EFI_SUCCESS            Table generated successfully.
  @retval EFI_BAD_BUFFER_SIZE    The size returned by the Configuration
                                 Manager is less than the Object size for
                                 the requested object.
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.
  @retval EFI_NOT_FOUND          Could not find information.
  @retval EFI_OUT_OF_RESOURCES   Could not allocate memory.
  @retval EFI_UNSUPPORTED        Unsupported configuration.
**/
STATIC
EFI_STATUS
EFIAPI
BuildDbg2TableEx (
  IN  CONST ACPI_TABLE_GENERATOR                           *This,
  IN  CONST CM_STD_OBJ_ACPI_TABLE_INFO             *CONST  AcpiTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL   *CONST  CfgMgrProtocol,
  OUT       EFI_ACPI_DESCRIPTION_HEADER                    ***Table,
  OUT       UINTN                                  *CONST  TableCount
  )
{
  EFI_STATUS                                     Status;
  EFI_ACPI_DEBUG_PORT_2_DESCRIPTION_TABLE        *AcpiDbg2;
  UINT32                                         AcpiDbg2Len;
  UINT32                                         Index;
  CM_ARCH_COMMON_DBG2_DEVICE_INFO                *Dbg2DeviceInfo;
  UINT32                                         Dbg2DeviceCount;
  UINT32                                         Dbg2DeviceSize;
  CM_ARCH_COMMON_MEMORY_RANGE_DESCRIPTOR         **Dbg2DevicesMemoryRange;
  UINT32                                         *Dbg2DevicesMemoryRangeCount;
  CM_ARCH_COMMON_SERIAL_PORT_INFO                *SerialPortInfo;
  UINT32                                         SerialPortCount;
  EFI_ACPI_DESCRIPTION_HEADER                    **TableList;
  UINT32                                         TotalDevices;
  EFI_ACPI_DBG2_DEBUG_DEVICE_INFORMATION_STRUCT  *CurrentDbg2Device;
  UINT64                                         TotalSize;

  ASSERT (This != NULL);
  ASSERT (AcpiTableInfo != NULL);
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (Table != NULL);
  ASSERT (TableCount != NULL);
  ASSERT (AcpiTableInfo->TableGeneratorId == This->GeneratorID);
  ASSERT (AcpiTableInfo->AcpiTableSignature == This->AcpiTableSignature);

  if ((AcpiTableInfo->AcpiTableRevision < This->MinAcpiTableRevision) ||
      (AcpiTableInfo->AcpiTableRevision > This->AcpiTableRevision))
  {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: DBG2: Requested table revision = %d, is not supported."
      "Supported table revision: Minimum = %d, Maximum = %d\n",
      AcpiTableInfo->AcpiTableRevision,
      This->MinAcpiTableRevision,
      This->AcpiTableRevision
      ));
    return EFI_INVALID_PARAMETER;
  }

  *Table                      = NULL;
  *TableCount                 = 0;
  AcpiDbg2                    = NULL;
  TableList                   = NULL;
  Dbg2DevicesMemoryRange      = NULL;
  Dbg2DevicesMemoryRangeCount = NULL;
  Dbg2DeviceSize              = 0;
  Dbg2DeviceCount             = 0;
  SerialPortCount             = 0;

  // Get serial port information
  Status = GetEArchCommonObjSerialDebugPortInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &SerialPortInfo,
             &SerialPortCount
             );
  if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    DEBUG ((DEBUG_ERROR, "ERROR: DBG2: Failed to get serial port information. Status = %r\n", Status));
    goto error_handler;
  }

  // Only one serial port is supported
  if (SerialPortCount > 1) {
    DEBUG ((DEBUG_WARN, "WARNING: DBG2: Too many serial ports to populate. Count = %u\n", SerialPortCount));
    SerialPortCount = 1;
  }

  if (SerialPortCount != 0) {
    Status = ValidateSerialPortInfo (SerialPortInfo, SerialPortCount);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "ERROR: DBG2: Invalid serial port information. Status = %r\n", Status));
      goto error_handler;
    }
  }

  // Get DBG2 device information
  Status = GetEArchCommonObjGenericDbg2DeviceInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &Dbg2DeviceInfo,
             &Dbg2DeviceCount
             );
  if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    DEBUG ((DEBUG_ERROR, "ERROR: DBG2: Failed to get DBG2 device information. Status = %r\n", Status));
    goto error_handler;
  }

  if (Dbg2DeviceCount != 0) {
    // Get all the memory ranges for the DBG2 devices
    Dbg2DevicesMemoryRange = (CM_ARCH_COMMON_MEMORY_RANGE_DESCRIPTOR **)AllocateZeroPool (sizeof (CM_ARCH_COMMON_MEMORY_RANGE_DESCRIPTOR *) * Dbg2DeviceCount);
    if (Dbg2DevicesMemoryRange == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      DEBUG ((DEBUG_ERROR, "ERROR: DBG2: Failed to allocate memory for device ranges\n"));
      goto error_handler;
    }

    Dbg2DevicesMemoryRangeCount = (UINT32 *)AllocateZeroPool (sizeof (UINT32) * Dbg2DeviceCount);
    if (Dbg2DevicesMemoryRangeCount == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      DEBUG ((DEBUG_ERROR, "ERROR: DBG2: Failed to allocate memory for range counts\n"));
      goto error_handler;
    }

    // Get memory ranges for each device
    for (Index = 0; Index < Dbg2DeviceCount; Index++) {
      if (Dbg2DeviceInfo[Index].AddressResourceToken != CM_NULL_TOKEN) {
        Status = GetEArchCommonObjMemoryRangeDescriptor (
                   CfgMgrProtocol,
                   Dbg2DeviceInfo[Index].AddressResourceToken,
                   &Dbg2DevicesMemoryRange[Index],
                   &Dbg2DevicesMemoryRangeCount[Index]
                   );
        if (EFI_ERROR (Status)) {
          DEBUG ((DEBUG_ERROR, "ERROR: DBG2: Failed to get memory range for device %u. Status = %r\n", Index, Status));
          goto error_handler;
        }

        // Calculate total size with overflow check
        TotalSize = (UINT64)sizeof (EFI_ACPI_DBG2_DEBUG_DEVICE_INFORMATION_STRUCT) +
                    ((UINT64)sizeof (EFI_ACPI_6_3_GENERIC_ADDRESS_STRUCTURE) + sizeof (UINT32)) * Dbg2DevicesMemoryRangeCount[Index] +
                    (UINT64)sizeof (CHAR8) * DBG2_NAMESPACESTRING_FIELD_SIZE;

        if (TotalSize > MAX_UINT32) {
          Status = EFI_INVALID_PARAMETER;
          DEBUG ((DEBUG_ERROR, "ERROR: DBG2: Device size too large for device %u\n", Index));
          goto error_handler;
        }

        Dbg2DeviceSize += (UINT32)TotalSize;
      } else {
        Status = EFI_INVALID_PARAMETER;
        DEBUG ((DEBUG_ERROR, "ERROR: DBG2: Missing address resource token for device %u\n", Index));
        goto error_handler;
      }
    }
  }

  TotalDevices = SerialPortCount + Dbg2DeviceCount;
  if (TotalDevices == 0) {
    DEBUG ((DEBUG_ERROR, "ERROR: DBG2: No devices found\n"));
    Status = EFI_NOT_FOUND;
    goto error_handler;
  }

  // Allocate table list
  TableList = (EFI_ACPI_DESCRIPTION_HEADER **)AllocateZeroPool (
                                                sizeof (EFI_ACPI_DESCRIPTION_HEADER *) * (1 + SerialPortCount)
                                                );
  if (TableList == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    DEBUG ((DEBUG_ERROR, "ERROR: DBG2: Failed to allocate table list\n"));
    goto error_handler;
  }

  // Calculate total DBG2 table size
  AcpiDbg2Len = sizeof (EFI_ACPI_DEBUG_PORT_2_DESCRIPTION_TABLE) + Dbg2DeviceSize;

  if (SerialPortCount > 0) {
    TotalSize = (UINT64)AcpiDbg2Len +
                (UINT64)sizeof (EFI_ACPI_DBG2_DEBUG_DEVICE_INFORMATION_STRUCT) +
                ((UINT64)sizeof (EFI_ACPI_6_3_GENERIC_ADDRESS_STRUCTURE) + sizeof (UINT32)) * SerialPortCount +
                (UINT64)sizeof (CHAR8) * DBG2_NAMESPACESTRING_FIELD_SIZE;

    if (TotalSize > MAX_UINT32) {
      Status = EFI_INVALID_PARAMETER;
      DEBUG ((DEBUG_ERROR, "ERROR: DBG2: Total table size too large\n"));
      goto error_handler;
    }

    AcpiDbg2Len = (UINT32)TotalSize;
  }

  // Allocate DBG2 table
  AcpiDbg2 = (EFI_ACPI_DEBUG_PORT_2_DESCRIPTION_TABLE *)AllocateZeroPool (AcpiDbg2Len);
  if (AcpiDbg2 == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    DEBUG ((DEBUG_ERROR, "ERROR: DBG2: Failed to allocate DBG2 table\n"));
    goto error_handler;
  }

  Status = AddAcpiHeader (
             CfgMgrProtocol,
             This,
             &AcpiDbg2->Header,
             AcpiTableInfo,
             AcpiDbg2Len
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "ERROR: DBG2: Failed to add ACPI header. Status = %r\n", Status));
    goto error_handler;
  }

  AcpiDbg2->OffsetDbgDeviceInfo = sizeof (EFI_ACPI_DEBUG_PORT_2_DESCRIPTION_TABLE);
  AcpiDbg2->NumberDbgDeviceInfo = TotalDevices;
  TableList[0]                  = &AcpiDbg2->Header;

  CurrentDbg2Device = (EFI_ACPI_DBG2_DEBUG_DEVICE_INFORMATION_STRUCT *)((UINT8 *)AcpiDbg2 + AcpiDbg2->OffsetDbgDeviceInfo);

  // Populate DBG2 devices
  for (Index = 0; Index < Dbg2DeviceCount; Index++) {
    if ((CurrentDbg2Device == NULL) ||
        ((UINTN)CurrentDbg2Device - (UINTN)AcpiDbg2 > AcpiDbg2Len))
    {
      Status = EFI_INVALID_PARAMETER;
      DEBUG ((DEBUG_ERROR, "ERROR: DBG2: Device pointer out of range\n"));
      goto error_handler;
    }

    if ((Dbg2DeviceInfo == NULL) || (Dbg2DevicesMemoryRange == NULL) || (Dbg2DevicesMemoryRangeCount == NULL)) {
      Status = EFI_INVALID_PARAMETER;
      DEBUG ((DEBUG_ERROR, "ERROR: DBG2: Dbg2DeviceInfo, Dbg2DevicesMemoryRange, or Dbg2DevicesMemoryRangeCount is NULL. Status = %r\n", Status));
      goto error_handler;
    }

    Status = PopulateDbg2Device (
               &CurrentDbg2Device,
               &Dbg2DeviceInfo[Index],
               Dbg2DevicesMemoryRange[Index],
               Dbg2DevicesMemoryRangeCount[Index]
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "ERROR: DBG2: Failed to populate device %u. Status = %r\n", Index, Status));
      goto error_handler;
    }
  }

  // Currently only one serial port is supported
  if (SerialPortCount > 0) {
    // Validate device pointer
    if ((CurrentDbg2Device == NULL) ||
        ((UINTN)CurrentDbg2Device - (UINTN)AcpiDbg2 > AcpiDbg2Len))
    {
      Status = EFI_INVALID_PARAMETER;
      DEBUG ((DEBUG_ERROR, "ERROR: DBG2: Serial port device pointer out of range\n"));
      goto error_handler;
    }

    Status = BuildDbg2SerialPortEntry (
               AcpiTableInfo,
               &CurrentDbg2Device,
               &SerialPortInfo[0],
               NAME_STR_DBG_PORT0,
               UID_DBG_PORT0,
               &TableList[1]
               );
  }

  *TableCount = 1 + SerialPortCount;
  *Table      = TableList;

error_handler:
  // Clear the memory ranges in all cases
  if (Dbg2DevicesMemoryRange != NULL) {
    FreePool (Dbg2DevicesMemoryRange);
    Dbg2DevicesMemoryRange = NULL;
  }

  if (Dbg2DevicesMemoryRangeCount != NULL) {
    FreePool (Dbg2DevicesMemoryRangeCount);
    Dbg2DevicesMemoryRangeCount = NULL;
  }

  // Free the tables in case of error
  if (EFI_ERROR (Status)) {
    if (AcpiDbg2 != NULL) {
      FreePool (AcpiDbg2);
    }

    if (TableList != NULL) {
      FreePool (TableList);
    }
  }

  return Status;
}

/** This macro defines the DBG2 Table Generator revision.
*/
#define DBG2_GENERATOR_REVISION  CREATE_REVISION (1, 0)

/** The interface for the DBG2 Table Generator.
*/
STATIC
CONST
ACPI_TABLE_GENERATOR  Dbg2Generator = {
  // Generator ID
  CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdDbg2),
  // Generator Description
  L"ACPI.STD.DBG2.GENERATOR",
  // ACPI Table Signature
  EFI_ACPI_6_3_DEBUG_PORT_2_TABLE_SIGNATURE,
  // ACPI Table Revision supported by this Generator
  EFI_ACPI_DBG2_DEBUG_DEVICE_INFORMATION_STRUCT_REVISION,
  // Minimum supported ACPI Table Revision
  EFI_ACPI_DBG2_DEBUG_DEVICE_INFORMATION_STRUCT_REVISION,
  // Creator ID
  TABLE_GENERATOR_CREATOR_ID,
  // Creator Revision
  DBG2_GENERATOR_REVISION,
  // Build table function. Use the extended version instead.
  NULL,
  // Free table function. Use the extended version instead.
  NULL,
  // Extended Build table function.
  BuildDbg2TableEx,
  // Extended free function.
  FreeDbg2TableEx
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
AcpiDbg2LibConstructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = RegisterAcpiTableGenerator (&Dbg2Generator);
  DEBUG ((DEBUG_INFO, "DBG2: Register Generator. Status = %r\n", Status));
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
AcpiDbg2LibDestructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = DeregisterAcpiTableGenerator (&Dbg2Generator);
  DEBUG ((DEBUG_INFO, "DBG2: Deregister Generator. Status = %r\n", Status));
  ASSERT_EFI_ERROR (Status);
  return Status;
}
