/** @file
  Library that helps implement monolithic PEI. (SEC goes to DXE)

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __PRE_PI_LIB_H__
#define __PRE_PI_LIB_H__

#include <Guid/ExtractSection.h>

/**
  This service enables discovery of additional firmware volumes.

  @param  Instance              This instance of the firmware volume to find.  The value 0 is the
                                Boot Firmware Volume (BFV).
  @param  FwVolHeader           Pointer to the firmware volume header of the volume to return.

  @retval EFI_SUCCESS           The volume was found.
  @retval EFI_NOT_FOUND         The volume was not found.
  @retval EFI_INVALID_PARAMETER FwVolHeader is NULL.

**/
EFI_STATUS
EFIAPI
FfsFindNextVolume (
  IN UINTN                          Instance,
  IN OUT EFI_PEI_FV_HANDLE          *VolumeHandle
  );


/**
  This service enables discovery of additional firmware files.

  @param  SearchType            A filter to find files only of this type.
  @param  FwVolHeader           Pointer to the firmware volume header of the volume to search.
                                This parameter must point to a valid FFS volume.
  @param  FileHeader            Pointer to the current file from which to begin searching.

  @retval EFI_SUCCESS           The file was found.
  @retval EFI_NOT_FOUND         The file was not found.
  @retval EFI_NOT_FOUND         The header checksum was not zero.

**/
EFI_STATUS
EFIAPI
FfsFindNextFile (
  IN EFI_FV_FILETYPE            SearchType,
  IN EFI_PEI_FV_HANDLE          VolumeHandle,
  IN OUT EFI_PEI_FILE_HANDLE    *FileHandle
  );


/**
  This service enables discovery sections of a given type within a valid FFS file.

  @param  SearchType            The value of the section type to find.
  @param  FfsFileHeader         A pointer to the file header that contains the set of sections to
                                be searched.
  @param  SectionData           A pointer to the discovered section, if successful.

  @retval EFI_SUCCESS           The section was found.
  @retval EFI_NOT_FOUND         The section was not found.

**/
EFI_STATUS
EFIAPI
FfsFindSectionData (
  IN EFI_SECTION_TYPE           SectionType,
  IN EFI_PEI_FILE_HANDLE        FileHandle,
  OUT VOID                      **SectionData
  );


/**
  Find a file in the volume by name

  @param FileName       A pointer to the name of the file to
                        find within the firmware volume.

  @param VolumeHandle   The firmware volume to search FileHandle
                        Upon exit, points to the found file's
                        handle or NULL if it could not be found.

  @retval EFI_SUCCESS             File was found.

  @retval EFI_NOT_FOUND           File was not found.

  @retval EFI_INVALID_PARAMETER   VolumeHandle or FileHandle or
                                  FileName was NULL.

**/
EFI_STATUS
EFIAPI
FfsFindFileByName (
  IN CONST  EFI_GUID            *FileName,
  IN CONST  EFI_PEI_FV_HANDLE   VolumeHandle,
  OUT       EFI_PEI_FILE_HANDLE *FileHandle
  );


/**
  Get information about the file by name.

  @param FileHandle   Handle of the file.

  @param FileInfo     Upon exit, points to the file's
                      information.

  @retval EFI_SUCCESS             File information returned.

  @retval EFI_INVALID_PARAMETER   If FileHandle does not
                                  represent a valid file.

  @retval EFI_INVALID_PARAMETER   If FileInfo is NULL.

**/
EFI_STATUS
EFIAPI
FfsGetFileInfo (
  IN CONST  EFI_PEI_FILE_HANDLE   FileHandle,
  OUT EFI_FV_FILE_INFO            *FileInfo
  );


/**
  Get Information about the volume by name

  @param VolumeHandle   Handle of the volume.

  @param VolumeInfo     Upon exit, points to the volume's
                        information.

  @retval EFI_SUCCESS             File information returned.

  @retval EFI_INVALID_PARAMETER   If FileHandle does not
                                  represent a valid file.

  @retval EFI_INVALID_PARAMETER   If FileInfo is NULL.

**/
EFI_STATUS
EFIAPI
FfsGetVolumeInfo (
  IN  EFI_PEI_FV_HANDLE       VolumeHandle,
  OUT EFI_FV_INFO             *VolumeInfo
  );



/**
  Get Fv image from the FV type file, then add FV & FV2 Hob.

  @param FileHandle      File handle of a Fv type file.

  @retval EFI_NOT_FOUND  FV image can't be found.
  @retval EFI_SUCCESS    Successfully to process it.

**/
EFI_STATUS
EFIAPI
FfsProcessFvFile (
  IN  EFI_PEI_FILE_HANDLE   FvFileHandle
  );


/**
  Search through every FV until you find a file of type FileType

  @param FileType        File handle of a Fv type file.
  @param Volumehandle    On succes Volume Handle of the match
  @param FileHandle      On success File Handle of the match

  @retval EFI_NOT_FOUND  FV image can't be found.
  @retval EFI_SUCCESS    Successfully found FileType

**/
EFI_STATUS
EFIAPI
FfsAnyFvFindFirstFile (
  IN  EFI_FV_FILETYPE       FileType,
  OUT EFI_PEI_FV_HANDLE     *VolumeHandle,
  OUT EFI_PEI_FILE_HANDLE   *FileHandle
  );


/**
  Get Fv image from the FV type file, then add FV & FV2 Hob.

  @param FileHandle  File handle of a Fv type file.


  @retval EFI_NOT_FOUND  FV image can't be found.
  @retval EFI_SUCCESS    Successfully to process it.

**/
EFI_STATUS
EFIAPI
FfsProcessFvFile (
  IN  EFI_PEI_FILE_HANDLE   FvFileHandle
  );


/**
  This service enables PEIMs to ascertain the present value of the boot mode.


  @retval BootMode

**/
EFI_BOOT_MODE
EFIAPI
GetBootMode (
  VOID
  );


/**
  This service enables PEIMs to update the boot mode variable.

  @param  BootMode              The value of the boot mode to set.

  @retval EFI_SUCCESS           The value was successfully updated

**/
EFI_STATUS
EFIAPI
SetBootMode (
  IN EFI_BOOT_MODE              BootMode
  );

/**
  This service enables a PEIM to ascertain the address of the list of HOBs in memory.

  @param  HobList               A pointer to the list of HOBs that the PEI Foundation will initialize.

  @retval EFI_SUCCESS           The list was successfully returned.
  @retval EFI_NOT_AVAILABLE_YET The HOB list is not yet published.

**/
VOID *
EFIAPI
GetHobList (
  VOID
  );


/**
  Updates the pointer to the HOB list.

  @param  HobList       Hob list pointer to store

**/
EFI_STATUS
EFIAPI
SetHobList (
  IN  VOID      *HobList
  );

EFI_HOB_HANDOFF_INFO_TABLE*
HobConstructor (
  IN VOID   *EfiMemoryBegin,
  IN UINTN  EfiMemoryLength,
  IN VOID   *EfiFreeMemoryBottom,
  IN VOID   *EfiFreeMemoryTop
  );

/**
  This service enables PEIMs to create various types of HOBs.

  @param  Type                  The type of HOB to be installed.
  @param  Length                The length of the HOB to be added.

  @retval !NULL                 The HOB was successfully created.
  @retval NULL                  There is no additional space for HOB creation.

**/
VOID *
CreateHob (
  IN  UINT16    HobType,
  IN  UINT16    HobLenght
  );


/**
  Returns the next instance of a HOB type from the starting HOB.

  This function searches the first instance of a HOB type from the starting HOB pointer.
  If there does not exist such HOB type from the starting HOB pointer, it will return NULL.
  In contrast with macro GET_NEXT_HOB(), this function does not skip the starting HOB pointer
  unconditionally: it returns HobStart back if HobStart itself meets the requirement;
  caller is required to use GET_NEXT_HOB() if it wishes to skip current HobStart.
  If HobStart is NULL, then ASSERT().

  @param  Type          The HOB type to return.
  @param  HobStart      The starting HOB pointer to search from.

  @return The next instance of a HOB type from the starting HOB.

**/
VOID *
EFIAPI
GetNextHob (
  IN UINT16                 Type,
  IN CONST VOID             *HobStart
  );

/**
  Returns the first instance of a HOB type among the whole HOB list.

  This function searches the first instance of a HOB type among the whole HOB list.
  If there does not exist such HOB type in the HOB list, it will return NULL.

  @param  Type          The HOB type to return.

  @return The next instance of a HOB type from the starting HOB.

**/
VOID *
EFIAPI
GetFirstHob (
  IN UINT16                 Type
  );

/**
  This function searches the first instance of a HOB from the starting HOB pointer.
  Such HOB should satisfy two conditions:
  its HOB type is EFI_HOB_TYPE_GUID_EXTENSION and its GUID Name equals to the input Guid.
  If there does not exist such HOB from the starting HOB pointer, it will return NULL.
  Caller is required to apply GET_GUID_HOB_DATA () and GET_GUID_HOB_DATA_SIZE ()
  to extract the data section and its size info respectively.
  In contrast with macro GET_NEXT_HOB(), this function does not skip the starting HOB pointer
  unconditionally: it returns HobStart back if HobStart itself meets the requirement;
  caller is required to use GET_NEXT_HOB() if it wishes to skip current HobStart.
  If Guid is NULL, then ASSERT().
  If HobStart is NULL, then ASSERT().

  @param  Guid          The GUID to match with in the HOB list.
  @param  HobStart      A pointer to a Guid.

  @return The next instance of the matched GUID HOB from the starting HOB.

**/
VOID *
EFIAPI
GetNextGuidHob (
  IN CONST EFI_GUID         *Guid,
  IN CONST VOID             *HobStart
  );

/**
  This function searches the first instance of a HOB among the whole HOB list.
  Such HOB should satisfy two conditions:
  its HOB type is EFI_HOB_TYPE_GUID_EXTENSION and its GUID Name equals to the input Guid.
  If there does not exist such HOB from the starting HOB pointer, it will return NULL.
  Caller is required to apply GET_GUID_HOB_DATA () and GET_GUID_HOB_DATA_SIZE ()
  to extract the data section and its size info respectively.
  If Guid is NULL, then ASSERT().

  @param  Guid          The GUID to match with in the HOB list.

  @return The first instance of the matched GUID HOB among the whole HOB list.

**/
VOID *
EFIAPI
GetFirstGuidHob (
  IN CONST EFI_GUID         *Guid
  );


/**
  Builds a HOB for a loaded PE32 module.

  This function builds a HOB for a loaded PE32 module.
  It can only be invoked during PEI phase;
  for DXE phase, it will ASSERT() since PEI HOB is read-only for DXE phase.
  If ModuleName is NULL, then ASSERT().
  If there is no additional space for HOB creation, then ASSERT().

  @param  ModuleName              The GUID File Name of the module.
  @param  MemoryAllocationModule  The 64 bit physical address of the module.
  @param  ModuleLength            The length of the module in bytes.
  @param  EntryPoint              The 64 bit physical address of the module entry point.

**/
VOID
EFIAPI
BuildModuleHob (
  IN CONST EFI_GUID         *ModuleName,
  IN EFI_PHYSICAL_ADDRESS   MemoryAllocationModule,
  IN UINT64                 ModuleLength,
  IN EFI_PHYSICAL_ADDRESS   EntryPoint
  );

/**
  Builds a HOB that describes a chunk of system memory.

  This function builds a HOB that describes a chunk of system memory.
  It can only be invoked during PEI phase;
  for DXE phase, it will ASSERT() since PEI HOB is read-only for DXE phase.
  If there is no additional space for HOB creation, then ASSERT().

  @param  ResourceType        The type of resource described by this HOB.
  @param  ResourceAttribute   The resource attributes of the memory described by this HOB.
  @param  PhysicalStart       The 64 bit physical address of memory described by this HOB.
  @param  NumberOfBytes       The length of the memory described by this HOB in bytes.

**/
VOID
EFIAPI
BuildResourceDescriptorHob (
  IN EFI_RESOURCE_TYPE            ResourceType,
  IN EFI_RESOURCE_ATTRIBUTE_TYPE  ResourceAttribute,
  IN EFI_PHYSICAL_ADDRESS         PhysicalStart,
  IN UINT64                       NumberOfBytes
  );

/**
  Builds a GUID HOB with a certain data length.

  This function builds a customized HOB tagged with a GUID for identification
  and returns the start address of GUID HOB data so that caller can fill the customized data.
  The HOB Header and Name field is already stripped.
  It can only be invoked during PEI phase;
  for DXE phase, it will ASSERT() since PEI HOB is read-only for DXE phase.
  If Guid is NULL, then ASSERT().
  If there is no additional space for HOB creation, then ASSERT().
  If DataLength >= (0x10000 - sizeof (EFI_HOB_GUID_TYPE)), then ASSERT().

  @param  Guid          The GUID to tag the customized HOB.
  @param  DataLength    The size of the data payload for the GUID HOB.

  @return The start address of GUID HOB data.

**/
VOID *
EFIAPI
BuildGuidHob (
  IN CONST EFI_GUID              *Guid,
  IN UINTN                       DataLength
  );

/**
  Copies a data buffer to a newly-built HOB.

  This function builds a customized HOB tagged with a GUID for identification,
  copies the input data to the HOB data field and returns the start address of the GUID HOB data.
  The HOB Header and Name field is already stripped.
  It can only be invoked during PEI phase;
  for DXE phase, it will ASSERT() since PEI HOB is read-only for DXE phase.
  If Guid is NULL, then ASSERT().
  If Data is NULL and DataLength > 0, then ASSERT().
  If there is no additional space for HOB creation, then ASSERT().
  If DataLength >= (0x10000 - sizeof (EFI_HOB_GUID_TYPE)), then ASSERT().

  @param  Guid          The GUID to tag the customized HOB.
  @param  Data          The data to be copied into the data field of the GUID HOB.
  @param  DataLength    The size of the data payload for the GUID HOB.

  @return The start address of GUID HOB data.

**/
VOID *
EFIAPI
BuildGuidDataHob (
  IN CONST EFI_GUID              *Guid,
  IN VOID                        *Data,
  IN UINTN                       DataLength
  );

/**
  Builds a Firmware Volume HOB.

  This function builds a Firmware Volume HOB.
  It can only be invoked during PEI phase;
  for DXE phase, it will ASSERT() since PEI HOB is read-only for DXE phase.
  If there is no additional space for HOB creation, then ASSERT().

  @param  BaseAddress   The base address of the Firmware Volume.
  @param  Length        The size of the Firmware Volume in bytes.

**/
VOID
EFIAPI
BuildFvHob (
  IN EFI_PHYSICAL_ADDRESS        BaseAddress,
  IN UINT64                      Length
  );

/**
  Builds a Firmware Volume HOB and a resrouce descriptor hob

  This function builds a Firmware Volume HOB.
  It can only be invoked during PEI phase;
  for DXE phase, it will ASSERT() since PEI HOB is read-only for DXE phase.
  If there is no additional space for HOB creation, then ASSERT().

  @param  BaseAddress   The base address of the Firmware Volume.
  @param  Length        The size of the Firmware Volume in bytes.

**/
VOID
EFIAPI
BuildFvHobs (
  IN EFI_PHYSICAL_ADDRESS         PhysicalStart,
  IN UINT64                       NumberOfBytes,
  IN EFI_RESOURCE_ATTRIBUTE_TYPE  *ResourceAttribute  OPTIONAL
  );


/**
  Builds a EFI_HOB_TYPE_FV2 HOB.

  This function builds a EFI_HOB_TYPE_FV2 HOB.
  It can only be invoked during PEI phase;
  for DXE phase, it will ASSERT() since PEI HOB is read-only for DXE phase.
  If there is no additional space for HOB creation, then ASSERT().

  @param  BaseAddress   The base address of the Firmware Volume.
  @param  Length        The size of the Firmware Volume in bytes.
  @param  FvName       The name of the Firmware Volume.
  @param  FileName      The name of the file.

**/
VOID
EFIAPI
BuildFv2Hob (
  IN          EFI_PHYSICAL_ADDRESS        BaseAddress,
  IN          UINT64                      Length,
  IN CONST    EFI_GUID                    *FvName,
  IN CONST    EFI_GUID                    *FileName
  );

/**
  Builds a Capsule Volume HOB.

  This function builds a Capsule Volume HOB.
  It can only be invoked during PEI phase;
  for DXE phase, it will ASSERT() since PEI HOB is read-only for DXE phase.
  If there is no additional space for HOB creation, then ASSERT().

  @param  BaseAddress   The base address of the Capsule Volume.
  @param  Length        The size of the Capsule Volume in bytes.

**/
VOID
EFIAPI
BuildCvHob (
  IN EFI_PHYSICAL_ADDRESS        BaseAddress,
  IN UINT64                      Length
  );

/**
  Builds a HOB for the CPU.

  This function builds a HOB for the CPU.
  It can only be invoked during PEI phase;
  for DXE phase, it will ASSERT() since PEI HOB is read-only for DXE phase.
  If there is no additional space for HOB creation, then ASSERT().

  @param  SizeOfMemorySpace   The maximum physical memory addressability of the processor.
  @param  SizeOfIoSpace       The maximum physical I/O addressability of the processor.

**/
VOID
EFIAPI
BuildCpuHob (
  IN UINT8                       SizeOfMemorySpace,
  IN UINT8                       SizeOfIoSpace
  );

/**
  Builds a HOB for the Stack.

  This function builds a HOB for the stack.
  It can only be invoked during PEI phase;
  for DXE phase, it will ASSERT() since PEI HOB is read-only for DXE phase.
  If there is no additional space for HOB creation, then ASSERT().

  @param  BaseAddress   The 64 bit physical address of the Stack.
  @param  Length        The length of the stack in bytes.

**/
VOID
EFIAPI
BuildStackHob (
  IN EFI_PHYSICAL_ADDRESS        BaseAddress,
  IN UINT64                      Length
  );

/**
  Update the Stack Hob if the stack has been moved

  @param  BaseAddress   The 64 bit physical address of the Stack.
  @param  Length        The length of the stack in bytes.

**/
VOID
UpdateStackHob (
  IN EFI_PHYSICAL_ADDRESS        BaseAddress,
  IN UINT64                      Length
  );


/**
  Builds a HOB for the BSP store.

  This function builds a HOB for BSP store.
  It can only be invoked during PEI phase;
  for DXE phase, it will ASSERT() since PEI HOB is read-only for DXE phase.
  If there is no additional space for HOB creation, then ASSERT().

  @param  BaseAddress   The 64 bit physical address of the BSP.
  @param  Length        The length of the BSP store in bytes.
  @param  MemoryType    Type of memory allocated by this HOB.

**/
VOID
EFIAPI
BuildBspStoreHob (
  IN EFI_PHYSICAL_ADDRESS        BaseAddress,
  IN UINT64                      Length,
  IN EFI_MEMORY_TYPE             MemoryType
  );

/**
  Builds a HOB for the memory allocation.

  This function builds a HOB for the memory allocation.
  It can only be invoked during PEI phase;
  for DXE phase, it will ASSERT() since PEI HOB is read-only for DXE phase.
  If there is no additional space for HOB creation, then ASSERT().

  @param  BaseAddress   The 64 bit physical address of the memory.
  @param  Length        The length of the memory allocation in bytes.
  @param  MemoryType    Type of memory allocated by this HOB.

**/
VOID
EFIAPI
BuildMemoryAllocationHob (
  IN EFI_PHYSICAL_ADDRESS        BaseAddress,
  IN UINT64                      Length,
  IN EFI_MEMORY_TYPE             MemoryType
  );


VOID
EFIAPI
BuildExtractSectionHob (
  IN  EFI_GUID                                  *Guid,
  IN  EXTRACT_GUIDED_SECTION_GET_INFO_HANDLER   SectionGetInfo,
  IN  EXTRACT_GUIDED_SECTION_DECODE_HANDLER     SectionExtraction
  );

VOID
EFIAPI
BuildPeCoffLoaderHob (
  VOID
  );


/**
  Allocates one or more 4KB pages of type EfiBootServicesData.

  Allocates the number of 4KB pages of MemoryType and returns a pointer to the
  allocated buffer.  The buffer returned is aligned on a 4KB boundary.  If Pages is 0, then NULL
  is returned.  If there is not enough memory remaining to satisfy the request, then NULL is
  returned.

  @param  Pages                 The number of 4 KB pages to allocate.

  @return A pointer to the allocated buffer or NULL if allocation fails.

**/
VOID *
EFIAPI
AllocatePages (
  IN UINTN            Pages
  );

/**
  Allocates a buffer of type EfiBootServicesData.

  Allocates the number bytes specified by AllocationSize of type EfiBootServicesData and returns a
  pointer to the allocated buffer.  If AllocationSize is 0, then a valid buffer of 0 size is
  returned.  If there is not enough memory remaining to satisfy the request, then NULL is returned.

  @param  AllocationSize        The number of bytes to allocate.

  @return A pointer to the allocated buffer or NULL if allocation fails.

**/
VOID *
EFIAPI
AllocatePool (
  IN UINTN  AllocationSize
  );


/**
  Allocates one or more 4KB pages of type EfiBootServicesData at a specified alignment.

  Allocates the number of 4KB pages specified by Pages of type EfiBootServicesData with an
  alignment specified by Alignment.  The allocated buffer is returned.  If Pages is 0, then NULL is
  returned.  If there is not enough memory at the specified alignment remaining to satisfy the
  request, then NULL is returned.
  If Alignment is not a power of two and Alignment is not zero, then ASSERT().

  @param  Pages                 The number of 4 KB pages to allocate.
  @param  Alignment             The requested alignment of the allocation.  Must be a power of two.
                                If Alignment is zero, then byte alignment is used.

  @return A pointer to the allocated buffer or NULL if allocation fails.

**/
VOID *
EFIAPI
AllocateAlignedPages (
  IN UINTN  Pages,
  IN UINTN  Alignment
  );


EFI_STATUS
EFIAPI
LoadPeCoffImage (
  IN  VOID                                      *PeCoffImage,
  OUT EFI_PHYSICAL_ADDRESS                      *ImageAddress,
  OUT UINT64                                    *ImageSize,
  OUT EFI_PHYSICAL_ADDRESS                      *EntryPoint
  );

EFI_STATUS
EFIAPI
LoadDxeCoreFromFfsFile (
  IN EFI_PEI_FILE_HANDLE  FileHandle,
  IN UINTN                StackSize
  );

EFI_STATUS
EFIAPI
LoadDxeCoreFromFv (
  IN UINTN  *FvInstance,   OPTIONAL
  IN UINTN  StackSize
  );

EFI_STATUS
EFIAPI
DecompressFirstFv (
  VOID
  );

VOID
EFIAPI
AddDxeCoreReportStatusCodeCallback (
  VOID
  );


#endif
