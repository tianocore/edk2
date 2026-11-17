/** @file
  Driver Binding and resource helpers for device-tree discovery.

  Binding descriptors registered by library constructors belong to the image
  linking this library. Separate DXE images do not share a binding registry.
  Descriptors must remain valid for the lifetime of that image.

  Copyright (c) 2026, Arm Ltd. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Uefi.h>
#include <Protocol/DriverBinding.h>
#include <Protocol/UfdtNode.h>
#include <Protocol/UfdtResource.h>

// Interrupt type encodings in a GIC device-tree interrupt specifier.
#define FDT_GIC_SPI  0
#define FDT_GIC_PPI  1

// Supported legacy GIC interrupt ranges, excluding reserved INTIDs.
#define FDT_GIC_PPI_BASE   16
#define FDT_GIC_PPI_COUNT  16
#define FDT_GIC_SPI_BASE   32
#define FDT_GIC_SPI_COUNT  988

typedef struct _UFDT_BINDING_DRIVER UFDT_BINDING_DRIVER;

/// One successfully connected device, owned by its binding until exit.
typedef struct {
  LIST_ENTRY                Link;
  EFI_HANDLE                Handle;
  UFDT_DEVICE_NODE          *Device;
  UFDT_RESOURCE_PROTOCOL    Resource;
} UFDT_BINDING_DEVICE;

/** Set global PCDs after all devices have been connected.

  Select a prepared device and publish readiness after PCD setup.

  @param[in] Driver  Binding descriptor and complete list of prepared devices.

  @retval EFI_SUCCESS    Success.
  @retval EFI_NOT_FOUND  No prepared device is available.
  @retval Other          PCD setup or resource-publication errors.
**/
typedef
EFI_STATUS
(EFIAPI *UFDT_RESOURCE_SET_PCDS)(
  IN UFDT_BINDING_DRIVER *Driver
  );

/** Parse and prepare a device's resources.

  Parse copied node properties and save resources in BindingDevice->Resource.
  Defer legacy PCD setup and selected-resource publication to SetPcds when
  provided; otherwise publish here. On failure, free unpublished resources.
  Consumers invoke Resource.MapMmio, when present, before hardware access.

  @param[in,out] BindingDevice  Identity and output resource snapshot.

  @retval EFI_SUCCESS    Success.
  @retval EFI_NOT_READY  A required provider is unavailable.
  @retval Other          Parsing or direct-publication errors.
**/
typedef
EFI_STATUS
(EFIAPI *UFDT_CONFIGURE)(
  IN OUT UFDT_BINDING_DEVICE *BindingDevice
  );

/** Fixed dependency order, independent of DT and constructor order.

  Publish every node handle before registering resource bindings. Each phase
  configures its providers before the next phase starts. PCD setup runs in
  driver registry order after every connection phase has completed.
  Every adapter must choose a phase; zero-initialized descriptors are invalid.
**/
typedef enum {
  UfdtPhaseInvalid,
  UfdtPhaseClock,
  UfdtPhaseInterruptController,
  UfdtPhaseDevice,
  UfdtPhaseMax
} UFDT_DISPATCH_PHASE;

/// Disable the published device's handoff DT node at ReadyToBoot for runtime use.
#define UFDT_BINDING_FLAG_RESERVE_NODE  BIT0
/// Publish at most one device for the binding's resource protocol GUID.
#define UFDT_BINDING_FLAG_SINGLE_DEVICE  BIT1
/// Publish at most one device for GenericClass, which must be non-NULL.
#define UFDT_BINDING_FLAG_SINGLE_CLASS_DEVICE  BIT2

/** Image-owned descriptor registered once by a NULL-library constructor.

  Initialize policy fields statically and leave dispatcher state zeroed. The
  descriptor and compatible strings must outlive all installed callbacks.
**/
struct _UFDT_BINDING_DRIVER {
  /// Installed by the dispatcher; shared callbacks recover this descriptor.
  EFI_DRIVER_BINDING_PROTOCOL             Binding;
  /// Diagnostic name; not used for matching.
  CONST CHAR8                             *Name;
  /// Required, non-NULL pointer to a NULL-terminated compatible list.
  CONST CHAR8                   *CONST    *Compatible;
  /// Parse resources and record their mapping callback from Driver Binding Start().
  UFDT_CONFIGURE                          Configure;
  /// Optional post-connection PCD setup and selected-resource publication.
  UFDT_RESOURCE_SET_PCDS                  SetPcds;
  /// Successfully connected UFDT_BINDING_DEVICE entries in connection order.
  LIST_ENTRY                              Devices;
  /// Configuration stage; providers precede their consumers.
  UFDT_DISPATCH_PHASE                     Phase;
  /// Static publication policy: UFDT_BINDING_FLAG_* bits.
  UINT32                                  Flags;
  /// Required resource protocol GUID published for a selected device.
  EFI_GUID                                *PresentProtocol;
  /// Required NULL-interface marker published after discovery if no resource was
  /// published, including after setup failure. Installed on DriverBindingHandle.
  /// Must differ from PresentProtocol and GenericClass.
  EFI_GUID                                *AbsentProtocol;
  /// Optional generic class protocol installed with PresentProtocol using a NULL interface.
  /// Must differ from PresentProtocol; required when SINGLE_CLASS_DEVICE is set.
  EFI_GUID                                *GenericClass;
  /// Library-owned registry link; descriptors are visited in reverse registration order.
  LIST_ENTRY                              Link;
};

/** Register a static UFDT driver, without discovering hardware.

  @param[in,out] Driver  Static driver to register once; its device list is initialized.

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  Invalid parameter
**/
EFI_STATUS
EFIAPI
UfdtRegisterDriver (
  IN OUT UFDT_BINDING_DRIVER  *Driver
  );

/** Iterate over registered drivers.

  @param[in] Driver  Registered driver, or NULL to retrieve the first one.

  @retval NULL   Reached the end of the registered drivers.
  @retval Other  Next registered driver.
**/
UFDT_BINDING_DRIVER *
EFIAPI
UfdtGetNextDriver (
  IN UFDT_BINDING_DRIVER  *Driver OPTIONAL
  );

/** Check whether Compatible string matches the Node.

  @param[in] Node        Node to search.
  @param[in] Compatible  Compatible string to match.

  @retval TRUE   A complete entry matches and every list entry is terminated.
  @retval FALSE  Invalid parameter, malformed property or no match.
**/
BOOLEAN
EFIAPI
UfdtNodeIsCompatible (
  IN CONST UFDT_DEVICE_NODE  *Node,
  IN CONST CHAR8             *Compatible
  );

/** Resolve a node's interrupt and decode its architectural GIC interrupt ID.

  Follow interrupt parents and interrupt-map entries internally. Require a
  three-cell, level-sensitive GIC SPI/PPI specifier.
  Extended SPI and PPI ranges are unsupported. The controller's compatible
  property is not checked.

  @param[in]  Node       Node whose interrupt is requested.
  @param[in]  Index      Zero-based interrupt specifier index.
  @param[out] Interrupt  Architectural interrupt ID, valid on success.

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  Invalid parameter
  @retval EFI_NOT_FOUND          Not Found.
  @retval EFI_COMPROMISED_DATA   Invalid DTB or GIC interrupt encoding.
  @retval EFI_UNSUPPORTED        Unsupported GIC cell count or trigger mode.
**/
EFI_STATUS
EFIAPI
UfdtGetInterruptId (
  IN CONST UFDT_DEVICE_NODE  *Node,
  IN UINT32                  Index,
  OUT UINT32                 *Interrupt
  );

/** Return the next node in depth-first order.

  Return the first valid node following this priority list:
  If Device == NULL:
  - The first child node of Root.
  If Device != NULL:
  - The first child node of Device
  - If no child, the sibling node of Device
  - If no sibling, the sibling node of the parent node of Device
  - If none, NULL

  Ascend through parents as needed, stopping at Root.

  @param[in] Root    Root of the subtree to traverse. Must not be NULL.
  @param[in] Device  Current node in Root's subtree, or NULL to start at Root's
                     first child.

  @retval NULL   Finished iterating over the nodes.
  @retval Other  The next node.
**/
UFDT_DEVICE_NODE *
EFIAPI
UfdtGetNextDeviceNode (
  IN CONST UFDT_DEVICE_NODE  *Root,
  IN UFDT_DEVICE_NODE        *Device OPTIONAL
  );

/** Return the previous node in depth-first order.

  Return the first valid node in an inverse fashion from UfdtGetNextDeviceNode().
  Traversal is limited to Root's descendants and does not return Root.

  @param[in] Root    Root of the subtree to traverse. Must not be NULL.
  @param[in] Device  Current node in Root's subtree, or NULL to start at Root's
                     last descendant.

  @retval NULL   Finished iterating over the nodes.
  @retval Other  The previous node.
**/
UFDT_DEVICE_NODE *
EFIAPI
UfdtGetPrevDeviceNode (
  IN CONST UFDT_DEVICE_NODE  *Root,
  IN UFDT_DEVICE_NODE        *Device OPTIONAL
  );

/** Return the root of the tree.

  @param[in] Node  Any node in the tree, or NULL.

  @retval NULL   Node is NULL.
  @retval Other  The root node.
**/
UFDT_DEVICE_NODE *
EFIAPI
UfdtGetRootDeviceNode (
  IN CONST UFDT_DEVICE_NODE  *Node
  );

/** Find a node by phandle.

  @param[in] Root     Root of the subtree to search, including Root itself, or NULL.
  @param[in] Phandle  Phandle to search; zero and MAX_UINT32 are invalid.

  @retval NULL   Invalid parameter or not found.
  @retval Other  The first node whose Phandle equals Phandle.
**/
UFDT_DEVICE_NODE *
EFIAPI
UfdtFindNodeByPhandle (
  IN CONST UFDT_DEVICE_NODE  *Root,
  IN UINT32                  Phandle
  );

/** Return a borrowed value from a node's copied property list.

  @param[in]  Node    Node whose properties are searched.
  @param[in]  Name    Property name to get.
  @param[out] Length  Optional, return the length on success.
                      - 0 if not found.
                      - -1 if an argument or the stored property is invalid.

  @retval NULL   The property is absent or invalid, or an argument is invalid.
                 Length, when supplied, distinguishes absence from an error.
  @retval Other  Borrowed property value, including zero-length properties.
**/
CONST VOID *
EFIAPI
UfdtNodeGetProperty (
  IN CONST UFDT_DEVICE_NODE  *Node,
  IN CONST CHAR8             *Name,
  OUT INT32                  *Length OPTIONAL
  );

/** Read the address-cell count declared on a node.

  Read #address-cells from Node only.
  Default to 2 when absent, cf. DeviceTree specification.

  @param[in] Node  Node whose address-cell count is requested.

  @retval -1     Invalid node or cell-count property.
  @retval 2      The property is absent or explicitly specifies two cells.
  @retval Other  The cell count.
**/
INT32
EFIAPI
UfdtNodeAddressCells (
  IN CONST UFDT_DEVICE_NODE  *Node
  );

/** Read the size-cell count declared on a node.

  Read #size-cells from Node only.
  Default to 1 when absent, cf. DeviceTree specification.
  Zero is valid.

  @param[in] Node  Node whose size-cell count is requested.

  @retval -1     Invalid node or cell-count property.
  @retval 1      The property is absent or explicitly specifies one cell.
  @retval Other  The cell count.
**/
INT32
EFIAPI
UfdtNodeSizeCells (
  IN CONST UFDT_DEVICE_NODE  *Node
  );

/** Check a node's availability and that of every ancestor.

  Each status property must be absent, "ok" or "okay".

  @param[in] Node  Node whose availability is requested, or NULL.

  @retval TRUE   Node and every ancestor permit use.
  @retval FALSE  Invalid node or an unavailable node in its ancestor chain.
**/
BOOLEAN
EFIAPI
UfdtDeviceNodeIsAvailable (
  IN CONST UFDT_DEVICE_NODE  *Node
  );

/** Get a translated address/size pair from a node "reg" property.

  The "reg" property stores addresses in the parent bus address space.
  This helper reads the requested entry and resolves parent bus "ranges"
  mappings up to the root bus to return a CPU physical address.

  The helper supports one or two address and size cells. Require one matching
  entry per nonempty ranges property for the complete region. Missing or
  empty ranges leave the address unchanged.

  @param[in]  Node   Parsed device-tree node.
  @param[in]  Index  Zero-based reg tuple index.
  @param[out] Base   CPU physical base, valid on success.
  @param[out] Size   Optional region length in bytes, valid on success.

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  Invalid parameter
  @retval EFI_NOT_FOUND          Not Found.
  @retval EFI_COMPROMISED_DATA   Invalid DTB.
**/
EFI_STATUS
EFIAPI
UfdtGetTranslatedReg (
  IN  CONST UFDT_DEVICE_NODE  *Node,
  IN  UINT32                  Index,
  OUT UINT64                  *Base,
  OUT UINT64                  *Size OPTIONAL
  );

/** Translate a region from the node's parent address space to CPU space.

  Resolve parent bus "ranges" mappings up to the root bus to return a CPU
  physical address. The complete region must fit in a single "ranges" entry
  at each bus. Missing or empty ranges leave the address unchanged.

  @param[in]  Node        Parsed device-tree node.
  @param[in]  Address     Region base in that parent address space.
  @param[in]  Length      Nonzero byte length of the complete region.
  @param[out] CpuAddress  Translated base, valid on success.

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  Invalid parameter
  @retval EFI_NOT_FOUND          Not Found.
  @retval EFI_COMPROMISED_DATA   Invalid DTB.
**/
EFI_STATUS
EFIAPI
UfdtTranslateAddress (
  IN  CONST UFDT_DEVICE_NODE  *Node,
  IN  UINT64                  Address,
  IN  UINT64                  Length,
  OUT UINT64                  *CpuAddress
  );

/** Publish a device's prepared resource interface using its binding policy.

  Shallow-copy Protocol and publish it after any required PCD setup. Apply
  SINGLE_DEVICE and SINGLE_CLASS_DEVICE policies. The dispatcher reserves
  selected nodes at ReadyToBoot only when RESERVE_NODE is set.
  The caller owns Protocol->Info and must retain it while published.

  @param[in]     Driver        Binding descriptor with publication policy and resource GUID.
  @param[in]     DeviceHandle  Existing device on which to install the interface.
  @param[in]     Protocol      Prepared interface with non-NULL Info.
                               Info must remain valid until ExitBootServices() on success.

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  Invalid parameter
  @retval EFI_UNSUPPORTED        A single-device class already has an instance.
  @retval EFI_OUT_OF_RESOURCES   Out of memory.
  @retval Other                  Protocol-lookup or protocol-publication errors.
**/
EFI_STATUS
EFIAPI
UfdtPublishResource (
  IN     UFDT_BINDING_DRIVER           *Driver,
  IN     EFI_HANDLE                    DeviceHandle,
  IN     CONST UFDT_RESOURCE_PROTOCOL  *Protocol
  );

/** Set legacy PCDs from one prepared device snapshot.

  @param[in] Info  Parsed resource snapshot owned by the binding.

  @retval EFI_SUCCESS  Success.
  @retval Other        Resource validation or PCD write errors.
**/
typedef
EFI_STATUS
(EFIAPI *UFDT_SET_DEVICE_PCDS)(
  IN CONST VOID *Info
  );

/** Select a prepared device and publish its resource interface.

  Select the first prepared device. Keep an existing SINGLE_DEVICE or
  SINGLE_CLASS_DEVICE selection without repeating PCD writes or publication.
  Abort selection on PCD setup or publication failure. Devices remain
  connected; PCD changes are not rolled back.

  @param[in] Driver   Binding descriptor and complete list of prepared devices.
  @param[in] SetPcds  Callback that sets PCDs from one resource snapshot; must not be NULL.

  @retval EFI_SUCCESS            Success.
  @retval EFI_NOT_FOUND          No prepared device is available.
  @retval EFI_INVALID_PARAMETER  Invalid parameter
  @retval Other                  Protocol lookup, PCD setup or publication error.
**/
EFI_STATUS
EFIAPI
UfdtSelectResource (
  IN UFDT_BINDING_DRIVER   *Driver,
  IN UFDT_SET_DEVICE_PCDS  SetPcds
  );
