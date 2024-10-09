/** @file
  SSDT Pcie Table Generator.

  Copyright (c) 2021 - 2022, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - PCI Firmware Specification - Revision 3.0
  - ACPI 6.4 specification:
   - s6.2.13 "_PRT (PCI Routing Table)"
   - s6.1.1 "_ADR (Address)"
  - linux kernel code
  - Arm Base Boot Requirements v1.0
  - Arm Base System Architecture v1.0
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
#include <Library/SsdtPcieSupportLib.h>
#include <Protocol/ConfigurationManagerProtocol.h>

#include "SsdtPcieGenerator.h"

#define PCI_MAX_DEVICE_COUNT_PER_BUS       32
#define PCI_MAX_FUNCTION_COUNT_PER_DEVICE  8

/** ARM standard SSDT Pcie Table Generator.

Requirements:
  The following Configuration Manager Object(s) are required by
  this Generator:
  - EArchCommonObjCmRef
  - EArchCommonObjPciConfigSpaceInfo
  - EArchCommonObjPciAddressMapInfo
  - EArchCommonObjPciInterruptMapInfo
*/

/** This macro expands to a function that retrieves the cross-CM-object-
    reference information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjCmRef,
  CM_ARCH_COMMON_OBJ_REF
  );

/** This macro expands to a function that retrieves the Pci
    Configuration Space Information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjPciConfigSpaceInfo,
  CM_ARCH_COMMON_PCI_CONFIG_SPACE_INFO
  );

/** This macro expands to a function that retrieves the Pci
    Address Mapping Information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjPciAddressMapInfo,
  CM_ARCH_COMMON_PCI_ADDRESS_MAP_INFO
  );

/** This macro expands to a function that retrieves the Pci
    Interrupt Mapping Information from the Configuration Manager.
*/
GET_OBJECT_LIST (
  EObjNameSpaceArchCommon,
  EArchCommonObjPciInterruptMapInfo,
  CM_ARCH_COMMON_PCI_INTERRUPT_MAP_INFO
  );

/** Initialize the MappingTable.

  @param [in] MappingTable  The mapping table structure.
  @param [in] Count         Number of entries to allocate in the
                            MappingTable.

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES   Failed to allocate memory.
**/
STATIC
EFI_STATUS
EFIAPI
MappingTableInitialize (
  IN  MAPPING_TABLE  *MappingTable,
  IN  UINT32         Count
  )
{
  UINT32  *Table;

  if ((MappingTable == NULL)  ||
      (Count == 0))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  Table = AllocateZeroPool (sizeof (*Table) * Count);
  if (Table == NULL) {
    ASSERT (0);
    return EFI_OUT_OF_RESOURCES;
  }

  MappingTable->Table     = Table;
  MappingTable->LastIndex = 0;
  MappingTable->MaxIndex  = Count;

  return EFI_SUCCESS;
}

/** Free the MappingTable.

  @param [in, out]  MappingTable  The mapping table structure.
**/
STATIC
VOID
EFIAPI
MappingTableFree (
  IN  OUT MAPPING_TABLE  *MappingTable
  )
{
  ASSERT (MappingTable != NULL);
  ASSERT (MappingTable->Table != NULL);

  if (MappingTable->Table != NULL) {
    FreePool (MappingTable->Table);
  }
}

/** Add a new entry to the MappingTable and return its index.

  If an entry with [Integer] is already available in the table,
  return its index without adding a new entry.

  @param [in] MappingTable  The mapping table structure.
  @param [in] Integer       New Integer entry to add.

  @retval The index of the Integer entry in the MappingTable.
**/
STATIC
UINT32
EFIAPI
MappingTableAdd (
  IN  MAPPING_TABLE  *MappingTable,
  IN  UINT32         Integer
  )
{
  UINT32  *Table;
  UINT32  Index;
  UINT32  LastIndex;

  ASSERT (MappingTable != NULL);
  ASSERT (MappingTable->Table != NULL);

  Table     = MappingTable->Table;
  LastIndex = MappingTable->LastIndex;

  // Search if there is already an entry with this Integer.
  for (Index = 0; Index < LastIndex; Index++) {
    if (Table[Index] == Integer) {
      return Index;
    }
  }

  ASSERT (LastIndex < MappingTable->MaxIndex);

  // If no, create a new entry.
  Table[LastIndex] = Integer;

  return MappingTable->LastIndex++;
}

/** Generate required Pci device information.

  ASL code:
    Name (_UID, <Uid>)                // Uid of the Pci device
    Name (_HID, EISAID ("PNP0A08"))   // PCI Express Root Bridge
    Name (_CID, EISAID ("PNP0A03"))   // Compatible PCI Root Bridge
    Name (_SEG, <Pci segment group>)  // PCI Segment Group number
    Name (_BBN, <Bus number>)         // PCI Base Bus Number
    Name (_CCA, 1)                    // Initially mark the PCI coherent

  @param [in]       PciInfo         Pci device information.
  @param [in]       Uid             Unique Id of the Pci device.
  @param [in, out]  PciNode         Pci node to amend.

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES   Failed to allocate memory.
**/
STATIC
EFI_STATUS
EFIAPI
GeneratePciDeviceInfo (
  IN      CONST CM_ARCH_COMMON_PCI_CONFIG_SPACE_INFO  *PciInfo,
  IN            UINT32                                Uid,
  IN  OUT       AML_OBJECT_NODE_HANDLE                PciNode
  )
{
  EFI_STATUS  Status;
  UINT32      EisaId;

  ASSERT (PciInfo != NULL);
  ASSERT (PciNode != NULL);

  // ASL: Name (_UID, <Uid>)
  Status = AmlCodeGenNameInteger ("_UID", Uid, PciNode, NULL);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // ASL: Name (_HID, EISAID ("PNP0A08"))
  Status = AmlGetEisaIdFromString ("PNP0A08", &EisaId);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  Status = AmlCodeGenNameInteger ("_HID", EisaId, PciNode, NULL);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // ASL: Name (_CID, EISAID ("PNP0A03"))
  Status = AmlGetEisaIdFromString ("PNP0A03", &EisaId);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  Status = AmlCodeGenNameInteger ("_CID", EisaId, PciNode, NULL);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // ASL: Name (_SEG, <Pci segment group>)
  Status = AmlCodeGenNameInteger (
             "_SEG",
             PciInfo->PciSegmentGroupNumber,
             PciNode,
             NULL
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // ASL: Name (_BBN, <Bus number>)
  Status = AmlCodeGenNameInteger (
             "_BBN",
             PciInfo->StartBusNumber,
             PciNode,
             NULL
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // ASL: Name (_CCA, 1)
  // Must be aligned with the IORT CCA property in
  // "Table 14 Memory access properties"
  Status = AmlCodeGenNameInteger ("_CCA", 1, PciNode, NULL);
  ASSERT_EFI_ERROR (Status);
  return Status;
}

/** Generate a _PRT object (Pci Routing Table) for the Pci device.

  Cf. ACPI 6.4 specification, s6.2.13 "_PRT (PCI Routing Table)"

  @param [in]       Generator       The SSDT Pci generator.
  @param [in]       CfgMgrProtocol  Pointer to the Configuration Manager
                                    Protocol interface.
  @param [in]       PciInfo         Pci device information.
  @param [in]       Uid             Unique Id of the Pci device.
  @param [in, out]  PciNode         Pci node to amend.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Could not allocate memory.
**/
STATIC
EFI_STATUS
EFIAPI
GeneratePrt (
  IN            ACPI_PCI_GENERATOR                            *Generator,
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN      CONST CM_ARCH_COMMON_PCI_CONFIG_SPACE_INFO          *PciInfo,
  IN            UINT32                                        Uid,
  IN  OUT       AML_OBJECT_NODE_HANDLE                        PciNode
  )
{
  EFI_STATUS                             Status;
  UINT32                                 Index;
  AML_OBJECT_NODE_HANDLE                 PrtNode;
  CM_ARCH_COMMON_OBJ_REF                 *RefInfo;
  UINT32                                 RefCount;
  CM_ARCH_COMMON_PCI_INTERRUPT_MAP_INFO  *IrqMapInfo;

  ASSERT (Generator != NULL);
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (PciInfo != NULL);
  ASSERT (PciNode != NULL);

  PrtNode = NULL;

  // Get the array of CM_ARCH_COMMON_OBJ_REF referencing the
  // CM_ARCH_COMMON_PCI_INTERRUPT_MAP_INFO objects.
  Status = GetEArchCommonObjCmRef (
             CfgMgrProtocol,
             PciInfo->InterruptMapToken,
             &RefInfo,
             &RefCount
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // Initialized DeviceTable.
  Status = MappingTableInitialize (&Generator->DeviceTable, RefCount);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto exit_handler0;
  }

  // ASL: Name (_PRT, Package () {})
  Status = AmlCodeGenNamePackage ("_PRT", NULL, &PrtNode);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto exit_handler;
  }

  for (Index = 0; Index < RefCount; Index++) {
    // Get CM_ARCH_COMMON_PCI_INTERRUPT_MAP_INFO structures one by one.
    Status = GetEArchCommonObjPciInterruptMapInfo (
               CfgMgrProtocol,
               RefInfo[Index].ReferenceToken,
               &IrqMapInfo,
               NULL
               );
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      goto exit_handler;
    }

    // Check that the interrupts flags are SPIs, level high.
    // Cf. Arm BSA v1.0, sE.6 "Legacy interrupts"
    if ((Index > 0)   &&
        (IrqMapInfo->IntcInterrupt.Interrupt >= 32)   &&
        (IrqMapInfo->IntcInterrupt.Interrupt < 1020)  &&
        ((IrqMapInfo->IntcInterrupt.Flags & 0xB) != 0))
    {
      Status = EFI_INVALID_PARAMETER;
      ASSERT_EFI_ERROR (Status);
      goto exit_handler;
    }

    // Add the device to the DeviceTable.
    MappingTableAdd (&Generator->DeviceTable, IrqMapInfo->PciDevice);

    /* Add a _PRT entry.
       ASL
       Name (_PRT, Package () {
         <OldPrtEntries>,
         <NewPrtEntry>
       })

     Address is set as:
     ACPI 6.4 specification, Table 6.2: "ADR Object Address Encodings"
       High word-Device #, Low word-Function #. (for example, device 3,
       function 2 is 0x00030002). To refer to all the functions on a device #,
       use a function number of FFFF).

      Use the second model for _PRT object and describe a hardwired interrupt.
    */
    Status = AmlAddPrtEntry (
               (IrqMapInfo->PciDevice << 16) | 0xFFFF,
               IrqMapInfo->PciInterrupt,
               NULL,
               IrqMapInfo->IntcInterrupt.Interrupt,
               PrtNode
               );
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      goto exit_handler;
    }
  } // for

  // Attach the _PRT entry.
  Status = AmlAttachNode (PciNode, PrtNode);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    goto exit_handler;
  }

  PrtNode = NULL;

  // Generate the Pci slots once all the device have been added.
  Status = GeneratePciSlots (PciInfo, &Generator->DeviceTable, Uid, PciNode);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    goto exit_handler;
  }

exit_handler:
  MappingTableFree (&Generator->DeviceTable);
exit_handler0:
  if (PrtNode != NULL) {
    AmlDeleteTree (PrtNode);
  }

  return Status;
}

/** Generate a _CRS method for the Pci device.

  @param [in]       Generator       The SSDT Pci generator.
  @param [in]       CfgMgrProtocol  Pointer to the Configuration Manager
                                    Protocol interface.
  @param [in]       PciInfo         Pci device information.
  @param [in, out]  PciNode         Pci node to amend.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Could not allocate memory.
**/
STATIC
EFI_STATUS
EFIAPI
GeneratePciCrs (
  IN            ACPI_PCI_GENERATOR                            *Generator,
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN      CONST CM_ARCH_COMMON_PCI_CONFIG_SPACE_INFO          *PciInfo,
  IN  OUT       AML_OBJECT_NODE_HANDLE                        PciNode
  )
{
  EFI_STATUS                           Status;
  BOOLEAN                              Translation;
  UINT32                               Index;
  CM_ARCH_COMMON_OBJ_REF               *RefInfo;
  UINT32                               RefCount;
  CM_ARCH_COMMON_PCI_ADDRESS_MAP_INFO  *AddrMapInfo;
  AML_OBJECT_NODE_HANDLE               CrsNode;
  BOOLEAN                              IsPosDecode;

  ASSERT (Generator != NULL);
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (PciInfo != NULL);
  ASSERT (PciNode != NULL);

  // ASL: Name (_CRS, ResourceTemplate () {})
  Status = AmlCodeGenNameResourceTemplate ("_CRS", PciNode, &CrsNode);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // ASL:
  // WordBusNumber (          // Bus numbers assigned to this root
  //   ResourceProducer, MinFixed, MaxFixed, PosDecode,
  //   0,                     // AddressGranularity
  //   <Start>,               // AddressMinimum - Minimum Bus Number
  //   <End>,                 // AddressMaximum - Maximum Bus Number
  //   0,                     // AddressTranslation - Set to 0
  //   <End> - <Start> + 1    // RangeLength - Number of Busses
  // )
  Status = AmlCodeGenRdWordBusNumber (
             FALSE,
             TRUE,
             TRUE,
             TRUE,
             0,
             PciInfo->StartBusNumber,
             PciInfo->EndBusNumber,
             0,
             PciInfo->EndBusNumber - PciInfo->StartBusNumber + 1,
             0,
             NULL,
             CrsNode,
             NULL
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // Get the array of CM_ARCH_COMMON_OBJ_REF referencing the
  // CM_ARCH_COMMON_PCI_ADDRESS_MAP_INFO objects.
  Status = GetEArchCommonObjCmRef (
             CfgMgrProtocol,
             PciInfo->AddressMapToken,
             &RefInfo,
             &RefCount
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  for (Index = 0; Index < RefCount; Index++) {
    // Get CM_ARCH_COMMON_PCI_ADDRESS_MAP_INFO structures one by one.
    Status = GetEArchCommonObjPciAddressMapInfo (
               CfgMgrProtocol,
               RefInfo[Index].ReferenceToken,
               &AddrMapInfo,
               NULL
               );
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      return Status;
    }

    Translation = (AddrMapInfo->CpuAddress != AddrMapInfo->PciAddress);
    if (AddrMapInfo->CpuAddress >= AddrMapInfo->PciAddress) {
      IsPosDecode = TRUE;
    } else {
      IsPosDecode = FALSE;
    }

    switch (AddrMapInfo->SpaceCode) {
      case PCI_SS_IO:
        Status = AmlCodeGenRdQWordIo (
                   FALSE,
                   TRUE,
                   TRUE,
                   IsPosDecode,
                   3,
                   0,
                   AddrMapInfo->PciAddress,
                   AddrMapInfo->PciAddress + AddrMapInfo->AddressSize - 1,
                   Translation ? AddrMapInfo->CpuAddress - AddrMapInfo->PciAddress : 0,
                   AddrMapInfo->AddressSize,
                   0,
                   NULL,
                   TRUE,
                   FALSE,
                   CrsNode,
                   NULL
                   );
        break;

      case PCI_SS_M32:
        ASSERT ((AddrMapInfo->PciAddress & ~MAX_UINT32) == 0);
        ASSERT (((AddrMapInfo->PciAddress + AddrMapInfo->AddressSize - 1) & ~MAX_UINT32) == 0);
        ASSERT (((Translation ? AddrMapInfo->CpuAddress - AddrMapInfo->PciAddress : 0) & ~MAX_UINT32) == 0);
        ASSERT ((AddrMapInfo->AddressSize & ~MAX_UINT32) == 0);

        Status = AmlCodeGenRdDWordMemory (
                   FALSE,
                   IsPosDecode,
                   TRUE,
                   TRUE,
                   AmlMemoryCacheable,
                   TRUE,
                   0,
                   (UINT32)(AddrMapInfo->PciAddress),
                   (UINT32)(AddrMapInfo->PciAddress + AddrMapInfo->AddressSize - 1),
                   (UINT32)(Translation ? AddrMapInfo->CpuAddress - AddrMapInfo->PciAddress : 0),
                   (UINT32)(AddrMapInfo->AddressSize),
                   0,
                   NULL,
                   AmlAddressRangeMemory,
                   TRUE,
                   CrsNode,
                   NULL
                   );
        break;

      case PCI_SS_M64:
        Status = AmlCodeGenRdQWordMemory (
                   FALSE,
                   IsPosDecode,
                   TRUE,
                   TRUE,
                   AmlMemoryCacheable,
                   TRUE,
                   0,
                   AddrMapInfo->PciAddress,
                   AddrMapInfo->PciAddress + AddrMapInfo->AddressSize - 1,
                   Translation ? AddrMapInfo->CpuAddress - AddrMapInfo->PciAddress : 0,
                   AddrMapInfo->AddressSize,
                   0,
                   NULL,
                   AmlAddressRangeMemory,
                   TRUE,
                   CrsNode,
                   NULL
                   );
        break;

      default:
        Status = EFI_INVALID_PARAMETER;
    } // switch

    if (EFI_ERROR (Status)) {
      ASSERT (0);
      return Status;
    }
  } // for

  return Status;
}

/** Generate a RES0 device node to reserve PNP motherboard resources
  for a given PCI node.

  @param [in]   PciNode       Parent PCI node handle of the generated
                              resource object.
  @param [out]  CrsNode       CRS node of the AML tree to populate.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid input parameter.
  @retval EFI_OUT_OF_RESOURCES    Could not allocate memory.
**/
STATIC
EFI_STATUS
EFIAPI
GenerateMotherboardDevice (
  IN  AML_OBJECT_NODE_HANDLE  PciNode,
  OUT AML_OBJECT_NODE_HANDLE  *CrsNode
  )
{
  EFI_STATUS              Status;
  UINT32                  EisaId;
  AML_OBJECT_NODE_HANDLE  ResNode;

  if (CrsNode == NULL) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // ASL: Device (RES0) {}
  Status = AmlCodeGenDevice ("RES0", PciNode, &ResNode);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // ASL: Name (_HID, EISAID ("PNP0C02"))
  Status = AmlGetEisaIdFromString ("PNP0C02", &EisaId); /* PNP Motherboard Resources */
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  Status = AmlCodeGenNameInteger ("_HID", EisaId, ResNode, NULL);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // ASL: Name (_CRS, ResourceTemplate () {})
  Status = AmlCodeGenNameResourceTemplate ("_CRS", ResNode, CrsNode);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  return Status;
}

/** Reserves ECAM space for PCI config space

  @param [in]       Generator       The SSDT Pci generator.
  @param [in]       CfgMgrProtocol  Pointer to the Configuration Manager
                                    Protocol interface.
  @param [in]       PciInfo         Pci device information.
  @param [in, out]  PciNode         RootNode of the AML tree to populate.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Could not allocate memory.
**/
STATIC
EFI_STATUS
EFIAPI
ReserveEcamSpace (
  IN            ACPI_PCI_GENERATOR                            *Generator,
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN      CONST CM_ARCH_COMMON_PCI_CONFIG_SPACE_INFO          *PciInfo,
  IN  OUT       AML_OBJECT_NODE_HANDLE                        PciNode
  )
{
  EFI_STATUS              Status;
  AML_OBJECT_NODE_HANDLE  CrsNode;
  UINT64                  AddressMinimum;
  UINT64                  AddressMaximum;

  Status = GenerateMotherboardDevice (PciNode, &CrsNode);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  AddressMinimum = PciInfo->BaseAddress + (PciInfo->StartBusNumber *
                                           PCI_MAX_DEVICE_COUNT_PER_BUS * PCI_MAX_FUNCTION_COUNT_PER_DEVICE * SIZE_4KB);
  AddressMaximum = PciInfo->BaseAddress + ((PciInfo->EndBusNumber + 1) *
                                           PCI_MAX_DEVICE_COUNT_PER_BUS * PCI_MAX_FUNCTION_COUNT_PER_DEVICE * SIZE_4KB) - 1;

  Status = AmlCodeGenRdQWordMemory (
             FALSE,
             TRUE,
             TRUE,
             TRUE,
             AmlMemoryNonCacheable,
             TRUE,
             0,
             AddressMinimum,
             AddressMaximum,
             0,  // no translation
             AddressMaximum - AddressMinimum + 1,
             0,
             NULL,
             AmlAddressRangeMemory,
             TRUE,
             CrsNode,
             NULL
             );

  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  return Status;
}

/** Generate a Pci device.

  @param [in]       Generator       The SSDT Pci generator.
  @param [in]       CfgMgrProtocol  Pointer to the Configuration Manager
                                    Protocol interface.
  @param [in]       PciInfo         Pci device information.
  @param [in]       Uid             Unique Id of the Pci device.
  @param [in, out]  RootNode        RootNode of the AML tree to populate.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Could not allocate memory.
**/
STATIC
EFI_STATUS
EFIAPI
GeneratePciDevice (
  IN            ACPI_PCI_GENERATOR                            *Generator,
  IN      CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN      CONST CM_ARCH_COMMON_PCI_CONFIG_SPACE_INFO          *PciInfo,
  IN            UINT32                                        Uid,
  IN  OUT       AML_ROOT_NODE_HANDLE                          *RootNode
  )
{
  EFI_STATUS  Status;

  CHAR8                   AslName[AML_NAME_SEG_SIZE + 1];
  AML_OBJECT_NODE_HANDLE  ScopeNode;
  AML_OBJECT_NODE_HANDLE  PciNode;

  ASSERT (Generator != NULL);
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (PciInfo != NULL);
  ASSERT (RootNode != NULL);

  PciNode = NULL;

  // ASL: Scope (\_SB) {}
  Status = AmlCodeGenScope (SB_SCOPE, RootNode, &ScopeNode);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // Write the name of the PCI device.
  CopyMem (AslName, "PCIx", AML_NAME_SEG_SIZE + 1);
  AslName[AML_NAME_SEG_SIZE - 1] = AsciiFromHex (Uid & 0xF);
  if (Uid > 0xF) {
    AslName[AML_NAME_SEG_SIZE - 2] = AsciiFromHex ((Uid >> 4) & 0xF);
  }

  // ASL: Device (PCIx) {}
  Status = AmlCodeGenDevice (AslName, ScopeNode, &PciNode);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // Populate the PCIx node with some Id values.
  Status = GeneratePciDeviceInfo (PciInfo, Uid, PciNode);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // Generate the Pci Routing Table (_PRT).
  if (PciInfo->InterruptMapToken != CM_NULL_TOKEN) {
    Status = GeneratePrt (
               Generator,
               CfgMgrProtocol,
               PciInfo,
               Uid,
               PciNode
               );
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      return Status;
    }
  }

  // Generate the _CRS method.
  Status = GeneratePciCrs (Generator, CfgMgrProtocol, PciInfo, PciNode);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // Add the PNP Motherboard Resources Device to reserve ECAM space
  Status = ReserveEcamSpace (Generator, CfgMgrProtocol, PciInfo, PciNode);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // Add the template _OSC method.
  Status = AddOscMethod (PciInfo, PciNode);
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/** Build an Ssdt table describing a Pci device.

  @param [in]  Generator        The SSDT Pci generator.
  @param [in]  CfgMgrProtocol   Pointer to the Configuration Manager
                                Protocol interface.
  @param [in]  AcpiTableInfo    Pointer to the ACPI table information.
  @param [in]  PciInfo          Pci device information.
  @param [in]  Uid              Unique Id of the Pci device.
  @param [out] Table            If success, contains the created SSDT table.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    Could not allocate memory.
**/
STATIC
EFI_STATUS
EFIAPI
BuildSsdtPciTable (
  IN        ACPI_PCI_GENERATOR                            *Generator,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL  *CONST  CfgMgrProtocol,
  IN  CONST CM_STD_OBJ_ACPI_TABLE_INFO            *CONST  AcpiTableInfo,
  IN  CONST CM_ARCH_COMMON_PCI_CONFIG_SPACE_INFO          *PciInfo,
  IN        UINT32                                        Uid,
  OUT       EFI_ACPI_DESCRIPTION_HEADER                   **Table
  )
{
  EFI_STATUS            Status;
  EFI_STATUS            Status1;
  AML_ROOT_NODE_HANDLE  RootNode;

  ASSERT (Generator != NULL);
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (PciInfo != NULL);
  ASSERT (Table != NULL);

  // Create a new Ssdt table.
  Status = AddSsdtAcpiHeader (
             CfgMgrProtocol,
             &Generator->Header,
             AcpiTableInfo,
             &RootNode
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  Status = GeneratePciDevice (
             Generator,
             CfgMgrProtocol,
             PciInfo,
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
      "ERROR: SSDT-PCI: Failed to Serialize SSDT Table Data."
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
      "ERROR: SSDT-PCI: Failed to cleanup AML tree."
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

/** Construct SSDT tables describing Pci root complexes.

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
BuildSsdtPciTableEx (
  IN  CONST ACPI_TABLE_GENERATOR                           *This,
  IN  CONST CM_STD_OBJ_ACPI_TABLE_INFO             *CONST  AcpiTableInfo,
  IN  CONST EDKII_CONFIGURATION_MANAGER_PROTOCOL   *CONST  CfgMgrProtocol,
  OUT       EFI_ACPI_DESCRIPTION_HEADER                    ***Table,
  OUT       UINTN                                  *CONST  TableCount
  )
{
  EFI_STATUS                            Status;
  CM_ARCH_COMMON_PCI_CONFIG_SPACE_INFO  *PciInfo;
  UINT32                                PciCount;
  UINT32                                Index;
  EFI_ACPI_DESCRIPTION_HEADER           **TableList;
  ACPI_PCI_GENERATOR                    *Generator;
  UINT32                                Uid;

  ASSERT (This != NULL);
  ASSERT (AcpiTableInfo != NULL);
  ASSERT (CfgMgrProtocol != NULL);
  ASSERT (Table != NULL);
  ASSERT (TableCount != NULL);
  ASSERT (AcpiTableInfo->TableGeneratorId == This->GeneratorID);
  ASSERT (AcpiTableInfo->AcpiTableSignature == This->AcpiTableSignature);

  *TableCount = 0;
  Generator   = (ACPI_PCI_GENERATOR *)This;

  Status = GetEArchCommonObjPciConfigSpaceInfo (
             CfgMgrProtocol,
             CM_NULL_TOKEN,
             &PciInfo,
             &PciCount
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  if (PciCount > MAX_PCI_ROOT_COMPLEXES_SUPPORTED) {
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-PCI: Too many Pci root complexes: %d."
      " Maximum Pci root complexes supported = %d.\n",
      PciCount,
      MAX_PCI_ROOT_COMPLEXES_SUPPORTED
      ));
    return EFI_INVALID_PARAMETER;
  }

  // Allocate a table to store pointers to the SSDT tables.
  TableList = (EFI_ACPI_DESCRIPTION_HEADER **)
              AllocateZeroPool (
                (sizeof (EFI_ACPI_DESCRIPTION_HEADER *) * PciCount)
                );
  if (TableList == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    DEBUG ((
      DEBUG_ERROR,
      "ERROR: SSDT-PCI: Failed to allocate memory for Table List."
      " Status = %r\n",
      Status
      ));
    return Status;
  }

  // Setup the table list early so that appropriate cleanup
  // can be done in case of failure.
  *Table = TableList;

  for (Index = 0; Index < PciCount; Index++) {
    if (PcdGetBool (PcdPciUseSegmentAsUid)) {
      Uid = PciInfo[Index].PciSegmentGroupNumber;
      if (Uid > MAX_PCI_ROOT_COMPLEXES_SUPPORTED) {
        DEBUG ((
          DEBUG_ERROR,
          "ERROR: SSDT-PCI: Pci root complexes segment number: %d."
          " Greater than maximum number of Pci root complexes supported = %d.\n",
          Uid,
          MAX_PCI_ROOT_COMPLEXES_SUPPORTED
          ));
        return EFI_INVALID_PARAMETER;
      }
    } else {
      Uid = Index;
    }

    // Build a SSDT table describing the Pci devices.
    Status = BuildSsdtPciTable (
               Generator,
               CfgMgrProtocol,
               AcpiTableInfo,
               &PciInfo[Index],
               Uid,
               &TableList[Index]
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((
        DEBUG_ERROR,
        "ERROR: SSDT-PCI: Failed to build associated SSDT table."
        " Status = %r\n",
        Status
        ));
      goto error_handler;
    }

    *TableCount += 1;
  } // for

error_handler:
  // Note: Table list and Table count have been setup. The
  // error handler does nothing here as the framework will invoke
  // FreeSsdtPciTableEx () even on failure.
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
FreeSsdtPciTableEx (
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
    DEBUG ((DEBUG_ERROR, "ERROR: SSDT-PCI: Invalid Table Pointer\n"));
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
        "ERROR: SSDT-PCI: Could not free SSDT table at index %d.",
        Index
        ));
      return EFI_INVALID_PARAMETER;
    }
  } // for

  // Free the table list.
  FreePool (*Table);

  return EFI_SUCCESS;
}

/** This macro defines the SSDT Pci Table Generator revision.
*/
#define SSDT_PCI_GENERATOR_REVISION  CREATE_REVISION (1, 0)

/** The interface for the SSDT Pci Table Generator.
*/
STATIC
ACPI_PCI_GENERATOR  SsdtPcieGenerator = {
  // ACPI table generator header
  {
    // Generator ID
    CREATE_STD_ACPI_TABLE_GEN_ID (EStdAcpiTableIdSsdtPciExpress),
    // Generator Description
    L"ACPI.STD.SSDT.PCI.GENERATOR",
    // ACPI Table Signature
    EFI_ACPI_6_3_SECONDARY_SYSTEM_DESCRIPTION_TABLE_SIGNATURE,
    // ACPI Table Revision - Unused
    0,
    // Minimum ACPI Table Revision - Unused
    0,
    // Creator ID
    TABLE_GENERATOR_CREATOR_ID,
    // Creator Revision
    SSDT_PCI_GENERATOR_REVISION,
    // Build table function. Use the extended version instead.
    NULL,
    // Free table function. Use the extended version instead.
    NULL,
    // Extended Build table function.
    BuildSsdtPciTableEx,
    // Extended free function.
    FreeSsdtPciTableEx
  },

  // Private fields are defined from here.

  // DeviceTable
  {
    // Table
    NULL,
    // LastIndex
    0,
    // MaxIndex
    0
  },
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
AcpiSsdtPcieLibConstructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = RegisterAcpiTableGenerator (&SsdtPcieGenerator.Header);
  DEBUG ((
    DEBUG_INFO,
    "SSDT-PCI: Register Generator. Status = %r\n",
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
AcpiSsdtPcieLibDestructor (
  IN  EFI_HANDLE        ImageHandle,
  IN  EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = DeregisterAcpiTableGenerator (&SsdtPcieGenerator.Header);
  DEBUG ((
    DEBUG_INFO,
    "SSDT-PCI: Deregister Generator. Status = %r\n",
    Status
    ));
  ASSERT_EFI_ERROR (Status);
  return Status;
}
