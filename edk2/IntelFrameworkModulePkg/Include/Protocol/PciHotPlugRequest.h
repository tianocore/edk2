/** @file
  Provides services to notify the PCI bus driver that some events have happened in a hot-plug controller
  (such as a PC Card socket, or PHPC), and to ask the PCI bus driver to create or destroy handles for 
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
  { \
    0x19cb87ab, 0x2cb9, 0x4665, {0x83, 0x60, 0xdd, 0xcf, 0x60, 0x54, 0xf7, 0x9d} \
  }

typedef enum {
  ///
  /// The PCI bus driver is requested to create handles for the specified devices. An array of
  /// EFI_HANDLE is returned, with a NULL element marking the end of the array.
  ///
  EfiPciHotPlugRequestAdd,

  ///
  /// The PCI bus driver is requested to destroy handles for the specified devices.
  ///
  EfiPciHotplugRequestRemove
} EFI_PCI_HOTPLUG_OPERATION;

typedef struct _EFI_PCI_HOTPLUG_REQUEST_PROTOCOL  EFI_PCI_HOTPLUG_REQUEST_PROTOCOL;

/**
  This function allows the PCI bus driver to be notified to act as requested when a hot-plug event has  happened on the hot-plug controller. Currently, the operations include add operation and remove operation.  
  @param This                    A pointer to the hot plug request protocol.
  @param Operation               The operation the PCI bus driver is requested to make.
  @param Controller              The handle of the hot-plug controller.
  @param RemainingDevicePath     The remaining device path for the PCI-like hot-plug device.
  @param NumberOfChildren        The number of child handles. For an add operation, it is an output parameter. 
                                 For a remove operation, it's an input parameter. When it contains a non-zero
                                 value, children handles specified in ChildHandleBuffer are destroyed. Otherwise,
                                 PCI bus driver is notified to stop managing the controller handle.
  @param ChildHandleBuffer       The buffer which contains the child handles. For an add operation, it is an output 
                                 parameter and contains all newly created child handles. For a remove operation, it 
                                 contains child handles to be destroyed when NumberOfChildren contains a non-
                                 zero value. It can be NULL when NumberOfChildren is 0. It's the caller's 
                                 responsibility to allocate and free memory for this buffer.
  
  @retval EFI_SUCCESS            The handles for the specified device have been created or destroyed
                                 as requested, and for an add operation, the new handles are
                                 returned in ChildHandleBuffer.
  @retval EFI_INVALID_PARAMETER  Operation is not a legal value.
  @retval EFI_INVALID_PARAMETER  Controller is NULL or not a valid handle.
  @retval EFI_INVALID_PARAMETER  NumberOfChildren is NULL.
  @retval EFI_INVALID_PARAMETER  ChildHandleBuffer is NULL while Operation is remove and 
                                 NumberOfChildren contains a non-zero value.
  @retval EFI_INVALID_PARAMETER  ChildHandleBuffer is NULL while Operation is add.
  @retval EFI_OUT_OF_RESOURCES   There are no enough resources to start the devices.
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
