/** @file
  Definition of Pei Core Structures and Services

Copyright (c) 2006 - 2019, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _PEI_MAIN_H_
#define _PEI_MAIN_H_

#include <PiPei.h>
#include <Ppi/DxeIpl.h>
#include <Ppi/MemoryDiscovered.h>
#include <Ppi/StatusCode.h>
#include <Ppi/Reset.h>
#include <Ppi/Reset2.h>
#include <Ppi/FirmwareVolume.h>
#include <Ppi/FirmwareVolumeInfo.h>
#include <Ppi/FirmwareVolumeInfo2.h>
#include <Ppi/Decompress.h>
#include <Ppi/GuidedSectionExtraction.h>
#include <Ppi/LoadFile.h>
#include <Ppi/Security2.h>
#include <Ppi/TemporaryRamSupport.h>
#include <Ppi/TemporaryRamDone.h>
#include <Ppi/SecHobData.h>
#include <Ppi/PeiCoreFvLocation.h>
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
#include <Library/PcdLib.h>
#include <IndustryStandard/PeImage.h>
#include <Library/PeiServicesTablePointerLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Guid/FirmwareFileSystem2.h>
#include <Guid/FirmwareFileSystem3.h>
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
/// Number of PEI_PPI_LIST_POINTERS to grow by each time we run out of room
///
#define PPI_GROWTH_STEP             64
#define CALLBACK_NOTIFY_GROWTH_STEP 32
#define DISPATCH_NOTIFY_GROWTH_STEP 8

typedef struct {
  UINTN                 CurrentCount;
  UINTN                 MaxCount;
  UINTN                 LastDispatchedCount;
  ///
  /// MaxCount number of entries.
  ///
  PEI_PPI_LIST_POINTERS *PpiPtrs;
} PEI_PPI_LIST;

typedef struct {
  UINTN                 CurrentCount;
  UINTN                 MaxCount;
  ///
  /// MaxCount number of entries.
  ///
  PEI_PPI_LIST_POINTERS *NotifyPtrs;
} PEI_CALLBACK_NOTIFY_LIST;

typedef struct {
  UINTN                 CurrentCount;
  UINTN                 MaxCount;
  UINTN                 LastDispatchedCount;
  ///
  /// MaxCount number of entries.
  ///
  PEI_PPI_LIST_POINTERS *NotifyPtrs;
} PEI_DISPATCH_NOTIFY_LIST;

///
/// PPI database structure which contains three links:
/// PpiList, CallbackNotifyList and DispatchNotifyList.
///
typedef struct {
  ///
  /// PPI List.
  ///
  PEI_PPI_LIST              PpiList;
  ///
  /// Notify List at dispatch level.
  ///
  PEI_CALLBACK_NOTIFY_LIST  CallbackNotifyList;
  ///
  /// Notify List at callback level.
  ///
  PEI_DISPATCH_NOTIFY_LIST  DispatchNotifyList;
} PEI_PPI_DATABASE;

//
// PEI_CORE_FV_HANDE.PeimState
// Do not change these values as there is code doing math to change states.
// Look for Private->Fv[FvCount].PeimState[PeimCount]++;
//
#define PEIM_STATE_NOT_DISPATCHED         0x00
#define PEIM_STATE_DISPATCHED             0x01
#define PEIM_STATE_REGISTER_FOR_SHADOW    0x02
#define PEIM_STATE_DONE                   0x03

//
// Number of FV instances to grow by each time we run out of room
//
#define FV_GROWTH_STEP 8

typedef struct {
  EFI_FIRMWARE_VOLUME_HEADER          *FvHeader;
  EFI_PEI_FIRMWARE_VOLUME_PPI         *FvPpi;
  EFI_PEI_FV_HANDLE                   FvHandle;
  UINTN                               PeimCount;
  //
  // Ponter to the buffer with the PeimCount number of Entries.
  //
  UINT8                               *PeimState;
  //
  // Ponter to the buffer with the PeimCount number of Entries.
  //
  EFI_PEI_FILE_HANDLE                 *FvFileHandles;
  BOOLEAN                             ScanFv;
  UINT32                              AuthenticationStatus;
} PEI_CORE_FV_HANDLE;

typedef struct {
  EFI_GUID                            FvFormat;
  VOID                                *FvInfo;
  UINT32                              FvInfoSize;
  UINT32                              AuthenticationStatus;
  EFI_PEI_NOTIFY_DESCRIPTOR           NotifyDescriptor;
} PEI_CORE_UNKNOW_FORMAT_FV_INFO;

#define CACHE_SETION_MAX_NUMBER       0x10
typedef struct {
  EFI_COMMON_SECTION_HEADER*          Section[CACHE_SETION_MAX_NUMBER];
  VOID*                               SectionData[CACHE_SETION_MAX_NUMBER];
  UINTN                               SectionSize[CACHE_SETION_MAX_NUMBER];
  UINT32                              AuthenticationStatus[CACHE_SETION_MAX_NUMBER];
  UINTN                               AllSectionCount;
  UINTN                               SectionIndex;
} CACHE_SECTION_DATA;

#define HOLE_MAX_NUMBER       0x3
typedef struct {
  EFI_PHYSICAL_ADDRESS               Base;
  UINTN                              Size;
  UINTN                              Offset;
  BOOLEAN                            OffsetPositive;
} HOLE_MEMORY_DATA;

///
/// Forward declaration for PEI_CORE_INSTANCE
///
typedef struct _PEI_CORE_INSTANCE  PEI_CORE_INSTANCE;


/**
  Function Pointer type for PeiCore function.
  @param SecCoreData     Points to a data structure containing SEC to PEI handoff data, such as the size
                         and location of temporary RAM, the stack location and the BFV location.
  @param PpiList         Points to a list of one or more PPI descriptors to be installed initially by the PEI core.
                         An empty PPI list consists of a single descriptor with the end-tag
                         EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST. As part of its initialization
                         phase, the PEI Foundation will add these SEC-hosted PPIs to its PPI database such
                         that both the PEI Foundation and any modules can leverage the associated service
                         calls and/or code in these early PPIs
  @param OldCoreData     Pointer to old core data that is used to initialize the
                         core's data areas.
**/
typedef
EFI_STATUS
(EFIAPI *PEICORE_FUNCTION_POINTER)(
  IN CONST  EFI_SEC_PEI_HAND_OFF    *SecCoreData,
  IN CONST  EFI_PEI_PPI_DESCRIPTOR  *PpiList,
  IN PEI_CORE_INSTANCE              *OldCoreData
  );

//
// Number of files to grow by each time we run out of room
//
#define TEMP_FILE_GROWTH_STEP 32

#define PEI_CORE_HANDLE_SIGNATURE  SIGNATURE_32('P','e','i','C')

///
/// Pei Core private data structure instance
///
struct _PEI_CORE_INSTANCE {
  UINTN                              Signature;

  ///
  /// Point to ServiceTableShadow
  ///
  EFI_PEI_SERVICES                   *Ps;
  PEI_PPI_DATABASE                   PpiData;

  ///
  /// The count of FVs which contains FFS and could be dispatched by PeiCore.
  ///
  UINTN                              FvCount;

  ///
  /// The max count of FVs which contains FFS and could be dispatched by PeiCore.
  ///
  UINTN                              MaxFvCount;

  ///
  /// Pointer to the buffer with the MaxFvCount number of entries.
  /// Each entry is for one FV which contains FFS and could be dispatched by PeiCore.
  ///
  PEI_CORE_FV_HANDLE                 *Fv;

  ///
  /// Pointer to the buffer with the MaxUnknownFvInfoCount number of entries.
  /// Each entry is for one FV which could not be dispatched by PeiCore.
  ///
  PEI_CORE_UNKNOW_FORMAT_FV_INFO     *UnknownFvInfo;
  UINTN                              MaxUnknownFvInfoCount;
  UINTN                              UnknownFvInfoCount;

  ///
  /// Pointer to the buffer FvFileHandlers in PEI_CORE_FV_HANDLE specified by CurrentPeimFvCount.
  ///
  EFI_PEI_FILE_HANDLE                *CurrentFvFileHandles;
  UINTN                              AprioriCount;
  UINTN                              CurrentPeimFvCount;
  UINTN                              CurrentPeimCount;
  EFI_PEI_FILE_HANDLE                CurrentFileHandle;
  BOOLEAN                            PeimNeedingDispatch;
  BOOLEAN                            PeimDispatchOnThisPass;
  BOOLEAN                            PeimDispatcherReenter;
  EFI_PEI_HOB_POINTERS               HobList;
  BOOLEAN                            SwitchStackSignal;
  BOOLEAN                            PeiMemoryInstalled;
  VOID                               *CpuIo;
  EFI_PEI_SECURITY2_PPI              *PrivateSecurityPpi;
  EFI_PEI_SERVICES                   ServiceTableShadow;
  EFI_PEI_PPI_DESCRIPTOR             *XipLoadFile;
  EFI_PHYSICAL_ADDRESS               PhysicalMemoryBegin;
  UINT64                             PhysicalMemoryLength;
  EFI_PHYSICAL_ADDRESS               FreePhysicalMemoryTop;
  UINTN                              HeapOffset;
  BOOLEAN                            HeapOffsetPositive;
  UINTN                              StackOffset;
  BOOLEAN                            StackOffsetPositive;
  //
  // Information for migrating memory pages allocated in pre-memory phase.
  //
  HOLE_MEMORY_DATA                   MemoryPages;
  PEICORE_FUNCTION_POINTER           ShadowedPeiCore;
  CACHE_SECTION_DATA                 CacheSection;
  //
  // For Loading modules at fixed address feature to cache the top address below which the
  // Runtime code, boot time code and PEI memory will be placed. Please note that the offset between this field
  // and  Ps should not be changed since maybe user could get this top address by using the offet to Ps.
  //
  EFI_PHYSICAL_ADDRESS               LoadModuleAtFixAddressTopAddress;
  //
  // The field is define for Loading modules at fixed address feature to tracker the PEI code
  // memory range usage. It is a bit mapped array in which every bit indicates the correspoding memory page
  // available or not.
  //
  UINT64                            *PeiCodeMemoryRangeUsageBitMap;
  //
  // This field points to the shadowed image read function
  //
  PE_COFF_LOADER_READ_FILE          ShadowedImageRead;

  UINTN                             TempPeimCount;

  //
  // Pointer to the temp buffer with the TempPeimCount number of entries.
  //
  EFI_PEI_FILE_HANDLE               *TempFileHandles;
  //
  // Pointer to the temp buffer with the TempPeimCount number of entries.
  //
  EFI_GUID                          *TempFileGuid;

  //
  // Temp Memory Range is not covered by PeiTempMem and Stack.
  // Those Memory Range will be migrated into physical memory.
  //
  HOLE_MEMORY_DATA                  HoleData[HOLE_MAX_NUMBER];
};

///
/// Pei Core Instance Data Macros
///
#define PEI_CORE_INSTANCE_FROM_PS_THIS(a) \
  CR(a, PEI_CORE_INSTANCE, Ps, PEI_CORE_HANDLE_SIGNATURE)

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


  @param SecCoreData     Points to a data structure containing SEC to PEI handoff data, such as the size
                         and location of temporary RAM, the stack location and the BFV location.
  @param PpiList         Points to a list of one or more PPI descriptors to be installed initially by the PEI core.
                         An empty PPI list consists of a single descriptor with the end-tag
                         EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST. As part of its initialization
                         phase, the PEI Foundation will add these SEC-hosted PPIs to its PPI database such
                         that both the PEI Foundation and any modules can leverage the associated service
                         calls and/or code in these early PPIs
  @param Data            Pointer to old core data that is used to initialize the
                         core's data areas.

**/
VOID
EFIAPI
PeiCore (
  IN CONST EFI_SEC_PEI_HAND_OFF        *SecCoreData,
  IN CONST EFI_PEI_PPI_DESCRIPTOR      *PpiList,
  IN VOID                              *Data
  );

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
  );

/**
  Conduct PEIM dispatch.

  @param SecCoreData     Pointer to the data structure containing SEC to PEI handoff data
  @param PrivateData     Pointer to the private data passed in from caller

**/
VOID
PeiDispatcher (
  IN CONST EFI_SEC_PEI_HAND_OFF  *SecCoreData,
  IN PEI_CORE_INSTANCE           *PrivateData
  );

/**
  Initialize the Dispatcher's data members

  @param PrivateData     PeiCore's private data structure
  @param OldCoreData     Old data from SecCore
                         NULL if being run in non-permament memory mode.
  @param SecCoreData     Points to a data structure containing SEC to PEI handoff data, such as the size
                         and location of temporary RAM, the stack location and the BFV location.

**/
VOID
InitializeDispatcherData (
  IN PEI_CORE_INSTANCE            *PrivateData,
  IN PEI_CORE_INSTANCE            *OldCoreData,
  IN CONST EFI_SEC_PEI_HAND_OFF   *SecCoreData
  );

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
  );

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
  );

/**

  Migrate the Hob list from the temporary memory to PEI installed memory.

  @param SecCoreData     Points to a data structure containing SEC to PEI handoff data, such as the size
                         and location of temporary RAM, the stack location and the BFV location.
  @param PrivateData     Pointer to PeiCore's private data structure.

**/
VOID
ConvertPpiPointers (
  IN CONST EFI_SEC_PEI_HAND_OFF  *SecCoreData,
  IN PEI_CORE_INSTANCE           *PrivateData
  );

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
  );

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
  );

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
  );

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
  );

/**

  Process the Notify List at dispatch level.

  @param PrivateData  PeiCore's private data structure.

**/
VOID
ProcessDispatchNotifyList (
  IN PEI_CORE_INSTANCE  *PrivateData
  );

/**

  Process notifications.

  @param PrivateData        PeiCore's private data structure
  @param NotifyType         Type of notify to fire.
  @param InstallStartIndex  Install Beginning index.
  @param InstallStopIndex   Install Ending index.
  @param NotifyStartIndex   Notify Beginning index.
  @param NotifyStopIndex    Notify Ending index.

**/
VOID
ProcessNotify (
  IN PEI_CORE_INSTANCE  *PrivateData,
  IN UINTN               NotifyType,
  IN INTN                InstallStartIndex,
  IN INTN                InstallStopIndex,
  IN INTN                NotifyStartIndex,
  IN INTN                NotifyStopIndex
  );

/**
  Process PpiList from SEC phase.

  @param PeiServices    An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  @param PpiList        Points to a list of one or more PPI descriptors to be installed initially by the PEI core.
                        These PPI's will be installed and/or immediately signaled if they are notification type.

**/
VOID
ProcessPpiListFromSec (
  IN CONST EFI_PEI_SERVICES         **PeiServices,
  IN CONST EFI_PEI_PPI_DESCRIPTOR   *PpiList
  );

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
  IN OUT   EFI_BOOT_MODE     *BootMode
  );

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
  );

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
  );

/**
  Verify a Firmware volume.

  @param CurrentFvAddress           Pointer to the current Firmware Volume under consideration

  @retval EFI_SUCCESS               Firmware Volume is legal
  @retval EFI_SECURITY_VIOLATION    Firmware Volume fails integrity test

**/
EFI_STATUS
VerifyFv (
  IN EFI_FIRMWARE_VOLUME_HEADER  *CurrentFvAddress
  );

/**
  Provide a callout to the security verification service.

  @param PrivateData     PeiCore's private data structure
  @param VolumeHandle    Handle of FV
  @param FileHandle      Handle of PEIM's ffs
  @param AuthenticationStatus Authentication status

  @retval EFI_SUCCESS              Image is OK
  @retval EFI_SECURITY_VIOLATION   Image is illegal
  @retval EFI_NOT_FOUND            If security PPI is not installed.
**/
EFI_STATUS
VerifyPeim (
  IN PEI_CORE_INSTANCE      *PrivateData,
  IN EFI_PEI_FV_HANDLE      VolumeHandle,
  IN EFI_PEI_FILE_HANDLE    FileHandle,
  IN UINT32                 AuthenticationStatus
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
PeiGetHobList (
  IN CONST EFI_PEI_SERVICES  **PeiServices,
  IN OUT VOID          **HobList
  );

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
  );

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
  );

/**
  Install SEC HOB data to the HOB List.

  @param PeiServices    An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  @param SecHobList     Pointer to SEC HOB List.

  @return EFI_SUCCESS           Success to install SEC HOB data.
  @retval EFI_OUT_OF_RESOURCES  If there is no more memory to grow the Hoblist.

**/
EFI_STATUS
PeiInstallSecHobData (
  IN CONST EFI_PEI_SERVICES     **PeiServices,
  IN EFI_HOB_GENERIC_HEADER     *SecHobList
  );


//
// FFS Fw Volume support functions
//
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
PeiFfsFindNextFile (
  IN CONST EFI_PEI_SERVICES      **PeiServices,
  IN UINT8                       SearchType,
  IN EFI_PEI_FV_HANDLE           FvHandle,
  IN OUT EFI_PEI_FILE_HANDLE     *FileHandle
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
PeiFfsFindSectionData (
  IN CONST EFI_PEI_SERVICES    **PeiServices,
  IN     EFI_SECTION_TYPE      SectionType,
  IN     EFI_PEI_FILE_HANDLE   FileHandle,
  OUT VOID                     **SectionData
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
PeiFfsFindSectionData3 (
  IN CONST EFI_PEI_SERVICES    **PeiServices,
  IN     EFI_SECTION_TYPE      SectionType,
  IN     UINTN                 SectionInstance,
  IN     EFI_PEI_FILE_HANDLE   FileHandle,
  OUT VOID                     **SectionData,
  OUT UINT32                   *AuthenticationStatus
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
PeiFfsFindNextVolume (
  IN CONST EFI_PEI_SERVICES          **PeiServices,
  IN UINTN                           Instance,
  IN OUT EFI_PEI_FV_HANDLE           *VolumeHandle
  );

//
// Memory support functions
//
/**

  Initialize the memory services.

  @param PrivateData     PeiCore's private data structure
  @param SecCoreData     Points to a data structure containing SEC to PEI handoff data, such as the size
                         and location of temporary RAM, the stack location and the BFV location.
  @param OldCoreData     Pointer to the PEI Core data.
                         NULL if being run in non-permament memory mode.

**/
VOID
InitializeMemoryServices (
  IN PEI_CORE_INSTANCE           *PrivateData,
  IN CONST EFI_SEC_PEI_HAND_OFF  *SecCoreData,
  IN PEI_CORE_INSTANCE           *OldCoreData
  );

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
  );

/**
  Migrate memory pages allocated in pre-memory phase.
  Copy memory pages at temporary heap top to permanent heap top.

  @param[in] Private                Pointer to the private data passed in from caller.
  @param[in] TemporaryRamMigrated   Temporary memory has been migrated to permanent memory.

**/
VOID
MigrateMemoryPages (
  IN PEI_CORE_INSTANCE      *Private,
  IN BOOLEAN                TemporaryRamMigrated
  );

/**
  Migrate MemoryBaseAddress in memory allocation HOBs
  from the temporary memory to PEI installed memory.

  @param[in] PrivateData        Pointer to PeiCore's private data structure.

**/
VOID
ConvertMemoryAllocationHobs (
  IN PEI_CORE_INSTANCE          *PrivateData
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
PeiAllocatePages (
  IN CONST EFI_PEI_SERVICES     **PeiServices,
  IN       EFI_MEMORY_TYPE      MemoryType,
  IN       UINTN                Pages,
  OUT      EFI_PHYSICAL_ADDRESS *Memory
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
PeiFreePages (
  IN CONST EFI_PEI_SERVICES     **PeiServices,
  IN EFI_PHYSICAL_ADDRESS       Memory,
  IN UINTN                      Pages
  );

/**

  Memory allocation service on the temporary memory.


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
  );

/**

  Routine for load image file.


  @param PeiServices            An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  @param FileHandle             Pointer to the FFS file header of the image.
  @param PeimState              The dispatch state of the input PEIM handle.
  @param EntryPoint             Pointer to entry point of specified image file for output.
  @param AuthenticationState    Pointer to attestation authentication state of image.

  @retval EFI_SUCCESS     Image is successfully loaded.
  @retval EFI_NOT_FOUND   Fail to locate necessary PPI
  @retval Others          Fail to load file.

**/
EFI_STATUS
PeiLoadImage (
  IN  CONST EFI_PEI_SERVICES      **PeiServices,
  IN  EFI_PEI_FILE_HANDLE         FileHandle,
  IN  UINT8                       PeimState,
  OUT    EFI_PHYSICAL_ADDRESS     *EntryPoint,
  OUT    UINT32                   *AuthenticationState
  );

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
PeiResetSystem (
  IN CONST EFI_PEI_SERVICES   **PeiServices
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
PeiResetSystem2 (
  IN EFI_RESET_TYPE     ResetType,
  IN EFI_STATUS         ResetStatus,
  IN UINTN              DataSize,
  IN VOID               *ResetData OPTIONAL
  );

/**

  Initialize PeiCore Fv List.


  @param PrivateData     - Pointer to PEI_CORE_INSTANCE.
  @param SecCoreData     - Pointer to EFI_SEC_PEI_HAND_OFF.

**/
VOID
PeiInitializeFv (
  IN  PEI_CORE_INSTANCE           *PrivateData,
  IN CONST EFI_SEC_PEI_HAND_OFF   *SecCoreData
  );

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
  );

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
PeiFfsGetFileInfo (
  IN EFI_PEI_FILE_HANDLE  FileHandle,
  OUT EFI_FV_FILE_INFO    *FileInfo
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
PeiFfsGetFileInfo2 (
  IN EFI_PEI_FILE_HANDLE  FileHandle,
  OUT EFI_FV_FILE_INFO2   *FileInfo
  );

/**
  Returns information about the specified volume.

  @param VolumeHandle    Handle of the volume.
  @param VolumeInfo      Upon exit, points to the volume's information.

  @retval EFI_INVALID_PARAMETER If VolumeHandle does not represent a valid volume.
  @retval EFI_INVALID_PARAMETER If VolumeInfo is NULL.
  @retval EFI_SUCCESS           Volume information returned.
**/
EFI_STATUS
EFIAPI
PeiFfsGetVolumeInfo (
  IN EFI_PEI_FV_HANDLE  VolumeHandle,
  OUT EFI_FV_INFO       *VolumeInfo
  );

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
  );

/**
  Initialize image service that install PeiLoadFilePpi.

  @param PrivateData     Pointer to PeiCore's private data structure PEI_CORE_INSTANCE.
  @param OldCoreData     Pointer to Old PeiCore's private data.
                         If NULL, PeiCore is entered at first time, stack/heap in temporary memory.
                         If not NULL, PeiCore is entered at second time, stack/heap has been moved
                         to permanent memory.

**/
VOID
InitializeImageServices (
  IN  PEI_CORE_INSTANCE   *PrivateData,
  IN  PEI_CORE_INSTANCE   *OldCoreData
  );

/**
  The wrapper function of PeiLoadImageLoadImage().

  @param This                 Pointer to EFI_PEI_LOAD_FILE_PPI.
  @param FileHandle           Pointer to the FFS file header of the image.
  @param ImageAddressArg      Pointer to PE/TE image.
  @param ImageSizeArg         Size of PE/TE image.
  @param EntryPoint           Pointer to entry point of specified image file for output.
  @param AuthenticationState  Pointer to attestation authentication state of image.

  @return Status of PeiLoadImageLoadImage().

**/
EFI_STATUS
EFIAPI
PeiLoadImageLoadImageWrapper (
  IN     CONST EFI_PEI_LOAD_FILE_PPI  *This,
  IN     EFI_PEI_FILE_HANDLE          FileHandle,
  OUT    EFI_PHYSICAL_ADDRESS         *ImageAddressArg,  OPTIONAL
  OUT    UINT64                       *ImageSizeArg,     OPTIONAL
  OUT    EFI_PHYSICAL_ADDRESS         *EntryPoint,
  OUT    UINT32                       *AuthenticationState
  );

/**

  Provide a callback for when the security PPI is installed.

  @param PeiServices        An indirect pointer to the EFI_PEI_SERVICES table published by the PEI Foundation.
  @param NotifyDescriptor   The descriptor for the notification event.
  @param Ppi                Pointer to the PPI in question.

  @return Always success

**/
EFI_STATUS
EFIAPI
SecurityPpiNotifyCallback (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  );

/**
  Get Fv image(s) from the FV type file, then install FV INFO(2) ppi, Build FV(2, 3) hob.

  @param PrivateData          PeiCore's private data structure
  @param ParentFvCoreHandle   Pointer of EFI_CORE_FV_HANDLE to parent Fv image that contain this Fv image.
  @param ParentFvFileHandle   File handle of a Fv type file that contain this Fv image.

  @retval EFI_NOT_FOUND         FV image can't be found.
  @retval EFI_SUCCESS           Successfully to process it.
  @retval EFI_OUT_OF_RESOURCES  Can not allocate page when aligning FV image
  @retval EFI_SECURITY_VIOLATION Image is illegal
  @retval Others                Can not find EFI_SECTION_FIRMWARE_VOLUME_IMAGE section

**/
EFI_STATUS
ProcessFvFile (
  IN  PEI_CORE_INSTANCE           *PrivateData,
  IN  PEI_CORE_FV_HANDLE          *ParentFvCoreHandle,
  IN  EFI_PEI_FILE_HANDLE         ParentFvFileHandle
  );

/**
  Get instance of PEI_CORE_FV_HANDLE for next volume according to given index.

  This routine also will install FvInfo ppi for FV hob in PI ways.

  @param Private    Pointer of PEI_CORE_INSTANCE
  @param Instance   The index of FV want to be searched.

  @return Instance of PEI_CORE_FV_HANDLE.
**/
PEI_CORE_FV_HANDLE *
FindNextCoreFvHandle (
  IN PEI_CORE_INSTANCE  *Private,
  IN UINTN              Instance
  );

//
// Default EFI_PEI_CPU_IO_PPI support for EFI_PEI_SERVICES table when PeiCore initialization.
//

/**
  Memory-based read services.

  This function is to perform the Memory Access Read service based on installed
  instance of the EFI_PEI_CPU_IO_PPI.
  If the EFI_PEI_CPU_IO_PPI is not installed by platform/chipset PEIM, then
  return EFI_NOT_YET_AVAILABLE.

  @param  PeiServices           An indirect pointer to the PEI Services Table
                                published by the PEI Foundation.
  @param  This                  Pointer to local data for the interface.
  @param  Width                 The width of the access. Enumerated in bytes.
  @param  Address               The physical address of the access.
  @param  Count                 The number of accesses to perform.
  @param  Buffer                A pointer to the buffer of data.

  @retval EFI_SUCCESS           The function completed successfully.
  @retval EFI_NOT_YET_AVAILABLE The service has not been installed.
**/
EFI_STATUS
EFIAPI
PeiDefaultMemRead (
  IN  CONST EFI_PEI_SERVICES            **PeiServices,
  IN  CONST EFI_PEI_CPU_IO_PPI          *This,
  IN  EFI_PEI_CPU_IO_PPI_WIDTH          Width,
  IN  UINT64                            Address,
  IN  UINTN                             Count,
  IN  OUT VOID                          *Buffer
  );

/**
  Memory-based write services.

  This function is to perform the Memory Access Write service based on installed
  instance of the EFI_PEI_CPU_IO_PPI.
  If the EFI_PEI_CPU_IO_PPI is not installed by platform/chipset PEIM, then
  return EFI_NOT_YET_AVAILABLE.

  @param  PeiServices           An indirect pointer to the PEI Services Table
                                published by the PEI Foundation.
  @param  This                  Pointer to local data for the interface.
  @param  Width                 The width of the access. Enumerated in bytes.
  @param  Address               The physical address of the access.
  @param  Count                 The number of accesses to perform.
  @param  Buffer                A pointer to the buffer of data.

  @retval EFI_SUCCESS           The function completed successfully.
  @retval EFI_NOT_YET_AVAILABLE The service has not been installed.
**/
EFI_STATUS
EFIAPI
PeiDefaultMemWrite (
  IN  CONST EFI_PEI_SERVICES            **PeiServices,
  IN  CONST EFI_PEI_CPU_IO_PPI          *This,
  IN  EFI_PEI_CPU_IO_PPI_WIDTH          Width,
  IN  UINT64                            Address,
  IN  UINTN                             Count,
  IN  OUT VOID                          *Buffer
  );

/**
  IO-based read services.

  This function is to perform the IO-base read service for the EFI_PEI_CPU_IO_PPI.
  If the EFI_PEI_CPU_IO_PPI is not installed by platform/chipset PEIM, then
  return EFI_NOT_YET_AVAILABLE.

  @param  PeiServices           An indirect pointer to the PEI Services Table
                                published by the PEI Foundation.
  @param  This                  Pointer to local data for the interface.
  @param  Width                 The width of the access. Enumerated in bytes.
  @param  Address               The physical address of the access.
  @param  Count                 The number of accesses to perform.
  @param  Buffer                A pointer to the buffer of data.

  @retval EFI_SUCCESS           The function completed successfully.
  @retval EFI_NOT_YET_AVAILABLE The service has not been installed.
**/
EFI_STATUS
EFIAPI
PeiDefaultIoRead (
  IN      CONST EFI_PEI_SERVICES          **PeiServices,
  IN      CONST EFI_PEI_CPU_IO_PPI        *This,
  IN      EFI_PEI_CPU_IO_PPI_WIDTH        Width,
  IN      UINT64                          Address,
  IN      UINTN                           Count,
  IN OUT  VOID                            *Buffer
  );

/**
  IO-based write services.

  This function is to perform the IO-base write service for the EFI_PEI_CPU_IO_PPI.
  If the EFI_PEI_CPU_IO_PPI is not installed by platform/chipset PEIM, then
  return EFI_NOT_YET_AVAILABLE.

  @param  PeiServices           An indirect pointer to the PEI Services Table
                                published by the PEI Foundation.
  @param  This                  Pointer to local data for the interface.
  @param  Width                 The width of the access. Enumerated in bytes.
  @param  Address               The physical address of the access.
  @param  Count                 The number of accesses to perform.
  @param  Buffer                A pointer to the buffer of data.

  @retval EFI_SUCCESS           The function completed successfully.
  @retval EFI_NOT_YET_AVAILABLE The service has not been installed.
**/
EFI_STATUS
EFIAPI
PeiDefaultIoWrite (
  IN      CONST EFI_PEI_SERVICES          **PeiServices,
  IN      CONST EFI_PEI_CPU_IO_PPI        *This,
  IN      EFI_PEI_CPU_IO_PPI_WIDTH        Width,
  IN      UINT64                          Address,
  IN      UINTN                           Count,
  IN OUT  VOID                            *Buffer
  );

/**
  8-bit I/O read operations.

  If the EFI_PEI_CPU_IO_PPI is not installed by platform/chipset PEIM, then
  return 0.

  @param  PeiServices    An indirect pointer to the PEI Services Table published by the PEI Foundation.
  @param  This           Pointer to local data for the interface.
  @param  Address        The physical address of the access.

  @return An 8-bit value returned from the I/O space.
**/
UINT8
EFIAPI
PeiDefaultIoRead8 (
  IN  CONST EFI_PEI_SERVICES      **PeiServices,
  IN  CONST EFI_PEI_CPU_IO_PPI    *This,
  IN  UINT64                      Address
  );

/**
  Reads an 16-bit I/O port.

  If the EFI_PEI_CPU_IO_PPI is not installed by platform/chipset PEIM, then
  return 0.

  @param  PeiServices    An indirect pointer to the PEI Services Table published by the PEI Foundation.
  @param  This           Pointer to local data for the interface.
  @param  Address        The physical address of the access.

  @return A 16-bit value returned from the I/O space.
**/
UINT16
EFIAPI
PeiDefaultIoRead16 (
  IN  CONST EFI_PEI_SERVICES      **PeiServices,
  IN  CONST EFI_PEI_CPU_IO_PPI    *This,
  IN  UINT64                      Address
  );

/**
  Reads an 32-bit I/O port.

  If the EFI_PEI_CPU_IO_PPI is not installed by platform/chipset PEIM, then
  return 0.

  @param  PeiServices    An indirect pointer to the PEI Services Table published by the PEI Foundation.
  @param  This           Pointer to local data for the interface.
  @param  Address        The physical address of the access.

  @return A 32-bit value returned from the I/O space.
**/
UINT32
EFIAPI
PeiDefaultIoRead32 (
  IN  CONST EFI_PEI_SERVICES      **PeiServices,
  IN  CONST EFI_PEI_CPU_IO_PPI    *This,
  IN  UINT64                      Address
  );

/**
  Reads an 64-bit I/O port.

  If the EFI_PEI_CPU_IO_PPI is not installed by platform/chipset PEIM, then
  return 0.

  @param  PeiServices    An indirect pointer to the PEI Services Table published by the PEI Foundation.
  @param  This           Pointer to local data for the interface.
  @param  Address        The physical address of the access.

  @return A 64-bit value returned from the I/O space.
**/
UINT64
EFIAPI
PeiDefaultIoRead64 (
  IN  CONST EFI_PEI_SERVICES      **PeiServices,
  IN  CONST EFI_PEI_CPU_IO_PPI    *This,
  IN  UINT64                      Address
  );

/**
  8-bit I/O write operations.

  @param  PeiServices    An indirect pointer to the PEI Services Table published by the PEI Foundation.
  @param  This           Pointer to local data for the interface.
  @param  Address        The physical address of the access.
  @param  Data           The data to write.
**/
VOID
EFIAPI
PeiDefaultIoWrite8 (
  IN  CONST EFI_PEI_SERVICES      **PeiServices,
  IN  CONST EFI_PEI_CPU_IO_PPI    *This,
  IN  UINT64                      Address,
  IN  UINT8                       Data
  );

/**
  16-bit I/O write operations.

  @param  PeiServices    An indirect pointer to the PEI Services Table published by the PEI Foundation.
  @param  This           Pointer to local data for the interface.
  @param  Address        The physical address of the access.
  @param  Data           The data to write.
**/
VOID
EFIAPI
PeiDefaultIoWrite16 (
  IN  CONST EFI_PEI_SERVICES      **PeiServices,
  IN  CONST EFI_PEI_CPU_IO_PPI    *This,
  IN  UINT64                      Address,
  IN  UINT16                      Data
  );

/**
  32-bit I/O write operations.

  @param  PeiServices    An indirect pointer to the PEI Services Table published by the PEI Foundation.
  @param  This           Pointer to local data for the interface.
  @param  Address        The physical address of the access.
  @param  Data           The data to write.
**/
VOID
EFIAPI
PeiDefaultIoWrite32 (
  IN  CONST EFI_PEI_SERVICES      **PeiServices,
  IN  CONST EFI_PEI_CPU_IO_PPI    *This,
  IN  UINT64                      Address,
  IN  UINT32                      Data
  );

/**
  64-bit I/O write operations.

  @param  PeiServices    An indirect pointer to the PEI Services Table published by the PEI Foundation.
  @param  This           Pointer to local data for the interface.
  @param  Address        The physical address of the access.
  @param  Data           The data to write.
**/
VOID
EFIAPI
PeiDefaultIoWrite64 (
  IN  CONST EFI_PEI_SERVICES      **PeiServices,
  IN  CONST EFI_PEI_CPU_IO_PPI    *This,
  IN  UINT64                      Address,
  IN  UINT64                      Data
  );

/**
  8-bit memory read operations.

  If the EFI_PEI_CPU_IO_PPI is not installed by platform/chipset PEIM, then
  return 0.

  @param  PeiServices    An indirect pointer to the PEI Services Table published by the PEI Foundation.
  @param  This           Pointer to local data for the interface.
  @param  Address        The physical address of the access.

  @return An 8-bit value returned from the memory space.

**/
UINT8
EFIAPI
PeiDefaultMemRead8 (
  IN  CONST EFI_PEI_SERVICES      **PeiServices,
  IN  CONST EFI_PEI_CPU_IO_PPI    *This,
  IN  UINT64                      Address
  );

/**
  16-bit memory read operations.

  If the EFI_PEI_CPU_IO_PPI is not installed by platform/chipset PEIM, then
  return 0.

  @param  PeiServices    An indirect pointer to the PEI Services Table published by the PEI Foundation.
  @param  This           Pointer to local data for the interface.
  @param  Address        The physical address of the access.

  @return An 16-bit value returned from the memory space.

**/
UINT16
EFIAPI
PeiDefaultMemRead16 (
  IN  CONST EFI_PEI_SERVICES      **PeiServices,
  IN  CONST EFI_PEI_CPU_IO_PPI    *This,
  IN  UINT64                      Address
  );

/**
  32-bit memory read operations.

  If the EFI_PEI_CPU_IO_PPI is not installed by platform/chipset PEIM, then
  return 0.

  @param  PeiServices    An indirect pointer to the PEI Services Table published by the PEI Foundation.
  @param  This           Pointer to local data for the interface.
  @param  Address        The physical address of the access.

  @return An 32-bit value returned from the memory space.

**/
UINT32
EFIAPI
PeiDefaultMemRead32 (
  IN  CONST EFI_PEI_SERVICES      **PeiServices,
  IN  CONST EFI_PEI_CPU_IO_PPI    *This,
  IN  UINT64                      Address
  );

/**
  64-bit memory read operations.

  If the EFI_PEI_CPU_IO_PPI is not installed by platform/chipset PEIM, then
  return 0.

  @param  PeiServices    An indirect pointer to the PEI Services Table published by the PEI Foundation.
  @param  This           Pointer to local data for the interface.
  @param  Address        The physical address of the access.

  @return An 64-bit value returned from the memory space.

**/
UINT64
EFIAPI
PeiDefaultMemRead64 (
  IN  CONST EFI_PEI_SERVICES      **PeiServices,
  IN  CONST EFI_PEI_CPU_IO_PPI    *This,
  IN  UINT64                      Address
  );

/**
  8-bit memory write operations.

  @param  PeiServices    An indirect pointer to the PEI Services Table published by the PEI Foundation.
  @param  This           Pointer to local data for the interface.
  @param  Address        The physical address of the access.
  @param  Data           The data to write.

**/
VOID
EFIAPI
PeiDefaultMemWrite8 (
  IN  CONST EFI_PEI_SERVICES        **PeiServices,
  IN  CONST EFI_PEI_CPU_IO_PPI      *This,
  IN  UINT64                        Address,
  IN  UINT8                         Data
  );

/**
  16-bit memory write operations.

  @param  PeiServices    An indirect pointer to the PEI Services Table published by the PEI Foundation.
  @param  This           Pointer to local data for the interface.
  @param  Address        The physical address of the access.
  @param  Data           The data to write.

**/
VOID
EFIAPI
PeiDefaultMemWrite16 (
  IN  CONST EFI_PEI_SERVICES        **PeiServices,
  IN  CONST EFI_PEI_CPU_IO_PPI      *This,
  IN  UINT64                        Address,
  IN  UINT16                        Data
  );

/**
  32-bit memory write operations.

  @param  PeiServices    An indirect pointer to the PEI Services Table published by the PEI Foundation.
  @param  This           Pointer to local data for the interface.
  @param  Address        The physical address of the access.
  @param  Data           The data to write.

**/
VOID
EFIAPI
PeiDefaultMemWrite32 (
  IN  CONST EFI_PEI_SERVICES        **PeiServices,
  IN  CONST EFI_PEI_CPU_IO_PPI      *This,
  IN  UINT64                        Address,
  IN  UINT32                        Data
  );

/**
  64-bit memory write operations.

  @param  PeiServices    An indirect pointer to the PEI Services Table published by the PEI Foundation.
  @param  This           Pointer to local data for the interface.
  @param  Address        The physical address of the access.
  @param  Data           The data to write.

**/
VOID
EFIAPI
PeiDefaultMemWrite64 (
  IN  CONST EFI_PEI_SERVICES        **PeiServices,
  IN  CONST EFI_PEI_CPU_IO_PPI      *This,
  IN  UINT64                        Address,
  IN  UINT64                        Data
  );

extern EFI_PEI_CPU_IO_PPI gPeiDefaultCpuIoPpi;

//
// Default EFI_PEI_PCI_CFG2_PPI support for EFI_PEI_SERVICES table when PeiCore initialization.
//

/**
  Reads from a given location in the PCI configuration space.

  If the EFI_PEI_PCI_CFG2_PPI is not installed by platform/chipset PEIM, then
  return EFI_NOT_YET_AVAILABLE.

  @param  PeiServices     An indirect pointer to the PEI Services Table published by the PEI Foundation.
  @param  This            Pointer to local data for the interface.
  @param  Width           The width of the access. Enumerated in bytes.
                          See EFI_PEI_PCI_CFG_PPI_WIDTH above.
  @param  Address         The physical address of the access. The format of
                          the address is described by EFI_PEI_PCI_CFG_PPI_PCI_ADDRESS.
  @param  Buffer          A pointer to the buffer of data.

  @retval EFI_SUCCESS           The function completed successfully.
  @retval EFI_INVALID_PARAMETER The invalid access width.
  @retval EFI_NOT_YET_AVAILABLE If the EFI_PEI_PCI_CFG2_PPI is not installed by platform/chipset PEIM.

**/
EFI_STATUS
EFIAPI
PeiDefaultPciCfg2Read (
  IN CONST  EFI_PEI_SERVICES          **PeiServices,
  IN CONST  EFI_PEI_PCI_CFG2_PPI      *This,
  IN        EFI_PEI_PCI_CFG_PPI_WIDTH Width,
  IN        UINT64                    Address,
  IN OUT    VOID                      *Buffer
  );

/**
  Write to a given location in the PCI configuration space.

  If the EFI_PEI_PCI_CFG2_PPI is not installed by platform/chipset PEIM, then
  return EFI_NOT_YET_AVAILABLE.

  @param  PeiServices     An indirect pointer to the PEI Services Table published by the PEI Foundation.
  @param  This            Pointer to local data for the interface.
  @param  Width           The width of the access. Enumerated in bytes.
                          See EFI_PEI_PCI_CFG_PPI_WIDTH above.
  @param  Address         The physical address of the access. The format of
                          the address is described by EFI_PEI_PCI_CFG_PPI_PCI_ADDRESS.
  @param  Buffer          A pointer to the buffer of data.

  @retval EFI_SUCCESS           The function completed successfully.
  @retval EFI_INVALID_PARAMETER The invalid access width.
  @retval EFI_NOT_YET_AVAILABLE If the EFI_PEI_PCI_CFG2_PPI is not installed by platform/chipset PEIM.
**/
EFI_STATUS
EFIAPI
PeiDefaultPciCfg2Write (
  IN CONST  EFI_PEI_SERVICES          **PeiServices,
  IN CONST  EFI_PEI_PCI_CFG2_PPI      *This,
  IN        EFI_PEI_PCI_CFG_PPI_WIDTH Width,
  IN        UINT64                    Address,
  IN OUT    VOID                      *Buffer
  );

/**
  This function performs a read-modify-write operation on the contents from a given
  location in the PCI configuration space.

  @param  PeiServices     An indirect pointer to the PEI Services Table
                          published by the PEI Foundation.
  @param  This            Pointer to local data for the interface.
  @param  Width           The width of the access. Enumerated in bytes. Type
                          EFI_PEI_PCI_CFG_PPI_WIDTH is defined in Read().
  @param  Address         The physical address of the access.
  @param  SetBits         Points to value to bitwise-OR with the read configuration value.
                          The size of the value is determined by Width.
  @param  ClearBits       Points to the value to negate and bitwise-AND with the read configuration value.
                          The size of the value is determined by Width.

  @retval EFI_SUCCESS           The function completed successfully.
  @retval EFI_INVALID_PARAMETER The invalid access width.
  @retval EFI_NOT_YET_AVAILABLE If the EFI_PEI_PCI_CFG2_PPI is not installed by platform/chipset PEIM.
**/
EFI_STATUS
EFIAPI
PeiDefaultPciCfg2Modify (
  IN CONST  EFI_PEI_SERVICES          **PeiServices,
  IN CONST  EFI_PEI_PCI_CFG2_PPI      *This,
  IN        EFI_PEI_PCI_CFG_PPI_WIDTH Width,
  IN        UINT64                    Address,
  IN        VOID                      *SetBits,
  IN        VOID                      *ClearBits
  );

extern EFI_PEI_PCI_CFG2_PPI gPeiDefaultPciCfg2Ppi;

/**
  After PeiCore image is shadowed into permanent memory, all build-in FvPpi should
  be re-installed with the instance in permanent memory and all cached FvPpi pointers in
  PrivateData->Fv[] array should be fixed up to be pointed to the one in permanent
  memory.

  @param PrivateData   Pointer to PEI_CORE_INSTANCE.
**/
VOID
PeiReinitializeFv (
  IN  PEI_CORE_INSTANCE           *PrivateData
  );

#endif
