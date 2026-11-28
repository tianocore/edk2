/** @file
  Generic class protocol for published real-time clocks.

  Copyright (c) 2026, Arm Ltd. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Uefi/UefiBaseType.h>

/** Identifies a published real-time clock using a NULL interface.

  Installed on the device handle alongside its prepared resource protocol.
  Its presence does not imply that runtime time services have been initialized.
  Unselected candidates do not carry this protocol. Available only before
  ExitBootServices().
**/
extern EFI_GUID  gEdkiiUfdtGenericRtcProtocolGuid;
