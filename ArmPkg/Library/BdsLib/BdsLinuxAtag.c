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
SetupInitrdTag (
  IN UINT32 InitrdImage,
  IN UINT32 InitrdImageSize
  )
{
  mLinuxKernelCurrentAtag->header.size = tag_size(LINUX_ATAG_INITRD2);
  mLinuxKernelCurrentAtag->header.type = ATAG_INITRD2;

  mLinuxKernelCurrentAtag->body.initrd2_tag.start = InitrdImage;
  mLinuxKernelCurrentAtag->body.initrd2_tag.size = InitrdImageSize;

  // Move pointer to next tag
  mLinuxKernelCurrentAtag = next_tag_address(mLinuxKernelCurrentAtag);
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

EFI_STATUS
PrepareAtagList (
  IN  CONST CHAR8*          CommandLineString,
  IN  EFI_PHYSICAL_ADDRESS  InitrdImage,
  IN  UINTN                 InitrdImageSize,
  OUT EFI_PHYSICAL_ADDRESS  *AtagBase,
  OUT UINT32                *AtagSize
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
    DEBUG ((EFI_D_WARN, "Warning: Failed to allocate Atag at 0x%lX (%r). The Atag will be allocated somewhere else in System Memory.\n", AtagStartAddress, Status));
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
  if (CommandLineString) {
    SetupCmdlineTag (CommandLineString);
  }

  if (InitrdImageSize > 0 && InitrdImage != 0) {
    SetupInitrdTag ((UINT32)InitrdImage, (UINT32)InitrdImageSize);
  }

  // End of tags
  SetupEndTag();

  // Calculate atag list size
  *AtagBase = AtagStartAddress;
  *AtagSize = (UINT32)mLinuxKernelCurrentAtag - (UINT32)AtagStartAddress + 1;

  return EFI_SUCCESS;
}

