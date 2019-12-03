/** @file
  The NvmExpressPei driver is used to manage non-volatile memory subsystem
  which follows NVM Express specification at PEI phase.

  Copyright (c) 2018 - 2019, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _NVM_EXPRESS_PEI_H_
#define _NVM_EXPRESS_PEI_H_

#include <PiPei.h>

#include <IndustryStandard/Nvme.h>

#include <Ppi/NvmExpressHostController.h>
#include <Ppi/BlockIo.h>
#include <Ppi/BlockIo2.h>
#include <Ppi/StorageSecurityCommand.h>
#include <Ppi/NvmExpressPassThru.h>
#include <Ppi/IoMmu.h>
#include <Ppi/EndOfPeiPhase.h>

#include <Library/DebugLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/IoLib.h>
#include <Library/TimerLib.h>

//
// Structure forward declarations
//
typedef struct _PEI_NVME_NAMESPACE_INFO           PEI_NVME_NAMESPACE_INFO;
typedef struct _PEI_NVME_CONTROLLER_PRIVATE_DATA  PEI_NVME_CONTROLLER_PRIVATE_DATA;

#include "NvmExpressPeiHci.h"
#include "NvmExpressPeiPassThru.h"
#include "NvmExpressPeiBlockIo.h"
#include "NvmExpressPeiStorageSecurity.h"

//
// NVME PEI driver implementation related definitions
//
#define NVME_MAX_QUEUES                               2     // Number of I/O queues supported by the driver, 1 for AQ, 1 for CQ
#define NVME_ASQ_SIZE                                 1     // Number of admin submission queue entries, which is 0-based
#define NVME_ACQ_SIZE                                 1     // Number of admin completion queue entries, which is 0-based
#define NVME_CSQ_SIZE                                 63    // Number of I/O submission queue entries, which is 0-based
#define NVME_CCQ_SIZE                                 63    // Number of I/O completion queue entries, which is 0-based
#define NVME_PRP_SIZE                                 (8)   // Pages of PRP list

#define NVME_MEM_MAX_PAGES                                           \
  (                                                                  \
  1                                         /* ASQ */             +  \
  1                                         /* ACQ */             +  \
  1                                         /* SQs */             +  \
  1                                         /* CQs */             +  \
  NVME_PRP_SIZE)                            /* PRPs */

#define NVME_ADMIN_QUEUE                              0x00
#define NVME_IO_QUEUE                                 0x01
#define NVME_GENERIC_TIMEOUT                          5000000   // Generic PassThru command timeout value, in us unit
#define NVME_POLL_INTERVAL                            100       // Poll interval for PassThru command, in us unit

//
// Nvme namespace data structure.
//
struct _PEI_NVME_NAMESPACE_INFO {
  UINT32                                    NamespaceId;
  UINT64                                    NamespaceUuid;
  EFI_PEI_BLOCK_IO2_MEDIA                   Media;

  PEI_NVME_CONTROLLER_PRIVATE_DATA          *Controller;
};

#define NVME_CONTROLLER_NSID        0

//
// Unique signature for private data structure.
//
#define NVME_PEI_CONTROLLER_PRIVATE_DATA_SIGNATURE    SIGNATURE_32 ('N','V','P','C')

//
// Nvme controller private data structure.
//
struct _PEI_NVME_CONTROLLER_PRIVATE_DATA {
  UINT32                                    Signature;
  UINTN                                     MmioBase;
  EFI_NVM_EXPRESS_PASS_THRU_MODE            PassThruMode;
  UINTN                                     DevicePathLength;
  EFI_DEVICE_PATH_PROTOCOL                  *DevicePath;

  EFI_PEI_RECOVERY_BLOCK_IO_PPI             BlkIoPpi;
  EFI_PEI_RECOVERY_BLOCK_IO2_PPI            BlkIo2Ppi;
  EDKII_PEI_STORAGE_SECURITY_CMD_PPI        StorageSecurityPpi;
  EDKII_PEI_NVM_EXPRESS_PASS_THRU_PPI       NvmePassThruPpi;
  EFI_PEI_PPI_DESCRIPTOR                    BlkIoPpiList;
  EFI_PEI_PPI_DESCRIPTOR                    BlkIo2PpiList;
  EFI_PEI_PPI_DESCRIPTOR                    StorageSecurityPpiList;
  EFI_PEI_PPI_DESCRIPTOR                    NvmePassThruPpiList;
  EFI_PEI_NOTIFY_DESCRIPTOR                 EndOfPeiNotifyList;

  //
  // Pointer to identify controller data
  //
  NVME_ADMIN_CONTROLLER_DATA                *ControllerData;

  //
  // (4 + NVME_PRP_SIZE) x 4kB aligned buffers will be carved out of this buffer
  // 1st 4kB boundary is the start of the admin submission queue
  // 2nd 4kB boundary is the start of the admin completion queue
  // 3rd 4kB boundary is the start of I/O submission queue
  // 4th 4kB boundary is the start of I/O completion queue
  // 5th 4kB boundary is the start of PRP list buffers
  //
  VOID                                      *Buffer;
  VOID                                      *BufferMapping;

  //
  // Pointers to 4kB aligned submission & completion queues
  //
  NVME_SQ                                   *SqBuffer[NVME_MAX_QUEUES];
  NVME_CQ                                   *CqBuffer[NVME_MAX_QUEUES];

  //
  // Submission and completion queue indices
  //
  NVME_SQTDBL                               SqTdbl[NVME_MAX_QUEUES];
  NVME_CQHDBL                               CqHdbl[NVME_MAX_QUEUES];

  UINT8                                     Pt[NVME_MAX_QUEUES];
  UINT16                                    Cid[NVME_MAX_QUEUES];

  //
  // Nvme controller capabilities
  //
  NVME_CAP                                  Cap;

  //
  // Namespaces information on the controller
  //
  UINT32                                    ActiveNamespaceNum;
  PEI_NVME_NAMESPACE_INFO                   *NamespaceInfo;
};

#define GET_NVME_PEIM_HC_PRIVATE_DATA_FROM_THIS_BLKIO(a)               \
  CR (a, PEI_NVME_CONTROLLER_PRIVATE_DATA, BlkIoPpi, NVME_PEI_CONTROLLER_PRIVATE_DATA_SIGNATURE)
#define GET_NVME_PEIM_HC_PRIVATE_DATA_FROM_THIS_BLKIO2(a)              \
  CR (a, PEI_NVME_CONTROLLER_PRIVATE_DATA, BlkIo2Ppi, NVME_PEI_CONTROLLER_PRIVATE_DATA_SIGNATURE)
#define GET_NVME_PEIM_HC_PRIVATE_DATA_FROM_THIS_STROAGE_SECURITY(a)    \
  CR (a, PEI_NVME_CONTROLLER_PRIVATE_DATA, StorageSecurityPpi, NVME_PEI_CONTROLLER_PRIVATE_DATA_SIGNATURE)
#define GET_NVME_PEIM_HC_PRIVATE_DATA_FROM_THIS_NVME_PASSTHRU(a)       \
  CR (a, PEI_NVME_CONTROLLER_PRIVATE_DATA, NvmePassThruPpi, NVME_PEI_CONTROLLER_PRIVATE_DATA_SIGNATURE)
#define GET_NVME_PEIM_HC_PRIVATE_DATA_FROM_THIS_NOTIFY(a)              \
  CR (a, PEI_NVME_CONTROLLER_PRIVATE_DATA, EndOfPeiNotifyList, NVME_PEI_CONTROLLER_PRIVATE_DATA_SIGNATURE)


//
// Internal functions
//

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
  );

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
  IN UINTN                  Pages,
  IN VOID                   *HostAddress,
  IN VOID                   *Mapping
  );

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
  IN  EDKII_IOMMU_OPERATION Operation,
  IN VOID                   *HostAddress,
  IN  OUT UINTN             *NumberOfBytes,
  OUT EFI_PHYSICAL_ADDRESS  *DeviceAddress,
  OUT VOID                  **Mapping
  );

/**
  Completes the Map() operation and releases any corresponding resources.

  @param  Mapping               The mapping value returned from Map().

  @retval EFI_SUCCESS           The range was unmapped.
  @retval EFI_INVALID_PARAMETER Mapping is not a value that was returned by Map().
  @retval EFI_DEVICE_ERROR      The data was not committed to the target system memory.
**/
EFI_STATUS
IoMmuUnmap (
  IN VOID                  *Mapping
  );

/**
  One notified function to cleanup the allocated resources at the end of PEI.

  @param[in] PeiServices         Pointer to PEI Services Table.
  @param[in] NotifyDescriptor    Pointer to the descriptor for the Notification
                                 event that caused this function to execute.
  @param[in] Ppi                 Pointer to the PPI data associated with this function.

  @retval     EFI_SUCCESS  The function completes successfully

**/
EFI_STATUS
EFIAPI
NvmePeimEndOfPei (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  );

/**
  Get the size of the current device path instance.

  @param[in]  DevicePath             A pointer to the EFI_DEVICE_PATH_PROTOCOL
                                     structure.
  @param[out] InstanceSize           The size of the current device path instance.
  @param[out] EntireDevicePathEnd    Indicate whether the instance is the last
                                     one in the device path strucure.

  @retval EFI_SUCCESS    The size of the current device path instance is fetched.
  @retval Others         Fails to get the size of the current device path instance.

**/
EFI_STATUS
GetDevicePathInstanceSize (
  IN  EFI_DEVICE_PATH_PROTOCOL    *DevicePath,
  OUT UINTN                       *InstanceSize,
  OUT BOOLEAN                     *EntireDevicePathEnd
  );

/**
  Check the validity of the device path of a NVM Express host controller.

  @param[in] DevicePath          A pointer to the EFI_DEVICE_PATH_PROTOCOL
                                 structure.
  @param[in] DevicePathLength    The length of the device path.

  @retval EFI_SUCCESS              The device path is valid.
  @retval EFI_INVALID_PARAMETER    The device path is invalid.

**/
EFI_STATUS
NvmeIsHcDevicePathValid (
  IN EFI_DEVICE_PATH_PROTOCOL    *DevicePath,
  IN UINTN                       DevicePathLength
  );

/**
  Build the device path for an Nvm Express device with given namespace identifier
  and namespace extended unique identifier.

  @param[in]  Private              A pointer to the PEI_NVME_CONTROLLER_PRIVATE_DATA
                                   data structure.
  @param[in]  NamespaceId          The given namespace identifier.
  @param[in]  NamespaceUuid        The given namespace extended unique identifier.
  @param[out] DevicePathLength     The length of the device path in bytes specified
                                   by DevicePath.
  @param[out] DevicePath           The device path of Nvm Express device.

  @retval EFI_SUCCESS              The operation succeeds.
  @retval EFI_INVALID_PARAMETER    The parameters are invalid.
  @retval EFI_OUT_OF_RESOURCES     The operation fails due to lack of resources.

**/
EFI_STATUS
NvmeBuildDevicePath (
  IN  PEI_NVME_CONTROLLER_PRIVATE_DATA    *Private,
  IN  UINT32                              NamespaceId,
  IN  UINT64                              NamespaceUuid,
  OUT UINTN                               *DevicePathLength,
  OUT EFI_DEVICE_PATH_PROTOCOL            **DevicePath
  );

/**
  Determine if a specific NVM Express controller can be skipped for S3 phase.

  @param[in]  HcDevicePath          Device path of the controller.
  @param[in]  HcDevicePathLength    Length of the device path specified by
                                    HcDevicePath.

  @retval    The number of ports that need to be enumerated.

**/
BOOLEAN
NvmeS3SkipThisController (
  IN  EFI_DEVICE_PATH_PROTOCOL    *HcDevicePath,
  IN  UINTN                       HcDevicePathLength
  );

#endif
