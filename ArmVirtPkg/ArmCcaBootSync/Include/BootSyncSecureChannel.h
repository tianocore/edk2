/** @file
  Arm CCA Boot Sync Secure Channel internal definitions and interfaces.

  Copyright (c) 2024, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Library/ArmCcaRsiLib.h>
#include <Library/ArmCcaRhiHostSessionLib.h>

/**
  A structure for storing the data relevant for the Secure session.
*/
typedef struct SecureChannel {
  /// RHI Session Connection Mode
  UINT64                    ConnectionMode;

  /// RHI Host Session Id
  ARM_CCA_RHI_SESSION_ID    SessionId;
} SECURE_CHANNEL;
