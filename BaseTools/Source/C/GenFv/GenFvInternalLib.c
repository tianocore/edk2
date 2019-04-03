/** @file
This file contains the internal functions required to generate a Firmware Volume.

Copyright (c) 2004 - 2018, Intel Corporation. All rights reserved.<BR>
Portions Copyright (c) 2011 - 2013, ARM Ltd. All rights reserved.<BR>
Portions Copyright (c) 2016 HP Development Company, L.P.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

//
// Include files
//

#if defined(__FreeBSD__)
#include <uuid.h>
#elif defined(__GNUC__)
#include <uuid/uuid.h>
#endif
#ifdef __GNUC__
#include <sys/stat.h>
#endif
#include <string.h>
#ifndef __GNUC__
#include <io.h>
#endif
#include <assert.h>

#include <Guid/FfsSectionAlignmentPadding.h>

#include "WinNtInclude.h"
#include "GenFvInternalLib.h"
#include "FvLib.h"
#include "PeCoffLib.h"

#define ARMT_UNCONDITIONAL_JUMP_INSTRUCTION       0xEB000000
#define ARM64_UNCONDITIONAL_JUMP_INSTRUCTION      0x14000000

BOOLEAN mArm = FALSE;
STATIC UINT32   MaxFfsAlignment = 0;
BOOLEAN VtfFileFlag = FALSE;

EFI_GUID  mEfiFirmwareVolumeTopFileGuid       = EFI_FFS_VOLUME_TOP_FILE_GUID;
EFI_GUID  mFileGuidArray [MAX_NUMBER_OF_FILES_IN_FV];
EFI_GUID  mZeroGuid                           = {0x0, 0x0, 0x0, {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}};
EFI_GUID  mDefaultCapsuleGuid                 = {0x3B6686BD, 0x0D76, 0x4030, { 0xB7, 0x0E, 0xB5, 0x51, 0x9E, 0x2F, 0xC5, 0xA0 }};
EFI_GUID  mEfiFfsSectionAlignmentPaddingGuid  = EFI_FFS_SECTION_ALIGNMENT_PADDING_GUID;

CHAR8      *mFvbAttributeName[] = {
  EFI_FVB2_READ_DISABLED_CAP_STRING,
  EFI_FVB2_READ_ENABLED_CAP_STRING,
  EFI_FVB2_READ_STATUS_STRING,
  EFI_FVB2_WRITE_DISABLED_CAP_STRING,
  EFI_FVB2_WRITE_ENABLED_CAP_STRING,
  EFI_FVB2_WRITE_STATUS_STRING,
  EFI_FVB2_LOCK_CAP_STRING,
  EFI_FVB2_LOCK_STATUS_STRING,
  NULL,
  EFI_FVB2_STICKY_WRITE_STRING,
  EFI_FVB2_MEMORY_MAPPED_STRING,
  EFI_FVB2_ERASE_POLARITY_STRING,
  EFI_FVB2_READ_LOCK_CAP_STRING,
  EFI_FVB2_READ_LOCK_STATUS_STRING,
  EFI_FVB2_WRITE_LOCK_CAP_STRING,
  EFI_FVB2_WRITE_LOCK_STATUS_STRING
};

CHAR8      *mFvbAlignmentName[] = {
  EFI_FVB2_ALIGNMENT_1_STRING,
  EFI_FVB2_ALIGNMENT_2_STRING,
  EFI_FVB2_ALIGNMENT_4_STRING,
  EFI_FVB2_ALIGNMENT_8_STRING,
  EFI_FVB2_ALIGNMENT_16_STRING,
  EFI_FVB2_ALIGNMENT_32_STRING,
  EFI_FVB2_ALIGNMENT_64_STRING,
  EFI_FVB2_ALIGNMENT_128_STRING,
  EFI_FVB2_ALIGNMENT_256_STRING,
  EFI_FVB2_ALIGNMENT_512_STRING,
  EFI_FVB2_ALIGNMENT_1K_STRING,
  EFI_FVB2_ALIGNMENT_2K_STRING,
  EFI_FVB2_ALIGNMENT_4K_STRING,
  EFI_FVB2_ALIGNMENT_8K_STRING,
  EFI_FVB2_ALIGNMENT_16K_STRING,
  EFI_FVB2_ALIGNMENT_32K_STRING,
  EFI_FVB2_ALIGNMENT_64K_STRING,
  EFI_FVB2_ALIGNMENT_128K_STRING,
  EFI_FVB2_ALIGNMENT_256K_STRING,
  EFI_FVB2_ALIGNMENT_512K_STRING,
  EFI_FVB2_ALIGNMENT_1M_STRING,
  EFI_FVB2_ALIGNMENT_2M_STRING,
  EFI_FVB2_ALIGNMENT_4M_STRING,
  EFI_FVB2_ALIGNMENT_8M_STRING,
  EFI_FVB2_ALIGNMENT_16M_STRING,
  EFI_FVB2_ALIGNMENT_32M_STRING,
  EFI_FVB2_ALIGNMENT_64M_STRING,
  EFI_FVB2_ALIGNMENT_128M_STRING,
  EFI_FVB2_ALIGNMENT_256M_STRING,
  EFI_FVB2_ALIGNMENT_512M_STRING,
  EFI_FVB2_ALIGNMENT_1G_STRING,
  EFI_FVB2_ALIGNMENT_2G_STRING
};

//
// This data array will be located at the base of the Firmware Volume Header (FVH)
// in the boot block.  It must not exceed 14 bytes of code.  The last 2 bytes
// will be used to keep the FVH checksum consistent.
// This code will be run in response to a startup IPI for HT-enabled systems.
//
#define SIZEOF_STARTUP_DATA_ARRAY 0x10

UINT8                                   m128kRecoveryStartupApDataArray[SIZEOF_STARTUP_DATA_ARRAY] = {
  //
  // EA D0 FF 00 F0               ; far jmp F000:FFD0
  // 0, 0, 0, 0, 0, 0, 0, 0, 0,   ; Reserved bytes
  // 0, 0                         ; Checksum Padding
  //
  0xEA,
  0xD0,
  0xFF,
  0x0,
  0xF0,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00
};

UINT8                                   m64kRecoveryStartupApDataArray[SIZEOF_STARTUP_DATA_ARRAY] = {
  //
  // EB CE                               ; jmp short ($-0x30)
  // ; (from offset 0x0 to offset 0xFFD0)
  // 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, ; Reserved bytes
  // 0, 0                                ; Checksum Padding
  //
  0xEB,
  0xCE,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00,
  0x00
};

FV_INFO                     mFvDataInfo;
CAP_INFO                    mCapDataInfo;
BOOLEAN                     mIsLargeFfs = FALSE;

EFI_PHYSICAL_ADDRESS mFvBaseAddress[0x10];
UINT32               mFvBaseAddressNumber = 0;

EFI_STATUS
ParseFvInf (
  IN  MEMORY_FILE  *InfFile,
  OUT FV_INFO      *FvInfo
  )
/*++

Routine Description:

  This function parses a FV.INF file and copies info into a FV_INFO structure.

Arguments:

  InfFile         Memory file image.
  FvInfo          Information read from INF file.

Returns:

  EFI_SUCCESS       INF file information successfully retrieved.
  EFI_ABORTED       INF file has an invalid format.
  EFI_NOT_FOUND     A required string was not found in the INF file.
--*/
{
  CHAR8       Value[MAX_LONG_FILE_PATH];
  UINT64      Value64;
  UINTN       Index;
  UINTN       Number;
  EFI_STATUS  Status;
  EFI_GUID    GuidValue;

  //
  // Read the FV base address
  //
  if (!mFvDataInfo.BaseAddressSet) {
    Status = FindToken (InfFile, OPTIONS_SECTION_STRING, EFI_FV_BASE_ADDRESS_STRING, 0, Value);
    if (Status == EFI_SUCCESS) {
      //
      // Get the base address
      //
      Status = AsciiStringToUint64 (Value, FALSE, &Value64);
      if (EFI_ERROR (Status)) {
        Error (NULL, 0, 2000, "Invalid parameter", "%s = %s", EFI_FV_BASE_ADDRESS_STRING, Value);
        return EFI_ABORTED;
      }
      DebugMsg (NULL, 0, 9, "rebase address", "%s = %s", EFI_FV_BASE_ADDRESS_STRING, Value);

      FvInfo->BaseAddress = Value64;
      FvInfo->BaseAddressSet = TRUE;
    }
  }

  //
  // Read the FV File System Guid
  //
  if (!FvInfo->FvFileSystemGuidSet) {
    Status = FindToken (InfFile, OPTIONS_SECTION_STRING, EFI_FV_FILESYSTEMGUID_STRING, 0, Value);
    if (Status == EFI_SUCCESS) {
      //
      // Get the guid value
      //
      Status = StringToGuid (Value, &GuidValue);
      if (EFI_ERROR (Status)) {
        Error (NULL, 0, 2000, "Invalid parameter", "%s = %s", EFI_FV_FILESYSTEMGUID_STRING, Value);
        return EFI_ABORTED;
      }
      memcpy (&FvInfo->FvFileSystemGuid, &GuidValue, sizeof (EFI_GUID));
      FvInfo->FvFileSystemGuidSet = TRUE;
    }
  }

  //
  // Read the FV Extension Header File Name
  //
  Status = FindToken (InfFile, ATTRIBUTES_SECTION_STRING, EFI_FV_EXT_HEADER_FILE_NAME, 0, Value);
  if (Status == EFI_SUCCESS) {
    strcpy (FvInfo->FvExtHeaderFile, Value);
  }

  //
  // Read the FV file name
  //
  Status = FindToken (InfFile, OPTIONS_SECTION_STRING, EFI_FV_FILE_NAME_STRING, 0, Value);
  if (Status == EFI_SUCCESS) {
    //
    // copy the file name
    //
    strcpy (FvInfo->FvName, Value);
  }

  //
  // Read Fv Attribute
  //
  for (Index = 0; Index < sizeof (mFvbAttributeName)/sizeof (CHAR8 *); Index ++) {
    if ((mFvbAttributeName [Index] != NULL) && \
        (FindToken (InfFile, ATTRIBUTES_SECTION_STRING, mFvbAttributeName [Index], 0, Value) == EFI_SUCCESS)) {
      if ((strcmp (Value, TRUE_STRING) == 0) || (strcmp (Value, ONE_STRING) == 0)) {
        FvInfo->FvAttributes |= 1 << Index;
      } else if ((strcmp (Value, FALSE_STRING) != 0) && (strcmp (Value, ZERO_STRING) != 0)) {
        Error (NULL, 0, 2000, "Invalid parameter", "%s expected %s | %s", mFvbAttributeName [Index], TRUE_STRING, FALSE_STRING);
        return EFI_ABORTED;
      }
    }
  }

  //
  // Read Fv Alignment
  //
  for (Index = 0; Index < sizeof (mFvbAlignmentName)/sizeof (CHAR8 *); Index ++) {
    if (FindToken (InfFile, ATTRIBUTES_SECTION_STRING, mFvbAlignmentName [Index], 0, Value) == EFI_SUCCESS) {
      if (strcmp (Value, TRUE_STRING) == 0) {
        FvInfo->FvAttributes |= Index << 16;
        DebugMsg (NULL, 0, 9, "FV file alignment", "Align = %s", mFvbAlignmentName [Index]);
        break;
      }
    }
  }

  //
  // Read weak alignment flag
  //
  Status = FindToken (InfFile, ATTRIBUTES_SECTION_STRING, EFI_FV_WEAK_ALIGNMENT_STRING, 0, Value);
  if (Status == EFI_SUCCESS) {
    if ((strcmp (Value, TRUE_STRING) == 0) || (strcmp (Value, ONE_STRING) == 0)) {
      FvInfo->FvAttributes |= EFI_FVB2_WEAK_ALIGNMENT;
    } else if ((strcmp (Value, FALSE_STRING) != 0) && (strcmp (Value, ZERO_STRING) != 0)) {
      Error (NULL, 0, 2000, "Invalid parameter", "Weak alignment value expected one of TRUE, FALSE, 1 or 0.");
      return EFI_ABORTED;
    }
  }

  //
  // Read block maps
  //
  for (Index = 0; Index < MAX_NUMBER_OF_FV_BLOCKS; Index++) {
    if (FvInfo->FvBlocks[Index].Length == 0) {
      //
      // Read block size
      //
      Status = FindToken (InfFile, OPTIONS_SECTION_STRING, EFI_BLOCK_SIZE_STRING, Index, Value);

      if (Status == EFI_SUCCESS) {
        //
        // Update the size of block
        //
        Status = AsciiStringToUint64 (Value, FALSE, &Value64);
        if (EFI_ERROR (Status)) {
          Error (NULL, 0, 2000, "Invalid parameter", "%s = %s", EFI_BLOCK_SIZE_STRING, Value);
          return EFI_ABORTED;
        }

        FvInfo->FvBlocks[Index].Length = (UINT32) Value64;
        DebugMsg (NULL, 0, 9, "FV Block Size", "%s = %s", EFI_BLOCK_SIZE_STRING, Value);
      } else {
        //
        // If there is no blocks size, but there is the number of block, then we have a mismatched pair
        // and should return an error.
        //
        Status = FindToken (InfFile, OPTIONS_SECTION_STRING, EFI_NUM_BLOCKS_STRING, Index, Value);
        if (!EFI_ERROR (Status)) {
          Error (NULL, 0, 2000, "Invalid parameter", "both %s and %s must be specified.", EFI_NUM_BLOCKS_STRING, EFI_BLOCK_SIZE_STRING);
          return EFI_ABORTED;
        } else {
          //
          // We are done
          //
          break;
        }
      }

      //
      // Read blocks number
      //
      Status = FindToken (InfFile, OPTIONS_SECTION_STRING, EFI_NUM_BLOCKS_STRING, Index, Value);

      if (Status == EFI_SUCCESS) {
        //
        // Update the number of blocks
        //
        Status = AsciiStringToUint64 (Value, FALSE, &Value64);
        if (EFI_ERROR (Status)) {
          Error (NULL, 0, 2000, "Invalid parameter", "%s = %s", EFI_NUM_BLOCKS_STRING, Value);
          return EFI_ABORTED;
        }

        FvInfo->FvBlocks[Index].NumBlocks = (UINT32) Value64;
        DebugMsg (NULL, 0, 9, "FV Block Number", "%s = %s", EFI_NUM_BLOCKS_STRING, Value);
      }
    }
  }

  if (Index == 0) {
    Error (NULL, 0, 2001, "Missing required argument", "block size.");
    return EFI_ABORTED;
  }

  //
  // Read files
  //
  Number = 0;
  for (Number = 0; Number < MAX_NUMBER_OF_FILES_IN_FV; Number ++) {
    if (FvInfo->FvFiles[Number][0] == '\0') {
      break;
    }
  }

  for (Index = 0; Number + Index < MAX_NUMBER_OF_FILES_IN_FV; Index++) {
    //
    // Read the FFS file list
    //
    Status = FindToken (InfFile, FILES_SECTION_STRING, EFI_FILE_NAME_STRING, Index, Value);

    if (Status == EFI_SUCCESS) {
      //
      // Add the file
      //
      strcpy (FvInfo->FvFiles[Number + Index], Value);
      DebugMsg (NULL, 0, 9, "FV component file", "the %uth name is %s", (unsigned) Index, Value);
    } else {
      break;
    }
  }

  if ((Index + Number) == 0) {
    Warning (NULL, 0, 0, "FV components are not specified.", NULL);
  }

  return EFI_SUCCESS;
}

VOID
UpdateFfsFileState (
  IN EFI_FFS_FILE_HEADER          *FfsFile,
  IN EFI_FIRMWARE_VOLUME_HEADER   *FvHeader
  )
/*++

Routine Description:

  This function changes the FFS file attributes based on the erase polarity
  of the FV. Update the reserved bits of State to EFI_FVB2_ERASE_POLARITY.

Arguments:

  FfsFile   File header.
  FvHeader  FV header.

Returns:

  None

--*/
{
  if (FvHeader->Attributes & EFI_FVB2_ERASE_POLARITY) {
    FfsFile->State = (UINT8)~(FfsFile->State);
    // FfsFile->State |= ~(UINT8) EFI_FILE_ALL_STATE_BITS;
  }
}

EFI_STATUS
ReadFfsAlignment (
  IN EFI_FFS_FILE_HEADER    *FfsFile,
  IN OUT UINT32             *Alignment
  )
/*++

Routine Description:

  This function determines the alignment of the FFS input file from the file
  attributes.

Arguments:

  FfsFile       FFS file to parse
  Alignment     The minimum required alignment offset of the FFS file

Returns:

  EFI_SUCCESS              The function completed successfully.
  EFI_INVALID_PARAMETER    One of the input parameters was invalid.
  EFI_ABORTED              An error occurred.

--*/
{
  //
  // Verify input parameters.
  //
  if (FfsFile == NULL || Alignment == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  switch ((FfsFile->Attributes >> 3) & 0x07) {

  case 0:
    //
    // 1 byte alignment
    //if bit 1 have set, 128K byte alignment
    //
    if (FfsFile->Attributes & FFS_ATTRIB_DATA_ALIGNMENT2) {
      *Alignment = 17;
    } else {
      *Alignment = 0;
    }
    break;

  case 1:
    //
    // 16 byte alignment
    //if bit 1 have set, 256K byte alignment
    //
    if (FfsFile->Attributes & FFS_ATTRIB_DATA_ALIGNMENT2) {
      *Alignment = 18;
    } else {
      *Alignment = 4;
    }
    break;

  case 2:
    //
    // 128 byte alignment
    //if bit 1 have set, 512K byte alignment
    //
    if (FfsFile->Attributes & FFS_ATTRIB_DATA_ALIGNMENT2) {
      *Alignment = 19;
    } else {
      *Alignment = 7;
    }
    break;

  case 3:
    //
    // 512 byte alignment
    //if bit 1 have set, 1M byte alignment
    //
    if (FfsFile->Attributes & FFS_ATTRIB_DATA_ALIGNMENT2) {
      *Alignment = 20;
    } else {
      *Alignment = 9;
    }
    break;

  case 4:
    //
    // 1K byte alignment
    //if bit 1 have set, 2M byte alignment
    //
    if (FfsFile->Attributes & FFS_ATTRIB_DATA_ALIGNMENT2) {
      *Alignment = 21;
    } else {
      *Alignment = 10;
    }
    break;

  case 5:
    //
    // 4K byte alignment
    //if bit 1 have set, 4M byte alignment
    //
    if (FfsFile->Attributes & FFS_ATTRIB_DATA_ALIGNMENT2) {
      *Alignment = 22;
    } else {
      *Alignment = 12;
    }
    break;

  case 6:
    //
    // 32K byte alignment
    //if bit 1 have set , 8M byte alignment
    //
    if (FfsFile->Attributes & FFS_ATTRIB_DATA_ALIGNMENT2) {
      *Alignment = 23;
    } else {
      *Alignment = 15;
    }
    break;

  case 7:
    //
    // 64K byte alignment
    //if bit 1 have set, 16M alignment
    //
    if (FfsFile->Attributes & FFS_ATTRIB_DATA_ALIGNMENT2) {
      *Alignment = 24;
    } else {
      *Alignment = 16;
    }
    break;

  default:
    break;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
AddPadFile (
  IN OUT MEMORY_FILE  *FvImage,
  IN UINT32           DataAlignment,
  IN VOID             *FvEnd,
  IN EFI_FIRMWARE_VOLUME_EXT_HEADER *ExtHeader,
  IN UINT32           NextFfsSize
  )
/*++

Routine Description:

  This function adds a pad file to the FV image if it required to align the
  data of the next file.

Arguments:

  FvImage         The memory image of the FV to add it to.
                  The current offset must be valid.
  DataAlignment   The data alignment of the next FFS file.
  FvEnd           End of the empty data in FvImage.
  ExtHeader       PI FvExtHeader Optional

Returns:

  EFI_SUCCESS              The function completed successfully.
  EFI_INVALID_PARAMETER    One of the input parameters was invalid.
  EFI_OUT_OF_RESOURCES     Insufficient resources exist in the FV to complete
                           the pad file add.

--*/
{
  EFI_FFS_FILE_HEADER *PadFile;
  UINTN               PadFileSize;
  UINT32              NextFfsHeaderSize;
  UINT32              CurFfsHeaderSize;
  UINT32              Index;

  Index = 0;
  CurFfsHeaderSize = sizeof (EFI_FFS_FILE_HEADER);
  //
  // Verify input parameters.
  //
  if (FvImage == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Calculate the pad file size
  //

  //
  // Append extension header size
  //
  if (ExtHeader != NULL) {
    PadFileSize = ExtHeader->ExtHeaderSize;
    if (PadFileSize + sizeof (EFI_FFS_FILE_HEADER) >= MAX_FFS_SIZE) {
      CurFfsHeaderSize = sizeof (EFI_FFS_FILE_HEADER2);
    }
    PadFileSize += CurFfsHeaderSize;
  } else {
    NextFfsHeaderSize = sizeof (EFI_FFS_FILE_HEADER);
    if (NextFfsSize >= MAX_FFS_SIZE) {
      NextFfsHeaderSize = sizeof (EFI_FFS_FILE_HEADER2);
    }
    //
    // Check if a pad file is necessary
    //
    if (((UINTN) FvImage->CurrentFilePointer - (UINTN) FvImage->FileImage + NextFfsHeaderSize) % DataAlignment == 0) {
      return EFI_SUCCESS;
    }
    PadFileSize = (UINTN) FvImage->CurrentFilePointer - (UINTN) FvImage->FileImage + sizeof (EFI_FFS_FILE_HEADER) + NextFfsHeaderSize;
    //
    // Add whatever it takes to get to the next aligned address
    //
    while ((PadFileSize % DataAlignment) != 0) {
      PadFileSize++;
    }
    //
    // Subtract the next file header size
    //
    PadFileSize -= NextFfsHeaderSize;
    //
    // Subtract the starting offset to get size
    //
    PadFileSize -= (UINTN) FvImage->CurrentFilePointer - (UINTN) FvImage->FileImage;
  }

  //
  // Verify that we have enough space for the file header
  //
  if (((UINTN) FvImage->CurrentFilePointer + PadFileSize) > (UINTN) FvEnd) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Write pad file header
  //
  PadFile = (EFI_FFS_FILE_HEADER *) FvImage->CurrentFilePointer;

  //
  // Write PadFile FFS header with PadType, don't need to set PAD file guid in its header.
  //
  PadFile->Type       = EFI_FV_FILETYPE_FFS_PAD;
  PadFile->Attributes = 0;

  //
  // Write pad file size (calculated size minus next file header size)
  //
  if (PadFileSize >= MAX_FFS_SIZE) {
    memset(PadFile->Size, 0, sizeof(UINT8) * 3);
    ((EFI_FFS_FILE_HEADER2 *)PadFile)->ExtendedSize = PadFileSize;
    PadFile->Attributes |= FFS_ATTRIB_LARGE_FILE;
  } else {
    PadFile->Size[0]  = (UINT8) (PadFileSize & 0xFF);
    PadFile->Size[1]  = (UINT8) ((PadFileSize >> 8) & 0xFF);
    PadFile->Size[2]  = (UINT8) ((PadFileSize >> 16) & 0xFF);
  }

  //
  // Fill in checksums and state, they must be 0 for checksumming.
  //
  PadFile->IntegrityCheck.Checksum.Header = 0;
  PadFile->IntegrityCheck.Checksum.File   = 0;
  PadFile->State                          = 0;
  PadFile->IntegrityCheck.Checksum.Header = CalculateChecksum8 ((UINT8 *) PadFile, CurFfsHeaderSize);
  PadFile->IntegrityCheck.Checksum.File   = FFS_FIXED_CHECKSUM;

  PadFile->State = EFI_FILE_HEADER_CONSTRUCTION | EFI_FILE_HEADER_VALID | EFI_FILE_DATA_VALID;
  UpdateFfsFileState (
    (EFI_FFS_FILE_HEADER *) PadFile,
    (EFI_FIRMWARE_VOLUME_HEADER *) FvImage->FileImage
    );

  //
  // Update the current FV pointer
  //
  FvImage->CurrentFilePointer += PadFileSize;

  if (ExtHeader != NULL) {
    //
    // Copy Fv Extension Header and Set Fv Extension header offset
    //
    if (ExtHeader->ExtHeaderSize > sizeof (EFI_FIRMWARE_VOLUME_EXT_HEADER)) {
      for (Index = sizeof (EFI_FIRMWARE_VOLUME_EXT_HEADER); Index < ExtHeader->ExtHeaderSize;) {
        if (((EFI_FIRMWARE_VOLUME_EXT_ENTRY *)((UINT8 *)ExtHeader + Index))-> ExtEntryType == EFI_FV_EXT_TYPE_USED_SIZE_TYPE) {
          if (VtfFileFlag) {
            ((EFI_FIRMWARE_VOLUME_EXT_ENTRY_USED_SIZE_TYPE *)((UINT8 *)ExtHeader + Index))->UsedSize = mFvTotalSize;
          } else {
            ((EFI_FIRMWARE_VOLUME_EXT_ENTRY_USED_SIZE_TYPE *)((UINT8 *)ExtHeader + Index))->UsedSize = mFvTakenSize;
          }
          break;
        }
        Index += ((EFI_FIRMWARE_VOLUME_EXT_ENTRY *)((UINT8 *)ExtHeader + Index))-> ExtEntrySize;
      }
    }
    memcpy ((UINT8 *)PadFile + CurFfsHeaderSize, ExtHeader, ExtHeader->ExtHeaderSize);
    ((EFI_FIRMWARE_VOLUME_HEADER *) FvImage->FileImage)->ExtHeaderOffset = (UINT16) ((UINTN) ((UINT8 *)PadFile + CurFfsHeaderSize) - (UINTN) FvImage->FileImage);
    //
    // Make next file start at QWord Boundary
    //
    while (((UINTN) FvImage->CurrentFilePointer & (EFI_FFS_FILE_HEADER_ALIGNMENT - 1)) != 0) {
      FvImage->CurrentFilePointer++;
    }
  }

  return EFI_SUCCESS;
}

BOOLEAN
IsVtfFile (
  IN EFI_FFS_FILE_HEADER    *FileBuffer
  )
/*++

Routine Description:

  This function checks the header to validate if it is a VTF file

Arguments:

  FileBuffer     Buffer in which content of a file has been read.

Returns:

  TRUE    If this is a VTF file
  FALSE   If this is not a VTF file

--*/
{
  if (!memcmp (&FileBuffer->Name, &mEfiFirmwareVolumeTopFileGuid, sizeof (EFI_GUID))) {
    return TRUE;
  } else {
    return FALSE;
  }
}

EFI_STATUS
WriteMapFile (
  IN OUT FILE                  *FvMapFile,
  IN     CHAR8                 *FileName,
  IN     EFI_FFS_FILE_HEADER   *FfsFile,
  IN     EFI_PHYSICAL_ADDRESS  ImageBaseAddress,
  IN     PE_COFF_LOADER_IMAGE_CONTEXT *pImageContext
  )
/*++

Routine Description:

  This function gets the basic debug information (entrypoint, baseaddress, .text, .data section base address)
  from PE/COFF image and abstracts Pe Map file information and add them into FvMap file for Debug.

Arguments:

  FvMapFile             A pointer to FvMap File
  FileName              Ffs File PathName
  FfsFile               A pointer to Ffs file image.
  ImageBaseAddress      PeImage Base Address.
  pImageContext         Image Context Information.

Returns:

  EFI_SUCCESS           Added required map information.

--*/
{
  CHAR8                               PeMapFileName [MAX_LONG_FILE_PATH];
  CHAR8                               *Cptr, *Cptr2;
  CHAR8                               FileGuidName [MAX_LINE_LEN];
  FILE                                *PeMapFile;
  CHAR8                               Line [MAX_LINE_LEN];
  CHAR8                               KeyWord [MAX_LINE_LEN];
  CHAR8                               FunctionName [MAX_LINE_LEN];
  EFI_PHYSICAL_ADDRESS                FunctionAddress;
  UINT32                              FunctionType;
  CHAR8                               FunctionTypeName [MAX_LINE_LEN];
  UINT32                              Index;
  UINT32                              AddressOfEntryPoint;
  UINT32                              Offset;
  EFI_IMAGE_OPTIONAL_HEADER_UNION     *ImgHdr;
  EFI_TE_IMAGE_HEADER                 *TEImageHeader;
  EFI_IMAGE_SECTION_HEADER            *SectionHeader;
  long long                           TempLongAddress;
  UINT32                              TextVirtualAddress;
  UINT32                              DataVirtualAddress;
  EFI_PHYSICAL_ADDRESS                LinkTimeBaseAddress;

  //
  // Init local variable
  //
  FunctionType = 0;
  //
  // Print FileGuid to string buffer.
  //
  PrintGuidToBuffer (&FfsFile->Name, (UINT8 *)FileGuidName, MAX_LINE_LEN, TRUE);

  //
  // Construct Map file Name
  //
  if (strlen (FileName) >= MAX_LONG_FILE_PATH) {
    return EFI_ABORTED;
  }
  strncpy (PeMapFileName, FileName, MAX_LONG_FILE_PATH - 1);
  PeMapFileName[MAX_LONG_FILE_PATH - 1] = 0;

  //
  // Change '\\' to '/', unified path format.
  //
  Cptr = PeMapFileName;
  while (*Cptr != '\0') {
    if (*Cptr == '\\') {
      *Cptr = FILE_SEP_CHAR;
    }
    Cptr ++;
  }

  //
  // Get Map file
  //
  Cptr = PeMapFileName + strlen (PeMapFileName);
  while ((*Cptr != '.') && (Cptr >= PeMapFileName)) {
    Cptr --;
  }
  if (Cptr < PeMapFileName) {
    return EFI_NOT_FOUND;
  } else {
    *(Cptr + 1) = 'm';
    *(Cptr + 2) = 'a';
    *(Cptr + 3) = 'p';
    *(Cptr + 4) = '\0';
  }

  //
  // Get module Name
  //
  Cptr2 = Cptr;
  while ((*Cptr != FILE_SEP_CHAR) && (Cptr >= PeMapFileName)) {
    Cptr --;
  }
  *Cptr2 = '\0';
  if (strlen (Cptr + 1) >= MAX_LINE_LEN) {
    return EFI_ABORTED;
  }
  strncpy (KeyWord, Cptr + 1, MAX_LINE_LEN - 1);
  KeyWord[MAX_LINE_LEN - 1] = 0;
  *Cptr2 = '.';

  //
  // AddressOfEntryPoint and Offset in Image
  //
  if (!pImageContext->IsTeImage) {
    ImgHdr = (EFI_IMAGE_OPTIONAL_HEADER_UNION *) ((UINT8 *) pImageContext->Handle + pImageContext->PeCoffHeaderOffset);
    AddressOfEntryPoint = ImgHdr->Pe32.OptionalHeader.AddressOfEntryPoint;
    Offset = 0;
    SectionHeader = (EFI_IMAGE_SECTION_HEADER *) (
                       (UINT8 *) ImgHdr +
                       sizeof (UINT32) +
                       sizeof (EFI_IMAGE_FILE_HEADER) +
                       ImgHdr->Pe32.FileHeader.SizeOfOptionalHeader
                       );
    Index = ImgHdr->Pe32.FileHeader.NumberOfSections;
  } else {
    TEImageHeader = (EFI_TE_IMAGE_HEADER *) pImageContext->Handle;
    AddressOfEntryPoint = TEImageHeader->AddressOfEntryPoint;
    Offset = TEImageHeader->StrippedSize - sizeof (EFI_TE_IMAGE_HEADER);
    SectionHeader = (EFI_IMAGE_SECTION_HEADER *) (TEImageHeader + 1);
    Index = TEImageHeader->NumberOfSections;
  }

  //
  // module information output
  //
  if (ImageBaseAddress == 0) {
    fprintf (FvMapFile, "%s (dummy) (", KeyWord);
    fprintf (FvMapFile, "BaseAddress=%010llx, ", (unsigned long long) ImageBaseAddress);
  } else {
    fprintf (FvMapFile, "%s (Fixed Flash Address, ", KeyWord);
    fprintf (FvMapFile, "BaseAddress=0x%010llx, ", (unsigned long long) (ImageBaseAddress + Offset));
  }

  fprintf (FvMapFile, "EntryPoint=0x%010llx", (unsigned long long) (ImageBaseAddress + AddressOfEntryPoint));
  fprintf (FvMapFile, ")\n");

  fprintf (FvMapFile, "(GUID=%s", FileGuidName);
  TextVirtualAddress = 0;
  DataVirtualAddress = 0;
  for (; Index > 0; Index --, SectionHeader ++) {
    if (stricmp ((CHAR8 *)SectionHeader->Name, ".text") == 0) {
      TextVirtualAddress = SectionHeader->VirtualAddress;
    } else if (stricmp ((CHAR8 *)SectionHeader->Name, ".data") == 0) {
      DataVirtualAddress = SectionHeader->VirtualAddress;
    } else if (stricmp ((CHAR8 *)SectionHeader->Name, ".sdata") == 0) {
      DataVirtualAddress = SectionHeader->VirtualAddress;
    }
  }
  fprintf (FvMapFile, " .textbaseaddress=0x%010llx", (unsigned long long) (ImageBaseAddress + TextVirtualAddress));
  fprintf (FvMapFile, " .databaseaddress=0x%010llx", (unsigned long long) (ImageBaseAddress + DataVirtualAddress));
  fprintf (FvMapFile, ")\n\n");

  //
  // Open PeMapFile
  //
  PeMapFile = fopen (LongFilePath (PeMapFileName), "r");
  if (PeMapFile == NULL) {
    // fprintf (stdout, "can't open %s file to reading\n", PeMapFileName);
    return EFI_ABORTED;
  }
  VerboseMsg ("The map file is %s", PeMapFileName);

  //
  // Output Functions information into Fv Map file
  //
  LinkTimeBaseAddress = 0;
  while (fgets (Line, MAX_LINE_LEN, PeMapFile) != NULL) {
    //
    // Skip blank line
    //
    if (Line[0] == 0x0a) {
      FunctionType = 0;
      continue;
    }
    //
    // By Address and Static keyword
    //
    if (FunctionType == 0) {
      sscanf (Line, "%s", KeyWord);
      if (stricmp (KeyWord, "Address") == 0) {
        //
        // function list
        //
        FunctionType = 1;
        fgets (Line, MAX_LINE_LEN, PeMapFile);
      } else if (stricmp (KeyWord, "Static") == 0) {
        //
        // static function list
        //
        FunctionType = 2;
        fgets (Line, MAX_LINE_LEN, PeMapFile);
      } else if (stricmp (KeyWord, "Preferred") ==0) {
        sscanf (Line + strlen (" Preferred load address is"), "%llx", &TempLongAddress);
        LinkTimeBaseAddress = (UINT64) TempLongAddress;
      }
      continue;
    }
    //
    // Printf Function Information
    //
    if (FunctionType == 1) {
      sscanf (Line, "%s %s %llx %s", KeyWord, FunctionName, &TempLongAddress, FunctionTypeName);
      FunctionAddress = (UINT64) TempLongAddress;
      if (FunctionTypeName [1] == '\0' && (FunctionTypeName [0] == 'f' || FunctionTypeName [0] == 'F')) {
        fprintf (FvMapFile, "  0x%010llx    ", (unsigned long long) (ImageBaseAddress + FunctionAddress - LinkTimeBaseAddress));
        fprintf (FvMapFile, "%s\n", FunctionName);
      }
    } else if (FunctionType == 2) {
      sscanf (Line, "%s %s %llx %s", KeyWord, FunctionName, &TempLongAddress, FunctionTypeName);
      FunctionAddress = (UINT64) TempLongAddress;
      if (FunctionTypeName [1] == '\0' && (FunctionTypeName [0] == 'f' || FunctionTypeName [0] == 'F')) {
        fprintf (FvMapFile, "  0x%010llx    ", (unsigned long long) (ImageBaseAddress + FunctionAddress - LinkTimeBaseAddress));
        fprintf (FvMapFile, "%s\n", FunctionName);
      }
    }
  }
  //
  // Close PeMap file
  //
  fprintf (FvMapFile, "\n\n");
  fclose (PeMapFile);

  return EFI_SUCCESS;
}

STATIC
BOOLEAN
AdjustInternalFfsPadding (
  IN OUT  EFI_FFS_FILE_HEADER   *FfsFile,
  IN OUT  MEMORY_FILE           *FvImage,
  IN      UINTN                 Alignment,
  IN OUT  UINTN                 *FileSize
  )
/*++

Routine Description:

  This function looks for a dedicated alignment padding section in the FFS, and
  shrinks it to the size required to line up subsequent sections correctly.

Arguments:

  FfsFile               A pointer to Ffs file image.
  FvImage               The memory image of the FV to adjust it to.
  Alignment             Current file alignment
  FileSize              Reference to a variable holding the size of the FFS file

Returns:

  TRUE                  Padding section was found and updated successfully
  FALSE                 Otherwise

--*/
{
  EFI_FILE_SECTION_POINTER  PadSection;
  UINT8                     *Remainder;
  EFI_STATUS                Status;
  UINT32                    FfsHeaderLength;
  UINT32                    FfsFileLength;
  UINT32                    PadSize;
  UINTN                     Misalignment;
  EFI_FFS_INTEGRITY_CHECK   *IntegrityCheck;

  //
  // Figure out the misalignment: all FFS sections are aligned relative to the
  // start of the FFS payload, so use that as the base of the misalignment
  // computation.
  //
  FfsHeaderLength = GetFfsHeaderLength(FfsFile);
  Misalignment = (UINTN) FvImage->CurrentFilePointer -
                 (UINTN) FvImage->FileImage + FfsHeaderLength;
  Misalignment &= Alignment - 1;
  if (Misalignment == 0) {
    // Nothing to do, return success
    return TRUE;
  }

  //
  // We only apply this optimization to FFS files with the FIXED attribute set,
  // since the FFS will not be loadable at arbitrary offsets anymore after
  // we adjust the size of the padding section.
  //
  if ((FfsFile->Attributes & FFS_ATTRIB_FIXED) == 0) {
    return FALSE;
  }

  //
  // Look for a dedicated padding section that we can adjust to compensate
  // for the misalignment. If such a padding section exists, it precedes all
  // sections with alignment requirements, and so the adjustment will correct
  // all of them.
  //
  Status = GetSectionByType (FfsFile, EFI_SECTION_FREEFORM_SUBTYPE_GUID, 1,
             &PadSection);
  if (EFI_ERROR (Status) ||
      CompareGuid (&PadSection.FreeformSubtypeSection->SubTypeGuid,
        &mEfiFfsSectionAlignmentPaddingGuid) != 0) {
    return FALSE;
  }

  //
  // Find out if the size of the padding section is sufficient to compensate
  // for the misalignment.
  //
  PadSize = GetSectionFileLength (PadSection.CommonHeader);
  if (Misalignment > PadSize - sizeof (EFI_FREEFORM_SUBTYPE_GUID_SECTION)) {
    return FALSE;
  }

  //
  // Move the remainder of the FFS file towards the front, and adjust the
  // file size output parameter.
  //
  Remainder = (UINT8 *) PadSection.CommonHeader + PadSize;
  memmove (Remainder - Misalignment, Remainder,
           *FileSize - (UINTN) (Remainder - (UINTN) FfsFile));
  *FileSize -= Misalignment;

  //
  // Update the padding section's length with the new values. Note that the
  // padding is always < 64 KB, so we can ignore EFI_COMMON_SECTION_HEADER2
  // ExtendedSize.
  //
  PadSize -= Misalignment;
  PadSection.CommonHeader->Size[0] = (UINT8) (PadSize & 0xff);
  PadSection.CommonHeader->Size[1] = (UINT8) ((PadSize & 0xff00) >> 8);
  PadSection.CommonHeader->Size[2] = (UINT8) ((PadSize & 0xff0000) >> 16);

  //
  // Update the FFS header with the new overall length
  //
  FfsFileLength = GetFfsFileLength (FfsFile) - Misalignment;
  if (FfsHeaderLength > sizeof(EFI_FFS_FILE_HEADER)) {
    ((EFI_FFS_FILE_HEADER2 *)FfsFile)->ExtendedSize = FfsFileLength;
  } else {
    FfsFile->Size[0] = (UINT8) (FfsFileLength & 0x000000FF);
    FfsFile->Size[1] = (UINT8) ((FfsFileLength & 0x0000FF00) >> 8);
    FfsFile->Size[2] = (UINT8) ((FfsFileLength & 0x00FF0000) >> 16);
  }

  //
  // Clear the alignment bits: these have become meaningless now that we have
  // adjusted the padding section.
  //
  FfsFile->Attributes &= ~(FFS_ATTRIB_DATA_ALIGNMENT | FFS_ATTRIB_DATA_ALIGNMENT2);

  //
  // Recalculate the FFS header checksum. Instead of setting Header and State
  // both to zero, set Header to (UINT8)(-State) so State preserves its original
  // value
  //
  IntegrityCheck = &FfsFile->IntegrityCheck;
  IntegrityCheck->Checksum.Header = (UINT8) (0x100 - FfsFile->State);
  IntegrityCheck->Checksum.File = 0;

  IntegrityCheck->Checksum.Header = CalculateChecksum8 (
                                      (UINT8 *) FfsFile, FfsHeaderLength);

  if (FfsFile->Attributes & FFS_ATTRIB_CHECKSUM) {
    //
    // Ffs header checksum = zero, so only need to calculate ffs body.
    //
    IntegrityCheck->Checksum.File = CalculateChecksum8 (
                                      (UINT8 *) FfsFile + FfsHeaderLength,
                                      FfsFileLength - FfsHeaderLength);
  } else {
    IntegrityCheck->Checksum.File = FFS_FIXED_CHECKSUM;
  }

  return TRUE;
}

EFI_STATUS
AddFile (
  IN OUT MEMORY_FILE          *FvImage,
  IN FV_INFO                  *FvInfo,
  IN UINTN                    Index,
  IN OUT EFI_FFS_FILE_HEADER  **VtfFileImage,
  IN FILE                     *FvMapFile,
  IN FILE                     *FvReportFile
  )
/*++

Routine Description:

  This function adds a file to the FV image.  The file will pad to the
  appropriate alignment if required.

Arguments:

  FvImage       The memory image of the FV to add it to.  The current offset
                must be valid.
  FvInfo        Pointer to information about the FV.
  Index         The file in the FvInfo file list to add.
  VtfFileImage  A pointer to the VTF file within the FvImage.  If this is equal
                to the end of the FvImage then no VTF previously found.
  FvMapFile     Pointer to FvMap File
  FvReportFile  Pointer to FvReport File

Returns:

  EFI_SUCCESS              The function completed successfully.
  EFI_INVALID_PARAMETER    One of the input parameters was invalid.
  EFI_ABORTED              An error occurred.
  EFI_OUT_OF_RESOURCES     Insufficient resources exist to complete the add.

--*/
{
  FILE                  *NewFile;
  UINTN                 FileSize;
  UINT8                 *FileBuffer;
  UINTN                 NumBytesRead;
  UINT32                CurrentFileAlignment;
  EFI_STATUS            Status;
  UINTN                 Index1;
  UINT8                 FileGuidString[PRINTED_GUID_BUFFER_SIZE];

  Index1 = 0;
  //
  // Verify input parameters.
  //
  if (FvImage == NULL || FvInfo == NULL || FvInfo->FvFiles[Index][0] == 0 || VtfFileImage == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Read the file to add
  //
  NewFile = fopen (LongFilePath (FvInfo->FvFiles[Index]), "rb");

  if (NewFile == NULL) {
    Error (NULL, 0, 0001, "Error opening file", FvInfo->FvFiles[Index]);
    return EFI_ABORTED;
  }

  //
  // Get the file size
  //
  FileSize = _filelength (fileno (NewFile));

  //
  // Read the file into a buffer
  //
  FileBuffer = malloc (FileSize);
  if (FileBuffer == NULL) {
    fclose (NewFile);
    Error (NULL, 0, 4001, "Resource", "memory cannot be allocated!");
    return EFI_OUT_OF_RESOURCES;
  }

  NumBytesRead = fread (FileBuffer, sizeof (UINT8), FileSize, NewFile);

  //
  // Done with the file, from this point on we will just use the buffer read.
  //
  fclose (NewFile);

  //
  // Verify read successful
  //
  if (NumBytesRead != sizeof (UINT8) * FileSize) {
    free  (FileBuffer);
    Error (NULL, 0, 0004, "Error reading file", FvInfo->FvFiles[Index]);
    return EFI_ABORTED;
  }

  //
  // For None PI Ffs file, directly add them into FvImage.
  //
  if (!FvInfo->IsPiFvImage) {
    memcpy (FvImage->CurrentFilePointer, FileBuffer, FileSize);
    if (FvInfo->SizeofFvFiles[Index] > FileSize) {
      FvImage->CurrentFilePointer += FvInfo->SizeofFvFiles[Index];
    } else {
      FvImage->CurrentFilePointer += FileSize;
    }
    goto Done;
  }

  //
  // Verify Ffs file
  //
  Status = VerifyFfsFile ((EFI_FFS_FILE_HEADER *)FileBuffer);
  if (EFI_ERROR (Status)) {
    free (FileBuffer);
    Error (NULL, 0, 3000, "Invalid", "%s is not a valid FFS file.", FvInfo->FvFiles[Index]);
    return EFI_INVALID_PARAMETER;
  }

  //
  // Verify space exists to add the file
  //
  if (FileSize > (UINTN) ((UINTN) *VtfFileImage - (UINTN) FvImage->CurrentFilePointer)) {
    free (FileBuffer);
    Error (NULL, 0, 4002, "Resource", "FV space is full, not enough room to add file %s.", FvInfo->FvFiles[Index]);
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Verify the input file is the duplicated file in this Fv image
  //
  for (Index1 = 0; Index1 < Index; Index1 ++) {
    if (CompareGuid ((EFI_GUID *) FileBuffer, &mFileGuidArray [Index1]) == 0) {
      Error (NULL, 0, 2000, "Invalid parameter", "the %dth file and %uth file have the same file GUID.", (unsigned) Index1 + 1, (unsigned) Index + 1);
      PrintGuid ((EFI_GUID *) FileBuffer);
      free (FileBuffer);
      return EFI_INVALID_PARAMETER;
    }
  }
  CopyMem (&mFileGuidArray [Index], FileBuffer, sizeof (EFI_GUID));

  //
  // Update the file state based on polarity of the FV.
  //
  UpdateFfsFileState (
    (EFI_FFS_FILE_HEADER *) FileBuffer,
    (EFI_FIRMWARE_VOLUME_HEADER *) FvImage->FileImage
    );

  //
  // Check if alignment is required
  //
  ReadFfsAlignment ((EFI_FFS_FILE_HEADER *) FileBuffer, &CurrentFileAlignment);

  //
  // Find the largest alignment of all the FFS files in the FV
  //
  if (CurrentFileAlignment > MaxFfsAlignment) {
    MaxFfsAlignment = CurrentFileAlignment;
  }
  //
  // If we have a VTF file, add it at the top.
  //
  if (IsVtfFile ((EFI_FFS_FILE_HEADER *) FileBuffer)) {
    if ((UINTN) *VtfFileImage == (UINTN) FvImage->Eof) {
      //
      // No previous VTF, add this one.
      //
      *VtfFileImage = (EFI_FFS_FILE_HEADER *) (UINTN) ((UINTN) FvImage->FileImage + FvInfo->Size - FileSize);
      //
      // Sanity check. The file MUST align appropriately
      //
      if (((UINTN) *VtfFileImage + GetFfsHeaderLength((EFI_FFS_FILE_HEADER *)FileBuffer) - (UINTN) FvImage->FileImage) % (1 << CurrentFileAlignment)) {
        Error (NULL, 0, 3000, "Invalid", "VTF file cannot be aligned on a %u-byte boundary.", (unsigned) (1 << CurrentFileAlignment));
        free (FileBuffer);
        return EFI_ABORTED;
      }
      //
      // Rebase the PE or TE image in FileBuffer of FFS file for XIP
      // Rebase for the debug genfvmap tool
      //
      Status = FfsRebase (FvInfo, FvInfo->FvFiles[Index], (EFI_FFS_FILE_HEADER *) FileBuffer, (UINTN) *VtfFileImage - (UINTN) FvImage->FileImage, FvMapFile);
      if (EFI_ERROR (Status)) {
        Error (NULL, 0, 3000, "Invalid", "Could not rebase %s.", FvInfo->FvFiles[Index]);
        return Status;
      }
      //
      // copy VTF File
      //
      memcpy (*VtfFileImage, FileBuffer, FileSize);

      PrintGuidToBuffer ((EFI_GUID *) FileBuffer, FileGuidString, sizeof (FileGuidString), TRUE);
      fprintf (FvReportFile, "0x%08X %s\n", (unsigned)(UINTN) (((UINT8 *)*VtfFileImage) - (UINTN)FvImage->FileImage), FileGuidString);

      free (FileBuffer);
      DebugMsg (NULL, 0, 9, "Add VTF FFS file in FV image", NULL);
      return EFI_SUCCESS;
    } else {
      //
      // Already found a VTF file.
      //
      Error (NULL, 0, 3000, "Invalid", "multiple VTF files are not permitted within a single FV.");
      free (FileBuffer);
      return EFI_ABORTED;
    }
  }

  //
  // Add pad file if necessary
  //
  if (!AdjustInternalFfsPadding ((EFI_FFS_FILE_HEADER *) FileBuffer, FvImage,
         1 << CurrentFileAlignment, &FileSize)) {
    Status = AddPadFile (FvImage, 1 << CurrentFileAlignment, *VtfFileImage, NULL, FileSize);
    if (EFI_ERROR (Status)) {
      Error (NULL, 0, 4002, "Resource", "FV space is full, could not add pad file for data alignment property.");
      free (FileBuffer);
      return EFI_ABORTED;
    }
  }
  //
  // Add file
  //
  if ((UINTN) (FvImage->CurrentFilePointer + FileSize) <= (UINTN) (*VtfFileImage)) {
    //
    // Rebase the PE or TE image in FileBuffer of FFS file for XIP.
    // Rebase Bs and Rt drivers for the debug genfvmap tool.
    //
    Status = FfsRebase (FvInfo, FvInfo->FvFiles[Index], (EFI_FFS_FILE_HEADER *) FileBuffer, (UINTN) FvImage->CurrentFilePointer - (UINTN) FvImage->FileImage, FvMapFile);
  if (EFI_ERROR (Status)) {
    Error (NULL, 0, 3000, "Invalid", "Could not rebase %s.", FvInfo->FvFiles[Index]);
    return Status;
  }
    //
    // Copy the file
    //
    memcpy (FvImage->CurrentFilePointer, FileBuffer, FileSize);
    PrintGuidToBuffer ((EFI_GUID *) FileBuffer, FileGuidString, sizeof (FileGuidString), TRUE);
    fprintf (FvReportFile, "0x%08X %s\n", (unsigned) (FvImage->CurrentFilePointer - FvImage->FileImage), FileGuidString);
    FvImage->CurrentFilePointer += FileSize;
  } else {
    Error (NULL, 0, 4002, "Resource", "FV space is full, cannot add file %s.", FvInfo->FvFiles[Index]);
    free (FileBuffer);
    return EFI_ABORTED;
  }
  //
  // Make next file start at QWord Boundary
  //
  while (((UINTN) FvImage->CurrentFilePointer & (EFI_FFS_FILE_HEADER_ALIGNMENT - 1)) != 0) {
    FvImage->CurrentFilePointer++;
  }

Done:
  //
  // Free allocated memory.
  //
  free (FileBuffer);

  return EFI_SUCCESS;
}

EFI_STATUS
PadFvImage (
  IN MEMORY_FILE          *FvImage,
  IN EFI_FFS_FILE_HEADER  *VtfFileImage
  )
/*++

Routine Description:

  This function places a pad file between the last file in the FV and the VTF
  file if the VTF file exists.

Arguments:

  FvImage       Memory file for the FV memory image
  VtfFileImage  The address of the VTF file.  If this is the end of the FV
                image, no VTF exists and no pad file is needed.

Returns:

  EFI_SUCCESS             Completed successfully.
  EFI_INVALID_PARAMETER   One of the input parameters was NULL.

--*/
{
  EFI_FFS_FILE_HEADER *PadFile;
  UINTN               FileSize;
  UINT32              FfsHeaderSize;

  //
  // If there is no VTF or the VTF naturally follows the previous file without a
  // pad file, then there's nothing to do
  //
  if ((UINTN) VtfFileImage == (UINTN) FvImage->Eof || \
      ((UINTN) VtfFileImage == (UINTN) FvImage->CurrentFilePointer)) {
    return EFI_SUCCESS;
  }

  if ((UINTN) VtfFileImage < (UINTN) FvImage->CurrentFilePointer) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Pad file starts at beginning of free space
  //
  PadFile = (EFI_FFS_FILE_HEADER *) FvImage->CurrentFilePointer;

  //
  // write PadFile FFS header with PadType, don't need to set PAD file guid in its header.
  //
  PadFile->Type       = EFI_FV_FILETYPE_FFS_PAD;
  PadFile->Attributes = 0;

  //
  // FileSize includes the EFI_FFS_FILE_HEADER
  //
  FileSize          = (UINTN) VtfFileImage - (UINTN) FvImage->CurrentFilePointer;
  if (FileSize >= MAX_FFS_SIZE) {
    PadFile->Attributes |= FFS_ATTRIB_LARGE_FILE;
    memset(PadFile->Size, 0, sizeof(UINT8) * 3);
    ((EFI_FFS_FILE_HEADER2 *)PadFile)->ExtendedSize = FileSize;
    FfsHeaderSize = sizeof(EFI_FFS_FILE_HEADER2);
    mIsLargeFfs = TRUE;
  } else {
    PadFile->Size[0]  = (UINT8) (FileSize & 0x000000FF);
    PadFile->Size[1]  = (UINT8) ((FileSize & 0x0000FF00) >> 8);
    PadFile->Size[2]  = (UINT8) ((FileSize & 0x00FF0000) >> 16);
    FfsHeaderSize = sizeof(EFI_FFS_FILE_HEADER);
  }

  //
  // Fill in checksums and state, must be zero during checksum calculation.
  //
  PadFile->IntegrityCheck.Checksum.Header = 0;
  PadFile->IntegrityCheck.Checksum.File   = 0;
  PadFile->State                          = 0;
  PadFile->IntegrityCheck.Checksum.Header = CalculateChecksum8 ((UINT8 *) PadFile, FfsHeaderSize);
  PadFile->IntegrityCheck.Checksum.File   = FFS_FIXED_CHECKSUM;

  PadFile->State = EFI_FILE_HEADER_CONSTRUCTION | EFI_FILE_HEADER_VALID | EFI_FILE_DATA_VALID;

  UpdateFfsFileState (
    (EFI_FFS_FILE_HEADER *) PadFile,
    (EFI_FIRMWARE_VOLUME_HEADER *) FvImage->FileImage
    );
  //
  // Update the current FV pointer
  //
  FvImage->CurrentFilePointer = FvImage->Eof;

  return EFI_SUCCESS;
}

EFI_STATUS
UpdateResetVector (
  IN MEMORY_FILE            *FvImage,
  IN FV_INFO                *FvInfo,
  IN EFI_FFS_FILE_HEADER    *VtfFile
  )
/*++

Routine Description:

  This parses the FV looking for the PEI core and then plugs the address into
  the SALE_ENTRY point of the BSF/VTF for IPF and does BUGBUG TBD action to
  complete an IA32 Bootstrap FV.

Arguments:

  FvImage       Memory file for the FV memory image
  FvInfo        Information read from INF file.
  VtfFile       Pointer to the VTF file in the FV image.

Returns:

  EFI_SUCCESS             Function Completed successfully.
  EFI_ABORTED             Error encountered.
  EFI_INVALID_PARAMETER   A required parameter was NULL.
  EFI_NOT_FOUND           PEI Core file not found.

--*/
{
  EFI_FFS_FILE_HEADER       *PeiCoreFile;
  EFI_FFS_FILE_HEADER       *SecCoreFile;
  EFI_STATUS                Status;
  EFI_FILE_SECTION_POINTER  Pe32Section;
  UINT32                    EntryPoint;
  UINT32                    BaseOfCode;
  UINT16                    MachineType;
  EFI_PHYSICAL_ADDRESS      PeiCorePhysicalAddress;
  EFI_PHYSICAL_ADDRESS      SecCorePhysicalAddress;
  INT32                     Ia32SecEntryOffset;
  UINT32                    *Ia32ResetAddressPtr;
  UINT8                     *BytePointer;
  UINT8                     *BytePointer2;
  UINT16                    *WordPointer;
  UINT16                    CheckSum;
  UINT32                    IpiVector;
  UINTN                     Index;
  EFI_FFS_FILE_STATE        SavedState;
  BOOLEAN                   Vtf0Detected;
  UINT32                    FfsHeaderSize;
  UINT32                    SecHeaderSize;

  //
  // Verify input parameters
  //
  if (FvImage == NULL || FvInfo == NULL || VtfFile == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  //
  // Initialize FV library
  //
  InitializeFvLib (FvImage->FileImage, FvInfo->Size);

  //
  // Verify VTF file
  //
  Status = VerifyFfsFile (VtfFile);
  if (EFI_ERROR (Status)) {
    return EFI_INVALID_PARAMETER;
  }

  if (
      (((UINTN)FvImage->Eof - (UINTN)FvImage->FileImage) >=
        IA32_X64_VTF_SIGNATURE_OFFSET) &&
      (*(UINT32 *)(VOID*)((UINTN) FvImage->Eof -
                                  IA32_X64_VTF_SIGNATURE_OFFSET) ==
        IA32_X64_VTF0_SIGNATURE)
     ) {
    Vtf0Detected = TRUE;
  } else {
    Vtf0Detected = FALSE;
  }

  //
  // Find the Sec Core
  //
  Status = GetFileByType (EFI_FV_FILETYPE_SECURITY_CORE, 1, &SecCoreFile);
  if (EFI_ERROR (Status) || SecCoreFile == NULL) {
    if (Vtf0Detected) {
      //
      // If the SEC core file is not found, but the VTF-0 signature
      // is found, we'll treat it as a VTF-0 'Volume Top File'.
      // This means no modifications are required to the VTF.
      //
      return EFI_SUCCESS;
    }

    Error (NULL, 0, 3000, "Invalid", "could not find the SEC core file in the FV.");
    return EFI_ABORTED;
  }
  //
  // Sec Core found, now find PE32 section
  //
  Status = GetSectionByType (SecCoreFile, EFI_SECTION_PE32, 1, &Pe32Section);
  if (Status == EFI_NOT_FOUND) {
    Status = GetSectionByType (SecCoreFile, EFI_SECTION_TE, 1, &Pe32Section);
  }

  if (EFI_ERROR (Status)) {
    Error (NULL, 0, 3000, "Invalid", "could not find a PE32 section in the SEC core file.");
    return EFI_ABORTED;
  }

  SecHeaderSize = GetSectionHeaderLength(Pe32Section.CommonHeader);
  Status = GetPe32Info (
            (VOID *) ((UINTN) Pe32Section.Pe32Section + SecHeaderSize),
            &EntryPoint,
            &BaseOfCode,
            &MachineType
            );

  if (EFI_ERROR (Status)) {
    Error (NULL, 0, 3000, "Invalid", "could not get the PE32 entry point for the SEC core.");
    return EFI_ABORTED;
  }

  if (
       Vtf0Detected &&
       (MachineType == EFI_IMAGE_MACHINE_IA32 ||
        MachineType == EFI_IMAGE_MACHINE_X64)
     ) {
    //
    // If the SEC core code is IA32 or X64 and the VTF-0 signature
    // is found, we'll treat it as a VTF-0 'Volume Top File'.
    // This means no modifications are required to the VTF.
    //
    return EFI_SUCCESS;
  }

  //
  // Physical address is FV base + offset of PE32 + offset of the entry point
  //
  SecCorePhysicalAddress = FvInfo->BaseAddress;
  SecCorePhysicalAddress += (UINTN) Pe32Section.Pe32Section + SecHeaderSize - (UINTN) FvImage->FileImage;
  SecCorePhysicalAddress += EntryPoint;
  DebugMsg (NULL, 0, 9, "SecCore physical entry point address", "Address = 0x%llX", (unsigned long long) SecCorePhysicalAddress);

  //
  // Find the PEI Core
  //
  PeiCorePhysicalAddress = 0;
  Status = GetFileByType (EFI_FV_FILETYPE_PEI_CORE, 1, &PeiCoreFile);
  if (!EFI_ERROR (Status) && (PeiCoreFile != NULL)) {
    //
    // PEI Core found, now find PE32 or TE section
    //
    Status = GetSectionByType (PeiCoreFile, EFI_SECTION_PE32, 1, &Pe32Section);
    if (Status == EFI_NOT_FOUND) {
      Status = GetSectionByType (PeiCoreFile, EFI_SECTION_TE, 1, &Pe32Section);
    }

    if (EFI_ERROR (Status)) {
      Error (NULL, 0, 3000, "Invalid", "could not find either a PE32 or a TE section in PEI core file.");
      return EFI_ABORTED;
    }

    SecHeaderSize = GetSectionHeaderLength(Pe32Section.CommonHeader);
    Status = GetPe32Info (
              (VOID *) ((UINTN) Pe32Section.Pe32Section + SecHeaderSize),
              &EntryPoint,
              &BaseOfCode,
              &MachineType
              );

    if (EFI_ERROR (Status)) {
      Error (NULL, 0, 3000, "Invalid", "could not get the PE32 entry point for the PEI core.");
      return EFI_ABORTED;
    }
    //
    // Physical address is FV base + offset of PE32 + offset of the entry point
    //
    PeiCorePhysicalAddress = FvInfo->BaseAddress;
    PeiCorePhysicalAddress += (UINTN) Pe32Section.Pe32Section + SecHeaderSize - (UINTN) FvImage->FileImage;
    PeiCorePhysicalAddress += EntryPoint;
    DebugMsg (NULL, 0, 9, "PeiCore physical entry point address", "Address = 0x%llX", (unsigned long long) PeiCorePhysicalAddress);
  }

if (MachineType == EFI_IMAGE_MACHINE_IA32 || MachineType == EFI_IMAGE_MACHINE_X64) {
    if (PeiCorePhysicalAddress != 0) {
      //
      // Get the location to update
      //
      Ia32ResetAddressPtr  = (UINT32 *) ((UINTN) FvImage->Eof - IA32_PEI_CORE_ENTRY_OFFSET);

      //
      // Write lower 32 bits of physical address for Pei Core entry
      //
      *Ia32ResetAddressPtr = (UINT32) PeiCorePhysicalAddress;
    }
    //
    // Write SecCore Entry point relative address into the jmp instruction in reset vector.
    //
    Ia32ResetAddressPtr  = (UINT32 *) ((UINTN) FvImage->Eof - IA32_SEC_CORE_ENTRY_OFFSET);

    Ia32SecEntryOffset   = (INT32) (SecCorePhysicalAddress - (FV_IMAGES_TOP_ADDRESS - IA32_SEC_CORE_ENTRY_OFFSET + 2));
    if (Ia32SecEntryOffset <= -65536) {
      Error (NULL, 0, 3000, "Invalid", "The SEC EXE file size is too large, it must be less than 64K.");
      return STATUS_ERROR;
    }

    *(UINT16 *) Ia32ResetAddressPtr = (UINT16) Ia32SecEntryOffset;

    //
    // Update the BFV base address
    //
    Ia32ResetAddressPtr   = (UINT32 *) ((UINTN) FvImage->Eof - 4);
    *Ia32ResetAddressPtr  = (UINT32) (FvInfo->BaseAddress);
    DebugMsg (NULL, 0, 9, "update BFV base address in the top FV image", "BFV base address = 0x%llX.", (unsigned long long) FvInfo->BaseAddress);

    //
    // Update the Startup AP in the FVH header block ZeroVector region.
    //
    BytePointer   = (UINT8 *) ((UINTN) FvImage->FileImage);
    if (FvInfo->Size <= 0x10000) {
      BytePointer2 = m64kRecoveryStartupApDataArray;
    } else if (FvInfo->Size <= 0x20000) {
      BytePointer2 = m128kRecoveryStartupApDataArray;
    } else {
      BytePointer2 = m128kRecoveryStartupApDataArray;
      //
      // Find the position to place Ap reset vector, the offset
      // between the position and the end of Fvrecovery.fv file
      // should not exceed 128kB to prevent Ap reset vector from
      // outside legacy E and F segment
      //
      Status = FindApResetVectorPosition (FvImage, &BytePointer);
      if (EFI_ERROR (Status)) {
        Error (NULL, 0, 3000, "Invalid", "FV image does not have enough space to place AP reset vector. The FV image needs to reserve at least 4KB of unused space.");
        return EFI_ABORTED;
      }
    }

    for (Index = 0; Index < SIZEOF_STARTUP_DATA_ARRAY; Index++) {
      BytePointer[Index] = BytePointer2[Index];
    }
    //
    // Calculate the checksum
    //
    CheckSum              = 0x0000;
    WordPointer = (UINT16 *) (BytePointer);
    for (Index = 0; Index < SIZEOF_STARTUP_DATA_ARRAY / 2; Index++) {
      CheckSum = (UINT16) (CheckSum + ((UINT16) *WordPointer));
      WordPointer++;
    }
    //
    // Update the checksum field
    //
    WordPointer   = (UINT16 *) (BytePointer + SIZEOF_STARTUP_DATA_ARRAY - 2);
    *WordPointer  = (UINT16) (0x10000 - (UINT32) CheckSum);

    //
    // IpiVector at the 4k aligned address in the top 2 blocks in the PEI FV.
    //
    IpiVector  = (UINT32) (FV_IMAGES_TOP_ADDRESS - ((UINTN) FvImage->Eof - (UINTN) BytePointer));
    DebugMsg (NULL, 0, 9, "Startup AP Vector address", "IpiVector at 0x%X", (unsigned) IpiVector);
    if ((IpiVector & 0xFFF) != 0) {
      Error (NULL, 0, 3000, "Invalid", "Startup AP Vector address are not 4K aligned, because the FV size is not 4K aligned");
      return EFI_ABORTED;
    }
    IpiVector  = IpiVector >> 12;
    IpiVector  = IpiVector & 0xFF;

    //
    // Write IPI Vector at Offset FvrecoveryFileSize - 8
    //
    Ia32ResetAddressPtr   = (UINT32 *) ((UINTN) FvImage->Eof - 8);
    *Ia32ResetAddressPtr  = IpiVector;
  } else if (MachineType == EFI_IMAGE_MACHINE_ARMT) {
    //
    // Since the ARM reset vector is in the FV Header you really don't need a
    // Volume Top File, but if you have one for some reason don't crash...
    //
  } else if (MachineType == EFI_IMAGE_MACHINE_AARCH64) {
    //
    // Since the AArch64 reset vector is in the FV Header you really don't need a
    // Volume Top File, but if you have one for some reason don't crash...
    //
  } else {
    Error (NULL, 0, 3000, "Invalid", "machine type=0x%X in PEI core.", MachineType);
    return EFI_ABORTED;
  }

  //
  // Now update file checksum
  //
  SavedState  = VtfFile->State;
  VtfFile->IntegrityCheck.Checksum.File = 0;
  VtfFile->State                        = 0;
  if (VtfFile->Attributes & FFS_ATTRIB_CHECKSUM) {
    FfsHeaderSize = GetFfsHeaderLength(VtfFile);
    VtfFile->IntegrityCheck.Checksum.File = CalculateChecksum8 (
                                              (UINT8 *) ((UINT8 *)VtfFile + FfsHeaderSize),
                                              GetFfsFileLength (VtfFile) - FfsHeaderSize
                                              );
  } else {
    VtfFile->IntegrityCheck.Checksum.File = FFS_FIXED_CHECKSUM;
  }

  VtfFile->State = SavedState;

  return EFI_SUCCESS;
}

EFI_STATUS
FindCorePeSection(
  IN VOID                       *FvImageBuffer,
  IN UINT64                     FvSize,
  IN EFI_FV_FILETYPE            FileType,
  OUT EFI_FILE_SECTION_POINTER  *Pe32Section
  )
/*++

Routine Description:

  Recursively searches the FV for the FFS file of specified type (typically
  SEC or PEI core) and extracts the PE32 section for further processing.

Arguments:

  FvImageBuffer   Buffer containing FV data
  FvSize          Size of the FV
  FileType        Type of FFS file to search for
  Pe32Section     PE32 section pointer when FFS file is found.

Returns:

  EFI_SUCCESS             Function Completed successfully.
  EFI_ABORTED             Error encountered.
  EFI_INVALID_PARAMETER   A required parameter was NULL.
  EFI_NOT_FOUND           Core file not found.

--*/
{
  EFI_STATUS                  Status;
  EFI_FIRMWARE_VOLUME_HEADER  *OrigFvHeader;
  UINT32                      OrigFvLength;
  EFI_FFS_FILE_HEADER         *CoreFfsFile;
  UINTN                       FvImageFileCount;
  EFI_FFS_FILE_HEADER         *FvImageFile;
  UINTN                       EncapFvSectionCount;
  EFI_FILE_SECTION_POINTER    EncapFvSection;
  EFI_FIRMWARE_VOLUME_HEADER  *EncapsulatedFvHeader;

  if (Pe32Section == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Initialize FV library, saving previous values
  //
  OrigFvHeader = (EFI_FIRMWARE_VOLUME_HEADER *)NULL;
  GetFvHeader (&OrigFvHeader, &OrigFvLength);
  InitializeFvLib(FvImageBuffer, (UINT32)FvSize);

  //
  // First see if we can obtain the file directly in outer FV
  //
  Status = GetFileByType(FileType, 1, &CoreFfsFile);
  if (!EFI_ERROR(Status) && (CoreFfsFile != NULL) ) {

    //
    // Core found, now find PE32 or TE section
    //
    Status = GetSectionByType(CoreFfsFile, EFI_SECTION_PE32, 1, Pe32Section);
    if (EFI_ERROR(Status)) {
      Status = GetSectionByType(CoreFfsFile, EFI_SECTION_TE, 1, Pe32Section);
    }

    if (EFI_ERROR(Status)) {
      Error(NULL, 0, 3000, "Invalid", "could not find a PE32 section in the core file.");
      return EFI_ABORTED;
    }

    //
    // Core PE/TE section, found, return
    //
    Status = EFI_SUCCESS;
    goto EarlyExit;
  }

  //
  // File was not found, look for FV Image file
  //

  // iterate through all FV image files in outer FV
  for (FvImageFileCount = 1;; FvImageFileCount++) {

    Status = GetFileByType(EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE, FvImageFileCount, &FvImageFile);

    if (EFI_ERROR(Status) || (FvImageFile == NULL) ) {
      // exit FV image file loop, no more found
      break;
    }

    // Found an fv image file, look for an FV image section.  The PI spec does not
    // preclude multiple FV image sections so we loop accordingly.
    for (EncapFvSectionCount = 1;; EncapFvSectionCount++) {

      // Look for the next FV image section.  The section search code will
      // iterate into encapsulation sections.  For example, it will iterate
      // into an EFI_SECTION_GUID_DEFINED encapsulation section to find the
      // EFI_SECTION_FIRMWARE_VOLUME_IMAGE sections contained therein.
      Status = GetSectionByType(FvImageFile, EFI_SECTION_FIRMWARE_VOLUME_IMAGE, EncapFvSectionCount, &EncapFvSection);

      if (EFI_ERROR(Status)) {
        // exit section inner loop, no more found
        break;
      }

      EncapsulatedFvHeader = (EFI_FIRMWARE_VOLUME_HEADER *)((UINT8 *)EncapFvSection.FVImageSection + GetSectionHeaderLength(EncapFvSection.FVImageSection));

      // recurse to search the encapsulated FV for this core file type
      Status = FindCorePeSection(EncapsulatedFvHeader, EncapsulatedFvHeader->FvLength, FileType, Pe32Section);

      if (!EFI_ERROR(Status)) {
        // we found the core in the capsulated image, success
        goto EarlyExit;
      }

    } // end encapsulated fv image section loop
  } // end fv image file loop

  // core was not found
  Status = EFI_NOT_FOUND;

EarlyExit:

  // restore FV lib values
  if(OrigFvHeader != NULL) {
    InitializeFvLib(OrigFvHeader, OrigFvLength);
  }

  return Status;
}

EFI_STATUS
GetCoreMachineType(
  IN  EFI_FILE_SECTION_POINTER     Pe32Section,
  OUT UINT16                      *CoreMachineType
  )
/*++

Routine Description:

  Returns the machine type of a P32 image, typically SEC or PEI core.

Arguments:

  Pe32Section       PE32 section data
  CoreMachineType   The extracted machine type

Returns:

  EFI_SUCCESS             Function Completed successfully.
  EFI_ABORTED             Error encountered.
  EFI_INVALID_PARAMETER   A required parameter was NULL.

--*/
{
  EFI_STATUS                  Status;
  UINT32                      EntryPoint;
  UINT32                      BaseOfCode;

  if (CoreMachineType == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = GetPe32Info(
    (VOID *)((UINTN)Pe32Section.Pe32Section + GetSectionHeaderLength(Pe32Section.CommonHeader)),
    &EntryPoint,
    &BaseOfCode,
    CoreMachineType
    );
  if (EFI_ERROR(Status)) {
    Error(NULL, 0, 3000, "Invalid", "could not get the PE32 machine type for the core.");
    return EFI_ABORTED;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
GetCoreEntryPointAddress(
  IN VOID                         *FvImageBuffer,
  IN FV_INFO                      *FvInfo,
  IN  EFI_FILE_SECTION_POINTER     Pe32Section,
  OUT EFI_PHYSICAL_ADDRESS        *CoreEntryAddress
)
/*++

Routine Description:

  Returns the physical address of the core (SEC or PEI) entry point.

Arguments:

  FvImageBuffer     Pointer to buffer containing FV data
  FvInfo            Info for the parent FV
  Pe32Section       PE32 section data
  CoreEntryAddress  The extracted core entry physical address

Returns:

  EFI_SUCCESS             Function Completed successfully.
  EFI_ABORTED             Error encountered.
  EFI_INVALID_PARAMETER   A required parameter was NULL.

--*/
{
  EFI_STATUS                  Status;
  UINT32                      EntryPoint;
  UINT32                      BaseOfCode;
  UINT16                      MachineType;
  EFI_PHYSICAL_ADDRESS        EntryPhysicalAddress;

  if (CoreEntryAddress == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = GetPe32Info(
    (VOID *)((UINTN)Pe32Section.Pe32Section + GetSectionHeaderLength(Pe32Section.CommonHeader)),
    &EntryPoint,
    &BaseOfCode,
    &MachineType
    );
  if (EFI_ERROR(Status)) {
    Error(NULL, 0, 3000, "Invalid", "could not get the PE32 entry point for the core.");
    return EFI_ABORTED;
  }

  //
  // Physical address is FV base + offset of PE32 + offset of the entry point
  //
  EntryPhysicalAddress = FvInfo->BaseAddress;
  EntryPhysicalAddress += (UINTN)Pe32Section.Pe32Section + GetSectionHeaderLength(Pe32Section.CommonHeader) - (UINTN)FvImageBuffer;
  EntryPhysicalAddress += EntryPoint;

  *CoreEntryAddress = EntryPhysicalAddress;

  return EFI_SUCCESS;
}

EFI_STATUS
UpdateArmResetVectorIfNeeded (
  IN MEMORY_FILE            *FvImage,
  IN FV_INFO                *FvInfo
  )
/*++

Routine Description:
  This parses the FV looking for SEC and patches that address into the
  beginning of the FV header.

  For ARM32 the reset vector is at 0x00000000 or 0xFFFF0000.
  For AArch64 the reset vector is at 0x00000000.

  This would commonly map to the first entry in the ROM.
  ARM32 Exceptions:
  Reset            +0
  Undefined        +4
  SWI              +8
  Prefetch Abort   +12
  Data Abort       +16
  IRQ              +20
  FIQ              +24

  We support two schemes on ARM.
  1) Beginning of the FV is the reset vector
  2) Reset vector is data bytes FDF file and that code branches to reset vector
    in the beginning of the FV (fixed size offset).

  Need to have the jump for the reset vector at location zero.
  We also need to store the address or PEI (if it exists).
  We stub out a return from interrupt in case the debugger
   is using SWI (not done for AArch64, not enough space in struct).
  The optional entry to the common exception handler is
   to support full featured exception handling from ROM and is currently
    not support by this tool.

Arguments:
  FvImage       Memory file for the FV memory image
  FvInfo        Information read from INF file.

Returns:

  EFI_SUCCESS             Function Completed successfully.
  EFI_ABORTED             Error encountered.
  EFI_INVALID_PARAMETER   A required parameter was NULL.
  EFI_NOT_FOUND           PEI Core file not found.

--*/
{
  EFI_STATUS                  Status;
  EFI_FILE_SECTION_POINTER    SecPe32;
  EFI_FILE_SECTION_POINTER    PeiPe32;
  BOOLEAN                     UpdateVectorSec = FALSE;
  BOOLEAN                     UpdateVectorPei = FALSE;
  UINT16                      MachineType = 0;
  EFI_PHYSICAL_ADDRESS        SecCoreEntryAddress = 0;
  UINT16                      PeiMachineType = 0;
  EFI_PHYSICAL_ADDRESS        PeiCoreEntryAddress = 0;

  //
  // Verify input parameters
  //
  if (FvImage == NULL || FvInfo == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Locate an SEC Core instance and if found extract the machine type and entry point address
  //
  Status = FindCorePeSection(FvImage->FileImage, FvInfo->Size, EFI_FV_FILETYPE_SECURITY_CORE, &SecPe32);
  if (!EFI_ERROR(Status)) {

    Status = GetCoreMachineType(SecPe32, &MachineType);
    if (EFI_ERROR(Status)) {
      Error(NULL, 0, 3000, "Invalid", "Could not get the PE32 machine type for SEC Core.");
      return EFI_ABORTED;
    }

    Status = GetCoreEntryPointAddress(FvImage->FileImage, FvInfo, SecPe32, &SecCoreEntryAddress);
    if (EFI_ERROR(Status)) {
      Error(NULL, 0, 3000, "Invalid", "Could not get the PE32 entry point address for SEC Core.");
      return EFI_ABORTED;
    }

    VerboseMsg("UpdateArmResetVectorIfNeeded found SEC core entry at 0x%llx", (unsigned long long)SecCoreEntryAddress);
    UpdateVectorSec = TRUE;
  }

  //
  // Locate a PEI Core instance and if found extract the machine type and entry point address
  //
  Status = FindCorePeSection(FvImage->FileImage, FvInfo->Size, EFI_FV_FILETYPE_PEI_CORE, &PeiPe32);
  if (!EFI_ERROR(Status)) {

    Status = GetCoreMachineType(PeiPe32, &PeiMachineType);
    if (EFI_ERROR(Status)) {
      Error(NULL, 0, 3000, "Invalid", "Could not get the PE32 machine type for PEI Core.");
      return EFI_ABORTED;
    }

    Status = GetCoreEntryPointAddress(FvImage->FileImage, FvInfo, PeiPe32, &PeiCoreEntryAddress);
    if (EFI_ERROR(Status)) {
      Error(NULL, 0, 3000, "Invalid", "Could not get the PE32 entry point address for PEI Core.");
      return EFI_ABORTED;
    }

    VerboseMsg("UpdateArmResetVectorIfNeeded found PEI core entry at 0x%llx", (unsigned long long)PeiCoreEntryAddress);

    // if we previously found an SEC Core make sure machine types match
    if (UpdateVectorSec && (MachineType != PeiMachineType)) {
      Error(NULL, 0, 3000, "Invalid", "SEC and PEI machine types do not match, can't update reset vector");
      return EFI_ABORTED;
    }
    else {
      MachineType = PeiMachineType;
    }

    UpdateVectorPei = TRUE;
  }

  if (!UpdateVectorSec && !UpdateVectorPei) {
    return EFI_SUCCESS;
  }

  if (MachineType == EFI_IMAGE_MACHINE_ARMT) {
    // ARM: Array of 4 UINT32s:
    // 0 - is branch relative to SEC entry point
    // 1 - PEI Entry Point
    // 2 - movs pc,lr for a SWI handler
    // 3 - Place holder for Common Exception Handler
    UINT32                      ResetVector[4];

    memset(ResetVector, 0, sizeof (ResetVector));

    // if we found an SEC core entry point then generate a branch instruction
    // to it and populate a debugger SWI entry as well
    if (UpdateVectorSec) {

      VerboseMsg("UpdateArmResetVectorIfNeeded updating ARM SEC vector");

      // B SecEntryPoint - signed_immed_24 part +/-32MB offset
      // on ARM, the PC is always 8 ahead, so we're not really jumping from the base address, but from base address + 8
      ResetVector[0] = (INT32)(SecCoreEntryAddress - FvInfo->BaseAddress - 8) >> 2;

      if (ResetVector[0] > 0x00FFFFFF) {
        Error(NULL, 0, 3000, "Invalid", "SEC Entry point must be within 32MB of the start of the FV");
        return EFI_ABORTED;
      }

      // Add opcode for an unconditional branch with no link. i.e.: " B SecEntryPoint"
      ResetVector[0] |= ARMT_UNCONDITIONAL_JUMP_INSTRUCTION;

      // SWI handler movs   pc,lr. Just in case a debugger uses SWI
      ResetVector[2] = 0xE1B0F07E;

      // Place holder to support a common interrupt handler from ROM.
      // Currently not supported. For this to be used the reset vector would not be in this FV
      // and the exception vectors would be hard coded in the ROM and just through this address
      // to find a common handler in the a module in the FV.
      ResetVector[3] = 0;
    }

    // if a PEI core entry was found place its address in the vector area
    if (UpdateVectorPei) {

      VerboseMsg("UpdateArmResetVectorIfNeeded updating ARM PEI address");

      // Address of PEI Core, if we have one
      ResetVector[1] = (UINT32)PeiCoreEntryAddress;
    }

    //
    // Copy to the beginning of the FV
    //
    memcpy(FvImage->FileImage, ResetVector, sizeof (ResetVector));

  } else if (MachineType == EFI_IMAGE_MACHINE_AARCH64) {
    // AArch64: Used as UINT64 ResetVector[2]
    // 0 - is branch relative to SEC entry point
    // 1 - PEI Entry Point
    UINT64                      ResetVector[2];

    memset(ResetVector, 0, sizeof (ResetVector));

    /* NOTE:
    ARMT above has an entry in ResetVector[2] for SWI. The way we are using the ResetVector
    array at the moment, for AArch64, does not allow us space for this as the header only
    allows for a fixed amount of bytes at the start. If we are sure that UEFI will live
    within the first 4GB of addressable RAM we could potentially adopt the same ResetVector
    layout as above. But for the moment we replace the four 32bit vectors with two 64bit
    vectors in the same area of the Image heasder. This allows UEFI to start from a 64bit
    base.
    */

    // if we found an SEC core entry point then generate a branch instruction to it
    if (UpdateVectorSec) {

      VerboseMsg("UpdateArmResetVectorIfNeeded updating AArch64 SEC vector");

      ResetVector[0] = (UINT64)(SecCoreEntryAddress - FvInfo->BaseAddress) >> 2;

      // B SecEntryPoint - signed_immed_26 part +/-128MB offset
      if (ResetVector[0] > 0x03FFFFFF) {
        Error(NULL, 0, 3000, "Invalid", "SEC Entry point must be within 128MB of the start of the FV");
        return EFI_ABORTED;
      }
      // Add opcode for an unconditional branch with no link. i.e.: " B SecEntryPoint"
      ResetVector[0] |= ARM64_UNCONDITIONAL_JUMP_INSTRUCTION;
    }

    // if a PEI core entry was found place its address in the vector area
    if (UpdateVectorPei) {

      VerboseMsg("UpdateArmResetVectorIfNeeded updating AArch64 PEI address");

      // Address of PEI Core, if we have one
      ResetVector[1] = (UINT64)PeiCoreEntryAddress;
    }

    //
    // Copy to the beginning of the FV
    //
    memcpy(FvImage->FileImage, ResetVector, sizeof (ResetVector));

  } else {
    Error(NULL, 0, 3000, "Invalid", "Unknown machine type");
    return EFI_ABORTED;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
GetPe32Info (
  IN UINT8                  *Pe32,
  OUT UINT32                *EntryPoint,
  OUT UINT32                *BaseOfCode,
  OUT UINT16                *MachineType
  )
/*++

Routine Description:

  Retrieves the PE32 entry point offset and machine type from PE image or TeImage.
  See EfiImage.h for machine types.  The entry point offset is from the beginning
  of the PE32 buffer passed in.

Arguments:

  Pe32          Beginning of the PE32.
  EntryPoint    Offset from the beginning of the PE32 to the image entry point.
  BaseOfCode    Base address of code.
  MachineType   Magic number for the machine type.

Returns:

  EFI_SUCCESS             Function completed successfully.
  EFI_ABORTED             Error encountered.
  EFI_INVALID_PARAMETER   A required parameter was NULL.
  EFI_UNSUPPORTED         The operation is unsupported.

--*/
{
  EFI_IMAGE_DOS_HEADER             *DosHeader;
  EFI_IMAGE_OPTIONAL_HEADER_UNION  *ImgHdr;
  EFI_TE_IMAGE_HEADER              *TeHeader;

  //
  // Verify input parameters
  //
  if (Pe32 == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // First check whether it is one TE Image.
  //
  TeHeader = (EFI_TE_IMAGE_HEADER *) Pe32;
  if (TeHeader->Signature == EFI_TE_IMAGE_HEADER_SIGNATURE) {
    //
    // By TeImage Header to get output
    //
    *EntryPoint   = TeHeader->AddressOfEntryPoint + sizeof (EFI_TE_IMAGE_HEADER) - TeHeader->StrippedSize;
    *BaseOfCode   = TeHeader->BaseOfCode + sizeof (EFI_TE_IMAGE_HEADER) - TeHeader->StrippedSize;
    *MachineType  = TeHeader->Machine;
  } else {

    //
    // Then check whether
    // First is the DOS header
    //
    DosHeader = (EFI_IMAGE_DOS_HEADER *) Pe32;

    //
    // Verify DOS header is expected
    //
    if (DosHeader->e_magic != EFI_IMAGE_DOS_SIGNATURE) {
      Error (NULL, 0, 3000, "Invalid", "Unknown magic number in the DOS header, 0x%04X.", DosHeader->e_magic);
      return EFI_UNSUPPORTED;
    }
    //
    // Immediately following is the NT header.
    //
    ImgHdr = (EFI_IMAGE_OPTIONAL_HEADER_UNION *) ((UINTN) Pe32 + DosHeader->e_lfanew);

    //
    // Verify NT header is expected
    //
    if (ImgHdr->Pe32.Signature != EFI_IMAGE_NT_SIGNATURE) {
      Error (NULL, 0, 3000, "Invalid", "Unrecognized image signature 0x%08X.", (unsigned) ImgHdr->Pe32.Signature);
      return EFI_UNSUPPORTED;
    }
    //
    // Get output
    //
    *EntryPoint   = ImgHdr->Pe32.OptionalHeader.AddressOfEntryPoint;
    *BaseOfCode   = ImgHdr->Pe32.OptionalHeader.BaseOfCode;
    *MachineType  = ImgHdr->Pe32.FileHeader.Machine;
  }

  //
  // Verify machine type is supported
  //
  if ((*MachineType != EFI_IMAGE_MACHINE_IA32) &&  (*MachineType != EFI_IMAGE_MACHINE_X64) && (*MachineType != EFI_IMAGE_MACHINE_EBC) &&
      (*MachineType != EFI_IMAGE_MACHINE_ARMT) && (*MachineType != EFI_IMAGE_MACHINE_AARCH64)) {
    Error (NULL, 0, 3000, "Invalid", "Unrecognized machine type in the PE32 file.");
    return EFI_UNSUPPORTED;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
GenerateFvImage (
  IN CHAR8                *InfFileImage,
  IN UINTN                InfFileSize,
  IN CHAR8                *FvFileName,
  IN CHAR8                *MapFileName
  )
/*++

Routine Description:

  This is the main function which will be called from application.

Arguments:

  InfFileImage   Buffer containing the INF file contents.
  InfFileSize    Size of the contents of the InfFileImage buffer.
  FvFileName     Requested name for the FV file.
  MapFileName    Fv map file to log fv driver information.

Returns:

  EFI_SUCCESS             Function completed successfully.
  EFI_OUT_OF_RESOURCES    Could not allocate required resources.
  EFI_ABORTED             Error encountered.
  EFI_INVALID_PARAMETER   A required parameter was NULL.

--*/
{
  EFI_STATUS                      Status;
  MEMORY_FILE                     InfMemoryFile;
  MEMORY_FILE                     FvImageMemoryFile;
  UINTN                           Index;
  EFI_FIRMWARE_VOLUME_HEADER      *FvHeader;
  EFI_FFS_FILE_HEADER             *VtfFileImage;
  UINT8                           *FvBufferHeader; // to make sure fvimage header 8 type alignment.
  UINT8                           *FvImage;
  UINTN                           FvImageSize;
  FILE                            *FvFile;
  CHAR8                           *FvMapName;
  FILE                            *FvMapFile;
  EFI_FIRMWARE_VOLUME_EXT_HEADER  *FvExtHeader;
  FILE                            *FvExtHeaderFile;
  UINTN                           FileSize;
  CHAR8                           *FvReportName;
  FILE                            *FvReportFile;

  FvBufferHeader = NULL;
  FvFile         = NULL;
  FvMapName      = NULL;
  FvMapFile      = NULL;
  FvReportName   = NULL;
  FvReportFile   = NULL;

  if (InfFileImage != NULL) {
    //
    // Initialize file structures
    //
    InfMemoryFile.FileImage           = InfFileImage;
    InfMemoryFile.CurrentFilePointer  = InfFileImage;
    InfMemoryFile.Eof                 = InfFileImage + InfFileSize;

    //
    // Parse the FV inf file for header information
    //
    Status = ParseFvInf (&InfMemoryFile, &mFvDataInfo);
    if (EFI_ERROR (Status)) {
      Error (NULL, 0, 0003, "Error parsing file", "the input FV INF file.");
      return Status;
    }
  }

  //
  // Update the file name return values
  //
  if (FvFileName == NULL && mFvDataInfo.FvName[0] != '\0') {
    FvFileName = mFvDataInfo.FvName;
  }

  if (FvFileName == NULL) {
    Error (NULL, 0, 1001, "Missing option", "Output file name");
    return EFI_ABORTED;
  }

  if (mFvDataInfo.FvBlocks[0].Length == 0) {
    Error (NULL, 0, 1001, "Missing required argument", "Block Size");
    return EFI_ABORTED;
  }

  //
  // Debug message Fv File System Guid
  //
  if (mFvDataInfo.FvFileSystemGuidSet) {
    DebugMsg (NULL, 0, 9, "FV File System Guid", "%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
                  (unsigned) mFvDataInfo.FvFileSystemGuid.Data1,
                  mFvDataInfo.FvFileSystemGuid.Data2,
                  mFvDataInfo.FvFileSystemGuid.Data3,
                  mFvDataInfo.FvFileSystemGuid.Data4[0],
                  mFvDataInfo.FvFileSystemGuid.Data4[1],
                  mFvDataInfo.FvFileSystemGuid.Data4[2],
                  mFvDataInfo.FvFileSystemGuid.Data4[3],
                  mFvDataInfo.FvFileSystemGuid.Data4[4],
                  mFvDataInfo.FvFileSystemGuid.Data4[5],
                  mFvDataInfo.FvFileSystemGuid.Data4[6],
                  mFvDataInfo.FvFileSystemGuid.Data4[7]);
  }

  //
  // Add PI FV extension header
  //
  FvExtHeader = NULL;
  FvExtHeaderFile = NULL;
  if (mFvDataInfo.FvExtHeaderFile[0] != 0) {
    //
    // Open the FV Extension Header file
    //
    FvExtHeaderFile = fopen (LongFilePath (mFvDataInfo.FvExtHeaderFile), "rb");
    if (FvExtHeaderFile == NULL) {
      Error (NULL, 0, 0001, "Error opening file", mFvDataInfo.FvExtHeaderFile);
      return EFI_ABORTED;
    }

    //
    // Get the file size
    //
    FileSize = _filelength (fileno (FvExtHeaderFile));

    //
    // Allocate a buffer for the FV Extension Header
    //
    FvExtHeader = malloc(FileSize);
    if (FvExtHeader == NULL) {
      fclose (FvExtHeaderFile);
      return EFI_OUT_OF_RESOURCES;
    }

    //
    // Read the FV Extension Header
    //
    fread (FvExtHeader, sizeof (UINT8), FileSize, FvExtHeaderFile);
    fclose (FvExtHeaderFile);

    //
    // See if there is an override for the FV Name GUID
    //
    if (mFvDataInfo.FvNameGuidSet) {
      memcpy (&FvExtHeader->FvName, &mFvDataInfo.FvNameGuid, sizeof (EFI_GUID));
    }
    memcpy (&mFvDataInfo.FvNameGuid, &FvExtHeader->FvName, sizeof (EFI_GUID));
    mFvDataInfo.FvNameGuidSet = TRUE;
  } else if (mFvDataInfo.FvNameGuidSet) {
    //
    // Allocate a buffer for the FV Extension Header
    //
    FvExtHeader = malloc(sizeof (EFI_FIRMWARE_VOLUME_EXT_HEADER));
    if (FvExtHeader == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    memcpy (&FvExtHeader->FvName, &mFvDataInfo.FvNameGuid, sizeof (EFI_GUID));
    FvExtHeader->ExtHeaderSize = sizeof (EFI_FIRMWARE_VOLUME_EXT_HEADER);
  }

  //
  // Debug message Fv Name Guid
  //
  if (mFvDataInfo.FvNameGuidSet) {
      DebugMsg (NULL, 0, 9, "FV Name Guid", "%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
                  (unsigned) mFvDataInfo.FvNameGuid.Data1,
                  mFvDataInfo.FvNameGuid.Data2,
                  mFvDataInfo.FvNameGuid.Data3,
                  mFvDataInfo.FvNameGuid.Data4[0],
                  mFvDataInfo.FvNameGuid.Data4[1],
                  mFvDataInfo.FvNameGuid.Data4[2],
                  mFvDataInfo.FvNameGuid.Data4[3],
                  mFvDataInfo.FvNameGuid.Data4[4],
                  mFvDataInfo.FvNameGuid.Data4[5],
                  mFvDataInfo.FvNameGuid.Data4[6],
                  mFvDataInfo.FvNameGuid.Data4[7]);
  }

  if (CompareGuid (&mFvDataInfo.FvFileSystemGuid, &mEfiFirmwareFileSystem2Guid) == 0 ||
    CompareGuid (&mFvDataInfo.FvFileSystemGuid, &mEfiFirmwareFileSystem3Guid) == 0) {
    mFvDataInfo.IsPiFvImage = TRUE;
  }

  //
  // FvMap file to log the function address of all modules in one Fvimage
  //
  if (MapFileName != NULL) {
    if (strlen (MapFileName) > MAX_LONG_FILE_PATH - 1) {
      Error (NULL, 0, 1003, "Invalid option value", "MapFileName %s is too long!", MapFileName);
      Status = EFI_ABORTED;
      goto Finish;
    }

    FvMapName = malloc (strlen (MapFileName) + 1);
    if (FvMapName == NULL) {
      Error (NULL, 0, 4001, "Resource", "memory cannot be allocated!");
      Status = EFI_OUT_OF_RESOURCES;
      goto Finish;
    }

    strcpy (FvMapName, MapFileName);
  } else {
    if (strlen (FvFileName) + strlen (".map") > MAX_LONG_FILE_PATH - 1) {
      Error (NULL, 0, 1003, "Invalid option value", "FvFileName %s is too long!", FvFileName);
      Status = EFI_ABORTED;
      goto Finish;
    }

    FvMapName = malloc (strlen (FvFileName) + strlen (".map") + 1);
    if (FvMapName == NULL) {
      Error (NULL, 0, 4001, "Resource", "memory cannot be allocated!");
      Status = EFI_OUT_OF_RESOURCES;
      goto Finish;
    }

    strcpy (FvMapName, FvFileName);
    strcat (FvMapName, ".map");
  }
  VerboseMsg ("FV Map file name is %s", FvMapName);

  //
  // FvReport file to log the FV information in one Fvimage
  //
  if (strlen (FvFileName) + strlen (".txt") > MAX_LONG_FILE_PATH - 1) {
    Error (NULL, 0, 1003, "Invalid option value", "FvFileName %s is too long!", FvFileName);
    Status = EFI_ABORTED;
    goto Finish;
  }

  FvReportName = malloc (strlen (FvFileName) + strlen (".txt") + 1);
  if (FvReportName == NULL) {
    Error (NULL, 0, 4001, "Resource", "memory cannot be allocated!");
    Status = EFI_OUT_OF_RESOURCES;
    goto Finish;
  }

  strcpy (FvReportName, FvFileName);
  strcat (FvReportName, ".txt");

  //
  // Calculate the FV size and Update Fv Size based on the actual FFS files.
  // And Update mFvDataInfo data.
  //
  Status = CalculateFvSize (&mFvDataInfo);
  if (EFI_ERROR (Status)) {
    goto Finish;
  }
  VerboseMsg ("the generated FV image size is %u bytes", (unsigned) mFvDataInfo.Size);

  //
  // support fv image and empty fv image
  //
  FvImageSize = mFvDataInfo.Size;

  //
  // Allocate the FV, assure FvImage Header 8 byte alignment
  //
  FvBufferHeader = malloc (FvImageSize + sizeof (UINT64));
  if (FvBufferHeader == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Finish;
  }
  FvImage = (UINT8 *) (((UINTN) FvBufferHeader + 7) & ~7);

  //
  // Initialize the FV to the erase polarity
  //
  if (mFvDataInfo.FvAttributes == 0) {
    //
    // Set Default Fv Attribute
    //
    mFvDataInfo.FvAttributes = FV_DEFAULT_ATTRIBUTE;
  }
  if (mFvDataInfo.FvAttributes & EFI_FVB2_ERASE_POLARITY) {
    memset (FvImage, -1, FvImageSize);
  } else {
    memset (FvImage, 0, FvImageSize);
  }

  //
  // Initialize FV header
  //
  FvHeader = (EFI_FIRMWARE_VOLUME_HEADER *) FvImage;

  //
  // Initialize the zero vector to all zeros.
  //
  memset (FvHeader->ZeroVector, 0, 16);

  //
  // Copy the Fv file system GUID
  //
  memcpy (&FvHeader->FileSystemGuid, &mFvDataInfo.FvFileSystemGuid, sizeof (EFI_GUID));

  FvHeader->FvLength        = FvImageSize;
  FvHeader->Signature       = EFI_FVH_SIGNATURE;
  FvHeader->Attributes      = mFvDataInfo.FvAttributes;
  FvHeader->Revision        = EFI_FVH_REVISION;
  FvHeader->ExtHeaderOffset = 0;
  FvHeader->Reserved[0]     = 0;

  //
  // Copy firmware block map
  //
  for (Index = 0; mFvDataInfo.FvBlocks[Index].Length != 0; Index++) {
    FvHeader->BlockMap[Index].NumBlocks   = mFvDataInfo.FvBlocks[Index].NumBlocks;
    FvHeader->BlockMap[Index].Length      = mFvDataInfo.FvBlocks[Index].Length;
  }

  //
  // Add block map terminator
  //
  FvHeader->BlockMap[Index].NumBlocks   = 0;
  FvHeader->BlockMap[Index].Length      = 0;

  //
  // Complete the header
  //
  FvHeader->HeaderLength  = (UINT16) (((UINTN) &(FvHeader->BlockMap[Index + 1])) - (UINTN) FvImage);
  FvHeader->Checksum      = 0;
  FvHeader->Checksum      = CalculateChecksum16 ((UINT16 *) FvHeader, FvHeader->HeaderLength / sizeof (UINT16));

  //
  // If there is no FFS file, generate one empty FV
  //
  if (mFvDataInfo.FvFiles[0][0] == 0 && !mFvDataInfo.FvNameGuidSet) {
    goto WriteFile;
  }

  //
  // Initialize our "file" view of the buffer
  //
  FvImageMemoryFile.FileImage           = (CHAR8 *)FvImage;
  FvImageMemoryFile.CurrentFilePointer  = (CHAR8 *)FvImage + FvHeader->HeaderLength;
  FvImageMemoryFile.Eof                 = (CHAR8 *)FvImage + FvImageSize;

  //
  // Initialize the FV library.
  //
  InitializeFvLib (FvImageMemoryFile.FileImage, FvImageSize);

  //
  // Initialize the VTF file address.
  //
  VtfFileImage = (EFI_FFS_FILE_HEADER *) FvImageMemoryFile.Eof;

  //
  // Open FvMap file
  //
  FvMapFile = fopen (LongFilePath (FvMapName), "w");
  if (FvMapFile == NULL) {
    Error (NULL, 0, 0001, "Error opening file", FvMapName);
    Status = EFI_ABORTED;
    goto Finish;
  }

  //
  // Open FvReport file
  //
  FvReportFile = fopen (LongFilePath (FvReportName), "w");
  if (FvReportFile == NULL) {
    Error (NULL, 0, 0001, "Error opening file", FvReportName);
    Status = EFI_ABORTED;
    goto Finish;
  }
  //
  // record FV size information into FvMap file.
  //
  if (mFvTotalSize != 0) {
    fprintf (FvMapFile, EFI_FV_TOTAL_SIZE_STRING);
    fprintf (FvMapFile, " = 0x%x\n", (unsigned) mFvTotalSize);
  }
  if (mFvTakenSize != 0) {
    fprintf (FvMapFile, EFI_FV_TAKEN_SIZE_STRING);
    fprintf (FvMapFile, " = 0x%x\n", (unsigned) mFvTakenSize);
  }
  if (mFvTotalSize != 0 && mFvTakenSize != 0) {
    fprintf (FvMapFile, EFI_FV_SPACE_SIZE_STRING);
    fprintf (FvMapFile, " = 0x%x\n\n", (unsigned) (mFvTotalSize - mFvTakenSize));
  }

  //
  // record FV size information to FvReportFile.
  //
  fprintf (FvReportFile, "%s = 0x%x\n", EFI_FV_TOTAL_SIZE_STRING, (unsigned) mFvTotalSize);
  fprintf (FvReportFile, "%s = 0x%x\n", EFI_FV_TAKEN_SIZE_STRING, (unsigned) mFvTakenSize);

  //
  // Add PI FV extension header
  //
  if (FvExtHeader != NULL) {
    //
    // Add FV Extended Header contents to the FV as a PAD file
    //
    AddPadFile (&FvImageMemoryFile, 4, VtfFileImage, FvExtHeader, 0);

    //
    // Fv Extension header change update Fv Header Check sum
    //
    FvHeader->Checksum      = 0;
    FvHeader->Checksum      = CalculateChecksum16 ((UINT16 *) FvHeader, FvHeader->HeaderLength / sizeof (UINT16));
  }

  //
  // Add files to FV
  //
  for (Index = 0; mFvDataInfo.FvFiles[Index][0] != 0; Index++) {
    //
    // Add the file
    //
    Status = AddFile (&FvImageMemoryFile, &mFvDataInfo, Index, &VtfFileImage, FvMapFile, FvReportFile);

    //
    // Exit if error detected while adding the file
    //
    if (EFI_ERROR (Status)) {
      goto Finish;
    }
  }

  //
  // If there is a VTF file, some special actions need to occur.
  //
  if ((UINTN) VtfFileImage != (UINTN) FvImageMemoryFile.Eof) {
    //
    // Pad from the end of the last file to the beginning of the VTF file.
    // If the left space is less than sizeof (EFI_FFS_FILE_HEADER)?
    //
    Status = PadFvImage (&FvImageMemoryFile, VtfFileImage);
    if (EFI_ERROR (Status)) {
      Error (NULL, 0, 4002, "Resource", "FV space is full, cannot add pad file between the last file and the VTF file.");
      goto Finish;
    }
    if (!mArm) {
      //
      // Update reset vector (SALE_ENTRY for IPF)
      // Now for IA32 and IA64 platform, the fv which has bsf file must have the
      // EndAddress of 0xFFFFFFFF (unless the section was rebased).
      // Thus, only this type fv needs to update the  reset vector.
      // If the PEI Core is found, the VTF file will probably get
      // corrupted by updating the entry point.
      //
      if (mFvDataInfo.ForceRebase == 1 ||
          (mFvDataInfo.BaseAddress + mFvDataInfo.Size) == FV_IMAGES_TOP_ADDRESS) {
        Status = UpdateResetVector (&FvImageMemoryFile, &mFvDataInfo, VtfFileImage);
        if (EFI_ERROR(Status)) {
          Error (NULL, 0, 3000, "Invalid", "Could not update the reset vector.");
          goto Finish;
        }
        DebugMsg (NULL, 0, 9, "Update Reset vector in VTF file", NULL);
      }
    }
  }

  if (mArm) {
    Status = UpdateArmResetVectorIfNeeded (&FvImageMemoryFile, &mFvDataInfo);
    if (EFI_ERROR (Status)) {
      Error (NULL, 0, 3000, "Invalid", "Could not update the reset vector.");
      goto Finish;
    }

    //
    // Update Checksum for FvHeader
    //
    FvHeader->Checksum = 0;
    FvHeader->Checksum = CalculateChecksum16 ((UINT16 *) FvHeader, FvHeader->HeaderLength / sizeof (UINT16));
  }

  //
  // Update FV Alignment attribute to the largest alignment of all the FFS files in the FV
  //
  if (((FvHeader->Attributes & EFI_FVB2_WEAK_ALIGNMENT) != EFI_FVB2_WEAK_ALIGNMENT) &&
      (((FvHeader->Attributes & EFI_FVB2_ALIGNMENT) >> 16)) < MaxFfsAlignment) {
    FvHeader->Attributes = ((MaxFfsAlignment << 16) | (FvHeader->Attributes & 0xFFFF));
    //
    // Update Checksum for FvHeader
    //
    FvHeader->Checksum      = 0;
    FvHeader->Checksum      = CalculateChecksum16 ((UINT16 *) FvHeader, FvHeader->HeaderLength / sizeof (UINT16));
  }

  //
  // If there are large FFS in FV, the file system GUID should set to system 3 GUID.
  //
  if (mIsLargeFfs && CompareGuid (&FvHeader->FileSystemGuid, &mEfiFirmwareFileSystem2Guid) == 0) {
    memcpy (&FvHeader->FileSystemGuid, &mEfiFirmwareFileSystem3Guid, sizeof (EFI_GUID));
    FvHeader->Checksum      = 0;
    FvHeader->Checksum      = CalculateChecksum16 ((UINT16 *) FvHeader, FvHeader->HeaderLength / sizeof (UINT16));
  }

WriteFile:
  //
  // Write fv file
  //
  FvFile = fopen (LongFilePath (FvFileName), "wb");
  if (FvFile == NULL) {
    Error (NULL, 0, 0001, "Error opening file", FvFileName);
    Status = EFI_ABORTED;
    goto Finish;
  }

  if (fwrite (FvImage, 1, FvImageSize, FvFile) != FvImageSize) {
    Error (NULL, 0, 0002, "Error writing file", FvFileName);
    Status = EFI_ABORTED;
    goto Finish;
  }

Finish:
  if (FvBufferHeader != NULL) {
    free (FvBufferHeader);
  }

  if (FvExtHeader != NULL) {
    free (FvExtHeader);
  }

  if (FvMapName != NULL) {
    free (FvMapName);
  }

  if (FvReportName != NULL) {
    free (FvReportName);
  }

  if (FvFile != NULL) {
    fflush (FvFile);
    fclose (FvFile);
  }

  if (FvMapFile != NULL) {
    fflush (FvMapFile);
    fclose (FvMapFile);
  }

  if (FvReportFile != NULL) {
    fflush (FvReportFile);
    fclose (FvReportFile);
  }
  return Status;
}

EFI_STATUS
UpdatePeiCoreEntryInFit (
  IN FIT_TABLE     *FitTablePtr,
  IN UINT64        PeiCorePhysicalAddress
  )
/*++

Routine Description:

  This function is used to update the Pei Core address in FIT, this can be used by Sec core to pass control from
  Sec to Pei Core

Arguments:

  FitTablePtr             - The pointer of FIT_TABLE.
  PeiCorePhysicalAddress  - The address of Pei Core entry.

Returns:

  EFI_SUCCESS             - The PEI_CORE FIT entry was updated successfully.
  EFI_NOT_FOUND           - Not found the PEI_CORE FIT entry.

--*/
{
  FIT_TABLE *TmpFitPtr;
  UINTN     Index;
  UINTN     NumFitComponents;

  TmpFitPtr         = FitTablePtr;
  NumFitComponents  = TmpFitPtr->CompSize;

  for (Index = 0; Index < NumFitComponents; Index++) {
    if ((TmpFitPtr->CvAndType & FIT_TYPE_MASK) == COMP_TYPE_FIT_PEICORE) {
      TmpFitPtr->CompAddress = PeiCorePhysicalAddress;
      return EFI_SUCCESS;
    }

    TmpFitPtr++;
  }

  return EFI_NOT_FOUND;
}

VOID
UpdateFitCheckSum (
  IN FIT_TABLE   *FitTablePtr
  )
/*++

Routine Description:

  This function is used to update the checksum for FIT.


Arguments:

  FitTablePtr             - The pointer of FIT_TABLE.

Returns:

  None.

--*/
{
  if ((FitTablePtr->CvAndType & CHECKSUM_BIT_MASK) >> 7) {
    FitTablePtr->CheckSum = 0;
    FitTablePtr->CheckSum = CalculateChecksum8 ((UINT8 *) FitTablePtr, FitTablePtr->CompSize * 16);
  }
}

EFI_STATUS
CalculateFvSize (
  FV_INFO *FvInfoPtr
  )
/*++
Routine Description:
  Calculate the FV size and Update Fv Size based on the actual FFS files.
  And Update FvInfo data.

Arguments:
  FvInfoPtr     - The pointer to FV_INFO structure.

Returns:
  EFI_ABORTED   - Ffs Image Error
  EFI_SUCCESS   - Successfully update FvSize
--*/
{
  UINTN               CurrentOffset;
  UINTN               Index;
  FILE                *fpin;
  UINTN               FfsFileSize;
  UINTN               FvExtendHeaderSize;
  UINT32              FfsAlignment;
  UINT32              FfsHeaderSize;
  EFI_FFS_FILE_HEADER FfsHeader;
  UINTN               VtfFileSize;

  FvExtendHeaderSize = 0;
  VtfFileSize = 0;
  fpin  = NULL;
  Index = 0;

  //
  // Compute size for easy access later
  //
  FvInfoPtr->Size = 0;
  for (Index = 0; FvInfoPtr->FvBlocks[Index].NumBlocks > 0 && FvInfoPtr->FvBlocks[Index].Length > 0; Index++) {
    FvInfoPtr->Size += FvInfoPtr->FvBlocks[Index].NumBlocks * FvInfoPtr->FvBlocks[Index].Length;
  }

  //
  // Calculate the required sizes for all FFS files.
  //
  CurrentOffset = sizeof (EFI_FIRMWARE_VOLUME_HEADER);

  for (Index = 1;; Index ++) {
    CurrentOffset += sizeof (EFI_FV_BLOCK_MAP_ENTRY);
    if (FvInfoPtr->FvBlocks[Index].NumBlocks == 0 || FvInfoPtr->FvBlocks[Index].Length == 0) {
      break;
    }
  }

  //
  // Calculate PI extension header
  //
  if (mFvDataInfo.FvExtHeaderFile[0] != '\0') {
    fpin = fopen (LongFilePath (mFvDataInfo.FvExtHeaderFile), "rb");
    if (fpin == NULL) {
      Error (NULL, 0, 0001, "Error opening file", mFvDataInfo.FvExtHeaderFile);
      return EFI_ABORTED;
    }
    FvExtendHeaderSize = _filelength (fileno (fpin));
    fclose (fpin);
    if (sizeof (EFI_FFS_FILE_HEADER) + FvExtendHeaderSize >= MAX_FFS_SIZE) {
      CurrentOffset += sizeof (EFI_FFS_FILE_HEADER2) + FvExtendHeaderSize;
      mIsLargeFfs = TRUE;
    } else {
      CurrentOffset += sizeof (EFI_FFS_FILE_HEADER) + FvExtendHeaderSize;
    }
    CurrentOffset = (CurrentOffset + 7) & (~7);
  } else if (mFvDataInfo.FvNameGuidSet) {
    CurrentOffset += sizeof (EFI_FFS_FILE_HEADER) + sizeof (EFI_FIRMWARE_VOLUME_EXT_HEADER);
    CurrentOffset = (CurrentOffset + 7) & (~7);
  }

  //
  // Accumulate every FFS file size.
  //
  for (Index = 0; FvInfoPtr->FvFiles[Index][0] != 0; Index++) {
    //
    // Open FFS file
    //
    fpin = NULL;
    fpin = fopen (LongFilePath (FvInfoPtr->FvFiles[Index]), "rb");
    if (fpin == NULL) {
      Error (NULL, 0, 0001, "Error opening file", FvInfoPtr->FvFiles[Index]);
      return EFI_ABORTED;
    }
    //
    // Get the file size
    //
    FfsFileSize = _filelength (fileno (fpin));
    if (FfsFileSize >= MAX_FFS_SIZE) {
      FfsHeaderSize = sizeof(EFI_FFS_FILE_HEADER2);
      mIsLargeFfs = TRUE;
    } else {
      FfsHeaderSize = sizeof(EFI_FFS_FILE_HEADER);
    }
    //
    // Read Ffs File header
    //
    fread (&FfsHeader, sizeof (UINT8), sizeof (EFI_FFS_FILE_HEADER), fpin);
    //
    // close file
    //
    fclose (fpin);

    if (FvInfoPtr->IsPiFvImage) {
        //
        // Check whether this ffs file is vtf file
        //
        if (IsVtfFile (&FfsHeader)) {
          if (VtfFileFlag) {
            //
            // One Fv image can't have two vtf files.
            //
            Error (NULL, 0, 3000,"Invalid", "One Fv image can't have two vtf files.");
            return EFI_ABORTED;
          }
          VtfFileFlag = TRUE;
        VtfFileSize = FfsFileSize;
        continue;
      }

      //
      // Get the alignment of FFS file
      //
      ReadFfsAlignment (&FfsHeader, &FfsAlignment);
      FfsAlignment = 1 << FfsAlignment;
      //
      // Add Pad file
      //
      if (((CurrentOffset + FfsHeaderSize) % FfsAlignment) != 0) {
        //
        // Only EFI_FFS_FILE_HEADER is needed for a pad section.
        //
        CurrentOffset = (CurrentOffset + FfsHeaderSize + sizeof(EFI_FFS_FILE_HEADER) + FfsAlignment - 1) & ~(FfsAlignment - 1);
        CurrentOffset -= FfsHeaderSize;
      }
    }

    //
    // Add ffs file size
    //
    if (FvInfoPtr->SizeofFvFiles[Index] > FfsFileSize) {
      CurrentOffset += FvInfoPtr->SizeofFvFiles[Index];
    } else {
      CurrentOffset += FfsFileSize;
    }

    //
    // Make next ffs file start at QWord Boundary
    //
    if (FvInfoPtr->IsPiFvImage) {
      CurrentOffset = (CurrentOffset + EFI_FFS_FILE_HEADER_ALIGNMENT - 1) & ~(EFI_FFS_FILE_HEADER_ALIGNMENT - 1);
    }
  }
  CurrentOffset += VtfFileSize;
  DebugMsg (NULL, 0, 9, "FvImage size", "The calculated fv image size is 0x%x and the current set fv image size is 0x%x", (unsigned) CurrentOffset, (unsigned) FvInfoPtr->Size);

  if (FvInfoPtr->Size == 0) {
    //
    // Update FvInfo data
    //
    FvInfoPtr->FvBlocks[0].NumBlocks = CurrentOffset / FvInfoPtr->FvBlocks[0].Length + ((CurrentOffset % FvInfoPtr->FvBlocks[0].Length)?1:0);
    FvInfoPtr->Size = FvInfoPtr->FvBlocks[0].NumBlocks * FvInfoPtr->FvBlocks[0].Length;
    FvInfoPtr->FvBlocks[1].NumBlocks = 0;
    FvInfoPtr->FvBlocks[1].Length = 0;
  } else if (FvInfoPtr->Size < CurrentOffset) {
    //
    // Not invalid
    //
    Error (NULL, 0, 3000, "Invalid", "the required fv image size 0x%x exceeds the set fv image size 0x%x", (unsigned) CurrentOffset, (unsigned) FvInfoPtr->Size);
    return EFI_INVALID_PARAMETER;
  }

  //
  // Set Fv Size Information
  //
  mFvTotalSize = FvInfoPtr->Size;
  mFvTakenSize = CurrentOffset;

  return EFI_SUCCESS;
}

EFI_STATUS
FfsRebaseImageRead (
  IN     VOID    *FileHandle,
  IN     UINTN   FileOffset,
  IN OUT UINT32  *ReadSize,
  OUT    VOID    *Buffer
  )
/*++

Routine Description:

  Support routine for the PE/COFF Loader that reads a buffer from a PE/COFF file

Arguments:

  FileHandle - The handle to the PE/COFF file

  FileOffset - The offset, in bytes, into the file to read

  ReadSize   - The number of bytes to read from the file starting at FileOffset

  Buffer     - A pointer to the buffer to read the data into.

Returns:

  EFI_SUCCESS - ReadSize bytes of data were read into Buffer from the PE/COFF file starting at FileOffset

--*/
{
  CHAR8   *Destination8;
  CHAR8   *Source8;
  UINT32  Length;

  Destination8  = Buffer;
  Source8       = (CHAR8 *) ((UINTN) FileHandle + FileOffset);
  Length        = *ReadSize;
  while (Length--) {
    *(Destination8++) = *(Source8++);
  }

  return EFI_SUCCESS;
}

EFI_STATUS
GetChildFvFromFfs (
  IN      FV_INFO               *FvInfo,
  IN      EFI_FFS_FILE_HEADER   *FfsFile,
  IN      UINTN                 XipOffset
  )
/*++

Routine Description:

  This function gets all child FvImages in the input FfsFile, and records
  their base address to the parent image.

Arguments:
  FvInfo            A pointer to FV_INFO structure.
  FfsFile           A pointer to Ffs file image that may contain FvImage.
  XipOffset         The offset address to the parent FvImage base.

Returns:

  EFI_SUCCESS        Base address of child Fv image is recorded.
--*/
{
  EFI_STATUS                          Status;
  UINTN                               Index;
  EFI_FILE_SECTION_POINTER            SubFvSection;
  EFI_FIRMWARE_VOLUME_HEADER          *SubFvImageHeader;
  EFI_PHYSICAL_ADDRESS                SubFvBaseAddress;
  EFI_FILE_SECTION_POINTER            CorePe32;
  UINT16                              MachineType;

  for (Index = 1;; Index++) {
    //
    // Find FV section
    //
    Status = GetSectionByType (FfsFile, EFI_SECTION_FIRMWARE_VOLUME_IMAGE, Index, &SubFvSection);
    if (EFI_ERROR (Status)) {
      break;
    }
    SubFvImageHeader = (EFI_FIRMWARE_VOLUME_HEADER *) ((UINT8 *) SubFvSection.FVImageSection + GetSectionHeaderLength(SubFvSection.FVImageSection));

    //
    // See if there's an SEC core in the child FV
    Status = FindCorePeSection(SubFvImageHeader, SubFvImageHeader->FvLength, EFI_FV_FILETYPE_SECURITY_CORE, &CorePe32);

    // if we couldn't find the SEC core, look for a PEI core
    if (EFI_ERROR(Status)) {
      Status = FindCorePeSection(SubFvImageHeader, SubFvImageHeader->FvLength, EFI_FV_FILETYPE_PEI_CORE, &CorePe32);
    }

    if (!EFI_ERROR(Status)) {
      Status = GetCoreMachineType(CorePe32, &MachineType);
      if (EFI_ERROR(Status)) {
        Error(NULL, 0, 3000, "Invalid", "Could not get the PE32 machine type for SEC/PEI Core.");
        return EFI_ABORTED;
      }

      // machine type is ARM, set a flag so ARM reset vector processing occurs
      if ((MachineType == EFI_IMAGE_MACHINE_ARMT) || (MachineType == EFI_IMAGE_MACHINE_AARCH64)) {
        VerboseMsg("Located ARM/AArch64 SEC/PEI core in child FV");
        mArm = TRUE;
      }
    }

    //
    // Rebase on Flash
    //
    SubFvBaseAddress = FvInfo->BaseAddress + (UINTN) SubFvImageHeader - (UINTN) FfsFile + XipOffset;
    mFvBaseAddress[mFvBaseAddressNumber ++ ] = SubFvBaseAddress;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
FfsRebase (
  IN OUT  FV_INFO               *FvInfo,
  IN      CHAR8                 *FileName,
  IN OUT  EFI_FFS_FILE_HEADER   *FfsFile,
  IN      UINTN                 XipOffset,
  IN      FILE                  *FvMapFile
  )
/*++

Routine Description:

  This function determines if a file is XIP and should be rebased.  It will
  rebase any PE32 sections found in the file using the base address.

Arguments:

  FvInfo            A pointer to FV_INFO structure.
  FileName          Ffs File PathName
  FfsFile           A pointer to Ffs file image.
  XipOffset         The offset address to use for rebasing the XIP file image.
  FvMapFile         FvMapFile to record the function address in one Fvimage

Returns:

  EFI_SUCCESS             The image was properly rebased.
  EFI_INVALID_PARAMETER   An input parameter is invalid.
  EFI_ABORTED             An error occurred while rebasing the input file image.
  EFI_OUT_OF_RESOURCES    Could not allocate a required resource.
  EFI_NOT_FOUND           No compressed sections could be found.

--*/
{
  EFI_STATUS                            Status;
  PE_COFF_LOADER_IMAGE_CONTEXT          ImageContext;
  PE_COFF_LOADER_IMAGE_CONTEXT          OrigImageContext;
  EFI_PHYSICAL_ADDRESS                  XipBase;
  EFI_PHYSICAL_ADDRESS                  NewPe32BaseAddress;
  UINTN                                 Index;
  EFI_FILE_SECTION_POINTER              CurrentPe32Section;
  EFI_FFS_FILE_STATE                    SavedState;
  EFI_IMAGE_OPTIONAL_HEADER_UNION       *ImgHdr;
  EFI_TE_IMAGE_HEADER                   *TEImageHeader;
  UINT8                                 *MemoryImagePointer;
  EFI_IMAGE_SECTION_HEADER              *SectionHeader;
  CHAR8                                 PeFileName [MAX_LONG_FILE_PATH];
  CHAR8                                 *Cptr;
  FILE                                  *PeFile;
  UINT8                                 *PeFileBuffer;
  UINT32                                PeFileSize;
  CHAR8                                 *PdbPointer;
  UINT32                                FfsHeaderSize;
  UINT32                                CurSecHdrSize;

  Index              = 0;
  MemoryImagePointer = NULL;
  TEImageHeader      = NULL;
  ImgHdr             = NULL;
  SectionHeader      = NULL;
  Cptr               = NULL;
  PeFile             = NULL;
  PeFileBuffer       = NULL;

  //
  // Don't need to relocate image when BaseAddress is zero and no ForceRebase Flag specified.
  //
  if ((FvInfo->BaseAddress == 0) && (FvInfo->ForceRebase == -1)) {
    return EFI_SUCCESS;
  }

  //
  // If ForceRebase Flag specified to FALSE, will always not take rebase action.
  //
  if (FvInfo->ForceRebase == 0) {
    return EFI_SUCCESS;
  }


  XipBase = FvInfo->BaseAddress + XipOffset;

  //
  // We only process files potentially containing PE32 sections.
  //
  switch (FfsFile->Type) {
    case EFI_FV_FILETYPE_SECURITY_CORE:
    case EFI_FV_FILETYPE_PEI_CORE:
    case EFI_FV_FILETYPE_PEIM:
    case EFI_FV_FILETYPE_COMBINED_PEIM_DRIVER:
    case EFI_FV_FILETYPE_DRIVER:
    case EFI_FV_FILETYPE_DXE_CORE:
      break;
    case EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE:
      //
      // Rebase the inside FvImage.
      //
      GetChildFvFromFfs (FvInfo, FfsFile, XipOffset);

      //
      // Search PE/TE section in FV sectin.
      //
      break;
    default:
      return EFI_SUCCESS;
  }

  FfsHeaderSize = GetFfsHeaderLength(FfsFile);
  //
  // Rebase each PE32 section
  //
  Status      = EFI_SUCCESS;
  for (Index = 1;; Index++) {
    //
    // Init Value
    //
    NewPe32BaseAddress = 0;

    //
    // Find Pe Image
    //
    Status = GetSectionByType (FfsFile, EFI_SECTION_PE32, Index, &CurrentPe32Section);
    if (EFI_ERROR (Status)) {
      break;
    }
    CurSecHdrSize = GetSectionHeaderLength(CurrentPe32Section.CommonHeader);

    //
    // Initialize context
    //
    memset (&ImageContext, 0, sizeof (ImageContext));
    ImageContext.Handle     = (VOID *) ((UINTN) CurrentPe32Section.Pe32Section + CurSecHdrSize);
    ImageContext.ImageRead  = (PE_COFF_LOADER_READ_FILE) FfsRebaseImageRead;
    Status                  = PeCoffLoaderGetImageInfo (&ImageContext);
    if (EFI_ERROR (Status)) {
      Error (NULL, 0, 3000, "Invalid PeImage", "The input file is %s and the return status is %x", FileName, (int) Status);
      return Status;
    }

    if ( (ImageContext.Machine == EFI_IMAGE_MACHINE_ARMT) ||
         (ImageContext.Machine == EFI_IMAGE_MACHINE_AARCH64) ) {
      mArm = TRUE;
    }

    //
    // Keep Image Context for PE image in FV
    //
    memcpy (&OrigImageContext, &ImageContext, sizeof (ImageContext));

    //
    // Get File PdbPointer
    //
    PdbPointer = PeCoffLoaderGetPdbPointer (ImageContext.Handle);

    //
    // Get PeHeader pointer
    //
    ImgHdr = (EFI_IMAGE_OPTIONAL_HEADER_UNION *)((UINTN) CurrentPe32Section.Pe32Section + CurSecHdrSize + ImageContext.PeCoffHeaderOffset);

    //
    // Calculate the PE32 base address, based on file type
    //
    switch (FfsFile->Type) {
      case EFI_FV_FILETYPE_SECURITY_CORE:
      case EFI_FV_FILETYPE_PEI_CORE:
      case EFI_FV_FILETYPE_PEIM:
      case EFI_FV_FILETYPE_COMBINED_PEIM_DRIVER:
        //
        // Check if section-alignment and file-alignment match or not
        //
        if ((ImgHdr->Pe32.OptionalHeader.SectionAlignment != ImgHdr->Pe32.OptionalHeader.FileAlignment)) {
          //
          // Xip module has the same section alignment and file alignment.
          //
          Error (NULL, 0, 3000, "Invalid", "PE image Section-Alignment and File-Alignment do not match : %s.", FileName);
          return EFI_ABORTED;
        }
        //
        // PeImage has no reloc section. It will try to get reloc data from the original EFI image.
        //
        if (ImageContext.RelocationsStripped) {
          //
          // Construct the original efi file Name
          //
          if (strlen (FileName) >= MAX_LONG_FILE_PATH) {
            Error (NULL, 0, 2000, "Invalid", "The file name %s is too long.", FileName);
            return EFI_ABORTED;
          }
          strncpy (PeFileName, FileName, MAX_LONG_FILE_PATH - 1);
          PeFileName[MAX_LONG_FILE_PATH - 1] = 0;
          Cptr = PeFileName + strlen (PeFileName);
          while (*Cptr != '.') {
            Cptr --;
          }
          if (*Cptr != '.') {
            Error (NULL, 0, 3000, "Invalid", "The file %s has no .reloc section.", FileName);
            return EFI_ABORTED;
          } else {
            *(Cptr + 1) = 'e';
            *(Cptr + 2) = 'f';
            *(Cptr + 3) = 'i';
            *(Cptr + 4) = '\0';
          }
          PeFile = fopen (LongFilePath (PeFileName), "rb");
          if (PeFile == NULL) {
            Warning (NULL, 0, 0, "Invalid", "The file %s has no .reloc section.", FileName);
            //Error (NULL, 0, 3000, "Invalid", "The file %s has no .reloc section.", FileName);
            //return EFI_ABORTED;
            break;
          }
          //
          // Get the file size
          //
          PeFileSize = _filelength (fileno (PeFile));
          PeFileBuffer = (UINT8 *) malloc (PeFileSize);
          if (PeFileBuffer == NULL) {
            fclose (PeFile);
            Error (NULL, 0, 4001, "Resource", "memory cannot be allocated on rebase of %s", FileName);
            return EFI_OUT_OF_RESOURCES;
          }
          //
          // Read Pe File
          //
          fread (PeFileBuffer, sizeof (UINT8), PeFileSize, PeFile);
          //
          // close file
          //
          fclose (PeFile);
          //
          // Handle pointer to the original efi image.
          //
          ImageContext.Handle = PeFileBuffer;
          Status              = PeCoffLoaderGetImageInfo (&ImageContext);
          if (EFI_ERROR (Status)) {
            Error (NULL, 0, 3000, "Invalid PeImage", "The input file is %s and the return status is %x", FileName, (int) Status);
            return Status;
          }
          ImageContext.RelocationsStripped = FALSE;
        }

        NewPe32BaseAddress = XipBase + (UINTN) CurrentPe32Section.Pe32Section + CurSecHdrSize - (UINTN)FfsFile;
        break;

      case EFI_FV_FILETYPE_DRIVER:
      case EFI_FV_FILETYPE_DXE_CORE:
        //
        // Check if section-alignment and file-alignment match or not
        //
        if ((ImgHdr->Pe32.OptionalHeader.SectionAlignment != ImgHdr->Pe32.OptionalHeader.FileAlignment)) {
          //
          // Xip module has the same section alignment and file alignment.
          //
          Error (NULL, 0, 3000, "Invalid", "PE image Section-Alignment and File-Alignment do not match : %s.", FileName);
          return EFI_ABORTED;
        }
        NewPe32BaseAddress = XipBase + (UINTN) CurrentPe32Section.Pe32Section + CurSecHdrSize - (UINTN)FfsFile;
        break;

      default:
        //
        // Not supported file type
        //
        return EFI_SUCCESS;
    }

    //
    // Relocation doesn't exist
    //
    if (ImageContext.RelocationsStripped) {
      Warning (NULL, 0, 0, "Invalid", "The file %s has no .reloc section.", FileName);
      continue;
    }

    //
    // Relocation exist and rebase
    //
    //
    // Load and Relocate Image Data
    //
    MemoryImagePointer = (UINT8 *) malloc ((UINTN) ImageContext.ImageSize + ImageContext.SectionAlignment);
    if (MemoryImagePointer == NULL) {
      Error (NULL, 0, 4001, "Resource", "memory cannot be allocated on rebase of %s", FileName);
      return EFI_OUT_OF_RESOURCES;
    }
    memset ((VOID *) MemoryImagePointer, 0, (UINTN) ImageContext.ImageSize + ImageContext.SectionAlignment);
    ImageContext.ImageAddress = ((UINTN) MemoryImagePointer + ImageContext.SectionAlignment - 1) & (~((UINTN) ImageContext.SectionAlignment - 1));

    Status =  PeCoffLoaderLoadImage (&ImageContext);
    if (EFI_ERROR (Status)) {
      Error (NULL, 0, 3000, "Invalid", "LocateImage() call failed on rebase of %s", FileName);
      free ((VOID *) MemoryImagePointer);
      return Status;
    }

    ImageContext.DestinationAddress = NewPe32BaseAddress;
    Status                          = PeCoffLoaderRelocateImage (&ImageContext);
    if (EFI_ERROR (Status)) {
      Error (NULL, 0, 3000, "Invalid", "RelocateImage() call failed on rebase of %s", FileName);
      free ((VOID *) MemoryImagePointer);
      return Status;
    }

    //
    // Copy Relocated data to raw image file.
    //
    SectionHeader = (EFI_IMAGE_SECTION_HEADER *) (
                       (UINTN) ImgHdr +
                       sizeof (UINT32) +
                       sizeof (EFI_IMAGE_FILE_HEADER) +
                       ImgHdr->Pe32.FileHeader.SizeOfOptionalHeader
                       );

    for (Index = 0; Index < ImgHdr->Pe32.FileHeader.NumberOfSections; Index ++, SectionHeader ++) {
      CopyMem (
        (UINT8 *) CurrentPe32Section.Pe32Section + CurSecHdrSize + SectionHeader->PointerToRawData,
        (VOID*) (UINTN) (ImageContext.ImageAddress + SectionHeader->VirtualAddress),
        SectionHeader->SizeOfRawData
        );
    }

    free ((VOID *) MemoryImagePointer);
    MemoryImagePointer = NULL;
    if (PeFileBuffer != NULL) {
      free (PeFileBuffer);
      PeFileBuffer = NULL;
    }

    //
    // Update Image Base Address
    //
    if (ImgHdr->Pe32.OptionalHeader.Magic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
      ImgHdr->Pe32.OptionalHeader.ImageBase = (UINT32) NewPe32BaseAddress;
    } else if (ImgHdr->Pe32Plus.OptionalHeader.Magic == EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
      ImgHdr->Pe32Plus.OptionalHeader.ImageBase = NewPe32BaseAddress;
    } else {
      Error (NULL, 0, 3000, "Invalid", "unknown PE magic signature %X in PE32 image %s",
        ImgHdr->Pe32.OptionalHeader.Magic,
        FileName
        );
      return EFI_ABORTED;
    }

    //
    // Now update file checksum
    //
    if (FfsFile->Attributes & FFS_ATTRIB_CHECKSUM) {
      SavedState  = FfsFile->State;
      FfsFile->IntegrityCheck.Checksum.File = 0;
      FfsFile->State                        = 0;
      FfsFile->IntegrityCheck.Checksum.File = CalculateChecksum8 (
                                                (UINT8 *) ((UINT8 *)FfsFile + FfsHeaderSize),
                                                GetFfsFileLength (FfsFile) - FfsHeaderSize
                                                );
      FfsFile->State = SavedState;
    }

    //
    // Get this module function address from ModulePeMapFile and add them into FvMap file
    //

    //
    // Default use FileName as map file path
    //
    if (PdbPointer == NULL) {
      PdbPointer = FileName;
    }

    WriteMapFile (FvMapFile, PdbPointer, FfsFile, NewPe32BaseAddress, &OrigImageContext);
  }

  if (FfsFile->Type != EFI_FV_FILETYPE_SECURITY_CORE &&
      FfsFile->Type != EFI_FV_FILETYPE_PEI_CORE &&
      FfsFile->Type != EFI_FV_FILETYPE_PEIM &&
      FfsFile->Type != EFI_FV_FILETYPE_COMBINED_PEIM_DRIVER &&
      FfsFile->Type != EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE
      ) {
    //
    // Only Peim code may have a TE section
    //
    return EFI_SUCCESS;
  }

  //
  // Now process TE sections
  //
  for (Index = 1;; Index++) {
    NewPe32BaseAddress = 0;

    //
    // Find Te Image
    //
    Status = GetSectionByType (FfsFile, EFI_SECTION_TE, Index, &CurrentPe32Section);
    if (EFI_ERROR (Status)) {
      break;
    }

    CurSecHdrSize = GetSectionHeaderLength(CurrentPe32Section.CommonHeader);

    //
    // Calculate the TE base address, the FFS file base plus the offset of the TE section less the size stripped off
    // by GenTEImage
    //
    TEImageHeader = (EFI_TE_IMAGE_HEADER *) ((UINT8 *) CurrentPe32Section.Pe32Section + CurSecHdrSize);

    //
    // Initialize context, load image info.
    //
    memset (&ImageContext, 0, sizeof (ImageContext));
    ImageContext.Handle     = (VOID *) TEImageHeader;
    ImageContext.ImageRead  = (PE_COFF_LOADER_READ_FILE) FfsRebaseImageRead;
    Status                  = PeCoffLoaderGetImageInfo (&ImageContext);
    if (EFI_ERROR (Status)) {
      Error (NULL, 0, 3000, "Invalid TeImage", "The input file is %s and the return status is %x", FileName, (int) Status);
      return Status;
    }

    if ( (ImageContext.Machine == EFI_IMAGE_MACHINE_ARMT) ||
         (ImageContext.Machine == EFI_IMAGE_MACHINE_AARCH64) ) {
      mArm = TRUE;
    }

    //
    // Keep Image Context for TE image in FV
    //
    memcpy (&OrigImageContext, &ImageContext, sizeof (ImageContext));

    //
    // Get File PdbPointer
    //
    PdbPointer = PeCoffLoaderGetPdbPointer (ImageContext.Handle);

    //
    // Set new rebased address.
    //
    NewPe32BaseAddress = XipBase + (UINTN) TEImageHeader + sizeof (EFI_TE_IMAGE_HEADER) \
                         - TEImageHeader->StrippedSize - (UINTN) FfsFile;

    //
    // if reloc is stripped, try to get the original efi image to get reloc info.
    //
    if (ImageContext.RelocationsStripped) {
      //
      // Construct the original efi file name
      //
      if (strlen (FileName) >= MAX_LONG_FILE_PATH) {
        Error (NULL, 0, 2000, "Invalid", "The file name %s is too long.", FileName);
        return EFI_ABORTED;
      }
      strncpy (PeFileName, FileName, MAX_LONG_FILE_PATH - 1);
      PeFileName[MAX_LONG_FILE_PATH - 1] = 0;
      Cptr = PeFileName + strlen (PeFileName);
      while (*Cptr != '.') {
        Cptr --;
      }

      if (*Cptr != '.') {
        Error (NULL, 0, 3000, "Invalid", "The file %s has no .reloc section.", FileName);
        return EFI_ABORTED;
      } else {
        *(Cptr + 1) = 'e';
        *(Cptr + 2) = 'f';
        *(Cptr + 3) = 'i';
        *(Cptr + 4) = '\0';
      }

      PeFile = fopen (LongFilePath (PeFileName), "rb");
      if (PeFile == NULL) {
        Warning (NULL, 0, 0, "Invalid", "The file %s has no .reloc section.", FileName);
        //Error (NULL, 0, 3000, "Invalid", "The file %s has no .reloc section.", FileName);
        //return EFI_ABORTED;
      } else {
        //
        // Get the file size
        //
        PeFileSize = _filelength (fileno (PeFile));
        PeFileBuffer = (UINT8 *) malloc (PeFileSize);
        if (PeFileBuffer == NULL) {
          fclose (PeFile);
          Error (NULL, 0, 4001, "Resource", "memory cannot be allocated on rebase of %s", FileName);
          return EFI_OUT_OF_RESOURCES;
        }
        //
        // Read Pe File
        //
        fread (PeFileBuffer, sizeof (UINT8), PeFileSize, PeFile);
        //
        // close file
        //
        fclose (PeFile);
        //
        // Append reloc section into TeImage
        //
        ImageContext.Handle = PeFileBuffer;
        Status              = PeCoffLoaderGetImageInfo (&ImageContext);
        if (EFI_ERROR (Status)) {
          Error (NULL, 0, 3000, "Invalid TeImage", "The input file is %s and the return status is %x", FileName, (int) Status);
          return Status;
        }
        ImageContext.RelocationsStripped = FALSE;
      }
    }
    //
    // Relocation doesn't exist
    //
    if (ImageContext.RelocationsStripped) {
      Warning (NULL, 0, 0, "Invalid", "The file %s has no .reloc section.", FileName);
      continue;
    }

    //
    // Relocation exist and rebase
    //
    //
    // Load and Relocate Image Data
    //
    MemoryImagePointer = (UINT8 *) malloc ((UINTN) ImageContext.ImageSize + ImageContext.SectionAlignment);
    if (MemoryImagePointer == NULL) {
      Error (NULL, 0, 4001, "Resource", "memory cannot be allocated on rebase of %s", FileName);
      return EFI_OUT_OF_RESOURCES;
    }
    memset ((VOID *) MemoryImagePointer, 0, (UINTN) ImageContext.ImageSize + ImageContext.SectionAlignment);
    ImageContext.ImageAddress = ((UINTN) MemoryImagePointer + ImageContext.SectionAlignment - 1) & (~((UINTN) ImageContext.SectionAlignment - 1));

    Status =  PeCoffLoaderLoadImage (&ImageContext);
    if (EFI_ERROR (Status)) {
      Error (NULL, 0, 3000, "Invalid", "LocateImage() call failed on rebase of %s", FileName);
      free ((VOID *) MemoryImagePointer);
      return Status;
    }
    //
    // Reloacate TeImage
    //
    ImageContext.DestinationAddress = NewPe32BaseAddress;
    Status                          = PeCoffLoaderRelocateImage (&ImageContext);
    if (EFI_ERROR (Status)) {
      Error (NULL, 0, 3000, "Invalid", "RelocateImage() call failed on rebase of TE image %s", FileName);
      free ((VOID *) MemoryImagePointer);
      return Status;
    }

    //
    // Copy the relocated image into raw image file.
    //
    SectionHeader = (EFI_IMAGE_SECTION_HEADER *) (TEImageHeader + 1);
    for (Index = 0; Index < TEImageHeader->NumberOfSections; Index ++, SectionHeader ++) {
      if (!ImageContext.IsTeImage) {
        CopyMem (
          (UINT8 *) TEImageHeader + sizeof (EFI_TE_IMAGE_HEADER) - TEImageHeader->StrippedSize + SectionHeader->PointerToRawData,
          (VOID*) (UINTN) (ImageContext.ImageAddress + SectionHeader->VirtualAddress),
          SectionHeader->SizeOfRawData
          );
      } else {
        CopyMem (
          (UINT8 *) TEImageHeader + sizeof (EFI_TE_IMAGE_HEADER) - TEImageHeader->StrippedSize + SectionHeader->PointerToRawData,
          (VOID*) (UINTN) (ImageContext.ImageAddress + sizeof (EFI_TE_IMAGE_HEADER) - TEImageHeader->StrippedSize + SectionHeader->VirtualAddress),
          SectionHeader->SizeOfRawData
          );
      }
    }

    //
    // Free the allocated memory resource
    //
    free ((VOID *) MemoryImagePointer);
    MemoryImagePointer = NULL;
    if (PeFileBuffer != NULL) {
      free (PeFileBuffer);
      PeFileBuffer = NULL;
    }

    //
    // Update Image Base Address
    //
    TEImageHeader->ImageBase = NewPe32BaseAddress;

    //
    // Now update file checksum
    //
    if (FfsFile->Attributes & FFS_ATTRIB_CHECKSUM) {
      SavedState  = FfsFile->State;
      FfsFile->IntegrityCheck.Checksum.File = 0;
      FfsFile->State                        = 0;
      FfsFile->IntegrityCheck.Checksum.File = CalculateChecksum8 (
                                                (UINT8 *)((UINT8 *)FfsFile + FfsHeaderSize),
                                                GetFfsFileLength (FfsFile) - FfsHeaderSize
                                                );
      FfsFile->State = SavedState;
    }
    //
    // Get this module function address from ModulePeMapFile and add them into FvMap file
    //

    //
    // Default use FileName as map file path
    //
    if (PdbPointer == NULL) {
      PdbPointer = FileName;
    }

    WriteMapFile (
      FvMapFile,
      PdbPointer,
      FfsFile,
      NewPe32BaseAddress,
      &OrigImageContext
      );
  }

  return EFI_SUCCESS;
}

EFI_STATUS
FindApResetVectorPosition (
  IN  MEMORY_FILE  *FvImage,
  OUT UINT8        **Pointer
  )
/*++

Routine Description:

  Find the position in this FvImage to place Ap reset vector.

Arguments:

  FvImage       Memory file for the FV memory image.
  Pointer       Pointer to pointer to position.

Returns:

  EFI_NOT_FOUND   - No satisfied position is found.
  EFI_SUCCESS     - The suitable position is return.

--*/
{
  EFI_FFS_FILE_HEADER   *PadFile;
  UINT32                Index;
  EFI_STATUS            Status;
  UINT8                 *FixPoint;
  UINT32                FileLength;

  for (Index = 1; ;Index ++) {
    //
    // Find Pad File to add ApResetVector info
    //
    Status = GetFileByType (EFI_FV_FILETYPE_FFS_PAD, Index, &PadFile);
    if (EFI_ERROR (Status) || (PadFile == NULL)) {
      //
      // No Pad file to be found.
      //
      break;
    }
    //
    // Get Pad file size.
    //
    FileLength = GetFfsFileLength(PadFile);
    FileLength = (FileLength + EFI_FFS_FILE_HEADER_ALIGNMENT - 1) & ~(EFI_FFS_FILE_HEADER_ALIGNMENT - 1);
    //
    // FixPoint must be align on 0x1000 relative to FvImage Header
    //
    FixPoint = (UINT8*) PadFile + GetFfsHeaderLength(PadFile);
    FixPoint = FixPoint + 0x1000 - (((UINTN) FixPoint - (UINTN) FvImage->FileImage) & 0xFFF);
    //
    // FixPoint be larger at the last place of one fv image.
    //
    while (((UINTN) FixPoint + SIZEOF_STARTUP_DATA_ARRAY - (UINTN) PadFile) <= FileLength) {
      FixPoint += 0x1000;
    }
    FixPoint -= 0x1000;

    if ((UINTN) FixPoint < ((UINTN) PadFile + GetFfsHeaderLength(PadFile))) {
      //
      // No alignment FixPoint in this Pad File.
      //
      continue;
    }

    if ((UINTN) FvImage->Eof - (UINTN)FixPoint <= 0x20000) {
      //
      // Find the position to place ApResetVector
      //
      *Pointer = FixPoint;
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

EFI_STATUS
ParseCapInf (
  IN  MEMORY_FILE  *InfFile,
  OUT CAP_INFO     *CapInfo
  )
/*++

Routine Description:

  This function parses a Cap.INF file and copies info into a CAP_INFO structure.

Arguments:

  InfFile        Memory file image.
  CapInfo        Information read from INF file.

Returns:

  EFI_SUCCESS       INF file information successfully retrieved.
  EFI_ABORTED       INF file has an invalid format.
  EFI_NOT_FOUND     A required string was not found in the INF file.
--*/
{
  CHAR8       Value[MAX_LONG_FILE_PATH];
  UINT64      Value64;
  UINTN       Index, Number;
  EFI_STATUS  Status;

  //
  // Initialize Cap info
  //
  // memset (CapInfo, 0, sizeof (CAP_INFO));
  //

  //
  // Read the Capsule Guid
  //
  Status = FindToken (InfFile, OPTIONS_SECTION_STRING, EFI_CAPSULE_GUID_STRING, 0, Value);
  if (Status == EFI_SUCCESS) {
    //
    // Get the Capsule Guid
    //
    Status = StringToGuid (Value, &CapInfo->CapGuid);
    if (EFI_ERROR (Status)) {
      Error (NULL, 0, 2000, "Invalid parameter", "%s = %s", EFI_CAPSULE_GUID_STRING, Value);
      return EFI_ABORTED;
    }
    DebugMsg (NULL, 0, 9, "Capsule Guid", "%s = %s", EFI_CAPSULE_GUID_STRING, Value);
  }

  //
  // Read the Capsule Header Size
  //
  Status = FindToken (InfFile, OPTIONS_SECTION_STRING, EFI_CAPSULE_HEADER_SIZE_STRING, 0, Value);
  if (Status == EFI_SUCCESS) {
    Status = AsciiStringToUint64 (Value, FALSE, &Value64);
    if (EFI_ERROR (Status)) {
      Error (NULL, 0, 2000, "Invalid parameter", "%s = %s", EFI_CAPSULE_HEADER_SIZE_STRING, Value);
      return EFI_ABORTED;
    }
    CapInfo->HeaderSize = (UINT32) Value64;
    DebugMsg (NULL, 0, 9, "Capsule Header size", "%s = %s", EFI_CAPSULE_HEADER_SIZE_STRING, Value);
  }

  //
  // Read the Capsule Flag
  //
  Status = FindToken (InfFile, OPTIONS_SECTION_STRING, EFI_CAPSULE_FLAGS_STRING, 0, Value);
  if (Status == EFI_SUCCESS) {
    if (strstr (Value, "PopulateSystemTable") != NULL) {
      CapInfo->Flags |= CAPSULE_FLAGS_PERSIST_ACROSS_RESET | CAPSULE_FLAGS_POPULATE_SYSTEM_TABLE;
      if (strstr (Value, "InitiateReset") != NULL) {
        CapInfo->Flags |= CAPSULE_FLAGS_INITIATE_RESET;
      }
    } else if (strstr (Value, "PersistAcrossReset") != NULL) {
      CapInfo->Flags |= CAPSULE_FLAGS_PERSIST_ACROSS_RESET;
      if (strstr (Value, "InitiateReset") != NULL) {
        CapInfo->Flags |= CAPSULE_FLAGS_INITIATE_RESET;
      }
    } else {
      Error (NULL, 0, 2000, "Invalid parameter", "invalid Flag setting for %s.", EFI_CAPSULE_FLAGS_STRING);
      return EFI_ABORTED;
    }
    DebugMsg (NULL, 0, 9, "Capsule Flag", Value);
  }

  Status = FindToken (InfFile, OPTIONS_SECTION_STRING, EFI_OEM_CAPSULE_FLAGS_STRING, 0, Value);
  if (Status == EFI_SUCCESS) {
    Status = AsciiStringToUint64 (Value, FALSE, &Value64);
    if (EFI_ERROR (Status) || Value64 > 0xffff) {
      Error (NULL, 0, 2000, "Invalid parameter",
        "invalid Flag setting for %s. Must be integer value between 0x0000 and 0xffff.",
        EFI_OEM_CAPSULE_FLAGS_STRING);
      return EFI_ABORTED;
    }
    CapInfo->Flags |= Value64;
    DebugMsg (NULL, 0, 9, "Capsule Extend Flag", Value);
  }

  //
  // Read Capsule File name
  //
  Status = FindToken (InfFile, OPTIONS_SECTION_STRING, EFI_FILE_NAME_STRING, 0, Value);
  if (Status == EFI_SUCCESS) {
    //
    // Get output file name
    //
    strcpy (CapInfo->CapName, Value);
  }

  //
  // Read the Capsule FileImage
  //
  Number = 0;
  for (Index = 0; Index < MAX_NUMBER_OF_FILES_IN_CAP; Index++) {
    if (CapInfo->CapFiles[Index][0] != '\0') {
      continue;
    }
    //
    // Read the capsule file name
    //
    Status = FindToken (InfFile, FILES_SECTION_STRING, EFI_FILE_NAME_STRING, Number++, Value);

    if (Status == EFI_SUCCESS) {
      //
      // Add the file
      //
      strcpy (CapInfo->CapFiles[Index], Value);
      DebugMsg (NULL, 0, 9, "Capsule component file", "the %uth file name is %s", (unsigned) Index, CapInfo->CapFiles[Index]);
    } else {
      break;
    }
  }

  if (Index == 0) {
    Warning (NULL, 0, 0, "Capsule components are not specified.", NULL);
  }

  return EFI_SUCCESS;
}

EFI_STATUS
GenerateCapImage (
  IN CHAR8                *InfFileImage,
  IN UINTN                InfFileSize,
  IN CHAR8                *CapFileName
  )
/*++

Routine Description:

  This is the main function which will be called from application to create UEFI Capsule image.

Arguments:

  InfFileImage   Buffer containing the INF file contents.
  InfFileSize    Size of the contents of the InfFileImage buffer.
  CapFileName    Requested name for the Cap file.

Returns:

  EFI_SUCCESS             Function completed successfully.
  EFI_OUT_OF_RESOURCES    Could not allocate required resources.
  EFI_ABORTED             Error encountered.
  EFI_INVALID_PARAMETER   A required parameter was NULL.

--*/
{
  UINT32                CapSize;
  UINT8                 *CapBuffer;
  EFI_CAPSULE_HEADER    *CapsuleHeader;
  MEMORY_FILE           InfMemoryFile;
  UINT32                FileSize;
  UINT32                Index;
  FILE                  *fpin, *fpout;
  EFI_STATUS            Status;

  if (InfFileImage != NULL) {
    //
    // Initialize file structures
    //
    InfMemoryFile.FileImage           = InfFileImage;
    InfMemoryFile.CurrentFilePointer  = InfFileImage;
    InfMemoryFile.Eof                 = InfFileImage + InfFileSize;

    //
    // Parse the Cap inf file for header information
    //
    Status = ParseCapInf (&InfMemoryFile, &mCapDataInfo);
    if (Status != EFI_SUCCESS) {
      return Status;
    }
  }

  if (mCapDataInfo.HeaderSize == 0) {
    //
    // make header size align 16 bytes.
    //
    mCapDataInfo.HeaderSize = sizeof (EFI_CAPSULE_HEADER);
    mCapDataInfo.HeaderSize = (mCapDataInfo.HeaderSize + 0xF) & ~0xF;
  }

  if (mCapDataInfo.HeaderSize < sizeof (EFI_CAPSULE_HEADER)) {
    Error (NULL, 0, 2000, "Invalid parameter", "The specified HeaderSize cannot be less than the size of EFI_CAPSULE_HEADER.");
    return EFI_INVALID_PARAMETER;
  }

  if (CapFileName == NULL && mCapDataInfo.CapName[0] != '\0') {
    CapFileName = mCapDataInfo.CapName;
  }

  if (CapFileName == NULL) {
    Error (NULL, 0, 2001, "Missing required argument", "Output Capsule file name");
    return EFI_INVALID_PARAMETER;
  }

  //
  // Set Default Capsule Guid value
  //
  if (CompareGuid (&mCapDataInfo.CapGuid, &mZeroGuid) == 0) {
    memcpy (&mCapDataInfo.CapGuid, &mDefaultCapsuleGuid, sizeof (EFI_GUID));
  }
  //
  // Calculate the size of capsule image.
  //
  Index    = 0;
  FileSize = 0;
  CapSize  = mCapDataInfo.HeaderSize;
  while (mCapDataInfo.CapFiles [Index][0] != '\0') {
    fpin = fopen (LongFilePath (mCapDataInfo.CapFiles[Index]), "rb");
    if (fpin == NULL) {
      Error (NULL, 0, 0001, "Error opening file", mCapDataInfo.CapFiles[Index]);
      return EFI_ABORTED;
    }
    FileSize  = _filelength (fileno (fpin));
    CapSize  += FileSize;
    fclose (fpin);
    Index ++;
  }

  //
  // Allocate buffer for capsule image.
  //
  CapBuffer = (UINT8 *) malloc (CapSize);
  if (CapBuffer == NULL) {
    Error (NULL, 0, 4001, "Resource", "memory cannot be allocated for creating the capsule.");
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Initialize the capsule header to zero
  //
  memset (CapBuffer, 0, mCapDataInfo.HeaderSize);

  //
  // create capsule header and get capsule body
  //
  CapsuleHeader = (EFI_CAPSULE_HEADER *) CapBuffer;
  memcpy (&CapsuleHeader->CapsuleGuid, &mCapDataInfo.CapGuid, sizeof (EFI_GUID));
  CapsuleHeader->HeaderSize       = mCapDataInfo.HeaderSize;
  CapsuleHeader->Flags            = mCapDataInfo.Flags;
  CapsuleHeader->CapsuleImageSize = CapSize;

  Index    = 0;
  FileSize = 0;
  CapSize  = CapsuleHeader->HeaderSize;
  while (mCapDataInfo.CapFiles [Index][0] != '\0') {
    fpin = fopen (LongFilePath (mCapDataInfo.CapFiles[Index]), "rb");
    if (fpin == NULL) {
      Error (NULL, 0, 0001, "Error opening file", mCapDataInfo.CapFiles[Index]);
      free (CapBuffer);
      return EFI_ABORTED;
    }
    FileSize = _filelength (fileno (fpin));
    fread (CapBuffer + CapSize, 1, FileSize, fpin);
    fclose (fpin);
    Index ++;
    CapSize += FileSize;
  }

  //
  // write capsule data into the output file
  //
  fpout = fopen (LongFilePath (CapFileName), "wb");
  if (fpout == NULL) {
    Error (NULL, 0, 0001, "Error opening file", CapFileName);
    free (CapBuffer);
    return EFI_ABORTED;
  }

  fwrite (CapBuffer, 1, CapSize, fpout);
  fclose (fpout);
  free (CapBuffer);

  VerboseMsg ("The size of the generated capsule image is %u bytes", (unsigned) CapSize);

  return EFI_SUCCESS;
}
