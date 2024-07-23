/** @file
  RISC-V SSDT PLIC/APLIC namespace Generator.

  Copyright (c) 2024-2025, Ventana Micro Systems Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - RISC-V Boot and Runtime Services (BRS) Specification : Section 6.2

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
  @param [in, out]  ScopeNode        ScopeNode of the AML tree to populate.

  @retval EFI_SUCCESS               The function completed successfully.
  @retval EFI_INVALID_PARAMETER     Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES      Could not allocate memory.
**/
STATIC
EFI_STATUS
EFIAPI
GeneratePlicAplicDevice (
  IN            PLIC_APLIC_COMMON_INFO  *PlicAplicInfo,
  IN            CHAR8                   *Hid,
  IN            UINT32                  Uid,
  IN            AML_OBJECT_NODE_HANDLE  ScopeNode
  )
{
  AML_OBJECT_NODE_HANDLE  IcNode;
  AML_OBJECT_NODE_HANDLE  CrsNode;
  EFI_STATUS              Status;
  CHAR8                   AslName[AML_NAME_SEG_SIZE + 1];

  ASSERT (PlicAplicInfo != NULL);
  ASSERT (Hid != NULL);
  ASSERT (Uid <= 0xFF);

  IcNode = NULL;

  // Write the name of the Interrupt Controller device.
  CopyMem (AslName, "ICxx", AML_NAME_SEG_SIZE + 1);
  AslName[AML_NAME_SEG_SIZE - 1] = AsciiFromHex (Uid & 0xF);
  AslName[AML_NAME_SEG_SIZE - 2] = AsciiFromHex ((Uid >> 4) & 0xF);

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
BuildSsdtPlicAplicTable (
  IN  CONST ACPI_TABLE_GENERATOR                           *This,
  IN  CONST CM_STD_OBJ_ACPI_TABLE_INFO             *CONST  AcpiTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL   *CONST  CfgMgrProtocol,
  OUT       EFI_ACPI_DESCRIPTION_HEADER                    **Table
  )
{
  AML_ROOT_NODE_HANDLE    RootNode;
  AML_OBJECT_NODE_HANDLE  ScopeNode;
  ACPI_TABLE_GENERATOR    *Generator;
  CM_RISCV_APLIC_INFO     *AplicInfo;
  CM_RISCV_PLIC_INFO      *PlicInfo;
  EFI_STATUS              Status;
  EFI_STATUS              Status1;
  UINT32                  Index;
  UINT32                  PlicCount;
  UINT32                  AplicCount;
  UINT32                  Count;
  UINT32                  Uid;

  ASSERT (This != NULL);
  ASSERT (AcpiTableInfo != NULL);
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (Table != NULL);
  ASSERT (AcpiTableInfo->TableGeneratorId == This->GeneratorID);
  ASSERT (AcpiTableInfo->AcpiTableSignature == This->AcpiTableSignature);

  Generator = (ACPI_TABLE_GENERATOR *)This;

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

  // ASL: Scope (\_SB) {}
  Status = AmlCodeGenScope (SB_SCOPE, RootNode, &ScopeNode);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  for (Index = 0; Index < Count; Index++) {
    Uid    = Index;
    Status = GeneratePlicAplicDevice (
               AplicCount ? &AplicInfo[Index].PlicAplicCommonInfo :
               &PlicInfo[Index].PlicAplicCommonInfo,
               AplicCount ? "RSCV0002" : "RSCV0001",
               Uid,
               ScopeNode
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
  } // for

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

/** Free any resources allocated for constructing the tables.

  @param [in]      This           Pointer to the ACPI table generator.
  @param [in]      AcpiTableInfo  Pointer to the ACPI Table Info.
  @param [in]      CfgMgrProtocol Pointer to the Configuration Manager
                                  Protocol Interface.
  @param [in, out] Table          Pointer to an array of pointers
                                  to ACPI Table(s).

  @retval EFI_SUCCESS           The resources were freed successfully.
  @retval EFI_INVALID_PARAMETER The table pointer is NULL or invalid.
**/
STATIC
EFI_STATUS
EFIAPI
FreeSsdtPlicAplicTable (
  IN      CONST ACPI_TABLE_GENERATOR                   *CONST   This,
  IN      CONST CM_STD_OBJ_ACPI_TABLE_INFO             *CONST   AcpiTableInfo,
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL   *CONST   CfgMgrProtocol,
  IN OUT        EFI_ACPI_DESCRIPTION_HEADER            **CONST  Table
  )
{
  ASSERT (This != NULL);
  ASSERT (AcpiTableInfo != NULL);
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (AcpiTableInfo->TableGeneratorId == This->GeneratorID);
  ASSERT (AcpiTableInfo->AcpiTableSignature == This->AcpiTableSignature);

  if ((Table == NULL) || (*Table == NULL)) {
    DEBUG ((DEBUG_ERROR, "ERROR: SSDT-PLIC-APLIC: Invalid Table Pointer\n"));
    return EFI_INVALID_PARAMETER;
  }

  // Free the table list.
  FreePool (*Table);
  *Table = NULL;
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
  TABLE_GENERATOR_CREATOR_ID,
  // Creator Revision
  SSDT_PLIC_APLIC_GENERATOR_REVISION,
  // Build table function. Use the extended version instead.
  BuildSsdtPlicAplicTable,
  // Free table function. Use the extended version instead.
  FreeSsdtPlicAplicTable,
  // Extended Build table function.
  NULL,
  // Extended free function.
  NULL,
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
