/**
**/
/**

Copyright (c) 2012  - 2014, Intel Corporation. All rights reserved

  SPDX-License-Identifier: BSD-2-Clause-Patent


  @file
  PchPlatformLib.h

  @brief
  Header file for PchPlatform Lib.

**/
#ifndef _PCH_PLATFORM_LIB_H_
#define _PCH_PLATFORM_LIB_H_

///
/// Timeout value used when Sending / Receiving messages.
/// NOTE: this must cover the longest possible wait time
/// between message being sent and response being available.
/// e.g. Virtual function readiness might take some time.
///
VOID
EFIAPI
PchPmTimerStall (
  IN  UINTN   Microseconds
  )
/**

  @brief
  Delay for at least the request number of microseconds.
  This function would be called by runtime driver, please do not use any MMIO marco here.

  @param[in] Microseconds         Number of microseconds to delay.

  @retval NONE

**/
;

BOOLEAN
EFIAPI
PchIsSpiDescriptorMode (
  IN  UINTN   SpiBase
  )
/**

  @brief
  Check whether SPI is in descriptor mode

  @param[in] SpiBase              The PCH Spi Base Address

  @retval TRUE                    SPI is in descriptor mode
  @retval FALSE                   SPI is not in descriptor mode

**/
;

PCH_STEPPING
EFIAPI
PchStepping (
  VOID
  )
/**

  @brief
  Return Pch stepping type

  @param[in] None

  @retval PCH_STEPPING            Pch stepping type

**/
;

BOOLEAN
IsPchSupported (
  VOID
  )
/**

  @brief
  Determine if PCH is supported

  @param[in] None

  @retval TRUE                    PCH is supported
  @retval FALSE                   PCH is not supported

**/
;

VOID
EFIAPI
PchAlternateAccessMode (
  IN  UINTN         IlbBase,
  IN  BOOLEAN       AmeCtrl
  )
/**

  This function can be called to enable/disable Alternate Access Mode

  @param[in] IlbBase              The PCH ILB Base Address
  @param[in] AmeCtrl              If TRUE, enable Alternate Access Mode.
                                  If FALSE, disable Alternate Access Mode.

  @retval NONE

**/
;

#endif
