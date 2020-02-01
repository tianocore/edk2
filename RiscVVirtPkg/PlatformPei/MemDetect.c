/**@file
  Memory Detection for Virtual Machines.

  Copyright (c) 2016, Hewlett Packard Enterprise Development LP. All rights reserved.<BR>
  Copyright (c) 2006 - 2014, Intel Corporation. All rights reserved.<BR>
  
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  MemDetect.c

**/

//
// The package level header files this module uses
//
#include <PiPei.h>

//
// The Library classes this module consumes
//
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/ResourcePublicationLib.h>

#include "Platform.h"
#include "Cmos.h"

UINT8 mPhysMemAddressWidth = 32;

UINT32
GetSystemMemorySizeBelow4gb (
  VOID
  )
{
  UINT8 Cmos0x34;
  UINT8 Cmos0x35;

  //
  // CMOS 0x34/0x35 specifies the system memory above 16 MB.
  // * CMOS(0x35) is the high byte
  // * CMOS(0x34) is the low byte
  // * The size is specified in 64kb chunks
  // * Since this is memory above 16MB, the 16MB must be added
  //   into the calculation to get the total memory size.
  //

  Cmos0x34 = (UINT8) CmosRead8 (0x34);
  Cmos0x35 = (UINT8) CmosRead8 (0x35);

  return (UINT32) (((UINTN)((Cmos0x35 << 8) + Cmos0x34) << 16) + SIZE_16MB);
}


STATIC
UINT64
GetSystemMemorySizeAbove4gb (
  )
{
  UINT32 Size;
  UINTN  CmosIndex;

  //
  // CMOS 0x5b-0x5d specifies the system memory above 4GB MB.
  // * CMOS(0x5d) is the most significant size byte
  // * CMOS(0x5c) is the middle size byte
  // * CMOS(0x5b) is the least significant size byte
  // * The size is specified in 64kb chunks
  //

  Size = 0;
  for (CmosIndex = 0x5d; CmosIndex >= 0x5b; CmosIndex--) {
    Size = (UINT32) (Size << 8) + (UINT32) CmosRead8 (CmosIndex);
  }

  return LShiftU64 (Size, 16);
}


/**
  Initialize the mPhysMemAddressWidth variable, based on guest RAM size.
**/
VOID
AddressWidthInitialization (
  VOID
  )
{
}


/**
  Calculate the cap for the permanent PEI memory.
**/
STATIC
UINT32
GetPeiMemoryCap (
  VOID
  )
{
  return SIZE_64MB;
}


/**
  Publish PEI core memory

  @return EFI_SUCCESS     The PEIM initialized successfully.

**/
EFI_STATUS
PublishPeiMemory (
  VOID
  )
{
  EFI_STATUS                  Status;
  EFI_PHYSICAL_ADDRESS        MemoryBase;
  UINT64                      MemorySize;
  UINT64                      LowerMemorySize;
  UINT32                      PeiMemoryCap;

  if (mBootMode == BOOT_ON_S3_RESUME) {
    MemoryBase = PcdGet32 (PcdS3AcpiReservedMemoryBase);
    MemorySize = PcdGet32 (PcdS3AcpiReservedMemorySize);
  } else {
    LowerMemorySize = GetSystemMemorySizeBelow4gb ();

    PeiMemoryCap = GetPeiMemoryCap ();
    DEBUG ((EFI_D_INFO, "%a: mPhysMemAddressWidth=%d PeiMemoryCap=%u KB Lower memory size=%u MB\n",
      __FUNCTION__, mPhysMemAddressWidth, PeiMemoryCap >> 10, LowerMemorySize >> 20));

    //
    // Determine the range of memory to use during PEI
    //
    MemoryBase = PcdGet32 (PcdOvmfDxeMemFvBase) + PcdGet32 (PcdOvmfDxeMemFvSize);
    MemorySize = LowerMemorySize - MemoryBase;
    if (MemorySize > PeiMemoryCap) {
      MemoryBase = LowerMemorySize - PeiMemoryCap;
      MemorySize = PeiMemoryCap;
    }
  }

  //
  // Publish this memory to the PEI Core
  //
  Status = PublishSystemMemory(MemoryBase, MemorySize);
  ASSERT_EFI_ERROR (Status);

  return Status;
}


/**
  Peform Memory Detection for QEMU / KVM

**/
STATIC
VOID
QemuInitializeRam (
  VOID
  )
{
  UINT64                      LowerMemorySize;
  UINT64                      UpperMemorySize;

  DEBUG ((EFI_D_INFO, "%a called\n", __FUNCTION__));

  //
  // Determine total memory size available
  //
  LowerMemorySize = GetSystemMemorySizeBelow4gb ();
  UpperMemorySize = GetSystemMemorySizeAbove4gb ();

  if (mBootMode != BOOT_ON_S3_RESUME) {
    //
    // Create memory HOBs
    //
    AddMemoryRangeHob (BASE_64KB, BASE_512KB + BASE_128KB); // From 64K, first 64K reserved by memory map I/O.
    AddMemoryRangeHob (BASE_1MB, LowerMemorySize);
    if (UpperMemorySize != 0) {
      AddUntestedMemoryBaseSizeHob (BASE_4GB, UpperMemorySize);
    }
  }
}

/**
  Publish system RAM and reserve memory regions

**/
VOID
InitializeRamRegions (
  VOID
  )
{
  if (!mXen) {
    QemuInitializeRam ();
  } else {
    XenPublishRamRegions ();
  }

  if (mS3Supported && mBootMode != BOOT_ON_S3_RESUME) {
    //
    // This is the memory range that will be used for PEI on S3 resume
    //
    BuildMemoryAllocationHob (
      (EFI_PHYSICAL_ADDRESS)(UINTN) PcdGet32 (PcdS3AcpiReservedMemoryBase),
      (UINT64)(UINTN) PcdGet32 (PcdS3AcpiReservedMemorySize),
      EfiACPIMemoryNVS
      );

    //
    // Cover the initial RAM area used as stack and temporary PEI heap.
    //
    // This is reserved as ACPI NVS so it can be used on S3 resume.
    //
    BuildMemoryAllocationHob (
      PcdGet32 (PcdRiscVSecPeiTempRamBase),
      PcdGet32 (PcdRiscVSecPeiTempRamSize),
      EfiACPIMemoryNVS
      );

    //
    // SEC stores its table of GUIDed section handlers here.
    //
    BuildMemoryAllocationHob (
      PcdGet64 (PcdGuidedExtractHandlerTableAddress),
      PcdGet32 (PcdGuidedExtractHandlerTableSize),
      EfiACPIMemoryNVS
      );
  }

  if (mBootMode != BOOT_ON_S3_RESUME) {
    //
    // Reserve the lock box storage area
    //
    // Since this memory range will be used on S3 resume, it must be
    // reserved as ACPI NVS.
    //
    // If S3 is unsupported, then various drivers might still write to the
    // LockBox area. We ought to prevent DXE from serving allocation requests
    // such that they would overlap the LockBox storage.
    //
    ZeroMem (
      (VOID*)(UINTN) PcdGet32 (PcdOvmfLockBoxStorageBase),
      (UINTN) PcdGet32 (PcdOvmfLockBoxStorageSize)
      );
    BuildMemoryAllocationHob (
      (EFI_PHYSICAL_ADDRESS)(UINTN) PcdGet32 (PcdOvmfLockBoxStorageBase),
      (UINT64)(UINTN) PcdGet32 (PcdOvmfLockBoxStorageSize),
      mS3Supported ? EfiACPIMemoryNVS : EfiBootServicesData
      );
  }
}
