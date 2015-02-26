/** @file
*
*  Copyright (c) 2011-2013, ARM Limited. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#include <Base.h>
#include <Uefi.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BdsLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include <Protocol/DevicePathFromText.h>

#include <Guid/ArmGlobalVariableHob.h>
#include <Guid/Fdt.h>

#include <libfdt.h>

#define ALIGN(x, a)     (((x) + ((a) - 1)) & ~((a) - 1))
#define PALIGN(p, a)    ((void *)(ALIGN((unsigned long)(p), (a))))
#define GET_CELL(p)     (p += 4, *((const uint32_t *)(p-4)))

STATIC
UINTN
IsPrintableString (
  IN CONST VOID* data,
  IN UINTN len
  )
{
  CONST CHAR8 *s = data;
  CONST CHAR8 *ss;

  // Zero length is not
  if (len == 0) {
    return 0;
  }

  // Must terminate with zero
  if (s[len - 1] != '\0') {
    return 0;
  }

  ss = s;
  while (*s/* && isprint(*s)*/) {
    s++;
  }

  // Not zero, or not done yet
  if (*s != '\0' || (s + 1 - ss) < len) {
    return 0;
  }

  return 1;
}

STATIC
VOID
PrintData (
  IN CONST CHAR8* data,
  IN UINTN len
  )
{
  UINTN i;
  CONST CHAR8 *p = data;

  // No data, don't print
  if (len == 0)
    return;

  if (IsPrintableString (data, len)) {
    Print(L" = \"%a\"", (const char *)data);
  } else if ((len % 4) == 0) {
    Print(L" = <");
    for (i = 0; i < len; i += 4) {
      Print(L"0x%08x%a", fdt32_to_cpu(GET_CELL(p)),i < (len - 4) ? " " : "");
    }
    Print(L">");
  } else {
    Print(L" = [");
    for (i = 0; i < len; i++)
      Print(L"%02x%a", *p++, i < len - 1 ? " " : "");
    Print(L"]");
  }
}

VOID
DumpFdt (
  IN VOID*                FdtBlob
  )
{
  struct fdt_header *bph;
  UINT32 off_dt;
  UINT32 off_str;
  CONST CHAR8* p_struct;
  CONST CHAR8* p_strings;
  CONST CHAR8* p;
  CONST CHAR8* s;
  CONST CHAR8* t;
  UINT32 tag;
  UINTN sz;
  UINTN depth;
  UINTN shift;
  UINT32 version;

  depth = 0;
  shift = 4;

  bph = FdtBlob;
  off_dt = fdt32_to_cpu(bph->off_dt_struct);
  off_str = fdt32_to_cpu(bph->off_dt_strings);
  p_struct = (CONST CHAR8*)FdtBlob + off_dt;
  p_strings = (CONST CHAR8*)FdtBlob + off_str;
  version = fdt32_to_cpu(bph->version);

  p = p_struct;
  while ((tag = fdt32_to_cpu(GET_CELL(p))) != FDT_END) {

    if (tag == FDT_BEGIN_NODE) {
      s = p;
      p = PALIGN(p + strlen(s) + 1, 4);

      if (*s == '\0')
              s = "/";

      Print(L"%*s%a {\n", depth * shift, L" ", s);

      depth++;
      continue;
    }

    if (tag == FDT_END_NODE) {
      depth--;

      Print(L"%*s};\n", depth * shift, L" ");
      continue;
    }

    if (tag == FDT_NOP) {
      Print(L"%*s// [NOP]\n", depth * shift, L" ");
      continue;
    }

    if (tag != FDT_PROP) {
      Print(L"%*s ** Unknown tag 0x%08x\n", depth * shift, L" ", tag);
      break;
    }
    sz = fdt32_to_cpu(GET_CELL(p));
    s = p_strings + fdt32_to_cpu(GET_CELL(p));
    if (version < 16 && sz >= 8)
            p = PALIGN(p, 8);
    t = p;

    p = PALIGN(p + sz, 4);

    Print(L"%*s%a", depth * shift, L" ", s);
    PrintData(t, sz);
    Print(L";\n");
  }
}

EFI_STATUS
EblDumpFdt (
  IN UINTN  Argc,
  IN CHAR8  **Argv
  )
{
  EFI_STATUS  Status;
  VOID        *FdtBlob;
  UINTN       Ret;

  // If no FDT file is passed to the argument then get the one from the platform
  if (Argc < 2) {
    Status = EfiGetSystemConfigurationTable (&gFdtTableGuid, &FdtBlob);
    if (EFI_ERROR (Status)) {
      Print (L"ERROR: Did not find the Fdt Blob.\n");
      return Status;
    }
  } else {
    return EFI_NOT_FOUND;
  }

  Ret = fdt_check_header (FdtBlob);
  if (Ret != 0) {
    Print (L"ERROR: Device Tree header not valid (err:%d)\n", Ret);
    return EFI_INVALID_PARAMETER;
  }

  DumpFdt (FdtBlob);

  return EFI_SUCCESS;
}
