/** @file
  Local APIC Library.

  This local APIC library instance supports xAPIC mode only.

  Copyright (c) 2010 - 2019, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2017 - 2020, AMD Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Register/Intel/Cpuid.h>
#include <Register/Amd/Cpuid.h>
#include <Register/Intel/Msr.h>
#include <Register/Intel/LocalApic.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/LocalApicLib.h>
#include <Library/IoLib.h>
#include <Library/TimerLib.h>
#include <Library/PcdLib.h>
#include <Library/CpuLib.h>
#include <Library/UefiCpuLib.h>

//
// Library internal functions
//

/**
  Determine if the CPU supports the Local APIC Base Address MSR.

  @retval TRUE  The CPU supports the Local APIC Base Address MSR.
  @retval FALSE The CPU does not support the Local APIC Base Address MSR.

**/
BOOLEAN
LocalApicBaseAddressMsrSupported (
  VOID
  )
{
  UINT32  RegEax;
  UINTN   FamilyId;

  AsmCpuid (1, &RegEax, NULL, NULL, NULL);
  FamilyId = BitFieldRead32 (RegEax, 8, 11);
  if ((FamilyId == 0x04) || (FamilyId == 0x05)) {
    //
    // CPUs with a FamilyId of 0x04 or 0x05 do not support the
    // Local APIC Base Address MSR
    //
    return FALSE;
  }

  return TRUE;
}

/**
  Retrieve the base address of local APIC.

  @return The base address of local APIC.

**/
UINTN
EFIAPI
GetLocalApicBaseAddress (
  VOID
  )
{
  MSR_IA32_APIC_BASE_REGISTER  ApicBaseMsr;

  if (!LocalApicBaseAddressMsrSupported ()) {
    //
    // If CPU does not support Local APIC Base Address MSR, then retrieve
    // Local APIC Base Address from PCD
    //
    return PcdGet32 (PcdCpuLocalApicBaseAddress);
  }

  ApicBaseMsr.Uint64 = AsmReadMsr64 (MSR_IA32_APIC_BASE);

  return (UINTN)(LShiftU64 ((UINT64)ApicBaseMsr.Bits.ApicBaseHi, 32)) +
         (((UINTN)ApicBaseMsr.Bits.ApicBase) << 12);
}

/**
  Set the base address of local APIC.

  If BaseAddress is not aligned on a 4KB boundary, then ASSERT().

  @param[in] BaseAddress   Local APIC base address to be set.

**/
VOID
EFIAPI
SetLocalApicBaseAddress (
  IN UINTN  BaseAddress
  )
{
  MSR_IA32_APIC_BASE_REGISTER  ApicBaseMsr;

  ASSERT ((BaseAddress & (SIZE_4KB - 1)) == 0);

  if (!LocalApicBaseAddressMsrSupported ()) {
    //
    // Ignore set request if the CPU does not support APIC Base Address MSR
    //
    return;
  }

  ApicBaseMsr.Uint64 = AsmReadMsr64 (MSR_IA32_APIC_BASE);

  ApicBaseMsr.Bits.ApicBase   = (UINT32)(BaseAddress >> 12);
  ApicBaseMsr.Bits.ApicBaseHi = (UINT32)(RShiftU64 ((UINT64)BaseAddress, 32));

  AsmWriteMsr64 (MSR_IA32_APIC_BASE, ApicBaseMsr.Uint64);
}

/**
  Read from a local APIC register.

  This function reads from a local APIC register either in xAPIC or x2APIC mode.
  It is required that in xAPIC mode wider registers (64-bit or 256-bit) must be
  accessed using multiple 32-bit loads or stores, so this function only performs
  32-bit read.

  @param  MmioOffset  The MMIO offset of the local APIC register in xAPIC mode.
                      It must be 16-byte aligned.

  @return 32-bit      Value read from the register.
**/
UINT32
EFIAPI
ReadLocalApicReg (
  IN UINTN  MmioOffset
  )
{
  ASSERT ((MmioOffset & 0xf) == 0);
  ASSERT (GetApicMode () == LOCAL_APIC_MODE_XAPIC);

  return MmioRead32 (GetLocalApicBaseAddress () + MmioOffset);
}

/**
  Write to a local APIC register.

  This function writes to a local APIC register either in xAPIC or x2APIC mode.
  It is required that in xAPIC mode wider registers (64-bit or 256-bit) must be
  accessed using multiple 32-bit loads or stores, so this function only performs
  32-bit write.

  if the register index is invalid or unsupported in current APIC mode, then ASSERT.

  @param  MmioOffset  The MMIO offset of the local APIC register in xAPIC mode.
                      It must be 16-byte aligned.
  @param  Value       Value to be written to the register.
**/
VOID
EFIAPI
WriteLocalApicReg (
  IN UINTN   MmioOffset,
  IN UINT32  Value
  )
{
  ASSERT ((MmioOffset & 0xf) == 0);
  ASSERT (GetApicMode () == LOCAL_APIC_MODE_XAPIC);

  MmioWrite32 (GetLocalApicBaseAddress () + MmioOffset, Value);
}

/**
  Send an IPI by writing to ICR.

  This function returns after the IPI has been accepted by the target processor.

  @param  IcrLow 32-bit value to be written to the low half of ICR.
  @param  ApicId APIC ID of the target processor if this IPI is targeted for a specific processor.
**/
VOID
SendIpi (
  IN UINT32  IcrLow,
  IN UINT32  ApicId
  )
{
  LOCAL_APIC_ICR_LOW  IcrLowReg;
  UINT32              IcrHigh;
  BOOLEAN             InterruptState;

  ASSERT (GetApicMode () == LOCAL_APIC_MODE_XAPIC);
  ASSERT (ApicId <= 0xff);

  InterruptState = SaveAndDisableInterrupts ();

  //
  // Save existing contents of ICR high 32 bits
  //
  IcrHigh = ReadLocalApicReg (XAPIC_ICR_HIGH_OFFSET);

  //
  // Wait for DeliveryStatus clear in case a previous IPI
  //  is still being sent
  //
  do {
    IcrLowReg.Uint32 = ReadLocalApicReg (XAPIC_ICR_LOW_OFFSET);
  } while (IcrLowReg.Bits.DeliveryStatus != 0);

  //
  // For xAPIC, the act of writing to the low doubleword of the ICR causes the IPI to be sent.
  //
  WriteLocalApicReg (XAPIC_ICR_HIGH_OFFSET, ApicId << 24);
  WriteLocalApicReg (XAPIC_ICR_LOW_OFFSET, IcrLow);

  //
  // Wait for DeliveryStatus clear again
  //
  do {
    IcrLowReg.Uint32 = ReadLocalApicReg (XAPIC_ICR_LOW_OFFSET);
  } while (IcrLowReg.Bits.DeliveryStatus != 0);

  //
  // And restore old contents of ICR high
  //
  WriteLocalApicReg (XAPIC_ICR_HIGH_OFFSET, IcrHigh);

  SetInterruptState (InterruptState);
}

//
// Library API implementation functions
//

/**
  Get the current local APIC mode.

  If local APIC is disabled, then ASSERT.

  @retval LOCAL_APIC_MODE_XAPIC  current APIC mode is xAPIC.
  @retval LOCAL_APIC_MODE_X2APIC current APIC mode is x2APIC.
**/
UINTN
EFIAPI
GetApicMode (
  VOID
  )
{
  DEBUG_CODE_BEGIN ();
  {
    MSR_IA32_APIC_BASE_REGISTER  ApicBaseMsr;

    //
    // Check to see if the CPU supports the APIC Base Address MSR
    //
    if (LocalApicBaseAddressMsrSupported ()) {
      ApicBaseMsr.Uint64 = AsmReadMsr64 (MSR_IA32_APIC_BASE);
      //
      // Local APIC should have been enabled
      //
      ASSERT (ApicBaseMsr.Bits.EN != 0);
      ASSERT (ApicBaseMsr.Bits.EXTD == 0);
    }
  }
  DEBUG_CODE_END ();
  return LOCAL_APIC_MODE_XAPIC;
}

/**
  Set the current local APIC mode.

  If the specified local APIC mode is not valid, then ASSERT.
  If the specified local APIC mode can't be set as current, then ASSERT.

  @param ApicMode APIC mode to be set.

  @note  This API must not be called from an interrupt handler or SMI handler.
         It may result in unpredictable behavior.
**/
VOID
EFIAPI
SetApicMode (
  IN UINTN  ApicMode
  )
{
  ASSERT (ApicMode == LOCAL_APIC_MODE_XAPIC);
  ASSERT (GetApicMode () == LOCAL_APIC_MODE_XAPIC);
}

/**
  Get the initial local APIC ID of the executing processor assigned by hardware upon power on or reset.

  In xAPIC mode, the initial local APIC ID may be different from current APIC ID.
  In x2APIC mode, the local APIC ID can't be changed and there is no concept of initial APIC ID. In this case,
  the 32-bit local APIC ID is returned as initial APIC ID.

  @return  32-bit initial local APIC ID of the executing processor.
**/
UINT32
EFIAPI
GetInitialApicId (
  VOID
  )
{
  UINT32  ApicId;
  UINT32  MaxCpuIdIndex;
  UINT32  RegEbx;

  ASSERT (GetApicMode () == LOCAL_APIC_MODE_XAPIC);

  //
  // Get the max index of basic CPUID
  //
  AsmCpuid (CPUID_SIGNATURE, &MaxCpuIdIndex, NULL, NULL, NULL);

  //
  // If CPUID Leaf B is supported,
  // And CPUID.0BH:EBX[15:0] reports a non-zero value,
  // Then the initial 32-bit APIC ID = CPUID.0BH:EDX
  // Else the initial 8-bit APIC ID = CPUID.1:EBX[31:24]
  //
  if (MaxCpuIdIndex >= CPUID_EXTENDED_TOPOLOGY) {
    AsmCpuidEx (CPUID_EXTENDED_TOPOLOGY, 0, NULL, &RegEbx, NULL, &ApicId);
    if ((RegEbx & (BIT16 - 1)) != 0) {
      return ApicId;
    }
  }

  AsmCpuid (CPUID_VERSION_INFO, NULL, &RegEbx, NULL, NULL);
  return RegEbx >> 24;
}

/**
  Get the local APIC ID of the executing processor.

  @return  32-bit local APIC ID of the executing processor.
**/
UINT32
EFIAPI
GetApicId (
  VOID
  )
{
  UINT32  ApicId;

  ASSERT (GetApicMode () == LOCAL_APIC_MODE_XAPIC);

  if ((ApicId = GetInitialApicId ()) < 0x100) {
    //
    // If the initial local APIC ID is less 0x100, read APIC ID from
    // XAPIC_ID_OFFSET, otherwise return the initial local APIC ID.
    //
    ApicId   = ReadLocalApicReg (XAPIC_ID_OFFSET);
    ApicId >>= 24;
  }

  return ApicId;
}

/**
  Get the value of the local APIC version register.

  @return  the value of the local APIC version register.
**/
UINT32
EFIAPI
GetApicVersion (
  VOID
  )
{
  return ReadLocalApicReg (XAPIC_VERSION_OFFSET);
}

/**
  Send a Fixed IPI to a specified target processor.

  This function returns after the IPI has been accepted by the target processor.

  @param  ApicId   The local APIC ID of the target processor.
  @param  Vector   The vector number of the interrupt being sent.
**/
VOID
EFIAPI
SendFixedIpi (
  IN UINT32  ApicId,
  IN UINT8   Vector
  )
{
  LOCAL_APIC_ICR_LOW  IcrLow;

  IcrLow.Uint32            = 0;
  IcrLow.Bits.DeliveryMode = LOCAL_APIC_DELIVERY_MODE_FIXED;
  IcrLow.Bits.Level        = 1;
  IcrLow.Bits.Vector       = Vector;
  SendIpi (IcrLow.Uint32, ApicId);
}

/**
  Send a Fixed IPI to all processors excluding self.

  This function returns after the IPI has been accepted by the target processors.

  @param  Vector   The vector number of the interrupt being sent.
**/
VOID
EFIAPI
SendFixedIpiAllExcludingSelf (
  IN UINT8  Vector
  )
{
  LOCAL_APIC_ICR_LOW  IcrLow;

  IcrLow.Uint32                    = 0;
  IcrLow.Bits.DeliveryMode         = LOCAL_APIC_DELIVERY_MODE_FIXED;
  IcrLow.Bits.Level                = 1;
  IcrLow.Bits.DestinationShorthand = LOCAL_APIC_DESTINATION_SHORTHAND_ALL_EXCLUDING_SELF;
  IcrLow.Bits.Vector               = Vector;
  SendIpi (IcrLow.Uint32, 0);
}

/**
  Send a SMI IPI to a specified target processor.

  This function returns after the IPI has been accepted by the target processor.

  @param  ApicId   Specify the local APIC ID of the target processor.
**/
VOID
EFIAPI
SendSmiIpi (
  IN UINT32  ApicId
  )
{
  LOCAL_APIC_ICR_LOW  IcrLow;

  IcrLow.Uint32            = 0;
  IcrLow.Bits.DeliveryMode = LOCAL_APIC_DELIVERY_MODE_SMI;
  IcrLow.Bits.Level        = 1;
  SendIpi (IcrLow.Uint32, ApicId);
}

/**
  Send a SMI IPI to all processors excluding self.

  This function returns after the IPI has been accepted by the target processors.
**/
VOID
EFIAPI
SendSmiIpiAllExcludingSelf (
  VOID
  )
{
  LOCAL_APIC_ICR_LOW  IcrLow;

  IcrLow.Uint32                    = 0;
  IcrLow.Bits.DeliveryMode         = LOCAL_APIC_DELIVERY_MODE_SMI;
  IcrLow.Bits.Level                = 1;
  IcrLow.Bits.DestinationShorthand = LOCAL_APIC_DESTINATION_SHORTHAND_ALL_EXCLUDING_SELF;
  SendIpi (IcrLow.Uint32, 0);
}

/**
  Send an INIT IPI to a specified target processor.

  This function returns after the IPI has been accepted by the target processor.

  @param  ApicId   Specify the local APIC ID of the target processor.
**/
VOID
EFIAPI
SendInitIpi (
  IN UINT32  ApicId
  )
{
  LOCAL_APIC_ICR_LOW  IcrLow;

  IcrLow.Uint32            = 0;
  IcrLow.Bits.DeliveryMode = LOCAL_APIC_DELIVERY_MODE_INIT;
  IcrLow.Bits.Level        = 1;
  SendIpi (IcrLow.Uint32, ApicId);
}

/**
  Send an INIT IPI to all processors excluding self.

  This function returns after the IPI has been accepted by the target processors.
**/
VOID
EFIAPI
SendInitIpiAllExcludingSelf (
  VOID
  )
{
  LOCAL_APIC_ICR_LOW  IcrLow;

  IcrLow.Uint32                    = 0;
  IcrLow.Bits.DeliveryMode         = LOCAL_APIC_DELIVERY_MODE_INIT;
  IcrLow.Bits.Level                = 1;
  IcrLow.Bits.DestinationShorthand = LOCAL_APIC_DESTINATION_SHORTHAND_ALL_EXCLUDING_SELF;
  SendIpi (IcrLow.Uint32, 0);
}

/**
  Send an INIT-Start-up-Start-up IPI sequence to a specified target processor.

  This function returns after the IPI has been accepted by the target processor.

  if StartupRoutine >= 1M, then ASSERT.
  if StartupRoutine is not multiple of 4K, then ASSERT.

  @param  ApicId          Specify the local APIC ID of the target processor.
  @param  StartupRoutine  Points to a start-up routine which is below 1M physical
                          address and 4K aligned.
**/
VOID
EFIAPI
SendInitSipiSipi (
  IN UINT32  ApicId,
  IN UINT32  StartupRoutine
  )
{
  LOCAL_APIC_ICR_LOW  IcrLow;

  ASSERT (StartupRoutine < 0x100000);
  ASSERT ((StartupRoutine & 0xfff) == 0);

  SendInitIpi (ApicId);
  MicroSecondDelay (PcdGet32 (PcdCpuInitIpiDelayInMicroSeconds));
  IcrLow.Uint32            = 0;
  IcrLow.Bits.Vector       = (StartupRoutine >> 12);
  IcrLow.Bits.DeliveryMode = LOCAL_APIC_DELIVERY_MODE_STARTUP;
  IcrLow.Bits.Level        = 1;
  SendIpi (IcrLow.Uint32, ApicId);
  if (!StandardSignatureIsAuthenticAMD ()) {
    MicroSecondDelay (200);
    SendIpi (IcrLow.Uint32, ApicId);
  }
}

/**
  Send an INIT-Start-up-Start-up IPI sequence to all processors excluding self.

  This function returns after the IPI has been accepted by the target processors.

  if StartupRoutine >= 1M, then ASSERT.
  if StartupRoutine is not multiple of 4K, then ASSERT.

  @param  StartupRoutine    Points to a start-up routine which is below 1M physical
                            address and 4K aligned.
**/
VOID
EFIAPI
SendInitSipiSipiAllExcludingSelf (
  IN UINT32  StartupRoutine
  )
{
  LOCAL_APIC_ICR_LOW  IcrLow;

  ASSERT (StartupRoutine < 0x100000);
  ASSERT ((StartupRoutine & 0xfff) == 0);

  SendInitIpiAllExcludingSelf ();
  MicroSecondDelay (PcdGet32 (PcdCpuInitIpiDelayInMicroSeconds));
  IcrLow.Uint32                    = 0;
  IcrLow.Bits.Vector               = (StartupRoutine >> 12);
  IcrLow.Bits.DeliveryMode         = LOCAL_APIC_DELIVERY_MODE_STARTUP;
  IcrLow.Bits.Level                = 1;
  IcrLow.Bits.DestinationShorthand = LOCAL_APIC_DESTINATION_SHORTHAND_ALL_EXCLUDING_SELF;
  SendIpi (IcrLow.Uint32, 0);
  if (!StandardSignatureIsAuthenticAMD ()) {
    MicroSecondDelay (200);
    SendIpi (IcrLow.Uint32, 0);
  }
}

/**
  Initialize the state of the SoftwareEnable bit in the Local APIC
  Spurious Interrupt Vector register.

  @param  Enable  If TRUE, then set SoftwareEnable to 1
                  If FALSE, then set SoftwareEnable to 0.

**/
VOID
EFIAPI
InitializeLocalApicSoftwareEnable (
  IN BOOLEAN  Enable
  )
{
  LOCAL_APIC_SVR  Svr;

  //
  // Set local APIC software-enabled bit.
  //
  Svr.Uint32 = ReadLocalApicReg (XAPIC_SPURIOUS_VECTOR_OFFSET);
  if (Enable) {
    if (Svr.Bits.SoftwareEnable == 0) {
      Svr.Bits.SoftwareEnable = 1;
      WriteLocalApicReg (XAPIC_SPURIOUS_VECTOR_OFFSET, Svr.Uint32);
    }
  } else {
    if (Svr.Bits.SoftwareEnable == 1) {
      Svr.Bits.SoftwareEnable = 0;
      WriteLocalApicReg (XAPIC_SPURIOUS_VECTOR_OFFSET, Svr.Uint32);
    }
  }
}

/**
  Programming Virtual Wire Mode.

  This function programs the local APIC for virtual wire mode following
  the example described in chapter A.3 of the MP 1.4 spec.

  IOxAPIC is not involved in this type of virtual wire mode.
**/
VOID
EFIAPI
ProgramVirtualWireMode (
  VOID
  )
{
  LOCAL_APIC_SVR       Svr;
  LOCAL_APIC_LVT_LINT  Lint;

  //
  // Enable the APIC via SVR and set the spurious interrupt to use Int 00F.
  //
  Svr.Uint32              = ReadLocalApicReg (XAPIC_SPURIOUS_VECTOR_OFFSET);
  Svr.Bits.SpuriousVector = 0xf;
  Svr.Bits.SoftwareEnable = 1;
  WriteLocalApicReg (XAPIC_SPURIOUS_VECTOR_OFFSET, Svr.Uint32);

  //
  // Program the LINT0 vector entry as ExtInt. Not masked, edge, active high.
  //
  Lint.Uint32                = ReadLocalApicReg (XAPIC_LVT_LINT0_OFFSET);
  Lint.Bits.DeliveryMode     = LOCAL_APIC_DELIVERY_MODE_EXTINT;
  Lint.Bits.InputPinPolarity = 0;
  Lint.Bits.TriggerMode      = 0;
  Lint.Bits.Mask             = 0;
  WriteLocalApicReg (XAPIC_LVT_LINT0_OFFSET, Lint.Uint32);

  //
  // Program the LINT0 vector entry as NMI. Not masked, edge, active high.
  //
  Lint.Uint32                = ReadLocalApicReg (XAPIC_LVT_LINT1_OFFSET);
  Lint.Bits.DeliveryMode     = LOCAL_APIC_DELIVERY_MODE_NMI;
  Lint.Bits.InputPinPolarity = 0;
  Lint.Bits.TriggerMode      = 0;
  Lint.Bits.Mask             = 0;
  WriteLocalApicReg (XAPIC_LVT_LINT1_OFFSET, Lint.Uint32);
}

/**
  Disable LINT0 & LINT1 interrupts.

  This function sets the mask flag in the LVT LINT0 & LINT1 registers.
**/
VOID
EFIAPI
DisableLvtInterrupts (
  VOID
  )
{
  LOCAL_APIC_LVT_LINT  LvtLint;

  LvtLint.Uint32    = ReadLocalApicReg (XAPIC_LVT_LINT0_OFFSET);
  LvtLint.Bits.Mask = 1;
  WriteLocalApicReg (XAPIC_LVT_LINT0_OFFSET, LvtLint.Uint32);

  LvtLint.Uint32    = ReadLocalApicReg (XAPIC_LVT_LINT1_OFFSET);
  LvtLint.Bits.Mask = 1;
  WriteLocalApicReg (XAPIC_LVT_LINT1_OFFSET, LvtLint.Uint32);
}

/**
  Read the initial count value from the init-count register.

  @return The initial count value read from the init-count register.
**/
UINT32
EFIAPI
GetApicTimerInitCount (
  VOID
  )
{
  return ReadLocalApicReg (XAPIC_TIMER_INIT_COUNT_OFFSET);
}

/**
  Read the current count value from the current-count register.

  @return The current count value read from the current-count register.
**/
UINT32
EFIAPI
GetApicTimerCurrentCount (
  VOID
  )
{
  return ReadLocalApicReg (XAPIC_TIMER_CURRENT_COUNT_OFFSET);
}

/**
  Initialize the local APIC timer.

  The local APIC timer is initialized and enabled.

  @param DivideValue   The divide value for the DCR. It is one of 1,2,4,8,16,32,64,128.
                       If it is 0, then use the current divide value in the DCR.
  @param InitCount     The initial count value.
  @param PeriodicMode  If TRUE, timer mode is peridoic. Othewise, timer mode is one-shot.
  @param Vector        The timer interrupt vector number.
**/
VOID
EFIAPI
InitializeApicTimer (
  IN UINTN    DivideValue,
  IN UINT32   InitCount,
  IN BOOLEAN  PeriodicMode,
  IN UINT8    Vector
  )
{
  LOCAL_APIC_DCR        Dcr;
  LOCAL_APIC_LVT_TIMER  LvtTimer;
  UINT32                Divisor;

  //
  // Ensure local APIC is in software-enabled state.
  //
  InitializeLocalApicSoftwareEnable (TRUE);

  //
  // Program init-count register.
  //
  WriteLocalApicReg (XAPIC_TIMER_INIT_COUNT_OFFSET, InitCount);

  if (DivideValue != 0) {
    ASSERT (DivideValue <= 128);
    ASSERT (DivideValue == GetPowerOfTwo32 ((UINT32)DivideValue));
    Divisor = (UINT32)((HighBitSet32 ((UINT32)DivideValue) - 1) & 0x7);

    Dcr.Uint32            = ReadLocalApicReg (XAPIC_TIMER_DIVIDE_CONFIGURATION_OFFSET);
    Dcr.Bits.DivideValue1 = (Divisor & 0x3);
    Dcr.Bits.DivideValue2 = (Divisor >> 2);
    WriteLocalApicReg (XAPIC_TIMER_DIVIDE_CONFIGURATION_OFFSET, Dcr.Uint32);
  }

  //
  // Enable APIC timer interrupt with specified timer mode.
  //
  LvtTimer.Uint32 = ReadLocalApicReg (XAPIC_LVT_TIMER_OFFSET);
  if (PeriodicMode) {
    LvtTimer.Bits.TimerMode = 1;
  } else {
    LvtTimer.Bits.TimerMode = 0;
  }

  LvtTimer.Bits.Mask   = 0;
  LvtTimer.Bits.Vector = Vector;
  WriteLocalApicReg (XAPIC_LVT_TIMER_OFFSET, LvtTimer.Uint32);
}

/**
  Get the state of the local APIC timer.

  This function will ASSERT if the local APIC is not software enabled.

  @param DivideValue   Return the divide value for the DCR. It is one of 1,2,4,8,16,32,64,128.
  @param PeriodicMode  Return the timer mode. If TRUE, timer mode is peridoic. Othewise, timer mode is one-shot.
  @param Vector        Return the timer interrupt vector number.
**/
VOID
EFIAPI
GetApicTimerState (
  OUT UINTN    *DivideValue  OPTIONAL,
  OUT BOOLEAN  *PeriodicMode  OPTIONAL,
  OUT UINT8    *Vector  OPTIONAL
  )
{
  UINT32                Divisor;
  LOCAL_APIC_DCR        Dcr;
  LOCAL_APIC_LVT_TIMER  LvtTimer;

  //
  // Check the APIC Software Enable/Disable bit (bit 8) in Spurious-Interrupt
  // Vector Register.
  // This bit will be 1, if local APIC is software enabled.
  //
  ASSERT ((ReadLocalApicReg (XAPIC_SPURIOUS_VECTOR_OFFSET) & BIT8) != 0);

  if (DivideValue != NULL) {
    Dcr.Uint32   = ReadLocalApicReg (XAPIC_TIMER_DIVIDE_CONFIGURATION_OFFSET);
    Divisor      = Dcr.Bits.DivideValue1 | (Dcr.Bits.DivideValue2 << 2);
    Divisor      = (Divisor + 1) & 0x7;
    *DivideValue = ((UINTN)1) << Divisor;
  }

  if ((PeriodicMode != NULL) || (Vector != NULL)) {
    LvtTimer.Uint32 = ReadLocalApicReg (XAPIC_LVT_TIMER_OFFSET);
    if (PeriodicMode != NULL) {
      if (LvtTimer.Bits.TimerMode == 1) {
        *PeriodicMode = TRUE;
      } else {
        *PeriodicMode = FALSE;
      }
    }

    if (Vector != NULL) {
      *Vector = (UINT8)LvtTimer.Bits.Vector;
    }
  }
}

/**
  Enable the local APIC timer interrupt.
**/
VOID
EFIAPI
EnableApicTimerInterrupt (
  VOID
  )
{
  LOCAL_APIC_LVT_TIMER  LvtTimer;

  LvtTimer.Uint32    = ReadLocalApicReg (XAPIC_LVT_TIMER_OFFSET);
  LvtTimer.Bits.Mask = 0;
  WriteLocalApicReg (XAPIC_LVT_TIMER_OFFSET, LvtTimer.Uint32);
}

/**
  Disable the local APIC timer interrupt.
**/
VOID
EFIAPI
DisableApicTimerInterrupt (
  VOID
  )
{
  LOCAL_APIC_LVT_TIMER  LvtTimer;

  LvtTimer.Uint32    = ReadLocalApicReg (XAPIC_LVT_TIMER_OFFSET);
  LvtTimer.Bits.Mask = 1;
  WriteLocalApicReg (XAPIC_LVT_TIMER_OFFSET, LvtTimer.Uint32);
}

/**
  Get the local APIC timer interrupt state.

  @retval TRUE  The local APIC timer interrupt is enabled.
  @retval FALSE The local APIC timer interrupt is disabled.
**/
BOOLEAN
EFIAPI
GetApicTimerInterruptState (
  VOID
  )
{
  LOCAL_APIC_LVT_TIMER  LvtTimer;

  LvtTimer.Uint32 = ReadLocalApicReg (XAPIC_LVT_TIMER_OFFSET);
  return (BOOLEAN)(LvtTimer.Bits.Mask == 0);
}

/**
  Send EOI to the local APIC.
**/
VOID
EFIAPI
SendApicEoi (
  VOID
  )
{
  WriteLocalApicReg (XAPIC_EOI_OFFSET, 0);
}

/**
  Get the 32-bit address that a device should use to send a Message Signaled
  Interrupt (MSI) to the Local APIC of the currently executing processor.

  @return 32-bit address used to send an MSI to the Local APIC.
**/
UINT32
EFIAPI
GetApicMsiAddress (
  VOID
  )
{
  LOCAL_APIC_MSI_ADDRESS  MsiAddress;

  //
  // Return address for an MSI interrupt to be delivered only to the APIC ID
  // of the currently executing processor.
  //
  MsiAddress.Uint32             = 0;
  MsiAddress.Bits.BaseAddress   = 0xFEE;
  MsiAddress.Bits.DestinationId = GetApicId ();
  return MsiAddress.Uint32;
}

/**
  Get the 64-bit data value that a device should use to send a Message Signaled
  Interrupt (MSI) to the Local APIC of the currently executing processor.

  If Vector is not in range 0x10..0xFE, then ASSERT().
  If DeliveryMode is not supported, then ASSERT().

  @param  Vector          The 8-bit interrupt vector associated with the MSI.
                          Must be in the range 0x10..0xFE
  @param  DeliveryMode    A 3-bit value that specifies how the recept of the MSI
                          is handled.  The only supported values are:
                            0: LOCAL_APIC_DELIVERY_MODE_FIXED
                            1: LOCAL_APIC_DELIVERY_MODE_LOWEST_PRIORITY
                            2: LOCAL_APIC_DELIVERY_MODE_SMI
                            4: LOCAL_APIC_DELIVERY_MODE_NMI
                            5: LOCAL_APIC_DELIVERY_MODE_INIT
                            7: LOCAL_APIC_DELIVERY_MODE_EXTINT

  @param  LevelTriggered  TRUE specifies a level triggered interrupt.
                          FALSE specifies an edge triggered interrupt.
  @param  AssertionLevel  Ignored if LevelTriggered is FALSE.
                          TRUE specifies a level triggered interrupt that active
                          when the interrupt line is asserted.
                          FALSE specifies a level triggered interrupt that active
                          when the interrupt line is deasserted.

  @return 64-bit data value used to send an MSI to the Local APIC.
**/
UINT64
EFIAPI
GetApicMsiValue (
  IN UINT8    Vector,
  IN UINTN    DeliveryMode,
  IN BOOLEAN  LevelTriggered,
  IN BOOLEAN  AssertionLevel
  )
{
  LOCAL_APIC_MSI_DATA  MsiData;

  ASSERT (Vector >= 0x10 && Vector <= 0xFE);
  ASSERT (DeliveryMode < 8 && DeliveryMode != 6 && DeliveryMode != 3);

  MsiData.Uint64            = 0;
  MsiData.Bits.Vector       = Vector;
  MsiData.Bits.DeliveryMode = (UINT32)DeliveryMode;
  if (LevelTriggered) {
    MsiData.Bits.TriggerMode = 1;
    if (AssertionLevel) {
      MsiData.Bits.Level = 1;
    }
  }

  return MsiData.Uint64;
}

/**
  Get Package ID/Core ID/Thread ID of a processor.

  The algorithm assumes the target system has symmetry across physical
  package  boundaries with respect to the number of logical processors
  per package,  number of cores per package.

  @param[in]  InitialApicId  Initial APIC ID of the target logical processor.
  @param[out]  Package       Returns the processor package ID.
  @param[out]  Core          Returns the processor core ID.
  @param[out]  Thread        Returns the processor thread ID.
**/
VOID
EFIAPI
GetProcessorLocationByApicId (
  IN  UINT32  InitialApicId,
  OUT UINT32  *Package  OPTIONAL,
  OUT UINT32  *Core    OPTIONAL,
  OUT UINT32  *Thread  OPTIONAL
  )
{
  BOOLEAN                             TopologyLeafSupported;
  CPUID_VERSION_INFO_EBX              VersionInfoEbx;
  CPUID_VERSION_INFO_EDX              VersionInfoEdx;
  CPUID_CACHE_PARAMS_EAX              CacheParamsEax;
  CPUID_EXTENDED_TOPOLOGY_EAX         ExtendedTopologyEax;
  CPUID_EXTENDED_TOPOLOGY_EBX         ExtendedTopologyEbx;
  CPUID_EXTENDED_TOPOLOGY_ECX         ExtendedTopologyEcx;
  CPUID_AMD_EXTENDED_CPU_SIG_ECX      AmdExtendedCpuSigEcx;
  CPUID_AMD_PROCESSOR_TOPOLOGY_EBX    AmdProcessorTopologyEbx;
  CPUID_AMD_VIR_PHY_ADDRESS_SIZE_ECX  AmdVirPhyAddressSizeEcx;
  UINT32                              MaxStandardCpuIdIndex;
  UINT32                              MaxExtendedCpuIdIndex;
  UINT32                              SubIndex;
  UINTN                               LevelType;
  UINT32                              MaxLogicProcessorsPerPackage;
  UINT32                              MaxCoresPerPackage;
  UINTN                               ThreadBits;
  UINTN                               CoreBits;

  //
  // Check if the processor is capable of supporting more than one logical processor.
  //
  AsmCpuid (CPUID_VERSION_INFO, NULL, NULL, NULL, &VersionInfoEdx.Uint32);
  if (VersionInfoEdx.Bits.HTT == 0) {
    if (Thread != NULL) {
      *Thread = 0;
    }

    if (Core != NULL) {
      *Core = 0;
    }

    if (Package != NULL) {
      *Package = 0;
    }

    return;
  }

  //
  // Assume three-level mapping of APIC ID: Package|Core|Thread.
  //
  ThreadBits = 0;
  CoreBits   = 0;

  //
  // Get max index of CPUID
  //
  AsmCpuid (CPUID_SIGNATURE, &MaxStandardCpuIdIndex, NULL, NULL, NULL);
  AsmCpuid (CPUID_EXTENDED_FUNCTION, &MaxExtendedCpuIdIndex, NULL, NULL, NULL);

  //
  // If the extended topology enumeration leaf is available, it
  // is the preferred mechanism for enumerating topology.
  //
  TopologyLeafSupported = FALSE;
  if (MaxStandardCpuIdIndex >= CPUID_EXTENDED_TOPOLOGY) {
    AsmCpuidEx (
      CPUID_EXTENDED_TOPOLOGY,
      0,
      &ExtendedTopologyEax.Uint32,
      &ExtendedTopologyEbx.Uint32,
      &ExtendedTopologyEcx.Uint32,
      NULL
      );
    //
    // If CPUID.(EAX=0BH, ECX=0H):EBX returns zero and maximum input value for
    // basic CPUID information is greater than 0BH, then CPUID.0BH leaf is not
    // supported on that processor.
    //
    if (ExtendedTopologyEbx.Uint32 != 0) {
      TopologyLeafSupported = TRUE;

      //
      // Sub-leaf index 0 (ECX= 0 as input) provides enumeration parameters to extract
      // the SMT sub-field of x2APIC ID.
      //
      LevelType = ExtendedTopologyEcx.Bits.LevelType;
      ASSERT (LevelType == CPUID_EXTENDED_TOPOLOGY_LEVEL_TYPE_SMT);
      ThreadBits = ExtendedTopologyEax.Bits.ApicIdShift;

      //
      // Software must not assume any "level type" encoding
      // value to be related to any sub-leaf index, except sub-leaf 0.
      //
      SubIndex = 1;
      do {
        AsmCpuidEx (
          CPUID_EXTENDED_TOPOLOGY,
          SubIndex,
          &ExtendedTopologyEax.Uint32,
          NULL,
          &ExtendedTopologyEcx.Uint32,
          NULL
          );
        LevelType = ExtendedTopologyEcx.Bits.LevelType;
        if (LevelType == CPUID_EXTENDED_TOPOLOGY_LEVEL_TYPE_CORE) {
          CoreBits = ExtendedTopologyEax.Bits.ApicIdShift - ThreadBits;
          break;
        }

        SubIndex++;
      } while (LevelType != CPUID_EXTENDED_TOPOLOGY_LEVEL_TYPE_INVALID);
    }
  }

  if (!TopologyLeafSupported) {
    //
    // Get logical processor count
    //
    AsmCpuid (CPUID_VERSION_INFO, NULL, &VersionInfoEbx.Uint32, NULL, NULL);
    MaxLogicProcessorsPerPackage = VersionInfoEbx.Bits.MaximumAddressableIdsForLogicalProcessors;

    //
    // Assume single-core processor
    //
    MaxCoresPerPackage = 1;

    //
    // Check for topology extensions on AMD processor
    //
    if (StandardSignatureIsAuthenticAMD ()) {
      if (MaxExtendedCpuIdIndex >= CPUID_AMD_PROCESSOR_TOPOLOGY) {
        AsmCpuid (CPUID_EXTENDED_CPU_SIG, NULL, NULL, &AmdExtendedCpuSigEcx.Uint32, NULL);
        if (AmdExtendedCpuSigEcx.Bits.TopologyExtensions != 0) {
          //
          // Account for max possible thread count to decode ApicId
          //
          AsmCpuid (CPUID_VIR_PHY_ADDRESS_SIZE, NULL, NULL, &AmdVirPhyAddressSizeEcx.Uint32, NULL);
          MaxLogicProcessorsPerPackage = 1 << AmdVirPhyAddressSizeEcx.Bits.ApicIdCoreIdSize;

          //
          // Get cores per processor package
          //
          AsmCpuid (CPUID_AMD_PROCESSOR_TOPOLOGY, NULL, &AmdProcessorTopologyEbx.Uint32, NULL, NULL);
          MaxCoresPerPackage = MaxLogicProcessorsPerPackage / (AmdProcessorTopologyEbx.Bits.ThreadsPerCore + 1);
        }
      }
    } else {
      //
      // Extract core count based on CACHE information
      //
      if (MaxStandardCpuIdIndex >= CPUID_CACHE_PARAMS) {
        AsmCpuidEx (CPUID_CACHE_PARAMS, 0, &CacheParamsEax.Uint32, NULL, NULL, NULL);
        if (CacheParamsEax.Uint32 != 0) {
          MaxCoresPerPackage = CacheParamsEax.Bits.MaximumAddressableIdsForLogicalProcessors + 1;
        }
      }
    }

    ThreadBits = (UINTN)(HighBitSet32 (MaxLogicProcessorsPerPackage / MaxCoresPerPackage - 1) + 1);
    CoreBits   = (UINTN)(HighBitSet32 (MaxCoresPerPackage - 1) + 1);
  }

  if (Thread != NULL) {
    *Thread = InitialApicId & ((1 << ThreadBits) - 1);
  }

  if (Core != NULL) {
    *Core = (InitialApicId >> ThreadBits) & ((1 << CoreBits) - 1);
  }

  if (Package != NULL) {
    *Package = (InitialApicId >> (ThreadBits + CoreBits));
  }
}

/**
  Get Package ID/Die ID/Tile ID/Module ID/Core ID/Thread ID of a processor.

  The algorithm assumes the target system has symmetry across physical
  package boundaries with respect to the number of threads per core, number of
  cores per module, number of modules per tile, number of tiles per die, number
  of dies per package.

  @param[in]   InitialApicId Initial APIC ID of the target logical processor.
  @param[out]  Package       Returns the processor package ID.
  @param[out]  Die           Returns the processor die ID.
  @param[out]  Tile          Returns the processor tile ID.
  @param[out]  Module        Returns the processor module ID.
  @param[out]  Core          Returns the processor core ID.
  @param[out]  Thread        Returns the processor thread ID.
**/
VOID
EFIAPI
GetProcessorLocation2ByApicId (
  IN  UINT32  InitialApicId,
  OUT UINT32  *Package  OPTIONAL,
  OUT UINT32  *Die      OPTIONAL,
  OUT UINT32  *Tile     OPTIONAL,
  OUT UINT32  *Module   OPTIONAL,
  OUT UINT32  *Core     OPTIONAL,
  OUT UINT32  *Thread   OPTIONAL
  )
{
  CPUID_EXTENDED_TOPOLOGY_EAX  ExtendedTopologyEax;
  CPUID_EXTENDED_TOPOLOGY_ECX  ExtendedTopologyEcx;
  UINT32                       MaxStandardCpuIdIndex;
  UINT32                       Index;
  UINTN                        LevelType;
  UINT32                       Bits[CPUID_V2_EXTENDED_TOPOLOGY_LEVEL_TYPE_DIE + 2];
  UINT32                       *Location[CPUID_V2_EXTENDED_TOPOLOGY_LEVEL_TYPE_DIE + 2];

  for (LevelType = 0; LevelType < ARRAY_SIZE (Bits); LevelType++) {
    Bits[LevelType] = 0;
  }

  //
  // Get max index of CPUID
  //
  AsmCpuid (CPUID_SIGNATURE, &MaxStandardCpuIdIndex, NULL, NULL, NULL);
  if (MaxStandardCpuIdIndex < CPUID_V2_EXTENDED_TOPOLOGY) {
    if (Die != NULL) {
      *Die = 0;
    }

    if (Tile != NULL) {
      *Tile = 0;
    }

    if (Module != NULL) {
      *Module = 0;
    }

    GetProcessorLocationByApicId (InitialApicId, Package, Core, Thread);
    return;
  }

  //
  // If the V2 extended topology enumeration leaf is available, it
  // is the preferred mechanism for enumerating topology.
  //
  for (Index = 0; ; Index++) {
    AsmCpuidEx (
      CPUID_V2_EXTENDED_TOPOLOGY,
      Index,
      &ExtendedTopologyEax.Uint32,
      NULL,
      &ExtendedTopologyEcx.Uint32,
      NULL
      );

    LevelType = ExtendedTopologyEcx.Bits.LevelType;

    //
    // first level reported should be SMT.
    //
    ASSERT ((Index != 0) || (LevelType == CPUID_EXTENDED_TOPOLOGY_LEVEL_TYPE_SMT));
    if (LevelType == CPUID_EXTENDED_TOPOLOGY_LEVEL_TYPE_INVALID) {
      break;
    }

    ASSERT (LevelType < ARRAY_SIZE (Bits));
    Bits[LevelType] = ExtendedTopologyEax.Bits.ApicIdShift;
  }

  for (LevelType = CPUID_EXTENDED_TOPOLOGY_LEVEL_TYPE_CORE; LevelType < ARRAY_SIZE (Bits); LevelType++) {
    //
    // If there are more levels between level-1 (low-level) and level-2 (high-level), the unknown levels will be ignored
    // and treated as an extension of the last known level (i.e., level-1 in this case).
    //
    if (Bits[LevelType] == 0) {
      Bits[LevelType] = Bits[LevelType - 1];
    }
  }

  Location[CPUID_V2_EXTENDED_TOPOLOGY_LEVEL_TYPE_DIE + 1] = Package;
  Location[CPUID_V2_EXTENDED_TOPOLOGY_LEVEL_TYPE_DIE]     = Die;
  Location[CPUID_V2_EXTENDED_TOPOLOGY_LEVEL_TYPE_TILE]    = Tile;
  Location[CPUID_V2_EXTENDED_TOPOLOGY_LEVEL_TYPE_MODULE]  = Module;
  Location[CPUID_EXTENDED_TOPOLOGY_LEVEL_TYPE_CORE]       = Core;
  Location[CPUID_EXTENDED_TOPOLOGY_LEVEL_TYPE_SMT]        = Thread;

  Bits[CPUID_V2_EXTENDED_TOPOLOGY_LEVEL_TYPE_DIE + 1] = 32;

  for ( LevelType = CPUID_EXTENDED_TOPOLOGY_LEVEL_TYPE_SMT
        ; LevelType <= CPUID_V2_EXTENDED_TOPOLOGY_LEVEL_TYPE_DIE + 1
        ; LevelType++
        )
  {
    if (Location[LevelType] != NULL) {
      //
      // Bits[i] holds the number of bits to shift right on x2APIC ID to get a unique
      // topology ID of the next level type.
      //
      *Location[LevelType] = InitialApicId >> Bits[LevelType - 1];

      //
      // Bits[i] - Bits[i-1] holds the number of bits for the next ONE level type.
      //
      *Location[LevelType] &= (1 << (Bits[LevelType] - Bits[LevelType - 1])) - 1;
    }
  }
}
