/** @file
  DBG2 Table Generator

  Copyright (c) 2017 - 2022, Arm Limited. All rights reserved.<BR>
  Copyright (c) 2024 NVIDIA CORPORATION & AFFILIATES. All rights reserved. <BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - Microsoft Debug Port Table 2 (DBG2) Specification - December 10, 2015.

**/

#include <IndustryStandard/AcpiAml.h>
#include <IndustryStandard/DebugPort2Table.h>
#include <IndustryStandard/Pci.h>
#include <Library/AcpiLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PL011UartLib.h>
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

/** ARM standard DBG2 Table Generator

  Constructs the DBG2 table for debug devices

Requirements:
  The following Configuration Manager Object(s) are required by
  this Generator:
  - EArmObjSerialDebugPortInfo
  - EArmObjDbg2DeviceInfo
*/

#pragma pack(1)

/** The number of Generic Address Registers
    presented in the debug device information.
*/
#define DBG2_NUMBER_OF_GENERIC_ADDRESS_REGISTERS  1

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

/** Max number of debug serial ports.
*/
#define DBG_SERIAL_PORT_MAX  15

/** The length of the namespace string.
*/
#define DBG2_NAMESPACESTRING_FIELD_SIZE  (sizeof (SB_SCOPE) + AML_NAME_SEG_SIZE)

/** A structure that provides the OS with the required information
    for initializing a debugger connection.
*/
typedef struct {
  /// The debug device information for the platform
  EFI_ACPI_DBG2_DEBUG_DEVICE_INFORMATION_STRUCT    Dbg2Device;

  /// The base address register for the serial port
  EFI_ACPI_6_3_GENERIC_ADDRESS_STRUCTURE           BaseAddressRegister[PCI_MAX_BAR];

  /// The address size
  UINT32                                           AddressSize[PCI_MAX_BAR];

  /// The debug port name string
  CHAR8                                            NameSpaceString[DBG2_NAMESPACESTRING_FIELD_SIZE];
} DBG2_DEBUG_DEVICE_INFORMATION;

#pragma pack()

/** This macro expands to a function that retrieves the Serial
    debug port information from the Configuration Manager
*/
GET_OBJECT_LIST (
  EObjNameSpaceArm,
  EArmObjSerialDebugPortInfo,
  CM_ARM_SERIAL_PORT_INFO
  );

/** This macro expands to a function that retrieves the DBG2
    device information from the Configuration Manager
*/
GET_OBJECT_LIST (
  EObjNameSpaceArm,
  EArmObjDbg2DeviceInfo,
  CM_ARM_DBG2_DEVICE_INFO
  );

/** Initialize the PL011/SBSA UART with the parameters obtained from
    the Configuration Manager.

  @param [in]  SerialPortInfo Pointer to the Serial Port Information.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER The parameters for serial port initialization
                                are invalid.
**/
STATIC
EFI_STATUS
SetupDebugUart (
  IN  CONST CM_ARM_SERIAL_PORT_INFO  *CONST  SerialPortInfo
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
  Status   = PL011UartInitializePort (
               (UINTN)SerialPortInfo->BaseAddress,
               SerialPortInfo->Clock,
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

  @param [in]      Dbg2Device     Pointer to the Debug Device info structure.
  @param [in]      DeviceInfo     Pointer to the Device Info structure.

  @retval EFI_SUCCESS           The structure was populated correctly.
**/
STATIC
EFI_STATUS
EFIAPI
PopulateDbg2Device (
  IN DBG2_DEBUG_DEVICE_INFORMATION  *Dbg2Device,
  IN CM_ARM_DBG2_DEVICE_INFO        *DeviceInfo
  )
{
  UINTN  Index;

  Dbg2Device->Dbg2Device.Revision                        = EFI_ACPI_DBG2_DEBUG_DEVICE_INFORMATION_STRUCT_REVISION;
  Dbg2Device->Dbg2Device.Length                          = sizeof (DBG2_DEBUG_DEVICE_INFORMATION);
  Dbg2Device->Dbg2Device.NumberofGenericAddressRegisters = DeviceInfo->NumberOfAddresses;
  Dbg2Device->Dbg2Device.NameSpaceStringLength           = DBG2_NAMESPACESTRING_FIELD_SIZE;
  Dbg2Device->Dbg2Device.NameSpaceStringOffset           = OFFSET_OF (DBG2_DEBUG_DEVICE_INFORMATION, NameSpaceString);
  Dbg2Device->Dbg2Device.OemDataLength                   = 0;
  Dbg2Device->Dbg2Device.OemDataOffset                   = 0;
  Dbg2Device->Dbg2Device.PortType                        = DeviceInfo->PortType;
  Dbg2Device->Dbg2Device.PortSubtype                     = DeviceInfo->PortSubtype;
  Dbg2Device->Dbg2Device.BaseAddressRegisterOffset       = OFFSET_OF (DBG2_DEBUG_DEVICE_INFORMATION, BaseAddressRegister);
  Dbg2Device->Dbg2Device.AddressSizeOffset               = OFFSET_OF (DBG2_DEBUG_DEVICE_INFORMATION, AddressSize);
  for (Index = 0; Index < DeviceInfo->NumberOfAddresses; Index++) {
    Dbg2Device->BaseAddressRegister[Index].AddressSpaceId    = EFI_ACPI_6_3_SYSTEM_MEMORY;
    Dbg2Device->BaseAddressRegister[Index].RegisterBitWidth  = 32;
    Dbg2Device->BaseAddressRegister[Index].RegisterBitOffset = 0;
    Dbg2Device->BaseAddressRegister[Index].AccessSize        = DeviceInfo->AccessSize;
    Dbg2Device->BaseAddressRegister[Index].Address           = DeviceInfo->BaseAddress[Index];
    Dbg2Device->AddressSize[Index]                           = DeviceInfo->BaseAddressLength[Index];
  }

  if (DeviceInfo->ObjectName[0] == '\0') {
    // If device string is empty then use "." as the name per the DBG2 specification.
    AsciiSPrint (Dbg2Device->NameSpaceString, DBG2_NAMESPACESTRING_FIELD_SIZE, ".");
  } else {
    // Construct the namespace string for the device (e.g. \_SB_.COM1)
    AsciiSPrint (Dbg2Device->NameSpaceString, DBG2_NAMESPACESTRING_FIELD_SIZE, "%a%a", SB_SCOPE, DeviceInfo->ObjectName);
  }

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
  UINTN                                    AcpiDbg2Len;
  DBG2_DEBUG_DEVICE_INFORMATION            *CurrentDbg2Device;
  UINT32                                   Index;
  CM_ARM_DBG2_DEVICE_INFO                  *Dbg2DeviceInfo;
  UINT32                                   Dbg2DeviceCount;
  CM_ARM_SERIAL_PORT_INFO                  *SerialPortInfo;
  UINT32                                   SerialPortCount;
  CM_ARM_DBG2_DEVICE_INFO                  SerialPortDeviceInfo;
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

  *Table = NULL;

  Status = GetEArmObjSerialDebugPortInfo (
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

  if (SerialPortCount > DBG_SERIAL_PORT_MAX) {
    DEBUG ((
      DEBUG_ERROR,
      "WARNING: DBG2: Too many serial ports to populated. Count = %x\n",
      Status
      ));
    SerialPortCount = DBG_SERIAL_PORT_MAX;
  }

  Status = GetEArmObjDbg2DeviceInfo (
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

  AcpiDbg2Len = sizeof (EFI_ACPI_DEBUG_PORT_2_DESCRIPTION_TABLE) +
                (TotalDevices * sizeof (DBG2_DEBUG_DEVICE_INFORMATION));

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

  TableList[0]      = &AcpiDbg2->Header;
  CurrentDbg2Device = (DBG2_DEBUG_DEVICE_INFORMATION *)(AcpiDbg2 + 1);

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
    Status = PopulateDbg2Device (CurrentDbg2Device, &Dbg2DeviceInfo[Index]);
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: DBG2: Failed to populate Dbg2 Device. Status = %r\n",
        Status
        ));
      goto error_handler;
    }

    CurrentDbg2Device++;
  }

  Uid = UID_DBG_PORT0;
  for (Index = 0; Index < SerialPortCount; Index++) {
    if (Uid == UID_DBG_SPCR_SKIP) {
      Uid++;
    }

    SerialPortDeviceInfo.NumberOfAddresses    = 1;
    SerialPortDeviceInfo.BaseAddress[0]       = SerialPortInfo[Index].BaseAddress;
    SerialPortDeviceInfo.BaseAddressLength[0] = SerialPortInfo[Index].BaseAddressLength;
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

    Status = PopulateDbg2Device (CurrentDbg2Device, &SerialPortDeviceInfo);
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

    CurrentDbg2Device++;
    Uid++;
  }

  *TableCount = 1+SerialPortCount;
  *Table      = TableList;

  return Status;

error_handler:
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
  TABLE_GENERATOR_CREATOR_ID_ARM,
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
