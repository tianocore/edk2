/** @file
  Master header file for DiskIo driver. It includes the module private defininitions.

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _DISK_IO_H_
#define _DISK_IO_H_

#include <Uefi.h>
#include <Protocol/BlockIo.h>
#include <Protocol/BlockIo2.h>
#include <Protocol/DiskIo2.h>
#include <Protocol/ComponentName.h>
#include <Protocol/DriverBinding.h>
#include <Protocol/DiskIo.h>
#include <Library/DebugLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>

#define DISK_IO_PRIVATE_DATA_SIGNATURE  SIGNATURE_32 ('d', 's', 'k', 'I')
typedef struct {
  UINT32                          Signature;

  EFI_DISK_IO_PROTOCOL            DiskIo;
  EFI_DISK_IO2_PROTOCOL           DiskIo2;
  EFI_BLOCK_IO_PROTOCOL           *BlockIo;
  EFI_BLOCK_IO2_PROTOCOL          *BlockIo2;

  UINT8                           *SharedWorkingBuffer;

  EFI_LOCK                        TaskQueueLock;
  LIST_ENTRY                      TaskQueue;
} DISK_IO_PRIVATE_DATA;
#define DISK_IO_PRIVATE_DATA_FROM_DISK_IO(a)  CR (a, DISK_IO_PRIVATE_DATA, DiskIo,  DISK_IO_PRIVATE_DATA_SIGNATURE)
#define DISK_IO_PRIVATE_DATA_FROM_DISK_IO2(a) CR (a, DISK_IO_PRIVATE_DATA, DiskIo2, DISK_IO_PRIVATE_DATA_SIGNATURE)

#define DISK_IO2_TASK_SIGNATURE   SIGNATURE_32 ('d', 'i', 'a', 't')
typedef struct {
  UINT32                          Signature;
  LIST_ENTRY                      Link;     /// < link to other task
  EFI_LOCK                        SubtasksLock;
  LIST_ENTRY                      Subtasks; /// < header of subtasks
  EFI_DISK_IO2_TOKEN              *Token;
  DISK_IO_PRIVATE_DATA            *Instance;
} DISK_IO2_TASK;

#define DISK_IO2_FLUSH_TASK_SIGNATURE SIGNATURE_32 ('d', 'i', 'f', 't')
typedef struct {
  UINT32                          Signature;
  EFI_BLOCK_IO2_TOKEN             BlockIo2Token;
  EFI_DISK_IO2_TOKEN              *Token;
} DISK_IO2_FLUSH_TASK;

#define DISK_IO_SUBTASK_SIGNATURE SIGNATURE_32 ('d', 'i', 's', 't')
typedef struct {
  //
  // UnderRun:  Offset != 0, Length < BlockSize
  // OverRun:   Offset == 0, Length < BlockSize
  // Middle:    Offset is block aligned, Length is multiple of block size
  //
  UINT32                          Signature;
  LIST_ENTRY                      Link;
  BOOLEAN                         Write;
  UINT64                          Lba;
  UINT32                          Offset;
  UINTN                           Length;
  UINT8                           *WorkingBuffer; /// < NULL indicates using "Buffer" directly
  UINT8                           *Buffer;
  BOOLEAN                         Blocking;

  //
  // Following fields are for DiskIo2
  //
  DISK_IO2_TASK                   *Task;
  EFI_BLOCK_IO2_TOKEN             BlockIo2Token;
} DISK_IO_SUBTASK;

//
// Global Variables
//
extern EFI_DRIVER_BINDING_PROTOCOL   gDiskIoDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL   gDiskIoComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL  gDiskIoComponentName2;

//
// Prototypes
// Driver model protocol interface
//
/**
  Test to see if this driver supports ControllerHandle.

  @param  This                Protocol instance pointer.
  @param  ControllerHandle    Handle of device to test
  @param  RemainingDevicePath Optional parameter use to pick a specific child
                              device to start.

  @retval EFI_SUCCESS         This driver supports this device
  @retval EFI_ALREADY_STARTED This driver is already running on this device
  @retval other               This driver does not support this device

**/
EFI_STATUS
EFIAPI
DiskIoDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  );

/**
  Start this driver on ControllerHandle by opening a Block IO protocol and
  installing a Disk IO protocol on ControllerHandle.

  @param  This                 Protocol instance pointer.
  @param  ControllerHandle     Handle of device to bind driver to
  @param  RemainingDevicePath  Optional parameter use to pick a specific child
                               device to start.

  @retval EFI_SUCCESS          This driver is added to ControllerHandle
  @retval EFI_ALREADY_STARTED  This driver is already running on ControllerHandle
  @retval other                This driver does not support this device

**/
EFI_STATUS
EFIAPI
DiskIoDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  );

/**
  Stop this driver on ControllerHandle by removing Disk IO protocol and closing
  the Block IO protocol on ControllerHandle.

  @param  This              Protocol instance pointer.
  @param  ControllerHandle  Handle of device to stop driver on
  @param  NumberOfChildren  Number of Handles in ChildHandleBuffer. If number of
                            children is zero stop the entire bus driver.
  @param  ChildHandleBuffer List of Child Handles to Stop.

  @retval EFI_SUCCESS       This driver is removed ControllerHandle
  @retval other             This driver was not removed from this device

**/
EFI_STATUS
EFIAPI
DiskIoDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN  EFI_HANDLE                     ControllerHandle,
  IN  UINTN                          NumberOfChildren,
  IN  EFI_HANDLE                     *ChildHandleBuffer
  );

//
// Disk I/O Protocol Interface
//
/**
  Read BufferSize bytes from Offset into Buffer.
  Reads may support reads that are not aligned on
  sector boundaries. There are three cases:
    UnderRun - The first byte is not on a sector boundary or the read request is
               less than a sector in length.
    Aligned  - A read of N contiguous sectors.
    OverRun  - The last byte is not on a sector boundary.

  @param  This                  Protocol instance pointer.
  @param  MediaId               Id of the media, changes every time the media is replaced.
  @param  Offset                The starting byte offset to read from
  @param  BufferSize            Size of Buffer
  @param  Buffer                Buffer containing read data

  @retval EFI_SUCCESS           The data was read correctly from the device.
  @retval EFI_DEVICE_ERROR      The device reported an error while performing the read.
  @retval EFI_NO_MEDIA          There is no media in the device.
  @retval EFI_MEDIA_CHNAGED     The MediaId does not matched the current device.
  @retval EFI_INVALID_PARAMETER The read request contains device addresses that are not
                                valid for the device.

**/
EFI_STATUS
EFIAPI
DiskIoReadDisk (
  IN EFI_DISK_IO_PROTOCOL  *This,
  IN UINT32                MediaId,
  IN UINT64                Offset,
  IN UINTN                 BufferSize,
  OUT VOID                 *Buffer
  );

/**
  Writes BufferSize bytes from Buffer into Offset.
  Writes may require a read modify write to support writes that are not
  aligned on sector boundaries. There are three cases:
    UnderRun - The first byte is not on a sector boundary or the write request
               is less than a sector in length. Read modify write is required.
    Aligned  - A write of N contiguous sectors.
    OverRun  - The last byte is not on a sector boundary. Read modified write
               required.

  @param  This       Protocol instance pointer.
  @param  MediaId    Id of the media, changes every time the media is replaced.
  @param  Offset     The starting byte offset to read from
  @param  BufferSize Size of Buffer
  @param  Buffer     Buffer containing read data

  @retval EFI_SUCCESS           The data was written correctly to the device.
  @retval EFI_WRITE_PROTECTED   The device can not be written to.
  @retval EFI_DEVICE_ERROR      The device reported an error while performing the write.
  @retval EFI_NO_MEDIA          There is no media in the device.
  @retval EFI_MEDIA_CHNAGED     The MediaId does not matched the current device.
  @retval EFI_INVALID_PARAMETER The write request contains device addresses that are not
                                 valid for the device.

**/
EFI_STATUS
EFIAPI
DiskIoWriteDisk (
  IN EFI_DISK_IO_PROTOCOL  *This,
  IN UINT32                MediaId,
  IN UINT64                Offset,
  IN UINTN                 BufferSize,
  IN VOID                  *Buffer
  );


/**
  Terminate outstanding asynchronous requests to a device.

  @param This                   Indicates a pointer to the calling context.

  @retval EFI_SUCCESS           All outstanding requests were successfully terminated.
  @retval EFI_DEVICE_ERROR      The device reported an error while performing the cancel
                                operation.
**/
EFI_STATUS
EFIAPI
DiskIo2Cancel (
  IN EFI_DISK_IO2_PROTOCOL *This
  );

/**
  Reads a specified number of bytes from a device.

  @param This                   Indicates a pointer to the calling context.
  @param MediaId                ID of the medium to be read.
  @param Offset                 The starting byte offset on the logical block I/O device to read from.
  @param Token                  A pointer to the token associated with the transaction.
                                If this field is NULL, synchronous/blocking IO is performed.
  @param  BufferSize            The size in bytes of Buffer. The number of bytes to read from the device.
  @param  Buffer                A pointer to the destination buffer for the data.
                                The caller is responsible either having implicit or explicit ownership of the buffer.

  @retval EFI_SUCCESS           If Event is NULL (blocking I/O): The data was read correctly from the device.
                                If Event is not NULL (asynchronous I/O): The request was successfully queued for processing.
                                                                         Event will be signaled upon completion.
  @retval EFI_DEVICE_ERROR      The device reported an error while performing the write.
  @retval EFI_NO_MEDIA          There is no medium in the device.
  @retval EFI_MEDIA_CHNAGED     The MediaId is not for the current medium.
  @retval EFI_INVALID_PARAMETER The read request contains device addresses that are not valid for the device.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack of resources.

**/
EFI_STATUS
EFIAPI
DiskIo2ReadDiskEx (
  IN EFI_DISK_IO2_PROTOCOL        *This,
  IN UINT32                       MediaId,
  IN UINT64                       Offset,
  IN OUT EFI_DISK_IO2_TOKEN       *Token,
  IN UINTN                        BufferSize,
  OUT VOID                        *Buffer
  );

/**
  Writes a specified number of bytes to a device.

  @param This        Indicates a pointer to the calling context.
  @param MediaId     ID of the medium to be written.
  @param Offset      The starting byte offset on the logical block I/O device to write to.
  @param Token       A pointer to the token associated with the transaction.
                     If this field is NULL, synchronous/blocking IO is performed.
  @param BufferSize  The size in bytes of Buffer. The number of bytes to write to the device.
  @param Buffer      A pointer to the buffer containing the data to be written.

  @retval EFI_SUCCESS           If Event is NULL (blocking I/O): The data was written correctly to the device.
                                If Event is not NULL (asynchronous I/O): The request was successfully queued for processing.
                                                                         Event will be signaled upon completion.
  @retval EFI_WRITE_PROTECTED   The device cannot be written to.
  @retval EFI_DEVICE_ERROR      The device reported an error while performing the write operation.
  @retval EFI_NO_MEDIA          There is no medium in the device.
  @retval EFI_MEDIA_CHNAGED     The MediaId is not for the current medium.
  @retval EFI_INVALID_PARAMETER The write request contains device addresses that are not valid for the device.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack of resources.

**/
EFI_STATUS
EFIAPI
DiskIo2WriteDiskEx (
  IN EFI_DISK_IO2_PROTOCOL        *This,
  IN UINT32                       MediaId,
  IN UINT64                       Offset,
  IN EFI_DISK_IO2_TOKEN           *Token,
  IN UINTN                        BufferSize,
  IN VOID                         *Buffer
  );

/**
  Flushes all modified data to the physical device.

  @param This        Indicates a pointer to the calling context.
  @param Token       A pointer to the token associated with the transaction.
                     If this field is NULL, synchronous/blocking IO is performed.

  @retval EFI_SUCCESS           If Event is NULL (blocking I/O): The data was flushed successfully to the device.
                                If Event is not NULL (asynchronous I/O): The request was successfully queued for processing.
                                                                         Event will be signaled upon completion.
  @retval EFI_WRITE_PROTECTED   The device cannot be written to.
  @retval EFI_DEVICE_ERROR      The device reported an error while performing the write operation.
  @retval EFI_NO_MEDIA          There is no medium in the device.
  @retval EFI_OUT_OF_RESOURCES  The request could not be completed due to a lack of resources.
**/
EFI_STATUS
EFIAPI
DiskIo2FlushDiskEx (
  IN EFI_DISK_IO2_PROTOCOL        *This,
  IN OUT EFI_DISK_IO2_TOKEN       *Token
  );

//
// EFI Component Name Functions
//
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
DiskIoComponentNameGetDriverName (
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
DiskIoComponentNameGetControllerName (
  IN  EFI_COMPONENT_NAME_PROTOCOL                     *This,
  IN  EFI_HANDLE                                      ControllerHandle,
  IN  EFI_HANDLE                                      ChildHandle        OPTIONAL,
  IN  CHAR8                                           *Language,
  OUT CHAR16                                          **ControllerName
  );


#endif
