/** @file
  Random number generator services.

Copyright (c) 2026, Loongson Technology Corporation Limited. All rights reserved.<BR>
Copyright (C) 2024 Advanced Micro Devices, Inc. All rights reserved.<BR>
Copyright (c) 2023, Arm Limited. All rights reserved.<BR>
Copyright (c) 2022, Pedro Falcato. All rights reserved.<BR>
Copyright (c) 2021, NUVIA Inc. All rights reserved.<BR>
Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>

#include "BaseRngLibInternals.h"

BOOLEAN
EFIAPI
AsmRdRng16 (
  OUT  UINT16 *
  );

BOOLEAN
EFIAPI
AsmRdRng32 (
  OUT  UINT32 *
  );

BOOLEAN
EFIAPI
AsmRdRng64 (
  OUT  UINT64 *
  );

/**
  The constructor function checks whether or not RNG functions is supported
  by the host hardware.

  It will always return EFI_SUCCESS because LoongArch64 currently uses the
  `rdtime` instruction.

  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
BaseRngLibConstructor (
  VOID
  )
{
  return EFI_SUCCESS;
}

/**
  Generates a 16-bit random number.

  @param[out] Rand     Buffer pointer to store the 16-bit random value.

  @retval TRUE         Random number generated successfully.
  @retval FALSE        Failed to generate the random number.

**/
BOOLEAN
EFIAPI
ArchGetRandomNumber16 (
  OUT     UINT16  *Rand
  )
{
  return AsmRdRng16 (Rand);
}

/**
  Generates a 32-bit random number.

  @param[out] Rand     Buffer pointer to store the 32-bit random value.

  @retval TRUE         Random number generated successfully.
  @retval FALSE        Failed to generate the random number.

**/
BOOLEAN
EFIAPI
ArchGetRandomNumber32 (
  OUT     UINT32  *Rand
  )
{
  return AsmRdRng32 (Rand);
}

/**
  Generates a 64-bit random number.

  @param[out] Rand     Buffer pointer to store the 64-bit random value.

  @retval TRUE         Random number generated successfully.
  @retval FALSE        Failed to generate the random number.

**/
BOOLEAN
EFIAPI
ArchGetRandomNumber64 (
  OUT     UINT64  *Rand
  )
{
  return AsmRdRng64 (Rand);
}

/**
  Checks whether RNG is supported.

  Currently, LoongArch64 RNG uses the `rdtime` instruction, so it is
  always supported.

  @retval TRUE         RDRAND is supported.
  @retval FALSE        RDRAND is not supported.

**/
BOOLEAN
EFIAPI
ArchIsRngSupported (
  VOID
  )
{
  return TRUE;
}

/**
  Get a GUID identifying the RNG algorithm implementation.

  LoongArch64 currently does not supported the HW or standards-compliant
  RNG. Therefore only BaseRngLib is implemented as a best-effort entropy
  source, and no EFI_RNG_PROTOCOL or algorithm GUID is exposed.

  @param [out] RngGuid  If success, contains the GUID identifying
                        the RNG algorithm implementation.

  @retval EFI_SUCCESS             Success.
  @retval EFI_UNSUPPORTED         Not supported.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
EFI_STATUS
EFIAPI
GetRngGuid (
  GUID  *RngGuid
  )
{
  return EFI_UNSUPPORTED;
}
