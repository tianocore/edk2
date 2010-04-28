/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  FirmwareVolume.c

Abstract:

  Firmware Volume protocol as defined in the Tiano Firmware Volume
  specification.

  File level access layered on top of Firmware File System protocol.  This
  protocol exists to provide a hook for a filter driver for a firmware volume.
 
--*/

#include "Tiano.h"
#include EFI_PROTOCOL_DEFINITION (FirmwareVolume)

EFI_GUID  gEfiFirmwareVolumeProtocolGuid = EFI_FIRMWARE_VOLUME_PROTOCOL_GUID;

EFI_GUID_STRING(&gEfiFirmwareVolumeProtocolGuid, "FirmwareVolume Protocol", "Firmware Volume protocol");
