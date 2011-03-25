/** @file
  Main file for Mode shell Debug1 function.

  Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved.<BR>
  This program and the acModeanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which acModeanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

STATIC CONST CHAR16 NameEfiReservedMemoryType[]      = L"Reserved";
STATIC CONST CHAR16 NameEfiLoaderCode[]              = L"LoaderCode";
STATIC CONST CHAR16 NameEfiLoaderData[]              = L"LoaderData";
STATIC CONST CHAR16 NameEfiBootServicesCode[]        = L"BS_Code";
STATIC CONST CHAR16 NameEfiBootServicesData[]        = L"BS_Data";
STATIC CONST CHAR16 NameEfiRuntimeServicesCode[]     = L"RT_Code";
STATIC CONST CHAR16 NameEfiRuntimeServicesData[]     = L"RT_Data";
STATIC CONST CHAR16 NameEfiConventionalMemory[]      = L"Available";
STATIC CONST CHAR16 NameEfiUnusableMemory[]          = L"Unusable";
STATIC CONST CHAR16 NameEfiACPIReclaimMemory[]       = L"ACPIRec";
STATIC CONST CHAR16 NameEfiACPIMemoryNVS[]           = L"ACPI_NVS";
STATIC CONST CHAR16 NameEfiMemoryMappedIO[]          = L"MMIO";
STATIC CONST CHAR16 NameEfiMemoryMappedIOPortSpace[] = L"MMIOPort";
STATIC CONST CHAR16 NameEfiPalCode[]                 = L"PalCode";

#include "UefiShellDebug1CommandsLib.h"

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
  EFI_STATUS          Status;
  LIST_ENTRY          *Package;
  CHAR16              *ProblemParam;
  SHELL_STATUS        ShellStatus;
  UINTN               Size;
  EFI_MEMORY_DESCRIPTOR *Buffer;
  UINTN               MapKey;
  UINTN               ItemSize;
  UINT32              Version;
  UINT8               *Walker;
  UINT64              ReservedPages;
  UINT64              LoadCodePages;
  UINT64              LoadDataPages;
  UINT64              BSCodePages;
  UINT64              BSDataPages;
  UINT64              RTDataPages;
  UINT64              RTCodePages;
  UINT64              AvailPages;
  UINT64              TotalPages;
  UINT64              ReservedPagesSize;
  UINT64              LoadCodePagesSize;
  UINT64              LoadDataPagesSize;
  UINT64              BSCodePagesSize;
  UINT64              BSDataPagesSize;
  UINT64              RTDataPagesSize;
  UINT64              RTCodePagesSize;
  UINT64              AvailPagesSize;
  UINT64              TotalPagesSize;
  UINT64              AcpiReclaimPages;
  UINT64              AcpiNvsPages;
  UINT64              MmioSpacePages;
  UINT64              AcpiReclaimPagesSize;
  UINT64              AcpiNvsPagesSize;
  UINT64              MmioSpacePagesSize;
  BOOLEAN             Sfo;

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
  Size                = 0;
  Buffer              = NULL;
  ShellStatus         = SHELL_SUCCESS;
  Status              = EFI_SUCCESS;

  //
  // initialize the shell lib (we must be in non-auto-init...)
  //
  Status = ShellInitialize();
  ASSERT_EFI_ERROR(Status);

  Status = CommandInit();
  ASSERT_EFI_ERROR(Status);

  //
  // parse the command line
  //
  Status = ShellCommandLineParse (SfoParamList, &Package, &ProblemParam, TRUE);
  if (EFI_ERROR(Status)) {
    if (Status == EFI_VOLUME_CORRUPTED && ProblemParam != NULL) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellDebug1HiiHandle, ProblemParam);
      FreePool(ProblemParam);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      ASSERT(FALSE);
    }
  } else {
    if (ShellCommandLineGetCount(Package) > 1) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellDebug1HiiHandle);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      Status = gBS->GetMemoryMap(&Size, Buffer, &MapKey, &ItemSize, &Version);
      if (Status == EFI_BUFFER_TOO_SMALL){
        Size += SIZE_1KB;
        Buffer = AllocateZeroPool(Size);
        Status = gBS->GetMemoryMap(&Size, Buffer, &MapKey, &ItemSize, &Version);
      }
      if (EFI_ERROR(Status)) {
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_MEMMAP_GET_FAILED), gShellDebug1HiiHandle, Status);
        ShellStatus = SHELL_ACCESS_DENIED;
      } else {
        ASSERT(Version == EFI_MEMORY_DESCRIPTOR_VERSION);
        Sfo = ShellCommandLineGetFlag(Package, L"-sfo");
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_MEMMAP_LIST_HEAD), gShellDebug1HiiHandle);
        for (Walker = (UINT8*)Buffer; Walker < (((UINT8*)Buffer)+Size) && Walker != NULL; Walker += ItemSize){
          switch (((EFI_MEMORY_DESCRIPTOR*)Walker)->Type) {
            // replaced ((EFI_MEMORY_DESCRIPTOR*)Walker)->PhysicalStart+MultU64x64(SIZE_4KB,((EFI_MEMORY_DESCRIPTOR*)Walker)->NumberOfPages) with 0000
            case  EfiReservedMemoryType:
              ShellPrintHiiEx(-1, -1, NULL, (EFI_STRING_ID)(!Sfo?STRING_TOKEN (STR_MEMMAP_LIST_ITEM):STRING_TOKEN (STR_MEMMAP_LIST_ITEM_SFO)), gShellDebug1HiiHandle, NameEfiReservedMemoryType, ((EFI_MEMORY_DESCRIPTOR*)Walker)->PhysicalStart, ((EFI_MEMORY_DESCRIPTOR*)Walker)->PhysicalStart+MultU64x64(SIZE_4KB,((EFI_MEMORY_DESCRIPTOR*)Walker)->NumberOfPages)-1, ((EFI_MEMORY_DESCRIPTOR*)Walker)->NumberOfPages, ((EFI_MEMORY_DESCRIPTOR*)Walker)->Attribute);
              ReservedPages += ((EFI_MEMORY_DESCRIPTOR*)Walker)->NumberOfPages;
              TotalPages += ((EFI_MEMORY_DESCRIPTOR*)Walker)->NumberOfPages;
              break;
            case EfiLoaderCode:
              ShellPrintHiiEx(-1, -1, NULL, (EFI_STRING_ID)(!Sfo?STRING_TOKEN (STR_MEMMAP_LIST_ITEM):STRING_TOKEN (STR_MEMMAP_LIST_ITEM_SFO)), gShellDebug1HiiHandle, NameEfiLoaderCode, ((EFI_MEMORY_DESCRIPTOR*)Walker)->PhysicalStart, ((EFI_MEMORY_DESCRIPTOR*)Walker)->PhysicalStart+MultU64x64(SIZE_4KB,((EFI_MEMORY_DESCRIPTOR*)Walker)->NumberOfPages)-1, ((EFI_MEMORY_DESCRIPTOR*)Walker)->NumberOfPages, ((EFI_MEMORY_DESCRIPTOR*)Walker)->Attribute);
              LoadCodePages += ((EFI_MEMORY_DESCRIPTOR*)Walker)->NumberOfPages;
              TotalPages += ((EFI_MEMORY_DESCRIPTOR*)Walker)->NumberOfPages;
              break;
            case EfiLoaderData:
              ShellPrintHiiEx(-1, -1, NULL, (EFI_STRING_ID)(!Sfo?STRING_TOKEN (STR_MEMMAP_LIST_ITEM):STRING_TOKEN (STR_MEMMAP_LIST_ITEM_SFO)), gShellDebug1HiiHandle, NameEfiLoaderData, ((EFI_MEMORY_DESCRIPTOR*)Walker)->PhysicalStart, ((EFI_MEMORY_DESCRIPTOR*)Walker)->PhysicalStart+MultU64x64(SIZE_4KB,((EFI_MEMORY_DESCRIPTOR*)Walker)->NumberOfPages)-1, ((EFI_MEMORY_DESCRIPTOR*)Walker)->NumberOfPages, ((EFI_MEMORY_DESCRIPTOR*)Walker)->Attribute);
              LoadDataPages += ((EFI_MEMORY_DESCRIPTOR*)Walker)->NumberOfPages;
              TotalPages += ((EFI_MEMORY_DESCRIPTOR*)Walker)->NumberOfPages;
              break;
            case EfiBootServicesCode:
              ShellPrintHiiEx(-1, -1, NULL, (EFI_STRING_ID)(!Sfo?STRING_TOKEN (STR_MEMMAP_LIST_ITEM):STRING_TOKEN (STR_MEMMAP_LIST_ITEM_SFO)), gShellDebug1HiiHandle, NameEfiBootServicesCode, ((EFI_MEMORY_DESCRIPTOR*)Walker)->PhysicalStart, ((EFI_MEMORY_DESCRIPTOR*)Walker)->PhysicalStart+MultU64x64(SIZE_4KB,((EFI_MEMORY_DESCRIPTOR*)Walker)->NumberOfPages)-1, ((EFI_MEMORY_DESCRIPTOR*)Walker)->NumberOfPages, ((EFI_MEMORY_DESCRIPTOR*)Walker)->Attribute);
              BSCodePages += ((EFI_MEMORY_DESCRIPTOR*)Walker)->NumberOfPages;
              TotalPages += ((EFI_MEMORY_DESCRIPTOR*)Walker)->NumberOfPages;
              break;
            case EfiBootServicesData:
              ShellPrintHiiEx(-1, -1, NULL, (EFI_STRING_ID)(!Sfo?STRING_TOKEN (STR_MEMMAP_LIST_ITEM):STRING_TOKEN (STR_MEMMAP_LIST_ITEM_SFO)), gShellDebug1HiiHandle, NameEfiBootServicesData, ((EFI_MEMORY_DESCRIPTOR*)Walker)->PhysicalStart, ((EFI_MEMORY_DESCRIPTOR*)Walker)->PhysicalStart+MultU64x64(SIZE_4KB,((EFI_MEMORY_DESCRIPTOR*)Walker)->NumberOfPages)-1, ((EFI_MEMORY_DESCRIPTOR*)Walker)->NumberOfPages, ((EFI_MEMORY_DESCRIPTOR*)Walker)->Attribute);
              BSDataPages += ((EFI_MEMORY_DESCRIPTOR*)Walker)->NumberOfPages;
              TotalPages += ((EFI_MEMORY_DESCRIPTOR*)Walker)->NumberOfPages;
              break;
            case EfiRuntimeServicesCode:
              ShellPrintHiiEx(-1, -1, NULL, (EFI_STRING_ID)(!Sfo?STRING_TOKEN (STR_MEMMAP_LIST_ITEM):STRING_TOKEN (STR_MEMMAP_LIST_ITEM_SFO)), gShellDebug1HiiHandle, NameEfiRuntimeServicesCode, ((EFI_MEMORY_DESCRIPTOR*)Walker)->PhysicalStart, ((EFI_MEMORY_DESCRIPTOR*)Walker)->PhysicalStart+MultU64x64(SIZE_4KB,((EFI_MEMORY_DESCRIPTOR*)Walker)->NumberOfPages)-1, ((EFI_MEMORY_DESCRIPTOR*)Walker)->NumberOfPages, ((EFI_MEMORY_DESCRIPTOR*)Walker)->Attribute);
              RTCodePages += ((EFI_MEMORY_DESCRIPTOR*)Walker)->NumberOfPages;
              TotalPages += ((EFI_MEMORY_DESCRIPTOR*)Walker)->NumberOfPages;
              break;
            case EfiRuntimeServicesData:
              ShellPrintHiiEx(-1, -1, NULL, (EFI_STRING_ID)(!Sfo?STRING_TOKEN (STR_MEMMAP_LIST_ITEM):STRING_TOKEN (STR_MEMMAP_LIST_ITEM_SFO)), gShellDebug1HiiHandle, NameEfiRuntimeServicesData, ((EFI_MEMORY_DESCRIPTOR*)Walker)->PhysicalStart, ((EFI_MEMORY_DESCRIPTOR*)Walker)->PhysicalStart+MultU64x64(SIZE_4KB,((EFI_MEMORY_DESCRIPTOR*)Walker)->NumberOfPages)-1, ((EFI_MEMORY_DESCRIPTOR*)Walker)->NumberOfPages, ((EFI_MEMORY_DESCRIPTOR*)Walker)->Attribute);
              RTDataPages += ((EFI_MEMORY_DESCRIPTOR*)Walker)->NumberOfPages;
              TotalPages += ((EFI_MEMORY_DESCRIPTOR*)Walker)->NumberOfPages;
              break;
            case EfiConventionalMemory:
              ShellPrintHiiEx(-1, -1, NULL, (EFI_STRING_ID)(!Sfo?STRING_TOKEN (STR_MEMMAP_LIST_ITEM):STRING_TOKEN (STR_MEMMAP_LIST_ITEM_SFO)), gShellDebug1HiiHandle, NameEfiConventionalMemory, ((EFI_MEMORY_DESCRIPTOR*)Walker)->PhysicalStart, ((EFI_MEMORY_DESCRIPTOR*)Walker)->PhysicalStart+MultU64x64(SIZE_4KB,((EFI_MEMORY_DESCRIPTOR*)Walker)->NumberOfPages)-1, ((EFI_MEMORY_DESCRIPTOR*)Walker)->NumberOfPages, ((EFI_MEMORY_DESCRIPTOR*)Walker)->Attribute);
              AvailPages += ((EFI_MEMORY_DESCRIPTOR*)Walker)->NumberOfPages;
              TotalPages += ((EFI_MEMORY_DESCRIPTOR*)Walker)->NumberOfPages;
              break;
            case EfiUnusableMemory:
              ShellPrintHiiEx(-1, -1, NULL, (EFI_STRING_ID)(!Sfo?STRING_TOKEN (STR_MEMMAP_LIST_ITEM):STRING_TOKEN (STR_MEMMAP_LIST_ITEM_SFO)), gShellDebug1HiiHandle, NameEfiUnusableMemory, ((EFI_MEMORY_DESCRIPTOR*)Walker)->PhysicalStart, ((EFI_MEMORY_DESCRIPTOR*)Walker)->PhysicalStart+MultU64x64(SIZE_4KB,((EFI_MEMORY_DESCRIPTOR*)Walker)->NumberOfPages)-1, ((EFI_MEMORY_DESCRIPTOR*)Walker)->NumberOfPages, ((EFI_MEMORY_DESCRIPTOR*)Walker)->Attribute);
              TotalPages += ((EFI_MEMORY_DESCRIPTOR*)Walker)->NumberOfPages;
              break;
            case EfiACPIReclaimMemory:
              ShellPrintHiiEx(-1, -1, NULL, (EFI_STRING_ID)(!Sfo?STRING_TOKEN (STR_MEMMAP_LIST_ITEM):STRING_TOKEN (STR_MEMMAP_LIST_ITEM_SFO)), gShellDebug1HiiHandle, NameEfiACPIReclaimMemory, ((EFI_MEMORY_DESCRIPTOR*)Walker)->PhysicalStart, ((EFI_MEMORY_DESCRIPTOR*)Walker)->PhysicalStart+MultU64x64(SIZE_4KB,((EFI_MEMORY_DESCRIPTOR*)Walker)->NumberOfPages)-1, ((EFI_MEMORY_DESCRIPTOR*)Walker)->NumberOfPages, ((EFI_MEMORY_DESCRIPTOR*)Walker)->Attribute);
              TotalPages += ((EFI_MEMORY_DESCRIPTOR*)Walker)->NumberOfPages;
              AcpiReclaimPages += ((EFI_MEMORY_DESCRIPTOR*)Walker)->NumberOfPages;
              break;
            case EfiACPIMemoryNVS:
              ShellPrintHiiEx(-1, -1, NULL, (EFI_STRING_ID)(!Sfo?STRING_TOKEN (STR_MEMMAP_LIST_ITEM):STRING_TOKEN (STR_MEMMAP_LIST_ITEM_SFO)), gShellDebug1HiiHandle, NameEfiACPIMemoryNVS, ((EFI_MEMORY_DESCRIPTOR*)Walker)->PhysicalStart, ((EFI_MEMORY_DESCRIPTOR*)Walker)->PhysicalStart+MultU64x64(SIZE_4KB,((EFI_MEMORY_DESCRIPTOR*)Walker)->NumberOfPages)-1, ((EFI_MEMORY_DESCRIPTOR*)Walker)->NumberOfPages, ((EFI_MEMORY_DESCRIPTOR*)Walker)->Attribute);
              TotalPages += ((EFI_MEMORY_DESCRIPTOR*)Walker)->NumberOfPages;
              AcpiNvsPages    += ((EFI_MEMORY_DESCRIPTOR*)Walker)->NumberOfPages;
              break;
            case EfiMemoryMappedIO:
              ShellPrintHiiEx(-1, -1, NULL, (EFI_STRING_ID)(!Sfo?STRING_TOKEN (STR_MEMMAP_LIST_ITEM):STRING_TOKEN (STR_MEMMAP_LIST_ITEM_SFO)), gShellDebug1HiiHandle, NameEfiMemoryMappedIO, ((EFI_MEMORY_DESCRIPTOR*)Walker)->PhysicalStart, ((EFI_MEMORY_DESCRIPTOR*)Walker)->PhysicalStart+MultU64x64(SIZE_4KB,((EFI_MEMORY_DESCRIPTOR*)Walker)->NumberOfPages)-1, ((EFI_MEMORY_DESCRIPTOR*)Walker)->NumberOfPages, ((EFI_MEMORY_DESCRIPTOR*)Walker)->Attribute);
              TotalPages += ((EFI_MEMORY_DESCRIPTOR*)Walker)->NumberOfPages;
              MmioSpacePages  += ((EFI_MEMORY_DESCRIPTOR*)Walker)->NumberOfPages;
              break;
            case EfiMemoryMappedIOPortSpace:
              ShellPrintHiiEx(-1, -1, NULL, (EFI_STRING_ID)(!Sfo?STRING_TOKEN (STR_MEMMAP_LIST_ITEM):STRING_TOKEN (STR_MEMMAP_LIST_ITEM_SFO)), gShellDebug1HiiHandle, NameEfiMemoryMappedIOPortSpace, ((EFI_MEMORY_DESCRIPTOR*)Walker)->PhysicalStart, ((EFI_MEMORY_DESCRIPTOR*)Walker)->PhysicalStart+MultU64x64(SIZE_4KB,((EFI_MEMORY_DESCRIPTOR*)Walker)->NumberOfPages)-1, ((EFI_MEMORY_DESCRIPTOR*)Walker)->NumberOfPages, ((EFI_MEMORY_DESCRIPTOR*)Walker)->Attribute);
              TotalPages += ((EFI_MEMORY_DESCRIPTOR*)Walker)->NumberOfPages;
              break;
            case EfiPalCode:
              ShellPrintHiiEx(-1, -1, NULL, (EFI_STRING_ID)(!Sfo?STRING_TOKEN (STR_MEMMAP_LIST_ITEM):STRING_TOKEN (STR_MEMMAP_LIST_ITEM_SFO)), gShellDebug1HiiHandle, NameEfiPalCode, ((EFI_MEMORY_DESCRIPTOR*)Walker)->PhysicalStart, ((EFI_MEMORY_DESCRIPTOR*)Walker)->PhysicalStart+MultU64x64(SIZE_4KB,((EFI_MEMORY_DESCRIPTOR*)Walker)->NumberOfPages)-1, ((EFI_MEMORY_DESCRIPTOR*)Walker)->NumberOfPages, ((EFI_MEMORY_DESCRIPTOR*)Walker)->Attribute);
              TotalPages += ((EFI_MEMORY_DESCRIPTOR*)Walker)->NumberOfPages;
              break;
            default:
              ASSERT(FALSE);
          }
        }
        //
        // print the summary
        //
        ReservedPagesSize		= MultU64x64(SIZE_4KB,ReservedPages);
        LoadCodePagesSize		= MultU64x64(SIZE_4KB,LoadCodePages);
        LoadDataPagesSize		= MultU64x64(SIZE_4KB,LoadDataPages);
        BSCodePagesSize		  = MultU64x64(SIZE_4KB,BSCodePages);
        BSDataPagesSize		  = MultU64x64(SIZE_4KB,BSDataPages);
        RTDataPagesSize		  = MultU64x64(SIZE_4KB,RTDataPages);
        RTCodePagesSize		  = MultU64x64(SIZE_4KB,RTCodePages);
        AvailPagesSize		  = MultU64x64(SIZE_4KB,AvailPages);
        TotalPagesSize		  = MultU64x64(SIZE_4KB,TotalPages);
        AcpiReclaimPagesSize     = MultU64x64(SIZE_4KB,AcpiReclaimPages);
        AcpiNvsPagesSize         = MultU64x64(SIZE_4KB,AcpiNvsPages);
        MmioSpacePagesSize       = MultU64x64(SIZE_4KB,MmioSpacePages);
        if (!Sfo) {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_MEMMAP_LIST_SUMM), gShellDebug1HiiHandle,
            ReservedPages, ReservedPagesSize,
            LoadCodePages, LoadCodePagesSize,
            LoadDataPages, LoadDataPagesSize,
            BSCodePages, BSCodePagesSize,
            BSDataPages, BSDataPagesSize,
            RTCodePages, RTCodePagesSize,
            RTDataPages, RTDataPagesSize,
            AcpiReclaimPages, AcpiReclaimPagesSize,
            AcpiNvsPages, AcpiNvsPagesSize,
            MmioSpacePages, MmioSpacePagesSize,
            AvailPages, AvailPagesSize,
            DivU64x32(MultU64x64(SIZE_4KB,TotalPages), SIZE_1MB), TotalPagesSize
           );
        } else {
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_MEMMAP_LIST_SUMM_SFO), gShellDebug1HiiHandle,
            TotalPagesSize,
            MultU64x64(SIZE_4KB,ReservedPages),
            BSCodePagesSize,
            BSDataPagesSize,
            RTCodePagesSize,
            RTDataPagesSize,
            LoadCodePagesSize,
            LoadDataPagesSize,
            AvailPages, AvailPagesSize
           );
        }
      }
    }
    ShellCommandLineFreeVarList (Package);
  }

  if (Buffer != NULL) {
    FreePool(Buffer);
  }

  return (ShellStatus);
}

