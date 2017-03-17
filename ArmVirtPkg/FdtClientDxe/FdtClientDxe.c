/** @file
*  FDT client driver
*
*  Copyright (c) 2016, Linaro Ltd. All rights reserved.<BR>
*
*  This program and the accompanying materials are
*  licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
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

  if (!FeaturePcdGet (PcdPureAcpiBoot)) {
    //
    // Only install the FDT as a configuration table if we want to leave it up
    // to the OS to decide whether it prefers ACPI over DT.
    //
    Status = gBS->InstallConfigurationTable (&gFdtTableGuid, DeviceTreeBase);
    ASSERT_EFI_ERROR (Status);
  }

  return gBS->InstallProtocolInterface (&ImageHandle, &gFdtClientProtocolGuid,
                EFI_NATIVE_INTERFACE, &mFdtClientProtocol);
}
