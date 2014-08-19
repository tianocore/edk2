/** @file

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
  Copyright (c) 2014, ARM Limited. All rights reserved.

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Library/ArmLib.h>
#include "ArmLibPrivate.h"

ARM_CACHE_TYPE
EFIAPI
ArmCacheType (
  VOID
  )
{
  switch (CACHE_TYPE (ArmCacheInfo ()))
  {
    case CACHE_TYPE_WRITE_BACK: return ARM_CACHE_TYPE_WRITE_BACK;
    default:                    return ARM_CACHE_TYPE_UNKNOWN;
  }
}

ARM_CACHE_ARCHITECTURE
EFIAPI
ArmCacheArchitecture (
  VOID
  )
{
  switch (CACHE_ARCHITECTURE (ArmCacheInfo ()))
  {
    case CACHE_ARCHITECTURE_UNIFIED:  return ARM_CACHE_ARCHITECTURE_UNIFIED;
    case CACHE_ARCHITECTURE_SEPARATE: return ARM_CACHE_ARCHITECTURE_SEPARATE;
    default:                          return ARM_CACHE_ARCHITECTURE_UNKNOWN;
  }
}

BOOLEAN
EFIAPI
ArmDataCachePresent (
  VOID
  )
{
  switch (DATA_CACHE_PRESENT (ArmCacheInfo ()))
  {
    case CACHE_PRESENT:     return TRUE;
    case CACHE_NOT_PRESENT: return FALSE;
    default:                return FALSE;
  }
}

UINTN
EFIAPI
ArmDataCacheSize (
  VOID
  )
{
  switch (DATA_CACHE_SIZE (ArmCacheInfo ()))
  {
    case CACHE_SIZE_4_KB:   return   4 * 1024;
    case CACHE_SIZE_8_KB:   return   8 * 1024;
    case CACHE_SIZE_16_KB:  return  16 * 1024;
    case CACHE_SIZE_32_KB:  return  32 * 1024;
    case CACHE_SIZE_64_KB:  return  64 * 1024;
    case CACHE_SIZE_128_KB: return 128 * 1024;
    default:                return   0;
  }
}

UINTN
EFIAPI
ArmDataCacheAssociativity (
  VOID
  )
{
  switch (DATA_CACHE_ASSOCIATIVITY (ArmCacheInfo ()))
  {
    case CACHE_ASSOCIATIVITY_4_WAY:   return 4;
    case CACHE_ASSOCIATIVITY_DIRECT:  return 1;
    default:                          return 0;
  }
}

UINTN
EFIAPI
ArmDataCacheLineLength (
  VOID
  )
{
  switch (DATA_CACHE_LINE_LENGTH (ArmCacheInfo ()))
  {
    case CACHE_LINE_LENGTH_32_BYTES: return 32;
    default:                         return  0;
  }
}

BOOLEAN
EFIAPI
ArmInstructionCachePresent (
  VOID
  )
{
  switch (INSTRUCTION_CACHE_PRESENT (ArmCacheInfo ()))
  {
    case CACHE_PRESENT:     return TRUE;
    case CACHE_NOT_PRESENT: return FALSE;
    default:                return FALSE;
  }
}

UINTN
EFIAPI
ArmInstructionCacheSize (
  VOID
  )
{
  switch (INSTRUCTION_CACHE_SIZE (ArmCacheInfo ()))
  {
    case CACHE_SIZE_4_KB:   return   4 * 1024;
    case CACHE_SIZE_8_KB:   return   8 * 1024;
    case CACHE_SIZE_16_KB:  return  16 * 1024;
    case CACHE_SIZE_32_KB:  return  32 * 1024;
    case CACHE_SIZE_64_KB:  return  64 * 1024;
    case CACHE_SIZE_128_KB: return 128 * 1024;
    default:                return   0;
  }
}

UINTN
EFIAPI
ArmInstructionCacheAssociativity (
  VOID
  )
{
  switch (INSTRUCTION_CACHE_ASSOCIATIVITY (ArmCacheInfo ()))
  {
    case CACHE_ASSOCIATIVITY_8_WAY:   return 8;
    case CACHE_ASSOCIATIVITY_4_WAY:   return 4;
    case CACHE_ASSOCIATIVITY_DIRECT:  return 1;
    default:                          return 0;
  }
}

UINTN
EFIAPI
ArmInstructionCacheLineLength (
  VOID
  )
{
  switch (INSTRUCTION_CACHE_LINE_LENGTH (ArmCacheInfo ()))
  {
    case CACHE_LINE_LENGTH_32_BYTES: return 32;
    default:                         return  0;
  }
}


