/*++

Copyright (c) 1999 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  GenTEImage.c

Abstract:

  Utility program to shrink a PE32 image down by replacing
  the DOS, PE, and optional headers with a minimal header.

--*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "Tiano.h"
#include "TianoCommon.h"
#include "EfiImage.h" // for PE32 structure definitions
#include "EfiUtilityMsgs.h"

//
// Version of this utility
//
#define UTILITY_NAME    "GenTEImage"
#define UTILITY_VERSION "v1.0"

//
// Define the max length of a filename
//
#define MAX_PATH                  256
#define DEFAULT_OUTPUT_EXTENSION  ".te"

//
// Use this to track our command-line options and globals
//
struct {
  INT8  OutFileName[MAX_PATH];
  INT8  InFileName[MAX_PATH];
  INT8  Verbose;
  INT8  Dump;
} mOptions;

//
// Use these to convert from machine type value to a named type
//
typedef struct {
  UINT16  Value;
  INT8    *Name;
} STRING_LOOKUP;

static STRING_LOOKUP  mMachineTypes[] = {
  EFI_IMAGE_MACHINE_IA32,
  "IA32",
  EFI_IMAGE_MACHINE_IA64,
  "IA64",
  EFI_IMAGE_MACHINE_X64,
  "X64",
  EFI_IMAGE_MACHINE_EBC,
  "EBC",
  0,
  NULL
};

static STRING_LOOKUP  mSubsystemTypes[] = {
  EFI_IMAGE_SUBSYSTEM_EFI_APPLICATION,
  "EFI application",
  EFI_IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER,
  "EFI boot service driver",
  EFI_IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER,
  "EFI runtime driver",
  0,
  NULL
};
//
//  Function prototypes
//
static
void
Usage (
  VOID
  );

static
STATUS
ParseCommandLine (
  int       Argc,
  char      *Argv[]
  );

static
STATUS
CheckPE32File (
  INT8      *FileName,
  FILE      *Fptr,
  UINT16    *MachineType,
  UINT16    *SubSystem
  );

static
STATUS
ProcessFile (
  INT8      *InFileName,
  INT8      *OutFileName
  );

static
void
DumpImage (
  INT8      *FileName
  );

static
INT8                  *
GetMachineTypeStr (
  UINT16    MachineType
  );

static
INT8                  *
GetSubsystemTypeStr (
  UINT16  SubsystemType
  );

int
main (
  int   Argc,
  char  *Argv[]
  )
/*++

Routine Description:
  

Arguments:

  Argc            - standard C main() argument count

  Argv            - standard C main() argument list

Returns:

  0             success
  non-zero      otherwise

--*/
// GC_TODO:    ] - add argument and description to function comment
{
  INT8    *Ext;
  UINT32  Status;

  SetUtilityName (UTILITY_NAME);
  //
  // Parse the command line arguments
  //
  if (ParseCommandLine (Argc, Argv)) {
    return STATUS_ERROR;
  }
  //
  // If dumping an image, then do that and quit
  //
  if (mOptions.Dump) {
    DumpImage (mOptions.InFileName);
    goto Finish;
  }
  //
  // Determine the output filename. Either what they specified on
  // the command line, or the first input filename with a different extension.
  //
  if (!mOptions.OutFileName[0]) {
    strcpy (mOptions.OutFileName, mOptions.InFileName);
    //
    // Find the last . on the line and replace the filename extension with
    // the default
    //
    for (Ext = mOptions.OutFileName + strlen (mOptions.OutFileName) - 1;
         (Ext >= mOptions.OutFileName) && (*Ext != '.') && (*Ext != '\\');
         Ext--
        )
      ;
    //
    // If dot here, then insert extension here, otherwise append
    //
    if (*Ext != '.') {
      Ext = mOptions.OutFileName + strlen (mOptions.OutFileName);
    }

    strcpy (Ext, DEFAULT_OUTPUT_EXTENSION);
  }
  //
  // Make sure we don't have the same filename for input and output files
  //
  if (_stricmp (mOptions.OutFileName, mOptions.InFileName) == 0) {
    Error (NULL, 0, 0, mOptions.OutFileName, "input and output file names must be different");
    goto Finish;
  }
  //
  // Process the file
  //
  ProcessFile (mOptions.InFileName, mOptions.OutFileName);
Finish:
  Status = GetUtilityStatus ();
  return Status;
}

static
STATUS
ProcessFile (
  INT8      *InFileName,
  INT8      *OutFileName
  )
/*++

Routine Description:
  
  Process a PE32 EFI file.

Arguments:
  
  InFileName      - the file name pointer to the input file
  OutFileName     - the file name pointer to the output file

Returns:

  STATUS_SUCCESS  - the process has been finished successfully
  STATUS_ERROR    - error occured during the processing

--*/
{
  STATUS                      Status;
  FILE                        *InFptr;
  FILE                        *OutFptr;
  UINT16                      MachineType;
  UINT16                      SubSystem;
  EFI_TE_IMAGE_HEADER         TEImageHeader;
  UINT32                      PESigOffset;
  EFI_IMAGE_FILE_HEADER       FileHeader;
  EFI_IMAGE_OPTIONAL_HEADER32 OptionalHeader32;
  EFI_IMAGE_OPTIONAL_HEADER64 OptionalHeader64;
  UINT32                      BytesStripped;
  UINT32                      FileSize;
  UINT8                       *Buffer;
  long                        SaveFilePosition;

  InFptr  = NULL;
  OutFptr = NULL;
  Buffer  = NULL;
  Status  = STATUS_ERROR;

  //
  // Try to open the input file
  //
  if ((InFptr = fopen (InFileName, "rb")) == NULL) {
    Error (NULL, 0, 0, InFileName, "failed to open input file for reading");
    return STATUS_ERROR;
  }
  //
  // Double-check the file to make sure it's what we expect it to be
  //
  if (CheckPE32File (InFileName, InFptr, &MachineType, &SubSystem) != STATUS_SUCCESS) {
    goto Finish;
  }
  //
  // Initialize our new header
  //
  memset (&TEImageHeader, 0, sizeof (EFI_TE_IMAGE_HEADER));

  //
  // Seek to the end to get the file size
  //
  fseek (InFptr, 0, SEEK_END);
  FileSize = ftell (InFptr);
  fseek (InFptr, 0, SEEK_SET);

  //
  // Per the PE/COFF specification, at offset 0x3C in the file is a 32-bit
  // offset (from the start of the file) to the PE signature, which always
  // follows the MSDOS stub. The PE signature is immediately followed by the
  // COFF file header.
  //
  //
  if (fseek (InFptr, 0x3C, SEEK_SET) != 0) {
    Error (NULL, 0, 0, InFileName, "failed to seek to PE signature in file", NULL);
    goto Finish;
  }

  if (fread (&PESigOffset, sizeof (PESigOffset), 1, InFptr) != 1) {
    Error (NULL, 0, 0, InFileName, "failed to read PE signature offset from file");
    goto Finish;
  }

  if (fseek (InFptr, PESigOffset + 4, SEEK_SET) != 0) {
    Error (NULL, 0, 0, InFileName, "failed to seek to PE signature");
    goto Finish;
  }
  //
  // We should now be at the COFF file header. Read it in and verify it's
  // of an image type we support.
  //
  if (fread (&FileHeader, sizeof (EFI_IMAGE_FILE_HEADER), 1, InFptr) != 1) {
    Error (NULL, 0, 0, InFileName, "failed to read file header from image");
    goto Finish;
  }

  if ((FileHeader.Machine != EFI_IMAGE_MACHINE_IA32) &&
      (FileHeader.Machine != EFI_IMAGE_MACHINE_X64) &&
      (FileHeader.Machine != EFI_IMAGE_MACHINE_IA64)) {
    Error (NULL, 0, 0, InFileName, "image is of an unsupported machine type 0x%X", (UINT32) FileHeader.Machine);
    goto Finish;
  }
  //
  // Calculate the total number of bytes we're going to strip off. The '4' is for the
  // PE signature PE\0\0. Then sanity check the size.
  //
  BytesStripped = PESigOffset + 4 + sizeof (EFI_IMAGE_FILE_HEADER) + FileHeader.SizeOfOptionalHeader;
  if (BytesStripped >= FileSize) {
    Error (NULL, 0, 0, InFileName, "attempt to strip more bytes than the total file size");
    goto Finish;
  }

  if (BytesStripped &~0xFFFF) {
    Error (NULL, 0, 0, InFileName, "attempt to strip more than 64K bytes", NULL);
    goto Finish;
  }

  TEImageHeader.StrippedSize = (UINT16) BytesStripped;

  //
  // Read in the optional header. Assume PE32, and if not, then re-read as PE32+
  //
  SaveFilePosition = ftell (InFptr);
  if (fread (&OptionalHeader32, sizeof (EFI_IMAGE_OPTIONAL_HEADER32), 1, InFptr) != 1) {
    Error (NULL, 0, 0, InFileName, "failed to read optional header from input file");
    goto Finish;
  }

  if (OptionalHeader32.SectionAlignment != OptionalHeader32.FileAlignment) {
    Error (NULL, 0, 0, InFileName, "Section alignment is not same to file alignment.");
    goto Finish;
  }

  if (OptionalHeader32.Magic == EFI_IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
    //
    // Fill in our new header with required data directory entries
    //
    TEImageHeader.AddressOfEntryPoint = OptionalHeader32.AddressOfEntryPoint;
    //
    // - BytesStripped + sizeof (EFI_TE_IMAGE_HEADER);
    //
    // We're going to pack the subsystem into 1 byte. Make sure it fits
    //
    if (OptionalHeader32.Subsystem &~0xFF) {
      Error (
        NULL,
        0,
        0,
        InFileName,
        NULL,
        "image subsystem 0x%X cannot be packed into 1 byte",
        (UINT32) OptionalHeader32.Subsystem
        );
      goto Finish;
    }

    TEImageHeader.Subsystem   = (UINT8) OptionalHeader32.Subsystem;
    TEImageHeader.BaseOfCode  = OptionalHeader32.BaseOfCode;
    TEImageHeader.ImageBase   = (UINT64) (OptionalHeader32.ImageBase);
    if (OptionalHeader32.NumberOfRvaAndSizes > EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC) {
      TEImageHeader.DataDirectory[EFI_TE_IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress = OptionalHeader32.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress;
      TEImageHeader.DataDirectory[EFI_TE_IMAGE_DIRECTORY_ENTRY_BASERELOC].Size = OptionalHeader32.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC].Size;
    }

    if (OptionalHeader32.NumberOfRvaAndSizes > EFI_IMAGE_DIRECTORY_ENTRY_DEBUG) {
      TEImageHeader.DataDirectory[EFI_TE_IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress = OptionalHeader32.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress;
      TEImageHeader.DataDirectory[EFI_TE_IMAGE_DIRECTORY_ENTRY_DEBUG].Size = OptionalHeader32.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_DEBUG].Size;
    }
  } else if (OptionalHeader32.Magic == EFI_IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
    //
    // Rewind and re-read the optional header
    //
    fseek (InFptr, SaveFilePosition, SEEK_SET);
    if (fread (&OptionalHeader64, sizeof (EFI_IMAGE_OPTIONAL_HEADER64), 1, InFptr) != 1) {
      Error (NULL, 0, 0, InFileName, "failed to re-read optional header from input file");
      goto Finish;
    }

    TEImageHeader.AddressOfEntryPoint = OptionalHeader64.AddressOfEntryPoint;
    //
    // - BytesStripped + sizeof (EFI_TE_IMAGE_HEADER);
    //
    // We're going to pack the subsystem into 1 byte. Make sure it fits
    //
    if (OptionalHeader64.Subsystem &~0xFF) {
      Error (
        NULL,
        0,
        0,
        InFileName,
        NULL,
        "image subsystem 0x%X cannot be packed into 1 byte",
        (UINT32) OptionalHeader64.Subsystem
        );
      goto Finish;
    }

    TEImageHeader.Subsystem   = (UINT8) OptionalHeader64.Subsystem;
    TEImageHeader.BaseOfCode  = OptionalHeader64.BaseOfCode;
    TEImageHeader.ImageBase = (UINT64) (OptionalHeader64.ImageBase);
    if (OptionalHeader64.NumberOfRvaAndSizes > EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC) {
      TEImageHeader.DataDirectory[EFI_TE_IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress = OptionalHeader64.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress;
      TEImageHeader.DataDirectory[EFI_TE_IMAGE_DIRECTORY_ENTRY_BASERELOC].Size = OptionalHeader64.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_BASERELOC].Size;
    }

    if (OptionalHeader64.NumberOfRvaAndSizes > EFI_IMAGE_DIRECTORY_ENTRY_DEBUG) {
      TEImageHeader.DataDirectory[EFI_TE_IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress = OptionalHeader64.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress;
      TEImageHeader.DataDirectory[EFI_TE_IMAGE_DIRECTORY_ENTRY_DEBUG].Size = OptionalHeader64.DataDirectory[EFI_IMAGE_DIRECTORY_ENTRY_DEBUG].Size;
    }
  } else {
    Error (
      NULL,
      0,
      0,
      InFileName,
      "unsupported magic number 0x%X found in optional header",
      (UINT32) OptionalHeader32.Magic
      );
    goto Finish;
  }
  //
  // Fill in the remainder of our new image header
  //
  TEImageHeader.Signature = EFI_TE_IMAGE_HEADER_SIGNATURE;
  TEImageHeader.Machine   = FileHeader.Machine;
  //
  // We're going to pack the number of sections into a single byte. Make sure it fits.
  //
  if (FileHeader.NumberOfSections &~0xFF) {
    Error (
      NULL,
      0,
      0,
      InFileName,
      NULL,
      "image's number of sections 0x%X cannot be packed into 1 byte",
      (UINT32) FileHeader.NumberOfSections
      );
    goto Finish;
  }

  TEImageHeader.NumberOfSections = (UINT8) FileHeader.NumberOfSections;

  //
  // Now open our output file
  //
  if ((OutFptr = fopen (OutFileName, "wb")) == NULL) {
    Error (NULL, 0, 0, OutFileName, "failed to open output file for writing");
    goto Finish;
  }
  //
  // Write the TE header
  //
  if (fwrite (&TEImageHeader, sizeof (EFI_TE_IMAGE_HEADER), 1, OutFptr) != 1) {
    Error (NULL, 0, 0, "failed to write image header to output file", NULL);
    goto Finish;
  }
  //
  // Position into the input file, read the part we're not stripping, and
  // write it out.
  //
  fseek (InFptr, BytesStripped, SEEK_SET);
  Buffer = (UINT8 *) malloc (FileSize - BytesStripped);
  if (Buffer == NULL) {
    Error (NULL, 0, 0, "application error", "failed to allocate memory");
    goto Finish;
  }

  if (fread (Buffer, FileSize - BytesStripped, 1, InFptr) != 1) {
    Error (NULL, 0, 0, InFileName, "failed to read remaining contents of input file");
    goto Finish;
  }

  if (fwrite (Buffer, FileSize - BytesStripped, 1, OutFptr) != 1) {
    Error (NULL, 0, 0, OutFileName, "failed to write all bytes to output file");
    goto Finish;
  }

  Status = STATUS_SUCCESS;

Finish:
  if (InFptr != NULL) {
    fclose (InFptr);
  }
  //
  // Close the output file. If there was an error, delete the output file so
  // that a subsequent build will rebuild it.
  //
  if (OutFptr != NULL) {
    fclose (OutFptr);
    if (GetUtilityStatus () == STATUS_ERROR) {
      remove (OutFileName);
    }
  }

  //
  // Free up our buffer
  //
  if (Buffer != NULL) {
    free (Buffer);
  }

  return Status;
}

static
STATUS
CheckPE32File (
  INT8      *FileName,
  FILE      *Fptr,
  UINT16    *MachineType,
  UINT16    *SubSystem
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  FileName    - GC_TODO: add argument description
  Fptr        - GC_TODO: add argument description
  MachineType - GC_TODO: add argument description
  SubSystem   - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
{
  /*++

Routine Description:
  
  Given a file pointer to a supposed PE32 image file, verify that it is indeed a
  PE32 image file, and then return the machine type in the supplied pointer.

Arguments:

  Fptr          File pointer to the already-opened PE32 file
  MachineType   Location to stuff the machine type of the PE32 file. This is needed
                because the image may be Itanium-based, IA32, or EBC.

Returns:

  0             success
  non-zero      otherwise

--*/
  EFI_IMAGE_DOS_HEADER      DosHeader;
  EFI_IMAGE_FILE_HEADER     FileHdr;
  EFI_IMAGE_OPTIONAL_HEADER OptionalHdr;
  UINT32                    PESig;
  STATUS                    Status;

  Status = STATUS_ERROR;
  //
  // Position to the start of the file
  //
  fseek (Fptr, 0, SEEK_SET);
  //
  // Read the DOS header
  //
  if (fread (&DosHeader, sizeof (DosHeader), 1, Fptr) != 1) {
    Error (NULL, 0, 0, FileName, "failed to read the DOS stub from the input file");
    goto Finish;
  }
  //
  // Check the magic number (0x5A4D)
  //
  if (DosHeader.e_magic != EFI_IMAGE_DOS_SIGNATURE) {
    Error (NULL, 0, 0, FileName, "input file does not appear to be a PE32 image (magic number)");
    goto Finish;
  }
  //
  // Position into the file and check the PE signature
  //
  fseek (Fptr, (long) DosHeader.e_lfanew, SEEK_SET);
  if (fread (&PESig, sizeof (PESig), 1, Fptr) != 1) {
    Error (NULL, 0, 0, FileName, "failed to read PE signature bytes");
    goto Finish;
  }
  //
  // Check the PE signature in the header "PE\0\0"
  //
  if (PESig != EFI_IMAGE_NT_SIGNATURE) {
    Error (NULL, 0, 0, FileName, "file does not appear to be a PE32 image (signature)");
    goto Finish;
  }
  //
  // Read the file header
  //
  if (fread (&FileHdr, sizeof (FileHdr), 1, Fptr) != 1) {
    Error (NULL, 0, 0, FileName, "failed to read PE file header from input file");
    goto Finish;
  }
  //
  // Read the optional header so we can get the subsystem
  //
  if (fread (&OptionalHdr, sizeof (OptionalHdr), 1, Fptr) != 1) {
    Error (NULL, 0, 0, FileName, "failed to read COFF optional header from input file");
    goto Finish;
  }

  *SubSystem = OptionalHdr.Subsystem;
  if (mOptions.Verbose) {
    fprintf (stdout, "  Got subsystem = 0x%X from image\n", (int) *SubSystem);
  }
  //
  // Good to go
  //
  Status = STATUS_SUCCESS;
Finish:
  fseek (Fptr, 0, SEEK_SET);
  return Status;
}

static
int
ParseCommandLine (
  int         Argc,
  char        *Argv[]
  )
/*++

Routine Description:
  
  Given the Argc/Argv program arguments, and a pointer to an options structure,
  parse the command-line options and check their validity.


Arguments:

  Argc            - standard C main() argument count
  Argv            - standard C main() argument list

Returns:

  STATUS_SUCCESS    success
  non-zero          otherwise

--*/
// GC_TODO:    ] - add argument and description to function comment
{
  //
  // Clear out the options
  //
  memset ((char *) &mOptions, 0, sizeof (mOptions));
  //
  // Skip over the program name
  //
  Argc--;
  Argv++;
  //
  // If no arguments, assume they want usage info
  //
  if (Argc == 0) {
    Usage ();
    return STATUS_ERROR;
  }
  //
  // Process until no more arguments
  //
  while ((Argc > 0) && ((Argv[0][0] == '-') || (Argv[0][0] == '/'))) {
    //
    // To simplify string comparisons, replace slashes with dashes
    //
    Argv[0][0] = '-';
    if (_stricmp (Argv[0], "-o") == 0) {
      //
      // Output filename specified with -o
      // Make sure there's another parameter
      //
      if (Argc > 1) {
        strcpy (mOptions.OutFileName, Argv[1]);
      } else {
        Error (NULL, 0, 0, Argv[0], "missing output file name with option");
        Usage ();
        return STATUS_ERROR;
      }

      Argv++;
      Argc--;
    } else if ((_stricmp (Argv[0], "-h") == 0) || (strcmp (Argv[0], "-?") == 0)) {
      //
      // Help option
      //
      Usage ();
      return STATUS_ERROR;
    } else if (_stricmp (Argv[0], "-v") == 0) {
      //
      // -v for verbose
      //
      mOptions.Verbose = 1;
    } else if (_stricmp (Argv[0], "-dump") == 0) {
      //
      // -dump for dumping an image
      //
      mOptions.Dump = 1;
    } else {
      Error (NULL, 0, 0, Argv[0], "unrecognized option");
      Usage ();
      return STATUS_ERROR;
    }
    //
    // Next argument
    //
    Argv++;
    Argc--;
  }
  //
  // Better be one more arg for input file name
  //
  if (Argc == 0) {
    Error (NULL, 0, 0, "input file name required", NULL);
    Usage ();
    return STATUS_ERROR;
  }

  if (Argc != 1) {
    Error (NULL, 0, 0, Argv[1], "extra arguments on command line");
    return STATUS_ERROR;
  }

  strcpy (mOptions.InFileName, Argv[0]);
  return STATUS_SUCCESS;
}

static
void
Usage (
  VOID
  )
/*++

Routine Description:
  
  Print usage information for this utility.

Arguments:

  None.

Returns:

  Nothing.

--*/
{
  int         Index;
  const char  *Str[] = {
    UTILITY_NAME" "UTILITY_VERSION" - Intel Generate TE Image Utility",
    "  Copyright (C), 1999 - 2008 Intel Corporation",
    
#if ( defined(UTILITY_BUILD) && defined(UTILITY_VENDOR) )
    "  Built from "UTILITY_BUILD", project of "UTILITY_VENDOR,
#endif
    "",
    "Usage:",
    "  "UTILITY_NAME" [OPTION]... PE32IMAGE",
    "Description:",
    "  Generate a TE image from an EFI PE32 image.",
    "Options:",
    "  -v             - for verbose output",
    "  -dump          - to dump the input file to a text file",
    "  -h -?          - for this help information",
    "  -o OutFileName - to write output to OutFileName rather than PE32IMAGE"DEFAULT_OUTPUT_EXTENSION,
    NULL
  };
  for (Index = 0; Str[Index] != NULL; Index++) {
    fprintf (stdout, "%s\n", Str[Index]);
  }
}

static
VOID
DumpImage (
  INT8    *FileName
  )
/*++

Routine Description:
  
  Dump a specified image information

Arguments:
  
  FileName - File name pointer to the image to dump

Returns:

  Nothing.

--*/
{
  FILE                *InFptr;
  EFI_TE_IMAGE_HEADER TEImageHeader;
  INT8                *NamePtr;

  //
  // Open the input file
  //
  InFptr  = NULL;

  if ((InFptr = fopen (FileName, "rb")) == NULL) {
    Error (NULL, 0, 0, FileName, "failed to open input file for reading");
    return ;
  }

  if (fread (&TEImageHeader, sizeof (EFI_TE_IMAGE_HEADER), 1, InFptr) != 1) {
    Error (NULL, 0, 0, FileName, "failed to read image header from input file");
    goto Finish;
  }

  if (TEImageHeader.Signature != EFI_TE_IMAGE_HEADER_SIGNATURE) {
    Error (NULL, 0, 0, FileName, "Image does not appear to be a TE image (bad signature)");
    goto Finish;
  }
  //
  // Dump the header
  //
  fprintf (stdout, "Header (%d bytes):\n", sizeof (EFI_TE_IMAGE_HEADER));
  fprintf (stdout, "  Signature:          0x%04X (TE)\n", (UINT32) TEImageHeader.Signature);
  NamePtr = GetMachineTypeStr (TEImageHeader.Machine);
  fprintf (stdout, "  Machine:            0x%04X (%s)\n", (UINT32) TEImageHeader.Machine, NamePtr);
  NamePtr = GetSubsystemTypeStr (TEImageHeader.Subsystem);
  fprintf (stdout, "  Subsystem:          0x%02X (%s)\n", (UINT32) TEImageHeader.Subsystem, NamePtr);
  fprintf (stdout, "  Number of sections  0x%02X\n", (UINT32) TEImageHeader.NumberOfSections);
  fprintf (stdout, "  Stripped size:      0x%04X\n", (UINT32) TEImageHeader.StrippedSize);
  fprintf (stdout, "  Entry point:        0x%08X\n", TEImageHeader.AddressOfEntryPoint);
  fprintf (stdout, "  Base of code:       0x%08X\n", TEImageHeader.BaseOfCode);
  fprintf (stdout, "  Data directories:\n");
  fprintf (
    stdout,
    "    %8X [%8X] RVA [size] of Base Relocation Directory\n",
    TEImageHeader.DataDirectory[EFI_TE_IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress,
    TEImageHeader.DataDirectory[EFI_TE_IMAGE_DIRECTORY_ENTRY_BASERELOC].Size
    );
  fprintf (
    stdout,
    "    %8X [%8X] RVA [size] of Debug Directory\n",
    TEImageHeader.DataDirectory[EFI_TE_IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress,
    TEImageHeader.DataDirectory[EFI_TE_IMAGE_DIRECTORY_ENTRY_DEBUG].Size
    );

Finish:
  if (InFptr != NULL) {
    fclose (InFptr);
  }
}

static
INT8 *
GetMachineTypeStr (
  UINT16    MachineType
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  MachineType - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
{
  int Index;

  for (Index = 0; mMachineTypes[Index].Name != NULL; Index++) {
    if (mMachineTypes[Index].Value == MachineType) {
      return mMachineTypes[Index].Name;
    }
  }

  return "unknown";
}

static
INT8 *
GetSubsystemTypeStr (
  UINT16  SubsystemType
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  SubsystemType - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
{
  int Index;

  for (Index = 0; mSubsystemTypes[Index].Name != NULL; Index++) {
    if (mSubsystemTypes[Index].Value == SubsystemType) {
      return mSubsystemTypes[Index].Name;
    }
  }

  return "unknown";
}
