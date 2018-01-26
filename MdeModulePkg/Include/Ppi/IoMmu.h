/** @file
  PEI IOMMU PPI.

Copyright (c) 2017 - 2018, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under
the terms and conditions of the BSD License that accompanies this distribution.
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#ifndef __PEI_IOMMU_H__
#define __PEI_IOMMU_H__

//
// for EFI_ALLOCATE_TYPE
//
#include <Uefi.h>

//
// Include protocol for common definition
//   EDKII_IOMMU_ACCESS_xxx
//   EDKII_IOMMU_OPERATION
//
#include <Protocol/IoMmu.h>

//
// IOMMU Ppi GUID value
//
#define EDKII_IOMMU_PPI_GUID \
    { \
      0x70b0af26, 0xf847, 0x4bb6, { 0xaa, 0xb9, 0xcd, 0xe8, 0x4f, 0xc6, 0x14, 0x31 } \
    }

//
// Forward reference for pure ANSI compatability
//
typedef struct _EDKII_IOMMU_PPI  EDKII_IOMMU_PPI;

//
// Revision The revision to which the IOMMU interface adheres.
//          All future revisions must be backwards compatible.
//          If a future version is not back wards compatible it is not the same GUID.
//
#define EDKII_IOMMU_PPI_REVISION 0x00010000

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
typedef
EFI_STATUS
(EFIAPI *EDKII_PEI_IOMMU_SET_ATTRIBUTE)(
  IN EDKII_IOMMU_PPI       *This,
  IN VOID                  *Mapping,
  IN UINT64                IoMmuAccess
  );

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
typedef
EFI_STATUS
(EFIAPI *EDKII_PEI_IOMMU_MAP)(
  IN     EDKII_IOMMU_PPI                            *This,
  IN     EDKII_IOMMU_OPERATION                      Operation,
  IN     VOID                                       *HostAddress,
  IN OUT UINTN                                      *NumberOfBytes,
  OUT    EFI_PHYSICAL_ADDRESS                       *DeviceAddress,
  OUT    VOID                                       **Mapping
  );

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
typedef
EFI_STATUS
(EFIAPI *EDKII_PEI_IOMMU_UNMAP)(
  IN  EDKII_IOMMU_PPI                          *This,
  IN  VOID                                     *Mapping
  );

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
typedef
EFI_STATUS
(EFIAPI *EDKII_PEI_IOMMU_ALLOCATE_BUFFER)(
  IN     EDKII_IOMMU_PPI                          *This,
  IN     EFI_MEMORY_TYPE                          MemoryType,
  IN     UINTN                                    Pages,
  IN OUT VOID                                     **HostAddress,
  IN     UINT64                                   Attributes
  );

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
typedef
EFI_STATUS
(EFIAPI *EDKII_PEI_IOMMU_FREE_BUFFER)(
  IN  EDKII_IOMMU_PPI                          *This,
  IN  UINTN                                    Pages,
  IN  VOID                                     *HostAddress
  );

///
/// IOMMU PPI structure.
///
struct _EDKII_IOMMU_PPI {
  UINT64                              Revision;
  EDKII_PEI_IOMMU_SET_ATTRIBUTE       SetAttribute;
  EDKII_PEI_IOMMU_MAP                 Map;
  EDKII_PEI_IOMMU_UNMAP               Unmap;
  EDKII_PEI_IOMMU_ALLOCATE_BUFFER     AllocateBuffer;
  EDKII_PEI_IOMMU_FREE_BUFFER         FreeBuffer;
};

///
/// IOMMU PPI GUID variable.
///
extern EFI_GUID gEdkiiIoMmuPpiGuid;

#endif
