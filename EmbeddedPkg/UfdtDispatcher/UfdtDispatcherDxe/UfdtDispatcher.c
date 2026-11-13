/** @file
  Represent every DTB node and connect FDT resource adapters before BDS.

  Copyright (c) 2026, Arm Ltd. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include "UfdtDispatcherInternal.h"

#include <PiDxe.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/DevicePathLib.h>
#include <Library/FdtLib.h>
#include <Library/UefiLib.h>
#include <Library/UfdtDispatcherLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>

/** Publish a parsed node as a UEFI protocol interface.

  Publish the node and its device path on one handle. The node must remain
  allocated while its protocols are installed.

  @param[in,out] Device  Unpublished node with a valid Path and a published parent,
                         if any. Receives its Handle and DevicePath on success.

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  Invalid parameter
  @retval EFI_OUT_OF_RESOURCES   Out of memory.
  @retval Other                  Protocol installation failed.
**/
STATIC
EFI_STATUS
PublishDeviceNode (
  IN OUT UFDT_DEVICE_NODE  *Device
  )
{
  VENDOR_DEVICE_PATH  *PathNode;
  UINTN               PathLength;
  EFI_STATUS          Status;

  if ((Device->Handle != NULL) ||
      ((Device->Parent != NULL) && (Device->Parent->Handle == NULL)))
  {
    return EFI_INVALID_PARAMETER;
  }

  PathLength = AsciiStrSize (Device->Path);
  if (PathLength > MAX_UINT16 - sizeof (*PathNode)) {
    return EFI_INVALID_PARAMETER;
  }

  PathNode = AllocateZeroPool (sizeof (*PathNode) + PathLength);
  if (PathNode == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  PathNode->Header.Type    = HARDWARE_DEVICE_PATH;
  PathNode->Header.SubType = HW_VENDOR_DP;
  SetDevicePathNodeLength (PathNode, sizeof (*PathNode) + PathLength);
  CopyGuid (&PathNode->Guid, &gEdkiiUfdtNodeProtocolGuid);
  CopyMem (PathNode + 1, Device->Path, PathLength);
  Device->DevicePath = AppendDevicePathNode (
                         (Device->Parent == NULL) ? NULL : Device->Parent->DevicePath,
                         &PathNode->Header
                         );
  FreePool (PathNode);
  if (Device->DevicePath == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Device->Handle,
                  &gEdkiiUfdtNodeProtocolGuid,
                  Device,
                  &gEfiDevicePathProtocolGuid,
                  Device->DevicePath,
                  NULL
                  );
  if (EFI_ERROR (Status)) {
    FreePool (Device->DevicePath);
    Device->DevicePath = NULL;
    Device->Handle     = NULL;
  }

  return Status;
}

/** Publish every node in depth-first preorder before resource binding.

  Publish parents before children. On failure, use UnpublishDeviceTree() to
  remove previously published nodes.

  @param[in,out] Root  Root of the unpublished tree, or NULL for an empty tree.

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  Invalid parameter
  @retval EFI_OUT_OF_RESOURCES   Out of memory.
  @retval Other                  Protocol installation failed.
**/
STATIC
EFI_STATUS
PublishDeviceTree (
  IN OUT UFDT_DEVICE_NODE  *Root
  )
{
  UFDT_DEVICE_NODE  *Device;
  EFI_STATUS        Status;

  for (Device = Root; Device != NULL; Device = UfdtGetNextDeviceNode (Root, Device)) {
    Status = PublishDeviceNode (Device);
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  return EFI_SUCCESS;
}

/** Remove node protocols and device paths in reverse preorder.

  Call before installing resource bindings. Stop on the first failure and
  retain the tree while any node remains published.

  @param[in,out] Root  Root of the completely or partially published tree.
                       NULL is an empty tree.

  @retval EFI_SUCCESS  Success.
  @retval Other        Protocol uninstallation failed; tree storage must be retained.
**/
STATIC
EFI_STATUS
UnpublishDeviceTree (
  IN OUT UFDT_DEVICE_NODE  *Root
  )
{
  UFDT_DEVICE_NODE  *Device;
  EFI_STATUS        Status;

  if (Root == NULL) {
    return EFI_SUCCESS;
  }

  // Undo publication in reverse order, then unpublish Root itself.
  Device = NULL;
  do {
    Device = UfdtGetPrevDeviceNode (Root, Device);
    if (Device == NULL) {
      Device = Root;
    }

    if (Device->Handle != NULL) {
      Status = gBS->UninstallMultipleProtocolInterfaces (
                      Device->Handle,
                      &gEdkiiUfdtNodeProtocolGuid,
                      Device,
                      &gEfiDevicePathProtocolGuid,
                      Device->DevicePath,
                      NULL
                      );
      if (EFI_ERROR (Status)) {
        return Status;
      }

      Device->Handle = NULL;
      FreePool (Device->DevicePath);
      Device->DevicePath = NULL;
    }
  } while (Device != Root);

  return EFI_SUCCESS;
}

/** Connect matching devices in the current dependency stage.

  Only bindings from the current and earlier phases are installed.

  @param[in] Root   Root of the published device tree.
  @param[in] Phase  Stage whose bindings have just been installed.
**/
STATIC
VOID
EFIAPI
ConnectPhase (
  IN UFDT_DEVICE_NODE     *Root,
  IN UFDT_DISPATCH_PHASE  Phase
  )
{
  UFDT_BINDING_DRIVER  *Driver;
  UFDT_DEVICE_NODE     *Device;
  EFI_STATUS           Status;

  for (Driver = UfdtGetNextDriver (NULL); Driver != NULL; Driver = UfdtGetNextDriver (Driver)) {
    if (Driver->Phase != Phase) {
      continue;
    }

    for (Device = Root; Device != NULL; Device = UfdtGetNextDeviceNode (Root, Device)) {
      if (!UfdtDriverIsCompatible (Driver, Device)) {
        continue;
      }

      Status = gBS->ConnectController (Device->Handle, NULL, NULL, FALSE);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "UfdtDriver: discovery %a %a: %r\n", Driver->Name, Device->Path, Status));
      }
    }
  }
}

/** Install and connect resource bindings in dependency order.

  Connection failures do not stop later phases. Installed bindings remain
  resident if a later installation fails.

  @param[in]  Root              Root of the published device tree.
  @param[in]  ImageHandle       Owning image shared by all bindings.
  @param[out] BindingInstalled  TRUE if the caller must retain the tree and image.

  @retval EFI_SUCCESS  Success.
  @retval Other        Driver Binding protocol installation failed.
**/
STATIC
EFI_STATUS
ConnectBindings (
  IN  UFDT_DEVICE_NODE  *Root,
  IN  EFI_HANDLE        ImageHandle,
  OUT BOOLEAN           *BindingInstalled
  )
{
  UFDT_BINDING_DRIVER  *Driver;
  UFDT_DISPATCH_PHASE  Phase;
  EFI_STATUS           Status;

  *BindingInstalled = FALSE;
  for (Phase = (UFDT_DISPATCH_PHASE)(UfdtPhaseInvalid + 1); Phase < UfdtPhaseMax; Phase++) {
    // Installing only the current phase prevents premature consumer starts.
    for (Driver = UfdtGetNextDriver (NULL); Driver != NULL; Driver = UfdtGetNextDriver (Driver)) {
      if (Driver->Phase != Phase) {
        continue;
      }

      Driver->Binding.Supported   = UfdtBindingSupported;
      Driver->Binding.Start       = UfdtBindingStart;
      Driver->Binding.Stop        = UfdtBindingStop;
      Driver->Binding.Version     = 0x10;
      Driver->Binding.ImageHandle = ImageHandle;
      Status                      = gBS->InstallMultipleProtocolInterfaces (
                                           &Driver->Binding.DriverBindingHandle,
                                           &gEfiDriverBindingProtocolGuid,
                                           &Driver->Binding,
                                           NULL
                                           );
      if (EFI_ERROR (Status)) {
        return Status;
      }

      *BindingInstalled = TRUE;
    }

    ConnectPhase (Root, Phase);
  }

  return EFI_SUCCESS;
}

/** Run post-connection callbacks in driver registry order.

**/
STATIC
VOID
UfdtSetPcds (
  VOID
  )
{
  UFDT_BINDING_DRIVER  *Driver;
  EFI_STATUS           Status;

  for (Driver = UfdtGetNextDriver (NULL); Driver != NULL; Driver = UfdtGetNextDriver (Driver)) {
    if (Driver->SetPcds == NULL) {
      continue;
    }

    Status = Driver->SetPcds (Driver);
    if (EFI_ERROR (Status) && ((Status != EFI_NOT_FOUND) || !IsListEmpty (&Driver->Devices))) {
      DEBUG ((DEBUG_ERROR, "UfdtDriver: %a SetPcds failed: %r\n", Driver->Name, Status));
    }
  }
}

/** Publish absence markers after all probing and resource selection complete.

  Skip bindings that published a resource. Probing or setup failures also count
  as absence if no usable resource was published.
  A prepared device excluded by a generic-class singleton policy may still
  leave its binding absent. Repeated calls preserve already installed markers.

  @retval EFI_SUCCESS  All applicable absence markers are installed.
  @retval Other        Protocol lookup or installation failed.
**/
STATIC
EFI_STATUS
PublishAbsentProtocols (
  VOID
  )
{
  UFDT_BINDING_DRIVER  *Driver;
  EFI_STATUS           Status;
  VOID                 *Interface;

  for (Driver = UfdtGetNextDriver (NULL); Driver != NULL; Driver = UfdtGetNextDriver (Driver)) {
    Status = gBS->LocateProtocol (Driver->PresentProtocol, NULL, &Interface);
    if (!EFI_ERROR (Status)) {
      continue;
    }

    if (Status != EFI_NOT_FOUND) {
      return Status;
    }

    Status = gBS->HandleProtocol (Driver->Binding.DriverBindingHandle, Driver->AbsentProtocol, &Interface);
    if (!EFI_ERROR (Status)) {
      continue;
    }

    if (Status != EFI_UNSUPPORTED) {
      return Status;
    }

    Status = gBS->InstallMultipleProtocolInterfaces (
                    &Driver->Binding.DriverBindingHandle,
                    Driver->AbsentProtocol,
                    NULL,
                    NULL
                    );
    if (EFI_ERROR (Status)) {
      return Status;
    }
  }

  return EFI_SUCCESS;
}

/** Reserve successfully published firmware devices in the handoff DTB.

  Only consider drivers with RESERVE_NODE and device handles carrying that
  driver's resource protocol. Resolve paths after every edit because
  adding a status property can move subsequent nodes. Keep the copied tree intact.

  @param[in,out] Fdt  Original writable handoff DTB, retained by the platform.

  @retval EFI_SUCCESS           Success.
  @retval EFI_COMPROMISED_DATA  A selected node is missing from the handoff DTB.
  @retval EFI_DEVICE_ERROR      A selected node could not be disabled.
  @retval Other                 Resource-protocol lookup failed.
**/
STATIC
EFI_STATUS
ReserveDeviceNodes (
  IN OUT VOID  *Fdt
  )
{
  UFDT_BINDING_DRIVER     *Driver;
  UFDT_BINDING_DEVICE     *Device;
  UFDT_RESOURCE_PROTOCOL  *Resource;
  LIST_ENTRY              *Link;
  EFI_STATUS              Status;
  INT32                   Node;

  for (Driver = UfdtGetNextDriver (NULL); Driver != NULL; Driver = UfdtGetNextDriver (Driver)) {
    if ((Driver->Flags & UFDT_BINDING_FLAG_RESERVE_NODE) == 0) {
      continue;
    }

    for (Link = GetFirstNode (&Driver->Devices);
         !IsNull (&Driver->Devices, Link);
         Link = GetNextNode (&Driver->Devices, Link))
    {
      Device = BASE_CR (Link, UFDT_BINDING_DEVICE, Link);

      // Check that the Device has been selected (cf. FLAG_SINGLE_DEVICE)
      Status = gBS->HandleProtocol (Device->Handle, Driver->PresentProtocol, (VOID **)&Resource);
      if (Status == EFI_UNSUPPORTED) {
        continue;
      }

      if (EFI_ERROR (Status)) {
        return Status;
      }

      Node = FdtPathOffset (Fdt, Device->Device->FullName);
      if (Node < 0) {
        DEBUG ((DEBUG_ERROR, "UfdtDispatcher: reserved node %a is missing: %d\n", Device->Device->FullName, Node));
        return EFI_COMPROMISED_DATA;
      }

      if (FdtSetProp (Fdt, Node, "status", "disabled", sizeof ("disabled")) != 0) {
        DEBUG ((DEBUG_ERROR, "UfdtDispatcher: cannot reserve %a\n", Device->Device->FullName));
        return EFI_DEVICE_ERROR;
      }
    }
  }

  return EFI_SUCCESS;
}

/** Disable selected firmware-owned nodes before the boot option is loaded.

  Stop boot on failure: resource consumers may already own the hardware, so
  allowing the OS to claim it is unsafe. Close the event only after success.
  The platform must keep the handoff DTB at Context valid and must not replace
  it or undo these reservations before handing it to the OS.

  @param[in] Event    ReadyToBoot notification event.
  @param[in] Context  Original writable handoff DTB.
**/
STATIC
VOID
EFIAPI
UfdtReadyToBoot (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS  Status;

  Status = ReserveDeviceNodes (Context);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "UfdtDispatcher: DT reservation failed: %r; stopping boot\n", Status));
    CpuDeadLoop ();
    return;
  }

  gBS->CloseEvent (Event);
}

/** Parse the DTB, publish every node, then connect resource bindings.

  Call once. Keep the image and tree resident after binding installation or
  failed publication rollback. Individual connection failures are logged.
  Publish absence markers after resource selection for bindings that published no resource.
  Register ReadyToBoot before publication to reserve selected nodes for firmware.
  The platform must retain the supplied DTB through handoff without replacing it.

  @param[in] ImageHandle  Dispatcher image retained for callback lifetime.
  @param[in] Fdt          Writable DTB, retained until ExitBootServices().

  @retval EFI_SUCCESS           Success.
  @retval EFI_COMPROMISED_DATA  Invalid DTB.
  @retval EFI_OUT_OF_RESOURCES  Out of memory.
  @retval Other                 Event-registration or initial protocol-publication errors.
**/
EFI_STATUS
EFIAPI
UfdtDispatcherInitialize (
  IN EFI_HANDLE  ImageHandle,
  IN VOID        *Fdt
  )
{
  UFDT_DEVICE_NODE  *Root;
  EFI_STATUS        Status;
  BOOLEAN           BindingInstalled;
  EFI_EVENT         ReadyToBootEvent;

  // Create UFDT_DEVICE_NODE nodes
  Status = UfdtParseDeviceTree (Fdt, &Root);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = EfiCreateEventReadyToBootEx (
             TPL_CALLBACK,
             UfdtReadyToBoot,
             Fdt,
             &ReadyToBootEvent
             );
  if (EFI_ERROR (Status)) {
    UfdtFreeDeviceTree (Root);
    return Status;
  }

  BindingInstalled = FALSE;
  Status           = PublishDeviceTree (Root);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }

  // Try to bind drivers to UFDT_DEVICE_NODE
  Status = ConnectBindings (Root, ImageHandle, &BindingInstalled);
  if (EFI_ERROR (Status)) {
    goto Failed;
  }

  UfdtSetPcds ();

  Status = PublishAbsentProtocols ();
  if (EFI_ERROR (Status)) {
    goto Failed;
  }

  return EFI_SUCCESS;

Failed:
  DEBUG ((DEBUG_ERROR, "UfdtDispatcher: initialization incomplete: %r\n", Status));
  if (!BindingInstalled && !EFI_ERROR (UnpublishDeviceTree (Root))) {
    gBS->CloseEvent (ReadyToBootEvent);
    UfdtFreeDeviceTree (Root);
    return Status;
  }

  return EFI_SUCCESS;
}
