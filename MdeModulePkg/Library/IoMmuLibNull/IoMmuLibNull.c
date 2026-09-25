/** @file IoMmuLib.c

    This file contains all the NULL implementation of the IoMmu protocol library functions.

    Copyright (c) Microsoft Corporation.<BR>
    SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Library/BaseLib.h>
#include <Library/IoMmuLib.h>

/**
  Returns True if the IoMmu protocol is available, otherwise returns False.
  This is a NULL implementation, so it always returns FALSE.

  @retval BOOLEAN    False, as this is a NULL implementation.
**/
BOOLEAN
EFIAPI
IoMmuIsPresent (
  VOID
  )
{
  return FALSE;
}

/**
  NULL implementation of IoMmuMap.

  @param [in]      Operation       The type of IOMMU operation.
  @param [in]      HostAddress     The host address to map.
  @param [in, out] NumberOfBytes   On input, the number of bytes to map. On output, the number of bytes mapped.
  @param [out]     DeviceAddress   The resulting device address.
  @param [out]     MappingInfo     A handle to the mapping. Used by IoMmuUnmap to unmap the address and IoMmuSetAttribute to set attributes.
                                   IoMmuMap allocates this memory, and it is be freed by IoMmuUnmap.

  @retval EFI_SUCCESS          NULL implementation, this function always succeeds.

**/
EFI_STATUS
EFIAPI
IoMmuMap (
  IN     EDKII_IOMMU_OPERATION  Operation,
  IN     VOID                   *HostAddress,
  IN OUT UINTN                  *NumberOfBytes,
  OUT    EFI_PHYSICAL_ADDRESS   *DeviceAddress,
  OUT    VOID                   **MappingInfo
  )
{
  return EFI_SUCCESS;
}

/**
  NULL implementation of IoMmuUnmap.

  @param [in]  MappingInfo   The mapping to unmap. This is the mapping that is returned from IoMmuMap.

  @retval EFI_SUCCESS          NULL implementation, this function always succeeds.

**/
EFI_STATUS
EFIAPI
IoMmuUnmap (
  IN  VOID  *MappingInfo
  )
{
  return EFI_SUCCESS;
}

/**
  NULL implementation of IoMmuFreeBuffer.

  @param [in]  Pages         The number of pages to free.
  @param [in]  HostAddress   The host address to free.

  @retval EFI_SUCCESS          NULL implementation, this function always succeeds.

**/
EFI_STATUS
EFIAPI
IoMmuFreeBuffer (
  IN  UINTN  Pages,
  IN  VOID   *HostAddress
  )
{
  return EFI_SUCCESS;
}

/**
  NULL implementation of IoMmuAllocateBuffer.

  @param [in]      Type          The type of allocation to perform.
  @param [in]      MemoryType    The type of memory to allocate.
  @param [in]      Pages         The number of pages to allocate.
  @param [in, out] HostAddress   On input, the desired host address. On output, the allocated host address.
  @param [in]      Attributes    The memory attributes to use for the allocation.

  @retval EFI_SUCCESS          NULL implementation, this function always succeeds.

**/
EFI_STATUS
EFIAPI
IoMmuAllocateBuffer (
  IN     EFI_ALLOCATE_TYPE  Type,
  IN     EFI_MEMORY_TYPE    MemoryType,
  IN     UINTN              Pages,
  IN OUT VOID               **HostAddress,
  IN     UINT64             Attributes
  )
{
  return EFI_SUCCESS;
}

/**
  NULL implementation of IoMmuSetAttribute.

  @param [in]  DeviceHandle  The device handle to set attributes for.
  @param [in]  MappingInfo   The mapping to set attributes for. This is the mapping that is returned from IoMmuMap.
  @param [in]  IoMmuAccess   The IOMMU access attributes.

  @retval EFI_SUCCESS          NULL implementation, this function always succeeds.

**/
EFI_STATUS
EFIAPI
IoMmuSetAttribute (
  IN EFI_HANDLE  DeviceHandle,
  IN VOID        *MappingInfo,
  IN UINT64      IoMmuAccess
  )
{
  return EFI_SUCCESS;
}

/**
  NULL implementation of IoMmuSetAttributeById.

  @param [in]  IommuBase     Base MMIO address of the IOMMU that owns DmaId.
  @param [in]  DmaId     DMA identifier emitted by the calling DMA agent (e.g. StreamID on Arm SMMU, RequesterID on VT-d).
  @param [in]  MappingInfo  The mapping to set attributes for.
  @param [in]  IoMmuAccess  The IOMMU access attributes.

  @retval EFI_SUCCESS       NULL implementation, this function always succeeds.
**/
EFI_STATUS
EFIAPI
IoMmuSetAttributeById (
  IN UINT64  IommuBase,
  IN UINT32  DmaId,
  IN VOID    *MappingInfo,
  IN UINT64  IoMmuAccess
  )
{
  return EFI_SUCCESS;
}
