/** @file
  RedfishRestExDxe support functions definitions.

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
  (C) Copyright 2019-2020 Hewlett Packard Enterprise Development LP<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef EFI_REDFISH_RESTEX_INTERNAL_H_
#define EFI_REDFISH_RESTEX_INTERNAL_H_

///
/// Libraries classes
///
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/HttpIoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/NetLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiDriverEntryPoint.h>

///
/// UEFI Driver Model Protocols
///
#include <Protocol/DriverBinding.h>
#include <Protocol/RestEx.h>
#include <Protocol/ServiceBinding.h>

#include "RedfishRestExDriver.h"

/**
  This function check

  @param[in]  Instance          Pointer to EFI_REST_EX_PROTOCOL instance for a particular
                                REST service.
  @param[in]  HttpReceiveEfiStatus  This is the status return from HttpIoRecvResponse

  @retval EFI_SUCCESS           The payload receive from Redfish service in successfully.
  @retval EFI_NOT_READY         May need to resend the HTTP request.
  @retval EFI_DEVICE_ERROR      Something wrong and can't be resolved.
  @retval Others                Other errors as indicated.

**/
EFI_STATUS
RedfishCheckHttpReceiveStatus (
  IN RESTEX_INSTANCE *Instance,
  IN EFI_STATUS HttpIoReceiveStatus
  );

/**
  This function send the HTTP request without body to see
  if the write to URL is permitted by Redfish service. This function
  checks if the HTTP request has Content-length in HTTP header. If yes,
  set HTTP body to NULL and then send to service. Check the HTTP status
  for the firther actions.

  @param[in]  This                    Pointer to EFI_REST_EX_PROTOCOL instance for a particular
                                      REST service.
  @param[in]  RequestMessage          Pointer to the HTTP request data for this resource
  @param[in]  PreservedRequestHeaders The pointer to save the request headers
  @param[in]  ItsWrite                This is write method to URL.

  @retval EFI_INVALID_PARAMETER  Improper given parameters.
  @retval EFI_SUCCESS            This HTTP request is free to send to Redfish service.
  @retval EFI_OUT_OF_RESOURCES   NOt enough memory to process.
  @retval EFI_ACCESS_DENIED      Not allowed to write to this URL.

  @retval Others                 Other errors as indicated.

**/
EFI_STATUS
RedfishHttpAddExpectation (
  IN EFI_REST_EX_PROTOCOL   *This,
  IN EFI_HTTP_MESSAGE       *RequestMessage,
  IN EFI_HTTP_HEADER        **PreservedRequestHeaders,
  IN BOOLEAN                *ItsWrite
  );

/**
  Provides a simple HTTP-like interface to send and receive resources from a REST service.

  The SendReceive() function sends an HTTP request to this REST service, and returns a
  response when the data is retrieved from the service. RequestMessage contains the HTTP
  request to the REST resource identified by RequestMessage.Request.Url. The
  ResponseMessage is the returned HTTP response for that request, including any HTTP
  status.

  @param[in]  This                Pointer to EFI_REST_EX_PROTOCOL instance for a particular
                                  REST service.
  @param[in]  RequestMessage      Pointer to the HTTP request data for this resource
  @param[out] ResponseMessage     Pointer to the HTTP response data obtained for this requested.

  @retval EFI_SUCCESS             operation succeeded.
  @retval EFI_INVALID_PARAMETER   This, RequestMessage, or ResponseMessage are NULL.
  @retval EFI_DEVICE_ERROR        An unexpected system or network error occurred.

**/
EFI_STATUS
EFIAPI
RedfishRestExSendReceive (
  IN      EFI_REST_EX_PROTOCOL   *This,
  IN      EFI_HTTP_MESSAGE       *RequestMessage,
  OUT     EFI_HTTP_MESSAGE       *ResponseMessage
  );

/**
  Obtain the current time from this REST service instance.

  The GetServiceTime() function is an optional interface to obtain the current time from
  this REST service instance. If this REST service does not support to retrieve the time,
  this function returns EFI_UNSUPPORTED. This function must returns EFI_UNSUPPORTED if
  EFI_REST_EX_SERVICE_TYPE returned in EFI_REST_EX_SERVICE_INFO from GetService() is
  EFI_REST_EX_SERVICE_UNSPECIFIC.

  @param[in]  This                Pointer to EFI_REST_EX_PROTOCOL instance for a particular
                                  REST service.
  @param[out] Time                A pointer to storage to receive a snapshot of the current time of
                                  the REST service.

  @retval EFI_SUCCESS             operation succeeded.
  @retval EFI_INVALID_PARAMETER   This or Time are NULL.
  @retval EFI_UNSUPPORTED         The RESTful service does not support returning the time.
  @retval EFI_DEVICE_ERROR        An unexpected system or network error occurred.
  @retval EFI_NOT_READY           The configuration of this instance is not set yet. Configure() must
                                  be executed and returns successfully prior to invoke this function.

**/
EFI_STATUS
EFIAPI
RedfishRestExGetServiceTime (
  IN      EFI_REST_EX_PROTOCOL   *This,
  OUT     EFI_TIME               *Time
  );

/**
  This function returns the information of REST service provided by this EFI REST EX driver instance.

  The information such as the type of REST service and the access mode of REST EX driver instance
  (In-band or Out-of-band) are described in EFI_REST_EX_SERVICE_INFO structure. For the vendor-specific
  REST service, vendor-specific REST service information is returned in VendorSpecifcData.
  REST EX driver designer is well know what REST service this REST EX driver instance intends to
  communicate with. The designer also well know this driver instance is used to talk to BMC through
  specific platform mechanism or talk to REST server through UEFI HTTP protocol. REST EX driver is
  responsible to fill up the correct information in EFI_REST_EX_SERVICE_INFO. EFI_REST_EX_SERVICE_INFO
  is referred by EFI REST clients to pickup the proper EFI REST EX driver instance to get and set resource.
  GetService() is a basic and mandatory function which must be able to use even Configure() is not invoked
  in previously.

  @param[in]  This                Pointer to EFI_REST_EX_PROTOCOL instance for a particular
                                  REST service.
  @param[out] RestExServiceInfo   Pointer to receive a pointer to EFI_REST_EX_SERVICE_INFO structure. The
                                  format of EFI_REST_EX_SERVICE_INFO is version controlled for the future
                                  extension. The version of EFI_REST_EX_SERVICE_INFO structure is returned
                                  in the header within this structure. EFI REST client refers to the correct
                                  format of structure according to the version number. The pointer to
                                  EFI_REST_EX_SERVICE_INFO is a memory block allocated by EFI REST EX driver
                                  instance. That is caller's responsibility to free this memory when this
                                  structure is no longer needed. Refer to Related Definitions below for the
                                  definitions of EFI_REST_EX_SERVICE_INFO structure.

  @retval EFI_SUCCESS             EFI_REST_EX_SERVICE_INFO is returned in RestExServiceInfo. This function
                                  is not supported in this REST EX Protocol driver instance.
  @retval EFI_UNSUPPORTED         This function is not supported in this REST EX Protocol driver instance.

**/
EFI_STATUS
EFIAPI
RedfishRestExGetService (
  IN   EFI_REST_EX_PROTOCOL      *This,
  OUT  EFI_REST_EX_SERVICE_INFO  **RestExServiceInfo
  );

/**
  This function returns operational configuration of current EFI REST EX child instance.

  This function returns the current configuration of EFI REST EX child instance. The format of
  operational configuration depends on the implementation of EFI REST EX driver instance. For
  example, HTTP-aware EFI REST EX driver instance uses EFI HTTP protocol as the undying protocol
  to communicate with REST service. In this case, the type of configuration is
  EFI_REST_EX_CONFIG_TYPE_HTTP returned from GetService(). EFI_HTTP_CONFIG_DATA is used as EFI REST
  EX configuration format and returned to EFI REST client. User has to type cast RestExConfigData
  to EFI_HTTP_CONFIG_DATA. For those non HTTP-aware REST EX driver instances, the type of configuration
  is EFI_REST_EX_CONFIG_TYPE_UNSPECIFIC returned from GetService(). In this case, the format of
  returning data could be non industrial. Instead, the format of configuration data is system/platform
  specific definition such as BMC mechanism used in EFI REST EX driver instance. EFI REST client and
  EFI REST EX driver instance have to refer to the specific system /platform spec which is out of UEFI scope.

  @param[in]  This                This is the EFI_REST_EX_PROTOCOL instance.
  @param[out] RestExConfigData    Pointer to receive a pointer to EFI_REST_EX_CONFIG_DATA.
                                  The memory allocated for configuration data should be freed
                                  by caller. See Related Definitions for the details.

  @retval EFI_SUCCESS             EFI_REST_EX_CONFIG_DATA is returned in successfully.
  @retval EFI_UNSUPPORTED         This function is not supported in this REST EX Protocol driver instance.
  @retval EFI_NOT_READY           The configuration of this instance is not set yet. Configure() must be
                                  executed and returns successfully prior to invoke this function.

**/
EFI_STATUS
EFIAPI
RedfishRestExGetModeData (
  IN  EFI_REST_EX_PROTOCOL      *This,
  OUT EFI_REST_EX_CONFIG_DATA   *RestExConfigData
  );

/**
  This function is used to configure EFI REST EX child instance.

  This function is used to configure the setting of underlying protocol of REST EX child
  instance. The type of configuration is according to the implementation of EFI REST EX
  driver instance. For example, HTTP-aware EFI REST EX driver instance uses EFI HTTP protocol
  as the undying protocol to communicate with REST service. The type of configuration is
  EFI_REST_EX_CONFIG_TYPE_HTTP and RestExConfigData is the same format with EFI_HTTP_CONFIG_DATA.
  Akin to HTTP configuration, REST EX child instance can be configure to use different HTTP
  local access point for the data transmission. Multiple REST clients may use different
  configuration of HTTP to distinguish themselves, such as to use the different TCP port.
  For those non HTTP-aware REST EX driver instance, the type of configuration is
  EFI_REST_EX_CONFIG_TYPE_UNSPECIFIC. RestExConfigData refers to the non industrial standard.
  Instead, the format of configuration data is system/platform specific definition such as BMC.
  In this case, EFI REST client and EFI REST EX driver instance have to refer to the specific
  system/platform spec which is out of the UEFI scope. Besides GetService()function, no other
  EFI REST EX functions can be executed by this instance until Configure()is executed and returns
  successfully. All other functions must returns EFI_NOT_READY if this instance is not configured
  yet. Set RestExConfigData to NULL means to put EFI REST EX child instance into the unconfigured
  state.

  @param[in]  This                This is the EFI_REST_EX_PROTOCOL instance.
  @param[in]  RestExConfigData    Pointer to EFI_REST_EX_CONFIG_DATA. See Related Definitions in
                                  GetModeData() protocol interface.

  @retval EFI_SUCCESS             EFI_REST_EX_CONFIG_DATA is set in successfully.
  @retval EFI_DEVICE_ERROR        Configuration for this REST EX child instance is failed with the given
                                  EFI_REST_EX_CONFIG_DATA.
  @retval EFI_UNSUPPORTED         This function is not supported in this REST EX Protocol driver instance.

**/
EFI_STATUS
EFIAPI
RedfishRestExConfigure (
  IN  EFI_REST_EX_PROTOCOL    *This,
  IN  EFI_REST_EX_CONFIG_DATA RestExConfigData
  );

/**
  This function sends REST request to REST service and signal caller's event asynchronously when
  the final response is received by REST EX Protocol driver instance.

  The essential design of this function is to handle asynchronous send/receive implicitly according
  to REST service asynchronous request mechanism. Caller will get the notification once the response
  is returned from REST service.

  @param[in]  This                  This is the EFI_REST_EX_PROTOCOL instance.
  @param[in]  RequestMessage        This is the HTTP request message sent to REST service. Set RequestMessage
                                    to NULL to cancel the previous asynchronous request associated with the
                                    corresponding RestExToken. See descriptions for the details.
  @param[in]  RestExToken           REST EX token which REST EX Protocol instance uses to notify REST client
                                    the status of response of asynchronous REST request. See related definition
                                    of EFI_REST_EX_TOKEN.
  @param[in]  TimeOutInMilliSeconds The pointer to the timeout in milliseconds which REST EX Protocol driver
                                    instance refers as the duration to drop asynchronous REST request. NULL
                                    pointer means no timeout for this REST request. REST EX Protocol driver
                                    signals caller's event with EFI_STATUS set to EFI_TIMEOUT in RestExToken
                                    if REST EX Protocol can't get the response from REST service within
                                    TimeOutInMilliSeconds.

  @retval EFI_SUCCESS               Asynchronous REST request is established.
  @retval EFI_UNSUPPORTED           This REST EX Protocol driver instance doesn't support asynchronous request.
  @retval EFI_TIMEOUT               Asynchronous REST request is not established and timeout is expired.
  @retval EFI_ABORT                 Previous asynchronous REST request has been canceled.
  @retval EFI_DEVICE_ERROR          Otherwise, returns EFI_DEVICE_ERROR for other errors according to HTTP Status Code.
  @retval EFI_NOT_READY             The configuration of this instance is not set yet. Configure() must be executed
                                    and returns successfully prior to invoke this function.

**/
EFI_STATUS
EFIAPI
RedfishRestExAyncSendReceive (
  IN      EFI_REST_EX_PROTOCOL   *This,
  IN      EFI_HTTP_MESSAGE       *RequestMessage OPTIONAL,
  IN      EFI_REST_EX_TOKEN      *RestExToken,
  IN      UINTN                  *TimeOutInMilliSeconds OPTIONAL
  );

/**
  This function sends REST request to a REST Event service and signals caller's event
  token asynchronously when the URI resource change event is received by REST EX
  Protocol driver instance.

  The essential design of this function is to monitor event implicitly according to
  REST service event service mechanism. Caller will get the notification if certain
  resource is changed.

  @param[in]  This                  This is the EFI_REST_EX_PROTOCOL instance.
  @param[in]  RequestMessage        This is the HTTP request message sent to REST service. Set RequestMessage
                                    to NULL to cancel the previous event service associated with the corresponding
                                    RestExToken. See descriptions for the details.
  @param[in]  RestExToken           REST EX token which REST EX Protocol driver instance uses to notify REST client
                                    the URI resource which monitored by REST client has been changed. See the related
                                    definition of EFI_REST_EX_TOKEN in EFI_REST_EX_PROTOCOL.AsyncSendReceive().

  @retval EFI_SUCCESS               Asynchronous REST request is established.
  @retval EFI_UNSUPPORTED           This REST EX Protocol driver instance doesn't support asynchronous request.
  @retval EFI_ABORT                 Previous asynchronous REST request has been canceled or event subscription has been
                                    delete from service.
  @retval EFI_DEVICE_ERROR          Otherwise, returns EFI_DEVICE_ERROR for other errors according to HTTP Status Code.
  @retval EFI_NOT_READY             The configuration of this instance is not set yet. Configure() must be executed
                                    and returns successfully prior to invoke this function.

**/
EFI_STATUS
EFIAPI
RedfishRestExEventService (
  IN      EFI_REST_EX_PROTOCOL   *This,
  IN      EFI_HTTP_MESSAGE       *RequestMessage OPTIONAL,
  IN      EFI_REST_EX_TOKEN      *RestExToken
  );
/**
  Create a new TLS session becuase the previous on is closed.
  status.

  @param[in]  Instance            Pointer to EFI_REST_EX_PROTOCOL instance for a particular
                                  REST service.
  @retval EFI_SUCCESS             operation succeeded.
  @retval EFI Errors              Other errors.

**/
EFI_STATUS
ResetHttpTslSession (
  IN   RESTEX_INSTANCE  *Instance
);


/**
  Callback function which provided by user to remove one node in NetDestroyLinkList process.

  @param[in]    Entry           The entry to be removed.
  @param[in]    Context         Pointer to the callback context corresponds to the Context in NetDestroyLinkList.

  @retval EFI_SUCCESS           The entry has been removed successfully.
  @retval Others                Fail to remove the entry.

**/
EFI_STATUS
EFIAPI
RestExDestroyChildEntryInHandleBuffer (
  IN LIST_ENTRY         *Entry,
  IN VOID               *Context
  );

/**
  Destroy the RestEx instance and recycle the resources.

  @param[in]  Instance        The pointer to the RestEx instance.

**/
VOID
RestExDestroyInstance (
  IN RESTEX_INSTANCE         *Instance
  );

/**
  Create the RestEx instance and initialize it.

  @param[in]  Service              The pointer to the RestEx service.
  @param[out] Instance             The pointer to the RestEx instance.

  @retval EFI_OUT_OF_RESOURCES   Failed to allocate resources.
  @retval EFI_SUCCESS            The RestEx instance is created.

**/
EFI_STATUS
RestExCreateInstance (
  IN  RESTEX_SERVICE         *Service,
  OUT RESTEX_INSTANCE        **Instance
  );


/**
  Release all the resource used the RestEx service binding instance.

  @param[in]  RestExSb           The RestEx service binding instance.

**/
VOID
RestExDestroyService (
  IN RESTEX_SERVICE     *RestExSb
  );

/**
  Create then initialize a RestEx service binding instance.

  @param[in]  Controller        The controller to install the RestEx service
                                binding on.
  @param[in]  Image             The driver binding image of the RestEx driver.
  @param[out] Service           The variable to receive the created service
                                binding instance.

  @retval EFI_OUT_OF_RESOURCES   Failed to allocate resource to create the instance.
  @retval EFI_SUCCESS            The service instance is created for the controller.

**/
EFI_STATUS
RestExCreateService (
  IN     EFI_HANDLE            Controller,
  IN     EFI_HANDLE            Image,
  OUT    RESTEX_SERVICE        **Service
  );

/**
  This is the declaration of an EFI image entry point. This entry point is
  the same for UEFI Applications, UEFI OS Loaders, and UEFI Drivers including
  both device drivers and bus drivers.

  @param[in]  ImageHandle      The firmware allocated handle for the UEFI image.
  @param[in]  SystemTable      A pointer to the EFI System Table.

  @retval EFI_SUCCESS           The operation completed successfully.
  @retval Others                An unexpected error occurred.
**/
EFI_STATUS
EFIAPI
RedfishRestExDriverEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
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
  Because ControllerHandle may have been previously started by the same driver, if a protocol is
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
RedfishRestExDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  );

/**
  Starts a device controller or a bus controller.

  The Start() function is designed to be invoked from the EFI boot service ConnectController().
  As a result, much of the error checking on the parameters to Start() has been moved into this
  common boot service. It is legal to call Start() from other locations,
  but the following calling restrictions must be followed, or the system behavior will not be deterministic.
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
RedfishRestExDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  );

/**
  Stops a device controller or a bus controller.

  The Stop() function is designed to be invoked from the EFI boot service DisconnectController().
  As a result, much of the error checking on the parameters to Stop() has been moved
  into this common boot service. It is legal to call Stop() from other locations,
  but the following calling restrictions must be followed, or the system behavior will not be deterministic.
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
RedfishRestExDriverBindingStop (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN UINTN                        NumberOfChildren,
  IN EFI_HANDLE                   *ChildHandleBuffer OPTIONAL
  );

/**
  Creates a child handle and installs a protocol.

  The CreateChild() function installs a protocol on ChildHandle.
  If ChildHandle is a pointer to NULL, then a new handle is created and returned in ChildHandle.
  If ChildHandle is not a pointer to NULL, then the protocol installs on the existing ChildHandle.

  @param[in] This        Pointer to the EFI_SERVICE_BINDING_PROTOCOL instance.
  @param[in] ChildHandle Pointer to the handle of the child to create. If it is NULL,
                         then a new handle is created. If it is a pointer to an existing UEFI handle,
                         then the protocol is added to the existing UEFI handle.

  @retval EFI_SUCCES            The protocol was added to ChildHandle.
  @retval EFI_INVALID_PARAMETER ChildHandle is NULL.
  @retval EFI_OUT_OF_RESOURCES  There are not enough resources available to create
                                the child
  @retval other                 The child handle was not created

**/
EFI_STATUS
EFIAPI
RedfishRestExServiceBindingCreateChild (
  IN EFI_SERVICE_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                    *ChildHandle
  );

/**
  Destroys a child handle with a protocol installed on it.

  The DestroyChild() function does the opposite of CreateChild(). It removes a protocol
  that was installed by CreateChild() from ChildHandle. If the removed protocol is the
  last protocol on ChildHandle, then ChildHandle is destroyed.

  @param[in] This        Pointer to the EFI_SERVICE_BINDING_PROTOCOL instance.
  @param[in] ChildHandle Handle of the child to destroy

  @retval EFI_SUCCES            The protocol was removed from ChildHandle.
  @retval EFI_UNSUPPORTED       ChildHandle does not support the protocol that is being removed.
  @retval EFI_INVALID_PARAMETER Child handle is NULL.
  @retval EFI_ACCESS_DENIED     The protocol could not be removed from the ChildHandle
                                because its services are being used.
  @retval other                 The child handle was not destroyed

**/
EFI_STATUS
EFIAPI
RedfishRestExServiceBindingDestroyChild (
  IN EFI_SERVICE_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                    ChildHandle
  );
#endif
