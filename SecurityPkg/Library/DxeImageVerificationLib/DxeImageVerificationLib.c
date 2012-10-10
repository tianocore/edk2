/** @file
  Implement image verification services for secure boot service in UEFI2.3.1.

  Caution: This file requires additional review when modified.
  This library will have external input - PE/COFF image.
  This external input must be validated carefully to avoid security issue like
  buffer overflow, integer overflow.

  DxeImageVerificationLibImageRead() function will make sure the PE/COFF image content
  read is within the image buffer.

  DxeImageVerificationHandler(), HashPeImageByType(), HashPeImage() function will accept
  untrusted PE/COFF image and validate its data structure within this image buffer before use.

Copyright (c) 2009 - 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "DxeImageVerificationLib.h"

//
// Caution: This is used by a function which may receive untrusted input.
// These global variables hold PE/COFF image data, and they should be validated before use.
//
EFI_IMAGE_OPTIONAL_HEADER_PTR_UNION mNtHeader;
UINT32                              mPeCoffHeaderOffset;
EFI_IMAGE_DATA_DIRECTORY            *mSecDataDir      = NULL;
EFI_GUID                            mCertType;

//
// Information on current PE/COFF image
//
UINTN                               mImageSize;
UINT8                               *mImageBase       = NULL;
UINT8                               mImageDigest[MAX_DIGEST_SIZE];
UINTN                               mImageDigestSize;

//
// Notify string for authorization UI.
//
CHAR16  mNotifyString1[MAX_NOTIFY_STRING_LEN] = L"Image verification pass but not found in authorized database!";
CHAR16  mNotifyString2[MAX_NOTIFY_STRING_LEN] = L"Launch this image anyway? (Yes/Defer/No)";
//
// Public Exponent of RSA Key.
//
CONST UINT8 mRsaE[] = { 0x01, 0x00, 0x01 };


//
// OID ASN.1 Value for Hash Algorithms
//
UINT8 mHashOidValue[] = {
  0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x02, 0x05,         // OBJ_md5
  0x2B, 0x0E, 0x03, 0x02, 0x1A,                           // OBJ_sha1
  0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x04,   // OBJ_sha224
  0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x01,   // OBJ_sha256
  0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x02,   // OBJ_sha384
  0x60, 0x86, 0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x03,   // OBJ_sha512
  };

HASH_TABLE mHash[] = {
  { L"SHA1",   20, &mHashOidValue[8],  5, Sha1GetContextSize,  Sha1Init,   Sha1Update,    Sha1Final  },
  { L"SHA224", 28, &mHashOidValue[13], 9, NULL,                NULL,       NULL,          NULL       },
  { L"SHA256", 32, &mHashOidValue[22], 9, Sha256GetContextSize,Sha256Init, Sha256Update,  Sha256Final},
  { L"SHA384", 48, &mHashOidValue[31], 9, NULL,                NULL,       NULL,          NULL       },
  { L"SHA512", 64, &mHashOidValue[40], 9, NULL,                NULL,       NULL,          NULL       }
};

/**
  Reads contents of a PE/COFF image in memory buffer.

  Caution: This function may receive untrusted input.
  PE/COFF image is external input, so this function will make sure the PE/COFF image content
  read is within the image buffer.

  @param  FileHandle      Pointer to the file handle to read the PE/COFF image.
  @param  FileOffset      Offset into the PE/COFF image to begin the read operation.
  @param  ReadSize        On input, the size in bytes of the requested read operation.  
                          On output, the number of bytes actually read.
  @param  Buffer          Output buffer that contains the data read from the PE/COFF image.
  
  @retval EFI_SUCCESS     The specified portion of the PE/COFF image was read and the size 
**/
EFI_STATUS
EFIAPI
DxeImageVerificationLibImageRead (
  IN     VOID    *FileHandle,
  IN     UINTN   FileOffset,
  IN OUT UINTN   *ReadSize,
  OUT    VOID    *Buffer
  )
{
  UINTN               EndPosition;

  if (FileHandle == NULL || ReadSize == NULL || Buffer == NULL) {
    return EFI_INVALID_PARAMETER;    
  }

  if (MAX_ADDRESS - FileOffset < *ReadSize) {
    return EFI_INVALID_PARAMETER;
  }

  EndPosition = FileOffset + *ReadSize;
  if (EndPosition > mImageSize) {
    *ReadSize = (UINT32)(mImageSize - FileOffset);
  }

  if (FileOffset >= mImageSize) {
    *ReadSize = 0;
  }

  CopyMem (Buffer, (UINT8 *)((UINTN) FileHandle + FileOffset), *ReadSize);

  return EFI_SUCCESS;
}


/**
  Get the image type.

  @param[in]    File       This is a pointer to the device path of the file that is
                           being dispatched.

  @return UINT32           Image Type

**/
UINT32
GetImageType (
  IN  CONST EFI_DEVICE_PATH_PROTOCOL   *File
  )
{
  EFI_STATUS                        Status;
  EFI_HANDLE                        DeviceHandle;
  EFI_DEVICE_PATH_PROTOCOL          *TempDevicePath;
  EFI_BLOCK_IO_PROTOCOL             *BlockIo;

  if (File == NULL) {
    return IMAGE_UNKNOWN;
  }

  //
  // First check to see if File is from a Firmware Volume
  //
  DeviceHandle      = NULL;
  TempDevicePath = (EFI_DEVICE_PATH_PROTOCOL *) File;
  Status = gBS->LocateDevicePath (
                  &gEfiFirmwareVolume2ProtocolGuid,
                  &TempDevicePath,
                  &DeviceHandle
                  );
  if (!EFI_ERROR (Status)) {
    Status = gBS->OpenProtocol (
                    DeviceHandle,
                    &gEfiFirmwareVolume2ProtocolGuid,
                    NULL,
                    NULL,
                    NULL,
                    EFI_OPEN_PROTOCOL_TEST_PROTOCOL
                    );
    if (!EFI_ERROR (Status)) {
      return IMAGE_FROM_FV;
    }
  }

  //
  // Next check to see if File is from a Block I/O device
  //
  DeviceHandle   = NULL;
  TempDevicePath = (EFI_DEVICE_PATH_PROTOCOL *) File;
  Status = gBS->LocateDevicePath (
                  &gEfiBlockIoProtocolGuid,
                  &TempDevicePath,
                  &DeviceHandle
                  );
  if (!EFI_ERROR (Status)) {
    BlockIo = NULL;
    Status = gBS->OpenProtocol (
                    DeviceHandle,
                    &gEfiBlockIoProtocolGuid,
                    (VOID **) &BlockIo,
                    NULL,
                    NULL,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if (!EFI_ERROR (Status) && BlockIo != NULL) {
      if (BlockIo->Media != NULL) {
        if (BlockIo->Media->RemovableMedia) {
          //
          // Block I/O is present and specifies the media is removable
          //
          return IMAGE_FROM_REMOVABLE_MEDIA;
        } else {
          //
          // Block I/O is present and specifies the media is not removable
          //
          return IMAGE_FROM_FIXED_MEDIA;
        }
      }
    }
  }

  //
  // File is not in a Firmware Volume or on a Block I/O device, so check to see if
  // the device path supports the Simple File System Protocol.
  //
  DeviceHandle   = NULL;
  TempDevicePath = (EFI_DEVICE_PATH_PROTOCOL *) File;
  Status = gBS->LocateDevicePath (
                  &gEfiSimpleFileSystemProtocolGuid,
                  &TempDevicePath,
                  &DeviceHandle
                  );
  if (!EFI_ERROR (Status)) {
    //
    // Simple File System is present without Block I/O, so assume media is fixed.
    //
    return IMAGE_FROM_FIXED_MEDIA;
  }

  //
  // File is not from an FV, Block I/O or Simple File System, so the only options
  // left are a PCI Option ROM and a Load File Protocol such as a PXE Boot from a NIC.
  //
  TempDevicePath = (EFI_DEVICE_PATH_PROTOCOL *) File;
  while (!IsDevicePathEndType (TempDevicePath)) {
    switch (DevicePathType (TempDevicePath)) {

    case MEDIA_DEVICE_PATH:
      if (DevicePathSubType (TempDevicePath) == MEDIA_RELATIVE_OFFSET_RANGE_DP) {
        return IMAGE_FROM_OPTION_ROM;
      }
      break;

    case MESSAGING_DEVICE_PATH:
      if (DevicePathSubType(TempDevicePath) == MSG_MAC_ADDR_DP) {
        return IMAGE_FROM_REMOVABLE_MEDIA;
      }
      break;

    default:
      break;
    }
    TempDevicePath = NextDevicePathNode (TempDevicePath);
  }
  return IMAGE_UNKNOWN;
}

/**
  Caculate hash of Pe/Coff image based on the authenticode image hashing in
  PE/COFF Specification 8.0 Appendix A

  Caution: This function may receive untrusted input.
  PE/COFF image is external input, so this function will validate its data structure
  within this image buffer before use.

  @param[in]    HashAlg   Hash algorithm type.

  @retval TRUE            Successfully hash image.
  @retval FALSE           Fail in hash image.

**/
BOOLEAN
HashPeImage (
  IN  UINT32              HashAlg
  )
{
  BOOLEAN                   Status;
  UINT16                    Magic;
  EFI_IMAGE_SECTION_HEADER  *Section;
  VOID                      *HashCtx;
  UINTN                     CtxSize;
  UINT8                     *HashBase;
  UINTN                     HashSize;
  UINTN                     SumOfBytesHashed;
  EFI_IMAGE_SECTION_HEADER  *SectionHeader;
  UINTN                     Index;
  UINTN                     Pos;
  UINT32                    CertSize;
  UINT32                    NumberOfRvaAndSizes;

  HashCtx       = NULL;
  SectionHeader = NULL;
  Status        = FALSE;

  if ((HashAlg != HASHALG_SHA1) && (HashAlg != HASHALG_SHA256)) {
    return FALSE;
  }

  //
  // Initialize context of hash.
  //
  ZeroMem (mImageDigest, MAX_DIGEST_SIZE);

  if (HashAlg == HASHALG_SHA1) {
    mImageDigestSize  = SHA1_DIGEST_SIZE;
    mCertType         = gEfiCertSha1Guid;
  } else if (HashAlg == HASHALG_SHA256) {
    mImageDigestSize  = SHA256_DIGEST_SIZE;
    mCertType         = gEfiCertSha256Guid;
  } else {
    return FALSE;
  }

  CtxSize   = mHash[HashAlg].GetContextSize();

  HashCtx = AllocatePool (CtxSize);
  if (HashCtx == NULL) {
    return FALSE;
  }

  // 1.  Load the image header into memory.

  // 2.  Initialize a SHA hash context.
  Status = mHash[HashAlg].HashInit(HashCtx);

  if (!Status) {
    goto Done;
  }

  //
  // Measuring PE/COFF Image Header;
  // But CheckSum field and SECURITY data directory (certificate) are excluded
  //
  Magic = mNtHeader.Pe32->OptionalHeader.Magic;
  //
  // 3.  Calculate the distance from the base of the image header to the image checksum address.
  // 4.  Hash the image header from its base to beginning of the image checksum.
  //
  HashBase = mImageBase;
  if (Magic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
    //
    // Use PE32 offset.
    //
    HashSize = (UINTN) ((UINT8 *) (&mNtHeader.Pe32->OptionalHeader.CheckSum) - HashBase);
    NumberOfRvaAndSizes = mNtHeader.Pe32->OptionalHeader.NumberOfRvaAndSizes;
  } else if (Magic == EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
    //
    // Use PE32+ offset.
    //
    HashSize = (UINTN) ((UINT8 *) (&mNtHeader.Pe32Plus->OptionalHeader.CheckSum) - HashBase);
    NumberOfRvaAndSizes = mNtHeader.Pe32Plus->OptionalHeader.NumberOfRvaAndSizes;
  } else {
    //
    // Invalid header magic number.
    //
    Status = FALSE;
    goto Done;
  }

  Status  = mHash[HashAlg].HashUpdate(HashCtx, HashBase, HashSize);
  if (!Status) {
    goto Done;
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
      HashBase = (UINT8 *) &mNtHeader.Pe32->OptionalHeader.CheckSum + sizeof (UINT32);
      HashSize = mNtHeader.Pe32->OptionalHeader.SizeOfHeaders - (UINTN) (HashBase - mImageBase);
    } else {
      //
      // Use PE32+ offset.
      //
      HashBase = (UINT8 *) &mNtHeader.Pe32Plus->OptionalHeader.CheckSum + sizeof (UINT32);
      HashSize = mNtHeader.Pe32Plus->OptionalHeader.SizeOfHeaders - (UINTN) (HashBase - mImageBase);
    }

    if (HashSize != 0) {
      Status  = mHash[HashAlg].HashUpdate(HashCtx, HashBase, HashSize);
      if (!Status) {
        goto Done;
      }
    }
  } else {
    //
    // 7.  Hash everything from the end of the checksum to the start of the Cert Directory.
    //
    if (Magic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
      //
      // Use PE32 offset.
      //
      HashBase = (UINT8 *) &mNtHeader.Pe32->OptionalHeader.CheckSum + sizeof (UINT32);
      HashSize = (UINTN) ((UINT8 *) (&mNtHeader.Pe32->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_SECURITY]) - HashBase);
    } else {
      //
      // Use PE32+ offset.
      //
      HashBase = (UINT8 *) &mNtHeader.Pe32Plus->OptionalHeader.CheckSum + sizeof (UINT32);
      HashSize = (UINTN) ((UINT8 *) (&mNtHeader.Pe32Plus->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_SECURITY]) - HashBase);
    }

    if (HashSize != 0) {
      Status  = mHash[HashAlg].HashUpdate(HashCtx, HashBase, HashSize);
      if (!Status) {
        goto Done;
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
      HashBase = (UINT8 *) &mNtHeader.Pe32->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_SECURITY + 1];
      HashSize = mNtHeader.Pe32->OptionalHeader.SizeOfHeaders - (UINTN) (HashBase - mImageBase);
    } else {
      //
      // Use PE32+ offset.
      //
      HashBase = (UINT8 *) &mNtHeader.Pe32Plus->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_SECURITY + 1];
      HashSize = mNtHeader.Pe32Plus->OptionalHeader.SizeOfHeaders - (UINTN) (HashBase - mImageBase);
    }

    if (HashSize != 0) {
      Status  = mHash[HashAlg].HashUpdate(HashCtx, HashBase, HashSize);
      if (!Status) {
        goto Done;
      }
    }    
  }

  //
  // 10. Set the SUM_OF_BYTES_HASHED to the size of the header.
  //
  if (Magic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
    //
    // Use PE32 offset.
    //
    SumOfBytesHashed = mNtHeader.Pe32->OptionalHeader.SizeOfHeaders;
  } else {
    //
    // Use PE32+ offset
    //
    SumOfBytesHashed = mNtHeader.Pe32Plus->OptionalHeader.SizeOfHeaders;
  }


  Section = (EFI_IMAGE_SECTION_HEADER *) (
               mImageBase +
               mPeCoffHeaderOffset +
               sizeof (UINT32) +
               sizeof (EFI_IMAGE_FILE_HEADER) +
               mNtHeader.Pe32->FileHeader.SizeOfOptionalHeader
               );

  //
  // 11. Build a temporary table of pointers to all the IMAGE_SECTION_HEADER
  //     structures in the image. The 'NumberOfSections' field of the image
  //     header indicates how big the table should be. Do not include any
  //     IMAGE_SECTION_HEADERs in the table whose 'SizeOfRawData' field is zero.
  //
  SectionHeader = (EFI_IMAGE_SECTION_HEADER *) AllocateZeroPool (sizeof (EFI_IMAGE_SECTION_HEADER) * mNtHeader.Pe32->FileHeader.NumberOfSections);
  if (SectionHeader == NULL) {
    Status = FALSE;
    goto Done;
  }
  //
  // 12.  Using the 'PointerToRawData' in the referenced section headers as
  //      a key, arrange the elements in the table in ascending order. In other
  //      words, sort the section headers according to the disk-file offset of
  //      the section.
  //
  for (Index = 0; Index < mNtHeader.Pe32->FileHeader.NumberOfSections; Index++) {
    Pos = Index;
    while ((Pos > 0) && (Section->PointerToRawData < SectionHeader[Pos - 1].PointerToRawData)) {
      CopyMem (&SectionHeader[Pos], &SectionHeader[Pos - 1], sizeof (EFI_IMAGE_SECTION_HEADER));
      Pos--;
    }
    CopyMem (&SectionHeader[Pos], Section, sizeof (EFI_IMAGE_SECTION_HEADER));
    Section += 1;
  }

  //
  // 13.  Walk through the sorted table, bring the corresponding section
  //      into memory, and hash the entire section (using the 'SizeOfRawData'
  //      field in the section header to determine the amount of data to hash).
  // 14.  Add the section's 'SizeOfRawData' to SUM_OF_BYTES_HASHED .
  // 15.  Repeat steps 13 and 14 for all the sections in the sorted table.
  //
  for (Index = 0; Index < mNtHeader.Pe32->FileHeader.NumberOfSections; Index++) {
    Section = &SectionHeader[Index];
    if (Section->SizeOfRawData == 0) {
      continue;
    }
    HashBase  = mImageBase + Section->PointerToRawData;
    HashSize  = (UINTN) Section->SizeOfRawData;

    Status  = mHash[HashAlg].HashUpdate(HashCtx, HashBase, HashSize);
    if (!Status) {
      goto Done;
    }

    SumOfBytesHashed += HashSize;
  }

  //
  // 16.  If the file size is greater than SUM_OF_BYTES_HASHED, there is extra
  //      data in the file that needs to be added to the hash. This data begins
  //      at file offset SUM_OF_BYTES_HASHED and its length is:
  //             FileSize  -  (CertDirectory->Size)
  //
  if (mImageSize > SumOfBytesHashed) {
    HashBase = mImageBase + SumOfBytesHashed;

    if (NumberOfRvaAndSizes <= EFI_IMAGE_DIRECTORY_ENTRY_SECURITY) {
      CertSize = 0;
    } else {
      if (Magic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
        //
        // Use PE32 offset.
        //
        CertSize = mNtHeader.Pe32->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_SECURITY].Size;
      } else {
        //
        // Use PE32+ offset.
        //
        CertSize = mNtHeader.Pe32Plus->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_SECURITY].Size;
      }
    }

    if (mImageSize > CertSize + SumOfBytesHashed) {
      HashSize = (UINTN) (mImageSize - CertSize - SumOfBytesHashed);

      Status  = mHash[HashAlg].HashUpdate(HashCtx, HashBase, HashSize);
      if (!Status) {
        goto Done;
      }
    } else if (mImageSize < CertSize + SumOfBytesHashed) {
      Status = FALSE;
      goto Done;
    }
  }

  Status  = mHash[HashAlg].HashFinal(HashCtx, mImageDigest);

Done:
  if (HashCtx != NULL) {
    FreePool (HashCtx);
  }
  if (SectionHeader != NULL) {
    FreePool (SectionHeader);
  }
  return Status;
}

/**
  Recognize the Hash algorithm in PE/COFF Authenticode and caculate hash of
  Pe/Coff image based on the authenticode image hashing in PE/COFF Specification
  8.0 Appendix A

  Caution: This function may receive untrusted input.
  PE/COFF image is external input, so this function will validate its data structure
  within this image buffer before use.

  @retval EFI_UNSUPPORTED             Hash algorithm is not supported.
  @retval EFI_SUCCESS                 Hash successfully.

**/
EFI_STATUS
HashPeImageByType (
  VOID
  )
{
  UINT8                     Index;
  WIN_CERTIFICATE_EFI_PKCS  *PkcsCertData;

  PkcsCertData = (WIN_CERTIFICATE_EFI_PKCS *) (mImageBase + mSecDataDir->VirtualAddress);

  if (PkcsCertData->Hdr.dwLength < sizeof (WIN_CERTIFICATE_EFI_PKCS) + 32) {
    return EFI_UNSUPPORTED;
  }

  for (Index = 0; Index < HASHALG_MAX; Index++) {
    //
    // Check the Hash algorithm in PE/COFF Authenticode.
    //    According to PKCS#7 Definition:
    //        SignedData ::= SEQUENCE {
    //            version Version,
    //            digestAlgorithms DigestAlgorithmIdentifiers,
    //            contentInfo ContentInfo,
    //            .... }
    //    The DigestAlgorithmIdentifiers can be used to determine the hash algorithm in PE/COFF hashing
    //    This field has the fixed offset (+32) in final Authenticode ASN.1 data.
    //    Fixed offset (+32) is calculated based on two bytes of length encoding.
    //
    if ((*(PkcsCertData->CertData + 1) & TWO_BYTE_ENCODE) != TWO_BYTE_ENCODE) {
      //
      // Only support two bytes of Long Form of Length Encoding.
      //
      continue;
    }

    if (PkcsCertData->Hdr.dwLength < sizeof (WIN_CERTIFICATE_EFI_PKCS) + 32 + mHash[Index].OidLength) {
      return EFI_UNSUPPORTED;
    }

    if (CompareMem (PkcsCertData->CertData + 32, mHash[Index].OidValue, mHash[Index].OidLength) == 0) {
      break;
    }
  }

  if (Index == HASHALG_MAX) {
    return EFI_UNSUPPORTED;
  }

  //
  // HASH PE Image based on Hash algorithm in PE/COFF Authenticode.
  //
  if (!HashPeImage(Index)) {
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}


/**
  Returns the size of a given image execution info table in bytes.

  This function returns the size, in bytes, of the image execution info table specified by
  ImageExeInfoTable. If ImageExeInfoTable is NULL, then 0 is returned.

  @param  ImageExeInfoTable          A pointer to a image execution info table structure.

  @retval 0       If ImageExeInfoTable is NULL.
  @retval Others  The size of a image execution info table in bytes.

**/
UINTN
GetImageExeInfoTableSize (
  EFI_IMAGE_EXECUTION_INFO_TABLE        *ImageExeInfoTable
  )
{
  UINTN                     Index;
  EFI_IMAGE_EXECUTION_INFO  *ImageExeInfoItem;
  UINTN                     TotalSize;

  if (ImageExeInfoTable == NULL) {
    return 0;
  }

  ImageExeInfoItem  = (EFI_IMAGE_EXECUTION_INFO *) ((UINT8 *) ImageExeInfoTable + sizeof (EFI_IMAGE_EXECUTION_INFO_TABLE));
  TotalSize         = sizeof (EFI_IMAGE_EXECUTION_INFO_TABLE);
  for (Index = 0; Index < ImageExeInfoTable->NumberOfImages; Index++) {
    TotalSize += ReadUnaligned32 ((UINT32 *) &ImageExeInfoItem->InfoSize);
    ImageExeInfoItem = (EFI_IMAGE_EXECUTION_INFO *) ((UINT8 *) ImageExeInfoItem + ReadUnaligned32 ((UINT32 *) &ImageExeInfoItem->InfoSize));
  }

  return TotalSize;
}

/**
  Create an Image Execution Information Table entry and add it to system configuration table.

  @param[in]  Action          Describes the action taken by the firmware regarding this image.
  @param[in]  Name            Input a null-terminated, user-friendly name.
  @param[in]  DevicePath      Input device path pointer.
  @param[in]  Signature       Input signature info in EFI_SIGNATURE_LIST data structure.
  @param[in]  SignatureSize   Size of signature.

**/
VOID
AddImageExeInfo (
  IN       EFI_IMAGE_EXECUTION_ACTION       Action,
  IN       CHAR16                           *Name OPTIONAL,
  IN CONST EFI_DEVICE_PATH_PROTOCOL         *DevicePath,
  IN       EFI_SIGNATURE_LIST               *Signature OPTIONAL,
  IN       UINTN                            SignatureSize
  )
{
  EFI_IMAGE_EXECUTION_INFO_TABLE  *ImageExeInfoTable;
  EFI_IMAGE_EXECUTION_INFO_TABLE  *NewImageExeInfoTable;
  EFI_IMAGE_EXECUTION_INFO        *ImageExeInfoEntry;
  UINTN                           ImageExeInfoTableSize;
  UINTN                           NewImageExeInfoEntrySize;
  UINTN                           NameStringLen;
  UINTN                           DevicePathSize;

  ImageExeInfoTable     = NULL;
  NewImageExeInfoTable  = NULL;
  ImageExeInfoEntry     = NULL;
  NameStringLen         = 0;

  if (DevicePath == NULL) {
    return ;
  }

  if (Name != NULL) {
    NameStringLen = StrSize (Name);
  }

  ImageExeInfoTable = NULL;
  EfiGetSystemConfigurationTable (&gEfiImageSecurityDatabaseGuid, (VOID **) &ImageExeInfoTable);
  if (ImageExeInfoTable != NULL) {
    //
    // The table has been found!
    // We must enlarge the table to accmodate the new exe info entry.
    //
    ImageExeInfoTableSize = GetImageExeInfoTableSize (ImageExeInfoTable);
  } else {
    //
    // Not Found!
    // We should create a new table to append to the configuration table.
    //
    ImageExeInfoTableSize = sizeof (EFI_IMAGE_EXECUTION_INFO_TABLE);
  }

  DevicePathSize            = GetDevicePathSize (DevicePath);
  NewImageExeInfoEntrySize  = sizeof (EFI_IMAGE_EXECUTION_INFO) + NameStringLen + DevicePathSize + SignatureSize;
  NewImageExeInfoTable      = (EFI_IMAGE_EXECUTION_INFO_TABLE *) AllocateRuntimePool (ImageExeInfoTableSize + NewImageExeInfoEntrySize);
  if (NewImageExeInfoTable == NULL) {
    return ;
  }

  if (ImageExeInfoTable != NULL) {
    CopyMem (NewImageExeInfoTable, ImageExeInfoTable, ImageExeInfoTableSize);
  } else {
    NewImageExeInfoTable->NumberOfImages = 0;
  }
  NewImageExeInfoTable->NumberOfImages++;
  ImageExeInfoEntry = (EFI_IMAGE_EXECUTION_INFO *) ((UINT8 *) NewImageExeInfoTable + ImageExeInfoTableSize);
  //
  // Update new item's infomation.
  //
  WriteUnaligned32 ((UINT32 *) &ImageExeInfoEntry->Action, Action);
  WriteUnaligned32 ((UINT32 *) &ImageExeInfoEntry->InfoSize, (UINT32) NewImageExeInfoEntrySize);

  if (Name != NULL) {
    CopyMem ((UINT8 *) &ImageExeInfoEntry->InfoSize + sizeof (UINT32), Name, NameStringLen);
  }
  CopyMem (
    (UINT8 *) &ImageExeInfoEntry->InfoSize + sizeof (UINT32) + NameStringLen,
    DevicePath,
    DevicePathSize
    );
  if (Signature != NULL) {
    CopyMem (
      (UINT8 *) &ImageExeInfoEntry->InfoSize + sizeof (UINT32) + NameStringLen + DevicePathSize,
      Signature,
      SignatureSize
      );
  }
  //
  // Update/replace the image execution table.
  //
  gBS->InstallConfigurationTable (&gEfiImageSecurityDatabaseGuid, (VOID *) NewImageExeInfoTable);

  //
  // Free Old table data!
  //
  if (ImageExeInfoTable != NULL) {
    FreePool (ImageExeInfoTable);
  }
}

/**
  Discover if the UEFI image is authorized by user's policy setting.

  @param[in]    Policy            Specify platform's policy setting.

  @retval EFI_ACCESS_DENIED       Image is not allowed to run.
  @retval EFI_SECURITY_VIOLATION  Image is deferred.
  @retval EFI_SUCCESS             Image is authorized to run.

**/
EFI_STATUS
ImageAuthorization (
  IN UINT32     Policy
  )
{
  EFI_STATUS    Status;
  EFI_INPUT_KEY Key;

  Status = EFI_ACCESS_DENIED;

  switch (Policy) {

  case QUERY_USER_ON_SECURITY_VIOLATION:
    do {
      CreatePopUp (EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE, &Key, mNotifyString1, mNotifyString2, NULL);
      if (Key.UnicodeChar == L'Y' || Key.UnicodeChar == L'y') {
        Status = EFI_SUCCESS;
        break;
      } else if (Key.UnicodeChar == L'N' || Key.UnicodeChar == L'n') {
        Status = EFI_ACCESS_DENIED;
        break;
      } else if (Key.UnicodeChar == L'D' || Key.UnicodeChar == L'd') {
        Status = EFI_SECURITY_VIOLATION;
        break;
      }
    } while (TRUE);
    break;

  case ALLOW_EXECUTE_ON_SECURITY_VIOLATION:
    Status = EFI_SUCCESS;
    break;

  case DEFER_EXECUTE_ON_SECURITY_VIOLATION:
    Status = EFI_SECURITY_VIOLATION;
    break;

  case DENY_EXECUTE_ON_SECURITY_VIOLATION:
    Status = EFI_ACCESS_DENIED;
    break;
  }

  return Status;
}

/**
  Check whether signature is in specified database.

  @param[in]  VariableName        Name of database variable that is searched in.
  @param[in]  Signature           Pointer to signature that is searched for.
  @param[in]  CertType            Pointer to hash algrithom.
  @param[in]  SignatureSize       Size of Signature.

  @return TRUE                    Found the signature in the variable database.
  @return FALSE                   Not found the signature in the variable database.

**/
BOOLEAN
IsSignatureFoundInDatabase (
  IN CHAR16             *VariableName,
  IN UINT8              *Signature,
  IN EFI_GUID           *CertType,
  IN UINTN              SignatureSize
  )
{
  EFI_STATUS          Status;
  EFI_SIGNATURE_LIST  *CertList;
  EFI_SIGNATURE_DATA  *Cert;
  UINTN               DataSize;
  UINT8               *Data;
  UINTN               Index;
  UINTN               CertCount;
  BOOLEAN             IsFound;
  //
  // Read signature database variable.
  //
  IsFound   = FALSE;
  Data      = NULL;
  DataSize  = 0;
  Status    = gRT->GetVariable (VariableName, &gEfiImageSecurityDatabaseGuid, NULL, &DataSize, NULL);
  if (Status != EFI_BUFFER_TOO_SMALL) {
    return FALSE;
  }

  Data = (UINT8 *) AllocateZeroPool (DataSize);
  if (Data == NULL) {
    return FALSE;
  }

  Status = gRT->GetVariable (VariableName, &gEfiImageSecurityDatabaseGuid, NULL, &DataSize, Data);
  if (EFI_ERROR (Status)) {
    goto Done;
  }
  //
  // Enumerate all signature data in SigDB to check if executable's signature exists.
  //
  CertList = (EFI_SIGNATURE_LIST *) Data;
  while ((DataSize > 0) && (DataSize >= CertList->SignatureListSize)) {
    CertCount = (CertList->SignatureListSize - CertList->SignatureHeaderSize) / CertList->SignatureSize;
    Cert      = (EFI_SIGNATURE_DATA *) ((UINT8 *) CertList + sizeof (EFI_SIGNATURE_LIST) + CertList->SignatureHeaderSize);
    if ((CertList->SignatureSize == sizeof(EFI_SIGNATURE_DATA) - 1 + SignatureSize) && (CompareGuid(&CertList->SignatureType, CertType))) {
      for (Index = 0; Index < CertCount; Index++) {
        if (CompareMem (Cert->SignatureData, Signature, SignatureSize) == 0) {
          //
          // Find the signature in database.
          //
          IsFound = TRUE;
          break;
        }

        Cert = (EFI_SIGNATURE_DATA *) ((UINT8 *) Cert + CertList->SignatureSize);
      }

      if (IsFound) {
        break;
      }
    }

    DataSize -= CertList->SignatureListSize;
    CertList = (EFI_SIGNATURE_LIST *) ((UINT8 *) CertList + CertList->SignatureListSize);
  }

Done:
  if (Data != NULL) {
    FreePool (Data);
  }

  return IsFound;
}

/**
  Verify PKCS#7 SignedData using certificate found in Variable which formatted
  as EFI_SIGNATURE_LIST. The Variable may be PK, KEK, DB or DBX.

  @param VariableName  Name of Variable to search for Certificate.
  @param VendorGuid    Variable vendor GUID.

  @retval TRUE         Image pass verification.
  @retval FALSE        Image fail verification.

**/
BOOLEAN
IsPkcsSignedDataVerifiedBySignatureList (
  IN CHAR16             *VariableName,
  IN EFI_GUID           *VendorGuid
  )
{
  EFI_STATUS                Status;
  BOOLEAN                   VerifyStatus;
  WIN_CERTIFICATE_EFI_PKCS  *PkcsCertData;
  EFI_SIGNATURE_LIST        *CertList;
  EFI_SIGNATURE_DATA        *Cert;
  UINTN                     DataSize;
  UINT8                     *Data;
  UINT8                     *RootCert;
  UINTN                     RootCertSize;
  UINTN                     Index;
  UINTN                     CertCount;

  Data         = NULL;
  CertList     = NULL;
  Cert         = NULL;
  RootCert     = NULL;
  RootCertSize = 0;
  VerifyStatus = FALSE;
  PkcsCertData = (WIN_CERTIFICATE_EFI_PKCS *) (mImageBase + mSecDataDir->VirtualAddress);

  DataSize = 0;
  Status   = gRT->GetVariable (VariableName, VendorGuid, NULL, &DataSize, NULL);
  if (Status == EFI_BUFFER_TOO_SMALL) {
    Data = (UINT8 *) AllocateZeroPool (DataSize);
    if (Data == NULL) {
      return VerifyStatus;
    }

    Status = gRT->GetVariable (VariableName, VendorGuid, NULL, &DataSize, (VOID *) Data);
    if (EFI_ERROR (Status)) {
      goto Done;
    }

    //
    // Find X509 certificate in Signature List to verify the signature in pkcs7 signed data.
    //
    CertList = (EFI_SIGNATURE_LIST *) Data;
    while ((DataSize > 0) && (DataSize >= CertList->SignatureListSize)) {
      if (CompareGuid (&CertList->SignatureType, &gEfiCertX509Guid)) {
        Cert          = (EFI_SIGNATURE_DATA *) ((UINT8 *) CertList + sizeof (EFI_SIGNATURE_LIST) + CertList->SignatureHeaderSize);
        CertCount     = (CertList->SignatureListSize - sizeof (EFI_SIGNATURE_LIST) - CertList->SignatureHeaderSize) / CertList->SignatureSize;
        for (Index = 0; Index < CertCount; Index++) {
          //
          // Iterate each Signature Data Node within this CertList for verify.
          //
          RootCert      = Cert->SignatureData;
          RootCertSize  = CertList->SignatureSize;

          //
          // Call AuthenticodeVerify library to Verify Authenticode struct.
          //
          VerifyStatus = AuthenticodeVerify (
                           PkcsCertData->CertData,
                           PkcsCertData->Hdr.dwLength - sizeof(PkcsCertData->Hdr),
                           RootCert,
                           RootCertSize,
                           mImageDigest,
                           mImageDigestSize
                           );
          if (VerifyStatus) {
            goto Done;
          }
          Cert = (EFI_SIGNATURE_DATA *) ((UINT8 *) Cert + CertList->SignatureSize);
        }
      }
      DataSize -= CertList->SignatureListSize;
      CertList = (EFI_SIGNATURE_LIST *) ((UINT8 *) CertList + CertList->SignatureListSize);
    }
  }

Done:
  if (Data != NULL) {
    FreePool (Data);
  }

  return VerifyStatus;
}

/**
  Verify certificate in WIN_CERT_TYPE_PKCS_SIGNED_DATA format.

  @retval EFI_SUCCESS                 Image pass verification.
  @retval EFI_SECURITY_VIOLATION      Image fail verification.

**/
EFI_STATUS
VerifyCertPkcsSignedData (
  VOID
  )
{
  //
  // 1: Find certificate from DBX forbidden database for revoked certificate.
  //
  if (IsPkcsSignedDataVerifiedBySignatureList (EFI_IMAGE_SECURITY_DATABASE1, &gEfiImageSecurityDatabaseGuid)) {
    //
    // DBX is forbidden database, if Authenticode verification pass with
    // one of the certificate in DBX, this image should be rejected.
    //
    return EFI_SECURITY_VIOLATION;
  }

  //
  // 2: Find certificate from KEK database and try to verify authenticode struct.
  //
  if (IsPkcsSignedDataVerifiedBySignatureList (EFI_KEY_EXCHANGE_KEY_NAME, &gEfiGlobalVariableGuid)) {
    return EFI_SUCCESS;
  }

  //
  // 3: Find certificate from DB database and try to verify authenticode struct.
  //
  if (IsPkcsSignedDataVerifiedBySignatureList (EFI_IMAGE_SECURITY_DATABASE, &gEfiImageSecurityDatabaseGuid)) {
    return EFI_SUCCESS;
  } else {
    return EFI_SECURITY_VIOLATION;
  }
}

/**
  Verify certificate in WIN_CERTIFICATE_UEFI_GUID format.

  @retval EFI_SUCCESS                 Image pass verification.
  @retval EFI_SECURITY_VIOLATION      Image fail verification.
  @retval other error value

**/
EFI_STATUS
VerifyCertUefiGuid (
  VOID
  )
{
  BOOLEAN                         Status;
  WIN_CERTIFICATE_UEFI_GUID       *EfiCert;
  EFI_SIGNATURE_LIST              *KekList;
  EFI_SIGNATURE_DATA              *KekItem;
  EFI_CERT_BLOCK_RSA_2048_SHA256  *CertBlock;
  VOID                            *Rsa;
  UINTN                           KekCount;
  UINTN                           Index;
  UINTN                           KekDataSize;
  BOOLEAN                         IsFound;
  EFI_STATUS                      Result;

  EfiCert   = NULL;
  KekList   = NULL;
  KekItem   = NULL;
  CertBlock = NULL;
  Rsa       = NULL;
  Status    = FALSE;
  IsFound   = FALSE;
  KekDataSize = 0;

  EfiCert   = (WIN_CERTIFICATE_UEFI_GUID *) (mImageBase + mSecDataDir->VirtualAddress);
  CertBlock = (EFI_CERT_BLOCK_RSA_2048_SHA256 *) EfiCert->CertData;
  if (!CompareGuid (&EfiCert->CertType, &gEfiCertTypeRsa2048Sha256Guid)) {
    //
    // Invalid Certificate Data Type.
    //
    return EFI_SECURITY_VIOLATION;
  }

  //
  // Get KEK database variable data size
  //
  Result = gRT->GetVariable (EFI_KEY_EXCHANGE_KEY_NAME, &gEfiGlobalVariableGuid, NULL, &KekDataSize, NULL);
  if (Result != EFI_BUFFER_TOO_SMALL) {
    return EFI_SECURITY_VIOLATION;
  }

  //
  // Get KEK database variable.
  //
  GetEfiGlobalVariable2 (EFI_KEY_EXCHANGE_KEY_NAME, (VOID**)&KekList, NULL);
  if (KekList == NULL) {
    return EFI_SECURITY_VIOLATION;
  }

  //
  // Enumerate all Kek items in this list to verify the variable certificate data.
  // If anyone is authenticated successfully, it means the variable is correct!
  //
  while ((KekDataSize > 0) && (KekDataSize >= KekList->SignatureListSize)) {
    if (CompareGuid (&KekList->SignatureType, &gEfiCertRsa2048Guid)) {
      KekItem   = (EFI_SIGNATURE_DATA *) ((UINT8 *) KekList + sizeof (EFI_SIGNATURE_LIST) + KekList->SignatureHeaderSize);
      KekCount  = (KekList->SignatureListSize - sizeof (EFI_SIGNATURE_LIST) - KekList->SignatureHeaderSize) / KekList->SignatureSize;
      for (Index = 0; Index < KekCount; Index++) {
        if (CompareMem (KekItem->SignatureData, CertBlock->PublicKey, EFI_CERT_TYPE_RSA2048_SIZE) == 0) {
          IsFound = TRUE;
          break;
        }
        KekItem = (EFI_SIGNATURE_DATA *) ((UINT8 *) KekItem + KekList->SignatureSize);
      }
    }
    KekDataSize -= KekList->SignatureListSize;
    KekList = (EFI_SIGNATURE_LIST *) ((UINT8 *) KekList + KekList->SignatureListSize);
  }

  if (!IsFound) {
    //
    // Signed key is not a trust one.
    //
    goto Done;
  }

  //
  // Now, we found the corresponding security policy.
  // Verify the data payload.
  //
  Rsa = RsaNew ();
  if (Rsa == NULL) {
    Status = FALSE;
    goto Done;
  }

  //
  // Set RSA Key Components.
  // NOTE: Only N and E are needed to be set as RSA public key for signature verification.
  //
  Status = RsaSetKey (Rsa, RsaKeyN, CertBlock->PublicKey, EFI_CERT_TYPE_RSA2048_SIZE);
  if (!Status) {
    goto Done;
  }
  Status = RsaSetKey (Rsa, RsaKeyE, mRsaE, sizeof (mRsaE));
  if (!Status) {
    goto Done;
  }
  //
  // Verify the signature.
  //
  Status = RsaPkcs1Verify (
             Rsa,
             mImageDigest,
             mImageDigestSize,
             CertBlock->Signature,
             EFI_CERT_TYPE_RSA2048_SHA256_SIZE
             );

Done:
  if (KekList != NULL) {
    FreePool (KekList);
  }
  if (Rsa != NULL ) {
    RsaFree (Rsa);
  }
  if (Status) {
    return EFI_SUCCESS;
  } else {
    return EFI_SECURITY_VIOLATION;
  }
}

/**
  Provide verification service for signed images, which include both signature validation
  and platform policy control. For signature types, both UEFI WIN_CERTIFICATE_UEFI_GUID and
  MSFT Authenticode type signatures are supported.

  In this implementation, only verify external executables when in USER MODE.
  Executables from FV is bypass, so pass in AuthenticationStatus is ignored.

  The image verification process is:
    Is the Image signed?
      If yes,
        Does the image verify against a certificate (root or intermediate) in the allowed db?
          Run it
        Image verification fail
          Is the Image's Hash not in forbidden database and the Image's Hash in allowed db?
            Run it
      If no,
        Is the Image's Hash in the forbidden database (DBX)?
          if yes,
            Error out
        Is the Image's Hash in the allowed database (DB)?
          If yes,
            Run it
          If no,
            Error out

  Caution: This function may receive untrusted input.
  PE/COFF image is external input, so this function will validate its data structure
  within this image buffer before use.

  @param[in]    AuthenticationStatus
                           This is the authentication status returned from the security
                           measurement services for the input file.
  @param[in]    File       This is a pointer to the device path of the file that is
                           being dispatched. This will optionally be used for logging.
  @param[in]    FileBuffer File buffer matches the input file device path.
  @param[in]    FileSize   Size of File buffer matches the input file device path.
  @param[in]    BootPolicy A boot policy that was used to call LoadImage() UEFI service.

  @retval EFI_SUCCESS            The file specified by DevicePath and non-NULL
                                 FileBuffer did authenticate, and the platform policy dictates
                                 that the DXE Foundation may use the file.
  @retval EFI_SUCCESS            The device path specified by NULL device path DevicePath
                                 and non-NULL FileBuffer did authenticate, and the platform
                                 policy dictates that the DXE Foundation may execute the image in
                                 FileBuffer.
  @retval EFI_OUT_RESOURCE       Fail to allocate memory.
  @retval EFI_SECURITY_VIOLATION The file specified by File did not authenticate, and
                                 the platform policy dictates that File should be placed
                                 in the untrusted state. The image has been added to the file
                                 execution table.
  @retval EFI_ACCESS_DENIED      The file specified by File and FileBuffer did not
                                 authenticate, and the platform policy dictates that the DXE
                                 Foundation many not use File.

**/
EFI_STATUS
EFIAPI
DxeImageVerificationHandler (
  IN  UINT32                           AuthenticationStatus,
  IN  CONST EFI_DEVICE_PATH_PROTOCOL   *File,
  IN  VOID                             *FileBuffer,
  IN  UINTN                            FileSize,
  IN  BOOLEAN                          BootPolicy
  )
{
  EFI_STATUS                           Status;
  UINT16                               Magic;
  EFI_IMAGE_DOS_HEADER                 *DosHdr;
  EFI_STATUS                           VerifyStatus;
  UINT8                                *SetupMode;
  EFI_SIGNATURE_LIST                   *SignatureList;
  UINTN                                SignatureListSize;
  EFI_SIGNATURE_DATA                   *Signature;
  EFI_IMAGE_EXECUTION_ACTION           Action;
  WIN_CERTIFICATE                      *WinCertificate;
  UINT32                               Policy;
  UINT8                                *SecureBootEnable;
  PE_COFF_LOADER_IMAGE_CONTEXT         ImageContext;
  UINT32                               NumberOfRvaAndSizes;
  UINT32                               CertSize;

  SignatureList     = NULL;
  SignatureListSize = 0;
  WinCertificate    = NULL;
  Action            = EFI_IMAGE_EXECUTION_AUTH_UNTESTED;
  Status            = EFI_ACCESS_DENIED;
  //
  // Check the image type and get policy setting.
  //
  switch (GetImageType (File)) {

  case IMAGE_FROM_FV:
    Policy = ALWAYS_EXECUTE;
    break;

  case IMAGE_FROM_OPTION_ROM:
    Policy = PcdGet32 (PcdOptionRomImageVerificationPolicy);
    break;

  case IMAGE_FROM_REMOVABLE_MEDIA:
    Policy = PcdGet32 (PcdRemovableMediaImageVerificationPolicy);
    break;

  case IMAGE_FROM_FIXED_MEDIA:
    Policy = PcdGet32 (PcdFixedMediaImageVerificationPolicy);
    break;

  default:
    Policy = DENY_EXECUTE_ON_SECURITY_VIOLATION;
    break;
  }
  //
  // If policy is always/never execute, return directly.
  //
  if (Policy == ALWAYS_EXECUTE) {
    return EFI_SUCCESS;
  } else if (Policy == NEVER_EXECUTE) {
    return EFI_ACCESS_DENIED;
  }

  GetVariable2 (EFI_SECURE_BOOT_ENABLE_NAME, &gEfiSecureBootEnableDisableGuid, (VOID**)&SecureBootEnable, NULL);
  //
  // Skip verification if SecureBootEnable variable doesn't exist.
  //
  if (SecureBootEnable == NULL) {
    return EFI_SUCCESS;
  }

  //
  // Skip verification if SecureBootEnable is disabled.
  //
  if (*SecureBootEnable == SECURE_BOOT_DISABLE) {
    FreePool (SecureBootEnable);
    return EFI_SUCCESS;
  }

  FreePool (SecureBootEnable);

  GetEfiGlobalVariable2 (EFI_SETUP_MODE_NAME, (VOID**)&SetupMode, NULL);

  //
  // SetupMode doesn't exist means no AuthVar driver is dispatched,
  // skip verification.
  //
  if (SetupMode == NULL) {
    return EFI_SUCCESS;
  }

  //
  // If platform is in SETUP MODE, skip verification.
  //
  if (*SetupMode == SETUP_MODE) {
    FreePool (SetupMode);
    return EFI_SUCCESS;
  }

  FreePool (SetupMode);

  //
  // Read the Dos header.
  //
  if (FileBuffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  mImageBase  = (UINT8 *) FileBuffer;
  mImageSize  = FileSize;

  ZeroMem (&ImageContext, sizeof (ImageContext));
  ImageContext.Handle    = (VOID *) FileBuffer;
  ImageContext.ImageRead = (PE_COFF_LOADER_READ_FILE) DxeImageVerificationLibImageRead;

  //
  // Get information about the image being loaded
  //
  Status = PeCoffLoaderGetImageInfo (&ImageContext);
  if (EFI_ERROR (Status)) {
    //
    // The information can't be got from the invalid PeImage
    //
    goto Done;
  }

  Status = EFI_ACCESS_DENIED;

  DosHdr = (EFI_IMAGE_DOS_HEADER *) mImageBase;
  if (DosHdr->e_magic == EFI_IMAGE_DOS_SIGNATURE) {
    //
    // DOS image header is present,
    // so read the PE header after the DOS image header.
    //
    mPeCoffHeaderOffset = DosHdr->e_lfanew;
  } else {
    mPeCoffHeaderOffset = 0;
  }
  //
  // Check PE/COFF image.
  //
  mNtHeader.Pe32 = (EFI_IMAGE_NT_HEADERS32 *) (mImageBase + mPeCoffHeaderOffset);
  if (mNtHeader.Pe32->Signature != EFI_IMAGE_NT_SIGNATURE) {
    //
    // It is not a valid Pe/Coff file.
    //
    goto Done;
  }

  Magic = mNtHeader.Pe32->OptionalHeader.Magic;
  if (Magic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
    //
    // Use PE32 offset.
    //
    NumberOfRvaAndSizes = mNtHeader.Pe32->OptionalHeader.NumberOfRvaAndSizes;
    if (NumberOfRvaAndSizes > EFI_IMAGE_DIRECTORY_ENTRY_SECURITY) {
      mSecDataDir = (EFI_IMAGE_DATA_DIRECTORY *) &mNtHeader.Pe32->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_SECURITY];
    }        
  } else {
    //
    // Use PE32+ offset.
    //
    NumberOfRvaAndSizes = mNtHeader.Pe32Plus->OptionalHeader.NumberOfRvaAndSizes;
    if (NumberOfRvaAndSizes > EFI_IMAGE_DIRECTORY_ENTRY_SECURITY) {
      mSecDataDir = (EFI_IMAGE_DATA_DIRECTORY *) &mNtHeader.Pe32Plus->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_SECURITY];
    }
  }

  if ((mSecDataDir == NULL) || ((mSecDataDir != NULL) && (mSecDataDir->Size == 0))) {
    //
    // This image is not signed.
    //
    if (!HashPeImage (HASHALG_SHA256)) {
      goto Done;
    }

    if (IsSignatureFoundInDatabase (EFI_IMAGE_SECURITY_DATABASE1, mImageDigest, &mCertType, mImageDigestSize)) {
      //
      // Image Hash is in forbidden database (DBX).
      //
      goto Done;
    }

    if (IsSignatureFoundInDatabase (EFI_IMAGE_SECURITY_DATABASE, mImageDigest, &mCertType, mImageDigestSize)) {
      //
      // Image Hash is in allowed database (DB).
      //
      return EFI_SUCCESS;
    }

    //
    // Image Hash is not found in both forbidden and allowed database.
    //
    goto Done;
  }

  //
  // Verify signature of executables.
  //
  WinCertificate = (WIN_CERTIFICATE *) (mImageBase + mSecDataDir->VirtualAddress);

  CertSize = sizeof (WIN_CERTIFICATE);

  if ((mSecDataDir->Size <= CertSize) || (mSecDataDir->Size < WinCertificate->dwLength)) {
    goto Done;
  }

  switch (WinCertificate->wCertificateType) {

  case WIN_CERT_TYPE_EFI_GUID:
    CertSize = sizeof (WIN_CERTIFICATE_UEFI_GUID) + sizeof (EFI_CERT_BLOCK_RSA_2048_SHA256) - sizeof (UINT8);
    if (WinCertificate->dwLength < CertSize) {
      goto Done;
    }

    //
    // Verify UEFI GUID type.
    //
    if (!HashPeImage (HASHALG_SHA256)) {
      goto Done;
    }

    VerifyStatus = VerifyCertUefiGuid ();
    break;

  case WIN_CERT_TYPE_PKCS_SIGNED_DATA:
    //
    // Verify Pkcs signed data type.
    //
    Status = HashPeImageByType();
    if (EFI_ERROR (Status)) {
      goto Done;
    }

    VerifyStatus = VerifyCertPkcsSignedData ();

    //
    // For image verification against enrolled certificate(root or intermediate),
    // no need to check image's hash in the allowed database.
    //
    if (!EFI_ERROR (VerifyStatus)) {
      if (!IsSignatureFoundInDatabase (EFI_IMAGE_SECURITY_DATABASE1, mImageDigest, &mCertType, mImageDigestSize)) {
        return EFI_SUCCESS;
      }
    }
    break;

  default:
    goto Done;
  }
  //
  // Get image hash value as executable's signature.
  //
  SignatureListSize = sizeof (EFI_SIGNATURE_LIST) + sizeof (EFI_SIGNATURE_DATA) - 1 + mImageDigestSize;
  SignatureList     = (EFI_SIGNATURE_LIST *) AllocateZeroPool (SignatureListSize);
  if (SignatureList == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }
  SignatureList->SignatureHeaderSize  = 0;
  SignatureList->SignatureListSize    = (UINT32) SignatureListSize;
  SignatureList->SignatureSize        = (UINT32) mImageDigestSize;
  CopyMem (&SignatureList->SignatureType, &mCertType, sizeof (EFI_GUID));
  Signature = (EFI_SIGNATURE_DATA *) ((UINT8 *) SignatureList + sizeof (EFI_SIGNATURE_LIST));
  CopyMem (Signature->SignatureData, mImageDigest, mImageDigestSize);
  //
  // Signature database check after verification.
  //
  if (EFI_ERROR (VerifyStatus)) {
    //
    // Verification failure.
    //
    if (!IsSignatureFoundInDatabase (EFI_IMAGE_SECURITY_DATABASE1, mImageDigest, &mCertType, mImageDigestSize) &&
        IsSignatureFoundInDatabase (EFI_IMAGE_SECURITY_DATABASE, mImageDigest, &mCertType, mImageDigestSize)) {
      //
      // Verification fail, Image Hash is not in forbidden database (DBX),
      // and Image Hash is in allowed database (DB).
      //
      Status = EFI_SUCCESS;
    } else {
      Action = EFI_IMAGE_EXECUTION_AUTH_SIG_FAILED;
      Status = EFI_ACCESS_DENIED;
    }
  } else if (IsSignatureFoundInDatabase (EFI_IMAGE_SECURITY_DATABASE1, Signature->SignatureData, &mCertType, mImageDigestSize)) {
    //
    // Executable signature verification passes, but is found in forbidden signature database.
    //
    Action = EFI_IMAGE_EXECUTION_AUTH_SIG_FOUND;
    Status = EFI_ACCESS_DENIED;
  } else if (IsSignatureFoundInDatabase (EFI_IMAGE_SECURITY_DATABASE, Signature->SignatureData, &mCertType, mImageDigestSize)) {
    //
    // Executable signature is found in authorized signature database.
    //
    Status = EFI_SUCCESS;
  } else {
    //
    // Executable signature verification passes, but cannot be found in authorized signature database.
    // Get platform policy to determine the action.
    //
    Action = EFI_IMAGE_EXECUTION_AUTH_SIG_PASSED;
    Status = ImageAuthorization (Policy);
  }

Done:
  if (Status != EFI_SUCCESS) {
    //
    // Policy decides to defer or reject the image; add its information in image executable information table.
    //
    AddImageExeInfo (Action, NULL, File, SignatureList, SignatureListSize);
    Status = EFI_SECURITY_VIOLATION;
  }

  if (SignatureList != NULL) {
    FreePool (SignatureList);
  }

  return Status;
}

/**
  When VariableWriteArchProtocol install, create "SecureBoot" variable.

  @param[in] Event    Event whose notification function is being invoked.
  @param[in] Context  Pointer to the notification function's context.

**/
VOID
EFIAPI
VariableWriteCallBack (
  IN  EFI_EVENT                           Event,
  IN  VOID                                *Context
  )
{
  UINT8                       SecureBootMode;
  UINT8                       *SecureBootModePtr;
  EFI_STATUS                  Status;
  VOID                        *ProtocolPointer;

  Status = gBS->LocateProtocol (&gEfiVariableWriteArchProtocolGuid, NULL, &ProtocolPointer);
  if (EFI_ERROR (Status)) {
    return;
  }

  //
  // Check whether "SecureBoot" variable exists.
  // If this library is built-in, it means firmware has capability to perform
  // driver signing verification.
  //
  GetEfiGlobalVariable2 (EFI_SECURE_BOOT_MODE_NAME, (VOID**)&SecureBootModePtr, NULL);
  if (SecureBootModePtr == NULL) {
    SecureBootMode   = SECURE_BOOT_MODE_DISABLE;
    //
    // Authenticated variable driver will update "SecureBoot" depending on SetupMode variable.
    //
    gRT->SetVariable (
           EFI_SECURE_BOOT_MODE_NAME,
           &gEfiGlobalVariableGuid,
           EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
           sizeof (UINT8),
           &SecureBootMode
           );
  } else {
    FreePool (SecureBootModePtr);
  }
}

/**
  Register security measurement handler.

  @param  ImageHandle   ImageHandle of the loaded driver.
  @param  SystemTable   Pointer to the EFI System Table.

  @retval EFI_SUCCESS   The handlers were registered successfully.
**/
EFI_STATUS
EFIAPI
DxeImageVerificationLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  VOID                *Registration;

  //
  // Register callback function upon VariableWriteArchProtocol.
  //
  EfiCreateProtocolNotifyEvent (
    &gEfiVariableWriteArchProtocolGuid,
    TPL_CALLBACK,
    VariableWriteCallBack,
    NULL,
    &Registration
    );

  return RegisterSecurity2Handler (
          DxeImageVerificationHandler,
          EFI_AUTH_OPERATION_VERIFY_IMAGE | EFI_AUTH_OPERATION_IMAGE_REQUIRED
          );
}
