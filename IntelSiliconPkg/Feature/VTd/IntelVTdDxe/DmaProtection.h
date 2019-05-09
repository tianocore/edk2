/** @file

  Copyright (c) 2017 - 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

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
#include <Library/CacheMaintenanceLib.h>
#include <Library/PerformanceLib.h>
#include <Library/PrintLib.h>
#include <Library/ReportStatusCodeLib.h>

#include <Guid/EventGroup.h>
#include <Guid/Acpi.h>

#include <Protocol/DxeSmmReadyToLock.h>
#include <Protocol/PciRootBridgeIo.h>
#include <Protocol/PciIo.h>
#include <Protocol/PciEnumerationComplete.h>
#include <Protocol/PlatformVtdPolicy.h>
#include <Protocol/IoMmu.h>

#include <IndustryStandard/Pci.h>
#include <IndustryStandard/DmaRemappingReportingTable.h>
#include <IndustryStandard/Vtd.h>

#define VTD_64BITS_ADDRESS(Lo, Hi) (LShiftU64 (Lo, 12) | LShiftU64 (Hi, 32))

#define ALIGN_VALUE_UP(Value, Alignment)  (((Value) + (Alignment) - 1) & (~((Alignment) - 1)))
#define ALIGN_VALUE_LOW(Value, Alignment) ((Value) & (~((Alignment) - 1)))

#define VTD_TPL_LEVEL TPL_NOTIFY

//
// This is the initial max PCI DATA number.
// The number may be enlarged later.
//
#define MAX_VTD_PCI_DATA_NUMBER             0x100

typedef struct {
  UINT8                            DeviceType;
  VTD_SOURCE_ID                    PciSourceId;
  EDKII_PLATFORM_VTD_PCI_DEVICE_ID PciDeviceId;
  // for statistic analysis
  UINTN                            AccessCount;
} PCI_DEVICE_DATA;

typedef struct {
  BOOLEAN                          IncludeAllFlag;
  UINTN                            PciDeviceDataNumber;
  UINTN                            PciDeviceDataMaxNumber;
  PCI_DEVICE_DATA                  *PciDeviceData;
} PCI_DEVICE_INFORMATION;

typedef struct {
  UINTN                            VtdUnitBaseAddress;
  UINT16                           Segment;
  VTD_CAP_REG                      CapReg;
  VTD_ECAP_REG                     ECapReg;
  VTD_ROOT_ENTRY                   *RootEntryTable;
  VTD_EXT_ROOT_ENTRY               *ExtRootEntryTable;
  VTD_SECOND_LEVEL_PAGING_ENTRY    *FixedSecondLevelPagingEntry;
  BOOLEAN                          HasDirtyContext;
  BOOLEAN                          HasDirtyPages;
  PCI_DEVICE_INFORMATION           PciDeviceInfo;
} VTD_UNIT_INFORMATION;

//
// This is the initial max ACCESS request.
// The number may be enlarged later.
//
#define MAX_VTD_ACCESS_REQUEST      0x100

typedef struct {
  UINT16                Segment;
  VTD_SOURCE_ID         SourceId;
  UINT64                BaseAddress;
  UINT64                Length;
  UINT64                IoMmuAccess;
} VTD_ACCESS_REQUEST;


/**
  The scan bus callback function.

  It is called in PCI bus scan for each PCI device under the bus.

  @param[in]  Context               The context of the callback.
  @param[in]  Segment               The segment of the source.
  @param[in]  Bus                   The bus of the source.
  @param[in]  Device                The device of the source.
  @param[in]  Function              The function of the source.

  @retval EFI_SUCCESS           The specific PCI device is processed in the callback.
**/
typedef
EFI_STATUS
(EFIAPI *SCAN_BUS_FUNC_CALLBACK_FUNC) (
  IN VOID           *Context,
  IN UINT16         Segment,
  IN UINT8          Bus,
  IN UINT8          Device,
  IN UINT8          Function
  );

extern EFI_ACPI_DMAR_HEADER  *mAcpiDmarTable;

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
  Flush VTd engine write buffer.

  @param[in]  VtdIndex          The index used to identify a VTd engine.
**/
VOID
FlushWriteBuffer (
  IN UINTN  VtdIndex
  );

/**
  Invalidate VTd context cache.

  @param[in]  VtdIndex          The index used to identify a VTd engine.
**/
EFI_STATUS
InvalidateContextCache (
  IN UINTN  VtdIndex
  );

/**
  Invalidate VTd IOTLB.

  @param[in]  VtdIndex          The index used to identify a VTd engine.
**/
EFI_STATUS
InvalidateIOTLB (
  IN UINTN  VtdIndex
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
  Register PCI device to VTd engine.

  @param[in]  VtdIndex              The index of VTd engine.
  @param[in]  Segment               The segment of the source.
  @param[in]  SourceId              The SourceId of the source.
  @param[in]  DeviceType            The DMAR device scope type.
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
  IN UINT8          DeviceType,
  IN BOOLEAN        CheckExist
  );

/**
  The scan bus callback function to always enable page attribute.

  @param[in]  Context               The context of the callback.
  @param[in]  Segment               The segment of the source.
  @param[in]  Bus                   The bus of the source.
  @param[in]  Device                The device of the source.
  @param[in]  Function              The function of the source.

  @retval EFI_SUCCESS           The VTd entry is updated to always enable all DMA access for the specific device.
**/
EFI_STATUS
EFIAPI
ScanBusCallbackRegisterPciDevice (
  IN VOID           *Context,
  IN UINT16         Segment,
  IN UINT8          Bus,
  IN UINT8          Device,
  IN UINT8          Function
  );

/**
  Scan PCI bus and invoke callback function for each PCI devices under the bus.

  @param[in]  Context               The context of the callback function.
  @param[in]  Segment               The segment of the source.
  @param[in]  Bus                   The bus of the source.
  @param[in]  Callback              The callback function in PCI scan.

  @retval EFI_SUCCESS           The PCI devices under the bus are scaned.
**/
EFI_STATUS
ScanPciBus (
  IN VOID                         *Context,
  IN UINT16                       Segment,
  IN UINT8                        Bus,
  IN SCAN_BUS_FUNC_CALLBACK_FUNC  Callback
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

  @return The index of the VTd engine.
  @retval (UINTN)-1  The VTd engine is not found.
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

  @retval EFI_SUCCESS           The DMAR ACPI table is got.
  @retval EFI_ALREADY_STARTED   The DMAR ACPI table has been got previously.
  @retval EFI_NOT_FOUND         The DMAR ACPI table is not found.
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
  @param[in]  DomainIdentifier        The domain ID of the source.
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
  IN UINT16                        DomainIdentifier,
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
  Return the index of PCI data.

  @param[in]  VtdIndex          The index used to identify a VTd engine.
  @param[in]  Segment           The Segment used to identify a VTd engine.
  @param[in]  SourceId          The SourceId used to identify a VTd engine and table entry.

  @return The index of the PCI data.
  @retval (UINTN)-1  The PCI data is not found.
**/
UINTN
GetPciDataIndex (
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

/**
  Flush VTD page table and context table memory.

  This action is to make sure the IOMMU engine can get final data in memory.

  @param[in]  VtdIndex          The index used to identify a VTd engine.
  @param[in]  Base              The base address of memory to be flushed.
  @param[in]  Size              The size of memory in bytes to be flushed.
**/
VOID
FlushPageTableMemory (
  IN UINTN  VtdIndex,
  IN UINTN  Base,
  IN UINTN  Size
  );

/**
  Get PCI device information from DMAR DevScopeEntry.

  @param[in]  Segment               The segment number.
  @param[in]  DmarDevScopeEntry     DMAR DevScopeEntry
  @param[out] Bus                   The bus number.
  @param[out] Device                The device number.
  @param[out] Function              The function number.

  @retval EFI_SUCCESS  The PCI device information is returned.
**/
EFI_STATUS
GetPciBusDeviceFunction (
  IN  UINT16                                      Segment,
  IN  EFI_ACPI_DMAR_DEVICE_SCOPE_STRUCTURE_HEADER *DmarDevScopeEntry,
  OUT UINT8                                       *Bus,
  OUT UINT8                                       *Device,
  OUT UINT8                                       *Function
  );

/**
  Append VTd Access Request to global.

  @param[in]  Segment           The Segment used to identify a VTd engine.
  @param[in]  SourceId          The SourceId used to identify a VTd engine and table entry.
  @param[in]  BaseAddress       The base of device memory address to be used as the DMA memory.
  @param[in]  Length            The length of device memory address to be used as the DMA memory.
  @param[in]  IoMmuAccess       The IOMMU access.

  @retval EFI_SUCCESS           The IoMmuAccess is set for the memory range specified by BaseAddress and Length.
  @retval EFI_INVALID_PARAMETER BaseAddress is not IoMmu Page size aligned.
  @retval EFI_INVALID_PARAMETER Length is not IoMmu Page size aligned.
  @retval EFI_INVALID_PARAMETER Length is 0.
  @retval EFI_INVALID_PARAMETER IoMmuAccess specified an illegal combination of access.
  @retval EFI_UNSUPPORTED       The bit mask of IoMmuAccess is not supported by the IOMMU.
  @retval EFI_UNSUPPORTED       The IOMMU does not support the memory range specified by BaseAddress and Length.
  @retval EFI_OUT_OF_RESOURCES  There are not enough resources available to modify the IOMMU access.
  @retval EFI_DEVICE_ERROR      The IOMMU device reported an error while attempting the operation.

**/
EFI_STATUS
RequestAccessAttribute (
  IN UINT16                 Segment,
  IN VTD_SOURCE_ID          SourceId,
  IN UINT64                 BaseAddress,
  IN UINT64                 Length,
  IN UINT64                 IoMmuAccess
  );

#endif
