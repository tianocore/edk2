/** @file
  Provides services to notify PCI bus driver that some events have happened in a hot-plug controller
  (for example, PC Card socket, or PHPC), and ask PCI bus driver to create or destroy handles for the
  PCI-like devices.

Copyright (c) 2006 - 2009, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef __PCI_HOTPLUG_REQUEST_H_
#define __PCI_HOTPLUG_REQUEST_H_

#define EFI_PCI_HOTPLUG_REQUEST_PROTOCOL_GUID \
  {0x19cb87ab,0x2cb9,{0x4665,0x83,0x60,0xdd,0xcf,0x60,0x54,0xf7,0x9d}}

typedef enum {
  ///
  /// The PCI bus driver is requested to create handles for the specified devices. An array of
  /// EFI_HANDLE is returned, a NULL element marks the end of the array.
  ///
  EfiPciHotPlugRequestAdd,

  ///
  /// The PCI bus driver is requested to destroy handles for the specified devices.
  ///
  EfiPciHotplugRequestRemove
} EFI_PCI_HOTPLUG_OPERATION;

typedef struct _EFI_PCI_HOTPLUG_REQUEST_PROTOCOL  EFI_PCI_HOTPLUG_REQUEST_PROTOCOL;

/**
  This function allows the PCI bus driver to be notified to act as requested when a hot-plug event has
  happened on the hot-plug controller. Currently, the operations include add operation and remove operation..
  
  @param This                 A pointer to the hot plug request protocol.
  @param Operation            The operation the PCI bus driver is requested to make.
  @param Controller           The handle of the hot-plug controller.
  @param RemainingDevicePath  The remaining device path for the PCI-like hot-plug device.
  @param NumberOfChildren     The number of child handles. 
                              For a add operation, it is an output parameter. 
                              For a remove operation, it¡¯s an input parameter.
  @param ChildHandleBuffer    The buffer which contains the child handles.
  
  @retval EFI_INVALID_PARAMETER  Operation is not a legal value.
                                 Controller is NULL or not a valid handle.
                                 NumberOfChildren is NULL.
                                 ChildHandleBuffer is NULL while Operation is add.
  @retval EFI_OUT_OF_RESOURCES   There are no enough resources to start the devices.
  @retval EFI_NOT_FOUND          Can not find bridge according to controller handle.
  @retval EFI_SUCCESS            The handles for the specified device have been created or destroyed
                                 as requested, and for an add operation, the new handles are
                                 returned in ChildHandleBuffer.
**/
typedef
EFI_STATUS
(EFIAPI *EFI_PCI_HOTPLUG_REQUEST_NOTIFY) (
 IN EFI_PCI_HOTPLUG_REQUEST_PROTOCOL *This,
 IN EFI_PCI_HOTPLUG_OPERATION        Operation,
 IN EFI_HANDLE                       Controller,
 IN EFI_DEVICE_PATH_PROTOCOL         *RemainingDevicePath OPTIONAL,
 IN OUT UINT8                        *NumberOfChildren,
 IN OUT EFI_HANDLE                   *ChildHandleBuffer
);



struct _EFI_PCI_HOTPLUG_REQUEST_PROTOCOL {
  EFI_PCI_HOTPLUG_REQUEST_NOTIFY     Notify;
};

extern EFI_GUID gEfiPciHotPlugRequestProtocolGuid;

#endif
