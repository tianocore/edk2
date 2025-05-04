/** @file
  Main SEC phase code.  Transitions to PEI.

  Copyright (c) 2008 - 2015, Intel Corporation. All rights reserved.<BR>
  (C) Copyright 2016 Hewlett Packard Enterprise Development LP<BR>
  Copyright (c) 2020, Advanced Micro Devices, Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/PcdLib.h>
#include <Library/CpuLib.h>
#include <Library/DebugAgentLib.h>
#include <Library/IoLib.h>
#include <Library/PeCoffLib.h>
#include <Library/PeCoffGetEntryPointLib.h>
#include <Library/PeCoffExtraActionLib.h>
#include <Library/ExtractGuidedSectionLib.h>
#include <Library/LocalApicLib.h>
#include <Library/CpuExceptionHandlerLib.h>
#include <Ppi/TemporaryRamSupport.h>
#include <Ppi/MpInitLibDep.h>
#include <Library/TdxHelperLib.h>
#include <Library/CcProbeLib.h>
#include <Register/Intel/ArchitecturalMsr.h>
#include <Register/Intel/Cpuid.h>
#include "AmdSev.h"
#include <Library/AmdSvsmLib.h>
#include <Library/MemEncryptSevLib.h>

#define SEC_IDT_ENTRY_COUNT  34

typedef struct _SEC_IDT_TABLE {
  EFI_PEI_SERVICES            *PeiService;
  IA32_IDT_GATE_DESCRIPTOR    IdtTable[SEC_IDT_ENTRY_COUNT];
} SEC_IDT_TABLE;

VOID
EFIAPI
SecStartupPhase2 (
  IN VOID  *Context
  );

EFI_STATUS
EFIAPI
TemporaryRamMigration (
  IN CONST EFI_PEI_SERVICES  **PeiServices,
  IN EFI_PHYSICAL_ADDRESS    TemporaryMemoryBase,
  IN EFI_PHYSICAL_ADDRESS    PermanentMemoryBase,
  IN UINTN                   CopySize
  );

//
//
//
EFI_PEI_TEMPORARY_RAM_SUPPORT_PPI  mTemporaryRamSupportPpi = {
  TemporaryRamMigration
};

EFI_PEI_PPI_DESCRIPTOR  mPrivateDispatchTableMp[] = {
  {
    (EFI_PEI_PPI_DESCRIPTOR_PPI),
    &gEfiTemporaryRamSupportPpiGuid,
    &mTemporaryRamSupportPpi
  },
  {
    (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
    &gEfiPeiMpInitLibMpDepPpiGuid,
    NULL
  },
};

EFI_PEI_PPI_DESCRIPTOR  mPrivateDispatchTableUp[] = {
  {
    (EFI_PEI_PPI_DESCRIPTOR_PPI),
    &gEfiTemporaryRamSupportPpiGuid,
    &mTemporaryRamSupportPpi
  },
  {
    (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
    &gEfiPeiMpInitLibUpDepPpiGuid,
    NULL
  },
};

//
// Template of an IDT entry pointing to 10:FFFFFFE4h.
//
IA32_IDT_GATE_DESCRIPTOR  mIdtEntryTemplate = {
  {                                      // Bits
    0xffe4,                              // OffsetLow
    0x10,                                // Selector
    0x0,                                 // Reserved_0
    IA32_IDT_GATE_TYPE_INTERRUPT_32,     // GateType
    0xffff                               // OffsetHigh
  }
};

/**
  Locates the main boot firmware volume.

  @param[in,out]  BootFv  On input, the base of the BootFv
                          On output, the decompressed main firmware volume

  @retval EFI_SUCCESS    The main firmware volume was located and decompressed
  @retval EFI_NOT_FOUND  The main firmware volume was not found

**/
EFI_STATUS
FindMainFv (
  IN OUT  EFI_FIRMWARE_VOLUME_HEADER  **BootFv
  )
{
  EFI_FIRMWARE_VOLUME_HEADER  *Fv;
  UINTN                       Distance;

  ASSERT (((UINTN)*BootFv & EFI_PAGE_MASK) == 0);

  Fv       = *BootFv;
  Distance = (UINTN)(*BootFv)->FvLength;
  do {
    Fv        = (EFI_FIRMWARE_VOLUME_HEADER *)((UINT8 *)Fv - EFI_PAGE_SIZE);
    Distance += EFI_PAGE_SIZE;
    if (Distance > SIZE_32MB) {
      return EFI_NOT_FOUND;
    }

    if (Fv->Signature != EFI_FVH_SIGNATURE) {
      continue;
    }

    if ((UINTN)Fv->FvLength > Distance) {
      continue;
    }

    *BootFv = Fv;
    return EFI_SUCCESS;
  } while (TRUE);
}

/**
  Locates a section within a series of sections
  with the specified section type.

  The Instance parameter indicates which instance of the section
  type to return. (0 is first instance, 1 is second...)

  @param[in]   Sections        The sections to search
  @param[in]   SizeOfSections  Total size of all sections
  @param[in]   SectionType     The section type to locate
  @param[in]   Instance        The section instance number
  @param[out]  FoundSection    The FFS section if found

  @retval EFI_SUCCESS           The file and section was found
  @retval EFI_NOT_FOUND         The file and section was not found
  @retval EFI_VOLUME_CORRUPTED  The firmware volume was corrupted

**/
EFI_STATUS
FindFfsSectionInstance (
  IN  VOID                       *Sections,
  IN  UINTN                      SizeOfSections,
  IN  EFI_SECTION_TYPE           SectionType,
  IN  UINTN                      Instance,
  OUT EFI_COMMON_SECTION_HEADER  **FoundSection
  )
{
  EFI_PHYSICAL_ADDRESS       CurrentAddress;
  UINT32                     Size;
  EFI_PHYSICAL_ADDRESS       EndOfSections;
  EFI_COMMON_SECTION_HEADER  *Section;
  EFI_PHYSICAL_ADDRESS       EndOfSection;

  //
  // Loop through the FFS file sections within the PEI Core FFS file
  //
  EndOfSection  = (EFI_PHYSICAL_ADDRESS)(UINTN)Sections;
  EndOfSections = EndOfSection + SizeOfSections;
  for ( ; ;) {
    if (EndOfSection == EndOfSections) {
      break;
    }

    CurrentAddress = (EndOfSection + 3) & ~(3ULL);
    if (CurrentAddress >= EndOfSections) {
      return EFI_VOLUME_CORRUPTED;
    }

    Section = (EFI_COMMON_SECTION_HEADER *)(UINTN)CurrentAddress;

    Size = SECTION_SIZE (Section);
    if (Size < sizeof (*Section)) {
      return EFI_VOLUME_CORRUPTED;
    }

    EndOfSection = CurrentAddress + Size;
    if (EndOfSection > EndOfSections) {
      return EFI_VOLUME_CORRUPTED;
    }

    //
    // Look for the requested section type
    //
    if (Section->Type == SectionType) {
      if (Instance == 0) {
        *FoundSection = Section;
        return EFI_SUCCESS;
      } else {
        Instance--;
      }
    }
  }

  return EFI_NOT_FOUND;
}

/**
  Locates a section within a series of sections
  with the specified section type.

  @param[in]   Sections        The sections to search
  @param[in]   SizeOfSections  Total size of all sections
  @param[in]   SectionType     The section type to locate
  @param[out]  FoundSection    The FFS section if found

  @retval EFI_SUCCESS           The file and section was found
  @retval EFI_NOT_FOUND         The file and section was not found
  @retval EFI_VOLUME_CORRUPTED  The firmware volume was corrupted

**/
EFI_STATUS
FindFfsSectionInSections (
  IN  VOID                       *Sections,
  IN  UINTN                      SizeOfSections,
  IN  EFI_SECTION_TYPE           SectionType,
  OUT EFI_COMMON_SECTION_HEADER  **FoundSection
  )
{
  return FindFfsSectionInstance (
           Sections,
           SizeOfSections,
           SectionType,
           0,
           FoundSection
           );
}

/**
  Locates a FFS file with the specified file type and a section
  within that file with the specified section type.

  @param[in]   Fv            The firmware volume to search
  @param[in]   FileType      The file type to locate
  @param[in]   SectionType   The section type to locate
  @param[out]  FoundSection  The FFS section if found

  @retval EFI_SUCCESS           The file and section was found
  @retval EFI_NOT_FOUND         The file and section was not found
  @retval EFI_VOLUME_CORRUPTED  The firmware volume was corrupted

**/
EFI_STATUS
FindFfsFileAndSection (
  IN  EFI_FIRMWARE_VOLUME_HEADER  *Fv,
  IN  EFI_FV_FILETYPE             FileType,
  IN  EFI_SECTION_TYPE            SectionType,
  OUT EFI_COMMON_SECTION_HEADER   **FoundSection
  )
{
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  CurrentAddress;
  EFI_PHYSICAL_ADDRESS  EndOfFirmwareVolume;
  EFI_FFS_FILE_HEADER   *File;
  UINT32                Size;
  EFI_PHYSICAL_ADDRESS  EndOfFile;

  if (Fv->Signature != EFI_FVH_SIGNATURE) {
    DEBUG ((DEBUG_ERROR, "FV at %p does not have FV header signature\n", Fv));
    return EFI_VOLUME_CORRUPTED;
  }

  CurrentAddress      = (EFI_PHYSICAL_ADDRESS)(UINTN)Fv;
  EndOfFirmwareVolume = CurrentAddress + Fv->FvLength;

  //
  // Loop through the FFS files in the Boot Firmware Volume
  //
  for (EndOfFile = CurrentAddress + Fv->HeaderLength; ; ) {
    CurrentAddress = (EndOfFile + 7) & ~(7ULL);
    if (CurrentAddress > EndOfFirmwareVolume) {
      return EFI_VOLUME_CORRUPTED;
    }

    File = (EFI_FFS_FILE_HEADER *)(UINTN)CurrentAddress;
    Size = FFS_FILE_SIZE (File);
    if (Size < (sizeof (*File) + sizeof (EFI_COMMON_SECTION_HEADER))) {
      return EFI_VOLUME_CORRUPTED;
    }

    EndOfFile = CurrentAddress + Size;
    if (EndOfFile > EndOfFirmwareVolume) {
      return EFI_VOLUME_CORRUPTED;
    }

    //
    // Look for the request file type
    //
    if (File->Type != FileType) {
      continue;
    }

    Status = FindFfsSectionInSections (
               (VOID *)(File + 1),
               (UINTN)EndOfFile - (UINTN)(File + 1),
               SectionType,
               FoundSection
               );
    if (!EFI_ERROR (Status) || (Status == EFI_VOLUME_CORRUPTED)) {
      return Status;
    }
  }
}

/**
  Locates the compressed main firmware volume and decompresses it.

  @param[in,out]  Fv            On input, the firmware volume to search
                                On output, the decompressed BOOT/PEI FV

  @retval EFI_SUCCESS           The file and section was found
  @retval EFI_NOT_FOUND         The file and section was not found
  @retval EFI_VOLUME_CORRUPTED  The firmware volume was corrupted

**/
EFI_STATUS
DecompressMemFvs (
  IN OUT EFI_FIRMWARE_VOLUME_HEADER  **Fv
  )
{
  EFI_STATUS                  Status;
  EFI_GUID_DEFINED_SECTION    *Section;
  UINT32                      OutputBufferSize;
  UINT32                      ScratchBufferSize;
  UINT16                      SectionAttribute;
  UINT32                      AuthenticationStatus;
  VOID                        *OutputBuffer;
  VOID                        *ScratchBuffer;
  EFI_COMMON_SECTION_HEADER   *FvSection;
  EFI_FIRMWARE_VOLUME_HEADER  *PeiMemFv;
  EFI_FIRMWARE_VOLUME_HEADER  *DxeMemFv;
  UINT32                      FvHeaderSize;
  UINT32                      FvSectionSize;

  FvSection = (EFI_COMMON_SECTION_HEADER *)NULL;

  Status = FindFfsFileAndSection (
             *Fv,
             EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE,
             EFI_SECTION_GUID_DEFINED,
             (EFI_COMMON_SECTION_HEADER **)&Section
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Unable to find GUID defined section\n"));
    return Status;
  }

  Status = ExtractGuidedSectionGetInfo (
             Section,
             &OutputBufferSize,
             &ScratchBufferSize,
             &SectionAttribute
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Unable to GetInfo for GUIDed section\n"));
    return Status;
  }

  OutputBuffer  = (VOID *)((UINT8 *)(UINTN)PcdGet32 (PcdOvmfDxeMemFvBase) + SIZE_1MB);
  ScratchBuffer = ALIGN_POINTER ((UINT8 *)OutputBuffer + OutputBufferSize, SIZE_1MB);

  DEBUG ((
    DEBUG_VERBOSE,
    "%a: OutputBuffer@%p+0x%x ScratchBuffer@%p+0x%x "
    "PcdOvmfDecompressionScratchEnd=0x%x\n",
    __func__,
    OutputBuffer,
    OutputBufferSize,
    ScratchBuffer,
    ScratchBufferSize,
    PcdGet32 (PcdOvmfDecompressionScratchEnd)
    ));
  ASSERT (
    (UINTN)ScratchBuffer + ScratchBufferSize ==
    PcdGet32 (PcdOvmfDecompressionScratchEnd)
    );

  Status = ExtractGuidedSectionDecode (
             Section,
             &OutputBuffer,
             ScratchBuffer,
             &AuthenticationStatus
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Error during GUID section decode\n"));
    return Status;
  }

  Status = FindFfsSectionInstance (
             OutputBuffer,
             OutputBufferSize,
             EFI_SECTION_FIRMWARE_VOLUME_IMAGE,
             0,
             &FvSection
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Unable to find PEI FV section\n"));
    return Status;
  }

  ASSERT (
    SECTION_SIZE (FvSection) ==
    (PcdGet32 (PcdOvmfPeiMemFvSize) + sizeof (*FvSection))
    );
  ASSERT (FvSection->Type == EFI_SECTION_FIRMWARE_VOLUME_IMAGE);

  PeiMemFv = (EFI_FIRMWARE_VOLUME_HEADER *)(UINTN)PcdGet32 (PcdOvmfPeiMemFvBase);
  CopyMem (PeiMemFv, (VOID *)(FvSection + 1), PcdGet32 (PcdOvmfPeiMemFvSize));

  if (PeiMemFv->Signature != EFI_FVH_SIGNATURE) {
    DEBUG ((DEBUG_ERROR, "Extracted FV at %p does not have FV header signature\n", PeiMemFv));
    CpuDeadLoop ();
    return EFI_VOLUME_CORRUPTED;
  }

  Status = FindFfsSectionInstance (
             OutputBuffer,
             OutputBufferSize,
             EFI_SECTION_FIRMWARE_VOLUME_IMAGE,
             1,
             &FvSection
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Unable to find DXE FV section\n"));
    return Status;
  }

  ASSERT (FvSection->Type == EFI_SECTION_FIRMWARE_VOLUME_IMAGE);

  if (IS_SECTION2 (FvSection)) {
    FvSectionSize = SECTION2_SIZE (FvSection);
    FvHeaderSize  = sizeof (EFI_COMMON_SECTION_HEADER2);
  } else {
    FvSectionSize = SECTION_SIZE (FvSection);
    FvHeaderSize  = sizeof (EFI_COMMON_SECTION_HEADER);
  }

  ASSERT (FvSectionSize == (PcdGet32 (PcdOvmfDxeMemFvSize) + FvHeaderSize));

  DxeMemFv = (EFI_FIRMWARE_VOLUME_HEADER *)(UINTN)PcdGet32 (PcdOvmfDxeMemFvBase);
  CopyMem (DxeMemFv, (VOID *)((UINTN)FvSection + FvHeaderSize), PcdGet32 (PcdOvmfDxeMemFvSize));

  if (DxeMemFv->Signature != EFI_FVH_SIGNATURE) {
    DEBUG ((DEBUG_ERROR, "Extracted FV at %p does not have FV header signature\n", DxeMemFv));
    CpuDeadLoop ();
    return EFI_VOLUME_CORRUPTED;
  }

  *Fv = PeiMemFv;
  return EFI_SUCCESS;
}

/**
  Locates the PEI Core entry point address

  @param[in]  Fv                 The firmware volume to search
  @param[out] PeiCoreEntryPoint  The entry point of the PEI Core image

  @retval EFI_SUCCESS           The file and section was found
  @retval EFI_NOT_FOUND         The file and section was not found
  @retval EFI_VOLUME_CORRUPTED  The firmware volume was corrupted

**/
EFI_STATUS
FindPeiCoreImageBaseInFv (
  IN  EFI_FIRMWARE_VOLUME_HEADER  *Fv,
  OUT  EFI_PHYSICAL_ADDRESS       *PeiCoreImageBase
  )
{
  EFI_STATUS                 Status;
  EFI_COMMON_SECTION_HEADER  *Section;

  Status = FindFfsFileAndSection (
             Fv,
             EFI_FV_FILETYPE_PEI_CORE,
             EFI_SECTION_PE32,
             &Section
             );
  if (EFI_ERROR (Status)) {
    Status = FindFfsFileAndSection (
               Fv,
               EFI_FV_FILETYPE_PEI_CORE,
               EFI_SECTION_TE,
               &Section
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Unable to find PEI Core image\n"));
      return Status;
    }
  }

  *PeiCoreImageBase = (EFI_PHYSICAL_ADDRESS)(UINTN)(Section + 1);
  return EFI_SUCCESS;
}

/**
  Reads 8-bits of CMOS data.

  Reads the 8-bits of CMOS data at the location specified by Index.
  The 8-bit read value is returned.

  @param  Index  The CMOS location to read.

  @return The value read.

**/
STATIC
UINT8
CmosRead8 (
  IN      UINTN  Index
  )
{
  IoWrite8 (0x70, (UINT8)Index);
  return IoRead8 (0x71);
}

STATIC
BOOLEAN
IsS3Resume (
  VOID
  )
{
  return (CmosRead8 (0xF) == 0xFE);
}

STATIC
EFI_STATUS
GetS3ResumePeiFv (
  IN OUT EFI_FIRMWARE_VOLUME_HEADER  **PeiFv
  )
{
  *PeiFv = (EFI_FIRMWARE_VOLUME_HEADER *)(UINTN)PcdGet32 (PcdOvmfPeiMemFvBase);
  return EFI_SUCCESS;
}

/**
  Locates the PEI Core entry point address

  @param[in,out]  Fv                 The firmware volume to search
  @param[out]     PeiCoreEntryPoint  The entry point of the PEI Core image

  @retval EFI_SUCCESS           The file and section was found
  @retval EFI_NOT_FOUND         The file and section was not found
  @retval EFI_VOLUME_CORRUPTED  The firmware volume was corrupted

**/
VOID
FindPeiCoreImageBase (
  IN OUT  EFI_FIRMWARE_VOLUME_HEADER  **BootFv,
  OUT  EFI_PHYSICAL_ADDRESS           *PeiCoreImageBase
  )
{
  BOOLEAN  S3Resume;

  *PeiCoreImageBase = 0;

  S3Resume = IsS3Resume ();
  if (S3Resume && !FeaturePcdGet (PcdSmmSmramRequire)) {
    //
    // A malicious runtime OS may have injected something into our previously
    // decoded PEI FV, but we don't care about that unless SMM/SMRAM is required.
    //
    DEBUG ((DEBUG_VERBOSE, "SEC: S3 resume\n"));
    GetS3ResumePeiFv (BootFv);
  } else {
    //
    // We're either not resuming, or resuming "securely" -- we'll decompress
    // both PEI FV and DXE FV from pristine flash.
    //
    DEBUG ((
      DEBUG_VERBOSE,
      "SEC: %a\n",
      S3Resume ? "S3 resume (with PEI decompression)" : "Normal boot"
      ));
    FindMainFv (BootFv);

    DecompressMemFvs (BootFv);
  }

  FindPeiCoreImageBaseInFv (*BootFv, PeiCoreImageBase);
}

/**
  Find core image base.

**/
EFI_STATUS
FindImageBase (
  IN  EFI_FIRMWARE_VOLUME_HEADER  *BootFirmwareVolumePtr,
  OUT EFI_PHYSICAL_ADDRESS        *SecCoreImageBase
  )
{
  EFI_PHYSICAL_ADDRESS       CurrentAddress;
  EFI_PHYSICAL_ADDRESS       EndOfFirmwareVolume;
  EFI_FFS_FILE_HEADER        *File;
  UINT32                     Size;
  EFI_PHYSICAL_ADDRESS       EndOfFile;
  EFI_COMMON_SECTION_HEADER  *Section;
  EFI_PHYSICAL_ADDRESS       EndOfSection;

  *SecCoreImageBase = 0;

  CurrentAddress      = (EFI_PHYSICAL_ADDRESS)(UINTN)BootFirmwareVolumePtr;
  EndOfFirmwareVolume = CurrentAddress + BootFirmwareVolumePtr->FvLength;

  //
  // Loop through the FFS files in the Boot Firmware Volume
  //
  for (EndOfFile = CurrentAddress + BootFirmwareVolumePtr->HeaderLength; ; ) {
    CurrentAddress = (EndOfFile + 7) & 0xfffffffffffffff8ULL;
    if (CurrentAddress > EndOfFirmwareVolume) {
      return EFI_NOT_FOUND;
    }

    File = (EFI_FFS_FILE_HEADER *)(UINTN)CurrentAddress;
    Size = FFS_FILE_SIZE (File);
    if (Size < sizeof (*File)) {
      return EFI_NOT_FOUND;
    }

    EndOfFile = CurrentAddress + Size;
    if (EndOfFile > EndOfFirmwareVolume) {
      return EFI_NOT_FOUND;
    }

    //
    // Look for SEC Core
    //
    if (File->Type != EFI_FV_FILETYPE_SECURITY_CORE) {
      continue;
    }

    //
    // Loop through the FFS file sections within the FFS file
    //
    EndOfSection = (EFI_PHYSICAL_ADDRESS)(UINTN)(File + 1);
    for ( ; ;) {
      CurrentAddress = (EndOfSection + 3) & 0xfffffffffffffffcULL;
      Section        = (EFI_COMMON_SECTION_HEADER *)(UINTN)CurrentAddress;

      Size = SECTION_SIZE (Section);
      if (Size < sizeof (*Section)) {
        return EFI_NOT_FOUND;
      }

      EndOfSection = CurrentAddress + Size;
      if (EndOfSection > EndOfFile) {
        return EFI_NOT_FOUND;
      }

      //
      // Look for executable sections
      //
      if ((Section->Type == EFI_SECTION_PE32) || (Section->Type == EFI_SECTION_TE)) {
        if (File->Type == EFI_FV_FILETYPE_SECURITY_CORE) {
          *SecCoreImageBase = (PHYSICAL_ADDRESS)(UINTN)(Section + 1);
        }

        break;
      }
    }

    //
    // SEC Core image found
    //
    if (*SecCoreImageBase != 0) {
      return EFI_SUCCESS;
    }
  }
}

/*
  Find and return Pei Core entry point.

  It also find SEC and PEI Core file debug information. It will report them if
  remote debug is enabled.

**/
VOID
FindAndReportEntryPoints (
  IN  EFI_FIRMWARE_VOLUME_HEADER  **BootFirmwareVolumePtr,
  OUT EFI_PEI_CORE_ENTRY_POINT    *PeiCoreEntryPoint
  )
{
  EFI_STATUS                    Status;
  EFI_PHYSICAL_ADDRESS          SecCoreImageBase;
  EFI_PHYSICAL_ADDRESS          PeiCoreImageBase;
  PE_COFF_LOADER_IMAGE_CONTEXT  ImageContext;

  //
  // Find SEC Core and PEI Core image base
  //
  Status = FindImageBase (*BootFirmwareVolumePtr, &SecCoreImageBase);
  ASSERT_EFI_ERROR (Status);

  FindPeiCoreImageBase (BootFirmwareVolumePtr, &PeiCoreImageBase);

  ZeroMem ((VOID *)&ImageContext, sizeof (PE_COFF_LOADER_IMAGE_CONTEXT));
  //
  // Report SEC Core debug information when remote debug is enabled
  //
  ImageContext.ImageAddress = SecCoreImageBase;
  ImageContext.PdbPointer   = PeCoffLoaderGetPdbPointer ((VOID *)(UINTN)ImageContext.ImageAddress);
  PeCoffLoaderRelocateImageExtraAction (&ImageContext);

  //
  // Report PEI Core debug information when remote debug is enabled
  //
  ImageContext.ImageAddress = (EFI_PHYSICAL_ADDRESS)(UINTN)PeiCoreImageBase;
  ImageContext.PdbPointer   = PeCoffLoaderGetPdbPointer ((VOID *)(UINTN)ImageContext.ImageAddress);
  PeCoffLoaderRelocateImageExtraAction (&ImageContext);

  //
  // Find PEI Core entry point
  //
  Status = PeCoffLoaderGetEntryPoint ((VOID *)(UINTN)PeiCoreImageBase, (VOID **)PeiCoreEntryPoint);
  if (EFI_ERROR (Status)) {
    *PeiCoreEntryPoint = 0;
  }

  return;
}

//
// Enable MTRR early, set default type to write back.
// Needed to make sure caching is enabled,
// without this lzma decompress can be very slow.
//
STATIC
VOID
SecMtrrSetup (
  VOID
  )
{
  CPUID_VERSION_INFO_EDX           Edx;
  MSR_IA32_MTRR_DEF_TYPE_REGISTER  DefType;

  AsmCpuid (CPUID_VERSION_INFO, NULL, NULL, NULL, &Edx.Uint32);
  if (!Edx.Bits.MTRR) {
    return;
  }

 #if defined (TDX_GUEST_SUPPORTED)
  if (CcProbe () == CcGuestTypeIntelTdx) {
    //
    // According to TDX Spec, the default MTRR type is enforced to WB
    // and CR0.CD is enforced to 0.
    // The TD guest has to disable MTRR otherwise it tries to
    // program MTRRs to disable caching. CR0.CD=1 results in the
    // unexpected #VE.
    //
    DEBUG ((DEBUG_INFO, "%a: Skip TD-Guest\n", __func__));
    return;
  }

 #endif

  DefType.Uint64    = AsmReadMsr64 (MSR_IA32_MTRR_DEF_TYPE);
  DefType.Bits.Type = MSR_IA32_MTRR_CACHE_WRITE_BACK;
  DefType.Bits.E    = 1; /* enable */
  AsmWriteMsr64 (MSR_IA32_MTRR_DEF_TYPE, DefType.Uint64);
}

VOID
EFIAPI
SecCoreStartupWithStack (
  IN EFI_FIRMWARE_VOLUME_HEADER  *BootFv,
  IN VOID                        *TopOfCurrentStack
  )
{
  EFI_SEC_PEI_HAND_OFF  SecCoreData;
  SEC_IDT_TABLE         IdtTableInStack;
  IA32_DESCRIPTOR       IdtDescriptor;
  UINT32                Index;
  volatile UINT8        *Table;

 #if defined (TDX_GUEST_SUPPORTED)
  if (CcProbe () == CcGuestTypeIntelTdx) {
    //
    // From the security perspective all the external input should be measured before
    // it is consumed. TdHob and Configuration FV (Cfv) image are passed from VMM
    // and should be measured here.
    //
    if (EFI_ERROR (TdxHelperMeasureTdHob ())) {
      CpuDeadLoop ();
    }

    if (EFI_ERROR (TdxHelperMeasureCfvImage ())) {
      CpuDeadLoop ();
    }

    //
    // For Td guests, the memory map info is in TdHobLib. It should be processed
    // first so that the memory is accepted. Otherwise access to the unaccepted
    // memory will trigger tripple fault.
    //
    if (TdxHelperProcessTdHob () != EFI_SUCCESS) {
      CpuDeadLoop ();
    }
  }

 #endif

  //
  // To ensure SMM can't be compromised on S3 resume, we must force re-init of
  // the BaseExtractGuidedSectionLib. Since this is before library contructors
  // are called, we must use a loop rather than SetMem.
  //
  Table = (UINT8 *)(UINTN)FixedPcdGet64 (PcdGuidedExtractHandlerTableAddress);
  for (Index = 0;
       Index < FixedPcdGet32 (PcdGuidedExtractHandlerTableSize);
       ++Index)
  {
    Table[Index] = 0;
  }

  //
  // Initialize IDT - Since this is before library constructors are called,
  // we use a loop rather than CopyMem.
  //
  IdtTableInStack.PeiService = NULL;

  for (Index = 0; Index < SEC_IDT_ENTRY_COUNT; Index++) {
    //
    // Declare the local variables that actually move the data elements as
    // volatile to prevent the optimizer from replacing this function with
    // the intrinsic memcpy()
    //
    CONST UINT8     *Src;
    volatile UINT8  *Dst;
    UINTN           Byte;

    Src = (CONST UINT8 *)&mIdtEntryTemplate;
    Dst = (volatile UINT8 *)&IdtTableInStack.IdtTable[Index];
    for (Byte = 0; Byte < sizeof (mIdtEntryTemplate); Byte++) {
      Dst[Byte] = Src[Byte];
    }
  }

  IdtDescriptor.Base  = (UINTN)&IdtTableInStack.IdtTable;
  IdtDescriptor.Limit = (UINT16)(sizeof (IdtTableInStack.IdtTable) - 1);

  if (SevEsIsEnabled ()) {
    SevEsProtocolCheck ();

    //
    // For SEV-ES guests, the exception handler is needed before calling
    // ProcessLibraryConstructorList() because some of the library constructors
    // perform some functions that result in #VC exceptions being generated.
    //
    // Due to this code executing before library constructors, *all* library
    // API calls are theoretically interface contract violations. However,
    // because this is SEC (executing in flash), those constructors cannot
    // write variables with static storage duration anyway. Furthermore, only
    // a small, restricted set of APIs, such as AsmWriteIdtr() and
    // InitializeCpuExceptionHandlers(), are called, where we require that the
    // underlying library not require constructors to have been invoked and
    // that the library instance not trigger any #VC exceptions.
    //
    AsmWriteIdtr (&IdtDescriptor);
    InitializeCpuExceptionHandlers (NULL);
  }

  ProcessLibraryConstructorList ();

  if (!SevEsIsEnabled ()) {
    //
    // For non SEV-ES guests, just load the IDTR.
    //
    AsmWriteIdtr (&IdtDescriptor);
  } else {
    //
    // Under SEV-ES, the hypervisor can't modify CR0 and so can't enable
    // caching in order to speed up the boot. Enable caching early for
    // an SEV-ES guest.
    //
    AsmEnableCache ();
  }

 #if defined (TDX_GUEST_SUPPORTED)
  if (CcProbe () == CcGuestTypeIntelTdx) {
    //
    // InitializeCpuExceptionHandlers () should be called in Td guests so that
    // #VE exceptions can be handled correctly.
    //
    InitializeCpuExceptionHandlers (NULL);
  }

 #endif

  DEBUG ((
    DEBUG_INFO,
    "SecCoreStartupWithStack(0x%x, 0x%x)\n",
    (UINT32)(UINTN)BootFv,
    (UINT32)(UINTN)TopOfCurrentStack
    ));

  //
  // Initialize floating point operating environment
  // to be compliant with UEFI spec.
  //
  InitializeFloatingPointUnits ();

 #if defined (MDE_CPU_X64)
  //
  // ASSERT that the Page Tables were set by the reset vector code to
  // the address we expect.
  //
  ASSERT (AsmReadCr3 () == (UINTN)PcdGet32 (PcdOvmfSecPageTablesBase));
 #endif

  //
  // |-------------|       <-- TopOfCurrentStack
  // |   Stack     | 32k
  // |-------------|
  // |    Heap     | 32k
  // |-------------|       <-- SecCoreData.TemporaryRamBase
  //

  ASSERT (
    (UINTN)(PcdGet32 (PcdOvmfSecPeiTempRamBase) +
            PcdGet32 (PcdOvmfSecPeiTempRamSize)) ==
    (UINTN)TopOfCurrentStack
    );

  //
  // Initialize SEC hand-off state
  //
  SecCoreData.DataSize = sizeof (EFI_SEC_PEI_HAND_OFF);

  SecCoreData.TemporaryRamSize = (UINTN)PcdGet32 (PcdOvmfSecPeiTempRamSize);
  SecCoreData.TemporaryRamBase = (VOID *)((UINT8 *)TopOfCurrentStack - SecCoreData.TemporaryRamSize);

  SecCoreData.PeiTemporaryRamBase = SecCoreData.TemporaryRamBase;
  SecCoreData.PeiTemporaryRamSize = SecCoreData.TemporaryRamSize >> 1;

  SecCoreData.StackBase = (UINT8 *)SecCoreData.TemporaryRamBase + SecCoreData.PeiTemporaryRamSize;
  SecCoreData.StackSize = SecCoreData.TemporaryRamSize >> 1;

  SecCoreData.BootFirmwareVolumeBase = BootFv;
  SecCoreData.BootFirmwareVolumeSize = (UINTN)BootFv->FvLength;

  //
  // Validate the System RAM used in the SEC Phase
  //
  SecValidateSystemRam ();

  //
  // Make sure the 8259 is masked before initializing the Debug Agent and the debug timer is enabled
  //
  IoWrite8 (0x21, 0xff);
  IoWrite8 (0xA1, 0xff);

  // Enable X2APIC mode if Alternate Injection is enabled.
  if (AmdSvsmIsSvsmPresent()) {
       if (AlternateInjectionEnabled()){
           SetApicMode (LOCAL_APIC_MODE_X2APIC);
       }
  }

  //
  // Initialize Local APIC Timer hardware and disable Local APIC Timer
  // interrupts before initializing the Debug Agent and the debug timer is
  // enabled.
  //
  SecMapApicBaseUnencrypted ();
  InitializeApicTimer (0, MAX_UINT32, TRUE, 5);
  DisableApicTimerInterrupt ();

  //
  // Initialize MTRR
  //
  SecMtrrSetup ();

  //
  // Initialize Debug Agent to support source level debug in SEC/PEI phases before memory ready.
  //
  InitializeDebugAgent (DEBUG_AGENT_INIT_PREMEM_SEC, &SecCoreData, SecStartupPhase2);
}

/**
  Caller provided function to be invoked at the end of InitializeDebugAgent().

  Entry point to the C language phase of SEC. After the SEC assembly
  code has initialized some temporary memory and set up the stack,
  the control is transferred to this function.

  @param[in] Context    The first input parameter of InitializeDebugAgent().

**/
VOID
EFIAPI
SecStartupPhase2 (
  IN VOID  *Context
  )
{
  EFI_SEC_PEI_HAND_OFF        *SecCoreData;
  EFI_FIRMWARE_VOLUME_HEADER  *BootFv;
  EFI_PEI_CORE_ENTRY_POINT    PeiCoreEntryPoint;
  EFI_PEI_PPI_DESCRIPTOR      *EfiPeiPpiDescriptor;

  SecCoreData = (EFI_SEC_PEI_HAND_OFF *)Context;

  //
  // Find PEI Core entry point. It will report SEC and Pei Core debug information if remote debug
  // is enabled.
  //
  BootFv = (EFI_FIRMWARE_VOLUME_HEADER *)SecCoreData->BootFirmwareVolumeBase;
  FindAndReportEntryPoints (&BootFv, &PeiCoreEntryPoint);
  SecCoreData->BootFirmwareVolumeBase = BootFv;
  SecCoreData->BootFirmwareVolumeSize = (UINTN)BootFv->FvLength;

  //
  // Td guest is required to use the MpInitLibUp (unique-processor version).
  // Other guests use the MpInitLib (multi-processor version).
  //
  if (CcProbe () == CcGuestTypeIntelTdx) {
    EfiPeiPpiDescriptor = (EFI_PEI_PPI_DESCRIPTOR *)&mPrivateDispatchTableUp;
  } else {
    EfiPeiPpiDescriptor = (EFI_PEI_PPI_DESCRIPTOR *)&mPrivateDispatchTableMp;
  }

  //
  // Transfer the control to the PEI core
  //
  (*PeiCoreEntryPoint)(SecCoreData, EfiPeiPpiDescriptor);

  //
  // If we get here then the PEI Core returned, which is not recoverable.
  //
  ASSERT (FALSE);
  CpuDeadLoop ();
}

EFI_STATUS
EFIAPI
TemporaryRamMigration (
  IN CONST EFI_PEI_SERVICES  **PeiServices,
  IN EFI_PHYSICAL_ADDRESS    TemporaryMemoryBase,
  IN EFI_PHYSICAL_ADDRESS    PermanentMemoryBase,
  IN UINTN                   CopySize
  )
{
  IA32_DESCRIPTOR                  IdtDescriptor;
  VOID                             *OldHeap;
  VOID                             *NewHeap;
  VOID                             *OldStack;
  VOID                             *NewStack;
  DEBUG_AGENT_CONTEXT_POSTMEM_SEC  DebugAgentContext;
  BOOLEAN                          OldStatus;
  BASE_LIBRARY_JUMP_BUFFER         JumpBuffer;

  DEBUG ((
    DEBUG_INFO,
    "TemporaryRamMigration(0x%Lx, 0x%Lx, 0x%Lx)\n",
    TemporaryMemoryBase,
    PermanentMemoryBase,
    (UINT64)CopySize
    ));

  OldHeap = (VOID *)(UINTN)TemporaryMemoryBase;
  NewHeap = (VOID *)((UINTN)PermanentMemoryBase + (CopySize >> 1));

  OldStack = (VOID *)((UINTN)TemporaryMemoryBase + (CopySize >> 1));
  NewStack = (VOID *)(UINTN)PermanentMemoryBase;

  DebugAgentContext.HeapMigrateOffset  = (UINTN)NewHeap - (UINTN)OldHeap;
  DebugAgentContext.StackMigrateOffset = (UINTN)NewStack - (UINTN)OldStack;

  OldStatus = SaveAndSetDebugTimerInterrupt (FALSE);
  InitializeDebugAgent (DEBUG_AGENT_INIT_POSTMEM_SEC, (VOID *)&DebugAgentContext, NULL);

  //
  // Migrate Heap
  //
  CopyMem (NewHeap, OldHeap, CopySize >> 1);

  //
  // Migrate Stack
  //
  CopyMem (NewStack, OldStack, CopySize >> 1);

  //
  // Rebase IDT table in permanent memory
  //
  AsmReadIdtr (&IdtDescriptor);
  IdtDescriptor.Base = IdtDescriptor.Base - (UINTN)OldStack + (UINTN)NewStack;

  AsmWriteIdtr (&IdtDescriptor);

  //
  // Use SetJump()/LongJump() to switch to a new stack.
  //
  if (SetJump (&JumpBuffer) == 0) {
 #if defined (MDE_CPU_IA32)
    JumpBuffer.Esp = JumpBuffer.Esp + DebugAgentContext.StackMigrateOffset;
    JumpBuffer.Ebp = JumpBuffer.Ebp + DebugAgentContext.StackMigrateOffset;
 #endif
 #if defined (MDE_CPU_X64)
    JumpBuffer.Rsp = JumpBuffer.Rsp + DebugAgentContext.StackMigrateOffset;
    JumpBuffer.Rbp = JumpBuffer.Rbp + DebugAgentContext.StackMigrateOffset;
 #endif
    LongJump (&JumpBuffer, (UINTN)-1);
  }

  SaveAndSetDebugTimerInterrupt (OldStatus);

  return EFI_SUCCESS;
}
