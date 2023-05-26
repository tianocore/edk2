/** @file
  This file implements some PEI services.

  Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UnitTestPeiServicesTablePointerLib.h"

/**
  This service enables PEIMs to ascertain the present value of the boot mode.

  @param PeiServices            An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  @param BootMode               A pointer to contain the value of the boot mode.

  @retval EFI_SUCCESS           The boot mode was returned successfully.
  @retval EFI_INVALID_PARAMETER BootMode is NULL.

**/
EFI_STATUS
EFIAPI
UnitTestGetBootMode (
  IN  CONST EFI_PEI_SERVICES  **PeiServices,
  IN  OUT   EFI_BOOT_MODE     *BootMode
  )
{
  return EFI_NOT_AVAILABLE_YET;
}

/**
  This service enables PEIMs to update the boot mode variable.


  @param PeiServices      An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  @param BootMode         The value of the boot mode to set.

  @return EFI_SUCCESS     The value was successfully updated

**/
EFI_STATUS
EFIAPI
UnitTestSetBootMode (
  IN CONST EFI_PEI_SERVICES  **PeiServices,
  IN EFI_BOOT_MODE           BootMode
  )
{
  return EFI_NOT_AVAILABLE_YET;
}

/**
  Search the firmware volumes by index

  @param PeiServices     An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation
  @param Instance        This instance of the firmware volume to find. The value 0 is the Boot Firmware
                         Volume (BFV).
  @param VolumeHandle    On exit, points to the next volume handle or NULL if it does not exist.

  @retval EFI_INVALID_PARAMETER  VolumeHandle is NULL
  @retval EFI_NOT_FOUND          The volume was not found.
  @retval EFI_SUCCESS            The volume was found.

**/
EFI_STATUS
EFIAPI
UnitTestFfsFindNextVolume (
  IN CONST EFI_PEI_SERVICES  **PeiServices,
  IN     UINTN               Instance,
  IN OUT EFI_PEI_FV_HANDLE   *VolumeHandle
  )
{
  return EFI_NOT_AVAILABLE_YET;
}

/**
  Searches for the next matching file in the firmware volume.

  @param PeiServices     An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  @param SearchType      Filter to find only files of this type.
                         Type EFI_FV_FILETYPE_ALL causes no filtering to be done.
  @param FvHandle        Handle of firmware volume in which to search.
  @param FileHandle      On entry, points to the current handle from which to begin searching or NULL to start
                         at the beginning of the firmware volume. On exit, points the file handle of the next file
                         in the volume or NULL if there are no more files.

  @retval EFI_NOT_FOUND  The file was not found.
  @retval EFI_NOT_FOUND  The header checksum was not zero.
  @retval EFI_SUCCESS    The file was found.

**/
EFI_STATUS
EFIAPI
UnitTestFfsFindNextFile (
  IN CONST EFI_PEI_SERVICES   **PeiServices,
  IN UINT8                    SearchType,
  IN EFI_PEI_FV_HANDLE        FvHandle,
  IN OUT EFI_PEI_FILE_HANDLE  *FileHandle
  )
{
  return EFI_NOT_AVAILABLE_YET;
}

/**
  Searches for the next matching section within the specified file.

  @param PeiServices     An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation
  @param SectionType     Filter to find only sections of this type.
  @param FileHandle      Pointer to the current file to search.
  @param SectionData     A pointer to the discovered section, if successful.
                         NULL if section not found

  @retval EFI_NOT_FOUND  The section was not found.
  @retval EFI_SUCCESS    The section was found.

**/
EFI_STATUS
EFIAPI
UnitTestFfsFindSectionData (
  IN CONST EFI_PEI_SERVICES   **PeiServices,
  IN     EFI_SECTION_TYPE     SectionType,
  IN     EFI_PEI_FILE_HANDLE  FileHandle,
  OUT VOID                    **SectionData
  )
{
  return EFI_NOT_AVAILABLE_YET;
}

/**

  This function registers the found memory configuration with the PEI Foundation.

  The usage model is that the PEIM that discovers the permanent memory shall invoke this service.
  This routine will hold discoveried memory information into PeiCore's private data,
  and set SwitchStackSignal flag. After PEIM who discovery memory is dispatched,
  PeiDispatcher will migrate temporary memory to permanent memory.

  @param PeiServices        An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  @param MemoryBegin        Start of memory address.
  @param MemoryLength       Length of memory.

  @return EFI_SUCCESS Always success.

**/
EFI_STATUS
EFIAPI
UnitTestInstallPeiMemory (
  IN CONST EFI_PEI_SERVICES  **PeiServices,
  IN EFI_PHYSICAL_ADDRESS    MemoryBegin,
  IN UINT64                  MemoryLength
  )
{
  return EFI_NOT_AVAILABLE_YET;
}

/**
  The purpose of the service is to publish an interface that allows
  PEIMs to allocate memory ranges that are managed by the PEI Foundation.

  Prior to InstallPeiMemory() being called, PEI will allocate pages from the heap.
  After InstallPeiMemory() is called, PEI will allocate pages within the region
  of memory provided by InstallPeiMemory() service in a best-effort fashion.
  Location-specific allocations are not managed by the PEI foundation code.

  @param  PeiServices      An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  @param  MemoryType       The type of memory to allocate.
  @param  Pages            The number of contiguous 4 KB pages to allocate.
  @param  Memory           Pointer to a physical address. On output, the address is set to the base
                           of the page range that was allocated.

  @retval EFI_SUCCESS           The memory range was successfully allocated.
  @retval EFI_OUT_OF_RESOURCES  The pages could not be allocated.
  @retval EFI_INVALID_PARAMETER Type is not equal to EfiLoaderCode, EfiLoaderData, EfiRuntimeServicesCode,
                                EfiRuntimeServicesData, EfiBootServicesCode, EfiBootServicesData,
                                EfiACPIReclaimMemory, EfiReservedMemoryType, or EfiACPIMemoryNVS.

**/
EFI_STATUS
EFIAPI
UnitTestAllocatePages (
  IN CONST EFI_PEI_SERVICES      **PeiServices,
  IN       EFI_MEMORY_TYPE       MemoryType,
  IN       UINTN                 Pages,
  OUT      EFI_PHYSICAL_ADDRESS  *Memory
  )
{
  return EFI_NOT_AVAILABLE_YET;
}

/**

  Pool allocation service. Before permanent memory is discovered, the pool will
  be allocated in the heap in temporary memory. Generally, the size of the heap in temporary
  memory does not exceed 64K, so the biggest pool size could be allocated is
  64K.

  @param PeiServices               An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  @param Size                      Amount of memory required
  @param Buffer                    Address of pointer to the buffer

  @retval EFI_SUCCESS              The allocation was successful
  @retval EFI_OUT_OF_RESOURCES     There is not enough heap to satisfy the requirement
                                   to allocate the requested size.

**/
EFI_STATUS
EFIAPI
UnitTestAllocatePool (
  IN CONST EFI_PEI_SERVICES  **PeiServices,
  IN       UINTN             Size,
  OUT      VOID              **Buffer
  )
{
  return EFI_NOT_AVAILABLE_YET;
}

/**

  Core version of the Status Code reporter


  @param PeiServices     An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  @param CodeType        Type of Status Code.
  @param Value           Value to output for Status Code.
  @param Instance        Instance Number of this status code.
  @param CallerId        ID of the caller of this status code.
  @param Data            Optional data associated with this status code.

  @retval EFI_SUCCESS             if status code is successfully reported
  @retval EFI_NOT_AVAILABLE_YET   if StatusCodePpi has not been installed

**/
EFI_STATUS
EFIAPI
UnitTestReportStatusCode (
  IN CONST EFI_PEI_SERVICES      **PeiServices,
  IN EFI_STATUS_CODE_TYPE        CodeType,
  IN EFI_STATUS_CODE_VALUE       Value,
  IN UINT32                      Instance,
  IN CONST EFI_GUID              *CallerId,
  IN CONST EFI_STATUS_CODE_DATA  *Data OPTIONAL
  )
{
  return EFI_NOT_AVAILABLE_YET;
}

/**

Core version of the Reset System


@param PeiServices                An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.

@retval EFI_NOT_AVAILABLE_YET     PPI not available yet.
@retval EFI_DEVICE_ERROR          Did not reset system.
                                  Otherwise, resets the system.

**/
EFI_STATUS
EFIAPI
UnitTestResetSystem (
  IN CONST EFI_PEI_SERVICES  **PeiServices
  )
{
  return EFI_NOT_AVAILABLE_YET;
}

/**
  Find a file within a volume by its name.

  @param FileName        A pointer to the name of the file to find within the firmware volume.
  @param VolumeHandle    The firmware volume to search
  @param FileHandle      Upon exit, points to the found file's handle
                         or NULL if it could not be found.

  @retval EFI_SUCCESS            File was found.
  @retval EFI_NOT_FOUND          File was not found.
  @retval EFI_INVALID_PARAMETER  VolumeHandle or FileHandle or FileName was NULL.

**/
EFI_STATUS
EFIAPI
UnitTestFfsFindFileByName (
  IN  CONST EFI_GUID       *FileName,
  IN  EFI_PEI_FV_HANDLE    VolumeHandle,
  OUT EFI_PEI_FILE_HANDLE  *FileHandle
  )
{
  return EFI_NOT_AVAILABLE_YET;
}

/**
  Returns information about a specific file.

  @param FileHandle       Handle of the file.
  @param FileInfo         Upon exit, points to the file's information.

  @retval EFI_INVALID_PARAMETER If FileInfo is NULL.
  @retval EFI_INVALID_PARAMETER If FileHandle does not represent a valid file.
  @retval EFI_SUCCESS           File information returned.

**/
EFI_STATUS
EFIAPI
UnitTestFfsGetFileInfo (
  IN EFI_PEI_FILE_HANDLE  FileHandle,
  OUT EFI_FV_FILE_INFO    *FileInfo
  )
{
  return EFI_NOT_AVAILABLE_YET;
}

/**
  Returns information about the specified volume.

  This function returns information about a specific firmware
  volume, including its name, type, attributes, starting address
  and size.

  @param VolumeHandle   Handle of the volume.
  @param VolumeInfo     Upon exit, points to the volume's information.

  @retval EFI_SUCCESS             Volume information returned.
  @retval EFI_INVALID_PARAMETER   If VolumeHandle does not represent a valid volume.
  @retval EFI_INVALID_PARAMETER   If VolumeHandle is NULL.
  @retval EFI_SUCCESS             Information successfully returned.
  @retval EFI_INVALID_PARAMETER   The volume designated by the VolumeHandle is not available.

**/
EFI_STATUS
EFIAPI
UnitTestFfsGetVolumeInfo (
  IN EFI_PEI_FV_HANDLE  VolumeHandle,
  OUT EFI_FV_INFO       *VolumeInfo
  )
{
  return EFI_NOT_AVAILABLE_YET;
}

/**
This routine enables a PEIM to register itself for shadow when the PEI Foundation
discovers permanent memory.

@param FileHandle             File handle of a PEIM.

@retval EFI_NOT_FOUND         The file handle doesn't point to PEIM itself.
@retval EFI_ALREADY_STARTED   Indicate that the PEIM has been registered itself.
@retval EFI_SUCCESS           Successfully to register itself.

**/
EFI_STATUS
EFIAPI
UnitTestRegisterForShadow (
  IN EFI_PEI_FILE_HANDLE  FileHandle
  )
{
  return EFI_NOT_AVAILABLE_YET;
}

/**
Searches for the next matching section within the specified file.

@param  PeiServices           An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
@param  SectionType           The value of the section type to find.
@param  SectionInstance       Section instance to find.
@param  FileHandle            Handle of the firmware file to search.
@param  SectionData           A pointer to the discovered section, if successful.
@param  AuthenticationStatus  A pointer to the authentication status for this section.

@retval EFI_SUCCESS      The section was found.
@retval EFI_NOT_FOUND    The section was not found.

**/
EFI_STATUS
EFIAPI
UnitTestFfsFindSectionData3 (
  IN CONST EFI_PEI_SERVICES   **PeiServices,
  IN     EFI_SECTION_TYPE     SectionType,
  IN     UINTN                SectionInstance,
  IN     EFI_PEI_FILE_HANDLE  FileHandle,
  OUT VOID                    **SectionData,
  OUT UINT32                  *AuthenticationStatus
  )
{
  return EFI_NOT_AVAILABLE_YET;
}

/**
Returns information about a specific file.

@param FileHandle       Handle of the file.
@param FileInfo         Upon exit, points to the file's information.

@retval EFI_INVALID_PARAMETER If FileInfo is NULL.
@retval EFI_INVALID_PARAMETER If FileHandle does not represent a valid file.
@retval EFI_SUCCESS           File information returned.

**/
EFI_STATUS
EFIAPI
UnitTestFfsGetFileInfo2 (
  IN EFI_PEI_FILE_HANDLE  FileHandle,
  OUT EFI_FV_FILE_INFO2   *FileInfo
  )
{
  return EFI_NOT_AVAILABLE_YET;
}

/**
Frees memory pages.

@param[in] PeiServices        An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
@param[in] Memory             The base physical address of the pages to be freed.
@param[in] Pages              The number of contiguous 4 KB pages to free.

@retval EFI_SUCCESS           The requested pages were freed.
@retval EFI_INVALID_PARAMETER Memory is not a page-aligned address or Pages is invalid.
@retval EFI_NOT_FOUND         The requested memory pages were not allocated with
                              AllocatePages().

**/
EFI_STATUS
EFIAPI
UnitTestFreePages (
  IN CONST EFI_PEI_SERVICES  **PeiServices,
  IN EFI_PHYSICAL_ADDRESS    Memory,
  IN UINTN                   Pages
  )
{
  return EFI_NOT_AVAILABLE_YET;
}
