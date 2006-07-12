/*++ 

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  VirtualMemory.h
  
Abstract:

  x64 Long Mode Virtual Memory Management Definitions  

  References:
    1) IA-32 Intel(R) Atchitecture Software Developer's Manual Volume 1:Basic Architecture, Intel
    2) IA-32 Intel(R) Atchitecture Software Developer's Manual Volume 2:Instruction Set Reference, Intel
    3) IA-32 Intel(R) Atchitecture Software Developer's Manual Volume 3:System Programmer's Guide, Intel
    4) AMD64 Architecture Programmer's Manual Volume 2: System Programming
--*/  
#ifndef _VIRTUAL_MEMORY_H_
#define _VIRTUAL_MEMORY_H_


#pragma pack(1)

//
// Page-Map Level-4 Offset (PML4) and
// Page-Directory-Pointer Offset (PDPE) entries 4K & 2MB
//

typedef union {
  struct {
    UINT64  Present:1;                // 0 = Not present in memory, 1 = Present in memory
    UINT64  ReadWrite:1;              // 0 = Read-Only, 1= Read/Write
    UINT64  UserSupervisor:1;         // 0 = Supervisor, 1=User
    UINT64  WriteThrough:1;           // 0 = Write-Back caching, 1=Write-Through caching
    UINT64  CacheDisabled:1;          // 0 = Cached, 1=Non-Cached
    UINT64  Accessed:1;               // 0 = Not accessed, 1 = Accessed (set by CPU)
    UINT64  Reserved:1;               // Reserved
    UINT64  MustBeZero:2;             // Must Be Zero
    UINT64  Available:3;              // Available for use by system software
    UINT64  PageTableBaseAddress:40;  // Page Table Base Address
    UINT64  AvabilableHigh:11;        // Available for use by system software
    UINT64  Nx:1;                     // No Execute bit
  } Bits;
  UINT64    Uint64;
} x64_PAGE_MAP_AND_DIRECTORY_POINTER_2MB_4K;

//
// Page-Directory Offset 4K
//
typedef union {
  struct {
    UINT64  Present:1;                // 0 = Not present in memory, 1 = Present in memory
    UINT64  ReadWrite:1;              // 0 = Read-Only, 1= Read/Write
    UINT64  UserSupervisor:1;         // 0 = Supervisor, 1=User
    UINT64  WriteThrough:1;           // 0 = Write-Back caching, 1=Write-Through caching
    UINT64  CacheDisabled:1;          // 0 = Cached, 1=Non-Cached
    UINT64  Accessed:1;               // 0 = Not accessed, 1 = Accessed (set by CPU)
    UINT64  Reserved:1;               // Reserved
    UINT64  MustBeZero:1;             // Must Be Zero
    UINT64  Reserved2:1;              // Reserved
    UINT64  Available:3;              // Available for use by system software
    UINT64  PageTableBaseAddress:40;  // Page Table Base Address
    UINT64  AvabilableHigh:11;        // Available for use by system software
    UINT64  Nx:1;                     // No Execute bit
  } Bits;
  UINT64    Uint64;
} x64_PAGE_DIRECTORY_ENTRY_4K;

//
// Page Table Entry 4K
//
typedef union {
  struct {
    UINT64  Present:1;                // 0 = Not present in memory, 1 = Present in memory
    UINT64  ReadWrite:1;              // 0 = Read-Only, 1= Read/Write
    UINT64  UserSupervisor:1;         // 0 = Supervisor, 1=User
    UINT64  WriteThrough:1;           // 0 = Write-Back caching, 1=Write-Through caching
    UINT64  CacheDisabled:1;          // 0 = Cached, 1=Non-Cached
    UINT64  Accessed:1;               // 0 = Not accessed, 1 = Accessed (set by CPU)
    UINT64  Dirty:1;                  // 0 = Not Dirty, 1 = written by processor on access to page
    UINT64  PAT:1;                    // 0 = Ignore Page Attribute Table 
    UINT64  Global:1;                 // 0 = Not global page, 1 = global page TLB not cleared on CR3 write
    UINT64  Available:3;              // Available for use by system software
    UINT64  PageTableBaseAddress:40;  // Page Table Base Address
    UINT64  AvabilableHigh:11;        // Available for use by system software
    UINT64  Nx:1;                     // 0 = Execute Code, 1 = No Code Execution
  } Bits;
  UINT64    Uint64;
} x64_PAGE_TABLE_ENTRY_4K;


//
// Page Table Entry 2MB
//
typedef union {
  struct {
    UINT64  Present:1;                // 0 = Not present in memory, 1 = Present in memory
    UINT64  ReadWrite:1;              // 0 = Read-Only, 1= Read/Write
    UINT64  UserSupervisor:1;         // 0 = Supervisor, 1=User
    UINT64  WriteThrough:1;           // 0 = Write-Back caching, 1=Write-Through caching
    UINT64  CacheDisabled:1;          // 0 = Cached, 1=Non-Cached
    UINT64  Accessed:1;               // 0 = Not accessed, 1 = Accessed (set by CPU)
    UINT64  Dirty:1;                  // 0 = Not Dirty, 1 = written by processor on access to page
    UINT64  MustBe1:1;                // Must be 1 
    UINT64  Global:1;                 // 0 = Not global page, 1 = global page TLB not cleared on CR3 write
    UINT64  Available:3;              // Available for use by system software
    UINT64  PAT:1;                    //
    UINT64  MustBeZero:8;             // Must be zero;
    UINT64  PageTableBaseAddress:31;  // Page Table Base Address
    UINT64  AvabilableHigh:11;        // Available for use by system software
    UINT64  Nx:1;                     // 0 = Execute Code, 1 = No Code Execution
  } Bits;
  UINT64    Uint64;
} x64_PAGE_TABLE_ENTRY_2M;

typedef union {
  UINT64  Present:1;                // 0 = Not present in memory, 1 = Present in memory
  UINT64  ReadWrite:1;              // 0 = Read-Only, 1= Read/Write
  UINT64  UserSupervisor:1;         // 0 = Supervisor, 1=User
  UINT64  WriteThrough:1;           // 0 = Write-Back caching, 1=Write-Through caching
  UINT64  CacheDisabled:1;          // 0 = Cached, 1=Non-Cached
  UINT64  Accessed:1;               // 0 = Not accessed, 1 = Accessed (set by CPU)
  UINT64  Dirty:1;                  // 0 = Not Dirty, 1 = written by processor on access to page
  UINT64  Reserved:57;
} x64_PAGE_TABLE_ENTRY_COMMON;

typedef union {
  x64_PAGE_TABLE_ENTRY_4K     Page4k;
  x64_PAGE_TABLE_ENTRY_2M     Page2Mb;
  x64_PAGE_TABLE_ENTRY_COMMON Common;
} x64_PAGE_TABLE_ENTRY;

//
// MTRR Definitions
//
typedef enum {
  Uncached       = 0,
  WriteCombining = 1,
  WriteThrough   = 4,
  WriteProtected = 5,
  WriteBack      = 6
} x64_MTRR_MEMORY_TYPE;

typedef union {
  struct {
    UINT32  VCNT:8;         // The number of Variable Range MTRRs
    UINT32  FIX:1;          // 1=Fixed Range MTRRs supported.  0=Fixed Range MTRRs not supported
    UINT32  Reserved_0;     // Reserved
    UINT32  WC:1;           // Write combining memory type supported
    UINT32  Reserved_1:21;  // Reserved
    UINT32  Reserved_2:32;  // Reserved
  } Bits;
  UINT64  Uint64;
} x64_MTRRCAP_MSR;

typedef union {
  struct {
    UINT32  Type:8;         // Default Memory Type
    UINT32  Reserved_0:2;   // Reserved
    UINT32  FE:1;           // 1=Fixed Range MTRRs enabled.  0=Fixed Range MTRRs disabled
    UINT32  E:1;            // 1=MTRRs enabled, 0=MTRRs disabled
    UINT32  Reserved_1:20;  // Reserved
    UINT32  Reserved_2:32;  // Reserved
  } Bits;
  UINT64  Uint64;
} x64_MTRR_DEF_TYPE_MSR;

typedef union {
  UINT8   Type[8];          // The 8 Memory Type values in the 64-bit MTRR
  UINT64  Uint64;           // The full 64-bit MSR
} x64_MTRR_FIXED_RANGE_MSR;

typedef struct {
  x64_MTRRCAP_MSR           Capabilities;   // MTRR Capabilities MSR value
  x64_MTRR_DEF_TYPE_MSR     DefaultType;    // Default Memory Type MSR Value
  x64_MTRR_FIXED_RANGE_MSR  Fixed[11];      // The 11 Fixed MTRR MSR Values
} x64_MTRR_FIXED_RANGE;


typedef union {
  struct {
    UINT64  Type:8;         // Memory Type
    UINT64  Reserved0:4;    // Reserved
    UINT64  PhysBase:40;    // The physical base address(bits 35..12) of the MTRR
    UINT64  Reserved1:12 ;  // Reserved
  } Bits;
  UINT64  Uint64;
} x64_MTRR_PHYSBASE_MSR;

typedef union {
  struct {
    UINT64  Reserved0:11;  // Reserved
    UINT64  Valid:1;        // 1=MTRR is valid, 0=MTRR is not valid
    UINT64  PhysMask:40;    // The physical address mask (bits 35..12) of the MTRR
    UINT64  Reserved1:12;  // Reserved
  } Bits;
  UINT64  Uint64;
} x64_MTRR_PHYSMASK_MSR;

typedef struct {
  x64_MTRR_PHYSBASE_MSR  PhysBase;  // Variable MTRR Physical Base MSR
  x64_MTRR_PHYSMASK_MSR  PhysMask;  // Variable MTRR Physical Mask MSR
} x64_MTRR_VARIABLE_RANGE;

#pragma pack()

x64_MTRR_MEMORY_TYPE
EfiGetMTRRMemoryType (
  IN  EFI_PHYSICAL_ADDRESS      Address
  )
;

BOOLEAN
CanNotUse2MBPage (
  IN  EFI_PHYSICAL_ADDRESS  BaseAddress
  )
;

VOID
Convert2MBPageTo4KPages (  
  IN  x64_PAGE_TABLE_ENTRY_2M   *PageDirectoryEntry2MB, 
  IN  EFI_PHYSICAL_ADDRESS        PageAddress
  )
;

EFI_PHYSICAL_ADDRESS
CreateIdentityMappingPageTables (
  IN UINT32                NumberOfProcessorPhysicalAddressBits
  )
;

#endif 
