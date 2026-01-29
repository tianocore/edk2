/** @file
  EDKII Device Security library for SPDM device.
  It follows the SPDM Specification.

Copyright (c) 2024, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include "hal/base.h"

/**
 * Suspends the execution of the current thread until the time-out interval elapses.
 *
 * @param milliseconds     The time interval for which execution is to be suspended, in milliseconds.
 *
 **/
void
libspdm_sleep (
  uint64_t  milliseconds
  )
{
  return;
}

/**
 * Suspends the execution of the current thread until the time-out interval elapses.
 *
 * @param microseconds     The time interval for which execution is to be suspended, in milliseconds.
 *
 **/
void
libspdm_sleep_in_us (
  uint64_t  microseconds
  )
{
  return;
}

/**
 * If no heartbeat arrives in seconds, the watchdog timeout event
 * should terminate the session.
 *
 * @param  session_id     Indicate the SPDM session ID.
 * @param  seconds        heartbeat period, in seconds.
 *
 **/
bool
libspdm_start_watchdog (
  uint32_t  session_id,
  uint16_t  seconds
  )
{
  return true;
}

/**
 * stop watchdog.
 *
 * @param  session_id     Indicate the SPDM session ID.
 *
 **/
bool
libspdm_stop_watchdog (
  uint32_t  session_id
  )
{
  return true;
}

/**
 * Reset the watchdog in heartbeat response.
 *
 * @param  session_id     Indicate the SPDM session ID.
 *
 **/
bool
libspdm_reset_watchdog (
  uint32_t  session_id
  )
{
  return true;
}
