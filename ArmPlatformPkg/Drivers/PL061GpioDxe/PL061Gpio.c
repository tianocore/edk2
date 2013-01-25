/** @file
*
*  Copyright (c) 2011, ARM Limited. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/


#include <PiDxe.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include <Protocol/EmbeddedGpio.h>
#include <Drivers/PL061Gpio.h>

BOOLEAN     mPL061Initialized = FALSE;

/**
  Function implementations
**/

EFI_STATUS
PL061Identify (
  VOID
  )
{
  // Check if this is a PrimeCell Peripheral
  if (    (MmioRead8 (PL061_GPIO_PCELL_ID0) != 0x0D)
      ||  (MmioRead8 (PL061_GPIO_PCELL_ID1) != 0xF0)
      ||  (MmioRead8 (PL061_GPIO_PCELL_ID2) != 0x05)
      ||  (MmioRead8 (PL061_GPIO_PCELL_ID3) != 0xB1)) {
    return EFI_NOT_FOUND;
  }

  // Check if this PrimeCell Peripheral is the PL061 GPIO
  if (    (MmioRead8 (PL061_GPIO_PERIPH_ID0) != 0x61)
      ||  (MmioRead8 (PL061_GPIO_PERIPH_ID1) != 0x10)
      ||  ((MmioRead8 (PL061_GPIO_PERIPH_ID2) & 0xF) != 0x04)
      ||  (MmioRead8 (PL061_GPIO_PERIPH_ID3) != 0x00)) {
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
PL061Initialize (
  VOID
  )
{
  EFI_STATUS  Status;

  // Check if the PL061 GPIO module exists on board
  Status = PL061Identify();
  if (EFI_ERROR (Status)) {
    Status = EFI_DEVICE_ERROR;
    goto EXIT;
  }

  // Do other hardware initialisation things here as required

  // Disable Interrupts
  //if (MmioRead8 (PL061_GPIO_IE_REG) != 0) {
  //   // Ensure interrupts are disabled
  //}

  mPL061Initialized = TRUE;

  EXIT:
  return Status;
}

/**

Routine Description:

  Gets the state of a GPIO pin

Arguments:

  This  - pointer to protocol
  Gpio  - which pin to read
  Value - state of the pin

Returns:

  EFI_SUCCESS           - GPIO state returned in Value
  EFI_INVALID_PARAMETER - Value is NULL pointer or Gpio pin is out of range
**/
EFI_STATUS
EFIAPI
Get (
  IN  EMBEDDED_GPIO     *This,
  IN  EMBEDDED_GPIO_PIN Gpio,
  OUT UINTN             *Value
  )
{
  EFI_STATUS    Status = EFI_SUCCESS;

  if (    (Value == NULL)
      ||  (Gpio > LAST_GPIO_PIN))
  {
    return EFI_INVALID_PARAMETER;
  }

  // Initialize the hardware if not already done
  if (!mPL061Initialized) {
    Status = PL061Initialize();
    if (EFI_ERROR(Status)) {
      goto EXIT;
    }
  }

  if (MmioRead8 (PL061_GPIO_DATA_REG) & GPIO_PIN_MASK_HIGH_8BIT(Gpio)) {
    *Value = 1;
  } else {
    *Value = 0;
  }

  EXIT:
  return Status;
}

/**

Routine Description:

  Sets the state of a GPIO pin

Arguments:

  This  - pointer to protocol
  Gpio  - which pin to modify
  Mode  - mode to set

Returns:

  EFI_SUCCESS           - GPIO set as requested
  EFI_UNSUPPORTED       - Mode is not supported
  EFI_INVALID_PARAMETER - Gpio pin is out of range
**/
EFI_STATUS
EFIAPI
Set (
  IN  EMBEDDED_GPIO       *This,
  IN  EMBEDDED_GPIO_PIN   Gpio,
  IN  EMBEDDED_GPIO_MODE  Mode
  )
{
  EFI_STATUS    Status = EFI_SUCCESS;

  // Check for errors
  if (Gpio > LAST_GPIO_PIN) {
    Status = EFI_INVALID_PARAMETER;
    goto EXIT;
  }

  // Initialize the hardware if not already done
  if (!mPL061Initialized) {
    Status = PL061Initialize();
    if (EFI_ERROR(Status)) {
      goto EXIT;
    }
  }

  switch (Mode)
  {
    case GPIO_MODE_INPUT:
      // Set the corresponding direction bit to LOW for input
      MmioAnd8 (PL061_GPIO_DIR_REG, GPIO_PIN_MASK_LOW_8BIT(Gpio));
      break;

    case GPIO_MODE_OUTPUT_0:
      // Set the corresponding data bit to LOW for 0
      MmioAnd8 (PL061_GPIO_DATA_REG, GPIO_PIN_MASK_LOW_8BIT(Gpio));
      // Set the corresponding direction bit to HIGH for output
      MmioOr8 (PL061_GPIO_DIR_REG, GPIO_PIN_MASK_HIGH_8BIT(Gpio));
      break;

    case GPIO_MODE_OUTPUT_1:
      // Set the corresponding data bit to HIGH for 1
      MmioOr8 (PL061_GPIO_DATA_REG, GPIO_PIN_MASK_HIGH_8BIT(Gpio));
      // Set the corresponding direction bit to HIGH for output
      MmioOr8 (PL061_GPIO_DIR_REG, GPIO_PIN_MASK_HIGH_8BIT(Gpio));
      break;

    default:
      // Other modes are not supported
      return EFI_UNSUPPORTED;
  }

EXIT:
  return Status;
}

/**

Routine Description:

  Gets the mode (function) of a GPIO pin

Arguments:

  This  - pointer to protocol
  Gpio  - which pin
  Mode  - pointer to output mode value

Returns:

  EFI_SUCCESS           - mode value retrieved
  EFI_INVALID_PARAMETER - Mode is a null pointer or Gpio pin is out of range

**/
EFI_STATUS
EFIAPI
GetMode (
  IN  EMBEDDED_GPIO       *This,
  IN  EMBEDDED_GPIO_PIN   Gpio,
  OUT EMBEDDED_GPIO_MODE  *Mode
  )
{
  EFI_STATUS Status;

  // Check for errors
  if (    (Mode == NULL)
      ||  (Gpio > LAST_GPIO_PIN)) {
    return EFI_INVALID_PARAMETER;
  }

  // Initialize the hardware if not already done
  if (!mPL061Initialized) {
    Status = PL061Initialize();
    if (EFI_ERROR(Status)) {
      return Status;
    }
  }

  // Check if it is input or output
  if (MmioRead8 (PL061_GPIO_DIR_REG) & GPIO_PIN_MASK_HIGH_8BIT(Gpio)) {
    // Pin set to output
    if (MmioRead8 (PL061_GPIO_DATA_REG) & GPIO_PIN_MASK_HIGH_8BIT(Gpio)) {
      *Mode = GPIO_MODE_OUTPUT_1;
    } else {
      *Mode = GPIO_MODE_OUTPUT_0;
    }
  } else {
    // Pin set to input
    *Mode = GPIO_MODE_INPUT;
  }

  return EFI_SUCCESS;
}

/**

Routine Description:

  Sets the pull-up / pull-down resistor of a GPIO pin

Arguments:

  This  - pointer to protocol
  Gpio  - which pin
  Direction - pull-up, pull-down, or none

Returns:

  EFI_UNSUPPORTED - Can not perform the requested operation

**/
EFI_STATUS
EFIAPI
SetPull (
  IN  EMBEDDED_GPIO       *This,
  IN  EMBEDDED_GPIO_PIN   Gpio,
  IN  EMBEDDED_GPIO_PULL  Direction
  )
{
  return EFI_UNSUPPORTED;
}

/**
 Protocol variable definition
 **/
EMBEDDED_GPIO gGpio = {
  Get,
  Set,
  GetMode,
  SetPull
};

/**
  Initialize the state information for the Embedded Gpio protocol.

  @param  ImageHandle   of the loaded driver
  @param  SystemTable   Pointer to the System Table

  @retval EFI_SUCCESS           Protocol registered
  @retval EFI_OUT_OF_RESOURCES  Cannot allocate protocol data structure
  @retval EFI_DEVICE_ERROR      Hardware problems

**/
EFI_STATUS
EFIAPI
PL061InstallProtocol (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  Handle;

  //
  // Make sure the Gpio protocol has not been installed in the system yet.
  //
  ASSERT_PROTOCOL_ALREADY_INSTALLED (NULL, &gEmbeddedGpioProtocolGuid);

  // Install the Embedded GPIO Protocol onto a new handle
  Handle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces(
                  &Handle,
                  &gEmbeddedGpioProtocolGuid, &gGpio,
                  NULL
                 );
  if (EFI_ERROR(Status)) {
    Status = EFI_OUT_OF_RESOURCES;
  }

  return Status;
}
