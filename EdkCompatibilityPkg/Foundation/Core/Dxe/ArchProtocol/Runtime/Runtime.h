/*++

Copyright (c) 2004 - 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  Runtime.h

Abstract:

  Runtime Architectural Protocol as defined in DXE CIS.


  This code is used to produce the EFI runtime services that are callable
  only in physical mode. 

  This driver must add SetVirtualAddressMap () and ConvertPointer () to
  the EFI system table. This driver is not responcible for CRCing the 
  EFI system table.

  This driver will add EFI_RUNTIME_ARCH_PROTOCOL_GUID protocol with a 
  pointer to the Runtime Arch Protocol instance structure. The protocol
  member functions are used by the DXE core to export information needed
  by this driver to produce the runtime transition of runtime drivers from
  physical mode calling to virtual mode calling.

--*/

#ifndef _ARCH_PROTOCOL_RUNTIME_H_
#define _ARCH_PROTOCOL_RUNTIME_H_

#include "LinkedList.h"

//
// Global ID for the Runtime Architectural Protocol
//
#define EFI_RUNTIME_ARCH_PROTOCOL_GUID \
  { 0xb7dfb4e1, 0x52f, 0x449f, {0x87, 0xbe, 0x98, 0x18, 0xfc, 0x91, 0xb7, 0x33} }

EFI_FORWARD_DECLARATION (EFI_RUNTIME_ARCH_PROTOCOL);

struct _EFI_RUNTIME_IMAGE_ENTRY {
  VOID                    *ImageBase;
  UINT64                  ImageSize;
  VOID                    *RelocationData;
  EFI_HANDLE              Handle;
  EFI_LIST_ENTRY          Link;
};

struct _EFI_RUNTIME_EVENT_ENTRY {
  UINT32                  Type;
  EFI_TPL                 NotifyTpl;
  EFI_EVENT_NOTIFY        NotifyFunction;
  VOID                    *NotifyContext;
  EFI_EVENT               *Event;
  EFI_LIST_ENTRY          Link;
};

//
// Interface stucture for the Runtime Architectural Protocol
//
struct _EFI_RUNTIME_ARCH_PROTOCOL {
  EFI_LIST_ENTRY          ImageHead;
  EFI_LIST_ENTRY          EventHead;
  UINTN                   MemoryDescriptorSize;
  UINT32                  MemoryDesciptorVersion;
  UINTN                   MemoryMapSize;
  EFI_MEMORY_DESCRIPTOR   *MemoryMapPhysical;
  EFI_MEMORY_DESCRIPTOR   *MemoryMapVirtual;
  BOOLEAN                 VirtualMode;
  BOOLEAN                 AtRuntime;
};
/*++

Protocol Description:

  Allows the runtime functionality of the DXE Foundation to be contained in a 
  separate driver. It also provides hooks for the DXE Foundation to export 
  information that is needed at runtime. As such, this protocol allows the DXE 
  Foundation to manage runtime drivers and events. This protocol also implies 
  that the runtime services required to transition to virtual mode, 
  SetVirtualAddressMap() and ConvertPointer(), have been registered into the 
  EFI Runtime Table in the EFI System Partition.  This protocol must be produced 
  by a runtime DXE driver and may only be consumed by the DXE Foundation.

Parameters:
  
  ImageHead               - A list of type EFI_RUNTIME_IMAGE_ENTRY.
  EventHead               - A list of type EFI_RUNTIME_EVENT_ENTRY.
  MemoryDescriptorSize    - Size of a memory descriptor that is return by 
                            GetMemoryMap().
  MemoryDescriptorVersion - Version of a memory descriptor that is return by 
                            GetMemoryMap().
  MemoryMapSize           - Size of the memory map in bytes contained in 
                            MemoryMapPhysical and MemoryMapVirtual. 
  MemoryMapPhysical       - Pointer to a runtime buffer that contains a copy of the 
                            memory map returned via GetMemoryMap().
  MemoryMapVirtual        - Pointer to MemoryMapPhysical that is updated to virtual mode 
                            after SetVirtualAddressMap(). 
  VirtualMode             - Boolean that is TRUE if SetVirtualAddressMap() has been called. 
  AtRuntime               - Boolean that is TRUE if ExitBootServices () has been called.
  
--*/

extern EFI_GUID gEfiRuntimeArchProtocolGuid;

#endif
