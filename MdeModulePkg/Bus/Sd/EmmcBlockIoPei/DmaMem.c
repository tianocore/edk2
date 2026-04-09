/** @file
  The DMA memory help function.

  Copyright (c) 2017, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "EmmcBlockIoPei.h"

EDKII_IOMMU_PPI  *mIoMmu;

/**
  Provides the controller-specific addresses required to access system memory from a
  DMA bus master.

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
IoMmuMap (
  IN  EDKII_IOMMU_OPERATION  Operation,
  IN VOID                    *HostAddress,
  IN  OUT UINTN              *NumberOfBytes,
  OUT EFI_PHYSICAL_ADDRESS   *DeviceAddress,
  OUT VOID                   **Mapping
  )
{
  EFI_STATUS  Status;
  UINT64      Attribute;

  if (mIoMmu != NULL) {
    Status = mIoMmu->Map (
                       mIoMmu,
                       Operation,
                       HostAddress,
                       NumberOfBytes,
                       DeviceAddress,
                       Mapping
                       );
    if (EFI_ERROR (Status)) {
      return EFI_OUT_OF_RESOURCES;
    }

    switch (Operation) {
      case EdkiiIoMmuOperationBusMasterRead:
      case EdkiiIoMmuOperationBusMasterRead64:
        Attribute = EDKII_IOMMU_ACCESS_READ;
        break;
      case EdkiiIoMmuOperationBusMasterWrite:
      case EdkiiIoMmuOperationBusMasterWrite64:
        Attribute = EDKII_IOMMU_ACCESS_WRITE;
        break;
      case EdkiiIoMmuOperationBusMasterCommonBuffer:
      case EdkiiIoMmuOperationBusMasterCommonBuffer64:
        Attribute = EDKII_IOMMU_ACCESS_READ | EDKII_IOMMU_ACCESS_WRITE;
        break;
      default:
        ASSERT (FALSE);
        return EFI_INVALID_PARAMETER;
    }

    Status = mIoMmu->SetAttribute (
                       mIoMmu,
                       *Mapping,
                       Attribute
                       );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  } else {
    *DeviceAddress = (EFI_PHYSICAL_ADDRESS)(UINTN)HostAddress;
    *Mapping       = NULL;
    Status         = EFI_SUCCESS;
  }

  return Status;
}

/**
  Completes the Map() operation and releases any corresponding resources.

  @param  Mapping               The mapping value returned from Map().

  @retval EFI_SUCCESS           The range was unmapped.
  @retval EFI_INVALID_PARAMETER Mapping is not a value that was returned by Map().
  @retval EFI_DEVICE_ERROR      The data was not committed to the target system memory.
**/
EFI_STATUS
IoMmuUnmap (
  IN VOID  *Mapping
  )
{
  EFI_STATUS  Status;

  if (mIoMmu != NULL) {
    Status = mIoMmu->SetAttribute (mIoMmu, Mapping, 0);
    Status = mIoMmu->Unmap (mIoMmu, Mapping);
  } else {
    Status = EFI_SUCCESS;
  }

  return Status;
}

/**
  Allocates pages that are suitable for an OperationBusMasterCommonBuffer or
  OperationBusMasterCommonBuffer64 mapping.

  @param  Pages                 The number of pages to allocate.
  @param  HostAddress           A pointer to store the base system memory address of the
                                allocated range.
  @param  DeviceAddress         The resulting map address for the bus master PCI controller to use to
                                access the hosts HostAddress.
  @param  Mapping               A resulting value to pass to Unmap().

  @retval EFI_SUCCESS           The requested memory pages were allocated.
  @retval EFI_UNSUPPORTED       Attributes is unsupported. The only legal attribute bits are
                                MEMORY_WRITE_COMBINE and MEMORY_CACHED.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES  The memory pages could not be allocated.

**/
EFI_STATUS
IoMmuAllocateBuffer (
  IN UINTN                  Pages,
  OUT VOID                  **HostAddress,
  OUT EFI_PHYSICAL_ADDRESS  *DeviceAddress,
  OUT VOID                  **Mapping
  )
{
  EFI_STATUS            Status;
  UINTN                 NumberOfBytes;
  EFI_PHYSICAL_ADDRESS  HostPhyAddress;

  *HostAddress   = NULL;
  *DeviceAddress = 0;

  if (mIoMmu != NULL) {
    Status = mIoMmu->AllocateBuffer (
                       mIoMmu,
                       EfiBootServicesData,
                       Pages,
                       HostAddress,
                       0
                       );
    if (EFI_ERROR (Status)) {
      return EFI_OUT_OF_RESOURCES;
    }

    NumberOfBytes = EFI_PAGES_TO_SIZE (Pages);
    Status        = mIoMmu->Map (
                              mIoMmu,
                              EdkiiIoMmuOperationBusMasterCommonBuffer,
                              *HostAddress,
                              &NumberOfBytes,
                              DeviceAddress,
                              Mapping
                              );
    if (EFI_ERROR (Status)) {
      return EFI_OUT_OF_RESOURCES;
    }

    Status = mIoMmu->SetAttribute (
                       mIoMmu,
                       *Mapping,
                       EDKII_IOMMU_ACCESS_READ | EDKII_IOMMU_ACCESS_WRITE
                       );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  } else {
    Status = PeiServicesAllocatePages (
               EfiBootServicesData,
               Pages,
               &HostPhyAddress
               );
    if (EFI_ERROR (Status)) {
      return EFI_OUT_OF_RESOURCES;
    }

    *HostAddress   = (VOID *)(UINTN)HostPhyAddress;
    *DeviceAddress = HostPhyAddress;
    *Mapping       = NULL;
  }

  return Status;
}

/**
  Frees memory that was allocated with AllocateBuffer().

  @param  Pages                 The number of pages to free.
  @param  HostAddress           The base system memory address of the allocated range.
  @param  Mapping               The mapping value returned from Map().

  @retval EFI_SUCCESS           The requested memory pages were freed.
  @retval EFI_INVALID_PARAMETER The memory range specified by HostAddress and Pages
                                was not allocated with AllocateBuffer().

**/
EFI_STATUS
IoMmuFreeBuffer (
  IN UINTN  Pages,
  IN VOID   *HostAddress,
  IN VOID   *Mapping
  )
{
  EFI_STATUS  Status;

  if (mIoMmu != NULL) {
    Status = mIoMmu->SetAttribute (mIoMmu, Mapping, 0);
    Status = mIoMmu->Unmap (mIoMmu, Mapping);
    Status = mIoMmu->FreeBuffer (mIoMmu, Pages, HostAddress);
  } else {
    Status = EFI_SUCCESS;
  }

  return Status;
}

/**
  Initialize IOMMU.
**/
VOID
IoMmuInit (
  VOID
  )
{
  PeiServicesLocatePpi (
    &gEdkiiIoMmuPpiGuid,
    0,
    NULL,
    (VOID **)&mIoMmu
    );
}
