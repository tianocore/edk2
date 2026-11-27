/** @file
  vm launch update protocol
  qemu fwcfg implementation

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <ConfidentialComputingGuestAttr.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemEncryptSevLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/QemuFwCfgLib.h>
#include <Library/ResetSystemLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/VmLaunchUpdate.h>

// ---------------------------------------------------------------------
// qemu firmware config interface

#define VM_LAUNCH_UPDATE_FILE_NAME  "etc/vmlaunchupdate"

// Status
#define VM_LAUNCH_UPDATE_STS_SUCCESS          0
#define VM_LAUNCH_UPDATE_STS_LOAD_ERROR       1
#define VM_LAUNCH_UPDATE_STS_IGVM_INIT_ERROR  2
#define VM_LAUNCH_UPDATE_STS_UNKNOWN_ERROR    0xffff

typedef struct {
  UINT16    Version;
  UINT16    Status;
  UINT8     Padding[4];
  UINT64    Capabilities;
  UINT64    Control;
  UINT64    FirmwareImageBase;
  UINT64    FirmwareImageSize;
  UINT64    OpaqueBlobBase;
  UINT64    OpaqueBlobSize;
} VM_LAUNCH_UPDATE_FW_CFG;

// ---------------------------------------------------------------------
// driver internals

#define VM_LAUNCH_UPDATE_DEVICE_SIGNATURE  SIGNATURE_32('V', 'M', 'L', 'U')

#define VM_LAUNCH_UPDATE_DEVICE_FROM_THIS(ptr) \
  CR (ptr, VM_LAUNCH_UPDATE_DEVICE, Protocol, VM_LAUNCH_UPDATE_DEVICE_SIGNATURE)

typedef struct {
  VOID      *Ptr;
  UINT64    Size;
  UINT64    Pages;
  VOID      *Buffer;
} VM_LAUNCH_BUFFER;

typedef struct VM_LAUNCH_UPDATE_DEVICE {
  UINTN                        Signature;
  VM_LAUNCH_UPDATE_PROTOCOL    Protocol;
  FIRMWARE_CONFIG_ITEM         FwCfgItem;
  UINTN                        FwCfgSize;

  VM_LAUNCH_BUFFER             FirmwareImage;
  VM_LAUNCH_BUFFER             OpaqueBlob;
  VM_LAUNCH_UPDATE_FW_CFG      Config;
  BOOLEAN                      LoadOk;
} VM_LAUNCH_UPDATE_DEVICE;

STATIC
VOID
VmLaunchUpdateFileRead (
  IN VM_LAUNCH_UPDATE_DEVICE  *Device
  )
{
  QemuFwCfgSelectItem (Device->FwCfgItem);
  QemuFwCfgReadBytes (sizeof (Device->Config), &Device->Config);
}

STATIC
VOID
VmLaunchUpdateFileWrite (
  IN VM_LAUNCH_UPDATE_DEVICE  *Device
  )
{
  QemuFwCfgSelectItem (Device->FwCfgItem);
  QemuFwCfgWriteBytes (sizeof (Device->Config), &Device->Config);
}

STATIC
EFI_STATUS
VmLaunchBufferSetShared (
  IN VM_LAUNCH_BUFFER  *Buffer
  )
{
  UINT64      CcAttr;
  EFI_STATUS  Status;

  CcAttr = PcdGet64 (PcdConfidentialComputingGuestAttr);

  if (!CcAttr) {
    // nothing to do
    return EFI_SUCCESS;
  }

  if (CC_GUEST_IS_SEV (CcAttr)) {
    Status = MemEncryptSevClearPageEncMask (0, (UINTN)(Buffer->Buffer), Buffer->Pages);
    DEBUG ((DEBUG_INFO, "%a: MemEncryptSevClearPageEncMask: %r\n", __func__, Status));
    return Status;
  }

  if (CC_GUEST_IS_TDX (CcAttr)) {
    // TODO
    DEBUG ((DEBUG_ERROR, "%a: TDX: not implemented yet\n", __func__));
    return EFI_UNSUPPORTED;
  }

  DEBUG ((DEBUG_ERROR, "%a: unknown CC guest type\n", __func__));
  return EFI_UNSUPPORTED;
}

STATIC
EFI_STATUS
VmLaunchBufferSetPrivate (
  IN VM_LAUNCH_BUFFER  *Buffer
  )
{
  UINT64      CcAttr;
  EFI_STATUS  Status;

  CcAttr = PcdGet64 (PcdConfidentialComputingGuestAttr);

  if (!CcAttr) {
    // nothing to do
    return EFI_SUCCESS;
  }

  if (CC_GUEST_IS_SEV (CcAttr)) {
    Status = MemEncryptSevSetPageEncMask (0, (UINTN)(Buffer->Buffer), Buffer->Pages);
    DEBUG ((DEBUG_INFO, "%a: MemEncryptSevSetPageEncMask: %r\n", __func__, Status));
    return Status;
  }

  if (CC_GUEST_IS_TDX (CcAttr)) {
    // TODO
    DEBUG ((DEBUG_ERROR, "%a: TDX: not implemented yet\n", __func__));
    return EFI_UNSUPPORTED;
  }

  DEBUG ((DEBUG_ERROR, "%a: unknown CC guest type\n", __func__));
  return EFI_UNSUPPORTED;
}

STATIC
EFI_STATUS
VmLaunchBufferInit (
  IN VM_LAUNCH_BUFFER  *Buffer,
  IN VOID              *Ptr,
  IN UINT64            Size
  )
{
  EFI_STATUS  Status;
  UINT64      Pages;

  if ((Ptr == NULL) || (Size == 0)) {
    return EFI_INVALID_PARAMETER;
  }

  Pages          = EFI_SIZE_TO_PAGES (Size);
  Buffer->Buffer = AllocatePages (Pages);
  if (Buffer->Buffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Buffer->Ptr   = Ptr;
  Buffer->Size  = Size;
  Buffer->Pages = Pages;
  DEBUG ((
    DEBUG_INFO,
    "%a: ptr %p, size 0x%x, pages 0x%x, buffer %p\n",
    __func__,
    Buffer->Ptr,
    Buffer->Size,
    Buffer->Pages,
    Buffer->Buffer
    ));

  Status = VmLaunchBufferSetShared (Buffer);
  if (Status != EFI_SUCCESS) {
    return Status;
  }

  CopyMem (Buffer->Buffer, Ptr, Size);
  return EFI_SUCCESS;
}

STATIC
VOID
VmLaunchBufferFini (
  IN VM_LAUNCH_BUFFER  *Buffer
  )
{
  if (Buffer->Buffer) {
    VmLaunchBufferSetPrivate (Buffer);
    FreePages (Buffer->Buffer, Buffer->Pages);
  }

  ZeroMem (Buffer, sizeof (*Buffer));
}

// ---------------------------------------------------------------------
// protocol implementation

STATIC
UINT64
EFIAPI
VmLaunchUpdateGetCapabilities (
  IN VM_LAUNCH_UPDATE_PROTOCOL  *This
  )
{
  VM_LAUNCH_UPDATE_DEVICE  *Device = VM_LAUNCH_UPDATE_DEVICE_FROM_THIS (This);

  return Device->Config.Capabilities;
}

STATIC
EFI_STATUS
EFIAPI
VmLaunchUpdateSetControl (
  IN VM_LAUNCH_UPDATE_PROTOCOL  *This,
  IN UINT64                     Control
  )
{
  VM_LAUNCH_UPDATE_DEVICE  *Device       = VM_LAUNCH_UPDATE_DEVICE_FROM_THIS (This);
  UINT64                   SupportedMask = (VM_LAUNCH_UPDATE_CAP_IGVM |
                                            VM_LAUNCH_UPDATE_CTL_DISABLE |
                                            VM_LAUNCH_UPDATE_CTL_ONCE);

  if ((Control & ~SupportedMask) != 0) {
    DEBUG ((DEBUG_INFO, "%a: unsupported control bit set\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  if ((Control & VM_LAUNCH_UPDATE_CAP_IGVM) == 0) {
    DEBUG ((DEBUG_INFO, "%a: igvm control bit not set\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  Device->Config.Control = Control;
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
VmLaunchUpdateSetImage (
  IN VM_LAUNCH_UPDATE_PROTOCOL  *This,
  IN VOID                       *Image,
  IN UINT64                     Size
  )
{
  VM_LAUNCH_UPDATE_DEVICE  *Device = VM_LAUNCH_UPDATE_DEVICE_FROM_THIS (This);
  EFI_STATUS               Status;

  if ((Image == NULL) && (Size == 0)) {
    VmLaunchBufferFini (&Device->FirmwareImage);
    return EFI_SUCCESS;
  }

  Status = VmLaunchBufferInit (&Device->FirmwareImage, Image, Size);
  if (Status != EFI_SUCCESS) {
    return Status;
  }

  Device->Config.FirmwareImageBase = (UINTN)Device->FirmwareImage.Buffer;
  Device->Config.FirmwareImageSize = Device->FirmwareImage.Size;
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
VmLaunchUpdateSetOpaque (
  IN VM_LAUNCH_UPDATE_PROTOCOL  *This,
  IN VOID                       *Opaque,
  IN UINT64                     Size
  )
{
  VM_LAUNCH_UPDATE_DEVICE  *Device = VM_LAUNCH_UPDATE_DEVICE_FROM_THIS (This);
  EFI_STATUS               Status;

  if ((Opaque == NULL) && (Size == 0)) {
    VmLaunchBufferFini (&Device->OpaqueBlob);
    return EFI_SUCCESS;
  }

  Status = VmLaunchBufferInit (&Device->OpaqueBlob, Opaque, Size);
  if (Status != EFI_SUCCESS) {
    return Status;
  }

  Device->Config.OpaqueBlobBase = (UINTN)Device->OpaqueBlob.Buffer;
  Device->Config.OpaqueBlobSize = Device->OpaqueBlob.Size;
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
VmLaunchUpdateLoad (
  IN VM_LAUNCH_UPDATE_PROTOCOL  *This
  )
{
  VM_LAUNCH_UPDATE_DEVICE  *Device = VM_LAUNCH_UPDATE_DEVICE_FROM_THIS (This);

  if ((Device->Config.Control & VM_LAUNCH_UPDATE_CAP_IGVM) == 0) {
    DEBUG ((DEBUG_INFO, "%a: igvm control bit not set\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  if (!Device->Config.FirmwareImageBase) {
    DEBUG ((DEBUG_INFO, "%a: no firmware image set\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

  Device->LoadOk = FALSE;
  VmLaunchUpdateFileWrite (Device);
  VmLaunchUpdateFileRead (Device);

  switch (Device->Config.Status) {
    case VM_LAUNCH_UPDATE_STS_SUCCESS:
      Device->LoadOk = TRUE;
      DEBUG ((DEBUG_INFO, "%a: success\n", __func__));
      return EFI_SUCCESS;
    case VM_LAUNCH_UPDATE_STS_LOAD_ERROR:
      return EFI_LOAD_ERROR;
    case VM_LAUNCH_UPDATE_STS_IGVM_INIT_ERROR:
      return EFI_INVALID_PARAMETER;
    default:
      DEBUG ((DEBUG_INFO, "%a: error (0x%x)\n", __func__, Device->Config.Status));
      return EFI_DEVICE_ERROR;
  }
}

STATIC
EFI_STATUS
EFIAPI
VmLaunchUpdateLaunch (
  IN VM_LAUNCH_UPDATE_PROTOCOL  *This
  )
{
  VM_LAUNCH_UPDATE_DEVICE  *Device = VM_LAUNCH_UPDATE_DEVICE_FROM_THIS (This);

  if (!Device->LoadOk) {
    DEBUG ((DEBUG_INFO, "%a: LoadOk is FALSE\n", __func__));
    return EFI_INVALID_PARAMETER;
  }

 #if 0
  // should we do this?
  VmLaunchBufferFini (&Device->FirmwareImage);
  VmLaunchBufferFini (&Device->OpaqueBlob);
 #endif

  DEBUG ((DEBUG_INFO, "%a: reset now\n", __func__));
  ResetCold ();
  return EFI_DEVICE_ERROR;
}

// ---------------------------------------------------------------------
// entry point

STATIC VM_LAUNCH_UPDATE_DEVICE  mLaunchUpdateDevice = {
  .Signature = VM_LAUNCH_UPDATE_DEVICE_SIGNATURE,
  .Protocol  = {
    .Version         = VM_LAUNCH_UPDATE_PROTOCOL_VERSION,
    .GetCapabilities = VmLaunchUpdateGetCapabilities,
    .SetControl      = VmLaunchUpdateSetControl,
    .SetImage        = VmLaunchUpdateSetImage,
    .SetOpaque       = VmLaunchUpdateSetOpaque,
    .Load            = VmLaunchUpdateLoad,
    .Launch          = VmLaunchUpdateLaunch,
  },
};

EFI_STATUS
EFIAPI
VmLaunchUpdateDxeEntrypoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS               Status;
  FIRMWARE_CONFIG_ITEM     FwCfgItem;
  UINTN                    FwCfgSize;
  VM_LAUNCH_UPDATE_DEVICE  *Device;

  if (!QemuFwCfgIsAvailable ()) {
    DEBUG ((DEBUG_INFO, "%a: QemuFwCfg not available\n", __func__));
    return EFI_REQUEST_UNLOAD_IMAGE;
  }

  Status = QemuFwCfgFindFile (VM_LAUNCH_UPDATE_FILE_NAME, &FwCfgItem, &FwCfgSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_INFO, "%a: %a not available (%r)\n", __func__, VM_LAUNCH_UPDATE_FILE_NAME, Status));
    return EFI_REQUEST_UNLOAD_IMAGE;
  }

  if (FwCfgSize != sizeof (VM_LAUNCH_UPDATE_FW_CFG)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: size mismatch (expected %d, found %d)\n",
      __func__,
      sizeof (VM_LAUNCH_UPDATE_FW_CFG),
      FwCfgSize
      ));
    return EFI_DEVICE_ERROR;
  }

  Device            = &mLaunchUpdateDevice;
  Device->FwCfgItem = FwCfgItem;
  Device->FwCfgSize = FwCfgSize;
  VmLaunchUpdateFileRead (Device);
  DEBUG ((
    DEBUG_INFO,
    "%a: item=0x%x size=0x%x / version=%d caps=%aigvm\n",
    __func__,
    FwCfgItem,
    FwCfgSize,
    Device->Config.Version,
    (Device->Config.Capabilities & VM_LAUNCH_UPDATE_CAP_IGVM) ? "+" : "-"
    ));

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &ImageHandle,
                  &gVMLaunchUpdateProtocolGuid,
                  &Device->Protocol,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: install protocol failed: %r\n", __func__, Status));
    return Status;
  }

  DEBUG ((DEBUG_INFO, "%a: ok (handle=0x%x, protocol=%p)\n", __func__, ImageHandle, &Device->Protocol));
  return EFI_SUCCESS;
}
