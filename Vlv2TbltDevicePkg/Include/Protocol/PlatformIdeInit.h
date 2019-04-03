/*++

  Copyright (c) 2004  - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  SPDX-License-Identifier: BSD-2-Clause-Patent

                                                                                   


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
