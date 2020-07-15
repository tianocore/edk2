/** @file
This file contains functions required to generate a Firmware File System file.

Copyright (c) 2004 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __GNUC__
#include <windows.h>
#include <io.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif

#ifdef __GNUC__
#include <unistd.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Common/UefiBaseTypes.h>
#include <Common/PiFirmwareFile.h>
#include <IndustryStandard/PeImage.h>
#include <Guid/FfsSectionAlignmentPadding.h>

#include "CommonLib.h"
#include "ParseInf.h"
#include "EfiUtilityMsgs.h"
#include "FvLib.h"
#include "PeCoffLib.h"

#define UTILITY_NAME            "GenFfs"
#define UTILITY_MAJOR_VERSION   0
#define UTILITY_MINOR_VERSION   1

STATIC CHAR8 *mFfsFileType[] = {
  NULL,                                   // 0x00
  "EFI_FV_FILETYPE_RAW",                  // 0x01
  "EFI_FV_FILETYPE_FREEFORM",             // 0x02
  "EFI_FV_FILETYPE_SECURITY_CORE",        // 0x03
  "EFI_FV_FILETYPE_PEI_CORE",             // 0x04
  "EFI_FV_FILETYPE_DXE_CORE",             // 0x05
  "EFI_FV_FILETYPE_PEIM",                 // 0x06
  "EFI_FV_FILETYPE_DRIVER",               // 0x07
  "EFI_FV_FILETYPE_COMBINED_PEIM_DRIVER", // 0x08
  "EFI_FV_FILETYPE_APPLICATION",          // 0x09
  "EFI_FV_FILETYPE_SMM",                  // 0x0A
  "EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE",// 0x0B
  "EFI_FV_FILETYPE_COMBINED_SMM_DXE",     // 0x0C
  "EFI_FV_FILETYPE_SMM_CORE",             // 0x0D
  "EFI_FV_FILETYPE_MM_STANDALONE",        // 0x0E
  "EFI_FV_FILETYPE_MM_CORE_STANDALONE"    // 0x0F
};

STATIC CHAR8 *mAlignName[] = {
  "1", "2", "4", "8", "16", "32", "64", "128", "256", "512",
  "1K", "2K", "4K", "8K", "16K", "32K", "64K", "128K", "256K",
  "512K", "1M", "2M", "4M", "8M", "16M"
 };

STATIC CHAR8 *mFfsValidAlignName[] = {
  "8", "16", "128", "512", "1K", "4K", "32K", "64K", "128K","256K",
  "512K", "1M", "2M", "4M", "8M", "16M"
 };

STATIC UINT32 mFfsValidAlign[] = {0, 8, 16, 128, 512, 1024, 4096, 32768, 65536, 131072, 262144,
                                  524288, 1048576, 2097152, 4194304, 8388608, 16777216};

STATIC EFI_GUID mZeroGuid = {0};

STATIC EFI_GUID mEfiFfsSectionAlignmentPaddingGuid = EFI_FFS_SECTION_ALIGNMENT_PADDING_GUID;

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

  Print Error / Help message.

Arguments:

  VOID

Returns:

  None

--*/
{
  //
  // Summary usage
  //
  fprintf (stdout, "\nUsage: %s [options]\n\n", UTILITY_NAME);

  //
  // Copyright declaration
  //
  fprintf (stdout, "Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.\n\n");

  //
  // Details Option
  //
  fprintf (stdout, "Options:\n");
  fprintf (stdout, "  -o FileName, --outputfile FileName\n\
                        File is FFS file to be created.\n");
  fprintf (stdout, "  -t Type, --filetype Type\n\
                        Type is one FV file type defined in PI spec, which is\n\
                        EFI_FV_FILETYPE_RAW, EFI_FV_FILETYPE_FREEFORM,\n\
                        EFI_FV_FILETYPE_SECURITY_CORE, EFI_FV_FILETYPE_PEIM,\n\
                        EFI_FV_FILETYPE_PEI_CORE, EFI_FV_FILETYPE_DXE_CORE,\n\
                        EFI_FV_FILETYPE_DRIVER, EFI_FV_FILETYPE_APPLICATION,\n\
                        EFI_FV_FILETYPE_COMBINED_PEIM_DRIVER,\n\
                        EFI_FV_FILETYPE_SMM, EFI_FV_FILETYPE_SMM_CORE,\n\
                        EFI_FV_FILETYPE_MM_STANDALONE,\n\
                        EFI_FV_FILETYPE_MM_CORE_STANDALONE,\n\
                        EFI_FV_FILETYPE_COMBINED_SMM_DXE, \n\
                        EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE.\n");
  fprintf (stdout, "  -g FileGuid, --fileguid FileGuid\n\
                        FileGuid is one module guid.\n\
                        Its format is xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx\n");
  fprintf (stdout, "  -x, --fixed           Indicates that the file may not be moved\n\
                        from its present location.\n");
  fprintf (stdout, "  -s, --checksum        Indicates to calculate file checksum.\n");
  fprintf (stdout, "  -a FileAlign, --align FileAlign\n\
                        FileAlign points to file alignment, which only support\n\
                        the following align: 1,2,4,8,16,128,512,1K,4K,32K,64K\n\
                        128K,256K,512K,1M,2M,4M,8M,16M\n");
  fprintf (stdout, "  -i SectionFile, --sectionfile SectionFile\n\
                        Section file will be contained in this FFS file.\n");
  fprintf (stdout, "  -oi SectionFile, --optionalsectionfile SectionFile\n\
                        If the Section file exists, it will be contained in this FFS file, otherwise, it will be ignored.\n");
  fprintf (stdout, "  -n SectionAlign, --sectionalign SectionAlign\n\
                        SectionAlign points to section alignment, which support\n\
                        the alignment scope 0~16M. If SectionAlign is specified\n\
                        as 0, tool get alignment value from SectionFile. It is\n\
                        specified together with sectionfile to point its\n\
                        alignment in FFS file.\n");
  fprintf (stdout, "  -v, --verbose         Turn on verbose output with informational messages.\n");
  fprintf (stdout, "  -q, --quiet           Disable all messages except key message and fatal error\n");
  fprintf (stdout, "  -d, --debug level     Enable debug messages, at input debug level.\n");
  fprintf (stdout, "  --version             Show program's version number and exit.\n");
  fprintf (stdout, "  -h, --help            Show this help message and exit.\n");
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

STATIC
UINT8
StringToType (
  IN CHAR8 *String
  )
/*++

Routine Description:

  Converts File Type String to value.  EFI_FV_FILETYPE_ALL indicates that an
  unrecognized file type was specified.

Arguments:

  String    - File type string

Returns:

  File Type Value

--*/
{
  UINT8 Index = 0;

  if (String == NULL) {
    return EFI_FV_FILETYPE_ALL;
  }

  for (Index = 0; Index < sizeof (mFfsFileType) / sizeof (CHAR8 *); Index ++) {
    if (mFfsFileType [Index] != NULL && (stricmp (String, mFfsFileType [Index]) == 0)) {
      return Index;
    }
  }
  return EFI_FV_FILETYPE_ALL;
}

STATIC
EFI_STATUS
GetSectionContents (
  IN  CHAR8                     **InputFileName,
  IN  UINT32                    *InputFileAlign,
  IN  UINT32                    InputFileNum,
  IN  EFI_FFS_FILE_ATTRIBUTES   FfsAttrib,
  OUT UINT8                     *FileBuffer,
  OUT UINT32                    *BufferLength,
  OUT UINT32                    *MaxAlignment,
  OUT UINT8                     *PESectionNum
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

  MaxAlignment   - The max alignment required by all the input file datas.

  PeSectionNum   - Calculate the number of Pe/Te Section in this FFS file.

Returns:

  EFI_SUCCESS on successful return
  EFI_INVALID_PARAMETER if InputFileNum is less than 1 or BufferLength point is NULL.
  EFI_ABORTED if unable to open input file.
  EFI_BUFFER_TOO_SMALL FileBuffer is not enough to contain all file data.
--*/
{
  UINT32                              Size;
  UINT32                              Offset;
  UINT32                              FileSize;
  UINT32                              Index;
  FILE                                *InFile;
  EFI_FREEFORM_SUBTYPE_GUID_SECTION   *SectHeader;
  EFI_COMMON_SECTION_HEADER2          TempSectHeader;
  EFI_TE_IMAGE_HEADER                 TeHeader;
  UINT32                              TeOffset;
  EFI_GUID_DEFINED_SECTION            GuidSectHeader;
  EFI_GUID_DEFINED_SECTION2           GuidSectHeader2;
  UINT32                              HeaderSize;
  UINT32                              MaxEncounteredAlignment;

  Size                    = 0;
  Offset                  = 0;
  TeOffset                = 0;
  MaxEncounteredAlignment = 1;

  //
  // Go through our array of file names and copy their contents
  // to the output buffer.
  //
  for (Index = 0; Index < InputFileNum; Index++) {
    //
    // make sure section ends on a DWORD boundary
    //
    while ((Size & 0x03) != 0) {
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
    DebugMsg (NULL, 0, 9, "Input section files",
              "the input section name is %s and the size is %u bytes", InputFileName[Index], (unsigned) FileSize);

    //
    // Check this section is Te/Pe section, and Calculate the numbers of Te/Pe section.
    //
    TeOffset = 0;
    if (FileSize >= MAX_FFS_SIZE) {
      HeaderSize = sizeof (EFI_COMMON_SECTION_HEADER2);
    } else {
      HeaderSize = sizeof (EFI_COMMON_SECTION_HEADER);
    }
    fread (&TempSectHeader, 1, HeaderSize, InFile);
    if (TempSectHeader.Type == EFI_SECTION_TE) {
      (*PESectionNum) ++;
      fread (&TeHeader, 1, sizeof (TeHeader), InFile);
      if (TeHeader.Signature == EFI_TE_IMAGE_HEADER_SIGNATURE) {
        TeOffset = TeHeader.StrippedSize - sizeof (TeHeader);
      }
    } else if (TempSectHeader.Type == EFI_SECTION_PE32) {
      (*PESectionNum) ++;
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
      (*PESectionNum) ++;
    } else if (TempSectHeader.Type == EFI_SECTION_COMPRESSION ||
               TempSectHeader.Type == EFI_SECTION_FIRMWARE_VOLUME_IMAGE) {
      //
      // for the encapsulated section, assume it contains Pe/Te section
      //
      (*PESectionNum) ++;
    }

    fseek (InFile, 0, SEEK_SET);

    //
    // Revert TeOffset to the converse value relative to Alignment
    // This is to assure the original PeImage Header at Alignment.
    //
    if ((TeOffset != 0) && (InputFileAlign [Index] != 0)) {
      TeOffset = InputFileAlign [Index] - (TeOffset % InputFileAlign [Index]);
      TeOffset = TeOffset % InputFileAlign [Index];
    }

    //
    // make sure section data meet its alignment requirement by adding one pad section.
    // But the different sections have the different section header. Necessary or not?
    // Based on section type to adjust offset? Todo
    //
    if ((InputFileAlign [Index] != 0) && (((Size + HeaderSize + TeOffset) % InputFileAlign [Index]) != 0)) {
      Offset = (Size + sizeof (EFI_COMMON_SECTION_HEADER) + HeaderSize + TeOffset + InputFileAlign [Index] - 1) & ~(InputFileAlign [Index] - 1);
      Offset = Offset - Size - HeaderSize - TeOffset;

      if (FileBuffer != NULL && ((Size + Offset) < *BufferLength)) {
        //
        // The maximal alignment is 64K, the raw section size must be less than 0xffffff
        //
        memset (FileBuffer + Size, 0, Offset);
        SectHeader                        = (EFI_FREEFORM_SUBTYPE_GUID_SECTION *) (FileBuffer + Size);
        SectHeader->CommonHeader.Size[0]  = (UINT8) (Offset & 0xff);
        SectHeader->CommonHeader.Size[1]  = (UINT8) ((Offset & 0xff00) >> 8);
        SectHeader->CommonHeader.Size[2]  = (UINT8) ((Offset & 0xff0000) >> 16);

        //
        // Only add a special reducible padding section if
        // - this FFS has the FFS_ATTRIB_FIXED attribute,
        // - none of the preceding sections have alignment requirements,
        // - the size of the padding is sufficient for the
        //   EFI_SECTION_FREEFORM_SUBTYPE_GUID header.
        //
        if ((FfsAttrib & FFS_ATTRIB_FIXED) != 0 &&
            MaxEncounteredAlignment <= 1 &&
            Offset >= sizeof (EFI_FREEFORM_SUBTYPE_GUID_SECTION)) {
          SectHeader->CommonHeader.Type   = EFI_SECTION_FREEFORM_SUBTYPE_GUID;
          SectHeader->SubTypeGuid         = mEfiFfsSectionAlignmentPaddingGuid;
        } else {
          SectHeader->CommonHeader.Type   = EFI_SECTION_RAW;
        }
      }
      DebugMsg (NULL, 0, 9, "Pad raw section for section data alignment",
                "Pad Raw section size is %u", (unsigned) Offset);

      Size = Size + Offset;
    }

    //
    // Get the Max alignment of all input file datas
    //
    if (MaxEncounteredAlignment < InputFileAlign [Index]) {
      MaxEncounteredAlignment = InputFileAlign [Index];
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

  *MaxAlignment = MaxEncounteredAlignment;

  //
  // Set the actual length of the data.
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
  /*++
    InFile is input file for getting alignment
    return the alignment
    --*/
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
  int   argc,
  CHAR8 *argv[]
  )
/*++

Routine Description:

  Main function.

Arguments:

  argc - Number of command line parameters.
  argv - Array of pointers to parameter strings.

Returns:
  STATUS_SUCCESS - Utility exits successfully.
  STATUS_ERROR   - Some error occurred during execution.

--*/
{
  EFI_STATUS              Status;
  EFI_FFS_FILE_ATTRIBUTES FfsAttrib;
  UINT32                  FfsAlign;
  EFI_FV_FILETYPE         FfsFiletype;
  CHAR8                   *OutputFileName;
  EFI_GUID                FileGuid = {0};
  UINT32                  InputFileNum;
  UINT32                  *InputFileAlign;
  CHAR8                   **InputFileName;
  UINT8                   *FileBuffer;
  UINT32                  FileSize;
  UINT32                  MaxAlignment;
  EFI_FFS_FILE_HEADER2    FfsFileHeader;
  FILE                    *FfsFile;
  UINT32                  Index;
  UINT64                  LogLevel;
  UINT8                   PeSectionNum;
  UINT32                  HeaderSize;
  UINT32                  Alignment;
  //
  // Workaround for static code checkers.
  // Ensures the size of 'AlignmentBuffer' can hold all the digits of an
  // unsigned 32-bit integer plus the size unit character.
  //
  CHAR8                   AlignmentBuffer[16];

  //
  // Init local variables
  //
  LogLevel       = 0;
  Index          = 0;
  FfsAttrib      = 0;
  FfsAlign       = 0;
  FfsFiletype    = EFI_FV_FILETYPE_ALL;
  OutputFileName = NULL;
  InputFileNum   = 0;
  InputFileName  = NULL;
  InputFileAlign = NULL;
  FileBuffer     = NULL;
  FileSize       = 0;
  MaxAlignment   = 1;
  FfsFile        = NULL;
  Status         = EFI_SUCCESS;
  PeSectionNum   = 0;

  SetUtilityName (UTILITY_NAME);

  if (argc == 1) {
    Error (NULL, 0, 1001, "Missing options", "no options input");
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
    if ((stricmp (argv[0], "-t") == 0) || (stricmp (argv[0], "--filetype") == 0)) {
      if (argv[1] == NULL || argv[1][0] == '-') {
        Error (NULL, 0, 1003, "Invalid option value", "file type is missing for -t option");
        goto Finish;
      }
      FfsFiletype = StringToType (argv[1]);
      if (FfsFiletype == EFI_FV_FILETYPE_ALL) {
        Error (NULL, 0, 1003, "Invalid option value", "%s is not a valid file type", argv[1]);
        goto Finish;
      }
      argc -= 2;
      argv += 2;
      continue;
    }

    if ((stricmp (argv[0], "-o") == 0) || (stricmp (argv[0], "--outputfile") == 0)) {
      if (argv[1] == NULL || argv[1][0] == '-') {
        Error (NULL, 0, 1003, "Invalid option value", "Output file is missing for -o option");
        goto Finish;
      }
      OutputFileName = argv[1];
      argc -= 2;
      argv += 2;
      continue;
    }

    if ((stricmp (argv[0], "-g") == 0) || (stricmp (argv[0], "--fileguid") == 0)) {
      Status = StringToGuid (argv[1], &FileGuid);
      if (EFI_ERROR (Status)) {
        Error (NULL, 0, 1003, "Invalid option value", "%s = %s", argv[0], argv[1]);
        goto Finish;
      }
      argc -= 2;
      argv += 2;
      continue;
    }

    if ((stricmp (argv[0], "-x") == 0) || (stricmp (argv[0], "--fixed") == 0)) {
      FfsAttrib |= FFS_ATTRIB_FIXED;
      argc -= 1;
      argv += 1;
      continue;
    }

    if ((stricmp (argv[0], "-s") == 0) || (stricmp (argv[0], "--checksum") == 0)) {
      FfsAttrib |= FFS_ATTRIB_CHECKSUM;
      argc -= 1;
      argv += 1;
      continue;
    }

    if ((stricmp (argv[0], "-a") == 0) || (stricmp (argv[0], "--align") == 0)) {
      if (argv[1] == NULL || argv[1][0] == '-') {
        Error (NULL, 0, 1003, "Invalid option value", "Align value is missing for -a option");
        goto Finish;
      }
      for (Index = 0; Index < sizeof (mFfsValidAlignName) / sizeof (CHAR8 *); Index ++) {
        if (stricmp (argv[1], mFfsValidAlignName[Index]) == 0) {
          break;
        }
      }
      if (Index == sizeof (mFfsValidAlignName) / sizeof (CHAR8 *)) {
        if ((stricmp (argv[1], "1") == 0) || (stricmp (argv[1], "2") == 0) || (stricmp (argv[1], "4") == 0)) {
          //
          // 1, 2, 4 byte alignment same to 8 byte alignment
          //
          Index = 0;
        } else {
          Error (NULL, 0, 1003, "Invalid option value", "%s = %s", argv[0], argv[1]);
          goto Finish;
        }
      }
      FfsAlign = Index;
      argc -= 2;
      argv += 2;
      continue;
    }

    if ((stricmp (argv[0], "-oi") == 0) || (stricmp (argv[0], "--optionalsectionfile") == 0) || (stricmp (argv[0], "-i") == 0) || (stricmp (argv[0], "--sectionfile") == 0)) {
      //
      // Get Input file name and its alignment
      //
      if (argv[1] == NULL || argv[1][0] == '-') {
        Error (NULL, 0, 1003, "Invalid option value", "input section file is missing for -i option");
        goto Finish;
      }
      if ((stricmp (argv[0], "-oi") == 0) || (stricmp (argv[0], "--optionalsectionfile") == 0) ){
        if (-1 == access(argv[1] , 0)){
          Warning(NULL, 0, 0001, "File is not found.", argv[1]);
          argc -= 2;
          argv += 2;
          continue;
        }
      }
      //
      // Allocate Input file name buffer and its alignment buffer.
      //
      if ((InputFileNum == 0) && (InputFileName == NULL)) {
        InputFileName = (CHAR8 **) malloc (MAXIMUM_INPUT_FILE_NUM * sizeof (CHAR8 *));
        if (InputFileName == NULL) {
          Error (NULL, 0, 4001, "Resource", "memory cannot be allocated!");
          return STATUS_ERROR;
        }
        memset (InputFileName, 0, (MAXIMUM_INPUT_FILE_NUM * sizeof (CHAR8 *)));

        InputFileAlign = (UINT32 *) malloc (MAXIMUM_INPUT_FILE_NUM * sizeof (UINT32));
        if (InputFileAlign == NULL) {
          Error (NULL, 0, 4001, "Resource", "memory cannot be allocated!");
          free (InputFileName);
          return STATUS_ERROR;
        }
        memset (InputFileAlign, 0, MAXIMUM_INPUT_FILE_NUM * sizeof (UINT32));
      } else if (InputFileNum % MAXIMUM_INPUT_FILE_NUM == 0) {
        //
        // InputFileName and alignment buffer too small, need to realloc
        //
        InputFileName = (CHAR8 **) realloc (
                                    InputFileName,
                                    (InputFileNum + MAXIMUM_INPUT_FILE_NUM) * sizeof (CHAR8 *)
                                    );

        if (InputFileName == NULL) {
          Error (NULL, 0, 4001, "Resource", "memory cannot be allocated!");
          free (InputFileAlign);
          return STATUS_ERROR;
        }
        memset (&(InputFileName[InputFileNum]), 0, (MAXIMUM_INPUT_FILE_NUM * sizeof (CHAR8 *)));

        InputFileAlign = (UINT32 *) realloc (
                                    InputFileAlign,
                                    (InputFileNum + MAXIMUM_INPUT_FILE_NUM) * sizeof (UINT32)
                                    );

        if (InputFileAlign == NULL) {
          Error (NULL, 0, 4001, "Resource", "memory cannot be allocated!");
          free (InputFileName);
          return STATUS_ERROR;
        }
        memset (&(InputFileAlign[InputFileNum]), 0, (MAXIMUM_INPUT_FILE_NUM * sizeof (UINT32)));
      }

      InputFileName[InputFileNum] = argv[1];
      argc -= 2;
      argv += 2;

      if (argc <= 0) {
        InputFileNum ++;
        break;
      }

      //
      // Section File alignment requirement
      //
      if ((stricmp (argv[0], "-n") == 0) || (stricmp (argv[0], "--sectionalign") == 0)) {
        if ((argv[1] != NULL) && (stricmp("0", argv[1]) == 0)) {
          Status = GetAlignmentFromFile(InputFileName[InputFileNum], &Alignment);
          if (EFI_ERROR(Status)) {
            Error (NULL, 0, 1003, "Fail to get Alignment from %s", InputFileName[InputFileNum]);
            goto Finish;
          }
          if (Alignment < 0x400){
            sprintf (AlignmentBuffer, "%d", Alignment);
          }
          else if (Alignment >= 0x400) {
            if (Alignment >= 0x100000) {
              sprintf (AlignmentBuffer, "%dM", Alignment/0x100000);
            } else {
              sprintf (AlignmentBuffer, "%dK", Alignment/0x400);
            }
          }
          Status = StringtoAlignment (AlignmentBuffer, &(InputFileAlign[InputFileNum]));
        }
        else {
          Status = StringtoAlignment (argv[1], &(InputFileAlign[InputFileNum]));
        }
        if (EFI_ERROR (Status)) {
          Error (NULL, 0, 1003, "Invalid option value", "%s = %s", argv[0], argv[1]);
          goto Finish;
        }
        argc -= 2;
        argv += 2;
      }
      InputFileNum ++;
      continue;
    }

    if ((stricmp (argv[0], "-n") == 0) || (stricmp (argv[0], "--sectionalign") == 0)) {
      Error (NULL, 0, 1000, "Unknown option", "SectionAlign option must be specified with section file.");
      goto Finish;
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
        Error (NULL, 0, 1003, "Invalid option value", "Debug Level range is 0-9, current input level is %d", (int) LogLevel);
        goto Finish;
      }
      SetPrintLevel (LogLevel);
      DebugMsg (NULL, 0, 9, "Debug Mode Set", "Debug Output Mode Level %s is set!", argv[1]);
      argc -= 2;
      argv += 2;
      continue;
    }

    Error (NULL, 0, 1000, "Unknown option", "%s", argv[0]);
    goto Finish;
  }

  VerboseMsg ("%s tool start.", UTILITY_NAME);

  //
  // Check the complete input parameters.
  //
  if (FfsFiletype == EFI_FV_FILETYPE_ALL) {
    Error (NULL, 0, 1001, "Missing option", "filetype");
    goto Finish;
  }

  if (CompareGuid (&FileGuid, &mZeroGuid) == 0) {
    Error (NULL, 0, 1001, "Missing option", "fileguid");
    goto Finish;
  }

  if (InputFileNum == 0) {
    Error (NULL, 0, 1001, "Missing option", "Input files");
    goto Finish;
  }

  //
  // Output input parameter information
  //
  VerboseMsg ("Fv File type is %s", mFfsFileType [FfsFiletype]);
  VerboseMsg ("Output file name is %s", OutputFileName);
  VerboseMsg ("FFS File Guid is %08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
                (unsigned) FileGuid.Data1,
                FileGuid.Data2,
                FileGuid.Data3,
                FileGuid.Data4[0],
                FileGuid.Data4[1],
                FileGuid.Data4[2],
                FileGuid.Data4[3],
                FileGuid.Data4[4],
                FileGuid.Data4[5],
                FileGuid.Data4[6],
                FileGuid.Data4[7]);
  if ((FfsAttrib & FFS_ATTRIB_FIXED) != 0) {
    VerboseMsg ("FFS File has the fixed file attribute");
  }
  if ((FfsAttrib & FFS_ATTRIB_CHECKSUM) != 0) {
    VerboseMsg ("FFS File requires the checksum of the whole file");
  }
  VerboseMsg ("FFS file alignment is %s", mFfsValidAlignName[FfsAlign]);
  for (Index = 0; Index < InputFileNum; Index ++) {
    if (InputFileAlign[Index] == 0) {
      //
      // Minimum alignment is 1 byte.
      //
      InputFileAlign[Index] = 1;
    }
    VerboseMsg ("the %dth input section name is %s and section alignment is %u", Index, InputFileName[Index], (unsigned) InputFileAlign[Index]);
  }

  //
  // Calculate the size of all input section files.
  //
  Status = GetSectionContents (
             InputFileName,
             InputFileAlign,
             InputFileNum,
             FfsAttrib,
             FileBuffer,
             &FileSize,
             &MaxAlignment,
             &PeSectionNum
             );

  if ((FfsFiletype == EFI_FV_FILETYPE_SECURITY_CORE ||
      FfsFiletype == EFI_FV_FILETYPE_PEI_CORE ||
      FfsFiletype == EFI_FV_FILETYPE_DXE_CORE) && (PeSectionNum != 1)) {
    Error (NULL, 0, 2000, "Invalid parameter", "Fv File type %s must have one and only one Pe or Te section, but %u Pe/Te section are input", mFfsFileType [FfsFiletype], PeSectionNum);
    goto Finish;
  }

  if ((FfsFiletype == EFI_FV_FILETYPE_PEIM ||
      FfsFiletype == EFI_FV_FILETYPE_DRIVER ||
      FfsFiletype == EFI_FV_FILETYPE_COMBINED_PEIM_DRIVER ||
      FfsFiletype == EFI_FV_FILETYPE_APPLICATION) && (PeSectionNum < 1)) {
    Error (NULL, 0, 2000, "Invalid parameter", "Fv File type %s must have at least one Pe or Te section, but no Pe/Te section is input", mFfsFileType [FfsFiletype]);
    goto Finish;
  }

  if (Status == EFI_BUFFER_TOO_SMALL) {
    FileBuffer = (UINT8 *) malloc (FileSize);
    if (FileBuffer == NULL) {
      Error (NULL, 0, 4001, "Resource", "memory cannot be allocated!");
      goto Finish;
    }
    memset (FileBuffer, 0, FileSize);

    //
    // read all input file contents into a buffer
    //
    Status = GetSectionContents (
               InputFileName,
               InputFileAlign,
               InputFileNum,
               FfsAttrib,
               FileBuffer,
               &FileSize,
               &MaxAlignment,
               &PeSectionNum
               );
  }

  if (EFI_ERROR (Status)) {
    goto Finish;
  }

  if (FileBuffer == NULL && FileSize != 0) {
    Error (NULL, 0, 4001, "Resource", "memory cannot be allocated!");
    goto Finish;
  }

  //
  // Create Ffs file header.
  //
  memset (&FfsFileHeader, 0, sizeof (EFI_FFS_FILE_HEADER2));
  memcpy (&FfsFileHeader.Name, &FileGuid, sizeof (EFI_GUID));
  FfsFileHeader.Type       = FfsFiletype;
  //
  // Update FFS Alignment based on the max alignment required by input section files
  //
  VerboseMsg ("the max alignment of all input sections is %u", (unsigned) MaxAlignment);
  for (Index = 0; Index < sizeof (mFfsValidAlign) / sizeof (UINT32) - 1; Index ++) {
    if ((MaxAlignment > mFfsValidAlign [Index]) && (MaxAlignment <= mFfsValidAlign [Index + 1])) {
      break;
    }
  }
  if (FfsAlign < Index) {
    FfsAlign = Index;
  }
  VerboseMsg ("the alignment of the generated FFS file is %u", (unsigned) mFfsValidAlign [FfsAlign + 1]);

  //
  // Now FileSize includes the EFI_FFS_FILE_HEADER
  //
  if (FileSize + sizeof (EFI_FFS_FILE_HEADER) >= MAX_FFS_SIZE) {
    HeaderSize = sizeof (EFI_FFS_FILE_HEADER2);
    FileSize += sizeof (EFI_FFS_FILE_HEADER2);
    FfsFileHeader.ExtendedSize = FileSize;
    memset(FfsFileHeader.Size, 0, sizeof (UINT8) * 3);
    FfsAttrib |= FFS_ATTRIB_LARGE_FILE;
  } else {
    HeaderSize = sizeof (EFI_FFS_FILE_HEADER);
    FileSize += sizeof (EFI_FFS_FILE_HEADER);
    FfsFileHeader.Size[0]  = (UINT8) (FileSize & 0xFF);
    FfsFileHeader.Size[1]  = (UINT8) ((FileSize & 0xFF00) >> 8);
    FfsFileHeader.Size[2]  = (UINT8) ((FileSize & 0xFF0000) >> 16);
  }
  VerboseMsg ("the size of the generated FFS file is %u bytes", (unsigned) FileSize);

  //FfsAlign larger than 7, set FFS_ATTRIB_DATA_ALIGNMENT2
  if (FfsAlign < 8) {
    FfsFileHeader.Attributes = (EFI_FFS_FILE_ATTRIBUTES) (FfsAttrib | (FfsAlign << 3));
  } else {
    FfsFileHeader.Attributes = (EFI_FFS_FILE_ATTRIBUTES) (FfsAttrib | ((FfsAlign & 0x7) << 3) | FFS_ATTRIB_DATA_ALIGNMENT2);
  }

  //
  // Fill in checksums and state, these must be zero for checksumming
  //
  // FileHeader.IntegrityCheck.Checksum.Header = 0;
  // FileHeader.IntegrityCheck.Checksum.File = 0;
  // FileHeader.State = 0;
  //
  FfsFileHeader.IntegrityCheck.Checksum.Header = CalculateChecksum8 (
                                                   (UINT8 *) &FfsFileHeader,
                                                   HeaderSize
                                                   );

  if (FfsFileHeader.Attributes & FFS_ATTRIB_CHECKSUM) {
    //
    // Ffs header checksum = zero, so only need to calculate ffs body.
    //
    FfsFileHeader.IntegrityCheck.Checksum.File = CalculateChecksum8 (
                                                   FileBuffer,
                                                   FileSize - HeaderSize
                                                   );
  } else {
    FfsFileHeader.IntegrityCheck.Checksum.File = FFS_FIXED_CHECKSUM;
  }

  FfsFileHeader.State = EFI_FILE_HEADER_CONSTRUCTION | EFI_FILE_HEADER_VALID | EFI_FILE_DATA_VALID;

  //
  // Open output file to write ffs data.
  //
  if (OutputFileName != NULL) {
    remove(OutputFileName);
    FfsFile = fopen (LongFilePath (OutputFileName), "wb");
    if (FfsFile == NULL) {
      Error (NULL, 0, 0001, "Error opening file", OutputFileName);
      goto Finish;
    }
    //
    // write header
    //
    fwrite (&FfsFileHeader, 1, HeaderSize, FfsFile);
    //
    // write data
    //
    if (FileBuffer != NULL) {
      fwrite (FileBuffer, 1, FileSize - HeaderSize, FfsFile);
    }

    fclose (FfsFile);
  }

Finish:
  if (InputFileName != NULL) {
    free (InputFileName);
  }
  if (InputFileAlign != NULL) {
    free (InputFileAlign);
  }
  if (FileBuffer != NULL) {
    free (FileBuffer);
  }
  //
  // If any errors were reported via the standard error reporting
  // routines, then the status has been saved. Get the value and
  // return it to the caller.
  //
  VerboseMsg ("%s tool done with return code is 0x%x.", UTILITY_NAME, GetUtilityStatus ());

  return GetUtilityStatus ();
}
