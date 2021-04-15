/** @file
  DBG2 Table Generator

  Copyright (c) 2017 - 2021, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - Microsoft Debug Port Table 2 (DBG2) Specification - December 10, 2015.

**/

#include <IndustryStandard/DebugPort2Table.h>
#include <Library/AcpiLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PL011UartLib.h>
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

/** The index for the debug port 0 in the Debug port information list.
*/
#define INDEX_DBG_PORT0                            0

/** A string representing the name of the debug port 0.
*/
#define NAME_STR_DBG_PORT0                         "COM0"

/** An UID representing the debug port 0.
*/
#define UID_DBG_PORT0                              0

/** The length of the namespace string.
*/
#define DBG2_NAMESPACESTRING_FIELD_SIZE            sizeof (NAME_STR_DBG_PORT0)

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
      NAME_STR_DBG_PORT0
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
  IN      CONST ACPI_TABLE_GENERATOR                   * CONST This,
  IN      CONST CM_STD_OBJ_ACPI_TABLE_INFO             * CONST AcpiTableInfo,
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL   * CONST CfgMgrProtocol,
  IN OUT        EFI_ACPI_DESCRIPTION_HEADER          *** CONST Table,
  IN      CONST UINTN                                          TableCount
  )
{
  EFI_STATUS                        Status;
  EFI_ACPI_DESCRIPTION_HEADER    ** TableList;

  ASSERT (This != NULL);
  ASSERT (AcpiTableInfo != NULL);
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (AcpiTableInfo->TableGeneratorId == This->GeneratorID);
  ASSERT (AcpiTableInfo->AcpiTableSignature == This->AcpiTableSignature);

  if ((Table == NULL)   ||
      (*Table == NULL)  ||
      (TableCount != 2)) {
    DEBUG ((DEBUG_ERROR, "ERROR: DBG2: Invalid Table Pointer\n"));
    return EFI_INVALID_PARAMETER;
  }

  TableList = *Table;

  if ((TableList[1] == NULL) ||
      (TableList[1]->Signature !=
       EFI_ACPI_6_3_SECONDARY_SYSTEM_DESCRIPTION_TABLE_SIGNATURE)) {
    DEBUG ((DEBUG_ERROR, "ERROR: DBG2: Invalid SSDT table pointer.\n"));
    return EFI_INVALID_PARAMETER;
  }

  // Only need to free the SSDT table at index 1. The DBG2 table is static.
  Status = FreeSsdtSerialPortTable (TableList[1]);
  ASSERT_EFI_ERROR (Status);

  // Free the table list.
  FreePool (*Table);

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
  IN  CONST ACPI_TABLE_GENERATOR                   *       This,
  IN  CONST CM_STD_OBJ_ACPI_TABLE_INFO             * CONST AcpiTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL   * CONST CfgMgrProtocol,
  OUT       EFI_ACPI_DESCRIPTION_HEADER          ***       Table,
  OUT       UINTN                                  * CONST TableCount
  )
{
  EFI_STATUS                      Status;
  CM_ARM_SERIAL_PORT_INFO       * SerialPortInfo;
  UINT32                          SerialPortCount;
  EFI_ACPI_DESCRIPTION_HEADER  ** TableList;

  ASSERT (This != NULL);
  ASSERT (AcpiTableInfo != NULL);
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (Table != NULL);
  ASSERT (TableCount != NULL);
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
             &SerialPortCount
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: DBG2: Failed to get serial port information. Status = %r\n",
      Status
      ));
    return Status;
  }

  if (SerialPortCount == 0) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: DBG2: Serial port information not found. Status = %r\n",
      EFI_NOT_FOUND
      ));
    return EFI_NOT_FOUND;
  }

  // Only use the first DBG2 port information.
  Status = ValidateSerialPortInfo (SerialPortInfo, 1);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: DBG2: Invalid serial port information. Status = %r\n",
      Status
      ));
    return Status;
  }

  // Allocate a table to store pointers to the DBG2 and SSDT tables.
  TableList = (EFI_ACPI_DESCRIPTION_HEADER**)
              AllocateZeroPool (sizeof (EFI_ACPI_DESCRIPTION_HEADER*) * 2);
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
  AcpiDbg2.Dbg2DeviceInfo[INDEX_DBG_PORT0].BaseAddressRegister.Address =
    SerialPortInfo->BaseAddress;

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
    AcpiDbg2.Dbg2DeviceInfo[INDEX_DBG_PORT0].BaseAddressRegister.AccessSize =
      EFI_ACPI_6_3_DWORD;
  } else {
    AcpiDbg2.Dbg2DeviceInfo[INDEX_DBG_PORT0].BaseAddressRegister.AccessSize =
      SerialPortInfo->AccessSize;
  }

  // Update the serial port subtype
  AcpiDbg2.Dbg2DeviceInfo[INDEX_DBG_PORT0].Dbg2Device.PortSubtype =
    SerialPortInfo->PortSubtype;

  if ((SerialPortInfo->PortSubtype ==
      EFI_ACPI_DBG2_PORT_SUBTYPE_SERIAL_ARM_PL011_UART)           ||
      (SerialPortInfo->PortSubtype ==
      EFI_ACPI_DBG2_PORT_SUBTYPE_SERIAL_ARM_SBSA_GENERIC_UART_2X) ||
      (SerialPortInfo->PortSubtype ==
      EFI_ACPI_DBG2_PORT_SUBTYPE_SERIAL_ARM_SBSA_GENERIC_UART)) {
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

  TableList[0] = (EFI_ACPI_DESCRIPTION_HEADER*)&AcpiDbg2;

  // Build a SSDT table describing the serial port.
  Status = BuildSsdtSerialPortTable (
             AcpiTableInfo,
             SerialPortInfo,
             NAME_STR_DBG_PORT0,
             UID_DBG_PORT0,
             &TableList[1]
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: DBG2: Failed to build associated SSDT table. Status = %r\n",
      Status
      ));
    goto error_handler;
  }

  *TableCount = 2;
  *Table = TableList;

  return Status;

error_handler:
  if (TableList != NULL) {
    FreePool (TableList);
  }

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
  IN  EFI_HANDLE           ImageHandle,
  IN  EFI_SYSTEM_TABLE  *  SystemTable
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
  IN  EFI_HANDLE           ImageHandle,
  IN  EFI_SYSTEM_TABLE  *  SystemTable
  )
{
  EFI_STATUS  Status;
  Status = DeregisterAcpiTableGenerator (&Dbg2Generator);
  DEBUG ((DEBUG_INFO, "DBG2: Deregister Generator. Status = %r\n", Status));
  ASSERT_EFI_ERROR (Status);
  return Status;
}
