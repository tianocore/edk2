/** @file
  Definitinos of RedfishHostInterfaceDxe driver.

  (C) Copyright 2020 Hewlett Packard Enterprise Development LP<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#ifndef REDFISH_HOST_INTERFACE_LIB_H_
#define REDFISH_HOST_INTERFACE_LIB_H_

#include <Uefi.h>
#include <IndustryStandard/RedfishHostInterface.h>
#include <IndustryStandard/SmBios.h>

#include <Protocol/Smbios.h>

/**
  Get platform Redfish host interface device descriptor.

  @param[in] DeviceType         Pointer to retrieve device type.
  @param[out] DeviceDescriptor  Pointer to retrieve REDFISH_INTERFACE_DATA, caller has to free
                                this memory using FreePool().
  @retval EFI_SUCCESS     Device descriptor is returned successfully in DeviceDescriptor.
  @retval EFI_NOT_FOUND   No Redfish host interface descriptor provided on this platform.
  @retval Others          Fail to get device descriptor.
**/
EFI_STATUS
RedfishPlatformHostInterfaceDeviceDescriptor (
  IN UINT8 *DeviceType,
  OUT REDFISH_INTERFACE_DATA  **DeviceDescriptor
);
/**
  Get platform Redfish host interface protocol data.
  Caller should pass NULL in ProtocolRecord to retrive the first protocol record.
  Then continuously pass previous ProtocolRecord for retrieving the next ProtocolRecord.

  @param[in, out] ProtocolRecord  Pointer to retrieve the first or the next protocol record.
                                  caller has to free the new protocol record returned from
                                  this function using FreePool().
  param[in] IndexOfProtocolData   The index of protocol data.

  @retval EFI_SUCCESS     Protocol records are all returned.
  @retval EFI_NOT_FOUND   No more protocol records.
  @retval Others          Fail to get protocol records.
**/
EFI_STATUS
RedfishPlatformHostInterfaceProtocolData (
  IN OUT MC_HOST_INTERFACE_PROTOCOL_RECORD **ProtocolRecord,
  IN UINT8  IndexOfProtocolData
);
#endif
