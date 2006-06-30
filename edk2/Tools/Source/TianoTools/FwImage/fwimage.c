/*++

Copyright (c) 2004, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

    fwimage.c

Abstract:

    Converts a pe32+ image to an FW image type

--*/

#include "WinNtInclude.h"

#ifndef __GNUC__
#include <windows.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <Common/UefiBaseTypes.h>
#include <Common/EfiImage.h>

#include "CommonLib.h"
#include "EfiUtilityMsgs.c"

#define UTILITY_NAME  "FwImage"

#ifdef __GNUC__
typedef unsigned long ULONG;
typedef unsigned char UCHAR;
typedef unsigned char *PUCHAR;
typedef unsigned short USHORT;
#endif

VOID
Usage (
  VOID
  )
{
  printf ("Usage: " UTILITY_NAME "  {-t time-date} [BASE|SEC|PEI_CORE|PEIM|DXE_CORE|DXE_DRIVER|DXE_RUNTIME_DRIVER|DXE_SAL_DRIVER|DXE_SMM_DRIVER|TOOL|UEFI_DRIVER|UEFI_APPLICATION|USER_DEFINED] peimage [outimage]");
}

static
STATUS
FCopyFile (
  FILE    *in,
  FILE    *out
  )
{
  ULONG filesize;
  ULONG offset;
  ULONG length;
  UCHAR Buffer[8 * 1024];

  fseek (in, 0, SEEK_END);
  filesize = ftell (in);

  fseek (in, 0, SEEK_SET);
  fseek (out, 0, SEEK_SET);

  offset = 0;
  while (offset < filesize) {
    length = sizeof (Buffer);
    if (filesize - offset < length) {
      length = filesize - offset;
    }

    fread (Buffer, length, 1, in);
    fwrite (Buffer, length, 1, out);
    offset += length;
  }

  if ((ULONG) ftell (out) != filesize) {
    Error (NULL, 0, 0, "write error", NULL);
    return STATUS_ERROR;
  }

  return STATUS_SUCCESS;
}

int
main (
  int  argc,
  char *argv[]
  )
/*++

Routine Description:

  Main function.

Arguments:

  argc - Number of command line parameters.
  argv - Array of pointers to command line parameter strings.

Returns:
  STATUS_SUCCESS - Utility exits successfully.
  STATUS_ERROR   - Some error occurred during execution.

--*/
{
  ULONG             Type;
  PUCHAR            Ext;
  PUCHAR            p;
  PUCHAR            pe;
  PUCHAR            OutImageName;
  UCHAR             outname[500];
  FILE              *fpIn;
  FILE              *fpOut;
  EFI_IMAGE_DOS_HEADER  DosHdr;
  EFI_IMAGE_NT_HEADERS  PeHdr;
  time_t            TimeStamp;
  struct tm         TimeStruct;
  EFI_IMAGE_DOS_HEADER  BackupDosHdr;
  ULONG             Index;
  BOOLEAN           TimeStampPresent;

  SetUtilityName (UTILITY_NAME);
  //
  // Assign to fix compile warning
  //
  OutImageName      = NULL;
  Type              = 0;
  Ext               = 0;
  TimeStamp         = 0;
  TimeStampPresent  = FALSE;

  //
  // Look for -t time-date option first. If the time is "0", then
  // skip it.
  //
  if ((argc > 2) && !strcmp (argv[1], "-t")) {
    TimeStampPresent = TRUE;
    if (strcmp (argv[2], "0") != 0) {
      //
      // Convert the string to a value
      //
      memset ((char *) &TimeStruct, 0, sizeof (TimeStruct));
      if (sscanf(
          argv[2], "%d/%d/%d,%d:%d:%d",
          &TimeStruct.tm_mon,   /* months since January - [0,11] */
          &TimeStruct.tm_mday,  /* day of the month - [1,31] */
          &TimeStruct.tm_year,  /* years since 1900 */
          &TimeStruct.tm_hour,  /* hours since midnight - [0,23] */
          &TimeStruct.tm_min,   /* minutes after the hour - [0,59] */
          &TimeStruct.tm_sec    /* seconds after the minute - [0,59] */
            ) != 6) {
        Error (NULL, 0, 0, argv[2], "failed to convert to mm/dd/yyyy,hh:mm:ss format");
        return STATUS_ERROR;
      }
      //
      // Now fixup some of the fields
      //
      TimeStruct.tm_mon--;
      TimeStruct.tm_year -= 1900;
      //
      // Sanity-check values?
      // Convert
      //
      TimeStamp = mktime (&TimeStruct);
      if (TimeStamp == (time_t) - 1) {
        Error (NULL, 0, 0, argv[2], "failed to convert time");
        return STATUS_ERROR;
      }
    }
    //
    // Skip over the args
    //
    argc -= 2;
    argv += 2;
  }
  //
  // Check for enough args
  //
  if (argc < 3) {
    Usage ();
    return STATUS_ERROR;
  }

  if (argc == 4) {
    OutImageName = argv[3];
  }
  //
  // Get new image type
  //
  p = argv[1];
  if (*p == '/' || *p == '\\') {
    p += 1;
  }

  if (stricmp (p, "app") == 0 || stricmp (p, "UEFI_APPLICATION") == 0) {
    Type  = EFI_IMAGE_SUBSYSTEM_EFI_APPLICATION;
    Ext   = ".efi";

  } else if (stricmp (p, "bsdrv") == 0 || stricmp (p, "DXE_DRIVER") == 0) {
    Type  = EFI_IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER;
    Ext   = ".efi";

  } else if (stricmp (p, "rtdrv") == 0 || stricmp (p, "DXE_RUNTIME_DRIVER") == 0) {
    Type  = EFI_IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER;
    Ext   = ".efi";

  } else if (stricmp (p, "rtdrv") == 0 || stricmp (p, "DXE_SAL_DRIVER") == 0) {
    Type  = EFI_IMAGE_SUBSYSTEM_SAL_RUNTIME_DRIVER;
    Ext   = ".efi";
  } else if (stricmp (p, "SEC") == 0) {
    Type  = EFI_IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER;
    Ext   = ".sec";
  } else if (stricmp (p, "peim") == 0 ||
           stricmp (p, "BASE") == 0 ||
           stricmp (p, "PEI_CORE") == 0 ||
           stricmp (p, "PEIM") == 0 ||
           stricmp (p, "DXE_SMM_DRIVER") == 0 ||
           stricmp (p, "TOOL") == 0 ||
           stricmp (p, "UEFI_APPLICATION") == 0 ||
           stricmp (p, "USER_DEFINED") == 0 ||
           stricmp (p, "UEFI_DRIVER") == 0 ||
           stricmp (p, "DXE_CORE") == 0
          ) {
    Type  = EFI_IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER;
    Ext   = ".pei";
  } else {
  	printf ("%s", p);
    Usage ();
    return STATUS_ERROR;
  }
  //
  // open source file
  //
  fpIn = fopen (argv[2], "rb");
  if (!fpIn) {
    Error (NULL, 0, 0, argv[2], "failed to open input file for reading");
    return STATUS_ERROR;
  }
  //
  // Read the dos & pe hdrs of the image
  //
  fseek (fpIn, 0, SEEK_SET);
  fread (&DosHdr, sizeof (DosHdr), 1, fpIn);
  if (DosHdr.e_magic != EFI_IMAGE_DOS_SIGNATURE) {
    Error (NULL, 0, 0, argv[2], "DOS header signature not found in source image");
    fclose (fpIn);
    return STATUS_ERROR;
  }

  fseek (fpIn, DosHdr.e_lfanew, SEEK_SET);
  fread (&PeHdr, sizeof (PeHdr), 1, fpIn);
  if (PeHdr.Signature != EFI_IMAGE_NT_SIGNATURE) {
    Error (NULL, 0, 0, argv[2], "PE header signature not found in source image");
    fclose (fpIn);
    return STATUS_ERROR;
  }
  //
  // open output file
  //
  strcpy (outname, argv[2]);
  pe = NULL;
  for (p = outname; *p; p++) {
    if (*p == '.') {
      pe = p;
    }
  }

  if (!pe) {
    pe = p;
  }

  strcpy (pe, Ext);

  if (!OutImageName) {
    OutImageName = outname;
  }

  fpOut = fopen (OutImageName, "w+b");
  if (!fpOut) {
    Error (NULL, 0, 0, OutImageName, "could not open output file for writing");
    fclose (fpIn);
    return STATUS_ERROR;
  }
  //
  // Copy the file
  //
  if (FCopyFile (fpIn, fpOut) != STATUS_SUCCESS) {
    fclose (fpIn);
    fclose (fpOut);
    return STATUS_ERROR;
  }
  //
  // Zero all unused fields of the DOS header
  //
  memcpy (&BackupDosHdr, &DosHdr, sizeof (DosHdr));
  memset (&DosHdr, 0, sizeof (DosHdr));
  DosHdr.e_magic  = BackupDosHdr.e_magic;
  DosHdr.e_lfanew = BackupDosHdr.e_lfanew;
  fseek (fpOut, 0, SEEK_SET);
  fwrite (&DosHdr, sizeof (DosHdr), 1, fpOut);

  fseek (fpOut, sizeof (DosHdr), SEEK_SET);
  for (Index = sizeof (DosHdr); Index < (ULONG) DosHdr.e_lfanew; Index++) {
    fwrite (&DosHdr.e_cp, 1, 1, fpOut);
  }
  //
  // Path the PE header
  //
  PeHdr.OptionalHeader.Subsystem = (USHORT) Type;
  if (TimeStampPresent) {
    PeHdr.FileHeader.TimeDateStamp = (UINT32) TimeStamp;
  }

  PeHdr.OptionalHeader.SizeOfStackReserve = 0;
  PeHdr.OptionalHeader.SizeOfStackCommit  = 0;
  PeHdr.OptionalHeader.SizeOfHeapReserve  = 0;
  PeHdr.OptionalHeader.SizeOfHeapCommit   = 0;
  fseek (fpOut, DosHdr.e_lfanew, SEEK_SET);
  fwrite (&PeHdr, sizeof (PeHdr), 1, fpOut);

  //
  // Done
  //
  fclose (fpIn);
  fclose (fpOut);
  //
  // printf ("Created %s\n", OutImageName);
  //
  return STATUS_SUCCESS;
}
