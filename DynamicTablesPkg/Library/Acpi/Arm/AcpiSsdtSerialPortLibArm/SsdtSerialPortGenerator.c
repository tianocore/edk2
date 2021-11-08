/** @file
  SSDT Serial Port Table Generator.

  Copyright (c) 2020 - 2021, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

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
#include <Library/SsdtSerialPortFixupLib.h>
#include <Protocol/ConfigurationManagerProtocol.h>

/** ARM standard SSDT Serial Port Table Generator

  Constructs SSDT tables describing serial ports (other than the serial ports
  used by the SPCR or DBG2 tables).

Requirements:
  The following Configuration Manager Object(s) are required by
  this Generator:
  - EArmObjSerialPortInfo
*/

/** This macro expands to a function that retrieves the Serial-port
    information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArm,
  EArmObjSerialPortInfo,
  CM_ARM_SERIAL_PORT_INFO
  );

/** Starting value for the UID to represent the serial ports.
    Note: The UID 0 and 1 are reserved for use by DBG2 port and SPCR
          respectively. So, the UIDs for serial ports for general use
          start at 2.
*/
#define SERIAL_PORT_START_UID                      2

/** Maximum serial ports supported by this generator.
    This generator supports a maximum of 14 (16 - 2) serial ports.
    The -2 here reflects the reservation for serial ports for the DBG2
    and SPCR ports regardless of whether the DBG2 or SPCR port is enabled.
    Note: This is not a hard limitation and can be extended if needed.
          Corresponding changes would be needed to support the Name and
          UID fields describing the serial port.

*/
#define MAX_SERIAL_PORTS_SUPPORTED                 14

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
FreeSsdtSerialPortTableEx (
  IN      CONST ACPI_TABLE_GENERATOR                   * CONST This,
  IN      CONST CM_STD_OBJ_ACPI_TABLE_INFO             * CONST AcpiTableInfo,
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL   * CONST CfgMgrProtocol,
  IN OUT        EFI_ACPI_DESCRIPTION_HEADER          *** CONST Table,
  IN      CONST UINTN                                          TableCount
  )
{
  EFI_STATUS                        Status;
  EFI_ACPI_DESCRIPTION_HEADER    ** TableList;
  UINTN                             Index;

  ASSERT (This != NULL);
  ASSERT (AcpiTableInfo != NULL);
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (AcpiTableInfo->TableGeneratorId == This->GeneratorID);
  ASSERT (AcpiTableInfo->AcpiTableSignature == This->AcpiTableSignature);

  if ((Table == NULL)   ||
      (*Table == NULL)  ||
      (TableCount == 0)) {
    DEBUG ((DEBUG_ERROR, "ERROR: SSDT-SERIAL-PORT: Invalid Table Pointer\n"));
    return EFI_INVALID_PARAMETER;
  }

  TableList = *Table;

  for (Index = 0; Index < TableCount; Index++) {
    if ((TableList[Index] != NULL) &&
        (TableList[Index]->Signature ==
         EFI_ACPI_6_3_SECONDARY_SYSTEM_DESCRIPTION_TABLE_SIGNATURE)) {
      Status = FreeSsdtSerialPortTable (TableList[Index]);
    } else {
      Status = EFI_INVALID_PARAMETER;
    }

    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-SERIAL-PORT: Could not free SSDT table at index %d."
        " Status = %r\n",
        Index,
        Status
        ));
      return Status;
    }
  } //for

  // Free the table list.
  FreePool (*Table);

  return EFI_SUCCESS;
}

/** Construct SSDT tables describing serial-ports.

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
BuildSsdtSerialPortTableEx (
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
  UINTN                           Index;
  CHAR8                           NewName[AML_NAME_SEG_SIZE + 1];
  UINT64                          Uid;
  EFI_ACPI_DESCRIPTION_HEADER  ** TableList;

  ASSERT (This != NULL);
  ASSERT (AcpiTableInfo != NULL);
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (Table != NULL);
  ASSERT (TableCount != NULL);
  ASSERT (AcpiTableInfo->TableGeneratorId == This->GeneratorID);
  ASSERT (AcpiTableInfo->AcpiTableSignature == This->AcpiTableSignature);

  *Table = NULL;

  Status = GetEArmObjSerialPortInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &SerialPortInfo,
             &SerialPortCount
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-SERIAL-PORT: Failed to get serial port information."
      " Status = %r\n",
      Status
      ));
    return Status;
  }

  if (SerialPortCount > MAX_SERIAL_PORTS_SUPPORTED) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-SERIAL-PORT: Too many serial ports: %d."
      " Maximum serial ports supported = %d.\n",
      SerialPortCount,
      MAX_SERIAL_PORTS_SUPPORTED
      ));
    return EFI_INVALID_PARAMETER;
  }

  // Validate the SerialPort info.
  Status = ValidateSerialPortInfo (SerialPortInfo, SerialPortCount);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-SERIAL-PORT: Invalid serial port information. Status = %r\n",
      Status
      ));
    return Status;
  }

  // Allocate a table to store pointers to the SSDT tables.
  TableList = (EFI_ACPI_DESCRIPTION_HEADER**)
              AllocateZeroPool (
                (sizeof (EFI_ACPI_DESCRIPTION_HEADER*) * SerialPortCount)
                );
  if (TableList == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-SERIAL-PORT: Failed to allocate memory for Table List."
      " Status = %r\n",
      Status
      ));
    return Status;
  }

  // Setup the table list early so that that appropriate cleanup
  // can be done in case of failure.
  *Table = TableList;

  NewName[0] = 'C';
  NewName[1] = 'O';
  NewName[2] = 'M';
  NewName[4] = '\0';
  for (Index = 0; Index < SerialPortCount; Index++) {
    Uid = SERIAL_PORT_START_UID + Index;
    NewName[3] = AsciiFromHex ((UINT8)(Uid));

    // Build a SSDT table describing the serial port.
    Status = BuildSsdtSerialPortTable (
               AcpiTableInfo,
               &SerialPortInfo[Index],
               NewName,
               Uid,
               &TableList[Index]
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-SERIAL-PORT: Failed to build associated SSDT table."
        " Status = %r\n",
        Status
        ));
      goto error_handler;
    }

    // Increment the table count here so that appropriate cleanup
    // can be done in case of failure.
    *TableCount += 1;
  } // for

error_handler:
  // Note: Table list and Serial port count has been setup. The
  // error handler does nothing here as the framework will invoke
  // FreeSsdtSerialPortTableEx() even on failure.
  return Status;
}

/** This macro defines the SSDT Serial Port Table Generator revision.
*/
#define SSDT_SERIAL_GENERATOR_REVISION CREATE_REVISION (1, 0)

/** The interface for the SSDT Serial Port Table Generator.
*/
STATIC
CONST
ACPI_TABLE_GENERATOR SsdtSerialPortGenerator = {
  // Generator ID
  CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdSsdtSerialPort),
  // Generator Description
  L"ACPI.STD.SSDT.SERIAL.PORT.GENERATOR",
  // ACPI Table Signature
  EFI_ACPI_6_3_SECONDARY_SYSTEM_DESCRIPTION_TABLE_SIGNATURE,
  // ACPI Table Revision - Unused
  0,
  // Minimum ACPI Table Revision - Unused
  0,
  // Creator ID
  TABLE_GENERATOR_CREATOR_ID_ARM,
  // Creator Revision
  SSDT_SERIAL_GENERATOR_REVISION,
  // Build table function. Use the extended version instead.
  NULL,
  // Free table function. Use the extended version instead.
  NULL,
  // Extended Build table function.
  BuildSsdtSerialPortTableEx,
  // Extended free function.
  FreeSsdtSerialPortTableEx
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
AcpiSsdtSerialPortLibConstructor (
  IN  EFI_HANDLE           ImageHandle,
  IN  EFI_SYSTEM_TABLE  *  SystemTable
  )
{
  EFI_STATUS  Status;
  Status = RegisterAcpiTableGenerator (&SsdtSerialPortGenerator);
  DEBUG ((
    DEBUG_INFO,
    "SSDT-SERIAL-PORT: Register Generator. Status = %r\n",
    Status
    ));
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
AcpiSsdtSerialPortLibDestructor (
  IN  EFI_HANDLE           ImageHandle,
  IN  EFI_SYSTEM_TABLE  *  SystemTable
  )
{
  EFI_STATUS  Status;
  Status = DeregisterAcpiTableGenerator (&SsdtSerialPortGenerator);
  DEBUG ((
    DEBUG_INFO,
    "SSDT-SERIAL-PORT: Deregister Generator. Status = %r\n",
    Status
    ));
  ASSERT_EFI_ERROR (Status);
  return Status;
}
