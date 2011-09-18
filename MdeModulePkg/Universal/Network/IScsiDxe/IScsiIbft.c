/** @file
  Implementation for iSCSI Boot Firmware Table publication.

Copyright (c) 2004 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "IScsiImpl.h"

BOOLEAN mIbftInstalled = FALSE;
UINTN   mTableKey;

/**
  Initialize the header of the iSCSI Boot Firmware Table.
  
  @param[out]  Header     The header of the iSCSI Boot Firmware Table.
  @param[in]   OemId      The OEM ID.
  @param[in]   OemTableId The OEM table ID for the iBFT.
**/
VOID
IScsiInitIbfTableHeader (
  OUT EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_HEADER   *Header,
  IN  UINT8                                       *OemId,
  IN  UINT64                                      *OemTableId
  )
{
  ZeroMem (Header, sizeof (EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_HEADER));

  Header->Signature = EFI_ACPI_3_0_ISCSI_BOOT_FIRMWARE_TABLE_SIGNATURE;
  Header->Length    = IBFT_HEAP_OFFSET;
  Header->Revision  = EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_REVISION;
  Header->Checksum  = 0;

  Header->OemId[0]  = 'I';
  Header->OemId[1]  = 'N';
  Header->OemId[2]  = 'T';
  Header->OemId[3]  = 'E';
  Header->OemId[4]  = 'L';
  
  CopyMem (Header->OemId, OemId, sizeof (Header->OemId));
  CopyMem (&Header->OemTableId, OemTableId, sizeof (UINT64));
}

/**
  Initialize the control section of the iSCSI Boot Firmware Table.
  
  @param[in]  Table       The ACPI table.
  @param[in]  HandleCount The number of the handles associated with iSCSI sessions, it's
                          equal to the number of iSCSI sessions.
**/
VOID
IScsiInitControlSection (
  IN EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_HEADER  *Table,
  IN UINTN                                      HandleCount
  )
{
  EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_CONTROL_STRUCTURE  *Control;
  UINTN                                                 NumOffset;

  Control = (EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_CONTROL_STRUCTURE *) (Table + 1);

  ZeroMem (Control, sizeof (EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_CONTROL_STRUCTURE));

  Control->Header.StructureId = EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_CONTROL_STRUCTURE_ID;
  Control->Header.Version     = EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_CONTROL_STRUCTURE_VERSION;
  Control->Header.Length      = (UINT16) sizeof (EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_CONTROL_STRUCTURE);

  //
  // Each session occupies two offsets, one for the NIC section,
  // the other for the Target section.
  //
  NumOffset = 2 * HandleCount;
  if (NumOffset > 4) {
    //
    // Need expand the control section if more than 2 NIC/Target sections
    // exist.
    //
    Control->Header.Length = (UINT16) (Control->Header.Length + (NumOffset - 4) * sizeof (UINT16));
  }
}

/**
  Add one item into the heap.

  @param[in, out]  Heap  On input, the current address of the heap; On output, the address of
                         the heap after the item is added.
  @param[in]       Data  The data to add into the heap.
  @param[in]       Len   Length of the Data in byte.
**/
VOID
IScsiAddHeapItem (
  IN OUT UINT8  **Heap,
  IN     VOID   *Data,
  IN     UINTN  Len
  )
{
  //
  // Add one byte for the NULL delimiter.
  //
  *Heap -= Len + 1;

  CopyMem (*Heap, Data, Len);
  *(*Heap + Len) = 0;
}

/**
  Fill the Initiator section of the iSCSI Boot Firmware Table.

  @param[in]       Table    The ACPI table.
  @param[in, out]  Heap     The heap.
  @param[in]       Handle   The handle associated with the iSCSI session.
**/
VOID
IScsiFillInitiatorSection (
  IN     EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_HEADER  *Table,
  IN OUT UINT8                                      **Heap,
  IN     EFI_HANDLE                                 Handle
  )
{
  EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_CONTROL_STRUCTURE    *Control;
  EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_INITIATOR_STRUCTURE  *Initiator;
  ISCSI_DRIVER_DATA                                       *DriverData;
  ISCSI_SESSION                                           *Session;
  ISCSI_PRIVATE_PROTOCOL                                  *IScsiIdentifier;
  EFI_STATUS                                              Status;

  Control = (EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_CONTROL_STRUCTURE *) (Table + 1);

  //
  // Initiator section immediately follows the control section.
  //
  Initiator = (EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_INITIATOR_STRUCTURE *) ((UINT8 *) Control + IBFT_ROUNDUP (Control->Header.Length));

  Control->InitiatorOffset = (UINT16) ((UINTN) Initiator - (UINTN) Table);

  ZeroMem (Initiator, sizeof (EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_INITIATOR_STRUCTURE));

  Initiator->Header.StructureId = EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_INITIATOR_STRUCTURE_ID;
  Initiator->Header.Version     = EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_INITIATOR_STRUCTURE_VERSION;
  Initiator->Header.Length      = (UINT16) sizeof (EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_INITIATOR_STRUCTURE);
  Initiator->Header.Flags       = EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_INITIATOR_STRUCTURE_FLAG_BLOCK_VALID | EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_INITIATOR_STRUCTURE_FLAG_BOOT_SELECTED;

  //
  // Get the identifier from the handle.
  //
  Status = gBS->HandleProtocol (Handle, &gEfiCallerIdGuid, (VOID **) &IScsiIdentifier);
  if (EFI_ERROR (Status)) {
    ASSERT (FALSE);
    return ;
  }

  DriverData  = ISCSI_DRIVER_DATA_FROM_IDENTIFIER (IScsiIdentifier);
  Session     = &DriverData->Session;

  //
  // Fill the iSCSI Initiator Name into the heap.
  //
  IScsiAddHeapItem (Heap, Session->InitiatorName, Session->InitiatorNameLength - 1);

  Initiator->IScsiNameLength  = (UINT16) (Session->InitiatorNameLength - 1);
  Initiator->IScsiNameOffset  = (UINT16) ((UINTN) *Heap - (UINTN) Table);
}

/**
  Map the v4 IP address into v6 IP address.

  @param[in]   V4 The v4 IP address.
  @param[out]  V6 The v6 IP address.
**/
VOID
IScsiMapV4ToV6Addr (
  IN  EFI_IPv4_ADDRESS *V4,
  OUT EFI_IPv6_ADDRESS *V6
  )
{
  UINTN Index;

  ZeroMem (V6, sizeof (EFI_IPv6_ADDRESS));

  V6->Addr[10]  = 0xff;
  V6->Addr[11]  = 0xff;

  for (Index = 0; Index < 4; Index++) {
    V6->Addr[12 + Index] = V4->Addr[Index];
  }
}

/**
  Get the NIC's PCI location and return it accroding to the composited
  format defined in iSCSI Boot Firmware Table.

  @param[in]  Controller  The handle of the controller.

  @return UINT16          The composited representation of the NIC PCI location.
  @retval 0               Other errors as indicated.
**/
UINT16
IScsiGetNICPciLocation (
  IN EFI_HANDLE  Controller
  )
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  EFI_HANDLE                PciIoHandle;
  EFI_PCI_IO_PROTOCOL       *PciIo;
  UINTN                     Segment;
  UINTN                     Bus;
  UINTN                     Device;
  UINTN                     Function;

  Status = gBS->HandleProtocol (
                  Controller,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **)&DevicePath
                  );
  if (EFI_ERROR (Status)) {
    return 0;
  }

  Status = gBS->LocateDevicePath (
                  &gEfiPciIoProtocolGuid,
                  &DevicePath,
                  &PciIoHandle
                  );
  if (EFI_ERROR (Status)) {
    return 0;
  }

  Status = gBS->HandleProtocol (PciIoHandle, &gEfiPciIoProtocolGuid, (VOID **)&PciIo);
  if (EFI_ERROR (Status)) {
    return 0;
  }

  Status = PciIo->GetLocation (PciIo, &Segment, &Bus, &Device, &Function);
  if (EFI_ERROR (Status)) {
    return 0;
  }

  return (UINT16) ((Bus << 8) | (Device << 3) | Function);
}

/**
  Fill the NIC and target sections in iSCSI Boot Firmware Table.

  @param[in]       Table       The buffer of the ACPI table.
  @param[in, out]  Heap        The heap buffer used to store the variable length parameters such as iSCSI name.
  @param[in]       HandleCount Count The number of handles having iSCSI private protocol installed.
  @param[in]       Handles     The handle buffer.
**/
VOID
IScsiFillNICAndTargetSections (
  IN     EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_HEADER  *Table,
  IN OUT UINT8                                      **Heap,
  IN     UINTN                                      HandleCount,
  IN     EFI_HANDLE                                 *Handles
  )
{
  EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_CONTROL_STRUCTURE  *Control;
  EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_NIC_STRUCTURE      *Nic;
  EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_TARGET_STRUCTURE   *Target;
  ISCSI_DRIVER_DATA                                     *DriverData;
  ISCSI_SESSION_CONFIG_DATA                             *SessionConfigData;
  ISCSI_CHAP_AUTH_CONFIG_NVDATA                         *AuthConfig;
  UINT16                                                *SectionOffset;
  UINTN                                                 Index;
  UINT16                                                Length;
  EFI_MAC_ADDRESS                                       MacAddress;
  UINTN                                                 HwAddressSize;
  ISCSI_PRIVATE_PROTOCOL                                *IScsiIdentifier;
  EFI_STATUS                                            Status;

  //
  // Get the offset of the first Nic and Target section.
  //
  Control = (EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_CONTROL_STRUCTURE *) (Table + 1);
  Nic     = (EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_NIC_STRUCTURE *) ((UINTN) Table +
          Control->InitiatorOffset + IBFT_ROUNDUP (sizeof (EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_INITIATOR_STRUCTURE)));
  Target  = (EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_TARGET_STRUCTURE *) ((UINTN) Nic +
          IBFT_ROUNDUP (sizeof (EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_NIC_STRUCTURE)));

  SectionOffset = &Control->NIC0Offset;

  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->HandleProtocol (Handles[Index], &gEfiCallerIdGuid, (VOID **)&IScsiIdentifier);
    if (EFI_ERROR (Status)) {
      ASSERT (FALSE);
      return ;
    }

    DriverData        = ISCSI_DRIVER_DATA_FROM_IDENTIFIER (IScsiIdentifier);
    SessionConfigData = &DriverData->Session.ConfigData;
    AuthConfig        = &DriverData->Session.AuthData.AuthConfig;

    //
    // Fill the Nic section.
    //
    ZeroMem (Nic, sizeof (EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_NIC_STRUCTURE));

    Nic->Header.StructureId = EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_NIC_STRUCTURE_ID;
    Nic->Header.Version     = EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_NIC_STRUCTURE_VERSION;
    Nic->Header.Length      = (UINT16) sizeof (EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_NIC_STRUCTURE);
    Nic->Header.Index       = (UINT8) Index;
    Nic->Header.Flags       = EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_NIC_STRUCTURE_FLAG_BLOCK_VALID |
                            EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_NIC_STRUCTURE_FLAG_BOOT_SELECTED |
                            EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_NIC_STRUCTURE_FLAG_GLOBAL;

    //
    // Get the subnet mask prefix length.
    //
    Nic->SubnetMaskPrefixLength = IScsiGetSubnetMaskPrefixLength (&SessionConfigData->NvData.SubnetMask);

    if (SessionConfigData->NvData.InitiatorInfoFromDhcp) {
      Nic->Origin = IpPrefixOriginDhcp;
    } else {
      Nic->Origin = IpPrefixOriginManual;
    }
    //
    // Map the various v4 addresses into v6 addresses.
    //
    IScsiMapV4ToV6Addr (&SessionConfigData->NvData.LocalIp, &Nic->Ip);
    IScsiMapV4ToV6Addr (&SessionConfigData->NvData.Gateway, &Nic->Gateway);
    IScsiMapV4ToV6Addr (&SessionConfigData->PrimaryDns, &Nic->PrimaryDns);
    IScsiMapV4ToV6Addr (&SessionConfigData->SecondaryDns, &Nic->SecondaryDns);
    IScsiMapV4ToV6Addr (&SessionConfigData->DhcpServer, &Nic->DhcpServer);

    Nic->VLanTag = NetLibGetVlanId (DriverData->Controller);

    Status = NetLibGetMacAddress (DriverData->Controller, &MacAddress, &HwAddressSize);
    ASSERT (Status == EFI_SUCCESS);
    CopyMem (Nic->Mac, MacAddress.Addr, sizeof (Nic->Mac));

    //
    // Get the PCI location of the Nic.
    //
    Nic->PciLocation  = IScsiGetNICPciLocation (DriverData->Controller);

    *SectionOffset    = (UINT16) ((UINTN) Nic - (UINTN) Table);
    SectionOffset++;

    //
    // Fill the Target section.
    //
    ZeroMem (Target, sizeof (EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_TARGET_STRUCTURE));

    Target->Header.StructureId  = EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_TARGET_STRUCTURE_ID;
    Target->Header.Version      = EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_TARGET_STRUCTURE_VERSION;
    Target->Header.Length       = (UINT16) sizeof (EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_TARGET_STRUCTURE);
    Target->Header.Index        = (UINT8) Index;
    Target->Header.Flags        = EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_TARGET_STRUCTURE_FLAG_BLOCK_VALID | EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_TARGET_STRUCTURE_FLAG_BOOT_SELECTED;
    Target->Port                = SessionConfigData->NvData.TargetPort;
    Target->CHAPType            = AuthConfig->CHAPType;
    Target->NicIndex            = (UINT8) Index;

    IScsiMapV4ToV6Addr (&SessionConfigData->NvData.TargetIp, &Target->Ip);
    CopyMem (Target->BootLun, SessionConfigData->NvData.BootLun, sizeof (Target->BootLun));

    //
    // Target iSCSI Name, CHAP name/secret, reverse CHAP name/secret.
    //
    Length = (UINT16) AsciiStrLen (SessionConfigData->NvData.TargetName);
    IScsiAddHeapItem (Heap, SessionConfigData->NvData.TargetName, Length);

    Target->IScsiNameLength = Length;
    Target->IScsiNameOffset = (UINT16) ((UINTN) *Heap - (UINTN) Table);

    if (Target->CHAPType != ISCSI_CHAP_NONE) {
      //
      // CHAP Name
      //
      Length = (UINT16) AsciiStrLen (AuthConfig->CHAPName);
      IScsiAddHeapItem (Heap, AuthConfig->CHAPName, Length);
      Target->CHAPNameLength  = Length;
      Target->CHAPNameOffset  = (UINT16) ((UINTN) *Heap - (UINTN) Table);

      //
      // CHAP Secret
      //
      Length = (UINT16) AsciiStrLen (AuthConfig->CHAPSecret);
      IScsiAddHeapItem (Heap, AuthConfig->CHAPSecret, Length);
      Target->CHAPSecretLength  = Length;
      Target->CHAPSecretOffset  = (UINT16) ((UINTN) *Heap - (UINTN) Table);

      if (Target->CHAPType == ISCSI_CHAP_MUTUAL) {
        //
        // Reverse CHAP Name
        //
        Length = (UINT16) AsciiStrLen (AuthConfig->ReverseCHAPName);
        IScsiAddHeapItem (Heap, AuthConfig->ReverseCHAPName, Length);
        Target->ReverseCHAPNameLength = Length;
        Target->ReverseCHAPNameOffset = (UINT16) ((UINTN) *Heap - (UINTN) Table);

        //
        // Reverse CHAP Secret
        //
        Length = (UINT16) AsciiStrLen (AuthConfig->ReverseCHAPSecret);
        IScsiAddHeapItem (Heap, AuthConfig->ReverseCHAPSecret, Length);
        Target->ReverseCHAPSecretLength = Length;
        Target->ReverseCHAPSecretOffset = (UINT16) ((UINTN) *Heap - (UINTN) Table);
      }
    }

    *SectionOffset = (UINT16) ((UINTN) Target - (UINTN) Table);
    SectionOffset++;

    //
    // Advance to the next NIC/Target pair
    //
    Nic    = (EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_NIC_STRUCTURE *) ((UINTN) Target +
           IBFT_ROUNDUP (sizeof (EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_TARGET_STRUCTURE)));
    Target = (EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_TARGET_STRUCTURE *) ((UINTN) Nic +
           IBFT_ROUNDUP (sizeof (EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_NIC_STRUCTURE)));
  }
}

/**
  Publish and remove the iSCSI Boot Firmware Table according to the iSCSI
  session status.
**/
VOID
IScsiPublishIbft (
  VOID
  )
{
  EFI_STATUS                                Status;
  EFI_ACPI_TABLE_PROTOCOL                   *AcpiTableProtocol;
  EFI_ACPI_ISCSI_BOOT_FIRMWARE_TABLE_HEADER *Table;
  UINTN                                     HandleCount;
  EFI_HANDLE                                *HandleBuffer;
  UINT8                                     *Heap;
  UINT8                                     Checksum;
  UINTN                                         Index;
  EFI_ACPI_3_0_ROOT_SYSTEM_DESCRIPTION_POINTER  *Rsdp;
  EFI_ACPI_DESCRIPTION_HEADER                   *Rsdt;

  Status = gBS->LocateProtocol (&gEfiAcpiTableProtocolGuid, NULL, (VOID **)&AcpiTableProtocol);
  if (EFI_ERROR (Status)) {
    return ;
  }


  //
  // Find ACPI table RSD_PTR from system table
  //
  for (Index = 0, Rsdp = NULL; Index < gST->NumberOfTableEntries; Index++) {
    if (CompareGuid (&(gST->ConfigurationTable[Index].VendorGuid), &gEfiAcpi20TableGuid) ||
      CompareGuid (&(gST->ConfigurationTable[Index].VendorGuid), &gEfiAcpi10TableGuid) ||
      CompareGuid (&(gST->ConfigurationTable[Index].VendorGuid), &gEfiAcpiTableGuid)
      ) {
      //
      // A match was found.
      //
      Rsdp = (EFI_ACPI_3_0_ROOT_SYSTEM_DESCRIPTION_POINTER *) gST->ConfigurationTable[Index].VendorTable;
      break;
    }
  }

  if (Rsdp == NULL) {
    return ;
  } else {
    Rsdt = (EFI_ACPI_DESCRIPTION_HEADER *) (UINTN) Rsdp->RsdtAddress;
  }


  if (mIbftInstalled) {
    Status = AcpiTableProtocol->UninstallAcpiTable (
                                  AcpiTableProtocol,
                                  mTableKey
                                  );
    if (EFI_ERROR (Status)) {
      return ;
    }
    mIbftInstalled = FALSE;
  }

  //
  // Get all iSCSI private protocols.
  //
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiCallerIdGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status)) {
    return ;
  }
  //
  // Allocate 4k bytes to hold the ACPI table.
  //
  Table = AllocateZeroPool (IBFT_MAX_SIZE);
  if (Table == NULL) {
    return ;
  }

  Heap = (UINT8 *) Table + IBFT_HEAP_OFFSET;

  //
  // Fill in the various section of the iSCSI Boot Firmware Table.
  //
  IScsiInitIbfTableHeader (Table, Rsdt->OemId, &Rsdt->OemTableId);
  IScsiInitControlSection (Table, HandleCount);
  IScsiFillInitiatorSection (Table, &Heap, HandleBuffer[0]);
  IScsiFillNICAndTargetSections (Table, &Heap, HandleCount, HandleBuffer);

  Checksum = CalculateCheckSum8((UINT8 *)Table, Table->Length);
  Table->Checksum = Checksum;

  FreePool (HandleBuffer);

  //
  // Install or update the iBFT table.
  //
  Status = AcpiTableProtocol->InstallAcpiTable (
                                AcpiTableProtocol,
                                Table,
                                Table->Length,
                                &mTableKey
                                );
  if (EFI_ERROR(Status)) {
    return;
  }

  mIbftInstalled = TRUE;
  FreePool (Table);
}
