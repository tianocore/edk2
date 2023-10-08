/** @file
  Determine the base addresses of serial ports from the Device Tree.

  Copyright (C) Red Hat
  Copyright (c) 2011 - 2023, Arm Ltd. All rights reserved.<BR>
  Copyright (c) 2008 - 2010, Apple Inc. All rights reserved.<BR>
  Copyright (c) 2014 - 2020, Linaro Ltd. All rights reserved.<BR>
  Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/BaseLib.h>
#include <Library/FdtSerialPortAddressLib.h>
#include <libfdt.h>

/**
  Read the "reg" property of Node in DeviceTree as a UINT64 base address.

  @param[in] DeviceTree    The flat device tree (FDT) to scan.

  @param[in] Node          The node to read the "reg" property of.

  @param[out] BaseAddress  On success, the base address read out of Node's "reg"
                           property. On error, not modified.

  @retval RETURN_DEVICE_ERROR     Node has a "status" property with value
                                  different from "okay".

  @retval RETURN_NOT_FOUND        Node does not have a "reg" property.

  @retval RETURN_BAD_BUFFER_SIZE  The size of Node's "reg" property is not 16
                                  bytes.

  @retval RETURN_SUCCESS          BaseAddress has been populated.
**/
STATIC
RETURN_STATUS
GetBaseAddress (
  IN  CONST VOID  *DeviceTree,
  IN  INT32       Node,
  OUT UINT64      *BaseAddress
  )
{
  CONST CHAR8  *NodeStatus;
  CONST VOID   *RegProp;
  INT32        PropSize;

  NodeStatus = fdt_getprop (DeviceTree, Node, "status", NULL);
  if ((NodeStatus != NULL) && (AsciiStrCmp (NodeStatus, "okay") != 0)) {
    return RETURN_DEVICE_ERROR;
  }

  RegProp = fdt_getprop (DeviceTree, Node, "reg", &PropSize);
  if (RegProp == NULL) {
    return RETURN_NOT_FOUND;
  }

  if (PropSize != 16) {
    return RETURN_BAD_BUFFER_SIZE;
  }

  *BaseAddress = fdt64_to_cpu (ReadUnaligned64 (RegProp));
  return RETURN_SUCCESS;
}

/**
  Collect the first ARRAY_SIZE (Ports->BaseAddress) serial ports into Ports from
  DeviceTree.

  @param[in] DeviceTree  The flat device tree (FDT) to scan.

  @param[in] Compatible  Look for Compatible in the "compatible" property of the
                         scanned nodes.

  @param[out] Ports      On successful return, Ports->NumberOfPorts contains the
                         number of serial ports found; it is (a) positive and
                         (b) at most ARRAY_SIZE (Ports->BaseAddress). If the FDT
                         had more serial ports, those are not reported. On
                         error, the contents of Ports are indeterminate.

  @retval RETURN_INVALID_PARAMETER  DeviceTree does not point to a valid FDT
                                    header.

  @retval RETURN_NOT_FOUND          No compatible and enabled serial port has
                                    been found.

  @retval RETURN_SUCCESS            At least one compatible and enabled serial
                                    port has been found; Ports has been filled
                                    in.
**/
RETURN_STATUS
EFIAPI
FdtSerialGetPorts (
  IN  CONST VOID        *DeviceTree,
  IN  CONST CHAR8       *Compatible,
  OUT FDT_SERIAL_PORTS  *Ports
  )
{
  INT32  Node;

  if (fdt_check_header (DeviceTree) != 0) {
    return RETURN_INVALID_PARAMETER;
  }

  Ports->NumberOfPorts = 0;
  Node                 = fdt_next_node (DeviceTree, 0, NULL);
  while ((Node > 0) &&
         (Ports->NumberOfPorts < ARRAY_SIZE (Ports->BaseAddress)))
  {
    CONST CHAR8  *CompatProp;
    INT32        PropSize;

    CompatProp = fdt_getprop (DeviceTree, Node, "compatible", &PropSize);
    if (CompatProp != NULL) {
      CONST CHAR8  *CompatItem;

      CompatItem = CompatProp;
      while ((CompatItem < CompatProp + PropSize) &&
             (AsciiStrCmp (CompatItem, Compatible) != 0))
      {
        CompatItem += AsciiStrLen (CompatItem) + 1;
      }

      if (CompatItem < CompatProp + PropSize) {
        RETURN_STATUS  Status;
        UINT64         BaseAddress;

        Status = GetBaseAddress (DeviceTree, Node, &BaseAddress);
        if (!RETURN_ERROR (Status)) {
          Ports->BaseAddress[Ports->NumberOfPorts++] = BaseAddress;
        }
      }
    }

    Node = fdt_next_node (DeviceTree, Node, NULL);
  }

  return Ports->NumberOfPorts > 0 ? RETURN_SUCCESS : RETURN_NOT_FOUND;
}

/**
  Fetch the base address of the serial port identified in the "stdout-path"
  property of the "/chosen" node in DeviceTree.

  @param[in] DeviceTree    The flat device tree (FDT) to scan.

  @param[out] BaseAddress  On success, the base address of the preferred serial
                           port (to be used as console). On error, BaseAddress
                           is not modified.

  @retval RETURN_INVALID_PARAMETER  DeviceTree does not point to a valid FDT
                                    header.

  @retval RETURN_NOT_FOUND          No enabled console port has been found.

  @retval RETURN_PROTOCOL_ERROR     The first (or only) node path in the
                                    "stdout-path" property is an empty string.

  @retval RETURN_PROTOCOL_ERROR     The console port has been found in the FDT,
                                    but its base address is not correctly
                                    represented.

  @retval RETURN_SUCCESS            BaseAddress has been populated.
**/
RETURN_STATUS
EFIAPI
FdtSerialGetConsolePort (
  IN  CONST VOID  *DeviceTree,
  OUT UINT64      *BaseAddress
  )
{
  INT32          ChosenNode;
  CONST CHAR8    *StdoutPathProp;
  INT32          PropSize;
  CONST CHAR8    *StdoutPathEnd;
  UINTN          StdoutPathLength;
  INT32          ConsoleNode;
  RETURN_STATUS  Status;

  if (fdt_check_header (DeviceTree) != 0) {
    return RETURN_INVALID_PARAMETER;
  }

  ChosenNode = fdt_path_offset (DeviceTree, "/chosen");
  if (ChosenNode < 0) {
    return RETURN_NOT_FOUND;
  }

  StdoutPathProp = fdt_getprop (
                     DeviceTree,
                     ChosenNode,
                     "stdout-path",
                     &PropSize
                     );
  if (StdoutPathProp == NULL) {
    return RETURN_NOT_FOUND;
  }

  //
  // If StdoutPathProp contains a colon (":"), then the colon terminates the
  // path we're interested in.
  //
  StdoutPathEnd = AsciiStrStr (StdoutPathProp, ":");
  if (StdoutPathEnd == NULL) {
    StdoutPathLength = PropSize - 1;
  } else {
    StdoutPathLength = StdoutPathEnd - StdoutPathProp;
  }

  if (StdoutPathLength == 0) {
    return RETURN_PROTOCOL_ERROR;
  }

  if (StdoutPathProp[0] == '/') {
    //
    // StdoutPathProp starts with an absolute node path.
    //
    ConsoleNode = fdt_path_offset_namelen (
                    DeviceTree,
                    StdoutPathProp,
                    (INT32)StdoutPathLength
                    );
  } else {
    //
    // StdoutPathProp starts with an alias.
    //
    CONST CHAR8  *ResolvedStdoutPath;

    ResolvedStdoutPath = fdt_get_alias_namelen (
                           DeviceTree,
                           StdoutPathProp,
                           (INT32)StdoutPathLength
                           );
    if (ResolvedStdoutPath == NULL) {
      return RETURN_NOT_FOUND;
    }

    ConsoleNode = fdt_path_offset (DeviceTree, ResolvedStdoutPath);
  }

  if (ConsoleNode < 0) {
    return RETURN_NOT_FOUND;
  }

  Status = GetBaseAddress (DeviceTree, ConsoleNode, BaseAddress);
  switch (Status) {
    case RETURN_NOT_FOUND:
    case RETURN_BAD_BUFFER_SIZE:
      return RETURN_PROTOCOL_ERROR;
    case RETURN_SUCCESS:
      return RETURN_SUCCESS;
    default:
      return RETURN_NOT_FOUND;
  }
}
