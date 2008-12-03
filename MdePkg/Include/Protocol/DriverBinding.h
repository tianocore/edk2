/** @file
  UEFI DriverBinding Protocol is defined in UEFI specification.
  
  This protocol is produced by every driver that follows the UEFI Driver Model, 
  and it is the central component that allows drivers and controllers to be managed.

  Copyright (c) 2006 - 2008, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef __EFI_DRIVER_BINDING_H__
#define __EFI_DRIVER_BINDING_H__

#include <Protocol/DevicePath.h>
///
/// Global ID for the ControllerHandle Driver Protocol
///
#define EFI_DRIVER_BINDING_PROTOCOL_GUID \
  { \
    0x18a031ab, 0xb443, 0x4d1a, {0xa5, 0xc0, 0xc, 0x9, 0x26, 0x1e, 0x9f, 0x71 } \
  }

typedef struct _EFI_DRIVER_BINDING_PROTOCOL  EFI_DRIVER_BINDING_PROTOCOL;

/**
  Tests to see if this driver supports a given controller. If a child device is provided, 
  it further tests to see if this driver supports creating a handle for the specified child device.

  @param  This                 A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param  ControllerHandle     The handle of the controller to test. This handle 
                               must support a protocol interface that supplies 
                               an I/O abstraction to the driver.
  @param  RemainingDevicePath  A pointer to the remaining portion of a device path. 
                               This parameter is ignored by device drivers, and is optional for bus drivers.


  @retval EFI_SUCCESS         The device specified by ControllerHandle and
                              RemainingDevicePath is supported by the driver specified by This.
  @retval EFI_ALREADY_STARTED The device specified by ControllerHandle and
                              RemainingDevicePath is already being managed by the driver
                              specified by This.
  @retval EFI_ACCESS_DENIED   The device specified by ControllerHandle and
                              RemainingDevicePath is already being managed by a different
                              driver or an application that requires exclusive acces.
  @retval EFI_UNSUPPORTED     The device specified by ControllerHandle and
                              RemainingDevicePath is not supported by the driver specified by This.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_DRIVER_BINDING_SUPPORTED)(
  IN EFI_DRIVER_BINDING_PROTOCOL            *This,
  IN EFI_HANDLE                             ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL               *RemainingDevicePath OPTIONAL
  );

/**
  Start this driver on ControllerHandle. The Start() function is designed to be 
  invoked from the EFI boot service ConnectController(). As a result, much of 
  the error checking on the parameters to Start() has been moved into this 
  common boot service. It is legal to call Start() from other locations, 
  but the following calling restrictions must be followed or the system behavior will not be deterministic.
  1. ControllerHandle must be a valid EFI_HANDLE.
  2. If RemainingDevicePath is not NULL, then it must be a pointer to a naturally aligned
     EFI_DEVICE_PATH_PROTOCOL.
  3. Prior to calling Start(), the Supported() function for the driver specified by This must
     have been called with the same calling parameters, and Supported() must have returned EFI_SUCCESS.  

  @param  This                 A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param  ControllerHandle     The handle of the controller to start. This handle 
                               must support a protocol interface that supplies 
                               an I/O abstraction to the driver.
  @param  RemainingDevicePath  A pointer to the remaining portion of a device path. 
                               This parameter is ignored by device drivers, and is optional for bus drivers.

  @retval EFI_SUCCESS          The device was started.
  @retval EFI_DEVICE_ERROR     The device could not be started due to a device error.
  @retval EFI_OUT_OF_RESOURCES The request could not be completed due to a lack of resources.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_DRIVER_BINDING_START)(
  IN EFI_DRIVER_BINDING_PROTOCOL            *This,
  IN EFI_HANDLE                             ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL               *RemainingDevicePath OPTIONAL
  );

/**
  Stop this driver on ControllerHandle. 
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
  
  @param  This              A pointer to the EFI_DRIVER_BINDING_PROTOCOL instance.
  @param  ControllerHandle  A handle to the device being stopped. The handle must 
                            support a bus specific I/O protocol for the driver 
                            to use to stop the device.
  @param  NumberOfChildren  The number of child device handles in ChildHandleBuffer.
  @param  ChildHandleBuffer An array of child handles to be freed. May be NULL if NumberOfChildren is 0.

  @retval EFI_SUCCESS       The device was stopped.
  @retval EFI_DEVICE_ERROR  The device could not be stopped due to a device error.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_DRIVER_BINDING_STOP)(
  IN EFI_DRIVER_BINDING_PROTOCOL            *This,
  IN  EFI_HANDLE                            ControllerHandle,
  IN  UINTN                                 NumberOfChildren,
  IN  EFI_HANDLE                            *ChildHandleBuffer OPTIONAL
  );

///
/// This protocol provides the services required to determine if a driver supports a given controller. 
/// If a controller is supported, then it also provides routines to start and stop the controller.
///
struct _EFI_DRIVER_BINDING_PROTOCOL {
  EFI_DRIVER_BINDING_SUPPORTED  Supported;
  EFI_DRIVER_BINDING_START      Start;
  EFI_DRIVER_BINDING_STOP       Stop;
  
  ///
  /// The version number of the UEFI driver that produced the
  /// EFI_DRIVER_BINDING_PROTOCOL. This field is used by
  /// the EFI boot service ConnectController() to determine
  /// the order that driver's Supported() service will be used when
  /// a controller needs to be started. EFI Driver Binding Protocol
  /// instances with higher Version values will be used before ones
  /// with lower Version values. The Version values of 0x0-
  /// 0x0f and 0xfffffff0-0xffffffff are reserved for
  /// platform/OEM specific drivers. The Version values of 0x10-
  /// 0xffffffef are reserved for IHV-developed drivers.
  ///
  UINT32                        Version;
  
  ///
  /// The image handle of the UEFI driver that produced this instance
  /// of the EFI_DRIVER_BINDING_PROTOCOL.
  ///
  EFI_HANDLE                    ImageHandle;
  
  ///
  /// The handle on which this instance of the
  /// EFI_DRIVER_BINDING_PROTOCOL is installed. In most
  /// cases, this is the same handle as ImageHandle. However, for
  /// UEFI drivers that produce more than one instance of the
  /// EFI_DRIVER_BINDING_PROTOCOL, this value may not be
  /// the same as ImageHandle.  
  ///
  EFI_HANDLE                    DriverBindingHandle;
};

extern EFI_GUID gEfiDriverBindingProtocolGuid;

#endif
