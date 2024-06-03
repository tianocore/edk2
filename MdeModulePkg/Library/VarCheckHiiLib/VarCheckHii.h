/** @file
  Include file for Var Check Hii handler and bin.

Copyright (c) 2015 - 2024, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _VAR_CHECK_HII_H_
#define _VAR_CHECK_HII_H_

#include <Library/VarCheckLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Guid/MdeModuleHii.h>

#include <Protocol/HiiDatabase.h>
#include <Protocol/FirmwareVolume2.h>
#include <Protocol/FirmwareVolumeBlock.h>

#include "InternalVarCheckStructure.h"
#include "VarCheckHiiGen.h"

// #define DUMP_VAR_CHECK_HII
// #define DUMP_HII_DATA

typedef struct {
  UINT8    HiiOpCode;
  CHAR8    *HiiOpCodeStr;
} VAR_CHECK_HII_OPCODE_STRING;

typedef struct {
  UINT8    PackageType;
  CHAR8    *PackageTypeStr;
} VAR_CHECK_HII_PACKAGE_TYPE_STRING;

/**
  Dump Var Check HII.

  @param[in] VarCheckHiiBin     Pointer to VarCheckHiiBin.
  @param[in] VarCheckHiiBinSize VarCheckHiiBin size.

**/
VOID
DumpVarCheckHii (
  IN VOID   *VarCheckHiiBin,
  IN UINTN  VarCheckHiiBinSize
  );

#define VAR_CHECK_RECEIVED_HII_BIN_HANDLER_GUID \
  { \
    0xabcdef12, 0x3456, 0x7890, { 0xab, 0xcd, 0xef, 0x12, 0x34, 0x56, 0x78, 0x90 } \
  }

extern EFI_GUID  gVarCheckReceivedHiiBinHandlerGuid;
gVarCheckReceivedHiiBinHandlerGuid = VAR_CHECK_RECEIVED_HII_BIN_HANDLER_GUID;
#endif
