/** @file

  Copyright (c) 2024 Loongson Technology Corporation Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

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
