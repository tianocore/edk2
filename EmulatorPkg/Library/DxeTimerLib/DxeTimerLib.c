/** @file
  A non-functional instance of the Timer Library.

  Copyright (c) 2007 - 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Library/BaseLib.h>
#include <Library/TimerLib.h>
#include <Library/DebugLib.h>
#include <Library/EmuThunkLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>

#include <Protocol/Timer.h>

STATIC UINT64                   gTimerPeriod   = 0;
STATIC EFI_TIMER_ARCH_PROTOCOL  *gTimerAp      = NULL;
STATIC EFI_EVENT                gTimerEvent    = NULL;
STATIC VOID                     *gRegistration = NULL;

VOID
EFIAPI
RegisterTimerArchProtocol (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS  Status;

  Status = gBS->LocateProtocol (&gEfiTimerArchProtocolGuid, NULL, (VOID **)&gTimerAp);
  if (!EFI_ERROR (Status)) {
    Status = gTimerAp->GetTimerPeriod (gTimerAp, &gTimerPeriod);
    ASSERT_EFI_ERROR (Status);

    // Convert to Nanoseconds.
    gTimerPeriod = MultU64x32 (gTimerPeriod, 100);

    if (gTimerEvent == NULL) {
      Status = gBS->CreateEvent (EVT_TIMER, 0, NULL, NULL, &gTimerEvent);
      ASSERT_EFI_ERROR (Status);
    }
  }
}

/**
  Stalls the CPU for at least the given number of microseconds.

  Stalls the CPU for the number of microseconds specified by MicroSeconds.

  @param  MicroSeconds  The minimum number of microseconds to delay.

  @return The value of MicroSeconds inputted.

**/
UINTN
EFIAPI
MicroSecondDelay (
  IN      UINTN  MicroSeconds
  )
{
  return NanoSecondDelay (MicroSeconds * 1000);
}

/**
  Stalls the CPU for at least the given number of nanoseconds.

  Stalls the CPU for the number of nanoseconds specified by NanoSeconds.

  @param  NanoSeconds The minimum number of nanoseconds to delay.

  @return The value of NanoSeconds inputted.

**/
UINTN
EFIAPI
NanoSecondDelay (
  IN      UINTN  NanoSeconds
  )
{
  EFI_STATUS  Status;
  UINT64      HundredNanoseconds;
  UINTN       Index;

  if ((gTimerPeriod != 0) &&
      ((UINT64)NanoSeconds > gTimerPeriod) &&
      (EfiGetCurrentTpl () == TPL_APPLICATION))
  {
    //
    // This stall is long, so use gBS->WaitForEvent () to yield CPU to DXE Core
    //

    HundredNanoseconds = DivU64x32 (NanoSeconds, 100);
    Status             = gBS->SetTimer (gTimerEvent, TimerRelative, HundredNanoseconds);
    ASSERT_EFI_ERROR (Status);

    Status = gBS->WaitForEvent (sizeof (gTimerEvent)/sizeof (EFI_EVENT), &gTimerEvent, &Index);
    ASSERT_EFI_ERROR (Status);
  } else {
    gEmuThunk->Sleep (NanoSeconds);
  }

  return NanoSeconds;
}

/**
  Retrieves the current value of a 64-bit free running performance counter.

  The counter can either count up by 1 or count down by 1. If the physical
  performance counter counts by a larger increment, then the counter values
  must be translated. The properties of the counter can be retrieved from
  GetPerformanceCounterProperties().

  @return The current value of the free running performance counter.

**/
UINT64
EFIAPI
GetPerformanceCounter (
  VOID
  )
{
  return gEmuThunk->QueryPerformanceCounter ();
}

/**
  Retrieves the 64-bit frequency in Hz and the range of performance counter
  values.

  If StartValue is not NULL, then the value that the performance counter starts
  with immediately after is it rolls over is returned in StartValue. If
  EndValue is not NULL, then the value that the performance counter end with
  immediately before it rolls over is returned in EndValue. The 64-bit
  frequency of the performance counter in Hz is always returned. If StartValue
  is less than EndValue, then the performance counter counts up. If StartValue
  is greater than EndValue, then the performance counter counts down. For
  example, a 64-bit free running counter that counts up would have a StartValue
  of 0 and an EndValue of 0xFFFFFFFFFFFFFFFF. A 24-bit free running counter
  that counts down would have a StartValue of 0xFFFFFF and an EndValue of 0.

  @param  StartValue  The value the performance counter starts with when it
                      rolls over.
  @param  EndValue    The value that the performance counter ends with before
                      it rolls over.

  @return The frequency in Hz.

**/
UINT64
EFIAPI
GetPerformanceCounterProperties (
  OUT      UINT64  *StartValue   OPTIONAL,
  OUT      UINT64  *EndValue     OPTIONAL
  )
{
  if (StartValue != NULL) {
    *StartValue = 0ULL;
  }

  if (EndValue != NULL) {
    *EndValue = (UINT64)-1LL;
  }

  return gEmuThunk->QueryPerformanceFrequency ();
}

/**
  Register for the Timer AP protocol.

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS   The constructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
DxeTimerLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EfiCreateProtocolNotifyEvent (
    &gEfiTimerArchProtocolGuid,
    TPL_CALLBACK,
    RegisterTimerArchProtocol,
    NULL,
    &gRegistration
    );

  return EFI_SUCCESS;
}

/**
  Converts elapsed ticks of performance counter to time in nanoseconds.

  This function converts the elapsed ticks of running performance counter to
  time value in unit of nanoseconds.

  @param  Ticks     The number of elapsed ticks of running performance counter.

  @return The elapsed time in nanoseconds.

**/
UINT64
EFIAPI
GetTimeInNanoSecond (
  IN UINT64  Ticks
  )
{
  UINT64  Frequency;
  UINT64  NanoSeconds;
  UINT64  Remainder;
  INTN    Shift;

  Frequency = GetPerformanceCounterProperties (NULL, NULL);

  //
  //          Ticks
  // Time = --------- x 1,000,000,000
  //        Frequency
  //
  NanoSeconds = MultU64x32 (DivU64x64Remainder (Ticks, Frequency, &Remainder), 1000000000u);

  //
  // Ensure (Remainder * 1,000,000,000) will not overflow 64-bit.
  // Since 2^29 < 1,000,000,000 = 0x3B9ACA00 < 2^30, Remainder should < 2^(64-30) = 2^34,
  // i.e. highest bit set in Remainder should <= 33.
  //
  Shift        = MAX (0, HighBitSet64 (Remainder) - 33);
  Remainder    = RShiftU64 (Remainder, (UINTN)Shift);
  Frequency    = RShiftU64 (Frequency, (UINTN)Shift);
  NanoSeconds += DivU64x64Remainder (MultU64x32 (Remainder, 1000000000u), Frequency, NULL);

  return NanoSeconds;
}
