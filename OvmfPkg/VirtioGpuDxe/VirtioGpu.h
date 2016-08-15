/** @file

  Internal type and macro definitions for the Virtio GPU hybrid driver.

  Copyright (C) 2016, Red Hat, Inc.

  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution. The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, WITHOUT
  WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _VIRTIO_GPU_DXE_H_
#define _VIRTIO_GPU_DXE_H_

#include <Library/DebugLib.h>
#include <Library/UefiLib.h>
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
  VIRTIO_DEVICE_PROTOCOL   *VirtIo;

  //
  // BusName carries a customized name for
  // EFI_COMPONENT_NAME2_PROTOCOL.GetControllerName(). It is expressed in table
  // form because it can theoretically support several languages. Never NULL.
  //
  EFI_UNICODE_STRING_TABLE *BusName;

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
  VGPU_GOP                 *Child;
} VGPU_DEV;

//
// The Graphics Output Protocol wrapper structure.
//
#define VGPU_GOP_SIG SIGNATURE_64 ('V', 'G', 'P', 'U', '_', 'G', 'O', 'P')

struct VGPU_GOP_STRUCT {
  UINT64                               Signature;

  //
  // ParentBus points to the parent VGPU_DEV object. Never NULL.
  //
  VGPU_DEV                             *ParentBus;

  //
  // GopName carries a customized name for
  // EFI_COMPONENT_NAME2_PROTOCOL.GetControllerName(). It is expressed in table
  // form because it can theoretically support several languages. Never NULL.
  //
  EFI_UNICODE_STRING_TABLE             *GopName;

  //
  // GopHandle is the UEFI child handle that carries the device path ending
  // with the ACPI ADR node, and the Graphics Output Protocol. Never NULL.
  //
  EFI_HANDLE                           GopHandle;

  //
  // The GopDevicePath field is the device path installed on GopHandle,
  // ending with an ACPI ADR node. Never NULL.
  //
  EFI_DEVICE_PATH_PROTOCOL             *GopDevicePath;

  //
  // The Gop field is installed on the child handle as Graphics Output Protocol
  // interface.
  //
  // For now it is just a placeholder.
  //
  UINT8                                Gop;
};

#endif // _VIRTIO_GPU_DXE_H_
