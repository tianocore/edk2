/**@file
  Vga Mini port binding for a VGA controller

Copyright (c) 2006 - 2007, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef __VGA_MINI_PORT_H_
#define __VGA_MINI_PORT_H_

#define EFI_VGA_MINI_PORT_PROTOCOL_GUID \
  { \
    0xc7735a2f, 0x88f5, 0x4882, {0xae, 0x63, 0xfa, 0xac, 0x8c, 0x8b, 0x86, 0xb3 } \
  }

typedef struct _EFI_VGA_MINI_PORT_PROTOCOL  EFI_VGA_MINI_PORT_PROTOCOL;

/**
  Sets the text display mode of a VGA controller
  
  @param This             Protocol instance pointer.
  @param Mode             Mode number.  0 - 80x25   1-80x50

  @retval EFI_SUCCESS            The mode was set
  @retval EFI_DEVICE_ERROR       The device is not functioning properly.
  
**/
typedef
EFI_STATUS
(EFIAPI *EFI_VGA_MINI_PORT_SET_MODE) (
  IN EFI_VGA_MINI_PORT_PROTOCOL          * This,
  IN UINTN                               ModeNumber
  );

struct _EFI_VGA_MINI_PORT_PROTOCOL {
  EFI_VGA_MINI_PORT_SET_MODE  SetMode;

  UINT64                      VgaMemoryOffset;
  UINT64                      CrtcAddressRegisterOffset;
  UINT64                      CrtcDataRegisterOffset;

  UINT8                       VgaMemoryBar;
  UINT8                       CrtcAddressRegisterBar;
  UINT8                       CrtcDataRegisterBar;

  UINT8                       MaxMode;
};

extern EFI_GUID gEfiVgaMiniPortProtocolGuid;

#endif
