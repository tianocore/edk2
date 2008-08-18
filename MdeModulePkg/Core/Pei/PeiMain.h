/** @file
  Definition of Pei Core Structures and Services
  
Copyright (c) 2006 - 2008, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _PEI_MAIN_H_
#define _PEI_MAIN_H_

#include <PiPei.h>
#include <Ppi/DxeIpl.h>
#include <Ppi/MemoryDiscovered.h>
#include <Ppi/StatusCode.h>
#include <Ppi/Reset.h>
#include <Ppi/FirmwareVolume.h>
#include <Ppi/FirmwareVolumeInfo.h>
#include <Ppi/Decompress.h>
#include <Ppi/GuidedSectionExtraction.h>
#include <Ppi/LoadFile.h>
#include <Ppi/Security2.h>
#include <Ppi/TemporaryRamSupport.h>
#include <Library/DebugLib.h>
#include <Library/PeiCoreEntryPoint.h>
#include <Library/BaseLib.h>
#include <Library/HobLib.h>
#include <Library/PerformanceLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/PeCoffLib.h>
#include <Library/PeCoffGetEntryPointLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/TimerLib.h>
#include <Library/PcdLib.h>
#include <IndustryStandard/PeImage.h>
#include <Library/PeiServicesTablePointerLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PeiPiLib.h>
#include <Guid/FirmwareFileSystem2.h>
#include <Guid/AprioriFileName.h>

///
/// It is an FFS type extension used for PeiFindFileEx. It indicates current
/// Ffs searching is for all PEIMs can be dispatched by PeiCore.
///
#define PEI_CORE_INTERNAL_FFS_FILE_DISPATCH_TYPE   0xff

///
/// Pei Core private data structures
///
typedef union {
  EFI_PEI_PPI_DESCRIPTOR      *Ppi;
  EFI_PEI_NOTIFY_DESCRIPTOR   *Notify;
  VOID                        *Raw;
} PEI_PPI_LIST_POINTERS;

///
/// PPI database structure which contains two link: PpiList and NotifyList. PpiList
/// is in head of PpiListPtrs array and notify is in end of PpiListPtrs.
///
typedef struct {
  ///
  /// index of end of PpiList link list.
  ///
  INTN                    PpiListEnd;
  ///
  /// index of end of notify link list.
  ///
  INTN                    NotifyListEnd;
  ///
  /// index of the dispatched notify list.
  ///
  INTN                    DispatchListEnd;
  ///
  /// index of last installed Ppi description in PpiList link list.
  ///
  INTN                    LastDispatchedInstall;
  ///
  /// index of last dispatched notify in Notify link list.
  /// 
  INTN                    LastDispatchedNotify;
  ///
  /// Ppi database.
  ///
  PEI_PPI_LIST_POINTERS   PpiListPtrs[FixedPcdGet32 (PcdPeiCoreMaxPpiSupported)];
} PEI_PPI_DATABASE;


//
// PEI_CORE_FV_HANDE.PeimState
// Do not change these values as there is code doing math to change states.
// Look for Private->Fv[FvCount].PeimState[PeimCount]++;
//
#define PEIM_STATE_NOT_DISPATCHED         0x00
#define PEIM_STATE_DISPATCHED             0x01
#define PEIM_STATE_REGISITER_FOR_SHADOW   0x02
#define PEIM_STATE_DONE                   0x03

typedef struct {
  EFI_FIRMWARE_VOLUME_HEADER          *FvHeader;
  UINT8                               PeimState[FixedPcdGet32 (PcdPeiCoreMaxPeimPerFv)];
  EFI_PEI_FILE_HANDLE                 FvFileHandles[FixedPcdGet32 (PcdPeiCoreMaxPeimPerFv)];
  BOOLEAN                             ScanFv;
} PEI_CORE_FV_HANDLE;

#define CACHE_SETION_MAX_NUMBER       0x10
typedef struct {
  EFI_COMMON_SECTION_HEADER*          Section[CACHE_SETION_MAX_NUMBER];
  VOID*                               SectionData[CACHE_SETION_MAX_NUMBER];
  UINTN                               SectionSize[CACHE_SETION_MAX_NUMBER];
  UINTN                               AllSectionCount;
  UINTN                               SectionIndex;
} CACHE_SECTION_DATA;


#define PEI_CORE_HANDLE_SIGNATURE  EFI_SIGNATURE_32('P','e','i','C')

///
/// Pei Core private data structure instance
///
typedef struct{
  UINTN                              Signature;
  EFI_PEI_SERVICES                   *PS;     // Point to ServiceTableShadow
  PEI_PPI_DATABASE                   PpiData;
  UINTN                              FvCount;
  PEI_CORE_FV_HANDLE                 Fv[FixedPcdGet32 (PcdPeiCoreMaxFvSupported)];
  EFI_PEI_FILE_HANDLE                CurrentFvFileHandles[FixedPcdGet32 (PcdPeiCoreMaxPeimPerFv)];
  UINTN                              AprioriCount;
  UINTN                              CurrentPeimFvCount;
  UINTN                              CurrentPeimCount;
  EFI_PEI_FILE_HANDLE                CurrentFileHandle;
  BOOLEAN                            PeimNeedingDispatch;
  BOOLEAN                            PeimDispatchOnThisPass;
  BOOLEAN                            PeimDispatcherReenter;
  UINTN                              AllFvCount;
  EFI_PEI_FV_HANDLE                  AllFv[FixedPcdGet32 (PcdPeiCoreMaxFvSupported)];
  EFI_PEI_HOB_POINTERS               HobList;
  BOOLEAN                            SwitchStackSignal;
  BOOLEAN                            PeiMemoryInstalled;
  EFI_PHYSICAL_ADDRESS               StackBase;
  UINT64                             StackSize;
  VOID                               *BottomOfCarHeap;
  VOID                               *TopOfCarHeap;
  VOID                               *CpuIo;
  EFI_PEI_SECURITY2_PPI              *PrivateSecurityPpi;
  EFI_PEI_SERVICES                   ServiceTableShadow;
  UINTN                              SizeOfTemporaryMemory;
  UINTN                              SizeOfCacheAsRam;
  VOID                               *MaxTopOfCarHeap;
  EFI_PEI_PPI_DESCRIPTOR             *XipLoadFile;
  EFI_PHYSICAL_ADDRESS               PhysicalMemoryBegin;
  UINT64                             PhysicalMemoryLength;
  EFI_PHYSICAL_ADDRESS               FreePhysicalMemoryTop;
  VOID*                              ShadowedPeiCore;
  CACHE_SECTION_DATA                 CacheSection;
} PEI_CORE_INSTANCE;

///
/// Pei Core Instance Data Macros
///
#define PEI_CORE_INSTANCE_FROM_PS_THIS(a) \
  CR(a, PEI_CORE_INSTANCE, PS, PEI_CORE_HANDLE_SIGNATURE)

/**
  Function Pointer type for PeiCore function.
  @param SecCoreData     Points to a data structure containing information about the PEI core's operating
                         environment, such as the size and location of temporary RAM, the stack location and
                         the BFV location.
  @param PpiList         Points to a list of one or more PPI descriptors to be installed initially by the PEI core.
                         An empty PPI list consists of a single descriptor with the end-tag
                         EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST. As part of its initialization
                         phase, the PEI Foundation will add these SEC-hosted PPIs to its PPI database such
                         that both the PEI Foundation and any modules can leverage the associated service
                         calls and/or code in these early PPIs
  @param Data            Pointer to old core data that is used to initialize the
                         core's data areas.
**/
typedef
EFI_STATUS
(EFIAPI *PEICORE_FUNCTION_POINTER)(
  IN CONST  EFI_SEC_PEI_HAND_OFF    *SecCoreData,
  IN CONST  EFI_PEI_PPI_DESCRIPTOR  *PpiList,
  IN PEI_CORE_INSTANCE              *OldCoreData
  );

///
/// Union of temporarily used function pointers (to save stack space)
///
typedef union {
  PEICORE_FUNCTION_POINTER     PeiCore;
  EFI_PEIM_ENTRY_POINT2        PeimEntry;
  EFI_PEIM_NOTIFY_ENTRY_POINT  PeimNotifyEntry;
  EFI_DXE_IPL_PPI              *DxeIpl;
  EFI_PEI_PPI_DESCRIPTOR       *PpiDescriptor;
  EFI_PEI_NOTIFY_DESCRIPTOR    *NotifyDescriptor;
  VOID                         *Raw;
} PEI_CORE_TEMP_POINTERS;

typedef struct {
  CONST EFI_SEC_PEI_HAND_OFF    *SecCoreData;
  EFI_PEI_PPI_DESCRIPTOR        *PpiList;
  VOID                          *Data;
} PEI_CORE_PARAMETERS;

//
// PeiCore function
//
/**

  The entry routine to Pei Core, invoked by PeiMain during transition
  from SEC to PEI. After switching stack in the PEI core, it will restart
  with the old core data.


  @param SecCoreData     Points to a data structure containing information about the PEI core's operating
                         environment, such as the size and location of temporary RAM, the stack location and
                         the BFV location.
  @param PpiList         Points to a list of one or more PPI descriptors to be installed initially by the PEI core.
                         An empty PPI list consists of a single descriptor with the end-tag
                         EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST. As part of its initialization
                         phase, the PEI Foundation will add these SEC-hosted PPIs to its PPI database such
                         that both the PEI Foundation and any modules can leverage the associated service
                         calls and/or code in these early PPIs
  @param Data            Pointer to old core data that is used to initialize the
                         core's data areas.

  @retval EFI_NOT_FOUND  Never reach

**/
EFI_STATUS
EFIAPI
PeiCore (
  IN CONST EFI_SEC_PEI_HAND_OFF        *SecCoreData,
  IN CONST EFI_PEI_PPI_DESCRIPTOR      *PpList,
  IN VOID                              *Data
  )
;

//
// Dispatcher support functions
//

/**

  This is the POSTFIX version of the dependency evaluator.  When a
  PUSH [PPI GUID] is encountered, a pointer to the GUID is stored on
  the evaluation stack.  When that entry is poped from the evaluation
  stack, the PPI is checked if it is installed.  This method allows
  some time savings as not all PPIs must be checked for certain
  operation types (AND, OR).


  @param PeiServices            An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  @param DependencyExpression   Pointer to a dependency expression.  The Grammar adheres to
                                the BNF described above and is stored in postfix notation.

  @retval TRUE      if it is a well-formed Grammar
  @retval FALSE     if the dependency expression overflows the evaluation stack
                    if the dependency expression underflows the evaluation stack
                    if the dependency expression is not a well-formed Grammar.

**/
BOOLEAN
PeimDispatchReadiness (
  IN EFI_PEI_SERVICES   **PeiServices,
  IN VOID               *DependencyExpression
  )
;

/**
  Conduct PEIM dispatch.

  @param SecCoreData     Pointer to the data structure containing SEC to PEI handoff data
  @param PrivateData     Pointer to the private data passed in from caller

  @retval EFI_SUCCESS    Successfully dispatched PEIM.
  @retval EFI_NOT_FOUND  The dispatch failed.

**/
VOID
PeiDispatcher (
  IN CONST EFI_SEC_PEI_HAND_OFF  *SecCoreData,
  IN PEI_CORE_INSTANCE           *PrivateData
  )
;

/**
  Initialize the Dispatcher's data members

  @param PrivateData     PeiCore's private data structure
  @param OldCoreData     Old data from SecCore
                         NULL if being run in non-permament memory mode.
  @param SecCoreData     Points to a data structure containing information about the PEI core's operating
                         environment, such as the size and location of temporary RAM, the stack location and
                         the BFV location.

**/
VOID
InitializeDispatcherData (
  IN PEI_CORE_INSTANCE            *PrivateData,
  IN PEI_CORE_INSTANCE            *OldCoreData,
  IN CONST EFI_SEC_PEI_HAND_OFF   *SecCoreData
  )
;

/**
  This routine parses the Dependency Expression, if available, and
  decides if the module can be executed.


  @param Private         PeiCore's private data structure
  @param FileHandle      PEIM's file handle
  @param PeimCount       The index of last dispatched PEIM.

  @retval TRUE           Can be dispatched
  @retval FALSE          Cannot be dispatched

**/
BOOLEAN
DepexSatisfied (
  IN PEI_CORE_INSTANCE          *Private,
  IN EFI_PEI_FILE_HANDLE        FileHandle,
  IN UINTN                      PeimCount
  )
;

//
// PPI support functions
//
/**

  Initialize PPI services.

  @param PrivateData     Pointer to the PEI Core data.
  @param OldCoreData     Pointer to old PEI Core data. 
                         NULL if being run in non-permament memory mode.

**/
VOID
InitializePpiServices (
  IN PEI_CORE_INSTANCE   *PrivateData,
  IN PEI_CORE_INSTANCE   *OldCoreData
  )
;

/**

  Migrate the Hob list from the CAR stack to PEI installed memory.

  @param PrivateData         Pointer to PeiCore's private data structure.
  @param OldCheckingBottom   Bottom of temporary memory range. All Ppi in this range
                             will be fixup for PpiData and PpiDescriptor pointer.
  @param OldCheckingTop      Top of temporary memory range. All Ppi in this range
                             will be fixup for PpiData and PpiDescriptor.
  @param Fixup               The address difference between
                             the new Hob list and old Hob list.

**/
VOID
ConvertPpiPointers (
  IN PEI_CORE_INSTANCE       *PrivateData,
  IN UINTN                   OldCheckingBottom,
  IN UINTN                   OldCheckingTop,
  IN INTN                    Fixup
  )
;

/**

  Install PPI services. It is implementation of EFI_PEI_SERVICE.InstallPpi.

  @param PeiServices                An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  @param PpiList                    Pointer to ppi array that want to be installed.

  @retval EFI_SUCCESS               if all PPIs in PpiList are successfully installed.
  @retval EFI_INVALID_PARAMETER     if PpiList is NULL pointer
                                    if any PPI in PpiList is not valid
  @retval EFI_OUT_OF_RESOURCES      if there is no more memory resource to install PPI

**/
EFI_STATUS
EFIAPI
PeiInstallPpi (
  IN CONST EFI_PEI_SERVICES        **PeiServices,
  IN CONST EFI_PEI_PPI_DESCRIPTOR  *PpiList
  )
;

/**

  Re-Install PPI services.

  @param PeiServices            An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  @param OldPpi                 Pointer to the old PEI PPI Descriptors.
  @param NewPpi                 Pointer to the new PEI PPI Descriptors.

  @retval EFI_SUCCESS           if the operation was successful
  @retval EFI_INVALID_PARAMETER if OldPpi or NewPpi is NULL
                                if NewPpi is not valid
  @retval EFI_NOT_FOUND         if the PPI was not in the database

**/
EFI_STATUS
EFIAPI
PeiReInstallPpi (
  IN CONST EFI_PEI_SERVICES        **PeiServices,
  IN CONST EFI_PEI_PPI_DESCRIPTOR  *OldPpi,
  IN CONST EFI_PEI_PPI_DESCRIPTOR  *NewPpi
  )
;

/**

  Locate a given named PPI.


  @param PeiServices     An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  @param Guid            Pointer to GUID of the PPI.
  @param Instance        Instance Number to discover.
  @param PpiDescriptor   Pointer to reference the found descriptor. If not NULL,
                         returns a pointer to the descriptor (includes flags, etc)
  @param Ppi             Pointer to reference the found PPI

  @retval EFI_SUCCESS   if the PPI is in the database
  @retval EFI_NOT_FOUND if the PPI is not in the database

**/
EFI_STATUS
EFIAPI
PeiLocatePpi (
  IN CONST EFI_PEI_SERVICES      **PeiServices,
  IN CONST EFI_GUID              *Guid,
  IN UINTN                       Instance,
  IN OUT EFI_PEI_PPI_DESCRIPTOR  **PpiDescriptor,
  IN OUT VOID                    **Ppi
  )
;

/**

  Install a notification for a given PPI.


  @param PeiServices            An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  @param NotifyList             Pointer to list of Descriptors to notify upon.

  @retval EFI_SUCCESS           if successful
  @retval EFI_OUT_OF_RESOURCES  if no space in the database
  @retval EFI_INVALID_PARAMETER if not a good decriptor

**/
EFI_STATUS
EFIAPI
PeiNotifyPpi (
  IN CONST EFI_PEI_SERVICES           **PeiServices,
  IN CONST EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyList
  )
;

/**

  Process the Notify List at dispatch level.

  @param PrivateData  PeiCore's private data structure.

**/
VOID
ProcessNotifyList (
  IN PEI_CORE_INSTANCE  *PrivateData
  )
;

/**

  Dispatch notifications.

  @param PrivateData        PeiCore's private data structure
  @param NotifyType         Type of notify to fire.
  @param InstallStartIndex  Install Beginning index.
  @param InstallStopIndex   Install Ending index.
  @param NotifyStartIndex   Notify Beginning index.
  @param NotifyStopIndex    Notify Ending index.

**/
VOID
DispatchNotify (
  IN PEI_CORE_INSTANCE  *PrivateData,
  IN UINTN               NotifyType,
  IN INTN                InstallStartIndex,
  IN INTN                InstallStopIndex,
  IN INTN                NotifyStartIndex,
  IN INTN                NotifyStopIndex
  )
;

//
// Boot mode support functions
//
/**
  This service enables PEIMs to ascertain the present value of the boot mode.

  @param PeiServices            An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  @param BootMode               A pointer to contain the value of the boot mode.

  @retval EFI_SUCCESS           The boot mode was returned successfully.
  @retval EFI_INVALID_PARAMETER BootMode is NULL.

**/
EFI_STATUS
EFIAPI
PeiGetBootMode (
  IN CONST EFI_PEI_SERVICES  **PeiServices,
  IN OUT EFI_BOOT_MODE *BootMode
  )
;

/**
  This service enables PEIMs to update the boot mode variable.


  @param PeiServices     An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  @param BootMode        The value of the boot mode to set.

  @return EFI_SUCCESS    The value was successfully updated

**/
EFI_STATUS
EFIAPI
PeiSetBootMode (
  IN CONST EFI_PEI_SERVICES  **PeiServices,
  IN EFI_BOOT_MODE     BootMode
  )
;

//
// Security support functions
//
/**

  Initialize the security services.

  @param PeiServices     An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  @param OldCoreData     Pointer to the old core data.
                         NULL if being run in non-permament memory mode.

**/
VOID
InitializeSecurityServices (
  IN EFI_PEI_SERVICES  **PeiServices,
  IN PEI_CORE_INSTANCE *OldCoreData
  )
;

/**
  Verify a Firmware volume

  @param CurrentFvAddress           Pointer to the current Firmware Volume under consideration

  @retval EFI_SUCCESS               Firmware Volume is legal
  @retval EFI_SECURITY_VIOLATION    Firmware Volume fails integrity test

**/
EFI_STATUS
VerifyFv (
  IN EFI_FIRMWARE_VOLUME_HEADER  *CurrentFvAddress
  )
;

/**

  Provide a callout to the security verification service.


  @param PrivateData     PeiCore's private data structure
  @param VolumeHandle    Handle of FV
  @param FileHandle      Handle of PEIM's ffs

  @retval EFI_SUCCESS              Image is OK
  @retval EFI_SECURITY_VIOLATION   Image is illegal

**/
EFI_STATUS
VerifyPeim (
  IN PEI_CORE_INSTANCE      *PrivateData,
  IN EFI_PEI_FV_HANDLE      VolumeHandle,
  IN EFI_PEI_FILE_HANDLE    FileHandle
  )
;

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
PeiGetHobList (
  IN CONST EFI_PEI_SERVICES  **PeiServices,
  IN OUT VOID          **HobList
  )
;

/**
  Add a new HOB to the HOB List.

  @param PeiServices        An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  @param Type               Type of the new HOB.
  @param Length             Length of the new HOB to allocate.
  @param Hob                Pointer to the new HOB.

  @return  EFI_SUCCESS           Success to create hob.
  @retval  EFI_INVALID_PARAMETER if Hob is NULL
  @retval  EFI_NOT_AVAILABLE_YET if HobList is still not available.
  @retval  EFI_OUT_OF_RESOURCES  if there is no more memory to grow the Hoblist.

**/
EFI_STATUS
EFIAPI
PeiCreateHob (
  IN CONST EFI_PEI_SERVICES  **PeiServices,
  IN UINT16            Type,
  IN UINT16            Length,
  IN OUT VOID          **Hob
  )
;

/**

  Builds a Handoff Information Table HOB

  @param BootMode        - Current Bootmode
  @param MemoryBegin     - Start Memory Address.
  @param MemoryLength    - Length of Memory.

  @return EFI_SUCCESS Always success to initialize HOB.

**/
EFI_STATUS
PeiCoreBuildHobHandoffInfoTable (
  IN EFI_BOOT_MODE         BootMode,
  IN EFI_PHYSICAL_ADDRESS  MemoryBegin,
  IN UINT64                MemoryLength
  )
;


//
// FFS Fw Volume support functions
//
/**
  Given the input file pointer, search for the next matching file in the
  FFS volume as defined by SearchType. The search starts from FileHeader inside
  the Firmware Volume defined by FwVolHeader.


  @param PeiServices     An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  @param SearchType      Filter to find only files of this type.
                         Type EFI_FV_FILETYPE_ALL causes no filtering to be done.
  @param FwVolHeader     Pointer to the FV header of the volume to search.
  @param FileHeader      Pointer to the current file from which to begin searching.
                         This pointer will be updated upon return to reflect the file found.
  @retval EFI_NOT_FOUND  No files matching the search criteria were found
  @retval EFI_SUCCESS    Success to find next file in given volume

**/
EFI_STATUS
EFIAPI
PeiFfsFindNextFile (
  IN CONST EFI_PEI_SERVICES      **PeiServices,
  IN UINT8                       SearchType,
  IN EFI_PEI_FV_HANDLE           FwVolHeader,
  IN OUT EFI_PEI_FILE_HANDLE     *FileHeader
  )
;

/**
  Given the input file pointer, search for the next matching section in the
  FFS volume.

  @param PeiServices     An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  @param SectionType     Filter to find only sections of this type.
  @param FfsFileHeader   Pointer to the current file to search.
  @param SectionData     Pointer to the Section matching SectionType in FfsFileHeader.
                         NULL if section not found

  @retval EFI_NOT_FOUND  No files matching the search criteria were found
  @retval EFI_SUCCESS    Success to find section data in given file

**/
EFI_STATUS
EFIAPI
PeiFfsFindSectionData (
  IN CONST EFI_PEI_SERVICES      **PeiServices,
  IN EFI_SECTION_TYPE            SectionType,
  IN EFI_PEI_FILE_HANDLE         FfsFileHeader,
  IN OUT VOID                    **SectionData
  )
;

/**
  search the firmware volumes by index

  @param PeiServices            An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  @param Instance               Instance of FV to find
  @param FwVolHeader            Pointer to found Volume.

  @retval EFI_INVALID_PARAMETER FwVolHeader is NULL
  @retval EFI_SUCCESS           Firmware volume instance successfully found.

**/
EFI_STATUS
EFIAPI
PeiFvFindNextVolume (
  IN CONST EFI_PEI_SERVICES          **PeiServices,
  IN UINTN                           Instance,
  IN OUT EFI_PEI_FV_HANDLE           *FwVolHeader
  )
;

//
// Memory support functions
//
/**

  Initialize the memory services.

  @param PrivateData     PeiCore's private data structure
  @param SecCoreData     Points to a data structure containing information about the PEI core's operating
                         environment, such as the size and location of temporary RAM, the stack location and
                         the BFV location.
  @param OldCoreData     Pointer to the PEI Core data.
                         NULL if being run in non-permament memory mode.

**/
VOID
InitializeMemoryServices (
  IN PEI_CORE_INSTANCE           *PrivateData,
  IN CONST EFI_SEC_PEI_HAND_OFF  *SecCoreData,
  IN PEI_CORE_INSTANCE           *OldCoreData
  )
;

/**

  Install the permanent memory is now available.
  Creates HOB (PHIT and Stack).

  @param PeiServices     An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  @param MemoryBegin     Start of memory address.
  @param MemoryLength    Length of memory.

  @return EFI_SUCCESS Always success.

**/
EFI_STATUS
EFIAPI
PeiInstallPeiMemory (
  IN CONST EFI_PEI_SERVICES      **PeiServices,
  IN EFI_PHYSICAL_ADDRESS  MemoryBegin,
  IN UINT64                MemoryLength
  )
;

/**

  Memory allocation service on permanent memory,
  not usable prior to the memory installation.


  @param PeiServices               An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  @param MemoryType                Type of memory to allocate.
  @param Pages                     Number of pages to allocate.
  @param Memory                    Pointer of memory allocated.

  @retval EFI_SUCCESS              The allocation was successful
  @retval EFI_INVALID_PARAMETER    Only AllocateAnyAddress is supported.
  @retval EFI_NOT_AVAILABLE_YET    Called with permanent memory not available
  @retval EFI_OUT_OF_RESOURCES     There is not enough HOB heap to satisfy the requirement
                                   to allocate the number of pages.

**/
EFI_STATUS
EFIAPI
PeiAllocatePages (
  IN CONST EFI_PEI_SERVICES           **PeiServices,
  IN EFI_MEMORY_TYPE            MemoryType,
  IN UINTN                      Pages,
  OUT EFI_PHYSICAL_ADDRESS      *Memory
  )
;

/**

  Memory allocation service on the CAR.


  @param PeiServices        An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  @param Size               Amount of memory required
  @param Buffer             Address of pointer to the buffer

  @retval EFI_SUCCESS              The allocation was successful
  @retval EFI_OUT_OF_RESOURCES     There is not enough heap to satisfy the requirement
                                   to allocate the requested size.

**/
EFI_STATUS
EFIAPI
PeiAllocatePool (
  IN CONST EFI_PEI_SERVICES           **PeiServices,
  IN UINTN                      Size,
  OUT VOID                      **Buffer
  )
;

/**

  Routine for load image file.


  @param PeiServices            An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  @param FileHandle             Pointer to the FFS file header of the image.
  @param EntryPoint             Pointer to entry point of specified image file for output.
  @param AuthenticationState    Pointer to attestation authentication state of image.

  @retval EFI_SUCCESS     Image is successfully loaded.
  @retval EFI_NOT_FOUND   Fail to locate necessary PPI
  @retval Others          Fail to load file.

**/
EFI_STATUS
PeiLoadImage (
  IN  EFI_PEI_SERVICES            **PeiServices,
  IN  EFI_PEI_FILE_HANDLE         FileHandle,
  OUT    EFI_PHYSICAL_ADDRESS     *EntryPoint,
  OUT    UINT32                   *AuthenticationState
  )
;

/**

  Core version of the Status Code reporter


  @param PeiServices            An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  @param CodeType               Type of Status Code.
  @param Value                  Value to output for Status Code.
  @param Instance               Instance Number of this status code.
  @param CallerId               ID of the caller of this status code.
  @param Data                   Optional data associated with this status code.

  @retval EFI_SUCCESS             if status code is successfully reported
  @retval EFI_NOT_AVAILABLE_YET   if StatusCodePpi has not been installed

**/
EFI_STATUS
EFIAPI
PeiReportStatusCode (
  IN CONST EFI_PEI_SERVICES         **PeiServices,
  IN EFI_STATUS_CODE_TYPE     CodeType,
  IN EFI_STATUS_CODE_VALUE    Value,
  IN UINT32                   Instance,
  IN CONST EFI_GUID                 *CallerId,
  IN CONST EFI_STATUS_CODE_DATA     *Data OPTIONAL
  )
;

/**

  Core version of the Reset System


  @param PeiServices                An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.

  @retval EFI_NOT_AVAILABLE_YET     PPI not available yet.
  @retval EFI_DEVICE_ERROR          Did not reset system.
                                    Otherwise, resets the system.

**/
EFI_STATUS
EFIAPI
PeiResetSystem (
  IN CONST EFI_PEI_SERVICES   **PeiServices
  )
;

/**

  Initialize PeiCore Fv List.


  @param PrivateData     - Pointer to PEI_CORE_INSTANCE.
  @param SecCoreData     - Pointer to EFI_SEC_PEI_HAND_OFF.

**/
VOID
PeiInitializeFv (
  IN  PEI_CORE_INSTANCE           *PrivateData,
  IN CONST EFI_SEC_PEI_HAND_OFF   *SecCoreData
  )
;

/**
  Process Firmware Volum Information once FvInfoPPI install.

  @param PeiServices       An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  @param NotifyDescriptor  Address of the notification descriptor data structure.
  @param Ppi               Address of the PPI that was installed.

  @retval EFI_SUCCESS if the interface could be successfully installed

**/
EFI_STATUS
EFIAPI
FirmwareVolmeInfoPpiNotifyCallback (
  IN EFI_PEI_SERVICES              **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR     *NotifyDescriptor,
  IN VOID                          *Ppi
  )
;

/**

  Given the input VolumeHandle, search for the next matching name file.

  @param FileName        File name to search.
  @param VolumeHandle    The current FV to search.
  @param FileHandle      Pointer to the file matching name in VolumeHandle.
                         NULL if file not found

  @retval EFI_NOT_FOUND  No files matching the search criteria were found
  @retval EFI_SUCCESS    Success to search given file

**/
EFI_STATUS
EFIAPI
PeiFfsFindFileByName (
  IN  CONST EFI_GUID        *FileName,
  IN  EFI_PEI_FV_HANDLE     VolumeHandle,
  OUT EFI_PEI_FILE_HANDLE   *FileHandle
  )
;

/**

  Returns information about a specific file.


  @param FileHandle         The handle to file.
  @param FileInfo           Pointer to the file information.

  @retval EFI_INVALID_PARAMETER Invalid FileHandle or FileInfo.
  @retval EFI_SUCCESS           Success to collect file info.

**/
EFI_STATUS
EFIAPI
PeiFfsGetFileInfo (
  IN EFI_PEI_FILE_HANDLE  FileHandle,
  OUT EFI_FV_FILE_INFO    *FileInfo
  )
;

/**

  Collect information of given Fv Volume.

  @param VolumeHandle           The handle to Fv Volume.
  @param VolumeInfo             The pointer to volume information.

  @retval EFI_INVALID_PARAMETER VolumeInfo is NULL
  @retval EFI_SUCCESS           Success to collect fv info.
**/
EFI_STATUS
EFIAPI
PeiFfsGetVolumeInfo (
  IN EFI_PEI_FV_HANDLE  VolumeHandle,
  OUT EFI_FV_INFO       *VolumeInfo
  )
;

/**
  This routine enable a PEIM to register itself to shadow when PEI Foundation
  discovery permanent memory.

  @param FileHandle             File handle of a PEIM.

  @retval EFI_NOT_FOUND         The file handle doesn't point to PEIM itself.
  @retval EFI_ALREADY_STARTED   Indicate that the PEIM has been registered itself.
  @retval EFI_SUCCESS           Successfully to register itself.

**/
EFI_STATUS
EFIAPI
PeiRegisterForShadow (
  IN EFI_PEI_FILE_HANDLE       FileHandle
  )
;

/**
  Given the input file pointer, search for the next matching file in the
  FFS volume as defined by SearchType. The search starts from FileHeader inside
  the Firmware Volume defined by FwVolHeader.


  @param FvHandle        Pointer to the FV header of the volume to search
  @param FileName        File name
  @param SearchType      Filter to find only files of this type.
                         Type EFI_FV_FILETYPE_ALL causes no filtering to be done.
  @param FileHandle      This parameter must point to a valid FFS volume.
  @param AprioriFile     Pointer to AprioriFile image in this FV if has

  @return EFI_NOT_FOUND  No files matching the search criteria were found
  @retval EFI_SUCCESS    Success to search given file

**/
EFI_STATUS
PeiFindFileEx (
  IN  CONST EFI_PEI_FV_HANDLE        FvHandle,
  IN  CONST EFI_GUID                 *FileName,   OPTIONAL
  IN        EFI_FV_FILETYPE          SearchType,
  IN OUT    EFI_PEI_FILE_HANDLE      *FileHandle,
  IN OUT    EFI_PEI_FV_HANDLE        *AprioriFile  OPTIONAL
  )
;

/**
  Initialize image service that install PeiLoadFilePpi.

  @param PrivateData     Pointer to PeiCore's private data structure PEI_CORE_INSTANCE.
  @param OldCoreData     Pointer to Old PeiCore's private data.
                         If NULL, PeiCore is entered at first time, stack/heap in temporary memory.
                         If not NULL, PeiCore is entered at second time, stack/heap has been moved
                         to permenent memory.

**/
VOID
InitializeImageServices (
  IN  PEI_CORE_INSTANCE   *PrivateData,
  IN  PEI_CORE_INSTANCE   *OldCoreData
  )
;

/**
  Get Fv image from the FV type file, then install FV INFO ppi, Build FV hob.

  @param PeiServices          An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  @param FileHandle           File handle of a Fv type file.
  @param AuthenticationState  Pointer to attestation authentication state of image.
                              If return 0, means pass security checking.

  @retval EFI_NOT_FOUND       FV image can't be found.
  @retval EFI_SUCCESS         Successfully to process it.

**/
EFI_STATUS
ProcessFvFile (
  IN  EFI_PEI_SERVICES      **PeiServices,
  IN  EFI_PEI_FILE_HANDLE   FvFileHandle,
  OUT UINT32                *AuthenticationState
  );

#endif
