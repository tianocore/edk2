/*++

Copyright (c)  2004-2006 Intel Corporation. All rights reserved
This program and the accompanying materials are licensed and made available 
under the terms and conditions of the BSD License which accompanies this 
distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  FlashMap.c

Abstract:

  Utility for flash management in the Intel Platform Innovation Framework
  for EFI build environment.

--*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include <Common/UefiBaseTypes.h>

#include "EfiUtilityMsgs.h"
#include "Microcode.h"
#include "FlashDefFile.h"
#include "Symbols.h"

//
// Version of this utility
//
#define UTILITY_NAME "FlashMap"
#define UTILITY_MAJOR_VERSION 1
#define UTILITY_MINOR_VERSION 0


typedef struct _STRING_LIST {
  struct _STRING_LIST *Next;
  char                *Str;
} STRING_LIST;

//
// Keep our globals in one of these structures
//
static struct {
  char          *CIncludeFileName;
  char          *FlashDevice;
  char          *FlashDeviceImage;
  char          *MCIFileName;
  char          *MCOFileName;
  char          *ImageOutFileName;
  char          *DscFileName;
  char          *AsmIncludeFileName;
  char          *FlashDefinitionFileName;
  char          *StringReplaceInFileName;
  char          *StringReplaceOutFileName;
  char          *DiscoverFDImageName;
  char          MicrocodePadByteValue;
  unsigned int  MicrocodeAlignment;
  STRING_LIST   *MCIFileNames;
  STRING_LIST   *LastMCIFileNames;
  unsigned int  BaseAddress;
} mGlobals;

#define DEFAULT_MC_PAD_BYTE_VALUE 0xFF
#define DEFAULT_MC_ALIGNMENT      16

static
STATUS
ProcessCommandLine (
  int     argc,
  char    *argv[]
  );

static
STATUS
MergeMicrocodeFiles (
  char            *OutFileName,
  STRING_LIST     *FileNames,
  unsigned int    Alignment,
  char            PadByteValue
  );

static
void
Version (
  VOID
  );

static
void
Usage (
  VOID
  );

char* 
NormalizePath (
  char* OldPathName
  );

int
main (
  int   argc,
  char  *argv[]
  )
/*++

Routine Description:
  Parse the command line arguments and then call worker functions to do the work
  
Arguments:
  argc      - number of elements in argv
  argv      - array of command-line arguments

Returns:
  STATUS_SUCCESS    - no problems encountered while processing
  STATUS_WARNING    - warnings, but no errors, were encountered while processing
  STATUS_ERROR      - errors were encountered while processing
  
--*/
{
  STATUS  Status;

  SetUtilityName (UTILITY_NAME);
  Status = ProcessCommandLine (argc, argv);
  if (Status != STATUS_SUCCESS) {
    return Status;
  }
  //
  // Check for discovery of an FD (command line option)
  //
  if (mGlobals.DiscoverFDImageName != NULL) {
    Status = FDDiscover (mGlobals.DiscoverFDImageName, mGlobals.BaseAddress);
    goto Done;
  }
  //
  // If they're doing microcode file parsing, then do that
  //
  if (mGlobals.MCIFileName != NULL) {
    MicrocodeConstructor ();
    MicrocodeParseFile (mGlobals.MCIFileName, mGlobals.MCOFileName);
    MicrocodeDestructor ();
  }
  //
  // If they're doing microcode file merging, then do that now
  //
  if (mGlobals.MCIFileNames != NULL) {
    MergeMicrocodeFiles (
      mGlobals.MCOFileName,
      mGlobals.MCIFileNames,
      mGlobals.MicrocodeAlignment,
      mGlobals.MicrocodePadByteValue
      );
  }
  //
  // If using a flash definition file, then process that and return
  //
  if (mGlobals.FlashDefinitionFileName != NULL) {
    FDFConstructor ();
    SymbolsConstructor ();
    Status = FDFParseFile (mGlobals.FlashDefinitionFileName);
    if (GetUtilityStatus () != STATUS_ERROR) {
      //
      // If they want us to do a string-replace on a file, then add the symbol definitions to
      // the symbol table, and then do the string replace.
      //
      if (mGlobals.StringReplaceInFileName != NULL) {
        Status  = FDFCreateSymbols (mGlobals.FlashDevice);
        Status  = SymbolsFileStringsReplace (mGlobals.StringReplaceInFileName, mGlobals.StringReplaceOutFileName);
      }
      //
      // If they want us to create a .h defines file or .c flashmap data file, then do so now
      //
      if (mGlobals.CIncludeFileName != NULL) {
        Status = FDFCreateCIncludeFile (mGlobals.FlashDevice, mGlobals.CIncludeFileName);
      }
      if (mGlobals.AsmIncludeFileName != NULL) {
        Status = FDFCreateAsmIncludeFile (mGlobals.FlashDevice, mGlobals.AsmIncludeFileName);
      }
      //
      // If they want us to create an image, do that now
      //
      if (mGlobals.ImageOutFileName != NULL) {
        Status = FDFCreateImage (mGlobals.FlashDevice, mGlobals.FlashDeviceImage, mGlobals.ImageOutFileName);
      }
      //
      // If they want to create an output DSC file, do that now
      //
      if (mGlobals.DscFileName != NULL) {
        Status = FDFCreateDscFile (mGlobals.FlashDevice, mGlobals.DscFileName);
      }
    }
    SymbolsDestructor ();
    FDFDestructor ();
  }
Done:
  //
  // Free up memory
  //
  while (mGlobals.MCIFileNames != NULL) {
    mGlobals.LastMCIFileNames = mGlobals.MCIFileNames->Next;
    _free (mGlobals.MCIFileNames);
    mGlobals.MCIFileNames = mGlobals.LastMCIFileNames;
  }
  return GetUtilityStatus ();
}

static
STATUS
MergeMicrocodeFiles (
  char            *OutFileName,
  STRING_LIST     *FileNames,
  unsigned int    Alignment,
  char            PadByteValue
  )
/*++

Routine Description:

  Merge binary microcode files into a single file, taking into consideration
  the alignment and pad value.

Arguments:

  OutFileName     - name of the output file to create
  FileNames       - linked list of input microcode files to merge
  Alignment       - alignment for each microcode file in the output image
  PadByteValue    - value to use when padding to meet alignment requirements

Returns:

  STATUS_SUCCESS  - merge completed successfully or with acceptable warnings
  STATUS_ERROR    - merge failed, output file not created

--*/
{
  long    FileSize;
  long    TotalFileSize;
  FILE    *InFptr;
  FILE    *OutFptr;
  char    *Buffer;
  STATUS  Status;

  //
  // Open the output file
  //
  if ((OutFptr = fopen (OutFileName, "wb")) == NULL) {
    Error (NULL, 0, 0, OutFileName, "failed to open output file for writing");
    return STATUS_ERROR;
  }
  //
  // Walk the list of files
  //
  Status        = STATUS_ERROR;
  Buffer        = NULL;
  InFptr        = NULL;
  TotalFileSize = 0;
  while (FileNames != NULL) {
    //
    // Open the file, determine the size, then read it in and write
    // it back out.
    //
    if ((InFptr = fopen (NormalizePath(FileNames->Str), "rb")) == NULL) {
      Error (NULL, 0, 0, NormalizePath(FileNames->Str), "failed to open input file for reading");
      goto Done;
    }
    fseek (InFptr, 0, SEEK_END);
    FileSize = ftell (InFptr);
    fseek (InFptr, 0, SEEK_SET);
    if (FileSize != 0) {
      Buffer = (char *) _malloc (FileSize);
      if (Buffer == NULL) {
        Error (NULL, 0, 0, "memory allocation failure", NULL);
        goto Done;
      }
      if (fread (Buffer, FileSize, 1, InFptr) != 1) {
        Error (NULL, 0, 0, FileNames->Str, "failed to read file contents");
        goto Done;
      }
      //
      // Align
      //
      if (Alignment != 0) {
        while ((TotalFileSize % Alignment) != 0) {
          if (fwrite (&PadByteValue, 1, 1, OutFptr) != 1) {
            Error (NULL, 0, 0, OutFileName, "failed to write pad bytes to output file");
            goto Done;
          }
          TotalFileSize++;
        }
      }
      TotalFileSize += FileSize;
      if (fwrite (Buffer, FileSize, 1, OutFptr) != 1) {
        Error (NULL, 0, 0, OutFileName, "failed to write to output file");
        goto Done;
      }
      _free (Buffer);
      Buffer = NULL;
    } else {
      Warning (NULL, 0, 0, FileNames->Str, "0-size file encountered");
    }
    fclose (InFptr);
    InFptr    = NULL;
    FileNames = FileNames->Next;
  }
  Status = STATUS_SUCCESS;
Done:
  fclose (OutFptr);
  if (InFptr != NULL) {
    fclose (InFptr);
  }
  if (Buffer != NULL) {
    _free (Buffer);
  }
  if (Status == STATUS_ERROR) {
    remove (OutFileName);
  }
  return Status;
}

static
STATUS
ProcessCommandLine (
  int   argc,
  char  *argv[]
  )
/*++

Routine Description:
  Process the command line arguments
  
Arguments:
  argc   - Standard C entry point arguments
  argv[] - Standard C entry point arguments

Returns:
  STATUS_SUCCESS    - no problems encountered while processing
  STATUS_WARNING    - warnings, but no errors, were encountered while processing
  STATUS_ERROR      - errors were encountered while processing
  
--*/
{
  int           ThingsToDo;
  unsigned int  Temp;
  STRING_LIST   *Str;
  //
  // Skip program name arg, process others
  //
  argc--;
  argv++;
  if (argc == 0) {
    Usage ();
    return STATUS_ERROR;
  }
  
  if ((strcmp(argv[0], "-h") == 0) || (strcmp(argv[0], "--help") == 0) ||
      (strcmp(argv[0], "-?") == 0) || (strcmp(argv[0], "/?") == 0)) {
    Usage();
    return STATUS_ERROR;
  }
  
  if ((strcmp(argv[0], "-V") == 0) || (strcmp(argv[0], "--version") == 0)) {
    Version();
    return STATUS_ERROR;
  }
 
  //
  // Clear out our globals, then start walking the arguments
  //
  memset ((void *) &mGlobals, 0, sizeof (mGlobals));
  mGlobals.MicrocodePadByteValue  = DEFAULT_MC_PAD_BYTE_VALUE;
  mGlobals.MicrocodeAlignment     = DEFAULT_MC_ALIGNMENT;
  ThingsToDo                      = 0;
  while (argc > 0) {
    if (strcmp (argv[0], "-?") == 0) {
      Usage ();
      return STATUS_ERROR;
    } else if (strcmp (argv[0], "-hfile") == 0) {
      //
      // -hfile FileName
      //
      // Used to specify an output C #include file to create that contains
      // #define statements for all the flashmap region offsets and sizes.
      // Check for additional argument.
      //
      if (argc < 2) {
        Error (NULL, 0, 0, argv[0], "option requires an output file name");
        return STATUS_ERROR;
      }
      argc--;
      argv++;
      mGlobals.CIncludeFileName = argv[0];
      ThingsToDo++;
    } else if (strcmp (argv[0], "-flashdevice") == 0) {
      //
      // -flashdevice FLASH_DEVICE_NAME
      //
      // Used to select which flash device definition to operate on.
      // Check for additional argument
      //
      if (argc < 2) {
        Error (NULL, 0, 0, argv[0], "option requires a flash device name to use");
        return STATUS_ERROR;
      }
      argc--;
      argv++;
      mGlobals.FlashDevice = argv[0];
    } else if (strcmp (argv[0], "-mco") == 0) {
      //
      // -mco OutFileName
      //
      // Used to specify a microcode output binary file to create.
      // Check for additional argument.
      //
      if (argc < 2) {
        Error (NULL, 0, 0, (INT8 *) argv[0], (INT8 *) "option requires an output microcode file name to create");
        return STATUS_ERROR;
      }
      argc--;
      argv++;
      mGlobals.MCOFileName = argv[0];
      ThingsToDo++;
    } else if (strcmp (argv[0], "-asmincfile") == 0) {
      //
      // -asmincfile FileName
      //
      // Used to specify the name of the output assembly include file that contains
      // equates for the flash region addresses and sizes.
      // Check for additional argument.
      //
      if (argc < 2) {
        Error (NULL, 0, 0, argv[0], "option requires an output ASM include file name to create");
        return STATUS_ERROR;
      }
      argc--;
      argv++;
      mGlobals.AsmIncludeFileName = argv[0];
      ThingsToDo++;
    } else if (strcmp (argv[0], "-mci") == 0) {
      //
      // -mci FileName
      //
      // Used to specify an input microcode text file to parse.
      // Check for additional argument
      //
      if (argc < 2) {
        Error (NULL, 0, 0, (INT8 *) argv[0], (INT8 *) "option requires an input microcode text file name to parse");
        return STATUS_ERROR;
      }
      argc--;
      argv++;
      mGlobals.MCIFileName = argv[0];
    } else if (strcmp (argv[0], "-flashdeviceimage") == 0) {
      //
      // -flashdeviceimage FlashDeviceImage
      //
      // Used to specify which flash device image definition from the input flash definition file
      // to create.
      // Check for additional argument.
      //
      if (argc < 2) {
        Error (NULL, 0, 0, argv[0], "option requires the name of a flash definition image to use");
        return STATUS_ERROR;
      }
      argc--;
      argv++;
      mGlobals.FlashDeviceImage = argv[0];
    } else if (strcmp (argv[0], "-imageout") == 0) {
      //
      // -imageout FileName
      //
      // Used to specify the name of the output FD image file to create.
      // Check for additional argument.
      //
      if (argc < 2) {
        Error (NULL, 0, 0, argv[0], "option requires an output image filename to create");
        return STATUS_ERROR;
      }
      argc--;
      argv++;
      mGlobals.ImageOutFileName = argv[0];
      ThingsToDo++;
    } else if (strcmp (argv[0], "-dsc") == 0) {
      //
      // -dsc FileName
      //
      // Used to specify the name of the output DSC file to create.
      // Check for additional argument.
      //
      if (argc < 2) {
        Error (NULL, 0, 0, argv[0], "option requires an output DSC filename to create");
        return STATUS_ERROR;
      }
      argc--;
      argv++;
      mGlobals.DscFileName = argv[0];
      ThingsToDo++;
    } else if (strcmp (argv[0], "-fdf") == 0) {
      //
      // -fdf FileName
      //
      // Used to specify the name of the input flash definition file.
      // Check for additional argument.
      //
      if (argc < 2) {
        Error (NULL, 0, 0, argv[0], "option requires an input flash definition file name");
        return STATUS_ERROR;
      }
      argc--;
      argv++;
      mGlobals.FlashDefinitionFileName = argv[0];
    } else if (strcmp (argv[0], "-discover") == 0) {
      //
      // -discover FDFileName
      //
      // Debug functionality used to scan an existing FD image, trying to find
      // firmware volumes at 64K boundaries.
      // Check for additional argument.
      //
      if (argc < 2) {
        Error (NULL, 0, 0, argv[0], "option requires an input FD image file name");
        return STATUS_ERROR;
      }
      argc--;
      argv++;
      mGlobals.DiscoverFDImageName = argv[0];
      ThingsToDo++;
    } else if (strcmp (argv[0], "-baseaddr") == 0) {
      //
      // -baseaddr Addr
      //
      // Used to specify a base address when doing a discover of an FD image.
      // Check for additional argument.
      //
      if (argc < 2) {
        Error (NULL, 0, 0, argv[0], "option requires a base address");
        return STATUS_ERROR;
      }
      argc--;
      argv++;
      if (tolower (argv[0][1]) == 'x') {
        sscanf (argv[0] + 2, "%x", &mGlobals.BaseAddress);
      } else {
        sscanf (argv[0], "%d", &mGlobals.BaseAddress);
      }
    } else if (strcmp (argv[0], "-padvalue") == 0) {
      //
      // -padvalue Value
      //
      // Used to specify the value to pad with when aligning data while
      // creating an FD image. Check for additional argument.
      //
      if (argc < 2) {
        Error (NULL, 0, 0, argv[0], "option requires a byte value");
        return STATUS_ERROR;
      }
      argc--;
      argv++;
      if (tolower (argv[0][1]) == 'x') {
        sscanf (argv[0] + 2, "%x", &Temp);
        mGlobals.MicrocodePadByteValue = (char) Temp;
      } else {
        sscanf (argv[0], "%d", &Temp);
        mGlobals.MicrocodePadByteValue = (char) Temp;
      }
    } else if (strcmp (argv[0], "-align") == 0) {
      //
      // -align Alignment
      //
      // Used to specify how each data file is aligned in the region
      // when creating an FD image. Check for additional argument.
      //
      if (argc < 2) {
        Error (NULL, 0, 0, argv[0], "option requires an alignment");
        return STATUS_ERROR;
      }
      argc--;
      argv++;
      if (tolower (argv[0][1]) == 'x') {
        sscanf (argv[0] + 2, "%x", &mGlobals.MicrocodeAlignment);
      } else {
        sscanf (argv[0], "%d", &mGlobals.MicrocodeAlignment);
      }
    } else if (strcmp (argv[0], "-mcmerge") == 0) {
      //
      // -mcmerge FileName(s)
      //
      // Used to concatenate multiple microde binary files. Can specify
      // multiple file names with the one -mcmerge flag. Check for additional argument.
      //
      if ((argc < 2) || (argv[1][0] == '-')) {
        Error (NULL, 0, 0, argv[0], "option requires one or more input file names");
        return STATUS_ERROR;
      }
      //
      // Take input files until another option or end of list
      //
      ThingsToDo++;
      while ((argc > 1) && (argv[1][0] != '-')) {
        Str = (STRING_LIST *) _malloc (sizeof (STRING_LIST));
        if (Str == NULL) {
          Error (NULL, 0, 0, "memory allocation failure", NULL);
          return STATUS_ERROR;
        }
        memset (Str, 0, sizeof (STRING_LIST));
        Str->Str = argv[1];
        if (mGlobals.MCIFileNames == NULL) {
          mGlobals.MCIFileNames = Str;
        } else {
          mGlobals.LastMCIFileNames->Next = Str;
        }
        mGlobals.LastMCIFileNames = Str;
        argc--;
        argv++;
      }
    } else if (strcmp (argv[0], "-strsub") == 0) {
      //
      // -strsub SrcFile DestFile
      //
      // Used to perform string substitutions on a file, writing the result to a new
      // file. Check for two additional arguments.
      //
      if (argc < 3) {
        Error (NULL, 0, 0, argv[0], "option requires input and output file names for string substitution");
        return STATUS_ERROR;
      }
      argc--;
      argv++;
      mGlobals.StringReplaceInFileName = argv[0];
      argc--;
      argv++;
      mGlobals.StringReplaceOutFileName = argv[0];
      ThingsToDo++;
    } else {
      Error (NULL, 0, 0, argv[0], "invalid option");
      return STATUS_ERROR;
    }
    argc--;
    argv++;
  }
  //
  // If no outputs requested, then report an error
  //
  if (ThingsToDo == 0) {
    Error (NULL, 0, 0, "nothing to do", NULL);
    return STATUS_ERROR;
  }
  //
  // If they want an asm file, #include file, or C file to be created, then they have to specify a
  // flash device name and flash definition file name.
  //
  if ((mGlobals.CIncludeFileName != NULL) &&
      ((mGlobals.FlashDevice == NULL) || (mGlobals.FlashDefinitionFileName == NULL))) {
    Error (NULL, 0, 0, "must specify -flashdevice and -fdf with -hfile", NULL);
    return STATUS_ERROR;
  }
  if ((mGlobals.AsmIncludeFileName != NULL) &&
      ((mGlobals.FlashDevice == NULL) || (mGlobals.FlashDefinitionFileName == NULL))) {
    Error (NULL, 0, 0, "must specify -flashdevice and -fdf with -asmincfile", NULL);
    return STATUS_ERROR;
  }
  //
  // If they want a dsc file to be created, then they have to specify a
  // flash device name and a flash definition file name
  //
  if (mGlobals.DscFileName != NULL) {
    if (mGlobals.FlashDevice == NULL) {
      Error (NULL, 0, 0, "must specify -flashdevice with -dsc", NULL);
      return STATUS_ERROR;
    }
    if (mGlobals.FlashDefinitionFileName == NULL) {
      Error (NULL, 0, 0, "must specify -fdf with -dsc", NULL);
      return STATUS_ERROR;
    }
  }
  //
  // If they specified an output microcode file name, then they have to specify an input
  // file name, and vice versa.
  //
  if ((mGlobals.MCIFileName != NULL) && (mGlobals.MCOFileName == NULL)) {
    Error (NULL, 0, 0, "must specify output microcode file name", NULL);
    return STATUS_ERROR;
  }
  if ((mGlobals.MCOFileName != NULL) && (mGlobals.MCIFileName == NULL) && (mGlobals.MCIFileNames == NULL)) {
    Error (NULL, 0, 0, "must specify input microcode file name", NULL);
    return STATUS_ERROR;
  }
  //
  // If doing merge, then have to specify output file name
  //
  if ((mGlobals.MCIFileNames != NULL) && (mGlobals.MCOFileName == NULL)) {
    Error (NULL, 0, 0, "must specify output microcode file name", NULL);
    return STATUS_ERROR;
  }
  //
  // If they want an output image to be created, then they have to specify
  // the flash device and the flash device image to use.
  //
  if (mGlobals.ImageOutFileName != NULL) {
    if (mGlobals.FlashDevice == NULL) {
      Error (NULL, 0, 0, "must specify -flashdevice with -imageout", NULL);
      return STATUS_ERROR;
    }
    if (mGlobals.FlashDeviceImage == NULL) {
      Error (NULL, 0, 0, "must specify -flashdeviceimage with -imageout", NULL);
      return STATUS_ERROR;
    }
    if (mGlobals.FlashDefinitionFileName == NULL) {
      Error (NULL, 0, 0, "must specify -c or -fdf with -imageout", NULL);
      return STATUS_ERROR;
    }
  }
  return STATUS_SUCCESS;
}

static
void
Version (
  VOID
  )
/*++

Routine Description:
  
  Print version information for this utility.

Arguments:

  None.

Returns:

  Nothing.
--*/
{
  printf ("%s v%d.%d -EDK Utility for flash management for EFI build environment.\n", UTILITY_NAME, UTILITY_MAJOR_VERSION, UTILITY_MINOR_VERSION);
  printf ("Copyright (c) 1999-2006 Intel Corporation. All rights reserved.\n");
}
 
static
void
Usage (
  VOID
  )
/*++

Routine Description:
  Print utility command line help
  
Arguments:
  None

Returns:
  NA

--*/
{
  int   i;
  char  *Msg[] = {
    "\nUsage: FlashTool -fdf FlashDefFile -flashdevice FlashDevice",
    "                 -flashdeviceimage FlashDeviceImage -mci MCIFile -mco MCOFile",
    "                 -discover FDImage -dsc DscFile -asmincfile AsmIncFile",
    "                 -imageOut ImageOutFile -hfile HFile -strsub InStrFile OutStrFile",
    "                 -baseaddr BaseAddr -align Alignment -padvalue PadValue",
    "                 -mcmerge MCIFile(s)",
    "                 -h,--help,-?,/? - to display help messages",
    "                 -V,--version   - to display version information",
    "  where",
    "    FlashDefFile     - input Flash Definition File",
    "    FlashDevice      - flash device to use (from flash definition file)",
    "    FlashDeviceImage - flash device image to use (from flash definition file)",
    "    MCIFile          - input microcode file to parse",
    "    MCOFile          - output binary microcode image to create from MCIFile",
    "    HFile            - output #include file to create",
    "    FDImage          - name of input FDImage file to scan",
    "    ImageOutFile     - output image file to create",
    "    DscFile          - output DSC file to create",
    "    AsmIncFile       - output ASM include file to create",
    "    InStrFile        - input file to replace symbol names, writing result to OutStrFile",
    "    BaseAddr         - base address of FDImage (used with -discover)",
    "    Alignment        - alignment to use when merging microcode binaries",
    "    PadValue         - byte value to use as pad value when aligning microcode binaries",
    "    MCIFile(s)       - one or more microcode binary files to merge/concatenate",
    NULL
  };
  
  Version();
  for (i = 0; Msg[i] != NULL; i++) {
    fprintf (stdout, "%s\n", Msg[i]);
  }
}

char* 
NormalizePath (
  char* OldPathName
  )
{
  char* Visitor;
  
  if (OldPathName == NULL) {
    return NULL;
  }
  
  Visitor = OldPathName;
  while (*Visitor != '\0') {
    if (*Visitor == '\\') {
      *Visitor = '/';
    }
    Visitor++;
  }
  
  return OldPathName;
}

