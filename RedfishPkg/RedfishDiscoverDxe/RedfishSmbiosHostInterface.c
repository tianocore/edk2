/** @file
  RedfishSmbiosHostInterface.c

  Discover Redfish SMBIOS Host Interface.

  (C) Copyright 2021 Hewlett Packard Enterprise Development LP<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "RedfishDiscoverInternal.h"

SMBIOS_TABLE_TYPE42  *mType42Record;

/**
  The function gets information reported in Redfish Host Interface.

  It simply frees the packet.

  @param[in]  Smbios           SMBIOS protocol.
  @param[out] DeviceDescriptor Pointer to REDFISH_INTERFACE_DATA.
  @param[out] ProtocolData     Pointer to REDFISH_OVER_IP_PROTOCOL_DATA.

  @retval EFI_SUCCESS    Get host interface succesfully.
  @retval Otherwise      Fail to tet host interface.

**/
EFI_STATUS
RedfishGetHostInterfaceProtocolData (
  IN EFI_SMBIOS_PROTOCOL              *Smbios,
  OUT REDFISH_INTERFACE_DATA          **DeviceDescriptor,
  OUT REDFISH_OVER_IP_PROTOCOL_DATA   **ProtocolData
  )
{
  EFI_STATUS                        Status;
  EFI_SMBIOS_HANDLE                 SmbiosHandle;
  EFI_SMBIOS_TABLE_HEADER           *Record;
  UINT16                            Offset;
  UINT8                             *RecordTmp;
  UINT8                             ProtocolLength;
  UINT8                             SpecificDataLen;

  if ((Smbios == NULL) || (ProtocolData == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;
  Status = Smbios->GetNext (Smbios, &SmbiosHandle, NULL, &Record, NULL);
  while (!EFI_ERROR (Status) && SmbiosHandle != SMBIOS_HANDLE_PI_RESERVED) {
    if (Record->Type == SMBIOS_TYPE_MANAGEMENT_CONTROLLER_HOST_INTERFACE) {
      //
      // Check Interface Type, should be Network Host Interface = 40h
      //
      mType42Record = (SMBIOS_TABLE_TYPE42 *) Record;
      if (mType42Record->InterfaceType == MCHostInterfaceTypeNetworkHostInterface) {
        ASSERT (Record->Length >= 9);
        Offset = 5;
        RecordTmp = (UINT8 *) Record + Offset;
        //
        // Get interface specific data length.
        //
        SpecificDataLen = *RecordTmp;
        Offset += 1;
        RecordTmp = (UINT8 *) Record + Offset;

        //
        // Check Device Type, only PCI/PCIe Network Interface v2 is supported now.
        //
        if (*RecordTmp == REDFISH_HOST_INTERFACE_DEVICE_TYPE_PCI_PCIE_V2) {
          ASSERT (SpecificDataLen == sizeof (PCI_OR_PCIE_INTERFACE_DEVICE_DESCRIPTOR_V2) + 1);
          *DeviceDescriptor = (REDFISH_INTERFACE_DATA *)RecordTmp;
          Offset = Offset + SpecificDataLen;
          RecordTmp = (UINT8 *) Record + Offset;
          //
          // Check Protocol count. if > 1, only use the first protocol.
          //
          ASSERT (*RecordTmp == 1);
          Offset += 1;
          RecordTmp = (UINT8 *) Record + Offset;
          //
          // Check protocol identifier.
          //
          if (*RecordTmp == MCHostInterfaceProtocolTypeRedfishOverIP) {
            Offset += 1;
            RecordTmp = (UINT8 *) Record + Offset;
            ProtocolLength = *RecordTmp;

            Offset += 1;
            RecordTmp = (UINT8 *) Record + Offset;

            //
            // This SMBIOS record is invalid, if the length of protocol specific data for
            // Redfish Over IP protocol is wrong.
            //
            if ((*(RecordTmp + 90) + sizeof (REDFISH_OVER_IP_PROTOCOL_DATA) - 1) != ProtocolLength) {
              return EFI_SECURITY_VIOLATION;
            }

            Offset += ProtocolLength;
            //
            // This SMBIOS record is invalid, if the length is smaller than the offset.
            //
            if (Offset > mType42Record->Hdr.Length) {
              return EFI_SECURITY_VIOLATION;
            }
            *ProtocolData = (REDFISH_OVER_IP_PROTOCOL_DATA *)RecordTmp;
            return EFI_SUCCESS;
          }
        }
      }
    }
    Status = Smbios->GetNext (Smbios, &SmbiosHandle, NULL, &Record, NULL);
  }

  *ProtocolData = NULL;
  return EFI_NOT_FOUND;
}
