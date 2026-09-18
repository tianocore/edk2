/** @file
  OVMF-specific IoMmuLib instance.

  This library switches behavior at runtime based on platform policy signaled
  by either gEdkiiIoMmuProtocolGuid (IOMMU present) or
  gIoMmuAbsentProtocolGuid (IOMMU intentionally absent). The library's INF
  declares a DEPEX on
  (gEdkiiIoMmuProtocolGuid OR gIoMmuAbsentProtocolGuid), so exactly one of
  those protocols is guaranteed to be published before this library's
  consumer dispatches.

  Copyright (c) Microsoft Corporation.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/IoMmuLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/IoMmu.h>

//
// Cached IoMmu protocol pointer set by IoMmuLibInit.
//   Non-NULL -> IOMMU present  (gEdkiiIoMmuProtocolGuid was located).
//   NULL     -> IOMMU absent   (gIoMmuAbsentProtocolGuid was located).
//
EDKII_IOMMU_PROTOCOL  *mIoMmuProtocol = NULL;

/**
  Returns True if the IoMmu protocol is available, otherwise returns False.

  @retval BOOLEAN    TRUE if the IoMmu protocol is available, FALSE otherwise.
**/
BOOLEAN
EFIAPI
IoMmuIsPresent (
  VOID
  )
{
  return (mIoMmuProtocol != NULL);
}

/**
  Map a host address to a device address using the Page Table.

  @param [in]      Operation       The type of IOMMU operation.
  @param [in]      HostAddress     The host address to map.
  @param [in, out] NumberOfBytes   On input, the number of bytes to map. On output, the number of bytes mapped.
  @param [out]     DeviceAddress   The resulting device address.
  @param [out]     MappingInfo     A double pointer to the mapping.

  @retval EFI_SUCCESS              Success (also returned when IOMMU is absent).
  @retval Other                    Errors as defined by the IoMmu protocol.
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
  if (mIoMmuProtocol == NULL) {
    return EFI_SUCCESS;
  }

  return mIoMmuProtocol->Map (mIoMmuProtocol, Operation, HostAddress, NumberOfBytes, DeviceAddress, MappingInfo);
}

/**
  Unmap a device address in the Page Table.

  @param [in]  MappingInfo   The mapping to unmap.

  @retval EFI_SUCCESS            Success (also returned when IOMMU is absent).
  @retval Other                  Errors as defined by the IoMmu protocol.
**/
EFI_STATUS
EFIAPI
IoMmuUnmap (
  IN  VOID  *MappingInfo
  )
{
  if (mIoMmuProtocol == NULL) {
    return EFI_SUCCESS;
  }

  return mIoMmuProtocol->Unmap (mIoMmuProtocol, MappingInfo);
}

/**
  Free a buffer allocated by IoMmuAllocateBuffer.

  @param [in]  Pages         The number of pages to free.
  @param [in]  HostAddress   The host address to free.

  @retval EFI_SUCCESS            Success (also returned when IOMMU is absent).
  @retval Other                  Errors as defined by the IoMmu protocol.
**/
EFI_STATUS
EFIAPI
IoMmuFreeBuffer (
  IN  UINTN  Pages,
  IN  VOID   *HostAddress
  )
{
  if (mIoMmuProtocol == NULL) {
    return EFI_SUCCESS;
  }

  return mIoMmuProtocol->FreeBuffer (mIoMmuProtocol, Pages, HostAddress);
}

/**
  Allocate a buffer for DMA use with the IoMmu.

  @param [in]      Type          The type of allocation to perform.
  @param [in]      MemoryType    The type of memory to allocate.
  @param [in]      Pages         The number of pages to allocate.
  @param [in, out] HostAddress   On input, the desired host address. On output, the allocated host address.
  @param [in]      Attributes    The memory attributes to use for the allocation.

  @retval EFI_SUCCESS           Success (also returned when IOMMU is absent).
  @retval Other                 Errors as defined by the IoMmu protocol.
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
  if (mIoMmuProtocol == NULL) {
    return EFI_SUCCESS;
  }

  return mIoMmuProtocol->AllocateBuffer (mIoMmuProtocol, Type, MemoryType, Pages, HostAddress, Attributes);
}

/**
  Set the R/W access attributes for MappingInfo in the Page Table.

  @param [in]  DeviceHandle  The device handle to set attributes for.
  @param [in]  MappingInfo   The mapping to set attributes for.
  @param [in]  IoMmuAccess   The IOMMU access attributes.

  @retval EFI_SUCCESS            Success (also returned when IOMMU is absent).
  @retval Other                  Errors as defined by the IoMmu protocol.
**/
EFI_STATUS
EFIAPI
IoMmuSetAttribute (
  IN EFI_HANDLE  DeviceHandle,
  IN VOID        *MappingInfo,
  IN UINT64      IoMmuAccess
  )
{
  if (mIoMmuProtocol == NULL) {
    return EFI_SUCCESS;
  }

  return mIoMmuProtocol->SetAttribute (mIoMmuProtocol, DeviceHandle, MappingInfo, IoMmuAccess);
}

/**
  Set the R/W access attributes for MappingInfo in the Page Table for a caller
  that explicitly specifies (IommuBase, DmaId) instead of an EFI_HANDLE.

  @param [in]  IommuBase     Base MMIO address of the IOMMU that owns DmaId.
  @param [in]  DmaId         DMA identifier emitted by the calling DMA agent.
  @param [in]  MappingInfo   The mapping to set attributes for.
  @param [in]  IoMmuAccess   The IOMMU access attributes.

  @retval EFI_SUCCESS        Success (also returned when IOMMU is absent).
  @retval EFI_UNSUPPORTED    The IoMmu protocol does not implement SetAttributeById.
  @retval Other              Errors as defined by the IoMmu protocol.
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
  if (mIoMmuProtocol == NULL) {
    return EFI_SUCCESS;
  }

  if ((mIoMmuProtocol->Revision < EDKII_IOMMU_PROTOCOL_REVISION) || (mIoMmuProtocol->SetAttributeById == NULL)) {
    DEBUG ((DEBUG_WARN, "%a: SetAttributeById not implemented by IoMmu producer.\n", __func__));
    return EFI_UNSUPPORTED;
  }

  return mIoMmuProtocol->SetAttributeById (mIoMmuProtocol, IommuBase, DmaId, MappingInfo, IoMmuAccess);
}

/**
  Constructor for OVMF IoMmuLib.

  Locates whichever of gEdkiiIoMmuProtocolGuid or gIoMmuAbsentProtocolGuid was
  published by IoMmuDxe and caches the runtime mode. The library's INF declares
  a DEPEX on either GUID, so at least one of them is guaranteed to be present
  by the time this constructor runs.

  @param[in]  ImageHandle  The firmware allocated handle for the EFI image.
  @param[in]  SystemTable  A pointer to the EFI System Table.

  @retval EFI_SUCCESS  The constructor completed successfully.
  @retval Other        Neither protocol was located despite the DEPEX.
**/
EFI_STATUS
EFIAPI
IoMmuLibInit (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  VOID        *AbsentInterface;

  Status = gBS->LocateProtocol (&gEdkiiIoMmuProtocolGuid, NULL, (VOID **)&mIoMmuProtocol);
  if (!EFI_ERROR (Status) && (mIoMmuProtocol != NULL)) {
    DEBUG ((DEBUG_VERBOSE, "%a: gEdkiiIoMmuProtocolGuid located; IOMMU present.\n", __func__));
    return EFI_SUCCESS;
  }

  mIoMmuProtocol = NULL;

  Status = gBS->LocateProtocol (&gIoMmuAbsentProtocolGuid, NULL, &AbsentInterface);
  if (!EFI_ERROR (Status)) {
    DEBUG ((DEBUG_VERBOSE, "%a: gIoMmuAbsentProtocolGuid located; IOMMU absent.\n", __func__));
    return EFI_SUCCESS;
  }

  DEBUG ((DEBUG_ERROR, "%a: Neither IoMmu nor IoMmuAbsent protocol found despite DEPEX. Status = %r\n", __func__, Status));
  ASSERT_EFI_ERROR (Status);
  return Status;
}
