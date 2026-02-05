/** @file
  Unified RISC-V IOMMU discovery framework.

  Copyright (c) 2025, 9elements GmbH. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/FdtLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Protocol/PciEnumerationComplete.h>
#include <Protocol/PciIo.h>
#include <Guid/Fdt.h>
#include <Guid/PlatformHasAcpi.h>
#include <Guid/PlatformHasDeviceTree.h>
#include <IndustryStandard/Pci.h>
#include <IndustryStandard/RiscVIoMappingTable.h>
#include "RiscVIoMmu.h"

STATIC RISCV_IOMMU_CONTEXT  RiscVIoMmuContextTemplate = {
  .Signature = RISCV_IOMMU_CONTEXT_SIGNATURE,
};

#define EFI_ACPI_RISCV_IO_MAPPING_TABLE_SIGNATURE  SIGNATURE_32('R', 'I', 'M', 'T')

/**
  PciEnumerationComplete Protocol notification event handler.

  @param[in] Event    Event whose notification function is being invoked.
  @param[in] Context  Pointer to the notification function's context.
**/
STATIC
VOID
EFIAPI
OnPciEnumerationComplete (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  VOID                               *Interface;
  EFI_STATUS                         Status;
  UINTN                              HandleCount;
  EFI_HANDLE                         *HandleBuffer;
  UINTN                              Index;
  EFI_PCI_IO_PROTOCOL                *PciIo;
  UINTN                              Seg;
  UINTN                              Bus;
  UINTN                              Dev;
  UINTN                              Func;
  LIST_ENTRY                         *Link;
  RISCV_IOMMU_CONTEXT                *IoMmuContext;
  UINT16                             Data;
  EFI_ACPI_ADDRESS_SPACE_DESCRIPTOR  *Descriptor;

  //
  // Try to locate it because gEfiPciEnumerationCompleteProtocolGuid will trigger it once when registration.
  // Just return if it is not found.
  //
  Status = gBS->LocateProtocol (&gEfiPciEnumerationCompleteProtocolGuid, NULL, &Interface);
  if (EFI_ERROR (Status)) {
    return;
  }

  //
  // Iterate all PCI devices, looking for IOMMUs found in the FDT/RIMT.
  //
  Status = gBS->LocateHandleBuffer (ByProtocol, &gEfiPciIoProtocolGuid, NULL, &HandleCount, &HandleBuffer);
  ASSERT_EFI_ERROR (Status);

  // TODO: Avoid continuing to iterate once all IOMMUs are initialised as a performance optimisation.
  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->HandleProtocol (HandleBuffer[Index], &gEfiPciIoProtocolGuid, (VOID **)&PciIo);
    ASSERT_EFI_ERROR (Status);

    //
    // Read the basics of the PCI location.
    //
    Status = PciIo->GetLocation (PciIo, &Seg, &Bus, &Dev, &Func);
    ASSERT_EFI_ERROR (Status);

    for (Link = GetFirstNode (&mRiscVIoMmuContexts)
       ; !IsNull (&mRiscVIoMmuContexts, Link)
       ; Link = GetNextNode (&mRiscVIoMmuContexts, Link)
       ) {
      IoMmuContext = RISCV_IOMMU_CONTEXT_FROM_LINK (Link);
      if (!IoMmuContext->IoMmuIsPciDevice) {
        continue;
      }
      if (((IoMmuContext->BaseAddress & 0xFF0000) == Bus << 16) && ((IoMmuContext->BaseAddress & 0x00F800) == Dev << 11) && ((IoMmuContext->BaseAddress & 0x000700) == Func << 8)) {
        break;
      }
    }

    // This PCI device isn't an IOMMU.
    if (IsNull (&mRiscVIoMmuContexts, Link)) {
      continue;
    }

    IoMmuContext->DriverState = STATE_AVAILABLE;

    // Also enable DMA to satisfy MSIs, etc.
    Data   = EFI_PCI_COMMAND_BUS_MASTER | EFI_PCI_COMMAND_MEMORY_SPACE;
    Status = PciIo->Pci.Write (PciIo, EfiPciIoWidthUint16, PCI_COMMAND_OFFSET, 1, (VOID *)&Data);
    ASSERT_EFI_ERROR (Status);

    Status = PciIo->GetBarAttributes (PciIo, 0, NULL, (VOID **)&Descriptor);
    ASSERT ((Status == EFI_SUCCESS) && (Descriptor->ResType == ACPI_ADDRESS_SPACE_TYPE_MEM));

    IoMmuContext->BaseAddress = Descriptor->AddrRangeMin;
    ASSERT (Descriptor->AddrLen == SIZE_4KB);
    FreePool (Descriptor);

    Status = IoMmuCommonInitialise (IoMmuContext);
    ASSERT_EFI_ERROR (Status);
  }

  FreePool (HandleBuffer);

  gBS->CloseEvent (Event);
}

/**
  Search the devicetree for an IOMMU.

  @retval  EFI_SUCCESS    At least one IOMMU was detected.
  @retval  EFI_NOT_FOUND  No IOMMUs were detected.

**/
STATIC
EFI_STATUS
IoMmuDeviceTreeDiscovery (
  VOID
  )
{
  VOID                     *Fdt;
  EFI_STATUS               Status;
  INT32                    IoMmuNode;
  INT32                    TempLen;
  CONST VOID               *PropertyPtr;
  RISCV_IOMMU_CONTEXT      *IoMmuContext;
  UINT32                   *Data32;
  UINT64                   *Data64;
  UINT64                   StartAddress;
  UINT64                   NumberOfBytes;
  VOID                     *Registration;
  EFI_EVENT                ProtocolNotifyEvent;
  INT32                    DownstreamDeviceNode;
  UINT32                   IoMmuMapMask;
  UINTN                    Index;
  UINT32                   MapLength;
  UINT32                   MapPhandle;
  LIST_ENTRY               *Link;
  RISCV_IOMMU_DOWNSTREAMS  *IoMmuDownstreams;

  Status = EfiGetSystemConfigurationTable (&gFdtTableGuid, &Fdt);
  ASSERT_EFI_ERROR (Status);

  //
  // Search for all nodes that are RISC-V IOMMU-compatible, getting the address/BDF of each,
  // registering a callback to get the MMIO address of PCI IOMMUs after enumeration (if needed).
  //
  ProtocolNotifyEvent = NULL;

  IoMmuNode = FdtNextNode (Fdt, -1, NULL);
  while (IoMmuNode != -FDT_ERR_NOTFOUND) {
    PropertyPtr = FdtGetProp (Fdt, IoMmuNode, "compatible", &TempLen);
    if (PropertyPtr == NULL) {
      goto NextIoMmuNode;
    }

    if (!FdtStringListContains (PropertyPtr, TempLen, "riscv,iommu") && !FdtStringListContains (PropertyPtr, TempLen, "riscv,pci-iommu")) {
      goto NextIoMmuNode;
    }

    IoMmuContext = AllocateCopyPool (sizeof (RISCV_IOMMU_CONTEXT), &RiscVIoMmuContextTemplate);
    ASSERT (IoMmuContext != NULL);
    InitializeListHead (&IoMmuContext->DownstreamDevices);
    InsertTailList (&mRiscVIoMmuContexts, &IoMmuContext->Link);

    if (FdtStringListContains (PropertyPtr, TempLen, "riscv,iommu")) {
      // TODO: `TempLen` is great enough. Effectively, an #address-cells and #size-cells check.
      Data64 = (UINT64 *)FdtGetProp (Fdt, IoMmuNode, "reg", &TempLen);
      ASSERT (Data64 != NULL);

      StartAddress  = Fdt64ToCpu (ReadUnaligned64 (Data64));
      NumberOfBytes = Fdt64ToCpu (ReadUnaligned64 (Data64 + 1));

      IoMmuContext->DriverState      = STATE_AVAILABLE;
      IoMmuContext->IoMmuIsPciDevice = FALSE;
      IoMmuContext->BaseAddress      = StartAddress;
      ASSERT (NumberOfBytes == SIZE_4KB);
    } else if (FdtStringListContains (PropertyPtr, TempLen, "riscv,pci-iommu")) {
      // TODO: `TempLen` is great enough. Effectively, an #address-cells and #size-cells check.
      Data32 = (UINT32 *)FdtGetProp (Fdt, IoMmuNode, "reg", &TempLen);
      ASSERT (Data32 != NULL);

      StartAddress = Fdt32ToCpu (ReadUnaligned32 (Data32));

      IoMmuContext->DriverState      = STATE_DETECTED;
      IoMmuContext->IoMmuIsPciDevice = TRUE;
      IoMmuContext->BaseAddress      = StartAddress;

      //
      // The FDT merely provides the BDF (and device references to the IOMMU), now scan for the device.
      //
      if (ProtocolNotifyEvent == NULL) {
        ProtocolNotifyEvent = EfiCreateProtocolNotifyEvent (
                                &gEfiPciEnumerationCompleteProtocolGuid,
                                TPL_CALLBACK,
                                OnPciEnumerationComplete,
                                NULL,
                                &Registration
                                );
        ASSERT (ProtocolNotifyEvent != NULL);
      }
    }

    // TODO: `TempLen` is great enough.
    Data32 = (UINT32 *)FdtGetProp (Fdt, IoMmuNode, "phandle", &TempLen);
    ASSERT (Data32 != NULL);

    IoMmuContext->TempHandle = Fdt32ToCpu (ReadUnaligned32 (Data32));

NextIoMmuNode:
    IoMmuNode = FdtNextNode (Fdt, IoMmuNode, NULL);
  }

  if (IsListEmpty (&mRiscVIoMmuContexts)) {
    return EFI_NOT_FOUND;
  }

  //
  // Now, gather all the downstreams. Perform this separately to avoid excessive nesting.
  // - TODO: Look for platform devices.
  //
  DownstreamDeviceNode = FdtNextNode (Fdt, -1, NULL);
  while (DownstreamDeviceNode != -FDT_ERR_NOTFOUND) {
    PropertyPtr = FdtGetProp (Fdt, DownstreamDeviceNode, "compatible", &TempLen);
    if (PropertyPtr == NULL) {
      goto NextDownstreamDeviceNode;
    }

    if (!FdtStringListContains (PropertyPtr, TempLen, "pci-host-ecam-generic") && !FdtStringListContains (PropertyPtr, TempLen, "pci-host-cam-generic")) {
      goto NextDownstreamDeviceNode;
    }

    IoMmuMapMask = -1;
    Data32 = (UINT32 *)FdtGetProp (Fdt, DownstreamDeviceNode, "iommu-map-mask", &TempLen);
    if (Data32 != NULL) {
      IoMmuMapMask = Fdt32ToCpu (ReadUnaligned32 (Data32));
    }

    Data32 = (UINT32 *)FdtGetProp (Fdt, DownstreamDeviceNode, "iommu-map", &TempLen);
    ASSERT ((Data32 != NULL) && (TempLen % (sizeof (UINT32) * 4) == 0));

    // Each entry is <RID_BASE PHANDLE IOMMU_BASE LENGTH>
    for (Index = 0; Index < TempLen / 4; Index += 4) {
      MapLength = Fdt32ToCpu (ReadUnaligned32 (Data32 + Index + 3));
      if (MapLength == 0) {
        continue;
      }

      MapPhandle = Fdt32ToCpu (ReadUnaligned32 (Data32 + Index + 1));
      for (Link = GetFirstNode (&mRiscVIoMmuContexts)
         ; !IsNull (&mRiscVIoMmuContexts, Link)
         ; Link = GetNextNode (&mRiscVIoMmuContexts, Link)
         ) {
        IoMmuContext = RISCV_IOMMU_CONTEXT_FROM_LINK (Link);
        if (MapPhandle == IoMmuContext->TempHandle) {
          break;
        }
      }

      ASSERT (!IsNull (&mRiscVIoMmuContexts, Link));

      IoMmuDownstreams = AllocatePool (sizeof (RISCV_IOMMU_DOWNSTREAMS));
      ASSERT (IoMmuDownstreams != NULL);
      InsertTailList (&IoMmuContext->DownstreamDevices, &IoMmuDownstreams->Link);

      IoMmuDownstreams->Signature = RISCV_IOMMU_DOWNSTREAMS_SIGNATURE;
      IoMmuDownstreams->MapMask   = IoMmuMapMask;
      IoMmuDownstreams->NodeMapping.SourceIdBase            = Fdt32ToCpu (ReadUnaligned32 (Data32 + Index));
      IoMmuDownstreams->NodeMapping.NumberOfIDs             = MapLength;
      IoMmuDownstreams->NodeMapping.DestinationDeviceIdBase = Fdt32ToCpu (ReadUnaligned32 (Data32 + Index + 2));
      //IoMmuDownstreams->NodeMapping.DestinationIommuOffset  = MapPhandle;
      IoMmuDownstreams->NodeMapping.Flags                   = 0;
    }

NextDownstreamDeviceNode:
    DownstreamDeviceNode = FdtNextNode (Fdt, DownstreamDeviceNode, NULL);
  }

  return EFI_SUCCESS;
}

/**
  Search ACPI's RIMT for an IOMMU.

  @retval  EFI_SUCCESS    At least one IOMMU was detected.
  @retval  EFI_NOT_FOUND  No IOMMUs were detected.

**/
STATIC
EFI_STATUS
IoMmuAcpiRimtDiscovery (
  VOID
  )
{
  EFI_ACPI_6_6_RIMT_STRUCTURE                         *AcpiRimtTable;
  EFI_ACPI_6_6_RIMT_NODE_HEADER_STRUCTURE             *RimtNodeHeader;
  UINTN                                               NodeIndex;
  RISCV_IOMMU_CONTEXT                                 *IoMmuContext;
  EFI_ACPI_6_6_RIMT_IOMMU_NODE_STRUCTURE              *RimtIoMmuNode;
  EFI_ACPI_6_6_RIMT_PCIE_ROOT_COMPLEX_NODE_STRUCTURE  *RimtPcieNode;
  UINT16                                              NumberOfIdMappings;
  EFI_ACPI_6_6_RIMT_ID_MAPPING_STRUCTURE              *NodeMapping;
  //EFI_ACPI_6_6_RIMT_PLATFORM_DEVICE_NODE_STRUCTURE    *RimtPlatformNode;
  UINTN                                               MappingIndex;
  LIST_ENTRY                                          *Link;
  RISCV_IOMMU_DOWNSTREAMS                             *IoMmuDownstreams;

  AcpiRimtTable = (VOID *)EfiLocateFirstAcpiTable (EFI_ACPI_RISCV_IO_MAPPING_TABLE_SIGNATURE);
  if (AcpiRimtTable == NULL) {
    return EFI_NOT_FOUND;
  }

  //
  // Gather all the IOMMUs.
  //
  RimtNodeHeader = (VOID *)((UINT8 *)AcpiRimtTable + AcpiRimtTable->OffsetToRimtNodeArray);
  for (NodeIndex = 0; NodeIndex < AcpiRimtTable->NumberOfRimtNodes; NodeIndex++) {
    if (RimtNodeHeader->Type == RimtNodeIommu) {
      IoMmuContext = AllocateCopyPool (sizeof (RISCV_IOMMU_CONTEXT), &RiscVIoMmuContextTemplate);
      ASSERT (IoMmuContext != NULL);
      InitializeListHead (&IoMmuContext->DownstreamDevices);
      InsertTailList (&mRiscVIoMmuContexts, &IoMmuContext->Link);

      IoMmuContext->TempHandle = (UINT8 *)RimtNodeHeader - (UINT8 *)AcpiRimtTable;

      RimtIoMmuNode = (VOID *)RimtNodeHeader;
      if (RimtIoMmuNode->Flags & (1 << RIMT_IOMMU_FLAGS_TYPE_BIT_OFFSET)) {
        IoMmuContext->DriverState      = STATE_DETECTED;
        IoMmuContext->IoMmuIsPciDevice = TRUE;
        IoMmuContext->BaseAddress      = RimtIoMmuNode->PcieBdf;
      } else {
        IoMmuContext->DriverState      = STATE_AVAILABLE;
        IoMmuContext->IoMmuIsPciDevice = FALSE;
        IoMmuContext->BaseAddress      = RimtIoMmuNode->BaseAddress;
      }
    }

    RimtNodeHeader = (VOID *)((UINT8 *)RimtNodeHeader + RimtNodeHeader->Length);
  }

  if (IsListEmpty (&mRiscVIoMmuContexts)) {
    return EFI_NOT_FOUND;
  }

  //
  // Now, gather all the downstreams. Perform this separately to avoid excessive nesting.
  //
  RimtNodeHeader = (VOID *)((UINT8 *)AcpiRimtTable + AcpiRimtTable->OffsetToRimtNodeArray);
  for (NodeIndex = 0; NodeIndex < AcpiRimtTable->NumberOfRimtNodes; NodeIndex++) {
    if ((RimtNodeHeader->Type != RimtNodePcieRc) && (RimtNodeHeader->Type != RimtNodePlatform)) {
      goto NodeEnd;
    }

    if (RimtNodeHeader->Type == RimtNodePcieRc) {
      RimtPcieNode       = (VOID *)RimtNodeHeader;
      NumberOfIdMappings = RimtPcieNode->NumberOfIdMappings;
      NodeMapping        = (VOID *)((UINT8 *)RimtPcieNode + RimtPcieNode->IdMappingArrayOffset);
    } else {
      // TODO: Look for platform devices properly.
      goto NodeEnd;
#if 0
      RimtPlatformNode   = (VOID *)RimtNodeHeader;
      NumberOfIdMappings = RimtPlatformNode->NumberOfIdMappings;
      NodeMapping        = (VOID *)((UINT8 *)RimtPlatformNode + RimtPlatformNode->IdMappingArrayOffset);
#endif
    }

    for (MappingIndex = 0; MappingIndex < NumberOfIdMappings; MappingIndex++) {
      for (Link = GetFirstNode (&mRiscVIoMmuContexts)
         ; !IsNull (&mRiscVIoMmuContexts, Link)
         ; Link = GetNextNode (&mRiscVIoMmuContexts, Link)
         ) {
        IoMmuContext = RISCV_IOMMU_CONTEXT_FROM_LINK (Link);
        if (NodeMapping->DestinationIommuOffset == IoMmuContext->TempHandle) {
          break;
        }
      }

      ASSERT (!IsNull (&mRiscVIoMmuContexts, Link));

      IoMmuDownstreams = AllocatePool (sizeof (RISCV_IOMMU_DOWNSTREAMS));
      ASSERT (IoMmuDownstreams != NULL);
      InsertTailList (&IoMmuContext->DownstreamDevices, &IoMmuDownstreams->Link);

      IoMmuDownstreams->Signature = RISCV_IOMMU_DOWNSTREAMS_SIGNATURE;
      IoMmuDownstreams->MapMask   = -1;
      CopyMem (&IoMmuDownstreams->NodeMapping, NodeMapping, sizeof (EFI_ACPI_6_6_RIMT_ID_MAPPING_STRUCTURE));

      NodeMapping++;
    }

NodeEnd:
    RimtNodeHeader = (VOID *)((UINT8 *)RimtNodeHeader + RimtNodeHeader->Length);
  }

  return EFI_SUCCESS;
}

/**
  Detect a RISC-V IOMMU device.

  @retval  EFI_SUCCESS    An IOMMU was detected.
  @retval  EFI_NOT_FOUND  No IOMMU could be detected.

**/
VOID
DetectRiscVIoMmus (
  VOID
  )
{
  EFI_STATUS  Status;
  VOID        *Registration;

  //
  // Search the devicetree for an IOMMU.
  //
  Status = gBS->LocateProtocol (&gEdkiiPlatformHasDeviceTreeGuid, NULL, (VOID **)&Registration);
  if (!EFI_ERROR (Status)) {
    Status = IoMmuDeviceTreeDiscovery ();
    if (!EFI_ERROR (Status)) {
      return;
    }
  }

  //
  // Search ACPI's RIMT for an IOMMU.
  //
  Status = gBS->LocateProtocol (&gEdkiiPlatformHasAcpiGuid, NULL, (VOID **)&Registration);
  if (!EFI_ERROR (Status)) {
    Status = IoMmuAcpiRimtDiscovery ();
    if (!EFI_ERROR (Status)) {
      return;
    }
  }
}
