/** @file
  This file declares Migrate Temporary Memory PPI.

  This PPI is published by the PEI Foundation when temporary RAM needs to evacuate.
  Its purpose is to be used as a signal for other PEIMs who can register for a
  notification on its installation.

  Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef PEI_MIGRATE_TEMP_RAM_PPI_H_
#define PEI_MIGRATE_TEMP_RAM_PPI_H_

#define EFI_PEI_MIGRATE_TEMP_RAM_PPI_GUID \
  { \
    0xc79dc53b, 0xafcd, 0x4a6a, {0xad, 0x94, 0xa7, 0x6a, 0x3f, 0xa9, 0xe9, 0xc2 } \
  }

extern EFI_GUID  gEdkiiPeiMigrateTempRamPpiGuid;

#endif
