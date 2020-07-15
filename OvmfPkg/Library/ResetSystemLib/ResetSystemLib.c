/** @file
  Reset System Library functions for OVMF

  Copyright (c) 2006 - 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>                   // BIT1

#include <Library/BaseLib.h>        // CpuDeadLoop()
#include <Library/IoLib.h>          // IoWrite8()
#include <Library/ResetSystemLib.h> // ResetCold()
#include <Library/TimerLib.h>       // MicroSecondDelay()

/**
  Calling this function causes a system-wide reset. This sets
  all circuitry within the system to its initial state. This type of reset
  is asynchronous to system operation and operates without regard to
  cycle boundaries.

  System reset should not return, if it returns, it means the system does
  not support cold reset.
**/
VOID
EFIAPI
ResetCold (
  VOID
  )
{
  IoWrite8 (0xCF9, BIT2 | BIT1); // 1st choice: PIIX3 RCR, RCPU|SRST
  MicroSecondDelay (50);

  IoWrite8 (0x64, 0xfe);         // 2nd choice: keyboard controller
  CpuDeadLoop ();
}

/**
  Calling this function causes a system-wide initialization. The processors
  are set to their initial state, and pending cycles are not corrupted.

  System reset should not return, if it returns, it means the system does
  not support warm reset.
**/
VOID
EFIAPI
ResetWarm (
  VOID
  )
{
  IoWrite8 (0x64, 0xfe);
  CpuDeadLoop ();
}


/**
  This function causes a systemwide reset. The exact type of the reset is
  defined by the EFI_GUID that follows the Null-terminated Unicode string
  passed into ResetData. If the platform does not recognize the EFI_GUID in
  ResetData the platform must pick a supported reset type to perform.The
  platform may optionally log the parameters from any non-normal reset that
  occurs.

  @param[in]  DataSize   The size, in bytes, of ResetData.
  @param[in]  ResetData  The data buffer starts with a Null-terminated string,
                         followed by the EFI_GUID.
**/
VOID
EFIAPI
ResetPlatformSpecific (
  IN UINTN   DataSize,
  IN VOID    *ResetData
  )
{
  ResetCold ();
}

/**
  The ResetSystem function resets the entire platform.

  @param[in] ResetType      The type of reset to perform.
  @param[in] ResetStatus    The status code for the reset.
  @param[in] DataSize       The size, in bytes, of ResetData.
  @param[in] ResetData      For a ResetType of EfiResetCold, EfiResetWarm, or
                            EfiResetShutdown the data buffer starts with a
                            Null-terminated string, optionally followed by
                            additional binary data. The string is a description
                            that the caller may use to further indicate the
                            reason for the system reset.
**/
VOID
EFIAPI
ResetSystem (
  IN EFI_RESET_TYPE               ResetType,
  IN EFI_STATUS                   ResetStatus,
  IN UINTN                        DataSize,
  IN VOID                         *ResetData OPTIONAL
  )
{
  switch (ResetType) {
  case EfiResetWarm:
    ResetWarm ();
    break;

  case EfiResetCold:
    ResetCold ();
    break;

  case EfiResetShutdown:
    ResetShutdown ();
    break;

  case EfiResetPlatformSpecific:
    ResetPlatformSpecific (DataSize, ResetData);
    break;

  default:
    break;
  }
}
