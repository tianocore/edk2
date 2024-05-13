/** @file

  SpiBus driver

  Copyright (C) 2024 Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Protocol/SpiConfiguration.h>
#include <Protocol/SpiHc.h>
#include <Protocol/SpiIo.h>
#include "SpiBus.h"

/**
  Checks if two device paths are the same.

  @param[in] DevicePath1        First device path to compare
  @param[in] DevicePath2        Second device path to compare

  @retval TRUE              The device paths share the same nodes and values
  @retval FALSE             The device paths differ
**/
BOOLEAN
EFIAPI
DevicePathsAreEqual (
  IN CONST EFI_DEVICE_PATH_PROTOCOL  *DevicePath1,
  IN CONST EFI_DEVICE_PATH_PROTOCOL  *DevicePath2
  )
{
  UINTN  Size1;
  UINTN  Size2;

  Size1 = GetDevicePathSize (DevicePath1);
  Size2 = GetDevicePathSize (DevicePath2);

  if (Size1 != Size2) {
    return FALSE;
  }

  if (CompareMem (DevicePath1, DevicePath2, Size1) != 0) {
    return FALSE;
  }

  return TRUE;
}

/**
  Calls the SpiPeripherals ChipSelect if it is not null, otherwise
  calls the Host Controllers ChipSelect function.

  @param[in] SpiChip        The SpiChip to place on the bus via asserting its chip select
  @param[in] PinValue       Value to place on the chip select pin

  @retval EFI_SUCCESS                 Chip select pin was placed at requested level
  @retval EFI_INVALID_PARAMETER       Invalid parameters passed into ChipSelect function
**/
EFI_STATUS
EFIAPI
SpiChipSelect (
  IN CONST SPI_IO_CHIP  *SpiChip,
  IN BOOLEAN            PinValue
  )
{
  EFI_STATUS  Status;

  // Check which chip select function to use
  if (SpiChip->Protocol.SpiPeripheral->ChipSelect != NULL) {
    Status = SpiChip->Protocol.SpiPeripheral->ChipSelect (
                                                SpiChip->BusTransaction.SpiPeripheral,
                                                PinValue
                                                );
  } else {
    Status = SpiChip->SpiHc->ChipSelect (
                               SpiChip->SpiHc,
                               SpiChip->BusTransaction.SpiPeripheral,
                               PinValue
                               );
  }

  return Status;
}

/**
  Checks the SpiChip's BusTransaction attributes to ensure its a valid SPI transaction.

  @param[in] SpiChip        The SpiChip where a bus transaction is requested

  @retval EFI_SUCCESS            This is a valid SPI bus transaction
  @retval EFI_BAD_BUFFER_SIZE    The WriteBytes value was invalid
  @retval EFI_BAD_BUFFER_SIZE    The ReadBytes value was invalid
  @retval EFI_INVALID_PARAMETER  TransactionType is not valid,
                                 or BusWidth not supported by SPI peripheral or
                                 SPI host controller,
                                 or WriteBytes non-zero and WriteBuffer is
                                 NULL,
                                 or ReadBytes non-zero and ReadBuffer is NULL,
                                 or ReadBuffer != WriteBuffer for full-duplex
                                 type,
                                 or WriteBuffer was NULL,
                                 or TPL is too high
  @retval EFI_OUT_OF_RESOURCES   Insufficient memory for SPI transaction
  @retval EFI_UNSUPPORTED        The FrameSize is not supported by the SPI bus
                                 layer or the SPI host controller
  @retval EFI_UNSUPPORTED        The SPI controller was not able to support
**/
EFI_STATUS
EFIAPI
IsValidSpiTransaction (
  IN SPI_IO_CHIP  *SpiChip
  )
{
  // Error checking
  if (SpiChip->BusTransaction.TransactionType > SPI_TRANSACTION_WRITE_THEN_READ) {
    return EFI_INVALID_PARAMETER;
  }

  if (((SpiChip->BusTransaction.BusWidth != 1) && (SpiChip->BusTransaction.BusWidth != 2) && (SpiChip->BusTransaction.BusWidth != 4) &&
       (SpiChip->BusTransaction.BusWidth != 8)) || (SpiChip->BusTransaction.FrameSize == 0))
  {
    return EFI_INVALID_PARAMETER;
  }

  if ((SpiChip->BusTransaction.BusWidth == 8) && (((SpiChip->Protocol.Attributes & SPI_IO_SUPPORTS_8_BIT_DATA_BUS_WIDTH) != SPI_IO_SUPPORTS_8_BIT_DATA_BUS_WIDTH) ||
                                                  ((SpiChip->BusTransaction.SpiPeripheral->Attributes & SPI_PART_SUPPORTS_8_BIT_DATA_BUS_WIDTH) != SPI_PART_SUPPORTS_8_BIT_DATA_BUS_WIDTH)))
  {
    return EFI_INVALID_PARAMETER;
  } else if ((SpiChip->BusTransaction.BusWidth == 4) && (((SpiChip->Protocol.Attributes & SPI_IO_SUPPORTS_4_BIT_DATA_BUS_WIDTH) != SPI_IO_SUPPORTS_4_BIT_DATA_BUS_WIDTH) ||
                                                         ((SpiChip->BusTransaction.SpiPeripheral->Attributes & SPI_PART_SUPPORTS_4_BIT_DATA_BUS_WIDTH) != SPI_PART_SUPPORTS_4_BIT_DATA_BUS_WIDTH)))
  {
    return EFI_INVALID_PARAMETER;
  } else if ((SpiChip->BusTransaction.BusWidth == 2) && (((SpiChip->Protocol.Attributes & SPI_IO_SUPPORTS_4_BIT_DATA_BUS_WIDTH) != SPI_IO_SUPPORTS_4_BIT_DATA_BUS_WIDTH) ||
                                                         ((SpiChip->BusTransaction.SpiPeripheral->Attributes & SPI_PART_SUPPORTS_2_BIT_DATA_BUS_WIDTH) != SPI_PART_SUPPORTS_2_BIT_DATA_BUS_WIDTH)))
  {
    return EFI_INVALID_PARAMETER;
  }

  if (((SpiChip->BusTransaction.WriteBytes > 0) && (SpiChip->BusTransaction.WriteBuffer == NULL)) || ((SpiChip->BusTransaction.ReadBytes > 0) && (SpiChip->BusTransaction.ReadBuffer == NULL))) {
    return EFI_INVALID_PARAMETER;
  }

  if ((SpiChip->BusTransaction.TransactionType == SPI_TRANSACTION_FULL_DUPLEX) &&  (SpiChip->BusTransaction.ReadBytes != SpiChip->BusTransaction.WriteBytes)) {
    return EFI_INVALID_PARAMETER;
  }

  // Check frame size, passed parameter is in bits
  if ((SpiChip->Protocol.FrameSizeSupportMask & (1<<(SpiChip->BusTransaction.FrameSize-1))) == 0) {
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

/**
  Initiate a SPI transaction between the host and a SPI peripheral.

  This routine must be called at or below TPL_NOTIFY.
  This routine works with the SPI bus layer to pass the SPI transaction to the
  SPI controller for execution on the SPI bus. There are four types of
  supported transactions supported by this routine:
  * Full Duplex: WriteBuffer and ReadBuffer are the same size.
  * Write Only: WriteBuffer contains data for SPI peripheral, ReadBytes = 0
  * Read Only: ReadBuffer to receive data from SPI peripheral, WriteBytes = 0
  * Write Then Read: WriteBuffer contains control data to write to SPI
                     peripheral before data is placed into the ReadBuffer.
                     Both WriteBytes and ReadBytes must be non-zero.

  @param[in]  This              Pointer to an EFI_SPI_IO_PROTOCOL structure.
  @param[in]  TransactionType   Type of SPI transaction.
  @param[in]  DebugTransaction  Set TRUE only when debugging is desired.
                                Debugging may be turned on for a single SPI
                                transaction. Only this transaction will display
                                debugging messages. All other transactions with
                                this value set to FALSE will not display any
                                debugging messages.
  @param[in]  ClockHz           Specify the ClockHz value as zero (0) to use
                                the maximum clock frequency supported by the
                                SPI controller and part. Specify a non-zero
                                value only when a specific SPI transaction
                                requires a reduced clock rate.
  @param[in]  BusWidth          Width of the SPI bus in bits: 1, 2, 4
  @param[in]  FrameSize         Frame size in bits, range: 1 - 32
  @param[in]  WriteBytes        The length of the WriteBuffer in bytes.
                                Specify zero for read-only operations.
  @param[in]  WriteBuffer       The buffer containing data to be sent from the
                                host to the SPI chip. Specify NULL for read
                                only operations.
                                * Frame sizes 1-8 bits: UINT8 (one byte) per
                                  frame
                                * Frame sizes 7-16 bits: UINT16 (two bytes) per
                                  frame
                                * Frame sizes 17-32 bits: UINT32 (four bytes)
                                  per frame The transmit frame is in the least
                                  significant N bits.
  @param[in]  ReadBytes         The length of the ReadBuffer in bytes.
                                Specify zero for write-only operations.
  @param[out] ReadBuffer        The buffer to receeive data from the SPI chip
                                during the transaction. Specify NULL for write
                                only operations.
                                * Frame sizes 1-8 bits: UINT8 (one byte) per
                                  frame
                                * Frame sizes 7-16 bits: UINT16 (two bytes) per
                                  frame
                                * Frame sizes 17-32 bits: UINT32 (four bytes)
                                  per frame The received frame is in the least
                                  significant N bits.

  @retval EFI_SUCCESS            The SPI transaction completed successfully
  @retval EFI_BAD_BUFFER_SIZE    The WriteBytes value was invalid
  @retval EFI_BAD_BUFFER_SIZE    The ReadBytes value was invalid
  @retval EFI_INVALID_PARAMETER  TransactionType is not valid,
                                 or BusWidth not supported by SPI peripheral or
                                 SPI host controller,
                                 or WriteBytes non-zero and WriteBuffer is
                                 NULL,
                                 or ReadBytes non-zero and ReadBuffer is NULL,
                                 or ReadBuffer != WriteBuffer for full-duplex
                                 type,
                                 or WriteBuffer was NULL,
                                 or TPL is too high
  @retval EFI_OUT_OF_RESOURCES   Insufficient memory for SPI transaction
  @retval EFI_UNSUPPORTED        The FrameSize is not supported by the SPI bus
                                 layer or the SPI host controller
  @retval EFI_UNSUPPORTED        The SPI controller was not able to support

**/
EFI_STATUS
EFIAPI
Transaction (
  IN  CONST EFI_SPI_IO_PROTOCOL  *This,
  IN  EFI_SPI_TRANSACTION_TYPE   TransactionType,
  IN  BOOLEAN                    DebugTransaction,
  IN  UINT32                     ClockHz OPTIONAL,
  IN  UINT32                     BusWidth,
  IN  UINT32                     FrameSize,
  IN  UINT32                     WriteBytes,
  IN  UINT8                      *WriteBuffer,
  IN  UINT32                     ReadBytes,
  OUT UINT8                      *ReadBuffer
  )
{
  EFI_STATUS   Status;
  SPI_IO_CHIP  *SpiChip;
  UINT32       MaxClockHz;
  UINT8        *DummyReadBuffer;
  UINT8        *DummyWriteBuffer;

  SpiChip                               = SPI_IO_CHIP_FROM_THIS (This);
  SpiChip->BusTransaction.SpiPeripheral =
    (EFI_SPI_PERIPHERAL *)SpiChip->Protocol.SpiPeripheral;
  SpiChip->BusTransaction.TransactionType  = TransactionType;
  SpiChip->BusTransaction.DebugTransaction = DebugTransaction;
  SpiChip->BusTransaction.BusWidth         = BusWidth;
  SpiChip->BusTransaction.FrameSize        = FrameSize;
  SpiChip->BusTransaction.WriteBytes       = WriteBytes;
  SpiChip->BusTransaction.WriteBuffer      = WriteBuffer;
  SpiChip->BusTransaction.ReadBytes        = ReadBytes;
  SpiChip->BusTransaction.ReadBuffer       = ReadBuffer;

  // Ensure valid spi transaction parameters
  Status = IsValidSpiTransaction (SpiChip);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Setup the proper clock frequency
  if (SpiChip->BusTransaction.SpiPeripheral->MaxClockHz != 0) {
    MaxClockHz = SpiChip->BusTransaction.SpiPeripheral->MaxClockHz;
  } else {
    MaxClockHz = SpiChip->BusTransaction.SpiPeripheral->SpiPart->MaxClockHz;
  }

  // Call proper clock function
  if (SpiChip->Protocol.SpiPeripheral->SpiBus->Clock != NULL) {
    Status = SpiChip->Protocol.SpiPeripheral->SpiBus->Clock (
                                                        SpiChip->BusTransaction.SpiPeripheral,
                                                        &MaxClockHz
                                                        );
  } else {
    Status = SpiChip->SpiHc->Clock (
                               SpiChip->SpiHc,
                               SpiChip->BusTransaction.SpiPeripheral,
                               &MaxClockHz
                               );
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = SpiChipSelect (SpiChip, SpiChip->BusTransaction.SpiPeripheral->SpiPart->ChipSelectPolarity);

  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Check transaction types and match to HC capabilities
  if ((TransactionType == SPI_TRANSACTION_WRITE_ONLY) &&
      ((SpiChip->SpiHc->Attributes & HC_SUPPORTS_WRITE_ONLY_OPERATIONS) != HC_SUPPORTS_WRITE_ONLY_OPERATIONS))
  {
    // Convert to full duplex transaction
    SpiChip->BusTransaction.ReadBytes  = SpiChip->BusTransaction.WriteBytes;
    SpiChip->BusTransaction.ReadBuffer = AllocateZeroPool (SpiChip->BusTransaction.ReadBytes);

    Status = SpiChip->SpiHc->Transaction (
                               SpiChip->SpiHc,
                               &SpiChip->BusTransaction
                               );

    SpiChip->BusTransaction.ReadBytes = ReadBytes; // assign to passed parameter
    FreePool (SpiChip->BusTransaction.ReadBuffer); // Free temporary buffer
  } else if ((TransactionType == SPI_TRANSACTION_READ_ONLY) &&
             ((SpiChip->SpiHc->Attributes & HC_SUPPORTS_READ_ONLY_OPERATIONS) != HC_SUPPORTS_READ_ONLY_OPERATIONS))
  {
    // Convert to full duplex transaction
    SpiChip->BusTransaction.WriteBytes  = SpiChip->BusTransaction.WriteBytes;
    SpiChip->BusTransaction.WriteBuffer = AllocateZeroPool (SpiChip->BusTransaction.WriteBytes);

    Status = SpiChip->SpiHc->Transaction (
                               SpiChip->SpiHc,
                               &SpiChip->BusTransaction
                               );

    SpiChip->BusTransaction.WriteBytes = WriteBytes;
    FreePool (SpiChip->BusTransaction.WriteBuffer);
  } else if ((TransactionType == SPI_TRANSACTION_WRITE_THEN_READ) &&
             ((SpiChip->SpiHc->Attributes & HC_SUPPORTS_WRITE_THEN_READ_OPERATIONS) != HC_SUPPORTS_WRITE_THEN_READ_OPERATIONS))
  {
    // Convert to full duplex transaction
    DummyReadBuffer                    = AllocateZeroPool (WriteBytes);
    DummyWriteBuffer                   = AllocateZeroPool (ReadBytes);
    SpiChip->BusTransaction.ReadBuffer = DummyReadBuffer;
    SpiChip->BusTransaction.ReadBytes  = WriteBytes;

    Status = SpiChip->SpiHc->Transaction (
                               SpiChip->SpiHc,
                               &SpiChip->BusTransaction
                               );

    if (EFI_ERROR (Status)) {
      return Status;
    }

    // Write is done, now need to read, restore passed in read buffer info
    SpiChip->BusTransaction.ReadBuffer = ReadBuffer;
    SpiChip->BusTransaction.ReadBytes  = ReadBytes;

    SpiChip->BusTransaction.WriteBuffer = DummyWriteBuffer;
    SpiChip->BusTransaction.WriteBytes  = ReadBytes;

    Status = SpiChip->SpiHc->Transaction (
                               SpiChip->SpiHc,
                               &SpiChip->BusTransaction
                               );
    // Restore write data
    SpiChip->BusTransaction.WriteBuffer = WriteBuffer;
    SpiChip->BusTransaction.WriteBytes  = WriteBytes;

    FreePool (DummyReadBuffer);
    FreePool (DummyWriteBuffer);
  } else {
    // Supported transaction type, just pass info the SPI HC Protocol Transaction
    Status = SpiChip->SpiHc->Transaction (
                               SpiChip->SpiHc,
                               &SpiChip->BusTransaction
                               );
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = SpiChipSelect (SpiChip, !SpiChip->BusTransaction.SpiPeripheral->SpiPart->ChipSelectPolarity);

  return Status;
}

/**
  Update the SPI peripheral associated with this SPI 10 SpiChip.

  Support socketed SPI parts by allowing the SPI peripheral driver to replace
  the SPI peripheral after the connection is made. An example use is socketed
  SPI NOR flash parts, where the size and parameters change depending upon
  device is in the socket.

  @param[in] This           Pointer to an EFI_SPI_IO_PROTOCOL structure.
  @param[in] SpiPeripheral  Pointer to an EFI_SPI_PERIPHERAL structure.

  @retval EFI_SUCCESS            The SPI peripheral was updated successfully
  @retval EFI_INVALID_PARAMETER  The SpiPeripheral value is NULL,
                                 or the SpiPeripheral->SpiBus is NULL,
                                 or the SpiPeripheral->SpiBus pointing at
                                 wrong bus, or the SpiPeripheral->SpiPart is NULL
**/
EFI_STATUS
EFIAPI
UpdateSpiPeripheral (
  IN CONST EFI_SPI_IO_PROTOCOL  *This,
  IN CONST EFI_SPI_PERIPHERAL   *SpiPeripheral
  )
{
  EFI_STATUS   Status;
  SPI_IO_CHIP  *SpiChip;

  DEBUG ((DEBUG_VERBOSE, "%a: SPI Bus - Entry\n", __func__));

  SpiChip = SPI_IO_CHIP_FROM_THIS (This);

  if ((SpiPeripheral == NULL) || (SpiPeripheral->SpiBus == NULL) ||
      (SpiPeripheral->SpiPart == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  // EFI_INVALID_PARAMETER if SpiPeripheral->SpiBus is pointing at wrong bus
  if (!DevicePathsAreEqual (SpiPeripheral->SpiBus->ControllerPath, SpiChip->SpiBus->ControllerPath)) {
    return EFI_INVALID_PARAMETER;
  }

  SpiChip->Protocol.OriginalSpiPeripheral = SpiChip->Protocol.SpiPeripheral;
  SpiChip->Protocol.SpiPeripheral         = SpiPeripheral;

  Status = EFI_SUCCESS;
  DEBUG ((
    DEBUG_VERBOSE,
    "%a: SPI Bus - Exit Status=%r\n",
    __func__,
    Status
    ));
  return Status;
}
