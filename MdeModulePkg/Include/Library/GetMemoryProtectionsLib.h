/** @file
Library for accessing the platform memory protection settings.

Copyright (c) Microsoft Corporation.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef GET_MEMORY_PROTECTION_SETTINGS_LIB_H_
#define GET_MEMORY_PROTECTION_SETTINGS_LIB_H_

#include <Library/BaseMemoryLib.h>
#include <Guid/MemoryProtectionSettings.h>

#pragma pack(1)

typedef union {
  DXE_MEMORY_PROTECTION_SETTINGS    Dxe;
  MM_MEMORY_PROTECTION_SETTINGS     Mm;
} MEMORY_PROTECTION_SETTINGS_UNION;

#pragma pack()

// The global used to access current Memory Protection Settings
extern MEMORY_PROTECTION_SETTINGS_UNION  gMps;

#define MPS_IS_DXE_SIGNATURE_VALID  (gMps.Dxe.Signature == DXE_MEMORY_PROTECTION_SIGNATURE)
#define MPS_IS_MM_SIGNATURE_VALID   (gMps.Mm.Signature == MM_MEMORY_PROTECTION_SIGNATURE)

#define IS_DXE_PAGE_GUARD_ACTIVE  (MPS_IS_DXE_SIGNATURE_VALID                                                     &&  \
                                  !IsZeroBuffer (&gMps.Dxe.PageGuard.EnabledForType, MPS_MEMORY_TYPE_BUFFER_SIZE) &&  \
                                  gMps.Dxe.HeapGuard.PageGuardEnabled)

#define IS_DXE_POOL_GUARD_ACTIVE  (MPS_IS_DXE_SIGNATURE_VALID                                                     &&  \
                                  !IsZeroBuffer (&gMps.Dxe.PoolGuard.EnabledForType, MPS_MEMORY_TYPE_BUFFER_SIZE) &&  \
                                  gMps.Dxe.HeapGuard.PoolGuardEnabled)

#define IS_DXE_EXECUTION_PROTECTION_ACTIVE  (MPS_IS_DXE_SIGNATURE_VALID                                           &&  \
                                            !IsZeroBuffer (&gMps.Dxe.ExecutionProtection.EnabledForType, MPS_MEMORY_TYPE_BUFFER_SIZE))

#define IS_DXE_IMAGE_PROTECTION_ACTIVE  (MPS_IS_DXE_SIGNATURE_VALID                         &&  \
                                        (gMps.Dxe.ImageProtection.ProtectImageFromFv        ||  \
                                         gMps.Dxe.ImageProtection.ProtectImageFromUnknown))

#define IS_DXE_MEMORY_PROTECTION_ACTIVE  (MPS_IS_DXE_SIGNATURE_VALID                  &&  \
                                         (IS_DXE_PAGE_GUARD_ACTIVE                    ||  \
                                          IS_DXE_POOL_GUARD_ACTIVE                    ||  \
                                          IS_DXE_EXECUTION_PROTECTION_ACTIVE          ||  \
                                          IS_DXE_IMAGE_PROTECTION_ACTIVE              ||  \
                                          gMps.Dxe.CpuStackGuardEnabled               ||  \
                                          gMps.Dxe.StackExecutionProtectionEnabled    ||  \
                                          gMps.Dxe.NullPointerDetection.Enabled       ||  \
                                          gMps.Dxe.HeapGuard.FreedMemoryGuardEnabled))

#define IS_MM_PAGE_GUARD_ACTIVE  (MPS_IS_MM_SIGNATURE_VALID                                                       &&  \
                                   gMps.Mm.HeapGuard.PageGuardEnabled                                             &&  \
                                   !IsZeroBuffer (&gMps.Mm.PageGuard.EnabledForType, MPS_MEMORY_TYPE_BUFFER_SIZE))

#define IS_MM_POOL_GUARD_ACTIVE  (MPS_IS_MM_SIGNATURE_VALID                                                       &&  \
                                  gMps.Mm.HeapGuard.PoolGuardEnabled                                              &&  \
                                  !IsZeroBuffer (&gMps.Mm.PoolGuard.EnabledForType, MPS_MEMORY_TYPE_BUFFER_SIZE))

#define IS_MM_MEMORY_PROTECTION_ACTIVE  (MPS_IS_MM_SIGNATURE_VALID              &&  \
                                        (IS_MM_PAGE_GUARD_ACTIVE                ||  \
                                         IS_MM_POOL_GUARD_ACTIVE                ||  \
                                         gMps.Mm.NullPointerDetection.Enabled));

/**
  Populates gMps global. This function is invoked by the library constructor and only needs to be
  called if library contructors have not yet been invoked.

  @retval EFI_SUCCESS       gMps global was populated.
  @retval EFI_NOT_FOUND     The gMemoryProtectionSettingsGuid HOB was not found.
  @retval EFI_ABORTED       The version number of the DXE or MM memory protection settings was invalid.
  @retval EFI_UNSUPPORTED   NULL implementation called.
**/
EFI_STATUS
EFIAPI
PopulateMpsGlobal (
  VOID
  );

#endif
