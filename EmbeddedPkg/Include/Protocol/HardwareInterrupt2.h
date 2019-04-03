/** @file

  Copyright (c) 2016-2017, Linaro Ltd. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __HARDWARE_INTERRUPT2_H__
#define __HARDWARE_INTERRUPT2_H__

#include <Protocol/HardwareInterrupt.h>

// 22838932-1a2d-4a47-aaba-f3f7cf569470

#define EFI_HARDWARE_INTERRUPT2_PROTOCOL_GUID \
  { 0x32898322, 0x2d1a, 0x474a, \
    { 0xba, 0xaa, 0xf3, 0xf7, 0xcf, 0x56, 0x94, 0x70 } }

typedef enum {
  EFI_HARDWARE_INTERRUPT2_TRIGGER_LEVEL_LOW,
  EFI_HARDWARE_INTERRUPT2_TRIGGER_LEVEL_HIGH,
  EFI_HARDWARE_INTERRUPT2_TRIGGER_EDGE_FALLING,
  EFI_HARDWARE_INTERRUPT2_TRIGGER_EDGE_RISING,
} EFI_HARDWARE_INTERRUPT2_TRIGGER_TYPE;

typedef struct _EFI_HARDWARE_INTERRUPT2_PROTOCOL \
                 EFI_HARDWARE_INTERRUPT2_PROTOCOL;

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
(EFIAPI *HARDWARE_INTERRUPT2_REGISTER) (
  IN EFI_HARDWARE_INTERRUPT2_PROTOCOL *This,
  IN HARDWARE_INTERRUPT_SOURCE Source,
  IN HARDWARE_INTERRUPT_HANDLER Handler
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
(EFIAPI *HARDWARE_INTERRUPT2_ENABLE) (
  IN EFI_HARDWARE_INTERRUPT2_PROTOCOL *This,
  IN HARDWARE_INTERRUPT_SOURCE Source
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
(EFIAPI *HARDWARE_INTERRUPT2_DISABLE) (
  IN EFI_HARDWARE_INTERRUPT2_PROTOCOL *This,
  IN HARDWARE_INTERRUPT_SOURCE Source
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
(EFIAPI *HARDWARE_INTERRUPT2_INTERRUPT_STATE) (
  IN EFI_HARDWARE_INTERRUPT2_PROTOCOL *This,
  IN HARDWARE_INTERRUPT_SOURCE Source,
  IN BOOLEAN *InterruptState
  );

/**
  Signal to the hardware that the End Of Interrupt state
  has been reached.

  @param This     Instance pointer for this protocol
  @param Source   Hardware source of the interrupt

  @retval EFI_SUCCESS       Source interrupt EOI'ed.
  @retval EFI_DEVICE_ERROR  Hardware could not be programmed.

**/
typedef
EFI_STATUS
(EFIAPI *HARDWARE_INTERRUPT2_END_OF_INTERRUPT) (
  IN EFI_HARDWARE_INTERRUPT2_PROTOCOL *This,
  IN HARDWARE_INTERRUPT_SOURCE Source
  );

/**
  Return the configured trigger type for an interrupt source

  @param This         Instance pointer for this protocol
  @param Source       Hardware source of the interrupt
  @param TriggerType  The configured trigger type

  @retval EFI_SUCCESS       Operation successful
  @retval EFI_DEVICE_ERROR  Information could not be returned

**/
typedef
EFI_STATUS
(EFIAPI *HARDWARE_INTERRUPT2_GET_TRIGGER_TYPE) (
  IN  EFI_HARDWARE_INTERRUPT2_PROTOCOL *This,
  IN  HARDWARE_INTERRUPT_SOURCE Source,
  OUT EFI_HARDWARE_INTERRUPT2_TRIGGER_TYPE *TriggerType
  );


/**
 Configure the trigger type for an interrupt source

  @param This         Instance pointer for this protocol
  @param Source       Hardware source of the interrupt
  @param TriggerType  The trigger type to configure

  @retval EFI_SUCCESS       Operation successful
  @retval EFI_DEVICE_ERROR  Hardware could not be programmed.

**/
typedef
EFI_STATUS
(EFIAPI *HARDWARE_INTERRUPT2_SET_TRIGGER_TYPE) (
  IN  EFI_HARDWARE_INTERRUPT2_PROTOCOL *This,
  IN  HARDWARE_INTERRUPT_SOURCE Source,
  IN  EFI_HARDWARE_INTERRUPT2_TRIGGER_TYPE TriggerType
  );

struct _EFI_HARDWARE_INTERRUPT2_PROTOCOL {
  HARDWARE_INTERRUPT2_REGISTER            RegisterInterruptSource;
  HARDWARE_INTERRUPT2_ENABLE              EnableInterruptSource;
  HARDWARE_INTERRUPT2_DISABLE             DisableInterruptSource;
  HARDWARE_INTERRUPT2_INTERRUPT_STATE     GetInterruptSourceState;
  HARDWARE_INTERRUPT2_END_OF_INTERRUPT    EndOfInterrupt;

  // v2 members
  HARDWARE_INTERRUPT2_GET_TRIGGER_TYPE    GetTriggerType;
  HARDWARE_INTERRUPT2_SET_TRIGGER_TYPE    SetTriggerType;
};

extern EFI_GUID gHardwareInterrupt2ProtocolGuid;

#endif
