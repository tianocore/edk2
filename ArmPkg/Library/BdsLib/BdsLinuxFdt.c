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

#include <Library/ArmSmcLib.h>
#include <Library/PcdLib.h>
#include <libfdt.h>

#include <IndustryStandard/ArmSmc.h>

#include "BdsInternal.h"
#include "BdsLinuxLoader.h"

#define ALIGN(x, a)     (((x) + ((a) - 1)) & ~((a) - 1))
#define PALIGN(p, a)    ((void *)(ALIGN((unsigned long)(p), (a))))
#define GET_CELL(p)     (p += 4, *((const UINT32 *)(p-4)))

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
DebugDumpFdt (
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

  {
    // Can 'memreserve' be printed by below code?
    INTN num = fdt_num_mem_rsv(FdtBlob);
    INTN i, err;
    UINT64 addr = 0,size = 0;

    for (i = 0; i < num; i++) {
      err = fdt_get_mem_rsv(FdtBlob, i, &addr, &size);
      if (err) {
        DEBUG((EFI_D_ERROR, "Error (%d) : Cannot get memreserve section (%d)\n", err, i));
      }
      else {
        Print(L"/memreserve/ \t0x%lx \t0x%lx;\n",addr,size);
      }
    }
  }

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
      p = PALIGN(p + AsciiStrLen (s) + 1, 4);

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

STATIC
BOOLEAN
IsLinuxReservedRegion (
  IN EFI_MEMORY_TYPE MemoryType
  )
{
  switch(MemoryType) {
  case EfiRuntimeServicesCode:
  case EfiRuntimeServicesData:
  case EfiUnusableMemory:
  case EfiACPIReclaimMemory:
  case EfiACPIMemoryNVS:
    return TRUE;
  default:
    return FALSE;
  }
}


typedef struct {
  UINTN   Base;
  UINTN   Size;
} FdtRegion;

EFI_STATUS
PrepareFdt (
  IN     CONST CHAR8*         CommandLineArguments,
  IN     EFI_PHYSICAL_ADDRESS InitrdImage,
  IN     UINTN                InitrdImageSize,
  IN OUT EFI_PHYSICAL_ADDRESS *FdtBlobBase,
  IN OUT UINTN                *FdtBlobSize
  )
{
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  NewFdtBlobBase;
  UINTN                 NewFdtBlobSize;
  VOID*                 fdt;
  INTN                  err;
  INTN                  node;
  INTN                  cpu_node;
  INTN                  lenp;
  CONST VOID*           BootArg;
  CONST VOID*           Method;
  EFI_PHYSICAL_ADDRESS  InitrdImageStart;
  EFI_PHYSICAL_ADDRESS  InitrdImageEnd;
  FdtRegion             Region;
  UINTN                 Index;
  CHAR8                 Name[10];
  LIST_ENTRY            ResourceList;
  BDS_SYSTEM_MEMORY_RESOURCE  *Resource;
  ARM_PROCESSOR_TABLE   *ArmProcessorTable;
  ARM_CORE_INFO         *ArmCoreInfoTable;
  UINT32                MpId;
  UINT32                ClusterId;
  UINT32                CoreId;
  UINT64                CpuReleaseAddr;
  UINTN                 MemoryMapSize;
  EFI_MEMORY_DESCRIPTOR *MemoryMap;
  UINTN                 MapKey;
  UINTN                 DescriptorSize;
  UINT32                DescriptorVersion;
  UINTN                 Pages;
  BOOLEAN               PsciSmcSupported;
  UINTN                 Rx;
  UINTN                 OriginalFdtSize;

  //
  // Ensure the Power State Coordination Interface (PSCI) SMCs are there if supported
  //
  PsciSmcSupported = FALSE;
  if (FeaturePcdGet (PcdArmPsciSupport) == TRUE) {
    // Check the SMC response to the Presence SMC
    Rx   = ARM_SMC_ID_PRESENCE;
    ArmCallSmc (&Rx);
    if (Rx == 1) {
      // Check the SMC UID
      Rx   = ARM_SMC_ID_UID;
      ArmCallSmc (&Rx);
      if (Rx == ARM_TRUSTZONE_UID_4LETTERID) {
        Rx   = ARM_SMC_ID_UID + 1;
        ArmCallSmc (&Rx);
        if (Rx == ARM_TRUSTZONE_ARM_UID) {
          PsciSmcSupported = TRUE;
        }
      }
      if (PsciSmcSupported == FALSE) {
        DEBUG((EFI_D_ERROR,"Warning: The Power State Coordination Interface (PSCI) is not supported"
                           "by your platform Trusted Firmware.\n"));
      }
    }
  }

  //
  // Sanity checks on the original FDT blob.
  //
  err = fdt_check_header ((VOID*)(UINTN)(*FdtBlobBase));
  if (err != 0) {
    Print (L"ERROR: Device Tree header not valid (err:%d)\n", err);
    return EFI_INVALID_PARAMETER;
  }

  // The original FDT blob might have been loaded partially.
  // Check that it is not the case.
  OriginalFdtSize = (UINTN)fdt_totalsize ((VOID*)(UINTN)(*FdtBlobBase));
  if (OriginalFdtSize > *FdtBlobSize) {
    Print (L"ERROR: Incomplete FDT. Only %d/%d bytes have been loaded.\n",
           *FdtBlobSize, OriginalFdtSize);
    return EFI_INVALID_PARAMETER;
  }

  //
  // Allocate memory for the new FDT
  //
  NewFdtBlobSize = OriginalFdtSize + FDT_ADDITIONAL_ENTRIES_SIZE;

  // Try below a watermark address
  Status = EFI_NOT_FOUND;
  if (PcdGet32(PcdArmLinuxFdtMaxOffset) != 0) {
    NewFdtBlobBase = LINUX_FDT_MAX_OFFSET;
    Status = gBS->AllocatePages (AllocateMaxAddress, EfiBootServicesData, EFI_SIZE_TO_PAGES(NewFdtBlobSize), &NewFdtBlobBase);
    if (EFI_ERROR(Status)) {
      DEBUG ((EFI_D_WARN, "Warning: Failed to load FDT below address 0x%lX (%r). Will try again at a random address anywhere.\n", NewFdtBlobBase, Status));
    }
  }

  // Try anywhere there is available space
  if (EFI_ERROR(Status)) {
    Status = gBS->AllocatePages (AllocateAnyPages, EfiBootServicesData, EFI_SIZE_TO_PAGES(NewFdtBlobSize), &NewFdtBlobBase);
    if (EFI_ERROR(Status)) {
      ASSERT_EFI_ERROR(Status);
      goto FAIL_ALLOCATE_NEW_FDT;
    } else {
      DEBUG ((EFI_D_WARN, "WARNING: Loaded FDT at random address 0x%lX.\nWARNING: There is a risk of accidental overwriting by other code/data.\n", NewFdtBlobBase));
    }
  }

  // Load the Original FDT tree into the new region
  fdt = (VOID*)(UINTN)NewFdtBlobBase;
  err = fdt_open_into((VOID*)(UINTN)(*FdtBlobBase), fdt, NewFdtBlobSize);
  if (err) {
    DEBUG((EFI_D_ERROR, "fdt_open_into(): %a\n", fdt_strerror(err)));
    Status = EFI_INVALID_PARAMETER;
    goto FAIL_NEW_FDT;
  }

  DEBUG_CODE_BEGIN();
    //DebugDumpFdt (fdt);
  DEBUG_CODE_END();

  node = fdt_subnode_offset(fdt, 0, "chosen");
  if (node < 0) {
    // The 'chosen' node does not exist, create it
    node = fdt_add_subnode(fdt, 0, "chosen");
    if (node < 0) {
      DEBUG((EFI_D_ERROR,"Error on finding 'chosen' node\n"));
      Status = EFI_INVALID_PARAMETER;
      goto FAIL_NEW_FDT;
    }
  }

  DEBUG_CODE_BEGIN();
    BootArg = fdt_getprop(fdt, node, "bootargs", &lenp);
    if (BootArg != NULL) {
      DEBUG((EFI_D_ERROR,"BootArg: %a\n",BootArg));
    }
  DEBUG_CODE_END();

  //
  // Set Linux CmdLine
  //
  if ((CommandLineArguments != NULL) && (AsciiStrLen (CommandLineArguments) > 0)) {
    err = fdt_setprop(fdt, node, "bootargs", CommandLineArguments, AsciiStrSize(CommandLineArguments));
    if (err) {
      DEBUG((EFI_D_ERROR,"Fail to set new 'bootarg' (err:%d)\n",err));
    }
  }

  //
  // Set Linux Initrd
  //
  if (InitrdImageSize != 0) {
    InitrdImageStart = cpu_to_fdt64 (InitrdImage);
    err = fdt_setprop(fdt, node, "linux,initrd-start", &InitrdImageStart, sizeof(EFI_PHYSICAL_ADDRESS));
    if (err) {
      DEBUG((EFI_D_ERROR,"Fail to set new 'linux,initrd-start' (err:%d)\n",err));
    }
    InitrdImageEnd = cpu_to_fdt64 (InitrdImage + InitrdImageSize);
    err = fdt_setprop(fdt, node, "linux,initrd-end", &InitrdImageEnd, sizeof(EFI_PHYSICAL_ADDRESS));
    if (err) {
      DEBUG((EFI_D_ERROR,"Fail to set new 'linux,initrd-start' (err:%d)\n",err));
    }
  }

  //
  // Set Physical memory setup if does not exist
  //
  node = fdt_subnode_offset(fdt, 0, "memory");
  if (node < 0) {
    // The 'memory' node does not exist, create it
    node = fdt_add_subnode(fdt, 0, "memory");
    if (node >= 0) {
      fdt_setprop_string(fdt, node, "name", "memory");
      fdt_setprop_string(fdt, node, "device_type", "memory");

      GetSystemMemoryResources (&ResourceList);
      Resource = (BDS_SYSTEM_MEMORY_RESOURCE*)ResourceList.ForwardLink;

      if (sizeof(UINTN) == sizeof(UINT32)) {
        Region.Base = cpu_to_fdt32((UINTN)Resource->PhysicalStart);
        Region.Size = cpu_to_fdt32((UINTN)Resource->ResourceLength);
      } else {
        Region.Base = cpu_to_fdt64((UINTN)Resource->PhysicalStart);
        Region.Size = cpu_to_fdt64((UINTN)Resource->ResourceLength);
      }

      err = fdt_setprop(fdt, node, "reg", &Region, sizeof(Region));
      if (err) {
        DEBUG((EFI_D_ERROR,"Fail to set new 'memory region' (err:%d)\n",err));
      }
    }
  }

  //
  // Add the memory regions reserved by the UEFI Firmware
  //

  // Retrieve the UEFI Memory Map
  MemoryMap = NULL;
  MemoryMapSize = 0;
  Status = gBS->GetMemoryMap (&MemoryMapSize, MemoryMap, &MapKey, &DescriptorSize, &DescriptorVersion);
  if (Status == EFI_BUFFER_TOO_SMALL) {
    Pages = EFI_SIZE_TO_PAGES (MemoryMapSize) + 1;
    MemoryMap = AllocatePages (Pages);
    Status = gBS->GetMemoryMap (&MemoryMapSize, MemoryMap, &MapKey, &DescriptorSize, &DescriptorVersion);
  }

  // Go through the list and add the reserved region to the Device Tree
  if (!EFI_ERROR(Status)) {
    for (Index = 0; Index < (MemoryMapSize / sizeof(EFI_MEMORY_DESCRIPTOR)); Index++) {
      if (IsLinuxReservedRegion ((EFI_MEMORY_TYPE)MemoryMap[Index].Type)) {
        DEBUG((DEBUG_VERBOSE, "Reserved region of type %d [0x%X, 0x%X]\n",
            MemoryMap[Index].Type,
            (UINTN)MemoryMap[Index].PhysicalStart,
            (UINTN)(MemoryMap[Index].PhysicalStart + MemoryMap[Index].NumberOfPages * EFI_PAGE_SIZE)));
        err = fdt_add_mem_rsv(fdt, MemoryMap[Index].PhysicalStart, MemoryMap[Index].NumberOfPages * EFI_PAGE_SIZE);
        if (err != 0) {
          Print(L"Warning: Fail to add 'memreserve' (err:%d)\n", err);
        }
      }
    }
  }

  //
  // Setup Arm Mpcore Info if it is a multi-core or multi-cluster platforms
  //
  for (Index=0; Index < gST->NumberOfTableEntries; Index++) {
    // Check for correct GUID type
    if (CompareGuid (&gArmMpCoreInfoGuid, &(gST->ConfigurationTable[Index].VendorGuid))) {
      MpId = ArmReadMpidr ();
      ClusterId = GET_CLUSTER_ID(MpId);
      CoreId    = GET_CORE_ID(MpId);

      node = fdt_subnode_offset(fdt, 0, "cpus");
      if (node < 0) {
        // Create the /cpus node
        node = fdt_add_subnode(fdt, 0, "cpus");
        fdt_setprop_string(fdt, node, "name", "cpus");
        fdt_setprop_cell(fdt, node, "#address-cells", 1);
        fdt_setprop_cell(fdt, node, "#size-cells", 0);
      }

      // Get pointer to ARM processor table
      ArmProcessorTable = (ARM_PROCESSOR_TABLE *)gST->ConfigurationTable[Index].VendorTable;
      ArmCoreInfoTable = ArmProcessorTable->ArmCpus;

      for (Index = 0; Index < ArmProcessorTable->NumberOfEntries; Index++) {
        AsciiSPrint (Name, 10, "cpu@%d", Index);
        cpu_node = fdt_subnode_offset(fdt, node, Name);
        if (cpu_node < 0) {
          cpu_node = fdt_add_subnode(fdt, node, Name);
          fdt_setprop_string(fdt, cpu_node, "device-type", "cpu");
          fdt_setprop(fdt, cpu_node, "reg", &Index, sizeof(Index));
        }

        // If Power State Coordination Interface (PSCI) is not supported then it is expected the secondary
        // cores are spinning waiting for the Operating System to release them
        if (PsciSmcSupported == FALSE) {
          // We as the bootloader are responsible for either creating or updating
          // these entries. Do not trust the entries in the DT. We only know about
          // 'spin-table' type. Do not try to update other types if defined.
          Method = fdt_getprop(fdt, cpu_node, "enable-method", &lenp);
          if ( (Method == NULL) || (!AsciiStrCmp((CHAR8 *)Method, "spin-table")) ) {
            fdt_setprop_string(fdt, cpu_node, "enable-method", "spin-table");
            CpuReleaseAddr = cpu_to_fdt64(ArmCoreInfoTable[Index].MailboxSetAddress);
            fdt_setprop(fdt, cpu_node, "cpu-release-addr", &CpuReleaseAddr, sizeof(CpuReleaseAddr));

            // If it is not the primary core than the cpu should be disabled
            if (((ArmCoreInfoTable[Index].ClusterId != ClusterId) || (ArmCoreInfoTable[Index].CoreId != CoreId))) {
              fdt_setprop_string(fdt, cpu_node, "status", "disabled");
            }
          } else {
            Print(L"Warning: Unsupported enable-method type for CPU[%d] : %a\n", Index, (CHAR8 *)Method);
          }
        }
      }
      break;
    }
  }

  // If the Power State Coordination Interface is supported then we signal it in the Device Tree
  if (PsciSmcSupported == TRUE) {
    // Before to create it we check if the node is not already defined in the Device Tree
    node = fdt_subnode_offset(fdt, 0, "psci");
    if (node < 0) {
      // The 'psci' node does not exist, create it
      node = fdt_add_subnode(fdt, 0, "psci");
      if (node < 0) {
        DEBUG((EFI_D_ERROR,"Error on creating 'psci' node\n"));
        Status = EFI_INVALID_PARAMETER;
        goto FAIL_NEW_FDT;
      } else {
        fdt_setprop_string(fdt, node, "compatible", "arm,psci");
        fdt_setprop_string(fdt, node, "method", "smc");
        fdt_setprop_cell(fdt, node, "cpu_suspend", ARM_SMC_ARM_CPU_SUSPEND);
        fdt_setprop_cell(fdt, node, "cpu_off", ARM_SMC_ARM_CPU_OFF);
        fdt_setprop_cell(fdt, node, "cpu_on", ARM_SMC_ARM_CPU_ON);
        fdt_setprop_cell(fdt, node, "cpu_migrate", ARM_SMC_ARM_MIGRATE);
      }
    }
  }

  DEBUG_CODE_BEGIN();
    //DebugDumpFdt (fdt);
  DEBUG_CODE_END();

  // If we succeeded to generate the new Device Tree then free the old Device Tree
  gBS->FreePages (*FdtBlobBase, EFI_SIZE_TO_PAGES (*FdtBlobSize));

  *FdtBlobBase = NewFdtBlobBase;
  *FdtBlobSize = (UINTN)fdt_totalsize ((VOID*)(UINTN)(NewFdtBlobBase));
  return EFI_SUCCESS;

FAIL_NEW_FDT:
  gBS->FreePages (NewFdtBlobBase, EFI_SIZE_TO_PAGES (NewFdtBlobSize));

FAIL_ALLOCATE_NEW_FDT:
  *FdtBlobSize = (UINTN)fdt_totalsize ((VOID*)(UINTN)(*FdtBlobBase));
  // Return success even if we failed to update the FDT blob. The original one is still valid.
  return EFI_SUCCESS;
}


