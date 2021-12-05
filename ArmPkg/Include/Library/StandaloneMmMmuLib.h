/** @file

  Copyright (c) 2018, ARM Ltd. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef STANDALONE_MM_MMU_LIB_
#define STANDALONE_MM_MMU_LIB_

EFI_STATUS
ArmSetMemoryRegionNoExec (
  IN  EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN  UINT64                Length
  );

EFI_STATUS
ArmClearMemoryRegionNoExec (
  IN  EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN  UINT64                Length
  );

EFI_STATUS
ArmSetMemoryRegionReadOnly (
  IN  EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN  UINT64                Length
  );

EFI_STATUS
ArmClearMemoryRegionReadOnly (
  IN  EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN  UINT64                Length
  );

#endif /* STANDALONE_MM_MMU_LIB_ */
