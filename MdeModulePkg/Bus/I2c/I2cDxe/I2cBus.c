/** @file
  This file implements I2C IO Protocol which enables the user to manipulate a single
  I2C device independent of the host controller and I2C design.

  Copyright (c) 2013 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "I2cDxe.h"

//
//  EFI_DRIVER_BINDING_PROTOCOL instance
//
EFI_DRIVER_BINDING_PROTOCOL  gI2cBusDriverBinding = {
  I2cBusDriverSupported,
  I2cBusDriverStart,
  I2cBusDriverStop,
  0x10,
  NULL,
  NULL
};

//
// Template for I2C Bus Child Device.
//
I2C_DEVICE_CONTEXT  gI2cDeviceContextTemplate = {
  I2C_DEVICE_SIGNATURE,
  NULL,
  {                     // I2cIo Protocol
    I2cBusQueueRequest, // QueueRequest
    NULL,               // DeviceGuid
    0,                  // DeviceIndex
    0,                  // HardwareRevision
    NULL                // I2cControllerCapabilities
  },
  NULL,                 // DevicePath
  NULL,                 // I2cDevice
  NULL,                 // I2cBusContext
};

//
// Template for controller device path node.
//
CONTROLLER_DEVICE_PATH  gControllerDevicePathTemplate = {
  {
    HARDWARE_DEVICE_PATH,
    HW_CONTROLLER_DP,
    {
      (UINT8)(sizeof (CONTROLLER_DEVICE_PATH)),
      (UINT8)((sizeof (CONTROLLER_DEVICE_PATH)) >> 8)
    }
  },
  0
};

//
// Template for vendor device path node.
//
VENDOR_DEVICE_PATH  gVendorDevicePathTemplate = {
  {
    HARDWARE_DEVICE_PATH,
    HW_VENDOR_DP,
    {
      (UINT8)(sizeof (VENDOR_DEVICE_PATH)),
      (UINT8)((sizeof (VENDOR_DEVICE_PATH)) >> 8)
    }
  },
  { 0x0, 0x0, 0x0, { 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 }
  }
};

//
// Driver name table
//
GLOBAL_REMOVE_IF_UNREFERENCED EFI_UNICODE_STRING_TABLE  mI2cBusDriverNameTable[] = {
  { "eng;en", (CHAR16 *)L"I2C Bus Driver" },
  { NULL,     NULL                        }
};

//
// EFI Component Name Protocol
//
GLOBAL_REMOVE_IF_UNREFERENCED EFI_COMPONENT_NAME_PROTOCOL  gI2cBusComponentName = {
  (EFI_COMPONENT_NAME_GET_DRIVER_NAME)I2cBusComponentNameGetDriverName,
  (EFI_COMPONENT_NAME_GET_CONTROLLER_NAME)I2cBusComponentNameGetControllerName,
  "eng"
};

//
// EFI Component Name 2 Protocol
//
GLOBAL_REMOVE_IF_UNREFERENCED EFI_COMPONENT_NAME2_PROTOCOL  gI2cBusComponentName2 = {
  I2cBusComponentNameGetDriverName,
  I2cBusComponentNameGetControllerName,
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
I2cBusComponentNameGetDriverName (
  IN  EFI_COMPONENT_NAME2_PROTOCOL  *This,
  IN  CHAR8                         *Language,
  OUT CHAR16                        **DriverName
  )
{
  return LookupUnicodeString2 (
           Language,
           This->SupportedLanguages,
           mI2cBusDriverNameTable,
           DriverName,
           (BOOLEAN)(This != &gI2cBusComponentName2)
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
I2cBusComponentNameGetControllerName (
  IN  EFI_COMPONENT_NAME2_PROTOCOL  *This,
  IN  EFI_HANDLE                    ControllerHandle,
  IN  EFI_HANDLE                    ChildHandle        OPTIONAL,
  IN  CHAR8                         *Language,
  OUT CHAR16                        **ControllerName
  )
{
  return EFI_UNSUPPORTED;
}

/**
  Check if the child of I2C controller has been created.

  @param[in] This                         A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param[in] Controller                   I2C controller handle.
  @param[in] RemainingDevicePath          A pointer to the remaining portion of a device path.
  @param[in] RemainingHasControllerNode   Indicate if RemainingDevicePath contains CONTROLLER_DEVICE_PATH.
  @param[in] RemainingControllerNumber    Controller number in CONTROLLER_DEVICE_PATH.

  @retval EFI_SUCCESS                     The child of I2C controller is not created.
  @retval Others                          The child of I2C controller has been created or other errors happen.

**/
EFI_STATUS
CheckRemainingDevicePath (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath,
  IN BOOLEAN                      RemainingHasControllerNode,
  IN UINT32                       RemainingControllerNumber
  )
{
  EFI_STATUS                           Status;
  EFI_DEVICE_PATH_PROTOCOL             *SystemDevicePath;
  EFI_OPEN_PROTOCOL_INFORMATION_ENTRY  *OpenInfoBuffer;
  UINTN                                EntryCount;
  UINTN                                Index;
  BOOLEAN                              SystemHasControllerNode;
  UINT32                               SystemControllerNumber;

  SystemHasControllerNode = FALSE;
  SystemControllerNumber  = 0;

  Status = gBS->OpenProtocolInformation (
                  Controller,
                  &gEfiI2cHostProtocolGuid,
                  &OpenInfoBuffer,
                  &EntryCount
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  for (Index = 0; Index < EntryCount; Index++) {
    if ((OpenInfoBuffer[Index].Attributes & EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER) != 0) {
      Status = gBS->OpenProtocol (
                      OpenInfoBuffer[Index].ControllerHandle,
                      &gEfiDevicePathProtocolGuid,
                      (VOID **)&SystemDevicePath,
                      This->DriverBindingHandle,
                      Controller,
                      EFI_OPEN_PROTOCOL_GET_PROTOCOL
                      );
      if (!EFI_ERROR (Status)) {
        //
        // Find vendor device path node and compare
        //
        while (!IsDevicePathEnd (SystemDevicePath)) {
          if ((DevicePathType (SystemDevicePath) == HARDWARE_DEVICE_PATH) &&
              (DevicePathSubType (SystemDevicePath) == HW_VENDOR_DP))
          {
            //
            // Check if vendor device path is same between system device path and remaining device path
            //
            if (CompareMem (SystemDevicePath, RemainingDevicePath, sizeof (VENDOR_DEVICE_PATH)) == 0) {
              //
              // Get controller node appended after vendor node
              //
              SystemDevicePath = NextDevicePathNode (SystemDevicePath);
              if ((DevicePathType (SystemDevicePath) == HARDWARE_DEVICE_PATH) &&
                  (DevicePathSubType (SystemDevicePath) == HW_CONTROLLER_DP))
              {
                SystemHasControllerNode = TRUE;
                SystemControllerNumber  = ((CONTROLLER_DEVICE_PATH *)SystemDevicePath)->ControllerNumber;
              } else {
                SystemHasControllerNode = FALSE;
                SystemControllerNumber  = 0;
              }

              if (((SystemHasControllerNode)  && (!RemainingHasControllerNode) && (SystemControllerNumber == 0)) ||
                  ((!SystemHasControllerNode) && (RemainingHasControllerNode)  && (RemainingControllerNumber == 0)) ||
                  ((SystemHasControllerNode)  && (RemainingHasControllerNode)  && (SystemControllerNumber == RemainingControllerNumber)) ||
                  ((!SystemHasControllerNode) && (!RemainingHasControllerNode)))
              {
                DEBUG ((DEBUG_ERROR, "This I2C device has been already started.\n"));
                Status = EFI_UNSUPPORTED;
                break;
              }
            }
          }

          SystemDevicePath = NextDevicePathNode (SystemDevicePath);
        }

        if (EFI_ERROR (Status)) {
          break;
        }
      }
    }
  }

  FreePool (OpenInfoBuffer);
  return Status;
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
I2cBusDriverSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_STATUS                  Status;
  EFI_I2C_ENUMERATE_PROTOCOL  *I2cEnumerate;
  EFI_I2C_HOST_PROTOCOL       *I2cHost;
  EFI_DEVICE_PATH_PROTOCOL    *ParentDevicePath;
  EFI_DEVICE_PATH_PROTOCOL    *DevPathNode;
  BOOLEAN                     RemainingHasControllerNode;
  UINT32                      RemainingControllerNumber;

  RemainingHasControllerNode = FALSE;
  RemainingControllerNumber  = 0;

  //
  //  Determine if the I2c Enumerate Protocol is available
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiI2cEnumerateProtocolGuid,
                  (VOID **)&I2cEnumerate,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if ((EFI_ERROR (Status)) && (Status != EFI_ALREADY_STARTED)) {
    return Status;
  }

  if (!EFI_ERROR (Status)) {
    gBS->CloseProtocol (
           Controller,
           &gEfiI2cEnumerateProtocolGuid,
           This->DriverBindingHandle,
           Controller
           );
  }

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **)&ParentDevicePath,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );

  if ((EFI_ERROR (Status)) && (Status != EFI_ALREADY_STARTED)) {
    return Status;
  }

  if (!EFI_ERROR (Status)) {
    gBS->CloseProtocol (
           Controller,
           &gEfiDevicePathProtocolGuid,
           This->DriverBindingHandle,
           Controller
           );
  }

  if ((RemainingDevicePath != NULL) && !IsDevicePathEnd (RemainingDevicePath)) {
    //
    // Check if the first node of RemainingDevicePath is a hardware vendor device path
    //
    if ((DevicePathType (RemainingDevicePath) != HARDWARE_DEVICE_PATH) ||
        (DevicePathSubType (RemainingDevicePath) != HW_VENDOR_DP))
    {
      return EFI_UNSUPPORTED;
    }

    //
    // Check if the second node of RemainingDevicePath is a controller node
    //
    DevPathNode = NextDevicePathNode (RemainingDevicePath);
    if (!IsDevicePathEnd (DevPathNode)) {
      if ((DevicePathType (DevPathNode) != HARDWARE_DEVICE_PATH) ||
          (DevicePathSubType (DevPathNode) != HW_CONTROLLER_DP))
      {
        return EFI_UNSUPPORTED;
      } else {
        RemainingHasControllerNode = TRUE;
        RemainingControllerNumber  = ((CONTROLLER_DEVICE_PATH *)DevPathNode)->ControllerNumber;
      }
    }
  }

  //
  // Determine if the I2C Host Protocol is available
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiI2cHostProtocolGuid,
                  (VOID **)&I2cHost,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );

  if (!EFI_ERROR (Status)) {
    gBS->CloseProtocol (
           Controller,
           &gEfiI2cHostProtocolGuid,
           This->DriverBindingHandle,
           Controller
           );
  }

  if (Status == EFI_ALREADY_STARTED) {
    if ((RemainingDevicePath == NULL) ||
        ((RemainingDevicePath != NULL) && IsDevicePathEnd (RemainingDevicePath)))
    {
      //
      // If RemainingDevicePath is NULL or is the End of Device Path Node, return EFI_SUCCESS.
      //
      Status = EFI_SUCCESS;
    } else {
      //
      // Test if the child with the RemainingDevicePath has already been created.
      //
      Status = CheckRemainingDevicePath (
                 This,
                 Controller,
                 RemainingDevicePath,
                 RemainingHasControllerNode,
                 RemainingControllerNumber
                 );
    }
  }

  return Status;
}

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
  @retval Others                   The driver failed to start the device.

**/
EFI_STATUS
EFIAPI
I2cBusDriverStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   Controller,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath
  )
{
  EFI_I2C_ENUMERATE_PROTOCOL  *I2cEnumerate;
  EFI_I2C_HOST_PROTOCOL       *I2cHost;
  I2C_BUS_CONTEXT             *I2cBusContext;
  EFI_STATUS                  Status;
  EFI_DEVICE_PATH_PROTOCOL    *ParentDevicePath;

  I2cBusContext    = NULL;
  ParentDevicePath = NULL;
  I2cEnumerate     = NULL;
  I2cHost          = NULL;

  //
  //  Determine if the I2C controller is available
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiI2cHostProtocolGuid,
                  (VOID **)&I2cHost,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status) && (Status != EFI_ALREADY_STARTED)) {
    DEBUG ((DEBUG_ERROR, "I2cBus: open I2C host error, Status = %r\n", Status));
    return Status;
  }

  if (Status == EFI_ALREADY_STARTED) {
    Status = gBS->OpenProtocol (
                    Controller,
                    &gEfiCallerIdGuid,
                    (VOID **)&I2cBusContext,
                    This->DriverBindingHandle,
                    Controller,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "I2cBus: open private protocol error, Status = %r.\n", Status));
      return Status;
    }
  }

  //
  //  Get the I2C bus enumeration API
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiI2cEnumerateProtocolGuid,
                  (VOID **)&I2cEnumerate,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status) && (Status != EFI_ALREADY_STARTED)) {
    DEBUG ((DEBUG_ERROR, "I2cBus: open I2C enumerate error, Status = %r\n", Status));
    goto Error;
  }

  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **)&ParentDevicePath,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status) && (Status != EFI_ALREADY_STARTED)) {
    DEBUG ((DEBUG_ERROR, "I2cBus: open device path error, Status = %r\n", Status));
    goto Error;
  }

  if ((RemainingDevicePath != NULL) && IsDevicePathEnd (RemainingDevicePath)) {
    //
    // If RemainingDevicePath is the End of Device Path Node,
    // don't create any child device and return EFI_SUCCESS
    //
    return EFI_SUCCESS;
  }

  //
  // Allocate the buffer for I2C_BUS_CONTEXT if it is not allocated before.
  //
  if (I2cBusContext == NULL) {
    //
    //  Allocate the I2C context structure for the current I2C controller
    //
    I2cBusContext = AllocateZeroPool (sizeof (I2C_BUS_CONTEXT));
    if (I2cBusContext == NULL) {
      DEBUG ((DEBUG_ERROR, "I2cBus: there is no enough memory to allocate.\n"));
      Status = EFI_OUT_OF_RESOURCES;
      goto Error;
    }

    /*
       +----------------+
    .->| I2C_BUS_CONTEXT|<----- This file Protocol (gEfiCallerIdGuid) installed on I2C Controller handle
    |  +----------------+
    |
    |  +----------------------------+
    |  | I2C_DEVICE_CONTEXT         |
    `--|                            |
       |                            |
       | I2C IO Protocol Structure  | <----- I2C IO Protocol
       |                            |
       +----------------------------+

    */
    I2cBusContext->I2cHost      = I2cHost;
    I2cBusContext->I2cEnumerate = I2cEnumerate;
    //
    // Parent controller used to create children
    //
    I2cBusContext->Controller = Controller;
    //
    // Parent controller device path used to create children device path
    //
    I2cBusContext->ParentDevicePath = ParentDevicePath;

    I2cBusContext->DriverBindingHandle = This->DriverBindingHandle;

    Status = gBS->InstallMultipleProtocolInterfaces (
                    &Controller,
                    &gEfiCallerIdGuid,
                    I2cBusContext,
                    NULL
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "I2cBus: install private protocol error, Status = %r.\n", Status));
      goto Error;
    }
  }

  //
  //  Start the driver
  //
  Status = RegisterI2cDevice (I2cBusContext, Controller, RemainingDevicePath);

  return Status;

Error:
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "I2cBus: Start() function failed, Status = %r\n", Status));
    if (ParentDevicePath != NULL) {
      gBS->CloseProtocol (
             Controller,
             &gEfiDevicePathProtocolGuid,
             This->DriverBindingHandle,
             Controller
             );
    }

    if (I2cHost != NULL) {
      gBS->CloseProtocol (
             Controller,
             &gEfiI2cHostProtocolGuid,
             This->DriverBindingHandle,
             Controller
             );
    }

    if (I2cEnumerate != NULL) {
      gBS->CloseProtocol (
             Controller,
             &gEfiI2cEnumerateProtocolGuid,
             This->DriverBindingHandle,
             Controller
             );
    }

    if (I2cBusContext != NULL) {
      Status = gBS->UninstallMultipleProtocolInterfaces (
                      Controller,
                      gEfiCallerIdGuid,
                      I2cBusContext,
                      NULL
                      );
      FreePool (I2cBusContext);
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
I2cBusDriverStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Controller,
  IN  UINTN                        NumberOfChildren,
  IN  EFI_HANDLE                   *ChildHandleBuffer
  )
{
  I2C_BUS_CONTEXT  *I2cBusContext;
  EFI_STATUS       Status;
  BOOLEAN          AllChildrenStopped;
  UINTN            Index;

  if (NumberOfChildren == 0) {
    gBS->CloseProtocol (
           Controller,
           &gEfiDevicePathProtocolGuid,
           This->DriverBindingHandle,
           Controller
           );

    gBS->CloseProtocol (
           Controller,
           &gEfiI2cHostProtocolGuid,
           This->DriverBindingHandle,
           Controller
           );

    gBS->CloseProtocol (
           Controller,
           &gEfiI2cEnumerateProtocolGuid,
           This->DriverBindingHandle,
           Controller
           );

    Status = gBS->OpenProtocol (
                    Controller,
                    &gEfiCallerIdGuid,
                    (VOID **)&I2cBusContext,
                    This->DriverBindingHandle,
                    Controller,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (!EFI_ERROR (Status)) {
      gBS->UninstallMultipleProtocolInterfaces (
             Controller,
             &gEfiCallerIdGuid,
             I2cBusContext,
             NULL
             );
      //
      // No more child now, free bus context data.
      //
      FreePool (I2cBusContext);
    }

    return Status;
  }

  AllChildrenStopped = TRUE;

  for (Index = 0; Index < NumberOfChildren; Index++) {
    Status = UnRegisterI2cDevice (This, Controller, ChildHandleBuffer[Index]);
    if (EFI_ERROR (Status)) {
      AllChildrenStopped = FALSE;
    }
  }

  if (!AllChildrenStopped) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

/**
  Enumerate the I2C bus

  This routine walks the platform specific data describing the
  I2C bus to create the I2C devices where driver GUIDs were
  specified.

  @param[in] I2cBusContext            Address of an I2C_BUS_CONTEXT structure
  @param[in] Controller               Handle to the controller
  @param[in] RemainingDevicePath      A pointer to the remaining portion of a device path.

  @retval EFI_SUCCESS       The bus is successfully configured

**/
EFI_STATUS
RegisterI2cDevice (
  IN I2C_BUS_CONTEXT           *I2cBusContext,
  IN EFI_HANDLE                Controller,
  IN EFI_DEVICE_PATH_PROTOCOL  *RemainingDevicePath
  )
{
  I2C_DEVICE_CONTEXT        *I2cDeviceContext;
  EFI_STATUS                Status;
  CONST EFI_I2C_DEVICE      *Device;
  CONST EFI_I2C_DEVICE      *TempDevice;
  UINT32                    RemainingPathDeviceIndex;
  EFI_DEVICE_PATH_PROTOCOL  *DevPathNode;
  BOOLEAN                   BuildControllerNode;
  UINTN                     Count;

  Status              = EFI_SUCCESS;
  BuildControllerNode = TRUE;

  //
  // Default DeviceIndex
  //
  RemainingPathDeviceIndex = 0;

  //
  // Determine the controller number in Controller Node Device Path when RemainingDevicePath is not NULL.
  //
  if (RemainingDevicePath != NULL) {
    //
    // Check if there is a controller node appended after vendor node
    //
    DevPathNode = NextDevicePathNode (RemainingDevicePath);
    if ((DevicePathType (DevPathNode) == HARDWARE_DEVICE_PATH) &&
        (DevicePathSubType (DevPathNode) == HW_CONTROLLER_DP))
    {
      //
      // RemainingDevicePath != NULL and RemainingDevicePath contains Controller Node,
      // add Controller Node to Device Path on child handle.
      //
      RemainingPathDeviceIndex = ((CONTROLLER_DEVICE_PATH *)DevPathNode)->ControllerNumber;
    } else {
      //
      // RemainingDevicePath != NULL and RemainingDevicePath does not contain Controller Node,
      // do not add controller node to Device Path on child handle.
      //
      BuildControllerNode = FALSE;
    }
  }

  //
  //  Walk the list of I2C devices on this bus
  //
  Device = NULL;
  while (TRUE) {
    //
    //  Get the next I2C device
    //
    Status = I2cBusContext->I2cEnumerate->Enumerate (I2cBusContext->I2cEnumerate, &Device);
    if (EFI_ERROR (Status) || (Device == NULL)) {
      if (RemainingDevicePath != NULL) {
        Status = EFI_NOT_FOUND;
      } else {
        Status = EFI_SUCCESS;
      }

      break;
    }

    //
    //  Determine if the device info is valid
    //
    if ((Device->DeviceGuid == NULL) || (Device->SlaveAddressCount == 0) || (Device->SlaveAddressArray == NULL)) {
      DEBUG ((DEBUG_ERROR, "Invalid EFI_I2C_DEVICE reported by I2c Enumerate protocol.\n"));
      continue;
    }

    if (RemainingDevicePath == NULL) {
      if (Device->DeviceIndex == 0) {
        //
        // Determine if the controller node is necessary when controller number is zero in I2C device
        //
        TempDevice = NULL;
        Count      = 0;
        while (TRUE) {
          //
          //  Get the next I2C device
          //
          Status = I2cBusContext->I2cEnumerate->Enumerate (I2cBusContext->I2cEnumerate, &TempDevice);
          if (EFI_ERROR (Status) || (TempDevice == NULL)) {
            Status = EFI_SUCCESS;
            break;
          }

          if (CompareGuid (Device->DeviceGuid, TempDevice->DeviceGuid)) {
            Count++;
          }
        }

        if (Count == 1) {
          //
          // RemainingDevicePath == NULL and only DeviceIndex 0 is present on the I2C bus,
          // do not add Controller Node to Device Path on child handle.
          //
          BuildControllerNode = FALSE;
        }
      }
    } else {
      //
      // Find I2C device reported in Remaining Device Path
      //
      if ((!CompareGuid (&((VENDOR_DEVICE_PATH *)RemainingDevicePath)->Guid, Device->DeviceGuid)) ||
          (RemainingPathDeviceIndex != Device->DeviceIndex))
      {
        continue;
      }
    }

    //
    // Build the device context for current I2C device.
    //
    I2cDeviceContext = NULL;
    I2cDeviceContext = AllocateCopyPool (sizeof (I2C_DEVICE_CONTEXT), &gI2cDeviceContextTemplate);
    ASSERT (I2cDeviceContext != NULL);
    if (I2cDeviceContext == NULL) {
      continue;
    }

    //
    //  Initialize the specific device context
    //
    I2cDeviceContext->I2cBusContext                   = I2cBusContext;
    I2cDeviceContext->I2cDevice                       = Device;
    I2cDeviceContext->I2cIo.DeviceGuid                = Device->DeviceGuid;
    I2cDeviceContext->I2cIo.DeviceIndex               = Device->DeviceIndex;
    I2cDeviceContext->I2cIo.HardwareRevision          = Device->HardwareRevision;
    I2cDeviceContext->I2cIo.I2cControllerCapabilities = I2cBusContext->I2cHost->I2cControllerCapabilities;

    //
    //  Build the device path
    //
    Status = I2cBusDevicePathAppend (I2cDeviceContext, BuildControllerNode);
    ASSERT_EFI_ERROR (Status);
    if (EFI_ERROR (Status)) {
      continue;
    }

    //
    //  Install the protocol
    //
    Status = gBS->InstallMultipleProtocolInterfaces (
                    &I2cDeviceContext->Handle,
                    &gEfiI2cIoProtocolGuid,
                    &I2cDeviceContext->I2cIo,
                    &gEfiDevicePathProtocolGuid,
                    I2cDeviceContext->DevicePath,
                    NULL
                    );
    if (EFI_ERROR (Status)) {
      //
      // Free resources for this I2C device
      //
      ReleaseI2cDeviceContext (I2cDeviceContext);
      continue;
    }

    //
    // Create the child handle
    //
    Status = gBS->OpenProtocol (
                    Controller,
                    &gEfiI2cHostProtocolGuid,
                    (VOID **)&I2cBusContext->I2cHost,
                    I2cBusContext->DriverBindingHandle,
                    I2cDeviceContext->Handle,
                    EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                    );
    if (EFI_ERROR (Status)) {
      Status = gBS->UninstallMultipleProtocolInterfaces (
                      I2cDeviceContext->Handle,
                      &gEfiDevicePathProtocolGuid,
                      I2cDeviceContext->DevicePath,
                      &gEfiI2cIoProtocolGuid,
                      &I2cDeviceContext->I2cIo,
                      NULL
                      );
      //
      // Free resources for this I2C device
      //
      ReleaseI2cDeviceContext (I2cDeviceContext);
      continue;
    }

    if (RemainingDevicePath != NULL) {
      //
      // Child has been created successfully
      //
      break;
    }
  }

  return Status;
}

/**
  Queue an I2C transaction for execution on the I2C device.

  This routine must be called at or below TPL_NOTIFY.  For synchronous
  requests this routine must be called at or below TPL_CALLBACK.

  This routine queues an I2C transaction to the I2C controller for
  execution on the I2C bus.

  When Event is NULL, QueueRequest() operates synchronously and returns
  the I2C completion status as its return value.

  When Event is not NULL, QueueRequest() synchronously returns EFI_SUCCESS
  indicating that the asynchronous I2C transaction was queued.  The values
  above are returned in the buffer pointed to by I2cStatus upon the
  completion of the I2C transaction when I2cStatus is not NULL.

  The upper layer driver writer provides the following to the platform
  vendor:

  1.  Vendor specific GUID for the I2C part
  2.  Guidance on proper construction of the slave address array when the
      I2C device uses more than one slave address.  The I2C bus protocol
      uses the SlaveAddressIndex to perform relative to physical address
      translation to access the blocks of hardware within the I2C device.

  @param[in] This               Pointer to an EFI_I2C_IO_PROTOCOL structure.
  @param[in] SlaveAddressIndex  Index value into an array of slave addresses
                                for the I2C device.  The values in the array
                                are specified by the board designer, with the
                                third party I2C device driver writer providing
                                the slave address order.

                                For devices that have a single slave address,
                                this value must be zero.  If the I2C device
                                uses more than one slave address then the
                                third party (upper level) I2C driver writer
                                needs to specify the order of entries in the
                                slave address array.

                                \ref ThirdPartyI2cDrivers "Third Party I2C
                                Drivers" section in I2cMaster.h.
  @param[in] Event              Event to signal for asynchronous transactions,
                                NULL for synchronous transactions
  @param[in] RequestPacket      Pointer to an EFI_I2C_REQUEST_PACKET structure
                                describing the I2C transaction
  @param[out] I2cStatus         Optional buffer to receive the I2C transaction
                                completion status

  @retval EFI_SUCCESS           The asynchronous transaction was successfully
                                queued when Event is not NULL.
  @retval EFI_SUCCESS           The transaction completed successfully when
                                Event is NULL.
  @retval EFI_BAD_BUFFER_SIZE   The RequestPacket->LengthInBytes value is too
                                large.
  @retval EFI_DEVICE_ERROR      There was an I2C error (NACK) during the
                                transaction.
  @retval EFI_INVALID_PARAMETER RequestPacket is NULL
  @retval EFI_NO_MAPPING        The EFI_I2C_HOST_PROTOCOL could not set the
                                bus configuration required to access this I2C
                                device.
  @retval EFI_NO_RESPONSE       The I2C device is not responding to the slave
                                address selected by SlaveAddressIndex.
                                EFI_DEVICE_ERROR will be returned if the
                                controller cannot distinguish when the NACK
                                occurred.
  @retval EFI_OUT_OF_RESOURCES  Insufficient memory for I2C transaction
  @retval EFI_UNSUPPORTED       The controller does not support the requested
                                transaction.

**/
EFI_STATUS
EFIAPI
I2cBusQueueRequest (
  IN CONST EFI_I2C_IO_PROTOCOL  *This,
  IN UINTN                      SlaveAddressIndex,
  IN EFI_EVENT                  Event               OPTIONAL,
  IN EFI_I2C_REQUEST_PACKET     *RequestPacket,
  OUT EFI_STATUS                *I2cStatus          OPTIONAL
  )
{
  CONST EFI_I2C_DEVICE         *I2cDevice;
  I2C_BUS_CONTEXT              *I2cBusContext;
  CONST EFI_I2C_HOST_PROTOCOL  *I2cHost;
  I2C_DEVICE_CONTEXT           *I2cDeviceContext;
  EFI_STATUS                   Status;

  if (RequestPacket == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  //  Validate the I2C slave index
  //
  I2cDeviceContext = I2C_DEVICE_CONTEXT_FROM_PROTOCOL (This);
  I2cDevice        = I2cDeviceContext->I2cDevice;
  if ( SlaveAddressIndex >= I2cDevice->SlaveAddressCount ) {
    return EFI_INVALID_PARAMETER;
  }

  //
  //  Locate the I2c Host Protocol to queue request
  //
  I2cBusContext = I2cDeviceContext->I2cBusContext;
  I2cHost       = I2cBusContext->I2cHost;

  //
  //  Start the I2C operation
  //
  Status = I2cHost->QueueRequest (
                      I2cHost,
                      I2cDevice->I2cBusConfiguration,
                      I2cDevice->SlaveAddressArray[SlaveAddressIndex],
                      Event,
                      RequestPacket,
                      I2cStatus
                      );

  return Status;
}

/**
  Release all the resources allocated for the I2C device.

  This function releases all the resources allocated for the I2C device.

  @param  I2cDeviceContext         The I2C child device involved for the operation.

**/
VOID
ReleaseI2cDeviceContext (
  IN I2C_DEVICE_CONTEXT  *I2cDeviceContext
  )
{
  if (I2cDeviceContext == NULL) {
    return;
  }

  if (I2cDeviceContext->DevicePath != NULL) {
    FreePool (I2cDeviceContext->DevicePath);
  }

  FreePool (I2cDeviceContext);
}

/**
  Unregister an I2C device.

  This function removes the protocols installed on the controller handle and
  frees the resources allocated for the I2C device.

  @param  This                  The pointer to EFI_DRIVER_BINDING_PROTOCOL instance.
  @param  Controller            The controller handle of the I2C device.
  @param  Handle                The child handle.

  @retval EFI_SUCCESS           The I2C device is successfully unregistered.
  @return Others                Some error occurs when unregistering the I2C device.

**/
EFI_STATUS
UnRegisterI2cDevice (
  IN  EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN  EFI_HANDLE                   Controller,
  IN  EFI_HANDLE                   Handle
  )
{
  EFI_STATUS             Status;
  I2C_DEVICE_CONTEXT     *I2cDeviceContext;
  EFI_I2C_IO_PROTOCOL    *I2cIo;
  EFI_I2C_HOST_PROTOCOL  *I2cHost;

  I2cIo = NULL;

  Status = gBS->OpenProtocol (
                  Handle,
                  &gEfiI2cIoProtocolGuid,
                  (VOID **)&I2cIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Get I2c device context data.
  //
  I2cDeviceContext = I2C_DEVICE_CONTEXT_FROM_PROTOCOL (I2cIo);

  //
  // Close the child handle
  //
  gBS->CloseProtocol (
         Controller,
         &gEfiI2cHostProtocolGuid,
         This->DriverBindingHandle,
         Handle
         );

  //
  // The I2C Bus driver installs the I2C Io and Device Path Protocol in the DriverBindingStart().
  // Here should uninstall them.
  //
  Status = gBS->UninstallMultipleProtocolInterfaces (
                  Handle,
                  &gEfiDevicePathProtocolGuid,
                  I2cDeviceContext->DevicePath,
                  &gEfiI2cIoProtocolGuid,
                  &I2cDeviceContext->I2cIo,
                  NULL
                  );

  if (EFI_ERROR (Status)) {
    //
    // Keep parent and child relationship
    //
    gBS->OpenProtocol (
           Controller,
           &gEfiI2cHostProtocolGuid,
           (VOID **)&I2cHost,
           This->DriverBindingHandle,
           Handle,
           EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
           );
    return Status;
  }

  //
  // Free resources for this I2C device
  //
  ReleaseI2cDeviceContext (I2cDeviceContext);

  return EFI_SUCCESS;
}

/**
  Create a path for the I2C device

  Append the I2C slave path to the I2C master controller path.

  @param[in] I2cDeviceContext           Address of an I2C_DEVICE_CONTEXT structure.
  @param[in] BuildControllerNode        Flag to build controller node in device path.

  @retval EFI_SUCCESS           The I2C device path is built successfully.
  @return Others                It is failed to built device path.

**/
EFI_STATUS
I2cBusDevicePathAppend (
  IN I2C_DEVICE_CONTEXT  *I2cDeviceContext,
  IN BOOLEAN             BuildControllerNode
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *PreviousDevicePath;

  PreviousDevicePath = NULL;

  //
  // Build vendor device path
  //
  CopyMem (&gVendorDevicePathTemplate.Guid, I2cDeviceContext->I2cDevice->DeviceGuid, sizeof (EFI_GUID));
  I2cDeviceContext->DevicePath = AppendDevicePathNode (
                                   I2cDeviceContext->I2cBusContext->ParentDevicePath,
                                   (EFI_DEVICE_PATH_PROTOCOL *)&gVendorDevicePathTemplate
                                   );
  ASSERT (I2cDeviceContext->DevicePath != NULL);
  if (I2cDeviceContext->DevicePath == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  if ((BuildControllerNode) && (I2cDeviceContext->DevicePath != NULL)) {
    //
    // Build the final I2C device path with controller node
    //
    PreviousDevicePath                             = I2cDeviceContext->DevicePath;
    gControllerDevicePathTemplate.ControllerNumber = I2cDeviceContext->I2cDevice->DeviceIndex;
    I2cDeviceContext->DevicePath                   = AppendDevicePathNode (
                                                       I2cDeviceContext->DevicePath,
                                                       (EFI_DEVICE_PATH_PROTOCOL *)&gControllerDevicePathTemplate
                                                       );
    gBS->FreePool (PreviousDevicePath);
    ASSERT (I2cDeviceContext->DevicePath != NULL);
    if (I2cDeviceContext->DevicePath == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
  }

  return EFI_SUCCESS;
}

/**
  The user entry point for the I2C bus module. The user code starts with
  this function.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
InitializeI2cBus (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  //
  // Install driver model protocol(s).
  //
  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &gI2cBusDriverBinding,
             NULL,
             &gI2cBusComponentName,
             &gI2cBusComponentName2
             );
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/**
  This is the unload handle for I2C bus module.

  Disconnect the driver specified by ImageHandle from all the devices in the handle database.
  Uninstall all the protocols installed in the driver entry point.

  @param[in] ImageHandle           The drivers' driver image.

  @retval    EFI_SUCCESS           The image is unloaded.
  @retval    Others                Failed to unload the image.

**/
EFI_STATUS
EFIAPI
I2cBusUnload (
  IN EFI_HANDLE  ImageHandle
  )
{
  EFI_STATUS                    Status;
  EFI_HANDLE                    *DeviceHandleBuffer;
  UINTN                         DeviceHandleCount;
  UINTN                         Index;
  EFI_COMPONENT_NAME_PROTOCOL   *ComponentName;
  EFI_COMPONENT_NAME2_PROTOCOL  *ComponentName2;

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
    // Disconnect the driver specified by Driver BindingHandle from all
    // the devices in the handle database.
    //
    for (Index = 0; Index < DeviceHandleCount; Index++) {
      Status = gBS->DisconnectController (
                      DeviceHandleBuffer[Index],
                      gI2cBusDriverBinding.DriverBindingHandle,
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
                  gI2cBusDriverBinding.DriverBindingHandle,
                  &gEfiDriverBindingProtocolGuid,
                  &gI2cBusDriverBinding,
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
                  gI2cBusDriverBinding.DriverBindingHandle,
                  &gEfiComponentNameProtocolGuid,
                  (VOID **)&ComponentName
                  );
  if (!EFI_ERROR (Status)) {
    gBS->UninstallProtocolInterface (
           gI2cBusDriverBinding.DriverBindingHandle,
           &gEfiComponentNameProtocolGuid,
           ComponentName
           );
  }

  Status = gBS->HandleProtocol (
                  gI2cBusDriverBinding.DriverBindingHandle,
                  &gEfiComponentName2ProtocolGuid,
                  (VOID **)&ComponentName2
                  );
  if (!EFI_ERROR (Status)) {
    gBS->UninstallProtocolInterface (
           gI2cBusDriverBinding.DriverBindingHandle,
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
