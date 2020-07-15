/** @file
  NvmExpressDxe driver is used to manage non-volatile memory subsystem which follows
  NVM Express specification.

  (C) Copyright 2016 Hewlett Packard Enterprise Development LP<BR>
  Copyright (c) 2013 - 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _EFI_NVM_EXPRESS_H_
#define _EFI_NVM_EXPRESS_H_

#include <Uefi.h>

#include <IndustryStandard/Pci.h>
#include <IndustryStandard/Nvme.h>

#include <Protocol/ComponentName.h>
#include <Protocol/ComponentName2.h>
#include <Protocol/DriverBinding.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/DevicePath.h>
#include <Protocol/PciIo.h>
#include <Protocol/NvmExpressPassthru.h>
#include <Protocol/BlockIo.h>
#include <Protocol/BlockIo2.h>
#include <Protocol/DiskInfo.h>
#include <Protocol/DriverSupportedEfiVersion.h>
#include <Protocol/StorageSecurityCommand.h>
#include <Protocol/ResetNotification.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/ReportStatusCodeLib.h>

typedef struct _NVME_CONTROLLER_PRIVATE_DATA NVME_CONTROLLER_PRIVATE_DATA;
typedef struct _NVME_DEVICE_PRIVATE_DATA     NVME_DEVICE_PRIVATE_DATA;

#include "NvmExpressBlockIo.h"
#include "NvmExpressDiskInfo.h"
#include "NvmExpressHci.h"

extern EFI_DRIVER_BINDING_PROTOCOL                gNvmExpressDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL                gNvmExpressComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL               gNvmExpressComponentName2;
extern EFI_DRIVER_SUPPORTED_EFI_VERSION_PROTOCOL  gNvmExpressDriverSupportedEfiVersion;

#define PCI_CLASS_MASS_STORAGE_NVM                0x08  // mass storage sub-class non-volatile memory.
#define PCI_IF_NVMHCI                             0x02  // mass storage programming interface NVMHCI.

#define NVME_ASQ_SIZE                             1     // Number of admin submission queue entries, which is 0-based
#define NVME_ACQ_SIZE                             1     // Number of admin completion queue entries, which is 0-based

#define NVME_CSQ_SIZE                             1     // Number of I/O submission queue entries, which is 0-based
#define NVME_CCQ_SIZE                             1     // Number of I/O completion queue entries, which is 0-based

//
// Number of asynchronous I/O submission queue entries, which is 0-based.
// The asynchronous I/O submission queue size is 4kB in total.
//
#define NVME_ASYNC_CSQ_SIZE                       63
//
// Number of asynchronous I/O completion queue entries, which is 0-based.
// The asynchronous I/O completion queue size is 4kB in total.
//
#define NVME_ASYNC_CCQ_SIZE                       255

#define NVME_MAX_QUEUES                           3     // Number of queues supported by the driver

#define NVME_CONTROLLER_ID                        0

//
// Time out value for Nvme transaction execution
//
#define NVME_GENERIC_TIMEOUT                      EFI_TIMER_PERIOD_SECONDS (5)

//
// Nvme async transfer timer interval, set by experience.
//
#define NVME_HC_ASYNC_TIMER                       EFI_TIMER_PERIOD_MILLISECONDS (1)

//
// Unique signature for private data structure.
//
#define NVME_CONTROLLER_PRIVATE_DATA_SIGNATURE    SIGNATURE_32 ('N','V','M','E')

//
// Nvme private data structure.
//
struct _NVME_CONTROLLER_PRIVATE_DATA {
  UINT32                              Signature;

  EFI_HANDLE                          ControllerHandle;
  EFI_HANDLE                          ImageHandle;
  EFI_HANDLE                          DriverBindingHandle;

  EFI_PCI_IO_PROTOCOL                 *PciIo;
  UINT64                              PciAttributes;

  EFI_DEVICE_PATH_PROTOCOL            *ParentDevicePath;

  EFI_NVM_EXPRESS_PASS_THRU_MODE      PassThruMode;
  EFI_NVM_EXPRESS_PASS_THRU_PROTOCOL  Passthru;

  //
  // pointer to identify controller data
  //
  NVME_ADMIN_CONTROLLER_DATA          *ControllerData;

  //
  // 6 x 4kB aligned buffers will be carved out of this buffer.
  // 1st 4kB boundary is the start of the admin submission queue.
  // 2nd 4kB boundary is the start of the admin completion queue.
  // 3rd 4kB boundary is the start of I/O submission queue #1.
  // 4th 4kB boundary is the start of I/O completion queue #1.
  // 5th 4kB boundary is the start of I/O submission queue #2.
  // 6th 4kB boundary is the start of I/O completion queue #2.
  //
  UINT8                               *Buffer;
  UINT8                               *BufferPciAddr;

  //
  // Pointers to 4kB aligned submission & completion queues.
  //
  NVME_SQ                             *SqBuffer[NVME_MAX_QUEUES];
  NVME_CQ                             *CqBuffer[NVME_MAX_QUEUES];
  NVME_SQ                             *SqBufferPciAddr[NVME_MAX_QUEUES];
  NVME_CQ                             *CqBufferPciAddr[NVME_MAX_QUEUES];

  //
  // Submission and completion queue indices.
  //
  NVME_SQTDBL                         SqTdbl[NVME_MAX_QUEUES];
  NVME_CQHDBL                         CqHdbl[NVME_MAX_QUEUES];
  UINT16                              AsyncSqHead;

  //
  // Flag to indicate internal IO queue creation.
  //
  BOOLEAN                             CreateIoQueue;

  UINT8                               Pt[NVME_MAX_QUEUES];
  UINT16                              Cid[NVME_MAX_QUEUES];

  //
  // Nvme controller capabilities
  //
  NVME_CAP                            Cap;

  VOID                                *Mapping;

  //
  // For Non-blocking operations.
  //
  EFI_EVENT                           TimerEvent;
  LIST_ENTRY                          AsyncPassThruQueue;
  LIST_ENTRY                          UnsubmittedSubtasks;
};

#define NVME_CONTROLLER_PRIVATE_DATA_FROM_PASS_THRU(a) \
  CR (a, \
      NVME_CONTROLLER_PRIVATE_DATA, \
      Passthru, \
      NVME_CONTROLLER_PRIVATE_DATA_SIGNATURE \
      )

//
// Unique signature for private data structure.
//
#define NVME_DEVICE_PRIVATE_DATA_SIGNATURE     SIGNATURE_32 ('X','S','S','D')

//
// Nvme device private data structure
//
struct _NVME_DEVICE_PRIVATE_DATA {
  UINT32                                   Signature;

  EFI_HANDLE                               DeviceHandle;
  EFI_HANDLE                               ControllerHandle;
  EFI_HANDLE                               DriverBindingHandle;

  EFI_DEVICE_PATH_PROTOCOL                 *DevicePath;

  EFI_UNICODE_STRING_TABLE                 *ControllerNameTable;

  UINT32                                   NamespaceId;
  UINT64                                   NamespaceUuid;

  EFI_BLOCK_IO_MEDIA                       Media;
  EFI_BLOCK_IO_PROTOCOL                    BlockIo;
  EFI_BLOCK_IO2_PROTOCOL                   BlockIo2;
  EFI_DISK_INFO_PROTOCOL                   DiskInfo;
  EFI_STORAGE_SECURITY_COMMAND_PROTOCOL    StorageSecurity;

  LIST_ENTRY                               AsyncQueue;

  EFI_LBA                                  NumBlocks;

  CHAR16                                   ModelName[80];
  NVME_ADMIN_NAMESPACE_DATA                NamespaceData;

  NVME_CONTROLLER_PRIVATE_DATA             *Controller;

};

//
// Statments to retrieve the private data from produced protocols.
//
#define NVME_DEVICE_PRIVATE_DATA_FROM_BLOCK_IO(a) \
  CR (a, \
      NVME_DEVICE_PRIVATE_DATA, \
      BlockIo, \
      NVME_DEVICE_PRIVATE_DATA_SIGNATURE \
      )

#define NVME_DEVICE_PRIVATE_DATA_FROM_BLOCK_IO2(a) \
  CR (a, \
      NVME_DEVICE_PRIVATE_DATA, \
      BlockIo2, \
      NVME_DEVICE_PRIVATE_DATA_SIGNATURE \
      )

#define NVME_DEVICE_PRIVATE_DATA_FROM_DISK_INFO(a) \
  CR (a, \
      NVME_DEVICE_PRIVATE_DATA, \
      DiskInfo, \
      NVME_DEVICE_PRIVATE_DATA_SIGNATURE \
      )

#define NVME_DEVICE_PRIVATE_DATA_FROM_STORAGE_SECURITY(a)\
  CR (a,                                                 \
      NVME_DEVICE_PRIVATE_DATA,                          \
      StorageSecurity,                                   \
      NVME_DEVICE_PRIVATE_DATA_SIGNATURE                 \
      )

//
// Nvme block I/O 2 request.
//
#define NVME_BLKIO2_REQUEST_SIGNATURE      SIGNATURE_32 ('N', 'B', '2', 'R')

typedef struct {
  UINT32                                   Signature;
  LIST_ENTRY                               Link;

  EFI_BLOCK_IO2_TOKEN                      *Token;
  UINTN                                    UnsubmittedSubtaskNum;
  BOOLEAN                                  LastSubtaskSubmitted;
  //
  // The queue for Nvme read/write sub-tasks of a BlockIo2 request.
  //
  LIST_ENTRY                               SubtasksQueue;
} NVME_BLKIO2_REQUEST;

#define NVME_BLKIO2_REQUEST_FROM_LINK(a) \
  CR (a, NVME_BLKIO2_REQUEST, Link, NVME_BLKIO2_REQUEST_SIGNATURE)

#define NVME_BLKIO2_SUBTASK_SIGNATURE      SIGNATURE_32 ('N', 'B', '2', 'S')

typedef struct {
  UINT32                                   Signature;
  LIST_ENTRY                               Link;

  BOOLEAN                                  IsLast;
  UINT32                                   NamespaceId;
  EFI_EVENT                                Event;
  EFI_NVM_EXPRESS_PASS_THRU_COMMAND_PACKET *CommandPacket;
  //
  // The BlockIo2 request this subtask belongs to
  //
  NVME_BLKIO2_REQUEST                      *BlockIo2Request;
} NVME_BLKIO2_SUBTASK;

#define NVME_BLKIO2_SUBTASK_FROM_LINK(a) \
  CR (a, NVME_BLKIO2_SUBTASK, Link, NVME_BLKIO2_SUBTASK_SIGNATURE)

//
// Nvme asynchronous passthru request.
//
#define NVME_PASS_THRU_ASYNC_REQ_SIG       SIGNATURE_32 ('N', 'P', 'A', 'R')

typedef struct {
  UINT32                                   Signature;
  LIST_ENTRY                               Link;

  EFI_NVM_EXPRESS_PASS_THRU_COMMAND_PACKET *Packet;
  UINT16                                   CommandId;
  VOID                                     *MapPrpList;
  UINTN                                    PrpListNo;
  VOID                                     *PrpListHost;
  VOID                                     *MapData;
  VOID                                     *MapMeta;
  EFI_EVENT                                CallerEvent;
} NVME_PASS_THRU_ASYNC_REQ;

#define NVME_PASS_THRU_ASYNC_REQ_FROM_THIS(a) \
  CR (a,                                                 \
      NVME_PASS_THRU_ASYNC_REQ,                          \
      Link,                                              \
      NVME_PASS_THRU_ASYNC_REQ_SIG                       \
      )

/**
  Retrieves a Unicode string that is the user readable name of the driver.

  This function retrieves the user readable name of a driver in the form of a
  Unicode string. If the driver specified by This has a user readable name in
  the language specified by Language, then a pointer to the driver name is
  returned in DriverName, and EFI_SUCCESS is returned. If the driver specified
  by This does not support the language specified by Language,
  then EFI_UNSUPPORTED is returned.

  @param  This[in]              A pointer to the EFI_COMPONENT_NAME2_PROTOCOL or
                                EFI_COMPONENT_NAME_PROTOCOL instance.

  @param  Language[in]          A pointer to a Null-terminated ASCII string
                                array indicating the language. This is the
                                language of the driver name that the caller is
                                requesting, and it must match one of the
                                languages specified in SupportedLanguages. The
                                number of languages supported by a driver is up
                                to the driver writer. Language is specified
                                in RFC 4646 or ISO 639-2 language code format.

  @param  DriverName[out]       A pointer to the Unicode string to return.
                                This Unicode string is the name of the
                                driver specified by This in the language
                                specified by Language.

  @retval EFI_SUCCESS           The Unicode string for the Driver specified by
                                This and the language specified by Language was
                                returned in DriverName.

  @retval EFI_INVALID_PARAMETER Language is NULL.

  @retval EFI_INVALID_PARAMETER DriverName is NULL.

  @retval EFI_UNSUPPORTED       The driver specified by This does not support
                                the language specified by Language.

**/
EFI_STATUS
EFIAPI
NvmExpressComponentNameGetDriverName (
  IN  EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **DriverName
  );

/**
  Retrieves a Unicode string that is the user readable name of the controller
  that is being managed by a driver.

  This function retrieves the user readable name of the controller specified by
  ControllerHandle and ChildHandle in the form of a Unicode string. If the
  driver specified by This has a user readable name in the language specified by
  Language, then a pointer to the controller name is returned in ControllerName,
  and EFI_SUCCESS is returned.  If the driver specified by This is not currently
  managing the controller specified by ControllerHandle and ChildHandle,
  then EFI_UNSUPPORTED is returned.  If the driver specified by This does not
  support the language specified by Language, then EFI_UNSUPPORTED is returned.

  @param  This[in]              A pointer to the EFI_COMPONENT_NAME2_PROTOCOL or
                                EFI_COMPONENT_NAME_PROTOCOL instance.

  @param  ControllerHandle[in]  The handle of a controller that the driver
                                specified by This is managing.  This handle
                                specifies the controller whose name is to be
                                returned.

  @param  ChildHandle[in]       The handle of the child controller to retrieve
                                the name of.  This is an optional parameter that
                                may be NULL.  It will be NULL for device
                                drivers.  It will also be NULL for a bus drivers
                                that wish to retrieve the name of the bus
                                controller.  It will not be NULL for a bus
                                driver that wishes to retrieve the name of a
                                child controller.

  @param  Language[in]          A pointer to a Null-terminated ASCII string
                                array indicating the language.  This is the
                                language of the driver name that the caller is
                                requesting, and it must match one of the
                                languages specified in SupportedLanguages. The
                                number of languages supported by a driver is up
                                to the driver writer. Language is specified in
                                RFC 4646 or ISO 639-2 language code format.

  @param  ControllerName[out]   A pointer to the Unicode string to return.
                                This Unicode string is the name of the
                                controller specified by ControllerHandle and
                                ChildHandle in the language specified by
                                Language from the point of view of the driver
                                specified by This.

  @retval EFI_SUCCESS           The Unicode string for the user readable name in
                                the language specified by Language for the
                                driver specified by This was returned in
                                DriverName.

  @retval EFI_INVALID_PARAMETER ControllerHandle is NULL.

  @retval EFI_INVALID_PARAMETER ChildHandle is not NULL and it is not a valid
                                EFI_HANDLE.

  @retval EFI_INVALID_PARAMETER Language is NULL.

  @retval EFI_INVALID_PARAMETER ControllerName is NULL.

  @retval EFI_UNSUPPORTED       The driver specified by This is not currently
                                managing the controller specified by
                                ControllerHandle and ChildHandle.

  @retval EFI_UNSUPPORTED       The driver specified by This does not support
                                the language specified by Language.

**/
EFI_STATUS
EFIAPI
NvmExpressComponentNameGetControllerName (
  IN  EFI_COMPONENT_NAME_PROTOCOL                     *This,
  IN  EFI_HANDLE                                      ControllerHandle,
  IN  EFI_HANDLE                                      ChildHandle        OPTIONAL,
  IN  CHAR8                                           *Language,
  OUT CHAR16                                          **ControllerName
  );

/**
  Tests to see if this driver supports a given controller. If a child device is provided,
  it further tests to see if this driver supports creating a handle for the specified child device.

  This function checks to see if the driver specified by This supports the device specified by
  ControllerHandle. Drivers will typically use the device path attached to
  ControllerHandle and/or the services from the bus I/O abstraction attached to
  ControllerHandle to determine if the driver supports ControllerHandle. This function
  may be called many times during platform initialization. In order to reduce boot times, the tests
  performed by this function must be very small, and take as little time as possible to execute. This
  function must not change the state of any hardware devices, and this function must be aware that the
  device specified by ControllerHandle may already be managed by the same driver or a
  different driver. This function must match its calls to AllocatePages() with FreePages(),
  AllocatePool() with FreePool(), and OpenProtocol() with CloseProtocol().
  Since ControllerHandle may have been previously started by the same driver, if a protocol is
  already in the opened state, then it must not be closed with CloseProtocol(). This is required
  to guarantee the state of ControllerHandle is not modified by this function.

  @param[in]  This                 A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param[in]  ControllerHandle     The handle of the controller to test. This handle
                                   must support a protocol interface that supplies
                                   an I/O abstraction to the driver.
  @param[in]  RemainingDevicePath  A pointer to the remaining portion of a device path.  This
                                   parameter is ignored by device drivers, and is optional for bus
                                   drivers. For bus drivers, if this parameter is not NULL, then
                                   the bus driver must determine if the bus controller specified
                                   by ControllerHandle and the child controller specified
                                   by RemainingDevicePath are both supported by this
                                   bus driver.

  @retval EFI_SUCCESS              The device specified by ControllerHandle and
                                   RemainingDevicePath is supported by the driver specified by This.
  @retval EFI_ALREADY_STARTED      The device specified by ControllerHandle and
                                   RemainingDevicePath is already being managed by the driver
                                   specified by This.
  @retval EFI_ACCESS_DENIED        The device specified by ControllerHandle and
                                   RemainingDevicePath is already being managed by a different
                                   driver or an application that requires exclusive access.
                                   Currently not implemented.
  @retval EFI_UNSUPPORTED          The device specified by ControllerHandle and
                                   RemainingDevicePath is not supported by the driver specified by This.
**/
EFI_STATUS
EFIAPI
NvmExpressDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  );

/**
  Starts a device controller or a bus controller.

  The Start() function is designed to be invoked from the EFI boot service ConnectController().
  As a result, much of the error checking on the parameters to Start() has been moved into this
  common boot service. It is legal to call Start() from other locations,
  but the following calling restrictions must be followed or the system behavior will not be deterministic.
  1. ControllerHandle must be a valid EFI_HANDLE.
  2. If RemainingDevicePath is not NULL, then it must be a pointer to a naturally aligned
     EFI_DEVICE_PATH_PROTOCOL.
  3. Prior to calling Start(), the Supported() function for the driver specified by This must
     have been called with the same calling parameters, and Supported() must have returned EFI_SUCCESS.

  @param[in]  This                 A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param[in]  ControllerHandle     The handle of the controller to start. This handle
                                   must support a protocol interface that supplies
                                   an I/O abstraction to the driver.
  @param[in]  RemainingDevicePath  A pointer to the remaining portion of a device path.  This
                                   parameter is ignored by device drivers, and is optional for bus
                                   drivers. For a bus driver, if this parameter is NULL, then handles
                                   for all the children of Controller are created by this driver.
                                   If this parameter is not NULL and the first Device Path Node is
                                   not the End of Device Path Node, then only the handle for the
                                   child device specified by the first Device Path Node of
                                   RemainingDevicePath is created by this driver.
                                   If the first Device Path Node of RemainingDevicePath is
                                   the End of Device Path Node, no child handle is created by this
                                   driver.

  @retval EFI_SUCCESS              The device was started.
  @retval EFI_DEVICE_ERROR         The device could not be started due to a device error.Currently not implemented.
  @retval EFI_OUT_OF_RESOURCES     The request could not be completed due to a lack of resources.
  @retval Others                   The driver failded to start the device.

**/
EFI_STATUS
EFIAPI
NvmExpressDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  );

/**
  Stops a device controller or a bus controller.

  The Stop() function is designed to be invoked from the EFI boot service DisconnectController().
  As a result, much of the error checking on the parameters to Stop() has been moved
  into this common boot service. It is legal to call Stop() from other locations,
  but the following calling restrictions must be followed or the system behavior will not be deterministic.
  1. ControllerHandle must be a valid EFI_HANDLE that was used on a previous call to this
     same driver's Start() function.
  2. The first NumberOfChildren handles of ChildHandleBuffer must all be a valid
     EFI_HANDLE. In addition, all of these handles must have been created in this driver's
     Start() function, and the Start() function must have called OpenProtocol() on
     ControllerHandle with an Attribute of EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER.

  @param[in]  This              A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param[in]  ControllerHandle  A handle to the device being stopped. The handle must
                                support a bus specific I/O protocol for the driver
                                to use to stop the device.
  @param[in]  NumberOfChildren  The number of child device handles in ChildHandleBuffer.
  @param[in]  ChildHandleBuffer An array of child handles to be freed. May be NULL
                                if NumberOfChildren is 0.

  @retval EFI_SUCCESS           The device was stopped.
  @retval EFI_DEVICE_ERROR      The device could not be stopped due to a device error.

**/
EFI_STATUS
EFIAPI
NvmExpressDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL     *This,
  IN  EFI_HANDLE                      Controller,
  IN  UINTN                           NumberOfChildren,
  IN  EFI_HANDLE                      *ChildHandleBuffer
  );

/**
  Sends an NVM Express Command Packet to an NVM Express controller or namespace. This function supports
  both blocking I/O and nonblocking I/O. The blocking I/O functionality is required, and the nonblocking
  I/O functionality is optional.

  @param[in]     This                A pointer to the EFI_NVM_EXPRESS_PASS_THRU_PROTOCOL instance.
  @param[in]     NamespaceId         Is a 32 bit Namespace ID to which the Express HCI command packet will be sent.
                                     A value of 0 denotes the NVM Express controller, a value of all 0FFh in the namespace
                                     ID specifies that the command packet should be sent to all valid namespaces.
  @param[in,out] Packet              A pointer to the NVM Express HCI Command Packet to send to the NVMe namespace specified
                                     by NamespaceId.
  @param[in]     Event               If nonblocking I/O is not supported then Event is ignored, and blocking I/O is performed.
                                     If Event is NULL, then blocking I/O is performed. If Event is not NULL and non blocking I/O
                                     is supported, then nonblocking I/O is performed, and Event will be signaled when the NVM
                                     Express Command Packet completes.

  @retval EFI_SUCCESS                The NVM Express Command Packet was sent by the host. TransferLength bytes were transferred
                                     to, or from DataBuffer.
  @retval EFI_BAD_BUFFER_SIZE        The NVM Express Command Packet was not executed. The number of bytes that could be transferred
                                     is returned in TransferLength.
  @retval EFI_NOT_READY              The NVM Express Command Packet could not be sent because the controller is not ready. The caller
                                     may retry again later.
  @retval EFI_DEVICE_ERROR           A device error occurred while attempting to send the NVM Express Command Packet.
  @retval EFI_INVALID_PARAMETER      Namespace, or the contents of EFI_NVM_EXPRESS_PASS_THRU_COMMAND_PACKET are invalid. The NVM
                                     Express Command Packet was not sent, so no additional status information is available.
  @retval EFI_UNSUPPORTED            The command described by the NVM Express Command Packet is not supported by the host adapter.
                                     The NVM Express Command Packet was not sent, so no additional status information is available.
  @retval EFI_TIMEOUT                A timeout occurred while waiting for the NVM Express Command Packet to execute.

**/
EFI_STATUS
EFIAPI
NvmExpressPassThru (
  IN     EFI_NVM_EXPRESS_PASS_THRU_PROTOCOL          *This,
  IN     UINT32                                      NamespaceId,
  IN OUT EFI_NVM_EXPRESS_PASS_THRU_COMMAND_PACKET    *Packet,
  IN     EFI_EVENT                                   Event OPTIONAL
  );

/**
  Used to retrieve the next namespace ID for this NVM Express controller.

  The EFI_NVM_EXPRESS_PASS_THRU_PROTOCOL.GetNextNamespace() function retrieves the next valid
  namespace ID on this NVM Express controller.

  If on input the value pointed to by NamespaceId is 0xFFFFFFFF, then the first valid namespace
  ID defined on the NVM Express controller is returned in the location pointed to by NamespaceId
  and a status of EFI_SUCCESS is returned.

  If on input the value pointed to by NamespaceId is an invalid namespace ID other than 0xFFFFFFFF,
  then EFI_INVALID_PARAMETER is returned.

  If on input the value pointed to by NamespaceId is a valid namespace ID, then the next valid
  namespace ID on the NVM Express controller is returned in the location pointed to by NamespaceId,
  and EFI_SUCCESS is returned.

  If the value pointed to by NamespaceId is the namespace ID of the last namespace on the NVM
  Express controller, then EFI_NOT_FOUND is returned.

  @param[in]     This           A pointer to the EFI_NVM_EXPRESS_PASS_THRU_PROTOCOL instance.
  @param[in,out] NamespaceId    On input, a pointer to a legal NamespaceId for an NVM Express
                                namespace present on the NVM Express controller. On output, a
                                pointer to the next NamespaceId of an NVM Express namespace on
                                an NVM Express controller. An input value of 0xFFFFFFFF retrieves
                                the first NamespaceId for an NVM Express namespace present on an
                                NVM Express controller.

  @retval EFI_SUCCESS           The Namespace ID of the next Namespace was returned.
  @retval EFI_NOT_FOUND         There are no more namespaces defined on this controller.
  @retval EFI_INVALID_PARAMETER NamespaceId is an invalid value other than 0xFFFFFFFF.

**/
EFI_STATUS
EFIAPI
NvmExpressGetNextNamespace (
  IN     EFI_NVM_EXPRESS_PASS_THRU_PROTOCOL          *This,
  IN OUT UINT32                                      *NamespaceId
  );

/**
  Used to translate a device path node to a namespace ID.

  The EFI_NVM_EXPRESS_PASS_THRU_PROTOCOL.GetNamespace() function determines the namespace ID associated with the
  namespace described by DevicePath.

  If DevicePath is a device path node type that the NVM Express Pass Thru driver supports, then the NVM Express
  Pass Thru driver will attempt to translate the contents DevicePath into a namespace ID.

  If this translation is successful, then that namespace ID is returned in NamespaceId, and EFI_SUCCESS is returned

  @param[in]  This                A pointer to the EFI_NVM_EXPRESS_PASS_THRU_PROTOCOL instance.
  @param[in]  DevicePath          A pointer to the device path node that describes an NVM Express namespace on
                                  the NVM Express controller.
  @param[out] NamespaceId         The NVM Express namespace ID contained in the device path node.

  @retval EFI_SUCCESS             DevicePath was successfully translated to NamespaceId.
  @retval EFI_INVALID_PARAMETER   If DevicePath or NamespaceId are NULL, then EFI_INVALID_PARAMETER is returned.
  @retval EFI_UNSUPPORTED         If DevicePath is not a device path node type that the NVM Express Pass Thru driver
                                  supports, then EFI_UNSUPPORTED is returned.
  @retval EFI_NOT_FOUND           If DevicePath is a device path node type that the NVM Express Pass Thru driver
                                  supports, but there is not a valid translation from DevicePath to a namespace ID,
                                  then EFI_NOT_FOUND is returned.
**/
EFI_STATUS
EFIAPI
NvmExpressGetNamespace (
  IN     EFI_NVM_EXPRESS_PASS_THRU_PROTOCOL          *This,
  IN     EFI_DEVICE_PATH_PROTOCOL                    *DevicePath,
     OUT UINT32                                      *NamespaceId
  );

/**
  Used to allocate and build a device path node for an NVM Express namespace on an NVM Express controller.

  The EFI_NVM_EXPRESS_PASS_THRU_PROTOCOL.BuildDevicePath() function allocates and builds a single device
  path node for the NVM Express namespace specified by NamespaceId.

  If the NamespaceId is not valid, then EFI_NOT_FOUND is returned.

  If DevicePath is NULL, then EFI_INVALID_PARAMETER is returned.

  If there are not enough resources to allocate the device path node, then EFI_OUT_OF_RESOURCES is returned.

  Otherwise, DevicePath is allocated with the boot service AllocatePool(), the contents of DevicePath are
  initialized to describe the NVM Express namespace specified by NamespaceId, and EFI_SUCCESS is returned.

  @param[in]     This                A pointer to the EFI_NVM_EXPRESS_PASS_THRU_PROTOCOL instance.
  @param[in]     NamespaceId         The NVM Express namespace ID  for which a device path node is to be
                                     allocated and built. Caller must set the NamespaceId to zero if the
                                     device path node will contain a valid UUID.
  @param[in,out] DevicePath          A pointer to a single device path node that describes the NVM Express
                                     namespace specified by NamespaceId. This function is responsible for
                                     allocating the buffer DevicePath with the boot service AllocatePool().
                                     It is the caller's responsibility to free DevicePath when the caller
                                     is finished with DevicePath.
  @retval EFI_SUCCESS                The device path node that describes the NVM Express namespace specified
                                     by NamespaceId was allocated and returned in DevicePath.
  @retval EFI_NOT_FOUND              The NamespaceId is not valid.
  @retval EFI_INVALID_PARAMETER      DevicePath is NULL.
  @retval EFI_OUT_OF_RESOURCES       There are not enough resources to allocate the DevicePath node.

**/
EFI_STATUS
EFIAPI
NvmExpressBuildDevicePath (
  IN     EFI_NVM_EXPRESS_PASS_THRU_PROTOCOL          *This,
  IN     UINT32                                      NamespaceId,
  IN OUT EFI_DEVICE_PATH_PROTOCOL                    **DevicePath
  );

/**
  Dump the execution status from a given completion queue entry.

  @param[in]     Cq               A pointer to the NVME_CQ item.

**/
VOID
NvmeDumpStatus (
  IN NVME_CQ             *Cq
  );

/**
  Register the shutdown notification through the ResetNotification protocol.

  Register the shutdown notification when mNvmeControllerNumber increased from 0 to 1.
**/
VOID
NvmeRegisterShutdownNotification (
  VOID
  );

/**
  Unregister the shutdown notification through the ResetNotification protocol.

  Unregister the shutdown notification when mNvmeControllerNumber decreased from 1 to 0.
**/
VOID
NvmeUnregisterShutdownNotification (
  VOID
  );

#endif
