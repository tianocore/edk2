/** @file
  The library instance provides security service of TPM measure boot.

  Caution: This file requires additional review when modified.
  This library will have external input - PE/COFF image and GPT partition.
  This external input must be validated carefully to avoid security issue like
  buffer overflow, integer overflow.

  DxeTpmMeasureBootLibImageRead() function will make sure the PE/COFF image content
  read is within the image buffer.

  TcgMeasurePeImage() function will accept untrusted PE/COFF image and validate its
  data structure within this image buffer before use.

  TcgMeasureGptTable() function will receive untrusted GPT partition table, and parse
  partition data carefully.

Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>

#include <Protocol/TcgService.h>
#include <Protocol/BlockIo.h>
#include <Protocol/DiskIo.h>
#include <Protocol/FirmwareVolumeBlock.h>

#include <Guid/MeasuredFvHob.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DevicePathLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseCryptLib.h>
#include <Library/PeCoffLib.h>
#include <Library/SecurityManagementLib.h>
#include <Library/HobLib.h>

//
// Flag to check GPT partition. It only need be measured once.
//
BOOLEAN                           mMeasureGptTableFlag = FALSE;
UINTN                             mMeasureGptCount = 0;
VOID                              *mFileBuffer;
UINTN                             mTpmImageSize;
//
// Measured FV handle cache
//
EFI_HANDLE                        mCacheMeasuredHandle  = NULL;
MEASURED_HOB_DATA                 *mMeasuredHobData     = NULL;

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
DxeTpmMeasureBootLibImageRead (
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
  if (EndPosition > mTpmImageSize) {
    *ReadSize = (UINT32)(mTpmImageSize - FileOffset);
  }

  if (FileOffset >= mTpmImageSize) {
    *ReadSize = 0;
  }

  CopyMem (Buffer, (UINT8 *)((UINTN) FileHandle + FileOffset), *ReadSize);

  return EFI_SUCCESS;
}

/**
  Measure GPT table data into TPM log.

  Caution: This function may receive untrusted input.
  The GPT partition table is external input, so this function should parse partition data carefully.

  @param TcgProtocol             Pointer to the located TCG protocol instance.
  @param GptHandle               Handle that GPT partition was installed.

  @retval EFI_SUCCESS            Successfully measure GPT table.
  @retval EFI_UNSUPPORTED        Not support GPT table on the given handle.
  @retval EFI_DEVICE_ERROR       Can't get GPT table because device error.
  @retval EFI_OUT_OF_RESOURCES   No enough resource to measure GPT table.
  @retval other error value
**/
EFI_STATUS
EFIAPI
TcgMeasureGptTable (
  IN  EFI_TCG_PROTOCOL   *TcgProtocol,
  IN  EFI_HANDLE         GptHandle
  )
{
  EFI_STATUS                        Status;
  EFI_BLOCK_IO_PROTOCOL             *BlockIo;
  EFI_DISK_IO_PROTOCOL              *DiskIo;
  EFI_PARTITION_TABLE_HEADER        *PrimaryHeader;
  EFI_PARTITION_ENTRY               *PartitionEntry;
  UINT8                             *EntryPtr;
  UINTN                             NumberOfPartition;
  UINT32                            Index;
  TCG_PCR_EVENT                     *TcgEvent;
  EFI_GPT_DATA                      *GptData;
  UINT32                            EventSize;
  UINT32                            EventNumber;
  EFI_PHYSICAL_ADDRESS              EventLogLastEntry;

  if (mMeasureGptCount > 0) {
    return EFI_SUCCESS;
  }

  Status = gBS->HandleProtocol (GptHandle, &gEfiBlockIoProtocolGuid, (VOID**)&BlockIo);
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }
  Status = gBS->HandleProtocol (GptHandle, &gEfiDiskIoProtocolGuid, (VOID**)&DiskIo);
  if (EFI_ERROR (Status)) {
    return EFI_UNSUPPORTED;
  }
  //
  // Read the EFI Partition Table Header
  //
  PrimaryHeader = (EFI_PARTITION_TABLE_HEADER *) AllocatePool (BlockIo->Media->BlockSize);
  if (PrimaryHeader == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  Status = DiskIo->ReadDisk (
                     DiskIo,
                     BlockIo->Media->MediaId,
                     1 * BlockIo->Media->BlockSize,
                     BlockIo->Media->BlockSize,
                     (UINT8 *)PrimaryHeader
                     );
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "Failed to Read Partition Table Header!\n"));
    FreePool (PrimaryHeader);
    return EFI_DEVICE_ERROR;
  }
  //
  // Read the partition entry.
  //
  EntryPtr = (UINT8 *)AllocatePool (PrimaryHeader->NumberOfPartitionEntries * PrimaryHeader->SizeOfPartitionEntry);
  if (EntryPtr == NULL) {
    FreePool (PrimaryHeader);
    return EFI_OUT_OF_RESOURCES;
  }
  Status = DiskIo->ReadDisk (
                     DiskIo,
                     BlockIo->Media->MediaId,
                     MultU64x32(PrimaryHeader->PartitionEntryLBA, BlockIo->Media->BlockSize),
                     PrimaryHeader->NumberOfPartitionEntries * PrimaryHeader->SizeOfPartitionEntry,
                     EntryPtr
                     );
  if (EFI_ERROR (Status)) {
    FreePool (PrimaryHeader);
    FreePool (EntryPtr);
    return EFI_DEVICE_ERROR;
  }

  //
  // Count the valid partition
  //
  PartitionEntry    = (EFI_PARTITION_ENTRY *)EntryPtr;
  NumberOfPartition = 0;
  for (Index = 0; Index < PrimaryHeader->NumberOfPartitionEntries; Index++) {
    if (!IsZeroGuid (&PartitionEntry->PartitionTypeGUID)) {
      NumberOfPartition++;
    }
    PartitionEntry = (EFI_PARTITION_ENTRY *)((UINT8 *)PartitionEntry + PrimaryHeader->SizeOfPartitionEntry);
  }

  //
  // Prepare Data for Measurement
  //
  EventSize = (UINT32)(sizeof (EFI_GPT_DATA) - sizeof (GptData->Partitions)
                        + NumberOfPartition * PrimaryHeader->SizeOfPartitionEntry);
  TcgEvent = (TCG_PCR_EVENT *) AllocateZeroPool (EventSize + sizeof (TCG_PCR_EVENT_HDR));
  if (TcgEvent == NULL) {
    FreePool (PrimaryHeader);
    FreePool (EntryPtr);
    return EFI_OUT_OF_RESOURCES;
  }

  TcgEvent->PCRIndex   = 5;
  TcgEvent->EventType  = EV_EFI_GPT_EVENT;
  TcgEvent->EventSize  = EventSize;
  GptData = (EFI_GPT_DATA *) TcgEvent->Event;

  //
  // Copy the EFI_PARTITION_TABLE_HEADER and NumberOfPartition
  //
  CopyMem ((UINT8 *)GptData, (UINT8*)PrimaryHeader, sizeof (EFI_PARTITION_TABLE_HEADER));
  GptData->NumberOfPartitions = NumberOfPartition;
  //
  // Copy the valid partition entry
  //
  PartitionEntry    = (EFI_PARTITION_ENTRY*)EntryPtr;
  NumberOfPartition = 0;
  for (Index = 0; Index < PrimaryHeader->NumberOfPartitionEntries; Index++) {
    if (!IsZeroGuid (&PartitionEntry->PartitionTypeGUID)) {
      CopyMem (
        (UINT8 *)&GptData->Partitions + NumberOfPartition * PrimaryHeader->SizeOfPartitionEntry,
        (UINT8 *)PartitionEntry,
        PrimaryHeader->SizeOfPartitionEntry
        );
      NumberOfPartition++;
    }
    PartitionEntry =(EFI_PARTITION_ENTRY *)((UINT8 *)PartitionEntry + PrimaryHeader->SizeOfPartitionEntry);
  }

  //
  // Measure the GPT data
  //
  EventNumber = 1;
  Status = TcgProtocol->HashLogExtendEvent (
             TcgProtocol,
             (EFI_PHYSICAL_ADDRESS) (UINTN) (VOID *) GptData,
             (UINT64) TcgEvent->EventSize,
             TPM_ALG_SHA,
             TcgEvent,
             &EventNumber,
             &EventLogLastEntry
             );
  if (!EFI_ERROR (Status)) {
    mMeasureGptCount++;
  }

  FreePool (PrimaryHeader);
  FreePool (EntryPtr);
  FreePool (TcgEvent);

  return Status;
}

/**
  Measure PE image into TPM log based on the authenticode image hashing in
  PE/COFF Specification 8.0 Appendix A.

  Caution: This function may receive untrusted input.
  PE/COFF image is external input, so this function will validate its data structure
  within this image buffer before use.

  Notes: PE/COFF image has been checked by BasePeCoffLib PeCoffLoaderGetImageInfo() in
  its caller function DxeTpmMeasureBootHandler().

  @param[in] TcgProtocol    Pointer to the located TCG protocol instance.
  @param[in] ImageAddress   Start address of image buffer.
  @param[in] ImageSize      Image size
  @param[in] LinkTimeBase   Address that the image is loaded into memory.
  @param[in] ImageType      Image subsystem type.
  @param[in] FilePath       File path is corresponding to the input image.

  @retval EFI_SUCCESS            Successfully measure image.
  @retval EFI_OUT_OF_RESOURCES   No enough resource to measure image.
  @retval EFI_UNSUPPORTED        ImageType is unsupported or PE image is mal-format.
  @retval other error value

**/
EFI_STATUS
EFIAPI
TcgMeasurePeImage (
  IN  EFI_TCG_PROTOCOL          *TcgProtocol,
  IN  EFI_PHYSICAL_ADDRESS      ImageAddress,
  IN  UINTN                     ImageSize,
  IN  UINTN                     LinkTimeBase,
  IN  UINT16                    ImageType,
  IN  EFI_DEVICE_PATH_PROTOCOL  *FilePath
  )
{
  EFI_STATUS                           Status;
  TCG_PCR_EVENT                        *TcgEvent;
  EFI_IMAGE_LOAD_EVENT                 *ImageLoad;
  UINT32                               FilePathSize;
  VOID                                 *Sha1Ctx;
  UINTN                                CtxSize;
  EFI_IMAGE_DOS_HEADER                 *DosHdr;
  UINT32                               PeCoffHeaderOffset;
  EFI_IMAGE_SECTION_HEADER             *Section;
  UINT8                                *HashBase;
  UINTN                                HashSize;
  UINTN                                SumOfBytesHashed;
  EFI_IMAGE_SECTION_HEADER             *SectionHeader;
  UINTN                                Index;
  UINTN                                Pos;
  UINT32                               EventSize;
  UINT32                               EventNumber;
  EFI_PHYSICAL_ADDRESS                 EventLogLastEntry;
  EFI_IMAGE_OPTIONAL_HEADER_PTR_UNION  Hdr;
  UINT32                               NumberOfRvaAndSizes;
  BOOLEAN                              HashStatus;
  UINT32                               CertSize;

  Status        = EFI_UNSUPPORTED;
  ImageLoad     = NULL;
  SectionHeader = NULL;
  Sha1Ctx       = NULL;
  FilePathSize  = (UINT32) GetDevicePathSize (FilePath);

  //
  // Determine destination PCR by BootPolicy
  //
  EventSize = sizeof (*ImageLoad) - sizeof (ImageLoad->DevicePath) + FilePathSize;
  TcgEvent = AllocateZeroPool (EventSize + sizeof (TCG_PCR_EVENT));
  if (TcgEvent == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  TcgEvent->EventSize = EventSize;
  ImageLoad           = (EFI_IMAGE_LOAD_EVENT *) TcgEvent->Event;

  switch (ImageType) {
    case EFI_IMAGE_SUBSYSTEM_EFI_APPLICATION:
      TcgEvent->EventType = EV_EFI_BOOT_SERVICES_APPLICATION;
      TcgEvent->PCRIndex  = 4;
      break;
    case EFI_IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER:
      TcgEvent->EventType = EV_EFI_BOOT_SERVICES_DRIVER;
      TcgEvent->PCRIndex  = 2;
      break;
    case EFI_IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER:
      TcgEvent->EventType = EV_EFI_RUNTIME_SERVICES_DRIVER;
      TcgEvent->PCRIndex  = 2;
      break;
    default:
      DEBUG ((
        EFI_D_ERROR,
        "TcgMeasurePeImage: Unknown subsystem type %d",
        ImageType
        ));
      goto Finish;
  }

  ImageLoad->ImageLocationInMemory = ImageAddress;
  ImageLoad->ImageLengthInMemory   = ImageSize;
  ImageLoad->ImageLinkTimeAddress  = LinkTimeBase;
  ImageLoad->LengthOfDevicePath    = FilePathSize;
  if ((FilePath != NULL) && (FilePathSize != 0)) {
    CopyMem (ImageLoad->DevicePath, FilePath, FilePathSize);
  }

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
  CtxSize = Sha1GetContextSize ();
  Sha1Ctx = AllocatePool (CtxSize);
  if (Sha1Ctx == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Finish;
  }

  HashStatus = Sha1Init (Sha1Ctx);
  if (!HashStatus) {
    goto Finish;
  }

  //
  // Measuring PE/COFF Image Header;
  // But CheckSum field and SECURITY data directory (certificate) are excluded
  //

  //
  // 3.  Calculate the distance from the base of the image header to the image checksum address.
  // 4.  Hash the image header from its base to beginning of the image checksum.
  //
  HashBase = (UINT8 *) (UINTN) ImageAddress;
  if (Hdr.Pe32->OptionalHeader.Magic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
    //
    // Use PE32 offset
    //
    NumberOfRvaAndSizes = Hdr.Pe32->OptionalHeader.NumberOfRvaAndSizes;
    HashSize = (UINTN) (&Hdr.Pe32->OptionalHeader.CheckSum) - (UINTN) HashBase;
  } else {
    //
    // Use PE32+ offset
    //
    NumberOfRvaAndSizes = Hdr.Pe32Plus->OptionalHeader.NumberOfRvaAndSizes;
    HashSize = (UINTN) (&Hdr.Pe32Plus->OptionalHeader.CheckSum) - (UINTN) HashBase;
  }

  HashStatus = Sha1Update (Sha1Ctx, HashBase, HashSize);
  if (!HashStatus) {
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
    if (Hdr.Pe32->OptionalHeader.Magic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
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
      HashStatus  = Sha1Update (Sha1Ctx, HashBase, HashSize);
      if (!HashStatus) {
        goto Finish;
      }
    }
  } else {
    //
    // 7.  Hash everything from the end of the checksum to the start of the Cert Directory.
    //
    if (Hdr.Pe32->OptionalHeader.Magic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
      //
      // Use PE32 offset
      //
      HashBase = (UINT8 *) &Hdr.Pe32->OptionalHeader.CheckSum + sizeof (UINT32);
      HashSize = (UINTN) (&Hdr.Pe32->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_SECURITY]) - (UINTN) HashBase;
    } else {
      //
      // Use PE32+ offset
      //
      HashBase = (UINT8 *) &Hdr.Pe32Plus->OptionalHeader.CheckSum + sizeof (UINT32);
      HashSize = (UINTN) (&Hdr.Pe32Plus->OptionalHeader.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_SECURITY]) - (UINTN) HashBase;
    }

    if (HashSize != 0) {
      HashStatus  = Sha1Update (Sha1Ctx, HashBase, HashSize);
      if (!HashStatus) {
        goto Finish;
      }
    }

    //
    // 8.  Skip over the Cert Directory. (It is sizeof(IMAGE_DATA_DIRECTORY) bytes.)
    // 9.  Hash everything from the end of the Cert Directory to the end of image header.
    //
    if (Hdr.Pe32->OptionalHeader.Magic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
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
      HashStatus  = Sha1Update (Sha1Ctx, HashBase, HashSize);
      if (!HashStatus) {
        goto Finish;
      }
    }
  }

  //
  // 10. Set the SUM_OF_BYTES_HASHED to the size of the header
  //
  if (Hdr.Pe32->OptionalHeader.Magic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
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

    HashStatus = Sha1Update (Sha1Ctx, HashBase, HashSize);
    if (!HashStatus) {
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
      if (Hdr.Pe32->OptionalHeader.Magic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
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

      HashStatus = Sha1Update (Sha1Ctx, HashBase, HashSize);
      if (!HashStatus) {
        goto Finish;
      }
    } else if (ImageSize < CertSize + SumOfBytesHashed) {
      goto Finish;
    }
  }

  //
  // 17.  Finalize the SHA hash.
  //
  HashStatus = Sha1Final (Sha1Ctx, (UINT8 *) &TcgEvent->Digest);
  if (!HashStatus) {
    goto Finish;
  }

  //
  // Log the PE data
  //
  EventNumber = 1;
  Status = TcgProtocol->HashLogExtendEvent (
             TcgProtocol,
             (EFI_PHYSICAL_ADDRESS) (UINTN) (VOID *) NULL,
             0,
             TPM_ALG_SHA,
             TcgEvent,
             &EventNumber,
             &EventLogLastEntry
             );
  if (Status == EFI_OUT_OF_RESOURCES) {
    //
    // Out of resource here means the image is hashed and its result is extended to PCR.
    // But the event log cann't be saved since log area is full.
    // Just return EFI_SUCCESS in order not to block the image load.
    //
    Status = EFI_SUCCESS;
  }

Finish:
  FreePool (TcgEvent);

  if (SectionHeader != NULL) {
    FreePool (SectionHeader);
  }

  if (Sha1Ctx != NULL ) {
    FreePool (Sha1Ctx);
  }
  return Status;
}

/**
  The security handler is used to abstract platform-specific policy
  from the DXE core response to an attempt to use a file that returns a
  given status for the authentication check from the section extraction protocol.

  The possible responses in a given SAP implementation may include locking
  flash upon failure to authenticate, attestation logging for all signed drivers,
  and other exception operations.  The File parameter allows for possible logging
  within the SAP of the driver.

  If File is NULL, then EFI_INVALID_PARAMETER is returned.

  If the file specified by File with an authentication status specified by
  AuthenticationStatus is safe for the DXE Core to use, then EFI_SUCCESS is returned.

  If the file specified by File with an authentication status specified by
  AuthenticationStatus is not safe for the DXE Core to use under any circumstances,
  then EFI_ACCESS_DENIED is returned.

  If the file specified by File with an authentication status specified by
  AuthenticationStatus is not safe for the DXE Core to use right now, but it
  might be possible to use it at a future time, then EFI_SECURITY_VIOLATION is
  returned.

  @param[in]      AuthenticationStatus  This is the authentication status returned
                                        from the securitymeasurement services for the
                                        input file.
  @param[in]      File       This is a pointer to the device path of the file that is
                             being dispatched. This will optionally be used for logging.
  @param[in]      FileBuffer File buffer matches the input file device path.
  @param[in]      FileSize   Size of File buffer matches the input file device path.
  @param[in]      BootPolicy A boot policy that was used to call LoadImage() UEFI service.

  @retval EFI_SUCCESS             The file specified by DevicePath and non-NULL
                                  FileBuffer did authenticate, and the platform policy dictates
                                  that the DXE Foundation may use the file.
  @retval other error value
**/
EFI_STATUS
EFIAPI
DxeTpmMeasureBootHandler (
  IN  UINT32                           AuthenticationStatus,
  IN  CONST EFI_DEVICE_PATH_PROTOCOL   *File,
  IN  VOID                             *FileBuffer,
  IN  UINTN                            FileSize,
  IN  BOOLEAN                          BootPolicy
  )
{
  EFI_TCG_PROTOCOL                    *TcgProtocol;
  EFI_STATUS                          Status;
  TCG_EFI_BOOT_SERVICE_CAPABILITY     ProtocolCapability;
  UINT32                              TCGFeatureFlags;
  EFI_PHYSICAL_ADDRESS                EventLogLocation;
  EFI_PHYSICAL_ADDRESS                EventLogLastEntry;
  EFI_DEVICE_PATH_PROTOCOL            *DevicePathNode;
  EFI_DEVICE_PATH_PROTOCOL            *OrigDevicePathNode;
  EFI_HANDLE                          Handle;
  EFI_HANDLE                          TempHandle;
  BOOLEAN                             ApplicationRequired;
  PE_COFF_LOADER_IMAGE_CONTEXT        ImageContext;
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *FvbProtocol;
  EFI_PHYSICAL_ADDRESS                FvAddress;
  UINT32                              Index;

  Status = gBS->LocateProtocol (&gEfiTcgProtocolGuid, NULL, (VOID **) &TcgProtocol);
  if (EFI_ERROR (Status)) {
    //
    // TCG protocol is not installed. So, TPM is not present.
    // Don't do any measurement, and directly return EFI_SUCCESS.
    //
    return EFI_SUCCESS;
  }

  ProtocolCapability.Size = (UINT8) sizeof (ProtocolCapability);
  Status = TcgProtocol->StatusCheck (
             TcgProtocol,
             &ProtocolCapability,
             &TCGFeatureFlags,
             &EventLogLocation,
             &EventLogLastEntry
           );
  if (EFI_ERROR (Status) || ProtocolCapability.TPMDeactivatedFlag || (!ProtocolCapability.TPMPresentFlag)) {
    //
    // TPM device doesn't work or activate.
    //
    return EFI_SUCCESS;
  }

  //
  // Copy File Device Path
  //
  OrigDevicePathNode = DuplicateDevicePath (File);

  //
  // 1. Check whether this device path support BlockIo protocol.
  // Is so, this device path may be a GPT device path.
  //
  DevicePathNode = OrigDevicePathNode;
  Status = gBS->LocateDevicePath (&gEfiBlockIoProtocolGuid, &DevicePathNode, &Handle);
  if (!EFI_ERROR (Status) && !mMeasureGptTableFlag) {
    //
    // Find the gpt partion on the given devicepath
    //
    DevicePathNode = OrigDevicePathNode;
    ASSERT (DevicePathNode != NULL);
    while (!IsDevicePathEnd (DevicePathNode)) {
      //
      // Find the Gpt partition
      //
      if (DevicePathType (DevicePathNode) == MEDIA_DEVICE_PATH &&
            DevicePathSubType (DevicePathNode) == MEDIA_HARDDRIVE_DP) {
        //
        // Check whether it is a gpt partition or not
        //
        if (((HARDDRIVE_DEVICE_PATH *) DevicePathNode)->MBRType == MBR_TYPE_EFI_PARTITION_TABLE_HEADER &&
            ((HARDDRIVE_DEVICE_PATH *) DevicePathNode)->SignatureType == SIGNATURE_TYPE_GUID) {

          //
          // Change the partition device path to its parent device path (disk) and get the handle.
          //
          DevicePathNode->Type    = END_DEVICE_PATH_TYPE;
          DevicePathNode->SubType = END_ENTIRE_DEVICE_PATH_SUBTYPE;
          DevicePathNode          = OrigDevicePathNode;
          Status = gBS->LocateDevicePath (
                         &gEfiDiskIoProtocolGuid,
                         &DevicePathNode,
                         &Handle
                         );
          if (!EFI_ERROR (Status)) {
            //
            // Measure GPT disk.
            //
            Status = TcgMeasureGptTable (TcgProtocol, Handle);
            if (!EFI_ERROR (Status)) {
              //
              // GPT disk check done.
              //
              mMeasureGptTableFlag = TRUE;
            }
          }
          FreePool (OrigDevicePathNode);
          OrigDevicePathNode = DuplicateDevicePath (File);
          ASSERT (OrigDevicePathNode != NULL);
          break;
        }
      }
      DevicePathNode    = NextDevicePathNode (DevicePathNode);
    }
  }

  //
  // 2. Measure PE image.
  //
  ApplicationRequired = FALSE;

  //
  // Check whether this device path support FVB protocol.
  //
  DevicePathNode = OrigDevicePathNode;
  Status = gBS->LocateDevicePath (&gEfiFirmwareVolumeBlockProtocolGuid, &DevicePathNode, &Handle);
  if (!EFI_ERROR (Status)) {
    //
    // Don't check FV image, and directly return EFI_SUCCESS.
    // It can be extended to the specific FV authentication according to the different requirement.
    //
    if (IsDevicePathEnd (DevicePathNode)) {
      return EFI_SUCCESS;
    }
    //
    // The PE image from unmeasured Firmware volume need be measured
    // The PE image from measured Firmware volume will be mearsured according to policy below.
    //   If it is driver, do not measure
    //   If it is application, still measure.
    //
    ApplicationRequired = TRUE;

    if (mCacheMeasuredHandle != Handle && mMeasuredHobData != NULL) {
      //
      // Search for Root FV of this PE image
      //
      TempHandle = Handle;
      do {
        Status = gBS->HandleProtocol(
                        TempHandle,
                        &gEfiFirmwareVolumeBlockProtocolGuid,
                        (VOID**)&FvbProtocol
                        );
        TempHandle = FvbProtocol->ParentHandle;
      } while (!EFI_ERROR(Status) && FvbProtocol->ParentHandle != NULL);

      //
      // Search in measured FV Hob
      //
      Status = FvbProtocol->GetPhysicalAddress(FvbProtocol, &FvAddress);
      if (EFI_ERROR(Status)){
        return Status;
      }

      ApplicationRequired = FALSE;

      for (Index = 0; Index < mMeasuredHobData->Num; Index++) {
        if(mMeasuredHobData->MeasuredFvBuf[Index].BlobBase == FvAddress) {
          //
          // Cache measured FV for next measurement
          //
          mCacheMeasuredHandle = Handle;
          ApplicationRequired  = TRUE;
          break;
        }
      }
    }
  }

  //
  // File is not found.
  //
  if (FileBuffer == NULL) {
    Status = EFI_SECURITY_VIOLATION;
    goto Finish;
  }

  mTpmImageSize  = FileSize;
  mFileBuffer = FileBuffer;

  //
  // Measure PE Image
  //
  DevicePathNode = OrigDevicePathNode;
  ZeroMem (&ImageContext, sizeof (ImageContext));
  ImageContext.Handle    = (VOID *) FileBuffer;
  ImageContext.ImageRead = (PE_COFF_LOADER_READ_FILE) DxeTpmMeasureBootLibImageRead;

  //
  // Get information about the image being loaded
  //
  Status = PeCoffLoaderGetImageInfo (&ImageContext);
  if (EFI_ERROR (Status)) {
    //
    // The information can't be got from the invalid PeImage
    //
    goto Finish;
  }

  //
  // Measure only application if Application flag is set
  // Measure drivers and applications if Application flag is not set
  //
  if ((!ApplicationRequired) ||
        (ApplicationRequired && ImageContext.ImageType == EFI_IMAGE_SUBSYSTEM_EFI_APPLICATION)) {
    //
    // Print the image path to be measured.
    //
    DEBUG_CODE_BEGIN ();
      CHAR16                            *ToText;
      ToText = ConvertDevicePathToText (
                 DevicePathNode,
                 FALSE,
                 TRUE
                 );
      if (ToText != NULL) {
        DEBUG ((DEBUG_INFO, "The measured image path is %s.\n", ToText));
        FreePool (ToText);
      }
    DEBUG_CODE_END ();

    //
    // Measure PE image into TPM log.
    //
    Status = TcgMeasurePeImage (
               TcgProtocol,
               (EFI_PHYSICAL_ADDRESS) (UINTN) FileBuffer,
               FileSize,
               (UINTN) ImageContext.ImageAddress,
               ImageContext.ImageType,
               DevicePathNode
               );
  }

  //
  // Done, free the allocated resource.
  //
Finish:
  if (OrigDevicePathNode != NULL) {
    FreePool (OrigDevicePathNode);
  }

  return Status;
}

/**
  Register the security handler to provide TPM measure boot service.

  @param  ImageHandle  ImageHandle of the loaded driver.
  @param  SystemTable  Pointer to the EFI System Table.

  @retval  EFI_SUCCESS            Register successfully.
  @retval  EFI_OUT_OF_RESOURCES   No enough memory to register this handler.
**/
EFI_STATUS
EFIAPI
DxeTpmMeasureBootLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_HOB_GUID_TYPE  *GuidHob;

  GuidHob = NULL;

  GuidHob = GetFirstGuidHob (&gMeasuredFvHobGuid);

  if (GuidHob != NULL) {
    mMeasuredHobData = GET_GUID_HOB_DATA (GuidHob);
  }

  return RegisterSecurity2Handler (
          DxeTpmMeasureBootHandler,
          EFI_AUTH_OPERATION_MEASURE_IMAGE | EFI_AUTH_OPERATION_IMAGE_REQUIRED
          );
}
