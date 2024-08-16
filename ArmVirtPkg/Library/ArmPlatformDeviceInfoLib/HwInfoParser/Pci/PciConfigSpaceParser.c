/** @file
  PCI Configuration Space Parser.

  Copyright (c) 2021 - 2024, ARM Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - linux/Documentation/devicetree/bindings/pci/host-generic-pci.yaml
  - PCI Firmware Specification - Revision 3.0
  - Open Firmware Recommended Practice: Interrupt Mapping, Version 0.9
  - Devicetree Specification Release v0.3
  - linux kernel code
**/

#include "FdtUtility.h"
#include "FdtInfoParser.h"
#include "PciConfigSpaceParser.h"

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

/** PCI configuration space attributes.
*/
STATIC CONST CHAR8  *PciConfigSpaceAttributesInfo[] = {
  "PCI Cfg Space",
  "PCI I/O Space",
  "PCI 32bit Mem",
  "PCI 64bit Mem"
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
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  Data = fdt_getprop (Fdt, HostPciNode, "linux,pci-domain", &DataSize);
  if ((Data == NULL) || (DataSize < 0)) {
    // Did not find property, assign the DomainIds ourselves.
    if (LocalSegGroup < 0) {
      // "linux,pci-domain" property was defined for another node.
      ASSERT (FALSE);
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
    ASSERT (FALSE);
    return EFI_ABORTED;
  }

  // If one node has the "linux,pci-domain" property, then all the host-pci
  // nodes must have it.
  LocalSegGroup = -1;

  *SegGroup = FdtReadUnaligned32 ((UINT32 *)Data);
  return EFI_SUCCESS;
}

/** Parse the bus-range controlled by this host-pci node.

  @param [in]     Fdt             Pointer to a Flattened Device Tree (Fdt).
  @param [in]     HostPciNode     Offset of a host-pci node.
  @param [out]    StartBusNumber  Start bus number
  @param [out]    EndBusNumber    End bus number

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
STATIC
EFI_STATUS
EFIAPI
PopulateBusRange (
  IN      CONST VOID   *Fdt,
  IN            INT32  HostPciNode,
  OUT           UINT8  *StartBusNumber,
  OUT           UINT8  *EndBusNumber
  )
{
  CONST UINT8  *Data;
  INT32        DataSize;
  UINT32       StartBus;
  UINT32       EndBus;

  if ((Fdt == NULL) ||
      (StartBusNumber == NULL) ||
      (EndBusNumber == NULL))
  {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  Data = fdt_getprop (Fdt, HostPciNode, "bus-range", &DataSize);
  if ((Data == NULL) || (DataSize < 0)) {
    // No evidence this property is mandatory. Use default values.
    StartBus = 0;
    EndBus   = 255;
  } else if (DataSize == (2 * sizeof (UINT32))) {
    // If available, the property is on two integers.
    StartBus = FdtReadUnaligned32 ((UINT32 *)Data);
    Data    += sizeof (UINT32);
    EndBus   = FdtReadUnaligned32 ((UINT32 *)Data);
  } else {
    ASSERT (FALSE);
    return EFI_ABORTED;
  }

  *StartBusNumber = StartBus;
  *EndBusNumber   = EndBus;

  return EFI_SUCCESS;
}

/** Parse the PCI address map.

  The PCI address map is available in the "ranges" device-tree property.

  @param [in]   FdtParserHandle   A handle to the parser instance.
  @param [in]   HostPciNode       Offset of a host-pci node.
  @param [in]   AddressCells      # of cells used to encode an address on
                                  the parent bus.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
STATIC
EFI_STATUS
EFIAPI
ParseAddressMap (
  IN  CONST   FDT_HW_INFO_PARSER_HANDLE  FdtParserHandle,
  IN          INT32                      HostPciNode,
  IN          INT32                      AddressCells
  )
{
  EFI_STATUS   Status;
  CONST UINT8  *Data;
  INT32        DataSize;
  UINT32       Index;
  UINT32       Offset;
  UINT32       AddressMapSize;
  UINT32       Count;
  UINT32       PciAddressAttr;
  UINT8        SpaceCode;
  UINT64       PciAddress;
  UINT64       CpuAddress;
  UINT64       AddressSize;
  VOID         *Fdt;

  if (FdtParserHandle == NULL) {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  Fdt = FdtParserHandle->Fdt;

  // The mapping is done on AddressMapSize bytes.
  AddressMapSize = (PCI_ADDRESS_CELLS + AddressCells + PCI_SIZE_CELLS) *
                   sizeof (UINT32);

  Data = fdt_getprop (Fdt, HostPciNode, "ranges", &DataSize);
  if ((Data == NULL) ||
      (DataSize < 0) ||
      ((DataSize % AddressMapSize) != 0))
  {
    // If error or not on AddressMapSize bytes.
    ASSERT (FALSE);
    return EFI_ABORTED;
  }

  Count  = DataSize / AddressMapSize;
  Status = EFI_SUCCESS;
  for (Index = 0; Index < Count; Index++) {
    Offset = Index * AddressMapSize;

    // Pci address attributes
    PciAddressAttr = FdtReadUnaligned32 ((UINT32 *)&Data[Offset]);
    SpaceCode      = READ_PCI_SS (PciAddressAttr);
    Offset        += sizeof (UINT32);

    // Pci address
    PciAddress = FdtReadUnaligned64 ((UINT64 *)&Data[Offset]);
    Offset    += (PCI_ADDRESS_CELLS - 1) * sizeof (UINT32);

    // Cpu address
    if (AddressCells == 2) {
      CpuAddress = FdtReadUnaligned64 ((UINT64 *)&Data[Offset]);
    } else {
      CpuAddress = FdtReadUnaligned32 ((UINT32 *)&Data[Offset]);
    }

    Offset += AddressCells * sizeof (UINT32);

    // Address size
    AddressSize = FdtReadUnaligned64 ((UINT64 *)&Data[Offset]);
    Offset     += PCI_SIZE_CELLS * sizeof (UINT32);

    if (FdtParserHandle->HwAddressInfo != NULL) {
      Status = FdtParserHandle->HwAddressInfo (
                                  FdtParserHandle->Context,
                                  PciConfigSpaceAttributesInfo[SpaceCode],
                                  CpuAddress,
                                  AddressSize
                                  );
      if (EFI_ERROR (Status)) {
        ASSERT (FALSE);
        break;
      }
    }
  } // for

  return Status;
}

/** Parse a Host-pci node.

  @param [in]   FdtParserHandle   A handle to the parser instance.
  @param [in]   HostPciNode       Offset of a host-pci node.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
STATIC
EFI_STATUS
EFIAPI
PciNodeParser (
  IN  CONST FDT_HW_INFO_PARSER_HANDLE  FdtParserHandle,
  IN            INT32                  HostPciNode
  )
{
  EFI_STATUS   Status;
  INT32        AddressCells;
  INT32        SizeCells;
  CONST UINT8  *Data;
  CONST UINT8  *Range;
  INT32        DataSize;
  INT32        SegGroup;
  UINT8        StartBusNumber;
  UINT8        EndBusNumber;
  UINT64       ConfigSpaceBaseAddress;
  UINT64       ConfigSpaceBaseAddressRange;
  VOID         *Fdt;

  if (FdtParserHandle == NULL) {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  Fdt = FdtParserHandle->Fdt;

  // Segment Group / DomainId
  Status = GetPciSegGroup (Fdt, HostPciNode, &SegGroup);
  if (EFI_ERROR (Status)) {
    ASSERT (FALSE);
    return Status;
  }

  // Bus range
  Status = PopulateBusRange (
             Fdt,
             HostPciNode,
             &StartBusNumber,
             &EndBusNumber
             );
  if (EFI_ERROR (Status)) {
    ASSERT (FALSE);
    return Status;
  }

  Status = FdtGetParentAddressInfo (
             Fdt,
             HostPciNode,
             &AddressCells,
             &SizeCells
             );
  if (EFI_ERROR (Status)) {
    ASSERT (FALSE);
    return Status;
  }

  // Only support 32/64 bits addresses.
  if ((AddressCells < 1)  ||
      (AddressCells > 2)  ||
      (SizeCells < 1)     ||
      (SizeCells > 2))
  {
    ASSERT (FALSE);
    return EFI_ABORTED;
  }

  Data = fdt_getprop (Fdt, HostPciNode, "reg", &DataSize);
  if ((Data == NULL) ||
      (DataSize != ((AddressCells + SizeCells) * sizeof (UINT32))))
  {
    // If error or wrong size.
    ASSERT (FALSE);
    return EFI_ABORTED;
  }

  Range = Data + (sizeof (UINT32) * AddressCells);

  // Base address
  if (AddressCells == 2) {
    ConfigSpaceBaseAddress      = FdtReadUnaligned64 ((UINT64 *)Data);
    ConfigSpaceBaseAddressRange = FdtReadUnaligned64 ((UINT64 *)Range);
  } else {
    ConfigSpaceBaseAddress      = FdtReadUnaligned32 ((UINT32 *)Data);
    ConfigSpaceBaseAddressRange = FdtReadUnaligned32 ((UINT32 *)Range);
  }

  if (FdtParserHandle->HwAddressInfo != NULL) {
    Status = FdtParserHandle->HwAddressInfo (
                                FdtParserHandle->Context,
                                "PCI Config",
                                ConfigSpaceBaseAddress,
                                ConfigSpaceBaseAddressRange
                                );
    if (EFI_ERROR (Status)) {
      ASSERT (FALSE);
      return Status;
    }
  }

  // Address map
  Status = ParseAddressMap (
             FdtParserHandle,
             HostPciNode,
             AddressCells
             );
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/** PCI information parser function.

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
PciConfigInfoParser (
  IN  CONST FDT_HW_INFO_PARSER_HANDLE  FdtParserHandle,
  IN        INT32                      FdtBranch
  )
{
  EFI_STATUS  Status;
  UINT32      Index;
  INT32       PciNode;
  UINT32      PciNodeCount;
  VOID        *Fdt;

  if (FdtParserHandle == NULL) {
    ASSERT (FALSE);
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
    ASSERT (FALSE);
    return Status;
  }

  if (PciNodeCount == 0) {
    return EFI_NOT_FOUND;
  }

  // Parse each host-pci node in the branch.
  PciNode = FdtBranch;
  for (Index = 0; Index < PciNodeCount; Index++) {
    Status = FdtGetNextCompatNodeInBranch (
               Fdt,
               FdtBranch,
               &PciCompatibleInfo,
               &PciNode
               );
    if (EFI_ERROR (Status)) {
      ASSERT (FALSE);
      if (Status == EFI_NOT_FOUND) {
        // Should have found the node.
        Status = EFI_ABORTED;
      }

      return Status;
    }

    Status = PciNodeParser (FdtParserHandle, PciNode);
    if (EFI_ERROR (Status)) {
      ASSERT (FALSE);
      break;
    }
  } // for

  return Status;
}
