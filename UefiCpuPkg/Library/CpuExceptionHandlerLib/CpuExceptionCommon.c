/** @file
  CPU Exception Handler Library common functions.

  Copyright (c) 2012 - 2014, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "CpuExceptionCommon.h"

//
// Error code flag indicating whether or not an error code will be
// pushed on the stack if an exception occurs.
//
// 1 means an error code will be pushed, otherwise 0
//
CONST UINT32 mErrorCodeFlag             = 0x00027d00;
RESERVED_VECTORS_DATA *mReservedVectors = NULL;

//
// Define the maximum message length 
//
#define MAX_DEBUG_MESSAGE_LENGTH  0x100

/**
  Prints a message to the serial port.

  @param  Format      Format string for the message to print.
  @param  ...         Variable argument list whose contents are accessed 
                      based on the format string specified by Format.

**/
VOID
EFIAPI
InternalPrintMessage (
  IN  CONST CHAR8  *Format,
  ...
  )
{
  CHAR8    Buffer[MAX_DEBUG_MESSAGE_LENGTH];
  VA_LIST  Marker;

  //
  // Convert the message to an ASCII String
  //
  VA_START (Marker, Format);
  AsciiVSPrint (Buffer, sizeof (Buffer), Format, Marker);
  VA_END (Marker);

  //
  // Send the print string to a Serial Port 
  //
  SerialPortWrite ((UINT8 *)Buffer, AsciiStrLen (Buffer));
}

/**
  Find and display image base address and return image base and its entry point.
  
  @param CurrentEip      Current instruction pointer.
  @param EntryPoint      Return module entry point if module header is found.
  
  @return !0     Image base address.
  @return 0      Image header cannot be found.
**/
UINTN 
FindModuleImageBase (
  IN  UINTN              CurrentEip,
  OUT UINTN              *EntryPoint
  )
{
  UINTN                                Pe32Data;
  EFI_IMAGE_DOS_HEADER                 *DosHdr;
  EFI_IMAGE_OPTIONAL_HEADER_PTR_UNION  Hdr;
  VOID                                 *PdbPointer;

  //
  // Find Image Base
  //
  Pe32Data = CurrentEip & ~(mImageAlignSize - 1);
  while (Pe32Data != 0) {
    DosHdr = (EFI_IMAGE_DOS_HEADER *) Pe32Data;
    if (DosHdr->e_magic == EFI_IMAGE_DOS_SIGNATURE) {
      //
      // DOS image header is present, so read the PE header after the DOS image header.
      //
      Hdr.Pe32 = (EFI_IMAGE_NT_HEADERS32 *)(Pe32Data + (UINTN) ((DosHdr->e_lfanew) & 0x0ffff));
      //
      // Make sure PE header address does not overflow and is less than the initial address.
      //
      if (((UINTN)Hdr.Pe32 > Pe32Data) && ((UINTN)Hdr.Pe32 < CurrentEip)) {
        if (Hdr.Pe32->Signature == EFI_IMAGE_NT_SIGNATURE) {
          //
          // It's PE image.
          //
          InternalPrintMessage ("!!!! Find PE image ");
          *EntryPoint = (UINTN)Pe32Data + (UINTN)(Hdr.Pe32->OptionalHeader.AddressOfEntryPoint & 0x0ffffffff);
          break;
        }
      }
    } else {
      //
      // DOS image header is not present, TE header is at the image base.
      //
      Hdr.Pe32 = (EFI_IMAGE_NT_HEADERS32 *)Pe32Data;
      if ((Hdr.Te->Signature == EFI_TE_IMAGE_HEADER_SIGNATURE) &&
          ((Hdr.Te->Machine == IMAGE_FILE_MACHINE_I386) || Hdr.Te->Machine == IMAGE_FILE_MACHINE_X64)) {
        //
        // It's TE image, it TE header and Machine type match
        //
        InternalPrintMessage ("!!!! Find TE image ");
        *EntryPoint = (UINTN)Pe32Data + (UINTN)(Hdr.Te->AddressOfEntryPoint & 0x0ffffffff) + sizeof(EFI_TE_IMAGE_HEADER) - Hdr.Te->StrippedSize;
        break;
      }
    }

    //
    // Not found the image base, check the previous aligned address
    // 
    Pe32Data -= mImageAlignSize;
  }

  if (Pe32Data != 0) {
    PdbPointer = PeCoffLoaderGetPdbPointer ((VOID *) Pe32Data);
    if (PdbPointer != NULL) {
      InternalPrintMessage ("%a", PdbPointer);
    } else {
      InternalPrintMessage ("(No PDB) " );
    }
  } else {
    InternalPrintMessage ("!!!! Can't find image information. !!!!\n");
  }

  return Pe32Data;
}

/**
  Read and save reserved vector information
  
  @param[in]  VectorInfo        Pointer to reserved vector list.
  @param[out] ReservedVector    Pointer to reserved vector data buffer.
  @param[in]  VectorCount       Vector number to be updated.
  
  @return EFI_SUCCESS           Read and save vector info successfully.
  @retval EFI_INVALID_PARAMETER VectorInfo includes the invalid content if VectorInfo is not NULL.

**/
EFI_STATUS
ReadAndVerifyVectorInfo (
  IN  EFI_VECTOR_HANDOFF_INFO       *VectorInfo,
  OUT RESERVED_VECTORS_DATA         *ReservedVector,
  IN  UINTN                         VectorCount
  )
{
  while (VectorInfo->Attribute != EFI_VECTOR_HANDOFF_LAST_ENTRY) {
    if (VectorInfo->Attribute > EFI_VECTOR_HANDOFF_HOOK_AFTER) {
      //
      // If vector attrubute is invalid
      //
      return EFI_INVALID_PARAMETER;
    }
    if (VectorInfo->VectorNumber < VectorCount) {
      ReservedVector[VectorInfo->VectorNumber].Attribute = VectorInfo->Attribute;
    }
    VectorInfo ++;
  }
  return EFI_SUCCESS;
}