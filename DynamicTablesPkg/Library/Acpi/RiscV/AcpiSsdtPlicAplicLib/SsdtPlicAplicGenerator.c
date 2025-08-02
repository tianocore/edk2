/** @file
  RISC-V SSDT PLIC/APLIC namespace Generator.

  Copyright (c) 2024, Ventana Micro Systems Inc. All rights reserved.<BR>

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
#include <Library/TableHelperLib.h>
#include <Library/AmlLib/AmlLib.h>
#include <Protocol/ConfigurationManagerProtocol.h>

// _SB scope of the AML namespace.
#define SB_SCOPE  "\\_SB_"

/** RISC-V SSDT PLIC/APLIC namespace device Generator.

Requirements:
  The following Configuration Manager Object(s) are used by
  this Generator:
  - ERiscVObjAplicInfo
  - ERiscVObjPlicInfo
*/

/** This macro expands to a function that retrieves the APLIC
    Information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceRiscV,
  ERiscVObjAplicInfo,
  CM_RISCV_APLIC_INFO
  );

/** This macro expands to a function that retrieves the PLIC
    Information from the Configuration Manager.
*/

GET_OBJECT_LIST (
  EObjNameSpaceRiscV,
  ERiscVObjPlicInfo,
  CM_RISCV_PLIC_INFO
  );

/** Generate a PLIC/APLIC device.

  @param [in]       Generator       The SSDT PlicAplic generator.
  @param [in]       CfgMgrProtocol  Pointer to the Configuration Manager
                                    Protocol interface.
  @param [in]       BaseAddress     MMIO base of the PLIC/APLIC device.
  @param [in]       Size            MMIO size of the PLIC/APLIC device.
  @param [in]       GsiBase         GSI Base of the PLIC/APLIC device.
  @param [in]       Uid             Unique Id of the PlicAplic device.
  @param [in]       Hid             HID/CID of the interrupt controller device.
  @param [in, out]  RootNode        RootNode of the AML tree to populate.

  @retval EFI_SUCCESS               The function completed successfully.
  @retval EFI_INVALID_PARAMETER     Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES      Could not allocate memory.
**/
STATIC
EFI_STATUS
EFIAPI
GeneratePlicAplicDevice (
  IN      CONST ACPI_TABLE_GENERATOR                          *Generator,
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN            PLIC_APLIC_COMMON_INFO                        *PlicAplicInfo,
  IN            CHAR8                                         *Hid,
  IN            UINT32                                        Uid,
  IN  OUT       AML_ROOT_NODE_HANDLE                          *RootNode
  )
{
  AML_OBJECT_NODE_HANDLE  ScopeNode;
  AML_OBJECT_NODE_HANDLE  IcNode;
  AML_OBJECT_NODE_HANDLE  CrsNode;
  EFI_STATUS              Status;
  CHAR8                   AslName[AML_NAME_SEG_SIZE + 1];

  ASSERT (Generator != NULL);
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (RootNode != NULL);
  ASSERT (PlicAplicInfo != NULL);

  IcNode = NULL;

  // ASL: Scope (\_SB) {}
  Status = AmlCodeGenScope (SB_SCOPE, RootNode, &ScopeNode);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  ASSERT (Uid <= 0xFF);
  // Write the name of the Interrupt Controller device.
  CopyMem (AslName, "ICxx", AML_NAME_SEG_SIZE + 1);
  AslName[AML_NAME_SEG_SIZE - 2] = AsciiFromHex (Uid & 0xFF);

  // ASL: Device (ICxx) {}
  Status = AmlCodeGenDevice (AslName, ScopeNode, &IcNode);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // ASL: Name (_UID, <Uid>)
  Status = AmlCodeGenNameInteger ("_UID", Uid, IcNode, NULL);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  Status = AmlCodeGenNameString (
             "_HID",
             Hid,
             IcNode,
             NULL
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // ASL: Name (_CRS, ResourceTemplate () {})
  Status = AmlCodeGenNameResourceTemplate ("_CRS", IcNode, &CrsNode);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  Status = AmlCodeGenRdMemory32Fixed (
             TRUE,
             PlicAplicInfo->BaseAddress,
             PlicAplicInfo->Size,
             CrsNode,
             NULL
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  Status = AmlCodeGenNameInteger ("_GSB", PlicAplicInfo->GsiBase, IcNode, NULL);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  return Status;
}

/** Build an Ssdt table describing a PlicAplic device.

  @param [in]  Generator        The SSDT PlicAplic generator.
  @param [in]  CfgMgrProtocol   Pointer to the Configuration Manager
                                Protocol interface.
  @param [in]  AcpiTableInfo    Pointer to the ACPI table information.
  @param [in]  BaseAddress      MMIO base of the PLIC/APLIC device.
  @param [in]  Size             MMIO size of the PLIC/APLIC device.
  @param [in]  GsiBase          GSI Base of the PLIC/APLIC device.
  @param [in]  Hid              HID/CID of the interrupt controller device.
  @param [in]  Uid              Unique Id of the PlicAplic device.
  @param [out] Table            If success, contains the created SSDT table.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Could not allocate memory.
**/
STATIC
EFI_STATUS
EFIAPI
BuildSsdtPlicAplicTable (
  IN        ACPI_TABLE_GENERATOR                          *Generator,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN  CONST CM_STD_OBJ_ACPI_TABLE_INFO            *CONST  AcpiTableInfo,
  IN        PLIC_APLIC_COMMON_INFO                        *PlicAplicInfo,
  IN        CHAR8                                         *Hid,
  IN        UINT32                                        Uid,
  OUT       EFI_ACPI_DESCRIPTION_HEADER                   **Table
  )
{
  AML_ROOT_NODE_HANDLE  RootNode;
  EFI_STATUS            Status;
  EFI_STATUS            Status1;

  ASSERT (Generator != NULL);
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (Table != NULL);

  // Create a new Ssdt table.
  Status = AddSsdtAcpiHeader (
             CfgMgrProtocol,
             Generator,
             AcpiTableInfo,
             &RootNode
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  Status = GeneratePlicAplicDevice (
             Generator,
             CfgMgrProtocol,
             PlicAplicInfo,
             Hid,
             Uid,
             RootNode
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
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
      "ERROR: SSDT-PLIC: Failed to Serialize SSDT Table Data."
      " Status = %r\n",
      Status
      ));
  }

exit_handler:
  // Cleanup
  Status1 = AmlDeleteTree (RootNode);
  if (EFI_ERROR (Status1)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-PLIC: Failed to cleanup AML tree."
      " Status = %r\n",
      Status1
      ));
    // If Status was success but we failed to delete the AML Tree
    // return Status1 else return the original error code, i.e. Status.
    if (!EFI_ERROR (Status)) {
      return Status1;
    }
  }

  return Status;
}

/** Construct SSDT tables describing PLIC/APLIC devices

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
BuildSsdtPlicAplicTableEx (
  IN  CONST ACPI_TABLE_GENERATOR                           *This,
  IN  CONST CM_STD_OBJ_ACPI_TABLE_INFO             *CONST  AcpiTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL   *CONST  CfgMgrProtocol,
  OUT       EFI_ACPI_DESCRIPTION_HEADER                    ***Table,
  OUT       UINTN                                  *CONST  TableCount
  )
{
  EFI_ACPI_DESCRIPTION_HEADER  **TableList;
  ACPI_TABLE_GENERATOR         *Generator;
  CM_RISCV_APLIC_INFO          *AplicInfo;
  CM_RISCV_PLIC_INFO           *PlicInfo;
  EFI_STATUS                   Status;
  UINT32                       Index;
  UINT32                       PlicCount;
  UINT32                       AplicCount;
  UINT32                       Count;
  UINT32                       Uid;

  ASSERT (This != NULL);
  ASSERT (AcpiTableInfo != NULL);
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (Table != NULL);
  ASSERT (TableCount != NULL);
  ASSERT (AcpiTableInfo->TableGeneratorId == This->GeneratorID);
  ASSERT (AcpiTableInfo->AcpiTableSignature == This->AcpiTableSignature);

  *TableCount = 0;
  Generator   = (ACPI_TABLE_GENERATOR *)This;

  Status = GetERiscVObjAplicInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &AplicInfo,
             &AplicCount
             );
  if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    ASSERT (0);
    return Status;
  } else if (Status == EFI_NOT_FOUND) {
    Status = GetERiscVObjPlicInfo (
               CfgMgrProtocol,
               CM_NULL_TOKEN,
               &PlicInfo,
               &PlicCount
               );
    if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
      ASSERT (0);
      return Status;
    }
  }

  Count = (AplicCount != 0) ? AplicCount : PlicCount;

  if (Count == 0) {
    Status = EFI_NOT_FOUND;
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-PLIC-APLIC: Failed to find any PLIC or APLIC."
      " Status = %r\n",
      Status
      ));
    return Status;
  }

  // Allocate a table to store pointers to the SSDT tables.
  TableList = (EFI_ACPI_DESCRIPTION_HEADER **)
              AllocateZeroPool (
                (sizeof (EFI_ACPI_DESCRIPTION_HEADER *) * Count)
                );
  if (TableList == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-PLIC-APLIC: Failed to allocate memory for Table List."
      " Status = %r\n",
      Status
      ));
    return Status;
  }

  // Setup the table list early so that appropriate cleanup
  // can be done in case of failure.
  *Table = TableList;

  for (Index = 0; Index < Count; Index++) {
    Uid = Index;
    if (AplicCount != 0) {
      // Build a SSDT table describing the APLIC devices.
      Status = BuildSsdtPlicAplicTable (
                 Generator,
                 CfgMgrProtocol,
                 AcpiTableInfo,
                 &AplicInfo[Index].PlicAplicCommonInfo,
                 "RSCV0002",
                 Uid,
                 &TableList[Index]
                 );
      if (EFI_ERROR (Status)) {
        DEBUG ((
          DEBUG_ERROR,
          "ERROR: SSDT-PLIC-APLIC: Failed to build associated SSDT table for "
          "APLIC/PLIC %d. Status = %r\n",
          Index,
          Status
          ));
        goto error_handler;
      }
    } else if (PlicCount != 0) {
      // Build a SSDT table describing the PLIC devices.
      Status = BuildSsdtPlicAplicTable (
                 Generator,
                 CfgMgrProtocol,
                 AcpiTableInfo,
                 &PlicInfo[Index].PlicAplicCommonInfo,
                 "RSCV0001",
                 Uid,
                 &TableList[Index]
                 );
      if (EFI_ERROR (Status)) {
        DEBUG ((
          DEBUG_ERROR,
          "ERROR: SSDT-PLIC-APLIC: Failed to build associated SSDT table."
          " Status = %r\n",
          Status
          ));
        goto error_handler;
      }
    } else {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-PLIC-APLIC: Neither PLIC not APLIC found, Failed to"
        " build associated SSDT table. Status = %r\n",
        Status
        ));
      goto error_handler;
    }

    *TableCount += 1;
  } // for

error_handler:
  // Note: Table list and Table count have been setup. The
  // error handler does nothing here as the framework will invoke
  // FreeSsdtPlicAplicTableEx () even on failure.
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
FreeSsdtPlicAplicTableEx (
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
    DEBUG ((DEBUG_ERROR, "ERROR: SSDT-PLIC-APLIC: Invalid Table Pointer\n"));
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
        "ERROR: SSDT-PLIC-APLIC: Could not free SSDT table at index %d.",
        Index
        ));
      return EFI_INVALID_PARAMETER;
    }
  } // for

  // Free the table list.
  FreePool (*Table);

  return EFI_SUCCESS;
}

/** This macro defines the SSDT PlicAplic Table Generator revision.
*/
#define SSDT_PLIC_APLIC_GENERATOR_REVISION  CREATE_REVISION (1, 0)

/** The interface for the RISC-V SSDT PLIC/APLIC Table Generator.
*/
STATIC
CONST
ACPI_TABLE_GENERATOR  SsdtPlicAplicGenerator = {
  // Generator ID
  CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdSsdtPlicAplic),
  // Generator Description
  L"ACPI.STD.SSDT.PLIC.APLIC.GENERATOR",
  // ACPI Table Signature
  EFI_ACPI_6_3_SECONDARY_SYSTEM_DESCRIPTION_TABLE_SIGNATURE,
  // ACPI Table Revision - Unused
  0,
  // Minimum ACPI Table Revision - Unused
  0,
  // Creator ID
  TABLE_GENERATOR_CREATOR_ID_RISCV,
  // Creator Revision
  SSDT_PLIC_APLIC_GENERATOR_REVISION,
  // Build table function. Use the extended version instead.
  NULL,
  // Free table function. Use the extended version instead.
  NULL,
  // Extended Build table function.
  BuildSsdtPlicAplicTableEx,
  // Extended free function.
  FreeSsdtPlicAplicTableEx
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
AcpiSsdtPlicAplicLibConstructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = RegisterAcpiTableGenerator (&SsdtPlicAplicGenerator);
  DEBUG ((
    DEBUG_INFO,
    "SSDT-PLIC-APLIC: Register Generator. Status = %r\n",
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
AcpiSsdtPlicAplicLibDestructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = DeregisterAcpiTableGenerator (&SsdtPlicAplicGenerator);
  DEBUG ((
    DEBUG_INFO,
    "SSDT-PLIC-APLIC: Deregister Generator. Status = %r\n",
    Status
    ));
  ASSERT_EFI_ERROR (Status);
  return Status;
}
