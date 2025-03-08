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
  - EArchCommonObjDbg2DeviceInfo
*/

/** A format string representing the name of serial port debug devices.
*/
#define NAME_STR_SERIAL_DBG_FMT  "COM%x"

/** Maximum number of serial port debug devices.
*/
#define MAX_SERIAL_DB2_DEVICES  14

// _SB scope of the AML namespace.
#define SB_SCOPE  "\\_SB_."

/** An UID representing the serial debug port 0.
*/
#define UID_DBG_PORT0  0

/** An UID representing the spcr to skip for any DBG2 entries.
*/
#define UID_DBG_SPCR_SKIP  1

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
  EArchCommonObjDbg2DeviceInfo,
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

  if (TableCount != 0) {
    FreePool (TableList[0]);
  }

  for (Index = 1; Index < TableCount; Index++) {
    if ((TableList[Index] == NULL) ||
        (TableList[Index]->Signature !=
         EFI_ACPI_6_3_SECONDARY_SYSTEM_DESCRIPTION_TABLE_SIGNATURE))
    {
      DEBUG ((DEBUG_ERROR, "ERROR: DBG2: Invalid SSDT table pointer.\n"));
      return EFI_INVALID_PARAMETER;
    }

    // Only need to free the SSDT table at index > 1.
    Status = FreeSsdtSerialPortTable (TableList[Index]);
    ASSERT_EFI_ERROR (Status);
  }

  // Free the table list.
  FreePool (*Table);

  return Status;
}

/** Populates the DBG2 device info structure.

  @param [in]  CfgMgrProtocol  Pointer to the Configuration Manager
                               Protocol interface.
  @param [in, out] AcpiDbg2       Pointer to the DBG2 ACPI table. Will be reallocated to fit the new device.
  @param [in, out] AcpiDbg2Len    Pointer to the length of the DBG2 ACPI table.
  @param [in]      DeviceInfo     Pointer to the Device Info structure.
  @param [in]      MemoryRange    The memory range of the device. Will be used if token in device info is CM_NULL_TOKEN.

  @retval EFI_SUCCESS           The structure was populated correctly.
**/
STATIC
EFI_STATUS
EFIAPI
PopulateDbg2Device (
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL   *CONST  CfgMgrProtocol,
  IN OUT EFI_ACPI_DEBUG_PORT_2_DESCRIPTION_TABLE           **AcpiDbg2,
  IN OUT UINT32                                            *AcpiDbg2Len,
  IN CM_ARCH_COMMON_DBG2_DEVICE_INFO                       *DeviceInfo,
  IN CM_ARCH_COMMON_MEMORY_RANGE_DESCRIPTOR                *MemoryRange OPTIONAL
  )
{
  EFI_STATUS                                     Status;
  UINTN                                          Index;
  CM_ARCH_COMMON_MEMORY_RANGE_DESCRIPTOR         *MemoryRanges;
  UINT32                                         MemoryRangesCount;
  UINT16                                         Dbg2DeviceSize;
  EFI_ACPI_DBG2_DEBUG_DEVICE_INFORMATION_STRUCT  *Dbg2Device;
  VOID                                           *NewAcpiDbg2;
  EFI_ACPI_6_3_GENERIC_ADDRESS_STRUCTURE         *BaseAddressRegister;
  UINT32                                         *AddressSize;

  // Parameter validation
  if ((CfgMgrProtocol == NULL) || (AcpiDbg2 == NULL) ||
      (AcpiDbg2Len == NULL) || (DeviceInfo == NULL))
  {
    DEBUG ((DEBUG_ERROR, "ERROR: DBG2: Invalid parameters to PopulateDbg2Device\n"));
    return EFI_INVALID_PARAMETER;
  }

  // Check string length before concatenation
  if ((DeviceInfo->ObjectName[0] != '\0') &&
      (AsciiStrLen (SB_SCOPE) + AsciiStrLen (DeviceInfo->ObjectName) >= DBG2_NAMESPACESTRING_FIELD_SIZE))
  {
    DEBUG ((DEBUG_ERROR, "ERROR: DBG2: Namespace string too long\n"));
    return EFI_BUFFER_TOO_SMALL;
  }

  if (DeviceInfo->AddressResourceToken != CM_NULL_TOKEN) {
    Status = GetEArchCommonObjMemoryRangeDescriptor (
               CfgMgrProtocol,
               DeviceInfo->AddressResourceToken,
               &MemoryRanges,
               &MemoryRangesCount
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "ERROR: DBG2: Failed to get memory range descriptor. Status = %r\n", Status));
      return Status;
    }

    if (MemoryRangesCount > MAX_UINT8) {
      DEBUG ((DEBUG_ERROR, "ERROR: DBG2: Too many memory ranges. Count = %u\n", MemoryRangesCount));
      return EFI_INVALID_PARAMETER;
    }

    // Validate all memory ranges
    for (Index = 0; Index < MemoryRangesCount; Index++) {
      if (MemoryRanges[Index].Length > MAX_UINT32) {
        DEBUG ((DEBUG_ERROR, "ERROR: DBG2: Memory range length too large. Length = %u\n", MemoryRanges[Index].Length));
        return EFI_INVALID_PARAMETER;
      }
    }
  } else {
    MemoryRanges      = MemoryRange;
    MemoryRangesCount = 1;
  }

  Dbg2DeviceSize = (UINT16)(sizeof (EFI_ACPI_DBG2_DEBUG_DEVICE_INFORMATION_STRUCT) +
                            ((sizeof (EFI_ACPI_6_3_GENERIC_ADDRESS_STRUCTURE) + sizeof (UINT32)) * (MemoryRangesCount)) +
                            (sizeof (CHAR8) * DBG2_NAMESPACESTRING_FIELD_SIZE));

  NewAcpiDbg2 = ReallocatePool (*AcpiDbg2Len, *AcpiDbg2Len + Dbg2DeviceSize, *AcpiDbg2);
  if (NewAcpiDbg2 == NULL) {
    DEBUG ((DEBUG_ERROR, "ERROR: DBG2: Failed to reallocate memory for DBG2 device.\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  // New table is at the end of the old table
  Dbg2Device = (EFI_ACPI_DBG2_DEBUG_DEVICE_INFORMATION_STRUCT *)((CHAR8 *)NewAcpiDbg2 + *AcpiDbg2Len);

  Dbg2Device->Revision                        = EFI_ACPI_DBG2_DEBUG_DEVICE_INFORMATION_STRUCT_REVISION;
  Dbg2Device->Length                          = Dbg2DeviceSize;
  Dbg2Device->NumberofGenericAddressRegisters = (UINT8)MemoryRangesCount;
  Dbg2Device->NameSpaceStringLength           = DBG2_NAMESPACESTRING_FIELD_SIZE;
  Dbg2Device->NameSpaceStringOffset           = sizeof (EFI_ACPI_DBG2_DEBUG_DEVICE_INFORMATION_STRUCT);
  Dbg2Device->OemDataLength                   = 0;
  Dbg2Device->OemDataOffset                   = 0;
  Dbg2Device->PortType                        = DeviceInfo->PortType;
  Dbg2Device->PortSubtype                     = DeviceInfo->PortSubtype;
  Dbg2Device->BaseAddressRegisterOffset       = Dbg2Device->NameSpaceStringOffset + DBG2_NAMESPACESTRING_FIELD_SIZE;
  Dbg2Device->AddressSizeOffset               = Dbg2Device->BaseAddressRegisterOffset +
                                                ((UINT16)sizeof (EFI_ACPI_6_3_GENERIC_ADDRESS_STRUCTURE) * ((UINT8)MemoryRangesCount));
  for (Index = 0; Index < MemoryRangesCount; Index++) {
    BaseAddressRegister                    = (EFI_ACPI_6_3_GENERIC_ADDRESS_STRUCTURE *)((UINT8 *)Dbg2Device + Dbg2Device->BaseAddressRegisterOffset + (Index * sizeof (EFI_ACPI_6_3_GENERIC_ADDRESS_STRUCTURE)));
    AddressSize                            = (UINT32 *)((UINT8 *)Dbg2Device + Dbg2Device->AddressSizeOffset + (Index * sizeof (UINT32)));
    BaseAddressRegister->AddressSpaceId    = EFI_ACPI_6_3_SYSTEM_MEMORY;
    BaseAddressRegister->RegisterBitWidth  = 32;
    BaseAddressRegister->RegisterBitOffset = 0;
    BaseAddressRegister->AccessSize        = DeviceInfo->AccessSize;
    BaseAddressRegister->Address           = MemoryRanges[Index].BaseAddress;
    *AddressSize                           = (UINT32)(MemoryRanges[Index].Length);
  }

  if (DeviceInfo->ObjectName[0] == '\0') {
    // If device string is empty then use "." as the name per the DBG2 specification.
    AsciiSPrint ((CHAR8 *)Dbg2Device + Dbg2Device->NameSpaceStringOffset, DBG2_NAMESPACESTRING_FIELD_SIZE, ".");
  } else {
    // Construct the namespace string for the device (e.g. \_SB_.COM1)
    AsciiSPrint ((CHAR8 *)Dbg2Device + Dbg2Device->NameSpaceStringOffset, DBG2_NAMESPACESTRING_FIELD_SIZE, "%a%a", SB_SCOPE, DeviceInfo->ObjectName);
  }

  *AcpiDbg2Len              += Dbg2DeviceSize;
  *AcpiDbg2                  = NewAcpiDbg2;
  (*AcpiDbg2)->Header.Length = *AcpiDbg2Len;

  return EFI_SUCCESS;
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
  EFI_STATUS                               Status;
  EFI_ACPI_DEBUG_PORT_2_DESCRIPTION_TABLE  *AcpiDbg2;
  UINT32                                   AcpiDbg2Len;
  UINT32                                   Index;
  CM_ARCH_COMMON_DBG2_DEVICE_INFO          *Dbg2DeviceInfo;
  UINT32                                   Dbg2DeviceCount;
  CM_ARCH_COMMON_SERIAL_PORT_INFO          *SerialPortInfo;
  UINT32                                   SerialPortCount;
  CM_ARCH_COMMON_DBG2_DEVICE_INFO          SerialPortDeviceInfo;
  CM_ARCH_COMMON_MEMORY_RANGE_DESCRIPTOR   SerialMemoryRange;
  EFI_ACPI_DESCRIPTION_HEADER              **TableList;
  UINT32                                   TotalDevices;
  UINT32                                   Uid;

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

  *Table    = NULL;
  AcpiDbg2  = NULL;
  TableList = NULL;

  Status = GetEArchCommonObjSerialDebugPortInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &SerialPortInfo,
             &SerialPortCount
             );
  if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: DBG2: Failed to get serial port information. Status = %r\n",
      Status
      ));
    return Status;
  }

  if (SerialPortCount > MAX_SERIAL_DB2_DEVICES) {
    DEBUG ((
      DEBUG_ERROR,
      "WARNING: DBG2: Too many serial ports to populated. Count = %x\n",
      Status
      ));
    SerialPortCount = MAX_SERIAL_DB2_DEVICES;
  }

  Status = GetEArchCommonObjDbg2DeviceInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &Dbg2DeviceInfo,
             &Dbg2DeviceCount
             );
  if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: DBG2: Failed to get DBG2 device information. Status = %r\n",
      Status
      ));
    return Status;
  }

  TotalDevices = SerialPortCount + Dbg2DeviceCount;
  if (TotalDevices == 0) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: DBG2: No information found. Status = %r\n",
      EFI_NOT_FOUND
      ));
    return EFI_NOT_FOUND;
  }

  if (SerialPortCount != 0) {
    Status = ValidateSerialPortInfo (SerialPortInfo, SerialPortCount);
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: DBG2: Invalid serial port information. Status = %r\n",
        Status
        ));
      return Status;
    }
  }

  // Allocate a table to store pointers to the DBG2 and SSDT tables.
  TableList = (EFI_ACPI_DESCRIPTION_HEADER **)
              AllocateZeroPool (sizeof (EFI_ACPI_DESCRIPTION_HEADER *) * (1+SerialPortCount));
  if (TableList == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: DBG2: Failed to allocate memory for Table List," \
      " Status = %r\n",
      Status
      ));
    return Status;
  }

  AcpiDbg2Len = sizeof (EFI_ACPI_DEBUG_PORT_2_DESCRIPTION_TABLE);

  AcpiDbg2 = (EFI_ACPI_DEBUG_PORT_2_DESCRIPTION_TABLE *)AllocateZeroPool (AcpiDbg2Len);
  if (AcpiDbg2 == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: DBG2: Failed to allocate memory for Dbg2 table," \
      " Status = %r\n",
      Status
      ));
    return Status;
  }

  Status = AddAcpiHeader (
             CfgMgrProtocol,
             This,
             &AcpiDbg2->Header,
             AcpiTableInfo,
             AcpiDbg2Len
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: DBG2: Failed to add ACPI header. Status = %r\n",
      Status
      ));
    goto error_handler;
  }

  AcpiDbg2->OffsetDbgDeviceInfo = sizeof (EFI_ACPI_DEBUG_PORT_2_DESCRIPTION_TABLE);
  AcpiDbg2->NumberDbgDeviceInfo = TotalDevices;

  for (Index = 0; Index < Dbg2DeviceCount; Index++) {
    Status = PopulateDbg2Device (CfgMgrProtocol, &AcpiDbg2, &AcpiDbg2Len, &Dbg2DeviceInfo[Index], NULL);
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: DBG2: Failed to populate Dbg2 Device. Status = %r\n",
        Status
        ));
      goto error_handler;
    }
  }

  Uid = UID_DBG_PORT0;
  for (Index = 0; Index < SerialPortCount; Index++) {
    if (Uid == UID_DBG_SPCR_SKIP) {
      Uid++;
    }

    SerialMemoryRange.BaseAddress             = SerialPortInfo[Index].BaseAddress;
    SerialMemoryRange.Length                  = SerialPortInfo[Index].BaseAddressLength;
    SerialPortDeviceInfo.AddressResourceToken = CM_NULL_TOKEN;
    SerialPortDeviceInfo.PortType             = EFI_ACPI_DBG2_PORT_TYPE_SERIAL;
    SerialPortDeviceInfo.PortSubtype          = SerialPortInfo[Index].PortSubtype;
    // Set the access size
    if (SerialPortInfo[Index].AccessSize >= EFI_ACPI_6_3_QWORD) {
      Status = EFI_INVALID_PARAMETER;
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: DBG2: Access size must be <= 3 (DWORD). Status = %r\n",
        Status
        ));
      goto error_handler;
    } else if (SerialPortInfo[Index].AccessSize == EFI_ACPI_6_3_UNDEFINED) {
      // 0 Undefined (legacy reasons)
      // Default to DWORD access size as the access
      // size field was introduced at a later date
      // and some ConfigurationManager implementations
      // may not be providing this field data
      SerialPortDeviceInfo.AccessSize = EFI_ACPI_6_3_DWORD;
    } else {
      SerialPortDeviceInfo.AccessSize = SerialPortInfo[Index].AccessSize;
    }

    AsciiSPrint (
      SerialPortDeviceInfo.ObjectName,
      sizeof (SerialPortDeviceInfo.ObjectName),
      NAME_STR_SERIAL_DBG_FMT,
      Uid
      );

    Status = PopulateDbg2Device (CfgMgrProtocol, &AcpiDbg2, &AcpiDbg2Len, &SerialPortDeviceInfo, &SerialMemoryRange);
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: DBG2: Failed to populate Dbg2 Device. Status = %r\n",
        Status
        ));
      goto error_handler;
    }

    if ((SerialPortInfo[Index].PortSubtype ==
         EFI_ACPI_DBG2_PORT_SUBTYPE_SERIAL_ARM_PL011_UART)           ||
        (SerialPortInfo[Index].PortSubtype ==
         EFI_ACPI_DBG2_PORT_SUBTYPE_SERIAL_ARM_SBSA_GENERIC_UART_2X) ||
        (SerialPortInfo[Index].PortSubtype ==
         EFI_ACPI_DBG2_PORT_SUBTYPE_SERIAL_ARM_SBSA_GENERIC_UART))
    {
      // Initialize the serial port
      Status = SetupDebugUart (&SerialPortInfo[Index]);
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
               &SerialPortInfo[Index],
               SerialPortDeviceInfo.ObjectName,
               Uid,
               &TableList[1+Index]
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: DBG2: Failed to build associated SSDT table. Status = %r\n",
        Status
        ));
      goto error_handler;
    }

    Uid++;
  }

  // Add to table list at end as the AcpiDbg2 is reallocated for each device
  TableList[0] = &AcpiDbg2->Header;

  *TableCount = 1+SerialPortCount;
  *Table      = TableList;

  return Status;

error_handler:
  if (AcpiDbg2 != NULL) {
    FreePool (AcpiDbg2);
  }

  if (TableList != NULL) {
    FreePool (TableList);
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
