/** @file
  VLAN Config Protocol implementation and VLAN packet process routine.

Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "MnpImpl.h"
#include "MnpVlan.h"

VLAN_DEVICE_PATH  mVlanDevicePathTemplate = {
  {
    MESSAGING_DEVICE_PATH,
    MSG_VLAN_DP,
    {
      (UINT8)(sizeof (VLAN_DEVICE_PATH)),
      (UINT8)((sizeof (VLAN_DEVICE_PATH)) >> 8)
    }
  },
  0
};

EFI_VLAN_CONFIG_PROTOCOL  mVlanConfigProtocolTemplate = {
  VlanConfigSet,
  VlanConfigFind,
  VlanConfigRemove
};

/**
  Create a child handle for the VLAN ID.

  @param[in]       ImageHandle        The driver image handle.
  @param[in]       ControllerHandle   Handle of device to bind driver to.
  @param[in]       VlanId             The VLAN ID.
  @param[out]      Devicepath         Pointer to returned device path for child handle.

  @return The handle of VLAN child or NULL if failed to create VLAN child.

**/
EFI_HANDLE
MnpCreateVlanChild (
  IN     EFI_HANDLE             ImageHandle,
  IN     EFI_HANDLE             ControllerHandle,
  IN     UINT16                 VlanId,
  OUT EFI_DEVICE_PATH_PROTOCOL  **Devicepath OPTIONAL
  )
{
  EFI_HANDLE                ChildHandle;
  VLAN_DEVICE_PATH          VlanNode;
  EFI_DEVICE_PATH_PROTOCOL  *ParentDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *VlanDevicePath;
  EFI_STATUS                Status;

  //
  // Try to get parent device path
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **)&ParentDevicePath,
                  ImageHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  //
  // Construct device path for child handle: MAC + VLAN
  //
  CopyMem (&VlanNode, &mVlanDevicePathTemplate, sizeof (VLAN_DEVICE_PATH));
  VlanNode.VlanId = VlanId;
  VlanDevicePath  = AppendDevicePathNode (
                      ParentDevicePath,
                      (EFI_DEVICE_PATH_PROTOCOL *)&VlanNode
                      );
  if (VlanDevicePath == NULL) {
    return NULL;
  }

  //
  // Create child VLAN handle by installing DevicePath protocol
  //
  ChildHandle = NULL;
  Status      = gBS->InstallMultipleProtocolInterfaces (
                       &ChildHandle,
                       &gEfiDevicePathProtocolGuid,
                       VlanDevicePath,
                       NULL
                       );
  if (EFI_ERROR (Status)) {
    FreePool (VlanDevicePath);
    return NULL;
  }

  if (Devicepath != NULL) {
    *Devicepath = VlanDevicePath;
  }

  return ChildHandle;
}

/**
  Remove VLAN tag from a packet.

  @param[in, out]  MnpDeviceData      Pointer to the mnp device context data.
  @param[in, out]  Nbuf               Pointer to the NET_BUF to remove VLAN tag.
  @param[out]      VlanId             Pointer to the returned VLAN ID.

  @retval TRUE             VLAN tag is removed from this packet.
  @retval FALSE            There is no VLAN tag in this packet.

**/
BOOLEAN
MnpRemoveVlanTag (
  IN OUT MNP_DEVICE_DATA  *MnpDeviceData,
  IN OUT NET_BUF          *Nbuf,
  OUT UINT16              *VlanId
  )
{
  UINT8     *Packet;
  UINTN     ProtocolOffset;
  UINT16    ProtocolType;
  VLAN_TCI  VlanTag;

  ProtocolOffset = MnpDeviceData->Snp->Mode->HwAddressSize * 2;

  //
  // Get the packet buffer.
  //
  Packet = NetbufGetByte (Nbuf, 0, NULL);
  ASSERT (Packet != NULL);

  //
  // Check whether this is VLAN tagged frame by Ether Type
  //
  *VlanId      = 0;
  ProtocolType = NTOHS (*(UINT16 *)(Packet + ProtocolOffset));
  if (ProtocolType != ETHER_TYPE_VLAN) {
    //
    // Not a VLAN tagged frame
    //
    return FALSE;
  }

  VlanTag.Uint16 = NTOHS (*(UINT16 *)(Packet + ProtocolOffset + sizeof (ProtocolType)));
  *VlanId        = VlanTag.Bits.Vid;

  //
  // Move hardware address (DA + SA) 4 bytes right to override VLAN tag
  //
  CopyMem (Packet + NET_VLAN_TAG_LEN, Packet, ProtocolOffset);

  //
  // Remove VLAN tag from the Nbuf
  //
  NetbufTrim (Nbuf, NET_VLAN_TAG_LEN, NET_BUF_HEAD);

  return TRUE;
}

/**
  Build the vlan packet to transmit from the TxData passed in.

  @param  MnpServiceData         Pointer to the mnp service context data.
  @param  TxData                 Pointer to the transmit data containing the
                                 information to build the packet.
  @param  ProtocolType           Pointer to the Ethernet protocol type.
  @param  Packet                 Pointer to record the address of the packet.
  @param  Length                 Pointer to a UINT32 variable used to record the
                                 packet's length.

**/
VOID
MnpInsertVlanTag (
  IN     MNP_SERVICE_DATA                   *MnpServiceData,
  IN     EFI_MANAGED_NETWORK_TRANSMIT_DATA  *TxData,
  OUT UINT16                                *ProtocolType,
  IN OUT UINT8                              **Packet,
  IN OUT UINT32                             *Length
  )
{
  VLAN_TCI                 *VlanTci;
  UINT16                   *Tpid;
  UINT16                   *EtherType;
  MNP_DEVICE_DATA          *MnpDeviceData;
  EFI_SIMPLE_NETWORK_MODE  *SnpMode;

  MnpDeviceData = MnpServiceData->MnpDeviceData;
  SnpMode       = MnpDeviceData->Snp->Mode;

  *ProtocolType = ETHER_TYPE_VLAN;
  *Length       = *Length + NET_VLAN_TAG_LEN;
  *Packet       = *Packet - NET_VLAN_TAG_LEN;

  Tpid    = (UINT16 *)(*Packet + SnpMode->MediaHeaderSize - sizeof (*ProtocolType));
  VlanTci = (VLAN_TCI *)(UINTN)(Tpid + 1);
  if (TxData->HeaderLength != 0) {
    //
    // Media header is in packet, move DA+SA 4 bytes left
    //
    CopyMem (
      *Packet,
      *Packet + NET_VLAN_TAG_LEN,
      SnpMode->MediaHeaderSize - sizeof (*ProtocolType)
      );
    *Tpid = HTONS (ETHER_TYPE_VLAN);
  } else {
    //
    // Media header not in packet, VLAN TCI and original protocol type becomes payload
    //
    EtherType  = (UINT16 *)(UINTN)(VlanTci + 1);
    *EtherType = HTONS (TxData->ProtocolType);
  }

  VlanTci->Bits.Vid      = MnpServiceData->VlanId;
  VlanTci->Bits.Cfi      = VLAN_TCI_CFI_CANONICAL_MAC;
  VlanTci->Bits.Priority = MnpServiceData->Priority;
  VlanTci->Uint16        = HTONS (VlanTci->Uint16);
}

/**
  Check VLAN configuration variable and delete the duplicative content if has identical Vlan ID.

  @param[in]      MnpDeviceData      Pointer to the MNP device context data.
  @param[in]      Buffer             Pointer to the buffer contains the array of VLAN_TCI.
  @param[in]      NumberOfVlan       Pointer to number of VLAN.
  @param[out]     NewNumberOfVlan    Pointer to number of unique VLAN.

  @retval EFI_SUCCESS            The VLAN variable is successfully checked.
  @retval EFI_OUT_OF_RESOURCES   There is not enough resource to set the configuration.

**/
EFI_STATUS
MnpCheckVlanVariable (
  IN     MNP_DEVICE_DATA  *MnpDeviceData,
  IN     VLAN_TCI         *Buffer,
  IN     UINTN            NumberOfVlan,
  OUT UINTN               *NewNumberOfVlan
  )
{
  UINTN       Index;
  UINTN       Index2;
  UINTN       Count;
  BOOLEAN     FoundDuplicateItem;
  EFI_STATUS  Status;

  Count              = 0;
  FoundDuplicateItem = FALSE;
  Status             = EFI_SUCCESS;

  for (Index = 0; Index < NumberOfVlan; Index++) {
    for (Index2 = Index + 1; Index2 < NumberOfVlan; Index2++) {
      if (Buffer[Index].Bits.Vid == Buffer[Index2].Bits.Vid) {
        FoundDuplicateItem = TRUE;
        Count++;
        break;
      }
    }

    if (FoundDuplicateItem) {
      for (Index2 = Index +1; Index2 < NumberOfVlan; Index++, Index2++) {
        CopyMem (Buffer + Index, Buffer + Index2, sizeof (VLAN_TCI));
      }
    }

    FoundDuplicateItem = FALSE;
  }

  *NewNumberOfVlan = NumberOfVlan - Count;
  if (Count != 0) {
    Status = MnpSetVlanVariable (MnpDeviceData, *NewNumberOfVlan, Buffer);
  }

  return Status;
}

/**
  Get VLAN configuration variable.

  @param[in]       MnpDeviceData      Pointer to the MNP device context data.
  @param[out]      NumberOfVlan       Pointer to number of VLAN to be returned.
  @param[out]      VlanVariable       Pointer to the buffer to return requested
                                      array of VLAN_TCI.

  @retval EFI_SUCCESS            The array of VLAN_TCI was returned in VlanVariable
                                 and number of VLAN was returned in NumberOfVlan.
  @retval EFI_NOT_FOUND          VLAN configuration variable not found.
  @retval EFI_OUT_OF_RESOURCES   There is not enough pool memory to store the configuration.

**/
EFI_STATUS
MnpGetVlanVariable (
  IN     MNP_DEVICE_DATA  *MnpDeviceData,
  OUT UINTN               *NumberOfVlan,
  OUT VLAN_TCI            **VlanVariable
  )
{
  UINTN       BufferSize;
  EFI_STATUS  Status;
  VLAN_TCI    *Buffer;
  UINTN       NewNumberOfVlan;

  //
  // Get VLAN configuration from EFI Variable
  //
  Buffer     = NULL;
  BufferSize = 0;
  Status     = gRT->GetVariable (
                      MnpDeviceData->MacString,
                      &gEfiVlanConfigProtocolGuid,
                      NULL,
                      &BufferSize,
                      NULL
                      );
  if (Status != EFI_BUFFER_TOO_SMALL) {
    return EFI_NOT_FOUND;
  }

  //
  // Allocate buffer to read the variable
  //
  Buffer = AllocateZeroPool (BufferSize);
  if (Buffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = gRT->GetVariable (
                  MnpDeviceData->MacString,
                  &gEfiVlanConfigProtocolGuid,
                  NULL,
                  &BufferSize,
                  Buffer
                  );
  if (EFI_ERROR (Status)) {
    FreePool (Buffer);
    return Status;
  }

  Status = MnpCheckVlanVariable (MnpDeviceData, Buffer, BufferSize / sizeof (VLAN_TCI), &NewNumberOfVlan);
  if (!EFI_ERROR (Status)) {
    *NumberOfVlan = NewNumberOfVlan;
    *VlanVariable = Buffer;
  }

  return Status;
}

/**
  Set VLAN configuration variable.

  @param[in] MnpDeviceData       Pointer to the MNP device context data.
  @param[in] NumberOfVlan        Number of VLAN in array VlanVariable.
  @param[in] VlanVariable        Pointer to array of VLAN_TCI.

  @retval EFI_SUCCESS            The VLAN variable is successfully set.
  @retval EFI_OUT_OF_RESOURCES   There is not enough resource to set the configuration.

**/
EFI_STATUS
MnpSetVlanVariable (
  IN MNP_DEVICE_DATA  *MnpDeviceData,
  IN UINTN            NumberOfVlan,
  IN VLAN_TCI         *VlanVariable
  )
{
  return gRT->SetVariable (
                MnpDeviceData->MacString,
                &gEfiVlanConfigProtocolGuid,
                EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS,
                NumberOfVlan * sizeof (VLAN_TCI),
                VlanVariable
                );
}

/**
  Create a VLAN device or modify the configuration parameter of an
  already-configured VLAN.

  The Set() function is used to create a new VLAN device or change the VLAN
  configuration parameters. If the VlanId hasn't been configured in the
  physical Ethernet device, a new VLAN device will be created. If a VLAN with
  this VlanId is already configured, then related configuration will be updated
  as the input parameters.

  If VlanId is zero, the VLAN device will send and receive untagged frames.
  Otherwise, the VLAN device will send and receive VLAN-tagged frames containing the VlanId.
  If VlanId is out of scope of (0-4094), EFI_INVALID_PARAMETER is returned.
  If Priority is out of the scope of (0-7), then EFI_INVALID_PARAMETER is returned.
  If there is not enough system memory to perform the registration, then
  EFI_OUT_OF_RESOURCES is returned.

  @param[in] This                Points to the EFI_VLAN_CONFIG_PROTOCOL.
  @param[in] VlanId              A unique identifier (1-4094) of the VLAN which is being created
                                 or modified, or zero (0).
  @param[in] Priority            3 bit priority in VLAN header. Priority 0 is default value. If
                                 VlanId is zero (0), Priority is ignored.

  @retval EFI_SUCCESS            The VLAN is successfully configured.
  @retval EFI_INVALID_PARAMETER  One or more of following conditions is TRUE:
                                 - This is NULL.
                                 - VlanId is an invalid VLAN Identifier.
                                 - Priority is invalid.
  @retval EFI_OUT_OF_RESOURCES   There is not enough system memory to perform the registration.

**/
EFI_STATUS
EFIAPI
VlanConfigSet (
  IN EFI_VLAN_CONFIG_PROTOCOL  *This,
  IN UINT16                    VlanId,
  IN UINT8                     Priority
  )
{
  EFI_STATUS        Status;
  MNP_DEVICE_DATA   *MnpDeviceData;
  MNP_SERVICE_DATA  *MnpServiceData;
  VLAN_TCI          *OldVariable;
  VLAN_TCI          *NewVariable;
  UINTN             NumberOfVlan;
  UINTN             Index;
  BOOLEAN           IsAdd;
  LIST_ENTRY        *Entry;

  if ((This == NULL) || (VlanId > 4094) || (Priority > 7)) {
    return EFI_INVALID_PARAMETER;
  }

  IsAdd         = FALSE;
  MnpDeviceData = MNP_DEVICE_DATA_FROM_THIS (This);
  if (MnpDeviceData->NumberOfVlan == 0) {
    //
    // No existing VLAN, this is the first VLAN to add
    //
    IsAdd          = TRUE;
    Entry          = GetFirstNode (&MnpDeviceData->ServiceList);
    MnpServiceData = MNP_SERVICE_DATA_FROM_LINK (Entry);

    if (VlanId != 0) {
      //
      // VlanId is not 0, need destroy the default MNP service data
      //
      Status = MnpDestroyServiceChild (MnpServiceData);
      if (EFI_ERROR (Status)) {
        return Status;
      }

      Status = MnpDestroyServiceData (MnpServiceData);
      if (EFI_ERROR (Status)) {
        return Status;
      }

      //
      // Create a new MNP service data for this VLAN
      //
      MnpServiceData = MnpCreateServiceData (MnpDeviceData, VlanId, Priority);
      if (MnpServiceData == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }
    }
  } else {
    //
    // Try to find VlanId in existing VLAN list
    //
    MnpServiceData = MnpFindServiceData (MnpDeviceData, VlanId);
    if (MnpServiceData == NULL) {
      //
      // VlanId not found, create a new MNP service data
      //
      IsAdd          = TRUE;
      MnpServiceData = MnpCreateServiceData (MnpDeviceData, VlanId, Priority);
      if (MnpServiceData == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }
    }
  }

  MnpServiceData->VlanId   = VlanId;
  MnpServiceData->Priority = Priority;
  if (IsAdd) {
    MnpDeviceData->NumberOfVlan++;
  }

  //
  // Update VLAN configuration variable
  //
  OldVariable  = NULL;
  NewVariable  = NULL;
  NumberOfVlan = 0;
  MnpGetVlanVariable (MnpDeviceData, &NumberOfVlan, &OldVariable);

  if (IsAdd) {
    //
    // VLAN not exist - add
    //
    NewVariable = AllocateZeroPool ((NumberOfVlan + 1) * sizeof (VLAN_TCI));
    if (NewVariable == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Exit;
    }

    if (OldVariable != NULL) {
      CopyMem (NewVariable, OldVariable, NumberOfVlan * sizeof (VLAN_TCI));
    }

    Index = NumberOfVlan++;
  } else {
    //
    // VLAN already exist - update
    //
    for (Index = 0; Index < NumberOfVlan; Index++) {
      if (OldVariable[Index].Bits.Vid == VlanId) {
        break;
      }
    }

    ASSERT (Index < NumberOfVlan);

    NewVariable = OldVariable;
    OldVariable = NULL;
  }

  NewVariable[Index].Bits.Vid      = VlanId;
  NewVariable[Index].Bits.Priority = Priority;

  Status = MnpSetVlanVariable (MnpDeviceData, NumberOfVlan, NewVariable);
  FreePool (NewVariable);

Exit:
  if (OldVariable != NULL) {
    FreePool (OldVariable);
  }

  return Status;
}

/**
  Find configuration information for specified VLAN or all configured VLANs.

  The Find() function is used to find the configuration information for matching
  VLAN and allocate a buffer into which those entries are copied.

  @param[in]  This               Points to the EFI_VLAN_CONFIG_PROTOCOL.
  @param[in]  VlanId             Pointer to VLAN identifier. Set to NULL to find all
                                 configured VLANs.
  @param[out] NumberOfVlan       The number of VLANs which is found by the specified criteria.
  @param[out] Entries            The buffer which receive the VLAN configuration.

  @retval EFI_SUCCESS            The VLAN is successfully found.
  @retval EFI_INVALID_PARAMETER  One or more of following conditions is TRUE:
                                 - This is NULL.
                                 - Specified VlanId is invalid.
  @retval EFI_NOT_FOUND          No matching VLAN is found.

**/
EFI_STATUS
EFIAPI
VlanConfigFind (
  IN     EFI_VLAN_CONFIG_PROTOCOL  *This,
  IN     UINT16                    *VlanId OPTIONAL,
  OUT UINT16                       *NumberOfVlan,
  OUT EFI_VLAN_FIND_DATA           **Entries
  )
{
  MNP_DEVICE_DATA     *MnpDeviceData;
  MNP_SERVICE_DATA    *MnpServiceData;
  LIST_ENTRY          *Entry;
  EFI_VLAN_FIND_DATA  *VlanData;

  if ((This == NULL) || ((VlanId != NULL) && (*VlanId > 4094)) || (NumberOfVlan == NULL) || (Entries == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *NumberOfVlan = 0;
  *Entries      = NULL;

  MnpDeviceData = MNP_DEVICE_DATA_FROM_THIS (This);
  if (MnpDeviceData->NumberOfVlan == 0) {
    return EFI_NOT_FOUND;
  }

  if (VlanId == NULL) {
    //
    // Return all current VLAN configuration
    //
    *NumberOfVlan = (UINT16)MnpDeviceData->NumberOfVlan;
    VlanData      = AllocateZeroPool (*NumberOfVlan * sizeof (EFI_VLAN_FIND_DATA));
    if (VlanData == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    *Entries = VlanData;
    NET_LIST_FOR_EACH (Entry, &MnpDeviceData->ServiceList) {
      MnpServiceData = MNP_SERVICE_DATA_FROM_LINK (Entry);

      VlanData->VlanId   = MnpServiceData->VlanId;
      VlanData->Priority = MnpServiceData->Priority;
      VlanData++;
    }

    return EFI_SUCCESS;
  }

  //
  // VlanId is specified, try to find it in current VLAN list
  //
  MnpServiceData = MnpFindServiceData (MnpDeviceData, *VlanId);
  if (MnpServiceData == NULL) {
    return EFI_NOT_FOUND;
  }

  VlanData = AllocateZeroPool (sizeof (EFI_VLAN_FIND_DATA));
  if (VlanData == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  VlanData->VlanId   = MnpServiceData->VlanId;
  VlanData->Priority = MnpServiceData->Priority;

  *NumberOfVlan = 1;
  *Entries      = VlanData;

  return EFI_SUCCESS;
}

/**
  Remove the configured VLAN device.

  The Remove() function is used to remove the specified VLAN device.
  If the VlanId is out of the scope of (0-4094), EFI_INVALID_PARAMETER is returned.
  If specified VLAN hasn't been previously configured, EFI_NOT_FOUND is returned.

  @param[in] This                Points to the EFI_VLAN_CONFIG_PROTOCOL.
  @param[in] VlanId              Identifier (0-4094) of the VLAN to be removed.

  @retval EFI_SUCCESS            The VLAN is successfully removed.
  @retval EFI_INVALID_PARAMETER  One or more of following conditions is TRUE:
                                 - This is NULL.
                                 - VlanId  is an invalid parameter.
  @retval EFI_NOT_FOUND          The to-be-removed VLAN does not exist.

**/
EFI_STATUS
EFIAPI
VlanConfigRemove (
  IN EFI_VLAN_CONFIG_PROTOCOL  *This,
  IN UINT16                    VlanId
  )
{
  EFI_STATUS        Status;
  MNP_DEVICE_DATA   *MnpDeviceData;
  MNP_SERVICE_DATA  *MnpServiceData;
  LIST_ENTRY        *Entry;
  VLAN_TCI          *VlanVariable;
  VLAN_TCI          *VlanData;

  if ((This == NULL) || (VlanId > 4094)) {
    return EFI_INVALID_PARAMETER;
  }

  MnpDeviceData = MNP_DEVICE_DATA_FROM_THIS (This);
  if (MnpDeviceData->NumberOfVlan == 0) {
    return EFI_NOT_FOUND;
  }

  //
  // Try to find the VlanId
  //
  MnpServiceData = MnpFindServiceData (MnpDeviceData, VlanId);
  if (MnpServiceData == NULL) {
    return EFI_NOT_FOUND;
  }

  MnpDeviceData->NumberOfVlan--;

  if ((VlanId != 0) || (MnpDeviceData->NumberOfVlan != 0)) {
    //
    // If VlanId is not 0 or VlanId is 0 and it is not the last VLAN to remove,
    // destroy its MNP service data
    //
    Status = MnpDestroyServiceChild (MnpServiceData);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    Status = MnpDestroyServiceData (MnpServiceData);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  if ((VlanId != 0) && (MnpDeviceData->NumberOfVlan == 0)) {
    //
    // This is the last VLAN to be removed, restore the default MNP service data
    //
    MnpServiceData = MnpCreateServiceData (MnpDeviceData, 0, 0);
    if (MnpServiceData == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
  }

  //
  // Update VLAN configuration variable
  //
  VlanVariable = NULL;
  if (MnpDeviceData->NumberOfVlan != 0) {
    VlanVariable = AllocatePool (MnpDeviceData->NumberOfVlan * sizeof (VLAN_TCI));
    if (VlanVariable == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    VlanData = VlanVariable;
    NET_LIST_FOR_EACH (Entry, &MnpDeviceData->ServiceList) {
      MnpServiceData = MNP_SERVICE_DATA_FROM_LINK (Entry);

      VlanData->Bits.Vid      = MnpServiceData->VlanId;
      VlanData->Bits.Priority = MnpServiceData->Priority;
      VlanData++;
    }
  }

  Status = MnpSetVlanVariable (MnpDeviceData, MnpDeviceData->NumberOfVlan, VlanVariable);

  if (VlanVariable != NULL) {
    FreePool (VlanVariable);
  }

  return Status;
}
