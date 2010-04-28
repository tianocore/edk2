/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  FirmwareVolumeDispatch.c

Abstract:

  Firmware Volume Dispatch protocol as defined in the Tiano Firmware Volume
  specification.

  Presence of this protocol tells the dispatch to dispatch from this Firmware 
  Volume
 
--*/

#include "Tiano.h"                  
#include EFI_PROTOCOL_DEFINITION (FirmwareVolumeDispatch)

EFI_GUID gEfiFirmwareVolumeDispatchProtocolGuid = EFI_FIRMWARE_VOLUME_DISPATCH_PROTOCOL_GUID;

EFI_GUID_STRING (&gEfiFirmwareVolumeDispatchProtocolGuid, "FirmwareVolumeDispatch Protocol", 
                 "Firmware Volume Dispatch protocol");
