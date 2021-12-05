/** @file
  EBC Debugger configuration protocol.

  Copyright (c) 2007-2016, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __EFI_DEBUGGER_CONFIGURATION_H__
#define __EFI_DEBUGGER_CONFIGURATION_H__

#define EFI_DEBUGGER_CONFIGURATION_PROTOCOL_GUID \
  { 0x577d959c, 0xe967, 0x4546, 0x86, 0x20, 0xc7, 0x78, 0xfa, 0xe5, 0xda, 0x5 }

#define EFI_DEBUGGER_CONFIGURATION_VERSION  0x00000001

typedef struct _EFI_DEBUGGER_CONFIGURATION_PROTOCOL {
  UINT32    DebuggerConfigurationRevision;
  VOID      *DebuggerPrivateData;
} EFI_DEBUGGER_CONFIGURATION_PROTOCOL;

extern EFI_GUID  gEfiDebuggerConfigurationProtocolGuid;

#endif
