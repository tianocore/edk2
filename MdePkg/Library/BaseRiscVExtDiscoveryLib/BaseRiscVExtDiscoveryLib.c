/** @file
  Instance of the RISC-V extension discovery library.

  Copyright (c) 2023, Ventana Micro Systems Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>

#include <Pi/PiBootMode.h>
#include <Pi/PiHob.h>

#include <Guid/Fdt.h>
#include <Guid/FdtHob.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/BaseRiscVSbiLib.h>
#include <Library/BaseRiscVExtDiscoveryLib.h>
#include <Library/DebugLib.h>
#include <Library/FdtLib.h>
#include <Library/HobLib.h>
#include <Library/MemoryAllocationLib.h>

/**
  Search for extension in the ISA string.

  The ISA string should always start with "rv64".

  @param  Isa     Complete ISA string in which we need to search for the extension.
  @param  Ext     RISC-V ratified extension name.

**/
STATIC
BOOLEAN
RiscVCheckExtInIsaString (
  IN CHAR8  *Isa,
  IN CHAR8  *Ext
  )
{
  if (AsciiStrLen (Ext) > 1) {
    CHAR8  TmpIsaString[RISCV_MAX_EXT_LENGTH];
    INT32  i;

    i = AsciiStrLen (Isa);
    while ((i > 0) && (*Isa != '\0')) {
      AsciiStrnCpyS (TmpIsaString, RISCV_MAX_EXT_LENGTH, Isa, AsciiStrLen (Ext));
      if (AsciiStriCmp (TmpIsaString, Ext) == 0) {
        DEBUG ((DEBUG_INFO, "%a: Found extension %a\n", __func__, Ext));
        return TRUE;
      }

      Isa++;
      i--;
    }
  } else {
    if (AsciiStriCmp (Isa, "RV64")) {
      DEBUG ((DEBUG_ERROR, "%a: Invalid ISA string\n", __func__));
      return FALSE;
    }

    Isa += 4; // Skip RV64
    while ((AsciiCharToUpper (*Isa) != 'Z') &&
           (AsciiCharToUpper (*Isa) != 'X') &&
           (AsciiCharToUpper (*Isa) != 'S') &&
           (AsciiCharToUpper (*Isa) != '\0'))
    {
      if (AsciiCharToUpper (*Ext) == AsciiCharToUpper (*Isa)) {
        DEBUG ((DEBUG_INFO, "%a: Found extension %a\n", __func__, Ext));
        return TRUE;
      }

      Isa++;
    }
  }

  return FALSE;
}

/**
  Find  whether an extension is present in FDT.

  @param  DeviceTreeBase     Base address of the Flattened Device Tree.
  @param  Ext                RISC-V ratified extension name.
**/
STATIC
CHAR8 *
GetIsaStringFromFdt (
  IN CONST VOID  *DeviceTreeBase,
  IN CHAR8       *Ext
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
    return NULL;
  }

  CpuNode = FdtPathOffset (DeviceTreeBase, "/cpus");
  if (CpuNode <= 0) {
    DEBUG ((DEBUG_ERROR, "Unable to locate /cpus in device tree\n"));
    return NULL;
  }

  CpuNode = FdtFirstSubnode (DeviceTreeBase, CpuNode);
  Prop    = (VOID *)FdtGetProp (DeviceTreeBase, CpuNode, "riscv,isa", &Len);
  if (!Prop) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: Failed to parse cpu node: riscv,isa\n",
      __func__
      ));

    return NULL;
  }

  return (CHAR8 *)Prop;
}

/**
  Find whether an extension is supported by the platform.

  @param  Ext                RISC-V ratified extension name.
 **/
BOOLEAN
EFIAPI
IsRiscVExtSupported (
  IN CHAR8  *Ext
  )
{
  VOID   *DeviceTreeBase;
  CHAR8  *IsaString;
  VOID   *Hob;

  if (Ext == NULL) {
    DEBUG ((DEBUG_ERROR, "Extension name is NULL\n"));
    return FALSE;
  }

  Hob = GetFirstGuidHob (&gFdtHobGuid);
  if ((Hob == NULL) || (GET_GUID_HOB_DATA_SIZE (Hob) != sizeof (UINT64))) {
    DEBUG ((DEBUG_ERROR, "%a: FDT HOB is NULL\n", __func__));
    return FALSE;
  }

  DeviceTreeBase = (VOID *)(UINTN)*(UINT64 *)GET_GUID_HOB_DATA (Hob);
  if (DeviceTreeBase == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: FDT is NULL\n", __func__));
    return FALSE;
  }

  IsaString = GetIsaStringFromFdt (DeviceTreeBase, Ext);
  if (IsaString == NULL) {
    DEBUG ((DEBUG_ERROR, "Unable to get ISA string from FDT\n"));
    return FALSE;
  }

  return RiscVCheckExtInIsaString (IsaString, Ext);
}
