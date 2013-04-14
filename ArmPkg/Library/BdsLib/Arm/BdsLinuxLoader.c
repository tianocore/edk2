/** @file
*
*  Copyright (c) 2011-2012, ARM Limited. All rights reserved.
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

#include "BdsInternal.h"
#include "BdsLinuxLoader.h"

#define ALIGN32_BELOW(addr)   ALIGN_POINTER(addr - 32,32)

STATIC
EFI_STATUS
PreparePlatformHardware (
  VOID
  )
{
  //Note: Interrupts will be disabled by the GIC driver when ExitBootServices() will be called.

  // Clean, invalidate, disable data cache
  ArmDisableDataCache();
  ArmCleanInvalidateDataCache();

  // Invalidate and disable the Instruction cache
  ArmDisableInstructionCache ();
  ArmInvalidateInstructionCache ();

  // Turn off MMU
  ArmDisableMmu();

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
StartLinux (
  IN  EFI_PHYSICAL_ADDRESS  LinuxImage,
  IN  UINTN                 LinuxImageSize,
  IN  EFI_PHYSICAL_ADDRESS  KernelParamsAddress,
  IN  UINTN                 KernelParamsSize,
  IN  UINT32                MachineType
  )
{
  EFI_STATUS            Status;
  LINUX_KERNEL          LinuxKernel;

  // Shut down UEFI boot services. ExitBootServices() will notify every driver that created an event on
  // ExitBootServices event. Example the Interrupt DXE driver will disable the interrupts on this event.
  Status = ShutdownUefiBootServices ();
  if(EFI_ERROR(Status)) {
    DEBUG((EFI_D_ERROR,"ERROR: Can not shutdown UEFI boot services. Status=0x%X\n", Status));
    goto Exit;
  }

  // Move the kernel parameters to any address inside the first 1MB.
  // This is necessary because the ARM Linux kernel requires
  // the FTD / ATAG List to reside entirely inside the first 1MB of
  // physical memory.
  //Note: There is no requirement on the alignment
  if (MachineType != ARM_FDT_MACHINE_TYPE) {
    if (((UINTN)KernelParamsAddress > LINUX_ATAG_MAX_OFFSET) && (KernelParamsSize < PcdGet32(PcdArmLinuxAtagMaxOffset))) {
      KernelParamsAddress = (EFI_PHYSICAL_ADDRESS)(UINTN)CopyMem (ALIGN32_BELOW(LINUX_ATAG_MAX_OFFSET - KernelParamsSize), (VOID*)(UINTN)KernelParamsAddress, KernelParamsSize);
    }
  } else {
    if (((UINTN)KernelParamsAddress > LINUX_FDT_MAX_OFFSET) && (KernelParamsSize < PcdGet32(PcdArmLinuxFdtMaxOffset))) {
      KernelParamsAddress = (EFI_PHYSICAL_ADDRESS)(UINTN)CopyMem (ALIGN32_BELOW(LINUX_FDT_MAX_OFFSET - KernelParamsSize), (VOID*)(UINTN)KernelParamsAddress, KernelParamsSize);
    }
  }

  if ((UINTN)LinuxImage > LINUX_KERNEL_MAX_OFFSET) {
    //Note: There is no requirement on the alignment
    LinuxKernel = (LINUX_KERNEL)CopyMem (ALIGN32_BELOW(LINUX_KERNEL_MAX_OFFSET - LinuxImageSize), (VOID*)(UINTN)LinuxImage, LinuxImageSize);
  } else {
    LinuxKernel = (LINUX_KERNEL)(UINTN)LinuxImage;
  }

  // Check if the Linux Image is a uImage
  if (*(UINT32*)LinuxKernel == LINUX_UIMAGE_SIGNATURE) {
    // Assume the Image Entry Point is just after the uImage header (64-byte size)
    LinuxKernel = (LINUX_KERNEL)((UINTN)LinuxKernel + 64);
    LinuxImageSize -= 64;
  }

  //TODO: Check there is no overlapping between kernel and Atag

  //
  // Switch off interrupts, caches, mmu, etc
  //
  Status = PreparePlatformHardware ();
  ASSERT_EFI_ERROR(Status);

  // Register and print out performance information
  PERF_END (NULL, "BDS", NULL, 0);
  if (PerformanceMeasurementEnabled ()) {
    PrintPerformance ();
  }

  //
  // Start the Linux Kernel
  //

  // Outside BootServices, so can't use Print();
  DEBUG((EFI_D_ERROR, "\nStarting the kernel:\n\n"));

  // Jump to kernel with register set
  LinuxKernel ((UINTN)0, MachineType, (UINTN)KernelParamsAddress);

  // Kernel should never exit
  // After Life services are not provided
  ASSERT(FALSE);

Exit:
  // Only be here if we fail to start Linux
  Print (L"ERROR  : Can not start the kernel. Status=0x%X\n", Status);

  // Free Runtimee Memory (kernel and FDT)
  return Status;
}

/**
  Start a Linux kernel from a Device Path

  @param  LinuxKernel           Device Path to the Linux Kernel
  @param  Parameters            Linux kernel arguments
  @param  Fdt                   Device Path to the Flat Device Tree

  @retval EFI_SUCCESS           All drivers have been connected
  @retval EFI_NOT_FOUND         The Linux kernel Device Path has not been found
  @retval EFI_OUT_OF_RESOURCES  There is not enough resource memory to store the matching results.

**/
EFI_STATUS
BdsBootLinuxAtag (
  IN  EFI_DEVICE_PATH_PROTOCOL* LinuxKernelDevicePath,
  IN  EFI_DEVICE_PATH_PROTOCOL* InitrdDevicePath,
  IN  CONST CHAR8*              CommandLineArguments
  )
{
  EFI_STATUS            Status;
  UINT32                LinuxImageSize;
  UINT32                InitrdImageBaseSize = 0;
  UINT32                InitrdImageSize = 0;
  UINT32                AtagSize;
  EFI_PHYSICAL_ADDRESS  AtagBase;
  EFI_PHYSICAL_ADDRESS  LinuxImage;
  EFI_PHYSICAL_ADDRESS  InitrdImageBase = 0;
  EFI_PHYSICAL_ADDRESS  InitrdImage = 0;

  PERF_START (NULL, "BDS", NULL, 0);

  // Load the Linux kernel from a device path
  LinuxImage = LINUX_KERNEL_MAX_OFFSET;
  Status = BdsLoadImage (LinuxKernelDevicePath, AllocateMaxAddress, &LinuxImage, &LinuxImageSize);
  if (EFI_ERROR(Status)) {
    Print (L"ERROR: Did not find Linux kernel.\n");
    return Status;
  }

  if (InitrdDevicePath) {
    // Load the initrd near to the Linux kernel
    InitrdImageBase = LINUX_KERNEL_MAX_OFFSET;
    Status = BdsLoadImage (InitrdDevicePath, AllocateMaxAddress, &InitrdImageBase, &InitrdImageBaseSize);
    if (Status == EFI_OUT_OF_RESOURCES) {
      Status = BdsLoadImage (InitrdDevicePath, AllocateAnyPages, &InitrdImageBase, &InitrdImageBaseSize);
    }
    if (EFI_ERROR(Status)) {
      Print (L"ERROR: Did not find initrd image.\n");
      goto EXIT_FREE_LINUX;
    }
    
    // Check if the initrd is a uInitrd
    if (*(UINT32*)((UINTN)InitrdImageBase) == LINUX_UIMAGE_SIGNATURE) {
      // Skip the 64-byte image header
      InitrdImage = (EFI_PHYSICAL_ADDRESS)((UINTN)InitrdImageBase + 64);
      InitrdImageSize = InitrdImageBaseSize - 64;
    } else {
      InitrdImage = InitrdImageBase;
      InitrdImageSize = InitrdImageBaseSize;
    }
  }

  //
  // Setup the Linux Kernel Parameters
  //
 
  // By setting address=0 we leave the memory allocation to the function
  Status = PrepareAtagList (CommandLineArguments, InitrdImage, InitrdImageSize, &AtagBase, &AtagSize);
  if (EFI_ERROR(Status)) {
    Print(L"ERROR: Can not prepare ATAG list. Status=0x%X\n", Status);
    goto EXIT_FREE_INITRD;
  }

  return StartLinux (LinuxImage, LinuxImageSize, AtagBase, AtagSize, PcdGet32(PcdArmMachineType));

EXIT_FREE_INITRD:
  if (InitrdDevicePath) {
    gBS->FreePages (InitrdImageBase, EFI_SIZE_TO_PAGES (InitrdImageBaseSize));
  }

EXIT_FREE_LINUX:
  gBS->FreePages (LinuxImage, EFI_SIZE_TO_PAGES (LinuxImageSize));

  return Status;
}

/**
  Start a Linux kernel from a Device Path

  @param  LinuxKernel           Device Path to the Linux Kernel
  @param  Parameters            Linux kernel arguments
  @param  Fdt                   Device Path to the Flat Device Tree

  @retval EFI_SUCCESS           All drivers have been connected
  @retval EFI_NOT_FOUND         The Linux kernel Device Path has not been found
  @retval EFI_OUT_OF_RESOURCES  There is not enough resource memory to store the matching results.

**/
EFI_STATUS
BdsBootLinuxFdt (
  IN  EFI_DEVICE_PATH_PROTOCOL* LinuxKernelDevicePath,
  IN  EFI_DEVICE_PATH_PROTOCOL* InitrdDevicePath,
  IN  CONST CHAR8*              CommandLineArguments,
  IN  EFI_DEVICE_PATH_PROTOCOL* FdtDevicePath
  )
{
  EFI_STATUS            Status;
  UINT32                LinuxImageSize;
  UINT32                InitrdImageBaseSize = 0;
  UINT32                InitrdImageSize = 0;
  UINT32                FdtBlobSize;
  EFI_PHYSICAL_ADDRESS  FdtBlobBase;
  EFI_PHYSICAL_ADDRESS  LinuxImage;
  EFI_PHYSICAL_ADDRESS  InitrdImageBase = 0;
  EFI_PHYSICAL_ADDRESS  InitrdImage = 0;

  PERF_START (NULL, "BDS", NULL, 0);

  // Load the Linux kernel from a device path
  LinuxImage = LINUX_KERNEL_MAX_OFFSET;
  Status = BdsLoadImage (LinuxKernelDevicePath, AllocateMaxAddress, &LinuxImage, &LinuxImageSize);
  if (EFI_ERROR(Status)) {
    Print (L"ERROR: Did not find Linux kernel.\n");
    return Status;
  }

  if (InitrdDevicePath) {
    InitrdImageBase = LINUX_KERNEL_MAX_OFFSET;
    Status = BdsLoadImage (InitrdDevicePath, AllocateMaxAddress, &InitrdImageBase, &InitrdImageBaseSize);
    if (Status == EFI_OUT_OF_RESOURCES) {
      Status = BdsLoadImage (InitrdDevicePath, AllocateAnyPages, &InitrdImageBase, &InitrdImageBaseSize);
    }
    if (EFI_ERROR(Status)) {
      Print (L"ERROR: Did not find initrd image.\n");
      goto EXIT_FREE_LINUX;
    }

    // Check if the initrd is a uInitrd
    if (*(UINT32*)((UINTN)InitrdImageBase) == LINUX_UIMAGE_SIGNATURE) {
      // Skip the 64-byte image header
      InitrdImage = (EFI_PHYSICAL_ADDRESS)((UINTN)InitrdImageBase + 64);
      InitrdImageSize = InitrdImageBaseSize - 64;
    } else {
      InitrdImage = InitrdImageBase;
      InitrdImageSize = InitrdImageBaseSize;
    }
  }

  // Load the FDT binary from a device path. The FDT will be reloaded later to a more appropriate location for the Linux kernel.
  FdtBlobBase = 0;
  Status = BdsLoadImage (FdtDevicePath, AllocateAnyPages, &FdtBlobBase, &FdtBlobSize);
  if (EFI_ERROR(Status)) {
    Print (L"ERROR: Did not find Device Tree blob.\n");
    goto EXIT_FREE_INITRD;
  }

  // Update the Fdt with the Initrd information. The FDT will increase in size.
  // By setting address=0 we leave the memory allocation to the function
  Status = PrepareFdt (CommandLineArguments, InitrdImage, InitrdImageSize, &FdtBlobBase, &FdtBlobSize);
  if (EFI_ERROR(Status)) {
    Print(L"ERROR: Can not load kernel with FDT. Status=%r\n", Status);
    goto EXIT_FREE_FDT;
  }

  return StartLinux (LinuxImage, LinuxImageSize, FdtBlobBase, FdtBlobSize, ARM_FDT_MACHINE_TYPE);

EXIT_FREE_FDT:
  gBS->FreePages (FdtBlobBase, EFI_SIZE_TO_PAGES (FdtBlobSize));

EXIT_FREE_INITRD:
  if (InitrdDevicePath) {
    gBS->FreePages (InitrdImageBase, EFI_SIZE_TO_PAGES (InitrdImageBaseSize));
  }

EXIT_FREE_LINUX:
  gBS->FreePages (LinuxImage, EFI_SIZE_TO_PAGES (LinuxImageSize));

  return Status;
}

