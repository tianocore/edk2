/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  
    FormBrowser.h

Abstract:

  The EFI_FORM_BROWSER_PROTOCOL is the interface to the EFI 
  Configuration Driver.  This will allow the caller to direct the 
  configuration driver to use either the HII database or use the passed 
  in packet of data.  This will also allow the caller to post messages 
  into the configuration drivers internal mailbox.

--*/

#ifndef _FORM_BROWSER_H_
#define _FORM_BROWSER_H_

#include EFI_PROTOCOL_DEFINITION (Hii)

//
// EFI_FORM_BROWSER_PROTOCOL_GUID has been changed from the one defined in Framework HII 0.92 as the
// Setup Browser protocol produced by HII Thunk Layer support the UEFI IFR and UEF String Package format.
//
#define EFI_FORM_BROWSER_PROTOCOL_GUID \
  { \
    0xfb7c852, 0xadca, 0x4853, {0x8d, 0xf, 0xfb, 0xa7, 0x1b, 0x1c, 0xe1, 0x1a} \
  }

/*
#define EFI_FORM_BROWSER_PROTOCOL_GUID \
  { \
    0xe5a1333e, 0xe1b4, 0x4d55, {0xce, 0xeb, 0x35, 0xc3, 0xef, 0x13, 0x34, 0x43} \
  }
*/

//
// Forward reference for pure ANSI compatability
//
typedef struct _EFI_FORM_BROWSER_PROTOCOL EFI_FORM_BROWSER_PROTOCOL;

typedef struct {
  UINT32  Length;
  UINT16  Type;
  UINT8   Data[1];
} EFI_HII_PACKET;

typedef struct {
  EFI_HII_IFR_PACK    *IfrData;
  EFI_HII_STRING_PACK *StringData;
} EFI_IFR_PACKET;

typedef struct {
  UINTN LeftColumn;
  UINTN RightColumn;
  UINTN TopRow;
  UINTN BottomRow;
} SCREEN_DESCRIPTOR;

//
// The following types are currently defined:
//
typedef
EFI_STATUS
(EFIAPI *EFI_SEND_FORM) (
  IN  EFI_FORM_BROWSER_PROTOCOL       * This,
  IN  BOOLEAN                         UseDatabase,
  IN  EFI_HII_HANDLE                  * Handle,
  IN  UINTN                           HandleCount,
  IN  EFI_IFR_PACKET                  * Packet,
  IN  EFI_HANDLE                      CallbackHandle,
  IN  UINT8                           *NvMapOverride,
  IN SCREEN_DESCRIPTOR                * ScreenDimensions,
  OUT BOOLEAN                         *ResetRequired OPTIONAL
  );

typedef
EFI_STATUS
(EFIAPI *EFI_CREATE_POP_UP) (
  IN  UINTN                           NumberOfLines,
  IN  BOOLEAN                         HotKey,
  IN  UINTN                           MaximumStringSize,
  OUT CHAR16                          *StringBuffer,
  OUT EFI_INPUT_KEY                   * KeyValue,
  IN  CHAR16                          *String,
  ...
  );

struct _EFI_FORM_BROWSER_PROTOCOL {
  EFI_SEND_FORM     SendForm;
  EFI_CREATE_POP_UP CreatePopUp;
};

extern EFI_GUID gEfiFormBrowserProtocolGuid;

#endif
