/** @file

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __EMBEDDED_GPIO_H__
#define __EMBEDDED_GPIO_H__

//
// Protocol interface structure
//
typedef struct _EMBEDDED_GPIO   EMBEDDED_GPIO;

//
// Data Types
//
typedef UINTN EMBEDDED_GPIO_PIN;

#define GPIO(Port, Pin) ((EMBEDDED_GPIO_PIN)(((Port) << (16)) | (Pin)))
#define GPIO_PIN(x)     ((EMBEDDED_GPIO_PIN)(x) & (0xFFFF))
#define GPIO_PORT(x)    ((EMBEDDED_GPIO_PIN)(x) >> (16))

typedef enum {
  GPIO_MODE_INPUT                 = 0x00,
  GPIO_MODE_OUTPUT_0              = 0x0E,
  GPIO_MODE_OUTPUT_1              = 0x0F,
  GPIO_MODE_SPECIAL_FUNCTION_2    = 0x02,
  GPIO_MODE_SPECIAL_FUNCTION_3    = 0x03,
  GPIO_MODE_SPECIAL_FUNCTION_4    = 0x04,
  GPIO_MODE_SPECIAL_FUNCTION_5    = 0x05,
  GPIO_MODE_SPECIAL_FUNCTION_6    = 0x06,
  GPIO_MODE_SPECIAL_FUNCTION_7    = 0x07
} EMBEDDED_GPIO_MODE;

typedef enum {
  GPIO_PULL_NONE,
  GPIO_PULL_UP,
  GPIO_PULL_DOWN
} EMBEDDED_GPIO_PULL;

//
// Function Prototypes
//
typedef
EFI_STATUS
(EFIAPI *EMBEDDED_GPIO_GET) (
  IN  EMBEDDED_GPIO       *This,
  IN  EMBEDDED_GPIO_PIN   Gpio,
  OUT UINTN               *Value
  );
/*++

Routine Description:

  Gets the state of a GPIO pin

Arguments:

  This  - pointer to protocol
  Gpio  - which pin to read
  Value - state of the pin

Returns:

  EFI_SUCCESS - GPIO state returned in Value

--*/


typedef
EFI_STATUS
(EFIAPI *EMBEDDED_GPIO_SET) (
    IN EMBEDDED_GPIO      *This,
    IN EMBEDDED_GPIO_PIN  Gpio,
    IN EMBEDDED_GPIO_MODE Mode
    );
/*++

Routine Description:

  Sets the state of a GPIO pin

Arguments:

  This  - pointer to protocol
  Gpio  - which pin to modify
  Mode  - mode to set

Returns:

  EFI_SUCCESS - GPIO set as requested

--*/


typedef
EFI_STATUS
(EFIAPI *EMBEDDED_GPIO_GET_MODE) (
    IN  EMBEDDED_GPIO         *This,
    IN  EMBEDDED_GPIO_PIN     Gpio,
    OUT EMBEDDED_GPIO_MODE    *Mode
    );
/*++

Routine Description:

  Gets the mode (function) of a GPIO pin

Arguments:

  This  - pointer to protocol
  Gpio  - which pin
  Mode  - pointer to output mode value

Returns:

  EFI_SUCCESS - mode value retrieved

--*/


typedef
EFI_STATUS
(EFIAPI *EMBEDDED_GPIO_SET_PULL) (
    IN  EMBEDDED_GPIO      *This,
    IN  EMBEDDED_GPIO_PIN  Gpio,
    IN  EMBEDDED_GPIO_PULL Direction
    );
/*++

Routine Description:

  Sets the pull-up / pull-down resistor of a GPIO pin

Arguments:

  This  - pointer to protocol
  Gpio  - which pin
  Direction - pull-up, pull-down, or none

Returns:

  EFI_SUCCESS - pin was set

--*/



struct _EMBEDDED_GPIO {
  EMBEDDED_GPIO_GET       Get;
  EMBEDDED_GPIO_SET       Set;
  EMBEDDED_GPIO_GET_MODE  GetMode;
  EMBEDDED_GPIO_SET_PULL  SetPull;
};

extern EFI_GUID gEmbeddedGpioProtocolGuid;

typedef struct _GPIO_CONTROLLER              GPIO_CONTROLLER;
typedef struct _PLATFORM_GPIO_CONTROLLER     PLATFORM_GPIO_CONTROLLER;

struct _GPIO_CONTROLLER {
  UINTN                   RegisterBase;
  UINTN                   GpioIndex;
  UINTN                   InternalGpioCount;
};

struct _PLATFORM_GPIO_CONTROLLER {
  UINTN                   GpioCount;
  UINTN                   GpioControllerCount;
  GPIO_CONTROLLER         *GpioController;
};

extern EFI_GUID gPlatformGpioProtocolGuid;

#endif
