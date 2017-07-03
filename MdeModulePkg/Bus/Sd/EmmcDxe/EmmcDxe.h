/** @file
  Header file for EmmcDxe Driver.

  This file defines common data structures, macro definitions and some module
  internal function header files.

  Copyright (c) 2015 - 2017, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _EMMC_DXE_H_
#define _EMMC_DXE_H_

#include <Uefi.h>
#include <IndustryStandard/Emmc.h>

#include <Protocol/SdMmcPassThru.h>
#include <Protocol/BlockIo.h>
#include <Protocol/BlockIo2.h>
#include <Protocol/StorageSecurityCommand.h>
#include <Protocol/EraseBlock.h>
#include <Protocol/DiskInfo.h>

#include <Protocol/DevicePath.h>

#include <Library/DebugLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/BaseLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DevicePathLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include "EmmcBlockIo.h"
#include "EmmcDiskInfo.h"

//
// Global Variables
//
extern EFI_DRIVER_BINDING_PROTOCOL      gEmmcDxeDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL      gEmmcDxeComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL     gEmmcDxeComponentName2;

#define EMMC_PARTITION_SIGNATURE        SIGNATURE_32 ('E', 'm', 'm', 'P')

#define EMMC_PARTITION_DATA_FROM_BLKIO(a) \
    CR(a, EMMC_PARTITION, BlockIo, EMMC_PARTITION_SIGNATURE)

#define EMMC_PARTITION_DATA_FROM_BLKIO2(a) \
    CR(a, EMMC_PARTITION, BlockIo2, EMMC_PARTITION_SIGNATURE)

#define EMMC_PARTITION_DATA_FROM_SSP(a) \
    CR(a, EMMC_PARTITION, StorageSecurity, EMMC_PARTITION_SIGNATURE)

#define EMMC_PARTITION_DATA_FROM_ERASEBLK(a) \
    CR(a, EMMC_PARTITION, EraseBlock, EMMC_PARTITION_SIGNATURE)

#define EMMC_PARTITION_DATA_FROM_DISKINFO(a) \
    CR(a, EMMC_PARTITION, DiskInfo, EMMC_PARTITION_SIGNATURE)

//
// Take 2.5 seconds as generic time out value, 1 microsecond as unit.
//
#define EMMC_GENERIC_TIMEOUT             2500 * 1000

#define EMMC_REQUEST_SIGNATURE           SIGNATURE_32 ('E', 'm', 'R', 'e')

typedef struct _EMMC_DEVICE              EMMC_DEVICE;
typedef struct _EMMC_DRIVER_PRIVATE_DATA EMMC_DRIVER_PRIVATE_DATA;

//
// Asynchronous I/O request.
//
typedef struct {
  UINT32                                Signature;
  LIST_ENTRY                            Link;

  EFI_SD_MMC_COMMAND_BLOCK              SdMmcCmdBlk;
  EFI_SD_MMC_STATUS_BLOCK               SdMmcStatusBlk;
  EFI_SD_MMC_PASS_THRU_COMMAND_PACKET   Packet;

  BOOLEAN                               IsEnd;

  EFI_BLOCK_IO2_TOKEN                   *Token;
  EFI_EVENT                             Event;
} EMMC_REQUEST;

#define EMMC_REQUEST_FROM_LINK(a) \
    CR(a, EMMC_REQUEST, Link, EMMC_REQUEST_SIGNATURE)

typedef struct {
  UINT32                                Signature;
  BOOLEAN                               Enable;
  EMMC_PARTITION_TYPE                   PartitionType;
  EFI_HANDLE                            Handle;
  EFI_DEVICE_PATH_PROTOCOL              *DevicePath;
  EFI_BLOCK_IO_PROTOCOL                 BlockIo;
  EFI_BLOCK_IO2_PROTOCOL                BlockIo2;
  EFI_BLOCK_IO_MEDIA                    BlockMedia;
  EFI_STORAGE_SECURITY_COMMAND_PROTOCOL StorageSecurity;
  EFI_ERASE_BLOCK_PROTOCOL              EraseBlock;
  EFI_DISK_INFO_PROTOCOL                DiskInfo;

  LIST_ENTRY                            Queue;

  EMMC_DEVICE                           *Device;
} EMMC_PARTITION;

//
// Up to 6 slots per EMMC PCI host controller
//
#define EMMC_MAX_DEVICES                6
//
// Up to 8 partitions per EMMC device.
//
#define EMMC_MAX_PARTITIONS             8
#define EMMC_MODEL_NAME_MAX_LEN         32

struct _EMMC_DEVICE {
  EFI_HANDLE                            Handle;
  EFI_DEVICE_PATH_PROTOCOL              *DevicePath;
  UINT8                                 Slot;
  BOOLEAN                               SectorAddressing;

  EMMC_PARTITION                        Partition[EMMC_MAX_PARTITIONS];
  EMMC_CSD                              Csd;
  EMMC_CID                              Cid;
  EMMC_EXT_CSD                          ExtCsd;
  EFI_UNICODE_STRING_TABLE              *ControllerNameTable;
  //
  // The model name consists of three fields in CID register
  // 1) OEM/Application ID (2 bytes)
  // 2) Product Name       (5 bytes)
  // 3) Product Serial Number (4 bytes)
  // The delimiters of these fields are whitespace.
  //
  CHAR16                                ModelName[EMMC_MODEL_NAME_MAX_LEN];
  EMMC_DRIVER_PRIVATE_DATA              *Private;
} ;

//
// EMMC DXE driver private data structure
//
struct _EMMC_DRIVER_PRIVATE_DATA {
  EFI_SD_MMC_PASS_THRU_PROTOCOL         *PassThru;
  EFI_HANDLE                            Controller;
  EFI_DEVICE_PATH_PROTOCOL              *ParentDevicePath;
  EFI_HANDLE                            DriverBindingHandle;

  EMMC_DEVICE                           Device[EMMC_MAX_DEVICES];
} ;

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
EmmcDxeDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL   *This,
  IN EFI_HANDLE                    Controller,
  IN EFI_DEVICE_PATH_PROTOCOL      *RemainingDevicePath
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
EmmcDxeDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL   *This,
  IN EFI_HANDLE                    Controller,
  IN EFI_DEVICE_PATH_PROTOCOL      *RemainingDevicePath
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
EmmcDxeDriverBindingStop (
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
EmmcDxeComponentNameGetDriverName (
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
EmmcDxeComponentNameGetControllerName (
  IN  EFI_COMPONENT_NAME_PROTOCOL                     *This,
  IN  EFI_HANDLE                                      ControllerHandle,
  IN  EFI_HANDLE                                      ChildHandle        OPTIONAL,
  IN  CHAR8                                           *Language,
  OUT CHAR16                                          **ControllerName
  );

/**
  Send command SELECT to the device to select/deselect the device.

  @param[in]  Device            A pointer to the EMMC_DEVICE instance.
  @param[in]  Rca               The relative device address to use.

  @retval EFI_SUCCESS           The request is executed successfully.
  @retval EFI_OUT_OF_RESOURCES  The request could not be executed due to a lack of resources.
  @retval Others                The request could not be executed successfully.

**/
EFI_STATUS
EmmcSelect (
  IN     EMMC_DEVICE            *Device,
  IN     UINT16                 Rca
  );

/**
  Send command SEND_STATUS to the device to get device status.

  @param[in]  Device            A pointer to the EMMC_DEVICE instance.
  @param[in]  Rca               The relative device address to use.
  @param[out] DevStatus         The buffer to store the device status.

  @retval EFI_SUCCESS           The request is executed successfully.
  @retval EFI_OUT_OF_RESOURCES  The request could not be executed due to a lack of resources.
  @retval Others                The request could not be executed successfully.

**/
EFI_STATUS
EmmcSendStatus (
  IN     EMMC_DEVICE            *Device,
  IN     UINT16                 Rca,
     OUT UINT32                 *DevStatus
  );

/**
  Send command SEND_CSD to the device to get the CSD register data.

  @param[in]  Device            A pointer to the EMMC_DEVICE instance.
  @param[in]  Rca               The relative device address to use.
  @param[out] Csd               The buffer to store the EMMC_CSD register data.

  @retval EFI_SUCCESS           The request is executed successfully.
  @retval EFI_OUT_OF_RESOURCES  The request could not be executed due to a lack of resources.
  @retval Others                The request could not be executed successfully.

**/
EFI_STATUS
EmmcGetCsd (
  IN     EMMC_DEVICE            *Device,
  IN     UINT16                 Rca,
     OUT EMMC_CSD               *Csd
  );

/**
  Send command SEND_CID to the device to get the CID register data.

  @param[in]  Device            A pointer to the EMMC_DEVICE instance.
  @param[in]  Rca               The relative device address to use.
  @param[out] Cid               The buffer to store the EMMC_CID register data.

  @retval EFI_SUCCESS           The request is executed successfully.
  @retval EFI_OUT_OF_RESOURCES  The request could not be executed due to a lack of resources.
  @retval Others                The request could not be executed successfully.

**/
EFI_STATUS
EmmcGetCid (
  IN     EMMC_DEVICE            *Device,
  IN     UINT16                 Rca,
     OUT EMMC_CID               *Cid
  );

/**
  Send command SEND_EXT_CSD to the device to get the EXT_CSD register data.

  @param[in]  Device            A pointer to the EMMC_DEVICE instance.
  @param[out] ExtCsd            The buffer to store the EXT_CSD register data.

  @retval EFI_SUCCESS           The request is executed successfully.
  @retval EFI_OUT_OF_RESOURCES  The request could not be executed due to a lack of resources.
  @retval Others                The request could not be executed successfully.

**/
EFI_STATUS
EmmcGetExtCsd (
  IN     EMMC_DEVICE            *Device,
     OUT EMMC_EXT_CSD           *ExtCsd
  );

#endif

