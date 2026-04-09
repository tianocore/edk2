/** @file
  SPCR Table Generator

  Copyright (c) 2017 - 2021, Arm Limited. All rights reserved.<BR>
  Copyright (C) 2025, Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - Microsoft Serial Port Console Redirection Table
    Specification - Version 1.03 - August 10, 2015.

**/

#include <IndustryStandard/DebugPort2Table.h>
#include <IndustryStandard/SerialPortConsoleRedirectionTable.h>
#include <Library/AcpiLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Protocol/AcpiTable.h>

// Module specific include files.
#include <AcpiTableGenerator.h>
#include <ConfigurationManagerObject.h>
#include <ConfigurationManagerHelper.h>
#include <Library/SsdtSerialPortFixupLib.h>
#include <Library/TableHelperLib.h>
#include <Protocol/ConfigurationManagerProtocol.h>

/** Standard SPCR Table Generator

  Constructs the SPCR table for PL011 or SBSA UART peripherals.

Requirements:
  The following Configuration Manager Object(s) are required by
  this Generator:
  - EArchCommonObjConsolePortInfo
  - EArchCommonObjSpcrInfo (OPTIONAL)


NOTE: This implementation ignores the possibility that the Serial settings may
      be modified from the UEFI Shell.  A more complex handler would be needed
      to (e.g.) recover serial port settings from the UART, or non-volatile
      storage.
*/

#pragma pack(1)

/** Valid SPCR interrupt types
*/
#define SPCR_VALID_INTERRUPT_TYPES  \
  (EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_INTERRUPT_TYPE_8259 | \
   EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_INTERRUPT_TYPE_APIC | \
   EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_INTERRUPT_TYPE_SAPIC | \
   EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_INTERRUPT_TYPE_GIC)

/** A string representing the name of the SPCR port.
*/
#define NAME_STR_SPCR_PORT  "COM1"

/** An UID representing the SPCR port.
*/
#define UID_SPCR_PORT  1

/** This macro defines the no flow control option.
*/
#define SPCR_FLOW_CONTROL_NONE  0

/**A template for generating the SPCR Table.

  Note: fields marked "{Template}" will be updated dynamically.
*/
STATIC
EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_4  AcpiSpcr = {
  ACPI_HEADER (
    EFI_ACPI_6_2_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_SIGNATURE,
    EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_4,
    EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_REVISION_4
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
  0, // {Template}: UART Clock Frequency
  0, // {Template}: Precise Baud Rate
  sizeof (NAME_STR_SPCR_PORT),
  OFFSET_OF (EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_4, NameSpaceString)
};

#pragma pack()

/** This macro expands to a function that retrieves the Serial
    Port Information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjConsolePortInfo,
  CM_ARCH_COMMON_SERIAL_PORT_INFO
  )

/** This macro expands to a function that retrieves the Serial
  Terminal and Interrupt Information from the Configuration Manager.
  This object is optional.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjSpcrInfo,
  CM_ARCH_COMMON_SPCR_INFO
  )

/** Validate the Serial Port Terminal Interrupt Information.

  @param [in]  SerialPortInfo Pointer to the Serial Port Information.
  @param [in]  SpcrInfo       Pointer to the SPCR additional Information.

  @retval EFI_SUCCESS           The information is valid.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
**/
STATIC
EFI_STATUS
EFIAPI
ValidateSerialTerminalInterruptInfo (
  IN  CM_ARCH_COMMON_SERIAL_PORT_INFO  *SerialPortInfo,
  IN  CM_ARCH_COMMON_SPCR_INFO         *SpcrInfo
  )
{
  EFI_STATUS  Status;

  if ((SerialPortInfo == NULL) || (SpcrInfo == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = EFI_SUCCESS;
  if ((SpcrInfo->TerminalType != EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_TERMINAL_TYPE_VT100) &&
      (SpcrInfo->TerminalType != EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_TERMINAL_TYPE_VT100_PLUS) &&
      (SpcrInfo->TerminalType != EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_TERMINAL_TYPE_VT_UTF8) &&
      (SpcrInfo->TerminalType != EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_TERMINAL_TYPE_ANSI))
  {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SPCR: Invalid terminal type %d for serial terminal.\n",
      SpcrInfo->TerminalType
      ));
    Status |= EFI_INVALID_PARAMETER;
  }

  if ((SerialPortInfo->PortSubtype == EFI_ACPI_DBG2_PORT_SUBTYPE_SERIAL_FULL_16550) &&
      (SerialPortInfo->AccessSize > EFI_ACPI_6_3_UNDEFINED))
  {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SPCR: Access size must be %d for legacy 16550 UART.\n",
      EFI_ACPI_6_3_UNDEFINED
      ));
    Status |= EFI_INVALID_PARAMETER;
  }

  if (SerialPortInfo->PortSubtype == EFI_ACPI_DBG2_PORT_SUBTYPE_SERIAL_16550_WITH_GAS) {
    if ((SerialPortInfo->AccessSize <= EFI_ACPI_6_3_UNDEFINED) ||
        (SerialPortInfo->AccessSize >= EFI_ACPI_6_3_QWORD))
    {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SPCR: Access size must be > %d and < %d for 16550 with GAS.\n",
        EFI_ACPI_6_3_UNDEFINED,
        EFI_ACPI_6_3_QWORD
        ));
      Status |= EFI_INVALID_PARAMETER;
    }
  }

  if (SpcrInfo->InterruptType != 0) {
    // Check that reserved bits [5:7] are zero
    if ((SpcrInfo->InterruptType & ~(SPCR_VALID_INTERRUPT_TYPES)) != 0) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SPCR: Reserved bits [5:7] must be zero in interrupt type (0x%02x).\n",
        SpcrInfo->InterruptType
        ));
      Status |= EFI_INVALID_PARAMETER;
    }

    if ((((SpcrInfo->InterruptType) &
          (EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_INTERRUPT_TYPE_GIC)) != 0) &&
        (SpcrInfo->InterruptType !=
         EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_INTERRUPT_TYPE_GIC))
    {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SPCR: %d GIC interrupt type cannot be combined with others.\n",
        SpcrInfo->InterruptType
        ));
      Status |= EFI_INVALID_PARAMETER;
    }

    if ((SpcrInfo->InterruptType &
         EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_INTERRUPT_TYPE_APIC) != 0)
    {
      if ((SpcrInfo->InterruptType &
           ~(EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_INTERRUPT_TYPE_8259 |
             EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_INTERRUPT_TYPE_APIC)) != 0)
      {
        DEBUG ((
          DEBUG_ERROR,
          "ERROR: SPCR: %d Invalid APIC interrupt type.\n",
          SpcrInfo->InterruptType
          ));
        Status |= EFI_INVALID_PARAMETER;
      }
    }

    if ((SpcrInfo->InterruptType &
         EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_INTERRUPT_TYPE_SAPIC) != 0)
    {
      if ((SpcrInfo->InterruptType &
           ~(EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_INTERRUPT_TYPE_8259 | \
             EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_INTERRUPT_TYPE_SAPIC)) != 0)
      {
        DEBUG ((
          DEBUG_ERROR,
          "ERROR: SPCR: %d Invalid SAPIC interrupt type.\n",
          SpcrInfo->InterruptType
          ));
        Status |= EFI_INVALID_PARAMETER;
      }
    }

    if ((SpcrInfo->InterruptType & EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_INTERRUPT_TYPE_8259) != 0) {
      // For legacy devices, only GSIs below 32 are used.
      // Valid IRQs: 2-7, 9-12, 14-15. Reserved: 0-1, 8, 13, 16-255.
      if (!(((SerialPortInfo->Interrupt >= 2) && (SerialPortInfo->Interrupt <= 7)) ||
            ((SerialPortInfo->Interrupt >= 9) && (SerialPortInfo->Interrupt <= 12)) ||
            ((SerialPortInfo->Interrupt >= 14) && (SerialPortInfo->Interrupt <= 15))))
      {
        DEBUG ((
          DEBUG_ERROR,
          "ERROR: SPCR: Invalid IRQ %d for 8259 interrupt type.\n",
          SerialPortInfo->Interrupt
          ));
        Status |= EFI_INVALID_PARAMETER;
      }
    }
  }

  return Status;
}

/** Update the SPCR table with the terminal interrupt type and terminal information.

  @param [in, out] SpcrTable  Pointer to the SPCR table.
  @param [in]      SpcrInfo   Pointer to the Serial Terminal Interrupt Information.
  @param [in]      SerialPortInfo Pointer to the Serial Port Information.

  @retval EFI_SUCCESS           The information was updated successfully.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
**/
STATIC
EFI_STATUS
EFIAPI
UpdateTerminalInterruptTypeInfo (
  IN OUT EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_4  *SpcrTable,
  IN     CM_ARCH_COMMON_SPCR_INFO                          *SpcrInfo,
  IN     CM_ARCH_COMMON_SERIAL_PORT_INFO                   *SerialPortInfo
  )
{
  if ((SpcrTable == NULL) || (SpcrInfo == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  SpcrTable->InterruptType = SpcrInfo->InterruptType;
  SpcrTable->TerminalType  = SpcrInfo->TerminalType;

  if ((SpcrInfo->InterruptType &
       EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_INTERRUPT_TYPE_8259) != 0)
  {
    SpcrTable->Irq = (SerialPortInfo->Interrupt & MAX_UINT8);
  }

  return EFI_SUCCESS;
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
FreeSpcrTableEx (
  IN      CONST ACPI_TABLE_GENERATOR                   *CONST  This,
  IN      CONST CM_STD_OBJ_ACPI_TABLE_INFO             *CONST  AcpiTableInfo,
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL   *CONST  CfgMgrProtocol,
  IN OUT        EFI_ACPI_DESCRIPTION_HEADER          ***CONST  Table,
  IN      CONST UINTN                                          TableCount
  )
{
  EFI_STATUS                   Status;
  EFI_ACPI_DESCRIPTION_HEADER  **TableList;

  ASSERT (This != NULL);
  ASSERT (AcpiTableInfo != NULL);
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (AcpiTableInfo->TableGeneratorId == This->GeneratorID);
  ASSERT (AcpiTableInfo->AcpiTableSignature == This->AcpiTableSignature);

  if ((Table == NULL)   ||
      (*Table == NULL)  ||
      (TableCount != 2))
  {
    DEBUG ((DEBUG_ERROR, "ERROR: SPCR: Invalid Table Pointer\n"));
    return EFI_INVALID_PARAMETER;
  }

  TableList = *Table;

  if ((TableList[1] == NULL) ||
      (TableList[1]->Signature !=
       EFI_ACPI_6_3_SECONDARY_SYSTEM_DESCRIPTION_TABLE_SIGNATURE))
  {
    DEBUG ((DEBUG_ERROR, "ERROR: SPCR: Invalid SSDT table pointer.\n"));
    return EFI_INVALID_PARAMETER;
  }

  // Only need to free the SSDT table at index 1. The SPCR table is static.
  Status = FreeSsdtSerialPortTable (TableList[1]);
  ASSERT_EFI_ERROR (Status);

  // Free the table list.
  FreePool (*Table);

  return Status;
}

/** Construct the SPCR ACPI table and its associated SSDT table.

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
BuildSpcrTableEx (
  IN  CONST ACPI_TABLE_GENERATOR                           *This,
  IN  CONST CM_STD_OBJ_ACPI_TABLE_INFO             *CONST  AcpiTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL   *CONST  CfgMgrProtocol,
  OUT       EFI_ACPI_DESCRIPTION_HEADER                    ***Table,
  OUT       UINTN                                  *CONST  TableCount
  )
{
  EFI_STATUS                       Status;
  CM_ARCH_COMMON_SERIAL_PORT_INFO  *SerialPortInfo;
  UINT32                           SerialPortCount;
  EFI_ACPI_DESCRIPTION_HEADER      **TableList;
  UINT32                           Size;
  CM_ARCH_COMMON_SPCR_INFO         *SpcrInfo;
  UINT32                           SpcrInfoCount;

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
      "ERROR: SPCR: Requested table revision = %d, is not supported."
      "Supported table revision: Minimum = %d, Maximum = %d\n",
      AcpiTableInfo->AcpiTableRevision,
      This->MinAcpiTableRevision,
      This->AcpiTableRevision
      ));
    return EFI_INVALID_PARAMETER;
  }

  *Table = NULL;

  Status = GetEArchCommonObjConsolePortInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &SerialPortInfo,
             &SerialPortCount
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SPCR: Failed to get serial port information. Status = %r\n",
      Status
      ));
    return Status;
  }

  if (SerialPortCount == 0) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SPCR: Serial port information not found. Status = %r\n",
      EFI_NOT_FOUND
      ));
    return EFI_NOT_FOUND;
  }

  SpcrInfo      = NULL;
  SpcrInfoCount = 0;
  Status        = GetEArchCommonObjSpcrInfo (
                    CfgMgrProtocol,
                    CM_NULL_TOKEN,
                    &SpcrInfo,
                    &SpcrInfoCount
                    );

  if (!EFI_ERROR (Status)) {
    Status = ValidateSerialTerminalInterruptInfo (
               SerialPortInfo,
               SpcrInfo
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SPCR: Invalid serial terminal interrupt information. Status = %r\n",
        Status
        ));
      return Status;
    }
  } else {
    DEBUG ((
      DEBUG_VERBOSE,
      "SPCR: Continue with default interrupt type (0x%x) and terminal type (0x%x).\n",
      EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_INTERRUPT_TYPE_GIC,
      EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_TERMINAL_TYPE_ANSI
      ));
  }

  // Validate the SerialPort info. Only one SPCR port can be described.
  // If platform provides description for multiple SPCR ports, use the
  // first SPCR port information.
  Status = ValidateSerialPortInfo (SerialPortInfo, 1);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SPCR: Invalid serial port information. Status = %r\n",
      Status
      ));
    return Status;
  }

  // Allocate a table to store pointers to the SPCR and SSDT tables.
  TableList = (EFI_ACPI_DESCRIPTION_HEADER **)
              AllocateZeroPool (sizeof (EFI_ACPI_DESCRIPTION_HEADER *) * 2);
  if (TableList == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SPCR: Failed to allocate memory for Table List," \
      " Status = %r\n",
      Status
      ));
    return Status;
  }

  if (AcpiTableInfo->AcpiTableRevision < EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_REVISION_4) {
    Size = sizeof (EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE);
  } else {
    Size = sizeof (EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_4) + sizeof (NAME_STR_SPCR_PORT);
  }

  // Build SPCR table.
  Status = AddAcpiHeader (
             CfgMgrProtocol,
             This,
             (EFI_ACPI_DESCRIPTION_HEADER *)&AcpiSpcr,
             AcpiTableInfo,
             Size
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SPCR: Failed to add ACPI header. Status = %r\n",
      Status
      ));
    goto error_handler;
  }

  // The SPCR InterfaceType uses the same encoding as that of the
  // DBG2 table Port Subtype field. However InterfaceType is 8-bit
  // while the Port Subtype field in the DBG2 table is 16-bit.
  if ((SerialPortInfo->PortSubtype & 0xFF00) != 0) {
    Status = EFI_INVALID_PARAMETER;
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SPCR: Invalid Port subtype (must be < 256). Status = %r\n",
      Status
      ));
    goto error_handler;
  }

  // Update the serial port subtype
  AcpiSpcr.InterfaceType = (UINT8)SerialPortInfo->PortSubtype;

  // Update the base address
  AcpiSpcr.BaseAddress.Address = SerialPortInfo->BaseAddress;

  // Set the access size
  if (SerialPortInfo->AccessSize >= EFI_ACPI_6_3_QWORD) {
    Status = EFI_INVALID_PARAMETER;
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SPCR: Access size must be <= 3 (DWORD). Status = %r\n",
      Status
      ));
    goto error_handler;
  } else if (SerialPortInfo->AccessSize == EFI_ACPI_6_3_UNDEFINED) {
    // 0 Undefined (legacy reasons)
    // Default to DWORD access size as the access
    // size field was introduced at a later date
    // and some ConfigurationManager implementations
    // may not be providing this field data
    AcpiSpcr.BaseAddress.AccessSize = EFI_ACPI_6_3_DWORD;
    if (SerialPortInfo->PortSubtype == EFI_ACPI_DBG2_PORT_SUBTYPE_SERIAL_FULL_16550) {
      AcpiSpcr.BaseAddress.AddressSpaceId    = EFI_ACPI_6_3_SYSTEM_IO;
      AcpiSpcr.BaseAddress.RegisterBitWidth  = 8;
      AcpiSpcr.BaseAddress.RegisterBitOffset = 0;
      AcpiSpcr.BaseAddress.AccessSize        = EFI_ACPI_6_3_UNDEFINED;
    }
  } else {
    AcpiSpcr.BaseAddress.AccessSize = SerialPortInfo->AccessSize;
  }

  // Update the UART interrupt
  AcpiSpcr.GlobalSystemInterrupt = SerialPortInfo->Interrupt;

  switch (SerialPortInfo->BaudRate) {
    case 0:
      // Baud rate 0 indicates that the OS should rely on the current UART
      // configuration until the full driver is initialized.
      AcpiSpcr.BaudRate =
        EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_BAUD_RATE_AS_IS;
      break;
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
      if (AcpiTableInfo->AcpiTableRevision < EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_REVISION_4) {
        Status = EFI_UNSUPPORTED;
        DEBUG ((
          DEBUG_ERROR,
          "ERROR: SPCR: Invalid Baud Rate %ld, Status = %r\n",
          SerialPortInfo->BaudRate,
          Status
          ));
        goto error_handler;
      }

      AcpiSpcr.BaudRate        = 0;
      AcpiSpcr.PreciseBaudRate = (UINT32)SerialPortInfo->BaudRate;
      break;
  } // switch

  if (AcpiTableInfo->AcpiTableRevision > EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_REVISION) {
    // UartClockFrequency supported on revision 3 and higher
    AcpiSpcr.UartClockFrequency = SerialPortInfo->Clock;

    AsciiStrCpyS (AcpiSpcr.NameSpaceString, sizeof (NAME_STR_SPCR_PORT), NAME_STR_SPCR_PORT);
  }

  if (SpcrInfoCount != 0) {
    Status = UpdateTerminalInterruptTypeInfo (
               &AcpiSpcr,
               SpcrInfo,
               SerialPortInfo
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SPCR: Failed to update interrupt terminal type info. Status = %r\n",
        Status
        ));
      goto error_handler;
    }
  }

  TableList[0] = (EFI_ACPI_DESCRIPTION_HEADER *)&AcpiSpcr;

  // Build a SSDT table describing the serial port.
  Status = BuildSsdtSerialPortTable (
             AcpiTableInfo,
             SerialPortInfo,
             NAME_STR_SPCR_PORT,
             UID_SPCR_PORT,
             &TableList[1]
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SPCR: Failed to build associated SSDT table. Status = %r\n",
      Status
      ));
    goto error_handler;
  }

  *TableCount = 2;
  *Table      = TableList;

  return Status;

error_handler:
  if (TableList != NULL) {
    FreePool (TableList);
  }

  return Status;
}

/** This macro defines the SPCR Table Generator revision.
*/
#define SPCR_GENERATOR_REVISION  CREATE_REVISION (1, 0)

/** The interface for the SPCR Table Generator.
*/
STATIC
CONST
ACPI_TABLE_GENERATOR  SpcrGenerator = {
  // Generator ID
  CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdSpcr),
  // Generator Description
  L"ACPI.STD.SPCR.GENERATOR",
  // ACPI Table Signature
  EFI_ACPI_6_3_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_SIGNATURE,
  // ACPI Table Revision supported by this Generator
  EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_REVISION_4,
  // Minimum supported ACPI Table Revision
  EFI_ACPI_SERIAL_PORT_CONSOLE_REDIRECTION_TABLE_REVISION,
  // Creator ID
  TABLE_GENERATOR_CREATOR_ID,
  // Creator Revision
  SPCR_GENERATOR_REVISION,
  // Build table function. Use the extended version instead.
  NULL,
  // Free table function. Use the extended version instead.
  NULL,
  // Extended Build table function.
  BuildSpcrTableEx,
  // Extended free function.
  FreeSpcrTableEx
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
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
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
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = DeregisterAcpiTableGenerator (&SpcrGenerator);
  DEBUG ((DEBUG_INFO, "SPCR: Deregister Generator. Status = %r\n", Status));
  ASSERT_EFI_ERROR (Status);
  return Status;
}
