/** @file

  Copyright (c) 2024 Loongson Technology Corporation Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef INVALID_TLB_H_
#define INVALID_TLB_H_

/**
  Invalid corresponding TLB entries are based on the address given

  @param Address The address corresponding to the invalid page table entry

  @retval  none
**/
VOID
InvalidTlb (
  UINTN  Address
  );

#endif // INVALID_TLB_H_
