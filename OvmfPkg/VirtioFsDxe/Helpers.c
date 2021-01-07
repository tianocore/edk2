/** @file
  Initialization and helper routines for the Virtio Filesystem device.

  Copyright (C) 2020, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/BaseLib.h>             // StrLen()
#include <Library/BaseMemoryLib.h>       // CopyMem()
#include <Library/MemoryAllocationLib.h> // AllocatePool()
#include <Library/TimeBaseLib.h>         // EpochToEfiTime()
#include <Library/VirtioLib.h>           // Virtio10WriteFeatures()

#include "VirtioFsDxe.h"

/**
  Read the Virtio Filesystem device configuration structure in full.

  @param[in] Virtio   The Virtio protocol underlying the VIRTIO_FS object.

  @param[out] Config  The fully populated VIRTIO_FS_CONFIG structure.

  @retval EFI_SUCCESS  Config has been filled in.

  @return              Error codes propagated from Virtio->ReadDevice(). The
                       contents of Config are indeterminate.
**/
STATIC
EFI_STATUS
VirtioFsReadConfig (
  IN  VIRTIO_DEVICE_PROTOCOL *Virtio,
  OUT VIRTIO_FS_CONFIG       *Config
  )
{
  UINTN      Idx;
  EFI_STATUS Status;

  for (Idx = 0; Idx < VIRTIO_FS_TAG_BYTES; Idx++) {
    Status = Virtio->ReadDevice (
                       Virtio,                                 // This
                       OFFSET_OF (VIRTIO_FS_CONFIG, Tag[Idx]), // FieldOffset
                       sizeof Config->Tag[Idx],                // FieldSize
                       sizeof Config->Tag[Idx],                // BufferSize
                       &Config->Tag[Idx]                       // Buffer
                       );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  Status = Virtio->ReadDevice (
                     Virtio,                                     // This
                     OFFSET_OF (VIRTIO_FS_CONFIG, NumReqQueues), // FieldOffset
                     sizeof Config->NumReqQueues,                // FieldSize
                     sizeof Config->NumReqQueues,                // BufferSize
                     &Config->NumReqQueues                       // Buffer
                     );
  return Status;
}

/**
  Configure the Virtio Filesystem device underlying VirtioFs.

  @param[in,out] VirtioFs  The VIRTIO_FS object for which Virtio communication
                           should be set up. On input, the caller is
                           responsible for VirtioFs->Virtio having been
                           initialized. On output, synchronous Virtio
                           Filesystem commands (primitives) may be submitted to
                           the device.

  @retval EFI_SUCCESS      Virtio machinery has been set up.

  @retval EFI_UNSUPPORTED  The host-side configuration of the Virtio Filesystem
                           is not supported by this driver.

  @return                  Error codes from underlying functions.
**/
EFI_STATUS
VirtioFsInit (
  IN OUT VIRTIO_FS *VirtioFs
  )
{
  UINT8            NextDevStat;
  EFI_STATUS       Status;
  UINT64           Features;
  VIRTIO_FS_CONFIG Config;
  UINTN            Idx;
  UINT64           RingBaseShift;

  //
  // Execute virtio-v1.1-cs01-87fa6b5d8155, 3.1.1 Driver Requirements: Device
  // Initialization.
  //
  // 1. Reset the device.
  //
  NextDevStat = 0;
  Status = VirtioFs->Virtio->SetDeviceStatus (VirtioFs->Virtio, NextDevStat);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }

  //
  // 2. Set the ACKNOWLEDGE status bit [...]
  //
  NextDevStat |= VSTAT_ACK;
  Status = VirtioFs->Virtio->SetDeviceStatus (VirtioFs->Virtio, NextDevStat);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }

  //
  // 3. Set the DRIVER status bit [...]
  //
  NextDevStat |= VSTAT_DRIVER;
  Status = VirtioFs->Virtio->SetDeviceStatus (VirtioFs->Virtio, NextDevStat);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }

  //
  // 4. Read device feature bits...
  //
  Status = VirtioFs->Virtio->GetDeviceFeatures (VirtioFs->Virtio, &Features);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }
  if ((Features & VIRTIO_F_VERSION_1) == 0) {
    Status = EFI_UNSUPPORTED;
    goto Failed;
  }
  //
  // No device-specific feature bits have been defined in file "virtio-fs.tex"
  // of the virtio spec at <https://github.com/oasis-tcs/virtio-spec.git>, as
  // of commit 87fa6b5d8155.
  //
  Features &= VIRTIO_F_VERSION_1 | VIRTIO_F_IOMMU_PLATFORM;

  //
  // ... and write the subset of feature bits understood by the [...] driver to
  // the device. [...]
  // 5. Set the FEATURES_OK status bit.
  // 6. Re-read device status to ensure the FEATURES_OK bit is still set [...]
  //
  Status = Virtio10WriteFeatures (VirtioFs->Virtio, Features, &NextDevStat);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }

  //
  // 7. Perform device-specific setup, including discovery of virtqueues for
  // the device, [...] reading [...] the device's virtio configuration space
  //
  Status = VirtioFsReadConfig (VirtioFs->Virtio, &Config);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }

  //
  // 7.a. Convert the filesystem label from UTF-8 to UCS-2. Only labels with
  // printable ASCII code points (U+0020 through U+007E) are supported.
  // NUL-terminate at either the terminator we find, or right after the
  // original label.
  //
  for (Idx = 0; Idx < VIRTIO_FS_TAG_BYTES && Config.Tag[Idx] != '\0'; Idx++) {
    if (Config.Tag[Idx] < 0x20 || Config.Tag[Idx] > 0x7E) {
      Status = EFI_UNSUPPORTED;
      goto Failed;
    }
    VirtioFs->Label[Idx] = Config.Tag[Idx];
  }
  VirtioFs->Label[Idx] = L'\0';

  //
  // 7.b. We need one queue for sending normal priority requests.
  //
  if (Config.NumReqQueues < 1) {
    Status = EFI_UNSUPPORTED;
    goto Failed;
  }

  //
  // 7.c. Fetch and remember the number of descriptors we can place on the
  // queue at once. We'll need two descriptors per request, as a minimum --
  // request header, response header.
  //
  Status = VirtioFs->Virtio->SetQueueSel (VirtioFs->Virtio,
                               VIRTIO_FS_REQUEST_QUEUE);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }
  Status = VirtioFs->Virtio->GetQueueNumMax (VirtioFs->Virtio,
                               &VirtioFs->QueueSize);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }
  if (VirtioFs->QueueSize < 2) {
    Status = EFI_UNSUPPORTED;
    goto Failed;
  }

  //
  // 7.d. [...] population of virtqueues [...]
  //
  Status = VirtioRingInit (VirtioFs->Virtio, VirtioFs->QueueSize,
             &VirtioFs->Ring);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }

  Status = VirtioRingMap (VirtioFs->Virtio, &VirtioFs->Ring, &RingBaseShift,
             &VirtioFs->RingMap);
  if (EFI_ERROR (Status)) {
    goto ReleaseQueue;
  }

  Status = VirtioFs->Virtio->SetQueueAddress (VirtioFs->Virtio,
                               &VirtioFs->Ring, RingBaseShift);
  if (EFI_ERROR (Status)) {
    goto UnmapQueue;
  }

  //
  // 8. Set the DRIVER_OK status bit.
  //
  NextDevStat |= VSTAT_DRIVER_OK;
  Status = VirtioFs->Virtio->SetDeviceStatus (VirtioFs->Virtio, NextDevStat);
  if (EFI_ERROR (Status)) {
    goto UnmapQueue;
  }

  return EFI_SUCCESS;

UnmapQueue:
  VirtioFs->Virtio->UnmapSharedBuffer (VirtioFs->Virtio, VirtioFs->RingMap);

ReleaseQueue:
  VirtioRingUninit (VirtioFs->Virtio, &VirtioFs->Ring);

Failed:
  //
  // If any of these steps go irrecoverably wrong, the driver SHOULD set the
  // FAILED status bit to indicate that it has given up on the device (it can
  // reset the device later to restart if desired). [...]
  //
  // Virtio access failure here should not mask the original error.
  //
  NextDevStat |= VSTAT_FAILED;
  VirtioFs->Virtio->SetDeviceStatus (VirtioFs->Virtio, NextDevStat);

  return Status;
}

/**
  De-configure the Virtio Filesystem device underlying VirtioFs.

  @param[in] VirtioFs  The VIRTIO_FS object for which Virtio communication
                       should be torn down. On input, the caller is responsible
                       for having called VirtioFsInit(). On output, Virtio
                       Filesystem commands (primitives) must no longer be
                       submitted to the device.
**/
VOID
VirtioFsUninit (
  IN OUT VIRTIO_FS *VirtioFs
  )
{
  //
  // Resetting the Virtio device makes it release its resources and forget its
  // configuration.
  //
  VirtioFs->Virtio->SetDeviceStatus (VirtioFs->Virtio, 0);
  VirtioFs->Virtio->UnmapSharedBuffer (VirtioFs->Virtio, VirtioFs->RingMap);
  VirtioRingUninit (VirtioFs->Virtio, &VirtioFs->Ring);
}

/**
  ExitBootServices event notification function for a Virtio Filesystem object.

  This function resets the VIRTIO_FS.Virtio device, causing it to release all
  references to guest-side resources. The function may only be called after
  VirtioFsInit() returns successfully and before VirtioFsUninit() is called.

  @param[in] ExitBootEvent   The VIRTIO_FS.ExitBoot event that has been
                             signaled.

  @param[in] VirtioFsAsVoid  Pointer to the VIRTIO_FS object, passed in as
                             (VOID*).
**/
VOID
EFIAPI
VirtioFsExitBoot (
  IN EFI_EVENT ExitBootEvent,
  IN VOID      *VirtioFsAsVoid
  )
{
  VIRTIO_FS *VirtioFs;

  VirtioFs = VirtioFsAsVoid;
  DEBUG ((DEBUG_VERBOSE, "%a: VirtioFs=0x%p Label=\"%s\"\n", __FUNCTION__,
    VirtioFsAsVoid, VirtioFs->Label));
  VirtioFs->Virtio->SetDeviceStatus (VirtioFs->Virtio, 0);
}

/**
  Validate two VIRTIO_FS_SCATTER_GATHER_LIST objects -- list of request
  buffers, list of response buffers -- together.

  On input, the caller is required to populate the following fields:
  - VIRTIO_FS_IO_VECTOR.Buffer,
  - VIRTIO_FS_IO_VECTOR.Size,
  - VIRTIO_FS_SCATTER_GATHER_LIST.IoVec,
  - VIRTIO_FS_SCATTER_GATHER_LIST.NumVec.

  On output (on successful return), the following fields will be
  zero-initialized:
  - VIRTIO_FS_IO_VECTOR.Mapped,
  - VIRTIO_FS_IO_VECTOR.MappedAddress,
  - VIRTIO_FS_IO_VECTOR.Mapping,
  - VIRTIO_FS_IO_VECTOR.Transferred.

  On output (on successful return), the following fields will be calculated:
  - VIRTIO_FS_SCATTER_GATHER_LIST.TotalSize.

  The function may only be called after VirtioFsInit() returns successfully and
  before VirtioFsUninit() is called.

  @param[in] VirtioFs            The Virtio Filesystem device that the
                                 request-response exchange, expressed via
                                 RequestSgList and ResponseSgList, will be
                                 submitted to.

  @param[in,out] RequestSgList   The scatter-gather list that describes the
                                 request part of the exchange -- the buffers
                                 that should be sent to the Virtio Filesystem
                                 device in the virtio transfer.

  @param[in,out] ResponseSgList  The scatter-gather list that describes the
                                 response part of the exchange -- the buffers
                                 that the Virtio Filesystem device should
                                 populate in the virtio transfer. May be NULL
                                 if the exchange with the Virtio Filesystem
                                 device consists of a request only, with the
                                 response part omitted altogether.

  @retval EFI_SUCCESS            RequestSgList and ResponseSgList have been
                                 validated, output fields have been set.

  @retval EFI_INVALID_PARAMETER  RequestSgList is NULL.

  @retval EFI_INVALID_PARAMETER  On input, a
                                 VIRTIO_FS_SCATTER_GATHER_LIST.IoVec field is
                                 NULL, or a
                                 VIRTIO_FS_SCATTER_GATHER_LIST.NumVec field is
                                 zero.

  @retval EFI_INVALID_PARAMETER  On input, a VIRTIO_FS_IO_VECTOR.Buffer field
                                 is NULL, or a VIRTIO_FS_IO_VECTOR.Size field
                                 is zero.

  @retval EFI_UNSUPPORTED        (RequestSgList->NumVec +
                                 ResponseSgList->NumVec) exceeds
                                 VirtioFs->QueueSize, meaning that the total
                                 list of buffers cannot be placed on the virtio
                                 queue in a single descriptor chain (with one
                                 descriptor per buffer).

  @retval EFI_UNSUPPORTED        One of the
                                 VIRTIO_FS_SCATTER_GATHER_LIST.TotalSize fields
                                 would exceed MAX_UINT32.
**/
EFI_STATUS
VirtioFsSgListsValidate (
  IN     VIRTIO_FS                     *VirtioFs,
  IN OUT VIRTIO_FS_SCATTER_GATHER_LIST *RequestSgList,
  IN OUT VIRTIO_FS_SCATTER_GATHER_LIST *ResponseSgList OPTIONAL
  )
{
  VIRTIO_FS_SCATTER_GATHER_LIST *SgListParam[2];
  UINT16                        DescriptorsNeeded;
  UINTN                         ListId;

  if (RequestSgList == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  SgListParam[0] = RequestSgList;
  SgListParam[1] = ResponseSgList;

  DescriptorsNeeded = 0;
  for (ListId = 0; ListId < ARRAY_SIZE (SgListParam); ListId++) {
    VIRTIO_FS_SCATTER_GATHER_LIST *SgList;
    UINT32                        SgListTotalSize;
    UINTN                         IoVecIdx;

    SgList = SgListParam[ListId];
    if (SgList == NULL) {
      continue;
    }
    //
    // Sanity-check SgList -- it must provide at least one IO Vector.
    //
    if (SgList->IoVec == NULL || SgList->NumVec == 0) {
      return EFI_INVALID_PARAMETER;
    }
    //
    // Make sure that, for each IO Vector in this SgList, a virtio descriptor
    // can be added to the virtio queue, after the other descriptors added
    // previously.
    //
    if (SgList->NumVec > (UINTN)(MAX_UINT16 - DescriptorsNeeded) ||
        DescriptorsNeeded + SgList->NumVec > VirtioFs->QueueSize) {
      return EFI_UNSUPPORTED;
    }
    DescriptorsNeeded += (UINT16)SgList->NumVec;

    SgListTotalSize = 0;
    for (IoVecIdx = 0; IoVecIdx < SgList->NumVec; IoVecIdx++) {
      VIRTIO_FS_IO_VECTOR *IoVec;

      IoVec = &SgList->IoVec[IoVecIdx];
      //
      // Sanity-check this IoVec -- it must describe a non-empty buffer.
      //
      if (IoVec->Buffer == NULL || IoVec->Size == 0) {
        return EFI_INVALID_PARAMETER;
      }
      //
      // Make sure the cumulative size of all IO Vectors in this SgList remains
      // expressible as a UINT32.
      //
      if (IoVec->Size > MAX_UINT32 - SgListTotalSize) {
        return EFI_UNSUPPORTED;
      }
      SgListTotalSize += (UINT32)IoVec->Size;

      //
      // Initialize those fields in this IO Vector that will be updated in
      // relation to mapping / transfer.
      //
      IoVec->Mapped        = FALSE;
      IoVec->MappedAddress = 0;
      IoVec->Mapping       = NULL;
      IoVec->Transferred   = 0;
    }

    //
    // Store the cumulative size of all IO Vectors that we have calculated in
    // this SgList.
    //
    SgList->TotalSize = SgListTotalSize;
  }

  return EFI_SUCCESS;
}

/**
  Submit a validated pair of (request buffer list, response buffer list) to the
  Virtio Filesystem device.

  On input, the pair of VIRTIO_FS_SCATTER_GATHER_LIST objects must have been
  validated together, using the VirtioFsSgListsValidate() function.

  On output (on successful return), the following fields will be re-initialized
  to zero (after temporarily setting them to different values):
  - VIRTIO_FS_IO_VECTOR.Mapped,
  - VIRTIO_FS_IO_VECTOR.MappedAddress,
  - VIRTIO_FS_IO_VECTOR.Mapping.

  On output (on successful return), the following fields will be calculated:
  - VIRTIO_FS_IO_VECTOR.Transferred.

  The function may only be called after VirtioFsInit() returns successfully and
  before VirtioFsUninit() is called.

  @param[in,out] VirtioFs        The Virtio Filesystem device that the
                                 request-response exchange, expressed via
                                 RequestSgList and ResponseSgList, should now
                                 be submitted to.

  @param[in,out] RequestSgList   The scatter-gather list that describes the
                                 request part of the exchange -- the buffers
                                 that should be sent to the Virtio Filesystem
                                 device in the virtio transfer.

  @param[in,out] ResponseSgList  The scatter-gather list that describes the
                                 response part of the exchange -- the buffers
                                 that the Virtio Filesystem device should
                                 populate in the virtio transfer. May be NULL
                                 if and only if NULL was passed to
                                 VirtioFsSgListsValidate() as ResponseSgList.

  @retval EFI_SUCCESS       Transfer complete. The caller should investigate
                            the VIRTIO_FS_IO_VECTOR.Transferred fields in
                            ResponseSgList, to ensure coverage of the relevant
                            response buffers. Subsequently, the caller should
                            investigate the contents of those buffers.

  @retval EFI_DEVICE_ERROR  The Virtio Filesystem device reported populating
                            more response bytes than ResponseSgList->TotalSize.

  @return                   Error codes propagated from
                            VirtioMapAllBytesInSharedBuffer(), VirtioFlush(),
                            or VirtioFs->Virtio->UnmapSharedBuffer().
**/
EFI_STATUS
VirtioFsSgListsSubmit (
  IN OUT VIRTIO_FS                     *VirtioFs,
  IN OUT VIRTIO_FS_SCATTER_GATHER_LIST *RequestSgList,
  IN OUT VIRTIO_FS_SCATTER_GATHER_LIST *ResponseSgList OPTIONAL
  )
{
  VIRTIO_FS_SCATTER_GATHER_LIST *SgListParam[2];
  VIRTIO_MAP_OPERATION          SgListVirtioMapOp[ARRAY_SIZE (SgListParam)];
  UINT16                        SgListDescriptorFlag[ARRAY_SIZE (SgListParam)];
  UINTN                         ListId;
  VIRTIO_FS_SCATTER_GATHER_LIST *SgList;
  UINTN                         IoVecIdx;
  VIRTIO_FS_IO_VECTOR           *IoVec;
  EFI_STATUS                    Status;
  DESC_INDICES                  Indices;
  UINT32                        TotalBytesWrittenByDevice;
  UINT32                        BytesPermittedForWrite;

  SgListParam[0]          = RequestSgList;
  SgListVirtioMapOp[0]    = VirtioOperationBusMasterRead;
  SgListDescriptorFlag[0] = 0;

  SgListParam[1]          = ResponseSgList;
  SgListVirtioMapOp[1]    = VirtioOperationBusMasterWrite;
  SgListDescriptorFlag[1] = VRING_DESC_F_WRITE;

  //
  // Map all IO Vectors.
  //
  for (ListId = 0; ListId < ARRAY_SIZE (SgListParam); ListId++) {
    SgList = SgListParam[ListId];
    if (SgList == NULL) {
      continue;
    }
    for (IoVecIdx = 0; IoVecIdx < SgList->NumVec; IoVecIdx++) {
      IoVec = &SgList->IoVec[IoVecIdx];
      //
      // Map this IO Vector.
      //
      Status = VirtioMapAllBytesInSharedBuffer (
                 VirtioFs->Virtio,
                 SgListVirtioMapOp[ListId],
                 IoVec->Buffer,
                 IoVec->Size,
                 &IoVec->MappedAddress,
                 &IoVec->Mapping
                 );
      if (EFI_ERROR (Status)) {
        goto Unmap;
      }
      IoVec->Mapped = TRUE;
    }
  }

  //
  // Compose the descriptor chain.
  //
  VirtioPrepare (&VirtioFs->Ring, &Indices);
  for (ListId = 0; ListId < ARRAY_SIZE (SgListParam); ListId++) {
    SgList = SgListParam[ListId];
    if (SgList == NULL) {
      continue;
    }
    for (IoVecIdx = 0; IoVecIdx < SgList->NumVec; IoVecIdx++) {
      UINT16 NextFlag;

      IoVec = &SgList->IoVec[IoVecIdx];
      //
      // Set VRING_DESC_F_NEXT on all except the very last descriptor.
      //
      NextFlag = VRING_DESC_F_NEXT;
      if (ListId == ARRAY_SIZE (SgListParam) - 1 &&
          IoVecIdx == SgList->NumVec - 1) {
        NextFlag = 0;
      }
      VirtioAppendDesc (
        &VirtioFs->Ring,
        IoVec->MappedAddress,
        (UINT32)IoVec->Size,
        SgListDescriptorFlag[ListId] | NextFlag,
        &Indices
        );
    }
  }

  //
  // Submit the descriptor chain.
  //
  Status = VirtioFlush (VirtioFs->Virtio, VIRTIO_FS_REQUEST_QUEUE,
             &VirtioFs->Ring, &Indices, &TotalBytesWrittenByDevice);
  if (EFI_ERROR (Status)) {
    goto Unmap;
  }

  //
  // Sanity-check: the Virtio Filesystem device should not have written more
  // bytes than what we offered buffers for.
  //
  if (ResponseSgList == NULL) {
    BytesPermittedForWrite = 0;
  } else {
    BytesPermittedForWrite = ResponseSgList->TotalSize;
  }
  if (TotalBytesWrittenByDevice > BytesPermittedForWrite) {
    Status = EFI_DEVICE_ERROR;
    goto Unmap;
  }

  //
  // Update the transfer sizes in the IO Vectors.
  //
  for (ListId = 0; ListId < ARRAY_SIZE (SgListParam); ListId++) {
    SgList = SgListParam[ListId];
    if (SgList == NULL) {
      continue;
    }
    for (IoVecIdx = 0; IoVecIdx < SgList->NumVec; IoVecIdx++) {
      IoVec = &SgList->IoVec[IoVecIdx];
      if (SgListVirtioMapOp[ListId] == VirtioOperationBusMasterRead) {
        //
        // We report that the Virtio Filesystem device has read all buffers in
        // the request.
        //
        IoVec->Transferred = IoVec->Size;
      } else {
        //
        // Regarding the response, calculate how much of the current IO Vector
        // has been populated by the Virtio Filesystem device. In
        // "TotalBytesWrittenByDevice", VirtioFlush() reported the total count
        // across all device-writeable descriptors, in the order they were
        // chained on the ring.
        //
        IoVec->Transferred = MIN ((UINTN)TotalBytesWrittenByDevice,
                               IoVec->Size);
        TotalBytesWrittenByDevice -= (UINT32)IoVec->Transferred;
      }
    }
  }

  //
  // By now, "TotalBytesWrittenByDevice" has been exhausted.
  //
  ASSERT (TotalBytesWrittenByDevice == 0);

  //
  // We've succeeded; fall through.
  //
Unmap:
  //
  // Unmap all mapped IO Vectors on both the success and the error paths. The
  // unmapping occurs in reverse order of mapping, in an attempt to avoid
  // memory fragmentation.
  //
  ListId = ARRAY_SIZE (SgListParam);
  while (ListId > 0) {
    --ListId;
    SgList = SgListParam[ListId];
    if (SgList == NULL) {
      continue;
    }
    IoVecIdx = SgList->NumVec;
    while (IoVecIdx > 0) {
      EFI_STATUS UnmapStatus;

      --IoVecIdx;
      IoVec = &SgList->IoVec[IoVecIdx];
      //
      // Unmap this IO Vector, if it has been mapped.
      //
      if (!IoVec->Mapped) {
        continue;
      }
      UnmapStatus = VirtioFs->Virtio->UnmapSharedBuffer (VirtioFs->Virtio,
                                        IoVec->Mapping);
      //
      // Re-set the following fields to the values they initially got from
      // VirtioFsSgListsValidate() -- the above unmapping attempt is considered
      // final, even if it fails.
      //
      IoVec->Mapped        = FALSE;
      IoVec->MappedAddress = 0;
      IoVec->Mapping       = NULL;

      //
      // If we are on the success path, but the unmapping failed, we need to
      // transparently flip to the failure path -- the caller must learn they
      // should not consult the response buffers.
      //
      // The branch below can be taken at most once.
      //
      if (!EFI_ERROR (Status) && EFI_ERROR (UnmapStatus)) {
        Status = UnmapStatus;
      }
    }
  }

  return Status;
}

/**
  Set up the fields of a new VIRTIO_FS_FUSE_REQUEST object.

  The function may only be called after VirtioFsInit() returns successfully and
  before VirtioFsUninit() is called.

  @param[in,out] VirtioFs The Virtio Filesystem device that the request is
                          being prepared for. The "VirtioFs->RequestId" field
                          will be copied into "Request->Unique". On output (on
                          successful return), "VirtioFs->RequestId" will be
                          incremented.

  @param[out] Request     The VIRTIO_FS_FUSE_REQUEST object whose fields are to
                          be set.

  @param[in] RequestSize  The total size of the request, including
                          sizeof(VIRTIO_FS_FUSE_REQUEST).

  @param[in] Opcode       The VIRTIO_FS_FUSE_OPCODE that identifies the command
                          to send.

  @param[in] NodeId       The inode number of the file that the request refers
                          to. When Opcode is VirtioFsFuseOpInit, NodeId is
                          ignored by the Virtio Filesystem device.

  @retval EFI_INVALID_PARAMETER  RequestSize is smaller than
                                 sizeof(VIRTIO_FS_FUSE_REQUEST).

  @retval EFI_OUT_OF_RESOURCES   "VirtioFs->RequestId" is MAX_UINT64, and can
                                 be incremented no more.

  @retval EFI_SUCCESS            Request has been populated,
                                 "VirtioFs->RequestId" has been incremented.
**/
EFI_STATUS
VirtioFsFuseNewRequest (
  IN OUT VIRTIO_FS              *VirtioFs,
     OUT VIRTIO_FS_FUSE_REQUEST *Request,
  IN     UINT32                 RequestSize,
  IN     VIRTIO_FS_FUSE_OPCODE  Opcode,
  IN     UINT64                 NodeId
  )
{
  if (RequestSize < sizeof *Request) {
    return EFI_INVALID_PARAMETER;
  }

  if (VirtioFs->RequestId == MAX_UINT64) {
    return EFI_OUT_OF_RESOURCES;
  }

  Request->Len     = RequestSize;
  Request->Opcode  = Opcode;
  Request->Unique  = VirtioFs->RequestId++;
  Request->NodeId  = NodeId;
  Request->Uid     = 0;
  Request->Gid     = 0;
  Request->Pid     = 1;
  Request->Padding = 0;

  return EFI_SUCCESS;
}

/**
  Check the common FUSE response format.

  The first buffer in the response scatter-gather list is assumed a
  VIRTIO_FS_FUSE_RESPONSE structure. Subsequent response buffers, if any, up to
  and excluding the last one, are assumed fixed size. The last response buffer
  may or may not be fixed size, as specified by the caller.

  This function may only be called after VirtioFsSgListsSubmit() returns
  successfully.

  @param[in] ResponseSgList   The scatter-gather list that describes the
                              response part of the exchange -- the buffers that
                              the Virtio Filesystem device filled in during the
                              virtio transfer.

  @param[in] RequestId        The request identifier to which the response is
                              expected to belong.

  @param[out] TailBufferFill  If NULL, then the last buffer in ResponseSgList
                              is considered fixed size. Otherwise, the last
                              buffer is considered variable size, and on
                              successful return, TailBufferFill reports the
                              number of bytes in the last buffer.

  @retval EFI_INVALID_PARAMETER  TailBufferFill is not NULL (i.e., the last
                                 buffer is considered variable size), and
                                 ResponseSgList->NumVec is 1.

  @retval EFI_INVALID_PARAMETER  The allocated size of the first buffer does
                                 not match sizeof(VIRTIO_FS_FUSE_RESPONSE).

  @retval EFI_PROTOCOL_ERROR     The VIRTIO_FS_FUSE_RESPONSE structure in the
                                 first buffer has not been fully populated.

  @retval EFI_PROTOCOL_ERROR     "VIRTIO_FS_FUSE_RESPONSE.Len" in the first
                                 buffer does not equal the sum of the
                                 individual buffer sizes (as populated).

  @retval EFI_PROTOCOL_ERROR     "VIRTIO_FS_FUSE_RESPONSE.Unique" in the first
                                 buffer does not equal RequestId.

  @retval EFI_PROTOCOL_ERROR     "VIRTIO_FS_FUSE_RESPONSE.Error" in the first
                                 buffer is zero, but a subsequent fixed size
                                 buffer has not been fully populated.

  @retval EFI_DEVICE_ERROR       "VIRTIO_FS_FUSE_RESPONSE.Error" in the first
                                 buffer is nonzero. The caller may investigate
                                 "VIRTIO_FS_FUSE_RESPONSE.Error". Note that the
                                 completeness of the subsequent fixed size
                                 buffers is not verified in this case.

  @retval EFI_SUCCESS            Verification successful.
**/
EFI_STATUS
VirtioFsFuseCheckResponse (
  IN  VIRTIO_FS_SCATTER_GATHER_LIST *ResponseSgList,
  IN  UINT64                        RequestId,
  OUT UINTN                         *TailBufferFill
  )
{
  UINTN                   NumFixedSizeVec;
  VIRTIO_FS_FUSE_RESPONSE *CommonResp;
  UINT32                  TotalTransferred;
  UINTN                   Idx;

  //
  // Ensured by VirtioFsSgListsValidate().
  //
  ASSERT (ResponseSgList->NumVec > 0);

  if (TailBufferFill == NULL) {
    //
    // All buffers are considered fixed size.
    //
    NumFixedSizeVec = ResponseSgList->NumVec;
  } else {
    //
    // If the last buffer is variable size, then we need that buffer to be
    // different from the first buffer, which is considered a
    // VIRTIO_FS_FUSE_RESPONSE (fixed size) structure.
    //
    if (ResponseSgList->NumVec == 1) {
      return EFI_INVALID_PARAMETER;
    }
    NumFixedSizeVec = ResponseSgList->NumVec - 1;
  }

  //
  // The first buffer is supposed to carry a (fully populated)
  // VIRTIO_FS_FUSE_RESPONSE structure.
  //
  if (ResponseSgList->IoVec[0].Size != sizeof *CommonResp) {
    return EFI_INVALID_PARAMETER;
  }
  if (ResponseSgList->IoVec[0].Transferred != ResponseSgList->IoVec[0].Size) {
    return EFI_PROTOCOL_ERROR;
  }

  //
  // FUSE must report the same number of bytes, written by the Virtio
  // Filesystem device, as the virtio transport does.
  //
  CommonResp = ResponseSgList->IoVec[0].Buffer;
  TotalTransferred = 0;
  for (Idx = 0; Idx < ResponseSgList->NumVec; Idx++) {
    //
    // Integer overflow and truncation are not possible, based on
    // VirtioFsSgListsValidate() and VirtioFsSgListsSubmit().
    //
    TotalTransferred += (UINT32)ResponseSgList->IoVec[Idx].Transferred;
  }
  if (CommonResp->Len != TotalTransferred) {
    return EFI_PROTOCOL_ERROR;
  }

  //
  // Enforce that FUSE match our request ID in the response.
  //
  if (CommonResp->Unique != RequestId) {
    return EFI_PROTOCOL_ERROR;
  }

  //
  // If there is an explicit error report, skip checking the transfer
  // counts for the rest of the fixed size buffers.
  //
  if (CommonResp->Error != 0) {
    return EFI_DEVICE_ERROR;
  }

  //
  // There was no error reported, so we require that the Virtio Filesystem
  // device populate all fixed size buffers. We checked this for the very first
  // buffer above; let's check the rest (if any).
  //
  ASSERT (NumFixedSizeVec >= 1);
  for (Idx = 1; Idx < NumFixedSizeVec; Idx++) {
    if (ResponseSgList->IoVec[Idx].Transferred !=
        ResponseSgList->IoVec[Idx].Size) {
      return EFI_PROTOCOL_ERROR;
    }
  }

  //
  // If the last buffer is considered variable size, report its filled size.
  //
  if (TailBufferFill != NULL) {
    *TailBufferFill = ResponseSgList->IoVec[NumFixedSizeVec].Transferred;
  }

  return EFI_SUCCESS;
}

/**
  An ad-hoc function for mapping FUSE (well, Linux) "errno" values to
  EFI_STATUS.

  @param[in] Errno  The "VIRTIO_FS_FUSE_RESPONSE.Error" value, returned by the
                    Virtio Filesystem device. The value is expected to be
                    negative.

  @return                   An EFI_STATUS error code that's deemed a passable
                            mapping for the Errno value.

  @retval EFI_DEVICE_ERROR  Fallback EFI_STATUS code for unrecognized Errno
                            values.
**/
EFI_STATUS
VirtioFsErrnoToEfiStatus (
  IN INT32 Errno
  )
{
  switch (Errno) {
  case   -1: // EPERM               Operation not permitted
    return EFI_SECURITY_VIOLATION;

  case   -2: // ENOENT              No such file or directory
  case   -3: // ESRCH               No such process
  case   -6: // ENXIO               No such device or address
  case  -10: // ECHILD              No child processes
  case  -19: // ENODEV              No such device
  case  -49: // EUNATCH             Protocol driver not attached
  case  -65: // ENOPKG              Package not installed
  case  -79: // ELIBACC             Can not access a needed shared library
  case -126: // ENOKEY              Required key not available
    return EFI_NOT_FOUND;

  case   -4: // EINTR               Interrupted system call
  case  -11: // EAGAIN, EWOULDBLOCK Resource temporarily unavailable
  case  -16: // EBUSY               Device or resource busy
  case  -26: // ETXTBSY             Text file busy
  case  -35: // EDEADLK, EDEADLOCK  Resource deadlock avoided
  case  -39: // ENOTEMPTY           Directory not empty
  case  -42: // ENOMSG              No message of desired type
  case  -61: // ENODATA             No data available
  case  -85: // ERESTART            Interrupted system call should be restarted
    return EFI_NOT_READY;

  case   -5: // EIO                 Input/output error
  case  -45: // EL2NSYNC            Level 2 not synchronized
  case  -46: // EL3HLT              Level 3 halted
  case  -47: // EL3RST              Level 3 reset
  case  -51: // EL2HLT              Level 2 halted
  case -121: // EREMOTEIO           Remote I/O error
  case -133: // EHWPOISON           Memory page has hardware error
    return EFI_DEVICE_ERROR;

  case   -7: // E2BIG               Argument list too long
  case  -36: // ENAMETOOLONG        File name too long
  case  -90: // EMSGSIZE            Message too long
    return EFI_BAD_BUFFER_SIZE;

  case   -8: // ENOEXEC             Exec format error
  case  -15: // ENOTBLK             Block device required
  case  -18: // EXDEV               Invalid cross-device link
  case  -20: // ENOTDIR             Not a directory
  case  -21: // EISDIR              Is a directory
  case  -25: // ENOTTY              Inappropriate ioctl for device
  case  -27: // EFBIG               File too large
  case  -29: // ESPIPE              Illegal seek
  case  -38: // ENOSYS              Function not implemented
  case  -59: // EBFONT              Bad font file format
  case  -60: // ENOSTR              Device not a stream
  case  -83: // ELIBEXEC            Cannot exec a shared library directly
  case  -88: // ENOTSOCK            Socket operation on non-socket
  case  -91: // EPROTOTYPE          Protocol wrong type for socket
  case  -92: // ENOPROTOOPT         Protocol not available
  case  -93: // EPROTONOSUPPORT     Protocol not supported
  case  -94: // ESOCKTNOSUPPORT     Socket type not supported
  case  -95: // ENOTSUP, EOPNOTSUPP Operation not supported
  case  -96: // EPFNOSUPPORT        Protocol family not supported
  case  -97: // EAFNOSUPPORT        Address family not supported by protocol
  case  -99: // EADDRNOTAVAIL       Cannot assign requested address
  case -118: // ENOTNAM             Not a XENIX named type file
  case -120: // EISNAM              Is a named type file
  case -124: // EMEDIUMTYPE         Wrong medium type
    return EFI_UNSUPPORTED;

  case   -9: // EBADF               Bad file descriptor
  case  -14: // EFAULT              Bad address
  case  -44: // ECHRNG              Channel number out of range
  case  -48: // ELNRNG              Link number out of range
  case  -53: // EBADR               Invalid request descriptor
  case  -56: // EBADRQC             Invalid request code
  case  -57: // EBADSLT             Invalid slot
  case  -76: // ENOTUNIQ            Name not unique on network
  case  -84: // EILSEQ        Invalid or incomplete multibyte or wide character
    return EFI_NO_MAPPING;

  case  -12: // ENOMEM              Cannot allocate memory
  case  -23: // ENFILE              Too many open files in system
  case  -24: // EMFILE              Too many open files
  case  -31: // EMLINK              Too many links
  case  -37: // ENOLCK              No locks available
  case  -40: // ELOOP               Too many levels of symbolic links
  case  -50: // ENOCSI              No CSI structure available
  case  -55: // ENOANO              No anode
  case  -63: // ENOSR               Out of streams resources
  case  -82: // ELIBMAX         Attempting to link in too many shared libraries
  case  -87: // EUSERS              Too many users
  case -105: // ENOBUFS             No buffer space available
  case -109: // ETOOMANYREFS        Too many references: cannot splice
  case -119: // ENAVAIL             No XENIX semaphores available
  case -122: // EDQUOT              Disk quota exceeded
    return EFI_OUT_OF_RESOURCES;

  case  -13: // EACCES              Permission denied
    return EFI_ACCESS_DENIED;

  case  -17: // EEXIST              File exists
  case  -98: // EADDRINUSE          Address already in use
  case -106: // EISCONN             Transport endpoint is already connected
  case -114: // EALREADY            Operation already in progress
  case -115: // EINPROGRESS         Operation now in progress
    return EFI_ALREADY_STARTED;

  case  -22: // EINVAL              Invalid argument
  case  -33: // EDOM                Numerical argument out of domain
    return EFI_INVALID_PARAMETER;

  case  -28: // ENOSPC              No space left on device
  case  -54: // EXFULL              Exchange full
    return EFI_VOLUME_FULL;

  case  -30: // EROFS               Read-only file system
    return EFI_WRITE_PROTECTED;

  case  -32: // EPIPE               Broken pipe
  case  -43: // EIDRM               Identifier removed
  case  -67: // ENOLINK             Link has been severed
  case  -68: // EADV                Advertise error
  case  -69: // ESRMNT              Srmount error
  case  -70: // ECOMM               Communication error on send
  case  -73: // EDOTDOT             RFS specific error
  case  -78: // EREMCHG             Remote address changed
  case  -86: // ESTRPIPE            Streams pipe error
  case -102: // ENETRESET           Network dropped connection on reset
  case -103: // ECONNABORTED        Software caused connection abort
  case -104: // ECONNRESET          Connection reset by peer
  case -116: // ESTALE              Stale file handle
  case -125: // ECANCELED           Operation canceled
  case -128: // EKEYREVOKED         Key has been revoked
  case -129: // EKEYREJECTED        Key was rejected by service
  case -130: // EOWNERDEAD          Owner died
  case -131: // ENOTRECOVERABLE     State not recoverable
    return EFI_ABORTED;

  case  -34: // ERANGE              Numerical result out of range
  case  -75: // EOVERFLOW           Value too large for defined data type
    return EFI_BUFFER_TOO_SMALL;

  case  -52: // EBADE               Invalid exchange
  case -108: // ESHUTDOWN         Cannot send after transport endpoint shutdown
  case -111: // ECONNREFUSED        Connection refused
    return EFI_END_OF_FILE;

  case  -62: // ETIME               Timer expired
  case -110: // ETIMEDOUT           Connection timed out
  case -127: // EKEYEXPIRED         Key has expired
    return EFI_TIMEOUT;

  case  -64: // ENONET              Machine is not on the network
  case  -66: // EREMOTE             Object is remote
  case  -72: // EMULTIHOP           Multihop attempted
  case -100: // ENETDOWN            Network is down
  case -101: // ENETUNREACH         Network is unreachable
  case -112: // EHOSTDOWN           Host is down
  case -113: // EHOSTUNREACH        No route to host
  case -123: // ENOMEDIUM           No medium found
  case -132: // ERFKILL             Operation not possible due to RF-kill
    return EFI_NO_MEDIA;

  case  -71: // EPROTO              Protocol error
    return EFI_PROTOCOL_ERROR;

  case  -74: // EBADMSG             Bad message
  case  -77: // EBADFD              File descriptor in bad state
  case  -80: // ELIBBAD             Accessing a corrupted shared library
  case  -81: // ELIBSCN             .lib section in a.out corrupted
  case -117: // EUCLEAN             Structure needs cleaning
    return EFI_VOLUME_CORRUPTED;

  case  -89: // EDESTADDRREQ        Destination address required
  case -107: // ENOTCONN            Transport endpoint is not connected
    return EFI_NOT_STARTED;

  default:
    break;
  }

  return EFI_DEVICE_ERROR;
}

//
// Parser states for canonicalizing a POSIX pathname.
//
typedef enum {
  ParserInit,   // just starting
  ParserEnd,    // finished
  ParserSlash,  // slash(es) seen
  ParserDot,    // one dot seen since last slash
  ParserDotDot, // two dots seen since last slash
  ParserNormal, // a different sequence seen
} PARSER_STATE;

/**
  Strip the trailing slash from the parser's output buffer, unless the trailing
  slash stands for the root directory.

  @param[in] Buffer        The parser's output buffer. Only used for
                           sanity-checking.

  @param[in,out] Position  On entry, points at the next character to produce
                           (i.e., right past the end of the output written by
                           the parser thus far). The last character in the
                           parser's output buffer is a slash. On return, the
                           slash is stripped, by decrementing Position by one.
                           If this action would remove the slash character
                           standing for the root directory, then the function
                           has no effect.
**/
STATIC
VOID
ParserStripSlash (
  IN     CHAR8 *Buffer,
  IN OUT UINTN *Position
  )
{
  ASSERT (*Position >= 1);
  ASSERT (Buffer[*Position - 1] == '/');
  if (*Position == 1) {
    return;
  }
  (*Position)--;
}

/**
  Produce one character in the parser's output buffer.

  @param[out] Buffer       The parser's output buffer. On return, Char8 will
                           have been written.

  @param[in,out] Position  On entry, points at the next character to produce
                           (i.e., right past the end of the output written by
                           the parser thus far). On return, Position is
                           incremented by one.

  @param[in] Size          Total allocated size of the parser's output buffer.
                           Used for sanity-checking.

  @param[in] Char8         The character to place in the output buffer.
**/
STATIC
VOID
ParserCopy (
     OUT CHAR8 *Buffer,
  IN OUT UINTN *Position,
  IN     UINTN Size,
  IN     CHAR8 Char8
  )
{
  ASSERT (*Position < Size);
  Buffer[(*Position)++] = Char8;
}

/**
  Rewind the last single-dot in the parser's output buffer.

  @param[in] Buffer        The parser's output buffer. Only used for
                           sanity-checking.

  @param[in,out] Position  On entry, points at the next character to produce
                           (i.e., right past the end of the output written by
                           the parser thus far); the parser's output buffer
                           ends with the characters ('/', '.'). On return, the
                           dot is rewound by decrementing Position by one; a
                           slash character will reside at the new end of the
                           parser's output buffer.
**/
STATIC
VOID
ParserRewindDot (
  IN     CHAR8 *Buffer,
  IN OUT UINTN *Position
  )
{
  ASSERT (*Position >= 2);
  ASSERT (Buffer[*Position - 1] == '.');
  ASSERT (Buffer[*Position - 2] == '/');
  (*Position)--;
}

/**
  Rewind the last dot-dot in the parser's output buffer.

  @param[in] Buffer        The parser's output buffer. Only used for
                           sanity-checking.

  @param[in,out] Position  On entry, points at the next character to produce
                           (i.e., right past the end of the output written by
                           the parser thus far); the parser's output buffer
                           ends with the characters ('/', '.', '.'). On return,
                           the ('.', '.') pair is rewound unconditionally, by
                           decrementing Position by two; a slash character
                           resides at the new end of the parser's output
                           buffer.

                           If this slash character stands for the root
                           directory, then RootEscape is set to TRUE.

                           Otherwise (i.e., if this slash character is not the
                           one standing for the root directory), then the slash
                           character, and the pathname component preceding it,
                           are removed by decrementing Position further. In
                           this case, the slash character preceding the removed
                           pathname component will reside at the new end of the
                           parser's output buffer.

  @param[out] RootEscape   Set to TRUE on output if the dot-dot component tries
                           to escape the root directory, as described above.
                           Otherwise, RootEscape is not modified.
**/
STATIC
VOID
ParserRewindDotDot (
  IN     CHAR8   *Buffer,
  IN OUT UINTN   *Position,
     OUT BOOLEAN *RootEscape

  )
{
  ASSERT (*Position >= 3);
  ASSERT (Buffer[*Position - 1] == '.');
  ASSERT (Buffer[*Position - 2] == '.');
  ASSERT (Buffer[*Position - 3] == '/');
  (*Position) -= 2;

  if (*Position == 1) {
    //
    // Root directory slash reached; don't try to climb higher.
    //
    *RootEscape = TRUE;
    return;
  }

  //
  // Skip slash.
  //
  (*Position)--;
  //
  // Scan until next slash to the left.
  //
  do {
    ASSERT (*Position > 0);
    (*Position)--;
  } while (Buffer[*Position] != '/');
  (*Position)++;
}

/**
  Append the UEFI-style RhsPath16 to the POSIX-style, canonical format
  LhsPath8. Output the POSIX-style, canonical format result in ResultPath, as a
  dynamically allocated string.

  Canonicalization (aka sanitization) establishes the following properties:
  - ResultPath is absolute (starts with "/"),
  - dot (.) and dot-dot (..) components are resolved/eliminated in ResultPath,
    with the usual semantics,
  - ResultPath uses forward slashes,
  - sequences of slashes are squashed in ResultPath,
  - the printable ASCII character set covers ResultPath,
  - CHAR8 encoding is used in ResultPath,
  - no trailing slash present in ResultPath except for the standalone root
    directory,
  - the length of ResultPath is at most VIRTIO_FS_MAX_PATHNAME_LENGTH.

  Any dot-dot in RhsPath16 that would remove the root directory is dropped, and
  reported through RootEscape, without failing the function call.

  @param[in] LhsPath8      Identifies the base directory. The caller is
                           responsible for ensuring that LhsPath8 conform to
                           the above canonical pathname format on entry.

  @param[in] RhsPath16     Identifies the desired file with a UEFI-style CHAR16
                           pathname. If RhsPath16 starts with a backslash, then
                           RhsPath16 is considered absolute, and LhsPath8 is
                           ignored; RhsPath16 is sanitized in isolation, for
                           producing ResultPath8. Otherwise (i.e., if RhsPath16
                           is relative), RhsPath16 is transliterated to CHAR8,
                           and naively appended to LhsPath8. The resultant
                           fused pathname is then sanitized, to produce
                           ResultPath8.

  @param[out] ResultPath8  The POSIX-style, canonical format pathname that
                           leads to the file desired by the caller. After use,
                           the caller is responsible for freeing ResultPath8.

  @param[out] RootEscape   Set to TRUE if at least one dot-dot component in
                           RhsPath16 attempted to escape the root directory;
                           set to FALSE otherwise.

  @retval EFI_SUCCESS            ResultPath8 has been produced. RootEscape has
                                 been output.

  @retval EFI_INVALID_PARAMETER  RhsPath16 is zero-length.

  @retval EFI_INVALID_PARAMETER  RhsPath16 failed the
                                 VIRTIO_FS_MAX_PATHNAME_LENGTH check.

  @retval EFI_OUT_OF_RESOURCES   Memory allocation failed.

  @retval EFI_OUT_OF_RESOURCES   ResultPath8 would have failed the
                                 VIRTIO_FS_MAX_PATHNAME_LENGTH check.

  @retval EFI_UNSUPPORTED        RhsPath16 contains a character that either
                                 falls outside of the printable ASCII set, or
                                 is a forward slash.
**/
EFI_STATUS
VirtioFsAppendPath (
  IN     CHAR8   *LhsPath8,
  IN     CHAR16  *RhsPath16,
     OUT CHAR8   **ResultPath8,
     OUT BOOLEAN *RootEscape
  )
{
  UINTN        RhsLen;
  CHAR8        *RhsPath8;
  UINTN        Idx;
  EFI_STATUS   Status;
  UINTN        SizeToSanitize;
  CHAR8        *BufferToSanitize;
  CHAR8        *SanitizedBuffer;
  PARSER_STATE State;
  UINTN        SanitizedPosition;

  //
  // Appending an empty pathname is not allowed.
  //
  RhsLen = StrLen (RhsPath16);
  if (RhsLen == 0) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Enforce length restriction on RhsPath16.
  //
  if (RhsLen > VIRTIO_FS_MAX_PATHNAME_LENGTH) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Transliterate RhsPath16 to RhsPath8 by:
  // - rejecting RhsPath16 if a character outside of printable ASCII is seen,
  // - rejecting RhsPath16 if a forward slash is seen,
  // - replacing backslashes with forward slashes,
  // - casting the characters from CHAR16 to CHAR8.
  //
  RhsPath8 = AllocatePool (RhsLen + 1);
  if (RhsPath8 == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  for (Idx = 0; RhsPath16[Idx] != L'\0'; Idx++) {
    if (RhsPath16[Idx] < 0x20 || RhsPath16[Idx] > 0x7E ||
        RhsPath16[Idx] == L'/') {
      Status = EFI_UNSUPPORTED;
      goto FreeRhsPath8;
    }
    RhsPath8[Idx] = (CHAR8)((RhsPath16[Idx] == L'\\') ? L'/' : RhsPath16[Idx]);
  }
  RhsPath8[Idx++] = '\0';

  //
  // Now prepare the input for the canonicalization (squashing of sequences of
  // forward slashes, and eliminating . (dot) and .. (dot-dot) pathname
  // components).
  //
  // The sanitized path can never be longer than the naive concatenation of the
  // left hand side and right hand side paths, so we'll use the catenated size
  // for allocating the sanitized output too.
  //
  if (RhsPath8[0] == '/') {
    //
    // If the right hand side path is absolute, then it is not appended to the
    // left hand side path -- it *replaces* the left hand side path.
    //
    SizeToSanitize = RhsLen + 1;
    BufferToSanitize = RhsPath8;
  } else {
    //
    // If the right hand side path is relative, then it is appended (naively)
    // to the left hand side.
    //
    UINTN LhsLen;

    LhsLen = AsciiStrLen (LhsPath8);
    SizeToSanitize = LhsLen + 1 + RhsLen + 1;
    BufferToSanitize = AllocatePool (SizeToSanitize);
    if (BufferToSanitize == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto FreeRhsPath8;
    }
    CopyMem (BufferToSanitize, LhsPath8, LhsLen);
    BufferToSanitize[LhsLen] = '/';
    CopyMem (BufferToSanitize + LhsLen + 1, RhsPath8, RhsLen + 1);
  }

  //
  // Allocate the output buffer.
  //
  SanitizedBuffer = AllocatePool (SizeToSanitize);
  if (SanitizedBuffer == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto FreeBufferToSanitize;
  }

  //
  // State machine for parsing the input and producing the canonical output
  // follows.
  //
  *RootEscape       = FALSE;
  Idx               = 0;
  State             = ParserInit;
  SanitizedPosition = 0;
  do {
    CHAR8 Chr8;

    ASSERT (Idx < SizeToSanitize);
    Chr8 = BufferToSanitize[Idx++];

    switch (State) {
    case ParserInit: // just starting
      ASSERT (Chr8 == '/');
      ParserCopy (SanitizedBuffer, &SanitizedPosition, SizeToSanitize, Chr8);
      State = ParserSlash;
      break;

    case ParserSlash: // slash(es) seen
      switch (Chr8) {
      case '\0':
        ParserStripSlash (SanitizedBuffer, &SanitizedPosition);
        ParserCopy (SanitizedBuffer, &SanitizedPosition, SizeToSanitize, Chr8);
        State = ParserEnd;
        break;
      case '/':
        //
        // skip & stay in same state
        //
        break;
      case '.':
        ParserCopy (SanitizedBuffer, &SanitizedPosition, SizeToSanitize, Chr8);
        State = ParserDot;
        break;
      default:
        ParserCopy (SanitizedBuffer, &SanitizedPosition, SizeToSanitize, Chr8);
        State = ParserNormal;
        break;
      }
      break;

    case ParserDot: // one dot seen since last slash
      switch (Chr8) {
      case '\0':
        ParserRewindDot (SanitizedBuffer, &SanitizedPosition);
        ParserStripSlash (SanitizedBuffer, &SanitizedPosition);
        ParserCopy (SanitizedBuffer, &SanitizedPosition, SizeToSanitize, Chr8);
        State = ParserEnd;
        break;
      case '/':
        ParserRewindDot (SanitizedBuffer, &SanitizedPosition);
        State = ParserSlash;
        break;
      case '.':
        ParserCopy (SanitizedBuffer, &SanitizedPosition, SizeToSanitize, Chr8);
        State = ParserDotDot;
        break;
      default:
        ParserCopy (SanitizedBuffer, &SanitizedPosition, SizeToSanitize, Chr8);
        State = ParserNormal;
        break;
      }
      break;

    case ParserDotDot: // two dots seen since last slash
      switch (Chr8) {
      case '\0':
        ParserRewindDotDot (SanitizedBuffer, &SanitizedPosition, RootEscape);
        ParserStripSlash (SanitizedBuffer, &SanitizedPosition);
        ParserCopy (SanitizedBuffer, &SanitizedPosition, SizeToSanitize, Chr8);
        State = ParserEnd;
        break;
      case '/':
        ParserRewindDotDot (SanitizedBuffer, &SanitizedPosition, RootEscape);
        State = ParserSlash;
        break;
      case '.':
        //
        // fall through
        //
      default:
        ParserCopy (SanitizedBuffer, &SanitizedPosition, SizeToSanitize, Chr8);
        State = ParserNormal;
        break;
      }
      break;

    case ParserNormal: // a different sequence seen
      switch (Chr8) {
      case '\0':
        ParserCopy (SanitizedBuffer, &SanitizedPosition, SizeToSanitize, Chr8);
        State = ParserEnd;
        break;
      case '/':
        ParserCopy (SanitizedBuffer, &SanitizedPosition, SizeToSanitize, Chr8);
        State = ParserSlash;
        break;
      case '.':
        //
        // fall through
        //
      default:
        //
        // copy and stay in same state
        //
        ParserCopy (SanitizedBuffer, &SanitizedPosition, SizeToSanitize, Chr8);
        break;
      }
      break;

    default:
      ASSERT (FALSE);
      break;
    }
  } while (State != ParserEnd);

  //
  // Ensure length invariant on ResultPath8.
  //
  ASSERT (SanitizedPosition >= 2);
  if (SanitizedPosition - 1 > VIRTIO_FS_MAX_PATHNAME_LENGTH) {
    Status = EFI_OUT_OF_RESOURCES;
    goto FreeSanitizedBuffer;
  }

  *ResultPath8    = SanitizedBuffer;
  SanitizedBuffer = NULL;
  Status          = EFI_SUCCESS;
  //
  // Fall through.
  //
FreeSanitizedBuffer:
  if (SanitizedBuffer != NULL) {
    FreePool (SanitizedBuffer);
  }

FreeBufferToSanitize:
  if (RhsPath8[0] != '/') {
    FreePool (BufferToSanitize);
  }

FreeRhsPath8:
  FreePool (RhsPath8);
  return Status;
}

/**
  For a given canonical pathname (as defined at VirtioFsAppendPath()), look up
  the NodeId of the most specific parent directory, plus output a pointer to
  the last pathname component (which is therefore a direct child of said parent
  directory).

  The function may only be called after VirtioFsFuseInitSession() returns
  successfully and before VirtioFsUninit() is called.

  @param[in,out] VirtioFs    The Virtio Filesystem device to send FUSE_LOOKUP
                             and FUSE_FORGET requests to. On output, the FUSE
                             request counter "VirtioFs->RequestId" will have
                             been incremented several times.

  @param[in,out] Path        The canonical pathname (as defined in the
                             description of VirtioFsAppendPath()) to split.
                             Path is modified in-place temporarily; however, on
                             return (successful or otherwise), Path reassumes
                             its original contents.

  @param[out] DirNodeId      The NodeId of the most specific parent directory
                             identified by Path. The caller is responsible for
                             sending a FUSE_FORGET request to the Virtio
                             Filesystem device for DirNodeId -- unless
                             DirNodeId equals VIRTIO_FS_FUSE_ROOT_DIR_NODE_ID
                             --, when DirNodeId's use ends.

  @param[out] LastComponent  A pointer into Path, pointing at the start of the
                             last pathname component.

  @retval EFI_SUCCESS            Splitting successful.

  @retval EFI_INVALID_PARAMETER  Path is "/".

  @retval EFI_ACCESS_DENIED      One of the components on Path before the last
                                 is not a directory.

  @return                        Error codes propagated from
                                 VirtioFsFuseLookup() and
                                 VirtioFsFuseAttrToEfiFileInfo().
**/
EFI_STATUS
VirtioFsLookupMostSpecificParentDir (
  IN OUT VIRTIO_FS *VirtioFs,
  IN OUT CHAR8     *Path,
     OUT UINT64    *DirNodeId,
     OUT CHAR8     **LastComponent
  )
{
  UINT64     ParentDirNodeId;
  CHAR8      *Slash;
  EFI_STATUS Status;
  UINT64     NextDirNodeId;

  if (AsciiStrCmp (Path, "/") == 0) {
    return EFI_INVALID_PARAMETER;
  }

  ParentDirNodeId = VIRTIO_FS_FUSE_ROOT_DIR_NODE_ID;
  Slash           = Path;
  for (;;) {
    CHAR8                              *NextSlash;
    VIRTIO_FS_FUSE_ATTRIBUTES_RESPONSE FuseAttr;
    EFI_FILE_INFO                      FileInfo;

    //
    // Find the slash (if any) that terminates the next pathname component.
    //
    NextSlash = AsciiStrStr (Slash + 1, "/");
    if (NextSlash == NULL) {
      break;
    }

    //
    // Temporarily replace the found slash character with a NUL in-place, for
    // easy construction of the single-component filename that we need to look
    // up.
    //
    *NextSlash = '\0';
    Status = VirtioFsFuseLookup (VirtioFs, ParentDirNodeId, Slash + 1,
               &NextDirNodeId, &FuseAttr);
    *NextSlash = '/';

    //
    // We're done with the directory inode that was the basis for the lookup.
    //
    if (ParentDirNodeId != VIRTIO_FS_FUSE_ROOT_DIR_NODE_ID) {
      VirtioFsFuseForget (VirtioFs, ParentDirNodeId);
    }

    //
    // If we couldn't look up the next *non-final* pathname component, bail.
    //
    if (EFI_ERROR (Status)) {
      return Status;
    }

    //
    // Lookup successful; now check if the next (non-final) component is a
    // directory. If not, bail.
    //
    Status = VirtioFsFuseAttrToEfiFileInfo (&FuseAttr, &FileInfo);
    if (EFI_ERROR (Status)) {
      goto ForgetNextDirNodeId;
    }
    if ((FileInfo.Attribute & EFI_FILE_DIRECTORY) == 0) {
      Status = EFI_ACCESS_DENIED;
      goto ForgetNextDirNodeId;
    }

    //
    // Advance.
    //
    ParentDirNodeId = NextDirNodeId;
    Slash           = NextSlash;
  }

  //
  // ParentDirNodeId corresponds to the last containing directory. The
  // remaining single-component filename represents a direct child under that
  // directory. Said filename starts at (Slash + 1).
  //
  *DirNodeId     = ParentDirNodeId;
  *LastComponent = Slash + 1;
  return EFI_SUCCESS;

ForgetNextDirNodeId:
  VirtioFsFuseForget (VirtioFs, NextDirNodeId);
  return Status;
}

/**
  Format the last component of a canonical pathname into a caller-provided
  CHAR16 array.

  @param[in] Path              The canonical pathname (as defined in the
                               description of VirtioFsAppendPath()) to format
                               the last component of.

  @param[out] Basename         If BasenameSize is zero on input, Basename may
                               be NULL. Otherwise, Basename is allocated by the
                               caller. On successful return, Basename contains
                               the last component of Path, formatted as a
                               NUL-terminated CHAR16 string. When Path is "/"
                               on input, Basename is L"" on output.

  @param[in,out] BasenameSize  On input, the number of bytes the caller
                               provides in Basename. On output, regardless of
                               return value, the number of bytes required for
                               formatting Basename, including the terminating
                               L'\0'.

  @retval EFI_SUCCESS           Basename has been filled in.

  @retval EFI_BUFFER_TOO_SMALL  BasenameSize was too small on input; Basename
                                has not been modified.
**/
EFI_STATUS
VirtioFsGetBasename (
  IN     CHAR8  *Path,
     OUT CHAR16 *Basename     OPTIONAL,
  IN OUT UINTN  *BasenameSize
  )
{
  UINTN AllocSize;
  UINTN LastComponent;
  UINTN Idx;
  UINTN PathSize;

  AllocSize = *BasenameSize;

  LastComponent = MAX_UINTN;
  for (Idx = 0; Path[Idx] != '\0'; Idx++) {
    if (Path[Idx] == '/') {
      LastComponent = Idx;
    }
  }
  PathSize = Idx + 1;
  ASSERT (LastComponent < MAX_UINTN);
  LastComponent++;
  *BasenameSize = (PathSize - LastComponent) * sizeof Basename[0];

  if (*BasenameSize > AllocSize) {
    return EFI_BUFFER_TOO_SMALL;
  }

  for (Idx = LastComponent; Idx < PathSize; Idx++) {
    Basename[Idx - LastComponent] = Path[Idx];
  }
  return EFI_SUCCESS;
}

/**
  Format the destination of a rename/move operation as a dynamically allocated
  canonical pathname.

  Any dot-dot in RhsPath16 that would remove the root directory is dropped, and
  reported through RootEscape, without failing the function call.

  @param[in] LhsPath8     The source pathname operand of the rename/move
                          operation, expressed as a canonical pathname (as
                          defined in the description of VirtioFsAppendPath()).
                          The root directory "/" cannot be renamed/moved, and
                          will be rejected.

  @param[in] RhsPath16    The destination pathname operand expressed as a
                          UEFI-style CHAR16 pathname.

                          If RhsPath16 starts with a backslash, then RhsPath16
                          is considered absolute. Otherwise, RhsPath16 is
                          interpreted relative to the most specific parent
                          directory found in LhsPath8.

                          Independently, if RhsPath16 ends with a backslash
                          (i.e., RhsPath16 is given in the "move into
                          directory" convenience form), then RhsPath16 is
                          interpreted with the basename of LhsPath8 appended.
                          Otherwise, the last pathname component of RhsPath16
                          is taken as the last pathname component of the
                          rename/move destination.

                          An empty RhsPath16 is rejected.

  @param[out] ResultPath8  The POSIX-style, canonical format pathname that
                           leads to the renamed/moved file. After use, the
                           caller is responsible for freeing ResultPath8.

  @param[out] RootEscape   Set to TRUE if at least one dot-dot component in
                           RhsPath16 attempted to escape the root directory;
                           set to FALSE otherwise.

  @retval EFI_SUCCESS            ResultPath8 has been produced. RootEscape has
                                 been output.

  @retval EFI_INVALID_PARAMETER  LhsPath8 is "/".

  @retval EFI_INVALID_PARAMETER  RhsPath16 is zero-length.

  @retval EFI_INVALID_PARAMETER  RhsPath16 failed the
                                 VIRTIO_FS_MAX_PATHNAME_LENGTH check.

  @retval EFI_OUT_OF_RESOURCES   Memory allocation failed.

  @retval EFI_OUT_OF_RESOURCES   ResultPath8 would have failed the
                                 VIRTIO_FS_MAX_PATHNAME_LENGTH check.

  @retval EFI_UNSUPPORTED        RhsPath16 contains a character that either
                                 falls outside of the printable ASCII set, or
                                 is a forward slash.
**/
EFI_STATUS
VirtioFsComposeRenameDestination (
  IN     CHAR8   *LhsPath8,
  IN     CHAR16  *RhsPath16,
     OUT CHAR8   **ResultPath8,
     OUT BOOLEAN *RootEscape
  )
{
  //
  // Lengths are expressed as numbers of characters (CHAR8 or CHAR16),
  // excluding terminating NULs. Sizes are expressed as byte counts, including
  // the bytes taken up by terminating NULs.
  //
  UINTN      RhsLen;
  UINTN      LhsBasename16Size;
  EFI_STATUS Status;
  UINTN      LhsBasenameLen;
  UINTN      DestSuffix16Size;
  CHAR16     *DestSuffix16;
  CHAR8      *DestPrefix8;

  //
  // An empty destination operand for the rename/move operation is not allowed.
  //
  RhsLen = StrLen (RhsPath16);
  if (RhsLen == 0) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Enforce length restriction on RhsPath16.
  //
  if (RhsLen > VIRTIO_FS_MAX_PATHNAME_LENGTH) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Determine the length of the basename of LhsPath8.
  //
  LhsBasename16Size = 0;
  Status = VirtioFsGetBasename (LhsPath8, NULL, &LhsBasename16Size);
  ASSERT (Status == EFI_BUFFER_TOO_SMALL);
  ASSERT (LhsBasename16Size >= sizeof (CHAR16));
  ASSERT (LhsBasename16Size % sizeof (CHAR16) == 0);
  LhsBasenameLen = LhsBasename16Size / sizeof (CHAR16) - 1;
  if (LhsBasenameLen == 0) {
    //
    // The root directory cannot be renamed/moved.
    //
    return EFI_INVALID_PARAMETER;
  }

  //
  // Resolve the "move into directory" convenience form in RhsPath16.
  //
  if (RhsPath16[RhsLen - 1] == L'\\') {
    //
    // Append the basename of LhsPath8 as a CHAR16 string to RhsPath16.
    //
    DestSuffix16Size = RhsLen * sizeof (CHAR16) + LhsBasename16Size;
    DestSuffix16 = AllocatePool (DestSuffix16Size);
    if (DestSuffix16 == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    CopyMem (DestSuffix16, RhsPath16, RhsLen * sizeof (CHAR16));
    Status = VirtioFsGetBasename (LhsPath8, DestSuffix16 + RhsLen,
               &LhsBasename16Size);
    ASSERT_EFI_ERROR (Status);
  } else {
    //
    // Just create a copy of RhsPath16.
    //
    DestSuffix16Size = (RhsLen + 1) * sizeof (CHAR16);
    DestSuffix16 = AllocateCopyPool (DestSuffix16Size, RhsPath16);
    if (DestSuffix16 == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
  }

  //
  // If the destination operand is absolute, it will be interpreted relative to
  // the root directory.
  //
  // Otherwise (i.e., if the destination operand is relative), then create the
  // canonical pathname that the destination operand is interpreted relatively
  // to; that is, the canonical pathname of the most specific parent directory
  // found in LhsPath8.
  //
  if (DestSuffix16[0] == L'\\') {
    DestPrefix8 = AllocateCopyPool (sizeof "/", "/");
    if (DestPrefix8 == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto FreeDestSuffix16;
    }
  } else {
    UINTN LhsLen;
    UINTN DestPrefixLen;

    //
    // Strip the basename of LhsPath8.
    //
    LhsLen = AsciiStrLen (LhsPath8);
    ASSERT (LhsBasenameLen < LhsLen);
    DestPrefixLen = LhsLen - LhsBasenameLen;
    ASSERT (LhsPath8[DestPrefixLen - 1] == '/');
    //
    // If we're not at the root directory, strip the slash too.
    //
    if (DestPrefixLen > 1) {
      DestPrefixLen--;
    }
    DestPrefix8 = AllocatePool (DestPrefixLen + 1);
    if (DestPrefix8 == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto FreeDestSuffix16;
    }
    CopyMem (DestPrefix8, LhsPath8, DestPrefixLen);
    DestPrefix8[DestPrefixLen] = '\0';
  }

  //
  // Now combine DestPrefix8 and DestSuffix16 into the final canonical
  // pathname.
  //
  Status = VirtioFsAppendPath (DestPrefix8, DestSuffix16, ResultPath8,
             RootEscape);

  FreePool (DestPrefix8);
  //
  // Fall through.
  //
FreeDestSuffix16:
  FreePool (DestSuffix16);

  return Status;
}

/**
  Convert select fields of a VIRTIO_FS_FUSE_ATTRIBUTES_RESPONSE object to
  corresponding fields in EFI_FILE_INFO.

  @param[in] FuseAttr   The VIRTIO_FS_FUSE_ATTRIBUTES_RESPONSE object to
                        convert the relevant fields from.

  @param[out] FileInfo  The EFI_FILE_INFO structure to modify. Importantly, the
                        FileInfo->Size and FileInfo->FileName fields are not
                        overwritten.

  @retval EFI_SUCCESS      Conversion successful.

  @retval EFI_UNSUPPORTED  The allocated size of the file is inexpressible in
                           EFI_FILE_INFO.

  @retval EFI_UNSUPPORTED  One of the file access times is inexpressible in
                           EFI_FILE_INFO.

  @retval EFI_UNSUPPORTED  The file type is inexpressible in EFI_FILE_INFO.

  @retval EFI_UNSUPPORTED  The file is a regular file that has multiple names
                           on the host side (i.e., its hard link count is
                           greater than one).
**/
EFI_STATUS
VirtioFsFuseAttrToEfiFileInfo (
  IN     VIRTIO_FS_FUSE_ATTRIBUTES_RESPONSE *FuseAttr,
     OUT EFI_FILE_INFO                      *FileInfo
  )
{
  UINT64   EpochTime[3];
  EFI_TIME *ConvertedTime[ARRAY_SIZE (EpochTime)];
  UINTN    Idx;

  FileInfo->FileSize = FuseAttr->Size;

  //
  // The unit for FuseAttr->Blocks is 512B.
  //
  if (FuseAttr->Blocks >= BIT55) {
    return EFI_UNSUPPORTED;
  }
  FileInfo->PhysicalSize = LShiftU64 (FuseAttr->Blocks, 9);

  //
  // Convert the timestamps. File creation time is not tracked by the Virtio
  // Filesystem device, so set FileInfo->CreateTime from FuseAttr->Mtime as
  // well.
  //
  EpochTime[0]     = FuseAttr->Mtime;
  EpochTime[1]     = FuseAttr->Atime;
  EpochTime[2]     = FuseAttr->Mtime;
  ConvertedTime[0] = &FileInfo->CreateTime;
  ConvertedTime[1] = &FileInfo->LastAccessTime;
  ConvertedTime[2] = &FileInfo->ModificationTime;

  for (Idx = 0; Idx < ARRAY_SIZE (EpochTime); Idx++) {
    //
    // EpochToEfiTime() takes a UINTN for seconds.
    //
    if (EpochTime[Idx] > MAX_UINTN) {
      return EFI_UNSUPPORTED;
    }
    //
    // Set the following fields in the converted time: Year, Month, Day, Hour,
    // Minute, Second, Nanosecond.
    //
    EpochToEfiTime ((UINTN)EpochTime[Idx], ConvertedTime[Idx]);
    //
    // The times are all expressed in UTC. Consequently, they are not affected
    // by daylight saving.
    //
    ConvertedTime[Idx]->TimeZone = 0;
    ConvertedTime[Idx]->Daylight = 0;
    //
    // Clear the padding fields.
    //
    ConvertedTime[Idx]->Pad1 = 0;
    ConvertedTime[Idx]->Pad2 = 0;
  }

  //
  // Set the attributes.
  //
  switch (FuseAttr->Mode & VIRTIO_FS_FUSE_MODE_TYPE_MASK) {
  case VIRTIO_FS_FUSE_MODE_TYPE_DIR:
    FileInfo->Attribute = EFI_FILE_DIRECTORY;
    break;
  case VIRTIO_FS_FUSE_MODE_TYPE_REG:
    FileInfo->Attribute = 0;
    break;
  default:
    //
    // Other file types are not supported.
    //
    return EFI_UNSUPPORTED;
  }
  //
  // Report the regular file or directory as read-only if all classes lack
  // write permission.
  //
  if ((FuseAttr->Mode & (VIRTIO_FS_FUSE_MODE_PERM_WUSR |
                         VIRTIO_FS_FUSE_MODE_PERM_WGRP |
                         VIRTIO_FS_FUSE_MODE_PERM_WOTH)) == 0) {
    FileInfo->Attribute |= EFI_FILE_READ_ONLY;
  }

  //
  // A hard link count greater than 1 is not supported for regular files.
  //
  if ((FileInfo->Attribute & EFI_FILE_DIRECTORY) == 0 && FuseAttr->Nlink > 1) {
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

/**
  Convert a VIRTIO_FS_FUSE_DIRENTPLUS_RESPONSE filename to an EFI_FILE_INFO
  filename.

  @param[in] FuseDirent    The VIRTIO_FS_FUSE_DIRENTPLUS_RESPONSE object to
                           convert the filename byte array from. The caller is
                           responsible for ensuring that FuseDirent->Namelen
                           describe valid storage.

  @param[in,out] FileInfo  The EFI_FILE_INFO structure to modify. On input, the
                           caller is responsible for setting FileInfo->Size
                           according to the allocated size. On successful
                           return, FileInfo->Size is reduced to reflect the
                           filename converted into FileInfo->FileName.
                           FileInfo->FileName is set from the filename byte
                           array that directly follows the FuseDirent header
                           object. Fields other than FileInfo->Size and
                           FileInfo->FileName are not modified.

  @retval EFI_SUCCESS            Conversion successful.

  @retval EFI_INVALID_PARAMETER  VIRTIO_FS_FUSE_DIRENTPLUS_RESPONSE_SIZE()
                                 returns zero for FuseDirent->Namelen.

  @retval EFI_BUFFER_TOO_SMALL   On input, FileInfo->Size does not provide
                                 enough room for converting the filename byte
                                 array from FuseDirent.

  @retval EFI_UNSUPPORTED        The FuseDirent filename byte array contains a
                                 byte that falls outside of the printable ASCII
                                 range, or is a forward slash or a backslash.
**/
EFI_STATUS
VirtioFsFuseDirentPlusToEfiFileInfo (
  IN     VIRTIO_FS_FUSE_DIRENTPLUS_RESPONSE *FuseDirent,
  IN OUT EFI_FILE_INFO                      *FileInfo
  )
{
  UINTN  DirentSize;
  UINTN  FileInfoSize;
  UINT8  *DirentName;
  UINT32 Idx;

  DirentSize = VIRTIO_FS_FUSE_DIRENTPLUS_RESPONSE_SIZE (FuseDirent->Namelen);
  if (DirentSize == 0) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // We're now safe from overflow in the calculation below.
  //
  FileInfoSize = (OFFSET_OF (EFI_FILE_INFO, FileName) +
                  ((UINTN)FuseDirent->Namelen + 1) * sizeof (CHAR16));
  if (FileInfoSize > FileInfo->Size) {
    return EFI_BUFFER_TOO_SMALL;
  }

  //
  // Convert the name.
  //
  DirentName = (UINT8 *)(FuseDirent + 1);
  for (Idx = 0; Idx < FuseDirent->Namelen; Idx++) {
    UINT8 NameByte;

    NameByte = DirentName[Idx];
    if (NameByte < 0x20 || NameByte > 0x7E ||
        NameByte == '/' || NameByte == '\\') {
      return EFI_UNSUPPORTED;
    }
    FileInfo->FileName[Idx] = (CHAR16)NameByte;
  }
  FileInfo->FileName[Idx++] = L'\0';
  //
  // Set the (possibly reduced) size.
  //
  FileInfo->Size = FileInfoSize;

  return EFI_SUCCESS;
}

/**
  Given an EFI_FILE_INFO object received in an EFI_FILE_PROTOCOL.SetInfo()
  call, determine whether updating the size of the file is necessary, relative
  to an EFI_FILE_INFO object describing the current state of the file.

  @param[in] Info     The EFI_FILE_INFO describing the current state of the
                      file. The caller is responsible for populating Info on
                      input with VirtioFsFuseAttrToEfiFileInfo(), from the
                      current FUSE attributes of the file. The Info->Size and
                      Info->FileName members are ignored.

  @param[in] NewInfo  The EFI_FILE_INFO object received in the
                      EFI_FILE_PROTOCOL.SetInfo() call.

  @param[out] Update  Set to TRUE on output if the file size needs to be
                      updated. Set to FALSE otherwise.

  @param[out] Size    If Update is set to TRUE, then Size provides the new file
                      size to set. Otherwise, Size is not written to.
**/
VOID
VirtioFsGetFuseSizeUpdate (
  IN     EFI_FILE_INFO *Info,
  IN     EFI_FILE_INFO *NewInfo,
     OUT BOOLEAN       *Update,
     OUT UINT64        *Size
  )
{
  BOOLEAN IsDirectory;

  IsDirectory = (BOOLEAN)((Info->Attribute & EFI_FILE_DIRECTORY) != 0);

  if (IsDirectory || Info->FileSize == NewInfo->FileSize) {
    *Update = FALSE;
    return;
  }
  *Update = TRUE;
  *Size = NewInfo->FileSize;
}

/**
  Given an EFI_FILE_INFO object received in an EFI_FILE_PROTOCOL.SetInfo()
  call, determine whether updating the last access time and/or the last
  modification time of the file is necessary, relative to an EFI_FILE_INFO
  object describing the current state of the file.

  @param[in] Info          The EFI_FILE_INFO describing the current state of
                           the file. The caller is responsible for populating
                           Info on input with VirtioFsFuseAttrToEfiFileInfo(),
                           from the current FUSE attributes of the file. The
                           Info->Size and Info->FileName members are ignored.

  @param[in] NewInfo       The EFI_FILE_INFO object received in the
                           EFI_FILE_PROTOCOL.SetInfo() call.

  @param[out] UpdateAtime  Set to TRUE on output if the last access time needs
                           to be updated. Set to FALSE otherwise.

  @param[out] UpdateMtime  Set to TRUE on output if the last modification time
                           needs to be updated. Set to FALSE otherwise.

  @param[out] Atime        If UpdateAtime is set to TRUE, then Atime provides
                           the last access timestamp to set (as seconds since
                           the Epoch). Otherwise, Atime is not written to.

  @param[out] Mtime        If UpdateMtime is set to TRUE, then Mtime provides
                           the last modification timestamp to set (as seconds
                           since the Epoch). Otherwise, Mtime is not written
                           to.

  @retval EFI_SUCCESS            Output parameters have been set successfully.

  @retval EFI_INVALID_PARAMETER  At least one of the CreateTime, LastAccessTime
                                 and ModificationTime fields in NewInfo
                                 represents an actual update relative to the
                                 current state of the file (expressed in Info),
                                 but does not satisfy the UEFI spec
                                 requirements on EFI_TIME.

  @retval EFI_ACCESS_DENIED      NewInfo requests changing both CreateTime and
                                 ModificationTime, but to values that differ
                                 from each other. The Virtio Filesystem device
                                 does not support this.
**/
EFI_STATUS
VirtioFsGetFuseTimeUpdates (
  IN     EFI_FILE_INFO *Info,
  IN     EFI_FILE_INFO *NewInfo,
     OUT BOOLEAN       *UpdateAtime,
     OUT BOOLEAN       *UpdateMtime,
     OUT UINT64        *Atime,
     OUT UINT64        *Mtime
  )
{
  EFI_TIME              *Time[3];
  EFI_TIME              *NewTime[ARRAY_SIZE (Time)];
  UINTN                 Idx;
  STATIC CONST EFI_TIME ZeroTime = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
  BOOLEAN               Change[ARRAY_SIZE (Time)];
  UINT64                Seconds[ARRAY_SIZE (Time)];

  Time[0]    = &Info->CreateTime;
  Time[1]    = &Info->LastAccessTime;
  Time[2]    = &Info->ModificationTime;
  NewTime[0] = &NewInfo->CreateTime;
  NewTime[1] = &NewInfo->LastAccessTime;
  NewTime[2] = &NewInfo->ModificationTime;

  //
  // Determine which timestamps differ from the current state. (A zero time
  // means "don't update", per UEFI spec.) For each timestamp that's being
  // changed, calculate the seconds since the Epoch.
  //
  for (Idx = 0; Idx < ARRAY_SIZE (Time); Idx++) {
    if (CompareMem (NewTime[Idx], &ZeroTime, sizeof (EFI_TIME)) == 0 ||
        CompareMem (NewTime[Idx], Time[Idx], sizeof (EFI_TIME)) == 0) {
      Change[Idx] = FALSE;
    } else {
      if (!IsTimeValid (NewTime[Idx])) {
        return EFI_INVALID_PARAMETER;
      }
      Change[Idx] = TRUE;
      Seconds[Idx] = EfiTimeToEpoch (NewTime[Idx]);
    }
  }

  //
  // If a change is requested for exactly one of CreateTime and
  // ModificationTime, we'll change the last modification time. If changes are
  // requested for both, and to the same timestamp, we'll similarly update the
  // last modification time. If changes are requested for both, but to
  // different timestamps, we reject the request.
  //
  if (Change[0] && Change[2] && Seconds[0] != Seconds[2]) {
    return EFI_ACCESS_DENIED;
  }

  *UpdateAtime = FALSE;
  *UpdateMtime = FALSE;

  if (Change[0]) {
    *UpdateMtime = TRUE;
    *Mtime = Seconds[0];
  }
  if (Change[1]) {
    *UpdateAtime = TRUE;
    *Atime = Seconds[1];
  }
  if (Change[2]) {
    *UpdateMtime = TRUE;
    *Mtime = Seconds[2];
  }

  return EFI_SUCCESS;
}

/**
  Given an EFI_FILE_INFO object received in an EFI_FILE_PROTOCOL.SetInfo()
  call, determine whether updating the file mode bits of the file is necessary,
  relative to an EFI_FILE_INFO object describing the current state of the file.

  @param[in] Info     The EFI_FILE_INFO describing the current state of the
                      file. The caller is responsible for populating Info on
                      input with VirtioFsFuseAttrToEfiFileInfo(), from the
                      current FUSE attributes of the file. The Info->Size and
                      Info->FileName members are ignored.

  @param[in] NewInfo  The EFI_FILE_INFO object received in the
                      EFI_FILE_PROTOCOL.SetInfo() call.

  @param[out] Update  Set to TRUE on output if the file mode bits need to be
                      updated. Set to FALSE otherwise.

  @param[out] Mode    If Update is set to TRUE, then Mode provides the file
                      mode bits to set. Otherwise, Mode is not written to.

  @retval EFI_SUCCESS        Output parameters have been set successfully.

  @retval EFI_ACCESS_DENIED  NewInfo requests toggling an unknown bit in the
                             Attribute bitmask.

  @retval EFI_ACCESS_DENIED  NewInfo requests toggling EFI_FILE_DIRECTORY in
                             the Attribute bitmask.
**/
EFI_STATUS
VirtioFsGetFuseModeUpdate (
  IN     EFI_FILE_INFO *Info,
  IN     EFI_FILE_INFO *NewInfo,
     OUT BOOLEAN       *Update,
     OUT UINT32        *Mode
     )
{
  UINT64  Toggle;
  BOOLEAN IsDirectory;
  BOOLEAN IsWriteable;
  BOOLEAN WillBeWriteable;

  Toggle = Info->Attribute ^ NewInfo->Attribute;
  if ((Toggle & ~EFI_FILE_VALID_ATTR) != 0) {
    //
    // Unknown attribute requested.
    //
    return EFI_ACCESS_DENIED;
  }
  if ((Toggle & EFI_FILE_DIRECTORY) != 0) {
    //
    // EFI_FILE_DIRECTORY cannot be toggled.
    //
    return EFI_ACCESS_DENIED;
  }

  IsDirectory     = (BOOLEAN)((Info->Attribute    & EFI_FILE_DIRECTORY) != 0);
  IsWriteable     = (BOOLEAN)((Info->Attribute    & EFI_FILE_READ_ONLY) == 0);
  WillBeWriteable = (BOOLEAN)((NewInfo->Attribute & EFI_FILE_READ_ONLY) == 0);

  if (IsWriteable == WillBeWriteable) {
    *Update = FALSE;
    return EFI_SUCCESS;
  }

  if (IsDirectory) {
    if (WillBeWriteable) {
      *Mode = (VIRTIO_FS_FUSE_MODE_PERM_RWXU |
               VIRTIO_FS_FUSE_MODE_PERM_RWXG |
               VIRTIO_FS_FUSE_MODE_PERM_RWXO);
    } else {
      *Mode = (VIRTIO_FS_FUSE_MODE_PERM_RUSR |
               VIRTIO_FS_FUSE_MODE_PERM_XUSR |
               VIRTIO_FS_FUSE_MODE_PERM_RGRP |
               VIRTIO_FS_FUSE_MODE_PERM_XGRP |
               VIRTIO_FS_FUSE_MODE_PERM_ROTH |
               VIRTIO_FS_FUSE_MODE_PERM_XOTH);
    }
  } else {
    if (WillBeWriteable) {
      *Mode = (VIRTIO_FS_FUSE_MODE_PERM_RUSR |
               VIRTIO_FS_FUSE_MODE_PERM_WUSR |
               VIRTIO_FS_FUSE_MODE_PERM_RGRP |
               VIRTIO_FS_FUSE_MODE_PERM_WGRP |
               VIRTIO_FS_FUSE_MODE_PERM_ROTH |
               VIRTIO_FS_FUSE_MODE_PERM_WOTH);
    } else {
      *Mode = (VIRTIO_FS_FUSE_MODE_PERM_RUSR |
               VIRTIO_FS_FUSE_MODE_PERM_RGRP |
               VIRTIO_FS_FUSE_MODE_PERM_ROTH);
    }
  }
  *Update = TRUE;
  return EFI_SUCCESS;
}
