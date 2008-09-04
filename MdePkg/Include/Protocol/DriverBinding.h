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

#include <PiDxe.h>
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
  Test to see if this driver supports ControllerHandle. 

  @param  This                Protocol instance pointer.
  @param  ControllerHandle    Handle of device to test
  @param  RemainingDevicePath Optional parameter use to pick a specific child
                              device to start.

  @retval EFI_SUCCESS         This driver supports this device
  @retval EFI_ALREADY_STARTED This driver is already running on this device
  @retval other               This driver does not support this device

**/
typedef
EFI_STATUS
(EFIAPI *EFI_DRIVER_BINDING_SUPPORTED)(
  IN EFI_DRIVER_BINDING_PROTOCOL            *This,
  IN EFI_HANDLE                             ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL               *RemainingDevicePath OPTIONAL
  );

/**
  Start this driver on ControllerHandle.

  @param  This                 Protocol instance pointer.
  @param  ControllerHandle     Handle of device to bind driver to
  @param  RemainingDevicePath  Optional parameter use to pick a specific child
                               device to start.

  @retval EFI_SUCCESS          This driver is added to ControllerHandle
  @retval EFI_ALREADY_STARTED  This driver is already running on ControllerHandle
  @retval other                This driver does not support this device

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

  @param  This              Protocol instance pointer.
  @param  ControllerHandle  Handle of device to stop driver on
  @param  NumberOfChildren  Number of Handles in ChildHandleBuffer. If number of
                            children is zero stop the entire bus driver.
  @param  ChildHandleBuffer List of Child Handles to Stop.

  @retval EFI_SUCCESS       This driver is removed ControllerHandle
  @retval other             This driver was not removed from this device

**/
typedef
EFI_STATUS
(EFIAPI *EFI_DRIVER_BINDING_STOP)(
  IN EFI_DRIVER_BINDING_PROTOCOL            *This,
  IN  EFI_HANDLE                            ControllerHandle,
  IN  UINTN                                 NumberOfChildren,
  IN  EFI_HANDLE                            *ChildHandleBuffer OPTIONAL
  );

//
// Interface structure for the ControllerHandle Driver Protocol
//
/**
  @par Protocol Description:
  This protocol provides the services required to determine if a driver supports a given controller. 
  If a controller is supported, then it also provides routines to start and stop the controller.
  
  @param Supported 
  Tests to see if this driver supports a given controller. This service
  is called by the EFI boot service ConnectController(). In
  order to make drivers as small as possible, there are a few calling
  restrictions for this service. ConnectController() must
  follow these calling restrictions. If any other agent wishes to call
  Supported() it must also follow these calling restrictions.


  @param Start 
  Starts a controller using this driver. This service is called by the
  EFI boot service ConnectController(). In order to make
  drivers as small as possible, there are a few calling restrictions for
  this service. ConnectController() must follow these
  calling restrictions. If any other agent wishes to call Start() it
  must also follow these calling restrictions. 
  
  @param Stop 
  Stops a controller using this driver. This service is called by the
  EFI boot service DisconnectController(). In order to
  make drivers as small as possible, there are a few calling
  restrictions for this service. DisconnectController()
  must follow these calling restrictions. If any other agent wishes
  to call Stop() it must also follow these calling restrictions.
  
  @param Version 
  The version number of the UEFI driver that produced the
  EFI_DRIVER_BINDING_PROTOCOL. This field is used by
  the EFI boot service ConnectController() to determine
  the order that driver's Supported() service will be used when
  a controller needs to be started. EFI Driver Binding Protocol
  instances with higher Version values will be used before ones
  with lower Version values. The Version values of 0x0-
  0x0f and 0xfffffff0-0xffffffff are reserved for
  platform/OEM specific drivers. The Version values of 0x10-
  0xffffffef are reserved for IHV-developed drivers.
  
  @param ImageHandle 
  The image handle of the UEFI driver that produced this instance
  of the EFI_DRIVER_BINDING_PROTOCOL.
  
  @param DriverBindingHandle
  The handle on which this instance of the
  EFI_DRIVER_BINDING_PROTOCOL is installed. In most
  cases, this is the same handle as ImageHandle. However, for
  UEFI drivers that produce more than one instance of the
  EFI_DRIVER_BINDING_PROTOCOL, this value may not be
  the same as ImageHandle.

**/
struct _EFI_DRIVER_BINDING_PROTOCOL {
  EFI_DRIVER_BINDING_SUPPORTED  Supported;
  EFI_DRIVER_BINDING_START      Start;
  EFI_DRIVER_BINDING_STOP       Stop;
  UINT32                        Version;
  EFI_HANDLE                    ImageHandle;
  EFI_HANDLE                    DriverBindingHandle;
};

extern EFI_GUID gEfiDriverBindingProtocolGuid;

#endif
