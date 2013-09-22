/** @file
  This module implements measuring PeCoff image for TrEE Protocol.
  
  Caution: This file requires additional review when modified.
  This driver will have external input - PE/COFF image.
  This external input must be validated carefully to avoid security issue like
  buffer overflow, integer overflow.

Copyright (c) 2013, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials 
are licensed and made available under the terms and conditions of the BSD License 
which accompanies this distribution.  The full text of the license may be found at 
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, 
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiDxe.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DevicePathLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/PeCoffLib.h>
#include <Library/Tpm2CommandLib.h>
#include <Library/HashLib.h>

/**
  Measure PE image into TPM log based on the authenticode image hashing in
  PE/COFF Specification 8.0 Appendix A.

  Caution: This function may receive untrusted input.
  PE/COFF image is external input, so this function will validate its data structure
  within this image buffer before use.

  @param[in]  PCRIndex       TPM PCR index
  @param[in]  ImageAddress   Start address of image buffer.
  @param[in]  ImageSize      Image size
  @param[out] DigestList     Digeest list of this image.

  @retval EFI_SUCCESS            Successfully measure image.
  @retval EFI_OUT_OF_RESOURCES   No enough resource to measure image.
  @retval other error value
**/
EFI_STATUS
MeasurePeImageAndExtend (
  IN  UINT32                    PCRIndex,
  IN  EFI_PHYSICAL_ADDRESS      ImageAddress,
  IN  UINTN                     ImageSize,
  OUT TPML_DIGEST_VALUES        *DigestList
  )
{
  EFI_STATUS                           Status;
  EFI_IMAGE_DOS_HEADER                 *DosHdr;
  UINT32                               PeCoffHeaderOffset;
  EFI_IMAGE_SECTION_HEADER             *Section;
  UINT8                                *HashBase;
  UINTN                                HashSize;
  UINTN                                SumOfBytesHashed;
  EFI_IMAGE_SECTION_HEADER             *SectionHeader;
  UINTN                                Index;
  UINTN                                Pos;
  UINT16                               Magic;
  EFI_IMAGE_OPTIONAL_HEADER_PTR_UNION  Hdr;
  UINT32                               NumberOfRvaAndSizes;
  UINT32                               CertSize;
  HASH_HANDLE                          HashHandle;

  HashHandle = 0xFFFFFFFF; // Know bad value

  Status        = EFI_UNSUPPORTED;
  SectionHeader = NULL;

  //
  // Check PE/COFF image
  //
  DosHdr = (EFI_IMAGE_DOS_HEADER *) (UINTN) ImageAddress;
  PeCoffHeaderOffset = 0;
  if (DosHdr->e_magic == EFI_IMAGE_DOS_SIGNATURE) {
    PeCoffHeaderOffset = DosHdr->e_lfanew;
  }

  Hdr.Pe32 = (EFI_IMAGE_NT_HEADERS32 *)((UINT8 *) (UINTN) ImageAddress + PeCoffHeaderOffset);
  if (Hdr.Pe32->Signature != EFI_IMAGE_NT_SIGNATURE) {
    Status = EFI_UNSUPPORTED;
    goto Finish;
  }

  //
  // PE/COFF Image Measurement
  //
  //    NOTE: The following codes/steps are based upon the authenticode image hashing in
  //      PE/COFF Specification 8.0 Appendix A.
  //
  //

  // 1.  Load the image header into memory.

  // 2.  Initialize a SHA hash context.

  Status = HashStart (&HashHandle);
  if (EFI_ERROR (Status)) {
    goto Finish;
  }

  //
  // Measuring PE/COFF Image Header;
  // But CheckSum field and SECURITY data directory (certificate) are excluded
  //
  if (Hdr.Pe32->FileHeader.Machine == IMAGE_FILE_MACHINE_IA64 && Hdr.Pe32->OptionalHeader.Magic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
    //
    // NOTE: Some versions of Linux ELILO for Itanium have an incorrect magic value 
    //       in the PE/COFF Header. If the MachineType is Itanium(IA64) and the 
    //       Magic value in the OptionalHeader is EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC
    //       then override the magic value to EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC
    //
    Magic = EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC;
  } else {
    //
    // Get the magic value from the PE/COFF Optional Header
    //
    Magic = Hdr.Pe32->OptionalHeader.Magic;
  }
  
  //
  // 3.  Calculate the distance from the base of the image header to the image checksum address.
  // 4.  Hash the image header from its base to beginning of the image checksum.
  //
  HashBase = (UINT8 *) (UINTN) ImageAddress;
  if (Magic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
    //
    // Use PE32 offset
    //
    NumberOfRvaAndSizes = Hdr.Pe32->OptionalHeader.NumberOfRvaAndSizes;
    HashSize = (UINTN) ((UINT8 *)(&Hdr.Pe32->OptionalHeader.CheckSum) - HashBase);
  } else {
    //
    // Use PE32+ offset
    //
    NumberOfRvaAndSizes = Hdr.Pe32Plus->OptionalHeader.NumberOfRvaAndSizes;
    HashSize = (UINTN) ((UINT8 *)(&Hdr.Pe32Plus->OptionalHeader.CheckSum) - HashBase);
  }

  Status = HashUpdate (HashHandle, HashBase, HashSize);
  if (EFI_ERROR (Status)) {
    goto Finish;
  }  

  //
  // 5.  Skip over the image checksum (it occupies a single ULONG).
  //
  if (NumberOfRvaAndSizes <= EFI_IMAGE_DIRECTORY_ENTRY_SECURITY) {
    //
    // 6.  Since there is no Cert Directory in optional header, hash everything
    //     from the end of the checksum to the end of image header.
    //
    if (Magic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
      //
      // Use PE32 offset.
      //
      HashBase = (UINT8 *) &Hdr.Pe32->OptionalHeader.CheckSum + sizeof (UINT32);
      HashSize = Hdr.Pe32->OptionalHeader.SizeOfHeaders - (UINTN) (HashBase - ImageAddress);
    } else {
      //
      // Use PE32+ offset.
      //
      HashBase = (UINT8 *) &Hdr.Pe32Plus->OptionalHeader.CheckSum + sizeof (UINT32);
      HashSize = Hdr.Pe32Plus->OptionalHeader.SizeOfHeaders - (UINTN) (HashBase - ImageAddress);
    }

    if (HashSize != 0) {
      Status  = HashUpdate (HashHandle, HashBase, HashSize);
      if (EFI_ERROR (Status)) {
        goto Finish;
      }
    }    
  } else {
    //
    // 7.  Hash everything from the end of the checksum to the start of the Cert Directory.
    //
    if (Magic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
      //
      // Use PE32 offset
      //
      HashBase = (UINT8 *) &Hdr.Pe32->OptionalHeader.CheckSum + sizeof (UINT32);
      HashSize = (UINTN) ((UINT8 *)(&Hdr.Pe32->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_SECURITY]) - HashBase);
    } else {
      //
      // Use PE32+ offset
      //    
      HashBase = (UINT8 *) &Hdr.Pe32Plus->OptionalHeader.CheckSum + sizeof (UINT32);
      HashSize = (UINTN) ((UINT8 *)(&Hdr.Pe32Plus->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_SECURITY]) - HashBase);
    }

    if (HashSize != 0) {
      Status  = HashUpdate (HashHandle, HashBase, HashSize);
      if (EFI_ERROR (Status)) {
        goto Finish;
      }
    }

    //
    // 8.  Skip over the Cert Directory. (It is sizeof(IMAGE_DATA_DIRECTORY) bytes.)
    // 9.  Hash everything from the end of the Cert Directory to the end of image header.
    //
    if (Magic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
      //
      // Use PE32 offset
      //
      HashBase = (UINT8 *) &Hdr.Pe32->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_SECURITY + 1];
      HashSize = Hdr.Pe32->OptionalHeader.SizeOfHeaders - (UINTN) (HashBase - ImageAddress);
    } else {
      //
      // Use PE32+ offset
      //
      HashBase = (UINT8 *) &Hdr.Pe32Plus->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_SECURITY + 1];
      HashSize = Hdr.Pe32Plus->OptionalHeader.SizeOfHeaders - (UINTN) (HashBase - ImageAddress);
    }
    
    if (HashSize != 0) {
      Status  = HashUpdate (HashHandle, HashBase, HashSize);
      if (EFI_ERROR (Status)) {
        goto Finish;
      }
    }
  }

  //
  // 10. Set the SUM_OF_BYTES_HASHED to the size of the header
  //
  if (Magic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
    //
    // Use PE32 offset
    //
    SumOfBytesHashed = Hdr.Pe32->OptionalHeader.SizeOfHeaders;
  } else {
    //
    // Use PE32+ offset
    //
    SumOfBytesHashed = Hdr.Pe32Plus->OptionalHeader.SizeOfHeaders;
  }

  //
  // 11. Build a temporary table of pointers to all the IMAGE_SECTION_HEADER
  //     structures in the image. The 'NumberOfSections' field of the image
  //     header indicates how big the table should be. Do not include any
  //     IMAGE_SECTION_HEADERs in the table whose 'SizeOfRawData' field is zero.
  //
  SectionHeader = (EFI_IMAGE_SECTION_HEADER *) AllocateZeroPool (sizeof (EFI_IMAGE_SECTION_HEADER) * Hdr.Pe32->FileHeader.NumberOfSections);
  if (SectionHeader == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Finish;
  }

  //
  // 12.  Using the 'PointerToRawData' in the referenced section headers as
  //      a key, arrange the elements in the table in ascending order. In other
  //      words, sort the section headers according to the disk-file offset of
  //      the section.
  //
  Section = (EFI_IMAGE_SECTION_HEADER *) (
               (UINT8 *) (UINTN) ImageAddress +
               PeCoffHeaderOffset +
               sizeof(UINT32) +
               sizeof(EFI_IMAGE_FILE_HEADER) +
               Hdr.Pe32->FileHeader.SizeOfOptionalHeader
               );
  for (Index = 0; Index < Hdr.Pe32->FileHeader.NumberOfSections; Index++) {
    Pos = Index;
    while ((Pos > 0) && (Section->PointerToRawData < SectionHeader[Pos - 1].PointerToRawData)) {
      CopyMem (&SectionHeader[Pos], &SectionHeader[Pos - 1], sizeof(EFI_IMAGE_SECTION_HEADER));
      Pos--;
    }
    CopyMem (&SectionHeader[Pos], Section, sizeof(EFI_IMAGE_SECTION_HEADER));
    Section += 1;
  }

  //
  // 13.  Walk through the sorted table, bring the corresponding section
  //      into memory, and hash the entire section (using the 'SizeOfRawData'
  //      field in the section header to determine the amount of data to hash).
  // 14.  Add the section's 'SizeOfRawData' to SUM_OF_BYTES_HASHED .
  // 15.  Repeat steps 13 and 14 for all the sections in the sorted table.
  //
  for (Index = 0; Index < Hdr.Pe32->FileHeader.NumberOfSections; Index++) {
    Section  = (EFI_IMAGE_SECTION_HEADER *) &SectionHeader[Index];
    if (Section->SizeOfRawData == 0) {
      continue;
    }
    HashBase = (UINT8 *) (UINTN) ImageAddress + Section->PointerToRawData;
    HashSize = (UINTN) Section->SizeOfRawData;

    Status = HashUpdate (HashHandle, HashBase, HashSize);
    if (EFI_ERROR (Status)) {
      goto Finish;
    }

    SumOfBytesHashed += HashSize;
  }

  //
  // 16.  If the file size is greater than SUM_OF_BYTES_HASHED, there is extra
  //      data in the file that needs to be added to the hash. This data begins
  //      at file offset SUM_OF_BYTES_HASHED and its length is:
  //             FileSize  -  (CertDirectory->Size)
  //
  if (ImageSize > SumOfBytesHashed) {
    HashBase = (UINT8 *) (UINTN) ImageAddress + SumOfBytesHashed;

    if (NumberOfRvaAndSizes <= EFI_IMAGE_DIRECTORY_ENTRY_SECURITY) {
      CertSize = 0;
    } else {
      if (Magic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
        //
        // Use PE32 offset.
        //
        CertSize = Hdr.Pe32->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_SECURITY].Size;
      } else {
        //
        // Use PE32+ offset.
        //
        CertSize = Hdr.Pe32Plus->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_SECURITY].Size;
      }
    }

    if (ImageSize > CertSize + SumOfBytesHashed) {
      HashSize = (UINTN) (ImageSize - CertSize - SumOfBytesHashed);

      Status = HashUpdate (HashHandle, HashBase, HashSize);
      if (EFI_ERROR (Status)) {
        goto Finish;
      }
    } else if (ImageSize < CertSize + SumOfBytesHashed) {
      Status = EFI_UNSUPPORTED;
      goto Finish;
    }
  }

  //
  // 17.  Finalize the SHA hash.
  //
  Status = HashCompleteAndExtend (HashHandle, PCRIndex, NULL, 0, DigestList);
  if (EFI_ERROR (Status)) {
    goto Finish;
  }

Finish:
  if (SectionHeader != NULL) {
    FreePool (SectionHeader);
  }

  return Status;
}
