/** @file

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>
#include <Omap3530/Omap3530.h>

#include <Library/DebugLib.h>
#include <Library/IoLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/SmbusHc.h>

#define MAX_RETRY  1000

//
// Internal Functions
//
STATIC
EFI_STATUS
WaitForBusBusy (
  VOID
  )
{
  UINTN Retry = 0;

  while (++Retry < MAX_RETRY && (MmioRead16(I2C_STAT) & BB) == 0x1);

  if (Retry == MAX_RETRY) {
    return EFI_TIMEOUT;
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
PollForStatus(
  UINT16 StatusBit
  )
{
  UINTN Retry = 0;

  while(Retry < MAX_RETRY) {
    if (MmioRead16(I2C_STAT) & StatusBit) {
      //Clear particular status bit from Status register.
      MmioOr16(I2C_STAT, StatusBit);
      break;
    }
    Retry++;
  }

  if (Retry == MAX_RETRY) {
    return EFI_TIMEOUT;
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
ConfigureI2c (
  VOID
  )
{
  //Program prescaler to obtain 12-MHz clock
  MmioWrite16(I2C_PSC, 0x0000);

  //Program SCLL and SCLH
  //NOTE: Following values are the register dump after U-Boot code executed.
  //We need to figure out how its calculated based on the I2C functional clock and I2C_PSC.
  MmioWrite16(I2C_SCLL, 0x0035);
  MmioWrite16(I2C_SCLH, 0x0035);

  //Take the I2C controller out of reset.
  MmioOr16(I2C_CON, I2C_EN);

  //Initialize the I2C controller.

  //Set I2C controller in Master mode.
  MmioOr16(I2C_CON, MST);

  //Enable interrupts for receive/transmit mode.
  MmioOr16(I2C_IE, (XRDY_IE | RRDY_IE | ARDY_IE | NACK_IE));

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
I2CReadOneByte (
  UINT8 *Data
  )
{
  EFI_STATUS Status;

  //I2C bus status checking
  Status = WaitForBusBusy();
  if (EFI_ERROR(Status)) {
    return Status;
  }

  //Poll till Receive ready bit is set.
  Status = PollForStatus(RRDY);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  *Data = MmioRead8(I2C_DATA);

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
I2CWriteOneByte (
  UINT8 Data
  )
{
  EFI_STATUS Status;

  //I2C bus status checking
  Status = WaitForBusBusy();
  if (EFI_ERROR(Status)) {
    return Status;
  }

  //Data transfer
  //Poll till Transmit ready bit is set
  Status = PollForStatus(XRDY);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  MmioWrite8(I2C_DATA, Data);

  //Wait and check if the NACK is not set.
  gBS->Stall(1000);
  if (MmioRead16(I2C_STAT) & NACK) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
SmbusBlockRead (
  OUT UINT8       *Buffer,
  IN  UINTN       Length
  )
{
  UINTN      Index = 0;
  EFI_STATUS Status = EFI_SUCCESS;

  //Transfer configuration for receiving data.
  MmioWrite16(I2C_CNT, Length);
  //Need stop bit before sending data.
  MmioWrite16(I2C_CON, (I2C_EN | MST | STP | STT));

  while (Index < Length) {
    //Read a byte
    Status = I2CReadOneByte(&Buffer[Index++]);
    if (EFI_ERROR(Status)) {
      return Status;
    }
  }

  //Transfer completion
  Status = PollForStatus(ARDY);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  return Status;
}

STATIC
EFI_STATUS
SmbusBlockWrite (
  IN UINT8       *Buffer,
  IN UINTN       Length
  )
{
  UINTN      Index = 0;
  EFI_STATUS Status = EFI_SUCCESS;

  //Transfer configuration for transmitting data
  MmioWrite16(I2C_CNT, Length);
  MmioWrite16(I2C_CON, (I2C_EN | TRX | MST | STT | STP));

  while (Index < Length) {
    //Send a byte
    Status = I2CWriteOneByte(Buffer[Index++]);
    if (EFI_ERROR(Status)) {
      return Status;
    }
  }

  //Transfer completion
  Status = PollForStatus(ARDY);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  return Status;
}

//
// Public Functions.
//
EFI_STATUS
EFIAPI
SmbusExecute (
  IN CONST EFI_SMBUS_HC_PROTOCOL    *This,
  IN CONST EFI_SMBUS_DEVICE_ADDRESS SlaveAddress,
  IN CONST EFI_SMBUS_DEVICE_COMMAND Command,
  IN CONST EFI_SMBUS_OPERATION      Operation,
  IN CONST BOOLEAN                  PecCheck,
  IN OUT   UINTN                    *Length,
  IN OUT   VOID                     *Buffer
  )
{
  UINT8      *ByteBuffer  = Buffer;
  EFI_STATUS Status       = EFI_SUCCESS;
  UINT8      SlaveAddr    = (UINT8)(SlaveAddress.SmbusDeviceAddress);

  if (PecCheck) {
    return EFI_UNSUPPORTED;
  }

  if ((Operation != EfiSmbusWriteBlock) && (Operation != EfiSmbusReadBlock)) {
    return EFI_UNSUPPORTED;
  }

  //Set the Slave address.
  MmioWrite16(I2C_SA, SlaveAddr);

  if (Operation == EfiSmbusReadBlock) {
    Status = SmbusBlockRead(ByteBuffer, *Length);
  } else if (Operation == EfiSmbusWriteBlock) {
    Status = SmbusBlockWrite(ByteBuffer, *Length);
  }

  return Status;
}

EFI_STATUS
EFIAPI
SmbusArpDevice (
  IN CONST EFI_SMBUS_HC_PROTOCOL    *This,
  IN       BOOLEAN                  ArpAll,
  IN       EFI_SMBUS_UDID           *SmbusUdid OPTIONAL,
  IN OUT   EFI_SMBUS_DEVICE_ADDRESS *SlaveAddress OPTIONAL
  )
{
  return EFI_UNSUPPORTED;
}


EFI_STATUS
EFIAPI
SmbusGetArpMap (
  IN CONST EFI_SMBUS_HC_PROTOCOL    *This,
  IN OUT   UINTN                    *Length,
  IN OUT   EFI_SMBUS_DEVICE_MAP     **SmbusDeviceMap
  )
{
  return EFI_UNSUPPORTED;
}


EFI_STATUS
EFIAPI
SmbusNotify (
  IN CONST  EFI_SMBUS_HC_PROTOCOL     *This,
  IN CONST  EFI_SMBUS_DEVICE_ADDRESS  SlaveAddress,
  IN CONST  UINTN                     Data,
  IN CONST  EFI_SMBUS_NOTIFY_FUNCTION NotifyFunction
  )
{
  return EFI_UNSUPPORTED;
}

EFI_SMBUS_HC_PROTOCOL SmbusProtocol =
{
  SmbusExecute,
  SmbusArpDevice,
  SmbusGetArpMap,
  SmbusNotify
};

EFI_STATUS
InitializeSmbus (
    IN EFI_HANDLE       ImageHandle,
    IN EFI_SYSTEM_TABLE *SystemTable
    )
{
  EFI_HANDLE      Handle = NULL;
  EFI_STATUS      Status;

  //Configure I2C controller.
  Status = ConfigureI2c();
  if (EFI_ERROR(Status)) {
    DEBUG ((EFI_D_ERROR, "InitializeI2c fails.\n"));
    return Status;
  }

  // Install the SMBUS interface
  Status = gBS->InstallMultipleProtocolInterfaces(&Handle, &gEfiSmbusHcProtocolGuid, &SmbusProtocol, NULL);
  ASSERT_EFI_ERROR(Status);

  return Status;
}

