/** @file

  EFI_GRAPHICS_OUTPUT_PROTOCOL member functions for the VirtIo GPU driver.

  Copyright (C) 2016, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>

#include "VirtioGpu.h"

/**
  Release guest-side and host-side resources that are related to an initialized
  VGPU_GOP.Gop.

  param[in,out] VgpuGop  The VGPU_GOP object to release resources for.

                         On input, the caller is responsible for having called
                         VgpuGop->Gop.SetMode() at least once successfully.
                         (This is equivalent to the requirement that
                         VgpuGop->BackingStore be non-NULL. It is also
                         equivalent to the requirement that VgpuGop->ResourceId
                         be nonzero.)

                         On output, resources will be released, and
                         VgpuGop->BackingStore and VgpuGop->ResourceId will be
                         nulled.

  param[in] DisableHead  Whether this head (scanout) currently references the
                         resource identified by VgpuGop->ResourceId. Only pass
                         FALSE when VgpuGop->Gop.SetMode() calls this function
                         while switching between modes, and set it to TRUE
                         every other time.
**/
VOID
ReleaseGopResources (
  IN OUT VGPU_GOP  *VgpuGop,
  IN     BOOLEAN   DisableHead
  )
{
  EFI_STATUS  Status;

  ASSERT (VgpuGop->ResourceId != 0);
  ASSERT (VgpuGop->BackingStore != NULL);

  //
  // If any of the following host-side destruction steps fail, we can't get out
  // of an inconsistent state, so we'll hang. In general errors in object
  // destruction can hardly be recovered from.
  //
  if (DisableHead) {
    //
    // Dissociate head (scanout) #0 from the currently used 2D host resource,
    // by setting ResourceId=0 for it.
    //
    Status = VirtioGpuSetScanout (
               VgpuGop->ParentBus, // VgpuDev
               0,
               0,
               0,
               0,                  // X, Y, Width, Height
               0,                  // ScanoutId
               0                   // ResourceId
               );
    //
    // HACK BEGINS HERE
    //
    // According to the GPU Device section of the VirtIo specification, the
    // above operation is valid:
    //
    // "The driver can use resource_id = 0 to disable a scanout."
    //
    // However, in practice QEMU does not allow us to disable head (scanout) #0
    // -- it rejects the command with response code 0x1202
    // (VIRTIO_GPU_RESP_ERR_INVALID_SCANOUT_ID). Looking at the QEMU source
    // code, function virtio_gpu_set_scanout() in "hw/display/virtio-gpu.c",
    // this appears fully intentional, despite not being documented in the
    // spec.
    //
    // Surprisingly, ignoring the error here, and proceeding to release
    // host-side resources that presumably underlie head (scanout) #0, work
    // without any problems -- the driver survives repeated "disconnect" /
    // "connect -r" commands in the UEFI shell.
    //
    // So, for now, let's just suppress the error.
    //
    Status = EFI_SUCCESS;
    //
    // HACK ENDS HERE
    //

    ASSERT_EFI_ERROR (Status);
    if (EFI_ERROR (Status)) {
      CpuDeadLoop ();
    }
  }

  //
  // Detach backing pages from the currently used 2D host resource.
  //
  Status = VirtioGpuResourceDetachBacking (
             VgpuGop->ParentBus, // VgpuDev
             VgpuGop->ResourceId // ResourceId
             );
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    CpuDeadLoop ();
  }

  //
  // Unmap and release backing pages.
  //
  VirtioGpuUnmapAndFreeBackingStore (
    VgpuGop->ParentBus,      // VgpuDev
    VgpuGop->NumberOfPages,  // NumberOfPages
    VgpuGop->BackingStore,   // HostAddress
    VgpuGop->BackingStoreMap // Mapping
    );
  VgpuGop->BackingStore    = NULL;
  VgpuGop->NumberOfPages   = 0;
  VgpuGop->BackingStoreMap = NULL;

  //
  // Destroy the currently used 2D host resource.
  //
  Status = VirtioGpuResourceUnref (
             VgpuGop->ParentBus, // VgpuDev
             VgpuGop->ResourceId // ResourceId
             );
  ASSERT_EFI_ERROR (Status);
  if (EFI_ERROR (Status)) {
    CpuDeadLoop ();
  }

  VgpuGop->ResourceId = 0;
}

//
// The resolutions supported by this driver.
//
typedef struct {
  UINT32    Width;
  UINT32    Height;
} GOP_RESOLUTION;

STATIC CONST GOP_RESOLUTION  mGopResolutions[] = {
  { 640,  480  },
  { 800,  480  },
  { 800,  600  },
  { 832,  624  },
  { 960,  640  },
  { 1024, 600  },
  { 1024, 768  },
  { 1152, 864  },
  { 1152, 870  },
  { 1280, 720  },
  { 1280, 760  },
  { 1280, 768  },
  { 1280, 800  },
  { 1280, 960  },
  { 1280, 1024 },
  { 1360, 768  },
  { 1366, 768  },
  { 1400, 1050 },
  { 1440, 900  },
  { 1600, 900  },
  { 1600, 1200 },
  { 1680, 1050 },
  { 1920, 1080 },
  { 1920, 1200 },
  { 1920, 1440 },
  { 2000, 2000 },
  { 2048, 1536 },
  { 2048, 2048 },
  { 2560, 1440 },
  { 2560, 1600 },
  { 2560, 2048 },
  { 2800, 2100 },
  { 3200, 2400 },
  { 3840, 2160 },
  { 4096, 2160 },
  { 7680, 4320 },
  { 8192, 4320 },
};

//
// Macro for casting VGPU_GOP.Gop to VGPU_GOP.
//
#define VGPU_GOP_FROM_GOP(GopPointer) \
          CR (GopPointer, VGPU_GOP, Gop, VGPU_GOP_SIG)

STATIC
VOID
EFIAPI
GopNativeResolution (
  IN  VGPU_GOP  *VgpuGop,
  OUT UINT32    *XRes,
  OUT UINT32    *YRes
  )
{
  volatile VIRTIO_GPU_RESP_DISPLAY_INFO  DisplayInfo;
  EFI_STATUS                             Status;
  UINTN                                  Index;

  Status = VirtioGpuGetDisplayInfo (VgpuGop->ParentBus, &DisplayInfo);
  if (Status != EFI_SUCCESS) {
    return;
  }

  for (Index = 0; Index < VIRTIO_GPU_MAX_SCANOUTS; Index++) {
    if (!DisplayInfo.Pmodes[Index].Enabled ||
        !DisplayInfo.Pmodes[Index].Rectangle.Width ||
        !DisplayInfo.Pmodes[Index].Rectangle.Height)
    {
      continue;
    }

    DEBUG ((
      DEBUG_INFO,
      "%a: #%d: %dx%d\n",
      __func__,
      Index,
      DisplayInfo.Pmodes[Index].Rectangle.Width,
      DisplayInfo.Pmodes[Index].Rectangle.Height
      ));
    if ((*XRes == 0) || (*YRes == 0)) {
      *XRes = DisplayInfo.Pmodes[Index].Rectangle.Width;
      *YRes = DisplayInfo.Pmodes[Index].Rectangle.Height;
    }
  }
}

STATIC
VOID
EFIAPI
GopInitialize (
  IN  EFI_GRAPHICS_OUTPUT_PROTOCOL  *This
  )
{
  VGPU_GOP    *VgpuGop;
  EFI_STATUS  Status;
  UINT32      XRes = 0, YRes = 0, Index;

  VgpuGop = VGPU_GOP_FROM_GOP (This);

  //
  // Set up the Gop -> GopMode -> GopModeInfo pointer chain, and the other
  // (nonzero) constant fields.
  //
  // No direct framebuffer access is supported, only Blt() is.
  //
  VgpuGop->Gop.Mode = &VgpuGop->GopMode;

  VgpuGop->GopMode.MaxMode    = (UINT32)(ARRAY_SIZE (mGopResolutions));
  VgpuGop->GopMode.Info       = &VgpuGop->GopModeInfo;
  VgpuGop->GopMode.SizeOfInfo = sizeof VgpuGop->GopModeInfo;

  VgpuGop->GopModeInfo.PixelFormat = PixelBltOnly;

  //
  // query host for display resolution
  //
  GopNativeResolution (VgpuGop, &XRes, &YRes);
  if ((XRes < 640) || (YRes < 480)) {
    /* ignore hint, GraphicsConsoleDxe needs 640x480 or larger */
    return;
  }

  if (PcdGet8 (PcdVideoResolutionSource) == 0) {
    Status = PcdSet32S (PcdVideoHorizontalResolution, XRes);
    ASSERT_RETURN_ERROR (Status);
    Status = PcdSet32S (PcdVideoVerticalResolution, YRes);
    ASSERT_RETURN_ERROR (Status);
    Status = PcdSet8S (PcdVideoResolutionSource, 2);
    ASSERT_RETURN_ERROR (Status);
  }

  VgpuGop->NativeXRes = XRes;
  VgpuGop->NativeYRes = YRes;
  for (Index = 0; Index < ARRAY_SIZE (mGopResolutions); Index++) {
    if ((mGopResolutions[Index].Width == XRes) &&
        (mGopResolutions[Index].Height == YRes))
    {
      // native resolution already is in mode list
      return;
    }
  }

  // add to mode list
  VgpuGop->GopMode.MaxMode++;
}

//
// EFI_GRAPHICS_OUTPUT_PROTOCOL member functions.
//
STATIC
EFI_STATUS
EFIAPI
GopQueryMode (
  IN  EFI_GRAPHICS_OUTPUT_PROTOCOL          *This,
  IN  UINT32                                ModeNumber,
  OUT UINTN                                 *SizeOfInfo,
  OUT EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  **Info
  )
{
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  *GopModeInfo;

  if ((Info == NULL) ||
      (SizeOfInfo == NULL) ||
      (ModeNumber >= This->Mode->MaxMode))
  {
    return EFI_INVALID_PARAMETER;
  }

  GopModeInfo = AllocateZeroPool (sizeof *GopModeInfo);
  if (GopModeInfo == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  if (ModeNumber < ARRAY_SIZE (mGopResolutions)) {
    GopModeInfo->HorizontalResolution = mGopResolutions[ModeNumber].Width;
    GopModeInfo->VerticalResolution   = mGopResolutions[ModeNumber].Height;
  } else {
    VGPU_GOP  *VgpuGop = VGPU_GOP_FROM_GOP (This);
    GopModeInfo->HorizontalResolution = VgpuGop->NativeXRes;
    GopModeInfo->VerticalResolution   = VgpuGop->NativeYRes;
  }

  GopModeInfo->PixelFormat       = PixelBltOnly;
  GopModeInfo->PixelsPerScanLine = GopModeInfo->HorizontalResolution;

  *SizeOfInfo = sizeof *GopModeInfo;
  *Info       = GopModeInfo;
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
GopSetMode (
  IN  EFI_GRAPHICS_OUTPUT_PROTOCOL  *This,
  IN  UINT32                        ModeNumber
  )
{
  VGPU_GOP                              *VgpuGop;
  UINT32                                NewResourceId;
  UINTN                                 NewNumberOfBytes;
  UINTN                                 NewNumberOfPages;
  VOID                                  *NewBackingStore;
  EFI_PHYSICAL_ADDRESS                  NewBackingStoreDeviceAddress;
  VOID                                  *NewBackingStoreMap;
  UINTN                                 SizeOfInfo;
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  *GopModeInfo;

  EFI_STATUS  Status;
  EFI_STATUS  Status2;

  if (!This->Mode) {
    // SetMode() call in InitVgpuGop() triggers this.
    GopInitialize (This);
  }

  Status = GopQueryMode (This, ModeNumber, &SizeOfInfo, &GopModeInfo);
  if (Status != EFI_SUCCESS) {
    return Status;
  }

  VgpuGop = VGPU_GOP_FROM_GOP (This);

  //
  // Distinguish the first (internal) call from the other (protocol consumer)
  // calls.
  //
  if (VgpuGop->ResourceId == 0) {
    //
    // This is the first time we create a host side resource.
    //
    NewResourceId = 1;
  } else {
    //
    // We already have an active host side resource. Create the new one without
    // interfering with the current one, so that we can cleanly bail out on
    // error, without disturbing the current graphics mode.
    //
    // The formula below will alternate between IDs 1 and 2.
    //
    NewResourceId = 3 - VgpuGop->ResourceId;
  }

  //
  // Create the 2D host resource.
  //
  Status = VirtioGpuResourceCreate2d (
             VgpuGop->ParentBus,                // VgpuDev
             NewResourceId,                     // ResourceId
             VirtioGpuFormatB8G8R8X8Unorm,      // Format
             GopModeInfo->HorizontalResolution, // Width
             GopModeInfo->VerticalResolution    // Height
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Allocate, zero and map guest backing store, for bus master common buffer
  // operation.
  //
  NewNumberOfBytes = GopModeInfo->HorizontalResolution *
                     GopModeInfo->VerticalResolution * sizeof (UINT32);
  NewNumberOfPages = EFI_SIZE_TO_PAGES (NewNumberOfBytes);
  Status           = VirtioGpuAllocateZeroAndMapBackingStore (
                       VgpuGop->ParentBus,            // VgpuDev
                       NewNumberOfPages,              // NumberOfPages
                       &NewBackingStore,              // HostAddress
                       &NewBackingStoreDeviceAddress, // DeviceAddress
                       &NewBackingStoreMap            // Mapping
                       );
  if (EFI_ERROR (Status)) {
    goto DestroyHostResource;
  }

  //
  // Attach backing store to the host resource.
  //
  Status = VirtioGpuResourceAttachBacking (
             VgpuGop->ParentBus,           // VgpuDev
             NewResourceId,                // ResourceId
             NewBackingStoreDeviceAddress, // BackingStoreDeviceAddress
             NewNumberOfPages              // NumberOfPages
             );
  if (EFI_ERROR (Status)) {
    goto UnmapAndFreeBackingStore;
  }

  //
  // Point head (scanout) #0 to the host resource.
  //
  Status = VirtioGpuSetScanout (
             VgpuGop->ParentBus,                 // VgpuDev
             0,                                  // X
             0,                                  // Y
             GopModeInfo->HorizontalResolution,  // Width
             GopModeInfo->VerticalResolution,    // Height
             0,                                  // ScanoutId
             NewResourceId                       // ResourceId
             );
  if (EFI_ERROR (Status)) {
    goto DetachBackingStore;
  }

  //
  // If this is not the first (i.e., internal) call, then we have to (a) flush
  // the new resource to head (scanout) #0, after having flipped the latter to
  // the former above, plus (b) release the old resources.
  //
  if (VgpuGop->ResourceId != 0) {
    Status = VirtioGpuResourceFlush (
               VgpuGop->ParentBus,                 // VgpuDev
               0,                                  // X
               0,                                  // Y
               GopModeInfo->HorizontalResolution,  // Width
               GopModeInfo->VerticalResolution,    // Height
               NewResourceId                       // ResourceId
               );
    if (EFI_ERROR (Status)) {
      //
      // Flip head (scanout) #0 back to the current resource. If this fails, we
      // cannot continue, as this error occurs on the error path and is
      // therefore non-recoverable.
      //
      Status2 = VirtioGpuSetScanout (
                  VgpuGop->ParentBus,                        // VgpuDev
                  0,                                         // X
                  0,                                         // Y
                  VgpuGop->GopModeInfo.HorizontalResolution, // Width
                  VgpuGop->GopModeInfo.VerticalResolution,   // Height
                  0,                                         // ScanoutId
                  VgpuGop->ResourceId                        // ResourceId
                  );
      ASSERT_EFI_ERROR (Status2);
      if (EFI_ERROR (Status2)) {
        CpuDeadLoop ();
      }

      goto DetachBackingStore;
    }

    //
    // Flush successful; release the old resources (without disabling head
    // (scanout) #0).
    //
    ReleaseGopResources (VgpuGop, FALSE /* DisableHead */);
  }

  //
  // This is either the first (internal) call when we have no old resources
  // yet, or we've changed the mode successfully and released the old
  // resources.
  //
  ASSERT (VgpuGop->ResourceId == 0);
  ASSERT (VgpuGop->BackingStore == NULL);

  VgpuGop->ResourceId      = NewResourceId;
  VgpuGop->BackingStore    = NewBackingStore;
  VgpuGop->NumberOfPages   = NewNumberOfPages;
  VgpuGop->BackingStoreMap = NewBackingStoreMap;

  //
  // Populate Mode and ModeInfo (mutable fields only).
  //
  VgpuGop->GopMode.Mode = ModeNumber;
  CopyMem (&VgpuGop->GopModeInfo, GopModeInfo, sizeof VgpuGop->GopModeInfo);
  FreePool (GopModeInfo);
  return EFI_SUCCESS;

DetachBackingStore:
  Status2 = VirtioGpuResourceDetachBacking (VgpuGop->ParentBus, NewResourceId);
  ASSERT_EFI_ERROR (Status2);
  if (EFI_ERROR (Status2)) {
    CpuDeadLoop ();
  }

UnmapAndFreeBackingStore:
  VirtioGpuUnmapAndFreeBackingStore (
    VgpuGop->ParentBus, // VgpuDev
    NewNumberOfPages,   // NumberOfPages
    NewBackingStore,    // HostAddress
    NewBackingStoreMap  // Mapping
    );

DestroyHostResource:
  Status2 = VirtioGpuResourceUnref (VgpuGop->ParentBus, NewResourceId);
  ASSERT_EFI_ERROR (Status2);
  if (EFI_ERROR (Status2)) {
    CpuDeadLoop ();
  }

  FreePool (GopModeInfo);
  return Status;
}

STATIC
EFI_STATUS
EFIAPI
GopBlt (
  IN  EFI_GRAPHICS_OUTPUT_PROTOCOL       *This,
  IN  EFI_GRAPHICS_OUTPUT_BLT_PIXEL      *BltBuffer    OPTIONAL,
  IN  EFI_GRAPHICS_OUTPUT_BLT_OPERATION  BltOperation,
  IN  UINTN                              SourceX,
  IN  UINTN                              SourceY,
  IN  UINTN                              DestinationX,
  IN  UINTN                              DestinationY,
  IN  UINTN                              Width,
  IN  UINTN                              Height,
  IN  UINTN                              Delta         OPTIONAL
  )
{
  VGPU_GOP    *VgpuGop;
  UINT32      CurrentHorizontal;
  UINT32      CurrentVertical;
  UINTN       SegmentSize;
  UINTN       Y;
  UINTN       ResourceOffset;
  EFI_STATUS  Status;

  VgpuGop           = VGPU_GOP_FROM_GOP (This);
  CurrentHorizontal = VgpuGop->GopModeInfo.HorizontalResolution;
  CurrentVertical   = VgpuGop->GopModeInfo.VerticalResolution;

  //
  // We can avoid pixel format conversion in the guest because the internal
  // representation of EFI_GRAPHICS_OUTPUT_BLT_PIXEL and that of
  // VirtioGpuFormatB8G8R8X8Unorm are identical.
  //
  SegmentSize = Width * sizeof (UINT32);

  //
  // Delta is relevant for operations that read a rectangle from, or write a
  // rectangle to, BltBuffer.
  //
  // In these cases, Delta is the stride of BltBuffer, in bytes. If Delta is
  // zero, then Width is the entire width of BltBuffer, and the stride is
  // supposed to be calculated from Width.
  //
  if ((BltOperation == EfiBltVideoToBltBuffer) ||
      (BltOperation == EfiBltBufferToVideo))
  {
    if (Delta == 0) {
      Delta = SegmentSize;
    }
  }

  //
  // For operations that write to the display, check if the destination fits
  // onto the display.
  //
  if ((BltOperation == EfiBltVideoFill) ||
      (BltOperation == EfiBltBufferToVideo) ||
      (BltOperation == EfiBltVideoToVideo))
  {
    if ((DestinationX > CurrentHorizontal) ||
        (Width > CurrentHorizontal - DestinationX) ||
        (DestinationY > CurrentVertical) ||
        (Height > CurrentVertical - DestinationY))
    {
      return EFI_INVALID_PARAMETER;
    }
  }

  //
  // For operations that read from the display, check if the source fits onto
  // the display.
  //
  if ((BltOperation == EfiBltVideoToBltBuffer) ||
      (BltOperation == EfiBltVideoToVideo))
  {
    if ((SourceX > CurrentHorizontal) ||
        (Width > CurrentHorizontal - SourceX) ||
        (SourceY > CurrentVertical) ||
        (Height > CurrentVertical - SourceY))
    {
      return EFI_INVALID_PARAMETER;
    }
  }

  //
  // Render the request. For requests that do not modify the display, there
  // won't be further steps.
  //
  switch (BltOperation) {
    case EfiBltVideoFill:
      //
      // Write data from the BltBuffer pixel (0, 0) directly to every pixel of
      // the video display rectangle (DestinationX, DestinationY) (DestinationX +
      // Width, DestinationY + Height). Only one pixel will be used from the
      // BltBuffer. Delta is NOT used.
      //
      for (Y = 0; Y < Height; ++Y) {
        SetMem32 (
          VgpuGop->BackingStore +
          (DestinationY + Y) * CurrentHorizontal + DestinationX,
          SegmentSize,
          *(UINT32 *)BltBuffer
          );
      }

      break;

    case EfiBltVideoToBltBuffer:
      //
      // Read data from the video display rectangle (SourceX, SourceY) (SourceX +
      // Width, SourceY + Height) and place it in the BltBuffer rectangle
      // (DestinationX, DestinationY ) (DestinationX + Width, DestinationY +
      // Height). If DestinationX or DestinationY is not zero then Delta must be
      // set to the length in bytes of a row in the BltBuffer.
      //
      for (Y = 0; Y < Height; ++Y) {
        CopyMem (
          (UINT8 *)BltBuffer +
          (DestinationY + Y) * Delta + DestinationX * sizeof *BltBuffer,
          VgpuGop->BackingStore +
          (SourceY + Y) * CurrentHorizontal + SourceX,
          SegmentSize
          );
      }

      return EFI_SUCCESS;

    case EfiBltBufferToVideo:
      //
      // Write data from the BltBuffer rectangle (SourceX, SourceY) (SourceX +
      // Width, SourceY + Height) directly to the video display rectangle
      // (DestinationX, DestinationY) (DestinationX + Width, DestinationY +
      // Height). If SourceX or SourceY is not zero then Delta must be set to the
      // length in bytes of a row in the BltBuffer.
      //
      for (Y = 0; Y < Height; ++Y) {
        CopyMem (
          VgpuGop->BackingStore +
          (DestinationY + Y) * CurrentHorizontal + DestinationX,
          (UINT8 *)BltBuffer +
          (SourceY + Y) * Delta + SourceX * sizeof *BltBuffer,
          SegmentSize
          );
      }

      break;

    case EfiBltVideoToVideo:
      //
      // Copy from the video display rectangle (SourceX, SourceY) (SourceX +
      // Width, SourceY + Height) to the video display rectangle (DestinationX,
      // DestinationY) (DestinationX + Width, DestinationY + Height). The
      // BltBuffer and Delta are not used in this mode.
      //
      // A single invocation of CopyMem() handles overlap between source and
      // destination (that is, within a single line), but for multiple
      // invocations, we must handle overlaps.
      //
      if (SourceY < DestinationY) {
        Y = Height;
        while (Y > 0) {
          --Y;
          CopyMem (
            VgpuGop->BackingStore +
            (DestinationY + Y) * CurrentHorizontal + DestinationX,
            VgpuGop->BackingStore +
            (SourceY + Y) * CurrentHorizontal + SourceX,
            SegmentSize
            );
        }
      } else {
        for (Y = 0; Y < Height; ++Y) {
          CopyMem (
            VgpuGop->BackingStore +
            (DestinationY + Y) * CurrentHorizontal + DestinationX,
            VgpuGop->BackingStore +
            (SourceY + Y) * CurrentHorizontal + SourceX,
            SegmentSize
            );
        }
      }

      break;

    default:
      return EFI_INVALID_PARAMETER;
  }

  //
  // For operations that wrote to the display, submit the updated area to the
  // host -- update the host resource from guest memory.
  //
  ResourceOffset = sizeof (UINT32) * (DestinationY * CurrentHorizontal +
                                      DestinationX);
  Status = VirtioGpuTransferToHost2d (
             VgpuGop->ParentBus,   // VgpuDev
             (UINT32)DestinationX, // X
             (UINT32)DestinationY, // Y
             (UINT32)Width,        // Width
             (UINT32)Height,       // Height
             ResourceOffset,       // Offset
             VgpuGop->ResourceId   // ResourceId
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Flush the updated resource to the display.
  //
  Status = VirtioGpuResourceFlush (
             VgpuGop->ParentBus,   // VgpuDev
             (UINT32)DestinationX, // X
             (UINT32)DestinationY, // Y
             (UINT32)Width,        // Width
             (UINT32)Height,       // Height
             VgpuGop->ResourceId   // ResourceId
             );
  return Status;
}

//
// Template for initializing VGPU_GOP.Gop.
//
CONST EFI_GRAPHICS_OUTPUT_PROTOCOL  mGopTemplate = {
  GopQueryMode,
  GopSetMode,
  GopBlt,
  NULL          // Mode, to be overwritten in the actual protocol instance
};
