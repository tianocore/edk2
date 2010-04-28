/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  PxeBaseCodeCallBack.h

Abstract:
  EFI PXE Base Code CallBack Protocol

--*/

#ifndef _PXE_BASE_CODE_CALLBACK_H_
#define _PXE_BASE_CODE_CALLBACK_H_

#include "Pxe.h"

//
// Call Back Definitions
//
#define EFI_PXE_BASE_CODE_CALLBACK_PROTOCOL_GUID \
  { \
    0x245dca21, 0xfb7b, 0x11d3, {0x8f, 0x01, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b} \
  }

//
// Revision Number
//
#define EFI_PXE_BASE_CODE_CALLBACK_INTERFACE_REVISION 0x00010000

//
// Protocol definition
//
EFI_FORWARD_DECLARATION (EFI_PXE_BASE_CODE_CALLBACK_PROTOCOL);

typedef enum {
  EFI_PXE_BASE_CODE_FUNCTION_FIRST,
  EFI_PXE_BASE_CODE_FUNCTION_DHCP,
  EFI_PXE_BASE_CODE_FUNCTION_DISCOVER,
  EFI_PXE_BASE_CODE_FUNCTION_MTFTP,
  EFI_PXE_BASE_CODE_FUNCTION_UDP_WRITE,
  EFI_PXE_BASE_CODE_FUNCTION_UDP_READ,
  EFI_PXE_BASE_CODE_FUNCTION_ARP,
  EFI_PXE_BASE_CODE_FUNCTION_IGMP,
  EFI_PXE_BASE_CODE_FUNCTION_TCP_WRITE,
  EFI_PXE_BASE_CODE_FUNCTION_TCP_READ,
  EFI_PXE_BASE_CODE_PXE_FUNCTION_LAST
} EFI_PXE_BASE_CODE_FUNCTION;

typedef enum {
  EFI_PXE_BASE_CODE_CALLBACK_STATUS_FIRST,
  EFI_PXE_BASE_CODE_CALLBACK_STATUS_CONTINUE,
  EFI_PXE_BASE_CODE_CALLBACK_STATUS_ABORT,
  EFI_PXE_BASE_CODE_CALLBACK_STATUS_LAST
} EFI_PXE_BASE_CODE_CALLBACK_STATUS;

typedef EFI_PXE_BASE_CODE_CALLBACK_STATUS (EFIAPI *EFI_PXE_CALLBACK)
  (
    IN EFI_PXE_BASE_CODE_CALLBACK_PROTOCOL * This,
    IN EFI_PXE_BASE_CODE_FUNCTION Function,
    IN BOOLEAN Received,
    IN UINT32 PacketLen,
    IN EFI_PXE_BASE_CODE_PACKET * Packet OPTIONAL
  );

struct _EFI_PXE_BASE_CODE_CALLBACK_PROTOCOL {
  UINT64            Revision;
  EFI_PXE_CALLBACK  Callback;
};

extern EFI_GUID gEfiPxeBaseCodeCallbackProtocolGuid;

#endif /* _EFIPXEBC_H */

/* EOF - PxeBaseCodeCallBack.h */
