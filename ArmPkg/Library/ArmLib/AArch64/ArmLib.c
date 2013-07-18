/** @file

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
  portions copyright (c) 2011 - 2013, ARM Ltd. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Base.h>

#include <Library/ArmLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>

#include "ArmLibPrivate.h"

VOID
EFIAPI
ArmCacheInformation (
  OUT ARM_CACHE_INFO  *CacheInfo
  )
{
  if (CacheInfo != NULL) {
    CacheInfo->Type                           = ArmCacheType();
    CacheInfo->Architecture                   = ArmCacheArchitecture();
    CacheInfo->DataCachePresent               = ArmDataCachePresent();
    CacheInfo->DataCacheSize                  = ArmDataCacheSize();
    CacheInfo->DataCacheAssociativity         = ArmDataCacheAssociativity();
    CacheInfo->DataCacheLineLength            = ArmDataCacheLineLength();
    CacheInfo->InstructionCachePresent        = ArmInstructionCachePresent();
    CacheInfo->InstructionCacheSize           = ArmInstructionCacheSize();
    CacheInfo->InstructionCacheAssociativity  = ArmInstructionCacheAssociativity();
    CacheInfo->InstructionCacheLineLength     = ArmInstructionCacheLineLength();
  }
}

VOID
EFIAPI
ArmSetAuxCrBit (
  IN  UINT32    Bits
  )
{
  UINT32 val = ArmReadAuxCr();
  val |= Bits;
  ArmWriteAuxCr(val);
}
