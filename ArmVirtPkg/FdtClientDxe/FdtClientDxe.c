/** @file
*  FDT client driver
*
*  Copyright (c) 2016, Linaro Ltd. All rights reserved.<BR>
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/HobLib.h>
#include <libfdt.h>

#include <Guid/Fdt.h>
#include <Guid/FdtHob.h>
#include <Guid/PlatformHasDeviceTree.h>

#include <Protocol/FdtClient.h>

STATIC VOID  *mDeviceTreeBase;

STATIC
EFI_STATUS
EFIAPI
GetNodeProperty (
  IN  FDT_CLIENT_PROTOCOL     *This,
  IN  INT32                   Node,
  IN  CONST CHAR8             *PropertyName,
  OUT CONST VOID              **Prop,
  OUT UINT32                  *PropSize OPTIONAL
  )
{
  INT32 Len;

  ASSERT (mDeviceTreeBase != NULL);
  ASSERT (Prop != NULL);

  *Prop = fdt_getprop (mDeviceTreeBase, Node, PropertyName, &Len);
  if (*Prop == NULL) {
    return EFI_NOT_FOUND;
  }

  if (PropSize != NULL) {
    *PropSize = Len;
  }
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
SetNodeProperty (
  IN  FDT_CLIENT_PROTOCOL     *This,
  IN  INT32                   Node,
  IN  CONST CHAR8             *PropertyName,
  IN  CONST VOID              *Prop,
  IN  UINT32                  PropSize
  )
{
  INT32 Ret;

  ASSERT (mDeviceTreeBase != NULL);

  Ret = fdt_setprop (mDeviceTreeBase, Node, PropertyName, Prop, PropSize);
  if (Ret != 0) {
    return EFI_DEVICE_ERROR;
  }

  return EFI_SUCCESS;
}

STATIC
BOOLEAN
IsNodeEnabled (
  INT32                       Node
  )
{
  CONST CHAR8   *NodeStatus;
  INT32         Len;

  //
  // A missing status property implies 'ok' so ignore any errors that
  // may occur here. If the status property is present, check whether
  // it is set to 'ok' or 'okay', anything else is treated as 'disabled'.
  //
  NodeStatus = fdt_getprop (mDeviceTreeBase, Node, "status", &Len);
  if (NodeStatus == NULL) {
    return TRUE;
  }
  if (Len >= 5 && AsciiStrCmp (NodeStatus, "okay") == 0) {
    return TRUE;
  }
  if (Len >= 3 && AsciiStrCmp (NodeStatus, "ok") == 0) {
    return TRUE;
  }
  return FALSE;
}

STATIC
EFI_STATUS
EFIAPI
FindNextCompatibleNode (
  IN  FDT_CLIENT_PROTOCOL     *This,
  IN  CONST CHAR8             *CompatibleString,
  IN  INT32                   PrevNode,
  OUT INT32                   *Node
  )
{
  INT32          Prev, Next;
  CONST CHAR8    *Type, *Compatible;
  INT32          Len;

  ASSERT (mDeviceTreeBase != NULL);
  ASSERT (Node != NULL);

  for (Prev = PrevNode;; Prev = Next) {
    Next = fdt_next_node (mDeviceTreeBase, Prev, NULL);
    if (Next < 0) {
      break;
    }

    if (!IsNodeEnabled (Next)) {
      continue;
    }

    Type = fdt_getprop (mDeviceTreeBase, Next, "compatible", &Len);
    if (Type == NULL) {
      continue;
    }

    //
    // A 'compatible' node may contain a sequence of NUL terminated
    // compatible strings so check each one
    //
    for (Compatible = Type; Compatible < Type + Len && *Compatible;
         Compatible += 1 + AsciiStrLen (Compatible)) {
      if (AsciiStrCmp (CompatibleString, Compatible) == 0) {
        *Node = Next;
        return EFI_SUCCESS;
      }
    }
  }
  return EFI_NOT_FOUND;
}

STATIC
EFI_STATUS
EFIAPI
FindCompatibleNode (
  IN  FDT_CLIENT_PROTOCOL     *This,
  IN  CONST CHAR8             *CompatibleString,
  OUT INT32                   *Node
  )
{
  return FindNextCompatibleNode (This, CompatibleString, 0, Node);
}

STATIC
EFI_STATUS
EFIAPI
FindCompatibleNodeProperty (
  IN  FDT_CLIENT_PROTOCOL     *This,
  IN  CONST CHAR8             *CompatibleString,
  IN  CONST CHAR8             *PropertyName,
  OUT CONST VOID              **Prop,
  OUT UINT32                  *PropSize OPTIONAL
  )
{
  EFI_STATUS        Status;
  INT32             Node;

  Status = FindCompatibleNode (This, CompatibleString, &Node);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return GetNodeProperty (This, Node, PropertyName, Prop, PropSize);
}

STATIC
EFI_STATUS
EFIAPI
FindCompatibleNodeReg (
  IN  FDT_CLIENT_PROTOCOL     *This,
  IN  CONST CHAR8             *CompatibleString,
  OUT CONST VOID              **Reg,
  OUT UINTN                   *AddressCells,
  OUT UINTN                   *SizeCells,
  OUT UINT32                  *RegSize
  )
{
  EFI_STATUS Status;

  ASSERT (RegSize != NULL);

  //
  // Get the 'reg' property of this node. For now, we will assume
  // 8 byte quantities for base and size, respectively.
  // TODO use #cells root properties instead
  //
  Status = FindCompatibleNodeProperty (This, CompatibleString, "reg", Reg,
             RegSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if ((*RegSize % 16) != 0) {
    DEBUG ((EFI_D_ERROR,
      "%a: '%a' compatible node has invalid 'reg' property (size == 0x%x)\n",
      __FUNCTION__, CompatibleString, *RegSize));
    return EFI_NOT_FOUND;
  }

  *AddressCells = 2;
  *SizeCells = 2;

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
EFIAPI
FindNextMemoryNodeReg (
  IN  FDT_CLIENT_PROTOCOL     *This,
  IN  INT32                   PrevNode,
  OUT INT32                   *Node,
  OUT CONST VOID              **Reg,
  OUT UINTN                   *AddressCells,
  OUT UINTN                   *SizeCells,
  OUT UINT32                  *RegSize
  )
{
  INT32          Prev, Next;
  CONST CHAR8    *DeviceType;
  INT32          Len;
  EFI_STATUS     Status;

  ASSERT (mDeviceTreeBase != NULL);
  ASSERT (Node != NULL);

  for (Prev = PrevNode;; Prev = Next) {
    Next = fdt_next_node (mDeviceTreeBase, Prev, NULL);
    if (Next < 0) {
      break;
    }

    if (!IsNodeEnabled (Next)) {
      DEBUG ((DEBUG_WARN, "%a: ignoring disabled memory node\n", __FUNCTION__));
      continue;
    }

    DeviceType = fdt_getprop (mDeviceTreeBase, Next, "device_type", &Len);
    if (DeviceType != NULL && AsciiStrCmp (DeviceType, "memory") == 0) {
      //
      // Get the 'reg' property of this memory node. For now, we will assume
      // 8 byte quantities for base and size, respectively.
      // TODO use #cells root properties instead
      //
      Status = GetNodeProperty (This, Next, "reg", Reg, RegSize);
      if (EFI_ERROR (Status)) {
        DEBUG ((EFI_D_WARN,
          "%a: ignoring memory node with no 'reg' property\n",
          __FUNCTION__));
        continue;
      }
      if ((*RegSize % 16) != 0) {
        DEBUG ((EFI_D_WARN,
          "%a: ignoring memory node with invalid 'reg' property (size == 0x%x)\n",
          __FUNCTION__, *RegSize));
        continue;
      }

      *Node = Next;
      *AddressCells = 2;
      *SizeCells = 2;
      return EFI_SUCCESS;
    }
  }
  return EFI_NOT_FOUND;
}

STATIC
EFI_STATUS
EFIAPI
FindMemoryNodeReg (
  IN  FDT_CLIENT_PROTOCOL     *This,
  OUT INT32                   *Node,
  OUT CONST VOID              **Reg,
  OUT UINTN                   *AddressCells,
  OUT UINTN                   *SizeCells,
  OUT UINT32                  *RegSize
  )
{
  return FindNextMemoryNodeReg (This, 0, Node, Reg, AddressCells, SizeCells,
           RegSize);
}

STATIC
EFI_STATUS
EFIAPI
GetOrInsertChosenNode (
  IN  FDT_CLIENT_PROTOCOL     *This,
  OUT INT32                   *Node
  )
{
  INT32 NewNode;

  ASSERT (mDeviceTreeBase != NULL);
  ASSERT (Node != NULL);

  NewNode = fdt_path_offset (mDeviceTreeBase, "/chosen");
  if (NewNode < 0) {
    NewNode = fdt_add_subnode (mDeviceTreeBase, 0, "/chosen");
  }

  if (NewNode < 0) {
    return EFI_OUT_OF_RESOURCES;
  }

  *Node = NewNode;

  return EFI_SUCCESS;
}

STATIC FDT_CLIENT_PROTOCOL mFdtClientProtocol = {
  GetNodeProperty,
  SetNodeProperty,
  FindCompatibleNode,
  FindNextCompatibleNode,
  FindCompatibleNodeProperty,
  FindCompatibleNodeReg,
  FindMemoryNodeReg,
  FindNextMemoryNodeReg,
  GetOrInsertChosenNode,
};

STATIC
VOID
EFIAPI
OnPlatformHasDeviceTree (
  IN EFI_EVENT Event,
  IN VOID      *Context
  )
{
  EFI_STATUS Status;
  VOID       *Interface;
  VOID       *DeviceTreeBase;

  Status = gBS->LocateProtocol (
                  &gEdkiiPlatformHasDeviceTreeGuid,
                  NULL,                             // Registration
                  &Interface
                  );
  if (EFI_ERROR (Status)) {
    return;
  }

  DeviceTreeBase = Context;
  DEBUG ((
    DEBUG_INFO,
    "%a: exposing DTB @ 0x%p to OS\n",
    __FUNCTION__,
    DeviceTreeBase
    ));
  Status = gBS->InstallConfigurationTable (&gFdtTableGuid, DeviceTreeBase);
  ASSERT_EFI_ERROR (Status);

  gBS->CloseEvent (Event);
}

EFI_STATUS
EFIAPI
InitializeFdtClientDxe (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  VOID              *Hob;
  VOID              *DeviceTreeBase;
  EFI_STATUS        Status;
  EFI_EVENT         PlatformHasDeviceTreeEvent;
  VOID              *Registration;

  Hob = GetFirstGuidHob (&gFdtHobGuid);
  if (Hob == NULL || GET_GUID_HOB_DATA_SIZE (Hob) != sizeof (UINT64)) {
    return EFI_NOT_FOUND;
  }
  DeviceTreeBase = (VOID *)(UINTN)*(UINT64 *)GET_GUID_HOB_DATA (Hob);

  if (fdt_check_header (DeviceTreeBase) != 0) {
    DEBUG ((EFI_D_ERROR, "%a: No DTB found @ 0x%p\n", __FUNCTION__,
      DeviceTreeBase));
    return EFI_NOT_FOUND;
  }

  mDeviceTreeBase = DeviceTreeBase;

  DEBUG ((EFI_D_INFO, "%a: DTB @ 0x%p\n", __FUNCTION__, mDeviceTreeBase));

  //
  // Register a protocol notify for the EDKII Platform Has Device Tree
  // Protocol.
  //
  Status = gBS->CreateEvent (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  OnPlatformHasDeviceTree,
                  DeviceTreeBase,             // Context
                  &PlatformHasDeviceTreeEvent
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: CreateEvent(): %r\n", __FUNCTION__, Status));
    return Status;
  }

  Status = gBS->RegisterProtocolNotify (
                  &gEdkiiPlatformHasDeviceTreeGuid,
                  PlatformHasDeviceTreeEvent,
                  &Registration
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: RegisterProtocolNotify(): %r\n",
      __FUNCTION__,
      Status
      ));
    goto CloseEvent;
  }

  //
  // Kick the event; the protocol could be available already.
  //
  Status = gBS->SignalEvent (PlatformHasDeviceTreeEvent);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: SignalEvent(): %r\n", __FUNCTION__, Status));
    goto CloseEvent;
  }

  Status = gBS->InstallProtocolInterface (
                  &ImageHandle,
                  &gFdtClientProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &mFdtClientProtocol
                  );
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: InstallProtocolInterface(): %r\n",
      __FUNCTION__,
      Status
      ));
    goto CloseEvent;
  }

  return Status;

CloseEvent:
  gBS->CloseEvent (PlatformHasDeviceTreeEvent);
  return Status;
}
