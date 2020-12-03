/** @file
  PCI/PCIe network interface instace of RedfishPlatformHostInterfaceLib

  Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>
  (C) Copyright 2020 Hewlett Packard Enterprise Development LP<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>
#include <Library/RedfishHostInterfaceLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include <Pcd/RestExServiceDevicePath.h>
#include <Guid/GlobalVariable.h>

#define VERBOSE_COLUME_SIZE  (16)

REDFISH_OVER_IP_PROTOCOL_DATA  *mRedfishOverIpProtocolData;
UINT8 mRedfishProtocolDataSize;

/**
  Get the MAC address of NIC.

  @param[out] MacAddress      Pointer to retrieve MAC address

  @retval   EFI_SUCCESS      MAC address is returned in MacAddress

**/
EFI_STATUS
GetMacAddressInformation (
  OUT EFI_MAC_ADDRESS *MacAddress
  )
{
  MAC_ADDR_DEVICE_PATH                     *Mac;
  REST_EX_SERVICE_DEVICE_PATH_DATA         *RestExServiceDevicePathData;
  EFI_DEVICE_PATH_PROTOCOL                 *RestExServiceDevicePath;
  MAC_ADDR_DEVICE_PATH                     *MacAddressDevicePath;

  Mac = NULL;
  RestExServiceDevicePathData  = NULL;
  RestExServiceDevicePath      = NULL;

  RestExServiceDevicePathData = (REST_EX_SERVICE_DEVICE_PATH_DATA *)PcdGetPtr(PcdRedfishRestExServiceDevicePath);
  if (RestExServiceDevicePathData == NULL ||
      RestExServiceDevicePathData->DevicePathNum == 0 ||
      !IsDevicePathValid (RestExServiceDevicePathData->DevicePath, 0)) {
    return EFI_NOT_FOUND;
  }

  RestExServiceDevicePath = RestExServiceDevicePathData->DevicePath;
  if (RestExServiceDevicePathData->DevicePathMatchMode != DEVICE_PATH_MATCH_MAC_NODE) {
    return EFI_NOT_FOUND;
  }

  //
  // Find Mac DevicePath Node.
  //
  while (!IsDevicePathEnd (RestExServiceDevicePath) &&
         ((DevicePathType (RestExServiceDevicePath) != MESSAGING_DEVICE_PATH) ||
          (DevicePathSubType (RestExServiceDevicePath) != MSG_MAC_ADDR_DP))) {
    RestExServiceDevicePath = NextDevicePathNode (RestExServiceDevicePath);
  }

  if (!IsDevicePathEnd (RestExServiceDevicePath)) {
    MacAddressDevicePath = (MAC_ADDR_DEVICE_PATH *)RestExServiceDevicePath;
    CopyMem ((VOID *)MacAddress, (VOID *)&MacAddressDevicePath->MacAddress, sizeof (EFI_MAC_ADDRESS));
    return EFI_SUCCESS;
  }
  return EFI_NOT_FOUND;
}

/**
  Get platform Redfish host interface device descriptor.

  @param[out] DeviceType        Pointer to retrieve device type.
  @param[out] DeviceDescriptor  Pointer to retrieve REDFISH_INTERFACE_DATA, caller has to free
                                this memory using FreePool().
  @retval EFI_SUCCESS     Device descriptor is returned successfully in DeviceDescriptor.
  @retval EFI_NOT_FOUND   No Redfish host interface descriptor provided on this platform.
  @retval Others          Fail to get device descriptor.
**/
EFI_STATUS
RedfishPlatformHostInterfaceDeviceDescriptor (
  OUT UINT8 *DeviceType,
  OUT REDFISH_INTERFACE_DATA  **DeviceDescriptor
)
{
  EFI_STATUS Status;
  EFI_MAC_ADDRESS MacAddress;
  REDFISH_INTERFACE_DATA *RedfishInterfaceData;
  PCI_OR_PCIE_INTERFACE_DEVICE_DESCRIPTOR_V2 *ThisDeviceDescriptor;

  RedfishInterfaceData = AllocateZeroPool (sizeof (PCI_OR_PCIE_INTERFACE_DEVICE_DESCRIPTOR_V2) + 1);
  if (RedfishInterfaceData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  RedfishInterfaceData->DeviceType = REDFISH_HOST_INTERFACE_DEVICE_TYPE_PCI_PCIE_V2;
  //
  // Fill up device type information.
  //
  ThisDeviceDescriptor = (PCI_OR_PCIE_INTERFACE_DEVICE_DESCRIPTOR_V2 *)((UINT8 *)RedfishInterfaceData + 1);
  ThisDeviceDescriptor->Length = sizeof (PCI_OR_PCIE_INTERFACE_DEVICE_DESCRIPTOR_V2) + 1;
  Status = GetMacAddressInformation (&MacAddress);
  if (EFI_ERROR (Status)) {
    FreePool (RedfishInterfaceData);
    return EFI_NOT_FOUND;
  }
  CopyMem ((VOID *)&ThisDeviceDescriptor->MacAddress, (VOID *)&MacAddress, sizeof (ThisDeviceDescriptor->MacAddress));
  *DeviceType = REDFISH_HOST_INTERFACE_DEVICE_TYPE_PCI_PCIE_V2;
  *DeviceDescriptor = RedfishInterfaceData;
  return EFI_SUCCESS;
}
/**
  Get platform Redfish host interface protocol data.
  Caller should pass NULL in ProtocolRecord to retrive the first protocol record.
  Then continuously pass previous ProtocolRecord for retrieving the next ProtocolRecord.

  @param[out] ProtocolRecord     Pointer to retrieve the protocol record.
                                 caller has to free the new protocol record returned from
                                 this function using FreePool().
  @param[in] IndexOfProtocolData The index of protocol data.

  @retval EFI_SUCCESS     Protocol records are all returned.
  @retval EFI_NOT_FOUND   No more protocol records.
  @retval Others          Fail to get protocol records.
**/
EFI_STATUS
RedfishPlatformHostInterfaceProtocolData (
  OUT MC_HOST_INTERFACE_PROTOCOL_RECORD **ProtocolRecord,
  IN UINT8  IndexOfProtocolData
)
{
  MC_HOST_INTERFACE_PROTOCOL_RECORD *ThisProtocolRecord;

  if (mRedfishOverIpProtocolData == 0) {
    return EFI_NOT_FOUND;
  }
  if (IndexOfProtocolData == 0) {
    //
    // Return the first Redfish protocol data to caller. We only have
    // one protocol data in this case.
    //
    ThisProtocolRecord = (MC_HOST_INTERFACE_PROTOCOL_RECORD *) AllocatePool (mRedfishProtocolDataSize + sizeof (MC_HOST_INTERFACE_PROTOCOL_RECORD) - 1);
    ThisProtocolRecord->ProtocolType = MCHostInterfaceProtocolTypeRedfishOverIP;
    ThisProtocolRecord->ProtocolTypeDataLen = mRedfishProtocolDataSize;
    CopyMem ((VOID *)&ThisProtocolRecord->ProtocolTypeData, (VOID *)mRedfishOverIpProtocolData, mRedfishProtocolDataSize);
    *ProtocolRecord = ThisProtocolRecord;
    return EFI_SUCCESS;
  }
  return EFI_NOT_FOUND;
}
/**
  Dump IPv4 address.

  @param[in] Ip IPv4 address
**/
VOID
InternalDumpIp4Addr (
  IN EFI_IPv4_ADDRESS   *Ip
  )
{
  UINTN                 Index;

  for (Index = 0; Index < 4; Index++) {
    DEBUG ((DEBUG_VERBOSE, "%d", Ip->Addr[Index]));
    if (Index < 3) {
      DEBUG ((DEBUG_VERBOSE, "."));
    }
  }

  DEBUG ((DEBUG_VERBOSE, "\n"));
}
/**
  Dump IPv6 address.

  @param[in] Ip IPv6 address
**/
VOID
InternalDumpIp6Addr (
  IN EFI_IPv6_ADDRESS   *Ip
  )
{
  UINTN Index;

  for (Index = 0; Index < 16; Index++) {
    if (Ip->Addr[Index] != 0) {
      DEBUG ((DEBUG_VERBOSE, "%x", Ip->Addr[Index]));
    }
    Index++;

    if (Index > 15) {
      return;
    }

    if (((Ip->Addr[Index] & 0xf0) == 0) && (Ip->Addr[Index - 1] != 0)) {
      DEBUG ((DEBUG_VERBOSE, "0"));
    }
    DEBUG ((DEBUG_VERBOSE, "%x", Ip->Addr[Index]));

    if (Index < 15) {
      DEBUG ((DEBUG_VERBOSE, ":"));
    }
  }
  DEBUG ((DEBUG_VERBOSE, "\n"));
}
/**
  Dump data

  @param[in] Data Pointer to data.
  @param[in] Size size of data to dump.
**/
VOID
InternalDumpData (
  IN UINT8  *Data,
  IN UINTN  Size
  )
{
  UINTN  Index;
  for (Index = 0; Index < Size; Index++) {
    DEBUG ((DEBUG_VERBOSE, "%02x ", (UINTN)Data[Index]));
  }
}
/**
  Dump hex data

  @param[in] Data Pointer to hex data.
  @param[in] Size size of hex data to dump.
**/
VOID
InternalDumpHex (
  IN UINT8  *Data,
  IN UINTN  Size
  )
{
  UINTN   Index;
  UINTN   Count;
  UINTN   Left;

  Count = Size / VERBOSE_COLUME_SIZE;
  Left  = Size % VERBOSE_COLUME_SIZE;
  for (Index = 0; Index < Count; Index++) {
    InternalDumpData (Data + Index * VERBOSE_COLUME_SIZE, VERBOSE_COLUME_SIZE);
    DEBUG ((DEBUG_VERBOSE, "\n"));
  }

  if (Left != 0) {
    InternalDumpData (Data + Index * VERBOSE_COLUME_SIZE, Left);
    DEBUG ((DEBUG_VERBOSE, "\n"));
  }

  DEBUG ((DEBUG_VERBOSE, "\n"));
}
/**
  Dump Redfish over IP protocol data

  @param[in] RedfishProtocolData     Pointer to REDFISH_OVER_IP_PROTOCOL_DATA
  @param[in] RedfishProtocolDataSize size of data to dump.
**/
VOID
DumpRedfishIpProtocolData (
  IN REDFISH_OVER_IP_PROTOCOL_DATA   *RedfishProtocolData,
  IN UINT8                           RedfishProtocolDataSize
  )
{
  CHAR16 Hostname[16];

  DEBUG ((DEBUG_VERBOSE, "RedfishProtocolData: \n"));
  InternalDumpHex ((UINT8 *) RedfishProtocolData, RedfishProtocolDataSize);

  DEBUG ((DEBUG_VERBOSE, "Parsing as below: \n"));

  DEBUG ((DEBUG_VERBOSE, "RedfishProtocolData->ServiceUuid - %g\n", &(RedfishProtocolData->ServiceUuid)));

  DEBUG ((DEBUG_VERBOSE, "RedfishProtocolData->HostIpAssignmentType - %d\n", RedfishProtocolData->HostIpAssignmentType));

  DEBUG ((DEBUG_VERBOSE, "RedfishProtocolData->HostIpAddressFormat - %d\n", RedfishProtocolData->HostIpAddressFormat));

  DEBUG ((DEBUG_VERBOSE, "RedfishProtocolData->HostIpAddress: \n"));
  if (RedfishProtocolData->HostIpAddressFormat == 0x01) {
    InternalDumpIp4Addr ((EFI_IPv4_ADDRESS *) (RedfishProtocolData->HostIpAddress));
  } else {
    InternalDumpIp6Addr ((EFI_IPv6_ADDRESS *) (RedfishProtocolData->HostIpAddress));
  }

  DEBUG ((DEBUG_VERBOSE, "RedfishProtocolData->HostIpMask: \n"));
  if (RedfishProtocolData->HostIpAddressFormat == 0x01) {
    InternalDumpIp4Addr ((EFI_IPv4_ADDRESS *) (RedfishProtocolData->HostIpMask));
  } else {
    InternalDumpIp6Addr ((EFI_IPv6_ADDRESS *) (RedfishProtocolData->HostIpMask));
  }

  DEBUG ((DEBUG_VERBOSE, "RedfishProtocolData->RedfishServiceIpDiscoveryType - %d\n", RedfishProtocolData->RedfishServiceIpDiscoveryType));

  DEBUG ((DEBUG_VERBOSE, "RedfishProtocolData->RedfishServiceIpAddressFormat - %d\n", RedfishProtocolData->RedfishServiceIpAddressFormat));

  DEBUG ((DEBUG_VERBOSE, "RedfishProtocolData->RedfishServiceIpAddress: \n"));
  if (RedfishProtocolData->RedfishServiceIpAddressFormat == 0x01) {
    InternalDumpIp4Addr ((EFI_IPv4_ADDRESS *) (RedfishProtocolData->RedfishServiceIpAddress));
  } else {
    InternalDumpIp6Addr ((EFI_IPv6_ADDRESS *) (RedfishProtocolData->RedfishServiceIpAddress));
  }

  DEBUG ((DEBUG_VERBOSE, "RedfishProtocolData->RedfishServiceIpMask: \n"));
  if (RedfishProtocolData->RedfishServiceIpAddressFormat == 0x01) {
    InternalDumpIp4Addr ((EFI_IPv4_ADDRESS *) (RedfishProtocolData->RedfishServiceIpMask));
  } else {
    InternalDumpIp6Addr ((EFI_IPv6_ADDRESS *) (RedfishProtocolData->RedfishServiceIpMask));
  }

  DEBUG ((DEBUG_VERBOSE, "RedfishProtocolData->RedfishServiceIpPort - %d\n", RedfishProtocolData->RedfishServiceIpPort));

  DEBUG ((DEBUG_VERBOSE, "RedfishProtocolData->RedfishServiceVlanId - %d\n", RedfishProtocolData->RedfishServiceVlanId));

  DEBUG ((DEBUG_VERBOSE, "RedfishProtocolData->RedfishServiceHostnameLength - %d\n", RedfishProtocolData->RedfishServiceHostnameLength));

  AsciiStrToUnicodeStrS((CHAR8 *) RedfishProtocolData->RedfishServiceHostname, Hostname, sizeof (Hostname) / sizeof (Hostname[0]));
  DEBUG ((DEBUG_VERBOSE, "RedfishProtocolData->RedfishServiceHostname - %s\n", Hostname));
}

/**
  Get Redfish host interface protocol data from variale.

  @param[out] RedfishProtocolData  Pointer to retrieve REDFISH_OVER_IP_PROTOCOL_DATA.
  @param[out] RedfishProtocolDataSize  Size of REDFISH_OVER_IP_PROTOCOL_DATA.

  @retval EFI_SUCESS   REDFISH_OVER_IP_PROTOCOL_DATA is returned successfully.
**/
EFI_STATUS
GetRedfishRecordFromVariable (
  OUT REDFISH_OVER_IP_PROTOCOL_DATA   **RedfishProtocolData,
  OUT UINT8 *RedfishProtocolDataSize
  )
{
  EFI_STATUS                      Status;
  UINT8                           HostIpAssignmentType;
  UINTN                           HostIpAssignmentTypeSize;
  EFI_IPv4_ADDRESS                HostIpAddress;
  UINTN                           IPv4DataSize;
  EFI_IPv4_ADDRESS                HostIpMask;
  EFI_IPv4_ADDRESS                RedfishServiceIpAddress;
  EFI_IPv4_ADDRESS                RedfishServiceIpMask;
  UINT16                          RedfishServiceIpPort;
  UINTN                           IpPortDataSize;
  UINT8                           HostNameSize;
  CHAR8                           RedfishHostName[20];

  if (RedfishProtocolData == NULL || RedfishProtocolDataSize == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // 1. Retrieve Address Information from variable.
  //
  Status = gRT->GetVariable (
                  L"HostIpAssignmentType",
                  &gEmuRedfishServiceGuid,
                  NULL,
                  &HostIpAssignmentTypeSize,
                  &HostIpAssignmentType
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "RedfishPlatformDxe: GetVariable HostIpAssignmentType - %r\n", Status));
    return Status;
  }

  IPv4DataSize = sizeof (EFI_IPv4_ADDRESS);
  if (HostIpAssignmentType == 1 ) {
    Status = gRT->GetVariable (
                    L"HostIpAddress",
                    &gEmuRedfishServiceGuid,
                    NULL,
                    &IPv4DataSize,
                    &HostIpAddress
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "RedfishPlatformDxe: GetVariable HostIpAddress - %r\n", Status));
      return Status;
    }

    Status = gRT->GetVariable (
                    L"HostIpMask",
                    &gEmuRedfishServiceGuid,
                    NULL,
                    &IPv4DataSize,
                    &HostIpMask
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "RedfishPlatformDxe: GetVariable HostIpMask - %r\n", Status));
      return Status;
    }
  }

  Status = gRT->GetVariable (
                  L"RedfishServiceIpAddress",
                  &gEmuRedfishServiceGuid,
                  NULL,
                  &IPv4DataSize,
                  &RedfishServiceIpAddress
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "RedfishPlatformDxe: GetVariable RedfishServiceIpAddress - %r\n", Status));
    return Status;
  }

  Status = gRT->GetVariable (
                  L"RedfishServiceIpMask",
                  &gEmuRedfishServiceGuid,
                  NULL,
                  &IPv4DataSize,
                  &RedfishServiceIpMask
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "RedfishPlatformDxe: GetVariable RedfishServiceIpMask - %r\n", Status));
    return Status;
  }

  Status = gRT->GetVariable (
                  L"RedfishServiceIpPort",
                  &gEmuRedfishServiceGuid,
                  NULL,
                  &IpPortDataSize,
                  &RedfishServiceIpPort
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "RedfishPlatformDxe: GetVariable RedfishServiceIpPort - %r\n", Status));
    return Status;
  }

  AsciiSPrint (
    RedfishHostName,
    sizeof (RedfishHostName),
    "%d.%d.%d.%d",
    RedfishServiceIpAddress.Addr[0],
    RedfishServiceIpAddress.Addr[1],
    RedfishServiceIpAddress.Addr[2],
    RedfishServiceIpAddress.Addr[3]
    );

  HostNameSize = (UINT8) AsciiStrLen (RedfishHostName) + 1;

  //
  // 2. Protocol Data Size.
  //
  *RedfishProtocolDataSize = sizeof (REDFISH_OVER_IP_PROTOCOL_DATA) - 1 + HostNameSize;

  //
  // 3. Protocol Data.
  //
  *RedfishProtocolData = (REDFISH_OVER_IP_PROTOCOL_DATA *) AllocateZeroPool (*RedfishProtocolDataSize);
  if (*RedfishProtocolData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  CopyGuid (&(*RedfishProtocolData)->ServiceUuid, &gEmuRedfishServiceGuid);

  (*RedfishProtocolData)->HostIpAssignmentType = HostIpAssignmentType;
  (*RedfishProtocolData)->HostIpAddressFormat = 1;   // Only support IPv4

  if (HostIpAssignmentType == 1 ) {
    (*RedfishProtocolData)->HostIpAddress[0] = HostIpAddress.Addr[0];
    (*RedfishProtocolData)->HostIpAddress[1] = HostIpAddress.Addr[1];
    (*RedfishProtocolData)->HostIpAddress[2] = HostIpAddress.Addr[2];
    (*RedfishProtocolData)->HostIpAddress[3] = HostIpAddress.Addr[3];

    (*RedfishProtocolData)->HostIpMask[0] = HostIpMask.Addr[0];
    (*RedfishProtocolData)->HostIpMask[1] = HostIpMask.Addr[1];
    (*RedfishProtocolData)->HostIpMask[2] = HostIpMask.Addr[2];
    (*RedfishProtocolData)->HostIpMask[3] = HostIpMask.Addr[3];
  }

  (*RedfishProtocolData)->RedfishServiceIpDiscoveryType = 1;  // Use static IP address
  (*RedfishProtocolData)->RedfishServiceIpAddressFormat = 1;  // Only support IPv4

  (*RedfishProtocolData)->RedfishServiceIpAddress[0] = RedfishServiceIpAddress.Addr[0];
  (*RedfishProtocolData)->RedfishServiceIpAddress[1] = RedfishServiceIpAddress.Addr[1];
  (*RedfishProtocolData)->RedfishServiceIpAddress[2] = RedfishServiceIpAddress.Addr[2];
  (*RedfishProtocolData)->RedfishServiceIpAddress[3] = RedfishServiceIpAddress.Addr[3];

  (*RedfishProtocolData)->RedfishServiceIpMask[0] = RedfishServiceIpMask.Addr[0];
  (*RedfishProtocolData)->RedfishServiceIpMask[1] = RedfishServiceIpMask.Addr[1];
  (*RedfishProtocolData)->RedfishServiceIpMask[2] = RedfishServiceIpMask.Addr[2];
  (*RedfishProtocolData)->RedfishServiceIpMask[3] = RedfishServiceIpMask.Addr[3];

  (*RedfishProtocolData)->RedfishServiceIpPort = RedfishServiceIpPort;
  (*RedfishProtocolData)->RedfishServiceVlanId = 0xffffffff;

  (*RedfishProtocolData)->RedfishServiceHostnameLength = HostNameSize;
  AsciiStrCpyS ((CHAR8 *) ((*RedfishProtocolData)->RedfishServiceHostname), HostNameSize, RedfishHostName);

  return Status;
}

/**
  Construct Redfish host interface protocol data.

  @param ImageHandle     The image handle.
  @param SystemTable     The system table.

  @retval  EFI_SUCEESS  Install Boot manager menu success.
  @retval  Other        Return error status.

**/
EFI_STATUS
EFIAPI
RedfishPlatformHostInterfaceConstructor (
  IN EFI_HANDLE                            ImageHandle,
  IN EFI_SYSTEM_TABLE                      *SystemTable
)
{
  EFI_STATUS Status;

  Status = GetRedfishRecordFromVariable (&mRedfishOverIpProtocolData, &mRedfishProtocolDataSize);
  DEBUG ((DEBUG_INFO, "%a: GetRedfishRecordFromVariable() - %r\n", __FUNCTION__, Status));
  if (!EFI_ERROR (Status)) {
    DumpRedfishIpProtocolData (mRedfishOverIpProtocolData, mRedfishProtocolDataSize);
  }
  return EFI_SUCCESS;
}
