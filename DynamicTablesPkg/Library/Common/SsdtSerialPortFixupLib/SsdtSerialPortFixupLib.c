/** @file
  SSDT Serial Port Fixup Library.

  Copyright (c) 2019 - 2021, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - Arm Server Base Boot Requirements (SBBR), s4.2.1.8 "SPCR".
  - Microsoft Debug Port Table 2 (DBG2) Specification - December 10, 2015.
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

/** C array containing the compiled AML template.
    This symbol is defined in the auto generated C file
    containing the AML bytecode array.
*/
extern CHAR8  ssdtserialporttemplate_aml_code[];

/** UART address range length.
*/
#define MIN_UART_ADDRESS_LENGTH  0x1000U

/** Validate the Serial Port Information.

  @param [in]  SerialPortInfoTable    Table of CM_ARM_SERIAL_PORT_INFO.
  @param [in]  SerialPortCount        Count of SerialPort in the table.

  @retval EFI_SUCCESS             Success.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
ValidateSerialPortInfo (
  IN  CONST CM_ARM_SERIAL_PORT_INFO  *SerialPortInfoTable,
  IN        UINT32                   SerialPortCount
  )
{
  UINT32                         Index;
  CONST CM_ARM_SERIAL_PORT_INFO  *SerialPortInfo;

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

/** Fixup the Serial Port Ids (_UID, _HID, _CID).

  @param  [in]  RootNodeHandle  Pointer to the root of an AML tree.
  @param  [in]  Uid             UID for the Serial Port.
  @param  [in]  SerialPortInfo  Pointer to a Serial Port Information
                                structure.
                                Get the Serial Port Information from there.

  @retval  EFI_SUCCESS            The function completed successfully.
  @retval  EFI_INVALID_PARAMETER  Invalid parameter.
  @retval  EFI_NOT_FOUND          Could not find information.
  @retval  EFI_OUT_OF_RESOURCES   Out of resources.
**/
STATIC
EFI_STATUS
EFIAPI
FixupIds (
  IN        AML_ROOT_NODE_HANDLE     RootNodeHandle,
  IN  CONST UINT64                   Uid,
  IN  CONST CM_ARM_SERIAL_PORT_INFO  *SerialPortInfo
  )
{
  EFI_STATUS              Status;
  AML_OBJECT_NODE_HANDLE  NameOpIdNode;
  CONST CHAR8             *HidString;
  CONST CHAR8             *CidString;
  CONST CHAR8             *NonBsaHid;

  // Get the _CID and _HID value to write.
  switch (SerialPortInfo->PortSubtype) {
    case EFI_ACPI_DBG2_PORT_SUBTYPE_SERIAL_FULL_16550:
    case EFI_ACPI_DBG2_PORT_SUBTYPE_SERIAL_16550_WITH_GAS:
    {
      // If there is a non-BSA compliant HID, use that.
      NonBsaHid = (CONST CHAR8 *)PcdGetPtr (PcdNonBsaCompliant16550SerialHid);
      if ((NonBsaHid != NULL) && (AsciiStrLen (NonBsaHid) != 0)) {
        if (!(IsValidPnpId (NonBsaHid) || IsValidAcpiId (NonBsaHid))) {
          return EFI_INVALID_PARAMETER;
        }

        HidString = NonBsaHid;
        CidString = "";
      } else {
        HidString = "PNP0501";
        CidString = "PNP0500";
      }

      break;
    }
    case EFI_ACPI_DBG2_PORT_SUBTYPE_SERIAL_ARM_PL011_UART:
    {
      HidString = "ARMH0011";
      CidString = "ARMHB000";
      break;
    }
    case EFI_ACPI_DBG2_PORT_SUBTYPE_SERIAL_ARM_SBSA_GENERIC_UART:
    case EFI_ACPI_DBG2_PORT_SUBTYPE_SERIAL_ARM_SBSA_GENERIC_UART_2X:
    {
      HidString = "ARMH0011";
      CidString = "";
      break;
    }
    default:
    {
      return EFI_INVALID_PARAMETER;
    }
  } // switch

  // Get the _UID NameOp object defined by the "Name ()" statement,
  // and update its value.
  Status = AmlFindNode (
             RootNodeHandle,
             "\\_SB_.COM0._UID",
             &NameOpIdNode
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = AmlNameOpUpdateInteger (NameOpIdNode, (UINT64)Uid);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Get the _HID NameOp object defined by the "Name ()" statement,
  // and update its value.
  Status = AmlFindNode (
             RootNodeHandle,
             "\\_SB_.COM0._HID",
             &NameOpIdNode
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = AmlNameOpUpdateString (NameOpIdNode, HidString);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Get the _CID NameOp object defined by the "Name ()" statement,
  // and update its value.
  Status = AmlFindNode (
             RootNodeHandle,
             "\\_SB_.COM0._CID",
             &NameOpIdNode
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // If we have a CID then update a _CID node else delete the node.
  if (AsciiStrLen (CidString) != 0) {
    Status = AmlNameOpUpdateString (NameOpIdNode, CidString);
  } else {
    // First detach the node from the tree.
    Status = AmlDetachNode (NameOpIdNode);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    // Delete the detached node.
    Status = AmlDeleteTree (NameOpIdNode);
  }

  return Status;
}

/** Fixup the Serial Port _CRS values (BaseAddress, ...).

  @param  [in]  RootNodeHandle  Pointer to the root of an AML tree.
  @param  [in]  SerialPortInfo  Pointer to a Serial Port Information
                                structure.
                                Get the Serial Port Information from there.

  @retval  EFI_SUCCESS            The function completed successfully.
  @retval  EFI_INVALID_PARAMETER  Invalid parameter.
  @retval  EFI_NOT_FOUND          Could not find information.
  @retval  EFI_OUT_OF_RESOURCES   Out of resources.
**/
STATIC
EFI_STATUS
EFIAPI
FixupCrs (
  IN        AML_ROOT_NODE_HANDLE     RootNodeHandle,
  IN  CONST CM_ARM_SERIAL_PORT_INFO  *SerialPortInfo
  )
{
  EFI_STATUS              Status;
  AML_OBJECT_NODE_HANDLE  NameOpCrsNode;
  AML_DATA_NODE_HANDLE    QWordRdNode;
  AML_DATA_NODE_HANDLE    InterruptRdNode;

  // Get the "_CRS" object defined by the "Name ()" statement.
  Status = AmlFindNode (
             RootNodeHandle,
             "\\_SB_.COM0._CRS",
             &NameOpCrsNode
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Get the first Rd node in the "_CRS" object.
  Status = AmlNameOpGetFirstRdNode (NameOpCrsNode, &QWordRdNode);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (QWordRdNode == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // Update the Serial Port base address and length.
  Status = AmlUpdateRdQWord (
             QWordRdNode,
             SerialPortInfo->BaseAddress,
             ((SerialPortInfo->BaseAddressLength < MIN_UART_ADDRESS_LENGTH) ?
              MIN_UART_ADDRESS_LENGTH : SerialPortInfo->BaseAddressLength)
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Get the Interrupt node.
  // It is the second Resource Data element in the NameOpCrsNode's
  // variable list of arguments.
  Status = AmlNameOpGetNextRdNode (QWordRdNode, &InterruptRdNode);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (InterruptRdNode == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // Update the interrupt number.
  return AmlUpdateRdInterrupt (InterruptRdNode, SerialPortInfo->Interrupt);
}

/** Fixup the Serial Port device name.

  @param  [in]  RootNodeHandle  Pointer to the root of an AML tree.
  @param  [in]  SerialPortInfo  Pointer to a Serial Port Information
                                structure.
                                Get the Serial Port Information from there.
  @param  [in]  Name            The Name to give to the Device.
                                Must be a NULL-terminated ASL NameString
                                e.g.: "DEV0", "DV15.DEV0", etc.

  @retval  EFI_SUCCESS            The function completed successfully.
  @retval  EFI_INVALID_PARAMETER  Invalid parameter.
  @retval  EFI_NOT_FOUND          Could not find information.
  @retval  EFI_OUT_OF_RESOURCES   Out of resources.
**/
STATIC
EFI_STATUS
EFIAPI
FixupName (
  IN        AML_ROOT_NODE_HANDLE     RootNodeHandle,
  IN  CONST CM_ARM_SERIAL_PORT_INFO  *SerialPortInfo,
  IN  CONST CHAR8                    *Name
  )
{
  EFI_STATUS              Status;
  AML_OBJECT_NODE_HANDLE  DeviceNode;

  // Get the COM0 variable defined by the "Device ()" statement.
  Status = AmlFindNode (RootNodeHandle, "\\_SB_.COM0", &DeviceNode);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Update the Device's name.
  return AmlDeviceOpUpdateName (DeviceNode, (CHAR8 *)Name);
}

/** Fixup the Serial Port Information in the AML tree.

  For each template value:
   - find the node to update;
   - update the value.

  @param  [in]  RootNodeHandle  Pointer to the root of the AML tree.
  @param  [in]  SerialPortInfo  Pointer to a Serial Port Information
                                structure.
                                Get the Serial Port Information from there.
  @param  [in]  Name            The Name to give to the Device.
                                Must be a NULL-terminated ASL NameString
                                e.g.: "DEV0", "DV15.DEV0", etc.
  @param  [in]  Uid             UID for the Serial Port.
  @param  [out] Table           If success, contains the serialized
                                SSDT table.

  @retval  EFI_SUCCESS            The function completed successfully.
  @retval  EFI_INVALID_PARAMETER  Invalid parameter.
  @retval  EFI_NOT_FOUND          Could not find information.
  @retval  EFI_OUT_OF_RESOURCES   Out of resources.
**/
STATIC
EFI_STATUS
EFIAPI
FixupSerialPortInfo (
  IN            AML_ROOT_NODE_HANDLE     RootNodeHandle,
  IN      CONST CM_ARM_SERIAL_PORT_INFO  *SerialPortInfo,
  IN      CONST CHAR8                    *Name,
  IN      CONST UINT64                   Uid,
  OUT       EFI_ACPI_DESCRIPTION_HEADER  **Table
  )
{
  EFI_STATUS  Status;

  ASSERT (RootNodeHandle != NULL);
  ASSERT (SerialPortInfo != NULL);
  ASSERT (Name != NULL);
  ASSERT (Table != NULL);

  // Fixup the _UID, _HID and _CID values.
  Status = FixupIds (RootNodeHandle, Uid, SerialPortInfo);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Fixup the _CRS values.
  Status = FixupCrs (RootNodeHandle, SerialPortInfo);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Fixup the serial-port name.
  // This MUST be done at the end, otherwise AML paths won't be valid anymore.
  return FixupName (RootNodeHandle, SerialPortInfo, Name);
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
  IN  CONST CM_STD_OBJ_ACPI_TABLE_INFO   *AcpiTableInfo,
  IN  CONST CM_ARM_SERIAL_PORT_INFO      *SerialPortInfo,
  IN  CONST CHAR8                        *Name,
  IN  CONST UINT64                       Uid,
  OUT       EFI_ACPI_DESCRIPTION_HEADER  **Table
  )
{
  EFI_STATUS            Status;
  EFI_STATUS            Status1;
  AML_ROOT_NODE_HANDLE  RootNodeHandle;

  ASSERT (AcpiTableInfo != NULL);
  ASSERT (SerialPortInfo != NULL);
  ASSERT (Name != NULL);
  ASSERT (Table != NULL);

  // Validate the Serial Port Info.
  Status = ValidateSerialPortInfo (SerialPortInfo, 1);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Parse the SSDT Serial Port Template.
  Status = AmlParseDefinitionBlock (
             (EFI_ACPI_DESCRIPTION_HEADER *)ssdtserialporttemplate_aml_code,
             &RootNodeHandle
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-SERIAL-PORT-FIXUP:"
      " Failed to parse SSDT Serial Port Template. Status = %r\n",
      Status
      ));
    return Status;
  }

  // Fixup the template values.
  Status = FixupSerialPortInfo (
             RootNodeHandle,
             SerialPortInfo,
             Name,
             Uid,
             Table
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-SERIAL-PORT-FIXUP: Failed to fixup SSDT Serial Port Table."
      " Status = %r\n",
      Status
      ));
    goto exit_handler;
  }

  // Serialize the tree.
  Status = AmlSerializeDefinitionBlock (
             RootNodeHandle,
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

exit_handler:
  // Cleanup
  if (RootNodeHandle != NULL) {
    Status1 = AmlDeleteTree (RootNodeHandle);
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
