/*++

Copyright (c) 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  EdidDiscovered.h

Abstract:

  EDID Discovered Protocol from the UEFI 2.0 specification.

  This protocol is placed on the video output device child handle and it represents
  the EDID information being used for output device represented by the child handle.

--*/

#ifndef __EDID_DISCOVERED_H__
#define __EDID_DISCOVERED_H__

#define EFI_EDID_DISCOVERED_PROTOCOL_GUID \
  { \
    0x1c0c34f6, 0xd380, 0x41fa, {0xa0, 0x49, 0x8a, 0xd0, 0x6c,0x1a, 0x66, 0xaa} \
  }

typedef struct {
  UINT32   SizeOfEdid;
  UINT8    *Edid;
} EFI_EDID_DISCOVERED_PROTOCOL;

extern EFI_GUID gEfiEdidDiscoveredProtocolGuid;

#endif
