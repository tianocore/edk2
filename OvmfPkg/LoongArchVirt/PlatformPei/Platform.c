/** @file
  Platform PEI driver

  Copyright (c) 2024 Loongson Technology Corporation Limited. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - Mem - Memory
**/

//
// The package level header files this module uses
//
#include <PiPei.h>
//
// The Library classes this module consumes
//
#include <Guid/MemoryTypeInformation.h>
#include <Guid/FdtHob.h>
#include <Library/BaseMemoryLib.h>
#include <Library/CpuMmuInitLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/MpInitLib.h>
#include <Library/PcdLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/PeiServicesLib.h>
#include <Library/PlatformHookLib.h>
#include <Library/QemuFwCfgLib.h>
#include <libfdt.h>
#include <Ppi/MasterBootMode.h>
#include <Register/LoongArch64/Cpucfg.h>
#include <Register/LoongArch64/Csr.h>
#include <Uefi/UefiSpec.h>

#include "Platform.h"

STATIC EFI_MEMORY_TYPE_INFORMATION  mDefaultMemoryTypeInformation[] = {
  { EfiReservedMemoryType,  0x004 },
  { EfiRuntimeServicesData, 0x024 },
  { EfiRuntimeServicesCode, 0x030 },
  { EfiBootServicesCode,    0x180 },
  { EfiBootServicesData,    0xF00 },
  { EfiMaxMemoryType,       0x000 }
};

//
// Module globals
//
CONST EFI_PEI_PPI_DESCRIPTOR  mPpiListBootMode = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiPeiMasterBootModePpiGuid,
  NULL
};

STATIC EFI_BOOT_MODE  mBootMode = BOOT_WITH_FULL_CONFIGURATION;

/**
  Create system type  memory range hand off block.

  @param  MemoryBase    memory base address.
  @param  MemoryLimit  memory length.

  @return  VOID
**/
STATIC
VOID
AddMemoryBaseSizeHob (
  EFI_PHYSICAL_ADDRESS  MemoryBase,
  UINT64                MemorySize
  )
{
  BuildResourceDescriptorHob (
    EFI_RESOURCE_SYSTEM_MEMORY,
    EFI_RESOURCE_ATTRIBUTE_PRESENT |
    EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
    EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE |
    EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE |
    EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE |
    EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE |
    EFI_RESOURCE_ATTRIBUTE_TESTED,
    MemoryBase,
    MemorySize
    );
}

/**
  Create  memory range hand off block.

  @param  MemoryBase    memory base address.
  @param  MemoryLimit  memory length.

  @return  VOID
**/
VOID
AddMemoryRangeHob (
  EFI_PHYSICAL_ADDRESS  MemoryBase,
  EFI_PHYSICAL_ADDRESS  MemoryLimit
  )
{
  AddMemoryBaseSizeHob (MemoryBase, (UINT64)(MemoryLimit - MemoryBase));
}

STATIC
VOID
SaveRtcRegisterAddressHob (
  UINT64  RtcRegisterBase
  )
{
  UINT64  Data64;

  //
  // Build location of RTC register base address buffer in HOB
  //
  Data64 = (UINT64)(UINTN)RtcRegisterBase;

  BuildGuidDataHob (
    &gRtcRegisterBaseAddressHobGuid,
    (VOID *)&Data64,
    sizeof (UINT64)
    );
}

/**
  Create  memory type information hand off block.

  @param  VOID

  @return  VOID
**/
STATIC
VOID
MemMapInitialization (
  VOID
  )
{
  DEBUG ((DEBUG_INFO, "==%a==\n", __func__));
  //
  // Create Memory Type Information HOB
  //
  BuildGuidDataHob (
    &gEfiMemoryTypeInformationGuid,
    mDefaultMemoryTypeInformation,
    sizeof (mDefaultMemoryTypeInformation)
    );
}

/** Get the Rtc base address from the DT.

  This function fetches the node referenced in the "loongson,ls7a-rtc"
  property of the "reg" node and returns the base address of
  the RTC.

  @param [in]   Fdt                   Pointer to a Flattened Device Tree (Fdt).
  @param [out]  RtcBaseAddress  If success, contains the base address
                                      of the Rtc.

  @retval EFI_SUCCESS             The function completed successfully.
  @retval EFI_NOT_FOUND           RTC info not found in DT.
  @retval EFI_INVALID_PARAMETER   Invalid parameter.
**/
STATIC
EFI_STATUS
EFIAPI
GetRtcAddress (
  IN  CONST VOID    *Fdt,
  OUT       UINT64  *RtcBaseAddress
  )
{
  INT32         Node;
  INT32         Prev;
  CONST CHAR8   *Type;
  INT32         Len;
  CONST UINT64  *RegProp;
  EFI_STATUS    Status;

  if ((Fdt == NULL) || (fdt_check_header (Fdt) != 0)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = EFI_NOT_FOUND;
  for (Prev = 0; ; Prev = Node) {
    Node = fdt_next_node (Fdt, Prev, NULL);
    if (Node < 0) {
      break;
    }

    //
    // Check for memory node
    //
    Type = fdt_getprop (Fdt, Node, "compatible", &Len);
    if ((Type) && (AsciiStrnCmp (Type, "loongson,ls7a-rtc", Len) == 0)) {
      //
      // Get the 'reg' property of this node. For now, we will assume
      // two 8 byte quantities for base and size, respectively.
      //
      RegProp = fdt_getprop (Fdt, Node, "reg", &Len);
      if ((RegProp != 0) && (Len == (2 * sizeof (UINT64)))) {
        *RtcBaseAddress = SwapBytes64 (RegProp[0]);
        Status          = RETURN_SUCCESS;
        DEBUG ((DEBUG_INFO, "%a Len %d RtcBase %llx\n", __func__, Len, *RtcBaseAddress));
        break;
      } else {
        DEBUG ((DEBUG_ERROR, "%a: Failed to parse FDT rtc node\n", __func__));
        break;
      }
    }
  }

  return Status;
}

/**
  Misc Initialization.

  @param  VOID

  @return  VOID
**/
STATIC
VOID
MiscInitialization (
  VOID
  )
{
  CPUCFG_REG1_INFO_DATA  CpucfgReg1Data;
  UINT8                  CpuPhysMemAddressWidth;

  DEBUG ((DEBUG_INFO, "==%a==\n", __func__));

  //
  // Get the the CPU physical memory address width.
  //
  AsmCpucfg (CPUCFG_REG1_INFO, &CpucfgReg1Data.Uint32);

  CpuPhysMemAddressWidth = (UINT8)(CpucfgReg1Data.Bits.PALEN + 1);

  //
  // Creat CPU HOBs.
  //
  BuildCpuHob (CpuPhysMemAddressWidth, FixedPcdGet8 (PcdPrePiCpuIoSize));
}

/**
  add fdt hand off block.

  @param  VOID

  @return  VOID
**/
STATIC
VOID
AddFdtHob (
  VOID
  )
{
  VOID           *Base;
  VOID           *NewBase;
  UINTN          FdtSize;
  UINTN          FdtPages;
  UINT64         *FdtHobData;
  UINT64         RtcBaseAddress;
  RETURN_STATUS  Status;

  Base = (VOID *)(UINTN)PcdGet64 (PcdDeviceTreeInitialBaseAddress);
  ASSERT (Base != NULL);

  Status = GetRtcAddress (Base, &RtcBaseAddress);
  if (RETURN_ERROR (Status)) {
    return;
  }

  SaveRtcRegisterAddressHob (RtcBaseAddress);

  FdtSize  = fdt_totalsize (Base) + PcdGet32 (PcdDeviceTreeAllocationPadding);
  FdtPages = EFI_SIZE_TO_PAGES (FdtSize);
  NewBase  = AllocatePages (FdtPages);
  ASSERT (NewBase != NULL);
  fdt_open_into (Base, NewBase, EFI_PAGES_TO_SIZE (FdtPages));

  FdtHobData = BuildGuidHob (&gFdtHobGuid, sizeof *FdtHobData);
  ASSERT (FdtHobData != NULL);
  *FdtHobData = (UINTN)NewBase;
}

/**
  Fetch the size of system memory from QEMU.

  @param  VOID

  @return  VOID
**/
STATIC
VOID
ReportSystemMemorySize (
  VOID
  )
{
  UINT64  RamSize;

  QemuFwCfgSelectItem (QemuFwCfgItemRamSize);
  RamSize = QemuFwCfgRead64 ();
  DEBUG ((
    DEBUG_INFO,
    "%a: QEMU reports %dM system memory\n",
    __func__,
    RamSize/1024/1024
    ));
}

/**
  Perform Platform PEI initialization.

  @param  FileHandle      Handle of the file being invoked.
  @param  PeiServices     Describes the list of possible PEI Services.

  @return EFI_SUCCESS     The PEIM initialized successfully.
**/
EFI_STATUS
EFIAPI
InitializePlatform (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS             Status;
  EFI_MEMORY_DESCRIPTOR  *MemoryTable;

  DEBUG ((DEBUG_INFO, "Platform PEIM Loaded\n"));

  Status = PeiServicesSetBootMode (mBootMode);
  ASSERT_EFI_ERROR (Status);

  Status = PeiServicesInstallPpi (&mPpiListBootMode);
  ASSERT_EFI_ERROR (Status);

  ReportSystemMemorySize ();

  PublishPeiMemory ();

  PeiFvInitialization ();
  InitializeRamRegions ();
  MemMapInitialization ();

  Status = PlatformHookSerialPortInitialize ();
  ASSERT_EFI_ERROR (Status);

  MiscInitialization ();

  AddFdtHob ();

  //
  // Initialization MMU
  //
  GetMemoryMapPolicy (&MemoryTable);
  Status = ConfigureMemoryManagementUnit (MemoryTable);
  ASSERT_EFI_ERROR (Status);

  MpInitLibInitialize ();

  return EFI_SUCCESS;
}
