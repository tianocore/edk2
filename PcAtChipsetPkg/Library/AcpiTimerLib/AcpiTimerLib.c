/** @file
  ACPI Timer implements one instance of Timer Library.

  Copyright (c) 2013 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Library/TimerLib.h>
#include <Library/BaseLib.h>
#include <Library/PcdLib.h>
#include <Library/PciLib.h>
#include <Library/IoLib.h>
#include <Library/DebugLib.h>
#include <IndustryStandard/Acpi.h>

GUID mFrequencyHobGuid = { 0x3fca54f6, 0xe1a2, 0x4b20, { 0xbe, 0x76, 0x92, 0x6b, 0x4b, 0x48, 0xbf, 0xaa }};

/**
  Internal function to retrieves the 64-bit frequency in Hz.

  Internal function to retrieves the 64-bit frequency in Hz.

  @return The frequency in Hz.

**/
UINT64
InternalGetPerformanceCounterFrequency (
  VOID
  );

/**
  The constructor function enables ACPI IO space.

  If ACPI I/O space not enabled, this function will enable it.
  It will always return RETURN_SUCCESS.

  @retval EFI_SUCCESS   The constructor always returns RETURN_SUCCESS.

**/
RETURN_STATUS
EFIAPI
AcpiTimerLibConstructor (
  VOID
  )
{
  UINTN   Bus;
  UINTN   Device;
  UINTN   Function;
  UINTN   EnableRegister;
  UINT8   EnableMask;

  //
  // ASSERT for the invalid PCD values. They must be configured to the real value.
  //
  ASSERT (PcdGet16 (PcdAcpiIoPciBarRegisterOffset) != 0xFFFF);
  ASSERT (PcdGet16 (PcdAcpiIoPortBaseAddress)      != 0xFFFF);

  //
  // If the register offset to the BAR for the ACPI I/O Port Base Address is 0x0000, then
  // no PCI register programming is required to enable access to the the ACPI registers
  // specified by PcdAcpiIoPortBaseAddress
  //
  if (PcdGet16 (PcdAcpiIoPciBarRegisterOffset) == 0x0000) {
    return RETURN_SUCCESS;
  }

  //
  // ASSERT for the invalid PCD values. They must be configured to the real value.
  //
  ASSERT (PcdGet8  (PcdAcpiIoPciDeviceNumber)   != 0xFF);
  ASSERT (PcdGet8  (PcdAcpiIoPciFunctionNumber) != 0xFF);
  ASSERT (PcdGet16 (PcdAcpiIoPciEnableRegisterOffset) != 0xFFFF);

  //
  // Retrieve the PCD values for the PCI configuration space required to program the ACPI I/O Port Base Address
  //
  Bus            = PcdGet8  (PcdAcpiIoPciBusNumber);
  Device         = PcdGet8  (PcdAcpiIoPciDeviceNumber);
  Function       = PcdGet8  (PcdAcpiIoPciFunctionNumber);
  EnableRegister = PcdGet16 (PcdAcpiIoPciEnableRegisterOffset);
  EnableMask     = PcdGet8  (PcdAcpiIoBarEnableMask);

  //
  // If ACPI I/O space is not enabled yet, program ACPI I/O base address and enable it.
  //
  if ((PciRead8 (PCI_LIB_ADDRESS (Bus, Device, Function, EnableRegister)) & EnableMask) != EnableMask) {
    PciWrite16 (
      PCI_LIB_ADDRESS (Bus, Device, Function, PcdGet16 (PcdAcpiIoPciBarRegisterOffset)),
      PcdGet16 (PcdAcpiIoPortBaseAddress)
      );
    PciOr8 (
      PCI_LIB_ADDRESS (Bus, Device, Function, EnableRegister),
      EnableMask
      );
  }

  return RETURN_SUCCESS;
}

/**
  Internal function to retrieve the ACPI I/O Port Base Address.

  Internal function to retrieve the ACPI I/O Port Base Address.

  @return The 16-bit ACPI I/O Port Base Address.

**/
UINT16
InternalAcpiGetAcpiTimerIoPort (
  VOID
  )
{
  UINT16  Port;

  Port = PcdGet16 (PcdAcpiIoPortBaseAddress);

  //
  // If the register offset to the BAR for the ACPI I/O Port Base Address is not 0x0000, then
  // read the PCI register for the ACPI BAR value in case the BAR has been programmed to a
  // value other than PcdAcpiIoPortBaseAddress
  //
  if (PcdGet16 (PcdAcpiIoPciBarRegisterOffset) != 0x0000) {
    Port = PciRead16 (PCI_LIB_ADDRESS (
                        PcdGet8  (PcdAcpiIoPciBusNumber),
                        PcdGet8  (PcdAcpiIoPciDeviceNumber),
                        PcdGet8  (PcdAcpiIoPciFunctionNumber),
                        PcdGet16 (PcdAcpiIoPciBarRegisterOffset)
                        ));
  }

  return (Port & PcdGet16 (PcdAcpiIoPortBaseAddressMask)) + PcdGet16 (PcdAcpiPm1TmrOffset);
}

/**
  Stalls the CPU for at least the given number of ticks.

  Stalls the CPU for at least the given number of ticks. It's invoked by
  MicroSecondDelay() and NanoSecondDelay().

  @param  Delay     A period of time to delay in ticks.

**/
VOID
InternalAcpiDelay (
  IN UINT32  Delay
  )
{
  UINT16   Port;
  UINT32   Ticks;
  UINT32   Times;

  Port   = InternalAcpiGetAcpiTimerIoPort ();
  Times  = Delay >> 22;
  Delay &= BIT22 - 1;
  do {
    //
    // The target timer count is calculated here
    //
    Ticks = IoBitFieldRead32 (Port, 0, 23) + Delay;
    Delay = BIT22;
    //
    // Wait until time out
    // Delay >= 2^23 could not be handled by this function
    // Timer wrap-arounds are handled correctly by this function
    //
    while (((Ticks - IoBitFieldRead32 (Port, 0, 23)) & BIT23) == 0) {
      CpuPause ();
    }
  } while (Times-- > 0);
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
  InternalAcpiDelay (
    (UINT32)DivU64x32 (
              MultU64x32 (
                MicroSeconds,
                ACPI_TIMER_FREQUENCY
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
  InternalAcpiDelay (
    (UINT32)DivU64x32 (
              MultU64x32 (
                NanoSeconds,
                ACPI_TIMER_FREQUENCY
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
  return AsmReadTsc ();
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
  OUT UINT64  *StartValue,  OPTIONAL
  OUT UINT64  *EndValue     OPTIONAL
  )
{
  if (StartValue != NULL) {
    *StartValue = 0;
  }

  if (EndValue != NULL) {
    *EndValue = 0xffffffffffffffffULL;
  }
  return InternalGetPerformanceCounterFrequency ();
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
  Shift = MAX (0, HighBitSet64 (Remainder) - 33);
  Remainder = RShiftU64 (Remainder, (UINTN) Shift);
  Frequency = RShiftU64 (Frequency, (UINTN) Shift);
  NanoSeconds += DivU64x64Remainder (MultU64x32 (Remainder, 1000000000u), Frequency, NULL);

  return NanoSeconds;
}

/**
  Calculate TSC frequency.

  The TSC counting frequency is determined by comparing how far it counts
  during a 101.4 us period as determined by the ACPI timer.
  The ACPI timer is used because it counts at a known frequency.
  The TSC is sampled, followed by waiting 363 counts of the ACPI timer,
  or 101.4 us. The TSC is then sampled again. The difference multiplied by
  9861 is the TSC frequency. There will be a small error because of the
  overhead of reading the ACPI timer. An attempt is made to determine and
  compensate for this error.

  @return The number of TSC counts per second.

**/
UINT64
InternalCalculateTscFrequency (
  VOID
  )
{
  UINT64      StartTSC;
  UINT64      EndTSC;
  UINT16      TimerAddr;
  UINT32      Ticks;
  UINT64      TscFrequency;
  BOOLEAN     InterruptState;

  InterruptState = SaveAndDisableInterrupts ();

  TimerAddr = InternalAcpiGetAcpiTimerIoPort ();
  //
  // Compute the number of ticks to wait to measure TSC frequency.
  // Use 363 * 9861 = 3579543 Hz which is within 2 Hz of ACPI_TIMER_FREQUENCY.
  // 363 counts is a calibration time of 101.4 uS.
  //
  Ticks = IoBitFieldRead32 (TimerAddr, 0, 23) + 363;

  StartTSC = AsmReadTsc ();                                         // Get base value for the TSC
  //
  // Wait until the ACPI timer has counted 101.4 us.
  // Timer wrap-arounds are handled correctly by this function.
  // When the current ACPI timer value is greater than 'Ticks',
  // the while loop will exit.
  //
  while (((Ticks - IoBitFieldRead32 (TimerAddr, 0, 23)) & BIT23) == 0) {
    CpuPause();
  }
  EndTSC = AsmReadTsc ();                                           // TSC value 101.4 us later

  TscFrequency = MultU64x32 (
                   (EndTSC - StartTSC),                             // Number of TSC counts in 101.4 us
                   9861                                             // Number of 101.4 us in a second
                   );

  SetInterruptState (InterruptState);

  return TscFrequency;
}

