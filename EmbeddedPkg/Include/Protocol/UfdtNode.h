/** @file
  Shared device-tree node and its optional DXE handle.

  Copyright (c) 2026, Arm Ltd. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#pragma once

#include <Uefi/UefiBaseType.h>
#include <Protocol/DevicePath.h>

typedef struct _UFDT_NODE_PROPERTY  UFDT_NODE_PROPERTY;
typedef struct _UFDT_DEVICE_NODE    UFDT_DEVICE_NODE;

/** An owned copy of a DT property, in its original wire format.

  Length is in bytes; integer cells in Value remain big-endian. Empty
  properties have Length zero. Names and values do not point into the DTB.
**/
struct _UFDT_NODE_PROPERTY {
  CONST CHAR8           *Name;
  UINT32                Length;
  CONST VOID            *Value;
  UFDT_NODE_PROPERTY    *Next;
};

/** Resident node identity and enumeration-time device-tree snapshot.

  The dispatcher creates one handle for every DTB node, including disabled
  nodes. Consumers borrow this interface and its strings and properties until
  ExitBootServices(). Parent, Child and Sibling describe the DT hierarchy;
  children retain their order in the blob. Handle identifies this node's UEFI
  handle. The node protocol exposes this structure directly. The dispatcher parser
  allocates each node and property without creating handles. Handle
  and DevicePath remain NULL until DXE publication. Consumers must not modify
  the tree.

  Name uses the "name" property when present, otherwise the node's unit name
  without its address suffix (empty for the root). The "name" property is copied
  without string validation. FullName is the absolute
  DT path; Path is an alias of FullName. Phandle is
  zero when absent, with "phandle" taking precedence over "linux,phandle".

  Properties, Name and Phandle are copies captured during enumeration. Later
  edits to the handoff DTB, including status reservations for the OS, do not
  update this snapshot. Binding and resource parsers use the copied properties
  and node hierarchy. Renaming, removing or adding nodes after enumeration is
  not supported.
**/
struct _UFDT_DEVICE_NODE {
  CONST CHAR8                 *Name;
  UINT32                      Phandle;
  CONST CHAR8                 *FullName;
  UFDT_NODE_PROPERTY          *Properties;
  UFDT_DEVICE_NODE            *Parent;
  UFDT_DEVICE_NODE            *Child;
  UFDT_DEVICE_NODE            *Sibling;
  /// Alias of FullName.
  CONST CHAR8                 *Path;
  /// Allocated UEFI device path; NULL before DXE publication.
  EFI_DEVICE_PATH_PROTOCOL    *DevicePath;
  /// Handle carrying this node's protocols; NULL before DXE publication.
  EFI_HANDLE                  Handle;
};

/// Claimed BY_DRIVER by one resource parser on this node.
extern EFI_GUID  gEdkiiUfdtNodeProtocolGuid;
