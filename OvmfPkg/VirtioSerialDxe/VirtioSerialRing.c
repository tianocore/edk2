/** @file

  Driver for virtio-serial devices.

  Helper functions to manage virtio rings.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/VirtioLib.h>

#include "VirtioSerial.h"

STATIC
VOID *
BufferPtr (
  IN VIRTIO_SERIAL_RING  *Ring,
  IN UINT32              BufferNr
  )
{
  return Ring->Buffers + Ring->BufferSize * BufferNr;
}

STATIC
EFI_PHYSICAL_ADDRESS
BufferAddr (
  IN VIRTIO_SERIAL_RING  *Ring,
  IN UINT32              BufferNr
  )
{
  return Ring->DeviceAddress + Ring->BufferSize * BufferNr;
}

STATIC
UINT32
BufferNext (
  IN VIRTIO_SERIAL_RING  *Ring
  )
{
  return Ring->Indices.NextDescIdx % Ring->Ring.QueueSize;
}

EFI_STATUS
EFIAPI
VirtioSerialInitRing (
  IN OUT VIRTIO_SERIAL_DEV  *Dev,
  IN     UINT16             Index,
  IN     UINT32             BufferSize
  )
{
  VIRTIO_SERIAL_RING  *Ring = Dev->Rings + Index;
  EFI_STATUS          Status;
  UINT16              QueueSize;
  UINT64              RingBaseShift;

  //
  // step 4b -- allocate request virtqueue
  //
  Status = Dev->VirtIo->SetQueueSel (Dev->VirtIo, Index);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }

  Status = Dev->VirtIo->GetQueueNumMax (Dev->VirtIo, &QueueSize);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }

  //
  // VirtioSerial uses one descriptor
  //
  if (QueueSize < 1) {
    Status = EFI_UNSUPPORTED;
    goto Failed;
  }

  Status = VirtioRingInit (Dev->VirtIo, QueueSize, &Ring->Ring);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }

  //
  // If anything fails from here on, we must release the ring resources.
  //
  Status = VirtioRingMap (
             Dev->VirtIo,
             &Ring->Ring,
             &RingBaseShift,
             &Ring->RingMap
             );
  if (EFI_ERROR (Status)) {
    goto ReleaseQueue;
  }

  //
  // Additional steps for MMIO: align the queue appropriately, and set the
  // size. If anything fails from here on, we must unmap the ring resources.
  //
  Status = Dev->VirtIo->SetQueueNum (Dev->VirtIo, QueueSize);
  if (EFI_ERROR (Status)) {
    goto UnmapQueue;
  }

  Status = Dev->VirtIo->SetQueueAlign (Dev->VirtIo, EFI_PAGE_SIZE);
  if (EFI_ERROR (Status)) {
    goto UnmapQueue;
  }

  //
  // step 4c -- Report GPFN (guest-physical frame number) of queue.
  //
  Status = Dev->VirtIo->SetQueueAddress (
                          Dev->VirtIo,
                          &Ring->Ring,
                          RingBaseShift
                          );
  if (EFI_ERROR (Status)) {
    goto UnmapQueue;
  }

  Ring->BufferCount = QueueSize;
  Ring->BufferSize  = BufferSize;
  Ring->BufferPages = EFI_SIZE_TO_PAGES (Ring->BufferCount * Ring->BufferSize);

  Status = Dev->VirtIo->AllocateSharedPages (Dev->VirtIo, Ring->BufferPages, (VOID **)&Ring->Buffers);
  if (EFI_ERROR (Status)) {
    goto UnmapQueue;
  }

  Status = VirtioMapAllBytesInSharedBuffer (
             Dev->VirtIo,
             VirtioOperationBusMasterCommonBuffer,
             Ring->Buffers,
             EFI_PAGES_TO_SIZE (Ring->BufferPages),
             &Ring->DeviceAddress,
             &Ring->BufferMap
             );
  if (EFI_ERROR (Status)) {
    goto ReleasePages;
  }

  VirtioPrepare (&Ring->Ring, &Ring->Indices);
  Ring->Ready = TRUE;

  return EFI_SUCCESS;

ReleasePages:
  Dev->VirtIo->FreeSharedPages (
                 Dev->VirtIo,
                 Ring->BufferPages,
                 Ring->Buffers
                 );
  Ring->Buffers = NULL;

UnmapQueue:
  Dev->VirtIo->UnmapSharedBuffer (Dev->VirtIo, Ring->RingMap);
  Ring->RingMap = NULL;

ReleaseQueue:
  VirtioRingUninit (Dev->VirtIo, &Ring->Ring);

Failed:
  return Status;
}

VOID
EFIAPI
VirtioSerialUninitRing (
  IN OUT VIRTIO_SERIAL_DEV  *Dev,
  IN     UINT16             Index
  )
{
  VIRTIO_SERIAL_RING  *Ring = Dev->Rings + Index;

  if (Ring->BufferMap) {
    Dev->VirtIo->UnmapSharedBuffer (Dev->VirtIo, Ring->BufferMap);
    Ring->BufferMap = NULL;
  }

  if (Ring->Buffers) {
    Dev->VirtIo->FreeSharedPages (
                   Dev->VirtIo,
                   Ring->BufferPages,
                   Ring->Buffers
                   );
    Ring->Buffers = NULL;
  }

  if (!Ring->RingMap) {
    Dev->VirtIo->UnmapSharedBuffer (Dev->VirtIo, Ring->RingMap);
    Ring->RingMap = NULL;
  }

  if (Ring->Ring.Base) {
    VirtioRingUninit (Dev->VirtIo, &Ring->Ring);
  }

  ZeroMem (Ring, sizeof (*Ring));
}

VOID
EFIAPI
VirtioSerialRingFillRx (
  IN OUT VIRTIO_SERIAL_DEV  *Dev,
  IN     UINT16             Index
  )
{
  VIRTIO_SERIAL_RING  *Ring = Dev->Rings + Index;
  UINT32              BufferNr;

  for (BufferNr = 0; BufferNr < Ring->BufferCount; BufferNr++) {
    VirtioSerialRingSendBuffer (Dev, Index, NULL, Ring->BufferSize, FALSE);
  }

  Dev->VirtIo->SetQueueNotify (Dev->VirtIo, Index);
}

VOID
EFIAPI
VirtioSerialRingClearTx (
  IN OUT VIRTIO_SERIAL_DEV  *Dev,
  IN     UINT16             Index
  )
{
  while (VirtioSerialRingGetBuffer (Dev, Index, NULL, NULL)) {
    /* nothing */ }
}

EFI_STATUS
EFIAPI
VirtioSerialRingSendBuffer (
  IN OUT VIRTIO_SERIAL_DEV  *Dev,
  IN     UINT16             Index,
  IN     VOID               *Data,
  IN     UINT32             DataSize,
  IN     BOOLEAN            Notify
  )
{
  VIRTIO_SERIAL_RING  *Ring    = Dev->Rings + Index;
  UINT32              BufferNr = BufferNext (Ring);
  UINT16              Idx      = *Ring->Ring.Avail.Idx;
  UINT16              Flags    = 0;

  ASSERT (DataSize <= Ring->BufferSize);

  if (Data) {
    /* driver -> device */
    CopyMem (BufferPtr (Ring, BufferNr), Data, DataSize);
  } else {
    /* device -> driver */
    Flags |= VRING_DESC_F_WRITE;
  }

  VirtioAppendDesc (
    &Ring->Ring,
    BufferAddr (Ring, BufferNr),
    DataSize,
    Flags,
    &Ring->Indices
    );

  Ring->Ring.Avail.Ring[Idx % Ring->Ring.QueueSize] =
    Ring->Indices.HeadDescIdx % Ring->Ring.QueueSize;
  Ring->Indices.HeadDescIdx = Ring->Indices.NextDescIdx;
  Idx++;

  MemoryFence ();
  *Ring->Ring.Avail.Idx = Idx;
  MemoryFence ();

  if (Notify) {
    Dev->VirtIo->SetQueueNotify (Dev->VirtIo, Index);
  }

  return EFI_SUCCESS;
}

BOOLEAN
EFIAPI
VirtioSerialRingHasBuffer (
  IN OUT VIRTIO_SERIAL_DEV  *Dev,
  IN     UINT16             Index
  )
{
  VIRTIO_SERIAL_RING  *Ring   = Dev->Rings + Index;
  UINT16              UsedIdx = *Ring->Ring.Used.Idx;

  if (!Ring->Ready) {
    return FALSE;
  }

  if (Ring->LastUsedIdx == UsedIdx) {
    return FALSE;
  }

  return TRUE;
}

BOOLEAN
EFIAPI
VirtioSerialRingGetBuffer (
  IN OUT VIRTIO_SERIAL_DEV  *Dev,
  IN     UINT16             Index,
  OUT    VOID               *Data,
  OUT    UINT32             *DataSize
  )
{
  VIRTIO_SERIAL_RING        *Ring   = Dev->Rings + Index;
  UINT16                    UsedIdx = *Ring->Ring.Used.Idx;
  volatile VRING_USED_ELEM  *UsedElem;

  if (!Ring->Ready) {
    return FALSE;
  }

  if (Ring->LastUsedIdx == UsedIdx) {
    return FALSE;
  }

  UsedElem = Ring->Ring.Used.UsedElem + (Ring->LastUsedIdx % Ring->Ring.QueueSize);

  if (UsedElem->Len > Ring->BufferSize) {
    DEBUG ((DEBUG_ERROR, "%a:%d: %d: invalid length\n", __func__, __LINE__, Index));
    UsedElem->Len = 0;
  }

  if (Data && DataSize) {
    CopyMem (Data, BufferPtr (Ring, UsedElem->Id), UsedElem->Len);
    *DataSize = UsedElem->Len;
  }

  if (Index % 2 == 0) {
    /* RX - re-queue buffer */
    VirtioSerialRingSendBuffer (Dev, Index, NULL, Ring->BufferSize, FALSE);
  }

  Ring->LastUsedIdx++;
  return TRUE;
}
