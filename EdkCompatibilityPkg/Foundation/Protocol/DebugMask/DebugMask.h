/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  DebugMask.h

Abstract:

  This protocol is used to abstract the Debug Mask serivces for 
  the specific driver or application image.

--*/

#ifndef _DEBUG_MASK_H_
#define _DEBUG_MASK_H_

//
//4C8A2451-C207-405b-9694-99EA13251341
//
#define EFI_DEBUG_MASK_PROTOCOL_GUID \
  { 0x4c8a2451, 0xc207, 0x405b, {0x96, 0x94, 0x99, 0xea, 0x13, 0x25, 0x13, 0x41} }


#define EFI_DEBUG_MASK_REVISION        0x00010000

//
// Forward reference for pure ANSI compatability
//
EFI_FORWARD_DECLARATION (EFI_DEBUG_MASK_PROTOCOL);

//
// DebugMask member functions definition
//
typedef
EFI_STATUS
(EFIAPI * EFI_GET_DEBUG_MASK) (
  IN EFI_DEBUG_MASK_PROTOCOL      *This,             // Calling context
  IN OUT UINTN                    *CurrentDebugMask  // Ptr to store current debug mask
  );

typedef 
EFI_STATUS
(EFIAPI *EFI_SET_DEBUG_MASK) (
  IN  EFI_DEBUG_MASK_PROTOCOL     *This,             // Calling context
  IN  UINTN                       NewDebugMask       // New Debug Mask value to set
  );

//
// DebugMask protocol definition
//
struct _EFI_DEBUG_MASK_PROTOCOL {
  INT64                               Revision;
  EFI_GET_DEBUG_MASK                  GetDebugMask;
  EFI_SET_DEBUG_MASK                  SetDebugMask;
};

extern EFI_GUID gEfiDebugMaskProtocolGuid;

#endif
