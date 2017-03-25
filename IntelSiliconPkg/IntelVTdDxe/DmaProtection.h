/** @file

  Copyright (c) 2017, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _DMAR_PROTECTION_H_
#define _DMAR_PROTECTION_H_

#include <Uefi.h>
#include <PiDxe.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/IoLib.h>
#include <Library/PciSegmentLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiLib.h>

#include <Guid/EventGroup.h>
#include <Guid/Acpi.h>

#include <Protocol/DxeSmmReadyToLock.h>
#include <Protocol/PciRootBridgeIo.h>
#include <Protocol/PciIo.h>
#include <Protocol/PciEnumerationComplete.h>
#include <Protocol/AcpiSystemDescriptionTable.h>
#include <Protocol/PlatformVtdPolicy.h>
#include <Protocol/IoMmu.h>

#include <IndustryStandard/Pci.h>
#include <IndustryStandard/DmaRemappingReportingTable.h>
#include <IndustryStandard/Vtd.h>

#define ALIGN_VALUE_UP(Value, Alignment)  (((Value) + (Alignment) - 1) & (~((Alignment) - 1)))
#define ALIGN_VALUE_LOW(Value, Alignment) ((Value) & (~((Alignment) - 1)))

//
// This is the initial max PCI descriptor.
// The number may be enlarged later.
//
#define MAX_PCI_DESCRIPTORS             0x100

typedef struct {
  BOOLEAN                IncludeAllFlag;
  UINTN                  PciDescriptorNumber;
  UINTN                  PciDescriptorMaxNumber;
  BOOLEAN                *IsRealPciDevice;
  VTD_SOURCE_ID          *PciDescriptors;
} PCI_DEVICE_INFORMATION;

typedef struct {
  UINTN                            VtdUnitBaseAddress;
  UINT16                           Segment;
  VTD_CAP_REG                      CapReg;
  VTD_ECAP_REG                     ECapReg;
  VTD_ROOT_ENTRY                   *RootEntryTable;
  VTD_EXT_ROOT_ENTRY               *ExtRootEntryTable;
  VTD_SECOND_LEVEL_PAGING_ENTRY    *FixedSecondLevelPagingEntry;
  BOOLEAN                          HasDirtyPages;
  PCI_DEVICE_INFORMATION           PciDeviceInfo;
} VTD_UNIT_INFORMATION;

extern EFI_ACPI_DMAR_HEADER  *mAcpiDmarTable;

extern UINT64                           mVtdHostAddressWidthMask;
extern UINTN                            mVtdUnitNumber;
extern VTD_UNIT_INFORMATION             *mVtdUnitInformation;

extern UINT64                           mBelow4GMemoryLimit;
extern UINT64                           mAbove4GMemoryLimit;

extern EDKII_PLATFORM_VTD_POLICY_PROTOCOL   *mPlatformVTdPolicy;

/**
  Prepare VTD configuration.
**/
VOID
PrepareVtdConfig (
  VOID
  );

/**
  Setup VTd translation table.

  @retval EFI_SUCCESS           Setup translation table successfully.
  @retval EFI_OUT_OF_RESOURCE   Setup translation table fail.
**/
EFI_STATUS
SetupTranslationTable (
  VOID
  );

/**
  Enable DMAR translation.

  @retval EFI_SUCCESS           DMAR translation is enabled.
  @retval EFI_DEVICE_ERROR      DMAR translation is not enabled.
**/
EFI_STATUS
EnableDmar (
  VOID
  );

/**
  Disable DMAR translation.

  @retval EFI_SUCCESS           DMAR translation is disabled.
  @retval EFI_DEVICE_ERROR      DMAR translation is not disabled.
**/
EFI_STATUS
DisableDmar (
  VOID
  );

/**
  Invalid VTd IOTLB page.

  @param[in]  VtdIndex              The index of VTd engine.
  @param[in]  Address               The address of IOTLB page.
  @param[in]  AddressMode           The address mode of IOTLB page.
  @param[in]  DomainIdentifier      The domain ID of the source.

  @retval EFI_SUCCESS           VTd IOTLB page is invalidated.
  @retval EFI_DEVICE_ERROR      VTd IOTLB page is not invalidated.
**/
EFI_STATUS
InvalidateVtdIOTLBPage (
  IN UINTN  VtdIndex,
  IN UINT64 Address,
  IN UINT8  AddressMode,
  IN UINT16 DomainIdentifier
  );

/**
  Invalid VTd IOTLB domain.

  @param[in]  VtdIndex              The index of VTd engine.
  @param[in]  DomainIdentifier      The domain ID of the source.

  @retval EFI_SUCCESS           VTd IOTLB domain is invalidated.
  @retval EFI_DEVICE_ERROR      VTd IOTLB domain is not invalidated.
**/
EFI_STATUS
InvalidateVtdIOTLBDomain (
  IN UINTN  VtdIndex,
  IN UINT16 DomainIdentifier
  );

/**
  Invalid VTd global IOTLB.

  @param[in]  VtdIndex              The index of VTd engine.

  @retval EFI_SUCCESS           VTd global IOTLB is invalidated.
  @retval EFI_DEVICE_ERROR      VTd global IOTLB is not invalidated.
**/
EFI_STATUS
InvalidateVtdIOTLBGlobal (
  IN UINTN  VtdIndex
  );

/**
  Dump VTd registers.

  @param[in]  VtdIndex              The index of VTd engine.
**/
VOID
DumpVtdRegs (
  IN UINTN  VtdIndex
  );

/**
  Dump VTd registers for all VTd engine.
**/
VOID
DumpVtdRegsAll (
  VOID
  );

/**
  Dump VTd capability registers.

  @param[in]  CapReg              The capability register.
**/
VOID
DumpVtdCapRegs (
  IN VTD_CAP_REG *CapReg
  );

/**
  Dump VTd extended capability registers.

  @param[in]  ECapReg              The extended capability register.
**/
VOID
DumpVtdECapRegs (
  IN VTD_ECAP_REG *ECapReg
  );

/**
  Register PCI device to VTd engine as PCI descriptor.

  @param[in]  VtdIndex              The index of VTd engine.
  @param[in]  Segment               The segment of the source.
  @param[in]  SourceId              The SourceId of the source.
  @param[in]  IsRealPciDevice       TRUE: It is a real PCI device.
                                    FALSE: It is not a real PCI device.
  @param[in]  CheckExist            TRUE: ERROR will be returned if the PCI device is already registered.
                                    FALSE: SUCCESS will be returned if the PCI device is registered.

  @retval EFI_SUCCESS           The PCI device is registered.
  @retval EFI_OUT_OF_RESOURCES  No enough resource to register a new PCI device.
  @retval EFI_ALREADY_STARTED   The device is already registered.
**/
EFI_STATUS
RegisterPciDevice (
  IN UINTN          VtdIndex,
  IN UINT16         Segment,
  IN VTD_SOURCE_ID  SourceId,
  IN BOOLEAN        IsRealPciDevice,
  IN BOOLEAN        CheckExist
  );

/**
  Scan PCI bus and register PCI devices under the bus.

  @param[in]  VtdIndex              The index of VTd engine.
  @param[in]  Segment               The segment of the source.
  @param[in]  Bus                   The bus of the source.

  @retval EFI_SUCCESS           The PCI devices under the bus are registered.
  @retval EFI_OUT_OF_RESOURCES  No enough resource to register a new PCI device.
**/
EFI_STATUS
ScanPciBus (
  IN UINTN          VtdIndex,
  IN UINT16         Segment,
  IN UINT8          Bus
  );

/**
  Dump the PCI device information managed by this VTd engine.

  @param[in]  VtdIndex              The index of VTd engine.
**/
VOID
DumpPciDeviceInfo (
  IN UINTN  VtdIndex
  );

/**
  Find the VTd index by the Segment and SourceId.

  @param[in]  Segment               The segment of the source.
  @param[in]  SourceId              The SourceId of the source.
  @param[out] ExtContextEntry       The ExtContextEntry of the source.
  @param[out] ContextEntry          The ContextEntry of the source.

  @return The index of the PCI descriptor.
  @retval (UINTN)-1  The PCI descriptor is not found.
**/
UINTN
FindVtdIndexByPciDevice (
  IN  UINT16                  Segment,
  IN  VTD_SOURCE_ID           SourceId,
  OUT VTD_EXT_CONTEXT_ENTRY   **ExtContextEntry,
  OUT VTD_CONTEXT_ENTRY       **ContextEntry
  );

/**
  Get the DMAR ACPI table.

  @retval EFI_SUCCESS    The DMAR ACPI table is got.
  @retval EFI_NOT_FOUND  The DMAR ACPI table is not found.
**/
EFI_STATUS
GetDmarAcpiTable (
  VOID
  );

/**
  Parse DMAR DRHD table.

  @return EFI_SUCCESS  The DMAR DRHD table is parsed.
**/
EFI_STATUS
ParseDmarAcpiTableDrhd (
  VOID
  );

/**
  Parse DMAR RMRR table.

  @return EFI_SUCCESS  The DMAR RMRR table is parsed.
**/
EFI_STATUS
ParseDmarAcpiTableRmrr (
  VOID
  );

/**
  Dump DMAR context entry table.

  @param[in]  RootEntry DMAR root entry.
**/
VOID
DumpDmarContextEntryTable (
  IN VTD_ROOT_ENTRY *RootEntry
  );

/**
  Dump DMAR extended context entry table.

  @param[in]  ExtRootEntry DMAR extended root entry.
**/
VOID
DumpDmarExtContextEntryTable (
  IN VTD_EXT_ROOT_ENTRY *ExtRootEntry
  );

/**
  Dump DMAR second level paging entry.

  @param[in]  SecondLevelPagingEntry The second level paging entry.
**/
VOID
DumpSecondLevelPagingEntry (
  IN VOID *SecondLevelPagingEntry
  );

/**
  Set VTd attribute for a system memory.

  @param[in]  VtdIndex                The index used to identify a VTd engine.
  @param[in]  SecondLevelPagingEntry  The second level paging entry in VTd table for the device.
  @param[in]  BaseAddress             The base of device memory address to be used as the DMA memory.
  @param[in]  Length                  The length of device memory address to be used as the DMA memory.
  @param[in]  IoMmuAccess             The IOMMU access.

  @retval EFI_SUCCESS            The IoMmuAccess is set for the memory range specified by BaseAddress and Length.
  @retval EFI_INVALID_PARAMETER  BaseAddress is not IoMmu Page size aligned.
  @retval EFI_INVALID_PARAMETER  Length is not IoMmu Page size aligned.
  @retval EFI_INVALID_PARAMETER  Length is 0.
  @retval EFI_INVALID_PARAMETER  IoMmuAccess specified an illegal combination of access.
  @retval EFI_UNSUPPORTED        The bit mask of IoMmuAccess is not supported by the IOMMU.
  @retval EFI_UNSUPPORTED        The IOMMU does not support the memory range specified by BaseAddress and Length.
  @retval EFI_OUT_OF_RESOURCES   There are not enough resources available to modify the IOMMU access.
  @retval EFI_DEVICE_ERROR       The IOMMU device reported an error while attempting the operation.
**/
EFI_STATUS
SetPageAttribute (
  IN UINTN                         VtdIndex,
  IN VTD_SECOND_LEVEL_PAGING_ENTRY *SecondLevelPagingEntry,
  IN UINT64                        BaseAddress,
  IN UINT64                        Length,
  IN UINT64                        IoMmuAccess
  );

/**
  Set VTd attribute for a system memory.

  @param[in]  Segment           The Segment used to identify a VTd engine.
  @param[in]  SourceId          The SourceId used to identify a VTd engine and table entry.
  @param[in]  BaseAddress       The base of device memory address to be used as the DMA memory.
  @param[in]  Length            The length of device memory address to be used as the DMA memory.
  @param[in]  IoMmuAccess       The IOMMU access.

  @retval EFI_SUCCESS            The IoMmuAccess is set for the memory range specified by BaseAddress and Length.
  @retval EFI_INVALID_PARAMETER  BaseAddress is not IoMmu Page size aligned.
  @retval EFI_INVALID_PARAMETER  Length is not IoMmu Page size aligned.
  @retval EFI_INVALID_PARAMETER  Length is 0.
  @retval EFI_INVALID_PARAMETER  IoMmuAccess specified an illegal combination of access.
  @retval EFI_UNSUPPORTED        The bit mask of IoMmuAccess is not supported by the IOMMU.
  @retval EFI_UNSUPPORTED        The IOMMU does not support the memory range specified by BaseAddress and Length.
  @retval EFI_OUT_OF_RESOURCES   There are not enough resources available to modify the IOMMU access.
  @retval EFI_DEVICE_ERROR       The IOMMU device reported an error while attempting the operation.
**/
EFI_STATUS
SetAccessAttribute (
  IN UINT16                Segment,
  IN VTD_SOURCE_ID         SourceId,
  IN UINT64                BaseAddress,
  IN UINT64                Length,
  IN UINT64                IoMmuAccess
  );

/**
  Return the index of PCI descriptor.

  @param[in]  VtdIndex          The index used to identify a VTd engine.
  @param[in]  Segment           The Segment used to identify a VTd engine.
  @param[in]  SourceId          The SourceId used to identify a VTd engine and table entry.

  @return The index of the PCI descriptor.
  @retval (UINTN)-1  The PCI descriptor is not found.
**/
UINTN
GetPciDescriptor (
  IN UINTN          VtdIndex,
  IN UINT16         Segment,
  IN VTD_SOURCE_ID  SourceId
  );

/**
  Dump VTd registers if there is error.
**/
VOID
DumpVtdIfError (
  VOID
  );

/**
  Initialize platform VTd policy.
**/
VOID
InitializePlatformVTdPolicy (
  VOID
  );

/**
  Always enable the VTd page attribute for the device.

  @param[in]  Segment           The Segment used to identify a VTd engine.
  @param[in]  SourceId          The SourceId used to identify a VTd engine and table entry.

  @retval EFI_SUCCESS           The VTd entry is updated to always enable all DMA access for the specific device.
**/
EFI_STATUS
AlwaysEnablePageAttribute (
  IN UINT16                  Segment,
  IN VTD_SOURCE_ID           SourceId
  );

/**
  Convert the DeviceHandle to SourceId and Segment.

  @param[in]  DeviceHandle      The device who initiates the DMA access request.
  @param[out] Segment           The Segment used to identify a VTd engine.
  @param[out] SourceId          The SourceId used to identify a VTd engine and table entry.

  @retval EFI_SUCCESS            The Segment and SourceId are returned.
  @retval EFI_INVALID_PARAMETER  DeviceHandle is an invalid handle.
  @retval EFI_UNSUPPORTED        DeviceHandle is unknown by the IOMMU.
**/
EFI_STATUS
DeviceHandleToSourceId (
  IN EFI_HANDLE            DeviceHandle,
  OUT UINT16               *Segment,
  OUT VTD_SOURCE_ID        *SourceId
  );

/**
  Get device information from mapping.

  @param[in]  Mapping        The mapping.
  @param[out] DeviceAddress  The device address of the mapping.
  @param[out] NumberOfPages  The number of pages of the mapping.

  @retval EFI_SUCCESS            The device information is returned.
  @retval EFI_INVALID_PARAMETER  The mapping is invalid.
**/
EFI_STATUS
GetDeviceInfoFromMapping (
  IN  VOID                                     *Mapping,
  OUT EFI_PHYSICAL_ADDRESS                     *DeviceAddress,
  OUT UINTN                                    *NumberOfPages
  );

/**
  Initialize DMA protection.
**/
VOID
InitializeDmaProtection (
  VOID
  );

/**
  Allocate zero pages.

  @param[in]  Pages the number of pages.

  @return the page address.
  @retval NULL No resource to allocate pages.
**/
VOID *
EFIAPI
AllocateZeroPages (
  IN UINTN  Pages
  );

#endif
