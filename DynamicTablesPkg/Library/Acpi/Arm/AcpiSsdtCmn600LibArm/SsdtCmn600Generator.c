/** @file
  SSDT CMN-600 AML Table Generator.

  Copyright (c) 2020 - 2021, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - Arm CoreLink CMN-600 Coherent Mesh Network Technical Reference Manual r3p0
  - Generic ACPI for Arm Components 1.0 Platform Design Document
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
#include <Library/AmlLib/AmlLib.h>
#include <Protocol/ConfigurationManagerProtocol.h>
#include "SsdtCmn600Generator.h"

/** C array containing the compiled AML template.
    This symbol is defined in the auto generated C file
    containing the AML bytecode array.
*/
extern CHAR8  ssdtcmn600template_aml_code[];

/** SSDT CMN-600 Table Generator.

  Requirements:
  The following Configuration Manager Object(s) are required by
  this Generator:
  - EArmObjCmn600Info
*/

/** This macro expands to a function that retrieves the CMN-600
    Information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArm,
  EArmObjCmn600Info,
  CM_ARM_CMN_600_INFO
  );

/** Check the CMN-600 Information.

  @param [in]  Cmn600InfoList           Array of CMN-600 information structure.
  @param [in]  Cmn600Count              Count of CMN-600 information structure.

  @retval  EFI_SUCCESS            The function completed successfully.
  @retval  EFI_INVALID_PARAMETER  Invalid parameter.
**/
STATIC
EFI_STATUS
EFIAPI
ValidateCmn600Info (
  IN  CONST CM_ARM_CMN_600_INFO  *Cmn600InfoList,
  IN  CONST UINT32               Cmn600Count
  )
{
  UINT32                                  Index;
  UINT32                                  DtcIndex;
  CONST CM_ARM_CMN_600_INFO               *Cmn600Info;
  CONST CM_ARCH_COMMON_GENERIC_INTERRUPT  *DtcInterrupt;

  if ((Cmn600InfoList == NULL) ||
      (Cmn600Count == 0))
  {
    return EFI_INVALID_PARAMETER;
  }

  // Validate each Cmn600Info structure.
  for (Index = 0; Index < Cmn600Count; Index++) {
    Cmn600Info = &Cmn600InfoList[Index];

    // At least one DTC is required.
    if ((Cmn600Info->DtcCount == 0) ||
        (Cmn600Info->DtcCount > MAX_DTC_COUNT))
    {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-CMN-600: Invalid DTC configuration:\n"
        ));
      goto error_handler;
    }

    // Check PERIPHBASE and ROOTNODEBASE address spaces are initialized.
    if ((Cmn600Info->PeriphBaseAddress == 0)    ||
        (Cmn600Info->RootNodeBaseAddress == 0))
    {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-CMN-600: Invalid PERIPHBASE or ROOTNODEBASE.\n"
        ));
      goto error_handler;
    }

    // The PERIPHBASE address must be 64MB aligned for a (X < 4) && (Y < 4)
    // dimension mesh, and 256MB aligned otherwise.
    // Check it is a least 64MB aligned.
    if ((Cmn600Info->PeriphBaseAddress &
         (PERIPHBASE_MIN_ADDRESS_LENGTH - 1)) != 0)
    {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-CMN-600: PERIPHBASE address must be 64MB aligned.\n"
        ));
      goto error_handler;
    }

    // The PERIPHBASE address is at most 64MB for a (X < 4) && (Y < 4)
    // dimension mesh, and 256MB otherwise. Check it is not more than 256MB.
    if (Cmn600Info->PeriphBaseAddressLength > PERIPHBASE_MAX_ADDRESS_LENGTH) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-CMN-600: PERIPHBASE address range must be < 256MB.\n"
        ));
      goto error_handler;
    }

    // Check the 16 KB alignment of the ROOTNODEBASE address.
    if ((Cmn600Info->PeriphBaseAddress &
         (ROOTNODEBASE_ADDRESS_LENGTH - 1)) != 0)
    {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-CMN-600: Root base address must be 16KB aligned.\n"
        ));
      goto error_handler;
    }

    // The ROOTNODEBASE address space should be included in the PERIPHBASE
    // address space.
    if ((Cmn600Info->PeriphBaseAddress > Cmn600Info->RootNodeBaseAddress)  ||
        ((Cmn600Info->PeriphBaseAddress + Cmn600Info->PeriphBaseAddressLength) <
         (Cmn600Info->RootNodeBaseAddress + ROOTNODEBASE_ADDRESS_LENGTH)))
    {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-CMN-600:"
        " ROOTNODEBASE address space not in PERIPHBASE address space.\n"
        ));
      goto error_handler;
    }

    for (DtcIndex = 0; DtcIndex < Cmn600Info->DtcCount; DtcIndex++) {
      DtcInterrupt = &Cmn600Info->DtcInterrupt[DtcIndex];
      if (((DtcInterrupt->Flags &
            EFI_ACPI_EXTENDED_INTERRUPT_FLAG_PRODUCER_CONSUMER_MASK) == 0))
      {
        DEBUG ((
          DEBUG_ERROR,
          "ERROR: SSDT-CMN-600: DTC Interrupt must be consumer.\n"
          ));
        goto error_handler;
      }
    } // for DTC Interrupt
  } // for Cmn600InfoList

  return EFI_SUCCESS;

error_handler:

  DEBUG ((
    DEBUG_ERROR,
    "PeriphBaseAddress = 0x%llx\n"
    "PeriphBaseAddressLength = 0x%llx\n"
    "RootNodeBaseAddress = 0x%llx\n"
    "DtcCount = %u\n",
    Cmn600Info->PeriphBaseAddress,
    Cmn600Info->PeriphBaseAddressLength,
    Cmn600Info->RootNodeBaseAddress,
    Cmn600Info->DtcCount
    ));

  DEBUG_CODE_BEGIN ();
  for (DtcIndex = 0; DtcIndex < Cmn600Info->DtcCount; DtcIndex++) {
    DtcInterrupt = &Cmn600Info->DtcInterrupt[DtcIndex];
    DEBUG ((
      DEBUG_ERROR,
      "  DTC[%d]:\n",
      DtcIndex
      ));
    DEBUG ((
      DEBUG_ERROR,
      "    Interrupt = 0x%lx\n",
      DtcInterrupt->Interrupt
      ));
    DEBUG ((
      DEBUG_ERROR,
      "    Flags = 0x%lx\n",
      DtcInterrupt->Flags
      ));
  }   // for

  DEBUG_CODE_END ();

  return EFI_INVALID_PARAMETER;
}

/** Build a SSDT table describing the CMN-600 device.

  The table created by this function must be freed by FreeSsdtCmn600Table.

  @param [in]  Cmn600Info       Pointer to a Cmn600 structure.
  @param [in]  Name             The Name to give to the Device.
                                Must be a NULL-terminated ASL NameString
                                e.g.: "DEV0", "DV15.DEV0", etc.
  @param [in]  Uid              UID for the CMN600 device.
  @param [out] Table            If success, pointer to the created SSDT table.

  @retval EFI_SUCCESS            Table generated successfully.
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.
  @retval EFI_NOT_FOUND          Could not find information.
  @retval EFI_OUT_OF_RESOURCES   Could not allocate memory.
**/
STATIC
EFI_STATUS
EFIAPI
FixupCmn600Info (
  IN  CONST CM_ARM_CMN_600_INFO          *Cmn600Info,
  IN  CONST CHAR8                        *Name,
  IN  CONST UINT64                       Uid,
  OUT       EFI_ACPI_DESCRIPTION_HEADER  **Table
  )
{
  EFI_STATUS                              Status;
  EFI_STATUS                              Status1;
  UINT8                                   Index;
  CONST CM_ARCH_COMMON_GENERIC_INTERRUPT  *DtcInt;

  EFI_ACPI_DESCRIPTION_HEADER  *SsdtCmn600Template;
  AML_ROOT_NODE_HANDLE         RootNodeHandle;
  AML_OBJECT_NODE_HANDLE       NameOpIdNode;
  AML_OBJECT_NODE_HANDLE       NameOpCrsNode;
  AML_DATA_NODE_HANDLE         CmnPeriphBaseRdNode;
  AML_DATA_NODE_HANDLE         CmnRootNodeBaseRdNode;
  AML_OBJECT_NODE_HANDLE       DeviceNode;

  // Parse the Ssdt CMN-600 Template.
  SsdtCmn600Template = (EFI_ACPI_DESCRIPTION_HEADER *)
                       ssdtcmn600template_aml_code;

  RootNodeHandle = NULL;
  Status         = AmlParseDefinitionBlock (
                     SsdtCmn600Template,
                     &RootNodeHandle
                     );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-CMN-600: Failed to parse SSDT CMN-600 Template."
      " Status = %r\n",
      Status
      ));
    return Status;
  }

  // Get the _UID NameOp object defined by the "Name ()" statement,
  // and update its value.
  Status = AmlFindNode (
             RootNodeHandle,
             "\\_SB_.CMN0._UID",
             &NameOpIdNode
             );
  if (EFI_ERROR (Status)) {
    goto error_handler;
  }

  Status = AmlNameOpUpdateInteger (NameOpIdNode, (UINT64)Uid);
  if (EFI_ERROR (Status)) {
    goto error_handler;
  }

  // Get the _CRS object defined by the "Name ()" statement.
  Status = AmlFindNode (
             RootNodeHandle,
             "\\_SB.CMN0._CRS",
             &NameOpCrsNode
             );
  if (EFI_ERROR (Status)) {
    goto error_handler;
  }

  // Get the first Rd node in the "_CRS" object.
  // This is the PERIPHBASE node.
  Status = AmlNameOpGetFirstRdNode (NameOpCrsNode, &CmnPeriphBaseRdNode);
  if (EFI_ERROR (Status)) {
    goto error_handler;
  }

  if (CmnPeriphBaseRdNode == NULL) {
    Status = EFI_INVALID_PARAMETER;
    goto error_handler;
  }

  // Update the PERIPHBASE base address and length.
  Status = AmlUpdateRdQWord (
             CmnPeriphBaseRdNode,
             Cmn600Info->PeriphBaseAddress,
             Cmn600Info->PeriphBaseAddressLength
             );
  if (EFI_ERROR (Status)) {
    goto error_handler;
  }

  // Get the QWord node corresponding to the ROOTNODEBASE.
  // It is the second Resource Data element in the BufferNode's
  // variable list of arguments.
  Status = AmlNameOpGetNextRdNode (
             CmnPeriphBaseRdNode,
             &CmnRootNodeBaseRdNode
             );
  if (EFI_ERROR (Status)) {
    goto error_handler;
  }

  if (CmnRootNodeBaseRdNode == NULL) {
    Status = EFI_INVALID_PARAMETER;
    goto error_handler;
  }

  // Update the ROOTNODEBASE base address and length.
  Status = AmlUpdateRdQWord (
             CmnRootNodeBaseRdNode,
             Cmn600Info->RootNodeBaseAddress,
             ROOTNODEBASE_ADDRESS_LENGTH
             );
  if (EFI_ERROR (Status)) {
    goto error_handler;
  }

  // Add the Interrupt node(s).
  // Generate Resource Data node(s) corresponding to the "Interrupt ()"
  // ASL function and add it at the last position in the list of
  // Resource Data nodes.
  for (Index = 0; Index < Cmn600Info->DtcCount; Index++) {
    DtcInt = &Cmn600Info->DtcInterrupt[Index];

    Status = AmlCodeGenRdInterrupt (
               ((DtcInt->Flags &
                 EFI_ACPI_EXTENDED_INTERRUPT_FLAG_PRODUCER_CONSUMER_MASK) != 0),
               ((DtcInt->Flags &
                 EFI_ACPI_EXTENDED_INTERRUPT_FLAG_MODE_MASK) != 0),
               ((DtcInt->Flags &
                 EFI_ACPI_EXTENDED_INTERRUPT_FLAG_POLARITY_MASK) != 0),
               ((DtcInt->Flags &
                 EFI_ACPI_EXTENDED_INTERRUPT_FLAG_SHARABLE_MASK) != 0),
               (UINT32 *)&DtcInt->Interrupt,
               1,
               NameOpCrsNode,
               NULL
               );
    if (EFI_ERROR (Status)) {
      goto error_handler;
    }
  } // for

  // Fixup the CMN600 device name.
  // This MUST be done at the end, otherwise AML paths won't be valid anymore.
  // Get the CMN0 variable defined by the "Device ()" statement.
  Status = AmlFindNode (RootNodeHandle, "\\_SB_.CMN0", &DeviceNode);
  if (EFI_ERROR (Status)) {
    goto error_handler;
  }

  // Update the CMN600 Device's name.
  Status = AmlDeviceOpUpdateName (DeviceNode, Name);
  if (EFI_ERROR (Status)) {
    goto error_handler;
  }

  // Serialise the definition block
  Status = AmlSerializeDefinitionBlock (
             RootNodeHandle,
             Table
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-CMN-600: Failed to Serialize SSDT Table Data."
      " Status = %r\n",
      Status
      ));
  }

error_handler:
  // Cleanup
  if (RootNodeHandle != NULL) {
    Status1 = AmlDeleteTree (RootNodeHandle);
    if (EFI_ERROR (Status1)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-CMN-600: Failed to cleanup AML tree."
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

/** Free any resources allocated for constructing the SSDT tables for CMN-600.

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
FreeSsdtCmn600TableResourcesEx (
  IN      CONST ACPI_TABLE_GENERATOR                   *CONST  This,
  IN      CONST CM_STD_OBJ_ACPI_TABLE_INFO             *CONST  AcpiTableInfo,
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL   *CONST  CfgMgrProtocol,
  IN OUT        EFI_ACPI_DESCRIPTION_HEADER          ***CONST  Table,
  IN      CONST UINTN                                          TableCount
  )
{
  EFI_ACPI_DESCRIPTION_HEADER  **TableList;
  UINTN                        Index;

  ASSERT (This != NULL);
  ASSERT (AcpiTableInfo != NULL);
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (AcpiTableInfo->TableGeneratorId == This->GeneratorID);
  ASSERT (AcpiTableInfo->AcpiTableSignature == This->AcpiTableSignature);

  if ((Table == NULL)   ||
      (*Table == NULL)  ||
      (TableCount == 0))
  {
    DEBUG ((DEBUG_ERROR, "ERROR: SSDT-CMN-600: Invalid Table Pointer\n"));
    return EFI_INVALID_PARAMETER;
  }

  TableList = *Table;

  for (Index = 0; Index < TableCount; Index++) {
    if ((TableList[Index] != NULL) &&
        (TableList[Index]->Signature ==
         EFI_ACPI_6_3_SECONDARY_SYSTEM_DESCRIPTION_TABLE_SIGNATURE))
    {
      FreePool (TableList[Index]);
    } else {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-CMN-600: Could not free SSDT table at index %d."
        " Status = %r\n",
        Index,
        EFI_INVALID_PARAMETER
        ));
      return EFI_INVALID_PARAMETER;
    }
  } // for

  // Free the table list.
  FreePool (*Table);
  *Table = NULL;
  return EFI_SUCCESS;
}

/** Construct SSDT tables for describing CMN-600 meshes.

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
BuildSsdtCmn600TableEx (
  IN  CONST ACPI_TABLE_GENERATOR                           *This,
  IN  CONST CM_STD_OBJ_ACPI_TABLE_INFO             *CONST  AcpiTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL   *CONST  CfgMgrProtocol,
  OUT       EFI_ACPI_DESCRIPTION_HEADER                    ***Table,
  OUT       UINTN                                  *CONST  TableCount
  )
{
  EFI_STATUS                   Status;
  UINT64                       Index;
  CM_ARM_CMN_600_INFO          *Cmn600Info;
  UINT32                       Cmn600Count;
  CHAR8                        NewName[AML_NAME_SEG_SIZE + 1];
  EFI_ACPI_DESCRIPTION_HEADER  **TableList;

  ASSERT (This != NULL);
  ASSERT (AcpiTableInfo != NULL);
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (Table != NULL);
  ASSERT (TableCount != NULL);
  ASSERT (AcpiTableInfo->TableGeneratorId == This->GeneratorID);
  ASSERT (AcpiTableInfo->AcpiTableSignature == This->AcpiTableSignature);

  *Table = NULL;

  // Get CMN-600 information.
  Status = GetEArmObjCmn600Info (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &Cmn600Info,
             &Cmn600Count
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-CMN-600: Failed to get the CMN-600 information."
      " Status = %r\n",
      Status
      ));
    return Status;
  }

  if ((Cmn600Count == 0) || (Cmn600Count > MAX_CMN600_DEVICES_SUPPORTED)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-CMN-600: CMN600 peripheral count = %d."
      " This must be between 1 to 16.\n",
      Cmn600Count
      ));
    return EFI_INVALID_PARAMETER;
  }

  // Validate the CMN-600 Info.
  Status = ValidateCmn600Info (Cmn600Info, Cmn600Count);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-CMN-600: Invalid CMN600 information. Status = %r\n",
      Status
      ));
    return Status;
  }

  // Allocate a table to store pointers to the SSDT tables.
  TableList = (EFI_ACPI_DESCRIPTION_HEADER **)
              AllocateZeroPool (
                (sizeof (EFI_ACPI_DESCRIPTION_HEADER *) * Cmn600Count)
                );
  if (TableList == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-CMN-600: Failed to allocate memory for Table List."
      " Status = %r\n",
      Status
      ));
    return Status;
  }

  // Setup the table list early so that appropriate cleanup
  // can be done in case of failure.
  *Table = TableList;

  NewName[0] = 'C';
  NewName[1] = 'M';
  NewName[2] = 'N';
  NewName[4] = '\0';
  for (Index = 0; Index < Cmn600Count; Index++) {
    NewName[3] = AsciiFromHex ((UINT8)(Index));

    // Build a SSDT table describing the CMN600 device.
    Status = FixupCmn600Info (
               &Cmn600Info[Index],
               NewName,
               Index,
               &TableList[Index]
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-CMN-600: Failed to build associated SSDT table."
        " Status = %r\n",
        Status
        ));
      break;
    }

    // Increment the table count here so that appropriate clean-up
    // can be done in case of failure.
    *TableCount += 1;
  } // for

  // Note: Table list and CMN600 device count has been setup. The
  // framework will invoke FreeSsdtCmn600TableResourcesEx() even
  // on failure, so appropriate clean-up will be done.
  return Status;
}

/** This macro defines the Raw Generator revision.
*/
#define SSDT_CMN_600_GENERATOR_REVISION  CREATE_REVISION (1, 0)

/** The interface for the Raw Table Generator.
*/
STATIC
CONST
ACPI_TABLE_GENERATOR  SsdtCmn600Generator = {
  // Generator ID
  CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdSsdtCmn600),
  // Generator Description
  L"ACPI.STD.SSDT.CMN600.GENERATOR",
  // ACPI Table Signature
  EFI_ACPI_6_3_SECONDARY_SYSTEM_DESCRIPTION_TABLE_SIGNATURE,
  // ACPI Table Revision - Unused
  0,
  // Minimum ACPI Table Revision - Unused
  0,
  // Creator ID
  TABLE_GENERATOR_CREATOR_ID_ARM,
  // Creator Revision
  SSDT_CMN_600_GENERATOR_REVISION,
  // Build table function. Use the extended version instead.
  NULL,
  // Free table function. Use the extended version instead.
  NULL,
  // Build Table function
  BuildSsdtCmn600TableEx,
  // Free Resource function
  FreeSsdtCmn600TableResourcesEx
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
AcpiSsdtCmn600LibConstructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = RegisterAcpiTableGenerator (&SsdtCmn600Generator);
  DEBUG ((
    DEBUG_INFO,
    "SSDT-CMN-600: Register Generator. Status = %r\n",
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
AcpiSsdtCmn600LibDestructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = DeregisterAcpiTableGenerator (&SsdtCmn600Generator);
  DEBUG ((
    DEBUG_INFO,
    "SSDT-CMN-600: Deregister Generator. Status = %r\n",
    Status
    ));
  ASSERT_EFI_ERROR (Status);
  return Status;
}
