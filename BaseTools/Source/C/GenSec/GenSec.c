/** @file
Creates output file that is a properly formed section per the PI spec.

Copyright (c) 2004 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#ifndef __GNUC__
#include <windows.h>
#include <io.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <Common/UefiBaseTypes.h>
#include <Common/PiFirmwareFile.h>
#include <Protocol/GuidedSectionExtraction.h>
#include <IndustryStandard/PeImage.h>

#include "CommonLib.h"
#include "Compress.h"
#include "Crc32.h"
#include "EfiUtilityMsgs.h"
#include "ParseInf.h"
#include "FvLib.h"
#include "PeCoffLib.h"

//
// GenSec Tool Information
//
#define UTILITY_NAME            "GenSec"
#define UTILITY_MAJOR_VERSION   0
#define UTILITY_MINOR_VERSION   1

STATIC CHAR8      *mSectionTypeName[] = {
  NULL,                                 // 0x00 - reserved
  "EFI_SECTION_COMPRESSION",            // 0x01
  "EFI_SECTION_GUID_DEFINED",           // 0x02
  NULL,                                 // 0x03 - reserved
  NULL,                                 // 0x04 - reserved
  NULL,                                 // 0x05 - reserved
  NULL,                                 // 0x06 - reserved
  NULL,                                 // 0x07 - reserved
  NULL,                                 // 0x08 - reserved
  NULL,                                 // 0x09 - reserved
  NULL,                                 // 0x0A - reserved
  NULL,                                 // 0x0B - reserved
  NULL,                                 // 0x0C - reserved
  NULL,                                 // 0x0D - reserved
  NULL,                                 // 0x0E - reserved
  NULL,                                 // 0x0F - reserved
  "EFI_SECTION_PE32",                   // 0x10
  "EFI_SECTION_PIC",                    // 0x11
  "EFI_SECTION_TE",                     // 0x12
  "EFI_SECTION_DXE_DEPEX",              // 0x13
  "EFI_SECTION_VERSION",                // 0x14
  "EFI_SECTION_USER_INTERFACE",         // 0x15
  "EFI_SECTION_COMPATIBILITY16",        // 0x16
  "EFI_SECTION_FIRMWARE_VOLUME_IMAGE",  // 0x17
  "EFI_SECTION_FREEFORM_SUBTYPE_GUID",  // 0x18
  "EFI_SECTION_RAW",                    // 0x19
  NULL,                                 // 0x1A
  "EFI_SECTION_PEI_DEPEX",              // 0x1B
  "EFI_SECTION_SMM_DEPEX"               // 0x1C
};

STATIC CHAR8      *mCompressionTypeName[]    = { "PI_NONE", "PI_STD" };

#define EFI_GUIDED_SECTION_NONE 0x80
STATIC CHAR8      *mGUIDedSectionAttribue[]  = { "NONE", "PROCESSING_REQUIRED", "AUTH_STATUS_VALID"};

STATIC CHAR8 *mAlignName[] = {
  "1", "2", "4", "8", "16", "32", "64", "128", "256", "512",
  "1K", "2K", "4K", "8K", "16K", "32K", "64K", "128K", "256K",
  "512K", "1M", "2M", "4M", "8M", "16M"
};

//
// Crc32 GUID section related definitions.
//
typedef struct {
  EFI_GUID_DEFINED_SECTION  GuidSectionHeader;
  UINT32                    CRC32Checksum;
} CRC32_SECTION_HEADER;

typedef struct {
  EFI_GUID_DEFINED_SECTION2 GuidSectionHeader;
  UINT32                    CRC32Checksum;
} CRC32_SECTION_HEADER2;

STATIC EFI_GUID  mZeroGuid                 = {0x0, 0x0, 0x0, {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}};
STATIC EFI_GUID  mEfiCrc32SectionGuid      = EFI_CRC32_GUIDED_SECTION_EXTRACTION_PROTOCOL_GUID;

STATIC
VOID
Version (
  VOID
  )
/*++

Routine Description:

  Print out version information for this utility.

Arguments:

  None

Returns:

  None

--*/
{
  fprintf (stdout, "%s Version %d.%d %s \n", UTILITY_NAME, UTILITY_MAJOR_VERSION, UTILITY_MINOR_VERSION, __BUILD_VERSION);
}

STATIC
VOID
Usage (
  VOID
  )
/*++

Routine Description:

  Print Help message.

Arguments:

  VOID

Returns:

  None

--*/
{
  //
  // Summary usage
  //
  fprintf (stdout, "\nUsage: %s [options] [input_file]\n\n", UTILITY_NAME);

  //
  // Copyright declaration
  //
  fprintf (stdout, "Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.\n\n");

  //
  // Details Option
  //
  fprintf (stdout, "Options:\n");
  fprintf (stdout, "  -o FileName, --outputfile FileName\n\
                        File is the SectionFile to be created.\n");
  fprintf (stdout, "  -s [SectionType], --sectiontype [SectionType]\n\
                        SectionType defined in PI spec is one type of\n\
                        EFI_SECTION_COMPRESSION, EFI_SECTION_GUID_DEFINED,\n\
                        EFI_SECTION_PE32, EFI_SECTION_PIC, EFI_SECTION_TE,\n\
                        EFI_SECTION_DXE_DEPEX, EFI_SECTION_COMPATIBILITY16,\n\
                        EFI_SECTION_USER_INTERFACE, EFI_SECTION_VERSION,\n\
                        EFI_SECTION_FIRMWARE_VOLUME_IMAGE, EFI_SECTION_RAW,\n\
                        EFI_SECTION_FREEFORM_SUBTYPE_GUID,\n\
                        EFI_SECTION_PEI_DEPEX, EFI_SECTION_SMM_DEPEX.\n\
                        if -s option is not given, \n\
                        EFI_SECTION_ALL is default section type.\n");
  fprintf (stdout, "  -c [Type], --compress [Type]\n\
                        Compress method type can be PI_NONE or PI_STD.\n\
                        if -c option is not given, PI_STD is default type.\n");
  fprintf (stdout, "  -g GuidValue, --vendor GuidValue\n\
                        GuidValue is one specific vendor guid value.\n\
                        Its format is xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx\n");
  fprintf (stdout, "  -l GuidHeaderLength, --HeaderLength GuidHeaderLength\n\
                        GuidHeaderLength is the size of header of guided data\n");
  fprintf (stdout, "  -r GuidAttr, --attributes GuidAttr\n\
                        GuidAttr is guid section atttributes, which may be\n\
                        PROCESSING_REQUIRED, AUTH_STATUS_VALID and NONE. \n\
                        if -r option is not given, default PROCESSING_REQUIRED\n");
  fprintf (stdout, "  -n String, --name String\n\
                        String is a NULL terminated string used in Ui section.\n");
  fprintf (stdout, "  -j Number, --buildnumber Number\n\
                        Number is an integer value between 0 and 65535\n\
                        used in Ver section.\n");
  fprintf (stdout, "  --sectionalign SectionAlign\n\
                        SectionAlign points to section alignment, which support\n\
                        the alignment scope 0~16M. If SectionAlign is specified\n\
                        as 0, tool get alignment value from SectionFile. It is\n\
                        specified in same order that the section file is input.\n");
  fprintf (stdout, "  --dummy dummyfile\n\
                        compare dummpyfile with input_file to decide whether\n\
                        need to set PROCESSING_REQUIRED attribute.\n");
  fprintf (stdout, "  -v, --verbose         Turn on verbose output with informational messages.\n");
  fprintf (stdout, "  -q, --quiet           Disable all messages except key message and fatal error\n");
  fprintf (stdout, "  -d, --debug level     Enable debug messages, at input debug level.\n");
  fprintf (stdout, "  --version             Show program's version number and exit.\n");
  fprintf (stdout, "  -h, --help            Show this help message and exit.\n");
}

VOID
Ascii2UnicodeString (
  CHAR8    *String,
  CHAR16   *UniString
  )
/*++

Routine Description:

  Write ascii string as unicode string format to FILE

Arguments:

  String      - Pointer to string that is written to FILE.
  UniString   - Pointer to unicode string

Returns:

  NULL

--*/
{
  while (*String != '\0') {
    *(UniString++) = (CHAR16) *(String++);
  }
  //
  // End the UniString with a NULL.
  //
  *UniString = '\0';
}

STATUS
GenSectionCommonLeafSection (
  CHAR8   **InputFileName,
  UINT32  InputFileNum,
  UINT8   SectionType,
  UINT8   **OutFileBuffer
  )
/*++

Routine Description:

  Generate a leaf section of type other than EFI_SECTION_VERSION
  and EFI_SECTION_USER_INTERFACE. Input file must be well formed.
  The function won't validate the input file's contents. For
  common leaf sections, the input file may be a binary file.
  The utility will add section header to the file.

Arguments:

  InputFileName  - Name of the input file.

  InputFileNum   - Number of input files. Should be 1 for leaf section.

  SectionType    - A valid section type string

  OutFileBuffer  - Buffer pointer to Output file contents

Returns:

  STATUS_ERROR            - can't continue
  STATUS_SUCCESS          - successful return

--*/
{
  UINT32                    InputFileLength;
  FILE                      *InFile;
  UINT8                     *Buffer;
  UINT32                    TotalLength;
  UINT32                    HeaderLength;
  EFI_COMMON_SECTION_HEADER *CommonSect;
  STATUS                    Status;

  if (InputFileNum > 1) {
    Error (NULL, 0, 2000, "Invalid parameter", "more than one input file specified");
    return STATUS_ERROR;
  } else if (InputFileNum < 1) {
    Error (NULL, 0, 2000, "Invalid parameter", "no input file specified");
    return STATUS_ERROR;
  }
  //
  // Open the input file
  //
  InFile = fopen (LongFilePath (InputFileName[0]), "rb");
  if (InFile == NULL) {
    Error (NULL, 0, 0001, "Error opening file", InputFileName[0]);
    return STATUS_ERROR;
  }

  Status  = STATUS_ERROR;
  Buffer  = NULL;
  //
  // Seek to the end of the input file so we can determine its size
  //
  fseek (InFile, 0, SEEK_END);
  InputFileLength = ftell (InFile);
  fseek (InFile, 0, SEEK_SET);
  DebugMsg (NULL, 0, 9, "Input file", "File name is %s and File size is %u bytes", InputFileName[0], (unsigned) InputFileLength);
  TotalLength     = sizeof (EFI_COMMON_SECTION_HEADER) + InputFileLength;
  //
  // Size must fit in 3 bytes
  //
  //if (TotalLength >= MAX_SECTION_SIZE) {
  //  Error (NULL, 0, 2000, "Invalid parameter", "%s file size (0x%X) exceeds section size limit(%uM).", InputFileName[0], (unsigned) TotalLength, MAX_SECTION_SIZE>>20);
  //  goto Done;
  //}
  HeaderLength = sizeof (EFI_COMMON_SECTION_HEADER);
  if (TotalLength >= MAX_SECTION_SIZE) {
    TotalLength = sizeof (EFI_COMMON_SECTION_HEADER2) + InputFileLength;
    HeaderLength = sizeof (EFI_COMMON_SECTION_HEADER2);
  }
  VerboseMsg ("the size of the created section file is %u bytes", (unsigned) TotalLength);
  //
  // Fill in the fields in the local section header structure
  //
  Buffer = (UINT8 *) malloc ((size_t) TotalLength);
  if (Buffer == NULL) {
    Error (NULL, 0, 4001, "Resource", "memory cannot be allocated");
    goto Done;
  }
  CommonSect = (EFI_COMMON_SECTION_HEADER *) Buffer;
  CommonSect->Type     = SectionType;
  if (TotalLength < MAX_SECTION_SIZE) {
    CommonSect->Size[0]  = (UINT8) (TotalLength & 0xff);
    CommonSect->Size[1]  = (UINT8) ((TotalLength & 0xff00) >> 8);
    CommonSect->Size[2]  = (UINT8) ((TotalLength & 0xff0000) >> 16);
  } else {
    memset(CommonSect->Size, 0xff, sizeof(UINT8) * 3);
    ((EFI_COMMON_SECTION_HEADER2 *)CommonSect)->ExtendedSize = TotalLength;
  }

  //
  // read data from the input file.
  //
  if (InputFileLength != 0) {
    if (fread (Buffer + HeaderLength, (size_t) InputFileLength, 1, InFile) != 1) {
      Error (NULL, 0, 0004, "Error reading file", InputFileName[0]);
      goto Done;
    }
  }

  //
  // Set OutFileBuffer
  //
  *OutFileBuffer = Buffer;
  Status = STATUS_SUCCESS;

Done:
  fclose (InFile);

  return Status;
}

STATIC
EFI_STATUS
StringtoAlignment (
  IN  CHAR8  *AlignBuffer,
  OUT UINT32 *AlignNumber
  )
/*++

Routine Description:

  Converts Align String to align value (1~16M).

Arguments:

  AlignBuffer    - Pointer to Align string.
  AlignNumber    - Pointer to Align value.

Returns:

  EFI_SUCCESS             Successfully convert align string to align value.
  EFI_INVALID_PARAMETER   Align string is invalid or align value is not in scope.

--*/
{
  UINT32 Index = 0;
  //
  // Check AlignBuffer
  //
  if (AlignBuffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  for (Index = 0; Index < sizeof (mAlignName) / sizeof (CHAR8 *); Index ++) {
    if (stricmp (AlignBuffer, mAlignName [Index]) == 0) {
      *AlignNumber = 1 << Index;
      return EFI_SUCCESS;
    }
  }
  return EFI_INVALID_PARAMETER;
}

EFI_STATUS
GetSectionContents (
  CHAR8   **InputFileName,
  UINT32  *InputFileAlign,
  UINT32  InputFileNum,
  UINT8   *FileBuffer,
  UINT32  *BufferLength
  )
/*++

Routine Description:

  Get the contents of all section files specified in InputFileName
  into FileBuffer.

Arguments:

  InputFileName  - Name of the input file.

  InputFileAlign - Alignment required by the input file data.

  InputFileNum   - Number of input files. Should be at least 1.

  FileBuffer     - Output buffer to contain data

  BufferLength   - On input, this is size of the FileBuffer.
                   On output, this is the actual length of the data.

Returns:

  EFI_SUCCESS on successful return
  EFI_INVALID_PARAMETER if InputFileNum is less than 1 or BufferLength point is NULL.
  EFI_ABORTED if unable to open input file.
  EFI_BUFFER_TOO_SMALL FileBuffer is not enough to contain all file data.
--*/
{
  UINT32                     Size;
  UINT32                     Offset;
  UINT32                     FileSize;
  UINT32                     Index;
  FILE                       *InFile;
  EFI_COMMON_SECTION_HEADER  *SectHeader;
  EFI_COMMON_SECTION_HEADER2 TempSectHeader;
  EFI_TE_IMAGE_HEADER        TeHeader;
  UINT32                     TeOffset;
  EFI_GUID_DEFINED_SECTION   GuidSectHeader;
  EFI_GUID_DEFINED_SECTION2  GuidSectHeader2;
  UINT32                     HeaderSize;

  if (InputFileNum < 1) {
    Error (NULL, 0, 2000, "Invalid parameter", "must specify at least one input file");
    return EFI_INVALID_PARAMETER;
  }

  if (BufferLength == NULL) {
    Error (NULL, 0, 2000, "Invalid parameter", "BufferLength can't be NULL");
    return EFI_INVALID_PARAMETER;
  }

  Size          = 0;
  Offset        = 0;
  TeOffset      = 0;
  //
  // Go through our array of file names and copy their contents
  // to the output buffer.
  //
  for (Index = 0; Index < InputFileNum; Index++) {
    //
    // make sure section ends on a DWORD boundary
    //
    while ((Size & 0x03) != 0) {
      if (FileBuffer != NULL && Size < *BufferLength) {
        FileBuffer[Size] = 0;
      }
      Size++;
    }

    //
    // Open file and read contents
    //
    InFile = fopen (LongFilePath (InputFileName[Index]), "rb");
    if (InFile == NULL) {
      Error (NULL, 0, 0001, "Error opening file", InputFileName[Index]);
      return EFI_ABORTED;
    }

    fseek (InFile, 0, SEEK_END);
    FileSize = ftell (InFile);
    fseek (InFile, 0, SEEK_SET);
    DebugMsg (NULL, 0, 9, "Input files", "the input file name is %s and the size is %u bytes", InputFileName[Index], (unsigned) FileSize);
    //
    // Adjust section buffer when section alignment is required.
    //
    if (InputFileAlign != NULL) {
      //
      // Check this section is Te/Pe section, and Calculate the numbers of Te/Pe section.
      //
      TeOffset = 0;
      //
      // The section might be EFI_COMMON_SECTION_HEADER2
      // But only Type needs to be checked
      //
      if (FileSize >= MAX_SECTION_SIZE) {
        HeaderSize = sizeof (EFI_COMMON_SECTION_HEADER2);
      } else {
        HeaderSize = sizeof (EFI_COMMON_SECTION_HEADER);
      }
      fread (&TempSectHeader, 1, HeaderSize, InFile);
      if (TempSectHeader.Type == EFI_SECTION_TE) {
        fread (&TeHeader, 1, sizeof (TeHeader), InFile);
        if (TeHeader.Signature == EFI_TE_IMAGE_HEADER_SIGNATURE) {
          TeOffset = TeHeader.StrippedSize - sizeof (TeHeader);
        }
      } else if (TempSectHeader.Type == EFI_SECTION_GUID_DEFINED) {
        fseek (InFile, 0, SEEK_SET);
        if (FileSize >= MAX_SECTION_SIZE) {
          fread (&GuidSectHeader2, 1, sizeof (GuidSectHeader2), InFile);
          if ((GuidSectHeader2.Attributes & EFI_GUIDED_SECTION_PROCESSING_REQUIRED) == 0) {
            HeaderSize = GuidSectHeader2.DataOffset;
          }
        } else {
          fread (&GuidSectHeader, 1, sizeof (GuidSectHeader), InFile);
          if ((GuidSectHeader.Attributes & EFI_GUIDED_SECTION_PROCESSING_REQUIRED) == 0) {
            HeaderSize = GuidSectHeader.DataOffset;
          }
        }
      }

      fseek (InFile, 0, SEEK_SET);

      //
      // Revert TeOffset to the converse value relative to Alignment
      // This is to assure the original PeImage Header at Alignment.
      //
      if (TeOffset != 0) {
        TeOffset = InputFileAlign [Index] - (TeOffset % InputFileAlign [Index]);
        TeOffset = TeOffset % InputFileAlign [Index];
      }

      //
      // make sure section data meet its alignment requirement by adding one raw pad section.
      //
      if ((InputFileAlign [Index] != 0) && (((Size + HeaderSize + TeOffset) % InputFileAlign [Index]) != 0)) {
        Offset = (Size + sizeof (EFI_COMMON_SECTION_HEADER) + HeaderSize + TeOffset + InputFileAlign [Index] - 1) & ~(InputFileAlign [Index] - 1);
        Offset = Offset - Size - HeaderSize - TeOffset;

        if (FileBuffer != NULL && ((Size + Offset) < *BufferLength)) {
          //
          // The maximal alignment is 64K, the raw section size must be less than 0xffffff
          //
          memset (FileBuffer + Size, 0, Offset);
          SectHeader          = (EFI_COMMON_SECTION_HEADER *) (FileBuffer + Size);
          SectHeader->Type    = EFI_SECTION_RAW;
          SectHeader->Size[0] = (UINT8) (Offset & 0xff);
          SectHeader->Size[1] = (UINT8) ((Offset & 0xff00) >> 8);
          SectHeader->Size[2] = (UINT8) ((Offset & 0xff0000) >> 16);
        }
        DebugMsg (NULL, 0, 9, "Pad raw section for section data alignment", "Pad Raw section size is %u", (unsigned) Offset);

        Size = Size + Offset;
      }
    }

    //
    // Now read the contents of the file into the buffer
    // Buffer must be enough to contain the file content.
    //
    if ((FileSize > 0) && (FileBuffer != NULL) && ((Size + FileSize) <= *BufferLength)) {
      if (fread (FileBuffer + Size, (size_t) FileSize, 1, InFile) != 1) {
        Error (NULL, 0, 0004, "Error reading file", InputFileName[Index]);
        fclose (InFile);
        return EFI_ABORTED;
      }
    }

    fclose (InFile);
    Size += FileSize;
  }

  //
  // Set the real required buffer size.
  //
  if (Size > *BufferLength) {
    *BufferLength = Size;
    return EFI_BUFFER_TOO_SMALL;
  } else {
    *BufferLength = Size;
    return EFI_SUCCESS;
  }
}

EFI_STATUS
GenSectionCompressionSection (
  CHAR8   **InputFileName,
  UINT32  *InputFileAlign,
  UINT32  InputFileNum,
  UINT8   SectCompSubType,
  UINT8   **OutFileBuffer
  )
/*++

Routine Description:

  Generate an encapsulating section of type EFI_SECTION_COMPRESSION
  Input file must be already sectioned. The function won't validate
  the input files' contents. Caller should hand in files already
  with section header.

Arguments:

  InputFileName  - Name of the input file.

  InputFileAlign - Alignment required by the input file data.

  InputFileNum   - Number of input files. Should be at least 1.

  SectCompSubType - Specify the compression algorithm requested.

  OutFileBuffer   - Buffer pointer to Output file contents

Returns:

  EFI_SUCCESS           on successful return
  EFI_INVALID_PARAMETER if InputFileNum is less than 1
  EFI_ABORTED           if unable to open input file.
  EFI_OUT_OF_RESOURCES  No resource to complete the operation.
--*/
{
  UINT32                  TotalLength;
  UINT32                  InputLength;
  UINT32                  CompressedLength;
  UINT32                  HeaderLength;
  UINT8                   *FileBuffer;
  UINT8                   *OutputBuffer;
  EFI_STATUS              Status;
  EFI_COMPRESSION_SECTION *CompressionSect;
  EFI_COMPRESSION_SECTION2 *CompressionSect2;
  COMPRESS_FUNCTION       CompressFunction;

  InputLength       = 0;
  FileBuffer        = NULL;
  OutputBuffer      = NULL;
  CompressedLength  = 0;
  TotalLength       = 0;
  //
  // read all input file contents into a buffer
  // first get the size of all file contents
  //
  Status = GetSectionContents (
            InputFileName,
            InputFileAlign,
            InputFileNum,
            FileBuffer,
            &InputLength
            );

  if (Status == EFI_BUFFER_TOO_SMALL) {
    FileBuffer = (UINT8 *) malloc (InputLength);
    if (FileBuffer == NULL) {
      Error (NULL, 0, 4001, "Resource", "memory cannot be allocated");
      return EFI_OUT_OF_RESOURCES;
    }
    //
    // read all input file contents into a buffer
    //
    Status = GetSectionContents (
              InputFileName,
              InputFileAlign,
              InputFileNum,
              FileBuffer,
              &InputLength
              );
  }

  if (EFI_ERROR (Status)) {
    if (FileBuffer != NULL) {
      free (FileBuffer);
    }
    return Status;
  }

  if (FileBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  CompressFunction = NULL;

  //
  // Now data is in FileBuffer, compress the data
  //
  switch (SectCompSubType) {
  case EFI_NOT_COMPRESSED:
    CompressedLength = InputLength;
    HeaderLength = sizeof (EFI_COMPRESSION_SECTION);
    if (CompressedLength + HeaderLength >= MAX_SECTION_SIZE) {
      HeaderLength = sizeof (EFI_COMPRESSION_SECTION2);
    }
    TotalLength = CompressedLength + HeaderLength;
    //
    // Copy file buffer to the none compressed data.
    //
    OutputBuffer = malloc (TotalLength);
    if (OutputBuffer == NULL) {
      free (FileBuffer);
      return EFI_OUT_OF_RESOURCES;
    }
    memcpy (OutputBuffer + HeaderLength, FileBuffer, CompressedLength);
    free (FileBuffer);
    FileBuffer = OutputBuffer;
    break;

  case EFI_STANDARD_COMPRESSION:
    CompressFunction = (COMPRESS_FUNCTION) EfiCompress;
    break;

  default:
    Error (NULL, 0, 2000, "Invalid parameter", "unknown compression type");
    free (FileBuffer);
    return EFI_ABORTED;
  }

  if (CompressFunction != NULL) {

    Status = CompressFunction (FileBuffer, InputLength, OutputBuffer, &CompressedLength);
    if (Status == EFI_BUFFER_TOO_SMALL) {
      HeaderLength = sizeof (EFI_COMPRESSION_SECTION);
      if (CompressedLength + HeaderLength >= MAX_SECTION_SIZE) {
        HeaderLength = sizeof (EFI_COMPRESSION_SECTION2);
      }
      TotalLength = CompressedLength + HeaderLength;
      OutputBuffer = malloc (TotalLength);
      if (!OutputBuffer) {
        free (FileBuffer);
        return EFI_OUT_OF_RESOURCES;
      }

      Status = CompressFunction (FileBuffer, InputLength, OutputBuffer + HeaderLength, &CompressedLength);
    }

    free (FileBuffer);
    FileBuffer = OutputBuffer;

    if (EFI_ERROR (Status)) {
      if (FileBuffer != NULL) {
        free (FileBuffer);
      }

      return Status;
    }

    if (FileBuffer == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
  }

  DebugMsg (NULL, 0, 9, "comprss file size",
            "the original section size is %d bytes and the compressed section size is %u bytes", (unsigned) InputLength, (unsigned) CompressedLength);

  //if (TotalLength >= MAX_SECTION_SIZE) {
  //  Error (NULL, 0, 2000, "Invalid parameter", "The size of all files exceeds section size limit(%uM).", MAX_SECTION_SIZE>>20);
  //  if (FileBuffer != NULL) {
  //    free (FileBuffer);
  //  }
  //  if (OutputBuffer != NULL) {
  //    free (OutputBuffer);
  //  }
  //  return STATUS_ERROR;
  //}
  VerboseMsg ("the size of the created section file is %u bytes", (unsigned) TotalLength);

  //
  // Add the section header for the compressed data
  //
  if (TotalLength >= MAX_SECTION_SIZE) {
    CompressionSect2 = (EFI_COMPRESSION_SECTION2 *)FileBuffer;

    memset(CompressionSect2->CommonHeader.Size, 0xff, sizeof(UINT8) * 3);
    CompressionSect2->CommonHeader.Type         = EFI_SECTION_COMPRESSION;
    CompressionSect2->CommonHeader.ExtendedSize = TotalLength;
    CompressionSect2->CompressionType           = SectCompSubType;
    CompressionSect2->UncompressedLength        = InputLength;
  } else {
    CompressionSect = (EFI_COMPRESSION_SECTION *) FileBuffer;

    CompressionSect->CommonHeader.Type     = EFI_SECTION_COMPRESSION;
    CompressionSect->CommonHeader.Size[0]  = (UINT8) (TotalLength & 0xff);
    CompressionSect->CommonHeader.Size[1]  = (UINT8) ((TotalLength & 0xff00) >> 8);
    CompressionSect->CommonHeader.Size[2]  = (UINT8) ((TotalLength & 0xff0000) >> 16);
    CompressionSect->CompressionType       = SectCompSubType;
    CompressionSect->UncompressedLength    = InputLength;
  }

  //
  // Set OutFileBuffer
  //
  *OutFileBuffer = FileBuffer;

  return EFI_SUCCESS;
}

EFI_STATUS
GenSectionGuidDefinedSection (
  CHAR8    **InputFileName,
  UINT32   *InputFileAlign,
  UINT32   InputFileNum,
  EFI_GUID *VendorGuid,
  UINT16   DataAttribute,
  UINT32   DataHeaderSize,
  UINT8    **OutFileBuffer
  )
/*++

Routine Description:

  Generate an encapsulating section of type EFI_SECTION_GUID_DEFINED
  Input file must be already sectioned. The function won't validate
  the input files' contents. Caller should hand in files already
  with section header.

Arguments:

  InputFileName - Name of the input file.

  InputFileAlign - Alignment required by the input file data.

  InputFileNum  - Number of input files. Should be at least 1.

  VendorGuid    - Specify vendor guid value.

  DataAttribute - Specify attribute for the vendor guid data.

  DataHeaderSize- Guided Data Header Size

  OutFileBuffer   - Buffer pointer to Output file contents

Returns:

  EFI_SUCCESS on successful return
  EFI_INVALID_PARAMETER if InputFileNum is less than 1
  EFI_ABORTED if unable to open input file.
  EFI_OUT_OF_RESOURCES  No resource to complete the operation.

--*/
{
  UINT32                TotalLength;
  UINT32                InputLength;
  UINT32                Offset;
  UINT8                 *FileBuffer;
  UINT32                Crc32Checksum;
  EFI_STATUS            Status;
  CRC32_SECTION_HEADER  *Crc32GuidSect;
  CRC32_SECTION_HEADER2  *Crc32GuidSect2;
  EFI_GUID_DEFINED_SECTION  *VendorGuidSect;
  EFI_GUID_DEFINED_SECTION2  *VendorGuidSect2;

  InputLength = 0;
  Offset      = 0;
  FileBuffer  = NULL;
  TotalLength = 0;

  //
  // read all input file contents into a buffer
  // first get the size of all file contents
  //
  Status = GetSectionContents (
            InputFileName,
            InputFileAlign,
            InputFileNum,
            FileBuffer,
            &InputLength
            );

  if (Status == EFI_BUFFER_TOO_SMALL) {
    if (CompareGuid (VendorGuid, &mZeroGuid) == 0) {
      Offset = sizeof (CRC32_SECTION_HEADER);
      if (InputLength + Offset >= MAX_SECTION_SIZE) {
        Offset = sizeof (CRC32_SECTION_HEADER2);
      }
    } else {
      Offset = sizeof (EFI_GUID_DEFINED_SECTION);
      if (InputLength + Offset >= MAX_SECTION_SIZE) {
        Offset = sizeof (EFI_GUID_DEFINED_SECTION2);
      }
    }
    TotalLength = InputLength + Offset;

    FileBuffer = (UINT8 *) malloc (InputLength + Offset);
    if (FileBuffer == NULL) {
      Error (NULL, 0, 4001, "Resource", "memory cannot be allocated");
      return EFI_OUT_OF_RESOURCES;
    }
    //
    // read all input file contents into a buffer
    //
    Status = GetSectionContents (
              InputFileName,
              InputFileAlign,
              InputFileNum,
              FileBuffer + Offset,
              &InputLength
              );
  }

  if (EFI_ERROR (Status)) {
    if (FileBuffer != NULL) {
      free (FileBuffer);
    }
    Error (NULL, 0, 0001, "Error opening file for reading", InputFileName[0]);
    return Status;
  }

  if (InputLength == 0) {
    if (FileBuffer != NULL) {
      free (FileBuffer);
    }
    Error (NULL, 0, 2000, "Invalid parameter", "the size of input file %s can't be zero", InputFileName);
    return EFI_NOT_FOUND;
  }

  //
  // InputLength != 0, but FileBuffer == NULL means out of resources.
  //
  if (FileBuffer == NULL) {
    Error (NULL, 0, 4001, "Resource", "memory cannot be allocated");
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Now data is in FileBuffer + Offset
  //
  if (CompareGuid (VendorGuid, &mZeroGuid) == 0) {
    //
    // Default Guid section is CRC32.
    //
    Crc32Checksum = 0;
    CalculateCrc32 (FileBuffer + Offset, InputLength, &Crc32Checksum);

    if (TotalLength >= MAX_SECTION_SIZE) {
      Crc32GuidSect2 = (CRC32_SECTION_HEADER2 *) FileBuffer;
      Crc32GuidSect2->GuidSectionHeader.CommonHeader.Type     = EFI_SECTION_GUID_DEFINED;
      Crc32GuidSect2->GuidSectionHeader.CommonHeader.Size[0]  = (UINT8) 0xff;
      Crc32GuidSect2->GuidSectionHeader.CommonHeader.Size[1]  = (UINT8) 0xff;
      Crc32GuidSect2->GuidSectionHeader.CommonHeader.Size[2]  = (UINT8) 0xff;
      Crc32GuidSect2->GuidSectionHeader.CommonHeader.ExtendedSize = TotalLength;
      memcpy (&(Crc32GuidSect2->GuidSectionHeader.SectionDefinitionGuid), &mEfiCrc32SectionGuid, sizeof (EFI_GUID));
      Crc32GuidSect2->GuidSectionHeader.Attributes  = EFI_GUIDED_SECTION_AUTH_STATUS_VALID;
      Crc32GuidSect2->GuidSectionHeader.DataOffset  = sizeof (CRC32_SECTION_HEADER2);
      Crc32GuidSect2->CRC32Checksum                 = Crc32Checksum;
      DebugMsg (NULL, 0, 9, "Guided section", "Data offset is %u", Crc32GuidSect2->GuidSectionHeader.DataOffset);
    } else {
      Crc32GuidSect = (CRC32_SECTION_HEADER *) FileBuffer;
      Crc32GuidSect->GuidSectionHeader.CommonHeader.Type     = EFI_SECTION_GUID_DEFINED;
      Crc32GuidSect->GuidSectionHeader.CommonHeader.Size[0]  = (UINT8) (TotalLength & 0xff);
      Crc32GuidSect->GuidSectionHeader.CommonHeader.Size[1]  = (UINT8) ((TotalLength & 0xff00) >> 8);
      Crc32GuidSect->GuidSectionHeader.CommonHeader.Size[2]  = (UINT8) ((TotalLength & 0xff0000) >> 16);
      memcpy (&(Crc32GuidSect->GuidSectionHeader.SectionDefinitionGuid), &mEfiCrc32SectionGuid, sizeof (EFI_GUID));
      Crc32GuidSect->GuidSectionHeader.Attributes  = EFI_GUIDED_SECTION_AUTH_STATUS_VALID;
      Crc32GuidSect->GuidSectionHeader.DataOffset  = sizeof (CRC32_SECTION_HEADER);
      Crc32GuidSect->CRC32Checksum                 = Crc32Checksum;
      DebugMsg (NULL, 0, 9, "Guided section", "Data offset is %u", Crc32GuidSect->GuidSectionHeader.DataOffset);
    }
  } else {
    if (TotalLength >= MAX_SECTION_SIZE) {
      VendorGuidSect2 = (EFI_GUID_DEFINED_SECTION2 *) FileBuffer;
      VendorGuidSect2->CommonHeader.Type     = EFI_SECTION_GUID_DEFINED;
      VendorGuidSect2->CommonHeader.Size[0]  = (UINT8) 0xff;
      VendorGuidSect2->CommonHeader.Size[1]  = (UINT8) 0xff;
      VendorGuidSect2->CommonHeader.Size[2]  = (UINT8) 0xff;
      VendorGuidSect2->CommonHeader.ExtendedSize = InputLength + sizeof (EFI_GUID_DEFINED_SECTION2);
      memcpy (&(VendorGuidSect2->SectionDefinitionGuid), VendorGuid, sizeof (EFI_GUID));
      VendorGuidSect2->Attributes  = DataAttribute;
      VendorGuidSect2->DataOffset  = (UINT16) (sizeof (EFI_GUID_DEFINED_SECTION2) + DataHeaderSize);
      DebugMsg (NULL, 0, 9, "Guided section", "Data offset is %u", VendorGuidSect2->DataOffset);
    } else {
      VendorGuidSect = (EFI_GUID_DEFINED_SECTION *) FileBuffer;
      VendorGuidSect->CommonHeader.Type     = EFI_SECTION_GUID_DEFINED;
      VendorGuidSect->CommonHeader.Size[0]  = (UINT8) (TotalLength & 0xff);
      VendorGuidSect->CommonHeader.Size[1]  = (UINT8) ((TotalLength & 0xff00) >> 8);
      VendorGuidSect->CommonHeader.Size[2]  = (UINT8) ((TotalLength & 0xff0000) >> 16);
      memcpy (&(VendorGuidSect->SectionDefinitionGuid), VendorGuid, sizeof (EFI_GUID));
      VendorGuidSect->Attributes  = DataAttribute;
      VendorGuidSect->DataOffset  = (UINT16) (sizeof (EFI_GUID_DEFINED_SECTION) + DataHeaderSize);
      DebugMsg (NULL, 0, 9, "Guided section", "Data offset is %u", VendorGuidSect->DataOffset);
    }
  }
  VerboseMsg ("the size of the created section file is %u bytes", (unsigned) TotalLength);

  //
  // Set OutFileBuffer
  //
  *OutFileBuffer = FileBuffer;

  return EFI_SUCCESS;
}

EFI_STATUS
FfsRebaseImageRead (
    IN      VOID    *FileHandle,
    IN      UINTN   FileOffset,
    IN OUT  UINT32  *ReadSize,
    OUT     VOID    *Buffer
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

STATIC
EFI_STATUS
GetAlignmentFromFile(char *InFile, UINT32 *Alignment)
  /*
    InFile is input file for getting alignment
    return the alignment
    */
{
  FILE                           *InFileHandle;
  UINT8                          *PeFileBuffer;
  UINTN                          PeFileSize;
  UINT32                         CurSecHdrSize;
  PE_COFF_LOADER_IMAGE_CONTEXT   ImageContext;
  EFI_COMMON_SECTION_HEADER      *CommonHeader;
  EFI_STATUS                     Status;

  InFileHandle        = NULL;
  PeFileBuffer        = NULL;
  *Alignment          = 0;

  memset (&ImageContext, 0, sizeof (ImageContext));

  InFileHandle = fopen(LongFilePath(InFile), "rb");
  if (InFileHandle == NULL){
    Error (NULL, 0, 0001, "Error opening file", InFile);
    return EFI_ABORTED;
  }
  PeFileSize = _filelength (fileno(InFileHandle));
  PeFileBuffer = (UINT8 *) malloc (PeFileSize);
  if (PeFileBuffer == NULL) {
    fclose (InFileHandle);
    Error(NULL, 0, 4001, "Resource", "memory cannot be allocated  of %s", InFileHandle);
    return EFI_OUT_OF_RESOURCES;
  }
  fread (PeFileBuffer, sizeof (UINT8), PeFileSize, InFileHandle);
  fclose (InFileHandle);
  CommonHeader = (EFI_COMMON_SECTION_HEADER *) PeFileBuffer;
  CurSecHdrSize = GetSectionHeaderLength(CommonHeader);
  ImageContext.Handle = (VOID *) ((UINTN)PeFileBuffer + CurSecHdrSize);
  ImageContext.ImageRead = (PE_COFF_LOADER_READ_FILE)FfsRebaseImageRead;
  Status               = PeCoffLoaderGetImageInfo(&ImageContext);
  if (EFI_ERROR (Status)) {
    Error (NULL, 0, 3000, "Invalid PeImage", "The input file is %s and return status is %x", InFile, (int) Status);
    return Status;
   }
  *Alignment = ImageContext.SectionAlignment;
  // Free the allocated memory resource
  if (PeFileBuffer != NULL) {
    free (PeFileBuffer);
    PeFileBuffer = NULL;
  }
  return EFI_SUCCESS;
}

int
main (
  int  argc,
  char *argv[]
  )
/*++

Routine Description:

  Main

Arguments:

  command line parameters

Returns:

  EFI_SUCCESS    Section header successfully generated and section concatenated.
  EFI_ABORTED    Could not generate the section
  EFI_OUT_OF_RESOURCES  No resource to complete the operation.

--*/
{
  UINT32                    Index;
  UINT32                    InputFileNum;
  FILE                      *OutFile;
  CHAR8                     **InputFileName;
  CHAR8                     *OutputFileName;
  CHAR8                     *SectionName;
  CHAR8                     *CompressionName;
  CHAR8                     *StringBuffer;
  EFI_GUID                  VendorGuid = mZeroGuid;
  int                       VersionNumber;
  UINT8                     SectType;
  UINT8                     SectCompSubType;
  UINT16                    SectGuidAttribute;
  UINT64                    SectGuidHeaderLength;
  EFI_VERSION_SECTION       *VersionSect;
  EFI_USER_INTERFACE_SECTION *UiSect;
  UINT32                    InputLength;
  UINT8                     *OutFileBuffer;
  EFI_STATUS                Status;
  UINT64                    LogLevel;
  UINT32                    *InputFileAlign;
  UINT32                    InputFileAlignNum;
  EFI_COMMON_SECTION_HEADER *SectionHeader;
  CHAR8                     *DummyFileName;
  FILE                      *DummyFile;
  UINTN                     DummyFileSize;
  UINT8                     *DummyFileBuffer;
  FILE                      *InFile;
  UINT8                     *InFileBuffer;
  UINTN                     InFileSize;

  InputFileAlign        = NULL;
  InputFileAlignNum     = 0;
  InputFileName         = NULL;
  OutputFileName        = NULL;
  SectionName           = NULL;
  CompressionName       = NULL;
  StringBuffer          = "";
  OutFile               = NULL;
  VersionNumber         = 0;
  InputFileNum          = 0;
  SectType              = EFI_SECTION_ALL;
  SectCompSubType       = 0;
  SectGuidAttribute     = EFI_GUIDED_SECTION_NONE;
  OutFileBuffer         = NULL;
  InputLength           = 0;
  Status                = STATUS_SUCCESS;
  LogLevel              = 0;
  SectGuidHeaderLength  = 0;
  VersionSect           = NULL;
  UiSect                = NULL;
  DummyFileSize         = 0;
  DummyFileName         = NULL;
  DummyFile             = NULL;
  DummyFileBuffer       = NULL;
  InFile                = NULL;
  InFileSize            = 0;
  InFileBuffer          = NULL;

  SetUtilityName (UTILITY_NAME);

  if (argc == 1) {
    Error (NULL, 0, 1001, "Missing options", "No options input");
    Usage ();
    return STATUS_ERROR;
  }

  //
  // Parse command line
  //
  argc --;
  argv ++;

  if ((stricmp (argv[0], "-h") == 0) || (stricmp (argv[0], "--help") == 0)) {
    Version ();
    Usage ();
    return STATUS_SUCCESS;
  }

  if (stricmp (argv[0], "--version") == 0) {
    Version ();
    return STATUS_SUCCESS;
  }

  while (argc > 0) {
    if ((stricmp (argv[0], "-s") == 0) || (stricmp (argv[0], "--SectionType") == 0)) {
      SectionName = argv[1];
      if (SectionName == NULL) {
        Error (NULL, 0, 1003, "Invalid option value", "Section Type can't be NULL");
        goto Finish;
      }
      argc -= 2;
      argv += 2;
      continue;
    }

    if ((stricmp (argv[0], "-o") == 0) || (stricmp (argv[0], "--outputfile") == 0)) {
      OutputFileName = argv[1];
      if (OutputFileName == NULL) {
        Error (NULL, 0, 1003, "Invalid option value", "Output file can't be NULL");
        goto Finish;
      }
      argc -= 2;
      argv += 2;
      continue;
    }

    if ((stricmp (argv[0], "-c") == 0) || (stricmp (argv[0], "--compress") == 0)) {
      CompressionName = argv[1];
      if (CompressionName == NULL) {
        Error (NULL, 0, 1003, "Invalid option value", "Compression Type can't be NULL");
        goto Finish;
      }
      argc -= 2;
      argv += 2;
      continue;
    }

    if ((stricmp (argv[0], "-g") == 0) || (stricmp (argv[0], "--vendor") == 0)) {
      Status = StringToGuid (argv[1], &VendorGuid);
      if (EFI_ERROR (Status)) {
        Error (NULL, 0, 1003, "Invalid option value", "%s = %s", argv[0], argv[1]);
        goto Finish;
      }
      argc -= 2;
      argv += 2;
      continue;
    }
    if (stricmp (argv[0], "--dummy") == 0) {
      DummyFileName = argv[1];
      if (DummyFileName == NULL) {
        Error (NULL, 0, 1003, "Invalid option value", "Dummy file can't be NULL");
        goto Finish;
      }
      argc -= 2;
      argv += 2;
      continue;
    }

    if ((stricmp (argv[0], "-r") == 0) || (stricmp (argv[0], "--attributes") == 0)) {
      if (argv[1] == NULL) {
        Error (NULL, 0, 1003, "Invalid option value", "Guid section attributes can't be NULL");
        goto Finish;
      }
      if (stricmp (argv[1], mGUIDedSectionAttribue[EFI_GUIDED_SECTION_PROCESSING_REQUIRED]) == 0) {
        SectGuidAttribute |= EFI_GUIDED_SECTION_PROCESSING_REQUIRED;
      } else if (stricmp (argv[1], mGUIDedSectionAttribue[EFI_GUIDED_SECTION_AUTH_STATUS_VALID]) == 0) {
        SectGuidAttribute |= EFI_GUIDED_SECTION_AUTH_STATUS_VALID;
      } else if (stricmp (argv[1], mGUIDedSectionAttribue[0]) == 0) {
        //
        // NONE attribute
        //
        SectGuidAttribute |= EFI_GUIDED_SECTION_NONE;
      } else {
        Error (NULL, 0, 1003, "Invalid option value", "%s = %s", argv[0], argv[1]);
        goto Finish;
      }
      argc -= 2;
      argv += 2;
      continue;
    }

    if ((stricmp (argv[0], "-l") == 0) || (stricmp (argv[0], "--HeaderLength") == 0)) {
      Status = AsciiStringToUint64 (argv[1], FALSE, &SectGuidHeaderLength);
      if (EFI_ERROR (Status)) {
        Error (NULL, 0, 1003, "Invalid option value for GuidHeaderLength", "%s = %s", argv[0], argv[1]);
        goto Finish;
      }
      argc -= 2;
      argv += 2;
      continue;
    }

    if ((stricmp (argv[0], "-n") == 0) || (stricmp (argv[0], "--name") == 0)) {
      StringBuffer = argv[1];
      if (StringBuffer == NULL) {
        Error (NULL, 0, 1003, "Invalid option value", "Name can't be NULL");
        goto Finish;
      }
      argc -= 2;
      argv += 2;
      continue;
    }

    if ((stricmp (argv[0], "-j") == 0) || (stricmp (argv[0], "--buildnumber") == 0)) {
      if (argv[1] == NULL) {
        Error (NULL, 0, 1003, "Invalid option value", "build number can't be NULL");
        goto Finish;
      }
      //
      // Verify string is a integrator number
      //
      for (Index = 0; Index < strlen (argv[1]); Index++) {
        if ((argv[1][Index] != '-') && (isdigit ((int)argv[1][Index]) == 0)) {
          Error (NULL, 0, 1003, "Invalid option value", "%s = %s", argv[0], argv[1]);
          goto Finish;
        }
      }

      sscanf (argv[1], "%d", &VersionNumber);
      argc -= 2;
      argv += 2;
      continue;
    }

    if ((stricmp (argv[0], "-v") == 0) || (stricmp (argv[0], "--verbose") == 0)) {
      SetPrintLevel (VERBOSE_LOG_LEVEL);
      VerboseMsg ("Verbose output Mode Set!");
      argc --;
      argv ++;
      continue;
    }

    if ((stricmp (argv[0], "-q") == 0) || (stricmp (argv[0], "--quiet") == 0)) {
      SetPrintLevel (KEY_LOG_LEVEL);
      KeyMsg ("Quiet output Mode Set!");
      argc --;
      argv ++;
      continue;
    }

    if ((stricmp (argv[0], "-d") == 0) || (stricmp (argv[0], "--debug") == 0)) {
      Status = AsciiStringToUint64 (argv[1], FALSE, &LogLevel);
      if (EFI_ERROR (Status)) {
        Error (NULL, 0, 1003, "Invalid option value", "%s = %s", argv[0], argv[1]);
        goto Finish;
      }
      if (LogLevel > 9) {
        Error (NULL, 0, 1003, "Invalid option value", "Debug Level range is 0~9, current input level is %d", (int) LogLevel);
        goto Finish;
      }
      SetPrintLevel (LogLevel);
      DebugMsg (NULL, 0, 9, "Debug Mode Set", "Debug Output Mode Level %s is set!", argv[1]);
      argc -= 2;
      argv += 2;
      continue;
    }

    //
    // Section File alignment requirement
    //
    if (stricmp (argv[0], "--sectionalign") == 0) {
      if (InputFileAlignNum == 0) {
        InputFileAlign = (UINT32 *) malloc (MAXIMUM_INPUT_FILE_NUM * sizeof (UINT32));
        if (InputFileAlign == NULL) {
          Error (NULL, 0, 4001, "Resource", "memory cannot be allocated!");
          goto Finish;
        }
        memset (InputFileAlign, 1, MAXIMUM_INPUT_FILE_NUM * sizeof (UINT32));
      } else if (InputFileAlignNum % MAXIMUM_INPUT_FILE_NUM == 0) {
        InputFileAlign = (UINT32 *) realloc (
          InputFileAlign,
          (InputFileNum + MAXIMUM_INPUT_FILE_NUM) * sizeof (UINT32)
          );

        if (InputFileAlign == NULL) {
          Error (NULL, 0, 4001, "Resource", "memory cannot be allocated!");
          goto Finish;
        }
        memset (&(InputFileAlign[InputFileNum]), 1, (MAXIMUM_INPUT_FILE_NUM * sizeof (UINT32)));
      }
      if (stricmp(argv[1], "0") == 0) {
        InputFileAlign[InputFileAlignNum] = 0;
      } else {
        Status = StringtoAlignment (argv[1], &(InputFileAlign[InputFileAlignNum]));
        if (EFI_ERROR (Status)) {
          Error (NULL, 0, 1003, "Invalid option value", "%s = %s", argv[0], argv[1]);
          goto Finish;
        }
      }
      argc -= 2;
      argv += 2;
      InputFileAlignNum ++;
      continue;
    }

    //
    // Get Input file name
    //
    if ((InputFileNum == 0) && (InputFileName == NULL)) {
      InputFileName = (CHAR8 **) malloc (MAXIMUM_INPUT_FILE_NUM * sizeof (CHAR8 *));
      if (InputFileName == NULL) {
        Error (NULL, 0, 4001, "Resource", "memory cannot be allocated");
        goto Finish;
      }
      memset (InputFileName, 0, (MAXIMUM_INPUT_FILE_NUM * sizeof (CHAR8 *)));
    } else if (InputFileNum % MAXIMUM_INPUT_FILE_NUM == 0) {
      //
      // InputFileName buffer too small, need to realloc
      //
      InputFileName = (CHAR8 **) realloc (
                                  InputFileName,
                                  (InputFileNum + MAXIMUM_INPUT_FILE_NUM) * sizeof (CHAR8 *)
                                  );

      if (InputFileName == NULL) {
        Error (NULL, 0, 4001, "Resource", "memory cannot be allocated");
        goto Finish;
      }
      memset (&(InputFileName[InputFileNum]), 0, (MAXIMUM_INPUT_FILE_NUM * sizeof (CHAR8 *)));
    }

    InputFileName[InputFileNum++] = argv[0];
    argc --;
    argv ++;
  }

  if (InputFileAlignNum > 0 && InputFileAlignNum != InputFileNum) {
    Error (NULL, 0, 1003, "Invalid option", "section alignment must be set for each section");
    goto Finish;
  }
  for (Index = 0; Index < InputFileAlignNum; Index++)
  {
    if (InputFileAlign[Index] == 0) {
      Status = GetAlignmentFromFile(InputFileName[Index], &(InputFileAlign[Index]));
      if (EFI_ERROR(Status)) {
        Error (NULL, 0, 1003, "Fail to get Alignment from %s", InputFileName[InputFileNum]);
        goto Finish;
      }
    }
  }

  VerboseMsg ("%s tool start.", UTILITY_NAME);

  if (DummyFileName != NULL) {
      //
      // Open file and read contents
      //
      DummyFile = fopen (LongFilePath (DummyFileName), "rb");
      if (DummyFile == NULL) {
        Error (NULL, 0, 0001, "Error opening file", DummyFileName);
        goto Finish;
      }

      fseek (DummyFile, 0, SEEK_END);
      DummyFileSize = ftell (DummyFile);
      fseek (DummyFile, 0, SEEK_SET);
      DummyFileBuffer = (UINT8 *) malloc (DummyFileSize);
      if (DummyFileBuffer == NULL) {
        fclose(DummyFile);
        Error (NULL, 0, 4001, "Resource", "memory cannot be allocated");
        goto Finish;
      }

      fread(DummyFileBuffer, 1, DummyFileSize, DummyFile);
      fclose(DummyFile);
      DebugMsg (NULL, 0, 9, "Dummy files", "the dummy file name is %s and the size is %u bytes", DummyFileName, (unsigned) DummyFileSize);

      if (InputFileName == NULL) {
        Error (NULL, 0, 4001, "Resource", "memory cannot be allocated");
        goto Finish;
      }
      InFile = fopen(LongFilePath(InputFileName[0]), "rb");
      if (InFile == NULL) {
        Error (NULL, 0, 0001, "Error opening file", InputFileName[0]);
        goto Finish;
      }

      fseek (InFile, 0, SEEK_END);
      InFileSize = ftell (InFile);
      fseek (InFile, 0, SEEK_SET);
      InFileBuffer = (UINT8 *) malloc (InFileSize);
      if (InFileBuffer == NULL) {
        fclose(InFile);
        Error (NULL, 0, 4001, "Resource", "memory cannot be allocated");
        goto Finish;
      }

      fread(InFileBuffer, 1, InFileSize, InFile);
      fclose(InFile);
      DebugMsg (NULL, 0, 9, "Input files", "the input file name is %s and the size is %u bytes", InputFileName[0], (unsigned) InFileSize);
      if (InFileSize > DummyFileSize){
        if (stricmp((CHAR8 *)DummyFileBuffer, (CHAR8 *)(InFileBuffer + (InFileSize - DummyFileSize))) == 0){
          SectGuidHeaderLength = InFileSize - DummyFileSize;
        }
      }
      if (SectGuidHeaderLength == 0) {
        SectGuidAttribute |= EFI_GUIDED_SECTION_PROCESSING_REQUIRED;
      }
      if (DummyFileBuffer != NULL) {
        free (DummyFileBuffer);
        DummyFileBuffer = NULL;
      }
      if (InFileBuffer != NULL) {
        free (InFileBuffer);
      }
    }

  //
  // Parse all command line parameters to get the corresponding section type.
  //
  VerboseMsg ("Section type is %s", SectionName);
  if (SectionName == NULL) {
    //
    // No specified Section type, default is SECTION_ALL.
    //
    SectType = EFI_SECTION_ALL;
  } else if (stricmp (SectionName, mSectionTypeName[EFI_SECTION_COMPRESSION]) == 0) {
    SectType     = EFI_SECTION_COMPRESSION;
    if (CompressionName == NULL) {
      //
      // Default is PI_STD compression algorithm.
      //
      SectCompSubType = EFI_STANDARD_COMPRESSION;
    } else if (stricmp (CompressionName, mCompressionTypeName[EFI_NOT_COMPRESSED]) == 0) {
      SectCompSubType = EFI_NOT_COMPRESSED;
    } else if (stricmp (CompressionName, mCompressionTypeName[EFI_STANDARD_COMPRESSION]) == 0) {
      SectCompSubType = EFI_STANDARD_COMPRESSION;
    } else {
      Error (NULL, 0, 1003, "Invalid option value", "--compress = %s", CompressionName);
      goto Finish;
    }
    VerboseMsg ("Compress method is %s", mCompressionTypeName [SectCompSubType]);
  } else if (stricmp (SectionName, mSectionTypeName[EFI_SECTION_GUID_DEFINED]) == 0) {
    SectType     = EFI_SECTION_GUID_DEFINED;

    if ((SectGuidAttribute & EFI_GUIDED_SECTION_NONE) != 0) {
      //
      // NONE attribute, clear attribute value.
      //
      SectGuidAttribute = SectGuidAttribute & ~EFI_GUIDED_SECTION_NONE;
    }
    VerboseMsg ("Vendor Guid is %08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
                (unsigned) VendorGuid.Data1,
                VendorGuid.Data2,
                VendorGuid.Data3,
                VendorGuid.Data4[0],
                VendorGuid.Data4[1],
                VendorGuid.Data4[2],
                VendorGuid.Data4[3],
                VendorGuid.Data4[4],
                VendorGuid.Data4[5],
                VendorGuid.Data4[6],
                VendorGuid.Data4[7]);
    if ((SectGuidAttribute & EFI_GUIDED_SECTION_PROCESSING_REQUIRED) != 0) {
      VerboseMsg ("Guid Attribute is %s", mGUIDedSectionAttribue[EFI_GUIDED_SECTION_PROCESSING_REQUIRED]);
    }
    if ((SectGuidAttribute & EFI_GUIDED_SECTION_AUTH_STATUS_VALID) != 0) {
      VerboseMsg ("Guid Attribute is %s", mGUIDedSectionAttribue[EFI_GUIDED_SECTION_AUTH_STATUS_VALID]);
    }
    if (SectGuidHeaderLength != 0) {
      VerboseMsg ("Guid Data Header size is 0x%llx", (unsigned long long) SectGuidHeaderLength);
    }
  } else if (stricmp (SectionName, mSectionTypeName[EFI_SECTION_PE32]) == 0) {
    SectType = EFI_SECTION_PE32;
  } else if (stricmp (SectionName, mSectionTypeName[EFI_SECTION_PIC]) == 0) {
    SectType = EFI_SECTION_PIC;
  } else if (stricmp (SectionName, mSectionTypeName[EFI_SECTION_TE]) == 0) {
    SectType = EFI_SECTION_TE;
  } else if (stricmp (SectionName, mSectionTypeName[EFI_SECTION_DXE_DEPEX]) == 0) {
    SectType = EFI_SECTION_DXE_DEPEX;
  } else if (stricmp (SectionName, mSectionTypeName[EFI_SECTION_SMM_DEPEX]) == 0) {
    SectType = EFI_SECTION_SMM_DEPEX;
  } else if (stricmp (SectionName, mSectionTypeName[EFI_SECTION_VERSION]) == 0) {
    SectType = EFI_SECTION_VERSION;
    if (VersionNumber < 0 || VersionNumber > 65535) {
      Error (NULL, 0, 1003, "Invalid option value", "%d is not in 0~65535", VersionNumber);
      goto Finish;
    }
    VerboseMsg ("Version section number is %d", VersionNumber);
  } else if (stricmp (SectionName, mSectionTypeName[EFI_SECTION_USER_INTERFACE]) == 0) {
    SectType = EFI_SECTION_USER_INTERFACE;
    if (StringBuffer[0] == '\0') {
      Error (NULL, 0, 1001, "Missing option", "user interface string");
      goto Finish;
    }
    VerboseMsg ("UI section string name is %s", StringBuffer);
  } else if (stricmp (SectionName, mSectionTypeName[EFI_SECTION_COMPATIBILITY16]) == 0) {
    SectType = EFI_SECTION_COMPATIBILITY16;
  } else if (stricmp (SectionName, mSectionTypeName[EFI_SECTION_FIRMWARE_VOLUME_IMAGE]) == 0) {
    SectType = EFI_SECTION_FIRMWARE_VOLUME_IMAGE;
  } else if (stricmp (SectionName, mSectionTypeName[EFI_SECTION_FREEFORM_SUBTYPE_GUID]) == 0) {
    SectType = EFI_SECTION_FREEFORM_SUBTYPE_GUID;
  } else if (stricmp (SectionName, mSectionTypeName[EFI_SECTION_RAW]) == 0) {
    SectType = EFI_SECTION_RAW;
  } else if (stricmp (SectionName, mSectionTypeName[EFI_SECTION_PEI_DEPEX]) == 0) {
    SectType = EFI_SECTION_PEI_DEPEX;
  } else {
    Error (NULL, 0, 1003, "Invalid option value", "SectionType = %s", SectionName);
    goto Finish;
  }

  //
  // GuidValue is only required by Guided section.
  //
  if ((SectType != EFI_SECTION_GUID_DEFINED) &&
    (SectionName != NULL) &&
    (CompareGuid (&VendorGuid, &mZeroGuid) != 0)) {
    fprintf (stdout, "Warning: the input guid value is not required for this section type %s\n", SectionName);
  }

  //
  // Check whether there is input file
  //
  if ((SectType != EFI_SECTION_VERSION) && (SectType != EFI_SECTION_USER_INTERFACE)) {
    //
    // The input file are required for other section type.
    //
    if (InputFileNum == 0) {
      Error (NULL, 0, 1001, "Missing options", "Input files");
      goto Finish;
    }
  }
  //
  // Check whether there is output file
  //
  for (Index = 0; Index < InputFileNum; Index ++) {
    VerboseMsg ("the %uth input file name is %s", (unsigned) Index, InputFileName[Index]);
  }
  if (OutputFileName == NULL) {
    Error (NULL, 0, 1001, "Missing options", "Output file");
    goto Finish;
    // OutFile = stdout;
  }
  VerboseMsg ("Output file name is %s", OutputFileName);

  //
  // At this point, we've fully validated the command line, and opened appropriate
  // files, so let's go and do what we've been asked to do...
  //
  //
  // Within this switch, build and write out the section header including any
  // section type specific pieces.  If there's an input file, it's tacked on later
  //
  switch (SectType) {
  case EFI_SECTION_COMPRESSION:
    if (InputFileAlign != NULL) {
      free (InputFileAlign);
      InputFileAlign = NULL;
    }
    Status = GenSectionCompressionSection (
              InputFileName,
              InputFileAlign,
              InputFileNum,
              SectCompSubType,
              &OutFileBuffer
              );
    break;

  case EFI_SECTION_GUID_DEFINED:
    if (InputFileAlign != NULL && (CompareGuid (&VendorGuid, &mZeroGuid) != 0)) {
      //
      // Only process alignment for the default known CRC32 guided section.
      // For the unknown guided section, the alignment is processed when the dummy all section (EFI_SECTION_ALL) is generated.
      //
      free (InputFileAlign);
      InputFileAlign = NULL;
    }
    Status = GenSectionGuidDefinedSection (
              InputFileName,
              InputFileAlign,
              InputFileNum,
              &VendorGuid,
              SectGuidAttribute,
              (UINT32) SectGuidHeaderLength,
              &OutFileBuffer
              );
    break;

  case EFI_SECTION_VERSION:
    Index           = sizeof (EFI_COMMON_SECTION_HEADER);
    //
    // 2 bytes for the build number UINT16
    //
    Index += 2;
    //
    // StringBuffer is ascii.. unicode is 2X + 2 bytes for terminating unicode null.
    //
    Index += (strlen (StringBuffer) * 2) + 2;
    OutFileBuffer = (UINT8 *) malloc (Index);
    if (OutFileBuffer == NULL) {
      Error (NULL, 0, 4001, "Resource", "memory cannot be allocated");
      goto Finish;
    }
    VersionSect = (EFI_VERSION_SECTION *) OutFileBuffer;
    VersionSect->CommonHeader.Type     = SectType;
    VersionSect->CommonHeader.Size[0]  = (UINT8) (Index & 0xff);
    VersionSect->CommonHeader.Size[1]  = (UINT8) ((Index & 0xff00) >> 8);
    VersionSect->CommonHeader.Size[2]  = (UINT8) ((Index & 0xff0000) >> 16);
    VersionSect->BuildNumber           = (UINT16) VersionNumber;
    Ascii2UnicodeString (StringBuffer, VersionSect->VersionString);
    VerboseMsg ("the size of the created section file is %u bytes", (unsigned) Index);
    break;

  case EFI_SECTION_USER_INTERFACE:
    Index           = sizeof (EFI_COMMON_SECTION_HEADER);
    //
    // StringBuffer is ascii.. unicode is 2X + 2 bytes for terminating unicode null.
    //
    Index += (strlen (StringBuffer) * 2) + 2;
    OutFileBuffer = (UINT8 *) malloc (Index);
    if (OutFileBuffer == NULL) {
      Error (NULL, 0, 4001, "Resource", "memory cannot be allocated");
      goto Finish;
    }
    UiSect = (EFI_USER_INTERFACE_SECTION *) OutFileBuffer;
    UiSect->CommonHeader.Type     = SectType;
    UiSect->CommonHeader.Size[0]  = (UINT8) (Index & 0xff);
    UiSect->CommonHeader.Size[1]  = (UINT8) ((Index & 0xff00) >> 8);
    UiSect->CommonHeader.Size[2]  = (UINT8) ((Index & 0xff0000) >> 16);
    Ascii2UnicodeString (StringBuffer, UiSect->FileNameString);
    VerboseMsg ("the size of the created section file is %u bytes", (unsigned) Index);
   break;

  case EFI_SECTION_ALL:
    //
    // read all input file contents into a buffer
    // first get the size of all file contents
    //
    Status = GetSectionContents (
              InputFileName,
              InputFileAlign,
              InputFileNum,
              OutFileBuffer,
              &InputLength
              );

    if (Status == EFI_BUFFER_TOO_SMALL) {
      OutFileBuffer = (UINT8 *) malloc (InputLength);
      if (OutFileBuffer == NULL) {
        Error (NULL, 0, 4001, "Resource", "memory cannot be allocated");
        goto Finish;
      }
      //
      // read all input file contents into a buffer
      //
      Status = GetSectionContents (
                InputFileName,
                InputFileAlign,
                InputFileNum,
                OutFileBuffer,
                &InputLength
                );
    }
    VerboseMsg ("the size of the created section file is %u bytes", (unsigned) InputLength);
    break;
  default:
    //
    // All other section types are caught by default (they're all the same)
    //
    Status = GenSectionCommonLeafSection (
              InputFileName,
              InputFileNum,
              SectType,
              &OutFileBuffer
              );
    break;
  }

  if (Status != EFI_SUCCESS || OutFileBuffer == NULL) {
    Error (NULL, 0, 2000, "Status is not successful", "Status value is 0x%X", (int) Status);
    goto Finish;
  }

  //
  // Get output file length
  //
  if (SectType != EFI_SECTION_ALL) {
    SectionHeader = (EFI_COMMON_SECTION_HEADER *)OutFileBuffer;
    InputLength = *(UINT32 *)SectionHeader->Size & 0x00ffffff;
    if (InputLength == 0xffffff) {
      InputLength = ((EFI_COMMON_SECTION_HEADER2 *)SectionHeader)->ExtendedSize;
    }
  }

  //
  // Write the output file
  //
  OutFile = fopen (LongFilePath (OutputFileName), "wb");
  if (OutFile == NULL) {
    Error (NULL, 0, 0001, "Error opening file for writing", OutputFileName);
    goto Finish;
  }

  fwrite (OutFileBuffer, InputLength, 1, OutFile);

Finish:
  if (InputFileName != NULL) {
    free (InputFileName);
  }

  if (InputFileAlign != NULL) {
    free (InputFileAlign);
  }

  if (OutFileBuffer != NULL) {
    free (OutFileBuffer);
  }

  if (OutFile != NULL) {
    fclose (OutFile);
  }

  if (DummyFileBuffer != NULL) {
    free (DummyFileBuffer);
  }

  VerboseMsg ("%s tool done with return code is 0x%x.", UTILITY_NAME, GetUtilityStatus ());

  return GetUtilityStatus ();
}
