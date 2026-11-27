/** @file
  SSDT CMN AML Table Generator.

  Copyright (c) 2020 - 2026, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - Arm CoreLink CMN-600 Coherent Mesh Network Technical Reference Manual r3p0
  - Generic ACPI for Arm Components 1.3 Platform Design Document
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
#include "SsdtCmnGenerator.h"

/** C array containing the compiled AML template.
    This symbol is defined in the auto generated C file
    containing the AML bytecode array.
*/
extern CHAR8  ssdtcmntemplate_aml_code[];

/** SSDT CMN Table Generator.

  Requirements:
  The following Configuration Manager Object(s) are required by
  this Generator:
  - EArmObjCmnInfo
*/

/** This macro expands to a function that retrieves the CMN
    Information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArm,
  EArmObjCmnInfo,
  CM_ARM_CMN_INFO
  );

/** Get the ACPI Hardware ID for a CMN implementation.

  @param [in] CmnType  CMN implementation type.

  @retval NULL  The CMN implementation type is unsupported.
  @return       The ACPI Hardware ID for the CMN implementation.
**/
STATIC
CONST CHAR8 *
GetCmnHid (
  IN ARM_CMN_TYPE  CmnType
  )
{
  switch (CmnType) {
    case ArmCmnType600:
      return ACPI_HID_CMN_600;

    case ArmCmnType650:
      return ACPI_HID_CMN_650;

    case ArmCmnType700:
      return ACPI_HID_CMN_700;

    case ArmCmnTypeS3:
      return ACPI_HID_CMN_S3;

    default:
      return NULL;
  }
}

/** Check the CMN information.

  @param [in]  CmnInfoList        Array of CMN information structure.
  @param [in]  CmnCount           Count of CMN information structure.

  @retval  EFI_SUCCESS            The function completed successfully.
  @retval  EFI_INVALID_PARAMETER  Invalid parameter.
**/
STATIC
EFI_STATUS
EFIAPI
ValidateCmnInfo (
  IN  CONST CM_ARM_CMN_INFO  *CmnInfoList,
  IN  CONST UINT32           CmnCount
  )
{
  UINT32                                  Index;
  UINT32                                  DtcIndex;
  CONST CM_ARM_CMN_INFO                   *CmnInfo;
  CONST CM_ARCH_COMMON_GENERIC_INTERRUPT  *DtcInterrupt;

  if ((CmnInfoList == NULL) ||
      (CmnCount == 0))
  {
    return EFI_INVALID_PARAMETER;
  }

  // Validate each CmnInfo structure.
  for (Index = 0; Index < CmnCount; Index++) {
    CmnInfo = &CmnInfoList[Index];

    // Validate the CMN configuration region.
    if ((CmnInfo->PeriphBaseAddress == 0)        ||
        (CmnInfo->PeriphBaseAddressLength == 0) ||
        ((CmnInfo->PeriphBaseAddressLength - 1) >
         (MAX_UINT64 - CmnInfo->PeriphBaseAddress)))
    {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-CMN: Invalid configuration region.\n"
        ));
      goto error_handler;
    }

    // Check that the CMN PERIPHBASE address is at least 64 MB aligned.
    if ((CmnInfo->PeriphBaseAddress &
         (CMN_PERIPHBASE_ADDRESS_ALIGNMENT - 1)) != 0)
    {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-CMN: CMN configuration base must be "
        "64 MB aligned.\n"
        ));
      goto error_handler;
    }

    if (GetCmnHid (CmnInfo->CmnType) == NULL) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-CMN: Invalid CMN implementation type.\n"
        ));
      goto error_handler;
    }

    if ((CmnInfo->DtcCount == 0) ||
        (CmnInfo->DtcCount > MAX_CMN_DTC_COUNT))
    {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-CMN: Invalid DTC count.\n"
        ));
      goto error_handler;
    }

    if (CmnInfo->CmnType == ArmCmnType600) {
      if ((CmnInfo->RootNodeBaseAddressLength !=
           CMN600_ROOTNODEBASE_ADDRESS_LENGTH) ||
          ((CmnInfo->RootNodeBaseAddress &
            (CMN600_ROOTNODEBASE_ADDRESS_LENGTH - 1)) != 0) ||
          ((CmnInfo->RootNodeBaseAddressLength - 1) >
           (MAX_UINT64 - CmnInfo->RootNodeBaseAddress)))
      {
        DEBUG ((
          DEBUG_ERROR,
          "ERROR: SSDT-CMN: Invalid CMN-600 ROOT region.\n"
          ));
        goto error_handler;
      }

      // The ROOTNODEBASE address space should be included in the PERIPHBASE
      // address space.
      if ((CmnInfo->RootNodeBaseAddress < CmnInfo->PeriphBaseAddress) ||
          (CmnInfo->RootNodeBaseAddressLength >
           CmnInfo->PeriphBaseAddressLength) ||
          ((CmnInfo->RootNodeBaseAddress -
            CmnInfo->PeriphBaseAddress) >
           (CmnInfo->PeriphBaseAddressLength -
            CmnInfo->RootNodeBaseAddressLength)))
      {
        DEBUG ((
          DEBUG_ERROR,
          "ERROR: SSDT-CMN: ROOT region is outside the "
          "configuration region.\n"
          ));
        goto error_handler;
      }

      // For CMN-600, the PERIPHBASE address space is at most 64 MB for a
      // (X < 4) && (Y < 4) mesh, and 256 MB otherwise.
      // Check that it is not larger than 256 MB.
      if (CmnInfo->PeriphBaseAddressLength >
          CMN600_PERIPHBASE_MAX_ADDRESS_LENGTH)
      {
        DEBUG ((
          DEBUG_ERROR,
          "ERROR: SSDT-CMN: CMN configuration region exceeds "
          "256 MB.\n"
          ));
        goto error_handler;
      }
    } else if ((CmnInfo->RootNodeBaseAddress != 0) ||
               (CmnInfo->RootNodeBaseAddressLength != 0))
    {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-CMN: ROOT region is only valid for CMN-600.\n"
        ));
      goto error_handler;
    }

    for (DtcIndex = 0; DtcIndex < CmnInfo->DtcCount; DtcIndex++) {
      DtcInterrupt = &CmnInfo->DtcInterrupt[DtcIndex];
      if (((DtcInterrupt->Flags &
            EFI_ACPI_EXTENDED_INTERRUPT_FLAG_PRODUCER_CONSUMER_MASK) == 0))
      {
        DEBUG ((
          DEBUG_ERROR,
          "ERROR: SSDT-CMN: DTC Interrupt must be consumer.\n"
          ));
        goto error_handler;
      }
    } // for DTC Interrupt
  } // for CmnInfoList

  return EFI_SUCCESS;

error_handler:

  DEBUG ((
    DEBUG_ERROR,
    "PeriphBaseAddress = 0x%llx\n"
    "PeriphBaseAddressLength = 0x%llx\n"
    "RootNodeBaseAddress = 0x%llx\n"
    "DtcCount = %u\n",
    CmnInfo->PeriphBaseAddress,
    CmnInfo->PeriphBaseAddressLength,
    CmnInfo->RootNodeBaseAddress,
    CmnInfo->DtcCount
    ));

  DEBUG_CODE_BEGIN ();
  for (DtcIndex = 0;
       (DtcIndex < CmnInfo->DtcCount) &&
       (DtcIndex < MAX_CMN_DTC_COUNT);
       DtcIndex++)
  {
    DtcInterrupt = &CmnInfo->DtcInterrupt[DtcIndex];
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

/** Build a SSDT table describing the CMN device.

  The table created by this function must be freed by FreeSsdtCmnTableResourcesEx.

  @param [in]  CmnInfo       Pointer to a CMN structure.
  @param [in]  Name             The Name to give to the Device.
                                Must be a NULL-terminated ASL NameString
                                e.g.: "DEV0", "DV15.DEV0", etc.
  @param [in]  Uid              UID for the CMN device.
  @param [out] Table            If success, pointer to the created SSDT table.

  @retval EFI_SUCCESS            Table generated successfully.
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.
  @retval EFI_NOT_FOUND          Could not find information.
  @retval EFI_OUT_OF_RESOURCES   Could not allocate memory.
**/
STATIC
EFI_STATUS
EFIAPI
FixupCmnInfo (
  IN  CONST CM_ARM_CMN_INFO              *CmnInfo,
  IN  CONST CHAR8                        *Name,
  IN  CONST UINT64                       Uid,
  OUT       EFI_ACPI_DESCRIPTION_HEADER  **Table
  )
{
  AML_OBJECT_NODE_HANDLE                  NameOpHidNode;
  EFI_STATUS                              Status;
  EFI_STATUS                              Status1;
  UINT8                                   Index;
  CONST CHAR8                             *Hid;
  CONST CM_ARCH_COMMON_GENERIC_INTERRUPT  *DtcInt;

  EFI_ACPI_DESCRIPTION_HEADER  *SsdtCmnTemplate;
  AML_ROOT_NODE_HANDLE         RootNodeHandle;
  AML_OBJECT_NODE_HANDLE       NameOpIdNode;
  AML_OBJECT_NODE_HANDLE       NameOpCrsNode;
  AML_DATA_NODE_HANDLE         CmnPeriphBaseRdNode;
  AML_DATA_NODE_HANDLE         CmnRootNodeBaseRdNode;
  AML_OBJECT_NODE_HANDLE       DeviceNode;

  Hid = GetCmnHid (CmnInfo->CmnType);
  if (Hid == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // Parse the Ssdt CMN Template.
  SsdtCmnTemplate = (EFI_ACPI_DESCRIPTION_HEADER *)
                    ssdtcmntemplate_aml_code;

  RootNodeHandle = NULL;
  Status         = AmlParseDefinitionBlock (
                     SsdtCmnTemplate,
                     &RootNodeHandle
                     );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-CMN: Failed to parse SSDT CMN Template."
      " Status = %r\n",
      Status
      ));
    return Status;
  }

  Status = AmlFindNode (
             RootNodeHandle,
             "\\_SB_.CMN0._HID",
             &NameOpHidNode
             );

  if (EFI_ERROR (Status)) {
    goto error_handler;
  }

  Status = AmlNameOpUpdateString (NameOpHidNode, Hid);
  if (EFI_ERROR (Status)) {
    goto error_handler;
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
  Status = AmlNameOpGetFirstRdNode (
             NameOpCrsNode,
             &CmnPeriphBaseRdNode
             );
  if (EFI_ERROR (Status)) {
    goto error_handler;
  }

  if (CmnPeriphBaseRdNode == NULL) {
    Status = EFI_INVALID_PARAMETER;
    goto error_handler;
  }

  // Get the second resource descriptor: the optional ROOT region.
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

  // Update the PERIPHBASE base address and length.
  Status = AmlUpdateRdQWord (
             CmnPeriphBaseRdNode,
             CmnInfo->PeriphBaseAddress,
             CmnInfo->PeriphBaseAddressLength
             );
  if (EFI_ERROR (Status)) {
    goto error_handler;
  }

  if (CmnInfo->RootNodeBaseAddressLength != 0) {
    Status = AmlUpdateRdQWord (
               CmnRootNodeBaseRdNode,
               CmnInfo->RootNodeBaseAddress,
               CmnInfo->RootNodeBaseAddressLength
               );
    if (EFI_ERROR (Status)) {
      goto error_handler;
    }
  } else {
    Status = AmlDetachNode (CmnRootNodeBaseRdNode);
    if (EFI_ERROR (Status)) {
      goto error_handler;
    }

    Status = AmlDeleteTree (CmnRootNodeBaseRdNode);
    if (EFI_ERROR (Status)) {
      goto error_handler;
    }

    CmnRootNodeBaseRdNode = NULL;
  }

  // Add the Interrupt node(s).
  // Generate Resource Data node(s) corresponding to the "Interrupt ()"
  // ASL function and add it at the last position in the list of
  // Resource Data nodes.
  for (Index = 0; Index < CmnInfo->DtcCount; Index++) {
    DtcInt = &CmnInfo->DtcInterrupt[Index];

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

  // Fixup the CMN device name.
  // This MUST be done at the end, otherwise AML paths won't be valid anymore.
  // Get the CMN0 variable defined by the "Device ()" statement.
  Status = AmlFindNode (RootNodeHandle, "\\_SB_.CMN0", &DeviceNode);
  if (EFI_ERROR (Status)) {
    goto error_handler;
  }

  // Update the CMN Device's name.
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
      "ERROR: SSDT-CMN: Failed to Serialize SSDT Table Data."
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
        "ERROR: SSDT-CMN: Failed to cleanup AML tree."
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

/** Free any resources allocated for constructing the SSDT tables for CMN.

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
FreeSsdtCmnTableResourcesEx (
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
    DEBUG ((DEBUG_ERROR, "ERROR: SSDT-CMN: Invalid Table Pointer\n"));
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
        "ERROR: SSDT-CMN: Could not free SSDT table at index %d."
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

/** Construct SSDT tables for describing CMN meshes.

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
BuildSsdtCmnTableEx (
  IN  CONST ACPI_TABLE_GENERATOR                           *This,
  IN  CONST CM_STD_OBJ_ACPI_TABLE_INFO             *CONST  AcpiTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL   *CONST  CfgMgrProtocol,
  OUT       EFI_ACPI_DESCRIPTION_HEADER                    ***Table,
  OUT       UINTN                                  *CONST  TableCount
  )
{
  EFI_STATUS                   Status;
  UINT64                       Index;
  CM_ARM_CMN_INFO              *CmnInfo;
  UINT32                       CmnCount;
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

  // Get CMN information.
  Status = GetEArmObjCmnInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &CmnInfo,
             &CmnCount
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-CMN: Failed to get the CMN information."
      " Status = %r\n",
      Status
      ));
    return Status;
  }

  if ((CmnCount == 0) || (CmnCount > MAX_CMN_DEVICES_SUPPORTED)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-CMN: CMN device count = %d."
      " This must be between 1 to 16.\n",
      CmnCount
      ));
    return EFI_INVALID_PARAMETER;
  }

  // Validate the CMN Info.
  Status = ValidateCmnInfo (CmnInfo, CmnCount);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-CMN: Invalid CMN information. Status = %r\n",
      Status
      ));
    return Status;
  }

  // Allocate a table to store pointers to the SSDT tables.
  TableList = (EFI_ACPI_DESCRIPTION_HEADER **)
              AllocateZeroPool (
                (sizeof (EFI_ACPI_DESCRIPTION_HEADER *) * CmnCount)
                );
  if (TableList == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-CMN: Failed to allocate memory for Table List."
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
  for (Index = 0; Index < CmnCount; Index++) {
    NewName[3] = AsciiFromHex ((UINT8)(Index));

    // Build a SSDT table describing the CMN device.
    Status = FixupCmnInfo (
               &CmnInfo[Index],
               NewName,
               Index,
               &TableList[Index]
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-CMN: Failed to build associated SSDT table."
        " Status = %r\n",
        Status
        ));
      break;
    }

    // Increment the table count here so that appropriate clean-up
    // can be done in case of failure.
    *TableCount += 1;
  } // for

  // Note: Table list and CMN device count has been setup. The
  // framework will invoke FreeSsdtCmnTableResourcesEx() even
  // on failure, so appropriate clean-up will be done.
  return Status;
}

/** This macro defines the Raw Generator revision.
*/
#define SSDT_CMN_GENERATOR_REVISION  CREATE_REVISION (1, 0)

/** The interface for the Raw Table Generator.
*/
STATIC
CONST
ACPI_TABLE_GENERATOR  SsdtCmnGenerator = {
  // Generator ID
  CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdSsdtCmn),
  // Generator Description
  L"ACPI.STD.SSDT.CMN.GENERATOR",
  // ACPI Table Signature
  EFI_ACPI_6_3_SECONDARY_SYSTEM_DESCRIPTION_TABLE_SIGNATURE,
  // ACPI Table Revision - Unused
  0,
  // Minimum ACPI Table Revision - Unused
  0,
  // Creator ID
  TABLE_GENERATOR_CREATOR_ID_ARM,
  // Creator Revision
  SSDT_CMN_GENERATOR_REVISION,
  // Build table function. Use the extended version instead.
  NULL,
  // Free table function. Use the extended version instead.
  NULL,
  // Build Table function
  BuildSsdtCmnTableEx,
  // Free Resource function
  FreeSsdtCmnTableResourcesEx
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
AcpiSsdtCmnLibConstructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = RegisterAcpiTableGenerator (&SsdtCmnGenerator);
  DEBUG ((
    DEBUG_INFO,
    "SSDT-CMN: Register Generator. Status = %r\n",
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
AcpiSsdtCmnLibDestructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = DeregisterAcpiTableGenerator (&SsdtCmnGenerator);
  DEBUG ((
    DEBUG_INFO,
    "SSDT-CMN: Deregister Generator. Status = %r\n",
    Status
    ));
  ASSERT_EFI_ERROR (Status);
  return Status;
}
