/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  DebugLevel.h

Abstract:
  This protocol is used to abstract the Debug Mask serivces for 
  the specific driver or application image.

--*/

#ifndef __DEBUG_LEVEL_H__
#define __DEBUG_LEVEL_H__

//
// 8D4C62E6-CD98-4e1d-AD6E-48BB50D29FF7
//
#define EFI_DEBUG_LEVEL_PROTOCOL_GUID \
  { 0x8d4c62e6, 0xcd98, 0x4e1d, {0xad, 0x6e, 0x48, 0xbb, 0x50, 0xd2, 0x9f, 0xf7 } }

//
// DebugLevel protocol definition
//
typedef struct _EFI_DEBUG_LEVEL_PROTOCOL {
  UINTN  DebugLevel;
} EFI_DEBUG_LEVEL_PROTOCOL;

extern EFI_GUID gEfiDebugLevelProtocolGuid;

#endif
