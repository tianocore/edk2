/** @file

  Copyright (c) 2017 - 2018, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <PiPei.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/IoLib.h>
#include <Library/DebugLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/HobLib.h>
#include <IndustryStandard/Vtd.h>
#include <Ppi/IoMmu.h>
#include <Ppi/VtdInfo.h>
#include <Ppi/MemoryDiscovered.h>
#include <Ppi/EndOfPeiPhase.h>

#include "IntelVTdPmrPei.h"

EFI_GUID mVTdInfoGuid = {
  0x222f5e30, 0x5cd, 0x49c6, { 0x8a, 0xc, 0x36, 0xd6, 0x58, 0x41, 0xe0, 0x82 }
};

EFI_GUID mDmaBufferInfoGuid = {
  0x7b624ec7, 0xfb67, 0x4f9c, { 0xb6, 0xb0, 0x4d, 0xfa, 0x9c, 0x88, 0x20, 0x39 }
};

typedef struct {
  UINTN                             DmaBufferBase;
  UINTN                             DmaBufferSize;
  UINTN                             DmaBufferCurrentTop;
  UINTN                             DmaBufferCurrentBottom;
} DMA_BUFFER_INFO;

#define MAP_INFO_SIGNATURE  SIGNATURE_32 ('D', 'M', 'A', 'P')
typedef struct {
  UINT32                                    Signature;
  EDKII_IOMMU_OPERATION                     Operation;
  UINTN                                     NumberOfBytes;
  EFI_PHYSICAL_ADDRESS                      HostAddress;
  EFI_PHYSICAL_ADDRESS                      DeviceAddress;
} MAP_INFO;

/**

  PEI Memory Layout:

              +------------------+ <=============== PHMR.Limit (+ alignment) (1 << (HostAddressWidth + 1))
              |   Mem Resource   |
              |                  |

              +------------------+ <------- EfiMemoryTop
              |   PEI allocated  |
  =========== +==================+ <=============== PHMR.Base
       ^      |    Commom Buf    |
       |      |  --------------  |
  DMA Buffer  |   * DMA FREE *   |
       |      |  --------------  |
       V      |  Read/Write Buf  |
  =========== +==================+ <=============== PLMR.Limit (+ alignment)
              |   PEI allocated  |
              |  --------------  | <------- EfiFreeMemoryTop
              |   * PEI FREE *   |
              |  --------------  | <------- EfiFreeMemoryBottom
              |       hob        |
              |  --------------  |
              |      Stack       |
              +------------------+ <------- EfiMemoryBottom / Stack Bottom

              +------------------+
              |   Mem Alloc Hob  |
              +------------------+

              |                  |
              |   Mem Resource   |
              +------------------+ <=============== PLMR.Base (0)
**/

/**
  Set IOMMU attribute for a system memory.

  If the IOMMU PPI exists, the system memory cannot be used
  for DMA by default.

  When a device requests a DMA access for a system memory,
  the device driver need use SetAttribute() to update the IOMMU
  attribute to request DMA access (read and/or write).

  @param[in]  This              The PPI instance pointer.
  @param[in]  Mapping           The mapping value returned from Map().
  @param[in]  IoMmuAccess       The IOMMU access.

  @retval EFI_SUCCESS            The IoMmuAccess is set for the memory range specified by DeviceAddress and Length.
  @retval EFI_INVALID_PARAMETER  Mapping is not a value that was returned by Map().
  @retval EFI_INVALID_PARAMETER  IoMmuAccess specified an illegal combination of access.
  @retval EFI_UNSUPPORTED        The bit mask of IoMmuAccess is not supported by the IOMMU.
  @retval EFI_UNSUPPORTED        The IOMMU does not support the memory range specified by Mapping.
  @retval EFI_OUT_OF_RESOURCES   There are not enough resources available to modify the IOMMU access.
  @retval EFI_DEVICE_ERROR       The IOMMU device reported an error while attempting the operation.
  @retval EFI_NOT_AVAILABLE_YET  DMA protection has been enabled, but DMA buffer are
                                 not available to be allocated yet.

**/
EFI_STATUS
EFIAPI
PeiIoMmuSetAttribute (
  IN EDKII_IOMMU_PPI       *This,
  IN VOID                  *Mapping,
  IN UINT64                IoMmuAccess
  )
{
  VOID                        *Hob;
  DMA_BUFFER_INFO             *DmaBufferInfo;

  Hob = GetFirstGuidHob (&mDmaBufferInfoGuid);
  DmaBufferInfo = GET_GUID_HOB_DATA(Hob);

  if (DmaBufferInfo->DmaBufferCurrentTop == 0) {
    return EFI_NOT_AVAILABLE_YET;
  }

  return EFI_SUCCESS;
}

/**
  Provides the controller-specific addresses required to access system memory from a
  DMA bus master.

  @param  This                  The PPI instance pointer.
  @param  Operation             Indicates if the bus master is going to read or write to system memory.
  @param  HostAddress           The system memory address to map to the PCI controller.
  @param  NumberOfBytes         On input the number of bytes to map. On output the number of bytes
                                that were mapped.
  @param  DeviceAddress         The resulting map address for the bus master PCI controller to use to
                                access the hosts HostAddress.
  @param  Mapping               A resulting value to pass to Unmap().

  @retval EFI_SUCCESS           The range was mapped for the returned NumberOfBytes.
  @retval EFI_UNSUPPORTED       The HostAddress cannot be mapped as a common buffer.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack of resources.
  @retval EFI_DEVICE_ERROR      The system hardware could not map the requested address.
  @retval EFI_NOT_AVAILABLE_YET DMA protection has been enabled, but DMA buffer are
                                not available to be allocated yet.

**/
EFI_STATUS
EFIAPI
PeiIoMmuMap (
  IN     EDKII_IOMMU_PPI                            *This,
  IN     EDKII_IOMMU_OPERATION                      Operation,
  IN     VOID                                       *HostAddress,
  IN OUT UINTN                                      *NumberOfBytes,
  OUT    EFI_PHYSICAL_ADDRESS                       *DeviceAddress,
  OUT    VOID                                       **Mapping
  )
{
  MAP_INFO                    *MapInfo;
  UINTN                       Length;
  VOID                        *Hob;
  DMA_BUFFER_INFO             *DmaBufferInfo;

  Hob = GetFirstGuidHob (&mDmaBufferInfoGuid);
  DmaBufferInfo = GET_GUID_HOB_DATA(Hob);

  DEBUG ((DEBUG_VERBOSE, "PeiIoMmuMap - HostAddress - 0x%x, NumberOfBytes - %x\n", HostAddress, *NumberOfBytes));
  DEBUG ((DEBUG_VERBOSE, "  DmaBufferCurrentTop - %x\n", DmaBufferInfo->DmaBufferCurrentTop));
  DEBUG ((DEBUG_VERBOSE, "  DmaBufferCurrentBottom - %x\n", DmaBufferInfo->DmaBufferCurrentBottom));

  if (DmaBufferInfo->DmaBufferCurrentTop == 0) {
    return EFI_NOT_AVAILABLE_YET;
  }

  if (Operation == EdkiiIoMmuOperationBusMasterCommonBuffer ||
      Operation == EdkiiIoMmuOperationBusMasterCommonBuffer64) {
    *DeviceAddress = (UINTN)HostAddress;
    *Mapping = NULL;
    return EFI_SUCCESS;
  }

  Length = *NumberOfBytes + sizeof(MAP_INFO);
  if (Length > DmaBufferInfo->DmaBufferCurrentTop - DmaBufferInfo->DmaBufferCurrentBottom) {
    DEBUG ((DEBUG_ERROR, "PeiIoMmuMap - OUT_OF_RESOURCE\n"));
    ASSERT (FALSE);
    return EFI_OUT_OF_RESOURCES;
  }

  *DeviceAddress = DmaBufferInfo->DmaBufferCurrentBottom;
  DmaBufferInfo->DmaBufferCurrentBottom += Length;

  MapInfo = (VOID *)(UINTN)(*DeviceAddress + *NumberOfBytes);
  MapInfo->Signature     = MAP_INFO_SIGNATURE;
  MapInfo->Operation     = Operation;
  MapInfo->NumberOfBytes = *NumberOfBytes;
  MapInfo->HostAddress   = (UINTN)HostAddress;
  MapInfo->DeviceAddress = *DeviceAddress;
  *Mapping = MapInfo;
  DEBUG ((DEBUG_VERBOSE, "  Op(%x):DeviceAddress - %x, Mapping - %x\n", Operation, (UINTN)*DeviceAddress, MapInfo));

  //
  // If this is a read operation from the Bus Master's point of view,
  // then copy the contents of the real buffer into the mapped buffer
  // so the Bus Master can read the contents of the real buffer.
  //
  if (Operation == EdkiiIoMmuOperationBusMasterRead ||
      Operation == EdkiiIoMmuOperationBusMasterRead64) {
    CopyMem (
      (VOID *) (UINTN) MapInfo->DeviceAddress,
      (VOID *) (UINTN) MapInfo->HostAddress,
      MapInfo->NumberOfBytes
      );
  }

  return EFI_SUCCESS;
}

/**
  Completes the Map() operation and releases any corresponding resources.

  @param  This                  The PPI instance pointer.
  @param  Mapping               The mapping value returned from Map().

  @retval EFI_SUCCESS           The range was unmapped.
  @retval EFI_INVALID_PARAMETER Mapping is not a value that was returned by Map().
  @retval EFI_DEVICE_ERROR      The data was not committed to the target system memory.
  @retval EFI_NOT_AVAILABLE_YET DMA protection has been enabled, but DMA buffer are
                                not available to be allocated yet.

**/
EFI_STATUS
EFIAPI
PeiIoMmuUnmap (
  IN  EDKII_IOMMU_PPI                          *This,
  IN  VOID                                     *Mapping
  )
{
  MAP_INFO                    *MapInfo;
  UINTN                       Length;
  VOID                        *Hob;
  DMA_BUFFER_INFO             *DmaBufferInfo;

  Hob = GetFirstGuidHob (&mDmaBufferInfoGuid);
  DmaBufferInfo = GET_GUID_HOB_DATA(Hob);

  DEBUG ((DEBUG_VERBOSE, "PeiIoMmuUnmap - Mapping - %x\n", Mapping));
  DEBUG ((DEBUG_VERBOSE, "  DmaBufferCurrentTop - %x\n", DmaBufferInfo->DmaBufferCurrentTop));
  DEBUG ((DEBUG_VERBOSE, "  DmaBufferCurrentBottom - %x\n", DmaBufferInfo->DmaBufferCurrentBottom));

  if (DmaBufferInfo->DmaBufferCurrentTop == 0) {
    return EFI_NOT_AVAILABLE_YET;
  }

  if (Mapping == NULL) {
    return EFI_SUCCESS;
  }

  MapInfo = Mapping;
  ASSERT (MapInfo->Signature == MAP_INFO_SIGNATURE);
  DEBUG ((DEBUG_VERBOSE, "  Op(%x):DeviceAddress - %x, NumberOfBytes - %x\n", MapInfo->Operation, (UINTN)MapInfo->DeviceAddress, MapInfo->NumberOfBytes));

  //
  // If this is a write operation from the Bus Master's point of view,
  // then copy the contents of the mapped buffer into the real buffer
  // so the processor can read the contents of the real buffer.
  //
  if (MapInfo->Operation == EdkiiIoMmuOperationBusMasterWrite ||
      MapInfo->Operation == EdkiiIoMmuOperationBusMasterWrite64) {
    CopyMem (
      (VOID *) (UINTN) MapInfo->HostAddress,
      (VOID *) (UINTN) MapInfo->DeviceAddress,
      MapInfo->NumberOfBytes
      );
  }

  Length = MapInfo->NumberOfBytes + sizeof(MAP_INFO);
  if (DmaBufferInfo->DmaBufferCurrentBottom == MapInfo->DeviceAddress + Length) {
    DmaBufferInfo->DmaBufferCurrentBottom -= Length;
  }

  return EFI_SUCCESS;
}

/**
  Allocates pages that are suitable for an OperationBusMasterCommonBuffer or
  OperationBusMasterCommonBuffer64 mapping.

  @param  This                  The PPI instance pointer.
  @param  MemoryType            The type of memory to allocate, EfiBootServicesData or
                                EfiRuntimeServicesData.
  @param  Pages                 The number of pages to allocate.
  @param  HostAddress           A pointer to store the base system memory address of the
                                allocated range.
  @param  Attributes            The requested bit mask of attributes for the allocated range.

  @retval EFI_SUCCESS           The requested memory pages were allocated.
  @retval EFI_UNSUPPORTED       Attributes is unsupported. The only legal attribute bits are
                                MEMORY_WRITE_COMBINE, MEMORY_CACHED and DUAL_ADDRESS_CYCLE.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES  The memory pages could not be allocated.
  @retval EFI_NOT_AVAILABLE_YET DMA protection has been enabled, but DMA buffer are
                                not available to be allocated yet.

**/
EFI_STATUS
EFIAPI
PeiIoMmuAllocateBuffer (
  IN     EDKII_IOMMU_PPI                          *This,
  IN     EFI_MEMORY_TYPE                          MemoryType,
  IN     UINTN                                    Pages,
  IN OUT VOID                                     **HostAddress,
  IN     UINT64                                   Attributes
  )
{
  UINTN                       Length;
  VOID                        *Hob;
  DMA_BUFFER_INFO             *DmaBufferInfo;

  Hob = GetFirstGuidHob (&mDmaBufferInfoGuid);
  DmaBufferInfo = GET_GUID_HOB_DATA(Hob);

  DEBUG ((DEBUG_VERBOSE, "PeiIoMmuAllocateBuffer - page - %x\n", Pages));
  DEBUG ((DEBUG_VERBOSE, "  DmaBufferCurrentTop - %x\n", DmaBufferInfo->DmaBufferCurrentTop));
  DEBUG ((DEBUG_VERBOSE, "  DmaBufferCurrentBottom - %x\n", DmaBufferInfo->DmaBufferCurrentBottom));

  if (DmaBufferInfo->DmaBufferCurrentTop == 0) {
    return EFI_NOT_AVAILABLE_YET;
  }

  Length = EFI_PAGES_TO_SIZE(Pages);
  if (Length > DmaBufferInfo->DmaBufferCurrentTop - DmaBufferInfo->DmaBufferCurrentBottom) {
    DEBUG ((DEBUG_ERROR, "PeiIoMmuAllocateBuffer - OUT_OF_RESOURCE\n"));
    ASSERT (FALSE);
    return EFI_OUT_OF_RESOURCES;
  }
  *HostAddress = (VOID *)(UINTN)(DmaBufferInfo->DmaBufferCurrentTop - Length);
  DmaBufferInfo->DmaBufferCurrentTop -= Length;

  DEBUG ((DEBUG_VERBOSE, "PeiIoMmuAllocateBuffer - allocate - %x\n", *HostAddress));
  return EFI_SUCCESS;
}

/**
  Frees memory that was allocated with AllocateBuffer().

  @param  This                  The PPI instance pointer.
  @param  Pages                 The number of pages to free.
  @param  HostAddress           The base system memory address of the allocated range.

  @retval EFI_SUCCESS           The requested memory pages were freed.
  @retval EFI_INVALID_PARAMETER The memory range specified by HostAddress and Pages
                                was not allocated with AllocateBuffer().
  @retval EFI_NOT_AVAILABLE_YET DMA protection has been enabled, but DMA buffer are
                                not available to be allocated yet.

**/
EFI_STATUS
EFIAPI
PeiIoMmuFreeBuffer (
  IN  EDKII_IOMMU_PPI                          *This,
  IN  UINTN                                    Pages,
  IN  VOID                                     *HostAddress
  )
{
  UINTN                       Length;
  VOID                        *Hob;
  DMA_BUFFER_INFO             *DmaBufferInfo;

  Hob = GetFirstGuidHob (&mDmaBufferInfoGuid);
  DmaBufferInfo = GET_GUID_HOB_DATA(Hob);

  DEBUG ((DEBUG_VERBOSE, "PeiIoMmuFreeBuffer - page - %x, HostAddr - %x\n", Pages, HostAddress));
  DEBUG ((DEBUG_VERBOSE, "  DmaBufferCurrentTop - %x\n", DmaBufferInfo->DmaBufferCurrentTop));
  DEBUG ((DEBUG_VERBOSE, "  DmaBufferCurrentBottom - %x\n", DmaBufferInfo->DmaBufferCurrentBottom));

  if (DmaBufferInfo->DmaBufferCurrentTop == 0) {
    return EFI_NOT_AVAILABLE_YET;
  }

  Length = EFI_PAGES_TO_SIZE(Pages);
  if ((UINTN)HostAddress == DmaBufferInfo->DmaBufferCurrentTop) {
    DmaBufferInfo->DmaBufferCurrentTop += Length;
  }

  return EFI_SUCCESS;
}

EDKII_IOMMU_PPI mIoMmuPpi = {
  EDKII_IOMMU_PPI_REVISION,
  PeiIoMmuSetAttribute,
  PeiIoMmuMap,
  PeiIoMmuUnmap,
  PeiIoMmuAllocateBuffer,
  PeiIoMmuFreeBuffer,
};

CONST EFI_PEI_PPI_DESCRIPTOR mIoMmuPpiList = {
  EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST,
  &gEdkiiIoMmuPpiGuid,
  (VOID *) &mIoMmuPpi
};

/**
  Initialize DMA protection.

  @param VTdInfo        The VTd engine context information.

  @retval EFI_SUCCESS           the DMA protection is initialized.
  @retval EFI_OUT_OF_RESOURCES  no enough resource to initialize DMA protection.
**/
EFI_STATUS
InitDmaProtection (
  IN   VTD_INFO                    *VTdInfo
  )
{
  EFI_STATUS                  Status;
  UINT32                      LowMemoryAlignment;
  UINT64                      HighMemoryAlignment;
  UINTN                       MemoryAlignment;
  UINTN                       LowBottom;
  UINTN                       LowTop;
  UINTN                       HighBottom;
  UINT64                      HighTop;
  DMA_BUFFER_INFO             *DmaBufferInfo;
  VOID                        *Hob;
  EFI_PEI_PPI_DESCRIPTOR      *OldDescriptor;
  EDKII_IOMMU_PPI             *OldIoMmuPpi;

  Hob = GetFirstGuidHob (&mDmaBufferInfoGuid);
  DmaBufferInfo = GET_GUID_HOB_DATA(Hob);

  DEBUG ((DEBUG_INFO, " DmaBufferSize : 0x%x\n", DmaBufferInfo->DmaBufferSize));

  LowMemoryAlignment = GetLowMemoryAlignment (VTdInfo, VTdInfo->EngineMask);
  HighMemoryAlignment = GetHighMemoryAlignment (VTdInfo, VTdInfo->EngineMask);
  if (LowMemoryAlignment < HighMemoryAlignment) {
    MemoryAlignment = (UINTN)HighMemoryAlignment;
  } else {
    MemoryAlignment = LowMemoryAlignment;
  }
  ASSERT (DmaBufferInfo->DmaBufferSize == ALIGN_VALUE(DmaBufferInfo->DmaBufferSize, MemoryAlignment));
  DmaBufferInfo->DmaBufferBase = (UINTN)AllocateAlignedPages (EFI_SIZE_TO_PAGES(DmaBufferInfo->DmaBufferSize), MemoryAlignment);
  ASSERT (DmaBufferInfo->DmaBufferBase != 0);
  if (DmaBufferInfo->DmaBufferBase == 0) {
    DEBUG ((DEBUG_INFO, " InitDmaProtection : OutOfResource\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  DEBUG ((DEBUG_INFO, " DmaBufferBase : 0x%x\n", DmaBufferInfo->DmaBufferBase));

  DmaBufferInfo->DmaBufferCurrentTop = DmaBufferInfo->DmaBufferBase + DmaBufferInfo->DmaBufferSize;
  DmaBufferInfo->DmaBufferCurrentBottom = DmaBufferInfo->DmaBufferBase;

  //
  // (Re)Install PPI.
  //
  Status = PeiServicesLocatePpi (
             &gEdkiiIoMmuPpiGuid,
             0,
             &OldDescriptor,
             (VOID **) &OldIoMmuPpi
             );
  if (!EFI_ERROR (Status)) {
    Status = PeiServicesReInstallPpi (OldDescriptor, &mIoMmuPpiList);
  } else {
    Status = PeiServicesInstallPpi (&mIoMmuPpiList);
  }
  ASSERT_EFI_ERROR (Status);

  LowBottom = 0;
  LowTop = DmaBufferInfo->DmaBufferBase;
  HighBottom = DmaBufferInfo->DmaBufferBase + DmaBufferInfo->DmaBufferSize;
  HighTop = LShiftU64 (1, VTdInfo->HostAddressWidth + 1);

  Status = SetDmaProtectedRange (
             VTdInfo,
             VTdInfo->EngineMask,
             (UINT32)LowBottom,
             (UINT32)(LowTop - LowBottom),
             HighBottom,
             HighTop - HighBottom
             );

  if (EFI_ERROR(Status)) {
    FreePages ((VOID *)DmaBufferInfo->DmaBufferBase, EFI_SIZE_TO_PAGES(DmaBufferInfo->DmaBufferSize));
  }

  return Status;
}

/**
  Initializes the Intel VTd Info.

  @retval EFI_SUCCESS            Usb bot driver is successfully initialized.
  @retval EFI_OUT_OF_RESOURCES   Can't initialize the driver.

**/
EFI_STATUS
InitVTdInfo (
  VOID
  )
{
  EFI_STATUS                  Status;
  EFI_ACPI_DMAR_HEADER        *AcpiDmarTable;
  VOID                        *Hob;

  Status = PeiServicesLocatePpi (
             &gEdkiiVTdInfoPpiGuid,
             0,
             NULL,
             (VOID **)&AcpiDmarTable
             );
  ASSERT_EFI_ERROR(Status);

  DumpAcpiDMAR (AcpiDmarTable);

  //
  // Clear old VTdInfo Hob.
  //
  Hob = GetFirstGuidHob (&mVTdInfoGuid);
  if (Hob != NULL) {
    ZeroMem (&((EFI_HOB_GUID_TYPE *)Hob)->Name, sizeof(EFI_GUID));
  }

  //
  // Get DMAR information to local VTdInfo
  //
  Status = ParseDmarAcpiTableDrhd (AcpiDmarTable);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  //
  // NOTE: Do not parse RMRR here, because RMRR may cause PMR programming.
  //

  return EFI_SUCCESS;
}

/**
  Initializes the Intel VTd PMR for all memory.

  @retval EFI_SUCCESS            Usb bot driver is successfully initialized.
  @retval EFI_OUT_OF_RESOURCES   Can't initialize the driver.

**/
EFI_STATUS
InitVTdPmrForAll (
  VOID
  )
{
  EFI_STATUS                  Status;
  VOID                        *Hob;
  VTD_INFO                    *VTdInfo;
  UINTN                       LowBottom;
  UINTN                       LowTop;
  UINTN                       HighBottom;
  UINT64                      HighTop;

  Hob = GetFirstGuidHob (&mVTdInfoGuid);
  VTdInfo = GET_GUID_HOB_DATA(Hob);

  LowBottom = 0;
  LowTop = 0;
  HighBottom = 0;
  HighTop = LShiftU64 (1, VTdInfo->HostAddressWidth + 1);

  Status = SetDmaProtectedRange (
             VTdInfo,
             VTdInfo->EngineMask,
             (UINT32)LowBottom,
             (UINT32)(LowTop - LowBottom),
             HighBottom,
             HighTop - HighBottom
             );

  return Status;
}

/**
  Initializes the Intel VTd PMR for DMA buffer.

  @retval EFI_SUCCESS            Usb bot driver is successfully initialized.
  @retval EFI_OUT_OF_RESOURCES   Can't initialize the driver.

**/
EFI_STATUS
InitVTdPmrForDma (
  VOID
  )
{
  EFI_STATUS                  Status;
  VOID                        *Hob;
  VTD_INFO                    *VTdInfo;

  Hob = GetFirstGuidHob (&mVTdInfoGuid);
  VTdInfo = GET_GUID_HOB_DATA(Hob);

  //
  // If there is RMRR memory, parse it here.
  //
  ParseDmarAcpiTableRmrr (VTdInfo);

  //
  // Allocate a range in PEI memory as DMA buffer
  // Mark others to be DMA protected.
  //
  Status = InitDmaProtection (VTdInfo);

  return Status;
}

/**
  This function handles S3 resume task at the end of PEI

  @param[in] PeiServices    Pointer to PEI Services Table.
  @param[in] NotifyDesc     Pointer to the descriptor for the Notification event that
                            caused this function to execute.
  @param[in] Ppi            Pointer to the PPI data associated with this function.

  @retval EFI_STATUS        Always return EFI_SUCCESS
**/
EFI_STATUS
EFIAPI
S3EndOfPeiNotify(
  IN EFI_PEI_SERVICES          **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR *NotifyDesc,
  IN VOID                      *Ppi
  )
{
  VOID                        *Hob;
  VTD_INFO                    *VTdInfo;
  UINT64                      EngineMask;

  DEBUG((DEBUG_INFO, "VTdPmr S3EndOfPeiNotify\n"));

  if ((PcdGet8(PcdVTdPolicyPropertyMask) & BIT1) == 0) {
    Hob = GetFirstGuidHob (&mVTdInfoGuid);
    if (Hob == NULL) {
      return EFI_SUCCESS;
    }
    VTdInfo = GET_GUID_HOB_DATA(Hob);

    EngineMask = LShiftU64 (1, VTdInfo->VTdEngineCount) - 1;
    DisableDmaProtection (VTdInfo, EngineMask);
  }
  return EFI_SUCCESS;
}

EFI_PEI_NOTIFY_DESCRIPTOR mS3EndOfPeiNotifyDesc = {
  (EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiEndOfPeiSignalPpiGuid,
  S3EndOfPeiNotify
};

/**
  This function handles VTd engine setup

  @param[in] PeiServices    Pointer to PEI Services Table.
  @param[in] NotifyDesc     Pointer to the descriptor for the Notification event that
                            caused this function to execute.
  @param[in] Ppi            Pointer to the PPI data associated with this function.

  @retval EFI_STATUS        Always return EFI_SUCCESS
**/
EFI_STATUS
EFIAPI
VTdInfoNotify (
  IN EFI_PEI_SERVICES          **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR *NotifyDesc,
  IN VOID                      *Ppi
  )
{
  EFI_STATUS                  Status;
  VOID                        *MemoryDiscovered;
  UINT64                      EnabledEngineMask;
  VOID                        *Hob;
  VTD_INFO                    *VTdInfo;
  BOOLEAN                     MemoryInitialized;

  DEBUG ((DEBUG_INFO, "VTdInfoNotify\n"));

  //
  // Check if memory is initialized.
  //
  MemoryInitialized = FALSE;
  Status = PeiServicesLocatePpi (
             &gEfiPeiMemoryDiscoveredPpiGuid,
             0,
             NULL,
             &MemoryDiscovered
             );
  if (!EFI_ERROR(Status)) {
    MemoryInitialized = TRUE;
  }

  DEBUG ((DEBUG_INFO, "MemoryInitialized - %x\n", MemoryInitialized));

  if (!MemoryInitialized) {
    //
    // If the memory is not initialized,
    // Protect all system memory
    //
    InitVTdInfo ();
    InitVTdPmrForAll ();

    //
    // Install PPI.
    //
    Status = PeiServicesInstallPpi (&mIoMmuPpiList);
    ASSERT_EFI_ERROR(Status);
  } else {
    //
    // If the memory is initialized,
    // Allocate DMA buffer and protect rest system memory
    //

    //
    // NOTE: We need reinit VTdInfo because previous information might be overriden.
    //
    InitVTdInfo ();

    Hob = GetFirstGuidHob (&mVTdInfoGuid);
    VTdInfo = GET_GUID_HOB_DATA(Hob);

    //
    // NOTE: We need check if PMR is enabled or not.
    //
    EnabledEngineMask = GetDmaProtectionEnabledEngineMask (VTdInfo, VTdInfo->EngineMask);
    if (EnabledEngineMask != 0) {
      EnableVTdTranslationProtection (VTdInfo, EnabledEngineMask);
      DisableDmaProtection (VTdInfo, EnabledEngineMask);
    }
    InitVTdPmrForDma ();
    if (EnabledEngineMask != 0) {
      DisableVTdTranslationProtection (VTdInfo, EnabledEngineMask);
    }

  }

  return EFI_SUCCESS;
}

EFI_PEI_NOTIFY_DESCRIPTOR mVTdInfoNotifyDesc = {
  (EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEdkiiVTdInfoPpiGuid,
  VTdInfoNotify
};

/**
  Initializes the Intel VTd PMR PEIM.

  @param  FileHandle  Handle of the file being invoked.
  @param  PeiServices Describes the list of possible PEI Services.

  @retval EFI_SUCCESS            Usb bot driver is successfully initialized.
  @retval EFI_OUT_OF_RESOURCES   Can't initialize the driver.

**/
EFI_STATUS
EFIAPI
IntelVTdPmrInitialize (
  IN EFI_PEI_FILE_HANDLE       FileHandle,
  IN CONST EFI_PEI_SERVICES    **PeiServices
  )
{
  EFI_STATUS                  Status;
  EFI_BOOT_MODE               BootMode;
  DMA_BUFFER_INFO             *DmaBufferInfo;

  DEBUG ((DEBUG_INFO, "IntelVTdPmrInitialize\n"));

  if ((PcdGet8(PcdVTdPolicyPropertyMask) & BIT0) == 0) {
    return EFI_UNSUPPORTED;
  }

  DmaBufferInfo = BuildGuidHob (&mDmaBufferInfoGuid, sizeof(DMA_BUFFER_INFO));
  ASSERT(DmaBufferInfo != NULL);
  if (DmaBufferInfo == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  ZeroMem (DmaBufferInfo, sizeof(DMA_BUFFER_INFO));

  PeiServicesGetBootMode (&BootMode);

  if (BootMode == BOOT_ON_S3_RESUME) {
    DmaBufferInfo->DmaBufferSize = PcdGet32 (PcdVTdPeiDmaBufferSizeS3);
  } else {
    DmaBufferInfo->DmaBufferSize = PcdGet32 (PcdVTdPeiDmaBufferSize);
  }

  Status = PeiServicesNotifyPpi (&mVTdInfoNotifyDesc);
  ASSERT_EFI_ERROR (Status);

  //
  // Register EndOfPei Notify for S3
  //
  if (BootMode == BOOT_ON_S3_RESUME) {
    Status = PeiServicesNotifyPpi (&mS3EndOfPeiNotifyDesc);
    ASSERT_EFI_ERROR (Status);
  }

  return EFI_SUCCESS;
}

