/** @file
*
*  Copyright (c) 2011-2014, ARM Limited. All rights reserved.
*  Copyright (c) 2014-2020, Linaro Limited. All rights reserved.
*  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
*
**/

#include <PiPei.h>

#include <Library/ArmCcaInitPeiLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/FdtLib.h>
#include <Library/HobLib.h>
#include <Library/PcdLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/FdtSerialPortAddressLib.h>
#include <Library/QemuFwCfgSimpleParserLib.h>

#include <Guid/ArmMpCoreInfo.h>
#include <Guid/EarlyPL011BaseAddress.h>
#include <Guid/FdtHob.h>

//
// Duplicated in ArmVirtPkg/Library/DebugLibFdtPL011Uart/Flash.c —
// keep both copies in sync.
//

/**
  Parse a serial debug level string.

  Accepted values are "silent" (DEBUG_ERROR only), "verbose" (no
  override), or a hex bitmask (e.g. "0x80000040").

  @param[in]  String      NUL-terminated ASCII string to parse.
  @param[out] DebugLevel  On success, the parsed debug level bitmask.

  @retval TRUE   String was recognised; *DebugLevel is valid.
  @retval FALSE  String is NULL, "verbose", or unrecognised; no override.
**/
STATIC
BOOLEAN
ParseSerialDebugLevel (
  IN  CONST CHAR8  *String,
  OUT UINT32       *DebugLevel
  )
{
  UINT64  Value;
  CHAR8   *End;

  if ((String == NULL) || (DebugLevel == NULL)) {
    return FALSE;
  }

  if (AsciiStrCmp (String, "silent") == 0) {
    *DebugLevel = DEBUG_ERROR;
    return TRUE;
  }

  if (AsciiStrCmp (String, "verbose") == 0) {
    return FALSE;
  }

  if (!EFI_ERROR (AsciiStrHexToUint64S (String, &End, &Value)) &&
      (*End == '\0'))
  {
    *DebugLevel = (UINT32)Value;
    return TRUE;
  }

  return FALSE;
}

STATIC CONST EFI_PEI_PPI_DESCRIPTOR  mTpm2DiscoveredPpi = {
  EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST,
  &gOvmfTpmDiscoveredPpiGuid,
  NULL
};

STATIC CONST EFI_PEI_PPI_DESCRIPTOR  mTpm2InitializationDonePpi = {
  EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST,
  &gPeiTpmInitializationDonePpiGuid,
  NULL
};

/**
  Get the MPIDR from an FDT CPU node.

  @param[in]  Fdt          The device tree.
  @param[in]  AddressCells The number of address cells in the /cpus node.
  @param[in]  CpuNode      The CPU node offset.
  @param[out] Mpidr        The MPIDR from the CPU node's reg property.

  @retval EFI_SUCCESS           The node is an enabled CPU with a valid MPIDR.
  @retval EFI_NOT_FOUND         The node is not an enabled CPU node.
  @retval EFI_COMPROMISED_DATA  The CPU node has an invalid reg property.
**/
STATIC
EFI_STATUS
GetFdtCpuMpidr (
  IN  CONST VOID  *Fdt,
  IN  INT32       AddressCells,
  IN  INT32       CpuNode,
  OUT UINT64      *Mpidr
  )
{
  CONST CHAR8   *DeviceType;
  CONST CHAR8   *NodeStatus;
  CONST UINT32  *RegProp;
  INT32         PropSize;

  DeviceType = FdtGetProp (Fdt, CpuNode, "device_type", &PropSize);
  if ((DeviceType == NULL) || (AsciiStrCmp (DeviceType, "cpu") != 0)) {
    return EFI_NOT_FOUND;
  }

  NodeStatus = FdtGetProp (Fdt, CpuNode, "status", &PropSize);
  if ((NodeStatus != NULL) && (AsciiStrCmp (NodeStatus, "disabled") == 0)) {
    return EFI_NOT_FOUND;
  }

  RegProp = FdtGetProp (Fdt, CpuNode, "reg", &PropSize);
  if ((RegProp == NULL) || (PropSize != AddressCells * sizeof (UINT32))) {
    return EFI_COMPROMISED_DATA;
  }

  if (AddressCells == 1) {
    *Mpidr = Fdt32ToCpu (ReadUnaligned32 (RegProp));
  } else {
    *Mpidr = Fdt64ToCpu (ReadUnaligned64 (RegProp));
  }

  return EFI_SUCCESS;
}

/**
  Build the MP core information HOB from the CPU nodes in the device tree.

  @param[in] Fdt  The device tree.

  @retval EFI_SUCCESS            The MP core info HOB was built.
  @retval EFI_DEVICE_ERROR       The FDT is invalid.
  @retval EFI_NOT_FOUND          No enabled CPU nodes were found.
  @retval EFI_COMPROMISED_DATA   A CPU node contained malformed data.
  @retval EFI_UNSUPPORTED        The FDT uses an unsupported address-cell size.
  @retval EFI_OUT_OF_RESOURCES   The HOB could not be allocated.
**/
STATIC
EFI_STATUS
BuildFdtMpCoreInfoHob (
  IN CONST VOID  *Fdt
  )
{
  EFI_STATUS     Status;
  INT32          CpusNode;
  INT32          CpuNode;
  INT32          AddressCells;
  UINTN          CpuCount;
  UINTN          CpuIndex;
  UINT64         Mpidr;
  ARM_CORE_INFO  *CoreInfo;

  if ((Fdt == NULL) || (FdtCheckHeader (Fdt) != 0)) {
    return EFI_DEVICE_ERROR;
  }

  CpusNode = FdtPathOffset (Fdt, "/cpus");
  if (CpusNode < 0) {
    return EFI_NOT_FOUND;
  }

  AddressCells = FdtAddressCells (Fdt, CpusNode);
  if ((AddressCells != 1) && (AddressCells != 2)) {
    return EFI_UNSUPPORTED;
  }

  // Count the enabled CPU nodes
  CpuCount = 0;
  FdtForEachSubnode (CpuNode, Fdt, CpusNode) {
    Status = GetFdtCpuMpidr (Fdt, AddressCells, CpuNode, &Mpidr);
    if (Status == EFI_NOT_FOUND) {
      continue;
    }

    if (EFI_ERROR (Status)) {
      return Status;
    }

    CpuCount++;
  }

  if (CpuCount == 0) {
    return EFI_NOT_FOUND;
  }

  CoreInfo = BuildGuidHob (&gArmMpCoreInfoGuid, sizeof (*CoreInfo) * CpuCount);
  if (CoreInfo == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  // Preserve the device-tree order when filling the HOB.
  ZeroMem (CoreInfo, sizeof (*CoreInfo) * CpuCount);

  CpuIndex = 0;
  FdtForEachSubnode (CpuNode, Fdt, CpusNode) {
    Status = GetFdtCpuMpidr (Fdt, AddressCells, CpuNode, &Mpidr);
    if (Status == EFI_NOT_FOUND) {
      continue;
    }

    if (EFI_ERROR (Status)) {
      return Status;
    }

    CoreInfo[CpuIndex].Mpidr               = Mpidr;
    CoreInfo[CpuIndex].MailboxSetAddress   = (EFI_PHYSICAL_ADDRESS)0;
    CoreInfo[CpuIndex].MailboxGetAddress   = (EFI_PHYSICAL_ADDRESS)0;
    CoreInfo[CpuIndex].MailboxClearAddress = (EFI_PHYSICAL_ADDRESS)0;
    CoreInfo[CpuIndex].MailboxClearValue   = (UINT64)0xFFFFFFFF;
    DEBUG ((DEBUG_INFO, "%a: MPIDR 0x%Lx\n", __func__, Mpidr));
    CpuIndex++;
  }

  ASSERT (CpuIndex == CpuCount);
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
PlatformPeim (
  VOID
  )
{
  RETURN_STATUS             RetStatus;
  VOID                      *Base;
  VOID                      *NewBase;
  UINTN                     FdtSize;
  UINTN                     FdtPages;
  UINT64                    *FdtHobData;
  EARLY_PL011_BASE_ADDRESS  *UartHobData;
  FDT_SERIAL_PORTS          Ports;
  INT32                     Node, Prev;
  INT32                     Parent, Depth;
  CONST CHAR8               *Compatible;
  CONST CHAR8               *CompItem;
  INT32                     Len;
  INT32                     RangesLen;
  CONST UINT64              *RegProp;
  CONST UINT32              *RangesProp;
  UINT64                    TpmBase;
  EFI_STATUS                Status;
  CHAR8                     DebugLevelBuf[32];
  UINTN                     DebugLevelBufSize;
  CONST CHAR8               *DebugLevelStr;

  Base = (VOID *)(UINTN)PcdGet64 (PcdDeviceTreeInitialBaseAddress);
  ASSERT (Base != NULL);
  ASSERT (FdtCheckHeader (Base) == 0);

  FdtSize  = FdtTotalSize (Base) + PcdGet32 (PcdDeviceTreeAllocationPadding);
  FdtPages = EFI_SIZE_TO_PAGES (FdtSize);
  NewBase  = AllocatePages (FdtPages);
  ASSERT (NewBase != NULL);
  FdtOpenInto (Base, NewBase, EFI_PAGES_TO_SIZE (FdtPages));

  FdtHobData = BuildGuidHob (&gFdtHobGuid, sizeof *FdtHobData);
  ASSERT (FdtHobData != NULL);
  *FdtHobData = (UINTN)NewBase;

  Status = BuildFdtMpCoreInfoHob (NewBase);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: failed to build MP core info HOB: %r\n", __func__, Status));
  }

  UartHobData = BuildGuidHob (&gEarlyPL011BaseAddressGuid, sizeof *UartHobData);
  ASSERT (UartHobData != NULL);
  SetMem (UartHobData, sizeof *UartHobData, 0);

  Status = FdtSerialGetPorts (Base, "arm,pl011", &Ports);
  if (!EFI_ERROR (Status)) {
    if (Ports.NumberOfPorts == 1) {
      //
      // Just one UART; direct both SerialPortLib+console and DebugLib to it.
      //
      UartHobData->ConsoleAddress = Ports.BaseAddress[0];
      UartHobData->DebugAddress   = Ports.BaseAddress[0];
    } else {
      UINT64  ConsoleAddress;

      Status = FdtSerialGetConsolePort (Base, &ConsoleAddress);
      if (EFI_ERROR (Status)) {
        //
        // At least two UARTs; but failed to get the console preference. Use the
        // first UART for SerialPortLib+console, and the second one for
        // DebugLib.
        //
        UartHobData->ConsoleAddress = Ports.BaseAddress[0];
        UartHobData->DebugAddress   = Ports.BaseAddress[1];
      } else {
        //
        // At least two UARTs; and console preference available. Use the
        // preferred UART for SerialPortLib+console, and *another* UART for
        // DebugLib.
        //
        UartHobData->ConsoleAddress = ConsoleAddress;
        if (ConsoleAddress == Ports.BaseAddress[0]) {
          UartHobData->DebugAddress = Ports.BaseAddress[1];
        } else {
          UartHobData->DebugAddress = Ports.BaseAddress[0];
        }
      }
    }

    DEBUG ((
      DEBUG_INFO,
      "%a: PL011 UART (console) @ 0x%lx\n",
      __func__,
      UartHobData->ConsoleAddress
      ));
    DEBUG ((
      DEBUG_INFO,
      "%a: PL011 UART (debug) @ 0x%lx\n",
      __func__,
      UartHobData->DebugAddress
      ));
  }

  DebugLevelBufSize = sizeof (DebugLevelBuf);
  Status            = QemuFwCfgGetAsString (
                        "opt/org.tianocore/DebugLevel",
                        &DebugLevelBufSize,
                        DebugLevelBuf
                        );
  if (!RETURN_ERROR (Status)) {
    DebugLevelStr = DebugLevelBuf;
  } else {
    DebugLevelStr = (CONST CHAR8 *)PcdGetPtr (PcdSerialDebugPrintErrorLevel);
  }

  UartHobData->DebugLevelSet = ParseSerialDebugLevel (
                                 DebugLevelStr,
                                 &UartHobData->DebugLevel
                                 );

  TpmBase = 0;

  //
  // Set Parent to suppress incorrect compiler/analyzer warnings.
  //
  Parent = 0;

  for (Prev = Depth = 0; ; Prev = Node) {
    Node = FdtNextNode (Base, Prev, &Depth);
    if (Node < 0) {
      break;
    }

    if (Depth == 1) {
      Parent = Node;
    }

    Compatible = FdtGetProp (Base, Node, "compatible", &Len);

    //
    // Iterate over the NULL-separated items in the compatible string
    //
    for (CompItem = Compatible; CompItem != NULL && CompItem < Compatible + Len;
         CompItem += 1 + AsciiStrLen (CompItem))
    {
      if (FeaturePcdGet (PcdTpm2SupportEnabled) &&
          (AsciiStrCmp (CompItem, "tcg,tpm-tis-mmio") == 0))
      {
        RegProp = FdtGetProp (Base, Node, "reg", &Len);
        ASSERT (Len == 8 || Len == 16);
        if (Len == 8) {
          TpmBase = Fdt32ToCpu (RegProp[0]);
        } else if (Len == 16) {
          TpmBase = Fdt64ToCpu (ReadUnaligned64 ((UINT64 *)RegProp));
        }

        if (Depth > 1) {
          //
          // QEMU/mach-virt may put the TPM on the platform bus, in which case
          // we have to take its 'ranges' property into account to translate the
          // MMIO address. This consists of a <child base, parent base, size>
          // tuple, where the child base and the size use the same number of
          // cells as the 'reg' property above, and the parent base uses 2 cells
          //
          RangesProp = FdtGetProp (Base, Parent, "ranges", &RangesLen);
          ASSERT (RangesProp != NULL);

          //
          // a plain 'ranges' attribute without a value implies a 1:1 mapping
          //
          if (RangesLen != 0) {
            //
            // assume a single translated range with 2 cells for the parent base
            //
            if (RangesLen != Len + 2 * sizeof (UINT32)) {
              DEBUG ((
                DEBUG_WARN,
                "%a: 'ranges' property has unexpected size %d\n",
                __func__,
                RangesLen
                ));
              break;
            }

            if (Len == 8) {
              TpmBase -= Fdt32ToCpu (RangesProp[0]);
            } else {
              TpmBase -= Fdt64ToCpu (ReadUnaligned64 ((UINT64 *)RangesProp));
            }

            //
            // advance RangesProp to the parent bus address
            //
            RangesProp = (UINT32 *)((UINT8 *)RangesProp + Len / 2);
            TpmBase   += Fdt64ToCpu (ReadUnaligned64 ((UINT64 *)RangesProp));
          }
        }

        break;
      }
    }
  }

  if (FeaturePcdGet (PcdTpm2SupportEnabled)) {
    if (TpmBase != 0) {
      DEBUG ((DEBUG_INFO, "%a: TPM @ 0x%lx\n", __func__, TpmBase));

      Status = (EFI_STATUS)PcdSet64S (PcdTpmBaseAddress, TpmBase);
      ASSERT_EFI_ERROR (Status);

      Status = PeiServicesInstallPpi (&mTpm2DiscoveredPpi);
    } else {
      Status = PeiServicesInstallPpi (&mTpm2InitializationDonePpi);
    }

    ASSERT_EFI_ERROR (Status);
  }

  BuildFvHob (PcdGet64 (PcdFvBaseAddress), PcdGet32 (PcdFvSize));

  RetStatus = ArmCcaInitialiseHobs ();
  if (RETURN_ERROR (RetStatus)) {
    ASSERT (0);
    return (EFI_STATUS)RetStatus;
  }

  return EFI_SUCCESS;
}
