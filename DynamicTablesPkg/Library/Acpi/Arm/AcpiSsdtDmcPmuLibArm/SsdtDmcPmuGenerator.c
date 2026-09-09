/** @file
  SSDT DMC AML Table Generator.

  Copyright (c) 2025, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - Arm CoreLink DMC Dynamic Memory Controller Technical Reference Manual r1p0
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
#include <Library/TableHelperLib.h>
#include "SsdtDmcPmuGenerator.h"

/** SSDT DMC PMU Table Generator.

  Requirements:
  The following Configuration Manager Object(s) are required by
  this Generator:
  - EArmObjDmcPmuSocketInfo
  - EArmObjDmcPmuRegInfo
*/

/** This macro expands to a function that retrieves the DMC PMU
    Socket Information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArm,
  EArmObjDmcPmuSocketInfo,
  CM_ARM_DMC_INFO
  );

/** This macro expands to a function that retrieves the DMC PMU
    Register Information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArm,
  EArmObjDmcPmuRegInfo,
  CM_ARM_DMC_PMU_REG_INFO
  );

/** Check the DMC PMU Information for a given socket.

  @param [in] DmcPmuRegInfo            Array of DMC information structure.
  @param [in] DevCount                 Count of DMC devices to validate.

  @retval  EFI_SUCCESS            The function completed successfully.
  @retval  EFI_INVALID_PARAMETER  Invalid parameter.
**/
STATIC
EFI_STATUS
EFIAPI
ValidateDmcPmuInfo (
  IN  CONST CM_ARM_DMC_PMU_REG_INFO  *CONST  DmcPmuRegInfo,
  IN        UINT32                           DevCount
  )
{
  UINT32                                  DevNum;
  CONST CM_ARM_DMC_PMU_REG_INFO           *RegInfo;
  CONST CM_ARCH_COMMON_GENERIC_INTERRUPT  *PmuIntr;

  if ((DmcPmuRegInfo == NULL) || (DevCount == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  for (DevNum = 0; DevNum < DevCount; DevNum++) {
    RegInfo = &DmcPmuRegInfo[DevNum];
    // Check Base address is initialized
    if (RegInfo->BaseAddress == 0) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-DMC: Invalid PMU Base Address.\n"
        ));
      goto error_handler;
    }

    if (RegInfo->Length != DMC_PERIPHBASE_MAX_ADDRESS_LENGTH) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-DMC: Invalid PMU Length.\n"
        ));
      goto error_handler;
    }

    // The PMU registers in the DMC start at an offset of
    // 0xA00. Check that that is so.
    if ((RegInfo->BaseAddress & DMC_REGISTER_SPACE_MASK) !=
        DMC_PMU_ADDRESS_OFFSET)
    {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-DMC: PMU Address offset must be 0xA00.\n"
        ));
      goto error_handler;
    }

    PmuIntr = &RegInfo->PmuIntr;
    if ((PmuIntr->Flags & BIT0) != 0) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-DMC: PMU Interrupt must be Level Triggered.\n"
        ));
      goto error_handler;
    }

    if ((PmuIntr->Flags & BIT1) != 0) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-DMC: PMU Interrupt must be Active High.\n"
        ));
      goto error_handler;
    }
  }

  return EFI_SUCCESS;

error_handler:

  ASSERT (FALSE);

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

  @param [in]  DmcPmuRegInfo   Pointer to the register info structure.
  @param [in]  DeviceNode         AML device node handle.

  @retval EFI_SUCCESS           The CRS node was created successfully.
  @retval Others                Failed to create CRS node.
**/
STATIC
EFI_STATUS
EFIAPI
CreateDmcPmuCrs (
  IN CONST CM_ARM_DMC_PMU_REG_INFO        *CONST  DmcPmuRegInfo,
  IN       AML_OBJECT_NODE_HANDLE                 DeviceNode
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
      "ERROR: SSDT-DMC-AML-CODEGEN: Failed to create AML _CRS Node."
      " Status = %r\n",
      Status
      ));
    return Status;
  }

  BaseAddress = DmcPmuRegInfo->BaseAddress;
  Length      = DmcPmuRegInfo->Length;
  Status      = AmlCodeGenRdQWordMemory (
                  FALSE,
                  TRUE,
                  TRUE,
                  TRUE,
                  AmlMemoryNonCacheable,
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

  Intr   = DmcPmuRegInfo->PmuIntr.Interrupt;
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

/** Build a SSDT table describing the DMC PMU register space.

  Add device nodes describing the DMC PMU register space, one
  socket at a time.

  @param [in]  Uid               For this socket, generate UID ids
                                 from this value onward.
  @param [in]  SockNum           Socket number on which devices are
                                 present.
  @param [in]  DevCount          Number of devices on this socket.
  @param [in]  ScopeNode         AML System Bus node handle.
  @param [in]  DmcPmuRegInfo  Array of DMC information structure.

  @retval EFI_SUCCESS            Device nodes added successfully.
  @retval Others                 Failed to create the device nodes.
**/
STATIC
EFI_STATUS
EFIAPI
BuildDmcPmuSocket (
  IN       UINT64                                 Uid,
  IN CONST UINT32                                 SockNum,
  IN CONST UINT32                                 DevCount,
  IN CONST AML_OBJECT_NODE_HANDLE                 ScopeNode,
  IN CONST CM_ARM_DMC_PMU_REG_INFO        *CONST  DmcPmuRegInfo
  )
{
  UINT32                  DevNum;
  CHAR8                   Name[AML_NAME_SEG_SIZE + 1];
  CHAR16                  Str[32];
  EFI_STATUS              Status;
  AML_OBJECT_NODE_HANDLE  DeviceNode;

  // Validate the DMC Info and get the number of devices.
  Status = ValidateDmcPmuInfo (DmcPmuRegInfo, DevCount);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-DMC: Invalid DMC PMU information. Status = %r\n",
      Status
      ));
    return Status;
  }

  Name[0] = 'M';
  Name[1] = 'C';
  Name[4] = '\0';

  for (DevNum = 0; DevNum < DevCount; DevNum++, Uid++) {
    Name[2] = AsciiFromHex ((Uid >> 4) & 0xF);
    Name[3] = AsciiFromHex (Uid & 0xF);

    Status = AmlCodeGenDevice (Name, ScopeNode, &DeviceNode);
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-DMC: Failed to create AML Device Node."
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
        "ERROR: SSDT-DMC-AML-CODEGEN: Failed to create AML _HID Node."
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
        "ERROR: SSDT-DMC-AML-CODEGEN: Failed to create AML _CID Node."
        " Status = %r\n",
        Status
        ));
      return Status;
    }

    Status = AmlCodeGenNameInteger ("_UID", Uid, DeviceNode, NULL);
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-DMC-AML-CODEGEN: Failed to create AML _UID Node."
        " Status = %r\n",
        Status
        ));
      return Status;
    }

    Status = AmlCodeGenNameInteger ("_CCA", 1, DeviceNode, NULL);
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-DMC-AML-CODEGEN: Failed to create AML _CCA Node."
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
        "ERROR: SSDT-DMC-AML-CODEGEN: Failed to create AML _STR Node."
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
        "ERROR: SSDT-DMC-AML-CODEGEN: Failed to create AML _STA Node."
        " Status = %r\n",
        Status
        ));
      return Status;
    }

    Status = CreateDmcPmuCrs (&DmcPmuRegInfo[DevNum], DeviceNode);
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-DMC-AML-CODEGEN: Failed to create AML _CRS Node."
        " Status = %r\n",
        Status
        ));
      return Status;
    }
  }

  return EFI_SUCCESS;
}

/** Construct SSDT tables for describing DMC PMU interface.

  This function invokes the Configuration Manager protocol interface
  to get the required hardware information for generating the ACPI
  table.

  If this function allocates any resources then they must be freed
  in the FreeXXXXTableRes function.

  @param [in]  This            Pointer to the ACPI table generator.
  @param [in]  AcpiTableInfo   Pointer to the ACPI table information.
  @param [in]  CfgMgrProtocol  Pointer to the Configuration Manager
                               Protocol interface.
  @param [out] Table           Pointer to the generated ACPI table.

  @retval EFI_SUCCESS            Table generated successfully.
  @retval EFI_INVALID_PARAMETER  A parameter is invalid.
  @retval EFI_UNSUPPORTED        Unsupported configuration.
**/
STATIC
EFI_STATUS
EFIAPI
BuildSsdtDmcPmuTable (
  IN  CONST ACPI_TABLE_GENERATOR                           *This,
  IN  CONST CM_STD_OBJ_ACPI_TABLE_INFO             *CONST  AcpiTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL   *CONST  CfgMgrProtocol,
  OUT       EFI_ACPI_DESCRIPTION_HEADER                    **Table
  )
{
  AML_ROOT_NODE_HANDLE     RootNode;
  AML_OBJECT_NODE_HANDLE   ScopeNode;
  EFI_STATUS               Status;
  EFI_STATUS               Status1;
  UINT64                   Uid;
  UINT32                   DevCount;
  UINT32                   SockNum;
  UINT32                   SocketCount;
  CM_ARM_DMC_PMU_REG_INFO  *DmcPmuRegInfo;
  CM_ARM_DMC_INFO          *DmcPmuSockInfo;

  ASSERT (This != NULL);
  ASSERT (AcpiTableInfo != NULL);
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (Table != NULL);
  ASSERT (AcpiTableInfo->TableGeneratorId == This->GeneratorID);
  ASSERT (AcpiTableInfo->AcpiTableSignature == This->AcpiTableSignature);

  *Table = NULL;

  Status = GetEArmObjDmcPmuSocketInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &DmcPmuSockInfo,
             &SocketCount
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-DMC: Failed to get the DMC Socket information."
      " Status = %r\n",
      Status
      ));
    return Status;
  }

  if (SocketCount == 0) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-DMC: Invalid DMC Socket information.\n"
      ));
    return EFI_INVALID_PARAMETER;
  }

  Status = AmlCodeGenDefinitionBlock (
             "SSDT",
             "ARMLTD",
             "DMC-",
             0x01,
             &RootNode
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-DMC: Failed to create AML Definition Block."
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
      "ERROR: SSDT-DMC: Failed to create AML Scope Node."
      " Status = %r\n",
      Status
      ));
    goto error_handler;
  }

  Uid = 0;
  for (SockNum = 0; SockNum < SocketCount; SockNum++) {
    DevCount = 0;
    Status   = GetEArmObjDmcPmuRegInfo (
                 CfgMgrProtocol,
                 DmcPmuSockInfo[SockNum].DmcPmuRegInfoToken,
                 &DmcPmuRegInfo,
                 &DevCount
                 );

    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-DMC: Failed to get the DMC per socket device information.\n"
        " Status = %r\n",
        Status
        ));
      ASSERT_EFI_ERROR (Status);
      goto error_handler;
    }

    if ((DevCount == 0) || (DmcPmuSockInfo[SockNum].NumDevices != DevCount)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-DMC: Invalid DMC device information.\n"
        ));
      ASSERT_EFI_ERROR (Status);
      goto error_handler;
    }

    Status = BuildDmcPmuSocket (
               Uid,
               SockNum,
               DevCount,
               ScopeNode,
               DmcPmuRegInfo
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-DMC: Failed to build table for DMC."
        " Status = %r\n",
        Status
        ));
      goto error_handler;
    }

    Uid += DevCount;
  }

  // Serialize the tree.
  Status = AmlSerializeDefinitionBlock (
             RootNode,
             Table
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-DMC: Failed to Serialize SSDT Table Data."
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
        "ERROR: SSDT-DMC-AML-CODEGEN: Failed to cleanup AML tree."
        " Status = %r\n",
        Status1
        ));
    }
  }

  return Status;
}

/** Free any resources allocated for constructing the SSDT tables for DMC PMU.

  @param [in]      This           Pointer to the ACPI table generator.
  @param [in]      AcpiTableInfo  Pointer to the ACPI Table Info.
  @param [in]      CfgMgrProtocol Pointer to the Configuration Manager
                                  Protocol Interface.
  @param [in, out] Table          Pointer to ACPI Table.

  @retval EFI_SUCCESS           The resources were freed successfully.
  @retval EFI_INVALID_PARAMETER The table pointer is NULL or invalid.
**/
STATIC
EFI_STATUS
EFIAPI
FreeSsdtDmcPmuTableRes (
  IN      CONST ACPI_TABLE_GENERATOR                   *CONST  This,
  IN      CONST CM_STD_OBJ_ACPI_TABLE_INFO             *CONST  AcpiTableInfo,
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL   *CONST  CfgMgrProtocol,
  IN OUT        EFI_ACPI_DESCRIPTION_HEADER           **CONST  Table
  )
{
  ASSERT (This != NULL);
  ASSERT (AcpiTableInfo != NULL);
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (AcpiTableInfo->TableGeneratorId == This->GeneratorID);
  ASSERT (AcpiTableInfo->AcpiTableSignature == This->AcpiTableSignature);

  if ((Table == NULL) || (*Table == NULL)) {
    DEBUG ((DEBUG_ERROR, "ERROR: SSDT-DMC: Invalid Table Pointer\n"));
    ASSERT ((Table != NULL) && (*Table != NULL));
    return EFI_INVALID_PARAMETER;
  }

  FreePool (*Table);
  *Table = NULL;

  return EFI_SUCCESS;
}

/** This macro defines the Raw Generator revision.
*/
#define SSDT_DMC_PMU_GENERATOR_REVISION  CREATE_REVISION (1, 0)

/** The interface for the Raw Table Generator.
*/
STATIC
CONST
ACPI_TABLE_GENERATOR  SsdtDmcPmuGenerator = {
  // Generator ID
  CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdSsdtDmcPmu),
  // Generator Description
  L"ACPI.STD.SSDT.DMC.PMU.GENERATOR",
  // ACPI Table Signature
  EFI_ACPI_6_6_SECONDARY_SYSTEM_DESCRIPTION_TABLE_SIGNATURE,
  // ACPI Table Revision - Unused
  0,
  // Minimum ACPI Table Revision - Unused
  0,
  // Creator ID
  TABLE_GENERATOR_CREATOR_ID_ARM,
  // Creator Revision
  SSDT_DMC_PMU_GENERATOR_REVISION,
  // Build table function.
  BuildSsdtDmcPmuTable,
  // Free table function.
  FreeSsdtDmcPmuTableRes,
  // Build Table function. Extended version not needed.
  NULL,
  // Free Resource function. Extended version not needed.
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
AcpiSsdtDmcPmuConstructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = RegisterAcpiTableGenerator (&SsdtDmcPmuGenerator);
  DEBUG ((
    DEBUG_INFO,
    "SSDT-DMC: Register Generator. Status = %r\n",
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
AcpiSsdtDmcPmuDestructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = DeregisterAcpiTableGenerator (&SsdtDmcPmuGenerator);
  DEBUG ((
    DEBUG_INFO,
    "SSDT-DMC: Deregister Generator. Status = %r\n",
    Status
    ));
  ASSERT_EFI_ERROR (Status);
  return Status;
}
