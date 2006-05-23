/*++

Copyright (c)  2001 - 2002 Intel Corporation. All rights reserved
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.


Module Name:
  ZeroDebugData.c

Abstract:
  Zero the Debug Data Fields of Portable Executable (PE) format file.

--*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void
PrintUsage (
  void
  )
/*++
Routine Description:
  print usage of ZeroDebugData command

Arguments:
  None

Returns:
  None
--*/
// GC_TODO:    void - add argument and description to function comment
{
  //
  // print usage of command
  //
  printf ("\nUsage: ZeroDebugData <PE-File> [DebugData-File]\n");
}

int
ReadFromFile (
  FILE      *fp,
  long      offset,
  void      *buffer,
  int       size
  )
/*++
Routine Description:
  read data from a specified location of file

Arguments:
  fp              - file pointer
  offset          - number of bytes from beginning of file
  buffer          - buffer used to store data
  size            - size of buffer

Returns:
  =  0            - Success
  = -1            - Failed
--*/
{
  //
  // set file pointer to the specified location of file
  //
  if (fseek (fp, offset, SEEK_SET) != 0) {
    printf ("Error: Cannot move the current location of the file.\n");
    return -1;
  }
  //
  // read data from the file
  //
  if (fread (buffer, size, 1, fp) != 1) {
    printf ("Error: Cannot read data from the file.\n");
    return -1;
  }

  return 0;
}

int
ZeroDebugData (
  FILE      *fp,
  FILE      *fpData
  )
/*++

Routine Description:

  Zero the debug data fields of the file

Arguments:

  fp              - file pointer
  fpData          - pointer to output file that ZeroDebugData progress is written to

Returns:

  =  0            - Success
  = -1            - Failed

--*/
{
  unsigned char header[4];
  unsigned long offset;
  unsigned long NumberOfRvaAndSizes;
  unsigned int  nvalue;
  unsigned long lvalue;
  unsigned long Size;
  unsigned long Pointer;
  unsigned char *Buffer;
  unsigned long Index;

  //
  // read the header of file
  //
  if (ReadFromFile (fp, 0, header, 2) != 0) {
    printf ("Error: open image file\n");
    return -1;
  }
  //
  // "MZ" -- the header of image file (PE)
  //
  if (strncmp ((char *) header, "MZ", 2) != 0) {
    printf ("Error: Invalid Image file.\n");
    return -1;
  }
  //
  // At location 0x3C, the stub has the file offset to the
  // PE signature.
  //
  if (ReadFromFile (fp, 0x3C, &offset, 4) != 0) {
    return -1;
  }
  //
  // read the header of optional
  //
  if (ReadFromFile (fp, offset, header, 4) != 0) {
    return -1;
  }
  //
  // "PE\0\0" -- the signature of optional header
  //
  if (strncmp ((char *) header, "PE\0\0", 4) != 0) {
    printf ("Error: Invalid PE format file.\n");
    return -1;
  }
  //
  // Add 16 to skip COFF file header, and get to optional header.
  //
  offset += 24;

  //
  // Check the magic field, 0x10B for PE32 and 0x20B for PE32+
  //
  if (ReadFromFile (fp, offset, &nvalue, 2) != 0) {
    return -1;
  }
  //
  // If this is PE32 image file, offset of NumberOfRvaAndSizes is 92.
  // Else it is 108.
  //
  switch (nvalue & 0xFFFF) {
  case 0x10B:
    offset += 92;
    printf ("Info: Image is PE32. ");
    break;

  case 0x20B:
    offset += 108;
    printf ("Info: Image is PE32+. ");
    break;

  default:
    printf ("Error: Magic value is unknown.\n");
    return -1;
  }
  //
  // get the value of NumberOfRvaAndSizes
  //
  if (ReadFromFile (fp, offset, &NumberOfRvaAndSizes, 4) != 0) {
    printf ("Error: read NumberOfRvaAndSizes error.\n");
    return -1;
  }
  //
  // printf ("Info: NumberOfRvaAndSizes = %d\n", NumberOfRvaAndSizes);
  //
  //
  // Finding Debug Table, offset of Debug Table
  // is 4 + 6 * 8 = 52.
  //
  if (NumberOfRvaAndSizes >= 7) {
    if (ReadFromFile (fp, offset + 52, &lvalue, 4) != 0) {
      return -1;
    }
    //
    // Read the SizeOfData(16) and PointerToRawData(24)
    //
    if (ReadFromFile (fp, lvalue + 16, &Size, 4) != 0) {
      printf ("error: Size = %d\n", Size);
      return -1;
    }

    printf ("Debug data: size = %xh, ", Size);
    fprintf (fpData, "Debug data: size = %xh, ", Size);

    if (ReadFromFile (fp, lvalue + 20, &Pointer, 4) != 0) {
      printf ("error: LoadOffset = %xh\n", Pointer);
      return -1;
    }
    //
    // printf ("LoadOffset = %xh, ", Pointer);
    //
    fprintf (fpData, "LoadOffset = %xh, ", Pointer);

    if (ReadFromFile (fp, lvalue + 24, &Pointer, 4) != 0) {
      printf ("error: FileOffset = %xh\n", Pointer);
      return -1;
    }

    printf ("FileOffset = %xh, ", Pointer);
    fprintf (fpData, "FileOffset = %xh, \n", Pointer);

    if ((lvalue != 0) && (Pointer != 0)) {
      //
      // prepare buffer
      //
      Buffer = malloc (Size + 1);
      if (Buffer == NULL) {
        printf ("Error: Cannot allocate memory.\n");
        return -1;
      }
      //
      // set file pointer to the specified location of file
      //
      if (fseek (fp, Pointer, SEEK_SET) != 0) {
        printf ("Error: Cannot move the current location of the file.\n");
        free (Buffer);
        return -1;
      }
      //
      // read data from PE file
      //
      if (fread (Buffer, Size, 1, fp) != 1) {
        printf ("Error: Cannot read data from the file.\n");
        free (Buffer);
        return -1;
      }
      //
      // write to data file
      //
      for (Index = 0; Index < Size;) {
        fprintf (fpData, "%02x ", Buffer[Index]);

        Index++;
        if (Index % 8 == 0) {
          fprintf (fpData, "\n");
        }
      }

      fprintf (fpData, "\n");

      //
      // zero buffer and write back to PE file
      //
      if (fseek (fp, Pointer, SEEK_SET) != 0) {
        printf ("Error: Cannot move the current location of the file.\n");
        free (Buffer);
        return -1;
      }

      memset (Buffer, 0, Size);
      if (fwrite (Buffer, Size, 1, fp) != 1) {
        perror ("Error: Cannot write zero to the file.\n");
        free (Buffer);
        return -1;
      }
      //
      // set file pointer to the specified location of file
      //
      if (fseek (fp, lvalue + 4, SEEK_SET) != 0) {
        printf ("Error: Cannot move the current location of the file.\n");
        free (Buffer);
        return -1;
      }

      if (fwrite (Buffer, 4, 1, fp) != 1) {
        perror ("Error: Cannot write zero to the file.\n");
        free (Buffer);
        return -1;
      }

      free (Buffer);
    }
  }

  return 0;
}

int
main (
  int       argc,
  char      *argv[]
  )
/*++

Routine Description:

  Prints the zero debug data of the PE file to the DebugData file.
  Executes the ZeroDebugData function.

Arguments:

  argc   - Standard C argument, number of command line arguments.
  argv[] - Standard C argument, array of pointers to the input files,
           such as the PE and DebugData files.

Returns:

  zero    - success
  nonzero - failure

--*/
{
  FILE  *fp;
  FILE  *fpData;
  char  DataFile[1024] = "";

  //
  // check the number of parameters
  //
  if (argc < 2) {
    printf ("\nUsage: ZeroDebugData <PE-File> [DebugData-File]\n");
    return -1;
  }
  //
  // open the DebugData file, if not exists, return
  //
  if (argc >= 3) {
    strcpy (DataFile, argv[2]);
  } else {
    strcpy (DataFile, "DebugData.dat");
  }

  fpData = fopen (DataFile, "a+");
  if (fpData == NULL) {
    fpData = fopen (DataFile, "w");
    if (fpData == NULL) {
      printf ("Error: Cannot open the data file!\n");
      return -1;
    }
  }
  //
  // open the PE file
  //
  fp = fopen (argv[1], "r+b");
  if (fp == NULL) {
    printf ("Error: Cannot open the PE file!\n");
    return -1;
  }
  //
  // Zero the Debug Data to the PE file
  //
  printf ("Zero Debug Data to file %s:\n", argv[1]);
  fprintf (fpData, "\nZero Debug Data to file %s:\n", argv[1]);
  if ((int *) ZeroDebugData (fp, fpData) != 0) {
    printf ("Error: Zero Debug Data PE file\n");
    fclose (fp);
    return -1;
  }

  printf (" success\n");

  //
  // close the PE file
  //
  fflush (fpData);
  fflush (fp);
  fclose (fpData);
  fclose (fp);

  return 0;
}
