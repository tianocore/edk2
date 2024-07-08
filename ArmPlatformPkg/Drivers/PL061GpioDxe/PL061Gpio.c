/** @file

  Copyright (c) 2011 - 2020, Arm Limited. All rights reserved.<BR>
  Copyright (c) 2016, Linaro Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include <Protocol/EmbeddedGpio.h>

#include "PL061Gpio.h"

PLATFORM_GPIO_CONTROLLER  *mPL061PlatformGpio;

EFI_STATUS
EFIAPI
PL061Locate (
  IN  EMBEDDED_GPIO_PIN  Gpio,
  OUT UINTN              *ControllerIndex,
  OUT UINTN              *ControllerOffset,
  OUT UINTN              *RegisterBase
  )
{
  UINT32  Index;

  for (Index = 0; Index < mPL061PlatformGpio->GpioControllerCount; Index++) {
    if (  (Gpio >= mPL061PlatformGpio->GpioController[Index].GpioIndex)
       &&  (Gpio < mPL061PlatformGpio->GpioController[Index].GpioIndex
            + mPL061PlatformGpio->GpioController[Index].InternalGpioCount))
    {
      *ControllerIndex  = Index;
      *ControllerOffset = Gpio % mPL061PlatformGpio->GpioController[Index].InternalGpioCount;
      *RegisterBase     = mPL061PlatformGpio->GpioController[Index].RegisterBase;
      return EFI_SUCCESS;
    }
  }

  DEBUG ((DEBUG_ERROR, "%a, failed to locate gpio %d\n", __func__, Gpio));
  return EFI_INVALID_PARAMETER;
}

//
// The PL061 is a strange beast. The 8-bit data register is aliased across a
// region 0x400 bytes in size, with bits [9:2] of the address operating as a
// mask for both read and write operations:
// For reads:
//   - All bits where their corresponding mask bit is 1 return the current
//     value of that bit in the GPIO_DATA register.
//   - All bits where their corresponding mask bit is 0 return 0.
// For writes:
//   - All bits where their corresponding mask bit is 1 set the bit in the
//     GPIO_DATA register to the written value.
//   - All bits where their corresponding mask bit is 0 are left untouched
//     in the GPIO_DATA register.
//
// To keep this driver intelligible, PL061EffectiveAddress, PL061GetPins and
// Pl061SetPins provide an internal abstraction from this interface.

STATIC
UINTN
EFIAPI
PL061EffectiveAddress (
  IN UINTN  Address,
  IN UINTN  Mask
  )
{
  return ((Address + PL061_GPIO_DATA_REG_OFFSET) + (Mask << 2));
}

STATIC
UINTN
EFIAPI
PL061GetPins (
  IN UINTN  Address,
  IN UINTN  Mask
  )
{
  return MmioRead8 (PL061EffectiveAddress (Address, Mask));
}

STATIC
VOID
EFIAPI
PL061SetPins (
  IN UINTN  Address,
  IN UINTN  Mask,
  IN UINTN  Value
  )
{
  MmioWrite8 (PL061EffectiveAddress (Address, Mask), Value);
}

/**
  Function implementations
**/
EFI_STATUS
PL061Identify (
  VOID
  )
{
  UINTN  Index;
  UINTN  RegisterBase;

  if (  (mPL061PlatformGpio->GpioCount == 0)
     || (mPL061PlatformGpio->GpioControllerCount == 0))
  {
    return EFI_NOT_FOUND;
  }

  for (Index = 0; Index < mPL061PlatformGpio->GpioControllerCount; Index++) {
    if (mPL061PlatformGpio->GpioController[Index].InternalGpioCount != PL061_GPIO_PINS) {
      return EFI_INVALID_PARAMETER;
    }

    RegisterBase = mPL061PlatformGpio->GpioController[Index].RegisterBase;

    // Check if this is a PrimeCell Peripheral
    if (  (MmioRead8 (RegisterBase + PL061_GPIO_PCELL_ID0) != 0x0D)
       ||  (MmioRead8 (RegisterBase + PL061_GPIO_PCELL_ID1) != 0xF0)
       ||  (MmioRead8 (RegisterBase + PL061_GPIO_PCELL_ID2) != 0x05)
       ||  (MmioRead8 (RegisterBase + PL061_GPIO_PCELL_ID3) != 0xB1))
    {
      return EFI_NOT_FOUND;
    }

    // Check if this PrimeCell Peripheral is the PL061 GPIO
    if (  (MmioRead8 (RegisterBase + PL061_GPIO_PERIPH_ID0) != 0x61)
       ||  (MmioRead8 (RegisterBase + PL061_GPIO_PERIPH_ID1) != 0x10)
       ||  ((MmioRead8 (RegisterBase + PL061_GPIO_PERIPH_ID2) & 0xF) != 0x04)
       ||  (MmioRead8 (RegisterBase + PL061_GPIO_PERIPH_ID3) != 0x00))
    {
      return EFI_NOT_FOUND;
    }
  }

  return EFI_SUCCESS;
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
  IN  EMBEDDED_GPIO      *This,
  IN  EMBEDDED_GPIO_PIN  Gpio,
  OUT UINTN              *Value
  )
{
  EFI_STATUS  Status;
  UINTN       Index, Offset, RegisterBase;

  if (Value == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = PL061Locate (Gpio, &Index, &Offset, &RegisterBase);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  if (PL061GetPins (RegisterBase, GPIO_PIN_MASK (Offset)) != 0) {
    *Value = 1;
  } else {
    *Value = 0;
  }

  return EFI_SUCCESS;
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
  EFI_STATUS  Status;
  UINTN       Index, Offset, RegisterBase;

  Status = PL061Locate (Gpio, &Index, &Offset, &RegisterBase);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  switch (Mode) {
    case GPIO_MODE_INPUT:
      // Set the corresponding direction bit to LOW for input
      MmioAnd8 (
        RegisterBase + PL061_GPIO_DIR_REG,
        ~GPIO_PIN_MASK(Offset) & 0xFF
        );
      break;

    case GPIO_MODE_OUTPUT_0:
      // Set the corresponding direction bit to HIGH for output
      MmioOr8 (RegisterBase + PL061_GPIO_DIR_REG, GPIO_PIN_MASK (Offset));
      // Set the corresponding data bit to LOW for 0
      PL061SetPins (RegisterBase, GPIO_PIN_MASK (Offset), 0);
      break;

    case GPIO_MODE_OUTPUT_1:
      // Set the corresponding direction bit to HIGH for output
      MmioOr8 (RegisterBase + PL061_GPIO_DIR_REG, GPIO_PIN_MASK (Offset));
      // Set the corresponding data bit to HIGH for 1
      PL061SetPins (RegisterBase, GPIO_PIN_MASK (Offset), 0xff);
      break;

    default:
      // Other modes are not supported
      return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
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
  EFI_STATUS  Status;
  UINTN       Index, Offset, RegisterBase;

  // Check for errors
  if (Mode == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = PL061Locate (Gpio, &Index, &Offset, &RegisterBase);
  if (EFI_ERROR (Status)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  // Check if it is input or output
  if (MmioRead8 (RegisterBase + PL061_GPIO_DIR_REG) & GPIO_PIN_MASK (Offset)) {
    // Pin set to output
    if (PL061GetPins (RegisterBase, GPIO_PIN_MASK (Offset)) != 0) {
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
EMBEDDED_GPIO  gGpio = {
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
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS       Status;
  EFI_HANDLE       Handle;
  GPIO_CONTROLLER  *GpioController;

  //
  // Make sure the Gpio protocol has not been installed in the system yet.
  //
  ASSERT_PROTOCOL_ALREADY_INSTALLED (NULL, &gEmbeddedGpioProtocolGuid);

  Status = gBS->LocateProtocol (&gPlatformGpioProtocolGuid, NULL, (VOID **)&mPL061PlatformGpio);
  if (EFI_ERROR (Status) && (Status == EFI_NOT_FOUND)) {
    // Create the mPL061PlatformGpio
    mPL061PlatformGpio = (PLATFORM_GPIO_CONTROLLER *)AllocateZeroPool (sizeof (PLATFORM_GPIO_CONTROLLER) + sizeof (GPIO_CONTROLLER));
    if (mPL061PlatformGpio == NULL) {
      DEBUG ((DEBUG_ERROR, "%a: failed to allocate PLATFORM_GPIO_CONTROLLER\n", __func__));
      return EFI_BAD_BUFFER_SIZE;
    }

    mPL061PlatformGpio->GpioCount           = PL061_GPIO_PINS;
    mPL061PlatformGpio->GpioControllerCount = 1;
    mPL061PlatformGpio->GpioController      = (GPIO_CONTROLLER *)((UINTN)mPL061PlatformGpio + sizeof (PLATFORM_GPIO_CONTROLLER));

    GpioController                    = mPL061PlatformGpio->GpioController;
    GpioController->RegisterBase      = (UINTN)PcdGet32 (PcdPL061GpioBase);
    GpioController->GpioIndex         = 0;
    GpioController->InternalGpioCount = PL061_GPIO_PINS;
  }

  Status = PL061Identify ();
  if (EFI_ERROR (Status)) {
    return EFI_DEVICE_ERROR;
  }

  // Install the Embedded GPIO Protocol onto a new handle
  Handle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Handle,
                  &gEmbeddedGpioProtocolGuid,
                  &gGpio,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    Status = EFI_OUT_OF_RESOURCES;
  }

  return Status;
}
