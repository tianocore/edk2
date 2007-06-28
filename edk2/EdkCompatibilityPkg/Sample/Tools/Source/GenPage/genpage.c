/*++ 

Copyright 2006 - 2007, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
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
--*/

#include <stdio.h>
#include <stdlib.h>
#include "VirtualMemory.h"

void
memset (void *, char, long);

unsigned int
xtoi (char  *);

#define EFI_PAGE_BASE_OFFSET_IN_LDR 0x70000
#define EFI_PAGE_BASE_ADDRESS       (EFI_PAGE_BASE_OFFSET_IN_LDR + 0x20000)

unsigned int gPageTableBaseAddress  = EFI_PAGE_BASE_ADDRESS;
unsigned int gPageTableOffsetInFile = EFI_PAGE_BASE_OFFSET_IN_LDR;

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

int
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
    fprintf (stderr, "GenBinPage: Could not open file %s\n", PageFileName);
    return -1;
  }

  NoPageFile = fopen (NoPageFileName, "r+b");
  if (NoPageFile == NULL) {
    fprintf (stderr, "GenBinPage: Could not open file %s\n", NoPageFileName);
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
    fprintf (stderr, "GenBinPage: file size too large - 0x%x\n", FileSize);
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
  void *BaseMemory;
  int  result;

  //
  // Check parameter
  //
  if ((argc != 3) && (argc != 5)) {
    printf ("Usage: GenPage.exe NoPageFile PageFile [<PageTableBaseAddrss> <PageTableOffsetInFile>]\n");
    return 1;
  }

  //
  // Get PageTable parameter, if have
  //
  if (argc == 5) {
    gPageTableBaseAddress  = xtoi (argv[3]);
    gPageTableOffsetInFile = xtoi (argv[4]);
  }

  //
  // Create X64 page table
  //
  BaseMemory = CreateIdentityMappingPageTables ();

  //
  // Add page table to binary file
  //
  result = GenBinPage (BaseMemory, argv[1], argv[2]);
  if (result < 0) {
    return 1;
  }

  return 0;
}

unsigned int
xtoi (
  char  *str
  )
/*++

Routine Description:

  Convert hex string to uint

Arguments:

  Str  -  The string
  
Returns:

--*/
{
  unsigned int u;
  char         c;
  unsigned int m;
  
  if (str == NULL) {
    return 0;
  }
  
  m = (unsigned int) -1 >> 4;
  //
  // skip preceeding white space
  //
  while (*str && *str == ' ') {
    str += 1;
  }
  //
  // skip preceeding zeros
  //
  while (*str && *str == '0') {
    str += 1;
  }
  //
  // skip preceeding white space
  //
  if (*str && (*str == 'x' || *str == 'X')) {
    str += 1;
  }
  //
  // convert hex digits
  //
  u = 0;
  c = *(str++);
  while (c) {
    if (c >= 'a' && c <= 'f') {
      c -= 'a' - 'A';
    }

    if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F')) {
      if (u > m) {
        return (unsigned int) -1;
      }

      u = u << 4 | c - (c >= 'A' ? 'A' - 10 : '0');
    } else {
      break;
    }

    c = *(str++);
  }

  return u;
}

