/** @file
The DMA memory help functions.

Copyright (c) 2017, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "EhcPeim.h"

/**
  Provides the controller-specific addresses required to access system memory from a
  DMA bus master.

  @param IoMmu                  Pointer to IOMMU PPI.
  @param Operation              Indicates if the bus master is going to read or write to system memory.
  @param HostAddress            The system memory address to map to the PCI controller.
  @param NumberOfBytes          On input the number of bytes to map. On output the number of bytes
                                that were mapped.
  @param DeviceAddress          The resulting map address for the bus master PCI controller to use to
                                access the hosts HostAddress.
  @param Mapping                A resulting value to pass to Unmap().

  @retval EFI_SUCCESS           The range was mapped for the returned NumberOfBytes.
  @retval EFI_UNSUPPORTED       The HostAddress cannot be mapped as a common buffer.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack of resources.
  @retval EFI_DEVICE_ERROR      The system hardware could not map the requested address.

**/
EFI_STATUS
IoMmuMap (
  IN EDKII_IOMMU_PPI        *IoMmu,
  IN EDKII_IOMMU_OPERATION  Operation,
  IN VOID                   *HostAddress,
  IN OUT UINTN              *NumberOfBytes,
  OUT EFI_PHYSICAL_ADDRESS  *DeviceAddress,
  OUT VOID                  **Mapping
  )
{
  EFI_STATUS  Status;
  UINT64      Attribute;

  if (IoMmu != NULL) {
    Status = IoMmu->Map (
                      IoMmu,
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

    Status = IoMmu->SetAttribute (
                      IoMmu,
                      *Mapping,
                      Attribute
                      );
    if (EFI_ERROR (Status)) {
      IoMmu->Unmap (IoMmu, Mapping);
      *Mapping = NULL;
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

  @param IoMmu              Pointer to IOMMU PPI.
  @param Mapping            The mapping value returned from Map().

**/
VOID
IoMmuUnmap (
  IN EDKII_IOMMU_PPI  *IoMmu,
  IN VOID             *Mapping
  )
{
  if (IoMmu != NULL) {
    IoMmu->SetAttribute (IoMmu, Mapping, 0);
    IoMmu->Unmap (IoMmu, Mapping);
  }
}

/**
  Allocates pages that are suitable for an OperationBusMasterCommonBuffer or
  OperationBusMasterCommonBuffer64 mapping.

  @param IoMmu                  Pointer to IOMMU PPI.
  @param Pages                  The number of pages to allocate.
  @param HostAddress            A pointer to store the base system memory address of the
                                allocated range.
  @param DeviceAddress          The resulting map address for the bus master PCI controller to use to
                                access the hosts HostAddress.
  @param Mapping                A resulting value to pass to Unmap().

  @retval EFI_SUCCESS           The requested memory pages were allocated.
  @retval EFI_UNSUPPORTED       Attributes is unsupported. The only legal attribute bits are
                                MEMORY_WRITE_COMBINE and MEMORY_CACHED.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES  The memory pages could not be allocated.

**/
EFI_STATUS
IoMmuAllocateBuffer (
  IN EDKII_IOMMU_PPI        *IoMmu,
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
  *Mapping       = NULL;

  if (IoMmu != NULL) {
    Status = IoMmu->AllocateBuffer (
                      IoMmu,
                      EfiBootServicesData,
                      Pages,
                      HostAddress,
                      0
                      );
    if (EFI_ERROR (Status)) {
      return EFI_OUT_OF_RESOURCES;
    }

    NumberOfBytes = EFI_PAGES_TO_SIZE (Pages);
    Status        = IoMmu->Map (
                             IoMmu,
                             EdkiiIoMmuOperationBusMasterCommonBuffer,
                             *HostAddress,
                             &NumberOfBytes,
                             DeviceAddress,
                             Mapping
                             );
    if (EFI_ERROR (Status)) {
      IoMmu->FreeBuffer (IoMmu, Pages, *HostAddress);
      *HostAddress = NULL;
      return EFI_OUT_OF_RESOURCES;
    }

    Status = IoMmu->SetAttribute (
                      IoMmu,
                      *Mapping,
                      EDKII_IOMMU_ACCESS_READ | EDKII_IOMMU_ACCESS_WRITE
                      );
    if (EFI_ERROR (Status)) {
      IoMmu->Unmap (IoMmu, *Mapping);
      IoMmu->FreeBuffer (IoMmu, Pages, *HostAddress);
      *Mapping     = NULL;
      *HostAddress = NULL;
      return Status;
    }
  } else {
    Status = PeiServicesAllocatePages (
               EfiBootServicesCode,
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

  @param IoMmu              Pointer to IOMMU PPI.
  @param Pages              The number of pages to free.
  @param HostAddress        The base system memory address of the allocated range.
  @param Mapping            The mapping value returned from Map().

**/
VOID
IoMmuFreeBuffer (
  IN EDKII_IOMMU_PPI  *IoMmu,
  IN UINTN            Pages,
  IN VOID             *HostAddress,
  IN VOID             *Mapping
  )
{
  if (IoMmu != NULL) {
    IoMmu->SetAttribute (IoMmu, Mapping, 0);
    IoMmu->Unmap (IoMmu, Mapping);
    IoMmu->FreeBuffer (IoMmu, Pages, HostAddress);
  }
}

/**
  Initialize IOMMU.

  @param IoMmu              Pointer to pointer to IOMMU PPI.

**/
VOID
IoMmuInit (
  OUT EDKII_IOMMU_PPI  **IoMmu
  )
{
  PeiServicesLocatePpi (
    &gEdkiiIoMmuPpiGuid,
    0,
    NULL,
    (VOID **)IoMmu
    );
}
