/** @file
  Serial Port Parser.

  Copyright (c) 2021 - 2026, Arm Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - linux/Documentation/devicetree/bindings/serial/serial.yaml
  - linux/Documentation/devicetree/bindings/serial/8250.txt
  - linux/Documentation/devicetree/bindings/serial/arm,sbsa-uart.yaml
  - linux/Documentation/devicetree/bindings/serial/pl011.yaml
**/

#include <IndustryStandard/DebugPort2Table.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/FdtLib.h>

#include "CmObjectDescUtility.h"
#include "FdtHwInfoParser.h"
#include "Serial/SerialPortParser.h"

/** Default baudrate.
    Some DT bindings don't permit specifying the baudrate.
*/
#define DEFAULT_UART_BAUD_RATE  115200

/** List of "compatible" property values for serial port nodes.

  Any other "compatible" value is not supported by this module.
*/
STATIC CONST COMPATIBILITY_STR  SerialCompatibleStr[] = {
  { "ns16550a"      },
  { "arm,sbsa-uart" },
  { "arm,pl011"     }
};

/** COMPATIBILITY_INFO structure for the SerialCompatible.
*/
CONST COMPATIBILITY_INFO  SerialCompatibleInfo = {
  ARRAY_SIZE (SerialCompatibleStr),
  SerialCompatibleStr
};

/** 16550 UART compatible strings.

  Any string of this list must be part of SerialCompatible.
*/
STATIC CONST COMPATIBILITY_STR  Serial16550CompatibleStr[] = {
  { "ns16550a" }
};

/** COMPATIBILITY_INFO structure for the Serial16550Compatible.
*/
CONST COMPATIBILITY_INFO  Serial16550CompatibleInfo = {
  ARRAY_SIZE (Serial16550CompatibleStr),
  Serial16550CompatibleStr
};

/** SBSA UART compatible strings.

  Any string of this list must be part of SerialCompatible.
*/
STATIC CONST COMPATIBILITY_STR  SerialSbsaCompatibleStr[] = {
  { "arm,sbsa-uart" }
};

/** COMPATIBILITY_INFO structure for the SerialSbsaCompatible.
*/
CONST COMPATIBILITY_INFO  SerialSbsaCompatibleInfo = {
  ARRAY_SIZE (SerialSbsaCompatibleStr),
  SerialSbsaCompatibleStr
};

/** PL011 UART compatible strings.

  Any string of this list must be part of SerialCompatible.
*/
STATIC CONST COMPATIBILITY_STR  SerialPl011CompatibleStr[] = {
  { "arm,pl011" }
};

/** COMPATIBILITY_INFO structure for the SerialPl011Compatible.
*/
CONST COMPATIBILITY_INFO  SerialPl011CompatibleInfo = {
  ARRAY_SIZE (SerialPl011CompatibleStr),
  SerialPl011CompatibleStr
};

/** Read the "clock-frequency" property from a node.

  @param [in]  Fdt             Pointer to a Flattened Device Tree (Fdt).
  @param [in]  Node            Offset of the node to inspect.
  @param [out] ClockFrequency  Clock frequency retrieved from the node.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_NOT_FOUND           The node does not expose "clock-frequency".
**/
STATIC
EFI_STATUS
EFIAPI
GetNodeClockFrequency (
  IN  CONST VOID  *Fdt,
  IN  INT32       Node,
  OUT UINT32      *ClockFrequency
  )
{
  CONST UINT32  *Data;
  INT32         DataSize;

  if ((Fdt == NULL) || (ClockFrequency == NULL)) {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  Data = FdtGetProp (Fdt, Node, "clock-frequency", &DataSize);
  if (Data == NULL) {
    return EFI_NOT_FOUND;
  }

  if (DataSize < sizeof (*Data)) {
    ASSERT (FALSE);
    return EFI_ABORTED;
  }

  *ClockFrequency = Fdt32ToCpu (*Data);
  return EFI_SUCCESS;
}

/** Resolve the first clock node referenced by a node's "clocks" property.

  @param [in]  Fdt        Pointer to a Flattened Device Tree (Fdt).
  @param [in]  Node       Offset of the node to inspect.
  @param [out] ClockNode  Clock node referenced by the first phandle.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_NOT_FOUND           The node does not expose "clocks".
**/
STATIC
EFI_STATUS
EFIAPI
GetNodeClockNode (
  IN  CONST VOID  *Fdt,
  IN  INT32       Node,
  OUT INT32       *ClockNode
  )
{
  CONST UINT32  *Data;
  INT32         DataSize;
  UINT32        ClockPHandle;

  if ((Fdt == NULL) || (ClockNode == NULL)) {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  Data = FdtGetProp (Fdt, Node, "clocks", &DataSize);
  if (Data == NULL) {
    return EFI_NOT_FOUND;
  }

  if (DataSize < sizeof (*Data)) {
    ASSERT (FALSE);
    return EFI_ABORTED;
  }

  ClockPHandle = Fdt32ToCpu (*Data);
  *ClockNode   = FdtNodeOffsetByPhandle (Fdt, ClockPHandle);
  if (*ClockNode < 0) {
    ASSERT (FALSE);
    return EFI_ABORTED;
  }

  return EFI_SUCCESS;
}

/** Get the 16550 UART input clock frequency.

  One of "clock-frequency", "clocks" properties is required.

  @param [in]  Fdt             Pointer to a Flattened Device Tree (Fdt).
  @param [in]  SerialPortNode  Offset of the serial-port node.
  @param [out] ClockFrequency  Clock frequency retrieved from DT.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_NOT_FOUND           No clock frequency was found.
  @retval EFI_UNSUPPORTED         Unsupported DT clock encoding.
**/
STATIC
EFI_STATUS
EFIAPI
Get16550UartClockFrequency (
  IN  CONST VOID  *Fdt,
  IN  INT32       SerialPortNode,
  OUT UINT32      *ClockFrequency
  )
{
  INT32       ClockNode;
  EFI_STATUS  Status;

  if ((Fdt == NULL) || (ClockFrequency == NULL)) {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  Status = GetNodeClockFrequency (Fdt, SerialPortNode, ClockFrequency);
  if (Status == EFI_SUCCESS) {
    return EFI_SUCCESS;
  } else if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  Status = GetNodeClockNode (Fdt, SerialPortNode, &ClockNode);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return GetNodeClockFrequency (Fdt, ClockNode, ClockFrequency);
}

/** Get the PL011 UART input clock frequency.

  The "clocks" property is optional.

  @param [in]  Fdt             Pointer to a Flattened Device Tree (Fdt).
  @param [in]  SerialPortNode  Offset of the serial-port node.
  @param [out] ClockFrequency  Clock frequency retrieved from DT.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_NOT_FOUND           No clock frequency was found.
  @retval EFI_UNSUPPORTED         Unsupported DT clock encoding.
**/
STATIC
EFI_STATUS
EFIAPI
GetPl011UartClockFrequency (
  IN  CONST VOID  *Fdt,
  IN  INT32       SerialPortNode,
  OUT UINT32      *ClockFrequency
  )
{
  EFI_STATUS  Status;
  INT32       ClockNode;

  if ((Fdt == NULL) || (ClockFrequency == NULL)) {
    ASSERT (FALSE);
    return EFI_INVALID_PARAMETER;
  }

  *ClockFrequency = 0;

  Status = GetNodeClockNode (Fdt, SerialPortNode, &ClockNode);
  if (!EFI_ERROR (Status)) {
    Status = GetNodeClockFrequency (Fdt, ClockNode, ClockFrequency);
  }

  // Advertising the clock frequency is optional.
  if (Status == EFI_NOT_FOUND) {
    Status = EFI_SUCCESS;
  }

  ASSERT_EFI_ERROR (Status);
  return Status;
}

/** Parse a serial port node.

  @param [in]  Fdt               Pointer to a Flattened Device Tree (Fdt).
  @param [in]  SerialPortNode    Offset of a serial-port node.
  @param [in]  SerialPortInfo    The CM_ARCH_COMMON_SERIAL_PORT_INFO to populate.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_UNSUPPORTED         Unsupported.
**/
STATIC
EFI_STATUS
EFIAPI
SerialPortNodeParser (
  IN  CONST VOID                       *Fdt,
  IN  INT32                            SerialPortNode,
  IN  CM_ARCH_COMMON_SERIAL_PORT_INFO  *SerialPortInfo
  )
{
  EFI_STATUS    Status;
  INT32         IntcNode;
  CONST UINT32  *DecodedInterruptData;
  INT32         DecodedInterruptCells;
  UINT32        ClockFrequency;

  CONST UINT8  *Data;
  INT32        DataSize;
  UINT8        AccessSize;

  if ((Fdt == NULL) ||
      (SerialPortInfo == NULL))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  Status = FdtGetTranslatedReg (
             Fdt,
             SerialPortNode,
             0,
             &SerialPortInfo->BaseAddress,
             &SerialPortInfo->BaseAddressLength
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // Get the associated interrupt-controller.
  Status = FdtGetIntControllerNode (Fdt, SerialPortNode, &IntcNode);
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    if (Status == EFI_NOT_FOUND) {
      // Should have found the node.
      Status = EFI_ABORTED;
    }

    return Status;
  }

  Status = FdtResolveInterrupt (
             Fdt,
             SerialPortNode,
             0,
             &DecodedInterruptData,
             &DecodedInterruptCells
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  SerialPortInfo->Interrupt = FdtGetInterruptId (DecodedInterruptData, DecodedInterruptCells);

  /*
   * In RISC-V, GSI space can be divided among multiple APLIC/PLICs.
   * So, convert the interrupt number in DT to appropriate GSI number
   * using the parent interrupt controller information.
   */
 #if defined (MDE_CPU_RISCV64)
  SerialPortInfo->Interrupt = FdtConvertToGsi (IntcNode, SerialPortInfo->Interrupt);
 #endif

  SerialPortInfo->BaudRate = DEFAULT_UART_BAUD_RATE;

  if (FdtNodeIsCompatible (Fdt, SerialPortNode, &Serial16550CompatibleInfo)) {
    Status = Get16550UartClockFrequency (Fdt, SerialPortNode, &ClockFrequency);
    if (EFI_ERROR (Status)) {
      ASSERT_EFI_ERROR (Status);
      return Status;
    }

    SerialPortInfo->Clock       = ClockFrequency;
    SerialPortInfo->PortSubtype =
      EFI_ACPI_DBG2_PORT_SUBTYPE_SERIAL_16550_WITH_GAS;

    Data = FdtGetProp (Fdt, SerialPortNode, "current-speed", &DataSize);
    if (Data != NULL) {
      if (DataSize != sizeof (UINT32)) {
        ASSERT (0);
        return EFI_ABORTED;
      }

      SerialPortInfo->BaudRate = Fdt32ToCpu (*(CONST UINT32 *)Data);
    }

    /* reg-io-width:
         description: |
         The size (in bytes) of the IO accesses that should be performed on the
         device. There are some systems that require 32-bit accesses to the
         UART.
    */
    Data = FdtGetProp (Fdt, SerialPortNode, "reg-io-width", &DataSize);
    if (Data != NULL) {
      if (DataSize < sizeof (UINT32)) {
        // If error or not enough space.
        ASSERT (0);
        return EFI_ABORTED;
      }

      AccessSize = Fdt32ToCpu (*(UINT32 *)Data);
      // Convert the DT byte width to the ACPI GAS access-size encoding.
      switch (AccessSize) {
        case 1:
          SerialPortInfo->AccessSize = EFI_ACPI_6_3_BYTE;
          break;
        case 2:
          SerialPortInfo->AccessSize = EFI_ACPI_6_3_WORD;
          break;
        case 4:
          SerialPortInfo->AccessSize = EFI_ACPI_6_3_DWORD;
          break;
        case 8:
          SerialPortInfo->AccessSize = EFI_ACPI_6_3_QWORD;
          break;
        default:
          ASSERT (0);
          return EFI_INVALID_PARAMETER;
      }
    } else {
      // 8250/16550 defaults to byte access.
      SerialPortInfo->AccessSize = EFI_ACPI_6_3_BYTE;
    }
  } else if (FdtNodeIsCompatible (
               Fdt,
               SerialPortNode,
               &SerialSbsaCompatibleInfo
               ))
  {
    //
    // SBSA-UART doesn't have a clock specifier.
    //
    SerialPortInfo->Clock       = 0;
    SerialPortInfo->PortSubtype =
      EFI_ACPI_DBG2_PORT_SUBTYPE_SERIAL_ARM_SBSA_GENERIC_UART;

    Data = FdtGetProp (Fdt, SerialPortNode, "current-speed", &DataSize);
    if (Data != NULL) {
      if (DataSize != sizeof (UINT32)) {
        ASSERT (0);
        return EFI_ABORTED;
      }

      SerialPortInfo->BaudRate = Fdt32ToCpu (*(CONST UINT32 *)Data);
    }
  } else if (FdtNodeIsCompatible (
               Fdt,
               SerialPortNode,
               &SerialPl011CompatibleInfo
               ))
  {
    //
    // Clock frequency is not mandatory for pl011 nodes.
    //
    ClockFrequency = 0;

    Status = GetPl011UartClockFrequency (Fdt, SerialPortNode, &ClockFrequency);
    if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
      ASSERT_EFI_ERROR (Status);
      return Status;
    }

    SerialPortInfo->Clock       = ClockFrequency;
    SerialPortInfo->PortSubtype =
      EFI_ACPI_DBG2_PORT_SUBTYPE_SERIAL_ARM_PL011_UART;
  } else {
    ASSERT (0);
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

/** Find the console serial-port node in the DT.

  This function fetches the node referenced in the "stdout-path"
  property of the "chosen" node.

  @param [in]  Fdt                  Pointer to a Flattened Device Tree (Fdt).
  @param [out] SerialConsoleNode    If success, contains the node offset
                                    of the console serial-port node.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_NOT_FOUND           Not found.
**/
STATIC
EFI_STATUS
EFIAPI
GetSerialConsoleNode (
  IN  CONST VOID   *Fdt,
  OUT       INT32  *SerialConsoleNode
  )
{
  CONST CHAR8  *Prop;
  INT32        PropSize;
  CONST CHAR8  *Path;
  INT32        PathLen;
  INT32        ChosenNode;

  if ((Fdt == NULL) ||
      (SerialConsoleNode == NULL))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // The "chosen" node resides at the root of the DT. Fetch it.
  ChosenNode = FdtPathOffset (Fdt, "/chosen");
  if (ChosenNode < 0) {
    return EFI_NOT_FOUND;
  }

  Prop = FdtGetProp (Fdt, ChosenNode, "stdout-path", &PropSize);
  if ((Prop == NULL) || (PropSize < 0)) {
    return EFI_NOT_FOUND;
  }

  // Determine the actual path length, as a colon terminates the path.
  Path = ScanMem8 (Prop, PropSize, ':');
  if (Path == NULL) {
    PathLen = (UINT32)AsciiStrLen (Prop);
  } else {
    PathLen = (INT32)(Path - Prop);
  }

  // Aliases cannot start with a '/', so it must be the actual path.
  if (Prop[0] == '/') {
    *SerialConsoleNode = FdtPathOffsetNameLen (Fdt, Prop, PathLen);
    return EFI_SUCCESS;
  }

  // Lookup the alias, as this contains the actual path.
  Path = FdtGetAliasNameLen (Fdt, Prop, PathLen);
  if (Path == NULL) {
    return EFI_NOT_FOUND;
  }

  *SerialConsoleNode = FdtPathOffset (Fdt, Path);
  return EFI_SUCCESS;
}

/** CM_ARCH_COMMON_SERIAL_PORT_INFO dispatcher function (for a generic serial-port).

  @param [in]  FdtParserHandle A handle to the parser instance.
  @param [in]  GenericSerialInfo  Pointer to a serial port info list.
  @param [in]  NodeCount          Count of serial ports to dispatch.
  @param [in]  SerialObjectId     Serial port object ID.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_NOT_FOUND           Not found.
  @retval EFI_UNSUPPORTED         Unsupported.
**/
STATIC
EFI_STATUS
EFIAPI
SerialPortInfoDispatch (
  IN  CONST FDT_HW_INFO_PARSER_HANDLE  FdtParserHandle,
  IN  CM_ARCH_COMMON_SERIAL_PORT_INFO  *GenericSerialInfo,
  IN  INT32                            NodeCount,
  IN  EARCH_COMMON_OBJECT_ID           SerialObjectId
  )
{
  EFI_STATUS         Status;
  CM_OBJ_DESCRIPTOR  *NewCmObjDesc;

  if ((GenericSerialInfo == NULL) || (NodeCount == 0)) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  if ((SerialObjectId != EArchCommonObjSerialPortInfo) &&
      (SerialObjectId != EArchCommonObjSerialDebugPortInfo) &&
      (SerialObjectId != EArchCommonObjConsolePortInfo))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  // Dispatch the Generic Serial ports
  Status = CreateCmObjDesc (
             CREATE_CM_ARCH_COMMON_OBJECT_ID (SerialObjectId),
             NodeCount,
             GenericSerialInfo,
             sizeof (CM_ARCH_COMMON_SERIAL_PORT_INFO) * NodeCount,
             &NewCmObjDesc
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  // Add all the CmObjs to the Configuration Manager.
  Status = AddMultipleCmObj (FdtParserHandle, NewCmObjDesc, 0, NULL);
  ASSERT_EFI_ERROR (Status);
  FreeCmObjDesc (NewCmObjDesc);
  return Status;
}

/** CM_ARCH_COMMON_SERIAL_PORT_INFO parser function (for debug/console serial-port).

  This parser expects FdtBranch to be the debug serial-port node.
  At most one CmObj is created.
  The following structure is populated:
  typedef struct EArchCommonSerialPortInfo {
    UINT64  BaseAddress;                      // {Populated}
    UINT32  Interrupt;                        // {Populated}
    UINT64  BaudRate;                         // {Populated, default to 115200}
    UINT32  Clock;                            // {Populated}
    UINT16  PortSubtype;                      // {Populated}
    UINT64  BaseAddressLength                 // {Populated}
  } CM_ARCH_COMMON_SERIAL_PORT_INFO;

  A parser parses a Device Tree to populate a specific CmObj type. None,
  one or many CmObj can be created by the parser.
  The created CmObj are then handed to the parser's caller through the
  HW_INFO_ADD_OBJECT interface.
  This can also be a dispatcher. I.e. a function that not parsing a
  Device Tree but calling other parsers.

  @param [in]  FdtParserHandle A handle to the parser instance.
  @param [in]  FdtBranch       When searching for DT node name, restrict
                               the search to this Device Tree branch.
  @param [in]  SerialObjectId  ArchCommon Namespace Object ID for the serial
                               port.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_NOT_FOUND           Not found.
  @retval EFI_UNSUPPORTED         Unsupported.
**/
STATIC
EFI_STATUS
EFIAPI
SerialPortInfoParser (
  IN  CONST FDT_HW_INFO_PARSER_HANDLE  FdtParserHandle,
  IN        INT32                      FdtBranch,
  IN        EARCH_COMMON_OBJECT_ID     SerialObjectId
  )
{
  EFI_STATUS                       Status;
  CM_ARCH_COMMON_SERIAL_PORT_INFO  SerialInfo;

  if ((SerialObjectId != EArchCommonObjSerialDebugPortInfo) &&
      (SerialObjectId != EArchCommonObjConsolePortInfo))
  {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  ZeroMem (&SerialInfo, sizeof (SerialInfo));

  Status = SerialPortNodeParser (
             FdtParserHandle->Fdt,
             FdtBranch,
             &SerialInfo
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  Status = SerialPortInfoDispatch (
             FdtParserHandle,
             &SerialInfo,
             1,
             SerialObjectId
             );
  ASSERT_EFI_ERROR (Status);
  return Status;
}

/** SerialPort dispatcher.

  This disptacher populates the CM_ARCH_COMMON_SERIAL_PORT_INFO structure for
  the following CM_OBJ_ID:
   - EArchCommonObjConsolePortInfo
   - EArchCommonObjSerialDebugPortInfo
   - EArchCommonObjSerialPortInfo

  A parser parses a Device Tree to populate a specific CmObj type. None,
  one or many CmObj can be created by the parser.
  The created CmObj are then handed to the parser's caller through the
  HW_INFO_ADD_OBJECT interface.
  This can also be a dispatcher. I.e. a function that not parsing a
  Device Tree but calling other parsers.

  @param [in]  FdtParserHandle A handle to the parser instance.
  @param [in]  FdtBranch       When searching for DT node name, restrict
                               the search to this Device Tree branch.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_ABORTED             An error occurred.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
  @retval EFI_NOT_FOUND           Not found.
  @retval EFI_UNSUPPORTED         Unsupported.
**/
EFI_STATUS
EFIAPI
SerialPortDispatcher (
  IN  CONST FDT_HW_INFO_PARSER_HANDLE  FdtParserHandle,
  IN        INT32                      FdtBranch
  )
{
  EFI_STATUS                       Status;
  INT32                            SerialConsoleNode;
  INT32                            SerialDebugNode;
  INT32                            SerialNode;
  UINT32                           Index;
  UINT32                           SerialNodeCount;
  UINT32                           SerialNodesRemaining;
  CM_ARCH_COMMON_SERIAL_PORT_INFO  *GenericSerialInfo;
  UINT32                           GenericSerialIndex;
  VOID                             *Fdt;

  if (FdtParserHandle == NULL) {
    ASSERT (0);
    return EFI_INVALID_PARAMETER;
  }

  Fdt = FdtParserHandle->Fdt;

  // Count the number of serial-ports.
  Status = FdtCountCompatNodeInBranch (
             Fdt,
             FdtBranch,
             &SerialCompatibleInfo,
             &SerialNodeCount
             );
  if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  }

  if (SerialNodeCount == 0) {
    return EFI_NOT_FOUND;
  }

  // Track remaining nodes separately as SerialNodeCount
  // is used in for loop below and reducing SerialNodeCount
  // would result in the Generic Serial port nodes not
  // being found if the serial console port node is among
  // the first few serial nodes.
  SerialNodesRemaining = SerialNodeCount;

  // Identify the serial console port.
  Status = GetSerialConsoleNode (Fdt, &SerialConsoleNode);
  if (Status == EFI_NOT_FOUND) {
    // No serial console.
    SerialConsoleNode = -1;
  } else if (EFI_ERROR (Status)) {
    ASSERT (0);
    return Status;
  } else {
    // Parse the console serial-port.
    Status = SerialPortInfoParser (
               FdtParserHandle,
               SerialConsoleNode,
               EArchCommonObjConsolePortInfo
               );
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      return Status;
    }

    SerialNodesRemaining--;
  }

  GenericSerialInfo = NULL;
  if (SerialNodesRemaining > 1) {
    // We have more than one serial port remaining.
    // This means that the first serial port will
    // be reserved as a debug port, and the remaining
    // will be for general purpose use.
    SerialNodesRemaining--;
    GenericSerialInfo = AllocateZeroPool (
                          SerialNodesRemaining *
                          sizeof (CM_ARCH_COMMON_SERIAL_PORT_INFO)
                          );
    if (GenericSerialInfo == NULL) {
      ASSERT (0);
      return EFI_OUT_OF_RESOURCES;
    }
  }

  SerialNode         = FdtBranch;
  SerialDebugNode    = -1;
  GenericSerialIndex = 0;
  for (Index = 0; Index < SerialNodeCount; Index++) {
    // Search the next serial-port node in the branch.
    Status = FdtGetNextCompatNodeInBranch (
               Fdt,
               FdtBranch,
               &SerialCompatibleInfo,
               &SerialNode
               );
    if (EFI_ERROR (Status)) {
      ASSERT (0);
      if (Status == EFI_NOT_FOUND) {
        // Should have found the node.
        Status = EFI_ABORTED;
      }

      goto exit_handler;
    }

    // Ignore the serial console node.
    if (SerialNode == SerialConsoleNode) {
      continue;
    } else if (SerialDebugNode == -1) {
      // The first serial-port node, not being the console serial-port,
      // will be the debug serial-port.
      SerialDebugNode = SerialNode;
      Status          = SerialPortInfoParser (
                          FdtParserHandle,
                          SerialDebugNode,
                          EArchCommonObjSerialDebugPortInfo
                          );
      if (EFI_ERROR (Status)) {
        ASSERT (0);
        goto exit_handler;
      }
    } else {
      if (GenericSerialInfo == NULL) {
        // Should not be possible.
        ASSERT (0);
        Status = EFI_ABORTED;
        goto exit_handler;
      }

      Status = SerialPortNodeParser (
                 Fdt,
                 SerialNode,
                 &GenericSerialInfo[GenericSerialIndex++]
                 );
      if (EFI_ERROR (Status)) {
        ASSERT (0);
        goto exit_handler;
      }
    }
  } // for

  if (GenericSerialIndex > 0) {
    Status = SerialPortInfoDispatch (
               FdtParserHandle,
               GenericSerialInfo,
               GenericSerialIndex,
               EArchCommonObjSerialPortInfo
               );
  }

exit_handler:
  if (GenericSerialInfo != NULL) {
    FreePool (GenericSerialInfo);
  }

  return Status;
}
