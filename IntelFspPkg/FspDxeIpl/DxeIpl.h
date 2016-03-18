/** @file

  Copyright (c) 2014, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __PEI_DXEIPL_H__
#define __PEI_DXEIPL_H__

#include <PiPei.h>
#include <Ppi/DxeIpl.h>
#include <Ppi/EndOfPeiPhase.h>
#include <Ppi/MemoryDiscovered.h>
#include <Ppi/Decompress.h>
#include <Ppi/FirmwareVolumeInfo.h>
#include <Ppi/GuidedSectionExtraction.h>

#include <Library/DebugLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/BaseLib.h>
#include <Library/HobLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/UefiDecompressLib.h>
#include <Library/ExtractGuidedSectionLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/FspSwitchStackLib.h>
#include <Library/FspCommonLib.h>
#include <Library/FspPlatformLib.h>

/**
   Main entry point to last PEIM.

   This function finds DXE Core in the firmware volume and transfer the control to
   DXE core.

   @param[in] This          Entry point for DXE IPL PPI.
   @param[in] PeiServices   General purpose services available to every PEIM.
   @param[in] HobList       Address to the Pei HOB list.

   @return EFI_SUCCESS              DXE core was successfully loaded.
   @return EFI_OUT_OF_RESOURCES     There are not enough resources to load DXE core.

**/
EFI_STATUS
EFIAPI
DxeLoadCore (
  IN CONST EFI_DXE_IPL_PPI *This,
  IN EFI_PEI_SERVICES      **PeiServices,
  IN EFI_PEI_HOB_POINTERS  HobList
  );



/**
   Transfers control to DxeCore.

   This function performs a CPU architecture specific operations to execute
   the entry point of DxeCore with the parameters of HobList.
   It also installs EFI_END_OF_PEI_PPI to signal the end of PEI phase.

   @param[in] DxeCoreEntryPoint         The entry point of DxeCore.
   @param[in] HobList                   The start of HobList passed to DxeCore.

**/
VOID
HandOffToDxeCore (
  IN EFI_PHYSICAL_ADDRESS   DxeCoreEntryPoint,
  IN EFI_PEI_HOB_POINTERS   HobList
  );



/**
   Updates the Stack HOB passed to DXE phase.

   This function traverses the whole HOB list and update the stack HOB to
   reflect the real stack that is used by DXE core.

   @param[in] BaseAddress           The lower address of stack used by DxeCore.
   @param[in] Length                The length of stack used by DxeCore.

**/
VOID
UpdateStackHob (
  IN EFI_PHYSICAL_ADDRESS        BaseAddress,
  IN UINT64                      Length
  );

/**
  The ExtractSection() function processes the input section and
  returns a pointer to the section contents. If the section being
  extracted does not require processing (if the section
  GuidedSectionHeader.Attributes has the
  EFI_GUIDED_SECTION_PROCESSING_REQUIRED field cleared), then
  OutputBuffer is just updated to point to the start of the
  section's contents. Otherwise, *Buffer must be allocated
  from PEI permanent memory.

  @param[in]  This                   Indicates the
                                     EFI_PEI_GUIDED_SECTION_EXTRACTION_PPI instance.
                                     Buffer containing the input GUIDed section to be
                                     processed. OutputBuffer OutputBuffer is
                                     allocated from PEI permanent memory and contains
                                     the new section stream.
  @param[in]  InputSection           A pointer to the input buffer, which contains
                                     the input section to be processed.
  @param[out] OutputBuffer           A pointer to a caller-allocated buffer, whose
                                     size is specified by the contents of OutputSize.
  @param[out] OutputSize             A pointer to a caller-allocated
                                     UINTN in which the size of *OutputBuffer
                                     allocation is stored. If the function
                                     returns anything other than EFI_SUCCESS,
                                     the value of OutputSize is undefined.
  @param[out] AuthenticationStatus   A pointer to a caller-allocated
                                     UINT32 that indicates the
                                     authentication status of the
                                     output buffer. If the input
                                     section's GuidedSectionHeader.
                                     Attributes field has the
                                     EFI_GUIDED_SECTION_AUTH_STATUS_VALID
                                     bit as clear,
                                     AuthenticationStatus must return
                                     zero. These bits reflect the
                                     status of the extraction
                                     operation. If the function
                                     returns anything other than
                                     EFI_SUCCESS, the value of
                                     AuthenticationStatus is
                                     undefined.

  @retval EFI_SUCCESS           The InputSection was
                                successfully processed and the
                                section contents were returned.

  @retval EFI_OUT_OF_RESOURCES  The system has insufficient
                                resources to process the request.

  @retval EFI_INVALID_PARAMETER The GUID in InputSection does
                                not match this instance of the
                                GUIDed Section Extraction PPI.

**/
EFI_STATUS
EFIAPI
CustomGuidedSectionExtract (
  IN CONST  EFI_PEI_GUIDED_SECTION_EXTRACTION_PPI *This,
  IN CONST  VOID                                  *InputSection,
  OUT       VOID                                  **OutputBuffer,
  OUT       UINTN                                 *OutputSize,
  OUT       UINT32                                *AuthenticationStatus
  );

/**
   Decompresses a section to the output buffer.

   This function looks up the compression type field in the input section and
   applies the appropriate compression algorithm to compress the section to a
   callee allocated buffer.

   @param[in]  This                  Points to this instance of the
                                     EFI_PEI_DECOMPRESS_PEI PPI.
   @param[in]  CompressionSection    Points to the compressed section.
   @param[out] OutputBuffer          Holds the returned pointer to the decompressed
                                     sections.
   @param[out] OutputSize            Holds the returned size of the decompress
                                     section streams.

   @retval EFI_SUCCESS           The section was decompressed successfully.
                                 OutputBuffer contains the resulting data and
                                 OutputSize contains the resulting size.

**/
EFI_STATUS
EFIAPI
Decompress (
  IN CONST  EFI_PEI_DECOMPRESS_PPI  *This,
  IN CONST  EFI_COMPRESSION_SECTION *CompressionSection,
  OUT       VOID                    **OutputBuffer,
  OUT       UINTN                   *OutputSize
  );

#endif
