/** @file

  Copyright (c) 2014 - 2021, Intel Corporation. All rights reserved.<BR>
  Copyright (c) Microsoft Corporation.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _UFS_PASS_THRU_H_
#define _UFS_PASS_THRU_H_

#include <Uefi.h>

#include <Protocol/ScsiPassThruExt.h>
#include <Protocol/UfsDeviceConfig.h>
#include <Protocol/UfsHostController.h>
#include <Protocol/UfsHostControllerPlatform.h>

#include <Library/DebugLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/BaseLib.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DevicePathLib.h>
#include <Library/PcdLib.h>
#include <Library/TimerLib.h>

#include "UfsPassThruHci.h"

#define UFS_PASS_THRU_SIG  SIGNATURE_32 ('U', 'F', 'S', 'P')

//
// Lun 0~7 is for 8 common luns.
// Lun 8~11 is for those 4 well known luns (Refer to UFS 2.0 spec Table 10.58 for details):
//  Lun 8:  REPORT LUNS
//  Lun 9:  UFS DEVICE
//  Lun 10: BOOT
//  Lun 11: RPMB
//
#define UFS_MAX_LUNS     12
#define UFS_WLUN_PREFIX  0xC1

typedef struct {
  UINT8     Lun[UFS_MAX_LUNS];
  UINT16    BitMask : 12;           // Bit 0~7 is 1/1 mapping to common luns. Bit 8~11 is 1/1 mapping to well-known luns.
  UINT16    Rsvd    : 4;
} UFS_EXPOSED_LUNS;

typedef struct _UFS_PASS_THRU_PRIVATE_DATA {
  UINT32                                Signature;
  EFI_HANDLE                            Handle;
  EFI_EXT_SCSI_PASS_THRU_MODE           ExtScsiPassThruMode;
  EFI_EXT_SCSI_PASS_THRU_PROTOCOL       ExtScsiPassThru;
  EFI_UFS_DEVICE_CONFIG_PROTOCOL        UfsDevConfig;
  EDKII_UFS_HOST_CONTROLLER_PROTOCOL    *UfsHostController;
  UINTN                                 UfsHcBase;
  EDKII_UFS_HC_INFO                     UfsHcInfo;
  EDKII_UFS_HC_DRIVER_INTERFACE         UfsHcDriverInterface;

  UINT8                                 TaskTag;

  VOID                                  *UtpTrlBase;
  UINT8                                 Nutrs;
  VOID                                  *TrlMapping;
  VOID                                  *UtpTmrlBase;
  UINT8                                 Nutmrs;
  VOID                                  *TmrlMapping;

  UFS_EXPOSED_LUNS                      Luns;

  //
  // For Non-blocking operation.
  //
  EFI_EVENT                             TimerEvent;
  LIST_ENTRY                            Queue;
} UFS_PASS_THRU_PRIVATE_DATA;

#define UFS_PASS_THRU_TRANS_REQ_SIG  SIGNATURE_32 ('U', 'F', 'S', 'T')

typedef struct {
  UINT32                                        Signature;
  LIST_ENTRY                                    TransferList;

  UINT8                                         Slot;
  UTP_TRD                                       *Trd;
  UINT32                                        CmdDescSize;
  VOID                                          *CmdDescHost;
  VOID                                          *CmdDescMapping;
  VOID                                          *AlignedDataBuf;
  UINTN                                         AlignedDataBufSize;
  VOID                                          *DataBufMapping;

  EFI_EXT_SCSI_PASS_THRU_SCSI_REQUEST_PACKET    *Packet;
  UINT64                                        TimeoutRemain;
  EFI_EVENT                                     CallerEvent;
} UFS_PASS_THRU_TRANS_REQ;

#define UFS_PASS_THRU_TRANS_REQ_FROM_THIS(a) \
    CR(a, UFS_PASS_THRU_TRANS_REQ, TransferList, UFS_PASS_THRU_TRANS_REQ_SIG)

#define UFS_TIMEOUT         EFI_TIMER_PERIOD_SECONDS(3)
#define UFS_HC_ASYNC_TIMER  EFI_TIMER_PERIOD_MILLISECONDS(1)

#define ROUNDUP8(x)  (((x) % 8 == 0) ? (x) : ((x) / 8 + 1) * 8)

#define UFS_PASS_THRU_PRIVATE_DATA_FROM_THIS(a) \
  CR (a, \
      UFS_PASS_THRU_PRIVATE_DATA, \
      ExtScsiPassThru, \
      UFS_PASS_THRU_SIG \
      )

#define UFS_PASS_THRU_PRIVATE_DATA_FROM_DEV_CONFIG(a) \
  CR (a, \
      UFS_PASS_THRU_PRIVATE_DATA, \
      UfsDevConfig, \
      UFS_PASS_THRU_SIG \
      )

#define UFS_PASS_THRU_PRIVATE_DATA_FROM_DRIVER_INTF(a) \
  CR (a, \
      UFS_PASS_THRU_PRIVATE_DATA, \
      UfsHcDriverInterface, \
      UFS_PASS_THRU_SIG \
      )

typedef struct _UFS_DEVICE_MANAGEMENT_REQUEST_PACKET {
  UINT64    Timeout;
  VOID      *DataBuffer;
  UINT8     Opcode;
  UINT8     DescId;
  UINT8     Index;
  UINT8     Selector;
  UINT32    TransferLength;
  UINT8     DataDirection;
} UFS_DEVICE_MANAGEMENT_REQUEST_PACKET;

//
// function prototype
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
UfsPassThruDriverBindingSupported (
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
UfsPassThruDriverBindingStart (
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
UfsPassThruDriverBindingStop (
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
UfsPassThruComponentNameGetDriverName (
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
UfsPassThruComponentNameGetControllerName (
  IN  EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  EFI_HANDLE                   ControllerHandle,
  IN  EFI_HANDLE                   ChildHandle        OPTIONAL,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **ControllerName
  );

/**
  Sends a SCSI Request Packet to a SCSI device that is attached to the SCSI channel. This function
  supports both blocking I/O and nonblocking I/O. The blocking I/O functionality is required, and the
  nonblocking I/O functionality is optional.

  @param  This    A pointer to the EFI_EXT_SCSI_PASS_THRU_PROTOCOL instance.
  @param  Target  The Target is an array of size TARGET_MAX_BYTES and it represents
                  the id of the SCSI device to send the SCSI Request Packet. Each
                  transport driver may choose to utilize a subset of this size to suit the needs
                  of transport target representation. For example, a Fibre Channel driver
                  may use only 8 bytes (WWN) to represent an FC target.
  @param  Lun     The LUN of the SCSI device to send the SCSI Request Packet.
  @param  Packet  A pointer to the SCSI Request Packet to send to the SCSI device
                  specified by Target and Lun.
  @param  Event   If nonblocking I/O is not supported then Event is ignored, and blocking
                  I/O is performed. If Event is NULL, then blocking I/O is performed. If
                  Event is not NULL and non blocking I/O is supported, then
                  nonblocking I/O is performed, and Event will be signaled when the
                  SCSI Request Packet completes.

  @retval EFI_SUCCESS           The SCSI Request Packet was sent by the host. For bi-directional
                                commands, InTransferLength bytes were transferred from
                                InDataBuffer. For write and bi-directional commands,
                                OutTransferLength bytes were transferred by
                                OutDataBuffer.
  @retval EFI_BAD_BUFFER_SIZE   The SCSI Request Packet was not executed. The number of bytes that
                                could be transferred is returned in InTransferLength. For write
                                and bi-directional commands, OutTransferLength bytes were
                                transferred by OutDataBuffer.
  @retval EFI_NOT_READY         The SCSI Request Packet could not be sent because there are too many
                                SCSI Request Packets already queued. The caller may retry again later.
  @retval EFI_DEVICE_ERROR      A device error occurred while attempting to send the SCSI Request
                                Packet.
  @retval EFI_INVALID_PARAMETER Target, Lun, or the contents of ScsiRequestPacket are invalid.
  @retval EFI_UNSUPPORTED       The command described by the SCSI Request Packet is not supported
                                by the host adapter. This includes the case of Bi-directional SCSI
                                commands not supported by the implementation. The SCSI Request
                                Packet was not sent, so no additional status information is available.
  @retval EFI_TIMEOUT           A timeout occurred while waiting for the SCSI Request Packet to execute.

**/
EFI_STATUS
EFIAPI
UfsPassThruPassThru (
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL                 *This,
  IN UINT8                                           *Target,
  IN UINT64                                          Lun,
  IN OUT EFI_EXT_SCSI_PASS_THRU_SCSI_REQUEST_PACKET  *Packet,
  IN EFI_EVENT                                       Event OPTIONAL
  );

/**
  Used to retrieve the list of legal Target IDs and LUNs for SCSI devices on a SCSI channel. These
  can either be the list SCSI devices that are actually present on the SCSI channel, or the list of legal
  Target Ids and LUNs for the SCSI channel. Regardless, the caller of this function must probe the
  Target ID and LUN returned to see if a SCSI device is actually present at that location on the SCSI
  channel.

  @param  This   A pointer to the EFI_EXT_SCSI_PASS_THRU_PROTOCOL instance.
  @param  Target On input, a pointer to the Target ID (an array of size
                 TARGET_MAX_BYTES) of a SCSI device present on the SCSI channel.
                 On output, a pointer to the Target ID (an array of
                 TARGET_MAX_BYTES) of the next SCSI device present on a SCSI
                 channel. An input value of 0xF(all bytes in the array are 0xF) in the
                 Target array retrieves the Target ID of the first SCSI device present on a
                 SCSI channel.
  @param  Lun    On input, a pointer to the LUN of a SCSI device present on the SCSI
                 channel. On output, a pointer to the LUN of the next SCSI device present
                 on a SCSI channel.

  @retval EFI_SUCCESS           The Target ID and LUN of the next SCSI device on the SCSI
                                channel was returned in Target and Lun.
  @retval EFI_INVALID_PARAMETER Target array is not all 0xF, and Target and Lun were
                                not returned on a previous call to GetNextTargetLun().
  @retval EFI_NOT_FOUND         There are no more SCSI devices on this SCSI channel.

**/
EFI_STATUS
EFIAPI
UfsPassThruGetNextTargetLun (
  IN  EFI_EXT_SCSI_PASS_THRU_PROTOCOL  *This,
  IN OUT UINT8                         **Target,
  IN OUT UINT64                        *Lun
  );

/**
  Used to allocate and build a device path node for a SCSI device on a SCSI channel.

  @param  This       A pointer to the EFI_EXT_SCSI_PASS_THRU_PROTOCOL instance.
  @param  Target     The Target is an array of size TARGET_MAX_BYTES and it specifies the
                     Target ID of the SCSI device for which a device path node is to be
                     allocated and built. Transport drivers may chose to utilize a subset of
                     this size to suit the representation of targets. For example, a Fibre
                     Channel driver may use only 8 bytes (WWN) in the array to represent a
                     FC target.
  @param  Lun        The LUN of the SCSI device for which a device path node is to be
                     allocated and built.
  @param  DevicePath A pointer to a single device path node that describes the SCSI device
                     specified by Target and Lun. This function is responsible for
                     allocating the buffer DevicePath with the boot service
                     AllocatePool(). It is the caller's responsibility to free
                     DevicePath when the caller is finished with DevicePath.

  @retval EFI_SUCCESS           The device path node that describes the SCSI device specified by
                                Target and Lun was allocated and returned in
                                DevicePath.
  @retval EFI_INVALID_PARAMETER DevicePath is NULL.
  @retval EFI_NOT_FOUND         The SCSI devices specified by Target and Lun does not exist
                                on the SCSI channel.
  @retval EFI_OUT_OF_RESOURCES  There are not enough resources to allocate DevicePath.

**/
EFI_STATUS
EFIAPI
UfsPassThruBuildDevicePath (
  IN     EFI_EXT_SCSI_PASS_THRU_PROTOCOL  *This,
  IN     UINT8                            *Target,
  IN     UINT64                           Lun,
  IN OUT EFI_DEVICE_PATH_PROTOCOL         **DevicePath
  );

/**
  Used to translate a device path node to a Target ID and LUN.

  @param  This       A pointer to the EFI_EXT_SCSI_PASS_THRU_PROTOCOL instance.
  @param  DevicePath A pointer to a single device path node that describes the SCSI device
                     on the SCSI channel.
  @param  Target     A pointer to the Target Array which represents the ID of a SCSI device
                     on the SCSI channel.
  @param  Lun        A pointer to the LUN of a SCSI device on the SCSI channel.

  @retval EFI_SUCCESS           DevicePath was successfully translated to a Target ID and
                                LUN, and they were returned in Target and Lun.
  @retval EFI_INVALID_PARAMETER DevicePath or Target or Lun is NULL.
  @retval EFI_NOT_FOUND         A valid translation from DevicePath to a Target ID and LUN
                                does not exist.
  @retval EFI_UNSUPPORTED       This driver does not support the device path node type in
                                 DevicePath.

**/
EFI_STATUS
EFIAPI
UfsPassThruGetTargetLun (
  IN  EFI_EXT_SCSI_PASS_THRU_PROTOCOL  *This,
  IN  EFI_DEVICE_PATH_PROTOCOL         *DevicePath,
  OUT UINT8                            **Target,
  OUT UINT64                           *Lun
  );

/**
  Resets a SCSI channel. This operation resets all the SCSI devices connected to the SCSI channel.

  @param  This A pointer to the EFI_EXT_SCSI_PASS_THRU_PROTOCOL instance.

  @retval EFI_SUCCESS      The SCSI channel was reset.
  @retval EFI_DEVICE_ERROR A device error occurred while attempting to reset the SCSI channel.
  @retval EFI_TIMEOUT      A timeout occurred while attempting to reset the SCSI channel.
  @retval EFI_UNSUPPORTED  The SCSI channel does not support a channel reset operation.

**/
EFI_STATUS
EFIAPI
UfsPassThruResetChannel (
  IN  EFI_EXT_SCSI_PASS_THRU_PROTOCOL  *This
  );

/**
  Resets a SCSI logical unit that is connected to a SCSI channel.

  @param  This   A pointer to the EFI_EXT_SCSI_PASS_THRU_PROTOCOL instance.
  @param  Target The Target is an array of size TARGET_MAX_BYTE and it represents the
                 target port ID of the SCSI device containing the SCSI logical unit to
                 reset. Transport drivers may chose to utilize a subset of this array to suit
                 the representation of their targets.
  @param  Lun    The LUN of the SCSI device to reset.

  @retval EFI_SUCCESS           The SCSI device specified by Target and Lun was reset.
  @retval EFI_INVALID_PARAMETER Target or Lun is NULL.
  @retval EFI_TIMEOUT           A timeout occurred while attempting to reset the SCSI device
                                specified by Target and Lun.
  @retval EFI_UNSUPPORTED       The SCSI channel does not support a target reset operation.
  @retval EFI_DEVICE_ERROR      A device error occurred while attempting to reset the SCSI device
                                 specified by Target and Lun.

**/
EFI_STATUS
EFIAPI
UfsPassThruResetTargetLun (
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL  *This,
  IN UINT8                            *Target,
  IN UINT64                           Lun
  );

/**
  Used to retrieve the list of legal Target IDs for SCSI devices on a SCSI channel. These can either
  be the list SCSI devices that are actually present on the SCSI channel, or the list of legal Target IDs
  for the SCSI channel. Regardless, the caller of this function must probe the Target ID returned to
  see if a SCSI device is actually present at that location on the SCSI channel.

  @param  This   A pointer to the EFI_EXT_SCSI_PASS_THRU_PROTOCOL instance.
  @param  Target (TARGET_MAX_BYTES) of a SCSI device present on the SCSI channel.
                 On output, a pointer to the Target ID (an array of
                 TARGET_MAX_BYTES) of the next SCSI device present on a SCSI
                 channel. An input value of 0xF(all bytes in the array are 0xF) in the
                 Target array retrieves the Target ID of the first SCSI device present on a
                 SCSI channel.

  @retval EFI_SUCCESS           The Target ID of the next SCSI device on the SCSI
                                channel was returned in Target.
  @retval EFI_INVALID_PARAMETER Target or Lun is NULL.
  @retval EFI_TIMEOUT           Target array is not all 0xF, and Target was not
                                returned on a previous call to GetNextTarget().
  @retval EFI_NOT_FOUND         There are no more SCSI devices on this SCSI channel.

**/
EFI_STATUS
EFIAPI
UfsPassThruGetNextTarget (
  IN  EFI_EXT_SCSI_PASS_THRU_PROTOCOL  *This,
  IN OUT UINT8                         **Target
  );

/**
  Sends a UFS-supported SCSI Request Packet to a UFS device that is attached to the UFS host controller.

  @param[in]      Private       The pointer to the UFS_PASS_THRU_PRIVATE_DATA data structure.
  @param[in]      Lun           The LUN of the UFS device to send the SCSI Request Packet.
  @param[in, out] Packet        A pointer to the SCSI Request Packet to send to a specified Lun of the
                                UFS device.
  @param[in]      Event         If nonblocking I/O is not supported then Event is ignored, and blocking
                                I/O is performed. If Event is NULL, then blocking I/O is performed. If
                                Event is not NULL and non blocking I/O is supported, then
                                nonblocking I/O is performed, and Event will be signaled when the
                                SCSI Request Packet completes.

  @retval EFI_SUCCESS           The SCSI Request Packet was sent by the host. For bi-directional
                                commands, InTransferLength bytes were transferred from
                                InDataBuffer. For write and bi-directional commands,
                                OutTransferLength bytes were transferred by
                                OutDataBuffer.
  @retval EFI_DEVICE_ERROR      A device error occurred while attempting to send the SCSI Request
                                Packet.
  @retval EFI_OUT_OF_RESOURCES  The resource for transfer is not available.
  @retval EFI_TIMEOUT           A timeout occurred while waiting for the SCSI Request Packet to execute.

**/
EFI_STATUS
UfsExecScsiCmds (
  IN     UFS_PASS_THRU_PRIVATE_DATA                  *Private,
  IN     UINT8                                       Lun,
  IN OUT EFI_EXT_SCSI_PASS_THRU_SCSI_REQUEST_PACKET  *Packet,
  IN     EFI_EVENT                                   Event    OPTIONAL
  );

/**
  Initialize the UFS host controller.

  @param[in] Private                 The pointer to the NVME_CONTROLLER_PRIVATE_DATA data structure.

  @retval EFI_SUCCESS                The NVM Express Controller is initialized successfully.
  @retval Others                     A device error occurred while initializing the controller.

**/
EFI_STATUS
UfsControllerInit (
  IN  UFS_PASS_THRU_PRIVATE_DATA  *Private
  );

/**
  Stop the UFS host controller.

  @param[in] Private                 The pointer to the UFS_PASS_THRU_PRIVATE_DATA data structure.

  @retval EFI_SUCCESS                The Ufs Host Controller is stopped successfully.
  @retval Others                     A device error occurred while stopping the controller.

**/
EFI_STATUS
UfsControllerStop (
  IN  UFS_PASS_THRU_PRIVATE_DATA  *Private
  );

/**
  Allocate common buffer for host and UFS bus master access simultaneously.

  @param[in]  Private                The pointer to the UFS_PASS_THRU_PRIVATE_DATA data structure.
  @param[in]  Size                   The length of buffer to be allocated.
  @param[out] CmdDescHost            A pointer to store the base system memory address of the allocated range.
  @param[out] CmdDescPhyAddr         The resulting map address for the UFS bus master to use to access the hosts CmdDescHost.
  @param[out] CmdDescMapping         A resulting value to pass to Unmap().

  @retval EFI_SUCCESS                The common buffer was allocated successfully.
  @retval EFI_DEVICE_ERROR           The allocation fails.
  @retval EFI_OUT_OF_RESOURCES       The memory resource is insufficient.

**/
EFI_STATUS
UfsAllocateAlignCommonBuffer (
  IN     UFS_PASS_THRU_PRIVATE_DATA  *Private,
  IN     UINTN                       Size,
  OUT VOID                           **CmdDescHost,
  OUT EFI_PHYSICAL_ADDRESS           *CmdDescPhyAddr,
  OUT VOID                           **CmdDescMapping
  );

/**
  Set specified flag to 1 on a UFS device.

  @param[in]  Private           The pointer to the UFS_PASS_THRU_PRIVATE_DATA data structure.
  @param[in]  FlagId            The ID of flag to be set.

  @retval EFI_SUCCESS           The flag was set successfully.
  @retval EFI_DEVICE_ERROR      A device error occurred while attempting to set the flag.
  @retval EFI_TIMEOUT           A timeout occurred while waiting for the completion of setting the flag.

**/
EFI_STATUS
UfsSetFlag (
  IN  UFS_PASS_THRU_PRIVATE_DATA  *Private,
  IN  UINT8                       FlagId
  );

/**
  Read specified flag from a UFS device.

  @param[in]  Private           The pointer to the UFS_PASS_THRU_PRIVATE_DATA data structure.
  @param[in]  FlagId            The ID of flag to be read.
  @param[out] Value             The flag's value.

  @retval EFI_SUCCESS           The flag was read successfully.
  @retval EFI_DEVICE_ERROR      A device error occurred while attempting to read the flag.
  @retval EFI_TIMEOUT           A timeout occurred while waiting for the completion of reading the flag.

**/
EFI_STATUS
UfsReadFlag (
  IN     UFS_PASS_THRU_PRIVATE_DATA  *Private,
  IN     UINT8                       FlagId,
  OUT UINT8                          *Value
  );

/**
  Read or write specified flag of a UFS device.

  @param[in]      Private       The pointer to the UFS_PASS_THRU_PRIVATE_DATA data structure.
  @param[in]      Read          The boolean variable to show r/w direction.
  @param[in]      FlagId        The ID of flag to be read or written.
  @param[in, out] Value         The value to set or clear flag.

  @retval EFI_SUCCESS           The flag was read/written successfully.
  @retval EFI_DEVICE_ERROR      A device error occurred while attempting to r/w the flag.
  @retval EFI_TIMEOUT           A timeout occurred while waiting for the completion of r/w the flag.

**/
EFI_STATUS
UfsRwFlags (
  IN     UFS_PASS_THRU_PRIVATE_DATA  *Private,
  IN     BOOLEAN                     Read,
  IN     UINT8                       FlagId,
  IN OUT UINT8                       *Value
  );

/**
  Read or write specified device descriptor of a UFS device.

  @param[in]      Private       The pointer to the UFS_PASS_THRU_PRIVATE_DATA data structure.
  @param[in]      Read          The boolean variable to show r/w direction.
  @param[in]      DescId        The ID of device descriptor.
  @param[in]      Index         The Index of device descriptor.
  @param[in]      Selector      The Selector of device descriptor.
  @param[in, out] Descriptor    The buffer of device descriptor to be read or written.
  @param[in, out] DescSize      The size of device descriptor buffer. On input, the size, in bytes,
                                of the data buffer specified by Descriptor. On output, the number
                                of bytes that were actually transferred.

  @retval EFI_SUCCESS           The device descriptor was read/written successfully.
  @retval EFI_DEVICE_ERROR      A device error occurred while attempting to r/w the device descriptor.
  @retval EFI_TIMEOUT           A timeout occurred while waiting for the completion of r/w the device descriptor.

**/
EFI_STATUS
UfsRwDeviceDesc (
  IN     UFS_PASS_THRU_PRIVATE_DATA  *Private,
  IN     BOOLEAN                     Read,
  IN     UINT8                       DescId,
  IN     UINT8                       Index,
  IN     UINT8                       Selector,
  IN OUT VOID                        *Descriptor,
  IN OUT UINT32                      *DescSize
  );

/**
  Read or write specified attribute of a UFS device.

  @param[in]      Private       The pointer to the UFS_PASS_THRU_PRIVATE_DATA data structure.
  @param[in]      Read          The boolean variable to show r/w direction.
  @param[in]      AttrId        The ID of Attribute.
  @param[in]      Index         The Index of Attribute.
  @param[in]      Selector      The Selector of Attribute.
  @param[in, out] Attributes    The value of Attribute to be read or written.

  @retval EFI_SUCCESS           The Attribute was read/written successfully.
  @retval EFI_DEVICE_ERROR      A device error occurred while attempting to r/w the Attribute.
  @retval EFI_TIMEOUT           A timeout occurred while waiting for the completion of r/w the Attribute.

**/
EFI_STATUS
UfsRwAttributes (
  IN     UFS_PASS_THRU_PRIVATE_DATA  *Private,
  IN     BOOLEAN                     Read,
  IN     UINT8                       AttrId,
  IN     UINT8                       Index,
  IN     UINT8                       Selector,
  IN OUT UINT32                      *Attributes
  );

/**
  Sends NOP IN cmd to a UFS device for initialization process request.
  For more details, please refer to UFS 2.0 spec Figure 13.3.

  @param[in]  Private           The pointer to the UFS_PASS_THRU_PRIVATE_DATA data structure.

  @retval EFI_SUCCESS           The NOP IN command was sent by the host. The NOP OUT response was
                                received successfully.
  @retval EFI_DEVICE_ERROR      A device error occurred while attempting to execute NOP IN command.
  @retval EFI_OUT_OF_RESOURCES  The resource for transfer is not available.
  @retval EFI_TIMEOUT           A timeout occurred while waiting for the NOP IN command to execute.

**/
EFI_STATUS
UfsExecNopCmds (
  IN  UFS_PASS_THRU_PRIVATE_DATA  *Private
  );

/**
  Call back function when the timer event is signaled.

  @param[in]  Event     The Event this notify function registered to.
  @param[in]  Context   Pointer to the context data registered to the Event.

**/
VOID
EFIAPI
ProcessAsyncTaskList (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );

/**
  Internal helper function which will signal the caller event and clean up
  resources.

  @param[in] Private   The pointer to the UFS_PASS_THRU_PRIVATE_DATA data
                       structure.
  @param[in] TransReq  The pointer to the UFS_PASS_THRU_TRANS_REQ data
                       structure.

**/
VOID
EFIAPI
SignalCallerEvent (
  IN UFS_PASS_THRU_PRIVATE_DATA  *Private,
  IN UFS_PASS_THRU_TRANS_REQ     *TransReq
  );

/**
  Read or write specified device descriptor of a UFS device.

  The function is used to read/write UFS device descriptors. The consumer of this API is
  responsible for allocating the data buffer pointed by Descriptor.

  @param[in]      This          The pointer to the EFI_UFS_DEVICE_CONFIG_PROTOCOL instance.
  @param[in]      Read          The boolean variable to show r/w direction.
  @param[in]      DescId        The ID of device descriptor.
  @param[in]      Index         The Index of device descriptor.
  @param[in]      Selector      The Selector of device descriptor.
  @param[in, out] Descriptor    The buffer of device descriptor to be read or written.
  @param[in, out] DescSize      The size of device descriptor buffer. On input, the size, in bytes,
                                of the data buffer specified by Descriptor. On output, the number
                                of bytes that were actually transferred.

  @retval EFI_SUCCESS           The device descriptor is read/written successfully.
  @retval EFI_INVALID_PARAMETER This is NULL or Descriptor is NULL or DescSize is NULL.
                                DescId, Index and Selector are invalid combination to point to a
                                type of UFS device descriptor.
  @retval EFI_DEVICE_ERROR      The device descriptor is not read/written successfully.

**/
EFI_STATUS
EFIAPI
UfsRwUfsDescriptor (
  IN EFI_UFS_DEVICE_CONFIG_PROTOCOL  *This,
  IN BOOLEAN                         Read,
  IN UINT8                           DescId,
  IN UINT8                           Index,
  IN UINT8                           Selector,
  IN OUT UINT8                       *Descriptor,
  IN OUT UINT32                      *DescSize
  );

/**
  Read or write specified flag of a UFS device.

  The function is used to read/write UFS flag descriptors. The consumer of this API is responsible
  for allocating the buffer pointed by Flag. The buffer size is 1 byte as UFS flag descriptor is
  just a single Boolean value that represents a TRUE or FALSE, '0' or '1', ON or OFF type of value.

  @param[in]      This          The pointer to the EFI_UFS_DEVICE_CONFIG_PROTOCOL instance.
  @param[in]      Read          The boolean variable to show r/w direction.
  @param[in]      FlagId        The ID of flag to be read or written.
  @param[in, out] Flag          The buffer to set or clear flag.

  @retval EFI_SUCCESS           The flag descriptor is set/clear successfully.
  @retval EFI_INVALID_PARAMETER This is NULL or Flag is NULL.
                                FlagId is an invalid UFS flag ID.
  @retval EFI_DEVICE_ERROR      The flag is not set/clear successfully.

**/
EFI_STATUS
EFIAPI
UfsRwUfsFlag (
  IN EFI_UFS_DEVICE_CONFIG_PROTOCOL  *This,
  IN BOOLEAN                         Read,
  IN UINT8                           FlagId,
  IN OUT UINT8                       *Flag
  );

/**
  Read or write specified attribute of a UFS device.

  The function is used to read/write UFS attributes. The consumer of this API is responsible for
  allocating the data buffer pointed by Attribute.

  @param[in]      This          The pointer to the EFI_UFS_DEVICE_CONFIG_PROTOCOL instance.
  @param[in]      Read          The boolean variable to show r/w direction.
  @param[in]      AttrId        The ID of Attribute.
  @param[in]      Index         The Index of Attribute.
  @param[in]      Selector      The Selector of Attribute.
  @param[in, out] Attribute     The buffer of Attribute to be read or written.
  @param[in, out] AttrSize      The size of Attribute buffer. On input, the size, in bytes, of the
                                data buffer specified by Attribute. On output, the number of bytes
                                that were actually transferred.

  @retval EFI_SUCCESS           The attribute is read/written successfully.
  @retval EFI_INVALID_PARAMETER This is NULL or Attribute is NULL or AttrSize is NULL.
                                AttrId, Index and Selector are invalid combination to point to a
                                type of UFS attribute.
  @retval EFI_DEVICE_ERROR      The attribute is not read/written successfully.

**/
EFI_STATUS
EFIAPI
UfsRwUfsAttribute (
  IN EFI_UFS_DEVICE_CONFIG_PROTOCOL  *This,
  IN BOOLEAN                         Read,
  IN UINT8                           AttrId,
  IN UINT8                           Index,
  IN UINT8                           Selector,
  IN OUT UINT8                       *Attribute,
  IN OUT UINT32                      *AttrSize
  );

/**
  Execute UIC command.

  @param[in]      This        Pointer to driver interface produced by the UFS controller.
  @param[in, out] UicCommand  Descriptor of the command that will be executed.

  @retval EFI_SUCCESS            Command executed successfully.
  @retval EFI_INVALID_PARAMETER  This or UicCommand is NULL.
  @retval Others                 Command failed to execute.
**/
EFI_STATUS
EFIAPI
UfsHcDriverInterfaceExecUicCommand (
  IN     EDKII_UFS_HC_DRIVER_INTERFACE  *This,
  IN OUT EDKII_UIC_COMMAND              *UicCommand
  );

/**
  Initializes UfsHcInfo field in private data.

  @param[in] Private  Pointer to host controller private data.

  @retval EFI_SUCCESS  UfsHcInfo initialized successfully.
  @retval Others       Failed to initalize UfsHcInfo.
**/
EFI_STATUS
GetUfsHcInfo (
  IN UFS_PASS_THRU_PRIVATE_DATA  *Private
  );

extern EFI_COMPONENT_NAME_PROTOCOL     gUfsPassThruComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL    gUfsPassThruComponentName2;
extern EFI_DRIVER_BINDING_PROTOCOL     gUfsPassThruDriverBinding;
extern EDKII_UFS_HC_PLATFORM_PROTOCOL  *mUfsHcPlatform;

#endif
