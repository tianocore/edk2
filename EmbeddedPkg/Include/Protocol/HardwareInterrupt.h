/** @file
  Abstraction for hardware based interrupt routine

  On non IA-32 systems it is common to have a single hardware interrupt vector
  and a 2nd layer of software that routes the interrupt handlers based on the
  interrupt source. This protocol enables this routing. The driver implementing
  this protocol is responsible for clearing the pending interrupt in the
  interrupt routing hardware. The HARDWARE_INTERRUPT_HANDLER is responsible
  for clearing interrupt sources from individual devices.


  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __HARDWARE_INTERRUPT_H__
#define __HARDWARE_INTERRUPT_H__

#include <Protocol/DebugSupport.h>


//
// Protocol GUID
//
// EAB39028-3D05-4316-AD0C-D64808DA3FF1

#define EFI_HARDWARE_INTERRUPT_PROTOCOL_GGUID \
  { 0x2890B3EA, 0x053D, 0x1643, { 0xAD, 0x0C, 0xD6, 0x48, 0x08, 0xDA, 0x3F, 0xF1 } }


typedef struct _EFI_HARDWARE_INTERRUPT_PROTOCOL EFI_HARDWARE_INTERRUPT_PROTOCOL;


typedef UINTN HARDWARE_INTERRUPT_SOURCE;


/**
  C Interrupt Handler calledin the interrupt context when Source interrupt is active.

  @param Source         Source of the interrupt. Hardware routing off a specific platform defines
                        what source means.
  @param SystemContext  Pointer to system register context. Mostly used by debuggers and will
                        update the system context after the return from the interrupt if
                        modified. Don't change these values unless you know what you are doing

**/
typedef
VOID
(EFIAPI *HARDWARE_INTERRUPT_HANDLER) (
  IN  HARDWARE_INTERRUPT_SOURCE   Source,
  IN  EFI_SYSTEM_CONTEXT          SystemContext
  );


/**
  Register Handler for the specified interrupt source.

  @param This     Instance pointer for this protocol
  @param Source   Hardware source of the interrupt
  @param Handler  Callback for interrupt. NULL to unregister

  @retval EFI_SUCCESS Source was updated to support Handler.
  @retval EFI_DEVICE_ERROR  Hardware could not be programmed.

**/
typedef
EFI_STATUS
(EFIAPI *HARDWARE_INTERRUPT_REGISTER) (
  IN EFI_HARDWARE_INTERRUPT_PROTOCOL    *This,
  IN HARDWARE_INTERRUPT_SOURCE          Source,
  IN HARDWARE_INTERRUPT_HANDLER         Handler
  );


/**
  Enable interrupt source Source.

  @param This     Instance pointer for this protocol
  @param Source   Hardware source of the interrupt

  @retval EFI_SUCCESS       Source interrupt enabled.
  @retval EFI_DEVICE_ERROR  Hardware could not be programmed.

**/
typedef
EFI_STATUS
(EFIAPI *HARDWARE_INTERRUPT_ENABLE) (
  IN EFI_HARDWARE_INTERRUPT_PROTOCOL    *This,
  IN HARDWARE_INTERRUPT_SOURCE          Source
  );



/**
  Disable interrupt source Source.

  @param This     Instance pointer for this protocol
  @param Source   Hardware source of the interrupt

  @retval EFI_SUCCESS       Source interrupt disabled.
  @retval EFI_DEVICE_ERROR  Hardware could not be programmed.

**/
typedef
EFI_STATUS
(EFIAPI *HARDWARE_INTERRUPT_DISABLE) (
  IN EFI_HARDWARE_INTERRUPT_PROTOCOL    *This,
  IN HARDWARE_INTERRUPT_SOURCE          Source
  );


/**
  Return current state of interrupt source Source.

  @param This     Instance pointer for this protocol
  @param Source   Hardware source of the interrupt
  @param InterruptState  TRUE: source enabled, FALSE: source disabled.

  @retval EFI_SUCCESS       InterruptState is valid
  @retval EFI_DEVICE_ERROR  InterruptState is not valid

**/
typedef
EFI_STATUS
(EFIAPI *HARDWARE_INTERRUPT_INTERRUPT_STATE) (
  IN EFI_HARDWARE_INTERRUPT_PROTOCOL    *This,
  IN HARDWARE_INTERRUPT_SOURCE          Source,
  IN BOOLEAN                            *InterruptState
  );

/**
  Signal to the hardware that the End Of Intrrupt state
  has been reached.

  @param This     Instance pointer for this protocol
  @param Source   Hardware source of the interrupt

  @retval EFI_SUCCESS       Source interrupt EOI'ed.
  @retval EFI_DEVICE_ERROR  Hardware could not be programmed.

**/
typedef
EFI_STATUS
(EFIAPI *HARDWARE_INTERRUPT_END_OF_INTERRUPT) (
  IN EFI_HARDWARE_INTERRUPT_PROTOCOL    *This,
  IN HARDWARE_INTERRUPT_SOURCE          Source
  );


struct _EFI_HARDWARE_INTERRUPT_PROTOCOL {
  HARDWARE_INTERRUPT_REGISTER         RegisterInterruptSource;
  HARDWARE_INTERRUPT_ENABLE           EnableInterruptSource;
  HARDWARE_INTERRUPT_DISABLE          DisableInterruptSource;
  HARDWARE_INTERRUPT_INTERRUPT_STATE  GetInterruptSourceState;
  HARDWARE_INTERRUPT_END_OF_INTERRUPT EndOfInterrupt;
};

extern EFI_GUID gHardwareInterruptProtocolGuid;

#endif


