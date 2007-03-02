/*++

Copyright (c) 2006 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  PeiMain.h

Abstract:

  Definition of Pei Core Structures and Services

Revision History

--*/

#ifndef _PEI_MAIN_H_
#define _PEI_MAIN_H_

extern EFI_GUID gEfiPeiCorePrivateGuid;

//
// Pei Core private data structures
//
typedef union {
  EFI_PEI_PPI_DESCRIPTOR      *Ppi;
  EFI_PEI_NOTIFY_DESCRIPTOR   *Notify;
  VOID                        *Raw;
} PEI_PPI_LIST_POINTERS;

#define PEI_STACK_SIZE 0x20000

#define MAX_PPI_DESCRIPTORS 64

typedef struct {
  INTN                    PpiListEnd;
  INTN                    NotifyListEnd;
  INTN                    DispatchListEnd;
  INTN                    LastDispatchedInstall;
  INTN                    LastDispatchedNotify;
  PEI_PPI_LIST_POINTERS   PpiListPtrs[MAX_PPI_DESCRIPTORS];
} PEI_PPI_DATABASE;

typedef struct {
  UINT8                       CurrentPeim;
  UINT8                       CurrentFv;
  UINT32                      DispatchedPeimBitMap;
  UINT32                      PreviousPeimBitMap;
  EFI_FFS_FILE_HEADER         *CurrentPeimAddress;
  EFI_FIRMWARE_VOLUME_HEADER  *CurrentFvAddress;
  EFI_FIRMWARE_VOLUME_HEADER  *BootFvAddress;
  EFI_PEI_FIND_FV_PPI         *FindFv;
} PEI_CORE_DISPATCH_DATA;


//
// Pei Core private data structure instance
//

#define PEI_CORE_HANDLE_SIGNATURE  EFI_SIGNATURE_32('P','e','i','C')

typedef struct{
  UINTN                              Signature;
  EFI_PEI_SERVICES                   *PS;     // Point to ServiceTableShadow
  PEI_PPI_DATABASE                   PpiData;
  PEI_CORE_DISPATCH_DATA             DispatchData;
  EFI_PEI_HOB_POINTERS               HobList;
  BOOLEAN                            SwitchStackSignal;
  BOOLEAN                            PeiMemoryInstalled;
  EFI_PHYSICAL_ADDRESS               StackBase;
  UINT64                             StackSize;
  VOID                               *BottomOfCarHeap;
  VOID                               *TopOfCarHeap;
  VOID                               *CpuIo;
  EFI_PEI_SECURITY_PPI               *PrivateSecurityPpi;
  EFI_PEI_SERVICES                   ServiceTableShadow;
  UINTN                              SizeOfCacheAsRam;
  VOID                               *MaxTopOfCarHeap;
} PEI_CORE_INSTANCE;

//
// Pei Core Instance Data Macros
//

#define PEI_CORE_INSTANCE_FROM_PS_THIS(a) \
  CR(a, PEI_CORE_INSTANCE, PS, PEI_CORE_HANDLE_SIGNATURE)

//
// BUGBUG: Where does this go really?
//
typedef
EFI_STATUS
(EFIAPI *PEI_CORE_ENTRY_POINT)(
  IN EFI_PEI_STARTUP_DESCRIPTOR  *PeiStartupDescriptor,
  IN PEI_CORE_INSTANCE           *OldCoreData
  );

//
// Union of temporarily used function pointers (to save stack space)
//
typedef union {
  PEI_CORE_ENTRY_POINT         PeiCore;
  EFI_PEIM_ENTRY_POINT         PeimEntry;
  EFI_PEIM_NOTIFY_ENTRY_POINT  PeimNotifyEntry;
  EFI_DXE_IPL_PPI              *DxeIpl;
  EFI_PEI_PPI_DESCRIPTOR       *PpiDescriptor;
  EFI_PEI_NOTIFY_DESCRIPTOR    *NotifyDescriptor;
  VOID                         *Raw;
} PEI_CORE_TEMP_POINTERS;


//
// PeiCore function
//
EFI_STATUS
EFIAPI
PeiCore (
  IN EFI_PEI_STARTUP_DESCRIPTOR  *PeiStartupDescriptor,
  IN VOID                        *Data
  )
/*++

Routine Description:

  The entry routine to Pei Core, invoked by PeiMain during transition
  from SEC to PEI. After switching stack in the PEI core, it will restart
  with the old core data.

Arguments:

  PeiStartupDescriptor - Information and services provided by SEC phase.
  OldCoreData          - Pointer to old core data that is used to initialize the
                         core's data areas.

Returns:

  This function never returns
  EFI_NOT_FOUND        - Never reach

--*/
;

//
// Dispatcher support functions
//

EFI_STATUS
PeimDispatchReadiness (
  IN EFI_PEI_SERVICES   **PeiServices,
  IN VOID               *DependencyExpression,
  IN OUT BOOLEAN        *Runnable
  )
/*++

Routine Description:

  This is the POSTFIX version of the dependency evaluator.  When a
  PUSH [PPI GUID] is encountered, a pointer to the GUID is stored on
  the evaluation stack.  When that entry is poped from the evaluation
  stack, the PPI is checked if it is installed.  This method allows
  some time savings as not all PPIs must be checked for certain
  operation types (AND, OR).

Arguments:

  PeiServices               - Calling context.

  DependencyExpression      - Pointer to a dependency expression.  The Grammar adheres to
                              the BNF described above and is stored in postfix notation.
  Runnable                  - is True if the driver can be scheduled and False if the driver
                              cannot be scheduled.  This is the value that the schedulers
                              should use for deciding the state of the driver.

Returns:

  Status = EFI_SUCCESS            if it is a well-formed Grammar
           EFI_INVALID_PARAMETER  if the dependency expression overflows
                                  the evaluation stack
           EFI_INVALID_PARAMETER  if the dependency expression underflows
                                  the evaluation stack
           EFI_INVALID_PARAMETER  if the dependency expression is not a
                                  well-formed Grammar.
--*/
;


EFI_STATUS
PeiDispatcher (
  IN EFI_PEI_STARTUP_DESCRIPTOR  *PeiStartupDescriptor,
  IN PEI_CORE_INSTANCE           *PrivateData,
  IN PEI_CORE_DISPATCH_DATA      *DispatchData
  )

/*++

Routine Description:

  Conduct PEIM dispatch.

Arguments:

  PeiStartupDescriptor - Pointer to IN EFI_PEI_STARTUP_DESCRIPTOR
  PrivateData          - Pointer to the private data passed in from caller
  DispatchData         - Pointer to PEI_CORE_DISPATCH_DATA data.

Returns:

  EFI_SUCCESS   - Successfully dispatched PEIM.
  EFI_NOT_FOUND - The dispatch failed.

--*/
;


VOID
InitializeDispatcherData (
  IN EFI_PEI_SERVICES             **PeiServices,
  IN PEI_CORE_INSTANCE            *OldCoreData,
  IN EFI_PEI_STARTUP_DESCRIPTOR   *PeiStartupDescriptor
  )
/*++

Routine Description:

  Initialize the Dispatcher's data members

Arguments:

  PeiServices          - The PEI core services table.
  OldCoreData          - Pointer to old core data (before switching stack).
                         NULL if being run in non-permament memory mode.
  PeiStartupDescriptor - Information and services provided by SEC phase.


Returns:

  None

--*/
;


EFI_STATUS
FindNextPeim (
  IN EFI_PEI_SERVICES            **PeiServices,
  IN EFI_FIRMWARE_VOLUME_HEADER  *FwVolHeader,
  IN OUT EFI_FFS_FILE_HEADER     **PeimFileHeader
  )
/*++

Routine Description:
    Given the input file pointer, search for the next matching file in the
    FFS volume. The search starts from FileHeader inside
    the Firmware Volume defined by FwVolHeader.

Arguments:
    PeiServices - Pointer to the PEI Core Services Table.

    FwVolHeader - Pointer to the FV header of the volume to search.
                     This parameter must point to a valid FFS volume.

    PeimFileHeader  - Pointer to the current file from which to begin searching.
                  This pointer will be updated upon return to reflect the file found.

Returns:
    EFI_NOT_FOUND - No files matching the search criteria were found
    EFI_SUCCESS

--*/
;

BOOLEAN
Dispatched (
  IN UINT8  CurrentPeim,
  IN UINT32 DispatchedPeimBitMap
  )
/*++

Routine Description:

  This routine checks to see if a particular PEIM has been dispatched during
  the PEI core dispatch.

Arguments:
  CurrentPeim - The PEIM/FV in the bit array to check.
  DispatchedPeimBitMap - Bit array, each bit corresponds to a PEIM/FV.

Returns:
  TRUE if PEIM already dispatched
  FALSE if not

--*/
;

VOID
SetDispatched (
  IN EFI_PEI_SERVICES   **PeiServices,
  IN UINT8              CurrentPeim,
  OUT UINT32            *DispatchedPeimBitMap
  )
/*++

Routine Description:

  This routine sets a PEIM as having been dispatched once its entry
  point has been invoked.

Arguments:

  PeiServices          - The PEI core services table.
  CurrentPeim          - The PEIM/FV in the bit array to check.
  DispatchedPeimBitMap - Bit array, each bit corresponds to a PEIM/FV.

Returns:
  None

--*/
;

BOOLEAN
DepexSatisfied (
  IN EFI_PEI_SERVICES  **PeiServices,
  IN  VOID             *CurrentPeimAddress
  )
/*++

Routine Description:

  This routine parses the Dependency Expression, if available, and
  decides if the module can be executed.

Arguments:
  PeiServices - The PEI Service Table
  CurrentPeimAddress - Address of the PEIM Firmware File under investigation

Returns:
  TRUE  - Can be dispatched
  FALSE - Cannot be dispatched

--*/
;

#if   defined (MDE_CPU_IPF)
  //
  // In Ipf we should make special changes for the PHIT pointers to support
  // recovery boot in cache mode.
  //
#define  SWITCH_TO_CACHE_MODE(CoreData)  SwitchToCacheMode(CoreData)
#define  CACHE_MODE_ADDRESS_MASK         0x7FFFFFFFFFFFFFFFULL
VOID
SwitchToCacheMode (
  IN PEI_CORE_INSTANCE           *CoreData
)
/*++

Routine Description:

 Switch the PHIT pointers to cache mode after InstallPeiMemory in CAR.

Arguments:

  CoreData   - The PEI core Private Data

Returns:

--*/
;

#else

#define  SWITCH_TO_CACHE_MODE(CoreData)

#endif

//
// PPI support functions
//
VOID
InitializePpiServices (
  IN EFI_PEI_SERVICES    **PeiServices,
  IN PEI_CORE_INSTANCE   *OldCoreData
  )
/*++

Routine Description:

  Initialize PPI services.

Arguments:

  PeiServices - The PEI core services table.
  OldCoreData - Pointer to the PEI Core data.
                NULL if being run in non-permament memory mode.

Returns:
  Nothing

--*/
;

VOID
ConvertPpiPointers (
  IN EFI_PEI_SERVICES              **PeiServices,
  IN EFI_HOB_HANDOFF_INFO_TABLE    *OldHandOffHob,
  IN EFI_HOB_HANDOFF_INFO_TABLE    *NewHandOffHob
  )
/*++

Routine Description:

  Migrate the Hob list from the CAR stack to PEI installed memory.

Arguments:

  PeiServices   - The PEI core services table.
  OldHandOffHob - The old handoff HOB list.
  NewHandOffHob - The new handoff HOB list.

Returns:

--*/
;

EFI_STATUS
EFIAPI
PeiInstallPpi (
  IN EFI_PEI_SERVICES        **PeiServices,
  IN EFI_PEI_PPI_DESCRIPTOR  *PpiList
  )
/*++

Routine Description:

  Install PPI services.

Arguments:

  PeiServices - Pointer to the PEI Service Table
  PpiList     - Pointer to a list of PEI PPI Descriptors.

Returns:

    EFI_SUCCESS             - if all PPIs in PpiList are successfully installed.
    EFI_INVALID_PARAMETER   - if PpiList is NULL pointer
    EFI_INVALID_PARAMETER   - if any PPI in PpiList is not valid
    EFI_OUT_OF_RESOURCES    - if there is no more memory resource to install PPI

--*/
;

EFI_STATUS
EFIAPI
PeiReInstallPpi (
  IN EFI_PEI_SERVICES        **PeiServices,
  IN EFI_PEI_PPI_DESCRIPTOR  *OldPpi,
  IN EFI_PEI_PPI_DESCRIPTOR  *NewPpi
  )
/*++

Routine Description:

  Re-Install PPI services.

Arguments:

  PeiServices - Pointer to the PEI Service Table
  OldPpi      - Pointer to the old PEI PPI Descriptors.
  NewPpi      - Pointer to the new PEI PPI Descriptors.

Returns:

  EFI_SUCCESS           - if the operation was successful
  EFI_INVALID_PARAMETER - if OldPpi or NewPpi is NULL
  EFI_INVALID_PARAMETER - if NewPpi is not valid
  EFI_NOT_FOUND         - if the PPI was not in the database

--*/
;

EFI_STATUS
EFIAPI
PeiLocatePpi (
  IN EFI_PEI_SERVICES            **PeiServices,
  IN EFI_GUID                    *Guid,
  IN UINTN                       Instance,
  IN OUT EFI_PEI_PPI_DESCRIPTOR  **PpiDescriptor,
  IN OUT VOID                    **Ppi
  )
/*++

Routine Description:

  Locate a given named PPI.

Arguments:

  PeiServices   - Pointer to the PEI Service Table
  Guid          - Pointer to GUID of the PPI.
  Instance      - Instance Number to discover.
  PpiDescriptor - Pointer to reference the found descriptor. If not NULL,
                returns a pointer to the descriptor (includes flags, etc)
  Ppi           - Pointer to reference the found PPI

Returns:

  Status -  EFI_SUCCESS   if the PPI is in the database
            EFI_NOT_FOUND if the PPI is not in the database
--*/
;

EFI_STATUS
EFIAPI
PeiNotifyPpi (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyList
  )
/*++

Routine Description:

  Install a notification for a given PPI.

Arguments:

  PeiServices - Pointer to the PEI Service Table
  NotifyList  - Pointer to list of Descriptors to notify upon.

Returns:

  Status - EFI_SUCCESS          if successful
           EFI_OUT_OF_RESOURCES if no space in the database
           EFI_INVALID_PARAMETER if not a good decriptor

--*/
;

VOID
ProcessNotifyList (
  IN EFI_PEI_SERVICES    **PeiServices
  )
/*++

Routine Description:

  Process the Notify List at dispatch level.

Arguments:

  PeiServices - Pointer to the PEI Service Table

Returns:

--*/
;

VOID
DispatchNotify (
  IN EFI_PEI_SERVICES    **PeiServices,
  IN UINTN               NotifyType,
  IN INTN                InstallStartIndex,
  IN INTN                InstallStopIndex,
  IN INTN                NotifyStartIndex,
  IN INTN                NotifyStopIndex
  )
/*++

Routine Description:

  Dispatch notifications.

Arguments:

  PeiServices         - Pointer to the PEI Service Table
  NotifyType          - Type of notify to fire.
  InstallStartIndex   - Install Beginning index.
  InstallStopIndex    - Install Ending index.
  NotifyStartIndex    - Notify Beginning index.
  NotifyStopIndex    - Notify Ending index.

Returns:  None

--*/
;

//
// Boot mode support functions
//
EFI_STATUS
EFIAPI
PeiGetBootMode (
  IN EFI_PEI_SERVICES  **PeiServices,
  IN OUT EFI_BOOT_MODE *BootMode
  )
/*++

Routine Description:

  This service enables PEIMs to ascertain the present value of the boot mode.

Arguments:

  PeiServices    - The PEI core services table.
  BootMode       - A pointer to contain the value of the boot mode.

Returns:

  EFI_SUCCESS           - The boot mode was returned successfully.
  EFI_INVALID_PARAMETER - BootMode is NULL.

--*/
;

EFI_STATUS
EFIAPI
PeiSetBootMode (
  IN EFI_PEI_SERVICES  **PeiServices,
  IN EFI_BOOT_MODE     BootMode
  )
/*++

Routine Description:

  This service enables PEIMs to update the boot mode variable.

Arguments:

  PeiServices    - The PEI core services table.
  BootMode       - The value of the boot mode to set.

Returns:

  EFI_SUCCESS    - The value was successfully updated

--*/
;

//
// Security support functions
//
VOID
InitializeSecurityServices (
  IN EFI_PEI_SERVICES  **PeiServices,
  IN PEI_CORE_INSTANCE *OldCoreData
  )
/*++

Routine Description:

  Initialize the security services.

Arguments:

  PeiServices - The PEI core services table.
  OldCoreData - Pointer to the old core data.
                NULL if being run in non-permament memory mode.
Returns:

  None

--*/
;

EFI_STATUS
VerifyFv (
  IN EFI_FIRMWARE_VOLUME_HEADER  *CurrentFvAddress
  )
/*++

Routine Description:

  Provide a callout to the OEM FV verification service.

Arguments:

  CurrentFvAddress       - Pointer to the FV under investigation.

Returns:

  Status - EFI_SUCCESS

--*/
;


EFI_STATUS
VerifyPeim (
  IN EFI_PEI_SERVICES     **PeiServices,
  IN EFI_FFS_FILE_HEADER  *CurrentPeimAddress
  )
/*++

Routine Description:

  Provide a callout to the security verification service.

Arguments:

  PeiServices          - The PEI core services table.
  CurrentPeimAddress   - Pointer to the Firmware File under investigation.

Returns:

  EFI_SUCCESS             - Image is OK
  EFI_SECURITY_VIOLATION  - Image is illegal

--*/
;


EFI_STATUS
EFIAPI
PeiGetHobList (
  IN EFI_PEI_SERVICES  **PeiServices,
  IN OUT VOID          **HobList
  )
/*++

Routine Description:

  Gets the pointer to the HOB List.

Arguments:

  PeiServices - The PEI core services table.
  HobList     - Pointer to the HOB List.

Returns:

  EFI_SUCCESS                 - Get the pointer of HOB List
  EFI_NOT_AVAILABLE_YET       - the HOB List is not yet published
  EFI_INVALID_PARAMETER       - HobList is NULL (in debug mode)

--*/
;

EFI_STATUS
EFIAPI
PeiCreateHob (
  IN EFI_PEI_SERVICES  **PeiServices,
  IN UINT16            Type,
  IN UINT16            Length,
  IN OUT VOID          **Hob
  )
/*++

Routine Description:

  Add a new HOB to the HOB List.

Arguments:

  PeiServices - The PEI core services table.
  Type        - Type of the new HOB.
  Length      - Length of the new HOB to allocate.
  Hob         - Pointer to the new HOB.

Returns:

  Status  - EFI_SUCCESS
          - EFI_INVALID_PARAMETER if Hob is NULL
          - EFI_NOT_AVAILABLE_YET if HobList is still not available.
          - EFI_OUT_OF_RESOURCES if there is no more memory to grow the Hoblist.

--*/
;

EFI_STATUS
PeiCoreBuildHobHandoffInfoTable (
  IN EFI_BOOT_MODE         BootMode,
  IN EFI_PHYSICAL_ADDRESS  MemoryBegin,
  IN UINT64                MemoryLength
  )
/*++

Routine Description:

  Builds a Handoff Information Table HOB

Arguments:

  BootMode      - Current Bootmode
  MemoryBegin   - Start Memory Address.
  MemoryLength  - Length of Memory.

Returns:

  EFI_SUCCESS

--*/
;


//
// FFS Fw Volume support functions
//
EFI_STATUS
EFIAPI
PeiFfsFindNextFile (
  IN EFI_PEI_SERVICES            **PeiServices,
  IN UINT8                       SearchType,
  IN EFI_FIRMWARE_VOLUME_HEADER  *FwVolHeader,
  IN OUT EFI_FFS_FILE_HEADER     **FileHeader
  )
/*++

Routine Description:
    Given the input file pointer, search for the next matching file in the
    FFS volume as defined by SearchType. The search starts from FileHeader inside
    the Firmware Volume defined by FwVolHeader.

Arguments:
    PeiServices - Pointer to the PEI Core Services Table.

    SearchType - Filter to find only files of this type.
      Type EFI_FV_FILETYPE_ALL causes no filtering to be done.

    FwVolHeader - Pointer to the FV header of the volume to search.
      This parameter must point to a valid FFS volume.

    FileHeader  - Pointer to the current file from which to begin searching.
      This pointer will be updated upon return to reflect the file found.

Returns:
    EFI_NOT_FOUND - No files matching the search criteria were found
    EFI_SUCCESS

--*/
;

EFI_STATUS
EFIAPI
PeiFfsFindSectionData (
  IN EFI_PEI_SERVICES            **PeiServices,
  IN EFI_SECTION_TYPE            SectionType,
  IN EFI_FFS_FILE_HEADER         *FfsFileHeader,
  IN OUT VOID                    **SectionData
  )
/*++

Routine Description:
    Given the input file pointer, search for the next matching section in the
    FFS volume.

Arguments:
    PeiServices - Pointer to the PEI Core Services Table.
    SearchType - Filter to find only sections of this type.
    FfsFileHeader  - Pointer to the current file to search.
    SectionData - Pointer to the Section matching SectionType in FfsFileHeader.
                - NULL if section not found

Returns:
    EFI_NOT_FOUND - No files matching the search criteria were found
    EFI_SUCCESS

--*/
;

EFI_STATUS
EFIAPI
PeiFvFindNextVolume (
  IN EFI_PEI_SERVICES                **PeiServices,
  IN UINTN                           Instance,
  IN OUT EFI_FIRMWARE_VOLUME_HEADER  **FwVolHeader
  )
/*++

Routine Description:

  Return the BFV location

  BugBug -- Move this to the location of this code to where the
  other FV and FFS support code lives.
  Also, update to use FindFV for instances #'s >= 1.

Arguments:

  PeiServices - The PEI core services table.
  Instance    - Instance of FV to find
  FwVolHeader - Pointer to contain the data to return

Returns:
  Pointer to the Firmware Volume instance requested

  EFI_INVALID_PARAMETER     - FwVolHeader is NULL

  EFI_SUCCESS               - Firmware volume instance successfully found.

--*/
;

//
// Memory support functions
//
VOID
InitializeMemoryServices (
  IN EFI_PEI_SERVICES            **PeiServices,
  IN EFI_PEI_STARTUP_DESCRIPTOR  *PeiStartupDescriptor,
  IN PEI_CORE_INSTANCE           *OldCoreData
  )
/*++

Routine Description:

  Initialize the memory services.

Arguments:

  PeiServices          - The PEI core services table.
  PeiStartupDescriptor - Information and services provided by SEC phase.
  OldCoreData          - Pointer to the PEI Core data.
                         NULL if being run in non-permament memory mode.

Returns:

  None

--*/
;

EFI_STATUS
EFIAPI
PeiInstallPeiMemory (
  IN EFI_PEI_SERVICES      **PeiServices,
  IN EFI_PHYSICAL_ADDRESS  MemoryBegin,
  IN UINT64                MemoryLength
  )
/*++

Routine Description:

  Install the permanent memory is now available.
  Creates HOB (PHIT and Stack).

Arguments:

  PeiServices   - The PEI core services table.
  MemoryBegin   - Start of memory address.
  MemoryLength  - Length of memory.

Returns:

  Status  - EFI_SUCCESS

--*/
;

EFI_STATUS
EFIAPI
PeiAllocatePages (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_MEMORY_TYPE            MemoryType,
  IN UINTN                      Pages,
  OUT EFI_PHYSICAL_ADDRESS      *Memory
  )
/*++

Routine Description:

  Memory allocation service on permanent memory,
  not usable prior to the memory installation.

Arguments:

  PeiServices - The PEI core services table.
  Type        - Type of allocation.
  MemoryType  - Type of memory to allocate.
  Pages       - Number of pages to allocate.
  Memory      - Pointer of memory allocated.

Returns:

  Status - EFI_SUCCESS              The allocation was successful
           EFI_INVALID_PARAMETER    Only AllocateAnyAddress is supported.
           EFI_NOT_AVAILABLE_YET    Called with permanent memory not available
           EFI_OUT_OF_RESOURCES     There is not enough HOB heap to satisfy the requirement
                                    to allocate the number of pages.

--*/
;

EFI_STATUS
EFIAPI
PeiAllocatePool (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN UINTN                      Size,
  OUT VOID                      **Buffer
  )
/*++

Routine Description:

  Memory allocation service on the CAR.

Arguments:

  PeiServices - The PEI core services table.

  Size        - Amount of memory required

  Buffer      - Address of pointer to the buffer

Returns:

  Status - EFI_SUCCESS              The allocation was successful
           EFI_OUT_OF_RESOURCES     There is not enough heap to satisfy the requirement
                                    to allocate the requested size.

--*/
;

EFI_STATUS
PeiLoadImage (
  IN  EFI_PEI_SERVICES            **PeiServices,
  IN  EFI_FFS_FILE_HEADER         *PeimFileHeader,
  OUT VOID                        **EntryPoint
  )
/*++

Routine Description:

  Get entry point of a Peim file.

Arguments:

  PeiServices                 - Calling context.

  PeimFileHeader              - Peim file's header.

  EntryPoint                  - Entry point of that Peim file.

Returns:

  Status code.

--*/
;


EFI_STATUS
EFIAPI
PeiReportStatusCode (
  IN EFI_PEI_SERVICES         **PeiServices,
  IN EFI_STATUS_CODE_TYPE     CodeType,
  IN EFI_STATUS_CODE_VALUE    Value,
  IN UINT32                   Instance,
  IN EFI_GUID                 *CallerId,
  IN EFI_STATUS_CODE_DATA     *Data OPTIONAL
  )
/*++

Routine Description:

  Core version of the Status Code reporter

Arguments:

  PeiServices - The PEI core services table.

  CodeType    - Type of Status Code.

  Value       - Value to output for Status Code.

  Instance    - Instance Number of this status code.

  CallerId    - ID of the caller of this status code.

  Data        - Optional data associated with this status code.

Returns:

  Status  - EFI_SUCCESS             if status code is successfully reported
          - EFI_NOT_AVAILABLE_YET   if StatusCodePpi has not been installed

--*/
;


EFI_STATUS
EFIAPI
PeiResetSystem (
  IN EFI_PEI_SERVICES   **PeiServices
  )
/*++

Routine Description:

  Core version of the Reset System

Arguments:

  PeiServices - The PEI core services table.

Returns:

  Status  - EFI_NOT_AVAILABLE_YET. PPI not available yet.
          - EFI_DEVICE_ERROR.   Did not reset system.

  Otherwise, resets the system.

--*/
;

/**
  Transfers control to a function starting with a new stack.

  Transfers control to the function specified by EntryPoint using the new stack
  specified by NewStack and passing in the parameters specified by Context1 and
  Context2. Context1 and Context2 are optional and may be NULL. The function
  EntryPoint must never return.

  If EntryPoint is NULL, then ASSERT().
  If NewStack is NULL, then ASSERT().

  @param  EntryPoint  A pointer to function to call with the new stack.
  @param  Context1    A pointer to the context to pass into the EntryPoint
                      function.
  @param  Context2    A pointer to the context to pass into the EntryPoint
                      function.
  @param  NewStack    A pointer to the new stack to use for the EntryPoint
                      function.
  @param  NewBsp      A pointer to the new BSP for the EntryPoint on IPF. It's
                      Reserved on other architectures.

**/
VOID
EFIAPI
PeiSwitchStacks (
  IN      SWITCH_STACK_ENTRY_POINT  EntryPoint,
  IN      VOID                      *Context1,  OPTIONAL
  IN      VOID                      *Context2,  OPTIONAL
  IN      VOID                      *NewStack,
  IN      VOID                      *NewBsp
  );

#endif
