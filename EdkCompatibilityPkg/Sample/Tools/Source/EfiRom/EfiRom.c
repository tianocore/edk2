/*++

Copyright (c) 2007 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             


Module Name:

  EfiRom.c
  
Abstract:

  Utility program to create an EFI option ROM image from binary and 
  EFI PE32 files.


--*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//
// Includes for EFI 1.1 build
//
// #include "Tiano.h"        // required defines for Compress.h
// #include "EfiImage.h"   // for PE32 structure definitions
// #include "Compress.h"   // for compression function
// Includes for Tiano build
//
#include "TianoCommon.h"
#include "EfiImage.h" // for PE32 structure definitions
#include "Compress.h"

//
// END include differences
//
#include "Pci.h"  // for option ROM header structures
//
// Version of this utility
//
#define UTILITY_NAME    "EfiRom"
#define UTILITY_VERSION "v2.7"

//
// Define some status return values
//
#define STATUS_SUCCESS  0
#define STATUS_WARNING  1
#define STATUS_ERROR    2

//
// Define the max length of a filename
//
#define MAX_PATH                  200

#define DEFAULT_OUTPUT_EXTENSION  ".rom"

//
// Max size for an option ROM image
//
#define MAX_OPTION_ROM_SIZE (1024 * 1024 * 16)  // 16MB
//
// Values for the indicator field in the PCI data structure
//
#define INDICATOR_LAST  0x80  // last file in series of files
//
// Masks for the FILE_LIST.FileFlags field
//
#define FILE_FLAG_BINARY    0x01
#define FILE_FLAG_EFI       0x02
#define FILE_FLAG_COMPRESS  0x04

//
// Use this linked list structure to keep track of all the filenames
// specified on the command line.
//
typedef struct _FILE_LIST {
  struct _FILE_LIST *Next;
  INT8              *FileName;
  UINT32            FileFlags;
  UINT32            ClassCode;
  UINT16            CodeRevision;
} FILE_LIST;

//
// Use this to track our command-line options
//
typedef struct {
  INT8      OutFileName[MAX_PATH];
  INT8      NoLast;
  INT8      Verbose;
  INT8      DumpOption;
  UINT8     DevIdValid;
  UINT8     VendIdValid;
  UINT16    VendId;
  UINT16    DevId;
  FILE_LIST *FileList;
} OPTIONS;

//
// Make a global structure to keep track of command-line options
//
static OPTIONS  mOptions;

//
// Use these to convert from machine type value to a named type
//
typedef struct {
  UINT16  Value;
  char    *Name;
} STRING_LOOKUP;

static STRING_LOOKUP  mMachineTypes[] = {
  EFI_IMAGE_MACHINE_IA32,
  "IA32",
  EFI_IMAGE_MACHINE_IA64,
  "IA64",
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

static char* mCodeTypeStr[] = {
  "PCAT Image",
  "Open Firmware Image",
  "HP PA RISC Image",
  "EFI Image",
  "Undefined"
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
int
ParseCommandLine (
  int       Argc,
  char      *Argv[],
  OPTIONS   *Options
  );

static
int
CheckPE32File (
  FILE      *Fptr,
  UINT16    *MachineType,
  UINT16    *SubSystem
  );

static
int
ProcessEfiFile (
  FILE      *OutFptr,
  FILE_LIST *InFile,
  UINT16    VendId,
  UINT16    DevId,
  UINT32    *Size
  );

static
int
ProcessBinFile (
  FILE      *OutFptr,
  FILE_LIST *InFile,
  UINT32    *Size
  );

static
void
DumpImage (
  FILE_LIST *InFile
  );

char                  *
GetMachineTypeStr (
  UINT16    MachineType
  );

static
char                  *
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
  
  Given an EFI image filename, create a ROM-able image by creating an option 
  ROM header and PCI data structure, filling them in, and then writing the
  option ROM header + PCI data structure + EFI image out to the output file.

Arguments:

  Argc            - standard C main() argument count

  Argv            - standard C main() argument list

Returns:

  0             success
  non-zero      otherwise

--*/
// GC_TODO:    ] - add argument and description to function comment
{
  INT8      *Ext;
  FILE      *FptrOut;
  UINT32    Status;
  FILE_LIST *FList;
  UINT32    TotalSize;
  UINT32    Size;

  Status  = STATUS_SUCCESS;
  FptrOut = NULL;

  //
  // Parse the command line arguments
  //
  if (ParseCommandLine (Argc, Argv, &mOptions)) {
    return STATUS_ERROR;
  }
  //
  // If dumping an image, then do that and quit
  //
  if (mOptions.DumpOption) {
    DumpImage (mOptions.FileList);
    goto BailOut;
  }
  //
  // Determine the output filename. Either what they specified on
  // the command line, or the first input filename with a different extension.
  //
  if (!mOptions.OutFileName[0]) {
    strcpy (mOptions.OutFileName, mOptions.FileList->FileName);
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
  for (FList = mOptions.FileList; FList != NULL; FList = FList->Next) {
    if (_stricmp (mOptions.OutFileName, FList->FileName) == 0) {
      Status = STATUS_ERROR;
      fprintf (
        stdout,
        "ERROR: Input and output file names must be different - %s = %s\n",
        FList->FileName,
        mOptions.OutFileName
        );
      goto BailOut;
    }
  }
  //
  // Now open our output file
  //
  if ((FptrOut = fopen (mOptions.OutFileName, "w+b")) == NULL) {
    fprintf (stdout, "ERROR: Failed to open output file %s\n", mOptions.OutFileName);
    goto BailOut;
  }
  //
  // Process all our files
  //
  TotalSize = 0;
  for (FList = mOptions.FileList; FList != NULL; FList = FList->Next) {
    Size = 0;
    if (FList->FileFlags & FILE_FLAG_EFI) {
      if (mOptions.Verbose) {
        fprintf (stdout, "Processing EFI file    %s\n", FList->FileName);
      }

      Status = ProcessEfiFile (FptrOut, FList, mOptions.VendId, mOptions.DevId, &Size);
    } else if (FList->FileFlags & FILE_FLAG_BINARY) {
      if (mOptions.Verbose) {
        fprintf (stdout, "Processing binary file %s\n", FList->FileName);
      }

      Status = ProcessBinFile (FptrOut, FList, &Size);
    } else {
      fprintf (stdout, "ERROR: File not specified as EFI or binary: %s\n", FList->FileName);
      Status = STATUS_ERROR;
    }

    if (mOptions.Verbose) {
      fprintf (stdout, "  Output size = 0x%X\n", Size);
    }

    if (Status != STATUS_SUCCESS) {
      break;
    }

    TotalSize += Size;
  }
  //
  // Check total size
  //
  if (TotalSize > MAX_OPTION_ROM_SIZE) {
    fprintf (
      stdout,
      "ERROR: Option ROM image size exceeds limit 0x%X bytes\n",
      MAX_OPTION_ROM_SIZE
      );
    Status = STATUS_ERROR;
  }

BailOut:
  if (FptrOut != NULL) {
    fclose (FptrOut);
  }
  //
  // Clean up our file list
  //
  while (mOptions.FileList != NULL) {
    FList = mOptions.FileList->Next;
    free (mOptions.FileList);
    mOptions.FileList = FList;
  }

  return Status;
}
  
UINT8
CheckSum (
  UINT8  *Buffer, 
  UINT32 DataSize,
  UINT32 PaddingSize
  )
/*++
Routine Description:
  Calculate checksum from DataSize of Buffer.

Arguments:
  Buffer      - pointer to data buffer
  DataSize    - size of data buffer in bytes

Return:
  UINT8       - checksum
--*/
{
  UINT8 Checksum = 0;
  while (DataSize-- != 0) {
    Checksum = Checksum + Buffer[DataSize];
  }
  while (PaddingSize-- != 0) {
    Checksum = Checksum + 0xff;
  }
  return Checksum;
}

char *
GetCodeTypeStr (
  UINT8     CodeType
  )
{
  if (CodeType >= sizeof (mCodeTypeStr) / sizeof (*mCodeTypeStr)) {
    CodeType = sizeof (mCodeTypeStr) / sizeof (*mCodeTypeStr) - 1;
  }
  return mCodeTypeStr[CodeType];
}

static
int
ProcessBinFile (
  FILE      *OutFptr,
  FILE_LIST *InFile,
  UINT32    *Size
  )
/*++

Routine Description:
  
  Process a binary input file.

Arguments:

  OutFptr     - file pointer to output binary ROM image file we're creating
  InFile      - structure contains information on the binary file to process
  Size        - pointer to where to return the size added to the output file

Returns:

  0 - successful

--*/
{
  FILE                      *InFptr;
  UINT32                    TotalSize;
  UINT32                    FileSize;
  UINT32                    DataSize;
  UINT32                    PaddingSize;
  UINT8                     *Buffer;
  UINT32                    Status;
  PCI_EXPANSION_ROM_HEADER  *RomHdr;
  PCI_DATA_STRUCTURE        *PciDs;
  UINT8                     ByteCheckSum;

  Status = STATUS_SUCCESS;

  //
  // Try to open the input file
  //
  if ((InFptr = fopen (InFile->FileName, "rb")) == NULL) {
    fprintf (stdout, "ERROR: Failed to open input file %s\n", InFile->FileName);
    return STATUS_ERROR;
  }
  //
  // Seek to the end of the input file and get the file size. Then allocate
  // a buffer to read it in to.
  //
  fseek (InFptr, 0, SEEK_END);
  FileSize = ftell (InFptr);
  if (mOptions.Verbose) {
    fprintf (stdout, "  File size   = 0x%X\n", FileSize);
  }

  fseek (InFptr, 0, SEEK_SET);
  Buffer = (INT8 *) malloc (FileSize);
  if (Buffer == NULL) {
    fprintf (stdout, "ERROR: Memory allocation failed\n");
    Status = STATUS_ERROR;
    goto BailOut;
  }

  if (fread (Buffer, FileSize, 1, InFptr) != 1) {
    fprintf (stdout, "ERROR: Failed to read all bytes from input file\n");
    Status = STATUS_ERROR;
    goto BailOut;
  }

  
  RomHdr = (PCI_EXPANSION_ROM_HEADER *) Buffer;
  PciDs  = (PCI_DATA_STRUCTURE *) (Buffer + RomHdr->PcirOffset);

  //
  // Crude check to make sure it's a legitimate ROM image
  //
  if (RomHdr->Signature != PCI_EXPANSION_ROM_HEADER_SIGNATURE) {
    fprintf (stdout, "ERROR: ROM image file has invalid ROM signature\n");
    Status = STATUS_ERROR;
    goto BailOut;
  }
  //
  // Make sure the pointer to the PCI data structure is within the size of the image.
  // Then check it for valid signature.
  //
  if ((RomHdr->PcirOffset > FileSize) || (RomHdr->PcirOffset == 0)) {
    fprintf (stdout, "ERROR: Invalid PCI data structure offset\n");
    Status = STATUS_ERROR;
    goto BailOut;
  }

  if (PciDs->Signature != PCI_DATA_STRUCTURE_SIGNATURE) {
    fprintf (stdout, "ERROR: PCI data structure has invalid signature\n");
    Status = STATUS_ERROR;
    goto BailOut;
  }

  if ((UINT32) (PciDs->ImageLength * 512) == FileSize) {
    //
    // ImageLength reflects the actual file size correctly.
    //
    DataSize    = FileSize - 1;
    PaddingSize = 0;
    TotalSize   = FileSize;
  } else {
    //
    // ImageLength doesn't reflect the actual file size,
    // 1). add additional 512 bytes if actual file size is multiple of 512
    // 2). add additional X (X <= 512) bytes so that the result size is multiple of 512
    //
    fprintf (stdout, "WARNING: ImageLength in PCI data structure != Actual File Size\n"
                     "         --> add additional padding bytes\n"
                     "         --> adjust ImageLength\n"
            );
    TotalSize   = (FileSize + 0x200) & ~0x1ff;
    DataSize    = FileSize;
    PaddingSize = TotalSize - DataSize - 1;
    PciDs->ImageLength = (UINT16) (TotalSize / 512);
  }

  //
  // Check size
  //
  if (TotalSize > MAX_OPTION_ROM_SIZE) {
    fprintf (
      stdout,
      "ERROR: Option ROM image %s size exceeds limit 0x%X bytes\n",
      InFile->FileName,
      MAX_OPTION_ROM_SIZE
      );
    Status = STATUS_ERROR;
    goto BailOut;
  }

  //
  // Return the size to the caller so they can keep track of the running total.
  //
  *Size = TotalSize;

  //
  // If this is the last image, then set the LAST bit unless requested not
  // to via the command-line -l argument. Otherwise, make sure you clear it.
  //
  if ((InFile->Next == NULL) && (mOptions.NoLast == 0)) {
    PciDs->Indicator |= INDICATOR_LAST;
  } else {
    PciDs->Indicator &= ~INDICATOR_LAST;
  }

  ByteCheckSum = -CheckSum (Buffer, DataSize, PaddingSize);

  if (fwrite (Buffer, DataSize, 1, OutFptr) != 1) {
    fprintf (stdout, "ERROR: Failed to write all file bytes to output file\n");
    Status = STATUS_ERROR;
    goto BailOut;
  }

  while (PaddingSize-- != 0) {
    putc (~0, OutFptr);
  }
  putc (ByteCheckSum, OutFptr);

BailOut:
  if (InFptr != NULL) {
    fclose (InFptr);
  }

  if (Buffer != NULL) {
    free (Buffer);
  }
  //
  // Print the file name if errors occurred
  //
  if (Status != STATUS_SUCCESS) {
    fprintf (stdout, "Error processing binary file %s\n", InFile->FileName);
  }

  return Status;
}

static
int
ProcessEfiFile (
  FILE      *OutFptr,
  FILE_LIST *InFile,
  UINT16    VendId,
  UINT16    DevId,
  UINT32    *Size
  )
/*++

Routine Description:
  
  Process a PE32 EFI file.

Arguments:

  OutFptr     - file pointer to output binary ROM image file we're creating
  InFile      - structure contains information on the PE32 file to process
  VendId      - vendor ID as required in the option ROM header
  DevId       - device ID as required in the option ROM header
  Size        - pointer to where to return the size added to the output file

Returns:

  0 - successful

--*/
{
  UINT32                        Status;
  FILE                          *InFptr;
  EFI_PCI_EXPANSION_ROM_HEADER  RomHdr;
  PCI_DATA_STRUCTURE            PciDs;
  UINT32                        FileSize;
  UINT32                        CompressedFileSize;
  UINT8                         *Buffer;
  UINT8                         *CompressedBuffer;
  UINT8                         *TempBufferPtr;
  UINT32                        TotalSize;
  UINT32                        HeaderSize;
  UINT16                        MachineType;
  UINT16                        SubSystem;
  UINT32                        HeaderPadBytes;

  //
  // Try to open the input file
  //
  if ((InFptr = fopen (InFile->FileName, "rb")) == NULL) {
    fprintf (stdout, "ERROR: Failed to open input file %s\n", InFile->FileName);
    return STATUS_ERROR;
  }
  //
  // Initialize our buffer pointers to null.
  //
  Buffer            = NULL;
  CompressedBuffer  = NULL;

  //
  // Double-check the file to make sure it's what we expect it to be
  //
  Status = CheckPE32File (InFptr, &MachineType, &SubSystem);
  if (Status != STATUS_SUCCESS) {
    goto BailOut;
  }
  //
  // Seek to the end of the input file and get the file size
  //
  fseek (InFptr, 0, SEEK_END);
  FileSize = ftell (InFptr);

  //
  // Get the size of the headers we're going to put in front of the image. The
  // EFI header must be aligned on a 4-byte boundary, so pad accordingly.
  //
  if (sizeof (RomHdr) & 0x03) {
    HeaderPadBytes = 4 - (sizeof (RomHdr) & 0x03);
  } else {
    HeaderPadBytes = 0;
  }

  HeaderSize = sizeof (PCI_DATA_STRUCTURE) + HeaderPadBytes + sizeof (EFI_PCI_EXPANSION_ROM_HEADER);
  if (mOptions.Verbose) {
    fprintf (stdout, "  File size   = 0x%X\n", FileSize);
  }
  //
  // Allocate memory for the entire file (in case we have to compress), then
  // seek back to the beginning of the file and read it into our buffer.
  //
  Buffer = (INT8 *) malloc (FileSize);
  if (Buffer == NULL) {
    fprintf (stdout, "ERROR: Memory allocation failed\n");
    Status = STATUS_ERROR;
    goto BailOut;
  }

  fseek (InFptr, 0, SEEK_SET);
  if (fread (Buffer, FileSize, 1, InFptr) != 1) {
    fprintf (stdout, "ERROR: Failed to read all bytes from input file\n");
    Status = STATUS_ERROR;
    goto BailOut;
  }
  //
  // Now determine the size of the final output file. It's either the header size
  // plus the file's size, or the header size plus the compressed file size.
  //
  if (InFile->FileFlags & FILE_FLAG_COMPRESS) {
    //
    // Allocate a buffer into which we can compress the image, compress it,
    // and use that size as the new size.
    //
    CompressedBuffer = (INT8 *) malloc (FileSize);
    if (CompressedBuffer == NULL) {
      fprintf (stdout, "ERROR: Memory allocation failed\n");
      Status = STATUS_ERROR;
      goto BailOut;
    }

    CompressedFileSize  = FileSize;
    Status              = EfiCompress (Buffer, FileSize, CompressedBuffer, &CompressedFileSize);
    if (Status != STATUS_SUCCESS) {
      fprintf (stdout, "ERROR: Compression failed\n");
      goto BailOut;
    }
    //
    // Now compute the size, then swap buffer pointers.
    //
    if (mOptions.Verbose) {
      fprintf (stdout, "  Comp size   = 0x%X\n", CompressedFileSize);
    }

    TotalSize         = CompressedFileSize + HeaderSize;
    FileSize          = CompressedFileSize;
    TempBufferPtr     = Buffer;
    Buffer            = CompressedBuffer;
    CompressedBuffer  = TempBufferPtr;
  } else {
    TotalSize = FileSize + HeaderSize;
  }
  //
  // Total size must be an even multiple of 512 bytes
  //
  TotalSize = (TotalSize + 0x1ff) & ~0x1ff;

  //
  // Check size
  //
  if (TotalSize > MAX_OPTION_ROM_SIZE) {
    fprintf (
      stdout,
      "ERROR: Option ROM image %s size exceeds limit 0x%X bytes\n",
      InFile->FileName,
      MAX_OPTION_ROM_SIZE
      );
    Status = STATUS_ERROR;
    goto BailOut;
  }
  //
  // Return the size to the caller so they can keep track of the running total.
  //
  *Size = TotalSize;

  //
  // Now fill in the ROM header. These values come from chapter 18 of the
  // EFI 1.02 specification.
  //
  memset (&RomHdr, 0, sizeof (RomHdr));
  RomHdr.Signature            = PCI_EXPANSION_ROM_HEADER_SIGNATURE;
  RomHdr.InitializationSize   = (UINT16) (TotalSize / 512);
  RomHdr.EfiSignature         = EFI_PCI_EXPANSION_ROM_HEADER_EFISIGNATURE;
  RomHdr.EfiSubsystem         = SubSystem;
  RomHdr.EfiMachineType       = MachineType;
  RomHdr.EfiImageHeaderOffset = (UINT16) HeaderSize;
  RomHdr.PcirOffset           = (UINT16) (sizeof (RomHdr) + HeaderPadBytes);
  //
  // Set image as compressed or not
  //
  if (InFile->FileFlags & FILE_FLAG_COMPRESS) {
    RomHdr.CompressionType = EFI_PCI_EXPANSION_ROM_HEADER_COMPRESSED;
  }
  //
  // Fill in the PCI data structure
  //
  memset (&PciDs, 0, sizeof (PCI_DATA_STRUCTURE));

  PciDs.Signature = PCI_DATA_STRUCTURE_SIGNATURE;
  PciDs.VendorId  = VendId;
  PciDs.DeviceId  = DevId;
  PciDs.Length    = (UINT16) sizeof (PCI_DATA_STRUCTURE);
  PciDs.Revision  = 0;
  //
  // Class code and code revision from the command line (optional)
  //
  PciDs.ClassCode[0]  = (UINT8) InFile->ClassCode;
  PciDs.ClassCode[1]  = (UINT8) (InFile->ClassCode >> 8);
  PciDs.ClassCode[2]  = (UINT8) (InFile->ClassCode >> 16);
  PciDs.ImageLength   = RomHdr.InitializationSize;
  PciDs.CodeRevision  = InFile->CodeRevision;
  PciDs.CodeType      = PCI_CODE_TYPE_EFI_IMAGE;

  //
  // If this is the last image, then set the LAST bit unless requested not
  // to via the command-line -l argument.
  //
  if ((InFile->Next == NULL) && (mOptions.NoLast == 0)) {
    PciDs.Indicator |= INDICATOR_LAST;
  }
  //
  // Write the ROM header to the output file
  //
  if (fwrite (&RomHdr, sizeof (RomHdr), 1, OutFptr) != 1) {
    fprintf (stdout, "ERROR: Failed to write ROM header to output file\n");
    Status = STATUS_ERROR;
    goto BailOut;
  }

  //
  // Write pad bytes to align the PciDs
  //
  while (HeaderPadBytes > 0) {
    if (putc (0, OutFptr) == EOF) {
      fprintf (stdout, "ERROR: Failed to write ROM header pad bytes to output file\n");
      Status = STATUS_ERROR;
      goto BailOut;
    }

    HeaderPadBytes--;
  }
  //
  // Write the PCI data structure header to the output file
  //
  if (fwrite (&PciDs, sizeof (PciDs), 1, OutFptr) != 1) {
    fprintf (stdout, "ERROR: Failed to write PCI ROM header to output file\n");
    Status = STATUS_ERROR;
    goto BailOut;
  }
  //
  // Keep track of how many bytes left to write
  //
  TotalSize -= HeaderSize;

  //
  // Now dump the input file's contents to the output file
  //
  if (fwrite (Buffer, FileSize, 1, OutFptr) != 1) {
    fprintf (stdout, "ERROR: Failed to write all file bytes to output file\n");
    Status = STATUS_ERROR;
    goto BailOut;
  }

  TotalSize -= FileSize;
  //
  // Pad the rest of the image to make it a multiple of 512 bytes
  //
  while (TotalSize > 0) {
    if (putc (~0, OutFptr) == EOF) {
      fprintf (stdout, "ERROR: Failed to write trailing pad bytes output file\n");
      Status = STATUS_ERROR;
      goto BailOut;
    }

    TotalSize--;
  }

BailOut:
  if (InFptr != NULL) {
    fclose (InFptr);
  }

  //
  // Free up our buffers
  //
  if (Buffer != NULL) {
    free (Buffer);
  }

  if (CompressedBuffer != NULL) {
    free (CompressedBuffer);
  }
  //
  // Print the file name if errors occurred
  //
  if (Status != STATUS_SUCCESS) {
    fprintf (stdout, "Error processing EFI file %s\n", InFile->FileName);
  }

  return Status;
}

static
int
CheckPE32File (
  FILE      *Fptr,
  UINT16    *MachineType,
  UINT16    *SubSystem
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

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

  //
  // Position to the start of the file
  //
  fseek (Fptr, 0, SEEK_SET);

  //
  // Read the DOS header
  //
  if (fread (&DosHeader, sizeof (DosHeader), 1, Fptr) != 1) {
    fprintf (stdout, "ERROR: Failed to read the DOS stub from the input file\n");
    return STATUS_ERROR;
  }
  //
  // Check the magic number (0x5A4D)
  //
  if (DosHeader.e_magic != EFI_IMAGE_DOS_SIGNATURE) {
    fprintf (stdout, "ERROR: Input file does not appear to be a PE32 image (magic number)\n");
    return STATUS_ERROR;
  }
  //
  // Position into the file and check the PE signature
  //
  fseek (Fptr, (long) DosHeader.e_lfanew, SEEK_SET);
  if (fread (&PESig, sizeof (PESig), 1, Fptr) != 1) {
    fprintf (stdout, "ERROR: Failed to read PE signature bytes from input file\n");
    return STATUS_ERROR;
  }
  //
  // Check the PE signature in the header "PE\0\0"
  //
  if (PESig != EFI_IMAGE_NT_SIGNATURE) {
    fprintf (stdout, "ERROR: Input file does not appear to be a PE32 image (signature)\n");
    return STATUS_ERROR;
  }
  //
  // Read the file header and stuff their MachineType
  //
  if (fread (&FileHdr, sizeof (FileHdr), 1, Fptr) != 1) {
    fprintf (stdout, "ERROR: Failed to read PE file header from input file\n");
    return STATUS_ERROR;
  }

  memcpy ((char *) MachineType, &FileHdr.Machine, 2);

  //
  // Read the optional header so we can get the subsystem
  //
  if (fread (&OptionalHdr, sizeof (OptionalHdr), 1, Fptr) != 1) {
    fprintf (stdout, "ERROR: Failed to read COFF optional header from input file\n");
    return STATUS_ERROR;
  }

  *SubSystem = OptionalHdr.Subsystem;
  if (mOptions.Verbose) {
    fprintf (stdout, "  Got subsystem = 0x%X from image\n", (int) *SubSystem);
  }
  //
  // Good to go
  //
  return STATUS_SUCCESS;
}

static
int
ParseCommandLine (
  int         Argc,
  char        *Argv[],
  OPTIONS     *Options
  )
/*++

Routine Description:
  
  Given the Argc/Argv program arguments, and a pointer to an options structure,
  parse the command-line options and check their validity.


Arguments:

  Argc            - standard C main() argument count
  Argv[]          - standard C main() argument list
  Options         - pointer to a structure to store the options in

Returns:

  STATUS_SUCCESS    success
  non-zero          otherwise

--*/
//
{
  FILE_LIST *FileList;

  FILE_LIST *PrevFileList;
  UINT32    FileFlags;
  UINT32    ClassCode;
  UINT32    CodeRevision;

  FileFlags = 0;

  //
  // Clear out the options
  //
  memset ((char *) Options, 0, sizeof (OPTIONS));

  //
  // To avoid compile warnings
  //
  FileList                = PrevFileList = NULL;

  ClassCode               = 0;
  CodeRevision            = 0;
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
  while (Argc > 0) {
    if ((Argv[0][0] == '-') || (Argv[0][0] == '/')) {
      //
      // To simplify string comparisons, replace slashes with dashes
      //
      Argv[0][0] = '-';

      //
      // Vendor ID specified with -v
      //
      if (_stricmp (Argv[0], "-v") == 0) {
        //
        // Make sure there's another parameter
        //
        if (Argc > 1) {
          Options->VendId       = (UINT16) strtol (Argv[1], NULL, 16);
          Options->VendIdValid  = 1;
        } else {
          fprintf (
            stdout,
            "ERROR: Missing Vendor ID with %s\n\n",
            Argv[0]
            );
          Usage ();
          return STATUS_ERROR;
        }

        Argv++;
        Argc--;
      } else if (_stricmp (Argv[0], "-d") == 0) {
        //
        // Device ID specified with -d
        // Make sure there's another parameter
        //
        if (Argc > 1) {
          Options->DevId      = (UINT16) strtol (Argv[1], NULL, 16);
          Options->DevIdValid = 1;
        } else {
          fprintf (
            stdout,
            "ERROR: Missing Device ID with %s\n\n",
            Argv[0]
            );
          Usage ();
          return STATUS_ERROR;
        }

        Argv++;
        Argc--;
      } else if (_stricmp (Argv[0], "-o") == 0) {
        //
        // Output filename specified with -o
        // Make sure there's another parameter
        //
        if (Argc > 1) {
          strcpy (Options->OutFileName, Argv[1]);
        } else {
          fprintf (
            stdout,
            "ERROR: Missing output file name with %s\n\n",
            Argv[0]
            );
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
      } else if (_stricmp (Argv[0], "-b") == 0) {
        //
        // Specify binary files with -b
        //
        FileFlags = (FileFlags & ~FILE_FLAG_EFI) | FILE_FLAG_BINARY;
      } else if ((_stricmp (Argv[0], "-e") == 0) || (_stricmp (Argv[0], "-ec") == 0)) {
        //
        // Specify EFI files with -e. Specify EFI-compressed with -ec.
        //
        FileFlags = (FileFlags &~FILE_FLAG_BINARY) | FILE_FLAG_EFI;
        if ((Argv[0][2] == 'c') || (Argv[0][2] == 'C')) {
          FileFlags |= FILE_FLAG_COMPRESS;
        }
        //
        // Specify not to set the LAST bit in the last file with -l
        //
      } else if (_stricmp (Argv[0], "-l") == 0) {
        Options->NoLast = 1;
      } else if (_stricmp (Argv[0], "-p") == 0) {
        //
        // -v for verbose would have been nicer, but it's already used. Let's use
        // -p for prolix (wordy) output
        //
        Options->Verbose = 1;
      } else if (_stricmp (Argv[0], "-dump") == 0) {
        //
        // -dump for dumping a ROM image. In this case, say that the device id
        // and vendor id are valid so we don't have to specify bogus ones on the
        // command line.
        //
        Options->DumpOption   = 1;

        Options->VendIdValid  = 1;
        Options->DevIdValid   = 1;
        FileFlags             = FILE_FLAG_BINARY;
      } else if (_stricmp (Argv[0], "-cc") == 0) {
        //
        // Class code value for the next file in the list.
        // Make sure there's another parameter
        //
        if (Argc > 1) {
          //
          // No error checking on the return value. Could check for LONG_MAX,
          // LONG_MIN, or 0 class code value if desired. Check range (3 bytes)
          // at least.
          //
          ClassCode = (UINT32) strtol (Argv[1], NULL, 16);
          if (ClassCode & 0xFF000000) {
            fprintf (stdout, "ERROR: Class code %s out of range\n", Argv[1]);
            return STATUS_ERROR;
          }
        } else {
          fprintf (
            stdout,
            "ERROR: Missing class code value with %s\n\n",
            Argv[0]
            );
          Usage ();
          return STATUS_ERROR;
        }

        Argv++;
        Argc--;
      } else if (_stricmp (Argv[0], "-rev") == 0) {
        //
        // Code revision in the PCI data structure. The value is for the next
        // file in the list.
        // Make sure there's another parameter
        //
        if (Argc > 1) {
          //
          // No error checking on the return value. Could check for LONG_MAX,
          // LONG_MIN, or 0 value if desired. Check range (2 bytes)
          // at least.
          //
          CodeRevision = (UINT32) strtol (Argv[1], NULL, 16);
          if (CodeRevision & 0xFFFF0000) {
            fprintf (stdout, "ERROR: Code revision %s out of range\n", Argv[1]);
            return STATUS_ERROR;
          }
        } else {
          fprintf (
            stdout,
            "ERROR: Missing code revision value with %s\n\n",
            Argv[0]
            );
          Usage ();
          return STATUS_ERROR;
        }

        Argv++;
        Argc--;
      } else {
        fprintf (stdout, "ERROR: Invalid option specified: %s\n\n", Argv[0]);
        Usage ();
        return STATUS_ERROR;
      }
    } else {
      //
      // Not a slash-option argument. Must be a file name. Make sure they've specified
      // -e or -b already.
      //
      if ((FileFlags & (FILE_FLAG_BINARY | FILE_FLAG_EFI)) == 0) {
        fprintf (stdout, "ERROR: Missing -e or -b with input file %s\n", Argv[0]);
        return STATUS_ERROR;
      }
      //
      // Create a new file structure
      //
      FileList = (FILE_LIST *) malloc (sizeof (FILE_LIST));
      if (FileList == NULL) {
        fprintf (stdout, "ERROR: Memory allocation failure\n");
        return STATUS_ERROR;
      }

      memset ((char *) FileList, 0, sizeof (FILE_LIST));
      FileList->FileName  = Argv[0];
      FileList->FileFlags = FileFlags;
      if (Options->FileList == NULL) {
        Options->FileList = FileList;
      } else {
        if (PrevFileList == NULL) {
          PrevFileList = FileList;
        } else {          
          PrevFileList->Next = FileList;
        }
      }

      PrevFileList = FileList;
      //
      // Set the class code and code revision for this file, then reset the values.
      //
      FileList->ClassCode     = ClassCode;
      FileList->CodeRevision  = (UINT16) CodeRevision;
      ClassCode               = 0;
      CodeRevision            = 0;
    }
    //
    // Next argument
    //
    Argv++;
    Argc--;
  }
  //
  // Make sure they specified a device ID and vendor ID
  //
  if (!Options->VendIdValid) {
    fprintf (stdout, "ERROR: Missing Vendor ID on command line\n\n");
    Usage ();
    return STATUS_ERROR;
  }

  if (!Options->DevIdValid) {
    fprintf (stdout, "ERROR: Missing Device ID on command line\n\n");
    Usage ();
    return STATUS_ERROR;
  }
  //
  // Must have specified some files
  //
  if (Options->FileList == NULL) {
    fprintf (stdout, "ERROR: Missing input file name\n");
    Usage ();
    return STATUS_ERROR;
  }

  return 0;
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
  int        Index;
  const char *Str[] = {
    UTILITY_NAME" "UTILITY_VERSION" - Intel EFI Make Option ROM Utility",
    "  Copyright (C), 1999 - 2008 Intel Coproration",

#if ( defined(UTILITY_BUILD) && defined(UTILITY_VENDOR) )
    "  Built from "UTILITY_BUILD", project of "UTILITY_VENDOR,
#endif

    "",
    "Usage:",
    "  "UTILITY_NAME" [OPTION]... SOURCE ...",
    "Description:",
    "  Create an option ROM image from a list of input files",
    "Options:",
    "  -v  VendorId      - required hex PCI Vendor ID for the device",
    "  -d  DeviceId      - required hex PCI Device ID for the device",
    "  -o  OutFileName   - optional output file name. Default is the first input",
    "                      file name with a "DEFAULT_OUTPUT_EXTENSION" file extension",
    "  -e  SOURCE ...    - input are EFI PE32 image file(s)",
    "  -b  SOURCE ...    - input are Legacy binary file(s)",
    "  -ec SOURCE ...    - input are EFI PE32 image file(s) and should be compressed",
    "  -p                - for verbose output",
    "  -l                - to not automatically set the LAST bit on the last file",
    "  -cc ClassCode     - to use hex ClassCode in the PCI data structure header for",
    "                      the following SOURCE(s)",
    "  -rev Revision     - to use hex Revision in the PCI data structure header for",
    "                      the following one SOURCE",
    "  -dump             - to dump the headers of an existing option ROM image",
    "Example Usage:",
    "  EfiRom -v 0xABCD -d 0x1234 -b File1.bin File2.bin -e File1.efi File2.efi",
    NULL
  };

  for (Index = 0; Str[Index] != NULL; Index++) {
    fprintf (stdout, "%s\n", Str[Index]);
  }
}

static
void
DumpImage (
  FILE_LIST *InFile
  )
/*++

Routine Description:

  GC_TODO: Add function description

Arguments:

  InFile  - GC_TODO: add argument description

Returns:

  GC_TODO: add return values

--*/
{
  PCI_EXPANSION_ROM_HEADER      PciRomHdr;
  FILE                          *InFptr;
  UINT32                        ImageStart;
  UINT32                        ImageCount;
  UINT16                        DeviceId;
  EFI_PCI_EXPANSION_ROM_HEADER  EfiRomHdr;
  PCI_3_0_DATA_STRUCTURE        PciDs;

  //
  // Open the input file
  //
  if ((InFptr = fopen (InFile->FileName, "rb")) == NULL) {
    fprintf (
      stdout,
      "ERROR: Could not open input file %s\n",
      InFile->FileName
      );
    return ;
  }
  //
  // Go through the image and dump the header stuff for each
  //
  ImageCount = 0;
  for (;;) {
    //
    // Save our postition in the file, since offsets in the headers
    // are relative to the particular image.
    //
    ImageStart = ftell (InFptr);
    ImageCount++;

    //
    // Read the option ROM header. Have to assume a raw binary image for now.
    //
    if (fread (&PciRomHdr, sizeof (PciRomHdr), 1, InFptr) != 1) {
      fprintf (stdout, "ERROR: Failed to read PCI ROM header from file\n");
      goto BailOut;
    }

    //
    // Dump the contents of the header
    //
    fprintf (stdout, "Image %d -- Offset 0x%X\n", ImageCount, ImageStart);
    fprintf (stdout, "  ROM header contents\n");
    fprintf (stdout, "    Signature              0x%04X\n", (UINT32) PciRomHdr.Signature);
    fprintf (stdout, "    PCIR offset            0x%04X\n", (UINT32) PciRomHdr.PcirOffset);
    //
    // Find PCI data structure
    //
    if (fseek (InFptr, ImageStart + PciRomHdr.PcirOffset, SEEK_SET)) {
      fprintf (stdout, "ERROR: Failed to seek to PCI data structure\n");
      goto BailOut;
    }
    //
    // Read and dump the PCI data structure
    //
    if (fread (&PciDs, sizeof (PciDs), 1, InFptr) != 1) {
      fprintf (stdout, "ERROR: Failed to read PCI data structure from file\n");
      goto BailOut;
    }

    fprintf (stdout, "  PCI Data Structure\n");
    fprintf (
      stdout,
      "    Signature              %c%c%c%c\n",
      (char) PciDs.Signature,
      (char) (PciDs.Signature >> 8),
      (char) (PciDs.Signature >> 16),
      (char) (PciDs.Signature >> 24)
      );
    fprintf (stdout, "    Vendor ID              0x%04X\n", PciDs.VendorId);
    fprintf (stdout, "    Device ID              0x%04X\n", PciDs.DeviceId);
    fprintf (stdout, "    Structure Length       0x%04X\n", PciDs.Length);
    fprintf (stdout, "    PCI Revision           0x%02X\n", PciDs.Revision);
    fprintf (
      stdout,
      "    Class Code             0x%06X\n",
      (UINT32) (PciDs.ClassCode[0] | (PciDs.ClassCode[1] << 8) | (PciDs.ClassCode[2] << 16))
      );
    fprintf (stdout, "    Image size             0x%X\n", PciDs.ImageLength * 512);
    fprintf (stdout, "    Code revision          0x%04X\n", PciDs.CodeRevision);
    fprintf (stdout, "    Indicator              0x%02X", (UINT32) PciDs.Indicator);
    //
    // Print the indicator, used to flag the last image
    //
    if ((PciDs.Indicator & INDICATOR_LAST) == INDICATOR_LAST) {
      fprintf (stdout, "   (last image)\n");
    } else {
      fprintf (stdout, "\n");
    }

    //
    // Print the code type. If EFI code, then we can provide more info.
    //
    fprintf (stdout, "    Code type              0x%02X   (%s)\n", (UINT32) PciDs.CodeType, GetCodeTypeStr (PciDs.CodeType));
    if (PciDs.CodeType == PCI_CODE_TYPE_EFI_IMAGE) {
      //
      // Re-read the header as an EFI ROM header, then dump more info
      //
      fprintf (stdout, "  EFI ROM header contents\n");
      memcpy (&EfiRomHdr, &PciRomHdr, sizeof (EfiRomHdr));
      //
      // Now dump more info
      //
      fprintf (stdout, "    EFI Signature          0x%04X\n", EfiRomHdr.EfiSignature);
      fprintf (
        stdout,
        "    Compression Type       0x%04X ",
        (UINT32) EfiRomHdr.CompressionType
        );
      if (EfiRomHdr.CompressionType == EFI_PCI_EXPANSION_ROM_HEADER_COMPRESSED) {
        fprintf (stdout, "(compressed)\n");
      } else {
        fprintf (stdout, "(not compressed)\n");
      }

      fprintf (
        stdout,
        "    Machine type           0x%04X (%s)\n",
        EfiRomHdr.EfiMachineType,
        GetMachineTypeStr (EfiRomHdr.EfiMachineType)
        );
      fprintf (
        stdout,
        "    Subsystem              0x%04X (%s)\n",
        EfiRomHdr.EfiSubsystem,
        GetSubsystemTypeStr (EfiRomHdr.EfiSubsystem)
        );
      fprintf (
        stdout,
        "    EFI image offset       0x%04X (@0x%X)\n",
        (UINT32) EfiRomHdr.EfiImageHeaderOffset,
        (UINT32) (EfiRomHdr.EfiImageHeaderOffset + ImageStart)
        );
    }

    //
    // Dump additional information for PCI 3.0 OpROM
    //
    if (PciDs.Revision >= 3) {
      fprintf (stdout, "  Extended for PCI 3.0\n");
      
      if (PciDs.DeviceListOffset != 0) {
        if (fseek (InFptr, ImageStart + PciRomHdr.PcirOffset + PciDs.DeviceListOffset, SEEK_SET)) {
          fprintf (stdout, "ERROR: Failed to seek to supported Device List\n");
          goto BailOut;
        }
        fprintf (stdout, "    Device ID List         ");
        while (TRUE) {
          if (fread (&DeviceId, sizeof (DeviceId), 1, InFptr) != 1) {
            fprintf (stdout, "ERROR: Failed to read supported DeviceId from DeviceId List\n");
            goto BailOut;
          }
          if (DeviceId == 0) {
            break;
          }
          fprintf (stdout, "0x%04X ", DeviceId);
        }
        fprintf (stdout, "\n");
      }
      fprintf (stdout, "    Max Runtime Image Length 0x%08X\n", PciDs.MaxRuntimeImageLength * 512);
      if (PciDs.Length == sizeof (PCI_3_0_DATA_STRUCTURE)) {
        fprintf (stdout, "    Config Utility Header  0x%04X\n", PciDs.ConfigUtilityCodeHeaderOffset);
        fprintf (stdout, "    DMTF CLP Entry Point   0x%04X\n", PciDs.DMTFCLPEntryPointOffset);
      } else {
        fprintf (stdout, "WARNING: Oprom declars 3.0 revision with wrong structure length 0x%04X\n", PciDs.Length);
      }
    }

    //
    // If last image, then we're done
    //
    if ((PciDs.Indicator & INDICATOR_LAST) == INDICATOR_LAST) {
      goto BailOut;
    }
    //
    // Seek to the start of the next image
    //
    if (fseek (InFptr, ImageStart + (PciDs.ImageLength * 512), SEEK_SET)) {
      fprintf (stdout, "ERROR: Failed to seek to next image\n");
      goto BailOut;
    }
  }

BailOut:
  fclose (InFptr);
}

char *
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
char *
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

