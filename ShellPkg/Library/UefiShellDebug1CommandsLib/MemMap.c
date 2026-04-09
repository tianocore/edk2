/** @file
  Main file for Mode shell Debug1 function.

  (C) Copyright 2016 Hewlett Packard Enterprise Development LP<BR>
  (C) Copyright 2013-2015 Hewlett-Packard Development Company, L.P.<BR>
  Copyright (c) 2010 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

//
// Need full names for Standard-Format Output
//
STATIC CONST CHAR16  *FullNameEfiMemory[] = {
  [EfiReservedMemoryType]      = L"Reserved",
  [EfiLoaderCode]              = L"LoaderCode",
  [EfiLoaderData]              = L"LoaderData",
  [EfiBootServicesCode]        = L"BootServiceCode",
  [EfiBootServicesData]        = L"BootServiceData",
  [EfiRuntimeServicesCode]     = L"RuntimeCode",
  [EfiRuntimeServicesData]     = L"RuntimeData",
  [EfiConventionalMemory]      = L"Available",
  [EfiUnusableMemory]          = L"UnusableMemory",
  [EfiACPIReclaimMemory]       = L"ACPIReclaimMemory",
  [EfiACPIMemoryNVS]           = L"ACPIMemoryNVS",
  [EfiMemoryMappedIO]          = L"MemoryMappedIO",
  [EfiMemoryMappedIOPortSpace] = L"MemoryMappedIOPortSpace",
  [EfiPalCode]                 = L"PalCode",
  [EfiPersistentMemory]        = L"Persistent",
  [EfiUnacceptedMemoryType]    = L"Unaccepted",
};

//
// Need short names for some memory types
//
STATIC CONST CHAR16  *ShortNameEfiMemory[] = {
  [EfiBootServicesCode]        = L"BS_Code",
  [EfiBootServicesData]        = L"BS_Data",
  [EfiRuntimeServicesCode]     = L"RT_Code",
  [EfiRuntimeServicesData]     = L"RT_Data",
  [EfiUnusableMemory]          = L"Unusable",
  [EfiACPIReclaimMemory]       = L"ACPI_Recl",
  [EfiACPIMemoryNVS]           = L"ACPI_NVS",
  [EfiMemoryMappedIO]          = L"MMIO",
  [EfiMemoryMappedIOPortSpace] = L"MMIO_Port",
};

/** Memory Type information.

  Store some information about the actions to operate on different
  Memory Types.
*/
typedef struct {
  /// Whether the MemoryType should contribute to the total number of pages.
  BOOLEAN    ContributeTotalPages;

  /// Whether the MemoryType has a short name.
  BOOLEAN    HasShortName;
} MEMORY_TYPE_INFO;

STATIC CONST MEMORY_TYPE_INFO  MemoryPageArray[EfiMaxMemoryType] = {
  [EfiReservedMemoryType]      = { FALSE, FALSE, },
  [EfiLoaderCode]              = { TRUE,  FALSE, },
  [EfiLoaderData]              = { TRUE,  FALSE, },
  [EfiBootServicesCode]        = { TRUE,  TRUE,  },
  [EfiBootServicesData]        = { TRUE,  TRUE,  },
  [EfiRuntimeServicesCode]     = { TRUE,  TRUE,  },
  [EfiRuntimeServicesData]     = { TRUE,  TRUE,  },
  [EfiConventionalMemory]      = { TRUE,  FALSE, },
  [EfiUnusableMemory]          = { FALSE, TRUE,  },
  [EfiACPIReclaimMemory]       = { TRUE,  TRUE,  },
  [EfiACPIMemoryNVS]           = { TRUE,  TRUE,  },
  [EfiMemoryMappedIO]          = { FALSE, TRUE,  },
  [EfiMemoryMappedIOPortSpace] = { FALSE, TRUE,  },
  [EfiPalCode]                 = { TRUE,  FALSE, },
  [EfiPersistentMemory]        = { TRUE,  FALSE, },
  [EfiUnacceptedMemoryType]    = { TRUE,  FALSE, },
};

/** Memory Type Pages Information.

  Store the number of Pages/PagesSize for each MemoryType.
*/
typedef struct {
  /// Number of pages for this MemoryType.
  UINT64    Pages;

  /// Number of Pages * Page Size for this MemoryType.
  UINT64    PagesSize;
} MEMORY_TYPE_PAGES_INFO;

#include "UefiShellDebug1CommandsLib.h"

typedef struct {
  UINT32        Type;
  UINT64        NumberOfPages;
  LIST_ENTRY    Link;
} MEMORY_LENGTH_ENTRY;

/**
  Add the length of the specified type to List.

  @param List          A list to hold all pairs of <Type, NumberOfPages>.
  @param Type          Memory type.
  @param NumberOfPages Number of pages.
**/
VOID
AddMemoryLength (
  LIST_ENTRY  *List,
  UINT32      Type,
  UINT64      NumberOfPages
  )
{
  MEMORY_LENGTH_ENTRY  *Entry;
  MEMORY_LENGTH_ENTRY  *NewEntry;
  LIST_ENTRY           *Link;

  Entry = NULL;
  for (Link = GetFirstNode (List); !IsNull (List, Link); Link = GetNextNode (List, Link)) {
    Entry = BASE_CR (Link, MEMORY_LENGTH_ENTRY, Link);
    if (Entry->Type >= Type) {
      break;
    }
  }

  if ((Entry != NULL) && (Entry->Type == Type)) {
    //
    // The Entry is the one we look for.
    //
    NewEntry = Entry;
  } else {
    //
    // The search operation breaks due to:
    // 1. Type of every entry < Type --> Insert to tail
    // 2. Type of an entry > Type --> Insert to previous of this entry
    //
    NewEntry = AllocatePool (sizeof (*NewEntry));
    if (NewEntry == NULL) {
      return;
    }

    NewEntry->Type          = Type;
    NewEntry->NumberOfPages = 0;
    InsertTailList (Link, &NewEntry->Link);
  }

  NewEntry->NumberOfPages += NumberOfPages;
}

/** Parse Memory Descriptors

@param[in]  Descriptors   Array of pointers to Memory Descriptors.
@param[in]  Size          Size of the Descriptors array.
@param[in]  ItemSize      Size of a Memory Descriptor.
@param[in]  Sfo           Whether 'Standard Format Output' should be used.
**/
STATIC
SHELL_STATUS
ParseMemoryDescriptors (
  IN  EFI_MEMORY_DESCRIPTOR  *Descriptors,
  IN  UINTN                  Size,
  IN  UINTN                  ItemSize,
  IN  BOOLEAN                Sfo
  )
{
  EFI_MEMORY_DESCRIPTOR   *Walker;
  UINT64                  TotalPages;
  UINT64                  TotalPagesSize;
  MEMORY_TYPE_PAGES_INFO  MemoryPageCount[EfiMaxMemoryType];
  CONST CHAR16            *MemoryTypeName;
  UINTN                   Index;
  LIST_ENTRY              MemoryList;
  LIST_ENTRY              *Link;
  MEMORY_LENGTH_ENTRY     *Entry;

  InitializeListHead (&MemoryList);

  TotalPages = 0;

  // Reset the MemoryPageCount array.
  ZeroMem (MemoryPageCount, sizeof (MemoryPageCount));

  for (Walker = Descriptors
       ; (Walker < (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)Descriptors + Size)) && (Walker != NULL)
       ; Walker = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)Walker + ItemSize)
       )
  {
    if (Walker->Type < EfiMaxMemoryType) {
      MemoryPageCount[Walker->Type].Pages += Walker->NumberOfPages;

      if (MemoryPageArray[Walker->Type].ContributeTotalPages) {
        TotalPages += Walker->NumberOfPages;
      }

      if (!Sfo && MemoryPageArray[Walker->Type].HasShortName) {
        MemoryTypeName = ShortNameEfiMemory[Walker->Type];
      } else {
        MemoryTypeName = FullNameEfiMemory[Walker->Type];
      }

      ShellPrintHiiDefaultEx (
        (EFI_STRING_ID)(!Sfo ? STRING_TOKEN (STR_MEMMAP_LIST_ITEM) : STRING_TOKEN (STR_MEMMAP_LIST_ITEM_SFO)),
        gShellDebug1HiiHandle,
        MemoryTypeName,
        Walker->PhysicalStart,
        Walker->PhysicalStart+MultU64x64 (SIZE_4KB, Walker->NumberOfPages)-1,
        Walker->NumberOfPages,
        Walker->Attribute
        );
    } else {
      //
      // Shell Spec defines the SFO format.
      // Do not print the OEM/OS memory usage in the SFO format, to avoid conflict with Shell Spec.
      //
      if (!Sfo) {
        ShellPrintHiiDefaultEx (
          STRING_TOKEN (STR_MEMMAP_LIST_ITEM_OTHER),
          gShellDebug1HiiHandle,
          Walker->Type,
          Walker->PhysicalStart,
          Walker->PhysicalStart + MultU64x64 (SIZE_4KB, Walker->NumberOfPages) - 1,
          Walker->NumberOfPages,
          Walker->Attribute
          );
      }

      TotalPages += Walker->NumberOfPages;
      AddMemoryLength (&MemoryList, Walker->Type, Walker->NumberOfPages);
    }
  }

  TotalPagesSize = MultU64x64 (SIZE_4KB, TotalPages);
  for (Index = EfiReservedMemoryType; Index < EfiMaxMemoryType; Index++) {
    MemoryPageCount[Index].PagesSize = MultU64x64 (SIZE_4KB, MemoryPageCount[Index].Pages);
  }

  //
  // print the summary
  //
  if (!Sfo) {
    ShellPrintHiiDefaultEx (
      STRING_TOKEN (STR_MEMMAP_LIST_SUMM),
      gShellDebug1HiiHandle,
      MemoryPageCount[EfiReservedMemoryType].Pages,
      MemoryPageCount[EfiReservedMemoryType].PagesSize,
      MemoryPageCount[EfiLoaderCode].Pages,
      MemoryPageCount[EfiLoaderCode].PagesSize,
      MemoryPageCount[EfiLoaderData].Pages,
      MemoryPageCount[EfiLoaderData].PagesSize,
      MemoryPageCount[EfiBootServicesCode].Pages,
      MemoryPageCount[EfiBootServicesCode].PagesSize,
      MemoryPageCount[EfiBootServicesData].Pages,
      MemoryPageCount[EfiBootServicesData].PagesSize,
      MemoryPageCount[EfiRuntimeServicesCode].Pages,
      MemoryPageCount[EfiRuntimeServicesCode].PagesSize,
      MemoryPageCount[EfiRuntimeServicesData].Pages,
      MemoryPageCount[EfiRuntimeServicesData].PagesSize,
      MemoryPageCount[EfiACPIReclaimMemory].Pages,
      MemoryPageCount[EfiACPIReclaimMemory].PagesSize,
      MemoryPageCount[EfiACPIMemoryNVS].Pages,
      MemoryPageCount[EfiACPIMemoryNVS].PagesSize,
      MemoryPageCount[EfiMemoryMappedIO].Pages,
      MemoryPageCount[EfiMemoryMappedIO].PagesSize,
      MemoryPageCount[EfiMemoryMappedIOPortSpace].Pages,
      MemoryPageCount[EfiMemoryMappedIOPortSpace].PagesSize,
      MemoryPageCount[EfiPalCode].Pages,
      MemoryPageCount[EfiPalCode].PagesSize,
      MemoryPageCount[EfiUnacceptedMemoryType].Pages,
      MemoryPageCount[EfiUnacceptedMemoryType].PagesSize,
      MemoryPageCount[EfiConventionalMemory].Pages,
      MemoryPageCount[EfiConventionalMemory].PagesSize,
      MemoryPageCount[EfiPersistentMemory].Pages,
      MemoryPageCount[EfiPersistentMemory].PagesSize
      );

    //
    // Print out the total memory usage for OEM/OS types in the order of type.
    //
    for (Link = GetFirstNode (&MemoryList); !IsNull (&MemoryList, Link); Link = GetNextNode (&MemoryList, Link)) {
      Entry = BASE_CR (Link, MEMORY_LENGTH_ENTRY, Link);
      ShellPrintHiiDefaultEx (
        STRING_TOKEN (STR_MEMMAP_LIST_SUMM_OTHER),
        gShellDebug1HiiHandle,
        Entry->Type,
        Entry->NumberOfPages,
        MultU64x64 (SIZE_4KB, Entry->NumberOfPages)
        );
    }

    ShellPrintHiiDefaultEx (
      STRING_TOKEN (STR_MEMMAP_LIST_SUMM2),
      gShellDebug1HiiHandle,
      DivU64x32 (MultU64x64 (SIZE_4KB, TotalPages), SIZE_1MB),
      TotalPagesSize
      );
  } else {
    ShellPrintHiiDefaultEx (
      STRING_TOKEN (STR_MEMMAP_LIST_SUMM_SFO),
      gShellDebug1HiiHandle,
      TotalPagesSize,
      MemoryPageCount[EfiReservedMemoryType].PagesSize,
      MemoryPageCount[EfiBootServicesCode].PagesSize,
      MemoryPageCount[EfiBootServicesData].PagesSize,
      MemoryPageCount[EfiRuntimeServicesCode].PagesSize,
      MemoryPageCount[EfiRuntimeServicesData].PagesSize,
      MemoryPageCount[EfiLoaderCode].PagesSize,
      MemoryPageCount[EfiLoaderData].PagesSize,
      MemoryPageCount[EfiConventionalMemory].PagesSize,
      MemoryPageCount[EfiMemoryMappedIO].PagesSize,
      MemoryPageCount[EfiMemoryMappedIOPortSpace].PagesSize,
      MemoryPageCount[EfiUnusableMemory].PagesSize,
      MemoryPageCount[EfiACPIReclaimMemory].PagesSize,
      MemoryPageCount[EfiACPIMemoryNVS].PagesSize,
      MemoryPageCount[EfiPalCode].PagesSize,
      MemoryPageCount[EfiUnacceptedMemoryType].PagesSize,
      MemoryPageCount[EfiPersistentMemory].PagesSize
      );
  }

  //
  // Free the memory list.
  //
  for (Link = GetFirstNode (&MemoryList); !IsNull (&MemoryList, Link); ) {
    Link = RemoveEntryList (Link);
  }

  return SHELL_SUCCESS;
}

/**
  Function for 'memmap' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunMemMap (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS             Status;
  LIST_ENTRY             *Package;
  CHAR16                 *ProblemParam;
  SHELL_STATUS           ShellStatus;
  UINTN                  Size;
  EFI_MEMORY_DESCRIPTOR  *Descriptors;
  UINTN                  MapKey;
  UINTN                  ItemSize;
  UINT32                 Version;
  BOOLEAN                Sfo;

  Size        = 0;
  Descriptors = NULL;
  ShellStatus = SHELL_SUCCESS;
  Status      = EFI_SUCCESS;

  //
  // initialize the shell lib (we must be in non-auto-init...)
  //
  Status = ShellInitialize ();
  ASSERT_EFI_ERROR (Status);

  Status = CommandInit ();
  ASSERT_EFI_ERROR (Status);

  //
  // parse the command line
  //
  Status = ShellCommandLineParse (SfoParamList, &Package, &ProblemParam, TRUE);
  if (EFI_ERROR (Status)) {
    if ((Status == EFI_VOLUME_CORRUPTED) && (ProblemParam != NULL)) {
      ShellPrintHiiDefaultEx (STRING_TOKEN (STR_GEN_PROBLEM), gShellDebug1HiiHandle, L"memmap", ProblemParam);
      FreePool (ProblemParam);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      ASSERT (FALSE);
    }
  } else {
    if (ShellCommandLineGetCount (Package) > 1) {
      ShellPrintHiiDefaultEx (STRING_TOKEN (STR_GEN_TOO_MANY), gShellDebug1HiiHandle, L"memmap");
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      Status = gBS->GetMemoryMap (&Size, Descriptors, &MapKey, &ItemSize, &Version);
      if (Status == EFI_BUFFER_TOO_SMALL) {
        Size       += SIZE_1KB;
        Descriptors = AllocateZeroPool (Size);
        if (Descriptors == NULL) {
          ShellPrintHiiDefaultEx (STRING_TOKEN (STR_GEN_OUT_MEM), gShellDebug1HiiHandle, L"memmap");
          ShellCommandLineFreeVarList (Package);
          return SHELL_OUT_OF_RESOURCES;
        }

        Status = gBS->GetMemoryMap (&Size, Descriptors, &MapKey, &ItemSize, &Version);
      }

      if (EFI_ERROR (Status)) {
        ShellPrintHiiDefaultEx (STRING_TOKEN (STR_MEMMAP_GET_FAILED), gShellDebug1HiiHandle, L"memmap");
        ShellStatus = SHELL_ACCESS_DENIED;
      } else {
        ASSERT (Version == EFI_MEMORY_DESCRIPTOR_VERSION);

        Sfo = ShellCommandLineGetFlag (Package, L"-sfo");
        if (!Sfo) {
          ShellPrintHiiDefaultEx (STRING_TOKEN (STR_MEMMAP_LIST_HEAD), gShellDebug1HiiHandle);
        } else {
          ShellPrintHiiDefaultEx (STRING_TOKEN (STR_GEN_SFO_HEADER), gShellDebug1HiiHandle, L"memmap");
        }

        ParseMemoryDescriptors (Descriptors, Size, ItemSize, Sfo);
      }
    }

    ShellCommandLineFreeVarList (Package);
  }

  if (Descriptors != NULL) {
    FreePool (Descriptors);
  }

  return (ShellStatus);
}
