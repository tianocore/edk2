/** @file

  VirtIo GPU initialization, and commands (primitives) for the GPU device.

  Copyright (C) 2016, Red Hat, Inc.
  Copyright (c) 2017, AMD Inc, All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/VirtioLib.h>

#include "VirtioGpu.h"

/**
  Configure the VirtIo GPU device that underlies VgpuDev.

  @param[in,out] VgpuDev  The VGPU_DEV object to set up VirtIo messaging for.
                          On input, the caller is responsible for having
                          initialized VgpuDev->VirtIo. On output, VgpuDev->Ring
                          has been initialized, and synchronous VirtIo GPU
                          commands (primitives) can be submitted to the device.

  @retval EFI_SUCCESS      VirtIo GPU configuration successful.

  @retval EFI_UNSUPPORTED  The host-side configuration of the VirtIo GPU is not
                           supported by this driver.

  @retval                  Error codes from underlying functions.
**/
EFI_STATUS
VirtioGpuInit (
  IN OUT VGPU_DEV *VgpuDev
  )
{
  UINT8      NextDevStat;
  EFI_STATUS Status;
  UINT64     Features;
  UINT16     QueueSize;
  UINT64     RingBaseShift;

  //
  // Execute virtio-v1.0-cs04, 3.1.1 Driver Requirements: Device
  // Initialization.
  //
  // 1. Reset the device.
  //
  NextDevStat = 0;
  Status = VgpuDev->VirtIo->SetDeviceStatus (VgpuDev->VirtIo, NextDevStat);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }

  //
  // 2. Set the ACKNOWLEDGE status bit [...]
  //
  NextDevStat |= VSTAT_ACK;
  Status = VgpuDev->VirtIo->SetDeviceStatus (VgpuDev->VirtIo, NextDevStat);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }

  //
  // 3. Set the DRIVER status bit [...]
  //
  NextDevStat |= VSTAT_DRIVER;
  Status = VgpuDev->VirtIo->SetDeviceStatus (VgpuDev->VirtIo, NextDevStat);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }

  //
  // 4. Read device feature bits...
  //
  Status = VgpuDev->VirtIo->GetDeviceFeatures (VgpuDev->VirtIo, &Features);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }
  if ((Features & VIRTIO_F_VERSION_1) == 0) {
    Status = EFI_UNSUPPORTED;
    goto Failed;
  }
  //
  // We only want the most basic 2D features.
  //
  Features &= VIRTIO_F_VERSION_1 | VIRTIO_F_IOMMU_PLATFORM;

  //
  // ... and write the subset of feature bits understood by the [...] driver to
  // the device. [...]
  // 5. Set the FEATURES_OK status bit.
  // 6. Re-read device status to ensure the FEATURES_OK bit is still set [...]
  //
  Status = Virtio10WriteFeatures (VgpuDev->VirtIo, Features, &NextDevStat);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }

  //
  // 7. Perform device-specific setup, including discovery of virtqueues for
  // the device [...]
  //
  Status = VgpuDev->VirtIo->SetQueueSel (VgpuDev->VirtIo,
                              VIRTIO_GPU_CONTROL_QUEUE);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }
  Status = VgpuDev->VirtIo->GetQueueNumMax (VgpuDev->VirtIo, &QueueSize);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }

  //
  // We implement each VirtIo GPU command that we use with two descriptors:
  // request, response.
  //
  if (QueueSize < 2) {
    Status = EFI_UNSUPPORTED;
    goto Failed;
  }

  //
  // [...] population of virtqueues [...]
  //
  Status = VirtioRingInit (VgpuDev->VirtIo, QueueSize, &VgpuDev->Ring);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }
  //
  // If anything fails from here on, we have to release the ring.
  //
  Status = VirtioRingMap (
             VgpuDev->VirtIo,
             &VgpuDev->Ring,
             &RingBaseShift,
             &VgpuDev->RingMap
             );
  if (EFI_ERROR (Status)) {
    goto ReleaseQueue;
  }
  //
  // If anything fails from here on, we have to unmap the ring.
  //
  Status = VgpuDev->VirtIo->SetQueueAddress (
                              VgpuDev->VirtIo,
                              &VgpuDev->Ring,
                              RingBaseShift
                              );
  if (EFI_ERROR (Status)) {
    goto UnmapQueue;
  }

  //
  // 8. Set the DRIVER_OK status bit.
  //
  NextDevStat |= VSTAT_DRIVER_OK;
  Status = VgpuDev->VirtIo->SetDeviceStatus (VgpuDev->VirtIo, NextDevStat);
  if (EFI_ERROR (Status)) {
    goto UnmapQueue;
  }

  return EFI_SUCCESS;

UnmapQueue:
  VgpuDev->VirtIo->UnmapSharedBuffer (VgpuDev->VirtIo, VgpuDev->RingMap);

ReleaseQueue:
  VirtioRingUninit (VgpuDev->VirtIo, &VgpuDev->Ring);

Failed:
  //
  // If any of these steps go irrecoverably wrong, the driver SHOULD set the
  // FAILED status bit to indicate that it has given up on the device (it can
  // reset the device later to restart if desired). [...]
  //
  // VirtIo access failure here should not mask the original error.
  //
  NextDevStat |= VSTAT_FAILED;
  VgpuDev->VirtIo->SetDeviceStatus (VgpuDev->VirtIo, NextDevStat);

  return Status;
}

/**
  De-configure the VirtIo GPU device that underlies VgpuDev.

  @param[in,out] VgpuDev  The VGPU_DEV object to tear down VirtIo messaging
                          for. On input, the caller is responsible for having
                          called VirtioGpuInit(). On output, VgpuDev->Ring has
                          been uninitialized; VirtIo GPU commands (primitives)
                          can no longer be submitted to the device.
**/
VOID
VirtioGpuUninit (
  IN OUT VGPU_DEV *VgpuDev
  )
{
  //
  // Resetting the VirtIo device makes it release its resources and forget its
  // configuration.
  //
  VgpuDev->VirtIo->SetDeviceStatus (VgpuDev->VirtIo, 0);
  VgpuDev->VirtIo->UnmapSharedBuffer (VgpuDev->VirtIo, VgpuDev->RingMap);
  VirtioRingUninit (VgpuDev->VirtIo, &VgpuDev->Ring);
}

/**
  Allocate, zero and map memory, for bus master common buffer operation, to be
  attached as backing store to a host-side VirtIo GPU resource.

  @param[in]  VgpuDev        The VGPU_DEV object that represents the VirtIo GPU
                             device.

  @param[in]  NumberOfPages  The number of whole pages to allocate and map.

  @param[out] HostAddress    The system memory address of the allocated area.

  @param[out] DeviceAddress  The bus master device address of the allocated
                             area. The VirtIo GPU device may be programmed to
                             access the allocated area through DeviceAddress;
                             DeviceAddress is to be passed to the
                             VirtioGpuResourceAttachBacking() function, as the
                             BackingStoreDeviceAddress parameter.

  @param[out] Mapping        A resulting token to pass to
                             VirtioGpuUnmapAndFreeBackingStore().

  @retval EFI_SUCCESS  The requested number of pages has been allocated, zeroed
                       and mapped.

  @return              Status codes propagated from
                       VgpuDev->VirtIo->AllocateSharedPages() and
                       VirtioMapAllBytesInSharedBuffer().
**/
EFI_STATUS
VirtioGpuAllocateZeroAndMapBackingStore (
  IN  VGPU_DEV             *VgpuDev,
  IN  UINTN                NumberOfPages,
  OUT VOID                 **HostAddress,
  OUT EFI_PHYSICAL_ADDRESS *DeviceAddress,
  OUT VOID                 **Mapping
  )
{
  EFI_STATUS Status;
  VOID       *NewHostAddress;

  Status = VgpuDev->VirtIo->AllocateSharedPages (
                              VgpuDev->VirtIo,
                              NumberOfPages,
                              &NewHostAddress
                              );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Avoid exposing stale data to the device even temporarily: zero the area
  // before mapping it.
  //
  ZeroMem (NewHostAddress, EFI_PAGES_TO_SIZE (NumberOfPages));

  Status = VirtioMapAllBytesInSharedBuffer (
             VgpuDev->VirtIo,                      // VirtIo
             VirtioOperationBusMasterCommonBuffer, // Operation
             NewHostAddress,                       // HostAddress
             EFI_PAGES_TO_SIZE (NumberOfPages),    // NumberOfBytes
             DeviceAddress,                        // DeviceAddress
             Mapping                               // Mapping
             );
  if (EFI_ERROR (Status)) {
    goto FreeSharedPages;
  }

  *HostAddress = NewHostAddress;
  return EFI_SUCCESS;

FreeSharedPages:
  VgpuDev->VirtIo->FreeSharedPages (
                     VgpuDev->VirtIo,
                     NumberOfPages,
                     NewHostAddress
                     );
  return Status;
}

/**
  Unmap and free memory originally allocated and mapped with
  VirtioGpuAllocateZeroAndMapBackingStore().

  If the memory allocated and mapped with
  VirtioGpuAllocateZeroAndMapBackingStore() was attached to a host-side VirtIo
  GPU resource with VirtioGpuResourceAttachBacking(), then the caller is
  responsible for detaching the backing store from the same resource, with
  VirtioGpuResourceDetachBacking(), before calling this function.

  @param[in] VgpuDev        The VGPU_DEV object that represents the VirtIo GPU
                            device.

  @param[in] NumberOfPages  The NumberOfPages parameter originally passed to
                            VirtioGpuAllocateZeroAndMapBackingStore().

  @param[in] HostAddress    The HostAddress value originally output by
                            VirtioGpuAllocateZeroAndMapBackingStore().

  @param[in] Mapping        The token that was originally output by
                            VirtioGpuAllocateZeroAndMapBackingStore().
**/
VOID
VirtioGpuUnmapAndFreeBackingStore (
  IN VGPU_DEV *VgpuDev,
  IN UINTN    NumberOfPages,
  IN VOID     *HostAddress,
  IN VOID     *Mapping
  )
{
  VgpuDev->VirtIo->UnmapSharedBuffer (
                     VgpuDev->VirtIo,
                     Mapping
                     );
  VgpuDev->VirtIo->FreeSharedPages (
                     VgpuDev->VirtIo,
                     NumberOfPages,
                     HostAddress
                     );
}

/**
  EFI_EVENT_NOTIFY function for the VGPU_DEV.ExitBoot event. It resets the
  VirtIo device, causing it to release its resources and to forget its
  configuration.

  This function may only be called (that is, VGPU_DEV.ExitBoot may only be
  signaled) after VirtioGpuInit() returns and before VirtioGpuUninit() is
  called.

  @param[in] Event    Event whose notification function is being invoked.

  @param[in] Context  Pointer to the associated VGPU_DEV object.
**/
VOID
EFIAPI
VirtioGpuExitBoot (
  IN EFI_EVENT Event,
  IN VOID      *Context
  )
{
  VGPU_DEV *VgpuDev;

  DEBUG ((DEBUG_VERBOSE, "%a: Context=0x%p\n", __FUNCTION__, Context));
  VgpuDev = Context;
  VgpuDev->VirtIo->SetDeviceStatus (VgpuDev->VirtIo, 0);
}

/**
  Internal utility function that sends a request to the VirtIo GPU device
  model, awaits the answer from the host, and returns a status.

  @param[in,out] VgpuDev  The VGPU_DEV object that represents the VirtIo GPU
                          device. The caller is responsible to have
                          successfully invoked VirtioGpuInit() on VgpuDev
                          previously, while VirtioGpuUninit() must not have
                          been called on VgpuDev.

  @param[in] RequestType  The type of the request. The caller is responsible
                          for providing a VirtioGpuCmd* RequestType which, on
                          success, elicits a VirtioGpuRespOkNodata response
                          from the host.

  @param[in] Fence        Whether to enable fencing for this request. Fencing
                          forces the host to complete the command before
                          producing a response. If Fence is TRUE, then
                          VgpuDev->FenceId is consumed, and incremented.

  @param[in,out] Header   Pointer to the caller-allocated request object. The
                          request must start with VIRTIO_GPU_CONTROL_HEADER.
                          This function overwrites all fields of Header before
                          submitting the request to the host:

                          - it sets Type from RequestType,

                          - it sets Flags and FenceId based on Fence,

                          - it zeroes CtxId and Padding.

  @param[in] RequestSize  Size of the entire caller-allocated request object,
                          including the leading VIRTIO_GPU_CONTROL_HEADER.

  @retval EFI_SUCCESS            Operation successful.

  @retval EFI_DEVICE_ERROR       The host rejected the request. The host error
                                 code has been logged on the EFI_D_ERROR level.

  @return                        Codes for unexpected errors in VirtIo
                                 messaging, or request/response
                                 mapping/unmapping.
**/
STATIC
EFI_STATUS
VirtioGpuSendCommand (
  IN OUT VGPU_DEV                           *VgpuDev,
  IN     VIRTIO_GPU_CONTROL_TYPE            RequestType,
  IN     BOOLEAN                            Fence,
  IN OUT volatile VIRTIO_GPU_CONTROL_HEADER *Header,
  IN     UINTN                              RequestSize
  )
{
  DESC_INDICES                       Indices;
  volatile VIRTIO_GPU_CONTROL_HEADER Response;
  EFI_STATUS                         Status;
  UINT32                             ResponseSize;
  EFI_PHYSICAL_ADDRESS               RequestDeviceAddress;
  VOID                               *RequestMap;
  EFI_PHYSICAL_ADDRESS               ResponseDeviceAddress;
  VOID                               *ResponseMap;

  //
  // Initialize Header.
  //
  Header->Type      = RequestType;
  if (Fence) {
    Header->Flags   = VIRTIO_GPU_FLAG_FENCE;
    Header->FenceId = VgpuDev->FenceId++;
  } else {
    Header->Flags   = 0;
    Header->FenceId = 0;
  }
  Header->CtxId     = 0;
  Header->Padding   = 0;

  ASSERT (RequestSize >= sizeof *Header);
  ASSERT (RequestSize <= MAX_UINT32);

  //
  // Map request and response to bus master device addresses.
  //
  Status = VirtioMapAllBytesInSharedBuffer (
             VgpuDev->VirtIo,
             VirtioOperationBusMasterRead,
             (VOID *)Header,
             RequestSize,
             &RequestDeviceAddress,
             &RequestMap
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }
  Status = VirtioMapAllBytesInSharedBuffer (
             VgpuDev->VirtIo,
             VirtioOperationBusMasterWrite,
             (VOID *)&Response,
             sizeof Response,
             &ResponseDeviceAddress,
             &ResponseMap
             );
  if (EFI_ERROR (Status)) {
    goto UnmapRequest;
  }

  //
  // Compose the descriptor chain.
  //
  VirtioPrepare (&VgpuDev->Ring, &Indices);
  VirtioAppendDesc (
    &VgpuDev->Ring,
    RequestDeviceAddress,
    (UINT32)RequestSize,
    VRING_DESC_F_NEXT,
    &Indices
    );
  VirtioAppendDesc (
    &VgpuDev->Ring,
    ResponseDeviceAddress,
    (UINT32)sizeof Response,
    VRING_DESC_F_WRITE,
    &Indices
    );

  //
  // Send the command.
  //
  Status = VirtioFlush (VgpuDev->VirtIo, VIRTIO_GPU_CONTROL_QUEUE,
             &VgpuDev->Ring, &Indices, &ResponseSize);
  if (EFI_ERROR (Status)) {
    goto UnmapResponse;
  }

  //
  // Verify response size.
  //
  if (ResponseSize != sizeof Response) {
    DEBUG ((EFI_D_ERROR, "%a: malformed response to Request=0x%x\n",
      __FUNCTION__, (UINT32)RequestType));
    Status = EFI_PROTOCOL_ERROR;
    goto UnmapResponse;
  }

  //
  // Unmap response and request, in reverse order of mapping. On error, the
  // respective mapping is invalidated anyway, only the data may not have been
  // committed to system memory (in case of VirtioOperationBusMasterWrite).
  //
  Status = VgpuDev->VirtIo->UnmapSharedBuffer (VgpuDev->VirtIo, ResponseMap);
  if (EFI_ERROR (Status)) {
    goto UnmapRequest;
  }
  Status = VgpuDev->VirtIo->UnmapSharedBuffer (VgpuDev->VirtIo, RequestMap);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Parse the response.
  //
  if (Response.Type == VirtioGpuRespOkNodata) {
    return EFI_SUCCESS;
  }

  DEBUG ((EFI_D_ERROR, "%a: Request=0x%x Response=0x%x\n", __FUNCTION__,
    (UINT32)RequestType, Response.Type));
  return EFI_DEVICE_ERROR;

UnmapResponse:
  VgpuDev->VirtIo->UnmapSharedBuffer (VgpuDev->VirtIo, ResponseMap);

UnmapRequest:
  VgpuDev->VirtIo->UnmapSharedBuffer (VgpuDev->VirtIo, RequestMap);

  return Status;
}

/**
  The following functions send requests to the VirtIo GPU device model, await
  the answer from the host, and return a status. They share the following
  interface details:

  @param[in,out] VgpuDev  The VGPU_DEV object that represents the VirtIo GPU
                          device. The caller is responsible to have
                          successfully invoked VirtioGpuInit() on VgpuDev
                          previously, while VirtioGpuUninit() must not have
                          been called on VgpuDev.

  @retval EFI_INVALID_PARAMETER  Invalid command-specific parameters were
                                 detected by this driver.

  @retval EFI_SUCCESS            Operation successful.

  @retval EFI_DEVICE_ERROR       The host rejected the request. The host error
                                 code has been logged on the EFI_D_ERROR level.

  @return                        Codes for unexpected errors in VirtIo
                                 messaging.

  For the command-specific parameters, please consult the GPU Device section of
  the VirtIo 1.0 specification (see references in
  "OvmfPkg/Include/IndustryStandard/VirtioGpu.h").
**/
EFI_STATUS
VirtioGpuResourceCreate2d (
  IN OUT VGPU_DEV           *VgpuDev,
  IN     UINT32             ResourceId,
  IN     VIRTIO_GPU_FORMATS Format,
  IN     UINT32             Width,
  IN     UINT32             Height
  )
{
  volatile VIRTIO_GPU_RESOURCE_CREATE_2D Request;

  if (ResourceId == 0) {
    return EFI_INVALID_PARAMETER;
  }

  Request.ResourceId = ResourceId;
  Request.Format     = (UINT32)Format;
  Request.Width      = Width;
  Request.Height     = Height;

  return VirtioGpuSendCommand (
           VgpuDev,
           VirtioGpuCmdResourceCreate2d,
           FALSE,                        // Fence
           &Request.Header,
           sizeof Request
           );
}

EFI_STATUS
VirtioGpuResourceUnref (
  IN OUT VGPU_DEV *VgpuDev,
  IN     UINT32   ResourceId
  )
{
  volatile VIRTIO_GPU_RESOURCE_UNREF Request;

  if (ResourceId == 0) {
    return EFI_INVALID_PARAMETER;
  }

  Request.ResourceId = ResourceId;
  Request.Padding    = 0;

  return VirtioGpuSendCommand (
           VgpuDev,
           VirtioGpuCmdResourceUnref,
           FALSE,                     // Fence
           &Request.Header,
           sizeof Request
           );
}

EFI_STATUS
VirtioGpuResourceAttachBacking (
  IN OUT VGPU_DEV             *VgpuDev,
  IN     UINT32               ResourceId,
  IN     EFI_PHYSICAL_ADDRESS BackingStoreDeviceAddress,
  IN     UINTN                NumberOfPages
  )
{
  volatile VIRTIO_GPU_RESOURCE_ATTACH_BACKING Request;

  if (ResourceId == 0) {
    return EFI_INVALID_PARAMETER;
  }

  Request.ResourceId    = ResourceId;
  Request.NrEntries     = 1;
  Request.Entry.Addr    = BackingStoreDeviceAddress;
  Request.Entry.Length  = (UINT32)EFI_PAGES_TO_SIZE (NumberOfPages);
  Request.Entry.Padding = 0;

  return VirtioGpuSendCommand (
           VgpuDev,
           VirtioGpuCmdResourceAttachBacking,
           FALSE,                             // Fence
           &Request.Header,
           sizeof Request
           );
}

EFI_STATUS
VirtioGpuResourceDetachBacking (
  IN OUT VGPU_DEV *VgpuDev,
  IN     UINT32   ResourceId
  )
{
  volatile VIRTIO_GPU_RESOURCE_DETACH_BACKING Request;

  if (ResourceId == 0) {
    return EFI_INVALID_PARAMETER;
  }

  Request.ResourceId = ResourceId;
  Request.Padding    = 0;

  //
  // In this case, we set Fence to TRUE, because after this function returns,
  // the caller might reasonably want to repurpose the backing pages
  // immediately. Thus we should ensure that the host releases all references
  // to the backing pages before we return.
  //
  return VirtioGpuSendCommand (
           VgpuDev,
           VirtioGpuCmdResourceDetachBacking,
           TRUE,                              // Fence
           &Request.Header,
           sizeof Request
           );
}

EFI_STATUS
VirtioGpuSetScanout (
  IN OUT VGPU_DEV *VgpuDev,
  IN     UINT32   X,
  IN     UINT32   Y,
  IN     UINT32   Width,
  IN     UINT32   Height,
  IN     UINT32   ScanoutId,
  IN     UINT32   ResourceId
  )
{
  volatile VIRTIO_GPU_SET_SCANOUT Request;

  //
  // Unlike for most other commands, ResourceId=0 is valid; it
  // is used to disable a scanout.
  //
  Request.Rectangle.X      = X;
  Request.Rectangle.Y      = Y;
  Request.Rectangle.Width  = Width;
  Request.Rectangle.Height = Height;
  Request.ScanoutId        = ScanoutId;
  Request.ResourceId       = ResourceId;

  return VirtioGpuSendCommand (
           VgpuDev,
           VirtioGpuCmdSetScanout,
           FALSE,                  // Fence
           &Request.Header,
           sizeof Request
           );
}

EFI_STATUS
VirtioGpuTransferToHost2d (
  IN OUT VGPU_DEV *VgpuDev,
  IN     UINT32   X,
  IN     UINT32   Y,
  IN     UINT32   Width,
  IN     UINT32   Height,
  IN     UINT64   Offset,
  IN     UINT32   ResourceId
  )
{
  volatile VIRTIO_GPU_CMD_TRANSFER_TO_HOST_2D Request;

  if (ResourceId == 0) {
    return EFI_INVALID_PARAMETER;
  }

  Request.Rectangle.X      = X;
  Request.Rectangle.Y      = Y;
  Request.Rectangle.Width  = Width;
  Request.Rectangle.Height = Height;
  Request.Offset           = Offset;
  Request.ResourceId       = ResourceId;
  Request.Padding          = 0;

  return VirtioGpuSendCommand (
           VgpuDev,
           VirtioGpuCmdTransferToHost2d,
           FALSE,                        // Fence
           &Request.Header,
           sizeof Request
           );
}

EFI_STATUS
VirtioGpuResourceFlush (
  IN OUT VGPU_DEV *VgpuDev,
  IN     UINT32   X,
  IN     UINT32   Y,
  IN     UINT32   Width,
  IN     UINT32   Height,
  IN     UINT32   ResourceId
  )
{
  volatile VIRTIO_GPU_RESOURCE_FLUSH Request;

  if (ResourceId == 0) {
    return EFI_INVALID_PARAMETER;
  }

  Request.Rectangle.X      = X;
  Request.Rectangle.Y      = Y;
  Request.Rectangle.Width  = Width;
  Request.Rectangle.Height = Height;
  Request.ResourceId       = ResourceId;
  Request.Padding          = 0;

  return VirtioGpuSendCommand (
           VgpuDev,
           VirtioGpuCmdResourceFlush,
           FALSE,                     // Fence
           &Request.Header,
           sizeof Request
           );
}
