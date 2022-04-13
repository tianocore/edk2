/** @file
  Functions for processor information common to ARM and AARCH64.

  Copyright (c) 2021, NUVIA Inc. All rights reserved.<BR>
  Copyright (c) 2021, Ampere Computing LLC. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <IndustryStandard/ArmCache.h>
#include <IndustryStandard/ArmStdSmc.h>
#include <IndustryStandard/SmBios.h>
#include <Library/ArmLib.h>
#include <Library/ArmSmcLib.h>
#include <Library/BaseMemoryLib.h>

#include "SmbiosProcessor.h"

/** Returns the maximum cache level implemented by the current CPU.

    @return The maximum cache level implemented.
**/
UINT8
SmbiosProcessorGetMaxCacheLevel (
  VOID
  )
{
  CLIDR_DATA  Clidr;
  UINT8       CacheLevel;
  UINT8       MaxCacheLevel;

  MaxCacheLevel = 0;

  // Read the CLIDR register to find out what caches are present.
  Clidr.Data = ReadCLIDR ();

  // Get the cache type for the L1 cache. If it's 0, there are no caches.
  if (CLIDR_GET_CACHE_TYPE (Clidr.Data, 1) == ClidrCacheTypeNone) {
    return 0;
  }

  for (CacheLevel = 1; CacheLevel <= MAX_ARM_CACHE_LEVEL; CacheLevel++) {
    if (CLIDR_GET_CACHE_TYPE (Clidr.Data, CacheLevel) == ClidrCacheTypeNone) {
      MaxCacheLevel = CacheLevel;
      break;
    }
  }

  return MaxCacheLevel;
}

/** Returns whether or not the specified cache level has separate I/D caches.

    @param CacheLevel The cache level (L1, L2 etc.).

    @return TRUE if the cache level has separate I/D caches, FALSE otherwise.
**/
BOOLEAN
SmbiosProcessorHasSeparateCaches (
  UINT8  CacheLevel
  )
{
  CLIDR_CACHE_TYPE  CacheType;
  CLIDR_DATA        Clidr;
  BOOLEAN           SeparateCaches;

  SeparateCaches = FALSE;

  Clidr.Data = ReadCLIDR ();

  CacheType = CLIDR_GET_CACHE_TYPE (Clidr.Data, CacheLevel - 1);

  if (CacheType == ClidrCacheTypeSeparate) {
    SeparateCaches = TRUE;
  }

  return SeparateCaches;
}

/** Checks if ther ARM64 SoC ID SMC call is supported

    @return Whether the ARM64 SoC ID call is supported.
**/
BOOLEAN
HasSmcArm64SocId (
  VOID
  )
{
  INT32    SmcCallStatus;
  BOOLEAN  Arm64SocIdSupported;
  UINTN    SmcParam;

  Arm64SocIdSupported = FALSE;

  SmcCallStatus = ArmCallSmc0 (SMCCC_VERSION, NULL, NULL, NULL);

  if ((SmcCallStatus < 0) || ((SmcCallStatus >> 16) >= 1)) {
    SmcParam      = SMCCC_ARCH_SOC_ID;
    SmcCallStatus = ArmCallSmc1 (SMCCC_ARCH_FEATURES, &SmcParam, NULL, NULL);
    if (SmcCallStatus >= 0) {
      Arm64SocIdSupported = TRUE;
    }
  }

  return Arm64SocIdSupported;
}

/** Fetches the JEP106 code and SoC Revision.

    @param Jep106Code  JEP 106 code.
    @param SocRevision SoC revision.

    @retval EFI_SUCCESS Succeeded.
    @retval EFI_UNSUPPORTED Failed.
**/
EFI_STATUS
SmbiosGetSmcArm64SocId (
  OUT INT32  *Jep106Code,
  OUT INT32  *SocRevision
  )
{
  INT32       SmcCallStatus;
  EFI_STATUS  Status;
  UINTN       SmcParam;

  Status = EFI_SUCCESS;

  SmcParam      = 0;
  SmcCallStatus = ArmCallSmc1 (SMCCC_ARCH_SOC_ID, &SmcParam, NULL, NULL);

  if (SmcCallStatus >= 0) {
    *Jep106Code = (INT32)SmcParam;
  } else {
    Status = EFI_UNSUPPORTED;
  }

  SmcParam      = 1;
  SmcCallStatus = ArmCallSmc1 (SMCCC_ARCH_SOC_ID, &SmcParam, NULL, NULL);

  if (SmcCallStatus >= 0) {
    *SocRevision = (INT32)SmcParam;
  } else {
    Status = EFI_UNSUPPORTED;
  }

  return Status;
}

/** Returns a value for the Processor ID field that conforms to SMBIOS
    requirements.

    @return Processor ID.
**/
UINT64
SmbiosGetProcessorId (
  VOID
  )
{
  INT32   Jep106Code;
  INT32   SocRevision;
  UINT64  ProcessorId;

  if (HasSmcArm64SocId ()) {
    SmbiosGetSmcArm64SocId (&Jep106Code, &SocRevision);
    ProcessorId = ((UINT64)SocRevision << 32) | Jep106Code;
  } else {
    ProcessorId = ArmReadMidr ();
  }

  return ProcessorId;
}

/** Returns the external clock frequency.

    @return The external clock frequency.
**/
UINTN
SmbiosGetExternalClockFrequency (
  VOID
  )
{
  return ArmReadCntFrq ();
}

/** Returns the SMBIOS ProcessorFamily field value.

    @return The value for the ProcessorFamily field.
**/
UINT8
SmbiosGetProcessorFamily (
  VOID
  )
{
  return ProcessorFamilyIndicatorFamily2;
}

/** Returns the ProcessorFamily2 field value.

    @return The value for the ProcessorFamily2 field.
**/
UINT16
SmbiosGetProcessorFamily2 (
  VOID
  )
{
  UINTN   MainIdRegister;
  UINT16  ProcessorFamily2;

  MainIdRegister = ArmReadMidr ();

  if (((MainIdRegister >> 16) & 0xF) < 8) {
    ProcessorFamily2 = ProcessorFamilyARM;
  } else {
    if (sizeof (VOID *) == 4) {
      ProcessorFamily2 = ProcessorFamilyARMv7;
    } else {
      ProcessorFamily2 = ProcessorFamilyARMv8;
    }
  }

  return ProcessorFamily2;
}

/** Returns the SMBIOS Processor Characteristics.

    @return Processor Characteristics bitfield.
**/
PROCESSOR_CHARACTERISTIC_FLAGS
SmbiosGetProcessorCharacteristics (
  VOID
  )
{
  PROCESSOR_CHARACTERISTIC_FLAGS  Characteristics;

  ZeroMem (&Characteristics, sizeof (Characteristics));

  Characteristics.ProcessorArm64SocId = HasSmcArm64SocId ();

  return Characteristics;
}
