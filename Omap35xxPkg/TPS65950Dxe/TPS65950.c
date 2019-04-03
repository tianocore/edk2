/** @file

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>

#include <TPS65950.h>

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/EmbeddedExternalDevice.h>
#include <Protocol/SmbusHc.h>

EFI_SMBUS_HC_PROTOCOL *Smbus;

EFI_STATUS
Read (
  IN  EMBEDDED_EXTERNAL_DEVICE    *This,
  IN  UINTN                       Register,
  IN  UINTN                       Length,
  OUT VOID                        *Buffer
  )
{
  EFI_STATUS               Status;
  EFI_SMBUS_DEVICE_ADDRESS SlaveAddress;
  UINT8                    DeviceRegister;
  UINTN                    DeviceRegisterLength = 1;

  SlaveAddress.SmbusDeviceAddress = EXTERNAL_DEVICE_REGISTER_TO_SLAVE_ADDRESS(Register);
  DeviceRegister = (UINT8)EXTERNAL_DEVICE_REGISTER_TO_REGISTER(Register);

  //Write DeviceRegister.
  Status = Smbus->Execute(Smbus, SlaveAddress, 0, EfiSmbusWriteBlock, FALSE, &DeviceRegisterLength, &DeviceRegister);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  //Read Data
  Status = Smbus->Execute(Smbus, SlaveAddress, 0, EfiSmbusReadBlock, FALSE, &Length, Buffer);
  return Status;
}

EFI_STATUS
Write (
  IN EMBEDDED_EXTERNAL_DEVICE   *This,
  IN UINTN                      Register,
  IN UINTN                      Length,
  IN VOID                       *Buffer
  )
{
  EFI_STATUS               Status;
  EFI_SMBUS_DEVICE_ADDRESS SlaveAddress;
  UINT8                    DeviceRegister;
  UINTN                    DeviceBufferLength = Length + 1;
  UINT8                    *DeviceBuffer;

  SlaveAddress.SmbusDeviceAddress = EXTERNAL_DEVICE_REGISTER_TO_SLAVE_ADDRESS(Register);
  DeviceRegister = (UINT8)EXTERNAL_DEVICE_REGISTER_TO_REGISTER(Register);

  //Prepare buffer for writing
  DeviceBuffer = (UINT8 *)AllocatePool(DeviceBufferLength);
  if (DeviceBuffer == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto exit;
  }

  //Set Device register followed by data to write.
  DeviceBuffer[0] = DeviceRegister;
  CopyMem(&DeviceBuffer[1], Buffer, Length);

  //Write Data
  Status = Smbus->Execute(Smbus, SlaveAddress, 0, EfiSmbusWriteBlock, FALSE, &DeviceBufferLength, DeviceBuffer);
  if (EFI_ERROR(Status)) {
    goto exit;
  }

exit:
  if (DeviceBuffer) {
    FreePool(DeviceBuffer);
  }

  return Status;
}

EMBEDDED_EXTERNAL_DEVICE ExternalDevice = {
  Read,
  Write
};

EFI_STATUS
TPS65950Initialize (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = gBS->LocateProtocol(&gEfiSmbusHcProtocolGuid, NULL, (VOID **)&Smbus);
  ASSERT_EFI_ERROR(Status);

  Status = gBS->InstallMultipleProtocolInterfaces(&ImageHandle, &gEmbeddedExternalDeviceProtocolGuid, &ExternalDevice, NULL);
  return Status;
}
