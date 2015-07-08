/** @file
  
  Copyright (c) 2014 - 2015, Intel Corporation. All rights reserved.<BR>
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
#include <Library/PeCoffGetEntryPointLib.h>
#include <Library/PrintLib.h>

#include <Protocol/SmmCommunication.h>
#include <Protocol/SmmAccess2.h>

#include <Guid/ZeroGuid.h>
#include <Guid/MemoryProfile.h>

CHAR16 *mActionString[] = {
  L"Unknown",
  L"AllocatePages",
  L"FreePages",
  L"AllocatePool",
  L"FreePool",
};

CHAR16 *mMemoryTypeString[] = {
  L"EfiReservedMemoryType",
  L"EfiLoaderCode",
  L"EfiLoaderData",
  L"EfiBootServicesCode",
  L"EfiBootServicesData",
  L"EfiRuntimeServicesCode",
  L"EfiRuntimeServicesData",
  L"EfiConventionalMemory",
  L"EfiUnusableMemory",
  L"EfiACPIReclaimMemory",
  L"EfiACPIMemoryNVS",
  L"EfiMemoryMappedIO",
  L"EfiMemoryMappedIOPortSpace",
  L"EfiPalCode",
  L"EfiPersistentMemory",
  L"EfiOSReserved",
  L"EfiOemReserved",
};

CHAR16 *mSubsystemString[] = {
  L"Unknown",
  L"NATIVE",
  L"WINDOWS_GUI",
  L"WINDOWS_CUI",
  L"Unknown",
  L"Unknown",
  L"Unknown",
  L"POSIX_CUI",
  L"Unknown",
  L"WINDOWS_CE_GUI",
  L"EFI_APPLICATION",
  L"EFI_BOOT_SERVICE_DRIVER",
  L"EFI_RUNTIME_DRIVER",
  L"EFI_ROM",
  L"XBOX",
  L"Unknown",
};

CHAR16 *mFileTypeString[] = {
  L"Unknown",
  L"RAW",
  L"FREEFORM",
  L"SECURITY_CORE",
  L"PEI_CORE",
  L"DXE_CORE",
  L"PEIM",
  L"DRIVER",
  L"COMBINED_PEIM_DRIVER",
  L"APPLICATION",
  L"SMM",
  L"FIRMWARE_VOLUME_IMAGE",
  L"COMBINED_SMM_DXE",
  L"SMM_CORE",
};

#define PROFILE_NAME_STRING_LENGTH  36
CHAR16 mNameString[PROFILE_NAME_STRING_LENGTH + 1];

/** 
  Get the file name portion of the Pdb File Name.
  
  The portion of the Pdb File Name between the last backslash and
  either a following period or the end of the string is converted
  to Unicode and copied into UnicodeBuffer.  The name is truncated,
  if necessary, to ensure that UnicodeBuffer is not overrun.
  
  @param[in]  PdbFileName     Pdb file name.
  @param[out] UnicodeBuffer   The resultant Unicode File Name.
  
**/
VOID
GetShortPdbFileName (
  IN  CHAR8     *PdbFileName,
  OUT CHAR16    *UnicodeBuffer
  )
{
  UINTN IndexA;     // Current work location within an ASCII string.
  UINTN IndexU;     // Current work location within a Unicode string.
  UINTN StartIndex;
  UINTN EndIndex;

  ZeroMem (UnicodeBuffer, (PROFILE_NAME_STRING_LENGTH + 1) * sizeof (CHAR16));

  if (PdbFileName == NULL) {
    StrnCpyS (UnicodeBuffer, PROFILE_NAME_STRING_LENGTH + 1, L" ", 1);
  } else {
    StartIndex = 0;
    for (EndIndex = 0; PdbFileName[EndIndex] != 0; EndIndex++);
    for (IndexA = 0; PdbFileName[IndexA] != 0; IndexA++) {
      if (PdbFileName[IndexA] == '\\') {
        StartIndex = IndexA + 1;
      }

      if (PdbFileName[IndexA] == '.') {
        EndIndex = IndexA;
      }
    }

    IndexU = 0;
    for (IndexA = StartIndex; IndexA < EndIndex; IndexA++) {
      UnicodeBuffer[IndexU] = (CHAR16) PdbFileName[IndexA];
      IndexU++;
      if (IndexU >= PROFILE_NAME_STRING_LENGTH) {
        UnicodeBuffer[PROFILE_NAME_STRING_LENGTH] = 0;
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

  @post The resulting Unicode name string is stored in the mNameString global array.

**/
VOID
GetDriverNameString (
 IN MEMORY_PROFILE_DRIVER_INFO  *DriverInfo
 )
{
  EFI_STATUS                  Status;
  CHAR8                       *PdbFileName;
  CHAR16                      *NameString;
  UINTN                       StringSize;

  //
  // Method 1: Get the name string from image PDB
  //
  if ((DriverInfo->ImageBase != 0) && (DriverInfo->FileType != EFI_FV_FILETYPE_SMM) && (DriverInfo->FileType != EFI_FV_FILETYPE_SMM_CORE)) {
    PdbFileName = PeCoffLoaderGetPdbPointer ((VOID *) (UINTN) DriverInfo->ImageBase);

    if (PdbFileName != NULL) {
      GetShortPdbFileName (PdbFileName, mNameString);
      return;
    }
  }

  if (!CompareGuid (&DriverInfo->FileName, &gZeroGuid)) {
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
      StrnCpyS (mNameString, PROFILE_NAME_STRING_LENGTH + 1, NameString, PROFILE_NAME_STRING_LENGTH);
      mNameString[PROFILE_NAME_STRING_LENGTH] = 0;
      FreePool (NameString);
      return;
    }
  }

  //
  // Method 3: Get the name string from image GUID
  //
  UnicodeSPrint (mNameString, sizeof (mNameString), L"%g", &DriverInfo->FileName);
}

/**
  Memory type to string.

  @param[in] MemoryType Memory type.

  @return Pointer to string.

**/
CHAR16 *
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
  Dump memory profile allocate information.

  @param[in] DriverInfo         Pointer to memory profile driver info.
  @param[in] AllocIndex         Memory profile alloc info index.
  @param[in] AllocInfo          Pointer to memory profile alloc info.

  @return Pointer to next memory profile alloc info.

**/
MEMORY_PROFILE_ALLOC_INFO *
DumpMemoryProfileAllocInfo (
  IN MEMORY_PROFILE_DRIVER_INFO *DriverInfo,
  IN UINTN                      AllocIndex,
  IN MEMORY_PROFILE_ALLOC_INFO  *AllocInfo
  )
{
  if (AllocInfo->Header.Signature != MEMORY_PROFILE_ALLOC_INFO_SIGNATURE) {
    return NULL;
  }
  Print (L"    MEMORY_PROFILE_ALLOC_INFO (0x%x)\n", AllocIndex);
  Print (L"      Signature     - 0x%08x\n", AllocInfo->Header.Signature);
  Print (L"      Length        - 0x%04x\n", AllocInfo->Header.Length);
  Print (L"      Revision      - 0x%04x\n", AllocInfo->Header.Revision);  
  Print (L"      CallerAddress - 0x%016lx (Offset: 0x%08x)\n", AllocInfo->CallerAddress, (UINTN) (AllocInfo->CallerAddress - DriverInfo->ImageBase));
  Print (L"      SequenceId    - 0x%08x\n", AllocInfo->SequenceId);
  Print (L"      Action        - 0x%08x (%s)\n", AllocInfo->Action, mActionString[(AllocInfo->Action < sizeof(mActionString)/sizeof(mActionString[0])) ? AllocInfo->Action : 0]);
  Print (L"      MemoryType    - 0x%08x (%s)\n", AllocInfo->MemoryType, ProfileMemoryTypeToStr (AllocInfo->MemoryType));
  Print (L"      Buffer        - 0x%016lx\n", AllocInfo->Buffer);
  Print (L"      Size          - 0x%016lx\n", AllocInfo->Size);

  return (MEMORY_PROFILE_ALLOC_INFO *) ((UINTN) AllocInfo + AllocInfo->Header.Length);
}

/**
  Dump memory profile driver information.

  @param[in] DriverIndex        Memory profile driver info index.
  @param[in] DriverInfo         Pointer to memory profile driver info.

  @return Pointer to next memory profile driver info.

**/
MEMORY_PROFILE_DRIVER_INFO *
DumpMemoryProfileDriverInfo (
  IN UINTN                      DriverIndex,
  IN MEMORY_PROFILE_DRIVER_INFO *DriverInfo
  )
{
  UINTN                         TypeIndex;
  MEMORY_PROFILE_ALLOC_INFO     *AllocInfo;
  UINTN                         AllocIndex;

  if (DriverInfo->Header.Signature != MEMORY_PROFILE_DRIVER_INFO_SIGNATURE) {
    return NULL;
  }
  Print (L"  MEMORY_PROFILE_DRIVER_INFO (0x%x)\n", DriverIndex);
  Print (L"    Signature               - 0x%08x\n", DriverInfo->Header.Signature);
  Print (L"    Length                  - 0x%04x\n", DriverInfo->Header.Length);
  Print (L"    Revision                - 0x%04x\n", DriverInfo->Header.Revision);  
  GetDriverNameString (DriverInfo);
  Print (L"    FileName                - %s\n", &mNameString);
  Print (L"    ImageBase               - 0x%016lx\n", DriverInfo->ImageBase);
  Print (L"    ImageSize               - 0x%016lx\n", DriverInfo->ImageSize);
  Print (L"    EntryPoint              - 0x%016lx\n", DriverInfo->EntryPoint);
  Print (L"    ImageSubsystem          - 0x%04x (%s)\n", DriverInfo->ImageSubsystem, mSubsystemString[(DriverInfo->ImageSubsystem < sizeof(mSubsystemString)/sizeof(mSubsystemString[0])) ? DriverInfo->ImageSubsystem : 0]);
  Print (L"    FileType                - 0x%02x (%s)\n", DriverInfo->FileType, mFileTypeString[(DriverInfo->FileType < sizeof(mFileTypeString)/sizeof(mFileTypeString[0])) ? DriverInfo->FileType : 0]);
  Print (L"    CurrentUsage            - 0x%016lx\n", DriverInfo->CurrentUsage);
  Print (L"    PeakUsage               - 0x%016lx\n", DriverInfo->PeakUsage);
  for (TypeIndex = 0; TypeIndex < sizeof (DriverInfo->CurrentUsageByType) / sizeof (DriverInfo->CurrentUsageByType[0]); TypeIndex++) {
    if ((DriverInfo->CurrentUsageByType[TypeIndex] != 0) ||
        (DriverInfo->PeakUsageByType[TypeIndex] != 0)) {
      Print (L"    CurrentUsage[0x%02x]      - 0x%016lx (%s)\n", TypeIndex, DriverInfo->CurrentUsageByType[TypeIndex], mMemoryTypeString[TypeIndex]);
      Print (L"    PeakUsage[0x%02x]         - 0x%016lx (%s)\n", TypeIndex, DriverInfo->PeakUsageByType[TypeIndex], mMemoryTypeString[TypeIndex]);
    }
  }
  Print (L"    AllocRecordCount        - 0x%08x\n", DriverInfo->AllocRecordCount);

  AllocInfo = (MEMORY_PROFILE_ALLOC_INFO *) ((UINTN) DriverInfo + DriverInfo->Header.Length);
  for (AllocIndex = 0; AllocIndex < DriverInfo->AllocRecordCount; AllocIndex++) {
    AllocInfo = DumpMemoryProfileAllocInfo (DriverInfo, AllocIndex, AllocInfo);
    if (AllocInfo == NULL) {
      return NULL;
    }
  }
  return (MEMORY_PROFILE_DRIVER_INFO *) AllocInfo;
}

/**
  Dump memory profile context information.

  @param[in] Context            Pointer to memory profile context.

  @return Pointer to the end of memory profile context buffer.

**/
VOID *
DumpMemoryProfileContext (
  IN MEMORY_PROFILE_CONTEXT     *Context
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
      Print (L"  CurrentTotalUsage[0x%02x]       - 0x%016lx (%s)\n", TypeIndex, Context->CurrentTotalUsageByType[TypeIndex], mMemoryTypeString[TypeIndex]);
      Print (L"  PeakTotalUsage[0x%02x]          - 0x%016lx (%s)\n", TypeIndex, Context->PeakTotalUsageByType[TypeIndex], mMemoryTypeString[TypeIndex]);
    }
  }
  Print (L"  TotalImageSize                - 0x%016lx\n", Context->TotalImageSize);
  Print (L"  ImageCount                    - 0x%08x\n", Context->ImageCount);
  Print (L"  SequenceCount                 - 0x%08x\n", Context->SequenceCount);

  DriverInfo = (MEMORY_PROFILE_DRIVER_INFO *) ((UINTN) Context + Context->Header.Length);
  for (DriverIndex = 0; DriverIndex < Context->ImageCount; DriverIndex++) {
    DriverInfo = DumpMemoryProfileDriverInfo (DriverIndex, DriverInfo);
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
    CommonHeader = (MEMORY_PROFILE_COMMON_HEADER *) ((UINTN) CommonHeader + CommonHeader->Length);
  }

  return NULL;
}

/**
  Dump memory profile information.

  @param[in] ProfileBuffer      Memory profile base address.
  @param[in] ProfileSize        Memory profile size.

**/
VOID
DumpMemoryProfile (
  IN PHYSICAL_ADDRESS           ProfileBuffer,
  IN UINT64                     ProfileSize
  )
{
  MEMORY_PROFILE_CONTEXT        *Context;
  MEMORY_PROFILE_FREE_MEMORY    *FreeMemory;
  MEMORY_PROFILE_MEMORY_RANGE   *MemoryRange;

  Context = (MEMORY_PROFILE_CONTEXT *) ScanMemoryProfileBySignature (ProfileBuffer, ProfileSize, MEMORY_PROFILE_CONTEXT_SIGNATURE);
  if (Context != NULL) {
    DumpMemoryProfileContext (Context);
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
  Get and dump UEFI memory profile data.

  @return EFI_SUCCESS   Get the memory profile data successfully.
  @return other         Fail to get the memory profile data.

**/
EFI_STATUS
GetUefiMemoryProfileData (
  VOID
  )
{
  EFI_STATUS                    Status;
  EDKII_MEMORY_PROFILE_PROTOCOL *ProfileProtocol;
  VOID                          *Data;
  UINT64                        Size;

  Status = gBS->LocateProtocol (&gEdkiiMemoryProfileGuid, NULL, (VOID **) &ProfileProtocol);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "UefiMemoryProfile: Locate MemoryProfile protocol - %r\n", Status));
    return Status;
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
    return Status;
  }

  //
  // Add one sizeof (MEMORY_PROFILE_ALLOC_INFO) to Size for this AllocatePool action.
  //
  Size = Size + sizeof (MEMORY_PROFILE_ALLOC_INFO);
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
    FreePool (Data);
    Print (L"UefiMemoryProfile: GetData - %r\n", Status);
    return Status;
  }


  Print (L"UefiMemoryProfileSize - 0x%x\n", Size);
  Print (L"======= UefiMemoryProfile begin =======\n");
  DumpMemoryProfile ((PHYSICAL_ADDRESS) (UINTN) Data, Size);
  Print (L"======= UefiMemoryProfile end =======\n\n\n");

  FreePool (Data);

  return EFI_SUCCESS;
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
  SMRAM_PROFILE_PARAMETER_GET_PROFILE_DATA      *CommGetProfileData;
  UINT64                                        ProfileSize;
  PHYSICAL_ADDRESS                              ProfileBuffer;
  EFI_SMM_COMMUNICATION_PROTOCOL                *SmmCommunication;

  Status = gBS->LocateProtocol (&gEfiSmmCommunicationProtocolGuid, NULL, (VOID **) &SmmCommunication);
  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "SmramProfile: Locate SmmCommunication protocol - %r\n", Status));
    return Status;
  }

  CommSize = sizeof (EFI_GUID) + sizeof (UINTN) + sizeof (SMRAM_PROFILE_PARAMETER_GET_PROFILE_DATA);
  CommBuffer = AllocateZeroPool (CommSize);
  if (CommBuffer == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    Print (L"SmramProfile: AllocateZeroPool (0x%x) for comm buffer - %r\n", CommSize, Status);
    return Status;
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
  if (EFI_ERROR (Status)) {
    FreePool (CommBuffer);
    DEBUG ((EFI_D_ERROR, "SmramProfile: SmmCommunication - %r\n", Status));
    return Status;
  }

  if (CommGetProfileInfo->Header.ReturnStatus != 0) {
    Print (L"SmramProfile: GetProfileInfo - 0x%0x\n", CommGetProfileInfo->Header.ReturnStatus);
    return EFI_SUCCESS;
  }

  ProfileSize = CommGetProfileInfo->ProfileSize;

  //
  // Get Data
  //
  ProfileBuffer = (PHYSICAL_ADDRESS) (UINTN) AllocateZeroPool ((UINTN) ProfileSize);
  if (ProfileBuffer == 0) {
    FreePool (CommBuffer);
    Status = EFI_OUT_OF_RESOURCES;
    Print (L"SmramProfile: AllocateZeroPool (0x%x) for profile buffer - %r\n", (UINTN) ProfileSize, Status);
    return Status;
  }

  CommHeader = (EFI_SMM_COMMUNICATE_HEADER *) &CommBuffer[0];
  CopyMem (&CommHeader->HeaderGuid, &gEdkiiMemoryProfileGuid, sizeof(gEdkiiMemoryProfileGuid));
  CommHeader->MessageLength = sizeof (SMRAM_PROFILE_PARAMETER_GET_PROFILE_DATA);

  CommGetProfileData = (SMRAM_PROFILE_PARAMETER_GET_PROFILE_DATA *) &CommBuffer[OFFSET_OF (EFI_SMM_COMMUNICATE_HEADER, Data)];
  CommGetProfileData->Header.Command      = SMRAM_PROFILE_COMMAND_GET_PROFILE_DATA;
  CommGetProfileData->Header.DataLength   = sizeof (*CommGetProfileData);
  CommGetProfileData->Header.ReturnStatus = (UINT64)-1;
  CommGetProfileData->ProfileSize         = ProfileSize;
  CommGetProfileData->ProfileBuffer       = ProfileBuffer;

  CommSize = sizeof (EFI_GUID) + sizeof (UINTN) + CommHeader->MessageLength;
  Status = SmmCommunication->Communicate (SmmCommunication, CommBuffer, &CommSize);
  ASSERT_EFI_ERROR (Status);

  if (CommGetProfileData->Header.ReturnStatus != 0) {
    FreePool ((VOID *) (UINTN) CommGetProfileData->ProfileBuffer);
    FreePool (CommBuffer);
    Print (L"GetProfileData - 0x%x\n", CommGetProfileData->Header.ReturnStatus);
    return EFI_SUCCESS;
  }


  Print (L"SmramProfileSize - 0x%x\n", CommGetProfileData->ProfileSize);
  Print (L"======= SmramProfile begin =======\n");
  DumpMemoryProfile (CommGetProfileData->ProfileBuffer, CommGetProfileData->ProfileSize);
  Print (L"======= SmramProfile end =======\n\n\n");

  FreePool ((VOID *) (UINTN) CommGetProfileData->ProfileBuffer);
  FreePool (CommBuffer);

  return EFI_SUCCESS;
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
  EFI_STATUS                    Status;

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
