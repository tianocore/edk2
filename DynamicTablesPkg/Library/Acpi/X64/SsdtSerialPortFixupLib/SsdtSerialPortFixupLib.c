/** @file
  SSDT Serial Port Fixup Library for X64.


  Copyright (c) 2019 - 2024, Arm Limited. All rights reserved.<BR>
  Copyright (C) 2025 Advanced Micro Devices, Inc. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - Arm Server Base Boot Requirements (SBBR), s4.2.1.8 "SPCR".
  - Microsoft Debug Port Table 2 (DBG2) Specification - December 10, 2015.
  - ACPI for Arm Components 1.0 - 2020
  - Arm Generic Interrupt Controller Architecture Specification,
    Issue H, January 2022.
    (https://developer.arm.com/documentation/ihi0069/)
**/

#include <IndustryStandard/DebugPort2Table.h>
#include <Library/AcpiLib.h>
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
#include <Protocol/ConfigurationManagerProtocol.h>

/** UART address range length.
*/
#define MIN_UART_ADDRESS_LENGTH  0x1000U

/** Validate the Serial Port Information.

  @param [in]  SerialPortInfoTable    Table of CM_ARCH_COMMON_SERIAL_PORT_INFO.
  @param [in]  SerialPortCount        Count of SerialPort in the table.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
ValidateSerialPortInfo (
  IN  CONST CM_ARCH_COMMON_SERIAL_PORT_INFO  *SerialPortInfoTable,
  IN        UINT32                           SerialPortCount
  )
{
  UINT32                                 Index;
  CONST CM_ARCH_COMMON_SERIAL_PORT_INFO  *SerialPortInfo;

  if ((SerialPortInfoTable == NULL)  ||
      (SerialPortCount == 0))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  for (Index = 0; Index < SerialPortCount; Index++) {
    SerialPortInfo = &SerialPortInfoTable[Index];
    ASSERT (SerialPortInfo != NULL);

    if ((SerialPortInfo == NULL) ||
        (SerialPortInfo->BaseAddress == 0))
    {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: UART port base address is invalid. BaseAddress = 0x%llx\n",
        SerialPortInfo->BaseAddress
        ));
      return EFI_INVALID_PARAMETER;
    }

    if ((SerialPortInfo->PortSubtype !=
         EFI_ACPI_DBG2_PORT_SUBTYPE_SERIAL_ARM_PL011_UART) &&
        (SerialPortInfo->PortSubtype !=
         EFI_ACPI_DBG2_PORT_SUBTYPE_SERIAL_ARM_SBSA_GENERIC_UART_2X) &&
        (SerialPortInfo->PortSubtype !=
         EFI_ACPI_DBG2_PORT_SUBTYPE_SERIAL_ARM_SBSA_GENERIC_UART) &&
        (SerialPortInfo->PortSubtype !=
         EFI_ACPI_DBG2_PORT_SUBTYPE_SERIAL_DCC) &&
        (SerialPortInfo->PortSubtype !=
         EFI_ACPI_DBG2_PORT_SUBTYPE_SERIAL_FULL_16550) &&
        (SerialPortInfo->PortSubtype !=
         EFI_ACPI_DBG2_PORT_SUBTYPE_SERIAL_16550_WITH_GAS))
    {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: UART port subtype is invalid."
        " UART Base  = 0x%llx, PortSubtype = 0x%x\n",
        SerialPortInfo->BaseAddress,
        SerialPortInfo->PortSubtype
        ));
      return EFI_INVALID_PARAMETER;
    }

    DEBUG ((DEBUG_INFO, "UART Configuration:\n"));
    DEBUG ((
      DEBUG_INFO,
      "  UART Base  = 0x%llx\n",
      SerialPortInfo->BaseAddress
      ));
    DEBUG ((
      DEBUG_INFO,
      "  Length     = 0x%llx\n",
      SerialPortInfo->BaseAddressLength
      ));
    DEBUG ((DEBUG_INFO, "  Clock      = %lu\n", SerialPortInfo->Clock));
    DEBUG ((DEBUG_INFO, "  BaudRate   = %llu\n", SerialPortInfo->BaudRate));
    DEBUG ((DEBUG_INFO, "  Interrupt  = %lu\n", SerialPortInfo->Interrupt));
  } // for

  return EFI_SUCCESS;
}

/**
  Create the _CRS (Current Resource Settings) AML node for a serial port device.

  @param [in]  SerialPortInfo   Pointer to the serial port information structure.
  @param [in]  Name             The Name to give to the Device.
                                Must be a NULL-terminated ASL NameString
                                e.g.: "DEV0", "DV15.DEV0", etc.
  @param [in]  DeviceNode       AML device node handle.

  @retval EFI_SUCCESS           The CRS node was created successfully.
  @retval EFI_INVALID_PARAMETER A parameter is invalid.
  @retval Others                Failed to create CRS node.
**/
STATIC
EFI_STATUS
EFIAPI
CreateSerialPortCrs (
  IN  CONST CM_ARCH_COMMON_SERIAL_PORT_INFO  *SerialPortInfo,
  IN  CONST CHAR8                            *Name,
  IN  AML_OBJECT_NODE_HANDLE                 DeviceNode
  )
{
  AML_OBJECT_NODE_HANDLE  CrsNode;
  EFI_STATUS              Status;
  UINT8                   IrqList[1];

  Status = AmlCodeGenNameResourceTemplate ("_CRS", DeviceNode, &CrsNode);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-SERIAL-PORT-FIXUP: Failed to create AML _CRS Node."
      " Status = %r\n",
      Status
      ));
    return Status;
  }

  IrqList[0] = SerialPortInfo->Interrupt & MAX_UINT8;

  if (SerialPortInfo->PortSubtype == EFI_ACPI_DBG2_PORT_SUBTYPE_SERIAL_FULL_16550) {
    Status = AmlCodeGenRdIo (
               TRUE,
               SerialPortInfo->BaseAddress & MAX_UINT16,
               SerialPortInfo->BaseAddress & MAX_UINT16,
               1,
               0x8,
               CrsNode,
               NULL
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-SERIAL-PORT-FIXUP: Failed to generate IO RD node."
        " Status = %r\n",
        Status
        ));
      return Status;
    }

    //
    // Generate the IRQ() ASL macro.
    // This is used for legacy X86/X64/PC-AT compatible systems.
    //
    Status = AmlCodeGenRdIrq (
               TRUE,
               TRUE,
               TRUE,
               IrqList,
               ARRAY_SIZE (IrqList),
               CrsNode,
               NULL
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-SERIAL-PORT-FIXUP: Failed to generate IRQ RD node."
        " Status = %r\n",
        Status
        ));
      return Status;
    }

    //
    //  Generate the UARTSerialBusV2() ASL macro.
    //  This describes legacy COM port resources for X86/X64/PC-AT compatible systems.
    //
    Status = AmlCodeGenRdUartSerialBusV2 (
               SerialPortInfo->BaudRate & MAX_UINT32, // BaudRate
               NULL,                                  // Default 8 Bits Per Byte
               NULL,                                  // Default 1 Stop Bit
               0,                                     // Lines in Use
               NULL,                                  // Default is little endian
               NULL,                                  // Default is no parity
               NULL,                                  // Default is no flow control
               0x1,                                   // ReceiveBufferSize
               0x1,                                   // TransmitBufferSize
               (CHAR8 *)Name,                         // Serial Port Name
               (AsciiStrLen (Name) + 1) & MAX_UINT16, // Serial Port Name Length
               NULL,                                  // Default resource index is zero
               NULL,                                  // Default is consumer
               NULL,                                  // Default is exclusive
               NULL,                                  // vendor defined data
               0,                                     // VendorDefinedDataLength
               CrsNode,
               NULL
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-SERIAL-PORT-FIXUP: Failed to generate UartSerialBus RD node."
        " Status = %r\n",
        Status
        ));
      return Status;
    }
  }

  if (SerialPortInfo->PortSubtype == EFI_ACPI_DBG2_PORT_SUBTYPE_SERIAL_16550_WITH_GAS) {
    Status = AmlCodeGenRdMemory32Fixed (
               TRUE,
               SerialPortInfo->BaseAddress & MAX_UINT32,
               ((SerialPortInfo->BaseAddressLength > MIN_UART_ADDRESS_LENGTH)
                 ? SerialPortInfo->BaseAddressLength
                 : MIN_UART_ADDRESS_LENGTH) & MAX_UINT32,
               CrsNode,
               NULL
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-SERIAL-PORT-FIXUP: Failed to generate MMIO RD node."
        " Status = %r\n",
        Status
        ));
      return Status;
    }

    Status = AmlCodeGenRdIrq (
               TRUE,
               TRUE,
               TRUE,
               IrqList,
               1,
               CrsNode,
               NULL
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-SERIAL-PORT-FIXUP: Failed to generate IRQ RD node."
        " Status = %r\n",
        Status
        ));
      return Status;
    }
  }

  return EFI_SUCCESS;
}

/** Build a SSDT table describing the input serial port.

  The table created by this function must be freed by FreeSsdtSerialTable.

  @param [in]  AcpiTableInfo    Pointer to the ACPI table information.
  @param [in]  SerialPortInfo   Serial port to describe in the SSDT table.
  @param [in]  Name             The Name to give to the Device.
                                Must be a NULL-terminated ASL NameString
                                e.g.: "DEV0", "DV15.DEV0", etc.
  @param [in]  Uid              UID for the Serial Port.
  @param [out] Table            If success, pointer to the created SSDT table.

  @retval EFI_SUCCESS            Table generated successfully.
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.
  @retval EFI_NOT_FOUND          Could not find information.
  @retval EFI_OUT_OF_RESOURCES   Could not allocate memory.
**/
EFI_STATUS
EFIAPI
BuildSsdtSerialPortTable (
  IN  CONST CM_STD_OBJ_ACPI_TABLE_INFO       *AcpiTableInfo,
  IN  CONST CM_ARCH_COMMON_SERIAL_PORT_INFO  *SerialPortInfo,
  IN  CONST CHAR8                            *Name,
  IN  CONST UINT64                           Uid,
  OUT       EFI_ACPI_DESCRIPTION_HEADER      **Table
  )
{
  AML_OBJECT_NODE_HANDLE  DeviceNode;
  AML_OBJECT_NODE_HANDLE  ScopeNode;
  AML_ROOT_NODE_HANDLE    RootNode;
  CONST CHAR8             *NonBsaHid;
  EFI_STATUS              Status;
  EFI_STATUS              Status1;
  UINT32                  EisaId;

  ASSERT (AcpiTableInfo != NULL);
  ASSERT (SerialPortInfo != NULL);
  ASSERT (Name != NULL);
  ASSERT (Table != NULL);

  // Validate the Serial Port Info.
  Status = ValidateSerialPortInfo (SerialPortInfo, 1);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = AmlCodeGenDefinitionBlock (
             "SSDT",
             "AMDINC",
             "SERIAL",
             0x01,
             &RootNode
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-SERIAL-PORT-FIXUP: Failed to create AML Definition Block."
      " Status = %r\n",
      Status
      ));
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  Status = AmlCodeGenScope ("\\_SB_", RootNode, &ScopeNode);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-SERIAL-PORT-FIXUP: Failed to create AML Scope Node."
      " Status = %r\n",
      Status
      ));
    goto exit_handler;
  }

  // Create the Device Node, COMx, where x is the Uid.
  Status = AmlCodeGenDevice (Name, ScopeNode, &DeviceNode);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-SERIAL-PORT-FIXUP: Failed to create AML Device Node."
      " Status = %r\n",
      Status
      ));
    goto exit_handler;
  }

  NonBsaHid = (CONST CHAR8 *)PcdGetPtr (PcdNonBsaCompliant16550SerialHid);
  if (SerialPortInfo->PortSubtype == EFI_ACPI_DBG2_PORT_SUBTYPE_SERIAL_16550_WITH_GAS) {
    if ((NonBsaHid != NULL) && (AsciiStrLen (NonBsaHid) != 0)) {
      if (!(IsValidPnpId (NonBsaHid) || IsValidAcpiId (NonBsaHid))) {
        DEBUG ((
          DEBUG_ERROR,
          "ERROR: SSDT-SERIAL-PORT-FIXUP: Invalid Supplied HID %a.\n",
          NonBsaHid
          ));
        goto exit_handler;
      }

      Status = AmlCodeGenNameString (
                 "_HID",
                 NonBsaHid,
                 DeviceNode,
                 NULL
                 );
      if (EFI_ERROR (Status)) {
        DEBUG ((
          DEBUG_ERROR,
          "ERROR: SSDT-SERIAL-PORT-FIXUP: Failed to create AML _HID Node."
          " Status = %r\n",
          Status
          ));
        goto exit_handler;
      }
    }
  }

  if ((SerialPortInfo->PortSubtype == EFI_ACPI_DBG2_PORT_SUBTYPE_SERIAL_FULL_16550) ||
      ((SerialPortInfo->PortSubtype == EFI_ACPI_DBG2_PORT_SUBTYPE_SERIAL_16550_WITH_GAS) &&
       ((NonBsaHid == NULL) || (AsciiStrLen (NonBsaHid) == 0))))
  {
    Status = AmlGetEisaIdFromString ("PNP0501", &EisaId);
    if (EFI_ERROR (Status)) {
      goto exit_handler;
    }

    Status = AmlCodeGenNameInteger ("_HID", EisaId, DeviceNode, NULL);
    if (EFI_ERROR (Status)) {
      goto exit_handler;
    }

    Status = AmlGetEisaIdFromString ("PNP0500", &EisaId);
    if (EFI_ERROR (Status)) {
      goto exit_handler;
    }

    Status = AmlCodeGenNameInteger ("_CID", EisaId, DeviceNode, NULL);
    if (EFI_ERROR (Status)) {
      goto exit_handler;
    }
  }

  // _UID
  Status = AmlCodeGenNameInteger ("_UID", Uid, DeviceNode, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-SERIAL-PORT-FIXUP: Failed to create AML _UID Node."
      " Status = %r\n",
      Status
      ));
    goto exit_handler;
  }

  // _DDN
  Status = AmlCodeGenNameString ("_DDN", Name, DeviceNode, NULL);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-SERIAL-PORT-FIXUP: Failed to create AML _DDN Node."
      " Status = %r\n",
      Status
      ));
    goto exit_handler;
  }

  // _STA
  Status = AmlCodeGenMethodRetInteger (
             "_STA",
             0x0F,
             0,
             FALSE,
             0,
             DeviceNode,
             NULL
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-SERIAL-PORT-FIXUP: Failed to create AML _STA Node."
      " Status = %r\n",
      Status
      ));
    goto exit_handler;
  }

  Status = CreateSerialPortCrs (SerialPortInfo, Name, DeviceNode);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-SERIAL-PORT-FIXUP: Failed to create _CRS for Serial Port."
      " Status = %r\n",
      Status
      ));
    goto exit_handler;
  }

  // Serialize the tree.
  Status = AmlSerializeDefinitionBlock (
             RootNode,
             Table
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-SERIAL-PORT-FIXUP: Failed to Serialize SSDT Table Data."
      " Status = %r\n",
      Status
      ));
  }

  return EFI_SUCCESS;

exit_handler:
  // Cleanup
  if (RootNode != NULL) {
    Status1 = AmlDeleteTree (RootNode);
    if (EFI_ERROR (Status1)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-SERIAL-PORT-FIXUP: Failed to cleanup AML tree."
        " Status = %r\n",
        Status1
        ));
      // If Status was success but we failed to delete the AML Tree
      // return Status1 else return the original error code, i.e. Status.
      if (!EFI_ERROR (Status)) {
        return Status1;
      }
    }
  }

  return Status;
}

/** Free an SSDT table previously created by
    the BuildSsdtSerialTable function.

  @param [in] Table   Pointer to a SSDT table allocated by
                      the BuildSsdtSerialTable function.

  @retval EFI_SUCCESS           Success.
**/
EFI_STATUS
EFIAPI
FreeSsdtSerialPortTable (
  IN EFI_ACPI_DESCRIPTION_HEADER  *Table
  )
{
  ASSERT (Table != NULL);
  FreePool (Table);
  return EFI_SUCCESS;
}
