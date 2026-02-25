/** @file
  VortexOracle Protocol definition.

  Copyright (c) 2026, Americo Simoes. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __VORTEX_ORACLE_PROTOCOL_H__
#define __VORTEX_ORACLE_PROTOCOL_H__

#define VORTEX_ORACLE_PROTOCOL_GUID \
  { \
    0x8f2269a8, 0x867c, 0x4871, { 0x98, 0x22, 0x2b, 0x1a, 0x3d, 0x42, 0xc1, 0x90 } \
  }

typedef struct _VORTEX_ORACLE_PROTOCOL VORTEX_ORACLE_PROTOCOL;

/**
  Retrieves the current CTT Resonance value.

  @param[in]  This            Pointer to the VORTEX_ORACLE_PROTOCOL instance.
  @param[out] ResonanceValue  Pointer to the UINT64 where the value will be stored.

  @retval EFI_SUCCESS           The resonance value was successfully retrieved.
  @retval EFI_INVALID_PARAMETER ResonanceValue is NULL.

**/
typedef
EFI_STATUS
(EFIAPI *VORTEX_GET_RESONANCE)(
  IN  VORTEX_ORACLE_PROTOCOL  *This,
  OUT UINT64                  *ResonanceValue
  );

struct _VORTEX_ORACLE_PROTOCOL {
  VORTEX_GET_RESONANCE    GetVortexResonance;
};

extern EFI_GUID  gVortexOracleProtocolGuid;

#endif
