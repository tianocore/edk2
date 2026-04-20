/** @file
  Provide functions to read or write FS base register.

  Copyright (c) 2026, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "BaseLibInternals.h"
#include <Register/Intel/Cpuid.h>

/**
  Worker function that returns TRUE if the CPU supports FS/GS base instructions,
  FALSE otherwise.

  @retval TRUE  The CPU supports FS/GS base instructions.
  @retval FALSE The CPU does not support FS/GS base instructions.
**/
BOOLEAN
InternalFsGsBaseSupported (
  VOID
  )
{
  UINT32                                       MaxLeaf;
  CPUID_STRUCTURED_EXTENDED_FEATURE_FLAGS_EBX  Ebx;

  AsmCpuid (CPUID_SIGNATURE, &MaxLeaf, NULL, NULL, NULL);
  if (MaxLeaf < CPUID_STRUCTURED_EXTENDED_FEATURE_FLAGS) {
    return FALSE;
  }

  AsmCpuidEx (
    CPUID_STRUCTURED_EXTENDED_FEATURE_FLAGS,
    CPUID_STRUCTURED_EXTENDED_FEATURE_FLAGS_SUB_LEAF_INFO,
    NULL,
    &Ebx.Uint32,
    NULL,
    NULL
    );
  return (BOOLEAN)(Ebx.Bits.FSGSBASE == 1);
}

/**
  Worker function that returns TRUE if the CPU enables FS/GS base instructions,
  FALSE otherwise.

  @retval TRUE  The CPU enables FS/GS base instructions.
  @retval FALSE The CPU does not enable FS/GS base instructions.
**/
BOOLEAN
InternalFsGsBaseEnabled (
  VOID
  )
{
  IA32_CR4  Cr4;

  Cr4.UintN = AsmReadCr4 ();
  return (BOOLEAN)(Cr4.Bits.FSGSBASE == 1);
}

/**
  Reads the current value of the FS segment base address.

  Reads and returns the current value of the FS segment base address using
  the RDFSBASE instruction. This function is only available on X64.

  Note: The function requires that CPUID.(EAX=7,ECX=0):EBX.FSGSBASE=1
  and CR4.FSGSBASE=1.

  @return The current value of the FS segment base address.
**/
UINT64
EFIAPI
AsmReadFsBase (
  VOID
  )
{
  ASSERT (InternalFsGsBaseSupported ());
  ASSERT (InternalFsGsBaseEnabled ());

  return InternalX86ReadFsBase ();
}

/**
  Writes a value to the FS segment base address.

  Writes FsBase to the FS segment base address register using the WRFSBASE
  instruction. This function is only available on X64.

  Note: The function requires that CPUID.(EAX=7,ECX=0):EBX.FSGSBASE=1
  and CR4.FSGSBASE=1.

  @param  FsBase  The value to write to the FS segment base address.

  @return  FsBase
**/
UINT64
EFIAPI
AsmWriteFsBase (
  IN      UINT64  FsBase
  )
{
  ASSERT (InternalFsGsBaseSupported ());
  ASSERT (InternalFsGsBaseEnabled ());

  InternalX86WriteFsBase (FsBase);
  return FsBase;
}
