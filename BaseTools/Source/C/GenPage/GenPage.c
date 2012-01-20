/** @file

Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  GenPage.c
  
Abstract:
  Pre-Create a 4G page table (2M pages).
  It's used in DUET x64 build needed to enter LongMode.
 
  Create 4G page table (2M pages)
 
                              Linear Address
    63    48 47   39 38           30 29       21 20                          0
   +--------+-------+---------------+-----------+-----------------------------+
               PML4   Directory-Ptr   Directory                 Offset

   Paging-Structures :=
                        PML4
                        (
                          Directory-Ptr Directory {512}
                        ) {4}
**/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "VirtualMemory.h"
#include "EfiUtilityMsgs.h"
#include "ParseInf.h"

#define EFI_PAGE_BASE_OFFSET_IN_LDR 0x70000
#define EFI_PAGE_BASE_ADDRESS       (EFI_PAGE_BASE_OFFSET_IN_LDR + 0x20000)

UINT32 gPageTableBaseAddress  = EFI_PAGE_BASE_ADDRESS;
UINT32 gPageTableOffsetInFile = EFI_PAGE_BASE_OFFSET_IN_LDR;

#define EFI_MAX_ENTRY_NUM     512

#define EFI_PML4_ENTRY_NUM    1
#define EFI_PDPTE_ENTRY_NUM   4
#define EFI_PDE_ENTRY_NUM     EFI_MAX_ENTRY_NUM

#define EFI_PML4_PAGE_NUM     1
#define EFI_PDPTE_PAGE_NUM    EFI_PML4_ENTRY_NUM
#define EFI_PDE_PAGE_NUM      (EFI_PML4_ENTRY_NUM * EFI_PDPTE_ENTRY_NUM)

#define EFI_PAGE_NUMBER       (EFI_PML4_PAGE_NUM + EFI_PDPTE_PAGE_NUM + EFI_PDE_PAGE_NUM)

#define EFI_SIZE_OF_PAGE      0x1000
#define EFI_PAGE_SIZE_2M      0x200000

#define CONVERT_BIN_PAGE_ADDRESS(a)  ((UINT8 *) a - PageTable + gPageTableBaseAddress)

//
// Utility Name
//
#define UTILITY_NAME  "GenPage"

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
  printf ("%s v%d.%d %s -Utility to generate the EfiLoader image containing page table.\n", UTILITY_NAME, UTILITY_MAJOR_VERSION, UTILITY_MINOR_VERSION, __BUILD_VERSION);
  printf ("Copyright (c) 2008 - 2011 Intel Corporation. All rights reserved.\n");
}

VOID
Usage (
  void
  )
{
  Version();
  printf ("\nUsage: \n\
   GenPage\n\
     -o, --output Filename\n\
                        The file that contains both non-page table part and\n\
                        page table\n\
     [-b, --baseaddr baseaddress]\n\
                        The page table location\n\
     [-f, --offset offset]\n\
                        The position that the page table will appear in the\n\
                        output file\n\
     [-v, --verbose]    Turn on verbose output with informational messages\n\
                        printed\n\
     [--version]        Print version and copyright of this program and exit\n\
     [-q, --quiet]      Disable all messages except unrecoverable errors\n\
     [-d, --debug[#]]   Enable debug messages, at level #\n\
     [-h, --help]       Print copyright, version and usage of this program\n\
                        and exit\n\
     EfiLoaderImageName\n");

}

void *
CreateIdentityMappingPageTables (
  void
  )
/*++

Routine Description:
  To create 4G PAE 2M pagetable

Return:
  void * - buffer containing created pagetable

--*/
{
  UINT64                                        PageAddress;
  UINT8                                         *PageTable;
  UINT8                                         *PageTablePtr;
  int                                           PML4Index;
  int                                           PDPTEIndex;
  int                                           PDEIndex;
  X64_PAGE_MAP_AND_DIRECTORY_POINTER_2MB_4K     *PageMapLevel4Entry;
  X64_PAGE_MAP_AND_DIRECTORY_POINTER_2MB_4K     *PageDirectoryPointerEntry;
  X64_PAGE_TABLE_ENTRY_2M                       *PageDirectoryEntry2MB;

  PageTable = (void *)malloc (EFI_PAGE_NUMBER * EFI_SIZE_OF_PAGE);
  memset (PageTable, 0, (EFI_PAGE_NUMBER * EFI_SIZE_OF_PAGE));
  PageTablePtr = PageTable;

  PageAddress = 0;

  //
  //  Page Table structure 3 level 2MB.
  //
  //                   Page-Map-Level-4-Table        : bits 47-39
  //                   Page-Directory-Pointer-Table  : bits 38-30
  //
  //  Page Table 2MB : Page-Directory(2M)            : bits 29-21
  //
  //

  PageMapLevel4Entry = (X64_PAGE_MAP_AND_DIRECTORY_POINTER_2MB_4K *)PageTablePtr;

  for (PML4Index = 0; PML4Index < EFI_PML4_ENTRY_NUM; PML4Index++, PageMapLevel4Entry++) {
    //
    // Each Page-Map-Level-4-Table Entry points to the base address of a Page-Directory-Pointer-Table Entry
    //  
    PageTablePtr += EFI_SIZE_OF_PAGE;
    PageDirectoryPointerEntry = (X64_PAGE_MAP_AND_DIRECTORY_POINTER_2MB_4K *)PageTablePtr;

    //
    // Make a Page-Map-Level-4-Table Entry
    //
    PageMapLevel4Entry->Uint64 = (UINT64)(UINT32)(CONVERT_BIN_PAGE_ADDRESS (PageDirectoryPointerEntry));
    PageMapLevel4Entry->Bits.ReadWrite = 1;
    PageMapLevel4Entry->Bits.Present = 1;

    for (PDPTEIndex = 0; PDPTEIndex < EFI_PDPTE_ENTRY_NUM; PDPTEIndex++, PageDirectoryPointerEntry++) {
      //
      // Each Page-Directory-Pointer-Table Entry points to the base address of a Page-Directory Entry
      //       
      PageTablePtr += EFI_SIZE_OF_PAGE;
      PageDirectoryEntry2MB = (X64_PAGE_TABLE_ENTRY_2M *)PageTablePtr;

      //
      // Make a Page-Directory-Pointer-Table Entry
      //
      PageDirectoryPointerEntry->Uint64 = (UINT64)(UINT32)(CONVERT_BIN_PAGE_ADDRESS (PageDirectoryEntry2MB));
      PageDirectoryPointerEntry->Bits.ReadWrite = 1;
      PageDirectoryPointerEntry->Bits.Present = 1;

      for (PDEIndex = 0; PDEIndex < EFI_PDE_ENTRY_NUM; PDEIndex++, PageDirectoryEntry2MB++) {
        //
        // Make a Page-Directory Entry
        //
        PageDirectoryEntry2MB->Uint64 = (UINT64)PageAddress;
        PageDirectoryEntry2MB->Bits.ReadWrite = 1;
        PageDirectoryEntry2MB->Bits.Present = 1;
        PageDirectoryEntry2MB->Bits.MustBe1 = 1;

        PageAddress += EFI_PAGE_SIZE_2M;
      }
    }
  }

  return PageTable;
}

INT32
GenBinPage (
  void *BaseMemory,
  char *NoPageFileName,
  char *PageFileName
  )
/*++

Routine Description:
  Write the buffer containing page table to file at a specified offset.
  Here the offset is defined as EFI_PAGE_BASE_OFFSET_IN_LDR.

Arguments:
  BaseMemory     - buffer containing page table
  NoPageFileName - file to write page table
  PageFileName   - file save to after writing

return:
  0  : successful
  -1 : failed

--*/
{
  FILE  *PageFile;
  FILE  *NoPageFile;
  UINT8 Data;
  unsigned long FileSize;

  //
  // Open files
  //
  PageFile = fopen (PageFileName, "w+b");
  if (PageFile == NULL) {
    Error (NoPageFileName, 0, 0x4002, "Invalid parameter option", "Output File %s open failure", PageFileName);
    return -1;
  }

  NoPageFile = fopen (NoPageFileName, "r+b");
  if (NoPageFile == NULL) {
    Error (NoPageFileName, 0, 0x4002, "Invalid parameter option", "Input File %s open failure", NoPageFileName);
    fclose (PageFile);
    return -1;
  }

  //
  // Check size - should not be great than EFI_PAGE_BASE_OFFSET_IN_LDR
  //
  fseek (NoPageFile, 0, SEEK_END);
  FileSize = ftell (NoPageFile);
  fseek (NoPageFile, 0, SEEK_SET);
  if (FileSize > gPageTableOffsetInFile) {
    Error (NoPageFileName, 0, 0x4002, "Invalid parameter option", "Input file size (0x%lx) exceeds the Page Table Offset (0x%x)", FileSize, (unsigned) gPageTableOffsetInFile);
    fclose (PageFile);
    fclose (NoPageFile);
    return -1;
  }

  //
  // Write data
  //
  while (fread (&Data, sizeof(UINT8), 1, NoPageFile)) {
    fwrite (&Data, sizeof(UINT8), 1, PageFile);
  }

  //
  // Write PageTable
  //
  fseek (PageFile, gPageTableOffsetInFile, SEEK_SET);
  fwrite (BaseMemory, (EFI_PAGE_NUMBER * EFI_SIZE_OF_PAGE), 1, PageFile);

  //
  // Close files
  //
  fclose (PageFile);
  fclose (NoPageFile);

  return 0;
}

int
main (
  int argc,
  char **argv
  )
{
  VOID        *BaseMemory;
  INTN        result;
  CHAR8       *OutputFile = NULL;
  CHAR8       *InputFile = NULL;
  EFI_STATUS  Status;
  UINT64      TempValue;

  SetUtilityName("GenPage");

  if (argc == 1) {
    Usage();
    return STATUS_ERROR;
  }
  
  argc --;
  argv ++;

  if ((stricmp (argv[0], "-h") == 0) || (stricmp (argv[0], "--help") == 0)) {
    Usage();
    return 0;    
  }

  if (stricmp (argv[0], "--version") == 0) {
    Version();
    return 0;    
  }
  
  while (argc > 0) {
    if ((stricmp (argv[0], "-o") == 0) || (stricmp (argv[0], "--output") == 0)) {
      if (argv[1] == NULL || argv[1][0] == '-') {
        Error (NULL, 0, 1003, "Invalid option value", "Output file is missing for -o option");
        return STATUS_ERROR;
      }
      OutputFile = argv[1];
      argc -= 2;
      argv += 2;
      continue; 
    }
    
    if ((stricmp (argv[0], "-b") == 0) || (stricmp (argv[0], "--baseaddr") == 0)) {
      if (argv[1] == NULL || argv[1][0] == '-') {
        Error (NULL, 0, 1003, "Invalid option value", "Base address is missing for -b option");
        return STATUS_ERROR;
      }
      Status = AsciiStringToUint64 (argv[1], FALSE, &TempValue);
      if (EFI_ERROR (Status)) {
        Error (NULL, 0, 1003, "Invalid option value", "Base address is not valid intergrator");
        return STATUS_ERROR;
      }
      gPageTableBaseAddress = (UINT32) TempValue;
      argc -= 2;
      argv += 2;
      continue; 
    }
    
    if ((stricmp (argv[0], "-f") == 0) || (stricmp (argv[0], "--offset") == 0)) {
      if (argv[1] == NULL || argv[1][0] == '-') {
        Error (NULL, 0, 1003, "Invalid option value", "Offset is missing for -f option");
        return STATUS_ERROR;
      }
      Status = AsciiStringToUint64 (argv[1], FALSE, &TempValue);
      if (EFI_ERROR (Status)) {
        Error (NULL, 0, 1003, "Invalid option value", "Offset is not valid intergrator");
        return STATUS_ERROR;
      }
      gPageTableOffsetInFile = (UINT32) TempValue;
      argc -= 2;
      argv += 2;
      continue; 
    }

    if ((stricmp (argv[0], "-q") == 0) || (stricmp (argv[0], "--quiet") == 0)) {
      argc --;
      argv ++;
      continue; 
    }
    
    if ((stricmp (argv[0], "-v") ==0) || (stricmp (argv[0], "--verbose") == 0)) {
      argc --;
      argv ++;
      continue; 
    }
    
    if ((stricmp (argv[0], "-d") == 0) || (stricmp (argv[0], "--debug") == 0)) {
      if (argv[1] == NULL || argv[1][0] == '-') {
        Error (NULL, 0, 1003, "Invalid option value", "Debug Level is not specified.");
        return STATUS_ERROR;
      }
      Status = AsciiStringToUint64 (argv[1], FALSE, &TempValue);
      if (EFI_ERROR (Status)) {
        Error (NULL, 0, 1003, "Invalid option value", "Debug Level is not valid intergrator.");
        return STATUS_ERROR;
      }
      if (TempValue > 9) {
        Error (NULL, 0, 1003, "Invalid option value", "Debug Level range is 0-9, currnt input level is %d", (int) TempValue);
        return STATUS_ERROR;
      }
      argc -= 2;
      argv += 2;
      continue; 
    }

    if (argv[0][0] == '-') {
      Error (NULL, 0, 1000, "Unknown option", argv[0]);
      return STATUS_ERROR;
    }
    
    //
    // Don't recognize the paramter.
    //
    InputFile = argv[0];
    argc--;
    argv++;
  }
  
  if (InputFile == NULL) {
    Error (NULL, 0, 1003, "Invalid option value", "Input file is not specified");
    return STATUS_ERROR;
  }
  
  //
  // Create X64 page table
  //
  BaseMemory = CreateIdentityMappingPageTables ();

  //
  // Add page table to binary file
  //
  result = GenBinPage (BaseMemory, InputFile, OutputFile);
  if (result < 0) {
    return STATUS_ERROR;
  }

  return 0;
}

