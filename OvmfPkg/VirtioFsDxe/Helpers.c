/** @file
  Initialization and helper routines for the Virtio Filesystem device.

  Copyright (C) 2020, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

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
                          to.

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
  IN     UINT32                 Opcode,
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
