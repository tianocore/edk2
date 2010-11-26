/*++

Copyright (c) 2004 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  
    GenCRC32Section.c

Abstract:

  This file contains functions required to generate a Firmware File System 
  file. The code is compliant with the Tiano C Coding standards.

--*/

#include "TianoCommon.h"
#include "EfiFirmwareFileSystem.h"
#include "EfiFirmwareVolumeHeader.h"
#include "ParseInf.h"
#include "crc32.h"
#include "EfiUtilityMsgs.h"
#include "GenCRC32Section.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "CommonLib.h"

#include EFI_PROTOCOL_DEFINITION (GuidedSectionExtraction)

#define UTILITY_VERSION "v1.0"

#define UTILITY_NAME    "GenCrc32Section"

EFI_GUID  gEfiCrc32SectionGuid = EFI_CRC32_GUIDED_SECTION_EXTRACTION_PROTOCOL_GUID;

EFI_STATUS
SignSectionWithCrc32 (
  IN OUT UINT8  *FileBuffer,
  IN OUT UINT32 *BufferSize,
  IN UINT32     DataSize
  )
/*++
        
Routine Description:
           
  Signs the section with CRC32 and add GUIDed section header for the 
  signed data. data stays in same location (overwrites source data).
            
Arguments:
               
  FileBuffer  - Buffer containing data to sign
                
  BufferSize  - On input, the size of FileBuffer. On output, the size of 
                actual section data (including added section header).              

  DataSize    - Length of data to Sign

  Key         - Key to use when signing. Currently only CRC32 is supported.
                                       
Returns:
                       
  EFI_SUCCESS           - Successful
  EFI_OUT_OF_RESOURCES  - Not enough resource to complete the operation.
                        
--*/
{

  UINT32                Crc32Checksum;
  EFI_STATUS            Status;
  UINT32                TotalSize;
  CRC32_SECTION_HEADER  Crc32Header;
  UINT8                 *SwapBuffer;

  Crc32Checksum = 0;
  SwapBuffer    = NULL;

  if (DataSize == 0) {
    *BufferSize = 0;

    return EFI_SUCCESS;
  }

  Status = CalculateCrc32 (FileBuffer, DataSize, &Crc32Checksum);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  TotalSize = DataSize + CRC32_SECTION_HEADER_SIZE;
  Crc32Header.GuidSectionHeader.CommonHeader.Type     = EFI_SECTION_GUID_DEFINED;
  Crc32Header.GuidSectionHeader.CommonHeader.Size[0]  = (UINT8) (TotalSize & 0xff);
  Crc32Header.GuidSectionHeader.CommonHeader.Size[1]  = (UINT8) ((TotalSize & 0xff00) >> 8);
  Crc32Header.GuidSectionHeader.CommonHeader.Size[2]  = (UINT8) ((TotalSize & 0xff0000) >> 16);
  memcpy (&(Crc32Header.GuidSectionHeader.SectionDefinitionGuid), &gEfiCrc32SectionGuid, sizeof (EFI_GUID));
  Crc32Header.GuidSectionHeader.Attributes  = EFI_GUIDED_SECTION_AUTH_STATUS_VALID;
  Crc32Header.GuidSectionHeader.DataOffset  = CRC32_SECTION_HEADER_SIZE;
  Crc32Header.CRC32Checksum                 = Crc32Checksum;

  SwapBuffer = (UINT8 *) malloc (DataSize);
  if (SwapBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  memcpy (SwapBuffer, FileBuffer, DataSize);
  memcpy (FileBuffer, &Crc32Header, CRC32_SECTION_HEADER_SIZE);
  memcpy (FileBuffer + CRC32_SECTION_HEADER_SIZE, SwapBuffer, DataSize);

  //
  // Make sure section ends on a DWORD boundary
  //
  while ((TotalSize & 0x03) != 0) {
    FileBuffer[TotalSize] = 0;
    TotalSize++;
  }

  *BufferSize = TotalSize;

  if (SwapBuffer != NULL) {
    free (SwapBuffer);
  }

  return EFI_SUCCESS;
}

VOID
PrintUsage (
  VOID
  )
{
  int         Index;
  const char  *Str[] = {
    UTILITY_NAME" "UTILITY_VERSION" - Intel Generate CRC32 Section Utility",
    "  Copyright (C), 2004 - 2008 Intel Corporation",
    
#if ( defined(UTILITY_BUILD) && defined(UTILITY_VENDOR) )
    "  Built from "UTILITY_BUILD", project of "UTILITY_VENDOR,
#endif
    "",
    "Usage:",
    "  "UTILITY_NAME" [OPTION]",
    "Options:",
    "  -i Input1 ...  specifies the input file(s) that would be signed to CRC32",
    "                 Guided section.",
    "  -o Output      specifies the output file that is a CRC32 Guided section",
    NULL
  };
  for (Index = 0; Str[Index] != NULL; Index++) {
    fprintf (stdout, "%s\n", Str[Index]);
  }
}

INT32
ReadFilesContentsIntoBuffer (
  IN      CHAR8   *argv[],
  IN      INT32   Start,
  IN OUT  UINT8   **FileBuffer,
  IN OUT  UINT32  *BufferSize,
  OUT     UINT32  *ContentSize,
  IN      INT32   MaximumArguments
  )
{
  INT32   Index;
  CHAR8   *FileName;
  FILE    *InputFile;
  UINT8   Temp;
  UINT32  Size;

  FileName  = NULL;
  InputFile = NULL;
  Size      = 0;
  Index     = 0;

  //
  // read all input files into one file buffer
  //
  while (argv[Start + Index][0] != '-') {

    FileName  = argv[Start + Index];
    InputFile = fopen (FileName, "rb");
    if (InputFile == NULL) {
      Error (NULL, 0, 0, FileName, "failed to open input binary file");
      return -1;
    }

    fread (&Temp, sizeof (UINT8), 1, InputFile);
    while (!feof (InputFile)) {
      (*FileBuffer)[Size++] = Temp;
      fread (&Temp, sizeof (UINT8), 1, InputFile);
    }

    fclose (InputFile);
    InputFile = NULL;

    //
    // Make sure section ends on a DWORD boundary
    //
    while ((Size & 0x03) != 0) {
      (*FileBuffer)[Size] = 0;
      Size++;
    }

    Index++;
    if (Index == MaximumArguments) {
      break;
    }
  }

  *ContentSize = Size;
  return Index;
}

INT32
main (
  INT32 argc,
  CHAR8 *argv[]
  )
{
  FILE        *OutputFile;
  UINT8       *FileBuffer;
  UINT32      BufferSize;
  EFI_STATUS  Status;
  UINT32      ContentSize;
  CHAR8       *OutputFileName;
  INT32       ReturnValue;
  INT32       Index;

  OutputFile      = NULL;
  FileBuffer      = NULL;
  ContentSize     = 0;
  OutputFileName  = NULL;

  SetUtilityName (UTILITY_NAME);

  if (argc == 1) {
    PrintUsage ();
    return -1;
  }

  BufferSize  = 1024 * 1024 * 16;
  FileBuffer  = (UINT8 *) malloc (BufferSize * sizeof (UINT8));
  if (FileBuffer == NULL) {
    Error (NULL, 0, 0, "memory allocation failed", NULL);
    return -1;
  }

  ZeroMem (FileBuffer, BufferSize);

  for (Index = 0; Index < argc; Index++) {
    if (_strcmpi (argv[Index], "-i") == 0) {
      ReturnValue = ReadFilesContentsIntoBuffer (
                      argv,
                      (Index + 1),
                      &FileBuffer,
                      &BufferSize,
                      &ContentSize,
                      (argc - (Index + 1))
                      );
      if (ReturnValue == -1) {
        Error (NULL, 0, 0, "failed to read file contents", NULL);
        return -1;
      }

      Index += ReturnValue;
    }

    if (_strcmpi (argv[Index], "-o") == 0) {
      OutputFileName = argv[Index + 1];
    }
  }

  OutputFile = fopen (OutputFileName, "wb");
  if (OutputFile == NULL) {
    Error (NULL, 0, 0, OutputFileName, "failed to open output binary file");
    free (FileBuffer);
    return -1;
  }

  /*  
  //
  // make sure section ends on a DWORD boundary ??
  //
  while ( (Size & 0x03) != 0 ) {
    FileBuffer[Size] = 0;
    Size ++;
  }
*/
  Status = SignSectionWithCrc32 (FileBuffer, &BufferSize, ContentSize);
  if (EFI_ERROR (Status)) {
    Error (NULL, 0, 0, "failed to sign section", NULL);
    free (FileBuffer);
    fclose (OutputFile);
    return -1;
  }

  ContentSize = fwrite (FileBuffer, sizeof (UINT8), BufferSize, OutputFile);
  if (ContentSize != BufferSize) {
    Error (NULL, 0, 0, "failed to write output buffer", NULL);
    ReturnValue = -1;
  } else {
    ReturnValue = 0;
  }

  free (FileBuffer);
  fclose (OutputFile);
  return ReturnValue;
}
