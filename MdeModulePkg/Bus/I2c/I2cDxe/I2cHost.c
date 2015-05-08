/** @file
  This file implements I2C Host Protocol which provides callers with the ability to 
  do I/O transactions to all of the devices on the I2C bus.

  Copyright (c) 2014, Hewlett-Packard Development Company, L.P.<BR>
  Copyright (c) 2013 - 2015, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "I2cDxe.h"

EFI_DRIVER_BINDING_PROTOCOL gI2cHostDriverBinding = {
  I2cHostDriverSupported,
  I2cHostDriverStart,
  I2cHostDriverStop,
  0x10,
  NULL,
  NULL
};

//
// Driver name table 
//
GLOBAL_REMOVE_IF_UNREFERENCED EFI_UNICODE_STRING_TABLE mI2cHostDriverNameTable[] = {
  { "eng;en", L"I2c Host Driver" },
  { NULL , NULL }
};

//
// EFI Component Name Protocol
//
GLOBAL_REMOVE_IF_UNREFERENCED EFI_COMPONENT_NAME_PROTOCOL  gI2cHostComponentName = {
  (EFI_COMPONENT_NAME_GET_DRIVER_NAME) I2cHostComponentNameGetDriverName,
  (EFI_COMPONENT_NAME_GET_CONTROLLER_NAME) I2cHostComponentNameGetControllerName,
  "eng"
};

//
// EFI Component Name 2 Protocol
//
GLOBAL_REMOVE_IF_UNREFERENCED EFI_COMPONENT_NAME2_PROTOCOL gI2cHostComponentName2 = {
  I2cHostComponentNameGetDriverName,
  I2cHostComponentNameGetControllerName,
  "en"
};

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
I2cHostComponentNameGetDriverName (
  IN  EFI_COMPONENT_NAME2_PROTOCOL  *This,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **DriverName
  )
{
  return LookupUnicodeString2 (
           Language,
           This->SupportedLanguages,
           mI2cHostDriverNameTable,
           DriverName,
           (BOOLEAN)(This != &gI2cHostComponentName2)
           );
}

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
I2cHostComponentNameGetControllerName (
  IN  EFI_COMPONENT_NAME2_PROTOCOL                    *This,
  IN  EFI_HANDLE                                      ControllerHandle,
  IN  EFI_HANDLE                                      ChildHandle        OPTIONAL,
  IN  CHAR8                                           *Language,
  OUT CHAR16                                          **ControllerName
  )
{
  return EFI_UNSUPPORTED;
}

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
I2cHostDriverSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_I2C_MASTER_PROTOCOL                       *I2cMaster;
  EFI_I2C_BUS_CONFIGURATION_MANAGEMENT_PROTOCOL *I2cBusConfigurationManagement;
  EFI_STATUS                                    Status;

  //
  //  Locate I2C Bus Configuration Management Protocol
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiI2cBusConfigurationManagementProtocolGuid,
                  (VOID **)&I2cBusConfigurationManagement,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Close the protocol because we don't use it here
  //
  gBS->CloseProtocol (
                  Controller,
                  &gEfiI2cBusConfigurationManagementProtocolGuid,
                  This->DriverBindingHandle,
                  Controller
                  );

  //
  //  Locate I2C Master Protocol
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiI2cMasterProtocolGuid,
                  (VOID **)&I2cMaster,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
}

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
I2cHostDriverStart (
  IN EFI_DRIVER_BINDING_PROTOCOL        *This,
  IN EFI_HANDLE                         Controller,
  IN EFI_DEVICE_PATH_PROTOCOL           *RemainingDevicePath
  )
{
  EFI_STATUS                                          Status;
  EFI_I2C_MASTER_PROTOCOL                             *I2cMaster;
  EFI_I2C_BUS_CONFIGURATION_MANAGEMENT_PROTOCOL       *I2cBusConfigurationManagement;
  I2C_HOST_CONTEXT                                    *I2cHostContext;

  I2cMaster                     = NULL;
  I2cHostContext                = NULL;
  I2cBusConfigurationManagement = NULL;

  //
  // Locate I2C Bus Configuration Management Protocol
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiI2cBusConfigurationManagementProtocolGuid,
                  (VOID **)&I2cBusConfigurationManagement,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "I2cHost: Open I2C bus configuration error, Status = %r\n", Status));
    return Status;
  }

  //
  // Locate I2C Master Protocol
  //
  Status = gBS->OpenProtocol ( 
                  Controller,
                  &gEfiI2cMasterProtocolGuid,
                  (VOID **)&I2cMaster,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "I2cHost: Open I2C master error, Status = %r\n", Status));
    goto Exit;
  }

  //
  // Allocate the I2C Host Context structure
  //
  I2cHostContext = AllocateZeroPool (sizeof (I2C_HOST_CONTEXT));
  if (I2cHostContext == NULL) {
    DEBUG ((EFI_D_ERROR, "I2cHost: there is no enough memory to allocate.\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }

  //
  // Initialize the context structure for the current I2C Controller
  //
  I2cHostContext->Signature                     = I2C_HOST_SIGNATURE;
  I2cHostContext->I2cMaster                     = I2cMaster;
  I2cHostContext->I2cBusConfigurationManagement = I2cBusConfigurationManagement;
  I2cHostContext->I2cBusConfiguration           = (UINTN) -1;
  InitializeListHead(&I2cHostContext->RequestList);

  //
  // Reset the controller
  //
  Status = I2cMaster->Reset (I2cMaster);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "I2cHost: I2C controller reset failed!\n"));
    goto Exit;
  }

  //
  // Create the I2C transaction complete event
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_I2C_SYNC,
                  I2cHostRequestCompleteEvent,
                  I2cHostContext,
                  &I2cHostContext->I2cEvent
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "I2cHost: create complete event error, Status = %r\n", Status));
    goto Exit;
  }

  //
  // Get the bus management event
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_I2C_SYNC,
                  I2cHostI2cBusConfigurationAvailable,
                  I2cHostContext,
                  &I2cHostContext->I2cBusConfigurationEvent
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "I2cHost: create bus available event error, Status = %r\n", Status));
    goto Exit;
  }  

  //
  // Build the I2C host protocol for the current I2C controller
  //
  I2cHostContext->I2cHost.QueueRequest              = I2cHostQueueRequest;
  I2cHostContext->I2cHost.I2cControllerCapabilities = I2cMaster->I2cControllerCapabilities;

  //
  //  Install the driver protocol
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Controller,
                  &gEfiI2cHostProtocolGuid,
                  &I2cHostContext->I2cHost,
                  NULL
                  );
Exit:
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "I2cHost: Start() function failed, Status = %r\n", Status));
    if (I2cBusConfigurationManagement != NULL) {
      gBS->CloseProtocol (
                      Controller,
                      &gEfiI2cBusConfigurationManagementProtocolGuid,
                      This->DriverBindingHandle,
                      Controller
                      );      
    }
    
    if ((I2cHostContext != NULL) && (I2cHostContext->I2cEvent != NULL)) {
      gBS->CloseEvent (I2cHostContext->I2cEvent);
      I2cHostContext->I2cEvent = NULL;
    }

    if ((I2cHostContext != NULL) && (I2cHostContext->I2cBusConfigurationEvent != NULL)) {
      gBS->CloseEvent (I2cHostContext->I2cBusConfigurationEvent);
      I2cHostContext->I2cBusConfigurationEvent = NULL;
    }

    //
    //  Release the context structure upon failure
    //
    if (I2cHostContext != NULL) {
      FreePool (I2cHostContext);
    }
  }

  //
  //  Return the operation status.
  //
  return Status;
}

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
I2cHostDriverStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL       *This,
  IN  EFI_HANDLE                        Controller,
  IN  UINTN                             NumberOfChildren,
  IN  EFI_HANDLE                        *ChildHandleBuffer
  )
{
  EFI_STATUS                  Status;  
  I2C_HOST_CONTEXT            *I2cHostContext;
  EFI_I2C_HOST_PROTOCOL       *I2cHost;
  EFI_TPL                     TplPrevious;

  TplPrevious = EfiGetCurrentTpl ();
  if (TplPrevious > TPL_I2C_SYNC) {
    DEBUG ((EFI_D_ERROR, "I2cHost: TPL %d is too high in Stop.\n", TplPrevious));
    return EFI_DEVICE_ERROR;
  }

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiI2cHostProtocolGuid,
                  (VOID **) &I2cHost,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );

  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  I2cHostContext = I2C_HOST_CONTEXT_FROM_PROTOCOL (I2cHost);

  //
  // Raise TPL for critical section
  //
  TplPrevious = gBS->RaiseTPL (TPL_I2C_SYNC);
  
  //
  // If there is pending request or pending bus configuration, do not stop
  //
  Status = EFI_DEVICE_ERROR;
  if (( !I2cHostContext->I2cBusConfigurationManagementPending )
    && IsListEmpty (&I2cHostContext->RequestList)) {
    
    //
    //  Remove the I2C host protocol
    //
    Status = gBS->UninstallMultipleProtocolInterfaces (
                    Controller,
                    &gEfiI2cHostProtocolGuid,
                    I2cHost,
                    NULL
                    );
  }
  
  //
  // Leave critical section
  //
  gBS->RestoreTPL (TplPrevious);
  if (!EFI_ERROR (Status)) {
    gBS->CloseProtocol (
           Controller,
           &gEfiI2cBusConfigurationManagementProtocolGuid,
           This->DriverBindingHandle,
           Controller
           );

    //
    // Release I2c Host resources
    //
    if (I2cHostContext->I2cBusConfigurationEvent != NULL) {
      gBS->CloseEvent (I2cHostContext->I2cBusConfigurationEvent);
      I2cHostContext->I2cBusConfigurationEvent = NULL;
    }
    
    if (I2cHostContext->I2cEvent != NULL) {
      gBS->CloseEvent (I2cHostContext->I2cEvent);
      I2cHostContext->I2cEvent = NULL;
    }
    
    FreePool (I2cHostContext);
  }

  //
  //  Return the stop status
  //
  return Status;
}

/**
  Handle the I2C bus configuration available event

  This routine is called at TPL_I2C_SYNC.

  @param[in] Event    Address of an EFI_EVENT handle
  @param[in] Context  Address of an I2C_HOST_CONTEXT structure

**/
VOID
EFIAPI
I2cHostI2cBusConfigurationAvailable (
  IN EFI_EVENT Event,
  IN VOID *Context
  )
{
  I2C_HOST_CONTEXT            *I2cHostContext;
  EFI_I2C_MASTER_PROTOCOL     *I2cMaster;
  I2C_REQUEST                 *I2cRequest;
  LIST_ENTRY                  *EntryHeader;
  LIST_ENTRY                  *Entry;
  EFI_STATUS                  Status;

  //
  // Mark this I2C bus configuration management operation as complete
  //
  I2cHostContext = (I2C_HOST_CONTEXT *)Context;
  I2cMaster      = I2cHostContext->I2cMaster;
  ASSERT (I2cMaster != NULL);
  //
  // Clear flag to indicate I2C bus configuration is finished
  //
  I2cHostContext->I2cBusConfigurationManagementPending = FALSE;

  //
  //  Validate the completion status
  //
  if (EFI_ERROR (I2cHostContext->Status)) {
    //
    // Setting I2C bus configuration failed before
    //
    I2cHostRequestComplete (I2cHostContext, I2cHostContext->Status);

    //
    // Unknown I2C bus configuration
    // Force next operation to enable the I2C bus configuration
    //
    I2cHostContext->I2cBusConfiguration = (UINTN) -1;
    
    //
    // Do not continue current I2C request
    //
    return;
  }

  //
  // Get the first request in the link with FIFO order
  //
  EntryHeader = &I2cHostContext->RequestList;
  Entry = GetFirstNode (EntryHeader);
  I2cRequest = I2C_REQUEST_FROM_ENTRY (Entry);

  //
  // Update the I2C bus configuration of the current I2C request
  //
  I2cHostContext->I2cBusConfiguration = I2cRequest->I2cBusConfiguration;

  //
  // Start an I2C operation on the host, the status is returned by I2cHostContext->Status
  //
  Status = I2cMaster->StartRequest ( 
                        I2cMaster,
                        I2cRequest->SlaveAddress,
                        I2cRequest->RequestPacket,
                        I2cHostContext->I2cEvent,
                        &I2cHostContext->Status
                        );

  if (EFI_ERROR (Status)) {
    DEBUG((DEBUG_ERROR, "I2cHostI2cBusConfigurationAvailable: Error starting I2C operation, %r\n", Status));
  }
}

/**
  Complete the current request

  This routine is called at TPL_I2C_SYNC.

  @param[in] I2cHostContext  Address of an I2C_HOST_CONTEXT structure.
  @param[in] Status          Status of the I2C operation.

  @return This routine returns the input status value.

**/
EFI_STATUS
I2cHostRequestComplete (
  I2C_HOST_CONTEXT *I2cHostContext,
  EFI_STATUS       Status
  )
{
  I2C_REQUEST *I2cRequest;
  LIST_ENTRY  *EntryHeader;
  LIST_ENTRY  *Entry;

  //
  // Remove the current I2C request from the list
  //
  EntryHeader = &I2cHostContext->RequestList;
  Entry = GetFirstNode (EntryHeader);
  I2cRequest = I2C_REQUEST_FROM_ENTRY (Entry);

  //
  // Save the status for QueueRequest
  //
  if ( NULL != I2cRequest->Status ) {
    *I2cRequest->Status = Status;
  }

  //
  //  Notify the user of the I2C request completion
  //
  if ( NULL != I2cRequest->Event ) {
    gBS->SignalEvent (I2cRequest->Event);
  }

  //
  // Done with this request, remove the current request from list
  //
  RemoveEntryList (&I2cRequest->Link);
  FreePool (I2cRequest->RequestPacket);
  FreePool (I2cRequest);

  //
  // If there is more I2C request, start next one
  //
  if(!IsListEmpty (EntryHeader)) {
    I2cHostRequestEnable (I2cHostContext);
  }
  
  return Status;
}

/**
  Handle the bus available event

  This routine is called at TPL_I2C_SYNC.

  @param[in] Event    Address of an EFI_EVENT handle
  @param[in] Context  Address of an I2C_HOST_CONTEXT structure

**/
VOID
EFIAPI
I2cHostRequestCompleteEvent (
  IN EFI_EVENT Event,
  IN VOID *Context
  )
{
  I2C_HOST_CONTEXT *I2cHostContext;

  //
  // Handle the completion event
  //
  I2cHostContext = (I2C_HOST_CONTEXT *)Context;
  I2cHostRequestComplete (I2cHostContext, I2cHostContext->Status);
}

/**
  Enable access to the I2C bus configuration

  @param[in] I2cHostContext     Address of an I2C_HOST_CONTEXT structure

  @retval EFI_SUCCESS           The operation completed successfully.
  @retval EFI_ABORTED           The request did not complete because the driver
                                was shutdown.
  @retval EFI_BAD_BUFFER_SIZE   The WriteBytes or ReadBytes buffer size is too large.
  @retval EFI_DEVICE_ERROR      There was an I2C error (NACK) during the operation.
                                This could indicate the slave device is not present.
  @retval EFI_INVALID_PARAMETER RequestPacket is NULL
  @retval EFI_NO_MAPPING        Invalid I2cBusConfiguration value
  @retval EFI_NO_RESPONSE       The I2C device is not responding to the
                                slave address.  EFI_DEVICE_ERROR may also be
                                returned if the controller can not distinguish
                                when the NACK occurred.
  @retval EFI_NOT_FOUND         I2C slave address exceeds maximum address
  @retval EFI_NOT_READY         I2C bus is busy or operation pending, wait for
                                the event and then read status.
  @retval EFI_OUT_OF_RESOURCES  Insufficient memory for I2C operation
  @retval EFI_TIMEOUT           The transaction did not complete within an internally
                                specified timeout period.

**/
EFI_STATUS
I2cHostRequestEnable (
  I2C_HOST_CONTEXT *I2cHostContext
  )
{
  UINTN                                                 I2cBusConfiguration;
  CONST EFI_I2C_BUS_CONFIGURATION_MANAGEMENT_PROTOCOL   *I2cBusConfigurationManagement;
  I2C_REQUEST                                           *I2cRequest;
  EFI_STATUS                                            Status;
  EFI_TPL                                               TplPrevious;
  LIST_ENTRY                                            *EntryHeader;
  LIST_ENTRY                                            *Entry;

  //
  //  Assume pending request
  //
  Status = EFI_NOT_READY;

  I2cBusConfigurationManagement = I2cHostContext->I2cBusConfigurationManagement;

  //
  //  Validate the I2c bus configuration
  //
  EntryHeader = &I2cHostContext->RequestList;
  Entry       = GetFirstNode (EntryHeader);
  I2cRequest = I2C_REQUEST_FROM_ENTRY (Entry);

  I2cBusConfiguration = I2cRequest->I2cBusConfiguration;

  if (I2cHostContext->I2cBusConfiguration != I2cBusConfiguration ) {
    //
    // Set flag to indicate I2C bus configuration is in progress
    //
    I2cHostContext->I2cBusConfigurationManagementPending = TRUE;
    //
    //  Update bus configuration for this device's requesting bus configuration
    //
    Status = I2cBusConfigurationManagement->EnableI2cBusConfiguration (
                I2cBusConfigurationManagement,
                I2cBusConfiguration,
                I2cHostContext->I2cBusConfigurationEvent,
                &I2cHostContext->Status
                );
  } else {
    //
    //  I2C bus configuration is same, no need change configuration and start I2c transaction directly
    //
    TplPrevious = gBS->RaiseTPL ( TPL_I2C_SYNC );

    //
    //  Same I2C bus configuration
    //
    I2cHostContext->Status = EFI_SUCCESS;
    I2cHostI2cBusConfigurationAvailable (I2cHostContext->I2cBusConfigurationEvent, I2cHostContext);

    //
    //  Release the thread synchronization
    //
    gBS->RestoreTPL ( TplPrevious );
  }
  return Status;
}

/**
  Queue an I2C operation for execution on the I2C controller.

  This routine must be called at or below TPL_NOTIFY.  For synchronous
  requests this routine must be called at or below TPL_CALLBACK.

  N.B. The typical consumers of this API are the I2C bus driver and
  on rare occasions the I2C test application.  Extreme care must be
  taken by other consumers of this API to prevent confusing the
  third party I2C drivers due to a state change at the I2C device
  which the third party I2C drivers did not initiate.  I2C platform
  drivers may use this API within these guidelines.

  This layer uses the concept of I2C bus configurations to describe
  the I2C bus.  An I2C bus configuration is defined as a unique
  setting of the multiplexers and switches in the I2C bus which
  enable access to one or more I2C devices.  When using a switch
  to divide a bus, due to speed differences, the I2C platform layer
  would define an I2C bus configuration for the I2C devices on each
  side of the switch.  When using a multiplexer, the I2C platform
  layer defines an I2C bus configuration for each of the selector
  values required to control the multiplexer.  See Figure 1 in the
  <a href="http://www.nxp.com/documents/user_manual/UM10204.pdf">I<sup>2</sup>C
  Specification</a> for a complex I2C bus configuration.

  The I2C host driver processes all operations in FIFO order.  Prior to
  performing the operation, the I2C host driver calls the I2C platform
  driver to reconfigure the switches and multiplexers in the I2C bus
  enabling access to the specified I2C device.  The I2C platform driver
  also selects the maximum bus speed for the device.  After the I2C bus
  is configured, the I2C host driver calls the I2C port driver to
  initialize the I2C controller and start the I2C operation.

  @param[in] This             Address of an EFI_I2C_HOST_PROTOCOL instance.
  @param[in] I2cBusConfiguration  I2C bus configuration to access the I2C
                                  device.
  @param[in] SlaveAddress     Address of the device on the I2C bus.
  @param[in] Event            Event to set for asynchronous operations,
                              NULL for synchronous operations
  @param[in] RequestPacket    Address of an EFI_I2C_REQUEST_PACKET
                              structure describing the I2C operation
  @param[out] I2cStatus       Optional buffer to receive the I2C operation
                              completion status

  @retval EFI_SUCCESS           The operation completed successfully.
  @retval EFI_BAD_BUFFER_SIZE   The WriteBytes or ReadBytes buffer size is too large.
  @retval EFI_DEVICE_ERROR      There was an I2C error (NACK) during the operation.
                                This could indicate the slave device is not present.
  @retval EFI_INVALID_PARAMETER RequestPacket is NULL
  @retval EFI_INVALID_PARAMETER TPL is too high
  @retval EFI_NO_MAPPING        Invalid I2cBusConfiguration value
  @retval EFI_NO_RESPONSE       The I2C device is not responding to the
                                slave address.  EFI_DEVICE_ERROR may also be
                                returned if the controller can not distinguish
                                when the NACK occurred.
  @retval EFI_NOT_FOUND         I2C slave address exceeds maximum address
  @retval EFI_NOT_READY         I2C bus is busy or operation pending, wait for
                                the event and then read status pointed to by
                                the request packet.
  @retval EFI_OUT_OF_RESOURCES  Insufficient memory for I2C operation
  @retval EFI_TIMEOUT           The transaction did not complete within an internally
                                specified timeout period.

**/
EFI_STATUS
EFIAPI
I2cHostQueueRequest (
  IN CONST EFI_I2C_HOST_PROTOCOL  *This,
  IN UINTN                        I2cBusConfiguration,
  IN UINTN                        SlaveAddress,
  IN EFI_EVENT                    Event            OPTIONAL,
  IN EFI_I2C_REQUEST_PACKET       *RequestPacket,
  OUT EFI_STATUS                  *I2cStatus       OPTIONAL
  )
{
  EFI_STATUS        Status;
  EFI_EVENT         SyncEvent;
  EFI_TPL           TplPrevious;
  I2C_REQUEST       *I2cRequest;
  I2C_HOST_CONTEXT  *I2cHostContext;
  BOOLEAN           FirstRequest;
  UINTN             RequestPacketSize;
  UINTN             StartBit;

  SyncEvent    = NULL;
  FirstRequest = FALSE;
  Status       = EFI_SUCCESS;

  if (RequestPacket == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  
  if ((SlaveAddress & I2C_ADDRESSING_10_BIT) != 0) {
    //
    // 10-bit address, bits 0-9 are used for 10-bit I2C slave addresses,
    // bits 10-30 are reserved bits and must be zero
    //
    StartBit = 10;
  } else {
    //
    // 7-bit address, Bits 0-6 are used for 7-bit I2C slave addresses,
    // bits 7-30 are reserved bits and must be zero
    //
    StartBit = 7;
  }

  if (BitFieldRead32 ((UINT32)SlaveAddress, StartBit, 30) != 0) {
    //
    // Reserved bit set in the SlaveAddress parameter
    //
    return EFI_NOT_FOUND;
  }

  I2cHostContext = I2C_HOST_CONTEXT_FROM_PROTOCOL (This);

  if (Event == NULL) {
    //
    // For synchronous transaction, register an event used to wait for finishing synchronous transaction
    //
    Status = gBS->CreateEvent ( 
                0,
                TPL_I2C_SYNC,
                NULL,
                NULL,
                &SyncEvent
                );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }
 
  //
  // TPL should be at or below TPL_NOTIFY.
  // For synchronous requests this routine must be called at or below TPL_CALLBACK.
  //
  TplPrevious = EfiGetCurrentTpl ();
  if ((TplPrevious > TPL_I2C_SYNC) || ((Event == NULL) && (TplPrevious > TPL_CALLBACK))) {
    DEBUG ((EFI_D_ERROR, "ERROR - TPL %d is too high!\n", TplPrevious));
    return EFI_INVALID_PARAMETER;
  }

  //
  // Allocate the request structure
  //
  I2cRequest = AllocateZeroPool (sizeof (I2C_REQUEST));
  if (I2cRequest == NULL) {
    DEBUG ((EFI_D_ERROR, "WARNING - Failed to allocate I2C_REQUEST!\n"));
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Initialize the request
  //
  I2cRequest->Signature           = I2C_REQUEST_SIGNATURE;
  I2cRequest->I2cBusConfiguration = I2cBusConfiguration;
  I2cRequest->SlaveAddress        = SlaveAddress;
  I2cRequest->Event               = (Event == NULL) ? SyncEvent : Event;
  I2cRequest->Status              = I2cStatus;

  //
  // Copy request packet into private buffer, as RequestPacket may be freed during asynchronous transaction
  //
  RequestPacketSize = sizeof (UINTN) + RequestPacket->OperationCount * sizeof (EFI_I2C_OPERATION);
  I2cRequest->RequestPacket = AllocateZeroPool (RequestPacketSize);
  ASSERT (I2cRequest->RequestPacket != NULL);
  CopyMem (I2cRequest->RequestPacket, RequestPacket, RequestPacketSize);

  //
  // Synchronize with the other threads
  //
  gBS->RaiseTPL ( TPL_I2C_SYNC );
  
  FirstRequest = IsListEmpty (&I2cHostContext->RequestList);
  
  //
  // Insert new I2C request in the list
  //
  InsertTailList (&I2cHostContext->RequestList, &I2cRequest->Link);

  //
  // Release the thread synchronization
  //
  gBS->RestoreTPL (TplPrevious);
  
  if (FirstRequest) {
    //
    // Start the first I2C request, then the subsequent of I2C request will continue
    //
    Status = I2cHostRequestEnable (I2cHostContext);
  }

  if (Event != NULL) {
    //
    // For asynchronous, return EFI_SUCCESS indicating that the asynchronously I2C transaction was queued.
    // No real I2C operation status in I2cStatus
    //
    return EFI_SUCCESS;
  }

  //
  // For synchronous transaction, wait for the operation completion
  //
  do {
    Status = gBS->CheckEvent (SyncEvent);
  } while (Status == EFI_NOT_READY);

  //
  // Get the I2C operation status
  //
  Status = I2cHostContext->Status;

  //
  // Return the I2C operation status
  //
  if (I2cStatus != NULL) {
    *I2cStatus = Status;
  }

  //
  // Close the event if necessary
  //
  if (SyncEvent != NULL) {
    gBS->CloseEvent (SyncEvent);
  }

  return Status;
}

/**
  The user Entry Point for I2C host module. The user code starts with this function.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
InitializeI2cHost(
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  EFI_STATUS              Status;

  //
  // Install driver model protocol(s).
  //
  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &gI2cHostDriverBinding,
             ImageHandle,
             &gI2cHostComponentName,
             &gI2cHostComponentName2
             );
  ASSERT_EFI_ERROR (Status);
  return Status;
}

/**
  This is the unload handle for I2C host module.

  Disconnect the driver specified by ImageHandle from all the devices in the handle database.
  Uninstall all the protocols installed in the driver entry point.

  @param[in] ImageHandle           The drivers' driver image.

  @retval    EFI_SUCCESS           The image is unloaded.
  @retval    Others                Failed to unload the image.

**/
EFI_STATUS
EFIAPI
I2cHostUnload (
  IN EFI_HANDLE             ImageHandle
  )
{
  EFI_STATUS                        Status;
  EFI_HANDLE                        *DeviceHandleBuffer;
  UINTN                             DeviceHandleCount;
  UINTN                             Index;
  EFI_COMPONENT_NAME_PROTOCOL       *ComponentName;
  EFI_COMPONENT_NAME2_PROTOCOL      *ComponentName2;

  //
  // Get the list of all I2C Controller handles in the handle database.
  // If there is an error getting the list, then the unload
  // operation fails.
  //
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiI2cHostProtocolGuid,
                  NULL,
                  &DeviceHandleCount,
                  &DeviceHandleBuffer
                  );

  if (!EFI_ERROR (Status)) {
    //
    // Disconnect the driver specified by ImageHandle from all
    // the devices in the handle database.
    //
    for (Index = 0; Index < DeviceHandleCount; Index++) {
      Status = gBS->DisconnectController (
                      DeviceHandleBuffer[Index],
                      ImageHandle,
                      NULL
                      );
      if (EFI_ERROR (Status)) {
        goto Done;
      }
    }
  }

  //
  // Uninstall all the protocols installed in the driver entry point
  //
  Status = gBS->UninstallMultipleProtocolInterfaces (
                  gI2cHostDriverBinding.DriverBindingHandle,
                  &gEfiDriverBindingProtocolGuid,
                  &gI2cHostDriverBinding,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  //
  // Note we have to one by one uninstall the following protocols.
  // It's because some of them are optionally installed based on
  // the following PCD settings.
  //   gEfiMdePkgTokenSpaceGuid.PcdDriverDiagnosticsDisable
  //   gEfiMdePkgTokenSpaceGuid.PcdComponentNameDisable
  //   gEfiMdePkgTokenSpaceGuid.PcdDriverDiagnostics2Disable
  //   gEfiMdePkgTokenSpaceGuid.PcdComponentName2Disable
  //
  Status = gBS->HandleProtocol (
                  gI2cHostDriverBinding.DriverBindingHandle,
                  &gEfiComponentNameProtocolGuid,
                  (VOID **) &ComponentName
                  );
  if (!EFI_ERROR (Status)) {
    gBS->UninstallProtocolInterface (
           gI2cHostDriverBinding.DriverBindingHandle,
           &gEfiComponentNameProtocolGuid,
           ComponentName
           );
  }

  Status = gBS->HandleProtocol (
                  gI2cHostDriverBinding.DriverBindingHandle,
                  &gEfiComponentName2ProtocolGuid,
                  (VOID **) &ComponentName2
                  );
  if (!EFI_ERROR (Status)) {
    gBS->UninstallProtocolInterface (
           gI2cHostDriverBinding.DriverBindingHandle,
           &gEfiComponentName2ProtocolGuid,
           ComponentName2
           );
  }

  Status = EFI_SUCCESS;

Done:
  //
  // Free the buffer containing the list of handles from the handle database
  //
  if (DeviceHandleBuffer != NULL) {
    gBS->FreePool (DeviceHandleBuffer);
  }

  return Status;
}
