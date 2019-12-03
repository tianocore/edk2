/**@file

Copyright (c) 2006 - 2009, Intel Corporation. All rights reserved.<BR>
Portions copyright (c) 2008 - 2010, Apple Inc. All rights reserved.<BR>
Portions copyright (c) 2011 - 2018, ARM Ltd. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>

#include <Library/ArmMmuLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/PeCoffLib.h>
#include <Library/PeCoffExtraActionLib.h>

typedef RETURN_STATUS (*REGION_PERMISSION_UPDATE_FUNC) (
  IN  EFI_PHYSICAL_ADDRESS      BaseAddress,
  IN  UINT64                    Length
  );

STATIC
RETURN_STATUS
UpdatePeCoffPermissions (
  IN  CONST PE_COFF_LOADER_IMAGE_CONTEXT      *ImageContext,
  IN  REGION_PERMISSION_UPDATE_FUNC           NoExecUpdater,
  IN  REGION_PERMISSION_UPDATE_FUNC           ReadOnlyUpdater
  )
{
  RETURN_STATUS                         Status;
  EFI_IMAGE_OPTIONAL_HEADER_PTR_UNION   Hdr;
  EFI_IMAGE_OPTIONAL_HEADER_UNION       HdrData;
  UINTN                                 Size;
  UINTN                                 ReadSize;
  UINT32                                SectionHeaderOffset;
  UINTN                                 NumberOfSections;
  UINTN                                 Index;
  EFI_IMAGE_SECTION_HEADER              SectionHeader;
  PE_COFF_LOADER_IMAGE_CONTEXT          TmpContext;
  EFI_PHYSICAL_ADDRESS                  Base;

  //
  // We need to copy ImageContext since PeCoffLoaderGetImageInfo ()
  // will mangle the ImageAddress field
  //
  CopyMem (&TmpContext, ImageContext, sizeof (TmpContext));

  if (TmpContext.PeCoffHeaderOffset == 0) {
    Status = PeCoffLoaderGetImageInfo (&TmpContext);
    if (RETURN_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR,
        "%a: PeCoffLoaderGetImageInfo () failed (Status = %r)\n",
        __FUNCTION__, Status));
      return Status;
    }
  }

  if (TmpContext.IsTeImage &&
      TmpContext.ImageAddress == ImageContext->ImageAddress) {
    DEBUG ((DEBUG_INFO, "%a: ignoring XIP TE image at 0x%lx\n", __FUNCTION__,
      ImageContext->ImageAddress));
    return RETURN_SUCCESS;
  }

  if (TmpContext.SectionAlignment < EFI_PAGE_SIZE) {
    //
    // The sections need to be at least 4 KB aligned, since that is the
    // granularity at which we can tighten permissions. So just clear the
    // noexec permissions on the entire region.
    //
    if (!TmpContext.IsTeImage) {
      DEBUG ((DEBUG_WARN,
        "%a: non-TE Image at 0x%lx has SectionAlignment < 4 KB (%lu)\n",
        __FUNCTION__, ImageContext->ImageAddress, TmpContext.SectionAlignment));
    }
    Base = ImageContext->ImageAddress & ~(EFI_PAGE_SIZE - 1);
    Size = ImageContext->ImageAddress - Base + ImageContext->ImageSize;
    return NoExecUpdater (Base, ALIGN_VALUE (Size, EFI_PAGE_SIZE));
  }

  //
  // Read the PE/COFF Header. For PE32 (32-bit) this will read in too much
  // data, but that should not hurt anything. Hdr.Pe32->OptionalHeader.Magic
  // determines if this is a PE32 or PE32+ image. The magic is in the same
  // location in both images.
  //
  Hdr.Union = &HdrData;
  Size = sizeof (EFI_IMAGE_OPTIONAL_HEADER_UNION);
  ReadSize = Size;
  Status = TmpContext.ImageRead (TmpContext.Handle,
                         TmpContext.PeCoffHeaderOffset, &Size, Hdr.Pe32);
  if (RETURN_ERROR (Status) || (Size != ReadSize)) {
    DEBUG ((DEBUG_ERROR,
      "%a: TmpContext.ImageRead () failed (Status = %r)\n",
      __FUNCTION__, Status));
    return Status;
  }

  ASSERT (Hdr.Pe32->Signature == EFI_IMAGE_NT_SIGNATURE);

  SectionHeaderOffset = TmpContext.PeCoffHeaderOffset + sizeof (UINT32) +
                        sizeof (EFI_IMAGE_FILE_HEADER);
  NumberOfSections    = (UINTN)(Hdr.Pe32->FileHeader.NumberOfSections);

  switch (Hdr.Pe32->OptionalHeader.Magic) {
    case EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC:
      SectionHeaderOffset += Hdr.Pe32->FileHeader.SizeOfOptionalHeader;
      break;
    case EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC:
      SectionHeaderOffset += Hdr.Pe32Plus->FileHeader.SizeOfOptionalHeader;
      break;
    default:
      ASSERT (FALSE);
  }

  //
  // Iterate over the sections
  //
  for (Index = 0; Index < NumberOfSections; Index++) {
    //
    // Read section header from file
    //
    Size = sizeof (EFI_IMAGE_SECTION_HEADER);
    ReadSize = Size;
    Status = TmpContext.ImageRead (TmpContext.Handle, SectionHeaderOffset,
                                   &Size, &SectionHeader);
    if (RETURN_ERROR (Status) || (Size != ReadSize)) {
      DEBUG ((DEBUG_ERROR,
        "%a: TmpContext.ImageRead () failed (Status = %r)\n",
        __FUNCTION__, Status));
      return Status;
    }

    Base = TmpContext.ImageAddress + SectionHeader.VirtualAddress;

    if ((SectionHeader.Characteristics & EFI_IMAGE_SCN_MEM_EXECUTE) == 0) {

      if ((SectionHeader.Characteristics & EFI_IMAGE_SCN_MEM_WRITE) == 0) {

        DEBUG ((DEBUG_INFO,
          "%a: Mapping section %d of image at 0x%lx with RO-XN permissions and size 0x%x\n",
          __FUNCTION__, Index, Base, SectionHeader.Misc.VirtualSize));
        ReadOnlyUpdater (Base, SectionHeader.Misc.VirtualSize);
      } else {
        DEBUG ((DEBUG_WARN,
          "%a: Mapping section %d of image at 0x%lx with RW-XN permissions and size 0x%x\n",
          __FUNCTION__, Index, Base, SectionHeader.Misc.VirtualSize));
      }
    } else {
        DEBUG ((DEBUG_INFO,
          "%a: Mapping section %d of image at 0x%lx with RO-X permissions and size 0x%x\n",
          __FUNCTION__, Index, Base, SectionHeader.Misc.VirtualSize));
        ReadOnlyUpdater (Base, SectionHeader.Misc.VirtualSize);
        NoExecUpdater (Base, SectionHeader.Misc.VirtualSize);
    }

    SectionHeaderOffset += sizeof (EFI_IMAGE_SECTION_HEADER);
  }
  return RETURN_SUCCESS;
}

/**
  Performs additional actions after a PE/COFF image has been loaded and relocated.

  If ImageContext is NULL, then ASSERT().

  @param  ImageContext  Pointer to the image context structure that describes the
                        PE/COFF image that has already been loaded and relocated.

**/
VOID
EFIAPI
PeCoffLoaderRelocateImageExtraAction (
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext
  )
{
  UpdatePeCoffPermissions (
    ImageContext,
    ArmClearMemoryRegionNoExec,
    ArmSetMemoryRegionReadOnly
    );
}



/**
  Performs additional actions just before a PE/COFF image is unloaded.  Any resources
  that were allocated by PeCoffLoaderRelocateImageExtraAction() must be freed.

  If ImageContext is NULL, then ASSERT().

  @param  ImageContext  Pointer to the image context structure that describes the
                        PE/COFF image that is being unloaded.

**/
VOID
EFIAPI
PeCoffLoaderUnloadImageExtraAction (
  IN OUT PE_COFF_LOADER_IMAGE_CONTEXT  *ImageContext
  )
{
  UpdatePeCoffPermissions (
    ImageContext,
    ArmSetMemoryRegionNoExec,
    ArmClearMemoryRegionReadOnly
    );
}
