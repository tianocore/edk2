/** @file
  Master header file for ATA Bus Driver.

  This file defines common data structures, macro definitions and some module
  internal function header files.

  Copyright (c) 2009 - 2015, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _ATA_BUS_H_
#define _ATA_BUS_H_

#include <Uefi.h>

#include <Protocol/AtaPassThru.h>
#include <Protocol/BlockIo.h>
#include <Protocol/BlockIo2.h>
#include <Protocol/DiskInfo.h>
#include <Protocol/DevicePath.h>
#include <Protocol/StorageSecurityCommand.h>

#include <Library/DebugLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/BaseLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DevicePathLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/TimerLib.h>
#include <Library/ReportStatusCodeLib.h>

#include <IndustryStandard/Atapi.h>

//
// Time out value for ATA pass through protocol
//
#define ATA_TIMEOUT                       EFI_TIMER_PERIOD_SECONDS (3)

//
// Maximum number of times to retry ATA command
//
#define MAX_RETRY_TIMES                   3

//
// The maximum total sectors count in 28 bit addressing mode
//
#define MAX_28BIT_ADDRESSING_CAPACITY     0xfffffff

//
// The maximum ATA transaction sector count in 28 bit addressing mode.
//
#define MAX_28BIT_TRANSFER_BLOCK_NUM      0x100

//
// The maximum ATA transaction sector count in 48 bit addressing mode.
//
//#define MAX_48BIT_TRANSFER_BLOCK_NUM      0x10000

//
// BugBug: if the TransferLength is equal with 0x10000 (the 48bit max length),
// there is a bug that even the register interrupt bit has been sit, the buffer
// seems not ready. Change the Maximum Sector Numbers to 0xFFFF to work round
// this issue.
//
#define MAX_48BIT_TRANSFER_BLOCK_NUM      0xFFFF

//
// The maximum model name in ATA identify data
//
#define MAX_MODEL_NAME_LEN                40

#define ATA_TASK_SIGNATURE                SIGNATURE_32 ('A', 'T', 'S', 'K')
#define ATA_DEVICE_SIGNATURE              SIGNATURE_32 ('A', 'B', 'I', 'D')
#define ATA_SUB_TASK_SIGNATURE            SIGNATURE_32 ('A', 'S', 'T', 'S')
#define IS_ALIGNED(addr, size)            (((UINTN) (addr) & (size - 1)) == 0)

//
// ATA bus data structure for ATA controller
//
typedef struct {
  EFI_ATA_PASS_THRU_PROTOCOL  *AtaPassThru;
  EFI_HANDLE                  Controller;
  EFI_DEVICE_PATH_PROTOCOL    *ParentDevicePath;
  EFI_HANDLE                  DriverBindingHandle;
} ATA_BUS_DRIVER_DATA;

//
// ATA device data structure for each child device
//
typedef struct {
  UINT32                                Signature;

  EFI_HANDLE                            Handle;
  EFI_BLOCK_IO_PROTOCOL                 BlockIo;
  EFI_BLOCK_IO2_PROTOCOL                BlockIo2;
  EFI_BLOCK_IO_MEDIA                    BlockMedia;
  EFI_DISK_INFO_PROTOCOL                DiskInfo;
  EFI_DEVICE_PATH_PROTOCOL              *DevicePath;
  EFI_STORAGE_SECURITY_COMMAND_PROTOCOL StorageSecurity;

  ATA_BUS_DRIVER_DATA                   *AtaBusDriverData;
  UINT16                                Port;
  UINT16                                PortMultiplierPort;

  //
  // Buffer for the execution of ATA pass through protocol
  //
  EFI_ATA_PASS_THRU_COMMAND_PACKET      Packet;
  EFI_ATA_COMMAND_BLOCK                 Acb;
  EFI_ATA_STATUS_BLOCK                  *Asb;

  BOOLEAN                               UdmaValid;
  BOOLEAN                               Lba48Bit;

  //
  // Cached data for ATA identify data
  //
  ATA_IDENTIFY_DATA                     *IdentifyData;

  EFI_UNICODE_STRING_TABLE              *ControllerNameTable;
  CHAR16                                ModelName[MAX_MODEL_NAME_LEN + 1];

  LIST_ENTRY                            AtaTaskList;
  LIST_ENTRY                            AtaSubTaskList;
  BOOLEAN                               Abort;
} ATA_DEVICE;

//
// Sub-Task for the non blocking I/O
//
typedef struct {
  UINT32                            Signature;
  ATA_DEVICE                        *AtaDevice;
  EFI_BLOCK_IO2_TOKEN               *Token;
  UINTN                             *UnsignalledEventCount;
  EFI_ATA_PASS_THRU_COMMAND_PACKET  Packet;
  BOOLEAN                           *IsError;// Indicate whether meeting error during source allocation for new task.
  LIST_ENTRY                        TaskEntry;
} ATA_BUS_ASYN_SUB_TASK;

//
// Task for the non blocking I/O
//
typedef struct {
  UINT32                            Signature;
  EFI_BLOCK_IO2_TOKEN               *Token;
  ATA_DEVICE                        *AtaDevice;
  UINT8                             *Buffer;
  EFI_LBA                           StartLba;
  UINTN                             NumberOfBlocks;
  BOOLEAN                           IsWrite;
  LIST_ENTRY                        TaskEntry;
} ATA_BUS_ASYN_TASK;

#define ATA_DEVICE_FROM_BLOCK_IO(a)         CR (a, ATA_DEVICE, BlockIo, ATA_DEVICE_SIGNATURE)
#define ATA_DEVICE_FROM_BLOCK_IO2(a)        CR (a, ATA_DEVICE, BlockIo2, ATA_DEVICE_SIGNATURE)
#define ATA_DEVICE_FROM_DISK_INFO(a)        CR (a, ATA_DEVICE, DiskInfo, ATA_DEVICE_SIGNATURE)
#define ATA_DEVICE_FROM_STORAGE_SECURITY(a) CR (a, ATA_DEVICE, StorageSecurity, ATA_DEVICE_SIGNATURE)
#define ATA_ASYN_SUB_TASK_FROM_ENTRY(a)     CR (a, ATA_BUS_ASYN_SUB_TASK, TaskEntry, ATA_SUB_TASK_SIGNATURE)
#define ATA_ASYN_TASK_FROM_ENTRY(a)         CR (a, ATA_BUS_ASYN_TASK, TaskEntry, ATA_TASK_SIGNATURE)

//
// Global Variables
//
extern EFI_DRIVER_BINDING_PROTOCOL        gAtaBusDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL        gAtaBusComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL       gAtaBusComponentName2;

/**
  Allocates an aligned buffer for ATA device.

  This function allocates an aligned buffer for the ATA device to perform
  ATA pass through operations. The alignment requirement is from ATA pass
  through interface.

  @param  AtaDevice         The ATA child device involved for the operation.
  @param  BufferSize        The request buffer size.

  @return A pointer to the aligned buffer or NULL if the allocation fails.

**/
VOID *
AllocateAlignedBuffer (
  IN ATA_DEVICE               *AtaDevice,
  IN UINTN                    BufferSize
  );

/**
  Frees an aligned buffer for ATA device.

  This function frees an aligned buffer for the ATA device to perform
  ATA pass through operations.

  @param  Buffer            The aligned buffer to be freed.
  @param  BufferSize        The request buffer size.

**/
VOID
FreeAlignedBuffer (
  IN VOID                     *Buffer,
  IN UINTN                    BufferSize
  );

/**
  Free SubTask.

  @param[in, out]  Task      Pointer to task to be freed.

**/
VOID
EFIAPI
FreeAtaSubTask (
  IN OUT ATA_BUS_ASYN_SUB_TASK  *Task
  );

/**
  Wrapper for EFI_ATA_PASS_THRU_PROTOCOL.ResetDevice().

  This function wraps the ResetDevice() invocation for ATA pass through function
  for an ATA device.

  @param  AtaDevice         The ATA child device involved for the operation.

  @return The return status from EFI_ATA_PASS_THRU_PROTOCOL.PassThru().

**/
EFI_STATUS
ResetAtaDevice (
  IN ATA_DEVICE                           *AtaDevice
  );


/**
  Discovers whether it is a valid ATA device.

  This function issues ATA_CMD_IDENTIFY_DRIVE command to the ATA device to identify it.
  If the command is executed successfully, it then identifies it and initializes
  the Media information in Block IO protocol interface.

  @param  AtaDevice         The ATA child device involved for the operation.

  @retval EFI_SUCCESS       The device is successfully identified and Media information
                            is correctly initialized.
  @return others            Some error occurs when discovering the ATA device.

**/
EFI_STATUS
DiscoverAtaDevice (
  IN OUT ATA_DEVICE                 *AtaDevice
  );

/**
  Read or write a number of blocks from ATA device.

  This function performs ATA pass through transactions to read/write data from/to
  ATA device. It may separate the read/write request into several ATA pass through
  transactions.

  @param[in, out]  AtaDevice       The ATA child device involved for the operation.
  @param[in, out]  Buffer          The pointer to the current transaction buffer.
  @param[in]       StartLba        The starting logical block address to be accessed.
  @param[in]       NumberOfBlocks  The block number or sector count of the transfer.
  @param[in]       IsWrite         Indicates whether it is a write operation.
  @param[in, out]  Token           A pointer to the token associated with the transaction.

  @retval EFI_SUCCESS       The data transfer is complete successfully.
  @return others            Some error occurs when transferring data.

**/
EFI_STATUS
AccessAtaDevice(
  IN OUT ATA_DEVICE                 *AtaDevice,
  IN OUT UINT8                      *Buffer,
  IN EFI_LBA                        StartLba,
  IN UINTN                          NumberOfBlocks,
  IN BOOLEAN                        IsWrite,
  IN OUT EFI_BLOCK_IO2_TOKEN        *Token
  );

/**
  Trust transfer data from/to ATA device.

  This function performs one ATA pass through transaction to do a trust transfer from/to
  ATA device. It chooses the appropriate ATA command and protocol to invoke PassThru
  interface of ATA pass through.

  @param  AtaDevice                    The ATA child device involved for the operation.
  @param  Buffer                       The pointer to the current transaction buffer.
  @param  SecurityProtocolId           The value of the "Security Protocol" parameter of
                                       the security protocol command to be sent.
  @param  SecurityProtocolSpecificData The value of the "Security Protocol Specific" parameter
                                       of the security protocol command to be sent.
  @param  TransferLength               The block number or sector count of the transfer.
  @param  IsTrustSend                  Indicates whether it is a trust send operation or not.
  @param  Timeout                      The timeout, in 100ns units, to use for the execution
                                       of the security protocol command. A Timeout value of 0
                                       means that this function will wait indefinitely for the
                                       security protocol command to execute. If Timeout is greater
                                       than zero, then this function will return EFI_TIMEOUT
                                       if the time required to execute the receive data command
                                       is greater than Timeout.
  @param  TransferLengthOut            A pointer to a buffer to store the size in bytes of the data
                                       written to the buffer. Ignore it when IsTrustSend is TRUE.

  @retval EFI_SUCCESS       The data transfer is complete successfully.
  @return others            Some error occurs when transferring data.

**/
EFI_STATUS
EFIAPI
TrustTransferAtaDevice (
  IN OUT ATA_DEVICE                 *AtaDevice,
  IN OUT VOID                       *Buffer,
  IN UINT8                          SecurityProtocolId,
  IN UINT16                         SecurityProtocolSpecificData,
  IN UINTN                          TransferLength,
  IN BOOLEAN                        IsTrustSend,
  IN UINT64                         Timeout,
  OUT UINTN                         *TransferLengthOut
  );

//
// Protocol interface prototypes
//
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
AtaBusDriverBindingSupported (
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
AtaBusDriverBindingStart (
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
AtaBusDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL     *This,
  IN  EFI_HANDLE                      Controller,
  IN  UINTN                           NumberOfChildren,
  IN  EFI_HANDLE                      *ChildHandleBuffer
  );


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
AtaBusComponentNameGetDriverName (
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
AtaBusComponentNameGetControllerName (
  IN  EFI_COMPONENT_NAME_PROTOCOL                     *This,
  IN  EFI_HANDLE                                      ControllerHandle,
  IN  EFI_HANDLE                                      ChildHandle        OPTIONAL,
  IN  CHAR8                                           *Language,
  OUT CHAR16                                          **ControllerName
  );


/**
  Reset the Block Device.

  @param  This                 Indicates a pointer to the calling context.
  @param  ExtendedVerification Driver may perform diagnostics on reset.

  @retval EFI_SUCCESS          The device was reset.
  @retval EFI_DEVICE_ERROR     The device is not functioning properly and could
                               not be reset.

**/
EFI_STATUS
EFIAPI
AtaBlockIoReset (
  IN  EFI_BLOCK_IO_PROTOCOL   *This,
  IN  BOOLEAN                 ExtendedVerification
  );


/**
  Read BufferSize bytes from Lba into Buffer.

  @param  This       Indicates a pointer to the calling context.
  @param  MediaId    Id of the media, changes every time the media is replaced.
  @param  Lba        The starting Logical Block Address to read from
  @param  BufferSize Size of Buffer, must be a multiple of device block size.
  @param  Buffer     A pointer to the destination buffer for the data. The caller is
                     responsible for either having implicit or explicit ownership of the buffer.

  @retval EFI_SUCCESS           The data was read correctly from the device.
  @retval EFI_DEVICE_ERROR      The device reported an error while performing the read.
  @retval EFI_NO_MEDIA          There is no media in the device.
  @retval EFI_MEDIA_CHANGED     The MediaId does not matched the current device.
  @retval EFI_BAD_BUFFER_SIZE   The Buffer was not a multiple of the block size of the device.
  @retval EFI_INVALID_PARAMETER The read request contains LBAs that are not valid,
                                or the buffer is not on proper alignment.

**/
EFI_STATUS
EFIAPI
AtaBlockIoReadBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL   *This,
  IN  UINT32                  MediaId,
  IN  EFI_LBA                 Lba,
  IN  UINTN                   BufferSize,
  OUT VOID                    *Buffer
  );


/**
  Write BufferSize bytes from Lba into Buffer.

  @param  This       Indicates a pointer to the calling context.
  @param  MediaId    The media ID that the write request is for.
  @param  Lba        The starting logical block address to be written. The caller is
                     responsible for writing to only legitimate locations.
  @param  BufferSize Size of Buffer, must be a multiple of device block size.
  @param  Buffer     A pointer to the source buffer for the data.

  @retval EFI_SUCCESS           The data was written correctly to the device.
  @retval EFI_WRITE_PROTECTED   The device can not be written to.
  @retval EFI_DEVICE_ERROR      The device reported an error while performing the write.
  @retval EFI_NO_MEDIA          There is no media in the device.
  @retval EFI_MEDIA_CHNAGED     The MediaId does not matched the current device.
  @retval EFI_BAD_BUFFER_SIZE   The Buffer was not a multiple of the block size of the device.
  @retval EFI_INVALID_PARAMETER The write request contains LBAs that are not valid,
                                or the buffer is not on proper alignment.

**/
EFI_STATUS
EFIAPI
AtaBlockIoWriteBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL   *This,
  IN  UINT32                  MediaId,
  IN  EFI_LBA                 Lba,
  IN  UINTN                   BufferSize,
  IN  VOID                    *Buffer
  );


/**
  Flush the Block Device.

  @param  This              Indicates a pointer to the calling context.

  @retval EFI_SUCCESS       All outstanding data was written to the device
  @retval EFI_DEVICE_ERROR  The device reported an error while writing back the data
  @retval EFI_NO_MEDIA      There is no media in the device.

**/
EFI_STATUS
EFIAPI
AtaBlockIoFlushBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL   *This
  );

/**
  Reset the Block Device throught Block I/O2 protocol.

  @param[in]  This                 Indicates a pointer to the calling context.
  @param[in]  ExtendedVerification Driver may perform diagnostics on reset.

  @retval EFI_SUCCESS          The device was reset.
  @retval EFI_DEVICE_ERROR     The device is not functioning properly and could
                               not be reset.

**/
EFI_STATUS
EFIAPI
AtaBlockIoResetEx (
  IN  EFI_BLOCK_IO2_PROTOCOL  *This,
  IN  BOOLEAN                 ExtendedVerification
  );

/**
  Read BufferSize bytes from Lba into Buffer.

  @param[in]       This       Indicates a pointer to the calling context.
  @param[in]       MediaId    Id of the media, changes every time the media is replaced.
  @param[in]       Lba        The starting Logical Block Address to read from.
  @param[in, out]  Token      A pointer to the token associated with the transaction.
  @param[in]       BufferSize Size of Buffer, must be a multiple of device block size.
  @param[out]      Buffer     A pointer to the destination buffer for the data. The caller is
                              responsible for either having implicit or explicit ownership of the buffer.

  @retval EFI_SUCCESS           The read request was queued if Event is not NULL.
                                The data was read correctly from the device if
                                the Event is NULL.
  @retval EFI_DEVICE_ERROR      The device reported an error while performing
                                the read.
  @retval EFI_NO_MEDIA          There is no media in the device.
  @retval EFI_MEDIA_CHANGED     The MediaId is not for the current media.
  @retval EFI_BAD_BUFFER_SIZE   The BufferSize parameter is not a multiple of the
                                intrinsic block size of the device.
  @retval EFI_INVALID_PARAMETER The read request contains LBAs that are not valid,
                                or the buffer is not on proper alignment.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack
                                of resources.

**/
EFI_STATUS
EFIAPI
AtaBlockIoReadBlocksEx (
  IN  EFI_BLOCK_IO2_PROTOCOL  *This,
  IN  UINT32                  MediaId,
  IN  EFI_LBA                 Lba,
  IN OUT EFI_BLOCK_IO2_TOKEN  *Token,
  IN  UINTN                   BufferSize,
  OUT VOID                    *Buffer
  );

/**
  Write BufferSize bytes from Lba into Buffer.

  @param[in]       This       Indicates a pointer to the calling context.
  @param[in]       MediaId    The media ID that the write request is for.
  @param[in]       Lba        The starting logical block address to be written. The
                              caller is responsible for writing to only legitimate
                              locations.
  @param[in, out]  Token      A pointer to the token associated with the transaction.
  @param[in]       BufferSize Size of Buffer, must be a multiple of device block size.
  @param[in]       Buffer     A pointer to the source buffer for the data.

  @retval EFI_SUCCESS           The data was written correctly to the device.
  @retval EFI_WRITE_PROTECTED   The device can not be written to.
  @retval EFI_DEVICE_ERROR      The device reported an error while performing the write.
  @retval EFI_NO_MEDIA          There is no media in the device.
  @retval EFI_MEDIA_CHNAGED     The MediaId does not matched the current device.
  @retval EFI_BAD_BUFFER_SIZE   The Buffer was not a multiple of the block size of the device.
  @retval EFI_INVALID_PARAMETER The write request contains LBAs that are not valid,
                                or the buffer is not on proper alignment.

**/
EFI_STATUS
EFIAPI
AtaBlockIoWriteBlocksEx (
  IN  EFI_BLOCK_IO2_PROTOCOL  *This,
  IN  UINT32                  MediaId,
  IN  EFI_LBA                 Lba,
  IN OUT EFI_BLOCK_IO2_TOKEN  *Token,
  IN  UINTN                   BufferSize,
  IN  VOID                    *Buffer
  );

/**
  Flush the Block Device.

  @param[in]       This       Indicates a pointer to the calling context.
  @param[in, out]  Token      A pointer to the token associated with the transaction.

  @retval EFI_SUCCESS       All outstanding data was written to the device
  @retval EFI_DEVICE_ERROR  The device reported an error while writing back the data
  @retval EFI_NO_MEDIA      There is no media in the device.

**/
EFI_STATUS
EFIAPI
AtaBlockIoFlushBlocksEx (
  IN  EFI_BLOCK_IO2_PROTOCOL  *This,
  IN OUT EFI_BLOCK_IO2_TOKEN  *Token
  );

/**
  Terminate any in-flight non-blocking I/O requests by signaling an EFI_ABORTED
  in the TransactionStatus member of the EFI_BLOCK_IO2_TOKEN for the non-blocking
  I/O. After that it is safe to free any Token or Buffer data structures that
  were allocated to initiate the non-blockingI/O requests that were in-flight for
  this device.

  @param[in]  AtaDevice     The ATA child device involved for the operation.

**/
VOID
EFIAPI
AtaTerminateNonBlockingTask (
  IN ATA_DEVICE               *AtaDevice
  );

/**
  Provides inquiry information for the controller type.

  This function is used by the IDE bus driver to get inquiry data.  Data format
  of Identify data is defined by the Interface GUID.

  @param[in]      This             Pointer to the EFI_DISK_INFO_PROTOCOL instance.
  @param[in, out] InquiryData      Pointer to a buffer for the inquiry data.
  @param[in, out] InquiryDataSize  Pointer to the value for the inquiry data size.

  @retval EFI_SUCCESS            The command was accepted without any errors.
  @retval EFI_NOT_FOUND          Device does not support this data class
  @retval EFI_DEVICE_ERROR       Error reading InquiryData from device
  @retval EFI_BUFFER_TOO_SMALL   InquiryDataSize not big enough

**/
EFI_STATUS
EFIAPI
AtaDiskInfoInquiry (
  IN     EFI_DISK_INFO_PROTOCOL   *This,
  IN OUT VOID                     *InquiryData,
  IN OUT UINT32                   *InquiryDataSize
  );


/**
  Provides identify information for the controller type.

  This function is used by the IDE bus driver to get identify data.  Data format
  of Identify data is defined by the Interface GUID.

  @param[in]      This              Pointer to the EFI_DISK_INFO_PROTOCOL
                                    instance.
  @param[in, out] IdentifyData      Pointer to a buffer for the identify data.
  @param[in, out] IdentifyDataSize  Pointer to the value for the identify data
                                    size.

  @retval EFI_SUCCESS            The command was accepted without any errors.
  @retval EFI_NOT_FOUND          Device does not support this data class
  @retval EFI_DEVICE_ERROR       Error reading IdentifyData from device
  @retval EFI_BUFFER_TOO_SMALL   IdentifyDataSize not big enough

**/
EFI_STATUS
EFIAPI
AtaDiskInfoIdentify (
  IN     EFI_DISK_INFO_PROTOCOL   *This,
  IN OUT VOID                     *IdentifyData,
  IN OUT UINT32                   *IdentifyDataSize
  );


/**
  Provides sense data information for the controller type.

  This function is used by the IDE bus driver to get sense data.
  Data format of Sense data is defined by the Interface GUID.

  @param[in]      This             Pointer to the EFI_DISK_INFO_PROTOCOL instance.
  @param[in, out] SenseData        Pointer to the SenseData.
  @param[in, out] SenseDataSize    Size of SenseData in bytes.
  @param[out]     SenseDataNumber  Pointer to the value for the sense data size.

  @retval EFI_SUCCESS            The command was accepted without any errors.
  @retval EFI_NOT_FOUND          Device does not support this data class.
  @retval EFI_DEVICE_ERROR       Error reading SenseData from device.
  @retval EFI_BUFFER_TOO_SMALL   SenseDataSize not big enough.

**/
EFI_STATUS
EFIAPI
AtaDiskInfoSenseData (
  IN     EFI_DISK_INFO_PROTOCOL   *This,
  IN OUT VOID                     *SenseData,
  IN OUT UINT32                   *SenseDataSize,
  OUT    UINT8                    *SenseDataNumber
  );


/**
  This function is used by the IDE bus driver to get controller information.

  @param[in]  This         Pointer to the EFI_DISK_INFO_PROTOCOL instance.
  @param[out] IdeChannel   Pointer to the Ide Channel number.  Primary or secondary.
  @param[out] IdeDevice    Pointer to the Ide Device number.  Master or slave.

  @retval EFI_SUCCESS       IdeChannel and IdeDevice are valid.
  @retval EFI_UNSUPPORTED   This is not an IDE device.

**/
EFI_STATUS
EFIAPI
AtaDiskInfoWhichIde (
  IN  EFI_DISK_INFO_PROTOCOL   *This,
  OUT UINT32                   *IdeChannel,
  OUT UINT32                   *IdeDevice
  );

/**
  Send a security protocol command to a device that receives data and/or the result
  of one or more commands sent by SendData.

  The ReceiveData function sends a security protocol command to the given MediaId.
  The security protocol command sent is defined by SecurityProtocolId and contains
  the security protocol specific data SecurityProtocolSpecificData. The function
  returns the data from the security protocol command in PayloadBuffer.

  For devices supporting the SCSI command set, the security protocol command is sent
  using the SECURITY PROTOCOL IN command defined in SPC-4.

  For devices supporting the ATA command set, the security protocol command is sent
  using one of the TRUSTED RECEIVE commands defined in ATA8-ACS if PayloadBufferSize
  is non-zero.

  If the PayloadBufferSize is zero, the security protocol command is sent using the
  Trusted Non-Data command defined in ATA8-ACS.

  If PayloadBufferSize is too small to store the available data from the security
  protocol command, the function shall copy PayloadBufferSize bytes into the
  PayloadBuffer and return EFI_WARN_BUFFER_TOO_SMALL.

  If PayloadBuffer or PayloadTransferSize is NULL and PayloadBufferSize is non-zero,
  the function shall return EFI_INVALID_PARAMETER.

  If the given MediaId does not support security protocol commands, the function shall
  return EFI_UNSUPPORTED. If there is no media in the device, the function returns
  EFI_NO_MEDIA. If the MediaId is not the ID for the current media in the device,
  the function returns EFI_MEDIA_CHANGED.

  If the security protocol fails to complete within the Timeout period, the function
  shall return EFI_TIMEOUT.

  If the security protocol command completes without an error, the function shall
  return EFI_SUCCESS. If the security protocol command completes with an error, the
  function shall return EFI_DEVICE_ERROR.

  @param  This                         Indicates a pointer to the calling context.
  @param  MediaId                      ID of the medium to receive data from.
  @param  Timeout                      The timeout, in 100ns units, to use for the execution
                                       of the security protocol command. A Timeout value of 0
                                       means that this function will wait indefinitely for the
                                       security protocol command to execute. If Timeout is greater
                                       than zero, then this function will return EFI_TIMEOUT
                                       if the time required to execute the receive data command
                                       is greater than Timeout.
  @param  SecurityProtocolId           The value of the "Security Protocol" parameter of
                                       the security protocol command to be sent.
  @param  SecurityProtocolSpecificData The value of the "Security Protocol Specific" parameter
                                       of the security protocol command to be sent.
  @param  PayloadBufferSize            Size in bytes of the payload data buffer.
  @param  PayloadBuffer                A pointer to a destination buffer to store the security
                                       protocol command specific payload data for the security
                                       protocol command. The caller is responsible for having
                                       either implicit or explicit ownership of the buffer.
  @param  PayloadTransferSize          A pointer to a buffer to store the size in bytes of the
                                       data written to the payload data buffer.

  @retval EFI_SUCCESS                  The security protocol command completed successfully.
  @retval EFI_WARN_BUFFER_TOO_SMALL    The PayloadBufferSize was too small to store the available
                                       data from the device. The PayloadBuffer contains the truncated data.
  @retval EFI_UNSUPPORTED              The given MediaId does not support security protocol commands.
  @retval EFI_DEVICE_ERROR             The security protocol command completed with an error.
  @retval EFI_NO_MEDIA                 There is no media in the device.
  @retval EFI_MEDIA_CHANGED            The MediaId is not for the current media.
  @retval EFI_INVALID_PARAMETER        The PayloadBuffer or PayloadTransferSize is NULL and
                                       PayloadBufferSize is non-zero.
  @retval EFI_TIMEOUT                  A timeout occurred while waiting for the security
                                       protocol command to execute.

**/
EFI_STATUS
EFIAPI
AtaStorageSecurityReceiveData (
  IN EFI_STORAGE_SECURITY_COMMAND_PROTOCOL    *This,
  IN UINT32                                   MediaId,
  IN UINT64                                   Timeout,
  IN UINT8                                    SecurityProtocolId,
  IN UINT16                                   SecurityProtocolSpecificData,
  IN UINTN                                    PayloadBufferSize,
  OUT VOID                                    *PayloadBuffer,
  OUT UINTN                                   *PayloadTransferSize
  );

/**
  Send a security protocol command to a device.

  The SendData function sends a security protocol command containing the payload
  PayloadBuffer to the given MediaId. The security protocol command sent is
  defined by SecurityProtocolId and contains the security protocol specific data
  SecurityProtocolSpecificData. If the underlying protocol command requires a
  specific padding for the command payload, the SendData function shall add padding
  bytes to the command payload to satisfy the padding requirements.

  For devices supporting the SCSI command set, the security protocol command is sent
  using the SECURITY PROTOCOL OUT command defined in SPC-4.

  For devices supporting the ATA command set, the security protocol command is sent
  using one of the TRUSTED SEND commands defined in ATA8-ACS if PayloadBufferSize
  is non-zero. If the PayloadBufferSize is zero, the security protocol command is
  sent using the Trusted Non-Data command defined in ATA8-ACS.

  If PayloadBuffer is NULL and PayloadBufferSize is non-zero, the function shall
  return EFI_INVALID_PARAMETER.

  If the given MediaId does not support security protocol commands, the function
  shall return EFI_UNSUPPORTED. If there is no media in the device, the function
  returns EFI_NO_MEDIA. If the MediaId is not the ID for the current media in the
  device, the function returns EFI_MEDIA_CHANGED.

  If the security protocol fails to complete within the Timeout period, the function
  shall return EFI_TIMEOUT.

  If the security protocol command completes without an error, the function shall return
  EFI_SUCCESS. If the security protocol command completes with an error, the function
  shall return EFI_DEVICE_ERROR.

  @param  This                         Indicates a pointer to the calling context.
  @param  MediaId                      ID of the medium to receive data from.
  @param  Timeout                      The timeout, in 100ns units, to use for the execution
                                       of the security protocol command. A Timeout value of 0
                                       means that this function will wait indefinitely for the
                                       security protocol command to execute. If Timeout is greater
                                       than zero, then this function will return EFI_TIMEOUT
                                       if the time required to execute the receive data command
                                       is greater than Timeout.
  @param  SecurityProtocolId           The value of the "Security Protocol" parameter of
                                       the security protocol command to be sent.
  @param  SecurityProtocolSpecificData The value of the "Security Protocol Specific" parameter
                                       of the security protocol command to be sent.
  @param  PayloadBufferSize            Size in bytes of the payload data buffer.
  @param  PayloadBuffer                A pointer to a destination buffer to store the security
                                       protocol command specific payload data for the security
                                       protocol command.

  @retval EFI_SUCCESS                  The security protocol command completed successfully.
  @retval EFI_UNSUPPORTED              The given MediaId does not support security protocol commands.
  @retval EFI_DEVICE_ERROR             The security protocol command completed with an error.
  @retval EFI_NO_MEDIA                 There is no media in the device.
  @retval EFI_MEDIA_CHANGED            The MediaId is not for the current media.
  @retval EFI_INVALID_PARAMETER        The PayloadBuffer is NULL and PayloadBufferSize is non-zero.
  @retval EFI_TIMEOUT                  A timeout occurred while waiting for the security
                                       protocol command to execute.

**/
EFI_STATUS
EFIAPI
AtaStorageSecuritySendData (
  IN EFI_STORAGE_SECURITY_COMMAND_PROTOCOL    *This,
  IN UINT32                                   MediaId,
  IN UINT64                                   Timeout,
  IN UINT8                                    SecurityProtocolId,
  IN UINT16                                   SecurityProtocolSpecificData,
  IN UINTN                                    PayloadBufferSize,
  IN VOID                                     *PayloadBuffer
  );

/**
  Send TPer Reset command to reset eDrive to lock all protected bands.
  Typically, there are 2 mechanism for resetting eDrive. They are:
  1. TPer Reset through IEEE 1667 protocol.
  2. TPer Reset through native TCG protocol.
  This routine will detect what protocol the attached eDrive comform to, TCG or
  IEEE 1667 protocol. Then send out TPer Reset command separately.

  @param[in] AtaDevice    ATA_DEVICE pointer.

**/
VOID
InitiateTPerReset (
  IN   ATA_DEVICE       *AtaDevice
  );

#endif
