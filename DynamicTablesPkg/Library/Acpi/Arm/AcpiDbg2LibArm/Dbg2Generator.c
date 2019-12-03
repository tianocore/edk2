/** @file
  DBG2 Table Generator

  Copyright (c) 2017 - 2019, ARM Limited. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - Microsoft Debug Port Table 2 (DBG2) Specification - December 10, 2015.

**/

#include <IndustryStandard/DebugPort2Table.h>
#include <Library/AcpiLib.h>
#include <Library/DebugLib.h>
#include <Library/PL011UartLib.h>
#include <Protocol/AcpiTable.h>
#include <Protocol/SerialIo.h>

// Module specific include files.
#include <AcpiTableGenerator.h>
#include <ConfigurationManagerObject.h>
#include <ConfigurationManagerHelper.h>
#include <Library/TableHelperLib.h>
#include <Protocol/ConfigurationManagerProtocol.h>

/** ARM standard DBG2 Table Generator

  Constructs the DBG2 table for PL011 or SBSA UART peripherals.

Requirements:
  The following Configuration Manager Object(s) are required by
  this Generator:
  - EArmObjSerialDebugPortInfo
*/

#pragma pack(1)

/** The number of debug ports represented by the Table.
*/
#define DBG2_NUM_DEBUG_PORTS                       1

/** The number of Generic Address Registers
    presented in the debug device information.
*/
#define DBG2_NUMBER_OF_GENERIC_ADDRESS_REGISTERS   1

/** The index for the debug port 1 in the Debug port information list.
*/
#define DBG_PORT_INDEX_PORT1                       0

/** A string representing the name of the debug port 1.
*/
#define NAME_STR_PORT1                            "COM1"

/** The length of the namespace string.
*/
#define DBG2_NAMESPACESTRING_FIELD_SIZE            sizeof (NAME_STR_PORT1)

/** The PL011 UART address range length.
*/
#define PL011_UART_LENGTH                          0x1000

/** A structure that provides the OS with the required information
    for initializing a debugger connection.
*/
typedef struct {
  /// The debug device information for the platform
  EFI_ACPI_DBG2_DEBUG_DEVICE_INFORMATION_STRUCT Dbg2Device;

  /// The base address register for the serial port
  EFI_ACPI_6_2_GENERIC_ADDRESS_STRUCTURE        BaseAddressRegister;

  /// The address size
  UINT32 AddressSize;

  /// The debug port name string
  UINT8  NameSpaceString[DBG2_NAMESPACESTRING_FIELD_SIZE];
} DBG2_DEBUG_DEVICE_INFORMATION;

/** A structure representing the information about the debug port(s)
    available on the platform.
*/
typedef struct {
  /// The DBG2 table header
  EFI_ACPI_DEBUG_PORT_2_DESCRIPTION_TABLE Description;

  /// Debug port information list
  DBG2_DEBUG_DEVICE_INFORMATION       Dbg2DeviceInfo[DBG2_NUM_DEBUG_PORTS];
} DBG2_TABLE;

/** A helper macro used for initializing the debug port device
    information structure.

  @param [in]  SubType      The DBG Port SubType.
  @param [in]  UartBase     The UART port base address.
  @param [in]  UartAddrLen  The UART port address range length.
  @param [in]  UartNameStr  The UART port name string.
**/
#define DBG2_DEBUG_PORT_DDI(                                          \
          SubType,                                                    \
          UartBase,                                                   \
          UartAddrLen,                                                \
          UartNameStr                                                 \
          ) {                                                         \
    {                                                                 \
      /* UINT8     Revision */                                        \
      EFI_ACPI_DBG2_DEBUG_DEVICE_INFORMATION_STRUCT_REVISION,         \
      /* UINT16    Length */                                          \
      sizeof (DBG2_DEBUG_DEVICE_INFORMATION),                         \
      /* UINT8     NumberofGenericAddressRegisters */                 \
      DBG2_NUMBER_OF_GENERIC_ADDRESS_REGISTERS,                       \
      /* UINT16    NameSpaceStringLength */                           \
      DBG2_NAMESPACESTRING_FIELD_SIZE,                                \
      /* UINT16    NameSpaceStringOffset */                           \
      OFFSET_OF (DBG2_DEBUG_DEVICE_INFORMATION, NameSpaceString),     \
      /* UINT16    OemDataLength */                                   \
      0,                                                              \
      /* UINT16    OemDataOffset */                                   \
      0,                                                              \
      /* UINT16    Port Type */                                       \
      EFI_ACPI_DBG2_PORT_TYPE_SERIAL,                                 \
      /* UINT16    Port Subtype */                                    \
      SubType,                                                        \
      /* UINT8     Reserved[2] */                                     \
      {EFI_ACPI_RESERVED_BYTE, EFI_ACPI_RESERVED_BYTE},               \
      /* UINT16    BaseAddressRegister Offset */                      \
      OFFSET_OF (DBG2_DEBUG_DEVICE_INFORMATION, BaseAddressRegister), \
      /* UINT16    AddressSize Offset */                              \
      OFFSET_OF (DBG2_DEBUG_DEVICE_INFORMATION, AddressSize)          \
    },                                                                \
    /* EFI_ACPI_6_2_GENERIC_ADDRESS_STRUCTURE BaseAddressRegister */  \
    ARM_GAS32 (UartBase),                                             \
    /* UINT32  AddressSize */                                         \
    UartAddrLen,                                                      \
    /* UINT8   NameSpaceString[MAX_DBG2_NAME_LEN] */                  \
    UartNameStr                                                       \
  }

/** The DBG2 Table template definition.

  Note: fields marked with "{Template}" will be set dynamically
*/
STATIC
DBG2_TABLE AcpiDbg2 = {
  {
    ACPI_HEADER (
      EFI_ACPI_6_2_DEBUG_PORT_2_TABLE_SIGNATURE,
      DBG2_TABLE,
      EFI_ACPI_DBG2_DEBUG_DEVICE_INFORMATION_STRUCT_REVISION
      ),
    OFFSET_OF (DBG2_TABLE, Dbg2DeviceInfo),
    DBG2_NUM_DEBUG_PORTS
  },
  {
    /*
     * Debug port 1
     */
    DBG2_DEBUG_PORT_DDI (
      0,                    // {Template}: Serial Port Subtype
      0,                    // {Template}: Serial Port Base Address
      PL011_UART_LENGTH,
      NAME_STR_PORT1
      )
  }
};

#pragma pack()

/** This macro expands to a function that retrieves the Serial
    debug port information from the Configuration Manager
*/
GET_OBJECT_LIST (
  EObjNameSpaceArm,
  EArmObjSerialDebugPortInfo,
  CM_ARM_SERIAL_PORT_INFO
  );

/** Initialize the PL011 UART with the parameters obtained from
    the Configuration Manager.

  @param [in]  SerialPortInfo Pointer to the Serial Port Information.

  @retval EFI_SUCCESS           Success.
  @retval EFI_INVALID_PARAMETER The parameters for serial port initialization
                                are invalid.
**/
STATIC
EFI_STATUS
SetupDebugUart (
  IN  CONST CM_ARM_SERIAL_PORT_INFO  * CONST SerialPortInfo
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
  Parity = (EFI_PARITY_TYPE)FixedPcdGet8 (PcdUartDefaultParity);
  DataBits = FixedPcdGet8 (PcdUartDefaultDataBits);
  StopBits = (EFI_STOP_BITS_TYPE)FixedPcdGet8 (PcdUartDefaultStopBits);

  BaudRate = SerialPortInfo->BaudRate;
  Status = PL011UartInitializePort (
             (UINTN)SerialPortInfo->BaseAddress,
             SerialPortInfo->Clock,
             &BaudRate,
             &ReceiveFifoDepth,
             &Parity,
             &DataBits,
             &StopBits
             );

  DEBUG ((DEBUG_INFO, "Debug UART Configuration:\n"));
  DEBUG ((DEBUG_INFO, "UART Base  = 0x%lx\n", SerialPortInfo->BaseAddress));
  DEBUG ((DEBUG_INFO, "Clock      = %d\n", SerialPortInfo->Clock));
  DEBUG ((DEBUG_INFO, "Baudrate   = %ld\n", BaudRate));
  DEBUG ((DEBUG_INFO, "Configuring Debug UART. Status = %r\n", Status));

  ASSERT_EFI_ERROR (Status);
  return Status;
}

/** Construct the DBG2 ACPI table

    The BuildDbg2Table function is called by the Dynamic Table Manager
    to construct the DBG2 ACPI table.

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
BuildDbg2Table (
  IN  CONST ACPI_TABLE_GENERATOR                  * CONST This,
  IN  CONST CM_STD_OBJ_ACPI_TABLE_INFO            * CONST AcpiTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  * CONST CfgMgrProtocol,
  OUT       EFI_ACPI_DESCRIPTION_HEADER          ** CONST Table
  )
{
  EFI_STATUS                 Status;
  CM_ARM_SERIAL_PORT_INFO  * SerialPortInfo;

  ASSERT (This != NULL);
  ASSERT (AcpiTableInfo != NULL);
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (Table != NULL);
  ASSERT (AcpiTableInfo->TableGeneratorId == This->GeneratorID);
  ASSERT (AcpiTableInfo->AcpiTableSignature == This->AcpiTableSignature);

  if ((AcpiTableInfo->AcpiTableRevision < This->MinAcpiTableRevision) ||
      (AcpiTableInfo->AcpiTableRevision > This->AcpiTableRevision)) {
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
             NULL
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: DBG2: Failed to get serial port information. Status = %r\n",
      Status
      ));
    goto error_handler;
  }

  if (SerialPortInfo->BaseAddress == 0) {
    Status = EFI_INVALID_PARAMETER;
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: DBG2: Uart port base address is invalid. BaseAddress = 0x%lx\n",
      SerialPortInfo->BaseAddress
      ));
    goto error_handler;
  }

  if ((SerialPortInfo->PortSubtype !=
      EFI_ACPI_DBG2_PORT_SUBTYPE_SERIAL_ARM_PL011_UART) &&
      (SerialPortInfo->PortSubtype !=
      EFI_ACPI_DBG2_PORT_SUBTYPE_SERIAL_ARM_SBSA_GENERIC_UART_2X) &&
      (SerialPortInfo->PortSubtype !=
      EFI_ACPI_DBG2_PORT_SUBTYPE_SERIAL_ARM_SBSA_GENERIC_UART) &&
      (SerialPortInfo->PortSubtype !=
      EFI_ACPI_DBG2_PORT_SUBTYPE_SERIAL_DCC)) {
    Status = EFI_INVALID_PARAMETER;
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: DBG2: Uart port sybtype is invalid. PortSubtype = 0x%x\n",
      SerialPortInfo->PortSubtype
      ));
    goto error_handler;
  }

  Status = AddAcpiHeader (
             CfgMgrProtocol,
             This,
             (EFI_ACPI_DESCRIPTION_HEADER*)&AcpiDbg2,
             AcpiTableInfo,
             sizeof (DBG2_TABLE)
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: DBG2: Failed to add ACPI header. Status = %r\n",
      Status
      ));
    goto error_handler;
  }

  // Update the base address
  AcpiDbg2.Dbg2DeviceInfo[DBG_PORT_INDEX_PORT1].BaseAddressRegister.Address =
    SerialPortInfo->BaseAddress;

  // Update the serial port subtype
  AcpiDbg2.Dbg2DeviceInfo[DBG_PORT_INDEX_PORT1].Dbg2Device.PortSubtype =
    SerialPortInfo->PortSubtype;

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

  *Table = (EFI_ACPI_DESCRIPTION_HEADER*)&AcpiDbg2;

error_handler:
  return Status;
}

/** This macro defines the DBG2 Table Generator revision.
*/
#define DBG2_GENERATOR_REVISION CREATE_REVISION (1, 0)

/** The interface for the DBG2 Table Generator.
*/
STATIC
CONST
ACPI_TABLE_GENERATOR Dbg2Generator = {
  // Generator ID
  CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdDbg2),
  // Generator Description
  L"ACPI.STD.DBG2.GENERATOR",
  // ACPI Table Signature
  EFI_ACPI_6_2_DEBUG_PORT_2_TABLE_SIGNATURE,
  // ACPI Table Revision supported by this Generator
  EFI_ACPI_DBG2_DEBUG_DEVICE_INFORMATION_STRUCT_REVISION,
  // Minimum supported ACPI Table Revision
  EFI_ACPI_DBG2_DEBUG_DEVICE_INFORMATION_STRUCT_REVISION,
  // Creator ID
  TABLE_GENERATOR_CREATOR_ID_ARM,
  // Creator Revision
  DBG2_GENERATOR_REVISION,
  // Build Table function
  BuildDbg2Table,
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
AcpiDbg2LibConstructor (
  IN CONST EFI_HANDLE                ImageHandle,
  IN       EFI_SYSTEM_TABLE  * CONST SystemTable
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
  IN CONST EFI_HANDLE                ImageHandle,
  IN       EFI_SYSTEM_TABLE  * CONST SystemTable
  )
{
  EFI_STATUS  Status;
  Status = DeregisterAcpiTableGenerator (&Dbg2Generator);
  DEBUG ((DEBUG_INFO, "DBG2: Deregister Generator. Status = %r\n", Status));
  ASSERT_EFI_ERROR (Status);
  return Status;
}
