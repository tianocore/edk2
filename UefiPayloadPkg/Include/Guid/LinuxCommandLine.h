/** @file
  Define the structure for the Boot Manager Menu File.

Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef LINUX_COMMAND_LINE_GUID_H_
#define LINUX_COMMAND_LINE_GUID_H_

#include <Uefi.h>
#include <UniversalPayload/UniversalPayload.h>

#pragma pack (1)

typedef struct {
  UNIVERSAL_PAYLOAD_GENERIC_HEADER    Header;
  UINT32                              Count;
  CHAR8                               CommandLine[0];
} UNIVERSAL_PAYLOAD_LINUX_COMMAND_LINE;

#pragma pack()

#define UNIVERSAL_PAYLOAD_LINUX_COMMAND_LINE_REVISION  1

extern GUID  gEdkiiLinuxKernelCommandLineGuid;
#endif
