/*++

Copyright (c) 2004 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  GenSection.c

Abstract:

  Creates output file that is a properly formed section per the FV spec.

--*/

#include "TianoCommon.h"
#include "EfiImageFormat.h"
#include "Compress.h"
#include "EfiCustomizedCompress.h"
#include "Crc32.h"
#include "EfiUtilityMsgs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "GenSection.h"

#include EFI_PROTOCOL_DEFINITION (GuidedSectionExtraction)

#define UTILITY_NAME            "GenSection"
#define UTILITY_VERSION         "v1.0"

#define PARAMETER_NOT_SPECIFIED "Parameter not specified"
#define MAXIMUM_INPUT_FILE_NUM  10
#define MAX_SECTION_SIZE        0x1000000

char      *SectionTypeName[] = {
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
  "EFI_SECTION_PEI_DEPEX"               // 0x1B
};

char      *CompressionTypeName[]    = { "NONE", "STANDARD" };
char      *GUIDedSectionTypeName[]  = { "CRC32" };
EFI_GUID  gEfiCrc32SectionGuid      = EFI_CRC32_GUIDED_SECTION_EXTRACTION_PROTOCOL_GUID;

static
VOID
PrintUsageMessage (
  VOID
  )
{
  UINTN       SectionType;
  int         Index;
  const char  *Str[] = {
    UTILITY_NAME" "UTILITY_VERSION" - Intel Generate Section Utility",
    "  Copyright (C), 2004 - 2008 Intel Corporation",
    
#if ( defined(UTILITY_BUILD) && defined(UTILITY_VENDOR) )
    "  Built from "UTILITY_BUILD", project of "UTILITY_VENDOR,
#endif
    "",
    "Usage:",
    "  "UTILITY_NAME" [OPTION]",
    "Common Options:",
    "  -i InputFile    Specifies the input file",
    "  -o OutputFile   Specifies the output file",
    "  -s SectionType  Specifies the type of the section, which can be one of",
    NULL
  };

  for (Index = 0; Str[Index] != NULL; Index++) {
    fprintf (stdout, "%s\n", Str[Index]);
  }

  for (SectionType = 0; SectionType <= EFI_SECTION_LAST_SECTION_TYPE; SectionType++) {
    if (SectionTypeName[SectionType] != NULL) {
      fprintf (stdout, "                  %s\n", SectionTypeName[SectionType]);
    }
  }
  fprintf (stdout, "Section dependent options:\n");
  fprintf (stdout, 
    "  %s:       -t < %s | %s >\n",
    SectionTypeName[EFI_SECTION_COMPRESSION],
    CompressionTypeName[EFI_NOT_COMPRESSED],
    CompressionTypeName[EFI_STANDARD_COMPRESSION]
    );
  fprintf (stdout,
    "  %s:      -t < %s >  // Only CRC32 is supported\n",
    SectionTypeName[EFI_SECTION_GUID_DEFINED],
    GUIDedSectionTypeName[EFI_SECTION_CRC32_GUID_DEFINED]
    );
  printf (
    "  %s:           -v VersionNumber [-a \"Version string\"]\n",
    SectionTypeName[EFI_SECTION_VERSION]
    );
  printf (
    "  %s:    -a \"Human readable name\"\n",
    SectionTypeName[EFI_SECTION_USER_INTERFACE]
    );
}

VOID
Ascii2UnicodeWriteString (
  char    *String,
  FILE    *OutFile,
  BOOLEAN WriteLangCode
  )
{
  UINTN Index;
  UINT8 AsciiNull;
  //
  // BUGBUG need to get correct language code...
  //
  char  *EnglishLangCode = "eng";
  AsciiNull = 0;
  //
  // first write the language code (english only)
  //
  if (WriteLangCode) {
    fwrite (EnglishLangCode, 1, 4, OutFile);
  }
  //
  // Next, write out the string... Convert ASCII to Unicode in the process.
  //
  Index = 0;
  do {
    fwrite (&String[Index], 1, 1, OutFile);
    fwrite (&AsciiNull, 1, 1, OutFile);
  } while (String[Index++] != 0);
}

STATUS
GenSectionCommonLeafSection (
  char    **InputFileName,
  int     InputFileNum,
  UINTN   SectionType,
  FILE    *OutFile
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

  OutFile        - Output file handle

Returns:
                       
  STATUS_ERROR            - can't continue
  STATUS_SUCCESS          - successful return

--*/
{
  UINT64                    InputFileLength;
  FILE                      *InFile;
  UINT8                     *Buffer;
  INTN                      TotalLength;
  EFI_COMMON_SECTION_HEADER CommonSect;
  STATUS                    Status;

  if (InputFileNum > 1) {
    Error (NULL, 0, 0, "invalid parameter", "more than one input file specified");
    return STATUS_ERROR;
  } else if (InputFileNum < 1) {
    Error (NULL, 0, 0, "no input file specified", NULL);
    return STATUS_ERROR;
  }
  //
  // Open the input file
  //
  InFile = fopen (InputFileName[0], "rb");
  if (InFile == NULL) {
    Error (NULL, 0, 0, InputFileName[0], "failed to open input file");
    return STATUS_ERROR;
  }

  Status  = STATUS_ERROR;
  Buffer  = NULL;
  //
  // Seek to the end of the input file so we can determine its size
  //
  fseek (InFile, 0, SEEK_END);
  fgetpos (InFile, &InputFileLength);
  fseek (InFile, 0, SEEK_SET);
  //
  // Fill in the fields in the local section header structure
  //
  CommonSect.Type = (EFI_SECTION_TYPE) SectionType;
  TotalLength     = sizeof (CommonSect) + (INTN) InputFileLength;
  //
  // Size must fit in 3 bytes
  //
  if (TotalLength >= MAX_SECTION_SIZE) {
    Error (NULL, 0, 0, InputFileName[0], "file size (0x%X) exceeds section size limit(%dM).", TotalLength, MAX_SECTION_SIZE>>20);
    goto Done;
  }
  //
  // Now copy the size into the section header and write out the section header
  //
  memcpy (&CommonSect.Size, &TotalLength, 3);
  fwrite (&CommonSect, sizeof (CommonSect), 1, OutFile);
  //
  // Allocate a buffer to read in the contents of the input file. Then
  // read it in as one block and write it to the output file.
  //
  if (InputFileLength != 0) {
    Buffer = (UINT8 *) malloc ((size_t) InputFileLength);
    if (Buffer == NULL) {
      Error (__FILE__, __LINE__, 0, "memory allocation failure", NULL);
      goto Done;
    }

    if (fread (Buffer, (size_t) InputFileLength, 1, InFile) != 1) {
      Error (NULL, 0, 0, InputFileName[0], "failed to read contents of file");
      goto Done;
    }

    if (fwrite (Buffer, (size_t) InputFileLength, 1, OutFile) != 1) {
      Error (NULL, 0, 0, "failed to write to output file", NULL);
      goto Done;
    }
  }

  Status = STATUS_SUCCESS;
Done:
  fclose (InFile);
  if (Buffer != NULL) {
    free (Buffer);
  }

  return Status;
}

EFI_STATUS
GetSectionContents (
  char    **InputFileName,
  int     InputFileNum,
  UINT8   *FileBuffer,
  UINTN   *BufferLength
  )
/*++
        
Routine Description:
           
  Get the contents of all section files specified in InputFileName
  into FileBuffer.
            
Arguments:
               
  InputFileName  - Name of the input file.
                
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
  UINTN   Size;
  fpos_t  FileSize;
  INTN    Index;
  FILE    *InFile;

  if (InputFileNum < 1) {
    Error (NULL, 0, 0, "must specify at least one input file", NULL);
    return EFI_INVALID_PARAMETER;
  }

  if (BufferLength == NULL) {
    Error (NULL, 0, 0, "BufferLength can't be NULL", NULL);
    return EFI_INVALID_PARAMETER;
  }

  Size = 0;
  //
  // Go through our array of file names and copy their contents
  // to the output buffer.
  //
  for (Index = 0; Index < InputFileNum; Index++) {
    InFile = fopen (InputFileName[Index], "rb");
    if (InFile == NULL) {
      Error (NULL, 0, 0, InputFileName[Index], "failed to open input file");
      return EFI_ABORTED;
    }

    fseek (InFile, 0, SEEK_END);
    fgetpos (InFile, &FileSize);
    fseek (InFile, 0, SEEK_SET);
    //
    // Now read the contents of the file into the buffer
    // Buffer must be enough to contain the file content.
    //
    if (FileSize > 0 && FileBuffer != NULL && (Size + (UINTN) FileSize) <= *BufferLength) {
      if (fread (FileBuffer + Size, (size_t) FileSize, 1, InFile) != 1) {
        Error (NULL, 0, 0, InputFileName[Index], "failed to read contents of input file");
        fclose (InFile);
        return EFI_ABORTED;
      }
    }

    fclose (InFile);
    Size += (UINTN) FileSize;
    //
    // make sure section ends on a DWORD boundary
    //
    while ((Size & 0x03) != 0) {
      if (FileBuffer != NULL && Size < *BufferLength) {
        FileBuffer[Size] = 0;
      }
      Size++;
    }
  }
  
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
  char    **InputFileName,
  int     InputFileNum,
  UINTN   SectionType,
  UINTN   SectionSubType,
  FILE    *OutFile
  )
/*++
        
Routine Description:
           
  Generate an encapsulating section of type EFI_SECTION_COMPRESSION
  Input file must be already sectioned. The function won't validate
  the input files' contents. Caller should hand in files already 
  with section header.
            
Arguments:
               
  InputFileName  - Name of the input file.
                
  InputFileNum   - Number of input files. Should be at least 1.

  SectionType    - Section type to generate. Should be 
                   EFI_SECTION_COMPRESSION

  SectionSubType - Specify the compression algorithm requested. 
  
  OutFile        - Output file handle

Returns:
                       
  EFI_SUCCESS           on successful return
  EFI_INVALID_PARAMETER if InputFileNum is less than 1
  EFI_ABORTED           if unable to open input file.
  EFI_OUT_OF_RESOURCES  No resource to complete the operation.
--*/
{
  UINTN                   TotalLength;
  UINTN                   InputLength;
  UINTN                   CompressedLength;
  UINT8                   *FileBuffer;
  UINT8                   *OutputBuffer;
  EFI_STATUS              Status;
  EFI_COMPRESSION_SECTION CompressionSect;
  COMPRESS_FUNCTION       CompressFunction;

  if (SectionType != EFI_SECTION_COMPRESSION) {
    Error (NULL, 0, 0, "parameter must be EFI_SECTION_COMPRESSION", NULL);
    return EFI_INVALID_PARAMETER;
  }

  InputLength       = 0;
  FileBuffer        = NULL;
  OutputBuffer      = NULL;
  CompressedLength  = 0;
  //
  // read all input file contents into a buffer
  // first get the size of all file contents
  //
  Status = GetSectionContents (
            InputFileName,
            InputFileNum,
            FileBuffer,
            &InputLength
            );

  if (Status == EFI_BUFFER_TOO_SMALL) {
    FileBuffer = (UINT8 *) malloc (InputLength);
    if (FileBuffer == NULL) {
      Error (__FILE__, __LINE__, 0, "application error", "failed to allocate memory");
      return EFI_OUT_OF_RESOURCES;
    }
    //
    // read all input file contents into a buffer
    //
    Status = GetSectionContents (
              InputFileName,
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

  CompressFunction = NULL;

  //
  // Now data is in FileBuffer, compress the data
  //
  switch (SectionSubType) {
  case EFI_NOT_COMPRESSED:
    CompressedLength = InputLength;
    break;

  case EFI_STANDARD_COMPRESSION:
    CompressFunction = (COMPRESS_FUNCTION) TianoCompress;
    break;

  case EFI_CUSTOMIZED_COMPRESSION:
    CompressFunction = (COMPRESS_FUNCTION) CustomizedCompress;
    break;

  default:
    Error (NULL, 0, 0, "unknown compression type", NULL);
    free (FileBuffer);
    return EFI_ABORTED;
  }

  if (CompressFunction != NULL) {

    Status = CompressFunction (FileBuffer, InputLength, OutputBuffer, &CompressedLength);
    if (Status == EFI_BUFFER_TOO_SMALL) {
      OutputBuffer = malloc (CompressedLength);
      if (!OutputBuffer) {
        free (FileBuffer);
        return EFI_OUT_OF_RESOURCES;
      }

      Status = CompressFunction (FileBuffer, InputLength, OutputBuffer, &CompressedLength);
    }

    free (FileBuffer);
    FileBuffer = OutputBuffer;

    if (EFI_ERROR (Status)) {
      if (FileBuffer != NULL) {
        free (FileBuffer);
      }

      return Status;
    }
  }

  TotalLength = CompressedLength + sizeof (EFI_COMPRESSION_SECTION);
  if (TotalLength >= MAX_SECTION_SIZE) {
    Error (__FILE__, __LINE__, 0, "input error", "The size of all files exceeds section size limit(%dM).", MAX_SECTION_SIZE>>20);
    if (FileBuffer != NULL) {
      free (FileBuffer);
    }
    if (OutputBuffer != NULL) {
      free (OutputBuffer);
    }
    return STATUS_ERROR;
  }
  //
  // Add the section header for the compressed data
  //
  CompressionSect.CommonHeader.Type     = (EFI_SECTION_TYPE) SectionType;
  CompressionSect.CommonHeader.Size[0]  = (UINT8) (TotalLength & 0xff);
  CompressionSect.CommonHeader.Size[1]  = (UINT8) ((TotalLength & 0xff00) >> 8);
  CompressionSect.CommonHeader.Size[2]  = (UINT8) ((TotalLength & 0xff0000) >> 16);
  CompressionSect.CompressionType       = (UINT8) SectionSubType;
  CompressionSect.UncompressedLength    = InputLength;

  fwrite (&CompressionSect, sizeof (CompressionSect), 1, OutFile);
  fwrite (FileBuffer, CompressedLength, 1, OutFile);
  free (FileBuffer);
  return EFI_SUCCESS;
}

EFI_STATUS
GenSectionGuidDefinedSection (
  char    **InputFileName,
  int     InputFileNum,
  UINTN   SectionType,
  UINTN   SectionSubType,
  FILE    *OutFile
  )
/*++
        
Routine Description:
           
  Generate an encapsulating section of type EFI_SECTION_GUID_DEFINED
  Input file must be already sectioned. The function won't validate
  the input files' contents. Caller should hand in files already 
  with section header.
            
Arguments:
               
  InputFileName  - Name of the input file.
                
  InputFileNum   - Number of input files. Should be at least 1.

  SectionType    - Section type to generate. Should be 
                   EFI_SECTION_GUID_DEFINED

  SectionSubType - Specify the authentication algorithm requested. 
  
  OutFile        - Output file handle

Returns:
                       
  EFI_SUCCESS on successful return
  EFI_INVALID_PARAMETER if InputFileNum is less than 1
  EFI_ABORTED if unable to open input file.
  EFI_OUT_OF_RESOURCES  No resource to complete the operation.

--*/
{
  INTN                  TotalLength;
  INTN                  InputLength;
  UINT8                 *FileBuffer;
  UINT32                Crc32Checksum;
  EFI_STATUS            Status;
  CRC32_SECTION_HEADER  Crc32GuidSect;

  if (SectionType != EFI_SECTION_GUID_DEFINED) {
    Error (NULL, 0, 0, "parameter must be EFI_SECTION_GUID_DEFINED", NULL);
    return EFI_INVALID_PARAMETER;
  }

  InputLength = 0;
  FileBuffer  = NULL;
  //
  // read all input file contents into a buffer
  // first get the size of all file contents
  //
  Status = GetSectionContents (
            InputFileName,
            InputFileNum,
            FileBuffer,
            &InputLength
            );

  if (Status == EFI_BUFFER_TOO_SMALL) {
    FileBuffer = (UINT8 *) malloc (InputLength);
    if (FileBuffer == NULL) {
      Error (__FILE__, __LINE__, 0, "application error", "failed to allocate memory");
      return EFI_OUT_OF_RESOURCES;
    }
    //
    // read all input file contents into a buffer
    //
    Status = GetSectionContents (
              InputFileName,
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
  //
  // Now data is in FileBuffer
  //
  switch (SectionSubType) {
  case EFI_SECTION_CRC32_GUID_DEFINED:
    Crc32Checksum = 0;
    CalculateCrc32 (FileBuffer, InputLength, &Crc32Checksum);
    if (EFI_ERROR (Status)) {
      free (FileBuffer);
      return Status;
    }

    TotalLength = InputLength + CRC32_SECTION_HEADER_SIZE;
    if (TotalLength >= MAX_SECTION_SIZE) {
      Error (__FILE__, __LINE__, 0, "input error", "The size of all files exceeds section size limit(%dM).", MAX_SECTION_SIZE>>20);
      free (FileBuffer);
      return STATUS_ERROR;
    }

    Crc32GuidSect.GuidSectionHeader.CommonHeader.Type     = (EFI_SECTION_TYPE) SectionType;
    Crc32GuidSect.GuidSectionHeader.CommonHeader.Size[0]  = (UINT8) (TotalLength & 0xff);
    Crc32GuidSect.GuidSectionHeader.CommonHeader.Size[1]  = (UINT8) ((TotalLength & 0xff00) >> 8);
    Crc32GuidSect.GuidSectionHeader.CommonHeader.Size[2]  = (UINT8) ((TotalLength & 0xff0000) >> 16);
    memcpy (&(Crc32GuidSect.GuidSectionHeader.SectionDefinitionGuid), &gEfiCrc32SectionGuid, sizeof (EFI_GUID));
    Crc32GuidSect.GuidSectionHeader.Attributes  = EFI_GUIDED_SECTION_AUTH_STATUS_VALID;
    Crc32GuidSect.GuidSectionHeader.DataOffset  = CRC32_SECTION_HEADER_SIZE;
    Crc32GuidSect.CRC32Checksum                 = Crc32Checksum;

    break;

  default:
    Error (NULL, 0, 0, "invalid parameter", "unknown GUID defined type");
    free (FileBuffer);
    return EFI_ABORTED;
  }

  fwrite (&Crc32GuidSect, sizeof (Crc32GuidSect), 1, OutFile);
  fwrite (FileBuffer, InputLength, 1, OutFile);

  free (FileBuffer);

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
  INTN                      Index;
  INTN                      VersionNumber;
  UINTN                     SectionType;
  UINTN                     SectionSubType;
  BOOLEAN                   InputFileRequired;
  BOOLEAN                   SubTypeRequired;
  FILE                      *InFile;
  FILE                      *OutFile;
  INTN                      InputFileNum;

  char                      **InputFileName;
  char                      *OutputFileName;
  char                      AuxString[500] = { 0 };

  char                      *ParamSectionType;
  char                      *ParamSectionSubType;
  char                      *ParamLength;
  char                      *ParamVersion;
  char                      *ParamDigitalSignature;

  EFI_STATUS                Status;
  EFI_COMMON_SECTION_HEADER CommonSect;

  InputFileName         = NULL;
  OutputFileName        = PARAMETER_NOT_SPECIFIED;
  ParamSectionType      = PARAMETER_NOT_SPECIFIED;
  ParamSectionSubType   = PARAMETER_NOT_SPECIFIED;
  ParamLength           = PARAMETER_NOT_SPECIFIED;
  ParamVersion          = PARAMETER_NOT_SPECIFIED;
  ParamDigitalSignature = PARAMETER_NOT_SPECIFIED;
  Status                = EFI_SUCCESS;

  VersionNumber         = 0;
  SectionType           = 0;
  SectionSubType        = 0;
  InputFileRequired     = TRUE;
  SubTypeRequired       = FALSE;
  InFile                = NULL;
  OutFile               = NULL;
  InputFileNum          = 0;
  Status                = EFI_SUCCESS;

  SetUtilityName (UTILITY_NAME);
  if (argc == 1) {
    PrintUsageMessage ();
    return STATUS_ERROR;
  }
  //
  // Parse command line
  //
  Index = 1;
  while (Index < argc) {
    if (_strcmpi (argv[Index], "-i") == 0) {
      //
      // Input File found
      //
      Index++;
      InputFileName = (char **) malloc (MAXIMUM_INPUT_FILE_NUM * sizeof (char *));
      if (InputFileName == NULL) {
        Error (__FILE__, __LINE__, 0, "application error", "failed to allocate memory");
        return EFI_OUT_OF_RESOURCES;
      }

      memset (InputFileName, 0, (MAXIMUM_INPUT_FILE_NUM * sizeof (char *)));
      InputFileName[InputFileNum] = argv[Index];
      InputFileNum++;
      Index++;
      //
      // Parse subsequent parameters until another switch is encountered
      //
      while ((Index < argc) && (argv[Index][0] != '-')) {
        if ((InputFileNum % MAXIMUM_INPUT_FILE_NUM) == 0) {
          //
          // InputFileName buffer too small, need to realloc
          //
          InputFileName = (char **) realloc (
                                      InputFileName,
                                      (InputFileNum + MAXIMUM_INPUT_FILE_NUM) * sizeof (char *)
                                      );
          if (InputFileName == NULL) {
            Error (__FILE__, __LINE__, 0, "application error", "failed to allocate memory");
            return EFI_OUT_OF_RESOURCES;
          }

          memset (&(InputFileName[InputFileNum]), 0, (MAXIMUM_INPUT_FILE_NUM * sizeof (char *)));
        }

        InputFileName[InputFileNum] = argv[Index];
        InputFileNum++;
        Index++;
      }

    }

    if (_strcmpi (argv[Index], "-o") == 0) {
      //
      // Output file found
      //
      Index++;
      OutputFileName = argv[Index];
    } else if (_strcmpi (argv[Index], "-s") == 0) {
      //
      // Section Type found
      //
      Index++;
      ParamSectionType = argv[Index];
    } else if (_strcmpi (argv[Index], "-t") == 0) {
      //
      // Compression or Authentication type
      //
      Index++;
      ParamSectionSubType = argv[Index];
    } else if (_strcmpi (argv[Index], "-l") == 0) {
      //
      // Length
      //
      Index++;
      ParamLength = argv[Index];
    } else if (_strcmpi (argv[Index], "-v") == 0) {
      //
      // VersionNumber
      //
      Index++;
      ParamVersion = argv[Index];
    } else if (_strcmpi (argv[Index], "-a") == 0) {
      //
      // Aux string
      //
      Index++;
      //
      // Note, the MSVC C-Start parses out and consolidates quoted strings from the command
      // line.  Quote characters are stripped.  If this tool is ported to other environments
      // this will need to be taken into account
      //
      strncpy (AuxString, argv[Index], 499);
    } else if (_strcmpi (argv[Index], "-d") == 0) {
      //
      // Digital signature for EFI_TEST_AUTHENTICAION (must be 0 or 1)
      //
      Index++;
      ParamDigitalSignature = argv[Index];
    } else if (_strcmpi (argv[Index], "-?") == 0) {
      PrintUsageMessage ();
      return STATUS_ERROR;
    } else {
      Error (NULL, 0, 0, argv[Index], "unknown option");
      return GetUtilityStatus ();
    }

    Index++;
  }
  //
  // At this point, all command line parameters are verified as not being totally
  // bogus.  Next verify the command line parameters are complete and make
  // sense...
  //
  if (_stricmp (ParamSectionType, SectionTypeName[EFI_SECTION_COMPRESSION]) == 0) {
    SectionType     = EFI_SECTION_COMPRESSION;
    SubTypeRequired = TRUE;
    if (_stricmp (ParamSectionSubType, CompressionTypeName[EFI_NOT_COMPRESSED]) == 0) {
      SectionSubType = EFI_NOT_COMPRESSED;
    } else if (_stricmp (ParamSectionSubType, CompressionTypeName[EFI_STANDARD_COMPRESSION]) == 0) {
      SectionSubType = EFI_STANDARD_COMPRESSION;
    } else {
      Error (NULL, 0, 0, ParamSectionSubType, "unknown compression type");
      PrintUsageMessage ();
      return GetUtilityStatus ();
    }
  } else if (_stricmp (ParamSectionType, SectionTypeName[EFI_SECTION_GUID_DEFINED]) == 0) {
    SectionType     = EFI_SECTION_GUID_DEFINED;
    SubTypeRequired = TRUE;
    if (_stricmp (ParamSectionSubType, GUIDedSectionTypeName[EFI_SECTION_CRC32_GUID_DEFINED]) == 0) {
      SectionSubType = EFI_SECTION_CRC32_GUID_DEFINED;
    } else {
      Error (NULL, 0, 0, ParamSectionSubType, "unknown GUID defined section type", ParamSectionSubType);
      PrintUsageMessage ();
      return GetUtilityStatus ();
    }
  } else if (_stricmp (ParamSectionType, SectionTypeName[EFI_SECTION_PE32]) == 0) {
    SectionType = EFI_SECTION_PE32;
  } else if (_stricmp (ParamSectionType, SectionTypeName[EFI_SECTION_PIC]) == 0) {
    SectionType = EFI_SECTION_PIC;
  } else if (_stricmp (ParamSectionType, SectionTypeName[EFI_SECTION_TE]) == 0) {
    SectionType = EFI_SECTION_TE;
  } else if (_stricmp (ParamSectionType, SectionTypeName[EFI_SECTION_DXE_DEPEX]) == 0) {
    SectionType = EFI_SECTION_DXE_DEPEX;
  } else if (_stricmp (ParamSectionType, SectionTypeName[EFI_SECTION_VERSION]) == 0) {
    SectionType       = EFI_SECTION_VERSION;
    InputFileRequired = FALSE;
    Index             = sscanf (ParamVersion, "%d", &VersionNumber);
    if (Index != 1 || VersionNumber < 0 || VersionNumber > 65565) {
      Error (NULL, 0, 0, ParamVersion, "illegal version number");
      PrintUsageMessage ();
      return GetUtilityStatus ();
    }

    if (strcmp (AuxString, PARAMETER_NOT_SPECIFIED) == 0) {
      AuxString[0] = 0;
    }
  } else if (_stricmp (ParamSectionType, SectionTypeName[EFI_SECTION_USER_INTERFACE]) == 0) {
    SectionType       = EFI_SECTION_USER_INTERFACE;
    InputFileRequired = FALSE;
    if (strcmp (AuxString, PARAMETER_NOT_SPECIFIED) == 0) {
      Error (NULL, 0, 0, "user interface string not specified", NULL);
      PrintUsageMessage ();
      return GetUtilityStatus ();
    }
  } else if (_stricmp (ParamSectionType, SectionTypeName[EFI_SECTION_COMPATIBILITY16]) == 0) {
    SectionType = EFI_SECTION_COMPATIBILITY16;
  } else if (_stricmp (ParamSectionType, SectionTypeName[EFI_SECTION_FIRMWARE_VOLUME_IMAGE]) == 0) {
    SectionType = EFI_SECTION_FIRMWARE_VOLUME_IMAGE;
  } else if (_stricmp (ParamSectionType, SectionTypeName[EFI_SECTION_FREEFORM_SUBTYPE_GUID]) == 0) {
    SectionType = EFI_SECTION_FREEFORM_SUBTYPE_GUID;
  } else if (_stricmp (ParamSectionType, SectionTypeName[EFI_SECTION_RAW]) == 0) {
    SectionType = EFI_SECTION_RAW;
  } else if (_stricmp (ParamSectionType, SectionTypeName[EFI_SECTION_PEI_DEPEX]) == 0) {
    SectionType = EFI_SECTION_PEI_DEPEX;
  } else {
    Error (NULL, 0, 0, ParamSectionType, "unknown section type");
    PrintUsageMessage ();
    return GetUtilityStatus ();
  }
  //
  // Open output file
  //
  OutFile = fopen (OutputFileName, "wb");
  if (OutFile == NULL) {
    Error (NULL, 0, 0, OutputFileName, "failed to open output file for writing");
    if (InFile != NULL) {
      fclose (InFile);
    }

    return GetUtilityStatus ();
  }
  //
  // At this point, we've fully validated the command line, and opened appropriate
  // files, so let's go and do what we've been asked to do...
  //
  //
  // Within this switch, build and write out the section header including any
  // section type specific pieces.  If there's an input file, it's tacked on later
  //
  switch (SectionType) {
  case EFI_SECTION_COMPRESSION:
    Status = GenSectionCompressionSection (
              InputFileName,
              InputFileNum,
              SectionType,
              SectionSubType,
              OutFile
              );
    break;

  case EFI_SECTION_GUID_DEFINED:
    Status = GenSectionGuidDefinedSection (
              InputFileName,
              InputFileNum,
              SectionType,
              SectionSubType,
              OutFile
              );
    break;

  case EFI_SECTION_VERSION:
    CommonSect.Type = (EFI_SECTION_TYPE) SectionType;

    Index           = sizeof (CommonSect);
    //
    // 2 characters for the build number
    //
    Index += 2;
    //
    // Aux string is ascii.. unicode is 2X + 2 bytes for terminating unicode null.
    //
    Index += (strlen (AuxString) * 2) + 2;
    memcpy (&CommonSect.Size, &Index, 3);
    fwrite (&CommonSect, sizeof (CommonSect), 1, OutFile);
    fwrite (&VersionNumber, 2, 1, OutFile);
    Ascii2UnicodeWriteString (AuxString, OutFile, FALSE);
    break;

  case EFI_SECTION_USER_INTERFACE:
    CommonSect.Type = (EFI_SECTION_TYPE) SectionType;
    Index           = sizeof (CommonSect);
    //
    // Aux string is ascii.. unicode is 2X + 2 bytes for terminating unicode null.
    //
    Index += (strlen (AuxString) * 2) + 2;
    memcpy (&CommonSect.Size, &Index, 3);
    fwrite (&CommonSect, sizeof (CommonSect), 1, OutFile);
    Ascii2UnicodeWriteString (AuxString, OutFile, FALSE);
    break;

  default:
    //
    // All other section types are caught by default (they're all the same)
    //
    Status = GenSectionCommonLeafSection (
              InputFileName,
              InputFileNum,
              SectionType,
              OutFile
              );
    break;
  }

  if (InputFileName != NULL) {
    free (InputFileName);
  }

  fclose (OutFile);
  //
  // If we had errors, then delete the output file
  //
  if (GetUtilityStatus () == STATUS_ERROR) {
    remove (OutputFileName);
  }

  return GetUtilityStatus ();
}
