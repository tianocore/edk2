/** @file
  Implementation of Managed Network Protocol private services.

Copyright (c) 2005 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "MnpImpl.h"
#include "MnpVlan.h"

EFI_SERVICE_BINDING_PROTOCOL  mMnpServiceBindingProtocol = {
  MnpServiceBindingCreateChild,
  MnpServiceBindingDestroyChild
};

EFI_MANAGED_NETWORK_PROTOCOL  mMnpProtocolTemplate = {
  MnpGetModeData,
  MnpConfigure,
  MnpMcastIpToMac,
  MnpGroups,
  MnpTransmit,
  MnpReceive,
  MnpCancel,
  MnpPoll
};

EFI_MANAGED_NETWORK_CONFIG_DATA  mMnpDefaultConfigData = {
  10000000,
  10000000,
  0,
  FALSE,
  FALSE,
  FALSE,
  FALSE,
  FALSE,
  FALSE,
  FALSE
};

/**
  Add Count of net buffers to MnpDeviceData->FreeNbufQue. The length of the net
  buffer is specified by MnpDeviceData->BufferLength.

  @param[in, out]  MnpDeviceData         Pointer to the MNP_DEVICE_DATA.
  @param[in]       Count                 Number of NET_BUFFERs to add.

  @retval EFI_SUCCESS           The specified amount of NET_BUFs are allocated
                                and added to MnpDeviceData->FreeNbufQue.
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate a NET_BUF structure.

**/
EFI_STATUS
MnpAddFreeNbuf (
  IN OUT MNP_DEVICE_DATA  *MnpDeviceData,
  IN     UINTN            Count
  )
{
  EFI_STATUS  Status;
  UINTN       Index;
  NET_BUF     *Nbuf;

  NET_CHECK_SIGNATURE (MnpDeviceData, MNP_DEVICE_DATA_SIGNATURE);
  ASSERT ((Count > 0) && (MnpDeviceData->BufferLength > 0));

  Status = EFI_SUCCESS;
  for (Index = 0; Index < Count; Index++) {
    Nbuf = NetbufAlloc (MnpDeviceData->BufferLength + MnpDeviceData->PaddingSize);
    if (Nbuf == NULL) {
      DEBUG ((DEBUG_ERROR, "MnpAddFreeNbuf: NetBufAlloc failed.\n"));

      Status = EFI_OUT_OF_RESOURCES;
      break;
    }

    if (MnpDeviceData->PaddingSize > 0) {
      //
      // Pad padding bytes before the media header
      //
      NetbufAllocSpace (Nbuf, MnpDeviceData->PaddingSize, NET_BUF_TAIL);
      NetbufTrim (Nbuf, MnpDeviceData->PaddingSize, NET_BUF_HEAD);
    }

    NetbufQueAppend (&MnpDeviceData->FreeNbufQue, Nbuf);
  }

  MnpDeviceData->NbufCnt += Index;
  return Status;
}

/**
  Allocate a free NET_BUF from MnpDeviceData->FreeNbufQue. If there is none
  in the queue, first try to allocate some and add them into the queue, then
  fetch the NET_BUF from the updated FreeNbufQue.

  @param[in, out]  MnpDeviceData        Pointer to the MNP_DEVICE_DATA.

  @return     Pointer to the allocated free NET_BUF structure, if NULL the
              operation is failed.

**/
NET_BUF *
MnpAllocNbuf (
  IN OUT MNP_DEVICE_DATA  *MnpDeviceData
  )
{
  EFI_STATUS     Status;
  NET_BUF_QUEUE  *FreeNbufQue;
  NET_BUF        *Nbuf;
  EFI_TPL        OldTpl;

  NET_CHECK_SIGNATURE (MnpDeviceData, MNP_DEVICE_DATA_SIGNATURE);

  FreeNbufQue = &MnpDeviceData->FreeNbufQue;
  OldTpl      = gBS->RaiseTPL (TPL_NOTIFY);

  //
  // Check whether there are available buffers, or else try to add some.
  //
  if (FreeNbufQue->BufNum == 0) {
    if ((MnpDeviceData->NbufCnt + MNP_NET_BUFFER_INCREASEMENT) > MNP_MAX_NET_BUFFER_NUM) {
      DEBUG (
        (DEBUG_ERROR,
         "MnpAllocNbuf: The maximum NET_BUF size is reached for MNP driver instance %p.\n",
         MnpDeviceData)
        );

      Nbuf = NULL;
      goto ON_EXIT;
    }

    Status = MnpAddFreeNbuf (MnpDeviceData, MNP_NET_BUFFER_INCREASEMENT);
    if (EFI_ERROR (Status)) {
      DEBUG (
        (DEBUG_ERROR,
         "MnpAllocNbuf: Failed to add NET_BUFs into the FreeNbufQue, %r.\n",
         Status)
        );

      //
      // Don't return NULL, perhaps MnpAddFreeNbuf does add some NET_BUFs but
      // the amount is less than MNP_NET_BUFFER_INCREASEMENT.
      //
    }
  }

  Nbuf = NetbufQueRemove (FreeNbufQue);

  //
  // Increase the RefCnt.
  //
  if (Nbuf != NULL) {
    NET_GET_REF (Nbuf);
  }

ON_EXIT:
  gBS->RestoreTPL (OldTpl);

  return Nbuf;
}

/**
  Try to reclaim the Nbuf into the buffer pool.

  @param[in, out]  MnpDeviceData         Pointer to the mnp device context data.
  @param[in, out]  Nbuf                  Pointer to the NET_BUF to free.

**/
VOID
MnpFreeNbuf (
  IN OUT MNP_DEVICE_DATA  *MnpDeviceData,
  IN OUT NET_BUF          *Nbuf
  )
{
  EFI_TPL  OldTpl;

  NET_CHECK_SIGNATURE (MnpDeviceData, MNP_DEVICE_DATA_SIGNATURE);
  ASSERT (Nbuf->RefCnt > 1);

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);

  NET_PUT_REF (Nbuf);

  if (Nbuf->RefCnt == 1) {
    //
    // Trim all buffer contained in the Nbuf, then append it to the NbufQue.
    //
    NetbufTrim (Nbuf, Nbuf->TotalSize, NET_BUF_TAIL);

    if (NetbufAllocSpace (Nbuf, NET_VLAN_TAG_LEN, NET_BUF_HEAD) != NULL) {
      //
      // There is space reserved for vlan tag in the head, reclaim it
      //
      NetbufTrim (Nbuf, NET_VLAN_TAG_LEN, NET_BUF_TAIL);
    }

    NetbufQueAppend (&MnpDeviceData->FreeNbufQue, Nbuf);
  }

  gBS->RestoreTPL (OldTpl);
}

/**
  Add Count of TX buffers to MnpDeviceData->AllTxBufList and MnpDeviceData->FreeTxBufList.
  The length of the buffer is specified by MnpDeviceData->BufferLength.

  @param[in, out]  MnpDeviceData         Pointer to the MNP_DEVICE_DATA.
  @param[in]       Count                 Number of TX buffers to add.

  @retval EFI_SUCCESS           The specified amount of TX buffers are allocated.
  @retval EFI_OUT_OF_RESOURCES  Failed to allocate a TX buffer.

**/
EFI_STATUS
MnpAddFreeTxBuf (
  IN OUT MNP_DEVICE_DATA  *MnpDeviceData,
  IN     UINTN            Count
  )
{
  EFI_STATUS       Status;
  UINT32           Index;
  MNP_TX_BUF_WRAP  *TxBufWrap;

  NET_CHECK_SIGNATURE (MnpDeviceData, MNP_DEVICE_DATA_SIGNATURE);
  ASSERT ((Count > 0) && (MnpDeviceData->BufferLength > 0));

  Status = EFI_SUCCESS;
  for (Index = 0; Index < Count; Index++) {
    TxBufWrap = (MNP_TX_BUF_WRAP *)AllocatePool (OFFSET_OF (MNP_TX_BUF_WRAP, TxBuf) + MnpDeviceData->BufferLength);
    if (TxBufWrap == NULL) {
      DEBUG ((DEBUG_ERROR, "MnpAddFreeTxBuf: TxBuf Alloc failed.\n"));

      Status = EFI_OUT_OF_RESOURCES;
      break;
    }

    DEBUG ((DEBUG_VERBOSE, "MnpAddFreeTxBuf: Add TxBufWrap %p, TxBuf %p\n", TxBufWrap, TxBufWrap->TxBuf));
    TxBufWrap->Signature = MNP_TX_BUF_WRAP_SIGNATURE;
    TxBufWrap->InUse     = FALSE;
    InsertTailList (&MnpDeviceData->FreeTxBufList, &TxBufWrap->WrapEntry);
    InsertTailList (&MnpDeviceData->AllTxBufList, &TxBufWrap->AllEntry);
  }

  MnpDeviceData->TxBufCount += Index;
  return Status;
}

/**
  Allocate a free TX buffer from MnpDeviceData->FreeTxBufList. If there is none
  in the queue, first try to recycle some from SNP, then try to allocate some and add
  them into the queue, then fetch the NET_BUF from the updated FreeTxBufList.

  @param[in, out]  MnpDeviceData        Pointer to the MNP_DEVICE_DATA.

  @return     Pointer to the allocated free NET_BUF structure, if NULL the
              operation is failed.

**/
UINT8 *
MnpAllocTxBuf (
  IN OUT MNP_DEVICE_DATA  *MnpDeviceData
  )
{
  EFI_TPL          OldTpl;
  UINT8            *TxBuf;
  EFI_STATUS       Status;
  LIST_ENTRY       *Entry;
  MNP_TX_BUF_WRAP  *TxBufWrap;

  NET_CHECK_SIGNATURE (MnpDeviceData, MNP_DEVICE_DATA_SIGNATURE);

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

  if (IsListEmpty (&MnpDeviceData->FreeTxBufList)) {
    //
    // First try to recycle some TX buffer from SNP
    //
    Status = MnpRecycleTxBuf (MnpDeviceData);
    if (EFI_ERROR (Status)) {
      TxBuf = NULL;
      goto ON_EXIT;
    }

    //
    // If still no free TX buffer, allocate more.
    //
    if (IsListEmpty (&MnpDeviceData->FreeTxBufList)) {
      if ((MnpDeviceData->TxBufCount + MNP_TX_BUFFER_INCREASEMENT) > MNP_MAX_TX_BUFFER_NUM) {
        DEBUG (
          (DEBUG_ERROR,
           "MnpAllocTxBuf: The maximum TxBuf size is reached for MNP driver instance %p.\n",
           MnpDeviceData)
          );

        TxBuf = NULL;
        goto ON_EXIT;
      }

      Status = MnpAddFreeTxBuf (MnpDeviceData, MNP_TX_BUFFER_INCREASEMENT);
      if (IsListEmpty (&MnpDeviceData->FreeTxBufList)) {
        DEBUG (
          (DEBUG_ERROR,
           "MnpAllocNbuf: Failed to add TxBuf into the FreeTxBufList, %r.\n",
           Status)
          );

        TxBuf = NULL;
        goto ON_EXIT;
      }
    }
  }

  ASSERT (!IsListEmpty (&MnpDeviceData->FreeTxBufList));
  Entry = MnpDeviceData->FreeTxBufList.ForwardLink;
  RemoveEntryList (MnpDeviceData->FreeTxBufList.ForwardLink);
  TxBufWrap        = NET_LIST_USER_STRUCT_S (Entry, MNP_TX_BUF_WRAP, WrapEntry, MNP_TX_BUF_WRAP_SIGNATURE);
  TxBufWrap->InUse = TRUE;
  TxBuf            = TxBufWrap->TxBuf;

ON_EXIT:
  gBS->RestoreTPL (OldTpl);

  return TxBuf;
}

/**
  Try to reclaim the TX buffer into the buffer pool.

  @param[in, out]  MnpDeviceData         Pointer to the mnp device context data.
  @param[in, out]  TxBuf                 Pointer to the TX buffer to free.

**/
VOID
MnpFreeTxBuf (
  IN OUT MNP_DEVICE_DATA  *MnpDeviceData,
  IN OUT UINT8            *TxBuf
  )
{
  MNP_TX_BUF_WRAP  *TxBufWrap;
  EFI_TPL          OldTpl;

  NET_CHECK_SIGNATURE (MnpDeviceData, MNP_DEVICE_DATA_SIGNATURE);

  if (TxBuf == NULL) {
    return;
  }

  TxBufWrap = NET_LIST_USER_STRUCT (TxBuf, MNP_TX_BUF_WRAP, TxBuf);
  if (TxBufWrap->Signature != MNP_TX_BUF_WRAP_SIGNATURE) {
    DEBUG (
      (DEBUG_ERROR,
       "MnpFreeTxBuf: Signature check failed in MnpFreeTxBuf.\n")
      );
    return;
  }

  if (!TxBufWrap->InUse) {
    DEBUG (
      (DEBUG_WARN,
       "MnpFreeTxBuf: Duplicated recycle report from SNP.\n")
      );
    return;
  }

  OldTpl = gBS->RaiseTPL (TPL_CALLBACK);
  InsertTailList (&MnpDeviceData->FreeTxBufList, &TxBufWrap->WrapEntry);
  TxBufWrap->InUse = FALSE;
  gBS->RestoreTPL (OldTpl);
}

/**
  Try to recycle all the transmitted buffer address from SNP.

  @param[in, out]  MnpDeviceData     Pointer to the mnp device context data.

  @retval EFI_SUCCESS             Successed to recyclethe transmitted buffer address.
  @retval Others                  Failed to recyclethe transmitted buffer address.

**/
EFI_STATUS
MnpRecycleTxBuf (
  IN OUT MNP_DEVICE_DATA  *MnpDeviceData
  )
{
  UINT8                        *TxBuf;
  EFI_SIMPLE_NETWORK_PROTOCOL  *Snp;
  EFI_STATUS                   Status;

  Snp = MnpDeviceData->Snp;
  ASSERT (Snp != NULL);

  do {
    TxBuf  = NULL;
    Status = Snp->GetStatus (Snp, NULL, (VOID **)&TxBuf);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    if (TxBuf != NULL) {
      MnpFreeTxBuf (MnpDeviceData, TxBuf);
    }
  } while (TxBuf != NULL);

  return EFI_SUCCESS;
}

/**
  Initialize the mnp device context data.

  @param[in, out]  MnpDeviceData      Pointer to the mnp device context data.
  @param[in]       ImageHandle        The driver image handle.
  @param[in]       ControllerHandle   Handle of device to bind driver to.

  @retval EFI_SUCCESS           The mnp service context is initialized.
  @retval EFI_UNSUPPORTED       ControllerHandle does not support Simple Network Protocol.
  @retval Others                Other errors as indicated.

**/
EFI_STATUS
MnpInitializeDeviceData (
  IN OUT MNP_DEVICE_DATA  *MnpDeviceData,
  IN     EFI_HANDLE       ImageHandle,
  IN     EFI_HANDLE       ControllerHandle
  )
{
  EFI_STATUS                   Status;
  EFI_SIMPLE_NETWORK_PROTOCOL  *Snp;
  EFI_SIMPLE_NETWORK_MODE      *SnpMode;

  MnpDeviceData->Signature        = MNP_DEVICE_DATA_SIGNATURE;
  MnpDeviceData->ImageHandle      = ImageHandle;
  MnpDeviceData->ControllerHandle = ControllerHandle;

  //
  // Copy the MNP Protocol interfaces from the template.
  //
  CopyMem (&MnpDeviceData->VlanConfig, &mVlanConfigProtocolTemplate, sizeof (EFI_VLAN_CONFIG_PROTOCOL));

  //
  // Open the Simple Network protocol.
  //
  Status = gBS->OpenProtocol (
                  ControllerHandle,
                  &gEfiSimpleNetworkProtocolGuid,
                  (VOID **)&Snp,
                  ImageHandle,
                  ControllerHandle,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }

  //
  // Get MTU from Snp.
  //
  SnpMode            = Snp->Mode;
  MnpDeviceData->Snp = Snp;

  //
  // Initialize the lists.
  //
  InitializeListHead (&MnpDeviceData->ServiceList);
  InitializeListHead (&MnpDeviceData->GroupAddressList);

  //
  // Get the buffer length used to allocate NET_BUF to hold data received
  // from SNP. Do this before fill the FreeNetBufQue.
  //
  //
  MnpDeviceData->BufferLength = SnpMode->MediaHeaderSize + NET_VLAN_TAG_LEN + SnpMode->MaxPacketSize + NET_ETHER_FCS_SIZE;

  //
  // Make sure the protocol headers immediately following the media header
  // 4-byte aligned, and also preserve additional space for VLAN tag
  //
  MnpDeviceData->PaddingSize = ((4 - SnpMode->MediaHeaderSize) & 0x3) + NET_VLAN_TAG_LEN;

  //
  // Initialize MAC string which will be used as VLAN configuration variable name
  //
  Status = NetLibGetMacString (ControllerHandle, ImageHandle, &MnpDeviceData->MacString);
  if (EFI_ERROR (Status)) {
    goto ERROR;
  }

  //
  // Initialize the FreeNetBufQue and pre-allocate some NET_BUFs.
  //
  NetbufQueInit (&MnpDeviceData->FreeNbufQue);
  Status = MnpAddFreeNbuf (MnpDeviceData, MNP_INIT_NET_BUFFER_NUM);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "MnpInitializeDeviceData: MnpAddFreeNbuf failed, %r.\n", Status));

    goto ERROR;
  }

  //
  // Get one NET_BUF from the FreeNbufQue for rx cache.
  //
  MnpDeviceData->RxNbufCache = MnpAllocNbuf (MnpDeviceData);
  NetbufAllocSpace (
    MnpDeviceData->RxNbufCache,
    MnpDeviceData->BufferLength,
    NET_BUF_TAIL
    );

  //
  // Allocate buffer pool for tx.
  //
  InitializeListHead (&MnpDeviceData->FreeTxBufList);
  InitializeListHead (&MnpDeviceData->AllTxBufList);
  MnpDeviceData->TxBufCount = 0;

  //
  // Create the system poll timer.
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL | EVT_TIMER,
                  TPL_CALLBACK,
                  MnpSystemPoll,
                  MnpDeviceData,
                  &MnpDeviceData->PollTimer
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "MnpInitializeDeviceData: CreateEvent for poll timer failed.\n"));

    goto ERROR;
  }

  //
  // Create the timer for packet timeout check.
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL | EVT_TIMER,
                  TPL_CALLBACK,
                  MnpCheckPacketTimeout,
                  MnpDeviceData,
                  &MnpDeviceData->TimeoutCheckTimer
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "MnpInitializeDeviceData: CreateEvent for packet timeout check failed.\n"));

    goto ERROR;
  }

  //
  // Create the timer for media detection.
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL | EVT_TIMER,
                  TPL_CALLBACK,
                  MnpCheckMediaStatus,
                  MnpDeviceData,
                  &MnpDeviceData->MediaDetectTimer
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "MnpInitializeDeviceData: CreateEvent for media detection failed.\n"));

    goto ERROR;
  }

ERROR:
  if (EFI_ERROR (Status)) {
    //
    // Free the dynamic allocated resources if necessary.
    //
    if (MnpDeviceData->MacString != NULL) {
      FreePool (MnpDeviceData->MacString);
    }

    if (MnpDeviceData->TimeoutCheckTimer != NULL) {
      gBS->CloseEvent (MnpDeviceData->TimeoutCheckTimer);
    }

    if (MnpDeviceData->MediaDetectTimer != NULL) {
      gBS->CloseEvent (MnpDeviceData->MediaDetectTimer);
    }

    if (MnpDeviceData->PollTimer != NULL) {
      gBS->CloseEvent (MnpDeviceData->PollTimer);
    }

    if (MnpDeviceData->RxNbufCache != NULL) {
      MnpFreeNbuf (MnpDeviceData, MnpDeviceData->RxNbufCache);
    }

    if (MnpDeviceData->FreeNbufQue.BufNum != 0) {
      NetbufQueFlush (&MnpDeviceData->FreeNbufQue);
    }

    //
    // Close the Simple Network Protocol.
    //
    gBS->CloseProtocol (
           ControllerHandle,
           &gEfiSimpleNetworkProtocolGuid,
           ImageHandle,
           ControllerHandle
           );
  }

  return Status;
}

/**
  Destroy the MNP device context data.

  @param[in, out]  MnpDeviceData      Pointer to the mnp device context data.
  @param[in]       ImageHandle        The driver image handle.

**/
VOID
MnpDestroyDeviceData (
  IN OUT MNP_DEVICE_DATA  *MnpDeviceData,
  IN     EFI_HANDLE       ImageHandle
  )
{
  LIST_ENTRY       *Entry;
  LIST_ENTRY       *NextEntry;
  MNP_TX_BUF_WRAP  *TxBufWrap;

  NET_CHECK_SIGNATURE (MnpDeviceData, MNP_DEVICE_DATA_SIGNATURE);

  //
  // Free Vlan Config variable name string
  //
  if (MnpDeviceData->MacString != NULL) {
    FreePool (MnpDeviceData->MacString);
  }

  //
  // The GroupAddressList must be empty.
  //
  ASSERT (IsListEmpty (&MnpDeviceData->GroupAddressList));

  //
  // Close the event.
  //
  gBS->CloseEvent (MnpDeviceData->TimeoutCheckTimer);
  gBS->CloseEvent (MnpDeviceData->MediaDetectTimer);
  gBS->CloseEvent (MnpDeviceData->PollTimer);

  //
  // Free the Tx buffer pool.
  //
  NET_LIST_FOR_EACH_SAFE (Entry, NextEntry, &MnpDeviceData->AllTxBufList) {
    TxBufWrap = NET_LIST_USER_STRUCT (Entry, MNP_TX_BUF_WRAP, AllEntry);
    RemoveEntryList (Entry);
    FreePool (TxBufWrap);
    MnpDeviceData->TxBufCount--;
  }
  ASSERT (IsListEmpty (&MnpDeviceData->AllTxBufList));
  ASSERT (MnpDeviceData->TxBufCount == 0);

  //
  // Free the RxNbufCache.
  //
  MnpFreeNbuf (MnpDeviceData, MnpDeviceData->RxNbufCache);

  //
  // Flush the FreeNbufQue.
  //
  MnpDeviceData->NbufCnt -= MnpDeviceData->FreeNbufQue.BufNum;
  NetbufQueFlush (&MnpDeviceData->FreeNbufQue);

  //
  // Close the Simple Network Protocol.
  //
  gBS->CloseProtocol (
         MnpDeviceData->ControllerHandle,
         &gEfiSimpleNetworkProtocolGuid,
         ImageHandle,
         MnpDeviceData->ControllerHandle
         );
}

/**
  Create mnp service context data.

  @param[in]       MnpDeviceData      Pointer to the mnp device context data.
  @param[in]       VlanId             The VLAN ID.
  @param[in]       Priority           The VLAN priority. If VlanId is 0,
                                      Priority is ignored.

  @return A pointer to MNP_SERVICE_DATA or NULL if failed to create MNP service context.

**/
MNP_SERVICE_DATA *
MnpCreateServiceData (
  IN MNP_DEVICE_DATA  *MnpDeviceData,
  IN UINT16           VlanId,
  IN UINT8            Priority OPTIONAL
  )
{
  EFI_HANDLE                MnpServiceHandle;
  MNP_SERVICE_DATA          *MnpServiceData;
  EFI_STATUS                Status;
  EFI_SIMPLE_NETWORK_MODE   *SnpMode;
  EFI_VLAN_CONFIG_PROTOCOL  *VlanConfig;

  //
  // Initialize the Mnp Service Data.
  //
  MnpServiceData = AllocateZeroPool (sizeof (MNP_SERVICE_DATA));
  if (MnpServiceData == NULL) {
    DEBUG ((DEBUG_ERROR, "MnpCreateServiceData: Failed to allocate memory for the new Mnp Service Data.\n"));

    return NULL;
  }

  //
  // Add to MNP service list
  //
  InsertTailList (&MnpDeviceData->ServiceList, &MnpServiceData->Link);

  MnpServiceData->Signature     = MNP_SERVICE_DATA_SIGNATURE;
  MnpServiceData->MnpDeviceData = MnpDeviceData;

  //
  // Copy the ServiceBinding structure.
  //
  CopyMem (&MnpServiceData->ServiceBinding, &mMnpServiceBindingProtocol, sizeof (EFI_SERVICE_BINDING_PROTOCOL));

  //
  // Initialize the lists.
  //
  InitializeListHead (&MnpServiceData->ChildrenList);

  SnpMode = MnpDeviceData->Snp->Mode;
  if (VlanId != 0) {
    //
    // Create VLAN child handle
    //
    MnpServiceHandle = MnpCreateVlanChild (
                         MnpDeviceData->ImageHandle,
                         MnpDeviceData->ControllerHandle,
                         VlanId,
                         &MnpServiceData->DevicePath
                         );
    if (MnpServiceHandle == NULL) {
      DEBUG ((DEBUG_ERROR, "MnpCreateServiceData: Failed to create child handle.\n"));

      return NULL;
    }

    //
    // Open VLAN Config Protocol by child
    //
    Status = gBS->OpenProtocol (
                    MnpDeviceData->ControllerHandle,
                    &gEfiVlanConfigProtocolGuid,
                    (VOID **)&VlanConfig,
                    MnpDeviceData->ImageHandle,
                    MnpServiceHandle,
                    EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER
                    );
    if (EFI_ERROR (Status)) {
      goto Exit;
    }

    //
    // Reduce MTU for VLAN device
    //
    MnpServiceData->Mtu = SnpMode->MaxPacketSize - NET_VLAN_TAG_LEN;
  } else {
    //
    // VlanId set to 0 means rx/tx untagged frame
    //
    MnpServiceHandle    = MnpDeviceData->ControllerHandle;
    MnpServiceData->Mtu = SnpMode->MaxPacketSize;
  }

  MnpServiceData->ServiceHandle = MnpServiceHandle;
  MnpServiceData->VlanId        = VlanId;
  MnpServiceData->Priority      = Priority;

  //
  // Install the MNP Service Binding Protocol
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &MnpServiceHandle,
                  &gEfiManagedNetworkServiceBindingProtocolGuid,
                  &MnpServiceData->ServiceBinding,
                  NULL
                  );

Exit:
  if (EFI_ERROR (Status)) {
    MnpDestroyServiceData (MnpServiceData);
    MnpServiceData = NULL;
  }

  return MnpServiceData;
}

/**
  Destroy the MNP service context data.

  @param[in, out]  MnpServiceData    Pointer to the mnp service context data.

  @retval EFI_SUCCESS           The mnp service context is destroyed.
  @retval Others                Errors as indicated.

**/
EFI_STATUS
MnpDestroyServiceData (
  IN OUT MNP_SERVICE_DATA  *MnpServiceData
  )
{
  EFI_STATUS  Status;

  //
  // Uninstall the MNP Service Binding Protocol
  //
  Status = gBS->UninstallMultipleProtocolInterfaces (
                  MnpServiceData->ServiceHandle,
                  &gEfiManagedNetworkServiceBindingProtocolGuid,
                  &MnpServiceData->ServiceBinding,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (MnpServiceData->VlanId != 0) {
    //
    // Close VlanConfig Protocol opened by VLAN child handle
    //
    Status = gBS->CloseProtocol (
                    MnpServiceData->MnpDeviceData->ControllerHandle,
                    &gEfiVlanConfigProtocolGuid,
                    MnpServiceData->MnpDeviceData->ImageHandle,
                    MnpServiceData->ServiceHandle
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    //
    // Uninstall Device Path Protocol to destroy the VLAN child handle
    //
    Status = gBS->UninstallMultipleProtocolInterfaces (
                    MnpServiceData->ServiceHandle,
                    &gEfiDevicePathProtocolGuid,
                    MnpServiceData->DevicePath,
                    NULL
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    if (MnpServiceData->DevicePath != NULL) {
      FreePool (MnpServiceData->DevicePath);
    }
  }

  //
  // Remove from MnpDeviceData service list
  //
  RemoveEntryList (&MnpServiceData->Link);

  FreePool (MnpServiceData);

  return Status;
}

/**
  Callback function which provided by user to remove one node in NetDestroyLinkList process.

  @param[in]    Entry           The entry to be removed.
  @param[in]    Context         Pointer to the callback context corresponds to the Context in NetDestroyLinkList.

  @retval EFI_SUCCESS           The entry has been removed successfully.
  @retval Others                Fail to remove the entry.

**/
EFI_STATUS
EFIAPI
MnpDestoryChildEntry (
  IN LIST_ENTRY  *Entry,
  IN VOID        *Context
  )
{
  MNP_INSTANCE_DATA             *Instance;
  EFI_SERVICE_BINDING_PROTOCOL  *ServiceBinding;

  ServiceBinding = (EFI_SERVICE_BINDING_PROTOCOL *)Context;
  Instance       = CR (Entry, MNP_INSTANCE_DATA, InstEntry, MNP_INSTANCE_DATA_SIGNATURE);
  return ServiceBinding->DestroyChild (ServiceBinding, Instance->Handle);
}

/**
  Destroy all child of the MNP service data.

  @param[in, out]  MnpServiceData    Pointer to the mnp service context data.

  @retval EFI_SUCCESS           All child are destroyed.
  @retval Others                Failed to destroy all child.

**/
EFI_STATUS
MnpDestroyServiceChild (
  IN OUT MNP_SERVICE_DATA  *MnpServiceData
  )
{
  LIST_ENTRY  *List;
  EFI_STATUS  Status;
  UINTN       ListLength;

  List = &MnpServiceData->ChildrenList;

  Status = NetDestroyLinkList (
             List,
             MnpDestoryChildEntry,
             &MnpServiceData->ServiceBinding,
             &ListLength
             );
  if (EFI_ERROR (Status) || (ListLength != 0)) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

/**
  Find the MNP Service Data for given VLAN ID.

  @param[in]  MnpDeviceData      Pointer to the mnp device context data.
  @param[in]  VlanId             The VLAN ID.

  @return A pointer to MNP_SERVICE_DATA or NULL if not found.

**/
MNP_SERVICE_DATA *
MnpFindServiceData (
  IN MNP_DEVICE_DATA  *MnpDeviceData,
  IN UINT16           VlanId
  )
{
  LIST_ENTRY        *Entry;
  MNP_SERVICE_DATA  *MnpServiceData;

  NET_LIST_FOR_EACH (Entry, &MnpDeviceData->ServiceList) {
    //
    // Check VLAN ID of each Mnp Service Data
    //
    MnpServiceData = MNP_SERVICE_DATA_FROM_LINK (Entry);
    if (MnpServiceData->VlanId == VlanId) {
      return MnpServiceData;
    }
  }

  return NULL;
}

/**
  Initialize the mnp instance context data.

  @param[in]       MnpServiceData   Pointer to the mnp service context data.
  @param[in, out]  Instance         Pointer to the mnp instance context data
                                    to initialize.

**/
VOID
MnpInitializeInstanceData (
  IN     MNP_SERVICE_DATA   *MnpServiceData,
  IN OUT MNP_INSTANCE_DATA  *Instance
  )
{
  NET_CHECK_SIGNATURE (MnpServiceData, MNP_SERVICE_DATA_SIGNATURE);
  ASSERT (Instance != NULL);

  //
  // Set the signature.
  //
  Instance->Signature = MNP_INSTANCE_DATA_SIGNATURE;

  //
  // Copy the MNP Protocol interfaces from the template.
  //
  CopyMem (&Instance->ManagedNetwork, &mMnpProtocolTemplate, sizeof (Instance->ManagedNetwork));

  //
  // Copy the default config data.
  //
  CopyMem (&Instance->ConfigData, &mMnpDefaultConfigData, sizeof (Instance->ConfigData));

  //
  // Initialize the lists.
  //
  InitializeListHead (&Instance->GroupCtrlBlkList);
  InitializeListHead (&Instance->RcvdPacketQueue);
  InitializeListHead (&Instance->RxDeliveredPacketQueue);

  //
  // Initialize the RxToken Map.
  //
  NetMapInit (&Instance->RxTokenMap);

  //
  // Save the MnpServiceData info.
  //
  Instance->MnpServiceData = MnpServiceData;
}

/**
  Check whether the token specified by Arg matches the token in Item.

  @param[in]  Map               Pointer to the NET_MAP.
  @param[in]  Item              Pointer to the NET_MAP_ITEM.
  @param[in]  Arg               Pointer to the Arg, it's a pointer to the token to
                                check.

  @retval EFI_SUCCESS           The token specified by Arg is different from the
                                token in Item.
  @retval EFI_ACCESS_DENIED     The token specified by Arg is the same as that in
                                Item.

**/
EFI_STATUS
EFIAPI
MnpTokenExist (
  IN NET_MAP       *Map,
  IN NET_MAP_ITEM  *Item,
  IN VOID          *Arg
  )
{
  EFI_MANAGED_NETWORK_COMPLETION_TOKEN  *Token;
  EFI_MANAGED_NETWORK_COMPLETION_TOKEN  *TokenInItem;

  Token       = (EFI_MANAGED_NETWORK_COMPLETION_TOKEN *)Arg;
  TokenInItem = (EFI_MANAGED_NETWORK_COMPLETION_TOKEN *)Item->Key;

  if ((Token == TokenInItem) || (Token->Event == TokenInItem->Event)) {
    //
    // The token is the same either the two tokens equals or the Events in
    // the two tokens are the same.
    //
    return EFI_ACCESS_DENIED;
  }

  return EFI_SUCCESS;
}

/**
  Cancel the token specified by Arg if it matches the token in Item.

  @param[in, out]  Map               Pointer to the NET_MAP.
  @param[in, out]  Item              Pointer to the NET_MAP_ITEM.
  @param[in]       Arg               Pointer to the Arg, it's a pointer to the
                                     token to cancel.

  @retval EFI_SUCCESS       The Arg is NULL, and the token in Item is cancelled,
                            or the Arg isn't NULL, and the token in Item is
                            different from the Arg.
  @retval EFI_ABORTED       The Arg isn't NULL, the token in Item mathces the
                            Arg, and the token is cancelled.

**/
EFI_STATUS
EFIAPI
MnpCancelTokens (
  IN OUT NET_MAP       *Map,
  IN OUT NET_MAP_ITEM  *Item,
  IN     VOID          *Arg
  )
{
  EFI_MANAGED_NETWORK_COMPLETION_TOKEN  *TokenToCancel;

  if ((Arg != NULL) && (Item->Key != Arg)) {
    //
    // The token in Item is not the token specified by Arg.
    //
    return EFI_SUCCESS;
  }

  TokenToCancel = (EFI_MANAGED_NETWORK_COMPLETION_TOKEN *)Item->Key;

  //
  // Remove the item from the map.
  //
  NetMapRemoveItem (Map, Item, NULL);

  //
  // Cancel this token with status set to EFI_ABORTED.
  //
  TokenToCancel->Status = EFI_ABORTED;
  gBS->SignalEvent (TokenToCancel->Event);

  if (Arg != NULL) {
    //
    // Only abort the token specified by Arg if Arg isn't NULL.
    //
    return EFI_ABORTED;
  }

  return EFI_SUCCESS;
}

/**
  Start and initialize the simple network.

  @param[in]  Snp               Pointer to the simple network protocol.

  @retval EFI_SUCCESS           The simple network protocol is started.
  @retval Others                Other errors as indicated.

**/
EFI_STATUS
MnpStartSnp (
  IN EFI_SIMPLE_NETWORK_PROTOCOL  *Snp
  )
{
  EFI_STATUS  Status;

  ASSERT (Snp != NULL);

  //
  // Start the simple network.
  //
  Status = Snp->Start (Snp);

  if (!EFI_ERROR (Status)) {
    //
    // Initialize the simple network.
    //
    Status = Snp->Initialize (Snp, 0, 0);
  }

  return Status;
}

/**
  Stop the simple network.

  @param[in]  MnpDeviceData     Pointer to the MNP_DEVICE_DATA.

  @retval EFI_SUCCESS           The simple network is stopped.
  @retval Others                Other errors as indicated.

**/
EFI_STATUS
MnpStopSnp (
  IN  MNP_DEVICE_DATA  *MnpDeviceData
  )
{
  EFI_STATUS                   Status;
  EFI_SIMPLE_NETWORK_PROTOCOL  *Snp;

  Snp = MnpDeviceData->Snp;
  ASSERT (Snp != NULL);

  //
  // Recycle all the transmit buffer from SNP.
  //
  Status = MnpRecycleTxBuf (MnpDeviceData);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Shut down the simple network.
  //
  Status = Snp->Shutdown (Snp);
  if (!EFI_ERROR (Status)) {
    //
    // Stop the simple network.
    //
    Status = Snp->Stop (Snp);
  }

  return Status;
}

/**
  Start the managed network, this function is called when one instance is configured
  or reconfigured.

  @param[in, out]  MnpServiceData       Pointer to the mnp service context data.
  @param[in]       IsConfigUpdate       The instance is reconfigured or it's the first
                                        time the instanced is configured.
  @param[in]       EnableSystemPoll     Enable the system polling or not.

  @retval EFI_SUCCESS                   The managed network is started and some
                                        configuration is updated.
  @retval Others                        Other errors as indicated.

**/
EFI_STATUS
MnpStart (
  IN OUT MNP_SERVICE_DATA  *MnpServiceData,
  IN     BOOLEAN           IsConfigUpdate,
  IN     BOOLEAN           EnableSystemPoll
  )
{
  EFI_STATUS       Status;
  EFI_TIMER_DELAY  TimerOpType;
  MNP_DEVICE_DATA  *MnpDeviceData;

  NET_CHECK_SIGNATURE (MnpServiceData, MNP_SERVICE_DATA_SIGNATURE);

  Status        = EFI_SUCCESS;
  MnpDeviceData = MnpServiceData->MnpDeviceData;

  if (!IsConfigUpdate) {
    //
    // If it's not a configuration update, increase the configured children number.
    //
    MnpDeviceData->ConfiguredChildrenNumber++;

    if (MnpDeviceData->ConfiguredChildrenNumber == 1) {
      //
      // It's the first configured child, start the simple network.
      //
      Status = MnpStartSnp (MnpDeviceData->Snp);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "MnpStart: MnpStartSnp failed, %r.\n", Status));

        goto ErrorExit;
      }

      //
      // Start the timeout timer.
      //
      Status = gBS->SetTimer (
                      MnpDeviceData->TimeoutCheckTimer,
                      TimerPeriodic,
                      MNP_TIMEOUT_CHECK_INTERVAL
                      );
      if (EFI_ERROR (Status)) {
        DEBUG (
          (DEBUG_ERROR,
           "MnpStart, gBS->SetTimer for TimeoutCheckTimer %r.\n",
           Status)
          );

        goto ErrorExit;
      }

      //
      // Start the media detection timer.
      //
      Status = gBS->SetTimer (
                      MnpDeviceData->MediaDetectTimer,
                      TimerPeriodic,
                      MNP_MEDIA_DETECT_INTERVAL
                      );
      if (EFI_ERROR (Status)) {
        DEBUG (
          (DEBUG_ERROR,
           "MnpStart, gBS->SetTimer for MediaDetectTimer %r.\n",
           Status)
          );

        goto ErrorExit;
      }
    }
  }

  if (MnpDeviceData->EnableSystemPoll ^ EnableSystemPoll) {
    //
    // The EnableSystemPoll differs with the current state, disable or enable
    // the system poll.
    //
    TimerOpType = EnableSystemPoll ? TimerPeriodic : TimerCancel;

    Status = gBS->SetTimer (MnpDeviceData->PollTimer, TimerOpType, MNP_SYS_POLL_INTERVAL);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "MnpStart: gBS->SetTimer for PollTimer failed, %r.\n", Status));

      goto ErrorExit;
    }

    MnpDeviceData->EnableSystemPoll = EnableSystemPoll;
  }

  //
  // Change the receive filters if need.
  //
  Status = MnpConfigReceiveFilters (MnpDeviceData);

ErrorExit:
  return Status;
}

/**
  Stop the managed network.

  @param[in, out]  MnpServiceData    Pointer to the mnp service context data.

  @retval EFI_SUCCESS                The managed network is stopped.
  @retval Others                     Other errors as indicated.

**/
EFI_STATUS
MnpStop (
  IN OUT MNP_SERVICE_DATA  *MnpServiceData
  )
{
  EFI_STATUS       Status;
  MNP_DEVICE_DATA  *MnpDeviceData;

  NET_CHECK_SIGNATURE (MnpServiceData, MNP_SERVICE_DATA_SIGNATURE);
  MnpDeviceData = MnpServiceData->MnpDeviceData;
  ASSERT (MnpDeviceData->ConfiguredChildrenNumber > 0);

  //
  // Configure the receive filters.
  //
  MnpConfigReceiveFilters (MnpDeviceData);

  //
  // Decrease the children number.
  //
  MnpDeviceData->ConfiguredChildrenNumber--;

  if (MnpDeviceData->ConfiguredChildrenNumber > 0) {
    //
    // If there are other configured children, return and keep the timers and
    // simple network unchanged.
    //
    return EFI_SUCCESS;
  }

  //
  // No configured children now.
  //
  if (MnpDeviceData->EnableSystemPoll) {
    //
    //  The system poll in on, cancel the poll timer.
    //
    Status                          = gBS->SetTimer (MnpDeviceData->PollTimer, TimerCancel, 0);
    MnpDeviceData->EnableSystemPoll = FALSE;
  }

  //
  // Cancel the timeout timer.
  //
  Status = gBS->SetTimer (MnpDeviceData->TimeoutCheckTimer, TimerCancel, 0);

  //
  // Cancel the media detect timer.
  //
  Status = gBS->SetTimer (MnpDeviceData->MediaDetectTimer, TimerCancel, 0);

  //
  // Stop the simple network.
  //
  Status = MnpStopSnp (MnpDeviceData);
  return Status;
}

/**
  Flush the instance's received data.

  @param[in, out]  Instance              Pointer to the mnp instance context data.

**/
VOID
MnpFlushRcvdDataQueue (
  IN OUT MNP_INSTANCE_DATA  *Instance
  )
{
  EFI_TPL          OldTpl;
  MNP_RXDATA_WRAP  *RxDataWrap;

  NET_CHECK_SIGNATURE (Instance, MNP_INSTANCE_DATA_SIGNATURE);

  OldTpl = gBS->RaiseTPL (TPL_NOTIFY);

  while (!IsListEmpty (&Instance->RcvdPacketQueue)) {
    //
    // Remove all the Wraps.
    //
    RxDataWrap = NET_LIST_HEAD (&Instance->RcvdPacketQueue, MNP_RXDATA_WRAP, WrapEntry);

    //
    // Recycle the RxDataWrap.
    //
    MnpRecycleRxData (NULL, (VOID *)RxDataWrap);
    Instance->RcvdPacketQueueSize--;
  }

  ASSERT (Instance->RcvdPacketQueueSize == 0);

  gBS->RestoreTPL (OldTpl);
}

/**
  Configure the Instance using ConfigData.

  @param[in, out]  Instance     Pointer to the mnp instance context data.
  @param[in]       ConfigData   Pointer to the configuration data used to configure
                                the instance.

  @retval EFI_SUCCESS           The Instance is configured.
  @retval EFI_UNSUPPORTED       EnableReceiveTimestamps is on and the
                                implementation doesn't support it.
  @retval Others                Other errors as indicated.

**/
EFI_STATUS
MnpConfigureInstance (
  IN OUT MNP_INSTANCE_DATA                *Instance,
  IN     EFI_MANAGED_NETWORK_CONFIG_DATA  *ConfigData OPTIONAL
  )
{
  EFI_STATUS                       Status;
  MNP_SERVICE_DATA                 *MnpServiceData;
  MNP_DEVICE_DATA                  *MnpDeviceData;
  EFI_MANAGED_NETWORK_CONFIG_DATA  *OldConfigData;
  EFI_MANAGED_NETWORK_CONFIG_DATA  *NewConfigData;
  BOOLEAN                          IsConfigUpdate;

  NET_CHECK_SIGNATURE (Instance, MNP_INSTANCE_DATA_SIGNATURE);

  if ((ConfigData != NULL) && ConfigData->EnableReceiveTimestamps) {
    //
    // Don't support timestamp.
    //
    return EFI_UNSUPPORTED;
  }

  Status = EFI_SUCCESS;

  MnpServiceData = Instance->MnpServiceData;
  MnpDeviceData  = MnpServiceData->MnpDeviceData;
  NET_CHECK_SIGNATURE (MnpDeviceData, MNP_DEVICE_DATA_SIGNATURE);

  IsConfigUpdate = (BOOLEAN)((Instance->Configured) && (ConfigData != NULL));

  OldConfigData = &Instance->ConfigData;
  NewConfigData = ConfigData;
  if (NewConfigData == NULL) {
    //
    // Restore back the default config data if a reset of this instance
    // is required.
    //
    NewConfigData = &mMnpDefaultConfigData;
  }

  //
  // Reset the instance's receive filter.
  //
  Instance->ReceiveFilter = 0;

  //
  // Clear the receive counters according to the old ConfigData.
  //
  if (OldConfigData->EnableUnicastReceive) {
    MnpDeviceData->UnicastCount--;
  }

  if (OldConfigData->EnableMulticastReceive) {
    MnpDeviceData->MulticastCount--;
  }

  if (OldConfigData->EnableBroadcastReceive) {
    MnpDeviceData->BroadcastCount--;
  }

  if (OldConfigData->EnablePromiscuousReceive) {
    MnpDeviceData->PromiscuousCount--;
  }

  //
  // Set the receive filter counters and the receive filter of the
  // instance according to the new ConfigData.
  //
  if (NewConfigData->EnableUnicastReceive) {
    MnpDeviceData->UnicastCount++;
    Instance->ReceiveFilter |= MNP_RECEIVE_UNICAST;
  }

  if (NewConfigData->EnableMulticastReceive) {
    MnpDeviceData->MulticastCount++;
  }

  if (NewConfigData->EnableBroadcastReceive) {
    MnpDeviceData->BroadcastCount++;
    Instance->ReceiveFilter |= MNP_RECEIVE_BROADCAST;
  }

  if (NewConfigData->EnablePromiscuousReceive) {
    MnpDeviceData->PromiscuousCount++;
  }

  if (OldConfigData->FlushQueuesOnReset) {
    MnpFlushRcvdDataQueue (Instance);
  }

  if (ConfigData == NULL) {
    Instance->ManagedNetwork.Cancel (&Instance->ManagedNetwork, NULL);
  }

  if (!NewConfigData->EnableMulticastReceive) {
    MnpGroupOp (Instance, FALSE, NULL, NULL);
  }

  //
  // Save the new configuration data.
  //
  CopyMem (OldConfigData, NewConfigData, sizeof (*OldConfigData));

  Instance->Configured = (BOOLEAN)(ConfigData != NULL);
  if (Instance->Configured) {
    //
    // The instance is configured, start the Mnp.
    //
    Status = MnpStart (
               MnpServiceData,
               IsConfigUpdate,
               (BOOLEAN) !NewConfigData->DisableBackgroundPolling
               );
  } else {
    //
    // The instance is changed to the unconfigured state, stop the Mnp.
    //
    Status = MnpStop (MnpServiceData);
  }

  return Status;
}

/**
  Configure the Snp receive filters according to the instances' receive filter
  settings.

  @param[in]  MnpDeviceData         Pointer to the mnp device context data.

  @retval     EFI_SUCCESS           The receive filters is configured.
  @retval     EFI_OUT_OF_RESOURCES  The receive filters can't be configured due
                                    to lack of memory resource.

**/
EFI_STATUS
MnpConfigReceiveFilters (
  IN MNP_DEVICE_DATA  *MnpDeviceData
  )
{
  EFI_STATUS                   Status;
  EFI_SIMPLE_NETWORK_PROTOCOL  *Snp;
  EFI_MAC_ADDRESS              *MCastFilter;
  UINT32                       MCastFilterCnt;
  UINT32                       EnableFilterBits;
  UINT32                       DisableFilterBits;
  BOOLEAN                      ResetMCastFilters;
  LIST_ENTRY                   *Entry;
  UINT32                       Index;
  MNP_GROUP_ADDRESS            *GroupAddress;

  NET_CHECK_SIGNATURE (MnpDeviceData, MNP_DEVICE_DATA_SIGNATURE);

  Snp = MnpDeviceData->Snp;

  //
  // Initialize the enable filter and disable filter.
  //
  EnableFilterBits  = 0;
  DisableFilterBits = Snp->Mode->ReceiveFilterMask;

  if (MnpDeviceData->UnicastCount != 0) {
    //
    // Enable unicast if any instance wants to receive unicast.
    //
    EnableFilterBits |= EFI_SIMPLE_NETWORK_RECEIVE_UNICAST;
  }

  if (MnpDeviceData->BroadcastCount != 0) {
    //
    // Enable broadcast if any instance wants to receive broadcast.
    //
    EnableFilterBits |= EFI_SIMPLE_NETWORK_RECEIVE_BROADCAST;
  }

  MCastFilter       = NULL;
  MCastFilterCnt    = 0;
  ResetMCastFilters = TRUE;

  if ((MnpDeviceData->MulticastCount != 0) && (MnpDeviceData->GroupAddressCount != 0)) {
    //
    // There are instances configured to receive multicast and already some group
    // addresses are joined.
    //

    ResetMCastFilters = FALSE;

    if (MnpDeviceData->GroupAddressCount <= Snp->Mode->MaxMCastFilterCount) {
      //
      // The joind group address is less than simple network's maximum count.
      // Just configure the snp to do the multicast filtering.
      //

      EnableFilterBits |= EFI_SIMPLE_NETWORK_RECEIVE_MULTICAST;

      //
      // Allocate pool for the multicast addresses.
      //
      MCastFilterCnt = MnpDeviceData->GroupAddressCount;
      MCastFilter    = AllocatePool (sizeof (EFI_MAC_ADDRESS) * MCastFilterCnt);
      if (MCastFilter == NULL) {
        DEBUG ((DEBUG_ERROR, "MnpConfigReceiveFilters: Failed to allocate memory resource for MCastFilter.\n"));

        return EFI_OUT_OF_RESOURCES;
      }

      //
      // Fill the multicast HW address buffer.
      //
      Index = 0;
      NET_LIST_FOR_EACH (Entry, &MnpDeviceData->GroupAddressList) {
        GroupAddress = NET_LIST_USER_STRUCT (Entry, MNP_GROUP_ADDRESS, AddrEntry);
        CopyMem (MCastFilter + Index, &GroupAddress->Address, sizeof (*(MCastFilter + Index)));
        Index++;

        ASSERT (Index <= MCastFilterCnt);
      }
    } else {
      //
      // The maximum multicast is reached, set the filter to be promiscuous
      // multicast.
      //

      if ((Snp->Mode->ReceiveFilterMask & EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS_MULTICAST) != 0) {
        EnableFilterBits |= EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS_MULTICAST;
      } else {
        //
        // Either MULTICAST or PROMISCUOUS_MULTICAST is not supported by Snp,
        // set the NIC to be promiscuous although this will tremendously degrade
        // the performance.
        //
        EnableFilterBits |= EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS;
      }
    }
  }

  if (MnpDeviceData->PromiscuousCount != 0) {
    //
    // Enable promiscuous if any instance wants to receive promiscuous.
    //
    EnableFilterBits |= EFI_SIMPLE_NETWORK_RECEIVE_PROMISCUOUS;
  }

  //
  // Set the disable filter.
  //
  DisableFilterBits ^= EnableFilterBits;

  //
  // Configure the receive filters of SNP.
  //
  Status = Snp->ReceiveFilters (
                  Snp,
                  EnableFilterBits,
                  DisableFilterBits,
                  ResetMCastFilters,
                  MCastFilterCnt,
                  MCastFilter
                  );
  DEBUG_CODE_BEGIN ();
  if (EFI_ERROR (Status)) {
    DEBUG (
      (DEBUG_ERROR,
       "MnpConfigReceiveFilters: Snp->ReceiveFilters failed, %r.\n",
       Status)
      );
  }

  DEBUG_CODE_END ();

  if (MCastFilter != NULL) {
    //
    // Free the buffer used to hold the group addresses.
    //
    FreePool (MCastFilter);
  }

  return Status;
}

/**
  Add a group address control block which controls the MacAddress for
  this instance.

  @param[in, out]  Instance        Pointer to the mnp instance context data.
  @param[in, out]  CtrlBlk         Pointer to the group address control block.
  @param[in, out]  GroupAddress    Pointer to the group address.
  @param[in]       MacAddress      Pointer to the mac address.
  @param[in]       HwAddressSize   The hardware address size.

  @retval EFI_SUCCESS              The group address control block is added.
  @retval EFI_OUT_OF_RESOURCES     Failed due to lack of memory resources.

**/
EFI_STATUS
MnpGroupOpAddCtrlBlk (
  IN OUT MNP_INSTANCE_DATA        *Instance,
  IN OUT MNP_GROUP_CONTROL_BLOCK  *CtrlBlk,
  IN OUT MNP_GROUP_ADDRESS        *GroupAddress OPTIONAL,
  IN     EFI_MAC_ADDRESS          *MacAddress,
  IN     UINT32                   HwAddressSize
  )
{
  MNP_DEVICE_DATA  *MnpDeviceData;

  NET_CHECK_SIGNATURE (Instance, MNP_INSTANCE_DATA_SIGNATURE);

  MnpDeviceData = Instance->MnpServiceData->MnpDeviceData;
  NET_CHECK_SIGNATURE (MnpDeviceData, MNP_DEVICE_DATA_SIGNATURE);

  if (GroupAddress == NULL) {
    ASSERT (MacAddress != NULL);

    //
    // Allocate a new GroupAddress to be added into MNP's GroupAddressList.
    //
    GroupAddress = AllocatePool (sizeof (MNP_GROUP_ADDRESS));
    if (GroupAddress == NULL) {
      DEBUG ((DEBUG_ERROR, "MnpGroupOpFormCtrlBlk: Failed to allocate memory resource.\n"));

      return EFI_OUT_OF_RESOURCES;
    }

    CopyMem (&GroupAddress->Address, MacAddress, sizeof (GroupAddress->Address));
    GroupAddress->RefCnt = 0;
    InsertTailList (
      &MnpDeviceData->GroupAddressList,
      &GroupAddress->AddrEntry
      );
    MnpDeviceData->GroupAddressCount++;
  }

  //
  // Increase the RefCnt.
  //
  GroupAddress->RefCnt++;

  //
  // Add the CtrlBlk into the instance's GroupCtrlBlkList.
  //
  CtrlBlk->GroupAddress = GroupAddress;
  InsertTailList (&Instance->GroupCtrlBlkList, &CtrlBlk->CtrlBlkEntry);

  return EFI_SUCCESS;
}

/**
  Delete a group control block from the instance. If the controlled group address's
  reference count reaches zero, the group address is removed too.

  @param[in]  Instance              Pointer to the instance context data.
  @param[in]  CtrlBlk               Pointer to the group control block to delete.

  @return The group address controlled by the control block is no longer used or not.

**/
BOOLEAN
MnpGroupOpDelCtrlBlk (
  IN MNP_INSTANCE_DATA        *Instance,
  IN MNP_GROUP_CONTROL_BLOCK  *CtrlBlk
  )
{
  MNP_DEVICE_DATA    *MnpDeviceData;
  MNP_GROUP_ADDRESS  *GroupAddress;

  NET_CHECK_SIGNATURE (Instance, MNP_INSTANCE_DATA_SIGNATURE);

  MnpDeviceData = Instance->MnpServiceData->MnpDeviceData;
  NET_CHECK_SIGNATURE (MnpDeviceData, MNP_DEVICE_DATA_SIGNATURE);

  //
  // Remove and free the CtrlBlk.
  //
  GroupAddress = CtrlBlk->GroupAddress;
  RemoveEntryList (&CtrlBlk->CtrlBlkEntry);
  FreePool (CtrlBlk);

  ASSERT (GroupAddress->RefCnt > 0);

  //
  // Count down the RefCnt.
  //
  GroupAddress->RefCnt--;

  if (GroupAddress->RefCnt == 0) {
    //
    // Free this GroupAddress entry if no instance uses it.
    //
    MnpDeviceData->GroupAddressCount--;
    RemoveEntryList (&GroupAddress->AddrEntry);
    FreePool (GroupAddress);

    return TRUE;
  }

  return FALSE;
}

/**
  Do the group operations for this instance.

  @param[in, out]  Instance        Pointer to the instance context data.
  @param[in]       JoinFlag        Set to TRUE to join a group. Set to TRUE to
                                   leave a group/groups.
  @param[in]       MacAddress      Pointer to the group address to join or leave.
  @param[in]       CtrlBlk         Pointer to the group control block if JoinFlag
                                   is FALSE.

  @retval EFI_SUCCESS              The group operation finished.
  @retval EFI_OUT_OF_RESOURCES     Failed due to lack of memory resources.
  @retval Others                   Other errors as indicated.

**/
EFI_STATUS
MnpGroupOp (
  IN OUT MNP_INSTANCE_DATA        *Instance,
  IN     BOOLEAN                  JoinFlag,
  IN     EFI_MAC_ADDRESS          *MacAddress OPTIONAL,
  IN     MNP_GROUP_CONTROL_BLOCK  *CtrlBlk OPTIONAL
  )
{
  MNP_DEVICE_DATA          *MnpDeviceData;
  LIST_ENTRY               *Entry;
  LIST_ENTRY               *NextEntry;
  MNP_GROUP_ADDRESS        *GroupAddress;
  EFI_SIMPLE_NETWORK_MODE  *SnpMode;
  MNP_GROUP_CONTROL_BLOCK  *NewCtrlBlk;
  EFI_STATUS               Status;
  BOOLEAN                  AddressExist;
  BOOLEAN                  NeedUpdate;

  NET_CHECK_SIGNATURE (Instance, MNP_INSTANCE_DATA_SIGNATURE);

  MnpDeviceData = Instance->MnpServiceData->MnpDeviceData;
  SnpMode       = MnpDeviceData->Snp->Mode;

  if (JoinFlag) {
    //
    // A new group address is to be added.
    //
    GroupAddress = NULL;
    AddressExist = FALSE;

    //
    // Allocate memory for the control block.
    //
    NewCtrlBlk = AllocatePool (sizeof (MNP_GROUP_CONTROL_BLOCK));
    if (NewCtrlBlk == NULL) {
      DEBUG ((DEBUG_ERROR, "MnpGroupOp: Failed to allocate memory resource.\n"));

      return EFI_OUT_OF_RESOURCES;
    }

    NET_LIST_FOR_EACH (Entry, &MnpDeviceData->GroupAddressList) {
      //
      // Check whether the MacAddress is already joined by other instances.
      //
      GroupAddress = NET_LIST_USER_STRUCT (Entry, MNP_GROUP_ADDRESS, AddrEntry);
      if (CompareMem (MacAddress, &GroupAddress->Address, SnpMode->HwAddressSize) == 0) {
        AddressExist = TRUE;
        break;
      }
    }

    if (!AddressExist) {
      GroupAddress = NULL;
    }

    //
    // Add the GroupAddress for this instance.
    //
    Status = MnpGroupOpAddCtrlBlk (
               Instance,
               NewCtrlBlk,
               GroupAddress,
               MacAddress,
               SnpMode->HwAddressSize
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    NeedUpdate = TRUE;
  } else {
    if (MacAddress != NULL) {
      ASSERT (CtrlBlk != NULL);

      //
      // Leave the specific multicast mac address.
      //
      NeedUpdate = MnpGroupOpDelCtrlBlk (Instance, CtrlBlk);
    } else {
      //
      // Leave all multicast mac addresses.
      //
      NeedUpdate = FALSE;

      NET_LIST_FOR_EACH_SAFE (Entry, NextEntry, &Instance->GroupCtrlBlkList) {
        NewCtrlBlk = NET_LIST_USER_STRUCT (
                       Entry,
                       MNP_GROUP_CONTROL_BLOCK,
                       CtrlBlkEntry
                       );
        //
        // Update is required if the group address left is no longer used
        // by other instances.
        //
        NeedUpdate = MnpGroupOpDelCtrlBlk (Instance, NewCtrlBlk);
      }
    }
  }

  Status = EFI_SUCCESS;

  if (NeedUpdate) {
    //
    // Reconfigure the receive filters if necessary.
    //
    Status = MnpConfigReceiveFilters (MnpDeviceData);
  }

  return Status;
}
