/** @file

  Helper functions used by at least two Simple Network Protocol methods.

  Copyright (C) 2013, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/MemoryAllocationLib.h>

#include "VirtioNet.h"

//
// The user structure for the ordered collection that will track the mapping
// info of the packets queued in TxRing
//
typedef struct {
  VOID                    *Buffer;
  EFI_PHYSICAL_ADDRESS    DeviceAddress; // lookup key for reverse mapping
  VOID                    *BufMap;
} TX_BUF_MAP_INFO;

/**
  Release RX and TX resources on the boundary of the
  EfiSimpleNetworkInitialized state.

  These functions contribute to rolling back a partial, failed initialization
  of the virtio-net SNP driver instance, or to shutting down a fully
  initialized, running instance.

  They are only callable by the VirtioNetInitialize() and the
  VirtioNetShutdown() SNP methods. See the state diagram in "VirtioNet.h".

  @param[in,out] Dev  The VNET_DEV driver instance being shut down, or whose
                      partial, failed initialization is being rolled back.
*/
VOID
EFIAPI
VirtioNetShutdownRx (
  IN OUT VNET_DEV  *Dev
  )
{
  Dev->VirtIo->UnmapSharedBuffer (Dev->VirtIo, Dev->RxBufMap);
  Dev->VirtIo->FreeSharedPages (
                 Dev->VirtIo,
                 Dev->RxBufNrPages,
                 Dev->RxBuf
                 );
}

VOID
EFIAPI
VirtioNetShutdownTx (
  IN OUT VNET_DEV  *Dev
  )
{
  ORDERED_COLLECTION_ENTRY  *Entry, *Entry2;
  TX_BUF_MAP_INFO           *TxBufMapInfo;
  VOID                      *UserStruct;

  Dev->VirtIo->UnmapSharedBuffer (Dev->VirtIo, Dev->TxSharedReqMap);
  Dev->VirtIo->FreeSharedPages (
                 Dev->VirtIo,
                 EFI_SIZE_TO_PAGES (sizeof *(Dev->TxSharedReq)),
                 Dev->TxSharedReq
                 );

  for (Entry = OrderedCollectionMin (Dev->TxBufCollection);
       Entry != NULL;
       Entry = Entry2)
  {
    Entry2 = OrderedCollectionNext (Entry);
    OrderedCollectionDelete (Dev->TxBufCollection, Entry, &UserStruct);
    TxBufMapInfo = UserStruct;
    Dev->VirtIo->UnmapSharedBuffer (Dev->VirtIo, TxBufMapInfo->BufMap);
    FreePool (TxBufMapInfo);
  }

  OrderedCollectionUninit (Dev->TxBufCollection);

  FreePool (Dev->TxFreeStack);
}

/**
  Release TX and RX VRING resources.

  @param[in,out] Dev       The VNET_DEV driver instance which was using
                           the ring.
  @param[in,out] Ring      The virtio ring to clean up.
  @param[in]     RingMap   A token return from the VirtioRingMap()
*/
VOID
EFIAPI
VirtioNetUninitRing (
  IN OUT VNET_DEV  *Dev,
  IN OUT VRING     *Ring,
  IN     VOID      *RingMap
  )
{
  Dev->VirtIo->UnmapSharedBuffer (Dev->VirtIo, RingMap);
  VirtioRingUninit (Dev->VirtIo, Ring);
}

/**
  Map Caller-supplied TxBuf buffer to the device-mapped address

  @param[in]    Dev               The VNET_DEV driver instance which wants to
                                  map the Tx packet.
  @param[in]    Buffer            The system physical address of TxBuf
  @param[in]    NumberOfBytes     Number of bytes to map
  @param[out]   DeviceAddress     The resulting device address for the bus
                                  master access.

  @retval EFI_OUT_OF_RESOURCES    The request could not be completed due to
                                  a lack of resources.
  @return                         Status codes from
                                  VirtioMapAllBytesInSharedBuffer()
  @retval EFI_SUCCESS             Caller-supplied buffer is successfully mapped.
*/
EFI_STATUS
EFIAPI
VirtioNetMapTxBuf (
  IN  VNET_DEV              *Dev,
  IN  VOID                  *Buffer,
  IN  UINTN                 NumberOfBytes,
  OUT EFI_PHYSICAL_ADDRESS  *DeviceAddress
  )
{
  EFI_STATUS            Status;
  TX_BUF_MAP_INFO       *TxBufMapInfo;
  EFI_PHYSICAL_ADDRESS  Address;
  VOID                  *Mapping;

  TxBufMapInfo = AllocatePool (sizeof (*TxBufMapInfo));
  if (TxBufMapInfo == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = VirtioMapAllBytesInSharedBuffer (
             Dev->VirtIo,
             VirtioOperationBusMasterRead,
             Buffer,
             NumberOfBytes,
             &Address,
             &Mapping
             );
  if (EFI_ERROR (Status)) {
    goto FreeTxBufMapInfo;
  }

  TxBufMapInfo->Buffer        = Buffer;
  TxBufMapInfo->DeviceAddress = Address;
  TxBufMapInfo->BufMap        = Mapping;

  Status = OrderedCollectionInsert (
             Dev->TxBufCollection,
             NULL,
             TxBufMapInfo
             );
  switch (Status) {
    case EFI_OUT_OF_RESOURCES:
      goto UnmapTxBuf;
    case EFI_ALREADY_STARTED:
      //
      // This should never happen: it implies
      //
      // - an identity-mapping VIRTIO_DEVICE_PROTOCOL.MapSharedBuffer()
      //   implementation -- which is fine,
      //
      // - and an SNP client that queues multiple instances of the exact same
      //   buffer address with SNP.Transmit() -- which is undefined behavior,
      //   based on the TxBuf language in UEFI-2.7,
      //   EFI_SIMPLE_NETWORK.GetStatus().
      //
      ASSERT (FALSE);
      Status = EFI_INVALID_PARAMETER;
      goto UnmapTxBuf;
    default:
      ASSERT_EFI_ERROR (Status);
      break;
  }

  *DeviceAddress = Address;
  return EFI_SUCCESS;

UnmapTxBuf:
  Dev->VirtIo->UnmapSharedBuffer (Dev->VirtIo, Mapping);

FreeTxBufMapInfo:
  FreePool (TxBufMapInfo);
  return Status;
}

/**
  Unmap (aka reverse mapping) device mapped TxBuf buffer to the system
  physical address

  @param[in]    Dev               The VNET_DEV driver instance which wants to
                                  reverse- and unmap the Tx packet.
  @param[out]   Buffer            The system physical address of TxBuf
  @param[in]    DeviceAddress     The device address for the TxBuf

  @retval EFI_INVALID_PARAMETER   The DeviceAddress is not mapped
  @retval EFI_SUCCESS             The TxBuf at DeviceAddress has been unmapped,
                                  and Buffer has been set to TxBuf's system
                                  physical address.

*/
EFI_STATUS
EFIAPI
VirtioNetUnmapTxBuf (
  IN  VNET_DEV              *Dev,
  OUT VOID                  **Buffer,
  IN  EFI_PHYSICAL_ADDRESS  DeviceAddress
  )
{
  ORDERED_COLLECTION_ENTRY  *Entry;
  TX_BUF_MAP_INFO           *TxBufMapInfo;
  VOID                      *UserStruct;

  Entry = OrderedCollectionFind (Dev->TxBufCollection, &DeviceAddress);
  if (Entry == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  OrderedCollectionDelete (Dev->TxBufCollection, Entry, &UserStruct);

  TxBufMapInfo = UserStruct;

  *Buffer = TxBufMapInfo->Buffer;
  Dev->VirtIo->UnmapSharedBuffer (Dev->VirtIo, TxBufMapInfo->BufMap);
  FreePool (TxBufMapInfo);

  return EFI_SUCCESS;
}

/**
  Comparator function for two TX_BUF_MAP_INFO objects.

  @param[in] UserStruct1  Pointer to the first TX_BUF_MAP_INFO object.

  @param[in] UserStruct2  Pointer to the second TX_BUF_MAP_INFO object.

  @retval <0  If UserStruct1 compares less than UserStruct2.

  @retval  0  If UserStruct1 compares equal to UserStruct2.

  @retval >0  If UserStruct1 compares greater than UserStruct2.
*/
INTN
EFIAPI
VirtioNetTxBufMapInfoCompare (
  IN CONST VOID  *UserStruct1,
  IN CONST VOID  *UserStruct2
  )
{
  CONST TX_BUF_MAP_INFO  *MapInfo1;
  CONST TX_BUF_MAP_INFO  *MapInfo2;

  MapInfo1 = UserStruct1;
  MapInfo2 = UserStruct2;

  return MapInfo1->DeviceAddress < MapInfo2->DeviceAddress ? -1 :
         MapInfo1->DeviceAddress > MapInfo2->DeviceAddress ?  1 :
         0;
}

/**
  Compare a standalone DeviceAddress against a TX_BUF_MAP_INFO object
  containing an embedded DeviceAddress.

  @param[in] StandaloneKey  Pointer to DeviceAddress, which has type
                            EFI_PHYSICAL_ADDRESS.

  @param[in] UserStruct     Pointer to the TX_BUF_MAP_INFO object with the
                            embedded DeviceAddress.

  @retval <0  If StandaloneKey compares less than UserStruct's key.

  @retval  0  If StandaloneKey compares equal to UserStruct's key.

  @retval >0  If StandaloneKey compares greater than UserStruct's key.
**/
INTN
EFIAPI
VirtioNetTxBufDeviceAddressCompare (
  IN CONST VOID  *StandaloneKey,
  IN CONST VOID  *UserStruct
  )
{
  CONST EFI_PHYSICAL_ADDRESS  *DeviceAddress;
  CONST TX_BUF_MAP_INFO       *MapInfo;

  DeviceAddress = StandaloneKey;
  MapInfo       = UserStruct;

  return *DeviceAddress < MapInfo->DeviceAddress ? -1 :
         *DeviceAddress > MapInfo->DeviceAddress ?  1 :
         0;
}
