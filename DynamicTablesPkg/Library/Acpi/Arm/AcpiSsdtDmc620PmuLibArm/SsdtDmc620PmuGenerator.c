/** @file
  SSDT DMC620 AML Table Generator.

  Copyright (c) 2025, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - Arm CoreLink DMC-620 Dynamic Memory Controller Technical Reference Manual r1p0
  - ACPI for the Arm Components 1.2 EAC1 Platform Design Document,
      dated July 2025.
    (https://developer.arm.com/documentation/den0093/1-2eac1/)
**/

#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>

// Module specific include files.
#include <AcpiTableGenerator.h>
#include <ConfigurationManagerHelper.h>
#include <Library/AcpiHelperLib.h>
#include <Library/AmlLib/AmlLib.h>
#include <Protocol/ConfigurationManagerProtocol.h>
#include "SsdtDmc620PmuGenerator.h"

/** SSDT DMC620 PMU Table Generator.

  Requirements:
  The following Configuration Manager Object(s) are required by
  this Generator:
  - EArmObjDmc620SocketInfo
  - EArmObjDmc620PmuRegInfo
*/

/** This macro expands to a function that retrieves the DMC620 PMU
    Socket Information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArm,
  EArmObjDmc620PmuSocketInfo,
  CM_ARM_DMC620_PMU_SOCKET_INFO
  );

/** This macro expands to a function that retrieves the DMC620 PMU
    Register Information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArm,
  EArmObjDmc620PmuRegInfo,
  CM_ARM_DMC620_PMU_REG_INFO
  );

/** Check the DMC620 PMU Information for a given socket.

  @param [in] Dmc620PmuRegInfo         Array of DMC620 information structure.
  @param [in] DevCount                 Count of DMC620 devices to validate.

  @retval  EFI_SUCCESS            The function completed successfully.
  @retval  EFI_INVALID_PARAMETER  Invalid parameter.
**/
STATIC
EFI_STATUS
EFIAPI
ValidateDmc620PmuInfo (
  IN  CONST CM_ARM_DMC620_PMU_REG_INFO  *CONST  Dmc620PmuRegInfo,
  IN        UINT32                              DevCount
  )
{
  UINT32                                  DevNum;
  CONST CM_ARM_DMC620_PMU_REG_INFO        *RegInfo;
  CONST CM_ARCH_COMMON_GENERIC_INTERRUPT  *PmuIntr;

  if ((Dmc620PmuRegInfo == NULL) || (DevCount == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  for (DevNum = 0; DevNum < DevCount; DevNum++) {
    RegInfo = &Dmc620PmuRegInfo[DevNum];
    // Check Base address is initialized
    if (RegInfo->BaseAddress == 0) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-DMC620: Invalid PMU Base Address.\n"
        ));
      goto error_handler;
    }

    if (RegInfo->Length != 0x200) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-DMC620: Invalid PMU Length.\n"
        ));
      goto error_handler;
    }

    // The PMU registers in the DMC620 start at an offset of
    // 0xA00. Check that that is so.
    if ((RegInfo->BaseAddress & DMC620_REGISTER_SPACE_MASK) !=
        DMC620_PMU_ADDRESS_OFFSET)
    {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-DMC620: PMU Address offset must be 0xA00.\n"
        ));
      goto error_handler;
    }

    PmuIntr = &RegInfo->PmuIntr;
    if ((PmuIntr->Flags & BIT0) != 0) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-DMC-620: PMU Interrupt must be Level Triggered.\n"
        ));
      goto error_handler;
    }

    if ((PmuIntr->Flags & BIT1) != 0) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-DMC-620: PMU Interrupt must be Active High.\n"
        ));
      goto error_handler;
    }
  }

  return EFI_SUCCESS;

error_handler:

  DEBUG ((
    DEBUG_ERROR,
    "PmuBaseAddress = 0x%llx\n"
    "PmuBaseAddressLength = 0x%llx\n"
    "PmuInterrupt = 0x%lx\n"
    "PmuInterruptFlags = 0x%lx\n",
    RegInfo->BaseAddress,
    RegInfo->Length,
    RegInfo->PmuIntr.Interrupt,
    RegInfo->PmuIntr.Flags
    ));

  return EFI_INVALID_PARAMETER;
}

/**
  Create the _CRS (Current Resource Settings) AML node for the device.

  @param [in]  Dmc620PmuRegInfo   Pointer to the register info structure.
  @param [in]  DeviceNode         AML device node handle.

  @retval EFI_SUCCESS           The CRS node was created successfully.
  @retval Others                Failed to create CRS node.
**/
STATIC
EFI_STATUS
EFIAPI
CreateDmc620PmuCrs (
  IN CONST CM_ARM_DMC620_PMU_REG_INFO        *CONST  Dmc620PmuRegInfo,
  IN       AML_OBJECT_NODE_HANDLE                    DeviceNode
  )
{
  UINT32                  Intr;
  UINT64                  Length;
  UINT64                  BaseAddress;
  EFI_STATUS              Status;
  AML_OBJECT_NODE_HANDLE  CrsNode;

  Status = AmlCodeGenNameResourceTemplate ("_CRS", DeviceNode, &CrsNode);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-DMC620-AML-CODEGEN: Failed to create AML _CRS Node."
      " Status = %r\n",
      Status
      ));
    return Status;
  }

  BaseAddress = Dmc620PmuRegInfo->BaseAddress;
  Length      = Dmc620PmuRegInfo->Length;
  Status      = AmlCodeGenRdQWordMemory (
                  FALSE,
                  TRUE,
                  TRUE,
                  TRUE,
                  0,
                  TRUE,
                  0x0,
                  BaseAddress,
                  BaseAddress + Length - 1,
                  0,
                  Length,
                  0,
                  NULL,
                  0,
                  TRUE,
                  CrsNode,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-SERIAL-PORT-FIXUP: Failed to create AML QWordMemory Node."
      " Status = %r\n",
      Status
      ));
    return Status;
  }

  Intr   = Dmc620PmuRegInfo->PmuIntr.Interrupt;
  Status = AmlCodeGenRdInterrupt (
             TRUE,
             FALSE,
             FALSE,
             FALSE,
             &Intr,
             1,
             CrsNode,
             NULL
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-SERIAL-PORT-FIXUP: Failed to create AML Interrupt Node."
      " Status = %r\n",
      Status
      ));
    return Status;
  }

  return EFI_SUCCESS;
}

/** Build a SSDT table describing the DMC620 PMU register space.

  Add device nodes describing the DMC620 PMU register space, one
  socket at a time.

  @param [in]  Uid               UID for the device.
  @param [in]  DevIndex          Start device number to be used on the
                                 Device node.
  @param [in]  SockNum           Socket number on which devices are
                                 present.
  @param [in]  DevCount          Number of devices on this socket.
  @param [in]  ScopeNode         AML System Bus node handle.
  @param [in]  Dmc620PmuRegInfo  Array of DMC620 information structure.

  @retval EFI_SUCCESS            Device nodes added successfully.
  @retval Others                 Failed to create the device nodes.
**/
STATIC
EFI_STATUS
EFIAPI
BuildSsdtDmc620PmuTable (
  IN       UINT64                                    Uid,
  IN       UINT32                                    DevIndex,
  IN CONST UINT32                                    SockNum,
  IN CONST UINT32                                    DevCount,
  IN CONST AML_OBJECT_NODE_HANDLE                    ScopeNode,
  IN CONST CM_ARM_DMC620_PMU_REG_INFO        *CONST  Dmc620PmuRegInfo
  )
{
  UINT32                  DevNum;
  CHAR8                   Name[AML_NAME_SEG_SIZE + 1];
  CHAR16                  Str[32];
  EFI_STATUS              Status;
  AML_OBJECT_NODE_HANDLE  DeviceNode;

  // Validate the DMC620 Info and get the number of devices.
  Status = ValidateDmc620PmuInfo (Dmc620PmuRegInfo, DevCount);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-DMC620: Invalid DMC620 PMU information. Status = %r\n",
      Status
      ));
    return Status;
  }

  Name[0] = 'M';
  Name[1] = 'C';
  Name[4] = '\0';

  for (DevNum = 0; DevNum < DevCount; DevNum++, Uid++, DevIndex++) {
    Name[2] = AsciiFromHex ((DevIndex >> 4) & 0xF);
    Name[3] = AsciiFromHex (DevIndex & 0xF);

    Status = AmlCodeGenDevice (Name, ScopeNode, &DeviceNode);
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-DMC620: Failed to create AML Device Node."
        " Status = %r\n",
        Status
        ));
      return Status;
    }

    Status = AmlCodeGenNameString (
               "_HID",
               "ARMHD620",
               DeviceNode,
               NULL
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-DMC620-AML-CODEGEN: Failed to create AML _HID Node."
        " Status = %r\n",
        Status
        ));
      return Status;
    }

    Status = AmlCodeGenNameString (
               "_CID",
               "ARMHD620",
               DeviceNode,
               NULL
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-DMC620-AML-CODEGEN: Failed to create AML _CID Node."
        " Status = %r\n",
        Status
        ));
      return Status;
    }

    Status = AmlCodeGenNameInteger ("_UID", Uid, DeviceNode, NULL);
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-DMC620-AML-CODEGEN: Failed to create AML _UID Node."
        " Status = %r\n",
        Status
        ));
      return Status;
    }

    Status = AmlCodeGenNameInteger ("_CCA", 1, DeviceNode, NULL);
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-DMC620-AML-CODEGEN: Failed to create AML _CCA Node."
        " Status = %r\n",
        Status
        ));
      return Status;
    }

    UnicodeSPrint (Str, sizeof (Str), L"Socket %u: MCU%u", SockNum, DevNum);
    Status = AmlCodeGenNameUnicodeString (
               "_STR",
               Str,
               DeviceNode,
               NULL
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-DMC620-AML-CODEGEN: Failed to create AML _STR Node."
        " Status = %r\n",
        Status
        ));
      return Status;
    }

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
        "ERROR: SSDT-DMC620-AML-CODEGEN: Failed to create AML _STA Node."
        " Status = %r\n",
        Status
        ));
      return Status;
    }

    Status = CreateDmc620PmuCrs (&Dmc620PmuRegInfo[DevNum], DeviceNode);
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-DMC620-AML-CODEGEN: Failed to create AML _CRS Node."
        " Status = %r\n",
        Status
        ));
      return Status;
    }
  }

  return EFI_SUCCESS;
}

/** Construct SSDT tables for describing DMC620 PMU interface.

  This function invokes the Configuration Manager protocol interface
  to get the required hardware information for generating the ACPI
  table.

  If this function allocates any resources then they must be freed
  in the FreeXXXXTableRes function.

  @param [in]  This            Pointer to the ACPI table generator.
  @param [in]  AcpiTableInfo   Pointer to the ACPI table information.
  @param [in]  CfgMgrProtocol  Pointer to the Configuration Manager
                               Protocol interface.
  @param [out] Table           Pointer to a list of generated ACPI table(s).
  @param [out] TableCount      Number of generated ACPI table(s).

  @retval EFI_SUCCESS            Table generated successfully.
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.
  @retval EFI_OUT_OF_RESOURCES   Could not allocate memory.
  @retval EFI_UNSUPPORTED        Unsupported configuration.
**/
STATIC
EFI_STATUS
EFIAPI
BuildSsdtDmc620PmuTableEx (
  IN  CONST ACPI_TABLE_GENERATOR                           *This,
  IN  CONST CM_STD_OBJ_ACPI_TABLE_INFO             *CONST  AcpiTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL   *CONST  CfgMgrProtocol,
  OUT       EFI_ACPI_DESCRIPTION_HEADER                    ***Table,
  OUT       UINTN                                  *CONST  TableCount
  )
{
  AML_ROOT_NODE_HANDLE           RootNode;
  AML_OBJECT_NODE_HANDLE         ScopeNode;
  EFI_STATUS                     Status;
  EFI_STATUS                     Status1;
  UINT64                         Uid;
  UINT32                         DevCount;
  UINT32                         SockNum;
  UINT32                         SocketCount;
  EFI_ACPI_DESCRIPTION_HEADER    **TableList;
  CM_ARM_DMC620_PMU_REG_INFO     *Dmc620PmuRegInfo;
  CM_ARM_DMC620_PMU_SOCKET_INFO  *Dmc620PmuSockInfo;

  ASSERT (This != NULL);
  ASSERT (AcpiTableInfo != NULL);
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (Table != NULL);
  ASSERT (TableCount != NULL);
  ASSERT (AcpiTableInfo->TableGeneratorId == This->GeneratorID);
  ASSERT (AcpiTableInfo->AcpiTableSignature == This->AcpiTableSignature);

  *Table      = NULL;
  *TableCount = 0;

  Status = GetEArmObjDmc620PmuSocketInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &Dmc620PmuSockInfo,
             &SocketCount
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-DMC620: Failed to get the DMC620 Socket information."
      " Status = %r\n",
      Status
      ));
    return Status;
  }

  if (SocketCount == 0) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-DMC620: Invalid DMC620 Socket information.\n"
      ));
    return EFI_INVALID_PARAMETER;
  }

  // Allocate a table to store pointers to the SSDT tables.
  TableList = (EFI_ACPI_DESCRIPTION_HEADER **)
              AllocateZeroPool (
                sizeof (EFI_ACPI_DESCRIPTION_HEADER *)
                );
  if (TableList == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-DMC620: Failed to allocate memory for Table List."
      " Status = %r\n",
      Status
      ));
    return Status;
  }

  // Setup the table list early so that appropriate cleanup
  // can be done in case of failure.
  *Table = TableList;

  Status = AmlCodeGenDefinitionBlock (
             "SSDT",
             "ARMLTD",
             "DMC-620",
             0x01,
             &RootNode
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-DMC620: Failed to create AML Definition Block."
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
      "ERROR: SSDT-DMC620: Failed to create AML Scope Node."
      " Status = %r\n",
      Status
      ));
    goto error_handler;
  }

  Uid = 0;
  for (SockNum = 0; SockNum < SocketCount; SockNum++) {
    DevCount = 0;
    Status   = GetEArmObjDmc620PmuRegInfo (
                 CfgMgrProtocol,
                 Dmc620PmuSockInfo[SockNum].Dmc620RegInfoToken,
                 &Dmc620PmuRegInfo,
                 &DevCount
                 );

    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-DMC620: Failed to get the DMC620 per socket device information.\n"
        " Status = %r\n",
        Status
        ));
      goto error_handler;
    }

    if ((DevCount == 0) || (Dmc620PmuSockInfo[SockNum].NumDevices != DevCount)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-DMC620: Invalid DMC620 device information.\n"
        ));
      goto error_handler;
    }

    Status = BuildSsdtDmc620PmuTable (
               Uid,
               Dmc620PmuSockInfo[SockNum].StartDevNum,
               SockNum,
               DevCount,
               ScopeNode,
               Dmc620PmuRegInfo
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-DMC620: Failed to build table for DMC620."
        " Status = %r\n",
        Status
        ));
      goto error_handler;
    }

    Uid += DevCount;
  }

  *TableCount = 1;
  // Serialize the tree.
  Status = AmlSerializeDefinitionBlock (
             RootNode,
             *Table
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-DMC620: Failed to Serialize SSDT Table Data."
      " Status = %r\n",
      Status
      ));
    goto error_handler;
  }

  return EFI_SUCCESS;

error_handler:
  if (RootNode != NULL) {
    Status1 = AmlDeleteTree (RootNode);
    if (EFI_ERROR (Status1)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-DMC620-AML-CODEGEN: Failed to cleanup AML tree."
        " Status = %r\n",
        Status1
        ));
    }
  }

  return Status;
}

/** Free any resources allocated for constructing the SSDT tables for DMC620 PMU.

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
FreeSsdtDmc620PmuTableResEx (
  IN      CONST ACPI_TABLE_GENERATOR                   *CONST  This,
  IN      CONST CM_STD_OBJ_ACPI_TABLE_INFO             *CONST  AcpiTableInfo,
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL   *CONST  CfgMgrProtocol,
  IN OUT        EFI_ACPI_DESCRIPTION_HEADER          ***CONST  Table,
  IN      CONST UINTN                                          TableCount
  )
{
  EFI_ACPI_DESCRIPTION_HEADER  **TableList;
  UINTN                        Index;
  UINT32                       TableSig;

  ASSERT (This != NULL);
  ASSERT (AcpiTableInfo != NULL);
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (AcpiTableInfo->TableGeneratorId == This->GeneratorID);
  ASSERT (AcpiTableInfo->AcpiTableSignature == This->AcpiTableSignature);

  if ((Table == NULL) || (*Table == NULL)  || (TableCount == 0)) {
    DEBUG ((DEBUG_ERROR, "ERROR: SSDT-DMC620: Invalid Table Pointer\n"));
    return EFI_INVALID_PARAMETER;
  }

  TableList = *Table;
  TableSig  = EFI_ACPI_6_6_SECONDARY_SYSTEM_DESCRIPTION_TABLE_SIGNATURE;

  for (Index = 0; Index < TableCount; Index++) {
    if ((TableList[Index] != NULL) && (TableList[Index]->Signature == TableSig)) {
      FreePool (TableList[Index]);
    } else {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-DMC620: Could not free SSDT table at index %d."
        " Status = %r\n",
        Index,
        EFI_INVALID_PARAMETER
        ));
      return EFI_INVALID_PARAMETER;
    }
  }

  // Free the table list.
  FreePool (*Table);
  *Table = NULL;
  return EFI_SUCCESS;
}

/** This macro defines the Raw Generator revision.
*/
#define SSDT_DMC620_PMU_GENERATOR_REVISION  CREATE_REVISION (1, 0)

/** The interface for the Raw Table Generator.
*/
STATIC
CONST
ACPI_TABLE_GENERATOR  SsdtDmc620PmuGenerator = {
  // Generator ID
  CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdSsdtDmc620Pmu),
  // Generator Description
  L"ACPI.STD.SSDT.DMC620.PMU.GENERATOR",
  // ACPI Table Signature
  EFI_ACPI_6_3_SECONDARY_SYSTEM_DESCRIPTION_TABLE_SIGNATURE,
  // ACPI Table Revision - Unused
  0,
  // Minimum ACPI Table Revision - Unused
  0,
  // Creator ID
  TABLE_GENERATOR_CREATOR_ID_ARM,
  // Creator Revision
  SSDT_DMC620_PMU_GENERATOR_REVISION,
  // Build table function. Use the extended version instead.
  NULL,
  // Free table function. Use the extended version instead.
  NULL,
  // Build Table function
  BuildSsdtDmc620PmuTableEx,
  // Free Resource function
  FreeSsdtDmc620PmuTableResEx
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
AcpiSsdtDmc620PmuConstructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = RegisterAcpiTableGenerator (&SsdtDmc620PmuGenerator);
  DEBUG ((
    DEBUG_INFO,
    "SSDT-DMC620: Register Generator. Status = %r\n",
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
AcpiSsdtDmc620PmuDestructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = DeregisterAcpiTableGenerator (&SsdtDmc620PmuGenerator);
  DEBUG ((
    DEBUG_INFO,
    "SSDT-DMC620: Deregister Generator. Status = %r\n",
    Status
    ));
  ASSERT_EFI_ERROR (Status);
  return Status;
}
