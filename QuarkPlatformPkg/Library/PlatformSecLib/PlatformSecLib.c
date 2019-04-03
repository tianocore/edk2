/** @file
Platform SEC Library for Quark.

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent


**/

#include <PiPei.h>

#include <Ppi/SecPlatformInformation.h>
#include <Ppi/TemporaryRamSupport.h>
#include <Library/PcdLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/HobLib.h>
#include <Library/MtrrLib.h>

/**

  Entry point to the C language phase of SEC. After the SEC assembly
  code has initialized some temporary memory and set up the stack,
  the control is transferred to this function.

  @param SizeOfRam           Size of the temporary memory available for use.
  @param TempRamBase         Base address of temporary ram
  @param BootFirmwareVolume  Base address of the Boot Firmware Volume.

**/
VOID
EFIAPI
SecStartup (
  IN UINT32                   SizeOfRam,
  IN UINT32                   TempRamBase,
  IN VOID                     *BootFirmwareVolume
  );

/**
  Auto-generated function that calls the library constructors for all of the module's
  dependent libraries.  This function must be called by the SEC Core once a stack has
  been established.

**/
VOID
EFIAPI
ProcessLibraryConstructorList (
  VOID
  );

/**

  Entry point to the C language phase of PlatformSecLib.  After the SEC assembly
  code has initialized some temporary memory and set up the stack, control is
  transferred to this function.

**/
VOID
EFIAPI
PlatformSecLibStartup (
  VOID
  )
{
  //
  // Process all library constructor functions linked to SecCore.
  // This function must be called before any library functions are called
  //
  ProcessLibraryConstructorList ();

  //
  // Set write back cache attribute for SPI FLASH
  //
  MtrrSetMemoryAttribute (
    PcdGet32 (PcdFlashAreaBaseAddress),
    PcdGet32 (PcdFlashAreaSize),
    CacheWriteBack
    );

  //
  // Set write back cache attribute for 512KB Embedded SRAM
  //
  MtrrSetMemoryAttribute (
    PcdGet32 (PcdEsramStage1Base),
    SIZE_512KB,
    CacheWriteBack
    );

  //
  // Pass control to SecCore module passing in the size of the temporary RAM in
  // Embedded SRAM, the base address of the temporary RAM in Embedded SRAM, and
  // the base address of the boot firmware volume.  The top 32KB of the 512 KB
  // embedded SRAM are used as temporary RAM.
  //
  SecStartup (
    SIZE_32KB,
    PcdGet32 (PcdEsramStage1Base) + SIZE_512KB - SIZE_32KB,
    (VOID *)(UINTN)PcdGet32 (PcdFlashFvRecoveryBase)
    );
}

/**
  A developer supplied function to perform platform specific operations.

  It's a developer supplied function to perform any operations appropriate to a
  given platform. It's invoked just before passing control to PEI core by SEC
  core. Platform developer may modify the SecCoreData and PPI list that is
  passed to PEI Core.

  @param  SecCoreData           The same parameter as passing to PEI core. It
                                could be overridden by this function.
  @param  PpiList               The default PPI list passed from generic SEC
                                part.

  @return The final PPI list that platform wishes to passed to PEI core.

**/
EFI_PEI_PPI_DESCRIPTOR *
EFIAPI
SecPlatformMain (
  IN OUT   EFI_SEC_PEI_HAND_OFF        *SecCoreData,
  IN       EFI_PEI_PPI_DESCRIPTOR      *PpiList
  )
{
  return NULL;
}

/**
  This interface conveys state information out of the Security (SEC) phase into PEI.

  @param  PeiServices               Pointer to the PEI Services Table.
  @param  StructureSize             Pointer to the variable describing size of the input buffer.
  @param  PlatformInformationRecord Pointer to the EFI_SEC_PLATFORM_INFORMATION_RECORD.

  @retval EFI_SUCCESS           The data was successfully returned.
  @retval EFI_BUFFER_TOO_SMALL  The buffer was too small.

**/
EFI_STATUS
EFIAPI
SecPlatformInformation (
  IN CONST EFI_PEI_SERVICES                     **PeiServices,
  IN OUT   UINT64                               *StructureSize,
     OUT   EFI_SEC_PLATFORM_INFORMATION_RECORD  *PlatformInformationRecord
  )
{
  UINT32             *BIST;
  UINT32             Size;
  UINT32             Count;
  EFI_HOB_GUID_TYPE  *GuidHob;
  UINT32             *TopOfStack;

  //
  // Top of the stack is the top of the 512KB Embedded SRAM region
  //
  TopOfStack = (UINT32 *)(UINTN)(PcdGet32 (PcdEsramStage1Base) + SIZE_512KB);

  GuidHob = GetFirstGuidHob (&gEfiSecPlatformInformationPpiGuid);
  if (GuidHob != NULL) {
    Size = GET_GUID_HOB_DATA_SIZE (GuidHob);
    BIST = GET_GUID_HOB_DATA (GuidHob);
  } else {
    //
    // The entries of BIST information, together with the number of them,
    // reside in the bottom of stack, left untouched by normal stack operation.
    // This routine copies the BIST information to the buffer pointed by
    // PlatformInformationRecord for output.
    //
    Count = *(TopOfStack - 1);
    Size  = Count * sizeof (IA32_HANDOFF_STATUS);
    BIST  = (UINT32 *) ((UINT32) TopOfStack - sizeof (UINT32) - Size);

    //
    // Copy Data from Stack to Hob to avoid data is lost after memory is ready.
    //
    BuildGuidDataHob (
      &gEfiSecPlatformInformationPpiGuid,
      BIST,
      (UINTN)Size
    );
    GuidHob = GetFirstGuidHob (&gEfiSecPlatformInformationPpiGuid);
    Size = GET_GUID_HOB_DATA_SIZE (GuidHob);
    BIST = GET_GUID_HOB_DATA (GuidHob);
  }

  if ((*StructureSize) < (UINT64) Size) {
    *StructureSize = Size;
    return EFI_BUFFER_TOO_SMALL;
  }

  *StructureSize  = Size;
  CopyMem (PlatformInformationRecord, BIST, Size);

  return EFI_SUCCESS;
}

/**
  This interface disables temporary memory in SEC Phase.
**/
VOID
EFIAPI
SecPlatformDisableTemporaryMemory (
  VOID
  )
{
}
