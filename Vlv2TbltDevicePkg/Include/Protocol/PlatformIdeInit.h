/*++

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   


Module Name:

    PlatformIdeInit.h

Abstract:

    EFI Platform Ide Init Protocol

Revision History

**/

#ifndef _EFI_PLATFORM_IDE_INIT_H_
#define _EFI_PLATFORM_IDE_INIT_H_

//
// Global ID for the IDE Platform Protocol
//
#define EFI_PLATFORM_IDE_INIT_PROTOCOL_GUID \
  { 0x377c66a3, 0x8fe7, 0x4ee8, 0x85, 0xb8, 0xf1, 0xa2, 0x82, 0x56, 0x9e, 0x3b };

EFI_FORWARD_DECLARATION (EFI_PLATFORM_IDE_INIT_PROTOCOL);


//
// Interface structure for the Platform IDE Init Protocol
//
typedef struct _EFI_PLATFORM_IDE_INIT_PROTOCOL {
  BOOLEAN                               SmartMode;
} EFI_PLATFORM_IDE_INIT_PROTOCOL;

extern EFI_GUID gEfiPlatformIdeInitProtocolGuid;

#endif
