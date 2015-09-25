/** @file
  The header file of IScsiDriver.c.

Copyright (c) 2004 - 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _ISCSI_DRIVER_H_
#define _ISCSI_DRIVER_H_

#define ISCSI_V4_PRIVATE_GUID \
  { \
    0xfa3cde4c, 0x87c2, 0x427d, { 0xae, 0xde, 0x7d, 0xd0, 0x96, 0xc8, 0x8c, 0x58 } \
  }

#define ISCSI_V6_PRIVATE_GUID \
  { \
    0x28be27e5, 0x66cc, 0x4a31, { 0xa3, 0x15, 0xdb, 0x14, 0xc3, 0x74, 0x4d, 0x85 } \
  }

#define ISCSI_INITIATOR_NAME_VAR_NAME L"I_NAME"

#define IP_MODE_AUTOCONFIG_IP4     3
#define IP_MODE_AUTOCONFIG_IP6     4

extern EFI_COMPONENT_NAME2_PROTOCOL       gIScsiComponentName2;
extern EFI_COMPONENT_NAME_PROTOCOL        gIScsiComponentName;
extern EFI_UNICODE_STRING_TABLE           *gIScsiControllerNameTable;
extern EFI_ISCSI_INITIATOR_NAME_PROTOCOL  gIScsiInitiatorName;
extern EFI_AUTHENTICATION_INFO_PROTOCOL   gIScsiAuthenticationInfo;
extern EFI_EXT_SCSI_PASS_THRU_PROTOCOL    gIScsiExtScsiPassThruProtocolTemplate;
extern EFI_GUID                           gIScsiV4PrivateGuid;
extern EFI_GUID                           gIScsiV6PrivateGuid;

typedef struct {
  CHAR16          PortString[ISCSI_NAME_IFR_MAX_SIZE];
  LIST_ENTRY      NicInfoList;
  UINT8           NicCount;
  UINT8           CurrentNic;
  UINT8           MaxNic;
  BOOLEAN         Ipv6Flag;
  BOOLEAN         OneSessionEstablished;
  BOOLEAN         EnableMpio;
  UINT8           MpioCount;            // The number of attempts in MPIO.
  UINT8           Krb5MpioCount;        // The number of attempts login with KRB5 in MPIO.
  UINT8           SinglePathCount;      // The number of single path attempts.
  UINT8           ValidSinglePathCount; // The number of valid single path attempts.
  UINT8           BootSelectedIndex;
  UINT8           AttemptCount;
  LIST_ENTRY      AttemptConfigs;       // User configured Attempt list.
  CHAR8           InitiatorName[ISCSI_NAME_MAX_SIZE];
  UINTN           InitiatorNameLength;
  VOID            *NewAttempt;          // Attempt is created but not saved.
} ISCSI_PRIVATE_DATA;

extern ISCSI_PRIVATE_DATA                 *mPrivate;

typedef struct {
  LIST_ENTRY      Link;
  UINT32          HwAddressSize;
  EFI_MAC_ADDRESS PermanentAddress;
  UINT8           NicIndex;
  UINT16          VlanId;
  UINTN           BusNumber;
  UINTN           DeviceNumber;
  UINTN           FunctionNumber;
} ISCSI_NIC_INFO;

typedef struct _ISCSI_PRIVATE_PROTOCOL {
  UINT32  Reserved;
} ISCSI_PRIVATE_PROTOCOL;

//
// EFI Driver Binding Protocol for iSCSI driver.
//

/**
  Tests to see if this driver supports a given controller. If a child device is provided, 
  it tests to see if this driver supports creating a handle for the specified child device.

  This function checks to see if the driver specified by This supports the device specified by 
  ControllerHandle. Drivers typically use the device path attached to 
  ControllerHandle and/or the services from the bus I/O abstraction attached to 
  ControllerHandle to determine if the driver supports ControllerHandle. This function 
  may be called many times during platform initialization. In order to reduce boot times, the tests 
  performed by this function must be very small and take as little time as possible to execute. This 
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
                                   RemainingDevicePath is already managed by the driver
                                   specified by This.
  @retval EFI_ACCESS_DENIED        The device specified by ControllerHandle and
                                   RemainingDevicePath is already managed by a different
                                   driver or an application that requires exclusive access.
                                   Currently not implemented.
  @retval EFI_UNSUPPORTED          The device specified by ControllerHandle and
                                   RemainingDevicePath is not supported by the driver specified by This.
**/
EFI_STATUS
EFIAPI
IScsiIp4DriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
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
  @retval EFI_DEVICE_ERROR         The device could not be started due to a device error. Currently not implemented.
  @retval EFI_OUT_OF_RESOURCES     The request could not be completed due to a lack of resources.
  @retval Others                   The driver failed to start the device.

**/
EFI_STATUS
EFIAPI
IScsiIp4DriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
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
IScsiIp4DriverBindingStop (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN UINTN                        NumberOfChildren,
  IN EFI_HANDLE                   *ChildHandleBuffer OPTIONAL
  );

/**
  Tests to see if this driver supports a given controller. If a child device is provided, 
  it tests to see if this driver supports creating a handle for the specified child device.

  This function checks to see if the driver specified by This supports the device specified by 
  ControllerHandle. Drivers typically use the device path attached to 
  ControllerHandle and/or the services from the bus I/O abstraction attached to 
  ControllerHandle to determine if the driver supports ControllerHandle. This function 
  may be called many times during platform initialization. In order to reduce boot times, the tests 
  performed by this function must be very small and take as little time as possible to execute. This 
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
                                   RemainingDevicePath is already managed by the driver
                                   specified by This.
  @retval EFI_ACCESS_DENIED        The device specified by ControllerHandle and
                                   RemainingDevicePath is already managed by a different
                                   driver or an application that requires exclusive access.
                                   Currently not implemented.
  @retval EFI_UNSUPPORTED          The device specified by ControllerHandle and
                                   RemainingDevicePath is not supported by the driver specified by This.
**/
EFI_STATUS
EFIAPI
IScsiIp6DriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
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
  @retval EFI_DEVICE_ERROR         The device could not be started due to a device error. Currently not implemented.
  @retval EFI_OUT_OF_RESOURCES     The request could not be completed due to a lack of resources.
  @retval Others                   The driver failed to start the device.

**/
EFI_STATUS
EFIAPI
IScsiIp6DriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
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
IScsiIp6DriverBindingStop (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN UINTN                        NumberOfChildren,
  IN EFI_HANDLE                   *ChildHandleBuffer OPTIONAL
  );

//
// EFI Component Name(2) Protocol for iSCSI driver.
//

/**
  Retrieves a Unicode string that is the user readable name of the driver.

  This function retrieves the user readable name of a driver in the form of a
  Unicode string. If the driver specified by This has a user readable name in
  the language specified by Language, then a pointer to the driver name is
  returned in DriverName, and EFI_SUCCESS is returned. If the driver specified
  by This does not support the language specified by Language,
  then EFI_UNSUPPORTED is returned.

  @param[in]  This              A pointer to the EFI_COMPONENT_NAME2_PROTOCOL or
                                EFI_COMPONENT_NAME_PROTOCOL instance.

  @param[in]  Language          A pointer to a Null-terminated ASCII string
                                array indicating the language. This is the
                                language of the driver name that the caller is
                                requesting, and it must match one of the
                                languages specified in SupportedLanguages. The
                                number of languages supported by a driver is up
                                to the driver writer. Language is specified
                                in RFC 4646 or ISO 639-2 language code format.

  @param[out]  DriverName       A pointer to the Unicode string to return.
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
IScsiComponentNameGetDriverName (
  IN  EFI_COMPONENT_NAME_PROTOCOL   *This,
  IN  CHAR8                         *Language,
  OUT CHAR16                        **DriverName
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

  @param[in]  This              A pointer to the EFI_COMPONENT_NAME2_PROTOCOL or
                                EFI_COMPONENT_NAME_PROTOCOL instance.

  @param[in]  ControllerHandle  The handle of a controller that the driver
                                specified by This is managing.  This handle
                                specifies the controller whose name is to be
                                returned.

  @param[in]  ChildHandle       The handle of the child controller to retrieve
                                the name of.  This is an optional parameter that
                                may be NULL.  It will be NULL for device
                                drivers.  It will also be NULL for a bus drivers
                                that wish to retrieve the name of the bus
                                controller.  It will not be NULL for a bus
                                driver that wishes to retrieve the name of a
                                child controller.

  @param[in]  Language          A pointer to a Null-terminated ASCII string
                                array indicating the language.  This is the
                                language of the driver name that the caller is
                                requesting, and it must match one of the
                                languages specified in SupportedLanguages. The
                                number of languages supported by a driver is 
                                determined by the driver writer. Language is 
                                specified inRFC 4646 or ISO 639-2 language code 
                                format.
                                
  @param[out]  ControllerName   A pointer to the Unicode string to return.
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
IScsiComponentNameGetControllerName (
  IN  EFI_COMPONENT_NAME_PROTOCOL   *This,
  IN  EFI_HANDLE                    ControllerHandle,
  IN  EFI_HANDLE                    ChildHandle        OPTIONAL,
  IN  CHAR8                         *Language,
  OUT CHAR16                        **ControllerName
  );

//
// EFI iSCSI Initiator Name Protocol for iSCSI driver.
//

/**
  Retrieves the current set value of iSCSI Initiator Name.

  @param[in]       This          Pointer to the EFI_ISCSI_INITIATOR_NAME_PROTOCOL
                                 instance.
  @param[in, out]  BufferSize    Size of the buffer in bytes pointed to by Buffer /
                                 Actual size of the variable data buffer.
  @param[out]      Buffer        Pointer to the buffer for data to be read.

  @retval EFI_SUCCESS            Data was successfully retrieved into the provided
                                 buffer and the BufferSize was sufficient to handle
                                 the iSCSI initiator name.
  @retval EFI_BUFFER_TOO_SMALL   BufferSize is too small for the result. BufferSize
                                 will be updated with the size required to complete
                                 the request. Buffer will not be affected.
  @retval EFI_INVALID_PARAMETER  BufferSize is NULL. BufferSize and Buffer will not
                                 be affected.
  @retval EFI_INVALID_PARAMETER  Buffer is NULL. BufferSize and Buffer will not be
                                 affected.
  @retval EFI_DEVICE_ERROR       The iSCSI initiator name could not be retrieved
                                 due to a hardware error.

**/
EFI_STATUS
EFIAPI
IScsiGetInitiatorName (
  IN     EFI_ISCSI_INITIATOR_NAME_PROTOCOL  *This,
  IN OUT UINTN                              *BufferSize,
  OUT    VOID                               *Buffer
  );

/**
  Sets the iSSI Initiator Name.

  @param[in]       This          Pointer to the EFI_ISCSI_INITIATOR_NAME_PROTOCOL
                                 instance.
  @param[in, out]  BufferSize    Size of the buffer in bytes pointed to by Buffer.
  @param[in]       Buffer        Pointer to the buffer for data to be written.

  @retval EFI_SUCCESS            Data was successfully stored by the protocol.
  @retval EFI_UNSUPPORTED        Platform policies do not allow for data to be
                                 written.
  @retval EFI_INVALID_PARAMETER  BufferSize exceeds the maximum allowed limit.
                                 BufferSize will be updated with the maximum size
                                 required to complete the request.
  @retval EFI_INVALID_PARAMETER  Buffersize is NULL. BufferSize and Buffer will not
                                 be affected.
  @retval EFI_INVALID_PARAMETER  Buffer is NULL. BufferSize and Buffer will not be
                                 affected.
  @retval EFI_DEVICE_ERROR       The data could not be stored due to a hardware
                                 error.
  @retval EFI_OUT_OF_RESOURCES   Not enough storage is available to hold the data
  @retval EFI_PROTOCOL_ERROR     Input iSCSI initiator name does not adhere to RFC
                                 3720

**/
EFI_STATUS
EFIAPI
IScsiSetInitiatorName (
  IN     EFI_ISCSI_INITIATOR_NAME_PROTOCOL  *This,
  IN OUT UINTN                              *BufferSize,
  IN     VOID                               *Buffer
  );

//
// EFI_AUTHENTICATION_INFO_PROTOCOL for iSCSI driver.
//

/**
  Retrieves the authentication information associated with a particular controller handle.

  @param[in]  This              Pointer to the EFI_AUTHENTICATION_INFO_PROTOCOL.
  @param[in]  ControllerHandle  Handle to the Controller.
  @param[out] Buffer            Pointer to the authentication information. This function is
                                responsible for allocating the buffer and it is the caller's
                                responsibility to free buffer when the caller is finished with buffer.

  @retval EFI_DEVICE_ERROR      The authentication information could not be
                                retrieved due to a hardware error.

**/
EFI_STATUS
EFIAPI
IScsiGetAuthenticationInfo (
  IN  EFI_AUTHENTICATION_INFO_PROTOCOL *This,
  IN  EFI_HANDLE                       ControllerHandle,
  OUT VOID                             **Buffer
  );

/**
  Set the authentication information for a given controller handle.

  @param[in]  This             Pointer to the EFI_AUTHENTICATION_INFO_PROTOCOL.
  @param[in]  ControllerHandle Handle to the Controller.
  @param[in]  Buffer           Pointer to the authentication information.

  @retval EFI_UNSUPPORTED      If the platform policies do not allow setting of
                               the authentication information.

**/
EFI_STATUS
EFIAPI
IScsiSetAuthenticationInfo (
  IN EFI_AUTHENTICATION_INFO_PROTOCOL  *This,
  IN EFI_HANDLE                        ControllerHandle,
  IN VOID                              *Buffer
  );

//
// EFI_EXT_SCSI_PASS_THRU_PROTOCOL for iSCSI driver.
//

/**
  Sends a SCSI Request Packet to a SCSI device that is attached to the SCSI channel.
  This function supports both blocking I/O and nonblocking I/O. The blocking I/O
  functionality is required, and the nonblocking I/O functionality is optional.  

  @param[in]       This    A pointer to the EFI_EXT_SCSI_PASS_THRU_PROTOCOL instance.
  @param[in]       Target  The Target is an array of size TARGET_MAX_BYTES and it
                           represents the id of the SCSI device to send the SCSI
                           Request Packet. Each transport driver may choose to
                           utilize a subset of this size to suit the needs
                           of transport target representation. For example, a 
                           Fibre Channel driver may use only 8 bytes (WWN)
                           to represent an FC target.
  @param[in]       Lun     The LUN of the SCSI device to send the SCSI Request Packet.
  @param[in, out]  Packet  A pointer to the SCSI Request Packet to send to the
                           SCSI device specified by Target and Lun.                  
  @param[in]       Event   If nonblocking I/O is not supported then Event is ignored,
                           and blocking I/O is performed. If Event is NULL, then
                           blocking I/O is performed. If Event is not NULL and non
                           blocking I/O is supported, then nonblocking I/O is performed,
                           and Event will be signaled when the SCSI Request Packet
                           completes.

  @retval EFI_SUCCESS           The SCSI Request Packet was sent by the host. For
                                bi-directional commands, InTransferLength bytes 
                                were transferred from InDataBuffer.
                                For write and bi-directional commands, OutTransferLength
                                bytes were transferred by OutDataBuffer.
  @retval EFI_BAD_BUFFER_SIZE   The SCSI Request Packet was not executed.
                                The number of bytes that could be transferred is
                                returned in InTransferLength. For write and
                                bi-directional commands, OutTransferLength bytes
                                were transferred by OutDataBuffer.
  @retval EFI_NOT_READY         The SCSI Request Packet could not be sent because
                                there are too many SCSI Request Packets already
                                queued. The caller may retry later.
  @retval EFI_DEVICE_ERROR      A device error occurred while attempting to send
                                the SCSI Request Packet.                                
  @retval EFI_INVALID_PARAMETER Target, Lun, or the contents of ScsiRequestPacket
                                are invalid.
  @retval EFI_UNSUPPORTED       The command described by the SCSI Request Packet
                                is not supported by the host adapter.
                                This includes the case of Bi-directional SCSI
                                commands not supported by the implementation.
                                The SCSI Request Packet was not sent,
                                so no additional status information is available.
  @retval EFI_TIMEOUT           A timeout occurred while waiting for the SCSI
                                Request Packet to execute.

**/
EFI_STATUS
EFIAPI
IScsiExtScsiPassThruFunction (
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL                          *This,
  IN UINT8                                                    *Target,
  IN UINT64                                                   Lun,
  IN OUT EFI_EXT_SCSI_PASS_THRU_SCSI_REQUEST_PACKET           *Packet,
  IN EFI_EVENT                                                Event     OPTIONAL
  );

/**
  Used to retrieve the list of legal Target IDs and LUNs for SCSI devices on
  a SCSI channel. These can either be the list SCSI devices that are actually
  present on the SCSI channel, or the list of legal Target Ids and LUNs for the
  SCSI channel. Regardless, the caller of this function must probe the Target ID      
  and LUN returned to see if a SCSI device is actually present at that location    
  on the SCSI channel. 

  @param[in]       This          The EFI_EXT_SCSI_PASS_THRU_PROTOCOL instance.
  @param[in, out]  Target        On input, a pointer to the Target ID of a SCSI
                                 device present on the SCSI channel.  On output, a
                                 pointer to the Target ID of the next SCSI device
                                 present on a SCSI channel.  An input value of
                                 0xFFFFFFFF retrieves the Target ID of the first
                                 SCSI device present on a SCSI channel.
  @param[in, out]  Lun           On input, a pointer to the LUN of a SCSI device
                                 present on the SCSI channel. On output, a pointer
                                 to the LUN of the next SCSI device present on a
                                 SCSI channel.

  @retval EFI_SUCCESS            The Target ID and Lun of the next SCSI device  on
                                 the SCSI channel was returned in Target and Lun.
  @retval EFI_NOT_FOUND          There are no more SCSI devices on this SCSI
                                 channel.
  @retval EFI_INVALID_PARAMETER  Target is not 0xFFFFFFFF,and Target and Lun were
                                 not returned on a previous call to
                                 GetNextDevice().

**/
EFI_STATUS
EFIAPI
IScsiExtScsiPassThruGetNextTargetLun (
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL  *This,
  IN OUT UINT8                        **Target,
  IN OUT UINT64                       *Lun
  );

/**
  Allocate and build a device path node for a SCSI device on a SCSI channel.

  @param[in]       This          Protocol instance pointer.
  @param[in]       Target        The Target ID of the SCSI device for which a
                                 device path node is to be allocated and built.
  @param[in]       Lun           The LUN of the SCSI device for which a device
                                 path node is to be allocated and built.
  @param[in, out]  DevicePath    A pointer to a single device path node that
                                 describes the SCSI device specified by  Target and
                                 Lun. This function is responsible  for allocating
                                 the buffer DevicePath with the boot service
                                 AllocatePool().  It is the caller's
                                 responsibility to free DevicePath when the caller
                                 is finished with DevicePath.

  @retval EFI_SUCCESS            The device path node that describes the SCSI
                                 device specified by Target and Lun was allocated
                                 and  returned in DevicePath.
  @retval EFI_NOT_FOUND          The SCSI devices specified by Target and Lun does
                                 not exist on the SCSI channel.
  @retval EFI_INVALID_PARAMETER  DevicePath is NULL.
  @retval EFI_OUT_OF_RESOURCES   There are not enough resources to allocate
                                 DevicePath.

**/
EFI_STATUS
EFIAPI
IScsiExtScsiPassThruBuildDevicePath (
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL  *This,
  IN UINT8                            *Target,
  IN UINT64                           Lun,
  IN OUT EFI_DEVICE_PATH_PROTOCOL     **DevicePath
  );

/**
  Translate a device path node to a Target ID and LUN.

  @param[in]   This              Protocol instance pointer.
  @param[in]   DevicePath        A pointer to the device path node that  describes
                                 a SCSI device on the SCSI channel.
  @param[out]  Target            A pointer to the Target ID of a SCSI device  on
                                 the SCSI channel.
  @param[out]  Lun               A pointer to the LUN of a SCSI device on  the SCSI
                                 channel.

  @retval EFI_SUCCESS            DevicePath was successfully translated to a
                                 Target ID and LUN, and they were returned  in
                                 Target and Lun.
  @retval EFI_INVALID_PARAMETER  DevicePath/Target/Lun is NULL.
  @retval EFI_UNSUPPORTED        This driver does not support the device path node
                                 type in DevicePath.
  @retval EFI_NOT_FOUND          A valid translation from DevicePath to a  Target
                                 ID and LUN does not exist.

**/
EFI_STATUS
EFIAPI
IScsiExtScsiPassThruGetTargetLun (
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL  *This,
  IN EFI_DEVICE_PATH_PROTOCOL         *DevicePath,
  OUT UINT8                           **Target,
  OUT UINT64                          *Lun
  );

/**
  Resets a SCSI channel.This operation resets all the SCSI devices connected to
  the SCSI channel.

  @param[in]  This               Protocol instance pointer.

  @retval EFI_UNSUPPORTED        It is not supported.

**/
EFI_STATUS
EFIAPI
IScsiExtScsiPassThruResetChannel (
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL  *This
  );

/**
  Resets a SCSI device that is connected to a SCSI channel.

  @param[in]  This               Protocol instance pointer.
  @param[in]  Target             The Target ID of the SCSI device to reset.
  @param[in]  Lun                The LUN of the SCSI device to reset.

  @retval EFI_UNSUPPORTED        It is not supported.

**/
EFI_STATUS
EFIAPI
IScsiExtScsiPassThruResetTargetLun (
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL  *This,
  IN UINT8                            *Target,
  IN UINT64                           Lun
  );

/**
  Retrieve the list of legal Target IDs for SCSI devices on a SCSI channel.                         

  @param[in]       This         A pointer to the EFI_EXT_SCSI_PASS_THRU_PROTOCOL
                                instance.
  @param[in, out]  Target       (TARGET_MAX_BYTES) of a SCSI device present on
                                the SCSI channel. On output, a pointer to the
                                Target ID (an array of TARGET_MAX_BYTES) of the
                                next SCSI device present on a SCSI channel.
                                An input value of 0xF(all bytes in the array are 0xF)
                                in the Target array retrieves the Target ID of the
                                first SCSI device present on a SCSI channel.                 

  @retval EFI_SUCCESS           The Target ID of the next SCSI device on the SCSI
                                channel was returned in Target.
  @retval EFI_INVALID_PARAMETER Target or Lun is NULL.
  @retval EFI_TIMEOUT           Target array is not all 0xF, and Target was not
                                returned on a previous call to GetNextTarget().
  @retval EFI_NOT_FOUND         There are no more SCSI devices on this SCSI channel.

**/
EFI_STATUS
EFIAPI
IScsiExtScsiPassThruGetNextTarget (
  IN EFI_EXT_SCSI_PASS_THRU_PROTOCOL  *This,
  IN OUT UINT8                        **Target
  );

#endif
