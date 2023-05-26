/** @file
  An internal header file for the Unit Test instance of the PEI services Table Pointer Library.

  Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef PEI_SERVICES_TABLE_POINTER_LIB_UNIT_TEST_H_
#define PEI_SERVICES_TABLE_POINTER_LIB_UNIT_TEST_H_

#include <Base.h>
#include <PiPei.h>
#include <Pi/PiMultiPhase.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/UnitTestLib.h>
#include <Library/PeiServicesTablePointerLib.h>
#include <Uefi.h>

#define MAX_PPI_COUNT  100
#define MAX_HOB_SIZE   SIZE_32MB

///
/// Forward declaration for PEI_CORE_INSTANCE
///
typedef struct _PEI_CORE_INSTANCE PEI_CORE_INSTANCE;

///
/// Pei Core private data structures
///
typedef union {
  EFI_PEI_PPI_DESCRIPTOR       *Ppi;
  EFI_PEI_NOTIFY_DESCRIPTOR    *Notify;
  VOID                         *Raw;
} PEI_PPI_LIST_POINTERS;

typedef struct {
  UINTN                    CurrentCount;
  UINTN                    MaxCount;
  UINTN                    LastDispatchedCount;
  ///
  /// MaxCount number of entries.
  ///
  PEI_PPI_LIST_POINTERS    PpiPtrs[MAX_PPI_COUNT];
} PEI_PPI_LIST;

typedef struct {
  UINTN                    CurrentCount;
  UINTN                    MaxCount;
  ///
  /// MaxCount number of entries.
  ///
  PEI_PPI_LIST_POINTERS    NotifyPtrs[MAX_PPI_COUNT];
} PEI_CALLBACK_NOTIFY_LIST;

typedef struct {
  UINTN                    CurrentCount;
  UINTN                    MaxCount;
  UINTN                    LastDispatchedCount;
  ///
  /// MaxCount number of entries.
  ///
  PEI_PPI_LIST_POINTERS    NotifyPtrs[MAX_PPI_COUNT];
} PEI_DISPATCH_NOTIFY_LIST;

///
/// PPI database structure which contains three links:
/// PpiList, CallbackNotifyList and DispatchNotifyList.
///
typedef struct {
  ///
  /// PPI List.
  ///
  PEI_PPI_LIST                PpiList;
  ///
  /// Notify List at dispatch level.
  ///
  PEI_CALLBACK_NOTIFY_LIST    CallbackNotifyList;
  ///
  /// Notify List at callback level.
  ///
  PEI_DISPATCH_NOTIFY_LIST    DispatchNotifyList;
} PEI_PPI_DATABASE;

///
/// Pei Core private data structure instance
///
struct _PEI_CORE_INSTANCE {
  PEI_PPI_DATABASE        PpiData;
  EFI_PEI_HOB_POINTERS    HobList;
};

extern PEI_CORE_INSTANCE  mPrivateData;
#define PEI_CORE_INSTANCE_FROM_PS_THIS(a)  &mPrivateData

VOID
ProcessNotify (
  IN PEI_CORE_INSTANCE  *PrivateData,
  IN UINTN              NotifyType,
  IN INTN               InstallStartIndex,
  IN INTN               InstallStopIndex,
  IN INTN               NotifyStartIndex,
  IN INTN               NotifyStopIndex
  );

/**

  This function installs an interface in the PEI PPI database by GUID.
  The purpose of the service is to publish an interface that other parties
  can use to call additional PEIMs.

  @param PeiServices                An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  @param PpiList                    Pointer to a list of PEI PPI Descriptors.

  @retval EFI_SUCCESS              if all PPIs in PpiList are successfully installed.
  @retval EFI_INVALID_PARAMETER    if PpiList is NULL pointer
                                   if any PPI in PpiList is not valid
  @retval EFI_OUT_OF_RESOURCES     if there is no more memory resource to install PPI

**/
EFI_STATUS
EFIAPI
UnitTestInstallPpi (
  IN CONST EFI_PEI_SERVICES        **PeiServices,
  IN CONST EFI_PEI_PPI_DESCRIPTOR  *PpiList
  );

/**

  This function reinstalls an interface in the PEI PPI database by GUID.
  The purpose of the service is to publish an interface that other parties can
  use to replace an interface of the same name in the protocol database with a
  different interface.

  @param PeiServices            An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  @param OldPpi                 Pointer to the old PEI PPI Descriptors.
  @param NewPpi                 Pointer to the new PEI PPI Descriptors.

  @retval EFI_SUCCESS           if the operation was successful
  @retval EFI_INVALID_PARAMETER if OldPpi or NewPpi is NULL
  @retval EFI_INVALID_PARAMETER if NewPpi is not valid
  @retval EFI_NOT_FOUND         if the PPI was not in the database

**/
EFI_STATUS
EFIAPI
UnitTestReInstallPpi (
  IN CONST EFI_PEI_SERVICES        **PeiServices,
  IN CONST EFI_PEI_PPI_DESCRIPTOR  *OldPpi,
  IN CONST EFI_PEI_PPI_DESCRIPTOR  *NewPpi
  );

/**

  Locate a given named PPI.


  @param PeiServices        An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  @param Guid               Pointer to GUID of the PPI.
  @param Instance           Instance Number to discover.
  @param PpiDescriptor      Pointer to reference the found descriptor. If not NULL,
                            returns a pointer to the descriptor (includes flags, etc)
  @param Ppi                Pointer to reference the found PPI

  @retval EFI_SUCCESS   if the PPI is in the database
  @retval EFI_NOT_FOUND if the PPI is not in the database

**/
EFI_STATUS
EFIAPI
UnitTestLocatePpi (
  IN CONST EFI_PEI_SERVICES      **PeiServices,
  IN CONST EFI_GUID              *Guid,
  IN UINTN                       Instance,
  IN OUT EFI_PEI_PPI_DESCRIPTOR  **PpiDescriptor,
  IN OUT VOID                    **Ppi
  );

/**

  This function installs a notification service to be called back when a given
  interface is installed or reinstalled. The purpose of the service is to publish
  an interface that other parties can use to call additional PPIs that may materialize later.

  @param PeiServices        An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  @param NotifyList         Pointer to list of Descriptors to notify upon.

  @retval EFI_SUCCESS           if successful
  @retval EFI_OUT_OF_RESOURCES  if no space in the database
  @retval EFI_INVALID_PARAMETER if not a good descriptor

**/
EFI_STATUS
EFIAPI
UnitTestNotifyPpi (
  IN CONST EFI_PEI_SERVICES           **PeiServices,
  IN CONST EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyList
  );

/**
  Gets the pointer to the HOB List.

  @param PeiServices                   An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  @param HobList                       Pointer to the HOB List.

  @retval EFI_SUCCESS                  Get the pointer of HOB List
  @retval EFI_NOT_AVAILABLE_YET        the HOB List is not yet published
  @retval EFI_INVALID_PARAMETER        HobList is NULL (in debug mode)

**/
EFI_STATUS
EFIAPI
UnitTestGetHobList (
  IN CONST EFI_PEI_SERVICES  **PeiServices,
  IN OUT VOID                **HobList
  );

/**
  Add a new HOB to the HOB List.

  @param PeiServices      An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  @param Type             Type of the new HOB.
  @param Length           Length of the new HOB to allocate.
  @param Hob              Pointer to the new HOB.

  @return  EFI_SUCCESS           Success to create HOB.
  @retval  EFI_INVALID_PARAMETER if Hob is NULL
  @retval  EFI_NOT_AVAILABLE_YET if HobList is still not available.
  @retval  EFI_OUT_OF_RESOURCES  if there is no more memory to grow the Hoblist.

**/
EFI_STATUS
EFIAPI
UnitTestCreateHob (
  IN CONST EFI_PEI_SERVICES  **PeiServices,
  IN UINT16                  Type,
  IN UINT16                  Length,
  IN OUT VOID                **Hob
  );

/**

  Builds a Handoff Information Table HOB

  @param BootMode        - Current Bootmode
  @param MemoryBegin     - Start Memory Address.
  @param MemoryLength    - Length of Memory.

  @return EFI_SUCCESS Always success to initialize HOB.

**/
EFI_STATUS
UnitTestCoreBuildHobHandoffInfoTable (
  IN EFI_BOOT_MODE         BootMode,
  IN EFI_PHYSICAL_ADDRESS  MemoryBegin,
  IN UINT64                MemoryLength
  );

/**
  Resets the entire platform.

  @param[in] ResetType      The type of reset to perform.
  @param[in] ResetStatus    The status code for the reset.
  @param[in] DataSize       The size, in bytes, of ResetData.
  @param[in] ResetData      For a ResetType of EfiResetCold, EfiResetWarm, or EfiResetShutdown
                            the data buffer starts with a Null-terminated string, optionally
                            followed by additional binary data. The string is a description
                            that the caller may use to further indicate the reason for the
                            system reset.

**/
VOID
EFIAPI
UnitTestResetSystem2 (
  IN EFI_RESET_TYPE  ResetType,
  IN EFI_STATUS      ResetStatus,
  IN UINTN           DataSize,
  IN VOID            *ResetData OPTIONAL
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

#endif
