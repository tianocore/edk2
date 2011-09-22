/** @file
*
*  Copyright (c) 2011, ARM Limited. All rights reserved.
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

// Point to the current ATAG
STATIC LINUX_ATAG *mLinuxKernelCurrentAtag;

STATIC
VOID
SetupCoreTag (
  IN UINT32 PageSize
  )
{
  mLinuxKernelCurrentAtag->header.size = tag_size(LINUX_ATAG_CORE);
  mLinuxKernelCurrentAtag->header.type = ATAG_CORE;

  mLinuxKernelCurrentAtag->body.core_tag.flags    = 1;            /* ensure read-only */
  mLinuxKernelCurrentAtag->body.core_tag.pagesize = PageSize;     /* systems PageSize (4k) */
  mLinuxKernelCurrentAtag->body.core_tag.rootdev  = 0;            /* zero root device (typically overridden from kernel command line )*/

  // move pointer to next tag
  mLinuxKernelCurrentAtag = next_tag_address(mLinuxKernelCurrentAtag);
}

STATIC
VOID
SetupMemTag (
  IN UINTN StartAddress,
  IN UINT32 Size
  )
{
  mLinuxKernelCurrentAtag->header.size = tag_size(LINUX_ATAG_MEM);
  mLinuxKernelCurrentAtag->header.type = ATAG_MEM;

  mLinuxKernelCurrentAtag->body.mem_tag.start = StartAddress;    /* Start of memory chunk for AtagMem */
  mLinuxKernelCurrentAtag->body.mem_tag.size  = Size;             /* Size of memory chunk for AtagMem */

  // move pointer to next tag
  mLinuxKernelCurrentAtag = next_tag_address(mLinuxKernelCurrentAtag);
}

STATIC
VOID
SetupCmdlineTag (
  IN CONST CHAR8 *CmdLine
  )
{
  UINT32 LineLength;

  // Increment the line length by 1 to account for the null string terminator character
  LineLength = AsciiStrLen(CmdLine) + 1;

  /* Check for NULL strings.
   * Do not insert a tag for an empty CommandLine, don't even modify the tag address pointer.
   * Remember, you have at least one null string terminator character.
   */
  if(LineLength > 1) {
    mLinuxKernelCurrentAtag->header.size = ((UINT32)sizeof(LINUX_ATAG_HEADER) + LineLength + (UINT32)3) >> 2;
    mLinuxKernelCurrentAtag->header.type = ATAG_CMDLINE;

    /* place CommandLine into tag */
    AsciiStrCpy(mLinuxKernelCurrentAtag->body.cmdline_tag.cmdline, CmdLine);

    // move pointer to next tag
    mLinuxKernelCurrentAtag = next_tag_address(mLinuxKernelCurrentAtag);
  }
}

STATIC
VOID
SetupEndTag (
  VOID
  )
{
  // Empty tag ends list; this has zero length and no body
  mLinuxKernelCurrentAtag->header.type = ATAG_NONE;
  mLinuxKernelCurrentAtag->header.size = 0;

  /* We can not calculate the next address by using the standard macro:
   * Params = next_tag_address(Params);
   * because it relies on the header.size, which here it is 0 (zero).
   * The easiest way is to add the sizeof(mLinuxKernelCurrentAtag->header).
   */
  mLinuxKernelCurrentAtag = (LINUX_ATAG*)((UINT32)mLinuxKernelCurrentAtag + sizeof(mLinuxKernelCurrentAtag->header));
}

STATIC
EFI_STATUS
PrepareAtagList (
  IN  CONST CHAR8*     CommandLineString,
  IN  EFI_PHYSICAL_ADDRESS InitrdImage,
  IN  UINTN            InitrdImageSize,
  OUT LINUX_ATAG       **AtagBase,
  OUT UINT32           *AtagSize
  )
{
  EFI_STATUS                  Status;
  LIST_ENTRY                  *ResourceLink;
  LIST_ENTRY                  ResourceList;
  EFI_PHYSICAL_ADDRESS        AtagStartAddress;
  BDS_SYSTEM_MEMORY_RESOURCE  *Resource;

  AtagStartAddress = LINUX_ATAG_MAX_OFFSET;
  Status = gBS->AllocatePages (AllocateMaxAddress, EfiBootServicesData, EFI_SIZE_TO_PAGES(ATAG_MAX_SIZE), &AtagStartAddress);
  if (EFI_ERROR(Status)) {
    DEBUG ((EFI_D_ERROR,"Failed to allocate Atag at 0x%lX (%r)\n",AtagStartAddress,Status));
    Status = gBS->AllocatePages (AllocateAnyPages, EfiBootServicesData, EFI_SIZE_TO_PAGES(ATAG_MAX_SIZE), &AtagStartAddress);
    ASSERT_EFI_ERROR(Status);
  }

  // Ready to setup the atag list
  mLinuxKernelCurrentAtag = (LINUX_ATAG*)(UINTN)AtagStartAddress;

  // Standard core tag 4k PageSize
  SetupCoreTag( (UINT32)SIZE_4KB );

  // Physical memory setup
  GetSystemMemoryResources (&ResourceList);
  ResourceLink = ResourceList.ForwardLink;
  while (ResourceLink != NULL && ResourceLink != &ResourceList) {
    Resource = (BDS_SYSTEM_MEMORY_RESOURCE*)ResourceLink;
    DEBUG((EFI_D_INFO,"- [0x%08X,0x%08X]\n",(UINT32)Resource->PhysicalStart,(UINT32)Resource->PhysicalStart+(UINT32)Resource->ResourceLength));
    SetupMemTag( (UINT32)Resource->PhysicalStart, (UINT32)Resource->ResourceLength );
    ResourceLink = ResourceLink->ForwardLink;
  }

  // CommandLine setting root device
  SetupCmdlineTag (CommandLineString);

  if (InitrdImageSize > 0 && InitrdImage != 0) {
    mLinuxKernelCurrentAtag->header.size = tag_size(LINUX_ATAG_INITRD2);
    mLinuxKernelCurrentAtag->header.type = ATAG_INITRD2;

    mLinuxKernelCurrentAtag->body.initrd2_tag.start = (UINT32)InitrdImage;
    mLinuxKernelCurrentAtag->body.initrd2_tag.size = (UINT32)InitrdImageSize;

    // Move pointer to next tag
    mLinuxKernelCurrentAtag = next_tag_address(mLinuxKernelCurrentAtag);
  }

  // end of tags
  SetupEndTag();

  // Calculate atag list size
  *AtagBase = (LINUX_ATAG*)(UINTN)AtagStartAddress;
  *AtagSize = (UINT32)mLinuxKernelCurrentAtag - (UINT32)AtagStartAddress + 1;

  return EFI_SUCCESS;
}

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

  // jump to kernel with register set
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
    Status = BdsLoadImage (InitrdDevicePath, AllocateAnyPages, &InitrdImage, &InitrdImageSize);
    if (EFI_ERROR(Status)) {
      Print (L"ERROR: Did not find initrd image.\n");
      return Status;
    }
  }

  //
  // Setup the Linux Kernel Parameters
  //
 
  // By setting address=0 we leave the memory allocation to the function
  Status = PrepareAtagList (Arguments, InitrdImage, InitrdImageSize, (LINUX_ATAG**)&KernelParamsAddress, &KernelParamsSize);
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
    Status = BdsLoadImage (InitrdDevicePath, AllocateAnyPages, &InitrdImage, &InitrdImageSize);
    if (EFI_ERROR(Status)) {
      Print (L"ERROR: Did not find initrd image.\n");
      return Status;
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

