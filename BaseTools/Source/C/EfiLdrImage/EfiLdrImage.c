/** @file
Creates and EFILDR image.
This tool combines several PE Image files together using following format denoted as EBNF:
FILE := EFILDR_HEADER
        EFILDR_IMAGE +
        <PeImageFileContent> +
The order of EFILDR_IMAGE is same as the order of placing PeImageFileContent.
  
Copyright (c) 2006 - 2014, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ParseInf.h"
#include "CommonLib.h"
#include "EfiUtilityMsgs.h"

#define MAX_PE_IMAGES                  63
#define FILE_TYPE_FIXED_LOADER         0
#define FILE_TYPE_RELOCATABLE_PE_IMAGE 1

typedef struct {
  UINT32 CheckSum;
  UINT32 Offset;
  UINT32 Length;
  UINT8  FileName[52];
} EFILDR_IMAGE;

typedef struct {          
  UINT32       Signature;     
  UINT32       HeaderCheckSum;
  UINT32       FileLength;
  UINT32       NumberOfImages;
} EFILDR_HEADER;

//
// Utility Name
//
#define UTILITY_NAME  "EfiLdrImage"

//
// Utility version information
//
#define UTILITY_MAJOR_VERSION 0
#define UTILITY_MINOR_VERSION 1

void
Version (
  void
  )
/*++

Routine Description:

  Displays the standard utility information to SDTOUT

Arguments:

  None

Returns:

  None

--*/
{
  printf ("%s Version %d.%d %s\n", UTILITY_NAME, UTILITY_MAJOR_VERSION, UTILITY_MINOR_VERSION, __BUILD_VERSION);
  printf ("Copyright (c) 1999-2014 Intel Corporation. All rights reserved.\n");
  printf ("\n  The EfiLdrImage tool is used to combine PE files into EFILDR image with Efi loader header.\n");
}

VOID
Usage (
  VOID
  )
{
  printf ("Usage: EfiLdrImage -o OutImage LoaderImage PeImage1 PeImage2 ... PeImageN\n");
  exit (1);
}

EFI_STATUS
CountVerboseLevel (
  IN CONST CHAR8* VerboseLevelString,
  IN CONST UINT64 Length,
  OUT UINT64 *ReturnValue
)
{
  UINT64 i = 0;
  for (;i < Length; ++i) {
    if (VerboseLevelString[i] != 'v' && VerboseLevelString[i] != 'V') {
      return EFI_ABORTED;
    }
    ++(*ReturnValue);
  }
  
  return EFI_SUCCESS;
}

UINT64
FCopyFile (
  FILE    *in,
  FILE    *out
  )
/*++
Routine Description:
  Write all the content of input file to output file.

Arguments:
  in  - input file pointer
  out - output file pointer

Return:
  UINT64 : file size of input file
--*/
{
  UINT32          filesize, offset, length;
  CHAR8           Buffer[8*1024];

  fseek (in, 0, SEEK_END);
  filesize = ftell(in);

  fseek (in, 0, SEEK_SET);

  offset = 0;
  while (offset < filesize)  {
    length = sizeof(Buffer);
    if (filesize-offset < length) {
      length = filesize-offset;
    }

    fread (Buffer, length, 1, in);
    fwrite (Buffer, length, 1, out);
    offset += length;
  }

  return filesize;
}


int
main (
  int argc,
  char *argv[]
  )
/*++

Routine Description:


Arguments:


Returns:


--*/
{
  UINT64         i;
  UINT64         filesize;
  FILE          *fpIn, *fpOut;
  EFILDR_HEADER EfiLdrHeader;
  EFILDR_IMAGE  EfiLdrImage[MAX_PE_IMAGES];
  CHAR8* OutputFileName = NULL;
  CHAR8* InputFileNames[MAX_PE_IMAGES + 1];
  UINT8 InputFileCount = 0;
  UINT64        DebugLevel = 0;
  UINT64        VerboseLevel = 0;
  EFI_STATUS Status = EFI_SUCCESS;
  
  SetUtilityName (UTILITY_NAME);

  if (argc == 1) {
    Usage();
    return STATUS_ERROR;
  }
  
  argc --;
  argv ++;

  if ((stricmp (argv[0], "-h") == 0) || (stricmp (argv[0], "--help") == 0)) {
    Usage();
    return STATUS_SUCCESS;    
  }

  if (stricmp (argv[0], "--version") == 0) {
    Version();
    return STATUS_SUCCESS;    
  }

  while (argc > 0) {
   
    if ((stricmp (argv[0], "-o") == 0) || (stricmp (argv[0], "--output") == 0)) {
      OutputFileName = argv[1];
      if (OutputFileName == NULL) {
        Error (NULL, 0, 1003, "Invalid option value", "Output file can't be null");
        return STATUS_ERROR;
      }
      argc -= 2;
      argv += 2;
      continue; 
    }
    
    if ((stricmp (argv[0], "-q") == 0) || (stricmp (argv[0], "--quiet") == 0)) {
      argc --;
      argv ++;
      continue; 
    }
    
    if ((strlen(argv[0]) >= 2 && argv[0][0] == '-' && (argv[0][1] == 'v' || argv[0][1] == 'V')) || (stricmp (argv[0], "--verbose") == 0)) {
      VerboseLevel = 1;
      if (strlen(argv[0]) > 2) {
        Status = CountVerboseLevel (&argv[0][2], strlen(argv[0]) - 2, &VerboseLevel);
        if (EFI_ERROR (Status)) {
          Error (NULL, 0, 1003, "Invalid option value", argv[0]);
          return STATUS_ERROR;        
        }
      }
      
      argc --;
      argv ++;
      continue; 
    }
    
    if ((stricmp (argv[0], "-d") == 0) || (stricmp (argv[0], "--debug") == 0)) {
      Status = AsciiStringToUint64 (argv[1], FALSE, &DebugLevel);
      if (EFI_ERROR (Status)) {
        Error (NULL, 0, 1003, "Invalid option value", "%s = %s", argv[0], argv[1]);
        return STATUS_ERROR;        
      }
      argc -= 2;
      argv += 2;
      continue; 
    }
    //
    // Don't recognize the parameter, should be regarded as the input file name.
    //
    InputFileNames[InputFileCount] = argv[0];
    InputFileCount++;
    argc--;
    argv++;
  }

  if (InputFileCount == 0) {
    Error (NULL, 0, 1001, "Missing option", "No input file");
    return STATUS_ERROR;
  }
  //
  // Open output file for write
  //
  if (OutputFileName == NULL) {
    Error (NULL, 0, 1001, "Missing option", "No output file");
    return STATUS_ERROR;
  }

  fpOut = fopen (LongFilePath (OutputFileName), "w+b");
  if (!fpOut) {
    Error (NULL, 0, 0001, "Could not open output file", OutputFileName);
    return STATUS_ERROR;
  }

  memset (&EfiLdrHeader, 0, sizeof (EfiLdrHeader));
  memset (&EfiLdrImage, 0, sizeof (EFILDR_IMAGE) * (InputFileCount));

  memcpy (&EfiLdrHeader.Signature, "EFIL", 4);
  EfiLdrHeader.FileLength = sizeof(EFILDR_HEADER) + sizeof(EFILDR_IMAGE)*(InputFileCount);

  //
  // Skip the file header first
  //
  fseek (fpOut, EfiLdrHeader.FileLength, SEEK_SET);

  //
  // copy all the input files to the output file
  //
  for(i=0;i<InputFileCount;i++) {
    //
    // Copy the content of PeImage file to output file
    //
    fpIn = fopen (LongFilePath (InputFileNames[i]), "rb");
    if (!fpIn) {
      Error (NULL, 0, 0001, "Could not open input file", InputFileNames[i]);
      fclose (fpOut);
      return STATUS_ERROR;
    }
    filesize = FCopyFile (fpIn, fpOut);
    fclose(fpIn);

    //
    //  And in the same time update the EfiLdrHeader and EfiLdrImage array
    //
    EfiLdrImage[i].Offset = EfiLdrHeader.FileLength;
    EfiLdrImage[i].Length = (UINT32) filesize;
    strncpy ((CHAR8*) EfiLdrImage[i].FileName, InputFileNames[i], sizeof (EfiLdrImage[i].FileName) - 1);
    EfiLdrHeader.FileLength += (UINT32) filesize;
    EfiLdrHeader.NumberOfImages++;
  }

  //
  // Write the image header to the output file finally
  //
  fseek (fpOut, 0, SEEK_SET);
  fwrite (&EfiLdrHeader, sizeof(EFILDR_HEADER)        , 1, fpOut);
  fwrite (&EfiLdrImage , sizeof(EFILDR_IMAGE)*(InputFileCount), 1, fpOut);

  fclose (fpOut);
  printf ("Created %s\n", OutputFileName);
  return 0;
}

