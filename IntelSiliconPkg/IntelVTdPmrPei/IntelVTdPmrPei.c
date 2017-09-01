/** @file

  Copyright (c) 2017, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License which accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

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
#include <Ppi/VTdInfo.h>

#include "IntelVTdPmrPei.h"

#define  TOTAL_DMA_BUFFER_SIZE    SIZE_4MB

EDKII_VTD_INFO_PPI                *mVTdInfoPpi;
UINTN                             mDmaBufferBase;
UINTN                             mDmaBufferSize = TOTAL_DMA_BUFFER_SIZE;
UINTN                             mDmaBufferCurrentTop;
UINTN                             mDmaBufferCurrentBottom;

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

              +------------------+ <------- EfiMemoryTop
              |   PEI allocated  |
  =========== +==================+
       ^      |    Commom Buf    |
       |      |  --------------  |
  DMA Buffer  |   * DMA FREE *   |
       |      |  --------------  |
       V      |  Read/Write Buf  |
  =========== +==================+
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

**/
EFI_STATUS
EFIAPI
PeiIoMmuSetAttribute (
  IN EDKII_IOMMU_PPI       *This,
  IN VOID                  *Mapping,
  IN UINT64                IoMmuAccess
  )
{
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
  MAP_INFO   *MapInfo;
  UINTN      Length;

  if (Operation == EdkiiIoMmuOperationBusMasterCommonBuffer ||
      Operation == EdkiiIoMmuOperationBusMasterCommonBuffer64) {
    *DeviceAddress = (UINTN)HostAddress;
    *Mapping = 0;
    return EFI_SUCCESS;
  }

  DEBUG ((DEBUG_VERBOSE, "PeiIoMmuMap - HostAddress - 0x%x, NumberOfBytes - %x\n", HostAddress, *NumberOfBytes));
  DEBUG ((DEBUG_VERBOSE, "  mDmaBufferCurrentTop - %x\n", mDmaBufferCurrentTop));
  DEBUG ((DEBUG_VERBOSE, "  mDmaBufferCurrentBottom - %x\n", mDmaBufferCurrentBottom));

  Length = *NumberOfBytes + sizeof(MAP_INFO);
  if (Length > mDmaBufferCurrentTop - mDmaBufferCurrentBottom) {
    DEBUG ((DEBUG_ERROR, "PeiIoMmuMap - OUT_OF_RESOURCE\n"));
    ASSERT (FALSE);
    return EFI_OUT_OF_RESOURCES;
  }

  *DeviceAddress = mDmaBufferCurrentBottom;
  mDmaBufferCurrentBottom += Length;

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
**/
EFI_STATUS
EFIAPI
PeiIoMmuUnmap (
  IN  EDKII_IOMMU_PPI                          *This,
  IN  VOID                                     *Mapping
  )
{
  MAP_INFO   *MapInfo;
  UINTN      Length;

  if (Mapping == NULL) {
    return EFI_SUCCESS;
  }

  DEBUG ((DEBUG_VERBOSE, "PeiIoMmuUnmap - Mapping - %x\n", Mapping));
  DEBUG ((DEBUG_VERBOSE, "  mDmaBufferCurrentTop - %x\n", mDmaBufferCurrentTop));
  DEBUG ((DEBUG_VERBOSE, "  mDmaBufferCurrentBottom - %x\n", mDmaBufferCurrentBottom));

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
  if (mDmaBufferCurrentBottom == MapInfo->DeviceAddress + Length) {
    mDmaBufferCurrentBottom -= Length;
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
                                MEMORY_WRITE_COMBINE and MEMORY_CACHED.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES  The memory pages could not be allocated.

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
  UINTN  Length;

  DEBUG ((DEBUG_VERBOSE, "PeiIoMmuAllocateBuffer - page - %x\n", Pages));
  DEBUG ((DEBUG_VERBOSE, "  mDmaBufferCurrentTop - %x\n", mDmaBufferCurrentTop));
  DEBUG ((DEBUG_VERBOSE, "  mDmaBufferCurrentBottom - %x\n", mDmaBufferCurrentBottom));

  Length = EFI_PAGES_TO_SIZE(Pages);
  if (Length > mDmaBufferCurrentTop - mDmaBufferCurrentBottom) {
    DEBUG ((DEBUG_ERROR, "PeiIoMmuAllocateBuffer - OUT_OF_RESOURCE\n"));
    ASSERT (FALSE);
    return EFI_OUT_OF_RESOURCES;
  }
  *HostAddress = (VOID *)(UINTN)(mDmaBufferCurrentTop - Length);
  mDmaBufferCurrentTop -= Length;

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

**/
EFI_STATUS
EFIAPI
PeiIoMmuFreeBuffer (
  IN  EDKII_IOMMU_PPI                          *This,
  IN  UINTN                                    Pages,
  IN  VOID                                     *HostAddress
  )
{
  UINTN  Length;

  DEBUG ((DEBUG_VERBOSE, "PeiIoMmuFreeBuffer - page - %x, HostAddr - %x\n", Pages, HostAddress));
  DEBUG ((DEBUG_VERBOSE, "  mDmaBufferCurrentTop - %x\n", mDmaBufferCurrentTop));
  DEBUG ((DEBUG_VERBOSE, "  mDmaBufferCurrentBottom - %x\n", mDmaBufferCurrentBottom));

  Length = EFI_PAGES_TO_SIZE(Pages);
  if ((UINTN)HostAddress == mDmaBufferCurrentTop) {
    mDmaBufferCurrentTop += Length;
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

#define MEMORY_ATTRIBUTE_MASK (EFI_RESOURCE_ATTRIBUTE_PRESENT | \
                               EFI_RESOURCE_ATTRIBUTE_INITIALIZED | \
                               EFI_RESOURCE_ATTRIBUTE_TESTED | \
                               EFI_RESOURCE_ATTRIBUTE_16_BIT_IO | \
                               EFI_RESOURCE_ATTRIBUTE_32_BIT_IO | \
                               EFI_RESOURCE_ATTRIBUTE_64_BIT_IO \
                               )

#define TESTED_MEMORY_ATTRIBUTES      (EFI_RESOURCE_ATTRIBUTE_PRESENT | EFI_RESOURCE_ATTRIBUTE_INITIALIZED | EFI_RESOURCE_ATTRIBUTE_TESTED)

#define INITIALIZED_MEMORY_ATTRIBUTES (EFI_RESOURCE_ATTRIBUTE_PRESENT | EFI_RESOURCE_ATTRIBUTE_INITIALIZED)

#define PRESENT_MEMORY_ATTRIBUTES     (EFI_RESOURCE_ATTRIBUTE_PRESENT)

GLOBAL_REMOVE_IF_UNREFERENCED CHAR8 *mResourceTypeShortName[] = {
  "Mem",
  "MMIO",
  "I/O",
  "FD",
  "MM Port I/O",
  "Reserved Mem",
  "Reserved I/O",
};

/**
  Return the short name of resource type.

  @param Type  resource type.

  @return the short name of resource type.
**/
CHAR8 *
ShortNameOfResourceType (
  IN UINT32 Type
  )
{
  if (Type < sizeof(mResourceTypeShortName) / sizeof(mResourceTypeShortName[0])) {
    return mResourceTypeShortName[Type];
  } else {
    return "Unknown";
  }
}

/**
  Dump resource hob.

  @param HobList  the HOB list.
**/
VOID
DumpResourceHob (
  IN VOID                        *HobList
  )
{
  EFI_PEI_HOB_POINTERS        Hob;
  EFI_HOB_RESOURCE_DESCRIPTOR *ResourceHob;

  DEBUG ((DEBUG_VERBOSE, "Resource Descriptor HOBs\n"));
  for (Hob.Raw = HobList; !END_OF_HOB_LIST (Hob); Hob.Raw = GET_NEXT_HOB (Hob)) {
    if (GET_HOB_TYPE (Hob) == EFI_HOB_TYPE_RESOURCE_DESCRIPTOR) {
      ResourceHob = Hob.ResourceDescriptor;
      DEBUG ((DEBUG_VERBOSE,
        "  BA=%016lx  L=%016lx  Attr=%08x  ",
        ResourceHob->PhysicalStart,
        ResourceHob->ResourceLength,
        ResourceHob->ResourceAttribute
        ));
      DEBUG ((DEBUG_VERBOSE, ShortNameOfResourceType(ResourceHob->ResourceType)));
      switch (ResourceHob->ResourceType) {
      case EFI_RESOURCE_SYSTEM_MEMORY:
        if ((ResourceHob->ResourceAttribute & EFI_RESOURCE_ATTRIBUTE_PERSISTENT) != 0) {
          DEBUG ((DEBUG_VERBOSE, " (Persistent)"));
        } else if ((ResourceHob->ResourceAttribute & EFI_RESOURCE_ATTRIBUTE_MORE_RELIABLE) != 0) {
          DEBUG ((DEBUG_VERBOSE, " (MoreReliable)"));
        } else if ((ResourceHob->ResourceAttribute & MEMORY_ATTRIBUTE_MASK) == TESTED_MEMORY_ATTRIBUTES) {
          DEBUG ((DEBUG_VERBOSE, " (Tested)"));
        } else if ((ResourceHob->ResourceAttribute & MEMORY_ATTRIBUTE_MASK) == INITIALIZED_MEMORY_ATTRIBUTES) {
          DEBUG ((DEBUG_VERBOSE, " (Init)"));
        } else if ((ResourceHob->ResourceAttribute & MEMORY_ATTRIBUTE_MASK) == PRESENT_MEMORY_ATTRIBUTES) {
          DEBUG ((DEBUG_VERBOSE, " (Present)"));
        } else {
          DEBUG ((DEBUG_VERBOSE, " (Unknown)"));
        }
        break;
      default:
        break;
      }
      DEBUG ((DEBUG_VERBOSE, "\n"));
    }
  }
}

/**
  Dump PHIT hob.

  @param HobList  the HOB list.
**/
VOID
DumpPhitHob (
  IN VOID                        *HobList
  )
{
  EFI_HOB_HANDOFF_INFO_TABLE  *PhitHob;

  PhitHob = HobList;
  ASSERT(GET_HOB_TYPE(HobList) == EFI_HOB_TYPE_HANDOFF);
  DEBUG ((DEBUG_VERBOSE, "PHIT HOB\n"));
  DEBUG ((DEBUG_VERBOSE, "  PhitHob             - 0x%x\n", PhitHob));
  DEBUG ((DEBUG_VERBOSE, "  BootMode            - 0x%x\n", PhitHob->BootMode));
  DEBUG ((DEBUG_VERBOSE, "  EfiMemoryTop        - 0x%016lx\n", PhitHob->EfiMemoryTop));
  DEBUG ((DEBUG_VERBOSE, "  EfiMemoryBottom     - 0x%016lx\n", PhitHob->EfiMemoryBottom));
  DEBUG ((DEBUG_VERBOSE, "  EfiFreeMemoryTop    - 0x%016lx\n", PhitHob->EfiFreeMemoryTop));
  DEBUG ((DEBUG_VERBOSE, "  EfiFreeMemoryBottom - 0x%016lx\n", PhitHob->EfiFreeMemoryBottom));
  DEBUG ((DEBUG_VERBOSE, "  EfiEndOfHobList     - 0x%lx\n", PhitHob->EfiEndOfHobList));
}

/**
  Get the highest memory.

  @param HobList  the HOB list.

  @return the highest memory.
**/
UINT64
GetTopMemory (
  IN VOID                        *HobList
  )
{
  EFI_PEI_HOB_POINTERS        Hob;
  EFI_HOB_RESOURCE_DESCRIPTOR *ResourceHob;
  UINT64                      TopMemory;
  UINT64                      ResourceTop;

  TopMemory = 0;
  for (Hob.Raw = HobList; !END_OF_HOB_LIST (Hob); Hob.Raw = GET_NEXT_HOB (Hob)) {
    if (GET_HOB_TYPE (Hob) == EFI_HOB_TYPE_RESOURCE_DESCRIPTOR) {
      ResourceHob = Hob.ResourceDescriptor;
      switch (ResourceHob->ResourceType) {
      case EFI_RESOURCE_SYSTEM_MEMORY:
        ResourceTop = ResourceHob->PhysicalStart + ResourceHob->ResourceLength;
        if (TopMemory < ResourceTop) {
          TopMemory = ResourceTop;
        }
        break;
      default:
        break;
      }
      DEBUG ((DEBUG_VERBOSE, "\n"));
    }
  }
  return TopMemory;
}

/**
  Initialize DMA protection.

  @param DmaBufferSize  the DMA buffer size
  @param DmaBufferBase  the DMA buffer base

  @retval EFI_SUCCESS           the DMA protection is initialized.
  @retval EFI_OUT_OF_RESOURCES  no enough resource to initialize DMA protection.
**/
EFI_STATUS
InitDmaProtection (
  IN   UINTN  DmaBufferSize,
  OUT  UINTN  *DmaBufferBase
  )
{
  EFI_STATUS                  Status;
  VOID                        *HobList;
  EFI_HOB_HANDOFF_INFO_TABLE  *PhitHob;
  UINT32                      LowMemoryAlignment;
  UINT64                      HighMemoryAlignment;
  UINTN                       MemoryAlignment;
  UINTN                       LowBottom;
  UINTN                       LowTop;
  UINTN                       HighBottom;
  UINT64                      HighTop;

  HobList = GetHobList ();
  DumpPhitHob (HobList);
  DumpResourceHob (HobList);

  PhitHob = HobList;

  ASSERT (PhitHob->EfiMemoryBottom < PhitHob->EfiMemoryTop);

  LowMemoryAlignment = GetLowMemoryAlignment ();
  HighMemoryAlignment = GetHighMemoryAlignment ();
  if (LowMemoryAlignment < HighMemoryAlignment) {
    MemoryAlignment = (UINTN)HighMemoryAlignment;
  } else {
    MemoryAlignment = LowMemoryAlignment;
  }
  ASSERT (DmaBufferSize == ALIGN_VALUE(DmaBufferSize, MemoryAlignment));
  *DmaBufferBase = (UINTN)AllocateAlignedPages (EFI_SIZE_TO_PAGES(DmaBufferSize), MemoryAlignment);
  if (*DmaBufferBase == 0) {
    DEBUG ((DEBUG_INFO, " InitDmaProtection : OutOfResource\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  LowBottom = 0;
  LowTop = *DmaBufferBase;
  HighBottom = *DmaBufferBase + DmaBufferSize;
  HighTop = GetTopMemory (HobList);

  Status = SetDmaProtectedRange (
               (UINT32)LowBottom,
               (UINT32)(LowTop - LowBottom),
               HighBottom,
               HighTop - HighBottom
               );

  if (EFI_ERROR(Status)) {
    FreePages ((VOID *)*DmaBufferBase, EFI_SIZE_TO_PAGES(DmaBufferSize));
  }

  return Status;
}

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

  if ((PcdGet8(PcdVTdPolicyPropertyMask) & BIT0) == 0) {
    return EFI_UNSUPPORTED;
  }

  Status = PeiServicesLocatePpi (
             &gEdkiiVTdInfoPpiGuid,
             0,
             NULL,
             (VOID **)&mVTdInfoPpi
             );
  ASSERT_EFI_ERROR(Status);

  //
  // Find a pre-memory in resource hob as DMA buffer
  // Mark PEI memory to be DMA protected.
  //
  Status = InitDmaProtection (mDmaBufferSize, &mDmaBufferBase);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  DEBUG ((DEBUG_INFO, " DmaBufferBase : 0x%x\n", mDmaBufferBase));
  DEBUG ((DEBUG_INFO, " DmaBufferSize : 0x%x\n", mDmaBufferSize));

  mDmaBufferCurrentTop = mDmaBufferBase + mDmaBufferSize;
  mDmaBufferCurrentBottom = mDmaBufferBase;

  //
  // Install PPI.
  //
  Status = PeiServicesInstallPpi (&mIoMmuPpiList);
  ASSERT_EFI_ERROR(Status);

  return Status;
}

