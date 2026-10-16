/** @file
  SSDT SMC91X network device AML Table Generator.

  Copyright (c) 2026, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>

#include <AcpiTableGenerator.h>
#include <ConfigurationManagerHelper.h>
#include <ConfigurationManagerObject.h>
#include <Library/AcpiHelperLib.h>
#include <Library/AmlLib/AmlLib.h>
#include <Library/TableHelperLib.h>
#include <Protocol/ConfigurationManagerProtocol.h>

#define SMC91X_HID  "LNRO0003"
#define SB_SCOPE    "\\_SB_"

GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjSmc91xInfo,
  CM_ARCH_COMMON_SMC91X_INFO
  );

/** Validate the SMC91X device register ranges and interrupts.

  @param [in]       Smc91xDevices             Pointer to the SMC91X device information array.
  @param [in]       Smc91xCount               Number of device information entries.

  @retval EFI_SUCCESS             All device information entries are valid.
  @retval EFI_INVALID_PARAMETER   A device pointer, register range or interrupt is invalid.
**/
STATIC
EFI_STATUS
ValidateSmc91xInfo (
  IN CONST CM_ARCH_COMMON_SMC91X_INFO  *Smc91xDevices,
  IN       UINT32                      Smc91xCount
  )
{
  CONST CM_ARCH_COMMON_SMC91X_INFO  *Smc91xInfo;
  UINT32                            Index;

  if ((Smc91xDevices == NULL) || (Smc91xCount == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  for (Index = 0; Index < Smc91xCount; Index++) {
    Smc91xInfo = &Smc91xDevices[Index];

    if ((Smc91xInfo->BaseAddress == 0) ||
        (Smc91xInfo->BaseAddressLength == 0) ||
        (Smc91xInfo->BaseAddress >
         MAX_UINT64 - (Smc91xInfo->BaseAddressLength - 1)))
    {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-SMC91X: Invalid MMIO range at index %u.\n",
        Index
        ));
      return EFI_INVALID_PARAMETER;
    }

    if ((Smc91xInfo->Interrupt.Interrupt == 0) ||
        ((Smc91xInfo->Interrupt.Flags & ~(BIT0 | BIT1)) != 0))
    {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-SMC91X: Invalid interrupt at index %u.\n",
        Index
        ));
      return EFI_INVALID_PARAMETER;
    }
  }

  return EFI_SUCCESS;
}

/** Create the _CRS resource template for a SMC91X device.

  @param [in]       Smc91xInfo                Pointer to the SMC91X device information.
  @param [in]       DeviceNode                AML device node to receive the resource template.

  @retval EFI_SUCCESS             The memory and interrupt resources were created.
  @return Other errors from the Configuration Manager or AML library.
**/
STATIC
EFI_STATUS
CreateSmc91xCrs (
  IN CONST CM_ARCH_COMMON_SMC91X_INFO  *Smc91xInfo,
  IN       AML_OBJECT_NODE_HANDLE      DeviceNode
  )
{
  AML_OBJECT_NODE_HANDLE  CrsNode;
  EFI_STATUS              Status;
  UINT32                  Interrupt;
  UINT64                  AddressMaximum;

  // ASL: Name (_CRS, ResourceTemplate () {})
  Status = AmlCodeGenNameResourceTemplate ("_CRS", DeviceNode, &CrsNode);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-SMC91X: Failed to create AML _CRS node."
      " Status = %r\n",
      Status
      ));
    return Status;
  }

  AddressMaximum = Smc91xInfo->BaseAddress + Smc91xInfo->BaseAddressLength - 1;

  if ((AddressMaximum <= MAX_UINT32) &&
      (Smc91xInfo->BaseAddressLength <= MAX_UINT32))
  {
    // ASL: Memory32Fixed (ReadWrite, <BaseAddress>, <BaseAddressLength>)
    Status = AmlCodeGenRdMemory32Fixed (
               TRUE,
               (UINT32)Smc91xInfo->BaseAddress,
               (UINT32)Smc91xInfo->BaseAddressLength,
               CrsNode,
               NULL
               );
  } else {
    // ASL: QWordMemory (ResourceConsumer, ..., ReadWrite, <Range>)
    Status = AmlCodeGenRdQWordMemory (
               TRUE,
               TRUE,
               TRUE,
               TRUE,
               AmlMemoryNonCacheable,
               TRUE,
               0,
               Smc91xInfo->BaseAddress,
               AddressMaximum,
               0,
               Smc91xInfo->BaseAddressLength,
               0,
               NULL,
               AmlAddressRangeMemory,
               TRUE,
               CrsNode,
               NULL
               );
  }

  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-SMC91X: Failed to create MMIO resource."
      " Status = %r\n",
      Status
      ));
    return Status;
  }

  Interrupt = Smc91xInfo->Interrupt.Interrupt;

  // ASL: Interrupt (ResourceConsumer, <Mode>, <Polarity>, Exclusive) {}
  Status = AmlCodeGenRdInterrupt (
             TRUE,
             ((Smc91xInfo->Interrupt.Flags & BIT0) != 0),
             ((Smc91xInfo->Interrupt.Flags & BIT1) != 0),
             FALSE,
             &Interrupt,
             1,
             CrsNode,
             NULL
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-SMC91X: Failed to create interrupt resource."
      " Status = %r\n",
      Status
      ));
  }

  return Status;
}

/** Build an SSDT describing a SMC91X device.

  The caller owns the generated table and must release it with the
  registered free callback.

  @param [in]       This                      Pointer to the ACPI table generator.
  @param [in]       AcpiTableInfo             Pointer to the ACPI table information.
  @param [in]       CfgMgrProtocol            Pointer to the Configuration Manager protocol.
  @param [out]      Table                     Pointer to the generated table.

  @retval EFI_SUCCESS             The requested SSDT was generated.
  @retval EFI_INVALID_PARAMETER   A parameter or device configuration is invalid.
  @retval EFI_OUT_OF_RESOURCES    Memory allocation failed.
  @return Other errors from the Configuration Manager or AML library.
**/
STATIC
EFI_STATUS
EFIAPI
BuildSsdtSmc91xTable (
  IN  CONST ACPI_TABLE_GENERATOR                          *This,
  IN  CONST CM_STD_OBJ_ACPI_TABLE_INFO            *CONST  AcpiTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  OUT       EFI_ACPI_DESCRIPTION_HEADER                   **Table
  )
{
  CM_ARCH_COMMON_SMC91X_INFO  *Smc91xInfo;
  UINT32                      Smc91xCount;
  EFI_STATUS                  Status;
  EFI_STATUS                  CleanupStatus;
  AML_OBJECT_NODE_HANDLE      DeviceNode;
  AML_OBJECT_NODE_HANDLE      ScopeNode;
  AML_ROOT_NODE_HANDLE        RootNode;

  if ((This == NULL) || (AcpiTableInfo == NULL) || (CfgMgrProtocol == NULL) ||
      (Table == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  if ((AcpiTableInfo->TableGeneratorId != This->GeneratorID) ||
      (AcpiTableInfo->AcpiTableSignature != This->AcpiTableSignature))
  {
    return EFI_INVALID_PARAMETER;
  }

  *Table = NULL;

  Status = GetEArchCommonObjSmc91xInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &Smc91xInfo,
             &Smc91xCount
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-SMC91X: Failed to get SMC91X network device information."
      " Status = %r\n",
      Status
      ));
    return Status;
  }

  if (Smc91xCount != 1) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-SMC91X: Invalid SMC91X device count: %u."
      " Maximum supported = %u.\n",
      Smc91xCount,
      1
      ));
    return EFI_INVALID_PARAMETER;
  }

  Status = ValidateSmc91xInfo (Smc91xInfo, Smc91xCount);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-SMC91X: Invalid SMC91X network device information."
      " Status = %r\n",
      Status
      ));
    return Status;
  }

  RootNode   = NULL;
  ScopeNode  = NULL;
  DeviceNode = NULL;
  *Table     = NULL;

  Status = AddSsdtAcpiHeader (
             CfgMgrProtocol,
             This,
             AcpiTableInfo,
             &RootNode
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // ASL: Scope (\_SB) {}
  Status = AmlCodeGenScope (SB_SCOPE, RootNode, &ScopeNode);
  if (EFI_ERROR (Status)) {
    goto Cleanup;
  }

  // ASL: Device (NET0) {}
  Status = AmlCodeGenDevice ("NET0", ScopeNode, &DeviceNode);
  if (EFI_ERROR (Status)) {
    goto Cleanup;
  }

  // ASL: Name (_HID, "LNRO0003")
  Status = AmlCodeGenNameString (
             "_HID",
             SMC91X_HID,
             DeviceNode,
             NULL
             );
  if (EFI_ERROR (Status)) {
    goto Cleanup;
  }

  // ASL: Name (_UID, 0)
  Status = AmlCodeGenNameInteger (
             "_UID",
             0,
             DeviceNode,
             NULL
             );
  if (EFI_ERROR (Status)) {
    goto Cleanup;
  }

  Status = CreateSmc91xCrs (Smc91xInfo, DeviceNode);
  if (EFI_ERROR (Status)) {
    goto Cleanup;
  }

  Status = AmlSerializeDefinitionBlock (RootNode, Table);

Cleanup:
  CleanupStatus = AmlDeleteTree (RootNode);
  if (EFI_ERROR (CleanupStatus)) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-SMC91X: Failed to delete AML tree."
      " Status = %r\n",
      CleanupStatus
      ));
    if (!EFI_ERROR (Status)) {
      Status = CleanupStatus;
    }
  }

  if (EFI_ERROR (Status) && (*Table != NULL)) {
    FreePool (*Table);
    *Table = NULL;
  }

  return Status;
}

/** Free resources for the generated SMC91X SSDT.

  An empty output after build failure requires no cleanup and succeeds.

  @param [in]       This                      Pointer to the ACPI table generator.
  @param [in]       AcpiTableInfo             Pointer to the ACPI table information.
  @param [in]       CfgMgrProtocol            Pointer to the Configuration Manager protocol.
  @param [in, out]  Table                     Pointer to the generated table.

  @retval EFI_SUCCESS             The resources were freed, or the output was empty.
  @retval EFI_INVALID_PARAMETER   An argument or table signature is invalid.
**/
STATIC
EFI_STATUS
EFIAPI
FreeSsdtSmc91xTable (
  IN      CONST ACPI_TABLE_GENERATOR                  *CONST   This,
  IN      CONST CM_STD_OBJ_ACPI_TABLE_INFO            *CONST   AcpiTableInfo,
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST   CfgMgrProtocol,
  IN OUT        EFI_ACPI_DESCRIPTION_HEADER           **CONST  Table
  )
{
  if ((This == NULL) || (AcpiTableInfo == NULL) || (CfgMgrProtocol == NULL) ||
      (Table == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  if ((AcpiTableInfo->TableGeneratorId != This->GeneratorID) ||
      (AcpiTableInfo->AcpiTableSignature != This->AcpiTableSignature))
  {
    return EFI_INVALID_PARAMETER;
  }

  // The framework also invokes this callback after build failure.
  if (*Table == NULL) {
    return EFI_SUCCESS;
  }

  if ((*Table)->Signature != EFI_ACPI_6_3_SECONDARY_SYSTEM_DESCRIPTION_TABLE_SIGNATURE) {
    return EFI_INVALID_PARAMETER;
  }

  FreePool (*Table);
  *Table = NULL;
  return EFI_SUCCESS;
}

#define SSDT_SMC91X_GENERATOR_REVISION  CREATE_REVISION (1, 0)

/** ACPI table generator for the SMC91X network device SSDT.
*/
STATIC
CONST
ACPI_TABLE_GENERATOR  SsdtSmc91xGenerator = {
  CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdSsdtSmc91x),
  L"ACPI.STD.SSDT.SMC91X.GENERATOR",
  EFI_ACPI_6_3_SECONDARY_SYSTEM_DESCRIPTION_TABLE_SIGNATURE,
  0,
  0,
  TABLE_GENERATOR_CREATOR_ID,
  SSDT_SMC91X_GENERATOR_REVISION,
  BuildSsdtSmc91xTable,
  FreeSsdtSmc91xTable,
  NULL,
  NULL
};

/** Register the SMC91X SSDT generator with the ACPI table factory.

  @param [in]       ImageHandle               Handle of the image containing this library.
  @param [in]       SystemTable               Pointer to the UEFI system table.

  @retval EFI_SUCCESS             The generator was registered.
  @retval EFI_INVALID_PARAMETER   The generator identifier is invalid.
  @retval EFI_ALREADY_STARTED     The generator is already registered.
**/
EFI_STATUS
EFIAPI
AcpiSsdtSmc91xLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = RegisterAcpiTableGenerator (&SsdtSmc91xGenerator);
  DEBUG ((DEBUG_INFO, "SSDT-SMC91X: Register Generator. Status = %r\n", Status));
  ASSERT_EFI_ERROR (Status);
  return Status;
}

/** Deregister the SMC91X SSDT generator from the ACPI table factory.

  @param [in]       ImageHandle               Handle of the image containing this library.
  @param [in]       SystemTable               Pointer to the UEFI system table.

  @retval EFI_SUCCESS             The generator was deregistered.
  @retval EFI_INVALID_PARAMETER   The generator identifier is invalid.
  @retval EFI_NOT_FOUND           The generator is not registered.
**/
EFI_STATUS
EFIAPI
AcpiSsdtSmc91xLibDestructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = DeregisterAcpiTableGenerator (&SsdtSmc91xGenerator);
  DEBUG ((DEBUG_INFO, "SSDT-SMC91X: Deregister Generator. Status = %r\n", Status));
  ASSERT_EFI_ERROR (Status);
  return Status;
}
