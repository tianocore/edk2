/** @file
  Instance of the RISC-V extension discovery library.

  Copyright (c) 2023, Ventana Micro Systems Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BaseRiscVSbiLib.h>
#include <Library/BaseRiscVExtDiscoveryLib.h>
#include <Library/DebugLib.h>
#include <Library/FdtLib.h>
#include <Library/MemoryAllocationLib.h>

STATIC
INT32
EFIAPI
AsciiStrinCmp (
  IN      CONST CHAR8  *FirstString,
  IN      CONST CHAR8  *SecondString,
  IN      UINTN        Length
  )
{
  CHAR8   C1, C2;
  UINT32  i = 0;

  while (i < Length) {
    C1 = AsciiCharToUpper (FirstString[i]);
    C2 = AsciiCharToUpper (SecondString[i]);

    if (C1 != C2) {
      return (INT32)(C1 - C2);
    }

    if (C1 == '\0') {
      break;
    }

    i++;
  }

  // Validate the end of extension in isa string is one of _, \0, or a number.
  if ((FirstString[i] == '_') ||
      (FirstString[i] == '\0') ||
      ((FirstString[i] >= 0x30) && (FirstString[i] <= 0x39)))
  {
    return 0;
  }

  return -1;
}

STATIC
EFIAPI
UINT32
CheckExtInIsaString (
  CHAR8  *Isa,
  CHAR8  *Ext
  )
{
  INT32  i = 0;

  if (AsciiStrLen (Ext) > 1) {
    while ((i < AsciiStrLen (Isa)) && *Isa) {
      if (AsciiStrinCmp (Isa, Ext, AsciiStrLen (Ext)) == 0) {
        DEBUG ((DEBUG_INFO, "%a: Found extension %a\n", __func__, Ext));
        return 1;
      }

      Isa++;
      i++;
    }
  } else {
    CHAR8  *TmpIsa = Isa;

    TmpIsa += 4; // Skip RV64
    while ((AsciiCharToUpper (*TmpIsa) != 'Z') &&
           (AsciiCharToUpper (*TmpIsa) != 'X') &&
           (AsciiCharToUpper (*TmpIsa) != 'S') &&
           (AsciiCharToUpper (*TmpIsa) != '\0'))
    {
      if (AsciiCharToUpper (*Ext) == AsciiCharToUpper (*TmpIsa)) {
        DEBUG ((DEBUG_INFO, "%a: Found extension %a\n", __func__, Ext));
        return 1;
      }

      TmpIsa++;
    }
  }

  return 0;
}

/*
 * Find  whether an  extension is enabled or not
 */
STATIC
EFIAPI
UINT32
IsRiscVExtPresentFdt (
  CONST VOID  *DeviceTreeBase,
  CHAR8       *Ext
  )
{
  VOID   *Prop;
  INT32  Len;
  INT32  CpuNode;

  // Make sure we have a valid device tree blob
  if (FdtCheckHeader (DeviceTreeBase) != 0) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: No DTB found @ 0x%p\n",
      __func__,
      DeviceTreeBase
      ));
    return 0;
  }

  CpuNode = FdtPathOffset (DeviceTreeBase, "/cpus");
  if (CpuNode <= 0) {
    DEBUG ((DEBUG_ERROR, "Unable to locate /cpus in device tree\n"));
    return 0;
  }

  CpuNode = FdtFirstSubnode (DeviceTreeBase, CpuNode);
  Prop    = (VOID *)FdtGetProp (DeviceTreeBase, CpuNode, "riscv,isa", &Len);
  if (!Prop) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to parse cpu node: riscv,isa\n",
      __func__
      ));

    return 0;
  }

  return CheckExtInIsaString ((CHAR8 *)Prop, Ext);
}

EFIAPI
UINT32
IsRiscVExtSupported (
  CHAR8  *Ext
  )
{
  EFI_RISCV_FIRMWARE_CONTEXT  *FirmwareContext;
  CONST VOID                  *DeviceTreeBase;

  if (Ext == NULL) {
    return 0;
  }

  FirmwareContext = NULL;
  GetFirmwareContextPointer (&FirmwareContext);
  if (FirmwareContext == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Firmware Context is NULL\n", __func__));
    return 0;
  }

  DeviceTreeBase = (VOID *)FirmwareContext->FlattenedDeviceTree;
  ASSERT (DeviceTreeBase != NULL);

  if (DeviceTreeBase == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: FDT is NULL\n", __func__));
    return 0;
  }

  return IsRiscVExtPresentFdt (DeviceTreeBase, Ext);
}
