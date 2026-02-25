/** @file

  Copyright (c) 2024 Loongson Technology Corporation Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

/**
  Invalid corresponding TLB entries are based on the address given

  @param Address The address corresponding to the invalid page table entry

  @retval  none
**/
VOID
InvalidTlb (
  UINTN  Address
  );
