/** @file

  Copyright (c) 2014 - 2017, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>
#include <PiDxe.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiApplicationEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/DxeServicesLib.h>
#include <Library/PrintLib.h>

#include <Protocol/SmmCommunication.h>
#include <Protocol/SmmAccess2.h>

#include <Guid/MemoryProfile.h>
#include <Guid/PiSmmCommunicationRegionTable.h>

CHAR8 *mActionString[] = {
  "Unknown",
  "gBS->AllocatePages",
  "gBS->FreePages",
  "gBS->AllocatePool",
  "gBS->FreePool",
};

CHAR8 *mSmmActionString[] = {
  "SmmUnknown",
  "gSmst->SmmAllocatePages",
  "gSmst->SmmFreePages",
  "gSmst->SmmAllocatePool",
  "gSmst->SmmFreePool",
};

typedef struct {
  MEMORY_PROFILE_ACTION  Action;
  CHAR8                 *String;
} ACTION_STRING;

ACTION_STRING mExtActionString[] = {
  {MEMORY_PROFILE_ACTION_LIB_ALLOCATE_PAGES,                    "Lib:AllocatePages"},
  {MEMORY_PROFILE_ACTION_LIB_ALLOCATE_RUNTIME_PAGES,            "Lib:AllocateRuntimePages"},
  {MEMORY_PROFILE_ACTION_LIB_ALLOCATE_RESERVED_PAGES,           "Lib:AllocateReservedPages"},
  {MEMORY_PROFILE_ACTION_LIB_FREE_PAGES,                        "Lib:FreePages"},
  {MEMORY_PROFILE_ACTION_LIB_ALLOCATE_ALIGNED_PAGES,            "Lib:AllocateAlignedPages"},
  {MEMORY_PROFILE_ACTION_LIB_ALLOCATE_ALIGNED_RUNTIME_PAGES,    "Lib:AllocateAlignedRuntimePages"},
  {MEMORY_PROFILE_ACTION_LIB_ALLOCATE_ALIGNED_RESERVED_PAGES,   "Lib:AllocateAlignedReservedPages"},
  {MEMORY_PROFILE_ACTION_LIB_FREE_ALIGNED_PAGES,                "Lib:FreeAlignedPages"},
  {MEMORY_PROFILE_ACTION_LIB_ALLOCATE_POOL,                     "Lib:AllocatePool"},
  {MEMORY_PROFILE_ACTION_LIB_ALLOCATE_RUNTIME_POOL,             "Lib:AllocateRuntimePool"},
  {MEMORY_PROFILE_ACTION_LIB_ALLOCATE_RESERVED_POOL,            "Lib:AllocateReservedPool"},
  {MEMORY_PROFILE_ACTION_LIB_FREE_POOL,                         "Lib:FreePool"},
  {MEMORY_PROFILE_ACTION_LIB_ALLOCATE_ZERO_POOL,                "Lib:AllocateZeroPool"},
  {MEMORY_PROFILE_ACTION_LIB_ALLOCATE_RUNTIME_ZERO_POOL,        "Lib:AllocateRuntimeZeroPool"},
  {MEMORY_PROFILE_ACTION_LIB_ALLOCATE_RESERVED_ZERO_POOL,       "Lib:AllocateReservedZeroPool"},
  {MEMORY_PROFILE_ACTION_LIB_ALLOCATE_COPY_POOL,                "Lib:AllocateCopyPool"},
  {MEMORY_PROFILE_ACTION_LIB_ALLOCATE_RUNTIME_COPY_POOL,        "Lib:AllocateRuntimeCopyPool"},
  {MEMORY_PROFILE_ACTION_LIB_ALLOCATE_RESERVED_COPY_POOL,       "Lib:AllocateReservedCopyPool"},
  {MEMORY_PROFILE_ACTION_LIB_REALLOCATE_POOL,                   "Lib:ReallocatePool"},
  {MEMORY_PROFILE_ACTION_LIB_REALLOCATE_RUNTIME_POOL,           "Lib:ReallocateRuntimePool"},
  {MEMORY_PROFILE_ACTION_LIB_REALLOCATE_RESERVED_POOL,          "Lib:ReallocateReservedPool"},
};

CHAR8 mUserDefinedActionString[] = {"UserDefined-0x80000000"};

CHAR8 *mMemoryTypeString[] = {
  "EfiReservedMemoryType",
  "EfiLoaderCode",
  "EfiLoaderData",
  "EfiBootServicesCode",
  "EfiBootServicesData",
  "EfiRuntimeServicesCode",
  "EfiRuntimeServicesData",
  "EfiConventionalMemory",
  "EfiUnusableMemory",
  "EfiACPIReclaimMemory",
  "EfiACPIMemoryNVS",
  "EfiMemoryMappedIO",
  "EfiMemoryMappedIOPortSpace",
  "EfiPalCode",
  "EfiPersistentMemory",
  "EfiOSReserved",
  "EfiOemReserved",
};

CHAR8 *mSubsystemString[] = {
  "Unknown",
  "NATIVE",
  "WINDOWS_GUI",
  "WINDOWS_CUI",
  "Unknown",
  "Unknown",
  "Unknown",
  "POSIX_CUI",
  "Unknown",
  "WINDOWS_CE_GUI",
  "EFI_APPLICATION",
  "EFI_BOOT_SERVICE_DRIVER",
  "EFI_RUNTIME_DRIVER",
  "EFI_ROM",
  "XBOX",
  "Unknown",
};

CHAR8 *mFileTypeString[] = {
  "Unknown",
  "RAW",
  "FREEFORM",
  "SECURITY_CORE",
  "PEI_CORE",
  "DXE_CORE",
  "PEIM",
  "DRIVER",
  "COMBINED_PEIM_DRIVER",
  "APPLICATION",
  "SMM",
  "FIRMWARE_VOLUME_IMAGE",
  "COMBINED_SMM_DXE",
  "SMM_CORE",
};

#define PROFILE_NAME_STRING_LENGTH  64
CHAR8 mNameString[PROFILE_NAME_STRING_LENGTH + 1];

//
// Profile summary information
//
#define MEMORY_PROFILE_ALLOC_SUMMARY_INFO_SIGNATURE SIGNATURE_32 ('M','P','A','S')
#define MEMORY_PROFILE_ALLOC_SUMMARY_INFO_REVISION 0x0001

typedef struct {
  MEMORY_PROFILE_COMMON_HEADER  Header;
  PHYSICAL_ADDRESS              CallerAddress;
  MEMORY_PROFILE_ACTION         Action;
  CHAR8                         *ActionString;
  UINT32                        AllocateCount;
  UINT64                        TotalSize;
} MEMORY_PROFILE_ALLOC_SUMMARY_INFO;

typedef struct {
  UINT32                            Signature;
  MEMORY_PROFILE_ALLOC_SUMMARY_INFO AllocSummaryInfo;
  LIST_ENTRY                        Link;
} MEMORY_PROFILE_ALLOC_SUMMARY_INFO_DATA;

typedef struct {
  UINT32                        Signature;
  MEMORY_PROFILE_DRIVER_INFO    *DriverInfo;
  LIST_ENTRY                    *AllocSummaryInfoList;
  LIST_ENTRY                    Link;
} MEMORY_PROFILE_DRIVER_SUMMARY_INFO_DATA;

typedef struct {
  UINT32                        Signature;
  MEMORY_PROFILE_CONTEXT        *Context;
  LIST_ENTRY                    *DriverSummaryInfoList;
} MEMORY_PROFILE_CONTEXT_SUMMARY_DATA;

LIST_ENTRY  mImageSummaryQueue = INITIALIZE_LIST_HEAD_VARIABLE (mImageSummaryQueue);
MEMORY_PROFILE_CONTEXT_SUMMARY_DATA mMemoryProfileContextSummary;

/**
  Get the file name portion of the Pdb File Name.

  The portion of the Pdb File Name between the last backslash and
  either a following period or the end of the string is copied into
  AsciiBuffer.  The name is truncated, if necessary, to ensure that
  AsciiBuffer is not overrun.

  @param[in]  PdbFileName     Pdb file name.
  @param[out] AsciiBuffer     The resultant Ascii File Name.

**/
VOID
GetShortPdbFileName (
  IN  CHAR8     *PdbFileName,
  OUT CHAR8     *AsciiBuffer
  )
{
  UINTN IndexPdb;     // Current work location within a Pdb string.
  UINTN IndexBuffer;  // Current work location within a Buffer string.
  UINTN StartIndex;
  UINTN EndIndex;

  ZeroMem (AsciiBuffer, PROFILE_NAME_STRING_LENGTH + 1);

  if (PdbFileName == NULL) {
    AsciiStrnCpyS (AsciiBuffer, PROFILE_NAME_STRING_LENGTH + 1, " ", 1);
  } else {
    StartIndex = 0;
    for (EndIndex = 0; PdbFileName[EndIndex] != 0; EndIndex++);
    for (IndexPdb = 0; PdbFileName[IndexPdb] != 0; IndexPdb++) {
      if ((PdbFileName[IndexPdb] == '\\') || (PdbFileName[IndexPdb] == '/')) {
        StartIndex = IndexPdb + 1;
      }

      if (PdbFileName[IndexPdb] == '.') {
        EndIndex = IndexPdb;
      }
    }

    IndexBuffer = 0;
    for (IndexPdb = StartIndex; IndexPdb < EndIndex; IndexPdb++) {
      AsciiBuffer[IndexBuffer] = PdbFileName[IndexPdb];
      IndexBuffer++;
      if (IndexBuffer >= PROFILE_NAME_STRING_LENGTH) {
        AsciiBuffer[PROFILE_NAME_STRING_LENGTH] = 0;
        break;
      }
    }
  }
}

/**
  Get a human readable name for an image.
  The following methods will be tried orderly:
    1. Image PDB
    2. FFS UI section
    3. Image GUID

  @param[in] DriverInfo Pointer to memory profile driver info.

  @return The resulting Ascii name string is stored in the mNameString global array.

**/
CHAR8 *
GetDriverNameString (
 IN MEMORY_PROFILE_DRIVER_INFO  *DriverInfo
 )
{
  EFI_STATUS                  Status;
  CHAR16                      *NameString;
  UINTN                       StringSize;

  //
  // Method 1: Get the name string from image PDB
  //
  if (DriverInfo->PdbStringOffset != 0) {
    GetShortPdbFileName ((CHAR8 *) ((UINTN) DriverInfo + DriverInfo->PdbStringOffset), mNameString);
    return mNameString;
  }

  if (!IsZeroGuid (&DriverInfo->FileName)) {
    //
    // Try to get the image's FFS UI section by image GUID
    //
    NameString = NULL;
    StringSize = 0;
    Status = GetSectionFromAnyFv (
              &DriverInfo->FileName,
              EFI_SECTION_USER_INTERFACE,
              0,
              (VOID **) &NameString,
              &StringSize
              );
    if (!EFI_ERROR (Status)) {
      //
      // Method 2: Get the name string from FFS UI section
      //
      if (StrLen (NameString) > PROFILE_NAME_STRING_LENGTH) {
        NameString[PROFILE_NAME_STRING_LENGTH] = 0;
      }
      UnicodeStrToAsciiStrS (NameString, mNameString, sizeof (mNameString));
      FreePool (NameString);
      return mNameString;
    }
  }

  //
  // Method 3: Get the name string from image GUID
  //
  AsciiSPrint (mNameString, sizeof (mNameString), "%g", &DriverInfo->FileName);
  return mNameString;
}

/**
  Memory type to string.

  @param[in] MemoryType Memory type.

  @return Pointer to string.

**/
CHAR8 *
ProfileMemoryTypeToStr (
  IN EFI_MEMORY_TYPE    MemoryType
  )
{
  UINTN     Index;

  if ((UINT32) MemoryType >= 0x80000000) {
    //
    // OS reserved memory type.
    //
    Index = EfiMaxMemoryType;
  } else if ((UINT32) MemoryType >= 0x70000000) {
    //
    // OEM reserved memory type.
    //
    Index = EfiMaxMemoryType + 1;
  } else {
    Index = MemoryType;
  }

  return mMemoryTypeString[Index];
}

/**
  Action to string.

  @param[in] Action                     Profile action.
  @param[in] UserDefinedActionString    Pointer to user defined action string.
  @param[in] IsForSmm                   TRUE  - SMRAM profile.
                                        FALSE - UEFI memory profile.

  @return Pointer to string.

**/
CHAR8 *
ProfileActionToStr (
  IN MEMORY_PROFILE_ACTION  Action,
  IN CHAR8                  *UserDefinedActionString,
  IN BOOLEAN                IsForSmm
  )
{
  UINTN     Index;
  UINTN     ActionStringCount;
  CHAR8     **ActionString;

  if (IsForSmm) {
    ActionString = mSmmActionString;
    ActionStringCount = ARRAY_SIZE (mSmmActionString);
  } else {
    ActionString = mActionString;
    ActionStringCount = ARRAY_SIZE (mActionString);
  }

  if ((UINTN) (UINT32) Action < ActionStringCount) {
    return ActionString[Action];
  }
  for (Index = 0; Index < ARRAY_SIZE (mExtActionString); Index++) {
    if (mExtActionString[Index].Action == Action) {
      return mExtActionString[Index].String;
    }
  }
  if ((Action & MEMORY_PROFILE_ACTION_USER_DEFINED_MASK) != 0) {
    if (UserDefinedActionString != NULL) {
      return UserDefinedActionString;
    }
    AsciiSPrint (mUserDefinedActionString, sizeof (mUserDefinedActionString), "UserDefined-0x%08x", Action);
    return mUserDefinedActionString;
  }

  return ActionString[0];
}

/**
  Dump memory profile allocate information.

  @param[in] DriverInfo         Pointer to memory profile driver info.
  @param[in] AllocIndex         Memory profile alloc info index.
  @param[in] AllocInfo          Pointer to memory profile alloc info.
  @param[in] IsForSmm           TRUE  - SMRAM profile.
                                FALSE - UEFI memory profile.

  @return Pointer to next memory profile alloc info.

**/
MEMORY_PROFILE_ALLOC_INFO *
DumpMemoryProfileAllocInfo (
  IN MEMORY_PROFILE_DRIVER_INFO *DriverInfo,
  IN UINTN                      AllocIndex,
  IN MEMORY_PROFILE_ALLOC_INFO  *AllocInfo,
  IN BOOLEAN                    IsForSmm
  )
{
  CHAR8     *ActionString;

  if (AllocInfo->Header.Signature != MEMORY_PROFILE_ALLOC_INFO_SIGNATURE) {
    return NULL;
  }

  if (AllocInfo->ActionStringOffset != 0) {
    ActionString = (CHAR8 *) ((UINTN) AllocInfo + AllocInfo->ActionStringOffset);
  } else {
    ActionString = NULL;
  }

  Print (L"    MEMORY_PROFILE_ALLOC_INFO (0x%x)\n", AllocIndex);
  Print (L"      Signature     - 0x%08x\n", AllocInfo->Header.Signature);
  Print (L"      Length        - 0x%04x\n", AllocInfo->Header.Length);
  Print (L"      Revision      - 0x%04x\n", AllocInfo->Header.Revision);
  Print (L"      CallerAddress - 0x%016lx (Offset: 0x%08x)\n", AllocInfo->CallerAddress, (UINTN) (AllocInfo->CallerAddress - DriverInfo->ImageBase));
  Print (L"      SequenceId    - 0x%08x\n", AllocInfo->SequenceId);
  Print (L"      Action        - 0x%08x (%a)\n", AllocInfo->Action, ProfileActionToStr (AllocInfo->Action, ActionString, IsForSmm));
  Print (L"      MemoryType    - 0x%08x (%a)\n", AllocInfo->MemoryType, ProfileMemoryTypeToStr (AllocInfo->MemoryType));
  Print (L"      Buffer        - 0x%016lx\n", AllocInfo->Buffer);
  Print (L"      Size          - 0x%016lx\n", AllocInfo->Size);

  return (MEMORY_PROFILE_ALLOC_INFO *) ((UINTN) AllocInfo + AllocInfo->Header.Length);
}

/**
  Dump memory profile driver information.

  @param[in] DriverIndex        Memory profile driver info index.
  @param[in] DriverInfo         Pointer to memory profile driver info.
  @param[in] IsForSmm           TRUE  - SMRAM profile.
                                FALSE - UEFI memory profile.

  @return Pointer to next memory profile driver info.

**/
MEMORY_PROFILE_DRIVER_INFO *
DumpMemoryProfileDriverInfo (
  IN UINTN                      DriverIndex,
  IN MEMORY_PROFILE_DRIVER_INFO *DriverInfo,
  IN BOOLEAN                    IsForSmm
  )
{
  UINTN                         TypeIndex;
  MEMORY_PROFILE_ALLOC_INFO     *AllocInfo;
  UINTN                         AllocIndex;
  CHAR8                         *NameString;

  if (DriverInfo->Header.Signature != MEMORY_PROFILE_DRIVER_INFO_SIGNATURE) {
    return NULL;
  }
  Print (L"  MEMORY_PROFILE_DRIVER_INFO (0x%x)\n", DriverIndex);
  Print (L"    Signature               - 0x%08x\n", DriverInfo->Header.Signature);
  Print (L"    Length                  - 0x%04x\n", DriverInfo->Header.Length);
  Print (L"    Revision                - 0x%04x\n", DriverInfo->Header.Revision);
  NameString = GetDriverNameString (DriverInfo);
  Print (L"    FileName                - %a\n", NameString);
  if (DriverInfo->PdbStringOffset != 0) {
    Print (L"    Pdb                     - %a\n", (CHAR8 *) ((UINTN) DriverInfo + DriverInfo->PdbStringOffset));
  }
  Print (L"    ImageBase               - 0x%016lx\n", DriverInfo->ImageBase);
  Print (L"    ImageSize               - 0x%016lx\n", DriverInfo->ImageSize);
  Print (L"    EntryPoint              - 0x%016lx\n", DriverInfo->EntryPoint);
  Print (L"    ImageSubsystem          - 0x%04x (%a)\n", DriverInfo->ImageSubsystem, mSubsystemString[(DriverInfo->ImageSubsystem < sizeof(mSubsystemString)/sizeof(mSubsystemString[0])) ? DriverInfo->ImageSubsystem : 0]);
  Print (L"    FileType                - 0x%02x (%a)\n", DriverInfo->FileType, mFileTypeString[(DriverInfo->FileType < sizeof(mFileTypeString)/sizeof(mFileTypeString[0])) ? DriverInfo->FileType : 0]);
  Print (L"    CurrentUsage            - 0x%016lx\n", DriverInfo->CurrentUsage);
  Print (L"    PeakUsage               - 0x%016lx\n", DriverInfo->PeakUsage);
  for (TypeIndex = 0; TypeIndex < sizeof (DriverInfo->CurrentUsageByType) / sizeof (DriverInfo->CurrentUsageByType[0]); TypeIndex++) {
    if ((DriverInfo->CurrentUsageByType[TypeIndex] != 0) ||
        (DriverInfo->PeakUsageByType[TypeIndex] != 0)) {
      Print (L"    CurrentUsage[0x%02x]      - 0x%016lx (%a)\n", TypeIndex, DriverInfo->CurrentUsageByType[TypeIndex], mMemoryTypeString[TypeIndex]);
      Print (L"    PeakUsage[0x%02x]         - 0x%016lx (%a)\n", TypeIndex, DriverInfo->PeakUsageByType[TypeIndex], mMemoryTypeString[TypeIndex]);
    }
  }
  Print (L"    AllocRecordCount        - 0x%08x\n", DriverInfo->AllocRecordCount);

  AllocInfo = (MEMORY_PROFILE_ALLOC_INFO *) ((UINTN) DriverInfo + DriverInfo->Header.Length);
  for (AllocIndex = 0; AllocIndex < DriverInfo->AllocRecordCount; AllocIndex++) {
    AllocInfo = DumpMemoryProfileAllocInfo (DriverInfo, AllocIndex, AllocInfo, IsForSmm);
    if (AllocInfo == NULL) {
      return NULL;
    }
  }
  return (MEMORY_PROFILE_DRIVER_INFO *) AllocInfo;
}

/**
  Dump memory profile context information.

  @param[in] Context            Pointer to memory profile context.
  @param[in] IsForSmm           TRUE  - SMRAM profile.
                                FALSE - UEFI memory profile.

  @return Pointer to the end of memory profile context buffer.

**/
VOID *
DumpMemoryProfileContext (
  IN MEMORY_PROFILE_CONTEXT     *Context,
  IN BOOLEAN                    IsForSmm
  )
{
  UINTN                         TypeIndex;
  MEMORY_PROFILE_DRIVER_INFO    *DriverInfo;
  UINTN                         DriverIndex;

  if (Context->Header.Signature != MEMORY_PROFILE_CONTEXT_SIGNATURE) {
    return NULL;
  }
  Print (L"MEMORY_PROFILE_CONTEXT\n");
  Print (L"  Signature                     - 0x%08x\n", Context->Header.Signature);
  Print (L"  Length                        - 0x%04x\n", Context->Header.Length);
  Print (L"  Revision                      - 0x%04x\n", Context->Header.Revision);
  Print (L"  CurrentTotalUsage             - 0x%016lx\n", Context->CurrentTotalUsage);
  Print (L"  PeakTotalUsage                - 0x%016lx\n", Context->PeakTotalUsage);
  for (TypeIndex = 0; TypeIndex < sizeof (Context->CurrentTotalUsageByType) / sizeof (Context->CurrentTotalUsageByType[0]); TypeIndex++) {
    if ((Context->CurrentTotalUsageByType[TypeIndex] != 0) ||
        (Context->PeakTotalUsageByType[TypeIndex] != 0)) {
      Print (L"  CurrentTotalUsage[0x%02x]       - 0x%016lx (%a)\n", TypeIndex, Context->CurrentTotalUsageByType[TypeIndex], mMemoryTypeString[TypeIndex]);
      Print (L"  PeakTotalUsage[0x%02x]          - 0x%016lx (%a)\n", TypeIndex, Context->PeakTotalUsageByType[TypeIndex], mMemoryTypeString[TypeIndex]);
    }
  }
  Print (L"  TotalImageSize                - 0x%016lx\n", Context->TotalImageSize);
  Print (L"  ImageCount                    - 0x%08x\n", Context->ImageCount);
  Print (L"  SequenceCount                 - 0x%08x\n", Context->SequenceCount);

  DriverInfo = (MEMORY_PROFILE_DRIVER_INFO *) ((UINTN) Context + Context->Header.Length);
  for (DriverIndex = 0; DriverIndex < Context->ImageCount; DriverIndex++) {
    DriverInfo = DumpMemoryProfileDriverInfo (DriverIndex, DriverInfo, IsForSmm);
    if (DriverInfo == NULL) {
      return NULL;
    }
  }
  return (VOID *) DriverInfo;
}

/**
  Dump memory profile descriptor information.

  @param[in] DescriptorIndex    Memory profile descriptor index.
  @param[in] Descriptor         Pointer to memory profile descriptor.

  @return Pointer to next memory profile descriptor.

**/
MEMORY_PROFILE_DESCRIPTOR *
DumpMemoryProfileDescriptor (
  IN UINTN                      DescriptorIndex,
  IN MEMORY_PROFILE_DESCRIPTOR  *Descriptor
  )
{
  if (Descriptor->Header.Signature != MEMORY_PROFILE_DESCRIPTOR_SIGNATURE) {
    return NULL;
  }
  Print (L"  MEMORY_PROFILE_DESCRIPTOR (0x%x)\n", DescriptorIndex);
  Print (L"    Signature               - 0x%08x\n", Descriptor->Header.Signature);
  Print (L"    Length                  - 0x%04x\n", Descriptor->Header.Length);
  Print (L"    Revision                - 0x%04x\n", Descriptor->Header.Revision);
  Print (L"    Address                 - 0x%016lx\n", Descriptor->Address);
  Print (L"    Size                    - 0x%016lx\n", Descriptor->Size);

  return (MEMORY_PROFILE_DESCRIPTOR *) ((UINTN) Descriptor + Descriptor->Header.Length);
}

/**
  Dump memory profile free memory information.

  @param[in] FreeMemory         Pointer to memory profile free memory.

  @return Pointer to the end of memory profile free memory buffer.

**/
VOID *
DumpMemoryProfileFreeMemory (
  IN MEMORY_PROFILE_FREE_MEMORY *FreeMemory
  )
{
  MEMORY_PROFILE_DESCRIPTOR     *Descriptor;
  UINTN                         DescriptorIndex;

  if (FreeMemory->Header.Signature != MEMORY_PROFILE_FREE_MEMORY_SIGNATURE) {
    return NULL;
  }
  Print (L"MEMORY_PROFILE_FREE_MEMORY\n");
  Print (L"  Signature                     - 0x%08x\n", FreeMemory->Header.Signature);
  Print (L"  Length                        - 0x%04x\n", FreeMemory->Header.Length);
  Print (L"  Revision                      - 0x%04x\n", FreeMemory->Header.Revision);
  Print (L"  TotalFreeMemoryPages          - 0x%016lx\n", FreeMemory->TotalFreeMemoryPages);
  Print (L"  FreeMemoryEntryCount          - 0x%08x\n", FreeMemory->FreeMemoryEntryCount);

  Descriptor = (MEMORY_PROFILE_DESCRIPTOR *) ((UINTN) FreeMemory + FreeMemory->Header.Length);
  for (DescriptorIndex = 0; DescriptorIndex < FreeMemory->FreeMemoryEntryCount; DescriptorIndex++) {
    Descriptor = DumpMemoryProfileDescriptor (DescriptorIndex, Descriptor);
    if (Descriptor == NULL) {
      return NULL;
    }
  }

  return (VOID *) Descriptor;
}

/**
  Dump memory profile memory range information.

  @param[in] MemoryRange        Pointer to memory profile memory range.

  @return Pointer to the end of memory profile memory range buffer.

**/
VOID *
DumpMemoryProfileMemoryRange (
  IN MEMORY_PROFILE_MEMORY_RANGE    *MemoryRange
  )
{
  MEMORY_PROFILE_DESCRIPTOR     *Descriptor;
  UINTN                         DescriptorIndex;

  if (MemoryRange->Header.Signature != MEMORY_PROFILE_MEMORY_RANGE_SIGNATURE) {
    return NULL;
  }
  Print (L"MEMORY_PROFILE_MEMORY_RANGE\n");
  Print (L"  Signature                     - 0x%08x\n", MemoryRange->Header.Signature);
  Print (L"  Length                        - 0x%04x\n", MemoryRange->Header.Length);
  Print (L"  Revision                      - 0x%04x\n", MemoryRange->Header.Revision);
  Print (L"  MemoryRangeCount              - 0x%08x\n", MemoryRange->MemoryRangeCount);

  Descriptor = (MEMORY_PROFILE_DESCRIPTOR *) ((UINTN) MemoryRange + MemoryRange->Header.Length);
  for (DescriptorIndex = 0; DescriptorIndex < MemoryRange->MemoryRangeCount; DescriptorIndex++) {
    Descriptor = DumpMemoryProfileDescriptor (DescriptorIndex, Descriptor);
    if (Descriptor == NULL) {
      return NULL;
    }
  }

  return (VOID *) Descriptor;
}

/**
  Scan memory profile by Signature.

  @param[in] ProfileBuffer      Memory profile base address.
  @param[in] ProfileSize        Memory profile size.
  @param[in] Signature          Signature.

  @return Pointer to the stucture with the signature.

**/
VOID *
ScanMemoryProfileBySignature (
  IN PHYSICAL_ADDRESS           ProfileBuffer,
  IN UINT64                     ProfileSize,
  IN UINT32                     Signature
  )
{
  MEMORY_PROFILE_COMMON_HEADER  *CommonHeader;
  UINTN                          ProfileEnd;

  ProfileEnd = (UINTN) (ProfileBuffer + ProfileSize);
  CommonHeader = (MEMORY_PROFILE_COMMON_HEADER *) (UINTN) ProfileBuffer;
  while ((UINTN) CommonHeader < ProfileEnd) {
    if (CommonHeader->Signature == Signature) {
      //
      // Found it.
      //
      return (VOID *) CommonHeader;
    }
    if (CommonHeader->Length == 0) {
      ASSERT (FALSE);
      return NULL;
    }
    CommonHeader = (MEMORY_PROFILE_COMMON_HEADER *) ((UINTN) CommonHeader + CommonHeader->Length);
  }

  return NULL;
}

/**
  Dump memory profile information.

  @param[in] ProfileBuffer      Memory profile base address.
  @param[in] ProfileSize        Memory profile size.
  @param[in] IsForSmm           TRUE  - SMRAM profile.
                                FALSE - UEFI memory profile.

**/
VOID
DumpMemoryProfile (
  IN PHYSICAL_ADDRESS           ProfileBuffer,
  IN UINT64                     ProfileSize,
  IN BOOLEAN                    IsForSmm
  )
{
  MEMORY_PROFILE_CONTEXT        *Context;
  MEMORY_PROFILE_FREE_MEMORY    *FreeMemory;
  MEMORY_PROFILE_MEMORY_RANGE   *MemoryRange;

  Context = (MEMORY_PROFILE_CONTEXT *) ScanMemoryProfileBySignature (ProfileBuffer, ProfileSize, MEMORY_PROFILE_CONTEXT_SIGNATURE);
  if (Context != NULL) {
    DumpMemoryProfileContext (Context, IsForSmm);
  }

  FreeMemory = (MEMORY_PROFILE_FREE_MEMORY *) ScanMemoryProfileBySignature (ProfileBuffer, ProfileSize, MEMORY_PROFILE_FREE_MEMORY_SIGNATURE);
  if (FreeMemory != NULL) {
    DumpMemoryProfileFreeMemory (FreeMemory);
  }

  MemoryRange = (MEMORY_PROFILE_MEMORY_RANGE *) ScanMemoryProfileBySignature (ProfileBuffer, ProfileSize, MEMORY_PROFILE_MEMORY_RANGE_SIGNATURE);
  if (MemoryRange != NULL) {
    DumpMemoryProfileMemoryRange (MemoryRange);
  }
}

/**
  Get Allocate summary information structure by caller address.

  @param[in] CallerAddress          Caller address.
  @param[in] DriverSummaryInfoData  Driver summary information data structure.

  @return Allocate summary information structure by caller address.

**/
MEMORY_PROFILE_ALLOC_SUMMARY_INFO_DATA *
GetAllocSummaryInfoByCallerAddress (
  IN PHYSICAL_ADDRESS                           CallerAddress,
  IN MEMORY_PROFILE_DRIVER_SUMMARY_INFO_DATA    *DriverSummaryInfoData
  )
{
  LIST_ENTRY                                    *AllocSummaryInfoList;
  LIST_ENTRY                                    *AllocSummaryLink;
  MEMORY_PROFILE_ALLOC_SUMMARY_INFO             *AllocSummaryInfo;
  MEMORY_PROFILE_ALLOC_SUMMARY_INFO_DATA        *AllocSummaryInfoData;

  AllocSummaryInfoList = DriverSummaryInfoData->AllocSummaryInfoList;

  for (AllocSummaryLink = AllocSummaryInfoList->ForwardLink;
       AllocSummaryLink != AllocSummaryInfoList;
       AllocSummaryLink = AllocSummaryLink->ForwardLink) {
    AllocSummaryInfoData = CR (
                             AllocSummaryLink,
                             MEMORY_PROFILE_ALLOC_SUMMARY_INFO_DATA,
                             Link,
                             MEMORY_PROFILE_ALLOC_SUMMARY_INFO_SIGNATURE
                             );
    AllocSummaryInfo = &AllocSummaryInfoData->AllocSummaryInfo;
    if (AllocSummaryInfo->CallerAddress == CallerAddress) {
      return AllocSummaryInfoData;
    }
  }
  return NULL;
}

/**
  Create Allocate summary information structure and
  link to Driver summary information data structure.

  @param[in, out] DriverSummaryInfoData Driver summary information data structure.
  @param[in]      AllocInfo             Pointer to memory profile alloc info.

  @return Pointer to next memory profile alloc info.

**/
MEMORY_PROFILE_ALLOC_INFO *
CreateAllocSummaryInfo (
  IN OUT MEMORY_PROFILE_DRIVER_SUMMARY_INFO_DATA    *DriverSummaryInfoData,
  IN MEMORY_PROFILE_ALLOC_INFO                      *AllocInfo
  )
{
  MEMORY_PROFILE_ALLOC_SUMMARY_INFO_DATA            *AllocSummaryInfoData;
  MEMORY_PROFILE_ALLOC_SUMMARY_INFO                 *AllocSummaryInfo;

  if (AllocInfo->Header.Signature != MEMORY_PROFILE_ALLOC_INFO_SIGNATURE) {
    return NULL;
  }

  AllocSummaryInfoData = GetAllocSummaryInfoByCallerAddress (AllocInfo->CallerAddress, DriverSummaryInfoData);
  if (AllocSummaryInfoData == NULL) {
    AllocSummaryInfoData = AllocatePool (sizeof (*AllocSummaryInfoData));
    if (AllocSummaryInfoData == NULL) {
      return NULL;
    }

    AllocSummaryInfoData->Signature = MEMORY_PROFILE_ALLOC_SUMMARY_INFO_SIGNATURE;
    AllocSummaryInfo = &AllocSummaryInfoData->AllocSummaryInfo;
    AllocSummaryInfo->Header.Signature = MEMORY_PROFILE_ALLOC_SUMMARY_INFO_SIGNATURE;
    AllocSummaryInfo->Header.Length    = sizeof (*AllocSummaryInfo);
    AllocSummaryInfo->Header.Revision  = MEMORY_PROFILE_ALLOC_SUMMARY_INFO_REVISION;
    AllocSummaryInfo->CallerAddress = AllocInfo->CallerAddress;
    AllocSummaryInfo->Action        = AllocInfo->Action;
    if (AllocInfo->ActionStringOffset != 0) {
      AllocSummaryInfo->ActionString = (CHAR8 *) ((UINTN) AllocInfo + AllocInfo->ActionStringOffset);
    } else {
      AllocSummaryInfo->ActionString  = NULL;
    }
    AllocSummaryInfo->AllocateCount = 0;
    AllocSummaryInfo->TotalSize     = 0;
    InsertTailList (DriverSummaryInfoData->AllocSummaryInfoList, &AllocSummaryInfoData->Link);
  }
  AllocSummaryInfo = &AllocSummaryInfoData->AllocSummaryInfo;
  AllocSummaryInfo->AllocateCount ++;
  AllocSummaryInfo->TotalSize += AllocInfo->Size;

  return (MEMORY_PROFILE_ALLOC_INFO *) ((UINTN) AllocInfo + AllocInfo->Header.Length);
}

/**
  Create Driver summary information structure and
  link to Context summary information data structure.

  @param[in, out] ContextSummaryData    Context summary information data structure.
  @param[in]      DriverInfo            Pointer to memory profile driver info.

  @return Pointer to next memory profile driver info.

**/
MEMORY_PROFILE_DRIVER_INFO *
CreateDriverSummaryInfo (
  IN OUT MEMORY_PROFILE_CONTEXT_SUMMARY_DATA    *ContextSummaryData,
  IN MEMORY_PROFILE_DRIVER_INFO                 *DriverInfo
  )
{
  MEMORY_PROFILE_DRIVER_SUMMARY_INFO_DATA       *DriverSummaryInfoData;
  MEMORY_PROFILE_ALLOC_INFO                     *AllocInfo;
  UINTN                                         AllocIndex;

  if (DriverInfo->Header.Signature != MEMORY_PROFILE_DRIVER_INFO_SIGNATURE) {
    return NULL;
  }

  DriverSummaryInfoData = AllocatePool (sizeof (*DriverSummaryInfoData) + sizeof (LIST_ENTRY));
  if (DriverSummaryInfoData == NULL) {
    return NULL;
  }
  DriverSummaryInfoData->Signature = MEMORY_PROFILE_DRIVER_INFO_SIGNATURE;
  DriverSummaryInfoData->DriverInfo = DriverInfo;
  DriverSummaryInfoData->AllocSummaryInfoList = (LIST_ENTRY *) (DriverSummaryInfoData + 1);
  InitializeListHead (DriverSummaryInfoData->AllocSummaryInfoList);
  InsertTailList (ContextSummaryData->DriverSummaryInfoList, &DriverSummaryInfoData->Link);

  AllocInfo = (MEMORY_PROFILE_ALLOC_INFO *) ((UINTN) DriverInfo + DriverInfo->Header.Length);
  for (AllocIndex = 0; AllocIndex < DriverInfo->AllocRecordCount; AllocIndex++) {
    AllocInfo = CreateAllocSummaryInfo (DriverSummaryInfoData, AllocInfo);
    if (AllocInfo == NULL) {
      return NULL;
    }
  }
  return (MEMORY_PROFILE_DRIVER_INFO *) AllocInfo;
}

/**
  Create Context summary information structure.

  @param[in] ProfileBuffer      Memory profile base address.
  @param[in] ProfileSize        Memory profile size.

  @return Context summary information structure.

**/
MEMORY_PROFILE_CONTEXT_SUMMARY_DATA *
CreateContextSummaryData (
  IN PHYSICAL_ADDRESS           ProfileBuffer,
  IN UINT64                     ProfileSize
  )
{
  MEMORY_PROFILE_CONTEXT        *Context;
  MEMORY_PROFILE_DRIVER_INFO    *DriverInfo;
  UINTN                         DriverIndex;

  Context = (MEMORY_PROFILE_CONTEXT *) ScanMemoryProfileBySignature (ProfileBuffer, ProfileSize, MEMORY_PROFILE_CONTEXT_SIGNATURE);
  if (Context == NULL) {
    return NULL;
  }

  mMemoryProfileContextSummary.Signature = MEMORY_PROFILE_CONTEXT_SIGNATURE;
  mMemoryProfileContextSummary.Context = Context;
  mMemoryProfileContextSummary.DriverSummaryInfoList = &mImageSummaryQueue;

  DriverInfo = (MEMORY_PROFILE_DRIVER_INFO *) ((UINTN) Context + Context->Header.Length);
  for (DriverIndex = 0; DriverIndex < Context->ImageCount; DriverIndex++) {
    DriverInfo = CreateDriverSummaryInfo (&mMemoryProfileContextSummary, DriverInfo);
    if (DriverInfo == NULL) {
      return NULL;
    }
  }

  return &mMemoryProfileContextSummary;
}

/**
  Dump Context summary information.

  @param[in] ContextSummaryData Context summary information data.
  @param[in] IsForSmm           TRUE  - SMRAM profile.
                                FALSE - UEFI memory profile.

**/
VOID
DumpContextSummaryData (
  IN MEMORY_PROFILE_CONTEXT_SUMMARY_DATA    *ContextSummaryData,
  IN BOOLEAN                                IsForSmm
  )
{
  MEMORY_PROFILE_DRIVER_SUMMARY_INFO_DATA   *DriverSummaryInfoData;
  MEMORY_PROFILE_ALLOC_SUMMARY_INFO_DATA    *AllocSummaryInfoData;
  LIST_ENTRY                                *DriverSummaryInfoList;
  LIST_ENTRY                                *DriverSummaryLink;
  LIST_ENTRY                                *AllocSummaryInfoList;
  LIST_ENTRY                                *AllocSummaryLink;
  MEMORY_PROFILE_DRIVER_INFO                *DriverInfo;
  MEMORY_PROFILE_ALLOC_SUMMARY_INFO         *AllocSummaryInfo;
  CHAR8                                     *NameString;

  if (ContextSummaryData == NULL) {
    return ;
  }

  Print (L"\nSummary Data:\n");

  DriverSummaryInfoList = ContextSummaryData->DriverSummaryInfoList;
  for (DriverSummaryLink = DriverSummaryInfoList->ForwardLink;
       DriverSummaryLink != DriverSummaryInfoList;
       DriverSummaryLink = DriverSummaryLink->ForwardLink) {
    DriverSummaryInfoData = CR (
                              DriverSummaryLink,
                              MEMORY_PROFILE_DRIVER_SUMMARY_INFO_DATA,
                              Link,
                              MEMORY_PROFILE_DRIVER_INFO_SIGNATURE
                              );
    DriverInfo = DriverSummaryInfoData->DriverInfo;

    NameString = GetDriverNameString (DriverInfo);
    Print (L"\nDriver - %a (Usage - 0x%08x)", NameString, DriverInfo->CurrentUsage);
    if (DriverInfo->CurrentUsage == 0) {
      Print (L"\n");
      continue;
    }

    if (DriverInfo->PdbStringOffset != 0) {
      Print (L" (Pdb - %a)\n", (CHAR8 *) ((UINTN) DriverInfo + DriverInfo->PdbStringOffset));
    } else {
      Print (L"\n");
    }
    Print (L"Caller List:\n");
    Print(L"  Count            Size                   RVA              Action\n");
    Print(L"==========  ==================     ================== (================================)\n");
    AllocSummaryInfoList = DriverSummaryInfoData->AllocSummaryInfoList;
    for (AllocSummaryLink = AllocSummaryInfoList->ForwardLink;
         AllocSummaryLink != AllocSummaryInfoList;
         AllocSummaryLink = AllocSummaryLink->ForwardLink) {
      AllocSummaryInfoData = CR (
                               AllocSummaryLink,
                               MEMORY_PROFILE_ALLOC_SUMMARY_INFO_DATA,
                               Link,
                               MEMORY_PROFILE_ALLOC_SUMMARY_INFO_SIGNATURE
                               );
      AllocSummaryInfo = &AllocSummaryInfoData->AllocSummaryInfo;

      Print(L"0x%08x  0x%016lx <== 0x%016lx",
        AllocSummaryInfo->AllocateCount,
        AllocSummaryInfo->TotalSize,
        AllocSummaryInfo->CallerAddress - DriverInfo->ImageBase
        );
      Print (L" (%a)\n", ProfileActionToStr (AllocSummaryInfo->Action, AllocSummaryInfo->ActionString, IsForSmm));
    }
  }
  return ;
}

/**
  Destroy Context summary information.

  @param[in, out] ContextSummaryData    Context summary information data.

**/
VOID
DestroyContextSummaryData (
  IN OUT MEMORY_PROFILE_CONTEXT_SUMMARY_DATA    *ContextSummaryData
  )
{
  MEMORY_PROFILE_DRIVER_SUMMARY_INFO_DATA       *DriverSummaryInfoData;
  MEMORY_PROFILE_ALLOC_SUMMARY_INFO_DATA        *AllocSummaryInfoData;
  LIST_ENTRY                                    *DriverSummaryInfoList;
  LIST_ENTRY                                    *DriverSummaryLink;
  LIST_ENTRY                                    *AllocSummaryInfoList;
  LIST_ENTRY                                    *AllocSummaryLink;

  if (ContextSummaryData == NULL) {
    return ;
  }

  DriverSummaryInfoList = ContextSummaryData->DriverSummaryInfoList;
  for (DriverSummaryLink = DriverSummaryInfoList->ForwardLink;
       DriverSummaryLink != DriverSummaryInfoList;
       ) {
    DriverSummaryInfoData = CR (
                              DriverSummaryLink,
                              MEMORY_PROFILE_DRIVER_SUMMARY_INFO_DATA,
                              Link,
                              MEMORY_PROFILE_DRIVER_INFO_SIGNATURE
                              );
    DriverSummaryLink = DriverSummaryLink->ForwardLink;

    AllocSummaryInfoList = DriverSummaryInfoData->AllocSummaryInfoList;
    for (AllocSummaryLink = AllocSummaryInfoList->ForwardLink;
         AllocSummaryLink != AllocSummaryInfoList;
         ) {
      AllocSummaryInfoData = CR (
                               AllocSummaryLink,
                               MEMORY_PROFILE_ALLOC_SUMMARY_INFO_DATA,
                               Link,
                               MEMORY_PROFILE_ALLOC_SUMMARY_INFO_SIGNATURE
                               );
      AllocSummaryLink = AllocSummaryLink->ForwardLink;

      RemoveEntryList (&AllocSummaryInfoData->Link);
      FreePool (AllocSummaryInfoData);
    }

    RemoveEntryList (&DriverSummaryInfoData->Link);
    FreePool (DriverSummaryInfoData);
  }
  return ;
}

/**
  Get and dump UEFI memory profile data.

  @return EFI_SUCCESS   Get the memory profile data successfully.
  @return other         Fail to get the memory profile data.

**/
EFI_STATUS
GetUefiMemoryProfileData (
  VOID
  )
{
  EFI_STATUS                          Status;
  EDKII_MEMORY_PROFILE_PROTOCOL       *ProfileProtocol;
  VOID                                *Data;
  UINT64                              Size;
  MEMORY_PROFILE_CONTEXT_SUMMARY_DATA *MemoryProfileContextSummaryData;
  BOOLEAN                             RecordingState;

  Status = gBS->LocateProtocol (&gEdkiiMemoryProfileGuid, NULL, (VOID **) &ProfileProtocol);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "UefiMemoryProfile: Locate MemoryProfile protocol - %r\n", Status));
    return Status;
  }

  //
  // Set recording state if needed.
  //
  RecordingState = MEMORY_PROFILE_RECORDING_DISABLE;
  Status = ProfileProtocol->GetRecordingState (ProfileProtocol, &RecordingState);
  if (RecordingState == MEMORY_PROFILE_RECORDING_ENABLE) {
    ProfileProtocol->SetRecordingState (ProfileProtocol, MEMORY_PROFILE_RECORDING_DISABLE);
  }

  Size = 0;
  Data = NULL;
  Status = ProfileProtocol->GetData (
                              ProfileProtocol,
                              &Size,
                              Data
                              );
  if (Status != EFI_BUFFER_TOO_SMALL) {
    Print (L"UefiMemoryProfile: GetData - %r\n", Status);
    goto Done;
  }

  Data = AllocateZeroPool ((UINTN) Size);
  if (Data == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    Print (L"UefiMemoryProfile: AllocateZeroPool (0x%x) - %r\n", Size, Status);
    return Status;
  }

  Status = ProfileProtocol->GetData (
                              ProfileProtocol,
                              &Size,
                              Data
                              );
  if (EFI_ERROR (Status)) {
    Print (L"UefiMemoryProfile: GetData - %r\n", Status);
    goto Done;
  }


  Print (L"UefiMemoryProfileSize - 0x%x\n", Size);
  Print (L"======= UefiMemoryProfile begin =======\n");
  DumpMemoryProfile ((PHYSICAL_ADDRESS) (UINTN) Data, Size, FALSE);

  //
  // Dump summary information
  //
  MemoryProfileContextSummaryData = CreateContextSummaryData ((PHYSICAL_ADDRESS) (UINTN) Data, Size);
  if (MemoryProfileContextSummaryData != NULL) {
    DumpContextSummaryData (MemoryProfileContextSummaryData, FALSE);
    DestroyContextSummaryData (MemoryProfileContextSummaryData);
  }

  Print (L"======= UefiMemoryProfile end =======\n\n\n");

Done:
  if (Data != NULL) {
    FreePool (Data);
  }

  //
  // Restore recording state if needed.
  //
  if (RecordingState == MEMORY_PROFILE_RECORDING_ENABLE) {
    ProfileProtocol->SetRecordingState (ProfileProtocol, MEMORY_PROFILE_RECORDING_ENABLE);
  }

  return Status;
}

/**
  Get and dump SMRAM profile data.

  @return EFI_SUCCESS   Get the SMRAM profile data successfully.
  @return other         Fail to get the SMRAM profile data.

**/
EFI_STATUS
GetSmramProfileData (
  VOID
  )
{
  EFI_STATUS                                    Status;
  UINTN                                         CommSize;
  UINT8                                         *CommBuffer;
  EFI_SMM_COMMUNICATE_HEADER                    *CommHeader;
  SMRAM_PROFILE_PARAMETER_GET_PROFILE_INFO      *CommGetProfileInfo;
  SMRAM_PROFILE_PARAMETER_GET_PROFILE_DATA_BY_OFFSET *CommGetProfileData;
  SMRAM_PROFILE_PARAMETER_RECORDING_STATE       *CommRecordingState;
  UINTN                                         ProfileSize;
  VOID                                          *ProfileBuffer;
  EFI_SMM_COMMUNICATION_PROTOCOL                *SmmCommunication;
  UINTN                                         MinimalSizeNeeded;
  EDKII_PI_SMM_COMMUNICATION_REGION_TABLE       *PiSmmCommunicationRegionTable;
  UINT32                                        Index;
  EFI_MEMORY_DESCRIPTOR                         *Entry;
  VOID                                          *Buffer;
  UINTN                                         Size;
  UINTN                                         Offset;
  MEMORY_PROFILE_CONTEXT_SUMMARY_DATA           *MemoryProfileContextSummaryData;
  BOOLEAN                                       RecordingState;

  ProfileBuffer = NULL;

  Status = gBS->LocateProtocol (&gEfiSmmCommunicationProtocolGuid, NULL, (VOID **) &SmmCommunication);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "SmramProfile: Locate SmmCommunication protocol - %r\n", Status));
    return Status;
  }

  MinimalSizeNeeded = sizeof (EFI_GUID) +
                      sizeof (UINTN) +
                      MAX (sizeof (SMRAM_PROFILE_PARAMETER_GET_PROFILE_INFO),
                           MAX (sizeof (SMRAM_PROFILE_PARAMETER_GET_PROFILE_DATA_BY_OFFSET),
                                sizeof (SMRAM_PROFILE_PARAMETER_RECORDING_STATE)));
  MinimalSizeNeeded += MAX (sizeof (MEMORY_PROFILE_CONTEXT),
                            MAX (sizeof (MEMORY_PROFILE_DRIVER_INFO),
                                 MAX (sizeof (MEMORY_PROFILE_ALLOC_INFO),
                                      MAX (sizeof (MEMORY_PROFILE_DESCRIPTOR),
                                           MAX (sizeof (MEMORY_PROFILE_FREE_MEMORY),
                                                sizeof (MEMORY_PROFILE_MEMORY_RANGE))))));

  Status = EfiGetSystemConfigurationTable (
             &gEdkiiPiSmmCommunicationRegionTableGuid,
             (VOID **) &PiSmmCommunicationRegionTable
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "SmramProfile: Get PiSmmCommunicationRegionTable - %r\n", Status));
    return Status;
  }
  ASSERT (PiSmmCommunicationRegionTable != NULL);
  Entry = (EFI_MEMORY_DESCRIPTOR *) (PiSmmCommunicationRegionTable + 1);
  Size = 0;
  for (Index = 0; Index < PiSmmCommunicationRegionTable->NumberOfEntries; Index++) {
    if (Entry->Type == EfiConventionalMemory) {
      Size = EFI_PAGES_TO_SIZE ((UINTN) Entry->NumberOfPages);
      if (Size >= MinimalSizeNeeded) {
        break;
      }
    }
    Entry = (EFI_MEMORY_DESCRIPTOR *) ((UINT8 *) Entry + PiSmmCommunicationRegionTable->DescriptorSize);
  }
  ASSERT (Index < PiSmmCommunicationRegionTable->NumberOfEntries);
  CommBuffer = (UINT8 *) (UINTN) Entry->PhysicalStart;

  //
  // Set recording state if needed.
  //
  RecordingState = MEMORY_PROFILE_RECORDING_DISABLE;

  CommHeader = (EFI_SMM_COMMUNICATE_HEADER *) &CommBuffer[0];
  CopyMem (&CommHeader->HeaderGuid, &gEdkiiMemoryProfileGuid, sizeof (gEdkiiMemoryProfileGuid));
  CommHeader->MessageLength = sizeof (SMRAM_PROFILE_PARAMETER_RECORDING_STATE);

  CommRecordingState = (SMRAM_PROFILE_PARAMETER_RECORDING_STATE *) &CommBuffer[OFFSET_OF (EFI_SMM_COMMUNICATE_HEADER, Data)];
  CommRecordingState->Header.Command      = SMRAM_PROFILE_COMMAND_GET_RECORDING_STATE;
  CommRecordingState->Header.DataLength   = sizeof (*CommRecordingState);
  CommRecordingState->Header.ReturnStatus = (UINT64)-1;
  CommRecordingState->RecordingState      = MEMORY_PROFILE_RECORDING_DISABLE;

  CommSize = sizeof (EFI_GUID) + sizeof (UINTN) + CommHeader->MessageLength;
  Status = SmmCommunication->Communicate (SmmCommunication, CommBuffer, &CommSize);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "SmramProfile: SmmCommunication - %r\n", Status));
    return Status;
  }

  if (CommRecordingState->Header.ReturnStatus != 0) {
    Print (L"SmramProfile: GetRecordingState - 0x%0x\n", CommRecordingState->Header.ReturnStatus);
    return EFI_SUCCESS;
  }
  RecordingState = CommRecordingState->RecordingState;
  if (RecordingState == MEMORY_PROFILE_RECORDING_ENABLE) {
    CommHeader = (EFI_SMM_COMMUNICATE_HEADER *) &CommBuffer[0];
    CopyMem (&CommHeader->HeaderGuid, &gEdkiiMemoryProfileGuid, sizeof (gEdkiiMemoryProfileGuid));
    CommHeader->MessageLength = sizeof (SMRAM_PROFILE_PARAMETER_RECORDING_STATE);

    CommRecordingState = (SMRAM_PROFILE_PARAMETER_RECORDING_STATE *) &CommBuffer[OFFSET_OF (EFI_SMM_COMMUNICATE_HEADER, Data)];
    CommRecordingState->Header.Command      = SMRAM_PROFILE_COMMAND_SET_RECORDING_STATE;
    CommRecordingState->Header.DataLength   = sizeof (*CommRecordingState);
    CommRecordingState->Header.ReturnStatus = (UINT64)-1;
    CommRecordingState->RecordingState      = MEMORY_PROFILE_RECORDING_DISABLE;

    CommSize = sizeof (EFI_GUID) + sizeof (UINTN) + CommHeader->MessageLength;
    SmmCommunication->Communicate (SmmCommunication, CommBuffer, &CommSize);
  }

  //
  // Get Size
  //
  CommHeader = (EFI_SMM_COMMUNICATE_HEADER *) &CommBuffer[0];
  CopyMem (&CommHeader->HeaderGuid, &gEdkiiMemoryProfileGuid, sizeof (gEdkiiMemoryProfileGuid));
  CommHeader->MessageLength = sizeof (SMRAM_PROFILE_PARAMETER_GET_PROFILE_INFO);

  CommGetProfileInfo = (SMRAM_PROFILE_PARAMETER_GET_PROFILE_INFO *) &CommBuffer[OFFSET_OF (EFI_SMM_COMMUNICATE_HEADER, Data)];
  CommGetProfileInfo->Header.Command      = SMRAM_PROFILE_COMMAND_GET_PROFILE_INFO;
  CommGetProfileInfo->Header.DataLength   = sizeof (*CommGetProfileInfo);
  CommGetProfileInfo->Header.ReturnStatus = (UINT64)-1;
  CommGetProfileInfo->ProfileSize         = 0;

  CommSize = sizeof (EFI_GUID) + sizeof (UINTN) + CommHeader->MessageLength;
  Status = SmmCommunication->Communicate (SmmCommunication, CommBuffer, &CommSize);
  ASSERT_EFI_ERROR (Status);

  if (CommGetProfileInfo->Header.ReturnStatus != 0) {
    Status = EFI_SUCCESS;
    Print (L"SmramProfile: GetProfileInfo - 0x%0x\n", CommGetProfileInfo->Header.ReturnStatus);
    goto Done;
  }

  ProfileSize = (UINTN) CommGetProfileInfo->ProfileSize;

  //
  // Get Data
  //
  ProfileBuffer = AllocateZeroPool (ProfileSize);
  if (ProfileBuffer == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    Print (L"SmramProfile: AllocateZeroPool (0x%x) for profile buffer - %r\n", ProfileSize, Status);
    goto Done;
  }

  CommHeader = (EFI_SMM_COMMUNICATE_HEADER *) &CommBuffer[0];
  CopyMem (&CommHeader->HeaderGuid, &gEdkiiMemoryProfileGuid, sizeof(gEdkiiMemoryProfileGuid));
  CommHeader->MessageLength = sizeof (SMRAM_PROFILE_PARAMETER_GET_PROFILE_DATA_BY_OFFSET);

  CommGetProfileData = (SMRAM_PROFILE_PARAMETER_GET_PROFILE_DATA_BY_OFFSET *) &CommBuffer[OFFSET_OF (EFI_SMM_COMMUNICATE_HEADER, Data)];
  CommGetProfileData->Header.Command      = SMRAM_PROFILE_COMMAND_GET_PROFILE_DATA_BY_OFFSET;
  CommGetProfileData->Header.DataLength   = sizeof (*CommGetProfileData);
  CommGetProfileData->Header.ReturnStatus = (UINT64)-1;

  CommSize = sizeof (EFI_GUID) + sizeof (UINTN) + CommHeader->MessageLength;
  Buffer = (UINT8 *) CommHeader + CommSize;
  Size -= CommSize;

  CommGetProfileData->ProfileBuffer       = (PHYSICAL_ADDRESS) (UINTN) Buffer;
  CommGetProfileData->ProfileOffset       = 0;
  while (CommGetProfileData->ProfileOffset < ProfileSize) {
    Offset = (UINTN) CommGetProfileData->ProfileOffset;
    if (Size <= (ProfileSize - CommGetProfileData->ProfileOffset)) {
      CommGetProfileData->ProfileSize = (UINT64) Size;
    } else {
      CommGetProfileData->ProfileSize = (UINT64) (ProfileSize - CommGetProfileData->ProfileOffset);
    }
    Status = SmmCommunication->Communicate (SmmCommunication, CommBuffer, &CommSize);
    ASSERT_EFI_ERROR (Status);

    if (CommGetProfileData->Header.ReturnStatus != 0) {
      Status = EFI_SUCCESS;
      Print (L"GetProfileData - 0x%x\n", CommGetProfileData->Header.ReturnStatus);
      goto Done;
    }
    CopyMem ((UINT8 *) ProfileBuffer + Offset, (VOID *) (UINTN) CommGetProfileData->ProfileBuffer, (UINTN) CommGetProfileData->ProfileSize);
  }


  Print (L"SmramProfileSize - 0x%x\n", ProfileSize);
  Print (L"======= SmramProfile begin =======\n");
  DumpMemoryProfile ((PHYSICAL_ADDRESS) (UINTN) ProfileBuffer, ProfileSize, TRUE);

  //
  // Dump summary information
  //
  MemoryProfileContextSummaryData = CreateContextSummaryData ((PHYSICAL_ADDRESS) (UINTN) ProfileBuffer, ProfileSize);
  if (MemoryProfileContextSummaryData != NULL) {
    DumpContextSummaryData (MemoryProfileContextSummaryData, TRUE);
    DestroyContextSummaryData (MemoryProfileContextSummaryData);
  }

  Print (L"======= SmramProfile end =======\n\n\n");

Done:
  if (ProfileBuffer != NULL) {
    FreePool (ProfileBuffer);
  }

  //
  // Restore recording state if needed.
  //
  if (RecordingState == MEMORY_PROFILE_RECORDING_ENABLE) {
    CommHeader = (EFI_SMM_COMMUNICATE_HEADER *) &CommBuffer[0];
    CopyMem (&CommHeader->HeaderGuid, &gEdkiiMemoryProfileGuid, sizeof (gEdkiiMemoryProfileGuid));
    CommHeader->MessageLength = sizeof (SMRAM_PROFILE_PARAMETER_RECORDING_STATE);

    CommRecordingState = (SMRAM_PROFILE_PARAMETER_RECORDING_STATE *) &CommBuffer[OFFSET_OF (EFI_SMM_COMMUNICATE_HEADER, Data)];
    CommRecordingState->Header.Command      = SMRAM_PROFILE_COMMAND_SET_RECORDING_STATE;
    CommRecordingState->Header.DataLength   = sizeof (*CommRecordingState);
    CommRecordingState->Header.ReturnStatus = (UINT64)-1;
    CommRecordingState->RecordingState      = MEMORY_PROFILE_RECORDING_ENABLE;

    CommSize = sizeof (EFI_GUID) + sizeof (UINTN) + CommHeader->MessageLength;
    SmmCommunication->Communicate (SmmCommunication, CommBuffer, &CommSize);
  }

  return Status;
}

/**
  The user Entry Point for Application. The user code starts with this function
  as the real entry point for the image goes into a library that calls this function.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
UefiMain (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS                     Status;

  Status = GetUefiMemoryProfileData ();
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "GetUefiMemoryProfileData - %r\n", Status));
  }

  Status = GetSmramProfileData ();
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "GetSmramProfileData - %r\n", Status));
  }

  return EFI_SUCCESS;
}
