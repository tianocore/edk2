/** @file
  RISC-V IOMMU driver.

  Copyright (c) 2025, 9elements GmbH. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _RISC_V_IO_MMU_
#define _RISC_V_IO_MMU_

#include <PiDxe.h>
#include <Library/DebugLib.h>
#include <Protocol/IoMmu.h>
#include <IndustryStandard/RiscVIoMappingTable.h>
#include "RiscVIoMmuRegisters.h"
#include "RiscVIoMmuStructures.h"

#define RISCV_IOMMU_DEBUG_LEVEL  DEBUG_INFO

#define RISCV_MMU_PAGE_SHIFT  12

enum {
  STATE_INIT,
  STATE_DETECTED,
  STATE_AVAILABLE,
  STATE_INITIALISED,
};

#define RISCV_IOMMU_DOWNSTREAMS_SIGNATURE  SIGNATURE_32 ('R', 'V', 'I', 'D')
typedef struct {
  UINT32      Signature;
  LIST_ENTRY  Link;

  UINT32      MapMask;

  // TODO: Extend this struct for platform devices, having some mechanism to correlate them later.
  RIMT_PCIE_NODE_ID_MAPPING  NodeMapping;
} RISCV_IOMMU_DOWNSTREAMS;
#define RISCV_IOMMU_DOWNSTREAMS_FROM_LINK(a) CR (a, RISCV_IOMMU_DOWNSTREAMS, Link, RISCV_IOMMU_DOWNSTREAMS_SIGNATURE)

typedef struct {
  BOOLEAN  IsExtended;
  UINT8    Levels;
  VOID     *Buffer;
} CONTEXT_WRAPPER;

enum {
  QUEUE_COMMAND,
  QUEUE_FAULT,
  QUEUE_PAGE_REQUEST,
};

#define QUEUE_NUMBER_OF_ENTRIES  128

typedef struct {
  UINT8  Type;
  UINTN  EntrySize;
  VOID   *Buffer;
} QUEUE_WRAPPER;

// TODO: Prune this struct, some fields remain unused after being set.
#define RISCV_IOMMU_CONTEXT_SIGNATURE  SIGNATURE_32 ('R', 'V', 'I', 'C')
typedef struct {
  UINT32           Signature;
  LIST_ENTRY       Link;

  UINT8            DriverState;

  UINT64           TempHandle;
  BOOLEAN          IoMmuIsPciDevice;
  //
  // For a PCI IOMMU, this contains the BDF until it becomes 'available.'
  //
  UINT64           BaseAddress;

  LIST_ENTRY       DownstreamDevices;

  CONTEXT_WRAPPER  DeviceContext;

  QUEUE_WRAPPER    CommandQueue;
  QUEUE_WRAPPER    FaultQueue;
  QUEUE_WRAPPER    PageRequestQueue;
} RISCV_IOMMU_CONTEXT;
#define RISCV_IOMMU_CONTEXT_FROM_LINK(a) CR (a, RISCV_IOMMU_CONTEXT, Link, RISCV_IOMMU_CONTEXT_SIGNATURE)

extern LIST_ENTRY            mRiscVIoMmuContexts;
extern EDKII_IOMMU_PROTOCOL  mRiscVIoMmuProtocol;

/**
  Detect a RISC-V IOMMU device.

  @retval  EFI_SUCCESS    An IOMMU was detected.
  @retval  EFI_NOT_FOUND  No IOMMU could be detected.

**/
VOID
DetectRiscVIoMmus (
  VOID
  );

/**
  Initialisation worker function.

**/
EFI_STATUS
IoMmuCommonInitialise (
  IN RISCV_IOMMU_CONTEXT  *IoMmuContext
  );

/**
  Set IOMMU attributes for accessing system memory.

  @param[in]  DeviceHandle      The device who initiates the DMA access request.
  @param[in]  Mapping           The mapping value returned from Map().
  @param[in]  IoMmuAccess       The IOMMU access.

  @retval EFI_SUCCESS            The IoMmuAccess is set for the memory range specified by DeviceAddress and Length.
  @retval EFI_DEVICE_ERROR       The IOMMU device reported an error while attempting the operation.

**/
EFI_STATUS
RiscVIoMmuSetAttributeWorker (
  IN RISCV_IOMMU_CONTEXT    *IoMmuContext,
  IN RISCV_IOMMU_DEVICE_ID  *IoMmuDeviceId,
  IN EFI_PHYSICAL_ADDRESS   DeviceAddress,
  IN UINTN                  Length,
  IN UINT64                 IoMmuAccess
  );

/**
  Probe hardware driven queues for problems.

**/
EFI_STATUS
ProbeHardwareQueuesForFaults (
  IN RISCV_IOMMU_CONTEXT  *IoMmuContext
  );

/**
  Set IOMMU attribute for a system memory.

  If the IOMMU protocol exists, the system memory cannot be used
  for DMA by default.

  When a device requests a DMA access for a system memory,
  the device driver need use SetAttribute() to update the IOMMU
  attribute to request DMA access (read and/or write).

  The DeviceHandle is used to identify which device submits the request.
  The IOMMU implementation need translate the device path to an IOMMU device ID,
  and set IOMMU hardware register accordingly.
  1) DeviceHandle can be a standard PCI device.
     The memory for BusMasterRead need set EDKII_IOMMU_ACCESS_READ.
     The memory for BusMasterWrite need set EDKII_IOMMU_ACCESS_WRITE.
     The memory for BusMasterCommonBuffer need set EDKII_IOMMU_ACCESS_READ|EDKII_IOMMU_ACCESS_WRITE.
     After the memory is used, the memory need set 0 to keep it being protected.
  2) DeviceHandle can be an ACPI device (ISA, I2C, SPI, etc).
     The memory for DMA access need set EDKII_IOMMU_ACCESS_READ and/or EDKII_IOMMU_ACCESS_WRITE.

  @param[in]  This              The protocol instance pointer.
  @param[in]  DeviceHandle      The device who initiates the DMA access request.
  @param[in]  Mapping           The mapping value returned from Map().
  @param[in]  IoMmuAccess       The IOMMU access.

  @retval EFI_SUCCESS            The IoMmuAccess is set for the memory range specified by DeviceAddress and Length.
  @retval EFI_INVALID_PARAMETER  DeviceHandle is an invalid handle.
  @retval EFI_INVALID_PARAMETER  Mapping is not a value that was returned by Map().
  @retval EFI_INVALID_PARAMETER  IoMmuAccess specified an illegal combination of access.
  @retval EFI_UNSUPPORTED        DeviceHandle is unknown by the IOMMU.
  @retval EFI_UNSUPPORTED        The bit mask of IoMmuAccess is not supported by the IOMMU.
  @retval EFI_UNSUPPORTED        The IOMMU does not support the memory range specified by Mapping.
  @retval EFI_OUT_OF_RESOURCES   There are not enough resources available to modify the IOMMU access.
  @retval EFI_DEVICE_ERROR       The IOMMU device reported an error while attempting the operation.

**/
EFI_STATUS
EFIAPI
IoMmuSetAttribute (
  IN EDKII_IOMMU_PROTOCOL  *This,
  IN EFI_HANDLE            DeviceHandle,
  IN VOID                  *Mapping,
  IN UINT64                IoMmuAccess
  );

/**
  Provides the controller-specific addresses required to access system memory from a
  DMA bus master.

  @param  This                  The protocol instance pointer.
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
IoMmuMap (
  IN     EDKII_IOMMU_PROTOCOL                       *This,
  IN     EDKII_IOMMU_OPERATION                      Operation,
  IN     VOID                                       *HostAddress,
  IN OUT UINTN                                      *NumberOfBytes,
  OUT    EFI_PHYSICAL_ADDRESS                       *DeviceAddress,
  OUT    VOID                                       **Mapping
  );

/**
  Completes the Map() operation and releases any corresponding resources.

  @param  This                  The protocol instance pointer.
  @param  Mapping               The mapping value returned from Map().

  @retval EFI_SUCCESS           The range was unmapped.
  @retval EFI_INVALID_PARAMETER Mapping is not a value that was returned by Map().
  @retval EFI_DEVICE_ERROR      The data was not committed to the target system memory.
**/
EFI_STATUS
EFIAPI
IoMmuUnmap (
  IN  EDKII_IOMMU_PROTOCOL                     *This,
  IN  VOID                                     *Mapping
  );

/**
  Allocates pages that are suitable for an OperationBusMasterCommonBuffer or
  OperationBusMasterCommonBuffer64 mapping.

  @param  This                  The protocol instance pointer.
  @param  Type                  This parameter is not used and must be ignored.
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

**/
EFI_STATUS
EFIAPI
IoMmuAllocateBuffer (
  IN     EDKII_IOMMU_PROTOCOL                     *This,
  IN     EFI_ALLOCATE_TYPE                        Type,
  IN     EFI_MEMORY_TYPE                          MemoryType,
  IN     UINTN                                    Pages,
  IN OUT VOID                                     **HostAddress,
  IN     UINT64                                   Attributes
  );

/**
  Frees memory that was allocated with AllocateBuffer().

  @param  This                  The protocol instance pointer.
  @param  Pages                 The number of pages to free.
  @param  HostAddress           The base system memory address of the allocated range.

  @retval EFI_SUCCESS           The requested memory pages were freed.
  @retval EFI_INVALID_PARAMETER The memory range specified by HostAddress and Pages
                                was not allocated with AllocateBuffer().

**/
EFI_STATUS
EFIAPI
IoMmuFreeBuffer (
  IN  EDKII_IOMMU_PROTOCOL                     *This,
  IN  UINTN                                    Pages,
  IN  VOID                                     *HostAddress
  );

/**
  Read a 32-bit IOMMU register.

  @param[in]  IoMmuContext  The IOMMU context to operate on.
  @param[in]  Offset        The register offset to read.
  @ret        The value read from the register.

**/
UINT32
IoMmuRead32 (
  IN RISCV_IOMMU_CONTEXT  *IoMmuContext,
  IN UINTN                Offset
  );

/**
  Write a 32-bit IOMMU register.

  @param[in]  IoMmuContext  The IOMMU context to operate on.
  @param[in]  Offset        The register offset to write.
  @param[in]  Value         The value to write to the register.

**/
VOID
IoMmuWrite32 (
  IN RISCV_IOMMU_CONTEXT  *IoMmuContext,
  IN UINTN                Offset,
  IN UINT32               Value
  );

/**
  Write a 32-bit IOMMU register and wait for a mask to be set/unset.

  @param[in]  IoMmuContext  The IOMMU context to operate on.
  @param[in]  Offset        The register offset to write.
  @param[in]  Value         The value to write to the register.
  @param[in]  Mask          The bitmask to wait for.
  @param[in]  Set           Whether the mask should be set or unset.

**/
VOID
IoMmuWriteAndWait32 (
  IN RISCV_IOMMU_CONTEXT  *IoMmuContext,
  IN UINTN                Offset,
  IN UINT32               Value,
  IN UINT32               Mask,
  IN BOOLEAN              Set
  );

/**
  Read a 64-bit IOMMU register.

  @param[in]  IoMmuContext  The IOMMU context to operate on.
  @param[in]  Offset        The register offset to read.
  @ret        The value read from the register.

**/
UINT64
IoMmuRead64 (
  IN RISCV_IOMMU_CONTEXT  *IoMmuContext,
  IN UINTN                Offset
  );

/**
  Write a 64-bit IOMMU register.

  @param[in]  IoMmuContext  The IOMMU context to operate on.
  @param[in]  Offset        The register offset to write.
  @param[in]  Value         The value to write to the register.

**/
VOID
IoMmuWrite64 (
  IN RISCV_IOMMU_CONTEXT  *IoMmuContext,
  IN UINTN                Offset,
  IN UINT64               Value
  );

/**
  Write a 64-bit IOMMU register and wait for a mask to be set/unset.

  @param[in]  IoMmuContext  The IOMMU context to operate on.
  @param[in]  Offset        The register offset to write.
  @param[in]  Value         The value to write to the register.
  @param[in]  Mask          The bitmask to wait for.
  @param[in]  Set           Whether the mask should be set or unset.

**/
VOID
IoMmuWriteAndWait64 (
  IN RISCV_IOMMU_CONTEXT  *IoMmuContext,
  IN UINTN                Offset,
  IN UINT64               Value,
  IN UINT64               Mask,
  IN BOOLEAN              Set
  );

UINT64
RiscVGetSupervisorStatusRegister (
  VOID
  );

#endif
