/** @file
  Runtime Architectural Protocol as defined in DXE CIS

  This code is used to produce the EFI 1.0 runtime virtual switch over

  This driver must add SetVirtualAddressMap () and ConvertPointer () to
  the EFI system table. This driver is not responcible for CRCing the 
  EFI system table.

  This driver will add EFI_RUNTIME_ARCH_PROTOCOL_GUID protocol with a 
  pointer to the Runtime Arch Protocol instance structure. The protocol
  member functions are used by the DXE core to export information need
  by this driver to produce the runtime transition to virtual mode
  calling.

  Copyright (c) 2006, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

  Module Name:  Runtime.h

  @par Revision Reference:
  Version 0.90.

**/

#ifndef __ARCH_PROTOCOL_RUNTIME_H__
#define __ARCH_PROTOCOL_RUNTIME_H__

//
// Global ID for the Runtime Architectural Protocol
//
#define EFI_RUNTIME_ARCH_PROTOCOL_GUID \
  { 0x96d08253, 0x8483, 0x11d4, {0xbc, 0xf1, 0x0, 0x80, 0xc7, 0x3c, 0x88, 0x81 } }

typedef struct _EFI_RUNTIME_ARCH_PROTOCOL   EFI_RUNTIME_ARCH_PROTOCOL;

/**
  When a SetVirtualAddressMap() is performed all the runtime images loaded by 
  DXE must be fixed up with the new virtual address map. To facilitate this the 
  Runtime Architectural Protocol needs to be informed of every runtime driver 
  that is registered.  All the runtime images loaded by DXE should be registered 
  with this service by the DXE Core when ExitBootServices() is called.  The 
  images that are registered with this service must have successfully been 
  loaded into memory with the Boot Service LoadImage().  As a result, no 
  parameter checking needs to be performed.

  @param  This The EFI_RUNTIME_ARCH_PROTOCOL instance.
  
  @param  ImageBase Start of image that has been loaded in memory. It is either
  a pointer to the DOS or PE header of the image.
  
  @param  ImageSize Size of the image in bytes.
  
  @param  RelocationData Information about the fixups that were performed on ImageBase
  when it was loaded into memory. This information is needed
  when the virtual mode fix-ups are reapplied so that data that
  has been programmatically updated will not be fixed up. If
  code updates a global variable the code is responsible for
  fixing up the variable for virtual mode.

  @retval  EFI_SUCCESS The ImageBase has been registered.
  
  @retval  EFI_OUT_OF_RESOURCES There are not enough resources to register ImageBase.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_RUNTIME_REGISTER_IMAGE) (
  IN EFI_RUNTIME_ARCH_PROTOCOL  *This,
  IN  EFI_PHYSICAL_ADDRESS              ImageBase,   
  IN  UINTN                             ImageSize,     
  IN  VOID                              *RelocationData    
  );


/**
  This function is used to support the required runtime events. Currently only 
  runtime events of type EFI_EVENT_SIGNAL_VIRTUAL_ADDRESS_CHANGE needs to be 
  registered with this service.  All the runtime events that exist in the DXE 
  Core should be registered with this service when ExitBootServices() is called.  
  All the events that are registered with this service must have been created 
  with the Boot Service CreateEvent().  As a result, no parameter checking needs 
  to be performed.

  @param  This The EFI_RUNTIME_ARCH_PROTOCOL instance.
  
  @param  Type The same as Type passed into CreateEvent().
  
  @param  NotifyTpl The same as NotifyTpl passed into CreateEvent().
  
  @param  NotifyFunction The same as NotifyFunction passed into CreateEvent().
  
  @param  NotifyContext The same as NotifyContext passed into CreateEvent().
  
  @param  Event The EFI_EVENT returned by CreateEvent().  Event must be in
  runtime memory.

  @retval  EFI_SUCCESS The Event has been registered.
  
  @retval  EFI_OUT_OF_RESOURCES There are not enough resources to register Event.

**/
typedef
EFI_STATUS
(EFIAPI *EFI_RUNTIME_REGISTER_EVENT) (
  IN EFI_RUNTIME_ARCH_PROTOCOL  *This,
  IN UINT32                             Type,
  IN EFI_TPL                            NotifyTpl,
  IN EFI_EVENT_NOTIFY                   NotifyFunction,
  IN VOID                               *NotifyContext,
  IN EFI_EVENT                          *Event
  );

//
// Interface stucture for the Runtime Architectural Protocol
//
/**
  @par Protocol Description:
  The DXE driver that produces this protocol must be a runtime driver.  This 
  driver is responsible for initializing the SetVirtualAddressMap() and 
  ConvertPointer() fields of the EFI Runtime Services Table and the 
  CalculateCrc32() field of the EFI Boot Services Table.  See the Runtime 
  Services chapter and the Boot Services chapter for details on these services.
  After the two fields of the EFI Runtime Services Table and the one field of 
  the EFI Boot Services Table have been initialized, the driver must install 
  the EFI_RUNTIME_ARCH_PROTOCOL_GUID on a new handle with an EFI_RUNTIME_ARCH_ 
  PROTOCOL interface pointer.  The installation of this protocol informs the 
  DXE core that the virtual memory services and the 32-bit CRC services are 
  now available, and the DXE core must update the 32-bit CRC of the EFI Runtime 
  Services Table and the 32-bit CRC of the EFI Boot Services Table.

  All runtime core services are provided by the EFI_RUNTIME_ARCH_PROTOCOL.  
  This includes the support for registering runtime images that must be 
  re-fixed up when a transition is made from physical mode to virtual mode. 
  This protocol also supports all events that are defined to fire at runtime. 
  This protocol also contains a CRC-32 function that will be used by the DXE 
  core as a boot service. The EFI_RUNTIME_ARCH_PROTOCOL needs the CRC-32 
  function when a transition is made from physical mode to virtual mode and 
  the EFI System Table and EFI Runtime Table are fixed up with virtual pointers.

  @param RegisterRuntimeImage
  Register a runtime image so it can be converted to virtual mode if the EFI Runtime Services 
  SetVirtualAddressMap() is called.

  @param RegisterRuntimeEvent
  Register an event than needs to be notified at runtime. 

**/
struct _EFI_RUNTIME_ARCH_PROTOCOL {
  EFI_RUNTIME_REGISTER_IMAGE  RegisterImage;
  EFI_RUNTIME_REGISTER_EVENT  RegisterEvent;
};

extern EFI_GUID gEfiRuntimeArchProtocolGuid;

#endif
