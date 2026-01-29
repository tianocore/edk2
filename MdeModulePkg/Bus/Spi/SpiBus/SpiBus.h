/** @file

  SPI bus driver

  Copyright (C) 2024 Advanced Micro Devices, Inc. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef SPI_BUS_H_
#define SPI_BUS_H_

#include <PiDxe.h>
#include <Library/BaseLib.h>
#include <Protocol/DevicePath.h>
#include <Library/DevicePathLib.h>
#include <Library/BaseMemoryLib.h>
#include <Protocol/SpiIo.h>
#include <Protocol/SpiHc.h>
#include <Protocol/SpiConfiguration.h>

#define SPI_IO_SIGNATURE  SIGNATURE_32 ('s', 'i', 'o', 'c')

typedef struct {
  UINTN                             Signature;
  EFI_HANDLE                        Handle;
  EFI_SPI_IO_PROTOCOL               Protocol;
  EFI_SPI_BUS_TRANSACTION           BusTransaction;
  EFI_SPI_CONFIGURATION_PROTOCOL    *SpiConfig;
  EFI_SPI_HC_PROTOCOL               *SpiHc;
  EFI_SPI_BUS                       *SpiBus;
} SPI_IO_CHIP;

#define SPI_IO_CHIP_FROM_THIS(a) \
  CR (a, SPI_IO_CHIP, Protocol, \
      SPI_IO_SIGNATURE)

/**
  Checks if two device paths are the same

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
  );

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
  @retval EFI_BAD_BUFFER_SIZE    The writeBytes value was invalid
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
  );

/**
  Update the SPI peripheral associated with this SPI 10 instance.

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
  );

#endif //SPI_BUS_H_
