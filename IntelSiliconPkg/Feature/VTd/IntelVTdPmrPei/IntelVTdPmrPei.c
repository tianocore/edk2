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
#include <Ppi/VtdInfo.h>
#include <Ppi/EndOfPeiPhase.h>

#include "IntelVTdPmrPei.h"

#define  TOTAL_DMA_BUFFER_SIZE    SIZE_4MB
#define  TOTAL_DMA_BUFFER_SIZE_S3 SIZE_1MB

EFI_ACPI_DMAR_HEADER              *mAcpiDmarTable;
VTD_INFO                          *mVTdInfo;
UINT64                            mEngineMask;
UINTN                             mDmaBufferBase;
UINTN                             mDmaBufferSize;
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

              +------------------+ <=============== PHMR.Limit (Top of memory)
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
  =========== +==================+ <=============== PLMR.Limit
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

  @return the highest memory.
**/
UINT64
GetTopMemory (
  VOID
  )
{
  VOID                        *HobList;
  EFI_PEI_HOB_POINTERS        Hob;
  EFI_HOB_RESOURCE_DESCRIPTOR *ResourceHob;
  UINT64                      TopMemory;
  UINT64                      ResourceTop;

  HobList = GetHobList ();

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

  LowMemoryAlignment = GetLowMemoryAlignment (mEngineMask);
  HighMemoryAlignment = GetHighMemoryAlignment (mEngineMask);
  if (LowMemoryAlignment < HighMemoryAlignment) {
    MemoryAlignment = (UINTN)HighMemoryAlignment;
  } else {
    MemoryAlignment = LowMemoryAlignment;
  }
  ASSERT (DmaBufferSize == ALIGN_VALUE(DmaBufferSize, MemoryAlignment));
  *DmaBufferBase = (UINTN)AllocateAlignedPages (EFI_SIZE_TO_PAGES(DmaBufferSize), MemoryAlignment);
  ASSERT (*DmaBufferBase != 0);
  if (*DmaBufferBase == 0) {
    DEBUG ((DEBUG_INFO, " InitDmaProtection : OutOfResource\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  LowBottom = 0;
  LowTop = *DmaBufferBase;
  HighBottom = *DmaBufferBase + DmaBufferSize;
  HighTop = GetTopMemory ();

  Status = SetDmaProtectedRange (
               mEngineMask,
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
  Dump DMAR DeviceScopeEntry.

  @param[in]  DmarDeviceScopeEntry  DMAR DeviceScopeEntry
**/
VOID
DumpDmarDeviceScopeEntry (
  IN EFI_ACPI_DMAR_DEVICE_SCOPE_STRUCTURE_HEADER     *DmarDeviceScopeEntry
  )
{
  UINTN   PciPathNumber;
  UINTN   PciPathIndex;
  EFI_ACPI_DMAR_PCI_PATH  *PciPath;

  if (DmarDeviceScopeEntry == NULL) {
    return;
  }

  DEBUG ((DEBUG_INFO,
    "    *************************************************************************\n"
    ));
  DEBUG ((DEBUG_INFO,
    "    *       DMA-Remapping Device Scope Entry Structure                      *\n"
    ));
  DEBUG ((DEBUG_INFO,
    "    *************************************************************************\n"
    ));
  DEBUG ((DEBUG_INFO,
    (sizeof(UINTN) == sizeof(UINT64)) ?
    "    DMAR Device Scope Entry address ...................... 0x%016lx\n" :
    "    DMAR Device Scope Entry address ...................... 0x%08x\n",
    DmarDeviceScopeEntry
    ));
  DEBUG ((DEBUG_INFO,
    "      Device Scope Entry Type ............................ 0x%02x\n",
    DmarDeviceScopeEntry->Type
    ));
  switch (DmarDeviceScopeEntry->Type) {
  case EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_PCI_ENDPOINT:
    DEBUG ((DEBUG_INFO,
      "        PCI Endpoint Device\n"
      ));
    break;
  case EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_PCI_BRIDGE:
    DEBUG ((DEBUG_INFO,
      "        PCI Sub-hierachy\n"
      ));
    break;
  default:
    break;
  }
  DEBUG ((DEBUG_INFO,
    "      Length ............................................. 0x%02x\n",
    DmarDeviceScopeEntry->Length
    ));
  DEBUG ((DEBUG_INFO,
    "      Enumeration ID ..................................... 0x%02x\n",
    DmarDeviceScopeEntry->EnumerationId
    ));
  DEBUG ((DEBUG_INFO,
    "      Starting Bus Number ................................ 0x%02x\n",
    DmarDeviceScopeEntry->StartBusNumber
    ));

  PciPathNumber = (DmarDeviceScopeEntry->Length - sizeof(EFI_ACPI_DMAR_DEVICE_SCOPE_STRUCTURE_HEADER)) / sizeof(EFI_ACPI_DMAR_PCI_PATH);
  PciPath = (EFI_ACPI_DMAR_PCI_PATH *)(DmarDeviceScopeEntry + 1);
  for (PciPathIndex = 0; PciPathIndex < PciPathNumber; PciPathIndex++) {
    DEBUG ((DEBUG_INFO,
      "      Device ............................................. 0x%02x\n",
      PciPath[PciPathIndex].Device
      ));
    DEBUG ((DEBUG_INFO,
      "      Function ........................................... 0x%02x\n",
      PciPath[PciPathIndex].Function
      ));
  }

  DEBUG ((DEBUG_INFO,
    "    *************************************************************************\n\n"
    ));

  return;
}

/**
  Dump DMAR RMRR table.

  @param[in]  Rmrr  DMAR RMRR table
**/
VOID
DumpDmarRmrr (
  IN EFI_ACPI_DMAR_RMRR_HEADER *Rmrr
  )
{
  EFI_ACPI_DMAR_DEVICE_SCOPE_STRUCTURE_HEADER       *DmarDeviceScopeEntry;
  INTN                                    RmrrLen;

  if (Rmrr == NULL) {
    return;
  }

  DEBUG ((DEBUG_INFO,
    "  ***************************************************************************\n"
    ));
  DEBUG ((DEBUG_INFO,
    "  *       Reserved Memory Region Reporting Structure                        *\n"
    ));
  DEBUG ((DEBUG_INFO,
    "  ***************************************************************************\n"
    ));
  DEBUG ((DEBUG_INFO,
    (sizeof(UINTN) == sizeof(UINT64)) ?
    "  RMRR address ........................................... 0x%016lx\n" :
    "  RMRR address ........................................... 0x%08x\n",
    Rmrr
    ));
  DEBUG ((DEBUG_INFO,
    "    Type ................................................. 0x%04x\n",
    Rmrr->Header.Type
    ));
  DEBUG ((DEBUG_INFO,
    "    Length ............................................... 0x%04x\n",
    Rmrr->Header.Length
    ));
  DEBUG ((DEBUG_INFO,
    "    Segment Number ....................................... 0x%04x\n",
    Rmrr->SegmentNumber
    ));
  DEBUG ((DEBUG_INFO,
    "    Reserved Memory Region Base Address .................. 0x%016lx\n",
    Rmrr->ReservedMemoryRegionBaseAddress
    ));
  DEBUG ((DEBUG_INFO,
    "    Reserved Memory Region Limit Address ................. 0x%016lx\n",
    Rmrr->ReservedMemoryRegionLimitAddress
    ));

  RmrrLen  = Rmrr->Header.Length - sizeof(EFI_ACPI_DMAR_RMRR_HEADER);
  DmarDeviceScopeEntry = (EFI_ACPI_DMAR_DEVICE_SCOPE_STRUCTURE_HEADER *)(Rmrr + 1);
  while (RmrrLen > 0) {
    DumpDmarDeviceScopeEntry (DmarDeviceScopeEntry);
    RmrrLen -= DmarDeviceScopeEntry->Length;
    DmarDeviceScopeEntry = (EFI_ACPI_DMAR_DEVICE_SCOPE_STRUCTURE_HEADER *)((UINTN)DmarDeviceScopeEntry + DmarDeviceScopeEntry->Length);
  }

  DEBUG ((DEBUG_INFO,
    "  ***************************************************************************\n\n"
    ));

  return;
}

/**
  Dump DMAR DRHD table.

  @param[in]  Drhd  DMAR DRHD table
**/
VOID
DumpDmarDrhd (
  IN EFI_ACPI_DMAR_DRHD_HEADER *Drhd
  )
{
  EFI_ACPI_DMAR_DEVICE_SCOPE_STRUCTURE_HEADER       *DmarDeviceScopeEntry;
  INTN                                    DrhdLen;

  if (Drhd == NULL) {
    return;
  }

  DEBUG ((DEBUG_INFO,
    "  ***************************************************************************\n"
    ));
  DEBUG ((DEBUG_INFO,
    "  *       DMA-Remapping Hardware Definition Structure                       *\n"
    ));
  DEBUG ((DEBUG_INFO,
    "  ***************************************************************************\n"
    ));
  DEBUG ((DEBUG_INFO,
    (sizeof(UINTN) == sizeof(UINT64)) ?
    "  DRHD address ........................................... 0x%016lx\n" :
    "  DRHD address ........................................... 0x%08x\n",
    Drhd
    ));
  DEBUG ((DEBUG_INFO,
    "    Type ................................................. 0x%04x\n",
    Drhd->Header.Type
    ));
  DEBUG ((DEBUG_INFO,
    "    Length ............................................... 0x%04x\n",
    Drhd->Header.Length
    ));
  DEBUG ((DEBUG_INFO,
    "    Flags ................................................ 0x%02x\n",
    Drhd->Flags
    ));
  DEBUG ((DEBUG_INFO,
    "      INCLUDE_PCI_ALL .................................... 0x%02x\n",
    Drhd->Flags & EFI_ACPI_DMAR_DRHD_FLAGS_INCLUDE_PCI_ALL
    ));
  DEBUG ((DEBUG_INFO,
    "    Segment Number ....................................... 0x%04x\n",
    Drhd->SegmentNumber
    ));
  DEBUG ((DEBUG_INFO,
    "    Register Base Address ................................ 0x%016lx\n",
    Drhd->RegisterBaseAddress
    ));

  DrhdLen  = Drhd->Header.Length - sizeof(EFI_ACPI_DMAR_DRHD_HEADER);
  DmarDeviceScopeEntry = (EFI_ACPI_DMAR_DEVICE_SCOPE_STRUCTURE_HEADER *)(Drhd + 1);
  while (DrhdLen > 0) {
    DumpDmarDeviceScopeEntry (DmarDeviceScopeEntry);
    DrhdLen -= DmarDeviceScopeEntry->Length;
    DmarDeviceScopeEntry = (EFI_ACPI_DMAR_DEVICE_SCOPE_STRUCTURE_HEADER *)((UINTN)DmarDeviceScopeEntry + DmarDeviceScopeEntry->Length);
  }

  DEBUG ((DEBUG_INFO,
    "  ***************************************************************************\n\n"
    ));

  return;
}

/**
  Dump DMAR ACPI table.

  @param[in]  Dmar  DMAR ACPI table
**/
VOID
DumpAcpiDMAR (
  IN EFI_ACPI_DMAR_HEADER  *Dmar
  )
{
  EFI_ACPI_DMAR_STRUCTURE_HEADER *DmarHeader;
  INTN                  DmarLen;

  if (Dmar == NULL) {
    return;
  }

  //
  // Dump Dmar table
  //
  DEBUG ((DEBUG_INFO,
    "*****************************************************************************\n"
    ));
  DEBUG ((DEBUG_INFO,
    "*         DMAR Table                                                        *\n"
    ));
  DEBUG ((DEBUG_INFO,
    "*****************************************************************************\n"
    ));

  DEBUG ((DEBUG_INFO,
    (sizeof(UINTN) == sizeof(UINT64)) ?
    "DMAR address ............................................. 0x%016lx\n" :
    "DMAR address ............................................. 0x%08x\n",
    Dmar
    ));

  DEBUG ((DEBUG_INFO,
    "  Table Contents:\n"
    ));
  DEBUG ((DEBUG_INFO,
    "    Host Address Width ................................... 0x%02x\n",
    Dmar->HostAddressWidth
    ));
  DEBUG ((DEBUG_INFO,
    "    Flags ................................................ 0x%02x\n",
    Dmar->Flags
    ));
  DEBUG ((DEBUG_INFO,
    "      INTR_REMAP ......................................... 0x%02x\n",
    Dmar->Flags & EFI_ACPI_DMAR_FLAGS_INTR_REMAP
    ));
  DEBUG ((DEBUG_INFO,
    "      X2APIC_OPT_OUT_SET ................................. 0x%02x\n",
    Dmar->Flags & EFI_ACPI_DMAR_FLAGS_X2APIC_OPT_OUT
    ));

  DmarLen  = Dmar->Header.Length - sizeof(EFI_ACPI_DMAR_HEADER);
  DmarHeader = (EFI_ACPI_DMAR_STRUCTURE_HEADER *)(Dmar + 1);
  while (DmarLen > 0) {
    switch (DmarHeader->Type) {
    case EFI_ACPI_DMAR_TYPE_DRHD:
      DumpDmarDrhd ((EFI_ACPI_DMAR_DRHD_HEADER *)DmarHeader);
      break;
    case EFI_ACPI_DMAR_TYPE_RMRR:
      DumpDmarRmrr ((EFI_ACPI_DMAR_RMRR_HEADER *)DmarHeader);
      break;
    default:
      break;
    }
    DmarLen -= DmarHeader->Length;
    DmarHeader = (EFI_ACPI_DMAR_STRUCTURE_HEADER *)((UINTN)DmarHeader + DmarHeader->Length);
  }

  DEBUG ((DEBUG_INFO,
    "*****************************************************************************\n\n"
    ));

  return;
}

/**
  Get VTd engine number.

  @return the VTd engine number.
**/
UINTN
GetVtdEngineNumber (
  VOID
  )
{
  EFI_ACPI_DMAR_STRUCTURE_HEADER                    *DmarHeader;
  UINTN                                             VtdIndex;

  VtdIndex = 0;
  DmarHeader = (EFI_ACPI_DMAR_STRUCTURE_HEADER *)((UINTN)(mAcpiDmarTable + 1));
  while ((UINTN)DmarHeader < (UINTN)mAcpiDmarTable + mAcpiDmarTable->Header.Length) {
    switch (DmarHeader->Type) {
    case EFI_ACPI_DMAR_TYPE_DRHD:
      VtdIndex++;
      break;
    default:
      break;
    }
    DmarHeader = (EFI_ACPI_DMAR_STRUCTURE_HEADER *)((UINTN)DmarHeader + DmarHeader->Length);
  }
  return VtdIndex ;
}

/**
  Process DMAR DHRD table.

  @param[in]  VtdIndex  The index of VTd engine.
  @param[in]  DmarDrhd  The DRHD table.
**/
VOID
ProcessDhrd (
  IN UINTN                      VtdIndex,
  IN EFI_ACPI_DMAR_DRHD_HEADER  *DmarDrhd
  )
{
  DEBUG ((DEBUG_INFO,"  VTD (%d) BaseAddress -  0x%016lx\n", VtdIndex, DmarDrhd->RegisterBaseAddress));
  mVTdInfo->VTdEngineAddress[VtdIndex] = DmarDrhd->RegisterBaseAddress;
}

/**
  Parse DMAR DRHD table.

  @return EFI_SUCCESS  The DMAR DRHD table is parsed.
**/
EFI_STATUS
ParseDmarAcpiTableDrhd (
  VOID
  )
{
  EFI_ACPI_DMAR_STRUCTURE_HEADER                    *DmarHeader;
  UINTN                                             VtdUnitNumber;
  UINTN                                             VtdIndex;

  VtdUnitNumber = GetVtdEngineNumber ();
  if (VtdUnitNumber == 0) {
    return EFI_UNSUPPORTED;
  }

  mVTdInfo = AllocateZeroPool (sizeof(VTD_INFO) + (VtdUnitNumber - 1) * sizeof(UINT64));
  if (mVTdInfo == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  mVTdInfo->HostAddressWidth = mAcpiDmarTable->HostAddressWidth;
  mVTdInfo->VTdEngineCount   = VtdUnitNumber;

  VtdIndex = 0;
  DmarHeader = (EFI_ACPI_DMAR_STRUCTURE_HEADER *)((UINTN)(mAcpiDmarTable + 1));
  while ((UINTN)DmarHeader < (UINTN)mAcpiDmarTable + mAcpiDmarTable->Header.Length) {
    switch (DmarHeader->Type) {
    case EFI_ACPI_DMAR_TYPE_DRHD:
      ASSERT (VtdIndex < VtdUnitNumber);
      ProcessDhrd (VtdIndex, (EFI_ACPI_DMAR_DRHD_HEADER *)DmarHeader);
      VtdIndex++;

      break;

    default:
      break;
    }
    DmarHeader = (EFI_ACPI_DMAR_STRUCTURE_HEADER *)((UINTN)DmarHeader + DmarHeader->Length);
  }
  ASSERT (VtdIndex == VtdUnitNumber);

  //
  // Initialize the engine mask to all.
  //
  mEngineMask = LShiftU64 (1, VtdUnitNumber) - 1;

  return EFI_SUCCESS;
}

/**
  Return the VTd engine index according to the Segment and DevScopeEntry.

  @param Segment         The segment of the VTd engine
  @param DevScopeEntry   The DevScopeEntry of the VTd engine

  @return The VTd engine index according to the Segment and DevScopeEntry.
  @retval -1  The VTd engine is not found.
**/
UINTN
GetVTdEngineFromDevScopeEntry (
  IN  UINT16                                      Segment,
  IN  EFI_ACPI_DMAR_DEVICE_SCOPE_STRUCTURE_HEADER *DevScopeEntry
  )
{
  EFI_ACPI_DMAR_STRUCTURE_HEADER                    *DmarHeader;
  UINTN                                             VtdIndex;
  EFI_ACPI_DMAR_DRHD_HEADER                         *DmarDrhd;
  EFI_ACPI_DMAR_DEVICE_SCOPE_STRUCTURE_HEADER       *ThisDevScopeEntry;

  VtdIndex = 0;
  DmarHeader = (EFI_ACPI_DMAR_STRUCTURE_HEADER *)((UINTN)(mAcpiDmarTable + 1));
  while ((UINTN)DmarHeader < (UINTN)mAcpiDmarTable + mAcpiDmarTable->Header.Length) {
    switch (DmarHeader->Type) {
    case EFI_ACPI_DMAR_TYPE_DRHD:
      DmarDrhd = (EFI_ACPI_DMAR_DRHD_HEADER *)DmarHeader;
      if (DmarDrhd->SegmentNumber != Segment) {
        // Mismatch
        break;
      }
      if ((DmarDrhd->Header.Length == sizeof(EFI_ACPI_DMAR_DRHD_HEADER)) ||
          ((DmarDrhd->Flags & EFI_ACPI_DMAR_DRHD_FLAGS_INCLUDE_PCI_ALL) != 0)) {
        // No DevScopeEntry
        // Do not handle PCI_ALL
        break;
      }
      ThisDevScopeEntry = (EFI_ACPI_DMAR_DEVICE_SCOPE_STRUCTURE_HEADER *)((UINTN)(DmarDrhd + 1));
      while ((UINTN)ThisDevScopeEntry < (UINTN)DmarDrhd + DmarDrhd->Header.Length) {
        if ((ThisDevScopeEntry->Length == DevScopeEntry->Length) &&
            (CompareMem (ThisDevScopeEntry, DevScopeEntry, DevScopeEntry->Length) == 0)) {
          return VtdIndex;
        }
        ThisDevScopeEntry = (EFI_ACPI_DMAR_DEVICE_SCOPE_STRUCTURE_HEADER *)((UINTN)ThisDevScopeEntry + ThisDevScopeEntry->Length);
      }
      break;
    default:
      break;
    }
    DmarHeader = (EFI_ACPI_DMAR_STRUCTURE_HEADER *)((UINTN)DmarHeader + DmarHeader->Length);
  }
  return (UINTN)-1;
}

/**
  Process DMAR RMRR table.

  @param[in]  DmarRmrr  The RMRR table.
**/
VOID
ProcessRmrr (
  IN EFI_ACPI_DMAR_RMRR_HEADER  *DmarRmrr
  )
{
  EFI_ACPI_DMAR_DEVICE_SCOPE_STRUCTURE_HEADER       *DmarDevScopeEntry;
  UINTN                                             VTdIndex;
  UINT64                                            RmrrMask;
  UINTN                                             LowBottom;
  UINTN                                             LowTop;
  UINTN                                             HighBottom;
  UINT64                                            HighTop;

  DEBUG ((DEBUG_INFO,"  RMRR (Base 0x%016lx, Limit 0x%016lx)\n", DmarRmrr->ReservedMemoryRegionBaseAddress, DmarRmrr->ReservedMemoryRegionLimitAddress));

  if ((DmarRmrr->ReservedMemoryRegionBaseAddress == 0) ||
      (DmarRmrr->ReservedMemoryRegionLimitAddress == 0)) {
    return ;
  }

  DmarDevScopeEntry = (EFI_ACPI_DMAR_DEVICE_SCOPE_STRUCTURE_HEADER *)((UINTN)(DmarRmrr + 1));
  while ((UINTN)DmarDevScopeEntry < (UINTN)DmarRmrr + DmarRmrr->Header.Length) {
    ASSERT (DmarDevScopeEntry->Type == EFI_ACPI_DEVICE_SCOPE_ENTRY_TYPE_PCI_ENDPOINT);

    VTdIndex = GetVTdEngineFromDevScopeEntry (DmarRmrr->SegmentNumber, DmarDevScopeEntry);
    if (VTdIndex != (UINTN)-1) {
      RmrrMask = LShiftU64 (1, VTdIndex);

      LowBottom = 0;
      LowTop = (UINTN)DmarRmrr->ReservedMemoryRegionBaseAddress;
      HighBottom = (UINTN)DmarRmrr->ReservedMemoryRegionLimitAddress + 1;
      HighTop = GetTopMemory ();

      SetDmaProtectedRange (
        RmrrMask,
        0,
        (UINT32)(LowTop - LowBottom),
        HighBottom,
        HighTop - HighBottom
        );

      //
      // Remove the engine from the engine mask.
      // The assumption is that any other PEI driver does not access
      // the device covered by this engine.
      //
      mEngineMask = mEngineMask & (~RmrrMask);
    }

    DmarDevScopeEntry = (EFI_ACPI_DMAR_DEVICE_SCOPE_STRUCTURE_HEADER *)((UINTN)DmarDevScopeEntry + DmarDevScopeEntry->Length);
  }
}

/**
  Parse DMAR DRHD table.
**/
VOID
ParseDmarAcpiTableRmrr (
  VOID
  )
{
  EFI_ACPI_DMAR_STRUCTURE_HEADER                    *DmarHeader;

  DmarHeader = (EFI_ACPI_DMAR_STRUCTURE_HEADER *)((UINTN)(mAcpiDmarTable + 1));
  while ((UINTN)DmarHeader < (UINTN)mAcpiDmarTable + mAcpiDmarTable->Header.Length) {
    switch (DmarHeader->Type) {
    case EFI_ACPI_DMAR_TYPE_RMRR:
      ProcessRmrr ((EFI_ACPI_DMAR_RMRR_HEADER *)DmarHeader);
      break;
    default:
      break;
    }
    DmarHeader = (EFI_ACPI_DMAR_STRUCTURE_HEADER *)((UINTN)DmarHeader + DmarHeader->Length);
  }
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
  UINT64                      EngineMask;

  DEBUG((DEBUG_INFO, "VTdPmr S3EndOfPeiNotify\n"));

  if ((PcdGet8(PcdVTdPolicyPropertyMask) & BIT1) == 0) {
    EngineMask = LShiftU64 (1, mVTdInfo->VTdEngineCount) - 1;
    DisableDmaProtection (EngineMask);
  }
  return EFI_SUCCESS;
}

EFI_PEI_NOTIFY_DESCRIPTOR mS3EndOfPeiNotifyDesc = {
  (EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiEndOfPeiSignalPpiGuid,
  S3EndOfPeiNotify
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

  if ((PcdGet8(PcdVTdPolicyPropertyMask) & BIT0) == 0) {
    return EFI_UNSUPPORTED;
  }

  PeiServicesGetBootMode (&BootMode);

  Status = PeiServicesLocatePpi (
             &gEdkiiVTdInfoPpiGuid,
             0,
             NULL,
             (VOID **)&mAcpiDmarTable
             );
  ASSERT_EFI_ERROR(Status);

  DumpAcpiDMAR (mAcpiDmarTable);

  //
  // Get DMAR information to local VTdInfo
  //
  Status = ParseDmarAcpiTableDrhd ();
  if (EFI_ERROR(Status)) {
    return Status;
  }

  //
  // If there is RMRR memory, parse it here.
  //
  ParseDmarAcpiTableRmrr ();

  if (BootMode == BOOT_ON_S3_RESUME) {
    mDmaBufferSize = TOTAL_DMA_BUFFER_SIZE_S3;
  } else {
    mDmaBufferSize = TOTAL_DMA_BUFFER_SIZE;
  }
  DEBUG ((DEBUG_INFO, " DmaBufferSize : 0x%x\n", mDmaBufferSize));

  //
  // Find a pre-memory in resource hob as DMA buffer
  // Mark PEI memory to be DMA protected.
  //
  Status = InitDmaProtection (mDmaBufferSize, &mDmaBufferBase);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  DEBUG ((DEBUG_INFO, " DmaBufferBase : 0x%x\n", mDmaBufferBase));

  mDmaBufferCurrentTop = mDmaBufferBase + mDmaBufferSize;
  mDmaBufferCurrentBottom = mDmaBufferBase;

  //
  // Install PPI.
  //
  Status = PeiServicesInstallPpi (&mIoMmuPpiList);
  ASSERT_EFI_ERROR(Status);

  //
  // Register EndOfPei Notify for S3 to run FSP NotifyPhase
  //
  if (BootMode == BOOT_ON_S3_RESUME) {
    Status = PeiServicesNotifyPpi (&mS3EndOfPeiNotifyDesc);
    ASSERT_EFI_ERROR (Status);
  }

  return Status;
}

