/** @file

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>
  Copyright (c) 2017, Linaro, Ltd. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __EMBEDDED_GPIO_PPI_H__
#define __EMBEDDED_GPIO_PPI_H__

//
// Protocol interface structure
//
typedef struct _EMBEDDED_GPIO_PPI   EMBEDDED_GPIO_PPI;

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

/**

  Gets the state of a GPIO pin

  @param This                   Pointer to protocol
  @param Gpio                   Which pin to read
  @param Value                  State of the pin

  @retval EFI_SUCCESS           GPIO state returned in Value
  @retval EFI_INVALID_PARAMETER Value is NULL
  @retval EFI_NOT_FOUND         Pin does not exit

**/
typedef
EFI_STATUS
(EFIAPI *EMBEDDED_GPIO_GET) (
  IN  EMBEDDED_GPIO_PPI     *This,
  IN  EMBEDDED_GPIO_PIN     Gpio,
  OUT UINTN                 *Value
  );

/**

  Sets the state of a GPIO pin

  @param This                   Pointer to protocol
  @param Gpio                   Which pin to modify
  @param Mode                   Mode to set

  @retval EFI_SUCCESS           GPIO set as requested
  @retval EFI_INVALID_PARAMETER Invalid mode
  @retval EFI_NOT_FOUND         Pin does not exit

**/
typedef
EFI_STATUS
(EFIAPI *EMBEDDED_GPIO_SET) (
  IN EMBEDDED_GPIO_PPI      *This,
  IN EMBEDDED_GPIO_PIN      Gpio,
  IN EMBEDDED_GPIO_MODE     Mode
  );


/**

  Gets the mode (function) of a GPIO pin

  @param This                   Pointer to protocol
  @param Gpio                   Which pin
  @param Mode                   Pointer to output mode value

  @retval EFI_SUCCESS           Mode value retrieved
  @retval EFI_INVALID_PARAMETER Mode is NULL
  @retval EFI_NOT_FOUND         Pin does not exit

**/
typedef
EFI_STATUS
(EFIAPI *EMBEDDED_GPIO_GET_MODE) (
  IN  EMBEDDED_GPIO_PPI     *This,
  IN  EMBEDDED_GPIO_PIN     Gpio,
  OUT EMBEDDED_GPIO_MODE    *Mode
  );


/**

  Sets the pull-up / pull-down resistor of a GPIO pin

  @param This                   Pointer to PPI
  @param Gpio                   Port/pin index
  @param Pull                   The pullup/pulldown mode to set

  @retval EFI_SUCCESS           Mode was set
  @retval EFI_NOT_FOUND         Pin does not exist
  @retval EFI_UNSUPPORTED       Action not supported

**/
typedef
EFI_STATUS
(EFIAPI *EMBEDDED_GPIO_SET_PULL) (
  IN  EMBEDDED_GPIO_PPI     *This,
  IN  EMBEDDED_GPIO_PIN     Gpio,
  IN  EMBEDDED_GPIO_PULL    Direction
  );


struct _EMBEDDED_GPIO_PPI {
  EMBEDDED_GPIO_GET         Get;
  EMBEDDED_GPIO_SET         Set;
  EMBEDDED_GPIO_GET_MODE    GetMode;
  EMBEDDED_GPIO_SET_PULL    SetPull;
};

extern EFI_GUID gEmbeddedGpioPpiGuid;

#endif
