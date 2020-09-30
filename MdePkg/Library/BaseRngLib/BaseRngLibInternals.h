/** @file

  Architecture specific interface to RNG functionality.

Copyright (c) 2020, NUVIA Inc. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __BASE_RNGLIB_INTERNALS_H__

BOOLEAN
EFIAPI
ArchGetRandomNumber16 (
  OUT UINT16 *Rand
  );

BOOLEAN
EFIAPI
ArchGetRandomNumber32 (
  OUT UINT32 *Rand
  );

BOOLEAN
EFIAPI
ArchGetRandomNumber64 (
  OUT UINT64 *Rand
  );

#endif    // __BASE_RNGLIB_INTERNALS_H__
