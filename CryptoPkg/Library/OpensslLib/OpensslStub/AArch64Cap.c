/** @file
  Arm capabilities probing.

  Copyright (c) 2023, Arm Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <openssl/types.h>
#include "crypto/arm_arch.h"

#include <Library/ArmLib.h>

/** Get bits from a value.

  Shift the input value from 'shift' bits and apply 'mask'.

  @param   value    The value to get the bits from.
  @param   shift    Index of the bits to read.
  @param   mask     Mask to apply to the value once shifted.

  @return  The desired bitfield from the value.
**/
#define GET_BITFIELD(value, shift, mask)    \
  ((value >> shift) & mask)

UINT32  OPENSSL_armcap_P = 0;

void
OPENSSL_cpuid_setup (
  void
  )
{
  OPENSSL_armcap_P = 0;

  /* Access to EL0 registers is possible from higher ELx. */
  OPENSSL_armcap_P |= ARMV8_CPUID;
  /* Access to Physical timer is possible. */
  OPENSSL_armcap_P |= ARMV7_TICK;

  /* Neon support is not guaranteed, but it is assumed to be present.
     Arm ARM for Armv8, sA1.5 Advanced SIMD and floating-point support
  */
  OPENSSL_armcap_P |= ARMV7_NEON;

  if (ArmHasAes ())
  {
    OPENSSL_armcap_P |= ARMV8_AES;
  }

  if (ArmHasSha1 ())
  {
    OPENSSL_armcap_P |= ARMV8_SHA1;
  }

  if (ArmHasSha256 ())
  {
    OPENSSL_armcap_P |= ARMV8_SHA256;
  }

  if (ArmHasPmull ())
  {
    OPENSSL_armcap_P |= ARMV8_PMULL;
  }

  if (ArmHasSha512 ())
  {
    OPENSSL_armcap_P |= ARMV8_SHA512;
  }
}

/** Read system counter value.

  Used to get some non-trusted entropy.

  @return Lower bits of the physical counter.
**/
uint32_t
OPENSSL_rdtsc (
  void
  )
{
  return (UINT32)ArmReadCntPct ();
}
