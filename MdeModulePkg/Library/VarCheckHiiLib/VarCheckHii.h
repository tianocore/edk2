/** @file
  Include file for Var Check Hii handler and bin.

Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _VAR_CHECK_HII_H_
#define _VAR_CHECK_HII_H_

#include <Library/VarCheckLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>

#include <Guid/ZeroGuid.h>

#include <Protocol/HiiDatabase.h>
#include <Protocol/FirmwareVolume2.h>
#include <Protocol/FirmwareVolumeBlock.h>

#include "InternalVarCheckStructure.h"
#include "VarCheckHiiGen.h"

//#define DUMP_VAR_CHECK_HII
//#define DUMP_HII_DATA

typedef struct {
  UINT8     HiiOpCode;
  CHAR8     *HiiOpCodeStr;
} VAR_CHECK_HII_OPCODE_STRING;

typedef struct {
  UINT8     PackageType;
  CHAR8     *PackageTypeStr;
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

extern VAR_CHECK_HII_VARIABLE_HEADER    *mVarCheckHiiBin;
extern UINTN                            mVarCheckHiiBinSize;

#endif
