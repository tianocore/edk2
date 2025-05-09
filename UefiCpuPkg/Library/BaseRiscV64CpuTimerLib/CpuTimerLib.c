/** @file
  RISC-V instance of Timer Library.

  Copyright (c) 2016 - 2022, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Register/RiscV64/RiscVImpl.h>
#include <Pi/PiBootMode.h>
#include <Pi/PiHob.h>
#include <Library/HobLib.h>
#include <Library/FdtLib.h>

// Timer base retrieved from DT
STATIC UINT32  mTimeBase;

/**
  Stalls the CPU for at least the given number of ticks.

  Stalls the CPU for at least the given number of ticks. It's invoked by
  MicroSecondDelay() and NanoSecondDelay().

  @param  Delay     A period of time to delay in ticks.

**/
STATIC
VOID
InternalRiscVTimerDelay (
  IN UINT64  Delay
  )
{
  UINT64  Ticks;

  Ticks = RiscVReadTimer () + Delay;

  while (RiscVReadTimer () <= Ticks) {
    CpuPause ();
  }
}

/**
  Stalls the CPU for at least the given number of microseconds.

  Stalls the CPU for the number of microseconds specified by MicroSeconds.

  @param  MicroSeconds  The minimum number of microseconds to delay.

  @return MicroSeconds

**/
UINTN
EFIAPI
MicroSecondDelay (
  IN UINTN  MicroSeconds
  )
{
  InternalRiscVTimerDelay (
    DivU64x32 (
      MultU64x32 (
        MicroSeconds,
        mTimeBase
        ),
      1000000u
      )
    );
  return MicroSeconds;
}

/**
  Stalls the CPU for at least the given number of nanoseconds.

  Stalls the CPU for the number of nanoseconds specified by NanoSeconds.

  @param  NanoSeconds The minimum number of nanoseconds to delay.

  @return NanoSeconds

**/
UINTN
EFIAPI
NanoSecondDelay (
  IN UINTN  NanoSeconds
  )
{
  InternalRiscVTimerDelay (
    DivU64x32 (
      MultU64x32 (
        NanoSeconds,
        mTimeBase
        ),
      1000000000u
      )
    );
  return NanoSeconds;
}

/**
  Retrieves the current value of a 64-bit free running performance counter.

  Retrieves the current value of a 64-bit free running performance counter. The
  counter can either count up by 1 or count down by 1. If the physical
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
  return (UINT64)RiscVReadTimer ();
}

/**return
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
  OUT      UINT64 *StartValue, OPTIONAL
  OUT      UINT64                    *EndValue     OPTIONAL
  )
{
  if (StartValue != NULL) {
    *StartValue = 0;
  }

  if (EndValue != NULL) {
    *EndValue = 32 - 1;
  }

  return mTimeBase;
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
  IN      UINT64  Ticks
  )
{
  UINT64  NanoSeconds;
  UINT32  Remainder;

  //
  //          Ticks
  // Time = --------- x 1,000,000,000
  //        Frequency
  //
  NanoSeconds = MultU64x32 (DivU64x32Remainder (Ticks, mTimeBase, &Remainder), 1000000000u);

  //
  // Frequency < 0x100000000, so Remainder < 0x100000000, then (Remainder * 1,000,000,000)
  // will not overflow 64-bit.
  //
  NanoSeconds += DivU64x32 (MultU64x32 ((UINT64)Remainder, 1000000000u), mTimeBase);

  return NanoSeconds;
}

/**
  Retrieve the CPU time-base frequency from the Device Tree and cache it.

  @retval UINT32  Time-base frequency in Hz.
**/
UINT32
EFIAPI
GetDTTimerFreq (
  VOID
  )
{
  if (mTimeBase != 0) {
    return mTimeBase;
  }

  //
  // Locate the FDT HOB and validate header
  //
  CONST EFI_HOB_GUID_TYPE  *Hob = GetFirstGuidHob (&gFdtHobGuid);

  ASSERT (Hob != NULL);

  CONST VOID  *DeviceTreeBase =
    (CONST VOID *)(UINTN)*(CONST UINT64 *)GET_GUID_HOB_DATA (Hob);

  ASSERT (FdtCheckHeader (DeviceTreeBase) == 0);

  //
  // /cpus node
  //
  INT32  Node = FdtSubnodeOffsetNameLen (
                  DeviceTreeBase,
                  0,
                  "cpus",
                  sizeof ("cpus") - 1
                  );

  ASSERT (Node >= 0);

  //
  // timebase-frequency property
  //
  INT32               Len;
  CONST FDT_PROPERTY  *Prop =
    FdtGetProperty (DeviceTreeBase, Node, "timebase-frequency", &Len);

  ASSERT (Prop != NULL && Len == sizeof (UINT32));

  //
  // Device-tree cells are big-endian
  //
  mTimeBase = SwapBytes32 (*(CONST UINT32 *)Prop->Data);
  return mTimeBase;
}
