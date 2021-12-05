/** @file
  Platform Hook Library instance for UART device.

  Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <PiDxe.h>
#include <UniversalPayload/SerialPortInfo.h>
#include <Library/PlatformHookLib.h>
#include <Library/PcdLib.h>
#include <Library/HobLib.h>

/** Library Constructor

  @retval RETURN_SUCCESS  Success.
**/
EFI_STATUS
EFIAPI
PlatformHookSerialPortConstructor (
  VOID
  )
{
  // Nothing to do here. This constructor is added to
  // enable the chain of constructor invocation for
  // dependent libraries.
  return RETURN_SUCCESS;
}

/**
  Performs platform specific initialization required for the CPU to access
  the hardware associated with a SerialPortLib instance.  This function does
  not initialize the serial port hardware itself.  Instead, it initializes
  hardware devices that are required for the CPU to access the serial port
  hardware.  This function may be called more than once.

  @retval RETURN_SUCCESS       The platform specific initialization succeeded.
  @retval RETURN_DEVICE_ERROR  The platform specific initialization could not be completed.

**/
RETURN_STATUS
EFIAPI
PlatformHookSerialPortInitialize (
  VOID
  )
{
  RETURN_STATUS                       Status;
  UNIVERSAL_PAYLOAD_SERIAL_PORT_INFO  *SerialPortInfo;
  UINT8                               *GuidHob;
  UNIVERSAL_PAYLOAD_GENERIC_HEADER    *GenericHeader;

  GuidHob = GetFirstGuidHob (&gUniversalPayloadSerialPortInfoGuid);
  if (GuidHob == NULL) {
    return EFI_NOT_FOUND;
  }

  GenericHeader = (UNIVERSAL_PAYLOAD_GENERIC_HEADER *)GET_GUID_HOB_DATA (GuidHob);
  if ((sizeof (UNIVERSAL_PAYLOAD_GENERIC_HEADER) > GET_GUID_HOB_DATA_SIZE (GuidHob)) || (GenericHeader->Length > GET_GUID_HOB_DATA_SIZE (GuidHob))) {
    return EFI_NOT_FOUND;
  }

  if (GenericHeader->Revision == UNIVERSAL_PAYLOAD_SERIAL_PORT_INFO_REVISION) {
    SerialPortInfo = (UNIVERSAL_PAYLOAD_SERIAL_PORT_INFO *)GET_GUID_HOB_DATA (GuidHob);
    if (GenericHeader->Length < UNIVERSAL_PAYLOAD_SIZEOF_THROUGH_FIELD (UNIVERSAL_PAYLOAD_SERIAL_PORT_INFO, RegisterBase)) {
      //
      // Return if can't find the Serial Port Info Hob with enough length
      //
      return EFI_NOT_FOUND;
    }

    Status = PcdSetBoolS (PcdSerialUseMmio, SerialPortInfo->UseMmio);
    if (RETURN_ERROR (Status)) {
      return Status;
    }

    Status = PcdSet64S (PcdSerialRegisterBase, SerialPortInfo->RegisterBase);
    if (RETURN_ERROR (Status)) {
      return Status;
    }

    Status = PcdSet32S (PcdSerialRegisterStride, SerialPortInfo->RegisterStride);
    if (RETURN_ERROR (Status)) {
      return Status;
    }

    Status = PcdSet32S (PcdSerialBaudRate, SerialPortInfo->BaudRate);
    if (RETURN_ERROR (Status)) {
      return Status;
    }

    return RETURN_SUCCESS;
  }

  return EFI_NOT_FOUND;
}
