/** @file

  Internal type and macro definitions for the Virtio GPU hybrid driver.

  Copyright (C) 2016, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _VIRTIO_GPU_DXE_H_
#define _VIRTIO_GPU_DXE_H_

#include <IndustryStandard/VirtioGpu.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiLib.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/VirtioDevice.h>

//
// Forward declaration of VGPU_GOP.
//
typedef struct VGPU_GOP_STRUCT VGPU_GOP;

//
// The abstraction that directly corresponds to a Virtio GPU device.
//
// This structure will be installed on the handle that has the VirtIo Device
// Protocol interface, with GUID gEfiCallerIdGuid. A similar trick is employed
// in TerminalDxe, and it is necessary so that we can look up VGPU_DEV just
// from the VirtIo Device Protocol handle in the Component Name 2 Protocol
// implementation.
//
typedef struct {
  //
  // VirtIo represents access to the Virtio GPU device. Never NULL.
  //
  VIRTIO_DEVICE_PROTOCOL      *VirtIo;

  //
  // BusName carries a customized name for
  // EFI_COMPONENT_NAME2_PROTOCOL.GetControllerName(). It is expressed in table
  // form because it can theoretically support several languages. Never NULL.
  //
  EFI_UNICODE_STRING_TABLE    *BusName;

  //
  // VirtIo ring used for VirtIo communication.
  //
  VRING                       Ring;

  //
  // Token associated with Ring's mapping for bus master common buffer
  // operation, from VirtioRingMap().
  //
  VOID                        *RingMap;

  //
  // Event to be signaled at ExitBootServices().
  //
  EFI_EVENT                   ExitBoot;

  //
  // Common running counter for all VirtIo GPU requests that ask for fencing.
  //
  UINT64                      FenceId;

  //
  // The Child field references the GOP wrapper structure. If this pointer is
  // NULL, then the hybrid driver has bound (i.e., started) the
  // VIRTIO_DEVICE_PROTOCOL controller without producing the child GOP
  // controller (that is, after Start() was called with RemainingDevicePath
  // pointing to and End of Device Path node). Child can be created and
  // destroyed, even repeatedly, independently of VGPU_DEV.
  //
  // In practice, this field represents the single head (scanout) that we
  // support.
  //
  VGPU_GOP    *Child;
} VGPU_DEV;

//
// The Graphics Output Protocol wrapper structure.
//
#define VGPU_GOP_SIG  SIGNATURE_64 ('V', 'G', 'P', 'U', '_', 'G', 'O', 'P')

struct VGPU_GOP_STRUCT {
  UINT64                                  Signature;

  //
  // ParentBus points to the parent VGPU_DEV object. Never NULL.
  //
  VGPU_DEV                                *ParentBus;

  //
  // GopName carries a customized name for
  // EFI_COMPONENT_NAME2_PROTOCOL.GetControllerName(). It is expressed in table
  // form because it can theoretically support several languages. Never NULL.
  //
  EFI_UNICODE_STRING_TABLE                *GopName;

  //
  // GopHandle is the UEFI child handle that carries the device path ending
  // with the ACPI ADR node, and the Graphics Output Protocol. Never NULL.
  //
  EFI_HANDLE                              GopHandle;

  //
  // The GopDevicePath field is the device path installed on GopHandle,
  // ending with an ACPI ADR node. Never NULL.
  //
  EFI_DEVICE_PATH_PROTOCOL                *GopDevicePath;

  //
  // The Gop field is installed on the child handle as Graphics Output Protocol
  // interface.
  //
  EFI_GRAPHICS_OUTPUT_PROTOCOL            Gop;

  //
  // Referenced by Gop.Mode, GopMode provides a summary about the supported
  // graphics modes, and the current mode.
  //
  EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE       GopMode;

  //
  // Referenced by GopMode.Info, GopModeInfo provides detailed information
  // about the current mode.
  //
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION    GopModeInfo;

  //
  // Identifier of the 2D host resource that is in use by this head (scanout)
  // of the VirtIo GPU device. Zero until the first successful -- internal --
  // Gop.SetMode() call, never zero afterwards.
  //
  UINT32                                  ResourceId;

  //
  // A number of whole pages providing the backing store for the 2D host
  // resource identified by ResourceId above. NULL until the first successful
  // -- internal -- Gop.SetMode() call, never NULL afterwards.
  //
  UINT32                                  *BackingStore;
  UINTN                                   NumberOfPages;

  //
  // Token associated with BackingStore's mapping for bus master common
  // buffer operation. BackingStoreMap is valid if, and only if,
  // BackingStore is non-NULL.
  //
  VOID                                    *BackingStoreMap;

  //
  // native display resolution
  //
  UINT32                                  NativeXRes;
  UINT32                                  NativeYRes;
};

//
// VirtIo GPU initialization, and commands (primitives) for the GPU device.
//

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
  IN OUT VGPU_DEV  *VgpuDev
  );

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
  IN OUT VGPU_DEV  *VgpuDev
  );

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
  IN  VGPU_DEV              *VgpuDev,
  IN  UINTN                 NumberOfPages,
  OUT VOID                  **HostAddress,
  OUT EFI_PHYSICAL_ADDRESS  *DeviceAddress,
  OUT VOID                  **Mapping
  );

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
  IN VGPU_DEV  *VgpuDev,
  IN UINTN     NumberOfPages,
  IN VOID      *HostAddress,
  IN VOID      *Mapping
  );

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
  IN EFI_EVENT  Event,
  IN VOID       *Context
  );

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
                                 code has been logged on the DEBUG_ERROR level.

  @return                        Codes for unexpected errors in VirtIo
                                 messaging.

  For the command-specific parameters, please consult the GPU Device section of
  the VirtIo 1.0 specification (see references in
  "OvmfPkg/Include/IndustryStandard/VirtioGpu.h").
**/
EFI_STATUS
VirtioGpuResourceCreate2d (
  IN OUT VGPU_DEV            *VgpuDev,
  IN     UINT32              ResourceId,
  IN     VIRTIO_GPU_FORMATS  Format,
  IN     UINT32              Width,
  IN     UINT32              Height
  );

EFI_STATUS
VirtioGpuResourceUnref (
  IN OUT VGPU_DEV  *VgpuDev,
  IN     UINT32    ResourceId
  );

EFI_STATUS
VirtioGpuResourceAttachBacking (
  IN OUT VGPU_DEV              *VgpuDev,
  IN     UINT32                ResourceId,
  IN     EFI_PHYSICAL_ADDRESS  BackingStoreDeviceAddress,
  IN     UINTN                 NumberOfPages
  );

EFI_STATUS
VirtioGpuResourceDetachBacking (
  IN OUT VGPU_DEV  *VgpuDev,
  IN     UINT32    ResourceId
  );

EFI_STATUS
VirtioGpuSetScanout (
  IN OUT VGPU_DEV  *VgpuDev,
  IN     UINT32    X,
  IN     UINT32    Y,
  IN     UINT32    Width,
  IN     UINT32    Height,
  IN     UINT32    ScanoutId,
  IN     UINT32    ResourceId
  );

EFI_STATUS
VirtioGpuTransferToHost2d (
  IN OUT VGPU_DEV  *VgpuDev,
  IN     UINT32    X,
  IN     UINT32    Y,
  IN     UINT32    Width,
  IN     UINT32    Height,
  IN     UINT64    Offset,
  IN     UINT32    ResourceId
  );

EFI_STATUS
VirtioGpuResourceFlush (
  IN OUT VGPU_DEV  *VgpuDev,
  IN     UINT32    X,
  IN     UINT32    Y,
  IN     UINT32    Width,
  IN     UINT32    Height,
  IN     UINT32    ResourceId
  );

EFI_STATUS
VirtioGpuGetDisplayInfo (
  IN OUT VGPU_DEV                        *VgpuDev,
  volatile VIRTIO_GPU_RESP_DISPLAY_INFO  *Response
  );

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
  );

//
// Template for initializing VGPU_GOP.Gop.
//
extern CONST EFI_GRAPHICS_OUTPUT_PROTOCOL  mGopTemplate;

#endif // _VIRTIO_GPU_DXE_H_
