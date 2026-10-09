/** @file
  Private interfaces for the unflattened device-tree dispatcher.

  Copyright (c) 2026, Arm Ltd. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Uefi.h>
#include <Library/UfdtDispatcherLib.h>
#include <Protocol/DevicePath.h>
#include <Protocol/DriverBinding.h>
#include <Protocol/UfdtNode.h>

/** Check whether a driver matches an available device-tree node.

  Match compatible strings and check that the node and its ancestors are enabled.

  @param[in] Driver  Driver with a NULL-terminated compatible list.
  @param[in] Device  Non-NULL parsed node to match.

  @retval TRUE   Availability and compatible checks pass.
  @retval FALSE  The node is unavailable or no compatible string matches.
**/
BOOLEAN
EFIAPI
UfdtDriverIsCompatible (
  IN     UFDT_BINDING_DRIVER  *Driver,
  IN CONST UFDT_DEVICE_NODE   *Device
  );

/** Test whether this binding can claim a matching FDT node.

  @param[in] This                 Binding embedded in UFDT_BINDING_DRIVER.
  @param[in] ControllerHandle     Controller carrying the FDT node protocol.
  @param[in] RemainingDevicePath  Unused; discovery always considers the whole controller.

  @retval EFI_SUCCESS          Success.
  @retval EFI_UNSUPPORTED      The controller is unsupported.
  @retval EFI_ALREADY_STARTED  This binding already owns the node protocol.
  @retval Other                Other OpenProtocol() failures, including ownership by another driver.
**/
EFI_STATUS
EFIAPI
UfdtBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  );

/** Claim a node and run the binding's configuration callback.

  @param[in] This                 Registered binding with a Configure callback.
  @param[in] ControllerHandle     Handle carrying the node protocol to claim.
  @param[in] RemainingDevicePath  Ignored; configuration covers the whole node.

  @retval EFI_SUCCESS           Success.
  @retval EFI_OUT_OF_RESOURCES  Out of memory.
  @retval EFI_ALREADY_STARTED   This binding already owns the node protocol.
  @retval EFI_NOT_READY         The callback requires an unavailable provider.
  @retval EFI_UNSUPPORTED       The node protocol is absent or the callback
                                does not support this device.
  @retval Other                 Protocol-open or configuration-callback errors.
**/
EFI_STATUS
EFIAPI
UfdtBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     *RemainingDevicePath OPTIONAL
  );

/** Refuse removal of resource bindings.

  @param[in] This               Binding embedded in UFDT_BINDING_DRIVER.
  @param[in] ControllerHandle   Controller requested for disconnection.
  @param[in] NumberOfChildren   Number of requested child handles.
  @param[in] ChildHandleBuffer  Optional child handle array.

  @retval EFI_ACCESS_DENIED  Successful bindings remain until firmware exit.
**/
EFI_STATUS
EFIAPI
UfdtBindingStop (
  IN EFI_DRIVER_BINDING_PROTOCOL  *This,
  IN EFI_HANDLE                   ControllerHandle,
  IN UINTN                        NumberOfChildren,
  IN EFI_HANDLE                   *ChildHandleBuffer OPTIONAL
  );

/** Unflatten a DTB into UFDT_DEVICE_NODE structures.

  Copy node names, paths and properties. Release the result with
  UfdtFreeDeviceTree(). On failure, free partial allocations and set *Root to NULL.

  @param[in]  Fdt   Device tree.
  @param[out] Root  Root UFDT_DEVICE_NODE.

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  Invalid parameter
  @retval EFI_COMPROMISED_DATA   Invalid DTB.
  @retval EFI_OUT_OF_RESOURCES   Out of memory.
**/
EFI_STATUS
EFIAPI
UfdtParseDeviceTree (
  IN  VOID              *Fdt,
  OUT UFDT_DEVICE_NODE  **Root
  );

/** Release an unpublished tree, including a partially parsed tree.

  Remove installed protocols and free device paths before calling this function.

  @param[in] Root  Root of the entire tree, with Parent == NULL, or NULL.
**/
VOID
EFIAPI
UfdtFreeDeviceTree (
  IN UFDT_DEVICE_NODE  *Root
  );

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
  );

/** Locate the firmware DTB and initialize FDT resource discovery.

  @param[in] ImageHandle  Dispatcher image retained for callback lifetime.
  @param[in] SystemTable  Firmware system table; unused.

  @retval EFI_NOT_FOUND         The FDT HOB is absent.
  @retval EFI_COMPROMISED_DATA  Invalid DTB.
  @retval EFI_SUCCESS           Success.
  @retval Other                 Dispatcher initialization failed.
**/
EFI_STATUS
EFIAPI
UfdtDispatcherEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  );
