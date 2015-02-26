/** @file
*
*  Copyright (c) 2011-2014, ARM Limited. All rights reserved.
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
#include <Library/ArmGicLib.h>
#include <Ppi/ArmMpCoreInfo.h>
#include <Library/IoLib.h>
#include <Guid/Fdt.h>
#include <libfdt.h>

#include "BdsInternal.h"
#include "BdsLinuxLoader.h"

/*
  Linux kernel booting: Look at the doc in the Kernel source :
    Documentation/arm64/booting.txt
  The kernel image must be placed at the start of the memory to be used by the
  kernel (2MB aligned) + 0x80000.

  The Device tree blob is expected to be under 2MB and be within the first 512MB
  of kernel memory and be 2MB aligned.

  A Flattened Device Tree (FDT) used to boot linux needs to be updated before
  the kernel is started. It needs to indicate how secondary cores are brought up
  and where they are waiting before loading Linux. The FDT also needs to hold
  the correct kernel command line and filesystem RAM-disk information.
  At the moment we do not fully support generating this FDT information at
  runtime. A prepared FDT should be provided at boot. FDT is the only supported
  method for booting the AArch64 Linux kernel.

  Linux does not use any runtime services at this time, so we can let it
  overwrite UEFI.
*/


#define LINUX_ALIGN_VAL       (0x080000) // 2MB + 0x80000 mask
#define LINUX_ALIGN_MASK      (0x1FFFFF) // Bottom 21bits
#define ALIGN_2MB(addr)       ALIGN_POINTER(addr , (2*1024*1024))

/* ARM32 and AArch64 kernel handover differ.
 * x0 is set to FDT base.
 * x1-x3 are reserved for future use and should be set to zero.
 */
typedef VOID (*LINUX_KERNEL64)(UINTN ParametersBase, UINTN Reserved0,
                               UINTN Reserved1, UINTN Reserved2);

/* These externs are used to relocate some ASM code into Linux memory. */
extern VOID  *SecondariesPenStart;
extern VOID  *SecondariesPenEnd;
extern UINTN *AsmMailboxbase;


STATIC
EFI_STATUS
PreparePlatformHardware (
  VOID
  )
{
  //Note: Interrupts will be disabled by the GIC driver when ExitBootServices() will be called.

  // Clean before Disable else the Stack gets corrupted with old data.
  ArmCleanDataCache ();
  ArmDisableDataCache ();
  // Invalidate all the entries that might have snuck in.
  ArmInvalidateDataCache ();

  // Disable and invalidate the instruction cache
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
  IN  EFI_PHYSICAL_ADDRESS  FdtBlobBase,
  IN  UINTN                 FdtBlobSize
  )
{
  EFI_STATUS            Status;
  LINUX_KERNEL64        LinuxKernel = (LINUX_KERNEL64)LinuxImage;

  // Send msg to secondary cores to go to the kernel pen.
  ArmGicSendSgiTo (PcdGet32(PcdGicDistributorBase), ARM_GIC_ICDSGIR_FILTER_EVERYONEELSE, 0x0E, PcdGet32 (PcdGicSgiIntId));

  // Shut down UEFI boot services. ExitBootServices() will notify every driver that created an event on
  // ExitBootServices event. Example the Interrupt DXE driver will disable the interrupts on this event.
  Status = ShutdownUefiBootServices ();
  if(EFI_ERROR(Status)) {
    DEBUG((EFI_D_ERROR,"ERROR: Can not shutdown UEFI boot services. Status=0x%X\n", Status));
    goto Exit;
  }

  // Check if the Linux Image is a uImage
  if (*(UINTN*)LinuxKernel == LINUX_UIMAGE_SIGNATURE) {
    // Assume the Image Entry Point is just after the uImage header (64-byte size)
    LinuxKernel = (LINUX_KERNEL64)((UINTN)LinuxKernel + 64);
    LinuxImageSize -= 64;
  }

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

  // x1-x3 are reserved (set to zero) for future use.
  LinuxKernel ((UINTN)FdtBlobBase, 0, 0, 0);

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
  // NOTE : AArch64 Linux kernel does not support ATAG, FDT only.
  ASSERT(0);

  return RETURN_UNSUPPORTED;
}

/**
  Start a Linux kernel from a Device Path

  @param[in]  LinuxKernelDevicePath  Device Path to the Linux Kernel
  @param[in]  InitrdDevicePath       Device Path to the Initrd
  @param[in]  Arguments              Linux kernel arguments

  @retval EFI_SUCCESS           All drivers have been connected
  @retval EFI_NOT_FOUND         The Linux kernel Device Path has not been found
  @retval EFI_OUT_OF_RESOURCES  There is not enough resource memory to store the matching results.

**/
EFI_STATUS
BdsBootLinuxFdt (
  IN  EFI_DEVICE_PATH_PROTOCOL* LinuxKernelDevicePath,
  IN  EFI_DEVICE_PATH_PROTOCOL* InitrdDevicePath,
  IN  CONST CHAR8*              Arguments
  )
{
  EFI_STATUS               Status;
  EFI_STATUS               PenBaseStatus;
  UINTN                    LinuxImageSize;
  UINTN                    InitrdImageSize;
  UINTN                    InitrdImageBaseSize;
  VOID                     *InstalledFdtBase;
  UINTN                    FdtBlobSize;
  EFI_PHYSICAL_ADDRESS     FdtBlobBase;
  EFI_PHYSICAL_ADDRESS     LinuxImage;
  EFI_PHYSICAL_ADDRESS     InitrdImage;
  EFI_PHYSICAL_ADDRESS     InitrdImageBase;
  ARM_PROCESSOR_TABLE      *ArmProcessorTable;
  ARM_CORE_INFO            *ArmCoreInfoTable;
  UINTN                    Index;
  EFI_PHYSICAL_ADDRESS     PenBase;
  UINTN                    PenSize;
  UINTN                    MailBoxBase;

  PenBaseStatus = EFI_UNSUPPORTED;
  PenSize = 0;
  InitrdImage = 0;
  InitrdImageSize = 0;
  InitrdImageBase = 0;
  InitrdImageBaseSize = 0;

  PERF_START (NULL, "BDS", NULL, 0);

  //
  // Load the Linux kernel from a device path
  //

  // Try to put the kernel at the start of RAM so as to give it access to all memory.
  // If that fails fall back to try loading it within LINUX_KERNEL_MAX_OFFSET of memory start.
  LinuxImage = PcdGet64 (PcdSystemMemoryBase) + 0x80000;
  Status = BdsLoadImage (LinuxKernelDevicePath, AllocateAddress, &LinuxImage, &LinuxImageSize);
  if (EFI_ERROR(Status)) {
    // Try again but give the loader more freedom of where to put the image.
    LinuxImage = LINUX_KERNEL_MAX_OFFSET;
    Status = BdsLoadImage (LinuxKernelDevicePath, AllocateMaxAddress, &LinuxImage, &LinuxImageSize);
    if (EFI_ERROR(Status)) {
      Print (L"ERROR: Did not find Linux kernel (%r).\n", Status);
      return Status;
    }
  }
  // Adjust the kernel location slightly if required. The kernel needs to be placed at start
  //  of memory (2MB aligned) + 0x80000.
  if ((LinuxImage & LINUX_ALIGN_MASK) != LINUX_ALIGN_VAL) {
    LinuxImage = (EFI_PHYSICAL_ADDRESS)CopyMem (ALIGN_2MB(LinuxImage) + 0x80000, (VOID*)(UINTN)LinuxImage, LinuxImageSize);
  }

  if (InitrdDevicePath) {
    InitrdImageBase = LINUX_KERNEL_MAX_OFFSET;
    Status = BdsLoadImage (InitrdDevicePath, AllocateMaxAddress, &InitrdImageBase, &InitrdImageBaseSize);
    if (Status == EFI_OUT_OF_RESOURCES) {
      Status = BdsLoadImage (InitrdDevicePath, AllocateAnyPages, &InitrdImageBase, &InitrdImageBaseSize);
    }
    if (EFI_ERROR (Status)) {
      Print (L"ERROR: Did not find initrd image (%r).\n", Status);
      goto EXIT_FREE_LINUX;
    }

    // Check if the initrd is a uInitrd
    if (*(UINTN*)((UINTN)InitrdImageBase) == LINUX_UIMAGE_SIGNATURE) {
      // Skip the 64-byte image header
      InitrdImage = (EFI_PHYSICAL_ADDRESS)((UINTN)InitrdImageBase + 64);
      InitrdImageSize = InitrdImageBaseSize - 64;
    } else {
      InitrdImage = InitrdImageBase;
      InitrdImageSize = InitrdImageBaseSize;
    }
  }

  //
  // Get the FDT from the Configuration Table.
  // The FDT will be reloaded in PrepareFdt() to a more appropriate
  // location for the Linux Kernel.
  //
  Status = EfiGetSystemConfigurationTable (&gFdtTableGuid, &InstalledFdtBase);
  if (EFI_ERROR (Status)) {
    Print (L"ERROR: Did not get the Device Tree blob (%r).\n", Status);
    goto EXIT_FREE_INITRD;
  }
  FdtBlobBase = (EFI_PHYSICAL_ADDRESS)InstalledFdtBase;
  FdtBlobSize = fdt_totalsize (InstalledFdtBase);

  //
  // Install secondary core pens if the Power State Coordination Interface is not supported
  //
  if (FeaturePcdGet (PcdArmLinuxSpinTable)) {
    // Place Pen at the start of Linux memory. We can then tell Linux to not use this bit of memory
    PenBase  = LinuxImage - 0x80000;
    PenSize  = (UINTN)&SecondariesPenEnd - (UINTN)&SecondariesPenStart;

    // Reserve the memory as RuntimeServices
    PenBaseStatus = gBS->AllocatePages (AllocateAddress, EfiRuntimeServicesCode, EFI_SIZE_TO_PAGES (PenSize), &PenBase);
    if (EFI_ERROR (PenBaseStatus)) {
      Print (L"Warning: Failed to reserve the memory required for the secondary cores at 0x%lX, Status = %r\n", PenBase, PenBaseStatus);
      // Even if there is a risk of memory corruption we carry on
    }

    // Put mailboxes below the pen code so we know where they are relative to code.
    MailBoxBase = (UINTN)PenBase + ((UINTN)&SecondariesPenEnd - (UINTN)&SecondariesPenStart);
    // Make sure this is 8 byte aligned.
    if (MailBoxBase % sizeof(MailBoxBase) != 0) {
      MailBoxBase += sizeof(MailBoxBase) - MailBoxBase % sizeof(MailBoxBase);
    }

    CopyMem ( (VOID*)(PenBase), (VOID*)&SecondariesPenStart, PenSize);

    // Update the MailboxBase variable used in the pen code
    *(UINTN*)(PenBase + ((UINTN)&AsmMailboxbase - (UINTN)&SecondariesPenStart)) = MailBoxBase;

    for (Index=0; Index < gST->NumberOfTableEntries; Index++) {
      // Check for correct GUID type
      if (CompareGuid (&gArmMpCoreInfoGuid, &(gST->ConfigurationTable[Index].VendorGuid))) {
        UINTN i;

        // Get them under our control. Move from depending on 32bit reg(sys_flags) and SWI
        // to 64 bit addr and WFE
        ArmProcessorTable = (ARM_PROCESSOR_TABLE *)gST->ConfigurationTable[Index].VendorTable;
        ArmCoreInfoTable = ArmProcessorTable->ArmCpus;

        for (i = 0; i < ArmProcessorTable->NumberOfEntries; i++ ) {
          // This goes into the SYSFLAGS register for the VE platform. We only have one 32bit reg to use
          MmioWrite32(ArmCoreInfoTable[i].MailboxSetAddress, (UINTN)PenBase);

          // So FDT can set the mailboxes correctly with the parser. These are 64bit Memory locations.
          ArmCoreInfoTable[i].MailboxSetAddress = (UINTN)MailBoxBase + i*sizeof(MailBoxBase);

          // Clear the mailboxes for the respective cores
          *((UINTN*)(ArmCoreInfoTable[i].MailboxSetAddress)) = 0x0;
        }
      }
    }
    // Flush caches to make sure our pen gets to mem before we free the cores.
    ArmCleanDataCache();
  }

  // By setting address=0 we leave the memory allocation to the function
  Status = PrepareFdt (Arguments, InitrdImage, InitrdImageSize, &FdtBlobBase, &FdtBlobSize);
  if (EFI_ERROR(Status)) {
    Print(L"ERROR: Can not load Linux kernel with Device Tree. Status=0x%X\n", Status);
    goto EXIT_FREE_FDT;
  }

  return StartLinux (LinuxImage, LinuxImageSize, FdtBlobBase, FdtBlobSize);

EXIT_FREE_FDT:
  if (!EFI_ERROR (PenBaseStatus)) {
    gBS->FreePages (PenBase, EFI_SIZE_TO_PAGES (PenSize));
  }

  gBS->FreePages (FdtBlobBase, EFI_SIZE_TO_PAGES (FdtBlobSize));

EXIT_FREE_INITRD:
  if (InitrdDevicePath) {
    gBS->FreePages (InitrdImageBase, EFI_SIZE_TO_PAGES (InitrdImageBaseSize));
  }

EXIT_FREE_LINUX:
  gBS->FreePages (LinuxImage, EFI_SIZE_TO_PAGES (LinuxImageSize));

  return Status;
}
