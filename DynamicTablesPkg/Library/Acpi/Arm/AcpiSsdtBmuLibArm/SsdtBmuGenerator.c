/** @file
  SSDT BMU AML Table Generator.

  Copyright (c) 2026, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - ACPI for the Arm Components 1.3 Platform Design Document
    (https://support.arm.com/documentation/den0093/1-3alp0)
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
#include "SsdtBmuGenerator.h"

/** SSDT BMU Table Generator.

  Requirements:
  The following Configuration Manager Object(s) are required by
  this Generator:
  - EArmObjBmuSocketInfo
  - EArmObjBmuRegInfo
*/

/** This macro expands to a function that retrieves the BMU
    Socket Information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArm,
  EArmObjBmuSocketInfo,
  CM_ARM_BMU_INFO
  );

/** This macro expands to a function that retrieves the BMU
    Register Information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArm,
  EArmObjBmuRegInfo,
  CM_ARM_BMU_REG_INFO
  );

/** Check the BMU Information for a given socket.

  @param [in] BmuRegInfo         Array of BMU information structure.
  @param [in] DevCount                 Count of DM devices to validate.

  @retval  EFI_SUCCESS            The function completed successfully.
  @retval  EFI_INVALID_PARAMETER  Invalid parameter.
**/
STATIC
EFI_STATUS
EFIAPI
ValidateBmuInfo (
  IN  CONST CM_ARM_BMU_REG_INFO  *CONST  BmuRegInfo,
  IN        UINT32                       DevCount
  )
{
  UINT32                                  DevNum;
  CONST CM_ARM_BMU_REG_INFO               *RegInfo;
  CONST CM_ARCH_COMMON_GENERIC_INTERRUPT  *BmuIntr;

  if ((BmuRegInfo == NULL) || (DevCount == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  for (DevNum = 0; DevNum < DevCount; DevNum++) {
    RegInfo = &BmuRegInfo[DevNum];
    // Check Base address is initialized
    if (RegInfo->BaseAddress == 0) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-BMU: Invalid BMU Base Address.\n"
        ));
      goto error_handler;
    }

    if ((RegInfo->Length != BMU_PERIPHBASE_MAX_ADDRESS_LENGTH_1) &&
        (RegInfo->Length != BMU_PERIPHBASE_MAX_ADDRESS_LENGTH_2))
    {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-BMU: Invalid BMU Length.\n"
        ));
      goto error_handler;
    }

    // The BMU registers start at an offset of 0x80000 and 0xC0000.
    // Check that that is so.
    if (((RegInfo->BaseAddress & BMU_REGISTER_SPACE_MASK) !=
         BMU_ADDRESS_OFFSET_1) &&
        ((RegInfo->BaseAddress & BMU_REGISTER_SPACE_MASK) !=
         BMU_ADDRESS_OFFSET_2))
    {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-BMU: BMU Address offset must be 0x80000 or 0xC0000.\n"
        ));
      goto error_handler;
    }

    BmuIntr = &RegInfo->BmuIntr;
    if ((BmuIntr->Flags & BIT0) != 0) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-BMU: BMU Interrupt must be Level Triggered.\n"
        ));
      goto error_handler;
    }

    if ((BmuIntr->Flags & BIT1) != 0) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-BMU: BMU Interrupt must be Active High.\n"
        ));
      goto error_handler;
    }
  }

  return EFI_SUCCESS;

error_handler:

  ASSERT (FALSE);

  DEBUG ((
    DEBUG_ERROR,
    "BmuBaseAddress = 0x%llx\n"
    "BmuBaseAddressLength = 0x%llx\n"
    "BmuInterrupt = 0x%lx\n"
    "BmuInterruptFlags = 0x%lx\n",
    RegInfo->BaseAddress,
    RegInfo->Length,
    RegInfo->BmuIntr.Interrupt,
    RegInfo->BmuIntr.Flags
    ));

  return EFI_INVALID_PARAMETER;
}

/**
  Create the _CRS (Current Resource Settings) AML node for the device.

  @param [in]  BmuRegInfo   Pointer to the register info structure.
  @param [in]  DeviceNode         AML device node handle.

  @retval EFI_SUCCESS           The CRS node was created successfully.
  @retval Others                Failed to create CRS node.
**/
STATIC
EFI_STATUS
EFIAPI
CreateBmuCrs (
  IN CONST CM_ARM_BMU_REG_INFO                    *CONST  BmuRegInfo,
  IN       AML_OBJECT_NODE_HANDLE                         DeviceNode
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
      "ERROR: SSDT-BMU-AML-CODEGEN: Failed to create AML _CRS Node."
      " Status = %r\n",
      Status
      ));
    return Status;
  }

  BaseAddress = BmuRegInfo->BaseAddress;
  Length      = BmuRegInfo->Length;
  Status      = AmlCodeGenRdQWordMemory (
                  TRUE,
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

  Intr   = BmuRegInfo->BmuIntr.Interrupt;
  Status = AmlCodeGenRdInterrupt (
             TRUE,
             FALSE,
             FALSE,
             BmuRegInfo->Shared,
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

/** Build a SSDT table describing the BMU register space.

  Add device nodes describing the BMU register space, one
  socket at a time.

  @param [in]  Uid               For this socket, generate UID ids
                                 from this value onward.
  @param [in]  SockNum           Socket number on which devices are
                                 present.
  @param [in]  DevCount          Number of devices on this socket.
  @param [in]  ScopeNode         AML System Bus node handle.
  @param [in]  BmuRegInfo        Array of BMU information structure.

  @retval EFI_SUCCESS            Device nodes added successfully.
  @retval Others                 Failed to create the device nodes.
**/
STATIC
EFI_STATUS
EFIAPI
BuildBmuSocket (
  IN       UINT64                                         Uid,
  IN CONST UINT32                                         SockNum,
  IN CONST UINT32                                         DevCount,
  IN CONST AML_OBJECT_NODE_HANDLE                         ScopeNode,
  IN CONST CM_ARM_BMU_REG_INFO                    *CONST  BmuRegInfo
  )
{
  UINT32                  DevNum;
  CHAR8                   Name[AML_NAME_SEG_SIZE + 1];
  CHAR16                  Str[32];
  EFI_STATUS              Status;
  AML_OBJECT_NODE_HANDLE  DeviceNode;

  // Validate the BMU Info and get the number of devices.
  Status = ValidateBmuInfo (BmuRegInfo, DevCount);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-BMU: Invalid BMU information. Status = %r\n",
      Status
      ));
    return Status;
  }

  Name[0] = 'B';
  Name[1] = 'C';
  Name[4] = '\0';

  for (DevNum = 0; DevNum < DevCount; DevNum++, Uid++) {
    Name[2] = AsciiFromHex ((Uid >> 4) & 0xF);
    Name[3] = AsciiFromHex (Uid & 0xF);

    Status = AmlCodeGenDevice (Name, ScopeNode, &DeviceNode);
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-BMU: Failed to create AML Device Node."
        " Status = %r\n",
        Status
        ));
      return Status;
    }

    Status = AmlCodeGenNameString (
               "_HID",
               "ARMHB001",
               DeviceNode,
               NULL
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-BMU-AML-CODEGEN: Failed to create AML _HID Node."
        " Status = %r\n",
        Status
        ));
      return Status;
    }

    Status = AmlCodeGenNameInteger ("_UID", Uid, DeviceNode, NULL);
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-BMU-AML-CODEGEN: Failed to create AML _UID Node."
        " Status = %r\n",
        Status
        ));
      return Status;
    }

    UnicodeSPrint (Str, sizeof (Str), L"Socket %u: BMU%u", SockNum, DevNum);
    Status = AmlCodeGenNameUnicodeString (
               "_STR",
               Str,
               DeviceNode,
               NULL
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-BMU-AML-CODEGEN: Failed to create AML _STR Node."
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
        "ERROR: SSDT-BMU-AML-CODEGEN: Failed to create AML _STA Node."
        " Status = %r\n",
        Status
        ));
      return Status;
    }

    Status = CreateBmuCrs (&BmuRegInfo[DevNum], DeviceNode);
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-BMU-AML-CODEGEN: Failed to create AML _CRS Node."
        " Status = %r\n",
        Status
        ));
      return Status;
    }
  }

  return EFI_SUCCESS;
}

/** Construct SSDT tables for describing BMU interface.

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
BuildSsdtBmuTable (
  IN  CONST ACPI_TABLE_GENERATOR                           *This,
  IN  CONST CM_STD_OBJ_ACPI_TABLE_INFO             *CONST  AcpiTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL   *CONST  CfgMgrProtocol,
  OUT       EFI_ACPI_DESCRIPTION_HEADER                    **Table
  )
{
  AML_ROOT_NODE_HANDLE    RootNode;
  AML_OBJECT_NODE_HANDLE  ScopeNode;
  EFI_STATUS              Status;
  EFI_STATUS              Status1;
  UINT64                  Uid;
  UINT32                  DevCount;
  UINT32                  SockNum;
  UINT32                  SocketCount;
  CM_ARM_BMU_REG_INFO     *BmuRegInfo;
  CM_ARM_BMU_INFO         *BmuSockInfo;

  ASSERT (This != NULL);
  ASSERT (AcpiTableInfo != NULL);
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (Table != NULL);
  ASSERT (AcpiTableInfo->TableGeneratorId == This->GeneratorID);
  ASSERT (AcpiTableInfo->AcpiTableSignature == This->AcpiTableSignature);

  *Table = NULL;

  Status = GetEArmObjBmuSocketInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &BmuSockInfo,
             &SocketCount
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-BMU: Failed to get the BMU Socket information."
      " Status = %r\n",
      Status
      ));
    return Status;
  }

  if (SocketCount == 0) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-BMU: Invalid BMU Socket information.\n"
      ));
    return EFI_INVALID_PARAMETER;
  }

  Status = AmlCodeGenDefinitionBlock (
             "SSDT",
             "ARMLTD",
             "BMU",
             0x01,
             &RootNode
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-BMU: Failed to create AML Definition Block."
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
      "ERROR: SSDT-BMU: Failed to create AML Scope Node."
      " Status = %r\n",
      Status
      ));
    goto error_handler;
  }

  Uid = 0;
  for (SockNum = 0; SockNum < SocketCount; SockNum++) {
    DevCount = 0;
    Status   = GetEArmObjBmuRegInfo (
                 CfgMgrProtocol,
                 BmuSockInfo[SockNum].BmuRegInfoToken,
                 &BmuRegInfo,
                 &DevCount
                 );

    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-BMU: Failed to get the BMU per socket device information.\n"
        " Status = %r\n",
        Status
        ));
      ASSERT_EFI_ERROR (Status);
      goto error_handler;
    }

    if ((DevCount == 0) || (BmuSockInfo[SockNum].NumDevices != DevCount)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-BMU: Invalid BMU device information.\n"
        ));
      ASSERT_EFI_ERROR (Status);
      Status = EFI_INVALID_PARAMETER;
      goto error_handler;
    }

    Status = BuildBmuSocket (
               Uid,
               SockNum,
               DevCount,
               ScopeNode,
               BmuRegInfo
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-BMU: Failed to build table for BMU."
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
      "ERROR: SSDT-BMU: Failed to Serialize SSDT Table Data."
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
        "ERROR: SSDT-BMU-AML-CODEGEN: Failed to cleanup AML tree."
        " Status = %r\n",
        Status1
        ));
    }
  }

  return Status;
}

/** Free any resources allocated for constructing the SSDT tables for BMU.

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
FreeSsdtBmuTableRes (
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
    DEBUG ((DEBUG_ERROR, "ERROR: SSDT-BMU: Invalid Table Pointer\n"));
    ASSERT ((Table != NULL) && (*Table != NULL));
    return EFI_INVALID_PARAMETER;
  }

  FreePool (*Table);
  *Table = NULL;

  return EFI_SUCCESS;
}

/** This macro defines the Raw Generator revision.
*/
#define SSDT_BMU_GENERATOR_REVISION  CREATE_REVISION (1, 0)

/** The interface for the Raw Table Generator.
*/
STATIC
CONST
ACPI_TABLE_GENERATOR  SsdtBmuGenerator = {
  // Generator ID
  CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdSsdtBmu),
  // Generator Description
  L"ACPI.STD.SSDT.BMU.GENERATOR",
  // ACPI Table Signature
  EFI_ACPI_6_6_SECONDARY_SYSTEM_DESCRIPTION_TABLE_SIGNATURE,
  // ACPI Table Revision - Unused
  0,
  // Minimum ACPI Table Revision - Unused
  0,
  // Creator ID
  TABLE_GENERATOR_CREATOR_ID_ARM,
  // Creator Revision
  SSDT_BMU_GENERATOR_REVISION,
  // Build table function.
  BuildSsdtBmuTable,
  // Free table function.
  FreeSsdtBmuTableRes,
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
AcpiSsdtBmuConstructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = RegisterAcpiTableGenerator (&SsdtBmuGenerator);
  DEBUG ((
    DEBUG_INFO,
    "SSDT-BMU: Register Generator. Status = %r\n",
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
AcpiSsdtBmuDestructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = DeregisterAcpiTableGenerator (&SsdtBmuGenerator);
  DEBUG ((
    DEBUG_INFO,
    "SSDT-BMU: Deregister Generator. Status = %r\n",
    Status
    ));
  ASSERT_EFI_ERROR (Status);
  return Status;
}
