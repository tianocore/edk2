/** @file
  NULL instace of RedfishPlatformHostInterfaceLib

  (C) Copyright 2020 Hewlett Packard Enterprise Development LP<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/RedfishHostInterfaceLib.h>
#include <Library/UefiLib.h>

/**
  Get platform Redfish host interface device descriptor.

  @param[in] DeviceType         Pointer to retrieve device type.
  @param[out] DeviceDescriptor  Pointer to retrieve REDFISH_INTERFACE_DATA, caller has to free
                                this memory using FreePool().

  @retval EFI_NOT_FOUND   No Redfish host interface descriptor provided on this platform.

**/
EFI_STATUS
RedfishPlatformHostInterfaceDeviceDescriptor (
  IN UINT8                    *DeviceType,
  OUT REDFISH_INTERFACE_DATA  **DeviceDescriptor
  )
{
  return EFI_NOT_FOUND;
}

/**
  Get platform Redfish host interface protocol data.
  Caller should pass NULL in ProtocolRecord to retrive the first protocol record.
  Then continuously pass previous ProtocolRecord for retrieving the next ProtocolRecord.

  @param[in, out] ProtocolRecord  Pointer to retrieve the first or the next protocol record.
                                  caller has to free the new protocol record returned from
                                  this function using FreePool().
  @param[in] IndexOfProtocolData  The index of protocol data.

  @retval EFI_NOT_FOUND   No more protocol records.

**/
EFI_STATUS
RedfishPlatformHostInterfaceProtocolData (
  IN OUT MC_HOST_INTERFACE_PROTOCOL_RECORD  **ProtocolRecord,
  IN UINT8                                  IndexOfProtocolData
  )
{
  return EFI_NOT_FOUND;
}
