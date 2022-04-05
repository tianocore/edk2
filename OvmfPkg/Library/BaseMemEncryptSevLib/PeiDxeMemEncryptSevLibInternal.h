/** @file

  Secure Encrypted Virtualization (SEV) library helper function

  Copyright (c) 2021, AMD Incorporated. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef PEI_DXE_MEM_ENCRYPT_SEV_LIB_INTERNAL_H_
#define PEI_DXE_MEM_ENCRYPT_SEV_LIB_INTERNAL_H_

#include <Library/BaseLib.h>

#define KVM_FEATURE_MIGRATION_CONTROL  BIT17

/**
  Figures out if we are running inside KVM HVM and
  KVM HVM supports SEV Live Migration feature.

  @retval TRUE           SEV live migration is supported.
  @retval FALSE          SEV live migration is not supported.
**/
BOOLEAN
EFIAPI
KvmDetectSevLiveMigrationFeature (
  VOID
  );

#endif // PEI_DXE_MEM_ENCRYPT_SEV_LIB_INTERNAL_H_
