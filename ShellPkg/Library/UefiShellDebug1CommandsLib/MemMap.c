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
  EFI_MEMORY_DESCRIPTOR  *Walker;
  UINT64                 ReservedPages;
  UINT64                 LoadCodePages;
  UINT64                 LoadDataPages;
  UINT64                 BSCodePages;
  UINT64                 BSDataPages;
  UINT64                 RTDataPages;
  UINT64                 RTCodePages;
  UINT64                 AvailPages;
  UINT64                 TotalPages;
  UINT64                 ReservedPagesSize;
  UINT64                 LoadCodePagesSize;
  UINT64                 LoadDataPagesSize;
  UINT64                 BSCodePagesSize;
  UINT64                 BSDataPagesSize;
  UINT64                 RTDataPagesSize;
  UINT64                 RTCodePagesSize;
  UINT64                 AvailPagesSize;
  UINT64                 TotalPagesSize;
  UINT64                 AcpiReclaimPages;
  UINT64                 AcpiNvsPages;
  UINT64                 MmioSpacePages;
  UINT64                 AcpiReclaimPagesSize;
  UINT64                 AcpiNvsPagesSize;
  UINT64                 MmioSpacePagesSize;
  UINT64                 MmioPortPages;
  UINT64                 MmioPortPagesSize;
  UINT64                 UnusableMemoryPages;
  UINT64                 UnusableMemoryPagesSize;
  UINT64                 PalCodePages;
  UINT64                 PalCodePagesSize;
  UINT64                 UnacceptedPages;
  UINT64                 UnacceptedPagesSize;
  UINT64                 PersistentPages;
  UINT64                 PersistentPagesSize;
  BOOLEAN                Sfo;
  LIST_ENTRY             MemoryList;
  MEMORY_LENGTH_ENTRY    *Entry;
  LIST_ENTRY             *Link;

  AcpiReclaimPages    = 0;
  AcpiNvsPages        = 0;
  MmioSpacePages      = 0;
  TotalPages          = 0;
  ReservedPages       = 0;
  LoadCodePages       = 0;
  LoadDataPages       = 0;
  BSCodePages         = 0;
  BSDataPages         = 0;
  RTDataPages         = 0;
  RTCodePages         = 0;
  AvailPages          = 0;
  MmioPortPages       = 0;
  UnusableMemoryPages = 0;
  PalCodePages        = 0;
  PersistentPages     = 0;
  Size                = 0;
  UnacceptedPages     = 0;
  Descriptors         = NULL;
  ShellStatus         = SHELL_SUCCESS;
  Status              = EFI_SUCCESS;
  InitializeListHead (&MemoryList);

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
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellDebug1HiiHandle, L"memmap", ProblemParam);
      FreePool (ProblemParam);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      ASSERT (FALSE);
    }
  } else {
    if (ShellCommandLineGetCount (Package) > 1) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellDebug1HiiHandle, L"memmap");
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      Status = gBS->GetMemoryMap (&Size, Descriptors, &MapKey, &ItemSize, &Version);
      if (Status == EFI_BUFFER_TOO_SMALL) {
        Size       += SIZE_1KB;
        Descriptors = AllocateZeroPool (Size);
        if (Descriptors == NULL) {
          ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_OUT_MEM), gShellDebug1HiiHandle, L"memmap");
          ShellCommandLineFreeVarList (Package);
          return SHELL_OUT_OF_RESOURCES;
        }

        Status = gBS->GetMemoryMap (&Size, Descriptors, &MapKey, &ItemSize, &Version);
      }

      if (EFI_ERROR (Status)) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_MEMMAP_GET_FAILED), gShellDebug1HiiHandle, L"memmap");
        ShellStatus = SHELL_ACCESS_DENIED;
      } else {
        ASSERT (Version == EFI_MEMORY_DESCRIPTOR_VERSION);

        Sfo = ShellCommandLineGetFlag (Package, L"-sfo");
        if (!Sfo) {
          ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_MEMMAP_LIST_HEAD), gShellDebug1HiiHandle);
        } else {
          ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_SFO_HEADER), gShellDebug1HiiHandle, L"memmap");
        }

        for ( Walker = Descriptors
              ; (Walker < (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)Descriptors + Size)) && (Walker != NULL)
              ; Walker = (EFI_MEMORY_DESCRIPTOR *)((UINT8 *)Walker + ItemSize)
              )
        {
          switch (Walker->Type) {
            case EfiReservedMemoryType:
              ShellPrintHiiEx (-1, -1, NULL, (EFI_STRING_ID)(!Sfo ? STRING_TOKEN (STR_MEMMAP_LIST_ITEM) : STRING_TOKEN (STR_MEMMAP_LIST_ITEM_SFO)), gShellDebug1HiiHandle, FullNameEfiMemory[EfiReservedMemoryType], Walker->PhysicalStart, Walker->PhysicalStart+MultU64x64 (SIZE_4KB, Walker->NumberOfPages)-1, Walker->NumberOfPages, Walker->Attribute);
              ReservedPages += Walker->NumberOfPages;
              break;
            case EfiLoaderCode:
              ShellPrintHiiEx (-1, -1, NULL, (EFI_STRING_ID)(!Sfo ? STRING_TOKEN (STR_MEMMAP_LIST_ITEM) : STRING_TOKEN (STR_MEMMAP_LIST_ITEM_SFO)), gShellDebug1HiiHandle, FullNameEfiMemory[EfiLoaderCode], Walker->PhysicalStart, Walker->PhysicalStart+MultU64x64 (SIZE_4KB, Walker->NumberOfPages)-1, Walker->NumberOfPages, Walker->Attribute);
              LoadCodePages += Walker->NumberOfPages;
              TotalPages    += Walker->NumberOfPages;
              break;
            case EfiLoaderData:
              ShellPrintHiiEx (-1, -1, NULL, (EFI_STRING_ID)(!Sfo ? STRING_TOKEN (STR_MEMMAP_LIST_ITEM) : STRING_TOKEN (STR_MEMMAP_LIST_ITEM_SFO)), gShellDebug1HiiHandle, FullNameEfiMemory[EfiLoaderData], Walker->PhysicalStart, Walker->PhysicalStart+MultU64x64 (SIZE_4KB, Walker->NumberOfPages)-1, Walker->NumberOfPages, Walker->Attribute);
              LoadDataPages += Walker->NumberOfPages;
              TotalPages    += Walker->NumberOfPages;
              break;
            case EfiBootServicesCode:
              ShellPrintHiiEx (-1, -1, NULL, (EFI_STRING_ID)(!Sfo ? STRING_TOKEN (STR_MEMMAP_LIST_ITEM) : STRING_TOKEN (STR_MEMMAP_LIST_ITEM_SFO)), gShellDebug1HiiHandle, !Sfo ? ShortNameEfiMemory[EfiBootServicesCode] : FullNameEfiMemory[EfiBootServicesCode], Walker->PhysicalStart, Walker->PhysicalStart+MultU64x64 (SIZE_4KB, Walker->NumberOfPages)-1, Walker->NumberOfPages, Walker->Attribute);
              BSCodePages += Walker->NumberOfPages;
              TotalPages  += Walker->NumberOfPages;
              break;
            case EfiBootServicesData:
              ShellPrintHiiEx (-1, -1, NULL, (EFI_STRING_ID)(!Sfo ? STRING_TOKEN (STR_MEMMAP_LIST_ITEM) : STRING_TOKEN (STR_MEMMAP_LIST_ITEM_SFO)), gShellDebug1HiiHandle, !Sfo ? ShortNameEfiMemory[EfiBootServicesData] : FullNameEfiMemory[EfiBootServicesData], Walker->PhysicalStart, Walker->PhysicalStart+MultU64x64 (SIZE_4KB, Walker->NumberOfPages)-1, Walker->NumberOfPages, Walker->Attribute);
              BSDataPages += Walker->NumberOfPages;
              TotalPages  += Walker->NumberOfPages;
              break;
            case EfiRuntimeServicesCode:
              ShellPrintHiiEx (-1, -1, NULL, (EFI_STRING_ID)(!Sfo ? STRING_TOKEN (STR_MEMMAP_LIST_ITEM) : STRING_TOKEN (STR_MEMMAP_LIST_ITEM_SFO)), gShellDebug1HiiHandle, !Sfo ? ShortNameEfiMemory[EfiRuntimeServicesCode] : FullNameEfiMemory[EfiRuntimeServicesCode], Walker->PhysicalStart, Walker->PhysicalStart+MultU64x64 (SIZE_4KB, Walker->NumberOfPages)-1, Walker->NumberOfPages, Walker->Attribute);
              RTCodePages += Walker->NumberOfPages;
              TotalPages  += Walker->NumberOfPages;
              break;
            case EfiRuntimeServicesData:
              ShellPrintHiiEx (-1, -1, NULL, (EFI_STRING_ID)(!Sfo ? STRING_TOKEN (STR_MEMMAP_LIST_ITEM) : STRING_TOKEN (STR_MEMMAP_LIST_ITEM_SFO)), gShellDebug1HiiHandle, !Sfo ? ShortNameEfiMemory[EfiRuntimeServicesData] : FullNameEfiMemory[EfiRuntimeServicesData], Walker->PhysicalStart, Walker->PhysicalStart+MultU64x64 (SIZE_4KB, Walker->NumberOfPages)-1, Walker->NumberOfPages, Walker->Attribute);
              RTDataPages += Walker->NumberOfPages;
              TotalPages  += Walker->NumberOfPages;
              break;
            case EfiConventionalMemory:
              ShellPrintHiiEx (-1, -1, NULL, (EFI_STRING_ID)(!Sfo ? STRING_TOKEN (STR_MEMMAP_LIST_ITEM) : STRING_TOKEN (STR_MEMMAP_LIST_ITEM_SFO)), gShellDebug1HiiHandle, FullNameEfiMemory[EfiConventionalMemory], Walker->PhysicalStart, Walker->PhysicalStart+MultU64x64 (SIZE_4KB, Walker->NumberOfPages)-1, Walker->NumberOfPages, Walker->Attribute);
              AvailPages += Walker->NumberOfPages;
              TotalPages += Walker->NumberOfPages;
              break;
            case EfiPersistentMemory:
              ShellPrintHiiEx (-1, -1, NULL, (EFI_STRING_ID)(!Sfo ? STRING_TOKEN (STR_MEMMAP_LIST_ITEM) : STRING_TOKEN (STR_MEMMAP_LIST_ITEM_SFO)), gShellDebug1HiiHandle, FullNameEfiMemory[EfiPersistentMemory], Walker->PhysicalStart, Walker->PhysicalStart+MultU64x64 (SIZE_4KB, Walker->NumberOfPages)-1, Walker->NumberOfPages, Walker->Attribute);
              PersistentPages += Walker->NumberOfPages;
              TotalPages      += Walker->NumberOfPages;
              break;
            case EfiUnusableMemory:
              ShellPrintHiiEx (-1, -1, NULL, (EFI_STRING_ID)(!Sfo ? STRING_TOKEN (STR_MEMMAP_LIST_ITEM) : STRING_TOKEN (STR_MEMMAP_LIST_ITEM_SFO)), gShellDebug1HiiHandle, !Sfo ? ShortNameEfiMemory[EfiUnusableMemory] : FullNameEfiMemory[EfiUnusableMemory], Walker->PhysicalStart, Walker->PhysicalStart+MultU64x64 (SIZE_4KB, Walker->NumberOfPages)-1, Walker->NumberOfPages, Walker->Attribute);
              UnusableMemoryPages += Walker->NumberOfPages;
              break;
            case EfiACPIReclaimMemory:
              ShellPrintHiiEx (-1, -1, NULL, (EFI_STRING_ID)(!Sfo ? STRING_TOKEN (STR_MEMMAP_LIST_ITEM) : STRING_TOKEN (STR_MEMMAP_LIST_ITEM_SFO)), gShellDebug1HiiHandle, !Sfo ? ShortNameEfiMemory[EfiACPIReclaimMemory] : FullNameEfiMemory[EfiACPIReclaimMemory], Walker->PhysicalStart, Walker->PhysicalStart+MultU64x64 (SIZE_4KB, Walker->NumberOfPages)-1, Walker->NumberOfPages, Walker->Attribute);
              TotalPages       += Walker->NumberOfPages;
              AcpiReclaimPages += Walker->NumberOfPages;
              break;
            case EfiACPIMemoryNVS:
              ShellPrintHiiEx (-1, -1, NULL, (EFI_STRING_ID)(!Sfo ? STRING_TOKEN (STR_MEMMAP_LIST_ITEM) : STRING_TOKEN (STR_MEMMAP_LIST_ITEM_SFO)), gShellDebug1HiiHandle, !Sfo ? ShortNameEfiMemory[EfiACPIMemoryNVS] : FullNameEfiMemory[EfiACPIMemoryNVS], Walker->PhysicalStart, Walker->PhysicalStart+MultU64x64 (SIZE_4KB, Walker->NumberOfPages)-1, Walker->NumberOfPages, Walker->Attribute);
              TotalPages   += Walker->NumberOfPages;
              AcpiNvsPages += Walker->NumberOfPages;
              break;
            case EfiMemoryMappedIO:
              ShellPrintHiiEx (-1, -1, NULL, (EFI_STRING_ID)(!Sfo ? STRING_TOKEN (STR_MEMMAP_LIST_ITEM) : STRING_TOKEN (STR_MEMMAP_LIST_ITEM_SFO)), gShellDebug1HiiHandle, !Sfo ? ShortNameEfiMemory[EfiMemoryMappedIO] : FullNameEfiMemory[EfiMemoryMappedIO], Walker->PhysicalStart, Walker->PhysicalStart+MultU64x64 (SIZE_4KB, Walker->NumberOfPages)-1, Walker->NumberOfPages, Walker->Attribute);
              MmioSpacePages += Walker->NumberOfPages;
              break;
            case EfiMemoryMappedIOPortSpace:
              ShellPrintHiiEx (-1, -1, NULL, (EFI_STRING_ID)(!Sfo ? STRING_TOKEN (STR_MEMMAP_LIST_ITEM) : STRING_TOKEN (STR_MEMMAP_LIST_ITEM_SFO)), gShellDebug1HiiHandle, !Sfo ? ShortNameEfiMemory[EfiMemoryMappedIOPortSpace] : FullNameEfiMemory[EfiMemoryMappedIOPortSpace], Walker->PhysicalStart, Walker->PhysicalStart+MultU64x64 (SIZE_4KB, Walker->NumberOfPages)-1, Walker->NumberOfPages, Walker->Attribute);
              MmioPortPages += Walker->NumberOfPages;
              break;
            case EfiPalCode:
              ShellPrintHiiEx (-1, -1, NULL, (EFI_STRING_ID)(!Sfo ? STRING_TOKEN (STR_MEMMAP_LIST_ITEM) : STRING_TOKEN (STR_MEMMAP_LIST_ITEM_SFO)), gShellDebug1HiiHandle, FullNameEfiMemory[EfiPalCode], Walker->PhysicalStart, Walker->PhysicalStart+MultU64x64 (SIZE_4KB, Walker->NumberOfPages)-1, Walker->NumberOfPages, Walker->Attribute);
              TotalPages   += Walker->NumberOfPages;
              PalCodePages += Walker->NumberOfPages;
              break;
            case EfiUnacceptedMemoryType:
              ShellPrintHiiEx (-1, -1, NULL, (EFI_STRING_ID)(!Sfo ? STRING_TOKEN (STR_MEMMAP_LIST_ITEM) : STRING_TOKEN (STR_MEMMAP_LIST_ITEM_SFO)), gShellDebug1HiiHandle, FullNameEfiMemory[EfiUnacceptedMemoryType], Walker->PhysicalStart, Walker->PhysicalStart+MultU64x64 (SIZE_4KB, Walker->NumberOfPages)-1, Walker->NumberOfPages, Walker->Attribute);
              TotalPages      += Walker->NumberOfPages;
              UnacceptedPages += Walker->NumberOfPages;
              break;
            default:
              //
              // Shell Spec defines the SFO format.
              // Do not print the OEM/OS memory usage in the SFO format, to avoid conflict with Shell Spec.
              //
              if (!Sfo) {
                ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_MEMMAP_LIST_ITEM_OTHER), gShellDebug1HiiHandle, Walker->Type, Walker->PhysicalStart, Walker->PhysicalStart + MultU64x64 (SIZE_4KB, Walker->NumberOfPages) - 1, Walker->NumberOfPages, Walker->Attribute);
              }

              TotalPages += Walker->NumberOfPages;
              AddMemoryLength (&MemoryList, Walker->Type, Walker->NumberOfPages);
              break;
          }
        }

        //
        // print the summary
        //
        ReservedPagesSize       = MultU64x64 (SIZE_4KB, ReservedPages);
        LoadCodePagesSize       = MultU64x64 (SIZE_4KB, LoadCodePages);
        LoadDataPagesSize       = MultU64x64 (SIZE_4KB, LoadDataPages);
        BSCodePagesSize         = MultU64x64 (SIZE_4KB, BSCodePages);
        BSDataPagesSize         = MultU64x64 (SIZE_4KB, BSDataPages);
        RTDataPagesSize         = MultU64x64 (SIZE_4KB, RTDataPages);
        RTCodePagesSize         = MultU64x64 (SIZE_4KB, RTCodePages);
        AvailPagesSize          = MultU64x64 (SIZE_4KB, AvailPages);
        TotalPagesSize          = MultU64x64 (SIZE_4KB, TotalPages);
        AcpiReclaimPagesSize    = MultU64x64 (SIZE_4KB, AcpiReclaimPages);
        AcpiNvsPagesSize        = MultU64x64 (SIZE_4KB, AcpiNvsPages);
        MmioSpacePagesSize      = MultU64x64 (SIZE_4KB, MmioSpacePages);
        MmioPortPagesSize       = MultU64x64 (SIZE_4KB, MmioPortPages);
        PalCodePagesSize        = MultU64x64 (SIZE_4KB, PalCodePages);
        UnacceptedPagesSize     = MultU64x64 (SIZE_4KB, UnacceptedPages);
        PersistentPagesSize     = MultU64x64 (SIZE_4KB, PersistentPages);
        UnusableMemoryPagesSize = MultU64x64 (SIZE_4KB, UnusableMemoryPages);
        if (!Sfo) {
          ShellPrintHiiEx (
            -1,
            -1,
            NULL,
            STRING_TOKEN (STR_MEMMAP_LIST_SUMM),
            gShellDebug1HiiHandle,
            ReservedPages,
            ReservedPagesSize,
            LoadCodePages,
            LoadCodePagesSize,
            LoadDataPages,
            LoadDataPagesSize,
            BSCodePages,
            BSCodePagesSize,
            BSDataPages,
            BSDataPagesSize,
            RTCodePages,
            RTCodePagesSize,
            RTDataPages,
            RTDataPagesSize,
            AcpiReclaimPages,
            AcpiReclaimPagesSize,
            AcpiNvsPages,
            AcpiNvsPagesSize,
            MmioSpacePages,
            MmioSpacePagesSize,
            MmioPortPages,
            MmioPortPagesSize,
            PalCodePages,
            PalCodePagesSize,
            UnacceptedPages,
            UnacceptedPagesSize,
            AvailPages,
            AvailPagesSize,
            PersistentPages,
            PersistentPagesSize
            );

          //
          // Print out the total memory usage for OEM/OS types in the order of type.
          //
          for (Link = GetFirstNode (&MemoryList); !IsNull (&MemoryList, Link); Link = GetNextNode (&MemoryList, Link)) {
            Entry = BASE_CR (Link, MEMORY_LENGTH_ENTRY, Link);
            ShellPrintHiiEx (
              -1,
              -1,
              NULL,
              STRING_TOKEN (STR_MEMMAP_LIST_SUMM_OTHER),
              gShellDebug1HiiHandle,
              Entry->Type,
              Entry->NumberOfPages,
              MultU64x64 (SIZE_4KB, Entry->NumberOfPages)
              );
          }

          ShellPrintHiiEx (
            -1,
            -1,
            NULL,
            STRING_TOKEN (STR_MEMMAP_LIST_SUMM2),
            gShellDebug1HiiHandle,
            DivU64x32 (MultU64x64 (SIZE_4KB, TotalPages), SIZE_1MB),
            TotalPagesSize
            );
        } else {
          ShellPrintHiiEx (
            -1,
            -1,
            NULL,
            STRING_TOKEN (STR_MEMMAP_LIST_SUMM_SFO),
            gShellDebug1HiiHandle,
            TotalPagesSize,
            ReservedPagesSize,
            BSCodePagesSize,
            BSDataPagesSize,
            RTCodePagesSize,
            RTDataPagesSize,
            LoadCodePagesSize,
            LoadDataPagesSize,
            AvailPagesSize,
            MmioSpacePagesSize,
            MmioPortPagesSize,
            UnusableMemoryPagesSize,
            AcpiReclaimPagesSize,
            AcpiNvsPagesSize,
            PalCodePagesSize,
            UnacceptedPagesSize,
            PersistentPagesSize
            );
        }
      }
    }

    ShellCommandLineFreeVarList (Package);
  }

  if (Descriptors != NULL) {
    FreePool (Descriptors);
  }

  //
  // Free the memory list.
  //
  for (Link = GetFirstNode (&MemoryList); !IsNull (&MemoryList, Link); ) {
    Link = RemoveEntryList (Link);
  }

  return (ShellStatus);
}
