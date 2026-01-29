/** @file

  Copyright (c) 2024 Loongson Technology Corporation Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef TLB_EXCEPTION_HANDLE_H_
#define TLB_EXCEPTION_HANDLE_H_

/**
  TLB refill handler start.

  @param  none

  @retval none
**/
VOID
HandleTlbRefillStart (
  VOID
  );

/**
  TLB refill handler end.

  @param  none

  @retval none
**/
VOID
HandleTlbRefillEnd (
  VOID
  );

#endif // TLB_EXCEPTION_HANDLE_H_
