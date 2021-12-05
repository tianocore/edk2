/** @file
  Header file for SCSI Bus Driver.

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _SCSI_BUS_H_
#define _SCSI_BUS_H_

#include <Uefi.h>

#include <Protocol/ScsiPassThru.h>
#include <Protocol/ScsiPassThruExt.h>
#include <Protocol/ScsiIo.h>
#include <Protocol/ComponentName.h>
#include <Protocol/DriverBinding.h>
#include <Protocol/DevicePath.h>

#include <Library/DebugLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiScsiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DevicePathLib.h>
#include <Library/ReportStatusCodeLib.h>

#include <IndustryStandard/Scsi.h>

#define SCSI_IO_DEV_SIGNATURE  SIGNATURE_32 ('s', 'c', 'i', 'o')

typedef union {
  UINT32    Scsi;
  UINT8     ExtScsi[4];
} SCSI_ID;

typedef struct _SCSI_TARGET_ID {
  SCSI_ID    ScsiId;
  UINT8      ExtScsiId[12];
} SCSI_TARGET_ID;

typedef struct {
  VOID    *Data1;
  VOID    *Data2;
} SCSI_EVENT_DATA;

//
// SCSI Bus Controller device structure
//
#define SCSI_BUS_DEVICE_SIGNATURE  SIGNATURE_32 ('s', 'c', 's', 'i')

//
// SCSI Bus Timeout Experience Value
//
#define SCSI_BUS_TIMEOUT  EFI_TIMER_PERIOD_SECONDS (3)

//
// The ScsiBusProtocol is just used to locate ScsiBusDev
// structure in the SCSIBusDriverBindingStop(). Then we can
// Close all opened protocols and release this structure.
// ScsiBusProtocol is the private protocol.
// gEfiCallerIdGuid will be used as its protocol guid.
//
typedef struct _EFI_SCSI_BUS_PROTOCOL {
  UINT64    Reserved;
} EFI_SCSI_BUS_PROTOCOL;

typedef struct _SCSI_BUS_DEVICE {
  UINTN                              Signature;
  EFI_SCSI_BUS_PROTOCOL              BusIdentify;
  BOOLEAN                            ExtScsiSupport;
  EFI_SCSI_PASS_THRU_PROTOCOL        *ScsiInterface;
  EFI_EXT_SCSI_PASS_THRU_PROTOCOL    *ExtScsiInterface;
  EFI_DEVICE_PATH_PROTOCOL           *DevicePath;
} SCSI_BUS_DEVICE;

#define SCSI_BUS_CONTROLLER_DEVICE_FROM_THIS(a)  CR (a, SCSI_BUS_DEVICE, BusIdentify, SCSI_BUS_DEVICE_SIGNATURE)

typedef struct {
  UINT32                             Signature;
  EFI_HANDLE                         Handle;
  EFI_SCSI_IO_PROTOCOL               ScsiIo;
  EFI_DEVICE_PATH_PROTOCOL           *DevicePath;
  BOOLEAN                            ExtScsiSupport;
  EFI_SCSI_PASS_THRU_PROTOCOL        *ScsiPassThru;
  EFI_EXT_SCSI_PASS_THRU_PROTOCOL    *ExtScsiPassThru;
  SCSI_BUS_DEVICE                    *ScsiBusDeviceData;
  SCSI_TARGET_ID                     Pun;
  UINT64                             Lun;
  UINT8                              ScsiDeviceType;
  UINT8                              ScsiVersion;
  BOOLEAN                            RemovableDevice;
} SCSI_IO_DEV;

#define SCSI_IO_DEV_FROM_THIS(a)  CR (a, SCSI_IO_DEV, ScsiIo, SCSI_IO_DEV_SIGNATURE)

//
// Global Variables
//
extern EFI_DRIVER_BINDING_PROTOCOL   gScsiBusDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL   gScsiBusComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL  gScsiBusComponentName2;

/**
  Test to see if this driver supports ControllerHandle.

  This service is called by the EFI boot service ConnectController(). In order
  to make drivers as small as possible, there are a few calling restrictions for
  this service. ConnectController() must follow these calling restrictions. If
  any other agent wishes to call Supported() it must also follow these calling
  restrictions.

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
SCSIBusDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  );

/**
  Start this driver on ControllerHandle.

  This service is called by the EFI boot service ConnectController(). In order
  to make drivers as small as possible, there are a few calling restrictions for
  this service. ConnectController() must follow these calling restrictions. If
  any other agent wishes to call Start() it must also follow these calling
  restrictions.

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
SCSIBusDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  );

/**
  Stop this driver on ControllerHandle.

  This service is called by the EFI boot service DisconnectController().
  In order to make drivers as small as possible, there are a few calling
  restrictions for this service. DisconnectController() must follow these
  calling restrictions. If any other agent wishes to call Stop() it must also
  follow these calling restrictions.

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
SCSIBusDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Controller,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
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

  @param  This                  A pointer to the EFI_COMPONENT_NAME2_PROTOCOL or
                                EFI_COMPONENT_NAME_PROTOCOL instance.

  @param  Language              A pointer to a Null-terminated ASCII string
                                array indicating the language. This is the
                                language of the driver name that the caller is
                                requesting, and it must match one of the
                                languages specified in SupportedLanguages. The
                                number of languages supported by a driver is up
                                to the driver writer. Language is specified
                                in RFC 4646 or ISO 639-2 language code format.

  @param  DriverName            A pointer to the Unicode string to return.
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
ScsiBusComponentNameGetDriverName (
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

  @param  This                  A pointer to the EFI_COMPONENT_NAME2_PROTOCOL or
                                EFI_COMPONENT_NAME_PROTOCOL instance.

  @param  ControllerHandle      The handle of a controller that the driver
                                specified by This is managing.  This handle
                                specifies the controller whose name is to be
                                returned.

  @param  ChildHandle           The handle of the child controller to retrieve
                                the name of.  This is an optional parameter that
                                may be NULL.  It will be NULL for device
                                drivers.  It will also be NULL for a bus drivers
                                that wish to retrieve the name of the bus
                                controller.  It will not be NULL for a bus
                                driver that wishes to retrieve the name of a
                                child controller.

  @param  Language              A pointer to a Null-terminated ASCII string
                                array indicating the language.  This is the
                                language of the driver name that the caller is
                                requesting, and it must match one of the
                                languages specified in SupportedLanguages. The
                                number of languages supported by a driver is up
                                to the driver writer. Language is specified in
                                RFC 4646 or ISO 639-2 language code format.

  @param  ControllerName        A pointer to the Unicode string to return.
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
ScsiBusComponentNameGetControllerName (
  IN  EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  EFI_HANDLE                   ChildHandle        OPTIONAL,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **ControllerName
  );

/**
  Retrieves the device type information of the SCSI Controller.

  @param  This          Protocol instance pointer.
  @param  DeviceType    A pointer to the device type information retrieved from
                        the SCSI Controller.

  @retval EFI_SUCCESS             Retrieves the device type information successfully.
  @retval EFI_INVALID_PARAMETER   The DeviceType is NULL.

**/
EFI_STATUS
EFIAPI
ScsiGetDeviceType (
  IN  EFI_SCSI_IO_PROTOCOL  *This,
  OUT UINT8                 *DeviceType
  );

/**
  Retrieves the device location in the SCSI channel.

  @param  This   Protocol instance pointer.
  @param  Target A pointer to the Target ID of a SCSI device
                 on the SCSI channel.
  @param  Lun    A pointer to the LUN of the SCSI device on
                 the SCSI channel.

  @retval EFI_SUCCESS           Retrieves the device location successfully.
  @retval EFI_INVALID_PARAMETER The Target or Lun is NULL.

**/
EFI_STATUS
EFIAPI
ScsiGetDeviceLocation (
  IN  EFI_SCSI_IO_PROTOCOL  *This,
  IN OUT UINT8              **Target,
  OUT UINT64                *Lun
  );

/**
  Resets the SCSI Bus that the SCSI Controller is attached to.

  @param  This  Protocol instance pointer.

  @retval  EFI_SUCCESS       The SCSI bus is reset successfully.
  @retval  EFI_DEVICE_ERROR  Errors encountered when resetting the SCSI bus.
  @retval  EFI_UNSUPPORTED   The bus reset operation is not supported by the
                             SCSI Host Controller.
  @retval  EFI_TIMEOUT       A timeout occurred while attempting to reset
                             the SCSI bus.
**/
EFI_STATUS
EFIAPI
ScsiResetBus (
  IN  EFI_SCSI_IO_PROTOCOL  *This
  );

/**
  Resets the SCSI Controller that the device handle specifies.

  @param  This  Protocol instance pointer.

  @retval  EFI_SUCCESS       Reset the SCSI controller successfully.
  @retval  EFI_DEVICE_ERROR  Errors are encountered when resetting the SCSI Controller.
  @retval  EFI_UNSUPPORTED   The SCSI bus does not support a device reset operation.
  @retval  EFI_TIMEOUT       A timeout occurred while attempting to reset the
                             SCSI Controller.
**/
EFI_STATUS
EFIAPI
ScsiResetDevice (
  IN  EFI_SCSI_IO_PROTOCOL  *This
  );

/**
  Sends a SCSI Request Packet to the SCSI Controller for execution.

  @param  This            Protocol instance pointer.
  @param  CommandPacket   The SCSI request packet to send to the SCSI
                          Controller specified by the device handle.
  @param  Event           If the SCSI bus where the SCSI device is attached
                          does not support non-blocking I/O, then Event is
                          ignored, and blocking I/O is performed.
                          If Event is NULL, then blocking I/O is performed.
                          If Event is not NULL and non-blocking I/O is
                          supported, then non-blocking I/O is performed,
                          and Event will be signaled when the SCSI Request
                          Packet completes.

  @retval EFI_SUCCESS         The SCSI Request Packet was sent by the host
                              successfully, and TransferLength bytes were
                              transferred to/from DataBuffer.See
                              HostAdapterStatus, TargetStatus,
                              SenseDataLength, and SenseData in that order
                              for additional status information.
  @retval EFI_BAD_BUFFER_SIZE The SCSI Request Packet was executed,
                              but the entire DataBuffer could not be transferred.
                              The actual number of bytes transferred is returned
                              in TransferLength. See HostAdapterStatus,
                              TargetStatus, SenseDataLength, and SenseData in
                              that order for additional status information.
  @retval EFI_NOT_READY       The SCSI Request Packet could not be sent because
                              there are too many SCSI Command Packets already
                              queued.The caller may retry again later.
  @retval EFI_DEVICE_ERROR    A device error occurred while attempting to send
                              the SCSI Request Packet. See HostAdapterStatus,
                              TargetStatus, SenseDataLength, and SenseData in
                              that order for additional status information.
  @retval EFI_INVALID_PARAMETER  The contents of CommandPacket are invalid.
                                 The SCSI Request Packet was not sent, so no
                                 additional status information is available.
  @retval EFI_UNSUPPORTED     The command described by the SCSI Request Packet
                              is not supported by the SCSI initiator(i.e., SCSI
                              Host Controller). The SCSI Request Packet was not
                              sent, so no additional status information is
                              available.
  @retval EFI_TIMEOUT         A timeout occurred while waiting for the SCSI
                              Request Packet to execute. See HostAdapterStatus,
                              TargetStatus, SenseDataLength, and SenseData in
                              that order for additional status information.
**/
EFI_STATUS
EFIAPI
ScsiExecuteSCSICommand (
  IN  EFI_SCSI_IO_PROTOCOL                 *This,
  IN OUT  EFI_SCSI_IO_SCSI_REQUEST_PACKET  *CommandPacket,
  IN  EFI_EVENT                            Event  OPTIONAL
  );

/**
  Scan SCSI Bus to discover the device, and attach ScsiIoProtocol to it.

  @param  This           Protocol instance pointer
  @param  Controller     Controller handle
  @param  TargetId       Target to be scanned
  @param  Lun            The Lun of the SCSI device on the SCSI channel.
  @param  ScsiBusDev     The pointer of SCSI_BUS_DEVICE

  @retval EFI_SUCCESS           Successfully to discover the device and attach
                                ScsiIoProtocol to it.
  @retval EFI_OUT_OF_RESOURCES  Fail to discover the device.

**/
EFI_STATUS
EFIAPI
ScsiScanCreateDevice (
  IN     EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN     EFI_HANDLE                   Controller,
  IN     SCSI_TARGET_ID               *TargetId,
  IN     UINT64                       Lun,
  IN OUT SCSI_BUS_DEVICE              *ScsiBusDev
  );

/**
  Discovery SCSI Device

  @param  ScsiIoDevice    The pointer of SCSI_IO_DEV

  @retval  TRUE   Find SCSI Device and verify it.
  @retval  FALSE  Unable to find SCSI Device.

**/
BOOLEAN
DiscoverScsiDevice (
  IN  OUT  SCSI_IO_DEV  *ScsiIoDevice
  );

#endif
