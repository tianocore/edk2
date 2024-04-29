/** @file

  Null implementation of SpiHcPlatformLib

  Copyright (C) 2024 Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <PiDxe.h>
#include <Protocol/DevicePath.h>
#include <Protocol/SpiHc.h>
#include <Library/SpiHcPlatformLib.h>

/**
  This function reports the details of the SPI Host Controller to the SpiHc driver.

  @param[out]     Attributes              The suported attributes of the SPI host controller
  @param[out]     FrameSizeSupportMask    The suported FrameSizeSupportMask of the SPI host controller
  @param[out]     MaximumTransferBytes    The suported MaximumTransferBytes of the SPI host controller

  @retval EFI_UNSUPPORTED
**/
EFI_STATUS
EFIAPI
GetPlatformSpiHcDetails (
  OUT     UINT32  *Attributes,
  OUT     UINT32  *FrameSizeSupportMask,
  OUT     UINT32  *MaximumTransferBytes
  )
{
  return EFI_UNSUPPORTED;
}

/**
  This function reports the device path of SPI host controller. This is needed in order for the SpiBus
  to match the correct SPI_BUS to the SPI host controller

  @param[out] DevicePath The device path for this SPI HC is returned in this variable

  @retval EFI_UNSUPPORTED
**/
EFI_STATUS
EFIAPI
GetSpiHcDevicePath (
  OUT EFI_DEVICE_PATH_PROTOCOL  **DevicePath
  )
{
  return EFI_UNSUPPORTED;
}

/**
  This is the platform specific Spi Chip select function.
  Assert or deassert the SPI chip select.

  This routine is called at TPL_NOTIFY.
  Update the value of the chip select line for a SPI peripheral. The SPI bus
  layer calls this routine either in the board layer or in the SPI controller
  to manipulate the chip select pin at the start and end of a SPI transaction.

  @param[in] This           Pointer to an EFI_SPI_HC_PROTOCOL structure.
  @param[in] SpiPeripheral  The address of an EFI_SPI_PERIPHERAL data structure
                            describing the SPI peripheral whose chip select pin
                            is to be manipulated. The routine may access the
                            ChipSelectParameter field to gain sufficient
                            context to complete the operati on.
  @param[in] PinValue       The value to be applied to the chip select line of
                            the SPI peripheral.

  @retval EFI_UNSUPPORTED

**/
EFI_STATUS
EFIAPI
PlatformSpiHcChipSelect (
  IN CONST EFI_SPI_HC_PROTOCOL  *This,
  IN CONST EFI_SPI_PERIPHERAL   *SpiPeripheral,
  IN BOOLEAN                    PinValue
  )
{
  return EFI_UNSUPPORTED;
}

/**
  This function is the platform specific SPI clock function.
  Set up the clock generator to produce the correct clock frequency, phase and
  polarity for a SPI chip.

  This routine is called at TPL_NOTIFY.
  This routine updates the clock generator to generate the correct frequency
  and polarity for the SPI clock.

  @param[in] This           Pointer to an EFI_SPI_HC_PROTOCOL structure.
  @param[in] SpiPeripheral  Pointer to a EFI_SPI_PERIPHERAL data structure from
                            which the routine can access the ClockParameter,
                            ClockPhase and ClockPolarity fields. The routine
                            also has access to the names for the SPI bus and
                            chip which can be used during debugging.
  @param[in] ClockHz        Pointer to the requested clock frequency. The SPI
                            host controller will choose a supported clock
                            frequency which is less then or equal to this
                            value. Specify zero to turn the clock generator
                            off. The actual clock frequency supported by the
                            SPI host controller will be returned.

  @retval EFI_UNSUPPORTED

**/
EFI_STATUS
EFIAPI
PlatformSpiHcClock (
  IN CONST EFI_SPI_HC_PROTOCOL  *This,
  IN CONST EFI_SPI_PERIPHERAL   *SpiPeripheral,
  IN UINT32                     *ClockHz
  )
{
  return EFI_UNSUPPORTED;
}

/**
  This function is the platform specific SPI transaction function
  Perform the SPI transaction on the SPI peripheral using the SPI host
  controller.

  This routine is called at TPL_NOTIFY.
  This routine synchronously returns EFI_SUCCESS indicating that the
  asynchronous SPI transaction was started. The routine then waits for
  completion of the SPI transaction prior to returning the final transaction
  status.

  @param[in] This            Pointer to an EFI_SPI_HC_PROTOCOL structure.
  @param[in] BusTransaction  Pointer to a EFI_SPI_BUS_ TRANSACTION containing
                             the description of the SPI transaction to perform.

  @retval EFI_UNSUPPORTED

**/
EFI_STATUS
EFIAPI
PlatformSpiHcTransaction (
  IN CONST EFI_SPI_HC_PROTOCOL  *This,
  IN EFI_SPI_BUS_TRANSACTION    *BusTransaction
  )
{
  return EFI_UNSUPPORTED;
}
