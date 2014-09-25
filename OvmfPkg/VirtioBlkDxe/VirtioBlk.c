/** @file

  This driver produces Block I/O Protocol instances for virtio-blk devices.

  The implementation is basic:

  - No attach/detach (ie. removable media).

  - Although the non-blocking interfaces of EFI_BLOCK_IO2_PROTOCOL could be a
    good match for multiple in-flight virtio-blk requests, we stick to
    synchronous requests and EFI_BLOCK_IO_PROTOCOL for now.

  Copyright (C) 2012, Red Hat, Inc.
  Copyright (c) 2012 - 2014, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution. The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, WITHOUT
  WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <IndustryStandard/VirtioBlk.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/VirtioLib.h>

#include "VirtioBlk.h"

/**

  Convenience macros to read and write region 0 IO space elements of the
  virtio-blk device, for configuration purposes.

  The following macros make it possible to specify only the "core parameters"
  for such accesses and to derive the rest. By the time VIRTIO_CFG_WRITE()
  returns, the transaction will have been completed.

  @param[in] Dev       Pointer to the VBLK_DEV structure whose VirtIo space
                       we're accessing. Dev->VirtIo must be valid.

  @param[in] Field     A field name from VBLK_HDR, identifying the virtio-blk
                       configuration item to access.

  @param[in] Value     (VIRTIO_CFG_WRITE() only.) The value to write to the
                       selected configuration item.

  @param[out] Pointer  (VIRTIO_CFG_READ() only.) The object to receive the
                       value read from the configuration item. Its type must be
                       one of UINT8, UINT16, UINT32, UINT64.


  @return  Status code returned by Virtio->WriteDevice() /
           Virtio->ReadDevice().

**/

#define VIRTIO_CFG_WRITE(Dev, Field, Value)  ((Dev)->VirtIo->WriteDevice ( \
                                                (Dev)->VirtIo,             \
                                                OFFSET_OF_VBLK (Field),    \
                                                SIZE_OF_VBLK (Field),      \
                                                (Value)                    \
                                                ))

#define VIRTIO_CFG_READ(Dev, Field, Pointer) ((Dev)->VirtIo->ReadDevice (  \
                                                (Dev)->VirtIo,             \
                                                OFFSET_OF_VBLK (Field),    \
                                                SIZE_OF_VBLK (Field),      \
                                                sizeof *(Pointer),         \
                                                (Pointer)                  \
                                                ))


//
// UEFI Spec 2.3.1 + Errata C, 12.8 EFI Block I/O Protocol
// Driver Writer's Guide for UEFI 2.3.1 v1.01,
//   24.2 Block I/O Protocol Implementations
//
EFI_STATUS
EFIAPI
VirtioBlkReset (
  IN EFI_BLOCK_IO_PROTOCOL *This,
  IN BOOLEAN               ExtendedVerification
  )
{
  //
  // If we managed to initialize and install the driver, then the device is
  // working correctly.
  //
  return EFI_SUCCESS;
}

/**

  Verify correctness of the read/write (not flush) request submitted to the
  EFI_BLOCK_IO_PROTOCOL instance.

  This function provides most verification steps described in:

    UEFI Spec 2.3.1 + Errata C, 12.8 EFI Block I/O Protocol, 12.8 EFI Block I/O
    Protocol,
    - EFI_BLOCK_IO_PROTOCOL.ReadBlocks()
    - EFI_BLOCK_IO_PROTOCOL.WriteBlocks()

    Driver Writer's Guide for UEFI 2.3.1 v1.01,
    - 24.2.2. ReadBlocks() and ReadBlocksEx() Implementation
    - 24.2.3 WriteBlocks() and WriteBlockEx() Implementation

  Request sizes are limited to 1 GB (checked). This is not a practical
  limitation, just conformance to virtio-0.9.5, 2.3.2 Descriptor Table: "no
  descriptor chain may be more than 2^32 bytes long in total".

  Some Media characteristics are hardcoded in VirtioBlkInit() below (like
  non-removable media, no restriction on buffer alignment etc); we rely on
  those here without explicit mention.

  @param[in] Media               The EFI_BLOCK_IO_MEDIA characteristics for
                                 this driver instance, extracted from the
                                 underlying virtio-blk device at initialization
                                 time. We validate the request against this set
                                 of attributes.


  @param[in] Lba                 Logical Block Address: number of logical
                                 blocks to skip from the beginning of the
                                 device.

  @param[in] PositiveBufferSize  Size of buffer to transfer, in bytes. The
                                 caller is responsible to ensure this parameter
                                 is positive.

  @param[in] RequestIsWrite      TRUE iff data transfer goes from guest to
                                 device.


  @@return                       Validation result to be forwarded outwards by
                                 ReadBlocks() and WriteBlocks, as required by
                                 the specs above.

**/
STATIC
EFI_STATUS
EFIAPI
VerifyReadWriteRequest (
  IN  EFI_BLOCK_IO_MEDIA *Media,
  IN  EFI_LBA            Lba,
  IN  UINTN              PositiveBufferSize,
  IN  BOOLEAN            RequestIsWrite
  )
{
  UINTN BlockCount;

  ASSERT (PositiveBufferSize > 0);

  if (PositiveBufferSize > SIZE_1GB ||
      PositiveBufferSize % Media->BlockSize > 0) {
    return EFI_BAD_BUFFER_SIZE;
  }
  BlockCount = PositiveBufferSize / Media->BlockSize;

  //
  // Avoid unsigned wraparound on either side in the second comparison.
  //
  if (Lba > Media->LastBlock || BlockCount - 1 > Media->LastBlock - Lba) {
    return EFI_INVALID_PARAMETER;
  }

  if (RequestIsWrite && Media->ReadOnly) {
    return EFI_WRITE_PROTECTED;
  }

  return EFI_SUCCESS;
}




/**

  Format a read / write / flush request as three consecutive virtio
  descriptors, push them to the host, and poll for the response.

  This is the main workhorse function. Two use cases are supported, read/write
  and flush. The function may only be called after the request parameters have
  been verified by
  - specific checks in ReadBlocks() / WriteBlocks() / FlushBlocks(), and
  - VerifyReadWriteRequest() (for read/write only).

  Parameters handled commonly:

    @param[in] Dev             The virtio-blk device the request is targeted
                               at.

  Flush request:

    @param[in] Lba             Must be zero.

    @param[in] BufferSize      Must be zero.

    @param[in out] Buffer      Ignored by the function.

    @param[in] RequestIsWrite  Must be TRUE.

  Read/Write request:

    @param[in] Lba             Logical Block Address: number of logical blocks
                               to skip from the beginning of the device.

    @param[in] BufferSize      Size of buffer to transfer, in bytes. The caller
                               is responsible to ensure this parameter is
                               positive.

    @param[in out] Buffer      The guest side area to read data from the device
                               into, or write data to the device from.

    @param[in] RequestIsWrite  TRUE iff data transfer goes from guest to
                               device.

  Return values are common to both use cases, and are appropriate to be
  forwarded by the EFI_BLOCK_IO_PROTOCOL functions (ReadBlocks(),
  WriteBlocks(), FlushBlocks()).


  @retval EFI_SUCCESS          Transfer complete.

  @retval EFI_DEVICE_ERROR     Failed to notify host side via VirtIo write, or
                               unable to parse host response, or host response
                               is not VIRTIO_BLK_S_OK.

**/

STATIC
EFI_STATUS
EFIAPI
SynchronousRequest (
  IN              VBLK_DEV *Dev,
  IN              EFI_LBA  Lba,
  IN              UINTN    BufferSize,
  IN OUT volatile VOID     *Buffer,
  IN              BOOLEAN  RequestIsWrite
  )
{
  UINT32                  BlockSize;
  volatile VIRTIO_BLK_REQ Request;
  volatile UINT8          HostStatus;
  DESC_INDICES            Indices;

  BlockSize = Dev->BlockIoMedia.BlockSize;

  //
  // ensured by VirtioBlkInit()
  //
  ASSERT (BlockSize > 0);
  ASSERT (BlockSize % 512 == 0);

  //
  // ensured by contract above, plus VerifyReadWriteRequest()
  //
  ASSERT (BufferSize % BlockSize == 0);

  //
  // Prepare virtio-blk request header, setting zero size for flush.
  // IO Priority is homogeneously 0.
  //
  Request.Type   = RequestIsWrite ?
                   (BufferSize == 0 ? VIRTIO_BLK_T_FLUSH : VIRTIO_BLK_T_OUT) :
                   VIRTIO_BLK_T_IN;
  Request.IoPrio = 0;
  Request.Sector = MultU64x32(Lba, BlockSize / 512);

  VirtioPrepare (&Dev->Ring, &Indices);

  //
  // preset a host status for ourselves that we do not accept as success
  //
  HostStatus = VIRTIO_BLK_S_IOERR;

  //
  // ensured by VirtioBlkInit() -- this predicate, in combination with the
  // lock-step progress, ensures we don't have to track free descriptors.
  //
  ASSERT (Dev->Ring.QueueSize >= 3);

  //
  // virtio-blk header in first desc
  //
  VirtioAppendDesc (&Dev->Ring, (UINTN) &Request, sizeof Request,
    VRING_DESC_F_NEXT, &Indices);

  //
  // data buffer for read/write in second desc
  //
  if (BufferSize > 0) {
    //
    // From virtio-0.9.5, 2.3.2 Descriptor Table:
    // "no descriptor chain may be more than 2^32 bytes long in total".
    //
    // The predicate is ensured by the call contract above (for flush), or
    // VerifyReadWriteRequest() (for read/write). It also implies that
    // converting BufferSize to UINT32 will not truncate it.
    //
    ASSERT (BufferSize <= SIZE_1GB);

    //
    // VRING_DESC_F_WRITE is interpreted from the host's point of view.
    //
    VirtioAppendDesc (&Dev->Ring, (UINTN) Buffer, (UINT32) BufferSize,
      VRING_DESC_F_NEXT | (RequestIsWrite ? 0 : VRING_DESC_F_WRITE),
      &Indices);
  }

  //
  // host status in last (second or third) desc
  //
  VirtioAppendDesc (&Dev->Ring, (UINTN) &HostStatus, sizeof HostStatus,
    VRING_DESC_F_WRITE, &Indices);

  //
  // virtio-blk's only virtqueue is #0, called "requestq" (see Appendix D).
  //
  if (VirtioFlush (Dev->VirtIo, 0, &Dev->Ring, &Indices) == EFI_SUCCESS &&
      HostStatus == VIRTIO_BLK_S_OK) {
    return EFI_SUCCESS;
  }

  return EFI_DEVICE_ERROR;
}


/**

  ReadBlocks() operation for virtio-blk.

  See
  - UEFI Spec 2.3.1 + Errata C, 12.8 EFI Block I/O Protocol, 12.8 EFI Block I/O
    Protocol, EFI_BLOCK_IO_PROTOCOL.ReadBlocks().
  - Driver Writer's Guide for UEFI 2.3.1 v1.01, 24.2.2. ReadBlocks() and
    ReadBlocksEx() Implementation.

  Parameter checks and conformant return values are implemented in
  VerifyReadWriteRequest() and SynchronousRequest().

  A zero BufferSize doesn't seem to be prohibited, so do nothing in that case,
  successfully.

**/

EFI_STATUS
EFIAPI
VirtioBlkReadBlocks (
  IN  EFI_BLOCK_IO_PROTOCOL *This,
  IN  UINT32                MediaId,
  IN  EFI_LBA               Lba,
  IN  UINTN                 BufferSize,
  OUT VOID                  *Buffer
  )
{
  VBLK_DEV   *Dev;
  EFI_STATUS Status;

  if (BufferSize == 0) {
    return EFI_SUCCESS;
  }

  Dev = VIRTIO_BLK_FROM_BLOCK_IO (This);
  Status = VerifyReadWriteRequest (
             &Dev->BlockIoMedia,
             Lba,
             BufferSize,
             FALSE               // RequestIsWrite
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return SynchronousRequest (
           Dev,
           Lba,
           BufferSize,
           Buffer,
           FALSE       // RequestIsWrite
           );
}

/**

  WriteBlocks() operation for virtio-blk.

  See
  - UEFI Spec 2.3.1 + Errata C, 12.8 EFI Block I/O Protocol, 12.8 EFI Block I/O
    Protocol, EFI_BLOCK_IO_PROTOCOL.WriteBlocks().
  - Driver Writer's Guide for UEFI 2.3.1 v1.01, 24.2.3 WriteBlocks() and
    WriteBlockEx() Implementation.

  Parameter checks and conformant return values are implemented in
  VerifyReadWriteRequest() and SynchronousRequest().

  A zero BufferSize doesn't seem to be prohibited, so do nothing in that case,
  successfully.

**/

EFI_STATUS
EFIAPI
VirtioBlkWriteBlocks (
  IN EFI_BLOCK_IO_PROTOCOL *This,
  IN UINT32                MediaId,
  IN EFI_LBA               Lba,
  IN UINTN                 BufferSize,
  IN VOID                  *Buffer
  )
{
  VBLK_DEV   *Dev;
  EFI_STATUS Status;

  if (BufferSize == 0) {
    return EFI_SUCCESS;
  }

  Dev = VIRTIO_BLK_FROM_BLOCK_IO (This);
  Status = VerifyReadWriteRequest (
             &Dev->BlockIoMedia,
             Lba,
             BufferSize,
             TRUE                // RequestIsWrite
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return SynchronousRequest (
           Dev,
           Lba,
           BufferSize,
           Buffer,
           TRUE        // RequestIsWrite
           );
}


/**

  FlushBlocks() operation for virtio-blk.

  See
  - UEFI Spec 2.3.1 + Errata C, 12.8 EFI Block I/O Protocol, 12.8 EFI Block I/O
    Protocol, EFI_BLOCK_IO_PROTOCOL.FlushBlocks().
  - Driver Writer's Guide for UEFI 2.3.1 v1.01, 24.2.4 FlushBlocks() and
    FlushBlocksEx() Implementation.

  If the underlying virtio-blk device doesn't support flushing (ie.
  write-caching), then this function should not be called by higher layers,
  according to EFI_BLOCK_IO_MEDIA characteristics set in VirtioBlkInit().
  Should they do nonetheless, we do nothing, successfully.

**/

EFI_STATUS
EFIAPI
VirtioBlkFlushBlocks (
  IN EFI_BLOCK_IO_PROTOCOL *This
  )
{
  VBLK_DEV *Dev;

  Dev = VIRTIO_BLK_FROM_BLOCK_IO (This);
  return Dev->BlockIoMedia.WriteCaching ?
           SynchronousRequest (
             Dev,
             0,    // Lba
             0,    // BufferSize
             NULL, // Buffer
             TRUE  // RequestIsWrite
             ) :
           EFI_SUCCESS;
}


/**

  Device probe function for this driver.

  The DXE core calls this function for any given device in order to see if the
  driver can drive the device.

  Specs relevant in the general sense:

  - UEFI Spec 2.3.1 + Errata C:
    - 6.3 Protocol Handler Services -- for accessing the underlying device
    - 10.1 EFI Driver Binding Protocol -- for exporting ourselves

  - Driver Writer's Guide for UEFI 2.3.1 v1.01:
    - 5.1.3.4 OpenProtocol() and CloseProtocol() -- for accessing the
      underlying device
    - 9 Driver Binding Protocol -- for exporting ourselves

  @param[in]  This                The EFI_DRIVER_BINDING_PROTOCOL object
                                  incorporating this driver (independently of
                                  any device).

  @param[in] DeviceHandle         The device to probe.

  @param[in] RemainingDevicePath  Relevant only for bus drivers, ignored.


  @retval EFI_SUCCESS      The driver supports the device being probed.

  @retval EFI_UNSUPPORTED  Based on virtio-blk discovery, we do not support
                           the device.

  @return                  Error codes from the OpenProtocol() boot service or
                           the VirtIo protocol.

**/

EFI_STATUS
EFIAPI
VirtioBlkDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL *This,
  IN EFI_HANDLE                  DeviceHandle,
  IN EFI_DEVICE_PATH_PROTOCOL    *RemainingDevicePath
  )
{
  EFI_STATUS          Status;
  VIRTIO_DEVICE_PROTOCOL *VirtIo;

  //
  // Attempt to open the device with the VirtIo set of interfaces. On success,
  // the protocol is "instantiated" for the VirtIo device. Covers duplicate
  // open attempts (EFI_ALREADY_STARTED).
  //
  Status = gBS->OpenProtocol (
                  DeviceHandle,               // candidate device
                  &gVirtioDeviceProtocolGuid, // for generic VirtIo access
                  (VOID **)&VirtIo,           // handle to instantiate
                  This->DriverBindingHandle,  // requestor driver identity
                  DeviceHandle,               // ControllerHandle, according to
                                              // the UEFI Driver Model
                  EFI_OPEN_PROTOCOL_BY_DRIVER // get exclusive VirtIo access to
                                              // the device; to be released
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (VirtIo->SubSystemDeviceId != VIRTIO_SUBSYSTEM_BLOCK_DEVICE) {
    Status = EFI_UNSUPPORTED;
  }

  //
  // We needed VirtIo access only transitorily, to see whether we support the
  // device or not.
  //
  gBS->CloseProtocol (DeviceHandle, &gVirtioDeviceProtocolGuid,
         This->DriverBindingHandle, DeviceHandle);
  return Status;
}


/**

  Set up all BlockIo and virtio-blk aspects of this driver for the specified
  device.

  @param[in out] Dev  The driver instance to configure. The caller is
                      responsible for Dev->VirtIo's validity (ie. working IO
                      access to the underlying virtio-blk device).

  @retval EFI_SUCCESS      Setup complete.

  @retval EFI_UNSUPPORTED  The driver is unable to work with the virtio ring or
                           virtio-blk attributes the host provides.

  @return                  Error codes from VirtioRingInit() or
                           VIRTIO_CFG_READ() / VIRTIO_CFG_WRITE().

**/

STATIC
EFI_STATUS
EFIAPI
VirtioBlkInit (
  IN OUT VBLK_DEV *Dev
  )
{
  UINT8      NextDevStat;
  EFI_STATUS Status;

  UINT32     Features;
  UINT64     NumSectors;
  UINT32     BlockSize;
  UINT8      PhysicalBlockExp;
  UINT8      AlignmentOffset;
  UINT32     OptIoSize;
  UINT16     QueueSize;

  PhysicalBlockExp = 0;
  AlignmentOffset = 0;
  OptIoSize = 0;

  //
  // Execute virtio-0.9.5, 2.2.1 Device Initialization Sequence.
  //
  NextDevStat = 0;             // step 1 -- reset device
  Status = Dev->VirtIo->SetDeviceStatus (Dev->VirtIo, NextDevStat);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }

  NextDevStat |= VSTAT_ACK;    // step 2 -- acknowledge device presence
  Status = Dev->VirtIo->SetDeviceStatus (Dev->VirtIo, NextDevStat);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }

  NextDevStat |= VSTAT_DRIVER; // step 3 -- we know how to drive it
  Status = Dev->VirtIo->SetDeviceStatus (Dev->VirtIo, NextDevStat);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }

  //
  // Set Page Size - MMIO VirtIo Specific
  //
  Status = Dev->VirtIo->SetPageSize (Dev->VirtIo, EFI_PAGE_SIZE);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }

  //
  // step 4a -- retrieve and validate features
  //
  Status = Dev->VirtIo->GetDeviceFeatures (Dev->VirtIo, &Features);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }

  Status = VIRTIO_CFG_READ (Dev, Capacity, &NumSectors);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }
  if (NumSectors == 0) {
    Status = EFI_UNSUPPORTED;
    goto Failed;
  }

  if (Features & VIRTIO_BLK_F_BLK_SIZE) {
    Status = VIRTIO_CFG_READ (Dev, BlkSize, &BlockSize);
    if (EFI_ERROR (Status)) {
      goto Failed;
    }
    if (BlockSize == 0 || BlockSize % 512 != 0 ||
        ModU64x32 (NumSectors, BlockSize / 512) != 0) {
      //
      // We can only handle a logical block consisting of whole sectors,
      // and only a disk composed of whole logical blocks.
      //
      Status = EFI_UNSUPPORTED;
      goto Failed;
    }
  }
  else {
    BlockSize = 512;
  }

  if (Features & VIRTIO_BLK_F_TOPOLOGY) {
    Status = VIRTIO_CFG_READ (Dev, Topology.PhysicalBlockExp,
               &PhysicalBlockExp);
    if (EFI_ERROR (Status)) {
      goto Failed;
    }
    if (PhysicalBlockExp >= 32) {
      Status = EFI_UNSUPPORTED;
      goto Failed;
    }

    Status = VIRTIO_CFG_READ (Dev, Topology.AlignmentOffset, &AlignmentOffset);
    if (EFI_ERROR (Status)) {
      goto Failed;
    }

    Status = VIRTIO_CFG_READ (Dev, Topology.OptIoSize, &OptIoSize);
    if (EFI_ERROR (Status)) {
      goto Failed;
    }
  }

  //
  // step 4b -- allocate virtqueue
  //
  Status = Dev->VirtIo->SetQueueSel (Dev->VirtIo, 0);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }
  Status = Dev->VirtIo->GetQueueNumMax (Dev->VirtIo, &QueueSize);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }
  if (QueueSize < 3) { // SynchronousRequest() uses at most three descriptors
    Status = EFI_UNSUPPORTED;
    goto Failed;
  }

  Status = VirtioRingInit (QueueSize, &Dev->Ring);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }

  //
  // Additional steps for MMIO: align the queue appropriately, and set the
  // size. If anything fails from here on, we must release the ring resources.
  //
  Status = Dev->VirtIo->SetQueueNum (Dev->VirtIo, QueueSize);
  if (EFI_ERROR (Status)) {
    goto ReleaseQueue;
  }

  Status = Dev->VirtIo->SetQueueAlign (Dev->VirtIo, EFI_PAGE_SIZE);
  if (EFI_ERROR (Status)) {
    goto ReleaseQueue;
  }

  //
  // step 4c -- Report GPFN (guest-physical frame number) of queue.
  //
  Status = Dev->VirtIo->SetQueueAddress (Dev->VirtIo,
      (UINT32) ((UINTN) Dev->Ring.Base >> EFI_PAGE_SHIFT));
  if (EFI_ERROR (Status)) {
    goto ReleaseQueue;
  }


  //
  // step 5 -- Report understood features. There are no virtio-blk specific
  // features to negotiate in virtio-0.9.5, plus we do not want any of the
  // device-independent (known or unknown) VIRTIO_F_* capabilities (see
  // Appendix B).
  //
  Status = Dev->VirtIo->SetGuestFeatures (Dev->VirtIo, 0);
  if (EFI_ERROR (Status)) {
    goto ReleaseQueue;
  }

  //
  // step 6 -- initialization complete
  //
  NextDevStat |= VSTAT_DRIVER_OK;
  Status = Dev->VirtIo->SetDeviceStatus (Dev->VirtIo, NextDevStat);
  if (EFI_ERROR (Status)) {
    goto ReleaseQueue;
  }

  //
  // Populate the exported interface's attributes; see UEFI spec v2.4, 12.9 EFI
  // Block I/O Protocol.
  //
  Dev->BlockIo.Revision              = 0;
  Dev->BlockIo.Media                 = &Dev->BlockIoMedia;
  Dev->BlockIo.Reset                 = &VirtioBlkReset;
  Dev->BlockIo.ReadBlocks            = &VirtioBlkReadBlocks;
  Dev->BlockIo.WriteBlocks           = &VirtioBlkWriteBlocks;
  Dev->BlockIo.FlushBlocks           = &VirtioBlkFlushBlocks;
  Dev->BlockIoMedia.MediaId          = 0;
  Dev->BlockIoMedia.RemovableMedia   = FALSE;
  Dev->BlockIoMedia.MediaPresent     = TRUE;
  Dev->BlockIoMedia.LogicalPartition = FALSE;
  Dev->BlockIoMedia.ReadOnly         = (BOOLEAN) ((Features & VIRTIO_BLK_F_RO) != 0);
  Dev->BlockIoMedia.WriteCaching     = (BOOLEAN) ((Features & VIRTIO_BLK_F_FLUSH) != 0);
  Dev->BlockIoMedia.BlockSize        = BlockSize;
  Dev->BlockIoMedia.IoAlign          = 0;
  Dev->BlockIoMedia.LastBlock        = DivU64x32 (NumSectors,
                                         BlockSize / 512) - 1;

  DEBUG ((DEBUG_INFO, "%a: LbaSize=0x%x[B] NumBlocks=0x%Lx[Lba]\n",
    __FUNCTION__, Dev->BlockIoMedia.BlockSize,
    Dev->BlockIoMedia.LastBlock + 1));

  if (Features & VIRTIO_BLK_F_TOPOLOGY) {
    Dev->BlockIo.Revision = EFI_BLOCK_IO_PROTOCOL_REVISION3;

    Dev->BlockIoMedia.LowestAlignedLba = AlignmentOffset;
    Dev->BlockIoMedia.LogicalBlocksPerPhysicalBlock = 1u << PhysicalBlockExp;
    Dev->BlockIoMedia.OptimalTransferLengthGranularity = OptIoSize;

    DEBUG ((DEBUG_INFO, "%a: FirstAligned=0x%Lx[Lba] PhysBlkSize=0x%x[Lba]\n",
      __FUNCTION__, Dev->BlockIoMedia.LowestAlignedLba,
      Dev->BlockIoMedia.LogicalBlocksPerPhysicalBlock));
    DEBUG ((DEBUG_INFO, "%a: OptimalTransferLengthGranularity=0x%x[Lba]\n",
      __FUNCTION__, Dev->BlockIoMedia.OptimalTransferLengthGranularity));
  }
  return EFI_SUCCESS;

ReleaseQueue:
  VirtioRingUninit (&Dev->Ring);

Failed:
  //
  // Notify the host about our failure to setup: virtio-0.9.5, 2.2.2.1 Device
  // Status. VirtIo access failure here should not mask the original error.
  //
  NextDevStat |= VSTAT_FAILED;
  Dev->VirtIo->SetDeviceStatus (Dev->VirtIo, NextDevStat);

  return Status; // reached only via Failed above
}


/**

  Uninitialize the internals of a virtio-blk device that has been successfully
  set up with VirtioBlkInit().

  @param[in out]  Dev  The device to clean up.

**/

STATIC
VOID
EFIAPI
VirtioBlkUninit (
  IN OUT VBLK_DEV *Dev
  )
{
  //
  // Reset the virtual device -- see virtio-0.9.5, 2.2.2.1 Device Status. When
  // VIRTIO_CFG_WRITE() returns, the host will have learned to stay away from
  // the old comms area.
  //
  Dev->VirtIo->SetDeviceStatus (Dev->VirtIo, 0);

  VirtioRingUninit (&Dev->Ring);

  SetMem (&Dev->BlockIo,      sizeof Dev->BlockIo,      0x00);
  SetMem (&Dev->BlockIoMedia, sizeof Dev->BlockIoMedia, 0x00);
}


/**

  After we've pronounced support for a specific device in
  DriverBindingSupported(), we start managing said device (passed in by the
  Driver Exeuction Environment) with the following service.

  See DriverBindingSupported() for specification references.

  @param[in]  This                The EFI_DRIVER_BINDING_PROTOCOL object
                                  incorporating this driver (independently of
                                  any device).

  @param[in] DeviceHandle         The supported device to drive.

  @param[in] RemainingDevicePath  Relevant only for bus drivers, ignored.


  @retval EFI_SUCCESS           Driver instance has been created and
                                initialized  for the virtio-blk device, it
                                is now accessibla via EFI_BLOCK_IO_PROTOCOL.

  @retval EFI_OUT_OF_RESOURCES  Memory allocation failed.

  @return                       Error codes from the OpenProtocol() boot
                                service, the VirtIo protocol, VirtioBlkInit(),
                                or the InstallProtocolInterface() boot service.

**/

EFI_STATUS
EFIAPI
VirtioBlkDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL *This,
  IN EFI_HANDLE                  DeviceHandle,
  IN EFI_DEVICE_PATH_PROTOCOL    *RemainingDevicePath
  )
{
  VBLK_DEV   *Dev;
  EFI_STATUS Status;

  Dev = (VBLK_DEV *) AllocateZeroPool (sizeof *Dev);
  if (Dev == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = gBS->OpenProtocol (DeviceHandle, &gVirtioDeviceProtocolGuid,
                  (VOID **)&Dev->VirtIo, This->DriverBindingHandle,
                  DeviceHandle, EFI_OPEN_PROTOCOL_BY_DRIVER);
  if (EFI_ERROR (Status)) {
    goto FreeVirtioBlk;
  }

  //
  // VirtIo access granted, configure virtio-blk device.
  //
  Status = VirtioBlkInit (Dev);
  if (EFI_ERROR (Status)) {
    goto CloseVirtIo;
  }

  //
  // Setup complete, attempt to export the driver instance's BlockIo interface.
  //
  Dev->Signature = VBLK_SIG;
  Status = gBS->InstallProtocolInterface (&DeviceHandle,
                  &gEfiBlockIoProtocolGuid, EFI_NATIVE_INTERFACE,
                  &Dev->BlockIo);
  if (EFI_ERROR (Status)) {
    goto UninitDev;
  }

  return EFI_SUCCESS;

UninitDev:
  VirtioBlkUninit (Dev);

CloseVirtIo:
  gBS->CloseProtocol (DeviceHandle, &gVirtioDeviceProtocolGuid,
         This->DriverBindingHandle, DeviceHandle);

FreeVirtioBlk:
  FreePool (Dev);

  return Status;
}


/**

  Stop driving a virtio-blk device and remove its BlockIo interface.

  This function replays the success path of DriverBindingStart() in reverse.
  The host side virtio-blk device is reset, so that the OS boot loader or the
  OS may reinitialize it.

  @param[in] This               The EFI_DRIVER_BINDING_PROTOCOL object
                                incorporating this driver (independently of any
                                device).

  @param[in] DeviceHandle       Stop driving this device.

  @param[in] NumberOfChildren   Since this function belongs to a device driver
                                only (as opposed to a bus driver), the caller
                                environment sets NumberOfChildren to zero, and
                                we ignore it.

  @param[in] ChildHandleBuffer  Ignored (corresponding to NumberOfChildren).

**/

EFI_STATUS
EFIAPI
VirtioBlkDriverBindingStop (
  IN EFI_DRIVER_BINDING_PROTOCOL *This,
  IN EFI_HANDLE                  DeviceHandle,
  IN UINTN                       NumberOfChildren,
  IN EFI_HANDLE                  *ChildHandleBuffer
  )
{
  EFI_STATUS            Status;
  EFI_BLOCK_IO_PROTOCOL *BlockIo;
  VBLK_DEV              *Dev;

  Status = gBS->OpenProtocol (
                  DeviceHandle,                  // candidate device
                  &gEfiBlockIoProtocolGuid,      // retrieve the BlockIo iface
                  (VOID **)&BlockIo,             // target pointer
                  This->DriverBindingHandle,     // requestor driver identity
                  DeviceHandle,                  // requesting lookup for dev.
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL // lookup only, no ref. added
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Dev = VIRTIO_BLK_FROM_BLOCK_IO (BlockIo);

  //
  // Handle Stop() requests for in-use driver instances gracefully.
  //
  Status = gBS->UninstallProtocolInterface (DeviceHandle,
                  &gEfiBlockIoProtocolGuid, &Dev->BlockIo);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  VirtioBlkUninit (Dev);

  gBS->CloseProtocol (DeviceHandle, &gVirtioDeviceProtocolGuid,
         This->DriverBindingHandle, DeviceHandle);

  FreePool (Dev);

  return EFI_SUCCESS;
}


//
// The static object that groups the Supported() (ie. probe), Start() and
// Stop() functions of the driver together. Refer to UEFI Spec 2.3.1 + Errata
// C, 10.1 EFI Driver Binding Protocol.
//
STATIC EFI_DRIVER_BINDING_PROTOCOL gDriverBinding = {
  &VirtioBlkDriverBindingSupported,
  &VirtioBlkDriverBindingStart,
  &VirtioBlkDriverBindingStop,
  0x10, // Version, must be in [0x10 .. 0xFFFFFFEF] for IHV-developed drivers
  NULL, // ImageHandle, to be overwritten by
        // EfiLibInstallDriverBindingComponentName2() in VirtioBlkEntryPoint()
  NULL  // DriverBindingHandle, ditto
};


//
// The purpose of the following scaffolding (EFI_COMPONENT_NAME_PROTOCOL and
// EFI_COMPONENT_NAME2_PROTOCOL implementation) is to format the driver's name
// in English, for display on standard console devices. This is recommended for
// UEFI drivers that follow the UEFI Driver Model. Refer to the Driver Writer's
// Guide for UEFI 2.3.1 v1.01, 11 UEFI Driver and Controller Names.
//
// Device type names ("Virtio Block Device") are not formatted because the
// driver supports only that device type. Therefore the driver name suffices
// for unambiguous identification.
//

STATIC
EFI_UNICODE_STRING_TABLE mDriverNameTable[] = {
  { "eng;en", L"Virtio Block Driver" },
  { NULL,     NULL                   }
};

STATIC
EFI_COMPONENT_NAME_PROTOCOL gComponentName;

EFI_STATUS
EFIAPI
VirtioBlkGetDriverName (
  IN  EFI_COMPONENT_NAME_PROTOCOL *This,
  IN  CHAR8                       *Language,
  OUT CHAR16                      **DriverName
  )
{
  return LookupUnicodeString2 (
           Language,
           This->SupportedLanguages,
           mDriverNameTable,
           DriverName,
           (BOOLEAN)(This == &gComponentName) // Iso639Language
           );
}

EFI_STATUS
EFIAPI
VirtioBlkGetDeviceName (
  IN  EFI_COMPONENT_NAME_PROTOCOL *This,
  IN  EFI_HANDLE                  DeviceHandle,
  IN  EFI_HANDLE                  ChildHandle,
  IN  CHAR8                       *Language,
  OUT CHAR16                      **ControllerName
  )
{
  return EFI_UNSUPPORTED;
}

STATIC
EFI_COMPONENT_NAME_PROTOCOL gComponentName = {
  &VirtioBlkGetDriverName,
  &VirtioBlkGetDeviceName,
  "eng" // SupportedLanguages, ISO 639-2 language codes
};

STATIC
EFI_COMPONENT_NAME2_PROTOCOL gComponentName2 = {
  (EFI_COMPONENT_NAME2_GET_DRIVER_NAME)     &VirtioBlkGetDriverName,
  (EFI_COMPONENT_NAME2_GET_CONTROLLER_NAME) &VirtioBlkGetDeviceName,
  "en" // SupportedLanguages, RFC 4646 language codes
};


//
// Entry point of this driver.
//
EFI_STATUS
EFIAPI
VirtioBlkEntryPoint (
  IN EFI_HANDLE       ImageHandle,
  IN EFI_SYSTEM_TABLE *SystemTable
  )
{
  return EfiLibInstallDriverBindingComponentName2 (
           ImageHandle,
           SystemTable,
           &gDriverBinding,
           ImageHandle,
           &gComponentName,
           &gComponentName2
           );
}

