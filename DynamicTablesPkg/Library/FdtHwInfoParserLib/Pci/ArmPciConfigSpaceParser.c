/** @file
  Arm PCI Configuration Space Parser.

  Copyright (c) 2021, ARM Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - linux/Documentation/devicetree/bindings/pci/host-generic-pci.yaml
  - PCI Firmware Specification - Revision 3.0
  - Open Firmware Recommended Practice: Interrupt Mapping, Version 0.9
  - Devicetree Specification Release v0.3
  - linux kernel code
**/

#include "CmObjectDescUtility.h"
#include <Library/DebugLib.h>

#include "FdtHwInfoParser.h"
#include "Pci/ArmPciConfigSpaceParser.h"
#include "Gic/ArmGicDispatcher.h"

/** List of "compatible" property values for host PCIe bridges nodes.

  Any other "compatible" value is not supported by this module.
*/
STATIC CONST COMPATIBILITY_STR  PciCompatibleStr[] = {
  { "pci-host-ecam-generic" }
};

/** COMPATIBILITY_INFO structure for the PCIe.
*/
STATIC CONST COMPATIBILITY_INFO  PciCompatibleInfo = {
  ARRAY_SIZE (PciCompatibleStr),
  PciCompatibleStr
};

/** Get the Segment group (also called: Domain Id) of a host-pci node.

  kernel/Documentation/devicetree/bindings/pci/pci.txt:
  "It is required to either not set this property at all or set it for all
  host bridges in the system"

  The function checks the "linux,pci-domain" property of the host-pci node.
  Either all host-pci nodes must have this property, or none of them. If the
  property is available, read it. Otherwise dynamically assign the Ids.

  @param [in]  Fdt          Pointer to a Flattened Device Tree (Fdt).
  @param [in]  HostPciNode  Offset of a host-pci node.
  @param [out] SegGroup     Segment group assigned to the host-pci controller.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
STATIC
EFI_STATUS
EFIAPI
GetPciSegGroup (
  IN  CONST VOID   *Fdt,
  IN        INT32  HostPciNode,
  OUT       INT32  *SegGroup
  )
{
  CONST UINT8   *Data;
  INT32         DataSize;
  STATIC INT32  LocalSegGroup = 0;

  if ((Fdt == NULL) ||
      (SegGroup == NULL))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  Data = fdt_getprop (Fdt, HostPciNode, "linux,pci-domain", &DataSize);
  if ((Data == NULL) || (DataSize < 0)) {
    // Did not find property, assign the DomainIds ourselves.
    if (LocalSegGroup < 0) {
      // "linux,pci-domain" property was defined for another node.
      ASSERT (0);
      return EFI_ABORTED;
    }

    *SegGroup = LocalSegGroup++;
    return EFI_SUCCESS;
  }

  if ((DataSize > sizeof (UINT32))  ||
      (LocalSegGroup > 0))
  {
    // Property on more than 1 cell or
    // "linux,pci-domain" property was not defined for a node.
    ASSERT (0);
    return EFI_ABORTED;
  }

  // If one node has the "linux,pci-domain" property, then all the host-pci
  // nodes must have it.
  LocalSegGroup = -1;

  *SegGroup = fdt32_to_cpu (*(UINT32 *)Data);
  return EFI_SUCCESS;
}

/** Parse the bus-range controlled by this host-pci node.

  @param [in]       Fdt           Pointer to a Flattened Device Tree (Fdt).
  @param [in]       HostPciNode   Offset of a host-pci node.
  @param [in, out]  PciInfo       PCI_PARSER_TABLE structure storing
                                  information about the current host-pci.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
STATIC
EFI_STATUS
EFIAPI
PopulateBusRange (
  IN      CONST VOID              *Fdt,
  IN            INT32             HostPciNode,
  IN OUT        PCI_PARSER_TABLE  *PciInfo
  )
{
  CONST UINT8  *Data;
  INT32        DataSize;
  UINT32       StartBus;
  UINT32       EndBus;

  if ((Fdt == NULL) ||
      (PciInfo == NULL))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  Data = fdt_getprop (Fdt, HostPciNode, "bus-range", &DataSize);
  if ((Data == NULL) || (DataSize < 0)) {
    // No evidence this property is mandatory. Use default values.
    StartBus = 0;
    EndBus   = 255;
  } else if (DataSize == (2 * sizeof (UINT32))) {
    // If available, the property is on two integers.
    StartBus = fdt32_to_cpu (((UINT32 *)Data)[0]);
    EndBus   = fdt32_to_cpu (((UINT32 *)Data)[1]);
  } else {
    ASSERT (0);
    return EFI_ABORTED;
  }

  PciInfo->PciConfigSpaceInfo.StartBusNumber = StartBus;
  PciInfo->PciConfigSpaceInfo.EndBusNumber   = EndBus;

  return EFI_SUCCESS;
}

/** Parse the PCI address map.

  The PCI address map is available in the "ranges" device-tree property.

  @param [in]       Fdt           Pointer to a Flattened Device Tree (Fdt).
  @param [in]       HostPciNode   Offset of a host-pci node.
  @param [in]       AddressCells  # of cells used to encode an address on
                                  the parent bus.
  @param [in, out]  PciInfo       PCI_PARSER_TABLE structure storing
                                  information about the current host-pci.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    An allocation has failed.
**/
STATIC
EFI_STATUS
EFIAPI
ParseAddressMap (
  IN      CONST VOID              *Fdt,
  IN            INT32             HostPciNode,
  IN            INT32             AddressCells,
  IN OUT        PCI_PARSER_TABLE  *PciInfo
  )
{
  CONST UINT8  *Data;
  INT32        DataSize;
  UINT32       Index;
  UINT32       Offset;
  UINT32       AddressMapSize;
  UINT32       Count;
  UINT32       PciAddressAttr;

  CM_ARM_PCI_ADDRESS_MAP_INFO  *PciAddressMapInfo;
  UINT32                       BufferSize;

  // The mapping is done on AddressMapSize bytes.
  AddressMapSize = (PCI_ADDRESS_CELLS + AddressCells + PCI_SIZE_CELLS) *
                   sizeof (UINT32);

  Data = fdt_getprop (Fdt, HostPciNode, "ranges", &DataSize);
  if ((Data == NULL) ||
      (DataSize < 0) ||
      ((DataSize % AddressMapSize) != 0))
  {
    // If error or not on AddressMapSize bytes.
    ASSERT (0);
    return EFI_ABORTED;
  }

  Count = DataSize / AddressMapSize;

  // Allocate a buffer to store each address mapping.
  BufferSize        = Count * sizeof (CM_ARM_PCI_ADDRESS_MAP_INFO);
  PciAddressMapInfo = AllocateZeroPool (BufferSize);
  if (PciAddressMapInfo == NULL) {
    ASSERT (0);
    return EFI_OUT_OF_RESOURCES;
  }

  for (Index = 0; Index < Count; Index++) {
    Offset = Index * AddressMapSize;

    // Pci address attributes
    PciAddressAttr                     = fdt32_to_cpu (*(UINT32 *)&Data[Offset]);
    PciAddressMapInfo[Index].SpaceCode = READ_PCI_SS (PciAddressAttr);
    Offset                            += sizeof (UINT32);

    // Pci address
    PciAddressMapInfo[Index].PciAddress =
      fdt64_to_cpu (*(UINT64 *)&Data[Offset]);
    Offset += (PCI_ADDRESS_CELLS - 1) * sizeof (UINT32);

    // Cpu address
    if (AddressCells == 2) {
      PciAddressMapInfo[Index].CpuAddress =
        fdt64_to_cpu (*(UINT64 *)&Data[Offset]);
    } else {
      PciAddressMapInfo[Index].CpuAddress =
        fdt32_to_cpu (*(UINT32 *)&Data[Offset]);
    }

    Offset += AddressCells * sizeof (UINT32);

    // Address size
    PciAddressMapInfo[Index].AddressSize =
      fdt64_to_cpu (*(UINT64 *)&Data[Offset]);
    Offset += PCI_SIZE_CELLS * sizeof (UINT32);
  } // for

  PciInfo->Mapping[PciMappingTableAddress].ObjectId =
    CREATE_CM_ARM_OBJECT_ID (EArmObjPciAddressMapInfo);
  PciInfo->Mapping[PciMappingTableAddress].Size =
    sizeof (CM_ARM_PCI_ADDRESS_MAP_INFO) * Count;
  PciInfo->Mapping[PciMappingTableAddress].Data  = PciAddressMapInfo;
  PciInfo->Mapping[PciMappingTableAddress].Count = Count;

  return EFI_SUCCESS;
}

/** Parse the PCI interrupt map.

  The PCI interrupt map is available in the "interrupt-map"
  and "interrupt-map-mask" device-tree properties.

  Cf Devicetree Specification Release v0.3,
  s2.4.3 Interrupt Nexus Properties

  An interrupt-map must be as:
  interrupt-map = < [child unit address] [child interrupt specifier]
                    [interrupt-parent]
                    [parent unit address] [parent interrupt specifier] >

  @param [in]       Fdt           Pointer to a Flattened Device Tree (Fdt).
  @param [in]       HostPciNode   Offset of a host-pci node.
  @param [in, out]  PciInfo       PCI_PARSER_TABLE structure storing
                                  information about the current host-pci.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_NOT_FOUND           Not found.
  @retval EFI_OUT_OF_RESOURCES    An allocation has failed.
**/
STATIC
EFI_STATUS
EFIAPI
ParseIrqMap (
  IN      CONST VOID              *Fdt,
  IN            INT32             HostPciNode,
  IN OUT        PCI_PARSER_TABLE  *PciInfo
  )
{
  EFI_STATUS   Status;
  CONST UINT8  *Data;
  INT32        DataSize;
  UINT32       Index;
  UINT32       Offset;

  INT32  IntcNode;
  INT32  IntcAddressCells;
  INT32  IntcCells;

  INT32  PciIntCells;
  INT32  IntcPhandle;

  INT32        IrqMapSize;
  UINT32       IrqMapCount;
  CONST UINT8  *IrqMapMask;
  INT32        IrqMapMaskSize;

  INT32   PHandleOffset;
  UINT32  GicVersion;

  UINT32  PciAddressAttr;

  CM_ARM_PCI_INTERRUPT_MAP_INFO  *PciInterruptMapInfo;
  UINT32                         BufferSize;

  Data = fdt_getprop (Fdt, HostPciNode, "interrupt-map", &DataSize);
  if ((Data == NULL) || (DataSize <= 0)) {
    DEBUG ((
      DEBUG_WARN,
      "Fdt parser: No Legacy interrupts found for PCI configuration space at "
      "address: 0x%lx, group segment: %d\n",
      PciInfo->PciConfigSpaceInfo.BaseAddress,
      PciInfo->PciConfigSpaceInfo.PciSegmentGroupNumber
      ));
    return EFI_NOT_FOUND;
  }

  // PCI interrupts are expected to be on 1 cell. Check it.
  Status = FdtGetInterruptCellsInfo (Fdt, HostPciNode, &PciIntCells);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  if (PciIntCells != PCI_INTERRUPTS_CELLS) {
    ASSERT (0);
    return EFI_ABORTED;
  }

  IrqMapMask = fdt_getprop (
                 Fdt,
                 HostPciNode,
                 "interrupt-map-mask",
                 &IrqMapMaskSize
                 );
  if ((IrqMapMask == NULL) ||
      (IrqMapMaskSize !=
       (PCI_ADDRESS_CELLS + PCI_INTERRUPTS_CELLS) * sizeof (UINT32)))
  {
    ASSERT (0);
    return EFI_ABORTED;
  }

  // Get the interrupt-controller of the first irq mapping.
  PHandleOffset = (PCI_ADDRESS_CELLS + PciIntCells) * sizeof (UINT32);
  if (PHandleOffset > DataSize) {
    ASSERT (0);
    return EFI_ABORTED;
  }

  IntcPhandle = fdt32_to_cpu (*(UINT32 *)&Data[PHandleOffset]);
  IntcNode    = fdt_node_offset_by_phandle (Fdt, IntcPhandle);
  if (IntcNode < 0) {
    ASSERT (0);
    return EFI_ABORTED;
  }

  // Only support Gic(s) for now.
  Status = GetGicVersion (Fdt, IntcNode, &GicVersion);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // Get the "address-cells" property of the IntcNode.
  Status = FdtGetAddressInfo (Fdt, IntcNode, &IntcAddressCells, NULL);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // Get the "interrupt-cells" property of the IntcNode.
  Status = FdtGetInterruptCellsInfo (Fdt, IntcNode, &IntcCells);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // An irq mapping is done on IrqMapSize bytes
  // (which includes 1 cell for the PHandle).
  IrqMapSize = (PCI_ADDRESS_CELLS + PciIntCells + 1
                + IntcAddressCells + IntcCells) * sizeof (UINT32);
  if ((DataSize % IrqMapSize) != 0) {
    // The mapping is not done on IrqMapSize bytes.
    ASSERT (0);
    return EFI_ABORTED;
  }

  IrqMapCount = DataSize / IrqMapSize;

  // We assume the same interrupt-controller is used for all the mappings.
  // Check this is correct.
  for (Index = 0; Index < IrqMapCount; Index++) {
    if (IntcPhandle != fdt32_to_cpu (
                         *(UINT32 *)&Data[(Index * IrqMapSize) + PHandleOffset]
                         ))
    {
      ASSERT (0);
      return EFI_ABORTED;
    }
  }

  // Allocate a buffer to store each interrupt mapping.
  IrqMapCount         = DataSize / IrqMapSize;
  BufferSize          = IrqMapCount * sizeof (CM_ARM_PCI_ADDRESS_MAP_INFO);
  PciInterruptMapInfo = AllocateZeroPool (BufferSize);
  if (PciInterruptMapInfo == NULL) {
    ASSERT (0);
    return EFI_OUT_OF_RESOURCES;
  }

  for (Index = 0; Index < IrqMapCount; Index++) {
    Offset = Index * IrqMapSize;

    // Pci address attributes
    PciAddressAttr = fdt32_to_cpu (
                       (*(UINT32 *)&Data[Offset]) &
                       (*(UINT32 *)&IrqMapMask[0])
                       );
    PciInterruptMapInfo[Index].PciBus    = READ_PCI_BBBBBBBB (PciAddressAttr);
    PciInterruptMapInfo[Index].PciDevice = READ_PCI_DDDDD (PciAddressAttr);
    Offset                              += PCI_ADDRESS_CELLS * sizeof (UINT32);

    // Pci irq
    PciInterruptMapInfo[Index].PciInterrupt = fdt32_to_cpu (
                                                (*(UINT32 *)&Data[Offset]) &
                                                (*(UINT32 *)&IrqMapMask[3 * sizeof (UINT32)])
                                                );
    // -1 to translate from device-tree (INTA=1) to ACPI (INTA=0) irq IDs.
    PciInterruptMapInfo[Index].PciInterrupt -= 1;
    Offset                                  += PCI_INTERRUPTS_CELLS * sizeof (UINT32);

    // PHandle (skip it)
    Offset += sizeof (UINT32);

    // "Parent unit address" (skip it)
    Offset += IntcAddressCells * sizeof (UINT32);

    // Interrupt controller interrupt and flags
    PciInterruptMapInfo[Index].IntcInterrupt.Interrupt =
      FdtGetInterruptId ((UINT32 *)&Data[Offset]);
    PciInterruptMapInfo[Index].IntcInterrupt.Flags =
      FdtGetInterruptFlags ((UINT32 *)&Data[Offset]);
  } // for

  PciInfo->Mapping[PciMappingTableInterrupt].ObjectId =
    CREATE_CM_ARM_OBJECT_ID (EArmObjPciInterruptMapInfo);
  PciInfo->Mapping[PciMappingTableInterrupt].Size =
    sizeof (CM_ARM_PCI_INTERRUPT_MAP_INFO) * IrqMapCount;
  PciInfo->Mapping[PciMappingTableInterrupt].Data  = PciInterruptMapInfo;
  PciInfo->Mapping[PciMappingTableInterrupt].Count = IrqMapCount;

  return Status;
}

/** Parse a Host-pci node.

  @param [in]       Fdt          Pointer to a Flattened Device Tree (Fdt).
  @param [in]       HostPciNode  Offset of a host-pci node.
  @param [in, out]  PciInfo      The CM_ARM_PCI_CONFIG_SPACE_INFO to populate.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    An allocation has failed.
**/
STATIC
EFI_STATUS
EFIAPI
PciNodeParser (
  IN      CONST VOID              *Fdt,
  IN            INT32             HostPciNode,
  IN OUT        PCI_PARSER_TABLE  *PciInfo
  )
{
  EFI_STATUS   Status;
  INT32        AddressCells;
  INT32        SizeCells;
  CONST UINT8  *Data;
  INT32        DataSize;
  INT32        SegGroup;

  if ((Fdt == NULL) ||
      (PciInfo == NULL))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // Segment Group / DomainId
  Status = GetPciSegGroup (Fdt, HostPciNode, &SegGroup);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  PciInfo->PciConfigSpaceInfo.PciSegmentGroupNumber = SegGroup;

  // Bus range
  Status = PopulateBusRange (Fdt, HostPciNode, PciInfo);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  Status = FdtGetParentAddressInfo (
             Fdt,
             HostPciNode,
             &AddressCells,
             &SizeCells
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // Only support 32/64 bits addresses.
  if ((AddressCells < 1)  ||
      (AddressCells > 2)  ||
      (SizeCells < 1)     ||
      (SizeCells > 2))
  {
    ASSERT (0);
    return EFI_ABORTED;
  }

  Data = fdt_getprop (Fdt, HostPciNode, "reg", &DataSize);
  if ((Data == NULL) ||
      (DataSize != ((AddressCells + SizeCells) * sizeof (UINT32))))
  {
    // If error or wrong size.
    ASSERT (0);
    return EFI_ABORTED;
  }

  // Base address
  if (AddressCells == 2) {
    PciInfo->PciConfigSpaceInfo.BaseAddress = fdt64_to_cpu (*(UINT64 *)Data);
  } else {
    PciInfo->PciConfigSpaceInfo.BaseAddress = fdt32_to_cpu (*(UINT32 *)Data);
  }

  // Address map
  Status = ParseAddressMap (
             Fdt,
             HostPciNode,
             AddressCells,
             PciInfo
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // Irq map
  Status = ParseIrqMap (
             Fdt,
             HostPciNode,
             PciInfo
             );
  if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    ASSERT (0);
  }

  return EFI_SUCCESS;
}

/** Add the parsed Pci information to the Configuration Manager.

  CmObj of the following types are concerned:
   - EArmObjPciConfigSpaceInfo
   - EArmObjPciAddressMapInfo
   - EArmObjPciInterruptMapInfo

  @param [in]  FdtParserHandle  A handle to the parser instance.
  @param [in]  PciTableInfo     PCI_PARSER_TABLE structure containing the
                                CmObjs to add.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES    An allocation has failed.
**/
STATIC
EFI_STATUS
EFIAPI
PciInfoAdd (
  IN  CONST FDT_HW_INFO_PARSER_HANDLE  FdtParserHandle,
  IN        PCI_PARSER_TABLE           *PciTableInfo
  )
{
  EFI_STATUS                    Status;
  CM_ARM_PCI_CONFIG_SPACE_INFO  *PciConfigSpaceInfo;

  if ((FdtParserHandle == NULL) ||
      (PciTableInfo == NULL))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  PciConfigSpaceInfo = &PciTableInfo->PciConfigSpaceInfo;

  // Add the address map space CmObj to the Configuration Manager.
  Status = AddMultipleCmObjWithCmObjRef (
             FdtParserHandle,
             &PciTableInfo->Mapping[PciMappingTableAddress],
             &PciConfigSpaceInfo->AddressMapToken
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // Add the interrupt map space CmObj to the Configuration Manager.
  // Possible to have no legacy interrupts, or no device described and
  // thus no interrupt-mapping.
  if (PciTableInfo->Mapping[PciMappingTableInterrupt].Count != 0) {
    Status = AddMultipleCmObjWithCmObjRef (
               FdtParserHandle,
               &PciTableInfo->Mapping[PciMappingTableInterrupt],
               &PciConfigSpaceInfo->InterruptMapToken
               );
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      return Status;
    }
  }

  // Add the configuration space CmObj to the Configuration Manager.
  Status = AddSingleCmObj (
             FdtParserHandle,
             CREATE_CM_ARM_OBJECT_ID (EArmObjPciConfigSpaceInfo),
             &PciTableInfo->PciConfigSpaceInfo,
             sizeof (CM_ARM_PCI_CONFIG_SPACE_INFO),
             NULL
             );
  ASSERT_EFI_ERROR (Status);
  return Status;
}

/** Free the CmObjDesc of the ParserTable.

  @param [in]  PciTableInfo     PCI_PARSER_TABLE structure containing the
                                CmObjs to free.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
STATIC
EFI_STATUS
EFIAPI
FreeParserTable (
  IN  PCI_PARSER_TABLE  *PciTableInfo
  )
{
  UINT32  Index;
  VOID    *Data;

  if (PciTableInfo == NULL) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  for (Index = 0; Index < PciMappingTableMax; Index++) {
    Data = PciTableInfo->Mapping[Index].Data;
    if (Data != NULL) {
      FreePool (Data);
    }
  }

  return EFI_SUCCESS;
}

/** CM_ARM_PCI_CONFIG_SPACE_INFO parser function.

  The following structure is populated:
  typedef struct CmArmPciConfigSpaceInfo {
    UINT64  BaseAddress;                          // {Populated}
    UINT16  PciSegmentGroupNumber;                // {Populated}
    UINT8   StartBusNumber;                       // {Populated}
    UINT8   EndBusNumber;                         // {Populated}
  } CM_ARM_PCI_CONFIG_SPACE_INFO;

  typedef struct CmArmPciAddressMapInfo {
    UINT8                     SpaceCode;          // {Populated}
    UINT64                    PciAddress;         // {Populated}
    UINT64                    CpuAddress;         // {Populated}
    UINT64                    AddressSize;        // {Populated}
  } CM_ARM_PCI_ADDRESS_MAP_INFO;

  typedef struct CmArmPciInterruptMapInfo {
    UINT8                       PciBus;           // {Populated}
    UINT8                       PciDevice;        // {Populated}
    UINT8                       PciInterrupt;     // {Populated}
    CM_ARM_GENERIC_INTERRUPT    IntcInterrupt;    // {Populated}
  } CM_ARM_PCI_INTERRUPT_MAP_INFO;

  A parser parses a Device Tree to populate a specific CmObj type. None,
  one or many CmObj can be created by the parser.
  The created CmObj are then handed to the parser's caller through the
  HW_INFO_ADD_OBJECT interface.
  This can also be a dispatcher. I.e. a function that not parsing a
  Device Tree but calling other parsers.

  @param [in]  FdtParserHandle A handle to the parser instance.
  @param [in]  FdtBranch       When searching for DT node name, restrict
                               the search to this Device Tree branch.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_NOT_FOUND           Not found.
  @retval EFI_UNSUPPORTED         Unsupported.
**/
EFI_STATUS
EFIAPI
ArmPciConfigInfoParser (
  IN  CONST FDT_HW_INFO_PARSER_HANDLE  FdtParserHandle,
  IN        INT32                      FdtBranch
  )
{
  EFI_STATUS        Status;
  UINT32            Index;
  INT32             PciNode;
  UINT32            PciNodeCount;
  PCI_PARSER_TABLE  PciTableInfo;
  VOID              *Fdt;

  if (FdtParserHandle == NULL) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  Fdt = FdtParserHandle->Fdt;

  // Only search host-pci devices.
  // PCI Firmware Specification Revision 3.0, s4.1.2. "MCFG Table Description":
  // "This table directly refers to PCI Segment Groups defined in the system
  // via the _SEG object in the ACPI name space for the applicable host bridge
  // device."
  Status = FdtCountCompatNodeInBranch (
             Fdt,
             FdtBranch,
             &PciCompatibleInfo,
             &PciNodeCount
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  if (PciNodeCount == 0) {
    return EFI_NOT_FOUND;
  }

  // Parse each host-pci node in the branch.
  PciNode = FdtBranch;
  for (Index = 0; Index < PciNodeCount; Index++) {
    ZeroMem (&PciTableInfo, sizeof (PCI_PARSER_TABLE));

    Status = FdtGetNextCompatNodeInBranch (
               Fdt,
               FdtBranch,
               &PciCompatibleInfo,
               &PciNode
               );
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      if (Status == EFI_NOT_FOUND) {
        // Should have found the node.
        Status = EFI_ABORTED;
      }

      return Status;
    }

    Status = PciNodeParser (Fdt, PciNode, &PciTableInfo);
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      goto error_handler;
    }

    // Add Pci information to the Configuration Manager.
    Status = PciInfoAdd (FdtParserHandle, &PciTableInfo);
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      goto error_handler;
    }

    Status = FreeParserTable (&PciTableInfo);
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      return Status;
    }
  } // for

  return Status;

error_handler:
  FreeParserTable (&PciTableInfo);
  return Status;
}
