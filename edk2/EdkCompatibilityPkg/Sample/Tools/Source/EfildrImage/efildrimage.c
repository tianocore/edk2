/*++

Copyright 2006 - 2007, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  efildrimage.c

Abstract:

  Creates and EFILDR image.
  This tool combines several PE Image files together using following format denoted as EBNF:
  FILE := EFILDR_HEADER
          EFILDR_IMAGE +
          <PeImageFileContent> +
  The order of EFILDR_IMAGE is same as the order of placing PeImageFileContent.

Revision History

--*/


#include <windows.h>
#include <stdio.h>
#include "Tiano.h"

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



VOID
Usage (
  VOID
  )
{
  printf ("Usage: EfiLdrImage OutImage LoaderImage PeImage1 PeImage2 ... PeImageN");
  exit (1);
}

ULONG
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
  ULONG : file size of input file
--*/
{
  ULONG           filesize, offset, length;
  UCHAR           Buffer[8*1024];

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
  ULONG         i;
  ULONG         filesize;
  FILE          *fpIn, *fpOut;
  EFILDR_HEADER EfiLdrHeader;
  EFILDR_IMAGE  EfiLdrImage[MAX_PE_IMAGES];

  if (argc < 4) {
    Usage();
  }

  //
  // Open output file for write
  //
  fpOut = fopen(argv[1], "w+b");
  if (!fpOut) {
    printf ("efildrimage: Could not open output file %s\n", argv[1]);
    exit(1);
  }

  memset (&EfiLdrHeader, 0, sizeof (EfiLdrHeader));
  memset (&EfiLdrImage, 0, sizeof (EFILDR_IMAGE) * (argc - 2));

  memcpy (&EfiLdrHeader.Signature, "EFIL", 4);
  EfiLdrHeader.FileLength = sizeof(EFILDR_HEADER) + sizeof(EFILDR_IMAGE)*(argc-2);

  //
  // Skip the file header first
  //
  fseek (fpOut, EfiLdrHeader.FileLength, SEEK_SET);

  //
  // copy all the input files to the output file
  //
  for(i=2;i<(ULONG)argc;i++) {
    //
    // Copy the content of PeImage file to output file
    //
    fpIn = fopen (argv[i], "rb");
    if (!fpIn) {
      printf ("efildrimage: Could not open input file %s\n", argv[i-2]);
      exit(1);
    }
    filesize = FCopyFile (fpIn, fpOut);
    fclose(fpIn);

    //
    //  And in the same time update the EfiLdrHeader and EfiLdrImage array
    //
    EfiLdrImage[i-2].Offset = EfiLdrHeader.FileLength;
    EfiLdrImage[i-2].Length = filesize;
    strncpy (EfiLdrImage[i-2].FileName, argv[i], sizeof (EfiLdrImage[i-2].FileName) - 1);
    EfiLdrHeader.FileLength += filesize;
    EfiLdrHeader.NumberOfImages++;
  }

  //
  // Write the image header to the output file finally
  //
  fseek (fpOut, 0, SEEK_SET);
  fwrite (&EfiLdrHeader, sizeof(EFILDR_HEADER)        , 1, fpOut);
  fwrite (&EfiLdrImage , sizeof(EFILDR_IMAGE)*(argc-2), 1, fpOut);

  fclose (fpOut);
  printf ("Created %s\n", argv[1]);
  return 0;
}

