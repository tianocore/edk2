/*++

Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  
  EfiGroup.c
    
--*/
#include "Tiano.h"

#include EFI_GUID_DEFINITION (EventGroup)

EFI_GUID gEfiEventExitBootServicesGuid = EFI_EVENT_GROUP_EXIT_BOOT_SERVICES;
EFI_GUID gEfiEventVirtualAddressChangeGuid = EFI_EVENT_GROUP_VIRTUAL_ADDRESS_CHANGE;
EFI_GUID gEfiEventMemoryMapChangeGuid = EFI_EVENT_GROUP_MEMORY_MAP_CHANGE;
EFI_GUID gEfiEventReadyToBootGuid = EFI_EVENT_GROUP_READY_TO_BOOT;

EFI_GUID_STRING (&gEfiEventExitBootServicesGuid, "EventExitBS", "Event Exit Boot Service GUID");
EFI_GUID_STRING (&gEfiEventVirtualAddressChangeGuid, "EventVirtualAddrChange", "Event Virtual Addr Change GUID");
EFI_GUID_STRING (&gEfiEventMemoryMapChangeGuid, "EventMemMapChange", "Event Memory Map Change GUID");
EFI_GUID_STRING (&gEfiEventReadyToBootGuid, "EventReadyToBoot", "Efi Ready To Boot GUID");

