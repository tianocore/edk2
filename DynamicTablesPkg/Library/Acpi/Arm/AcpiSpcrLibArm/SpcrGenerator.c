/** @file
  SPCR Table Generator

  Copyright (c) 2017 - 2019, ARM Limited. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - Microsoft Serial Port Console Redirection Table
    Specification - Version 1.03 - August 10, 2015.

**/

#include <IndustryStandard/DebugPort2Table.h>
#include <IndustryStandard/SerialPortConsoleRedirectionTable.h>
#include <Library/AcpiLib.h>
#include <Library/DebugLib.h>
#include <Protocol/AcpiTable.h>

// Module specific include files.
#include <AcpiTableGenerator.h>
#include <ConfigurationManagerObject.h>
#include <ConfigurationManagerHelper.h>
#include <Library/TableHelperLib.h>
#include <Protocol/ConfigurationManagerProtocol.h>

/** ARM standard SPCR Table Generator

  Constructs the SPCR table for PL011 or SBSA UART peripherals.

Requirements:
  The following Configuration Manager Object(s) are required by
  this Generator:
  - EArmObjSerialConsolePortInfo

NOTE: This implementation ignores the possibility that the Serial settings may
      be modified from the UEFI Shell.  A more complex handler would be needed
      to (e.g.) recover serial port settings from the UART, or non-volatile
      storage.
*/

#pragma pack(1)

/** This macro defines the no flow control option.
*/
#define SPCR_FLOW_CONTROL_NONE           0

/**A template for generating the SPCR Table.

  Note: fields marked "{Template}" will be updated dynamically.
*/
STATIC
EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE AcpiSpcr = {
  ACPI_HEADER (
    EFI_ACPI_6_2_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_SIGNATURE,
    EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE,
    EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_REVISION
    ),
  0, // {Template}: Serial Port Subtype
  {
    EFI_ACPI_RESERVED_BYTE,
    EFI_ACPI_RESERVED_BYTE,
    EFI_ACPI_RESERVED_BYTE
  },
  ARM_GAS32 (0), // {Template}: Serial Port Base Address
  EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_INTERRUPT_TYPE_GIC,
  0, // Not used on ARM
  0, // {Template}: Serial Port Interrupt
  0, // {Template}: Serial Port Baudrate
  EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_PARITY_NO_PARITY,
  EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_STOP_BITS_1,
  SPCR_FLOW_CONTROL_NONE,
  EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_TERMINAL_TYPE_ANSI,
  EFI_ACPI_RESERVED_BYTE,
  0xFFFF,
  0xFFFF,
  0x00,
  0x00,
  0x00,
  0x00000000,
  0x00,
  EFI_ACPI_RESERVED_DWORD
};

#pragma pack()

/** This macro expands to a function that retrieves the Serial
    Port Information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArm,
  EArmObjSerialConsolePortInfo,
  CM_ARM_SERIAL_PORT_INFO
  )

/** Construct the SPCR ACPI table.

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
  @retval EFI_UNSUPPORTED       An unsupported baudrate was specified by the
                                Configuration Manager.
  @retval EFI_BAD_BUFFER_SIZE   The size returned by the Configuration
                                Manager is less than the Object size for the
                                requested object.
**/
STATIC
EFI_STATUS
EFIAPI
BuildSpcrTable (
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
      "ERROR: SPCR: Requested table revision = %d, is not supported."
      "Supported table revision: Minimum = %d, Maximum = %d\n",
      AcpiTableInfo->AcpiTableRevision,
      This->MinAcpiTableRevision,
      This->AcpiTableRevision
      ));
    return EFI_INVALID_PARAMETER;
  }

  *Table = NULL;

  Status = GetEArmObjSerialConsolePortInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &SerialPortInfo,
             NULL
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SPCR: Failed to get serial port information. Status = %r\n",
      Status
      ));
    goto error_handler;
  }

  if (SerialPortInfo->BaseAddress == 0) {
    Status = EFI_INVALID_PARAMETER;
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SPCR: Uart port base address is invalid. BaseAddress = 0x%lx\n",
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
      EFI_ACPI_DBG2_PORT_SUBTYPE_SERIAL_FULL_16550) &&
      (SerialPortInfo->PortSubtype !=
      EFI_ACPI_DBG2_PORT_SUBTYPE_SERIAL_DCC)) {
    Status = EFI_INVALID_PARAMETER;
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SPCR: Uart port sybtype is invalid. PortSubtype = 0x%x\n",
      SerialPortInfo->PortSubtype
      ));
    goto error_handler;
  }

  DEBUG ((DEBUG_INFO, "SPCR UART Configuration:\n"));
  DEBUG ((DEBUG_INFO, "  UART Base  = 0x%lx\n", SerialPortInfo->BaseAddress));
  DEBUG ((DEBUG_INFO, "  Clock      = %d\n", SerialPortInfo->Clock));
  DEBUG ((DEBUG_INFO, "  Baudrate   = %ld\n", SerialPortInfo->BaudRate));
  DEBUG ((DEBUG_INFO, "  Interrupt  = %d\n", SerialPortInfo->Interrupt));

  Status = AddAcpiHeader (
             CfgMgrProtocol,
             This,
             (EFI_ACPI_DESCRIPTION_HEADER*)&AcpiSpcr,
             AcpiTableInfo,
             sizeof (EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE)
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SPCR: Failed to add ACPI header. Status = %r\n",
      Status
      ));
    goto error_handler;
  }

  // Update the serial port subtype
  AcpiSpcr.InterfaceType = SerialPortInfo->PortSubtype;

  // Update the base address
  AcpiSpcr.BaseAddress.Address = SerialPortInfo->BaseAddress;

  // Update the UART interrupt
  AcpiSpcr.GlobalSystemInterrupt = SerialPortInfo->Interrupt;

  switch (SerialPortInfo->BaudRate) {
    case 9600:
      AcpiSpcr.BaudRate =
        EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_BAUD_RATE_9600;
      break;
    case 19200:
      AcpiSpcr.BaudRate =
        EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_BAUD_RATE_19200;
      break;
    case 57600:
      AcpiSpcr.BaudRate =
        EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_BAUD_RATE_57600;
      break;
    case 115200:
      AcpiSpcr.BaudRate =
        EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_BAUD_RATE_115200;
      break;
    default:
      Status = EFI_UNSUPPORTED;
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SPCR: Invalid Baud Rate %ld, Status = %r\n",
        SerialPortInfo->BaudRate,
        Status
        ));
      goto error_handler;
  } // switch

  *Table = (EFI_ACPI_DESCRIPTION_HEADER*)&AcpiSpcr;

error_handler:
  return Status;
}

/** This macro defines the SPCR Table Generator revision.
*/
#define SPCR_GENERATOR_REVISION CREATE_REVISION (1, 0)

/** The interface for the SPCR Table Generator.
*/
STATIC
CONST
ACPI_TABLE_GENERATOR SpcrGenerator = {
  // Generator ID
  CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdSpcr),
  // Generator Description
  L"ACPI.STD.SPCR.GENERATOR",
  // ACPI Table Signature
  EFI_ACPI_6_2_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_SIGNATURE,
  // ACPI Table Revision supported by this Generator
  EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_REVISION,
  // Minimum supported ACPI Table Revision
  EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_REVISION,
  // Creator ID
  TABLE_GENERATOR_CREATOR_ID_ARM,
  // Creator Revision
  SPCR_GENERATOR_REVISION,
  // Build Table function
  BuildSpcrTable,
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
AcpiSpcrLibConstructor (
  IN CONST EFI_HANDLE                ImageHandle,
  IN       EFI_SYSTEM_TABLE  * CONST SystemTable
  )
{
  EFI_STATUS  Status;
  Status = RegisterAcpiTableGenerator (&SpcrGenerator);
  DEBUG ((DEBUG_INFO, "SPCR: Register Generator. Status = %r\n", Status));
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
AcpiSpcrLibDestructor (
  IN CONST EFI_HANDLE                ImageHandle,
  IN       EFI_SYSTEM_TABLE  * CONST SystemTable
  )
{
  EFI_STATUS  Status;
  Status = DeregisterAcpiTableGenerator (&SpcrGenerator);
  DEBUG ((DEBUG_INFO, "SPCR: Deregister Generator. Status = %r\n", Status));
  ASSERT_EFI_ERROR (Status);
  return Status;
}
