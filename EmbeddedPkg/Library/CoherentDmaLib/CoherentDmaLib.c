/** @file
  Generic ARM implementation of DmaLib.h

  Copyright (c) 2008 - 2010, Apple Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Library/DmaLib.h>
#include <Library/IoMmuLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Protocol/IoMmu.h>

typedef struct {
  VOID      *IoMmuContext;
  UINT64    IommuBase;
  UINT32    DmaId;
} COHERENT_MAP_INFO;

STATIC
PHYSICAL_ADDRESS
HostToDeviceAddress (
  IN  VOID  *Address
  )
{
  return (PHYSICAL_ADDRESS)(UINTN)Address + PcdGet64 (PcdDmaDeviceOffset);
}

/**
  Translate a DMA bus master operation into the corresponding IOMMU operation
  and access attributes, install an IOMMU mapping for IoMmuHostAddress and set
  the access attributes on the resulting mapping via (Map->IommuBase, Map->DmaId).

  On success, Map->IoMmuContext holds the mapping handle that DmaUnmapIoMmu()
  can later release. On failure, no IOMMU mapping is leaked.

  @param[in]      Operation         The DMA bus master operation.
  @param[in]      IoMmuHostAddress  Host address to be mapped through the IOMMU.
  @param[in,out]  NumberOfBytes     Length of the mapping.
  @param[in,out]  DeviceAddress     Resulting device address.
  @param[in,out]  Map               Map info; Map->IoMmuContext is populated on
                                    success.

  @retval EFI_SUCCESS            Mapping established and attributes set.
  @retval EFI_INVALID_PARAMETER  Operation is not a valid bus master operation.
  @retval Other                  Error returned by the IOMMU library.
**/
STATIC
EFI_STATUS
DmaMapIoMmu (
  IN     DMA_MAP_OPERATION  Operation,
  IN     VOID               *IoMmuHostAddress,
  IN OUT UINTN              *NumberOfBytes,
  IN OUT PHYSICAL_ADDRESS   *DeviceAddress,
  IN OUT COHERENT_MAP_INFO  *Map
  )
{
  EFI_STATUS             Status;
  EDKII_IOMMU_OPERATION  IoMmuOperation;
  UINT64                 IoMmuAttribute;

  if ((Operation >= MapOperationMaximum) || (IoMmuHostAddress == NULL) ||
      (NumberOfBytes == NULL) || (DeviceAddress == NULL) || (Map == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  switch (Operation) {
    case MapOperationBusMasterRead:
      IoMmuOperation = EdkiiIoMmuOperationBusMasterRead;
      IoMmuAttribute = EDKII_IOMMU_ACCESS_READ;
      break;

    case MapOperationBusMasterWrite:
      IoMmuOperation = EdkiiIoMmuOperationBusMasterWrite;
      IoMmuAttribute = EDKII_IOMMU_ACCESS_WRITE;
      break;

    case MapOperationBusMasterCommonBuffer:
      IoMmuOperation = EdkiiIoMmuOperationBusMasterCommonBuffer;
      IoMmuAttribute = (EDKII_IOMMU_ACCESS_READ | EDKII_IOMMU_ACCESS_WRITE);
      break;

    default:
      DEBUG ((DEBUG_ERROR, "%a - Invalid operation %d\n", __func__, Operation));
      ASSERT (FALSE);
      return EFI_INVALID_PARAMETER;
  }

  Status = IoMmuMap (
             IoMmuOperation,
             IoMmuHostAddress,
             NumberOfBytes,
             DeviceAddress,
             &Map->IoMmuContext
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - IoMmuMap failed.\n", __func__));
    ASSERT (FALSE);
    return Status;
  }

  Status = IoMmuSetAttributeById (
             Map->IommuBase,
             Map->DmaId,
             Map->IoMmuContext,
             IoMmuAttribute
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - IoMmuSetAttributeById failed.\n", __func__));
    ASSERT (FALSE);
    if (EFI_ERROR (IoMmuUnmap (Map->IoMmuContext))) {
      DEBUG ((DEBUG_ERROR, "%a - IoMmuUnmap failed.\n", __func__));
      ASSERT (FALSE);
    }

    Map->IoMmuContext = NULL;
  }

  return Status;
}

/**
  Reverse a previous DmaMapIoMmu() call by clearing the IOMMU access attributes
  and tearing down the mapping.

  @param[in] Map  Map info populated by DmaMapIoMmu().

  @retval EFI_SUCCESS  The mapping has been released.
  @retval Other        Error returned by the IOMMU library.
**/
STATIC
EFI_STATUS
DmaUnmapIoMmu (
  IN COHERENT_MAP_INFO  *Map
  )
{
  EFI_STATUS  Status;

  if ((Map == NULL)) {
    return EFI_SUCCESS;
  }

  Status = IoMmuSetAttributeById (Map->IommuBase, Map->DmaId, Map->IoMmuContext, 0);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - IoMmuSetAttributeById failed.\n", __func__));
    ASSERT (FALSE);
    return Status;
  }

  Status = IoMmuUnmap (Map->IoMmuContext);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a - IoMmuUnmap failed.\n", __func__));
    ASSERT (FALSE);
    return Status;
  }

  return EFI_SUCCESS;
}

/**
  Provides the DMA controller-specific addresses needed to access system memory.

  Operation is relative to the DMA bus master.

  @param  Operation             Indicates if the bus master is going to read or write to system memory.
  @param  HostAddress           The system memory address to map to the DMA controller.
  @param  NumberOfBytes         On input the number of bytes to map. On output the number of bytes
                                that were mapped.
  @param  DeviceAddress         The resulting map address for the bus master controller to use to
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
DmaMap (
  IN     DMA_MAP_OPERATION  Operation,
  IN     VOID               *HostAddress,
  IN OUT UINTN              *NumberOfBytes,
  IN     UINT64             IommuBase,
  IN     UINT32             DmaId,
  OUT    PHYSICAL_ADDRESS   *DeviceAddress,
  OUT    VOID               **Mapping
  )
{
  EFI_STATUS         Status;
  VOID               *IoMmuHostAddress;
  COHERENT_MAP_INFO  *Map;

  if ((HostAddress == NULL) ||
      (NumberOfBytes == NULL) ||
      (DeviceAddress == NULL) ||
      (Mapping == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  *DeviceAddress   = HostToDeviceAddress (HostAddress);
  IoMmuHostAddress = HostAddress;
  *Mapping         = NULL;

  if (IommuBase == 0) {
    // No IoMmu
    return EFI_SUCCESS;
  }

  Map = AllocateZeroPool (sizeof (COHERENT_MAP_INFO));
  if (Map == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Map->IommuBase = IommuBase;
  Map->DmaId     = DmaId;

  Status = DmaMapIoMmu (
             Operation,
             IoMmuHostAddress,
             NumberOfBytes,
             DeviceAddress,
             Map
             );
  if (EFI_ERROR (Status)) {
    FreePool (Map);
    return Status;
  }

  *Mapping = Map;
  return EFI_SUCCESS;
}

/**
  Completes the DmaMapBusMasterRead(), DmaMapBusMasterWrite(), or DmaMapBusMasterCommonBuffer()
  operation and releases any corresponding resources.

  @param  Mapping               The mapping value returned from DmaMap*().

  @retval EFI_SUCCESS           The range was unmapped.
  @retval EFI_DEVICE_ERROR      The data was not committed to the target system memory.

**/
EFI_STATUS
EFIAPI
DmaUnmap (
  IN  VOID  *Mapping
  )
{
  EFI_STATUS         Status;
  COHERENT_MAP_INFO  *Map;

  if (Mapping == NULL) {
    return EFI_SUCCESS;
  }

  Map = (COHERENT_MAP_INFO *)Mapping;

  Status = DmaUnmapIoMmu (Map);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  FreePool (Map);
  return EFI_SUCCESS;
}

/**
  Allocates pages that are suitable for an DmaMap() of type MapOperationBusMasterCommonBuffer.
  mapping.

  @param  MemoryType            The type of memory to allocate, EfiBootServicesData or
                                EfiRuntimeServicesData.
  @param  Pages                 The number of pages to allocate.
  @param  HostAddress           A pointer to store the base system memory address of the
                                allocated range.

                                @retval EFI_SUCCESS           The requested memory pages were allocated.
  @retval EFI_UNSUPPORTED       Attributes is unsupported. The only legal attribute bits are
                                MEMORY_WRITE_COMBINE and MEMORY_CACHED.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES  The memory pages could not be allocated.

**/
EFI_STATUS
EFIAPI
DmaAllocateBuffer (
  IN  EFI_MEMORY_TYPE  MemoryType,
  IN  UINTN            Pages,
  OUT VOID             **HostAddress
  )
{
  return DmaAllocateAlignedBuffer (MemoryType, Pages, 0, HostAddress);
}

/**
  Allocates pages that are suitable for an DmaMap() of type
  MapOperationBusMasterCommonBuffer mapping, at the requested alignment.

  @param  MemoryType            The type of memory to allocate, EfiBootServicesData or
                                EfiRuntimeServicesData.
  @param  Pages                 The number of pages to allocate.
  @param  Alignment             Alignment in bytes of the base of the returned
                                buffer (must be a power of 2)
  @param  HostAddress           A pointer to store the base system memory address of the
                                allocated range.

  @retval EFI_SUCCESS           The requested memory pages were allocated.
  @retval EFI_UNSUPPORTED       Attributes is unsupported. The only legal attribute bits are
                                MEMORY_WRITE_COMBINE and MEMORY_CACHED.
  @retval EFI_INVALID_PARAMETER One or more parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES  The memory pages could not be allocated.

**/
EFI_STATUS
EFIAPI
DmaAllocateAlignedBuffer (
  IN  EFI_MEMORY_TYPE  MemoryType,
  IN  UINTN            Pages,
  IN  UINTN            Alignment,
  OUT VOID             **HostAddress
  )
{
  if (Alignment == 0) {
    Alignment = EFI_PAGE_SIZE;
  }

  if ((HostAddress == NULL) ||
      ((Alignment & (Alignment - 1)) != 0))
  {
    return EFI_INVALID_PARAMETER;
  }

  //
  // The only valid memory types are EfiBootServicesData and EfiRuntimeServicesData
  //
  if (MemoryType == EfiBootServicesData) {
    *HostAddress = AllocateAlignedPages (Pages, Alignment);
  } else if (MemoryType == EfiRuntimeServicesData) {
    *HostAddress = AllocateAlignedRuntimePages (Pages, Alignment);
  } else {
    return EFI_INVALID_PARAMETER;
  }

  if (*HostAddress == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  return EFI_SUCCESS;
}

/**
  Frees memory that was allocated with DmaAllocateBuffer().

  @param  Pages                 The number of pages to free.
  @param  HostAddress           The base system memory address of the allocated range.

  @retval EFI_SUCCESS           The requested memory pages were freed.
  @retval EFI_INVALID_PARAMETER The memory range specified by HostAddress and Pages
                                was not allocated with DmaAllocateBuffer().

**/
EFI_STATUS
EFIAPI
DmaFreeBuffer (
  IN  UINTN  Pages,
  IN  VOID   *HostAddress
  )
{
  if (HostAddress == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  FreePages (HostAddress, Pages);
  return EFI_SUCCESS;
}
