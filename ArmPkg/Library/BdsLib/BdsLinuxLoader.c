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
#include "BdsLinuxLoader.h"

#include <Library/PcdLib.h>
#include <Library/ArmLib.h>
#include <Library/HobLib.h>

STATIC
EFI_STATUS
GetARMLinuxMachineType (
    IN  BOOLEAN FdtSupported,
    OUT UINT32  *MachineType
) {
  if (FdtSupported)
  {
    // FDT requires that the machine type is set to the maximum 32-bit number.
    *MachineType = 0xFFFFFFFF;
  }
  else
  {
    // Non-FDT requires a specific machine type.
    // This OS Boot loader supports just one machine type,
    // but that could change in the future.
    *MachineType = PcdGet32(PcdArmMachineType);
  }

  return EFI_SUCCESS;
}

STATIC
VOID
SetupCoreTag( IN UINT32 PageSize )
{
  Params->header.size = tag_size(atag_core);
  Params->header.type = ATAG_CORE;

  Params->body.core_tag.flags    = 1;            /* ensure read-only */
  Params->body.core_tag.pagesize = PageSize;     /* systems PageSize (4k) */
  Params->body.core_tag.rootdev  = 0;            /* zero root device (typically overridden from kernel command line )*/

  Params = next_tag_address(Params);             /* move pointer to next tag */
}

STATIC
VOID
SetupMemTag( IN UINTN StartAddress, IN UINT32 Size )
{
  Params->header.size = tag_size(atag_mem);
  Params->header.type = ATAG_MEM;

  Params->body.mem_tag.start = StartAddress;    /* Start of memory chunk for AtagMem */
  Params->body.mem_tag.size  = Size;             /* Size of memory chunk for AtagMem */

  Params = next_tag_address(Params);             /* move pointer to next tag */
}

STATIC
VOID
SetupCmdlineTag( IN CONST CHAR8 *CmdLine )
{
  UINT32 LineLength;

  // Increment the line length by 1 to account for the null string terminator character
  LineLength = AsciiStrLen(CmdLine) + 1;

  /* Check for NULL strings.
   * Do not insert a tag for an empty CommandLine, don't even modify the tag address pointer.
   * Remember, you have at least one null string terminator character.
   */
  if( LineLength > 1 )
  {
    Params->header.size = ((UINT32)sizeof(struct atag_header) + LineLength + (UINT32)3) >> 2;
    Params->header.type = ATAG_CMDLINE;

    /* place CommandLine into tag */
    AsciiStrCpy(Params->body.cmdline_tag.cmdline, CmdLine);

    Params = next_tag_address(Params);             /* move pointer to next tag */
  }
}

STATIC
VOID
SetupEndTag( VOID )
{
  // Empty tag ends list; this has zero length and no body
  Params->header.type = ATAG_NONE;
  Params->header.size = 0;

  /* We can not calculate the next address by using the standard macro:
   * Params = next_tag_address(Params);
   * because it relies on the header.size, which here it is 0 (zero).
   * The easiest way is to add the sizeof(Params->header).
   */
  Params = (struct atag *)((UINT32)Params + sizeof(Params->header));
}

STATIC
EFI_STATUS
PrepareAtagList(
    IN OUT struct atag      **AtagStartAddress,
    IN     CONST CHAR8*     CommandLineString,
    OUT    UINT32           *AtagSize
) {
  LIST_ENTRY    *ResourceLink;
  LIST_ENTRY ResourceList;
  BDS_SYSTEM_MEMORY_RESOURCE  *Resource;

  // If no address supplied then this function will decide where to put it
  if( *AtagStartAddress == 0 )
  {
    /* WARNING: At the time of writing (2010-July-30) the linux kernel expects
     * the atag list it in the first 1MB of memory and preferably at address 0x100.
     * This has a very high risk of overwriting UEFI code, but as
     * the linux kernel does not expect any runtime services from uefi
     * and there is no afterlife section following the linux kernel termination,
     * it does not matter if we stamp over that memory area.
     *
     * The proposed workaround is to create the atag list somewhere in boot services memory
     * and then transfer it to address 0x100 (or to runtime services memory) immediately
     * before starting the kernel.
     * An additional benefit of this is that when we copy the ATAG list to it's final place,
     * we can trim down the memory allocation size. Before we create the list we don't know
     * how much space it is going to take, so we are over-allocating space.
     */
    *AtagStartAddress = (struct atag *) AllocatePool(ATAG_MAX_SIZE);
  }

  // Ensure the pointer is not NULL.
  ASSERT( *AtagStartAddress != (struct atag *)NULL );

  // Ready to setup the atag list
  Params = *AtagStartAddress;

  // Standard core tag 4k PageSize
  SetupCoreTag( (UINT32)SIZE_4KB );

  // Physical memory setup
  GetSystemMemoryResources(&ResourceList);
  ResourceLink = ResourceList.ForwardLink;
  while (ResourceLink != NULL && ResourceLink != &ResourceList) {
    Resource = (BDS_SYSTEM_MEMORY_RESOURCE*)ResourceList.ForwardLink;
    SetupMemTag( (UINT32)Resource->PhysicalStart, (UINT32)Resource->ResourceLength );
    ResourceLink = ResourceLink->ForwardLink;
  }

  // CommandLine setting root device
  SetupCmdlineTag( CommandLineString );

  // end of tags
  SetupEndTag();

  // Calculate atag list size
  *AtagSize = (UINT32)Params - (UINT32)*AtagStartAddress + 1;

  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
PreparePlatformHardware( VOID )
{
  //Note: Interrupts will be disabled by the GIC driver when ExitBootServices() will be called.

  // clean, invalidate, disable data cache
  ArmCleanInvalidateDataCache();
  ArmDisableDataCache();

  // Invalidate and disable the Instruction cache
  ArmInvalidateInstructionCache ();
  ArmDisableInstructionCache ();

  // turn off MMU
  ArmInvalidateTlb();
  ArmDisableMmu();

  return EFI_SUCCESS;
}

/*************************************************
 * R0, R1, R2 correspond to registers R0, R1, R2
 *************************************************/
//STATIC
EFI_STATUS
StartLinuxKernel( IN VOID* KernelAddress, IN UINTN R0, IN UINTN R1, IN UINTN R2 )
{
  VOID     (*Kernel)(UINT32 Zero, UINT32 Arch, UINTN AtagListParams);

  // set the kernel address
  Kernel = (VOID (*)(UINT32, UINT32, UINTN)) KernelAddress;

  // Outside BootServices, so can't use Print();
  DEBUG((EFI_D_ERROR, "\nStarting the kernel:\n\n"));

  // jump to kernel with register set
  Kernel( R0, R1, R2 );

  // Kernel should never exit
  // After Life services are not provided
  ASSERT( FALSE );

  return EFI_SUCCESS;
}

EFI_STATUS BdsBootLinux(
    IN  CONST CHAR16* LinuxKernel,
    IN  CONST CHAR8*  ATag,
    IN  CONST CHAR16* Fdt
) {
    BDS_FILE   LinuxKernelFile;
    BDS_FILE   FdtFile;
    EFI_STATUS          Status;
    VOID*               LinuxImage;

    UINT32              KernelParamsSize;
    VOID*               KernelParamsAddress = NULL;
    UINTN               KernelParamsNewAddress;
    UINTN               *AtagAddress;
    UINT32              MachineType;
    BOOLEAN             FdtSupported = FALSE;
    EFI_HOB_RESOURCE_DESCRIPTOR *ResHob;

    // Load the Linux kernel from a device path
    Status = BdsLoadFilePath(LinuxKernel, &LinuxKernelFile);
    if (EFI_ERROR(Status)) {
        DEBUG ((EFI_D_ERROR, "ERROR: Do not find Linux kernel %s\n",LinuxKernel));
        return Status;
    }

    // Copy the Linux Kernel from the raw file to Runtime memory
    Status = BdsCopyRawFileToRuntimeMemory(&LinuxKernelFile,&LinuxImage,NULL);
    if (EFI_ERROR(Status)) {
        goto Exit;
    }

    // Load the FDT binary from a device path
    Status = BdsLoadFilePath(Fdt, &FdtFile);
    if (!EFI_ERROR(Status)) {
        // Copy the FDT binary from the raw file to Runtime memory
        Status = BdsCopyRawFileToRuntimeMemory(&FdtFile,&KernelParamsAddress,&KernelParamsSize);
        if (EFI_ERROR(Status)) {
            goto Exit;
        } else {
            FdtSupported = TRUE;
        }
    }

    /**********************************************************
    * Setup the platform type
    **********************************************************/
    Status = GetARMLinuxMachineType(FdtSupported, &MachineType);
    if(EFI_ERROR(Status))
    {
        Print(L"ERROR  : Can not prepare ARM Linux machine type. Status=0x%X\n", Status);
        goto Exit;
    }

    if (!FdtSupported) {
        /**********************************************************
         * Setup the ATAG list
         **********************************************************/
        // By setting address=0 we leave the memory allocation to the function
        AtagAddress = 0;
        Status = PrepareAtagList( (struct atag **)&AtagAddress, ATag, &KernelParamsSize );
        KernelParamsAddress = (VOID*)AtagAddress;
        if(EFI_ERROR(Status))
        {
            Print(L"ERROR  : Can not prepare ATAG list. Status=0x%X\n", Status);
            goto Exit;
        }
    }

    /**********************************************************
    * Switch off interrupts, caches, mmu, etc
    **********************************************************/
    Status = PreparePlatformHardware();
    if(EFI_ERROR(Status))
    {
        Print(L"ERROR  : Can not prepare platform hardware. Status=0x%X\n", Status);
        goto Exit;
    }

    // Initialize the ATag destination
    KernelParamsNewAddress = 0x100;

    // Update the ATag destination by finding the start address of the first System Memory Resource Descriptor Hob
    ResHob = (EFI_HOB_RESOURCE_DESCRIPTOR *)GetFirstHob (EFI_HOB_TYPE_RESOURCE_DESCRIPTOR);
    while (ResHob != NULL) {
        if (ResHob->ResourceType == EFI_RESOURCE_SYSTEM_MEMORY) {
            KernelParamsNewAddress = (UINTN)ResHob->PhysicalStart + 0x100;
            break;
        }
        ResHob = (EFI_HOB_RESOURCE_DESCRIPTOR *)GetNextHob (EFI_HOB_TYPE_RESOURCE_DESCRIPTOR, (VOID *)((UINTN)ResHob + ResHob->Header.HobLength)); 
    }

    // Shut down UEFI boot services. ExitBootServices() will notify every driver that created an event on
    // ExitBootServices event. Example the Interrupt DXE driver will disable the interrupts on this event.
    Status = ShutdownUefiBootServices();
    if(EFI_ERROR(Status))
    {
        Print(L"ERROR  : Can not shutdown UEFI boot services. Status=0x%X\n", Status);
        goto Exit;
    }

    // Move the kernel parameters to any address inside the first 1MB.
    // This is necessary because the ARM Linux kernel requires
    // the FTD / ATAG List to reside entirely inside the first 1MB of
    // physical memory.
    CopyMem((VOID*)KernelParamsNewAddress, KernelParamsAddress, KernelParamsSize);

    //**********************************************************
    // * Start the Linux Kernel
    // **********************************************************
    // Lift off ...
    Status = StartLinuxKernel(LinuxImage, (UINTN)0, (UINTN)MachineType, KernelParamsNewAddress );

    // Only be here if we fail to start Linux
    DEBUG((EFI_D_ERROR, "ERROR  : Can not start the kernel. Status=0x%X\n", Status));

Exit:
    // Free Runtimee Memory (kernel and FDT)
    return Status;
}
