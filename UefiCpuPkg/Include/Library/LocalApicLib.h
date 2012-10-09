/** @file
  Public include file for Local APIC library.

  Local APIC library assumes local APIC is enabled. It does not
  handles cases where local APIC is disabled.

  Copyright (c) 2010 - 2012, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __LOCAL_APIC_LIB_H__
#define __LOCAL_APIC_LIB_H__

#define LOCAL_APIC_MODE_XAPIC   0x1  ///< xAPIC mode.
#define LOCAL_APIC_MODE_X2APIC  0x2  ///< x2APIC mode.

/**
  Retrieve the base address of local APIC.

  @return The base address of local APIC.

**/
UINTN
EFIAPI
GetLocalApicBaseAddress (
  VOID
  );

/**
  Set the base address of local APIC.

  If BaseAddress is not aligned on a 4KB boundary, then ASSERT().

  @param[in] BaseAddress   Local APIC base address to be set.

**/
VOID
EFIAPI
SetLocalApicBaseAddress (
  IN UINTN                BaseAddress
  );

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
  );

/**
  Set the current local APIC mode.

  If the specified local APIC mode is not valid, then ASSERT.
  If the specified local APIC mode can't be set as current, then ASSERT.

  @param ApicMode APIC mode to be set.
**/
VOID
EFIAPI
SetApicMode (
  IN UINTN  ApicMode
  );

/**
  Get the initial local APIC ID of the executing processor assigned by hardware upon power on or reset.

  In xAPIC mode, the initial local APIC ID is 8-bit, and may be different from current APIC ID.
  In x2APIC mode, the local APIC ID can't be changed and there is no concept of initial APIC ID. In this case, 
  the 32-bit local APIC ID is returned as initial APIC ID.

  @return  32-bit initial local APIC ID of the executing processor.
**/
UINT32
EFIAPI
GetInitialApicId (
  VOID
  );

/**
  Get the local APIC ID of the executing processor.

  @return  32-bit local APIC ID of the executing processor.
**/
UINT32
EFIAPI
GetApicId (
  VOID
  );

/**
  Get the value of the local APIC version register.

  @return  the value of the local APIC version register.
**/
UINT32
EFIAPI
GetApicVersion (
  VOID
  );

/**
  Send a Fixed IPI to a specified target processor.

  This function returns after the IPI has been accepted by the target processor. 

  @param  ApicId   The local APIC ID of the target processor.
  @param  Vector   The vector number of the interrupt being sent.
**/
VOID
EFIAPI
SendFixedIpi (
  IN UINT32          ApicId,
  IN UINT8           Vector
  );

/**
  Send a Fixed IPI to all processors excluding self.

  This function returns after the IPI has been accepted by the target processors. 

  @param  Vector   The vector number of the interrupt being sent.
**/
VOID
EFIAPI
SendFixedIpiAllExcludingSelf (
  IN UINT8           Vector
  );

/**
  Send a SMI IPI to a specified target processor.

  This function returns after the IPI has been accepted by the target processor. 

  @param  ApicId   Specify the local APIC ID of the target processor.
**/
VOID
EFIAPI
SendSmiIpi (
  IN UINT32          ApicId
  );

/**
  Send a SMI IPI to all processors excluding self.

  This function returns after the IPI has been accepted by the target processors. 
**/
VOID
EFIAPI
SendSmiIpiAllExcludingSelf (
  VOID
  );

/**
  Send an INIT IPI to a specified target processor.

  This function returns after the IPI has been accepted by the target processor. 

  @param  ApicId   Specify the local APIC ID of the target processor.
**/
VOID
EFIAPI
SendInitIpi (
  IN UINT32          ApicId
  );

/**
  Send an INIT IPI to all processors excluding self.

  This function returns after the IPI has been accepted by the target processors. 
**/
VOID
EFIAPI
SendInitIpiAllExcludingSelf (
  VOID
  );

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
  IN UINT32          ApicId,
  IN UINT32          StartupRoutine
  );

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
  IN UINT32          StartupRoutine
  );

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
  );

/**
  Disable LINT0 & LINT1 interrupts.

  This function sets the mask flag in the LVT LINT0 & LINT1 registers.
**/
VOID
EFIAPI
DisableLvtInterrupts (
  VOID
  );

/**
  Read the initial count value from the init-count register.

  @return The initial count value read from the init-count register.
**/
UINT32
EFIAPI
GetApicTimerInitCount (
  VOID
  );

/**
  Read the current count value from the current-count register.

  @return The current count value read from the current-count register.
**/
UINT32
EFIAPI
GetApicTimerCurrentCount (
  VOID
  );

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
  IN UINTN   DivideValue,
  IN UINT32  InitCount,
  IN BOOLEAN PeriodicMode,
  IN UINT8   Vector
  );

/**
  Get the state of the local APIC timer.

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
  );

/**
  Enable the local APIC timer interrupt.
**/
VOID
EFIAPI
EnableApicTimerInterrupt (
  VOID
  );

/**
  Disable the local APIC timer interrupt.
**/
VOID
EFIAPI
DisableApicTimerInterrupt (
  VOID
  );

/**
  Get the local APIC timer interrupt state.

  @retval TRUE  The local APIC timer interrupt is enabled.
  @retval FALSE The local APIC timer interrupt is disabled.
**/
BOOLEAN
EFIAPI
GetApicTimerInterruptState (
  VOID
  );

/**
  Send EOI to the local APIC.
**/
VOID
EFIAPI
SendApicEoi (
  VOID
  );

/**
  Get the 32-bit address that a device should use to send a Message Signaled 
  Interrupt (MSI) to the Local APIC of the currently executing processor.

  @return 32-bit address used to send an MSI to the Local APIC.
**/
UINT32
EFIAPI    
GetApicMsiAddress (
  VOID
  );
    
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
  );
  
#endif

