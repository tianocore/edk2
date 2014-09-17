/** @file
Calculate Crc32 value and Verify Crc32 value for input data.

Copyright (c) 2007 - 2014, Intel Corporation. All rights reserved.<BR>
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
#include "EfiUtilityMsgs.h"
#include "CommonLib.h"
#include "Crc32.h"

#define UTILITY_NAME            "GenCrc32"
#define UTILITY_MAJOR_VERSION   0
#define UTILITY_MINOR_VERSION   2

#define CRC32_NULL              0
#define CRC32_ENCODE            1
#define CRC32_DECODE            2 

VOID
Version (
  VOID
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
  fprintf (stdout, "%s Version %d.%d %s \n", UTILITY_NAME, UTILITY_MAJOR_VERSION, UTILITY_MINOR_VERSION, __BUILD_VERSION);
}

VOID
Usage (
  VOID
  )
/*++

Routine Description:

  Displays the utility usage syntax to STDOUT

Arguments:

  None

Returns:

  None

--*/
{
  //
  // Summary usage
  //
  fprintf (stdout, "Usage: GenCrc32 -e|-d [options] <input_file>\n\n");
  
  //
  // Copyright declaration
  // 
  fprintf (stdout, "Copyright (c) 2007 - 2014, Intel Corporation. All rights reserved.\n\n");

  //
  // Details Option
  //
  fprintf (stdout, "optional arguments:\n");
  fprintf (stdout, "  -h, --help            Show this help message and exit\n");
  fprintf (stdout, "  --version             Show program's version number and exit\n");
  fprintf (stdout, "  --debug [DEBUG]       Output DEBUG statements, where DEBUG_LEVEL is 0 (min)\n\
                        - 9 (max)\n");
  fprintf (stdout, "  -v, --verbose         Print informational statements\n");
  fprintf (stdout, "  -q, --quiet           Returns the exit code, error messages will be\n\
                        displayed\n");
  fprintf (stdout, "  -s, --silent          Returns only the exit code; informational and error\n\
                        messages are not displayed\n");
  fprintf (stdout, "  -e, --encode          Calculate CRC32 value for the input file\n");
  fprintf (stdout, "  -d, --decode          Verify CRC32 value for the input file\n");
  fprintf (stdout, "  -o OUTPUT_FILENAME, --output OUTPUT_FILENAME\n\
                        Output file name\n");
  fprintf (stdout, "  --sfo                 Reserved for future use\n");
  
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
  CHAR8                   *OutputFileName;
  CHAR8                   *InputFileName;
  UINT8                   *FileBuffer;
  UINT32                  FileSize;
  UINT64                  LogLevel;
  UINT8                   FileAction;
  UINT32                  Crc32Value;
  FILE                    *InFile;
  FILE                    *OutFile;
  
  //
  // Init local variables
  //
  LogLevel       = 0;
  Status         = EFI_SUCCESS;
  InputFileName  = NULL;
  OutputFileName = NULL;
  FileAction     = CRC32_NULL;
  InFile         = NULL;
  OutFile        = NULL;
  Crc32Value     = 0;
  FileBuffer     = NULL;

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
    Usage ();
    return STATUS_SUCCESS;    
  }

  if (stricmp (argv[0], "--version") == 0) {
    Version ();
    return STATUS_SUCCESS;    
  }

  while (argc > 0) {
    if ((stricmp (argv[0], "-o") == 0) || (stricmp (argv[0], "--output") == 0)) {
      if (argv[1] == NULL || argv[1][0] == '-') {
        Error (NULL, 0, 1003, "Invalid option value", "Output File name is missing for -o option");
        goto Finish;
      }
      OutputFileName = argv[1];
      argc -= 2;
      argv += 2;
      continue; 
    }

    if ((stricmp (argv[0], "-e") == 0) || (stricmp (argv[0], "--encode") == 0)) {
      FileAction     = CRC32_ENCODE;
      argc --;
      argv ++;
      continue; 
    }

    if ((stricmp (argv[0], "-d") == 0) || (stricmp (argv[0], "--decode") == 0)) {
      FileAction     = CRC32_DECODE;
      argc --;
      argv ++;
      continue; 
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

    if (stricmp (argv[0], "--debug") == 0) {
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

    if (argv[0][0] == '-') {
      Error (NULL, 0, 1000, "Unknown option", argv[0]);
      goto Finish;
    }

    //
    // Get Input file file name.
    //
    InputFileName = argv[0];
    argc --;
    argv ++;
  }

  VerboseMsg ("%s tool start.", UTILITY_NAME);
  
  //
  // Check Input paramters
  //
  if (FileAction == CRC32_NULL) {
    Error (NULL, 0, 1001, "Missing option", "either the encode or the decode option must be specified!");
    return STATUS_ERROR;
  } else if (FileAction == CRC32_ENCODE) {
    VerboseMsg ("File will be encoded by Crc32");
  } else if (FileAction == CRC32_DECODE) {
    VerboseMsg ("File will be decoded by Crc32");
  }
  
  if (InputFileName == NULL) {
    Error (NULL, 0, 1001, "Missing option", "Input files are not specified");
    goto Finish;
  } else {
    VerboseMsg ("Input file name is %s", InputFileName);
  }

  if (OutputFileName == NULL) {
    Error (NULL, 0, 1001, "Missing option", "Output file are not specified");
    goto Finish;
  } else {
    VerboseMsg ("Output file name is %s", OutputFileName);
  }
  
  //
  // Open Input file and read file data.
  //
  InFile = fopen (LongFilePath (InputFileName), "rb");
  if (InFile == NULL) {
    Error (NULL, 0, 0001, "Error opening file", InputFileName);
    return STATUS_ERROR;
  }

  fseek (InFile, 0, SEEK_END);
  FileSize = ftell (InFile);
  fseek (InFile, 0, SEEK_SET);
  
  FileBuffer = (UINT8 *) malloc (FileSize);
  if (FileBuffer == NULL) {
    Error (NULL, 0, 4001, "Resource", "memory cannot be allcoated!");
    goto Finish;
  }
  
  fread (FileBuffer, 1, FileSize, InFile);
  fclose (InFile);
  VerboseMsg ("the size of the input file is %u bytes", (unsigned) FileSize);
  
  //
  // Open output file
  //
  OutFile = fopen (LongFilePath (OutputFileName), "wb");
  if (OutFile == NULL) {
    Error (NULL, 0, 0001, "Error opening file", OutputFileName);
    goto Finish;
  }
  
  //
  // Calculate Crc32 value
  //
  if (FileAction == CRC32_ENCODE) {
    Status = CalculateCrc32 (FileBuffer, FileSize, &Crc32Value);
    if (Status != EFI_SUCCESS) {
      Error (NULL, 0, 3000, "Invalid", "Calculate CRC32 value failed!");
      goto Finish;
    }
    //
    // Done, write output file.
    //
    fwrite (&Crc32Value, 1, sizeof (Crc32Value), OutFile);
    VerboseMsg ("The calculated CRC32 value is 0x%08x", (unsigned) Crc32Value);
    fwrite (FileBuffer, 1, FileSize, OutFile);
    VerboseMsg ("the size of the encoded file is %u bytes", (unsigned) FileSize + sizeof (UINT32));
  } else {
    //
    // Verify Crc32 Value
    //
    Status = CalculateCrc32 (FileBuffer + sizeof (UINT32), FileSize - sizeof (UINT32), &Crc32Value);
    if (Status != EFI_SUCCESS) {
      Error (NULL, 0, 3000, "Invalid", "Calculate CRC32 value failed!");
      goto Finish;
    }
    VerboseMsg ("The calculated CRC32 value is 0x%08x and File Crc32 value is 0x%08x", (unsigned) Crc32Value, (unsigned) (*(UINT32 *)FileBuffer));
    if (Crc32Value != *(UINT32 *)FileBuffer) {
      Error (NULL, 0, 3000, "Invalid", "CRC32 value of input file is not correct!");
      Status = STATUS_ERROR;
      goto Finish;
    }
    //
    // Done, write output file.
    //
    fwrite (FileBuffer + sizeof (UINT32), 1, FileSize - sizeof (UINT32), OutFile);
    VerboseMsg ("the size of the decoded file is %u bytes", (unsigned) FileSize - sizeof (UINT32));
  }

Finish:
  if (FileBuffer != NULL) {
    free (FileBuffer);
  }
  
  if (OutFile != NULL) {
    fclose (OutFile);
  }
  
  VerboseMsg ("%s tool done with return code is 0x%x.", UTILITY_NAME, GetUtilityStatus ());

  return GetUtilityStatus ();
}

  
  
  
