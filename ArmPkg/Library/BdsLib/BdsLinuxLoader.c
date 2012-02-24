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

#define ALIGN32_BELOW(addr)   ALIGN_POINTER(addr - 32,32)

STATIC
EFI_STATUS
PreparePlatformHardware (
  VOID
  )
{
  //Note: Interrupts will be disabled by the GIC driver when ExitBootServices() will be called.

  // Clean, invalidate, disable data cache
  ArmCleanInvalidateDataCache();
  ArmDisableDataCache();

  // Invalidate and disable the Instruction cache
  ArmInvalidateInstructionCache ();
  ArmDisableInstructionCache ();

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
  if ((UINTN)KernelParamsAddress > LINUX_ATAG_MAX_OFFSET) {
    //Note: There is no requirement on the alignment
    KernelParamsAddress = (EFI_PHYSICAL_ADDRESS)(UINTN)CopyMem (ALIGN32_BELOW(LINUX_ATAG_MAX_OFFSET - KernelParamsSize), (VOID*)(UINTN)KernelParamsAddress, KernelParamsSize);
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
  @param  Parameters            Linux kernel agruments
  @param  Fdt                   Device Path to the Flat Device Tree

  @retval EFI_SUCCESS           All drivers have been connected
  @retval EFI_NOT_FOUND         The Linux kernel Device Path has not been found
  @retval EFI_OUT_OF_RESOURCES  There is not enough resource memory to store the matching results.

**/
EFI_STATUS
BdsBootLinuxAtag (
  IN  EFI_DEVICE_PATH_PROTOCOL* LinuxKernelDevicePath,
  IN  EFI_DEVICE_PATH_PROTOCOL* InitrdDevicePath,
  IN  CONST CHAR8*              Arguments
  )
{
  EFI_STATUS            Status;
  UINT32                LinuxImageSize;
  UINT32                InitrdImageSize = 0;
  UINT32                KernelParamsSize;
  EFI_PHYSICAL_ADDRESS  KernelParamsAddress;
  EFI_PHYSICAL_ADDRESS  LinuxImage;
  EFI_PHYSICAL_ADDRESS  InitrdImage;

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
    InitrdImage = LINUX_KERNEL_MAX_OFFSET;
    Status = BdsLoadImage (InitrdDevicePath, AllocateMaxAddress, &InitrdImage, &InitrdImageSize);
    if (Status == EFI_OUT_OF_RESOURCES) {
      Status = BdsLoadImage (InitrdDevicePath, AllocateAnyPages, &InitrdImage, &InitrdImageSize);
    }
    if (EFI_ERROR(Status)) {
      Print (L"ERROR: Did not find initrd image.\n");
      return Status;
    }
    
    // Check if the initrd is a uInitrd
    if (*(UINT32*)((UINTN)InitrdImage) == LINUX_UIMAGE_SIGNATURE) {
      // Skip the 64-byte image header
      InitrdImage = (EFI_PHYSICAL_ADDRESS)((UINTN)InitrdImage + 64);
      InitrdImageSize -= 64;
    }
  }

  //
  // Setup the Linux Kernel Parameters
  //
 
  // By setting address=0 we leave the memory allocation to the function
  Status = PrepareAtagList (Arguments, InitrdImage, InitrdImageSize, &KernelParamsAddress, &KernelParamsSize);
  if (EFI_ERROR(Status)) {
    Print(L"ERROR: Can not prepare ATAG list. Status=0x%X\n", Status);
    return Status;
  }

  return StartLinux (LinuxImage, LinuxImageSize, KernelParamsAddress, KernelParamsSize, PcdGet32(PcdArmMachineType));
}

/**
  Start a Linux kernel from a Device Path

  @param  LinuxKernel           Device Path to the Linux Kernel
  @param  Parameters            Linux kernel agruments
  @param  Fdt                   Device Path to the Flat Device Tree

  @retval EFI_SUCCESS           All drivers have been connected
  @retval EFI_NOT_FOUND         The Linux kernel Device Path has not been found
  @retval EFI_OUT_OF_RESOURCES  There is not enough resource memory to store the matching results.

**/
EFI_STATUS
BdsBootLinuxFdt (
  IN  EFI_DEVICE_PATH_PROTOCOL* LinuxKernelDevicePath,
  IN  EFI_DEVICE_PATH_PROTOCOL* InitrdDevicePath,
  IN  CONST CHAR8*              Arguments,
  IN  EFI_DEVICE_PATH_PROTOCOL* FdtDevicePath
  )
{
  EFI_STATUS            Status;
  UINT32                LinuxImageSize;
  UINT32                InitrdImageSize = 0;
  UINT32                KernelParamsSize;
  EFI_PHYSICAL_ADDRESS  KernelParamsAddress;
  UINT32                FdtMachineType;
  EFI_PHYSICAL_ADDRESS  LinuxImage;
  EFI_PHYSICAL_ADDRESS  InitrdImage;

  FdtMachineType = 0xFFFFFFFF;

  PERF_START (NULL, "BDS", NULL, 0);

  // Load the Linux kernel from a device path
  LinuxImage = LINUX_KERNEL_MAX_OFFSET;
  Status = BdsLoadImage (LinuxKernelDevicePath, AllocateMaxAddress, &LinuxImage, &LinuxImageSize);
  if (EFI_ERROR(Status)) {
    Print (L"ERROR: Did not find Linux kernel.\n");
    return Status;
  }

  if (InitrdDevicePath) {
    InitrdImage = LINUX_KERNEL_MAX_OFFSET;
    Status = BdsLoadImage (InitrdDevicePath, AllocateMaxAddress, &InitrdImage, &InitrdImageSize);
    if (Status == EFI_OUT_OF_RESOURCES) {
      Status = BdsLoadImage (InitrdDevicePath, AllocateAnyPages, &InitrdImage, &InitrdImageSize);
    }
    if (EFI_ERROR(Status)) {
      Print (L"ERROR: Did not find initrd image.\n");
      return Status;
    }

    // Check if the initrd is a uInitrd
    if (*(UINT32*)((UINTN)InitrdImage) == LINUX_UIMAGE_SIGNATURE) {
      // Skip the 64-byte image header
      InitrdImage = (EFI_PHYSICAL_ADDRESS)((UINTN)InitrdImage + 64);
      InitrdImageSize -= 64;
    }
  }

  // Load the FDT binary from a device path
  KernelParamsAddress = LINUX_ATAG_MAX_OFFSET;
  Status = BdsLoadImage (FdtDevicePath, AllocateMaxAddress, &KernelParamsAddress, &KernelParamsSize);
  if (EFI_ERROR(Status)) {
    Print (L"ERROR: Did not find Device Tree blob.\n");
    return Status;
  }
  return StartLinux (LinuxImage, LinuxImageSize, KernelParamsAddress, KernelParamsSize, FdtMachineType);
}

