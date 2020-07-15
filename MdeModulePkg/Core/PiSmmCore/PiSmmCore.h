/** @file
  The internal header file includes the common header files, defines
  internal structure and functions used by SmmCore module.

  Copyright (c) 2009 - 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _SMM_CORE_H_
#define _SMM_CORE_H_

#include <PiSmm.h>

#include <Protocol/DxeSmmReadyToLock.h>
#include <Protocol/SmmReadyToLock.h>
#include <Protocol/SmmEndOfDxe.h>
#include <Protocol/CpuIo2.h>
#include <Protocol/SmmCommunication.h>
#include <Protocol/SmmAccess2.h>
#include <Protocol/FirmwareVolume2.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/DevicePath.h>
#include <Protocol/Security.h>
#include <Protocol/Security2.h>
#include <Protocol/SmmExitBootServices.h>
#include <Protocol/SmmLegacyBoot.h>
#include <Protocol/SmmReadyToBoot.h>
#include <Protocol/SmmMemoryAttribute.h>
#include <Protocol/SmmSxDispatch2.h>

#include <Guid/Apriori.h>
#include <Guid/EventGroup.h>
#include <Guid/EventLegacyBios.h>
#include <Guid/MemoryProfile.h>
#include <Guid/LoadModuleAtFixedAddress.h>
#include <Guid/SmiHandlerProfile.h>
#include <Guid/EndOfS3Resume.h>
#include <Guid/S3SmmInitDone.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/PeCoffLib.h>
#include <Library/PeCoffGetEntryPointLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/DebugLib.h>
#include <Library/ReportStatusCodeLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DevicePathLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/PcdLib.h>
#include <Library/SmmCorePlatformHookLib.h>
#include <Library/PerformanceLib.h>
#include <Library/HobLib.h>
#include <Library/SmmMemLib.h>

#include "PiSmmCorePrivateData.h"
#include "HeapGuard.h"

//
// Used to build a table of SMI Handlers that the SMM Core registers
//
typedef struct {
  EFI_SMM_HANDLER_ENTRY_POINT2  Handler;
  EFI_GUID                      *HandlerType;
  EFI_HANDLE                    DispatchHandle;
  BOOLEAN                       UnRegister;
} SMM_CORE_SMI_HANDLERS;

//
// SMM_HANDLER - used for each SMM handler
//

#define SMI_ENTRY_SIGNATURE  SIGNATURE_32('s','m','i','e')

 typedef struct {
  UINTN       Signature;
  LIST_ENTRY  AllEntries;  // All entries

  EFI_GUID    HandlerType; // Type of interrupt
  LIST_ENTRY  SmiHandlers; // All handlers
} SMI_ENTRY;

#define SMI_HANDLER_SIGNATURE  SIGNATURE_32('s','m','i','h')

 typedef struct {
  UINTN                         Signature;
  LIST_ENTRY                    Link;        // Link on SMI_ENTRY.SmiHandlers
  EFI_SMM_HANDLER_ENTRY_POINT2  Handler;     // The smm handler's entry point
  UINTN                         CallerAddr;  // The address of caller who register the SMI handler.
  SMI_ENTRY                     *SmiEntry;
  VOID                          *Context;    // for profile
  UINTN                         ContextSize; // for profile
} SMI_HANDLER;

//
// Structure for recording the state of an SMM Driver
//
#define EFI_SMM_DRIVER_ENTRY_SIGNATURE SIGNATURE_32('s', 'd','r','v')

typedef struct {
  UINTN                           Signature;
  LIST_ENTRY                      Link;             // mDriverList

  LIST_ENTRY                      ScheduledLink;    // mScheduledQueue

  EFI_HANDLE                      FvHandle;
  EFI_GUID                        FileName;
  EFI_DEVICE_PATH_PROTOCOL        *FvFileDevicePath;
  EFI_FIRMWARE_VOLUME2_PROTOCOL   *Fv;

  VOID                            *Depex;
  UINTN                           DepexSize;

  BOOLEAN                         Before;
  BOOLEAN                         After;
  EFI_GUID                        BeforeAfterGuid;

  BOOLEAN                         Dependent;
  BOOLEAN                         Scheduled;
  BOOLEAN                         Initialized;
  BOOLEAN                         DepexProtocolError;

  EFI_HANDLE                      ImageHandle;
  EFI_LOADED_IMAGE_PROTOCOL       *LoadedImage;
  //
  // Image EntryPoint in SMRAM
  //
  PHYSICAL_ADDRESS                ImageEntryPoint;
  //
  // Image Buffer in SMRAM
  //
  PHYSICAL_ADDRESS                ImageBuffer;
  //
  // Image Page Number
  //
  UINTN                           NumberOfPage;
  EFI_HANDLE                      SmmImageHandle;
  EFI_LOADED_IMAGE_PROTOCOL       SmmLoadedImage;
} EFI_SMM_DRIVER_ENTRY;

#define EFI_HANDLE_SIGNATURE            SIGNATURE_32('s','h','d','l')

///
/// IHANDLE - contains a list of protocol handles
///
typedef struct {
  UINTN               Signature;
  /// All handles list of IHANDLE
  LIST_ENTRY          AllHandles;
  /// List of PROTOCOL_INTERFACE's for this handle
  LIST_ENTRY          Protocols;
  UINTN               LocateRequest;
} IHANDLE;

#define ASSERT_IS_HANDLE(a)  ASSERT((a)->Signature == EFI_HANDLE_SIGNATURE)

#define PROTOCOL_ENTRY_SIGNATURE        SIGNATURE_32('s','p','t','e')

///
/// PROTOCOL_ENTRY - each different protocol has 1 entry in the protocol
/// database.  Each handler that supports this protocol is listed, along
/// with a list of registered notifies.
///
typedef struct {
  UINTN               Signature;
  /// Link Entry inserted to mProtocolDatabase
  LIST_ENTRY          AllEntries;
  /// ID of the protocol
  EFI_GUID            ProtocolID;
  /// All protocol interfaces
  LIST_ENTRY          Protocols;
  /// Registered notification handlers
  LIST_ENTRY          Notify;
} PROTOCOL_ENTRY;

#define PROTOCOL_INTERFACE_SIGNATURE  SIGNATURE_32('s','p','i','f')

///
/// PROTOCOL_INTERFACE - each protocol installed on a handle is tracked
/// with a protocol interface structure
///
typedef struct {
  UINTN                       Signature;
  /// Link on IHANDLE.Protocols
  LIST_ENTRY                  Link;
  /// Back pointer
  IHANDLE                     *Handle;
  /// Link on PROTOCOL_ENTRY.Protocols
  LIST_ENTRY                  ByProtocol;
  /// The protocol ID
  PROTOCOL_ENTRY              *Protocol;
  /// The interface value
  VOID                        *Interface;
} PROTOCOL_INTERFACE;

#define PROTOCOL_NOTIFY_SIGNATURE       SIGNATURE_32('s','p','t','n')

///
/// PROTOCOL_NOTIFY - used for each register notification for a protocol
///
typedef struct {
  UINTN               Signature;
  PROTOCOL_ENTRY      *Protocol;
  /// All notifications for this protocol
  LIST_ENTRY          Link;
  /// Notification function
  EFI_SMM_NOTIFY_FN   Function;
  /// Last position notified
  LIST_ENTRY          *Position;
} PROTOCOL_NOTIFY;

//
// SMM Core Global Variables
//
extern SMM_CORE_PRIVATE_DATA  *gSmmCorePrivate;
extern EFI_SMM_SYSTEM_TABLE2  gSmmCoreSmst;
extern LIST_ENTRY             gHandleList;
extern EFI_PHYSICAL_ADDRESS   gLoadModuleAtFixAddressSmramBase;

/**
  Called to initialize the memory service.

  @param   SmramRangeCount       Number of SMRAM Regions
  @param   SmramRanges           Pointer to SMRAM Descriptors

**/
VOID
SmmInitializeMemoryServices (
  IN UINTN                 SmramRangeCount,
  IN EFI_SMRAM_DESCRIPTOR  *SmramRanges
  );

/**
  The SmmInstallConfigurationTable() function is used to maintain the list
  of configuration tables that are stored in the System Management System
  Table.  The list is stored as an array of (GUID, Pointer) pairs.  The list
  must be allocated from pool memory with PoolType set to EfiRuntimeServicesData.

  @param  SystemTable      A pointer to the SMM System Table (SMST).
  @param  Guid             A pointer to the GUID for the entry to add, update, or remove.
  @param  Table            A pointer to the buffer of the table to add.
  @param  TableSize        The size of the table to install.

  @retval EFI_SUCCESS           The (Guid, Table) pair was added, updated, or removed.
  @retval EFI_INVALID_PARAMETER Guid is not valid.
  @retval EFI_NOT_FOUND         An attempt was made to delete a non-existent entry.
  @retval EFI_OUT_OF_RESOURCES  There is not enough memory available to complete the operation.

**/
EFI_STATUS
EFIAPI
SmmInstallConfigurationTable (
  IN  CONST EFI_SMM_SYSTEM_TABLE2  *SystemTable,
  IN  CONST EFI_GUID              *Guid,
  IN  VOID                        *Table,
  IN  UINTN                       TableSize
  );

/**
  Wrapper function to SmmInstallProtocolInterfaceNotify.  This is the public API which
  Calls the private one which contains a BOOLEAN parameter for notifications

  @param  UserHandle             The handle to install the protocol handler on,
                                 or NULL if a new handle is to be allocated
  @param  Protocol               The protocol to add to the handle
  @param  InterfaceType          Indicates whether Interface is supplied in
                                 native form.
  @param  Interface              The interface for the protocol being added

  @return Status code

**/
EFI_STATUS
EFIAPI
SmmInstallProtocolInterface (
  IN OUT EFI_HANDLE     *UserHandle,
  IN EFI_GUID           *Protocol,
  IN EFI_INTERFACE_TYPE InterfaceType,
  IN VOID               *Interface
  );

/**
  Allocates pages from the memory map.

  @param  Type                   The type of allocation to perform
  @param  MemoryType             The type of memory to turn the allocated pages
                                 into
  @param  NumberOfPages          The number of pages to allocate
  @param  Memory                 A pointer to receive the base allocated memory
                                 address

  @retval EFI_INVALID_PARAMETER  Parameters violate checking rules defined in spec.
  @retval EFI_NOT_FOUND          Could not allocate pages match the requirement.
  @retval EFI_OUT_OF_RESOURCES   No enough pages to allocate.
  @retval EFI_SUCCESS            Pages successfully allocated.

**/
EFI_STATUS
EFIAPI
SmmAllocatePages (
  IN      EFI_ALLOCATE_TYPE         Type,
  IN      EFI_MEMORY_TYPE           MemoryType,
  IN      UINTN                     NumberOfPages,
  OUT     EFI_PHYSICAL_ADDRESS      *Memory
  );

/**
  Allocates pages from the memory map.

  @param  Type                   The type of allocation to perform
  @param  MemoryType             The type of memory to turn the allocated pages
                                 into
  @param  NumberOfPages          The number of pages to allocate
  @param  Memory                 A pointer to receive the base allocated memory
                                 address
  @param  NeedGuard              Flag to indicate Guard page is needed or not

  @retval EFI_INVALID_PARAMETER  Parameters violate checking rules defined in spec.
  @retval EFI_NOT_FOUND          Could not allocate pages match the requirement.
  @retval EFI_OUT_OF_RESOURCES   No enough pages to allocate.
  @retval EFI_SUCCESS            Pages successfully allocated.

**/
EFI_STATUS
EFIAPI
SmmInternalAllocatePages (
  IN      EFI_ALLOCATE_TYPE         Type,
  IN      EFI_MEMORY_TYPE           MemoryType,
  IN      UINTN                     NumberOfPages,
  OUT     EFI_PHYSICAL_ADDRESS      *Memory,
  IN      BOOLEAN                   NeedGuard
  );

/**
  Frees previous allocated pages.

  @param  Memory                 Base address of memory being freed
  @param  NumberOfPages          The number of pages to free

  @retval EFI_NOT_FOUND          Could not find the entry that covers the range
  @retval EFI_INVALID_PARAMETER  Address not aligned, Address is zero or NumberOfPages is zero.
  @return EFI_SUCCESS            Pages successfully freed.

**/
EFI_STATUS
EFIAPI
SmmFreePages (
  IN      EFI_PHYSICAL_ADDRESS      Memory,
  IN      UINTN                     NumberOfPages
  );

/**
  Frees previous allocated pages.

  @param  Memory                 Base address of memory being freed
  @param  NumberOfPages          The number of pages to free
  @param  IsGuarded              Flag to indicate if the memory is guarded
                                 or not

  @retval EFI_NOT_FOUND          Could not find the entry that covers the range
  @retval EFI_INVALID_PARAMETER  Address not aligned, Address is zero or NumberOfPages is zero.
  @return EFI_SUCCESS            Pages successfully freed.

**/
EFI_STATUS
EFIAPI
SmmInternalFreePages (
  IN      EFI_PHYSICAL_ADDRESS      Memory,
  IN      UINTN                     NumberOfPages,
  IN      BOOLEAN                   IsGuarded
  );

/**
  Allocate pool of a particular type.

  @param  PoolType               Type of pool to allocate
  @param  Size                   The amount of pool to allocate
  @param  Buffer                 The address to return a pointer to the allocated
                                 pool

  @retval EFI_INVALID_PARAMETER  PoolType not valid
  @retval EFI_OUT_OF_RESOURCES   Size exceeds max pool size or allocation failed.
  @retval EFI_SUCCESS            Pool successfully allocated.

**/
EFI_STATUS
EFIAPI
SmmAllocatePool (
  IN      EFI_MEMORY_TYPE           PoolType,
  IN      UINTN                     Size,
  OUT     VOID                      **Buffer
  );

/**
  Allocate pool of a particular type.

  @param  PoolType               Type of pool to allocate
  @param  Size                   The amount of pool to allocate
  @param  Buffer                 The address to return a pointer to the allocated
                                 pool

  @retval EFI_INVALID_PARAMETER  PoolType not valid
  @retval EFI_OUT_OF_RESOURCES   Size exceeds max pool size or allocation failed.
  @retval EFI_SUCCESS            Pool successfully allocated.

**/
EFI_STATUS
EFIAPI
SmmInternalAllocatePool (
  IN      EFI_MEMORY_TYPE           PoolType,
  IN      UINTN                     Size,
  OUT     VOID                      **Buffer
  );

/**
  Frees pool.

  @param  Buffer                 The allocated pool entry to free

  @retval EFI_INVALID_PARAMETER  Buffer is not a valid value.
  @retval EFI_SUCCESS            Pool successfully freed.

**/
EFI_STATUS
EFIAPI
SmmFreePool (
  IN      VOID                      *Buffer
  );

/**
  Frees pool.

  @param  Buffer                 The allocated pool entry to free

  @retval EFI_INVALID_PARAMETER  Buffer is not a valid value.
  @retval EFI_SUCCESS            Pool successfully freed.

**/
EFI_STATUS
EFIAPI
SmmInternalFreePool (
  IN      VOID                      *Buffer
  );

/**
  Installs a protocol interface into the boot services environment.

  @param  UserHandle             The handle to install the protocol handler on,
                                 or NULL if a new handle is to be allocated
  @param  Protocol               The protocol to add to the handle
  @param  InterfaceType          Indicates whether Interface is supplied in
                                 native form.
  @param  Interface              The interface for the protocol being added
  @param  Notify                 indicates whether notify the notification list
                                 for this protocol

  @retval EFI_INVALID_PARAMETER  Invalid parameter
  @retval EFI_OUT_OF_RESOURCES   No enough buffer to allocate
  @retval EFI_SUCCESS            Protocol interface successfully installed

**/
EFI_STATUS
SmmInstallProtocolInterfaceNotify (
  IN OUT EFI_HANDLE     *UserHandle,
  IN EFI_GUID           *Protocol,
  IN EFI_INTERFACE_TYPE InterfaceType,
  IN VOID               *Interface,
  IN BOOLEAN            Notify
  );

/**
  Uninstalls all instances of a protocol:interfacer from a handle.
  If the last protocol interface is remove from the handle, the
  handle is freed.

  @param  UserHandle             The handle to remove the protocol handler from
  @param  Protocol               The protocol, of protocol:interface, to remove
  @param  Interface              The interface, of protocol:interface, to remove

  @retval EFI_INVALID_PARAMETER  Protocol is NULL.
  @retval EFI_SUCCESS            Protocol interface successfully uninstalled.

**/
EFI_STATUS
EFIAPI
SmmUninstallProtocolInterface (
  IN EFI_HANDLE       UserHandle,
  IN EFI_GUID         *Protocol,
  IN VOID             *Interface
  );

/**
  Queries a handle to determine if it supports a specified protocol.

  @param  UserHandle             The handle being queried.
  @param  Protocol               The published unique identifier of the protocol.
  @param  Interface              Supplies the address where a pointer to the
                                 corresponding Protocol Interface is returned.

  @return The requested protocol interface for the handle

**/
EFI_STATUS
EFIAPI
SmmHandleProtocol (
  IN EFI_HANDLE       UserHandle,
  IN EFI_GUID         *Protocol,
  OUT VOID            **Interface
  );

/**
  Add a new protocol notification record for the request protocol.

  @param  Protocol               The requested protocol to add the notify
                                 registration
  @param  Function               Points to the notification function
  @param  Registration           Returns the registration record

  @retval EFI_INVALID_PARAMETER  Invalid parameter
  @retval EFI_SUCCESS            Successfully returned the registration record
                                 that has been added

**/
EFI_STATUS
EFIAPI
SmmRegisterProtocolNotify (
  IN  CONST EFI_GUID              *Protocol,
  IN  EFI_SMM_NOTIFY_FN           Function,
  OUT VOID                        **Registration
  );

/**
  Locates the requested handle(s) and returns them in Buffer.

  @param  SearchType             The type of search to perform to locate the
                                 handles
  @param  Protocol               The protocol to search for
  @param  SearchKey              Dependant on SearchType
  @param  BufferSize             On input the size of Buffer.  On output the
                                 size of data returned.
  @param  Buffer                 The buffer to return the results in

  @retval EFI_BUFFER_TOO_SMALL   Buffer too small, required buffer size is
                                 returned in BufferSize.
  @retval EFI_INVALID_PARAMETER  Invalid parameter
  @retval EFI_SUCCESS            Successfully found the requested handle(s) and
                                 returns them in Buffer.

**/
EFI_STATUS
EFIAPI
SmmLocateHandle (
  IN EFI_LOCATE_SEARCH_TYPE   SearchType,
  IN EFI_GUID                 *Protocol   OPTIONAL,
  IN VOID                     *SearchKey  OPTIONAL,
  IN OUT UINTN                *BufferSize,
  OUT EFI_HANDLE              *Buffer
  );

/**
  Return the first Protocol Interface that matches the Protocol GUID. If
  Registration is pasased in return a Protocol Instance that was just add
  to the system. If Registration is NULL return the first Protocol Interface
  you find.

  @param  Protocol               The protocol to search for
  @param  Registration           Optional Registration Key returned from
                                 RegisterProtocolNotify()
  @param  Interface              Return the Protocol interface (instance).

  @retval EFI_SUCCESS            If a valid Interface is returned
  @retval EFI_INVALID_PARAMETER  Invalid parameter
  @retval EFI_NOT_FOUND          Protocol interface not found

**/
EFI_STATUS
EFIAPI
SmmLocateProtocol (
  IN  EFI_GUID  *Protocol,
  IN  VOID      *Registration OPTIONAL,
  OUT VOID      **Interface
  );

/**
  Function returns an array of handles that support the requested protocol
  in a buffer allocated from pool. This is a version of SmmLocateHandle()
  that allocates a buffer for the caller.

  @param  SearchType             Specifies which handle(s) are to be returned.
  @param  Protocol               Provides the protocol to search by.    This
                                 parameter is only valid for SearchType
                                 ByProtocol.
  @param  SearchKey              Supplies the search key depending on the
                                 SearchType.
  @param  NumberHandles          The number of handles returned in Buffer.
  @param  Buffer                 A pointer to the buffer to return the requested
                                 array of  handles that support Protocol.

  @retval EFI_SUCCESS            The result array of handles was returned.
  @retval EFI_NOT_FOUND          No handles match the search.
  @retval EFI_OUT_OF_RESOURCES   There is not enough pool memory to store the
                                 matching results.
  @retval EFI_INVALID_PARAMETER  One or more parameters are not valid.

**/
EFI_STATUS
EFIAPI
SmmLocateHandleBuffer (
  IN     EFI_LOCATE_SEARCH_TYPE  SearchType,
  IN     EFI_GUID                *Protocol OPTIONAL,
  IN     VOID                    *SearchKey OPTIONAL,
  IN OUT UINTN                   *NumberHandles,
  OUT    EFI_HANDLE              **Buffer
  );

/**
  Manage SMI of a particular type.

  @param  HandlerType    Points to the handler type or NULL for root SMI handlers.
  @param  Context        Points to an optional context buffer.
  @param  CommBuffer     Points to the optional communication buffer.
  @param  CommBufferSize Points to the size of the optional communication buffer.

  @retval EFI_SUCCESS                        Interrupt source was processed successfully but not quiesced.
  @retval EFI_INTERRUPT_PENDING              One or more SMI sources could not be quiesced.
  @retval EFI_WARN_INTERRUPT_SOURCE_PENDING  Interrupt source was not handled or quiesced.
  @retval EFI_WARN_INTERRUPT_SOURCE_QUIESCED Interrupt source was handled and quiesced.

**/
EFI_STATUS
EFIAPI
SmiManage (
  IN     CONST EFI_GUID           *HandlerType,
  IN     CONST VOID               *Context         OPTIONAL,
  IN OUT VOID                     *CommBuffer      OPTIONAL,
  IN OUT UINTN                    *CommBufferSize  OPTIONAL
  );

/**
  Registers a handler to execute within SMM.

  @param  Handler        Handler service function pointer.
  @param  HandlerType    Points to the handler type or NULL for root SMI handlers.
  @param  DispatchHandle On return, contains a unique handle which can be used to later unregister the handler function.

  @retval EFI_SUCCESS           Handler register success.
  @retval EFI_INVALID_PARAMETER Handler or DispatchHandle is NULL.

**/
EFI_STATUS
EFIAPI
SmiHandlerRegister (
  IN   EFI_SMM_HANDLER_ENTRY_POINT2   Handler,
  IN   CONST EFI_GUID                 *HandlerType  OPTIONAL,
  OUT  EFI_HANDLE                     *DispatchHandle
  );

/**
  Unregister a handler in SMM.

  @param  DispatchHandle  The handle that was specified when the handler was registered.

  @retval EFI_SUCCESS           Handler function was successfully unregistered.
  @retval EFI_INVALID_PARAMETER DispatchHandle does not refer to a valid handle.

**/
EFI_STATUS
EFIAPI
SmiHandlerUnRegister (
  IN  EFI_HANDLE                      DispatchHandle
  );

/**
  This function is the main entry point for an SMM handler dispatch
  or communicate-based callback.

  @param  DispatchHandle  The unique handle assigned to this handler by SmiHandlerRegister().
  @param  Context         Points to an optional handler context which was specified when the handler was registered.
  @param  CommBuffer      A pointer to a collection of data in memory that will
                          be conveyed from a non-SMM environment into an SMM environment.
  @param  CommBufferSize  The size of the CommBuffer.

  @return Status Code

**/
EFI_STATUS
EFIAPI
SmmDriverDispatchHandler (
  IN     EFI_HANDLE               DispatchHandle,
  IN     CONST VOID               *Context,        OPTIONAL
  IN OUT VOID                     *CommBuffer,     OPTIONAL
  IN OUT UINTN                    *CommBufferSize  OPTIONAL
  );

/**
  This function is the main entry point for an SMM handler dispatch
  or communicate-based callback.

  @param  DispatchHandle  The unique handle assigned to this handler by SmiHandlerRegister().
  @param  Context         Points to an optional handler context which was specified when the handler was registered.
  @param  CommBuffer      A pointer to a collection of data in memory that will
                          be conveyed from a non-SMM environment into an SMM environment.
  @param  CommBufferSize  The size of the CommBuffer.

  @return Status Code

**/
EFI_STATUS
EFIAPI
SmmLegacyBootHandler (
  IN     EFI_HANDLE               DispatchHandle,
  IN     CONST VOID               *Context,        OPTIONAL
  IN OUT VOID                     *CommBuffer,     OPTIONAL
  IN OUT UINTN                    *CommBufferSize  OPTIONAL
  );

/**
  This function is the main entry point for an SMM handler dispatch
  or communicate-based callback.

  @param  DispatchHandle  The unique handle assigned to this handler by SmiHandlerRegister().
  @param  Context         Points to an optional handler context which was specified when the handler was registered.
  @param  CommBuffer      A pointer to a collection of data in memory that will
                          be conveyed from a non-SMM environment into an SMM environment.
  @param  CommBufferSize  The size of the CommBuffer.

  @return Status Code

**/
EFI_STATUS
EFIAPI
SmmReadyToLockHandler (
  IN     EFI_HANDLE               DispatchHandle,
  IN     CONST VOID               *Context,        OPTIONAL
  IN OUT VOID                     *CommBuffer,     OPTIONAL
  IN OUT UINTN                    *CommBufferSize  OPTIONAL
  );

/**
  This function is the main entry point for an SMM handler dispatch
  or communicate-based callback.

  @param  DispatchHandle  The unique handle assigned to this handler by SmiHandlerRegister().
  @param  Context         Points to an optional handler context which was specified when the handler was registered.
  @param  CommBuffer      A pointer to a collection of data in memory that will
                          be conveyed from a non-SMM environment into an SMM environment.
  @param  CommBufferSize  The size of the CommBuffer.

  @return Status Code

**/
EFI_STATUS
EFIAPI
SmmEndOfDxeHandler (
  IN     EFI_HANDLE               DispatchHandle,
  IN     CONST VOID               *Context,        OPTIONAL
  IN OUT VOID                     *CommBuffer,     OPTIONAL
  IN OUT UINTN                    *CommBufferSize  OPTIONAL
  );

/**
  This function is the main entry point for an SMM handler dispatch
  or communicate-based callback.

  @param  DispatchHandle  The unique handle assigned to this handler by SmiHandlerRegister().
  @param  Context         Points to an optional handler context which was specified when the handler was registered.
  @param  CommBuffer      A pointer to a collection of data in memory that will
                          be conveyed from a non-SMM environment into an SMM environment.
  @param  CommBufferSize  The size of the CommBuffer.

  @return Status Code

**/
EFI_STATUS
EFIAPI
SmmExitBootServicesHandler (
  IN     EFI_HANDLE               DispatchHandle,
  IN     CONST VOID               *Context,        OPTIONAL
  IN OUT VOID                     *CommBuffer,     OPTIONAL
  IN OUT UINTN                    *CommBufferSize  OPTIONAL
  );

/**
  This function is the main entry point for an SMM handler dispatch
  or communicate-based callback.

  @param  DispatchHandle  The unique handle assigned to this handler by SmiHandlerRegister().
  @param  Context         Points to an optional handler context which was specified when the handler was registered.
  @param  CommBuffer      A pointer to a collection of data in memory that will
                          be conveyed from a non-SMM environment into an SMM environment.
  @param  CommBufferSize  The size of the CommBuffer.

  @return Status Code

**/
EFI_STATUS
EFIAPI
SmmReadyToBootHandler (
  IN     EFI_HANDLE               DispatchHandle,
  IN     CONST VOID               *Context,        OPTIONAL
  IN OUT VOID                     *CommBuffer,     OPTIONAL
  IN OUT UINTN                    *CommBufferSize  OPTIONAL
  );

/**
  Software SMI handler that is called when the S3SmmInitDone signal is triggered.
  This function installs the SMM S3SmmInitDone Protocol so SMM Drivers are informed that
  S3 SMM initialization has been done.

  @param  DispatchHandle  The unique handle assigned to this handler by SmiHandlerRegister().
  @param  Context         Points to an optional handler context which was specified when the handler was registered.
  @param  CommBuffer      A pointer to a collection of data in memory that will
                          be conveyed from a non-SMM environment into an SMM environment.
  @param  CommBufferSize  The size of the CommBuffer.

  @return Status Code

**/
EFI_STATUS
EFIAPI
SmmS3SmmInitDoneHandler (
  IN     EFI_HANDLE  DispatchHandle,
  IN     CONST VOID  *Context,        OPTIONAL
  IN OUT VOID        *CommBuffer,     OPTIONAL
  IN OUT UINTN       *CommBufferSize  OPTIONAL
  );

/**
  Software SMI handler that is called when the EndOfS3Resume event is trigged.
  This function installs the SMM EndOfS3Resume Protocol so SMM Drivers are informed that
  S3 resume has finished.

  @param  DispatchHandle  The unique handle assigned to this handler by SmiHandlerRegister().
  @param  Context         Points to an optional handler context which was specified when the handler was registered.
  @param  CommBuffer      A pointer to a collection of data in memory that will
                          be conveyed from a non-SMM environment into an SMM environment.
  @param  CommBufferSize  The size of the CommBuffer.

  @return Status Code

**/
EFI_STATUS
EFIAPI
SmmEndOfS3ResumeHandler (
  IN     EFI_HANDLE  DispatchHandle,
  IN     CONST VOID  *Context,        OPTIONAL
  IN OUT VOID        *CommBuffer,     OPTIONAL
  IN OUT UINTN       *CommBufferSize  OPTIONAL
  );

/**
  Place holder function until all the SMM System Table Service are available.

  @param  Arg1                   Undefined
  @param  Arg2                   Undefined
  @param  Arg3                   Undefined
  @param  Arg4                   Undefined
  @param  Arg5                   Undefined

  @return EFI_NOT_AVAILABLE_YET

**/
EFI_STATUS
EFIAPI
SmmEfiNotAvailableYetArg5 (
  UINTN Arg1,
  UINTN Arg2,
  UINTN Arg3,
  UINTN Arg4,
  UINTN Arg5
  );

//
//Functions used during debug builds
//

/**
  Traverse the discovered list for any drivers that were discovered but not loaded
  because the dependency expressions evaluated to false.

**/
VOID
SmmDisplayDiscoveredNotDispatched (
  VOID
  );

/**
  Add free SMRAM region for use by memory service.

  @param  MemBase                Base address of memory region.
  @param  MemLength              Length of the memory region.
  @param  Type                   Memory type.
  @param  Attributes             Memory region state.

**/
VOID
SmmAddMemoryRegion (
  IN      EFI_PHYSICAL_ADDRESS      MemBase,
  IN      UINT64                    MemLength,
  IN      EFI_MEMORY_TYPE           Type,
  IN      UINT64                    Attributes
  );

/**
  Finds the protocol entry for the requested protocol.

  @param  Protocol               The ID of the protocol
  @param  Create                 Create a new entry if not found

  @return Protocol entry

**/
PROTOCOL_ENTRY  *
SmmFindProtocolEntry (
  IN EFI_GUID   *Protocol,
  IN BOOLEAN    Create
  );

/**
  Signal event for every protocol in protocol entry.

  @param  Prot                   Protocol interface

**/
VOID
SmmNotifyProtocol (
  IN PROTOCOL_INTERFACE   *Prot
  );

/**
  Finds the protocol instance for the requested handle and protocol.
  Note: This function doesn't do parameters checking, it's caller's responsibility
  to pass in valid parameters.

  @param  Handle                 The handle to search the protocol on
  @param  Protocol               GUID of the protocol
  @param  Interface              The interface for the protocol being searched

  @return Protocol instance (NULL: Not found)

**/
PROTOCOL_INTERFACE *
SmmFindProtocolInterface (
  IN IHANDLE        *Handle,
  IN EFI_GUID       *Protocol,
  IN VOID           *Interface
  );

/**
  Removes Protocol from the protocol list (but not the handle list).

  @param  Handle                 The handle to remove protocol on.
  @param  Protocol               GUID of the protocol to be moved
  @param  Interface              The interface of the protocol

  @return Protocol Entry

**/
PROTOCOL_INTERFACE *
SmmRemoveInterfaceFromProtocol (
  IN IHANDLE        *Handle,
  IN EFI_GUID       *Protocol,
  IN VOID           *Interface
  );

/**
  This is the POSTFIX version of the dependency evaluator.  This code does
  not need to handle Before or After, as it is not valid to call this
  routine in this case. POSTFIX means all the math is done on top of the stack.

  @param  DriverEntry           DriverEntry element to update.

  @retval TRUE                  If driver is ready to run.
  @retval FALSE                 If driver is not ready to run or some fatal error
                                was found.

**/
BOOLEAN
SmmIsSchedulable (
  IN  EFI_SMM_DRIVER_ENTRY   *DriverEntry
  );

//
// SmramProfile
//

/**
  Initialize SMRAM profile.

**/
VOID
SmramProfileInit (
  VOID
  );

/**
  Install SMRAM profile protocol.

**/
VOID
SmramProfileInstallProtocol (
  VOID
  );

/**
  Register SMM image to SMRAM profile.

  @param DriverEntry    SMM image info.
  @param RegisterToDxe  Register image to DXE.

  @return EFI_SUCCESS           Register successfully.
  @return EFI_UNSUPPORTED       Memory profile unsupported,
                                or memory profile for the image is not required.
  @return EFI_OUT_OF_RESOURCES  No enough resource for this register.

**/
EFI_STATUS
RegisterSmramProfileImage (
  IN EFI_SMM_DRIVER_ENTRY   *DriverEntry,
  IN BOOLEAN                RegisterToDxe
  );

/**
  Unregister image from SMRAM profile.

  @param DriverEntry        SMM image info.
  @param UnregisterToDxe    Unregister image from DXE.

  @return EFI_SUCCESS           Unregister successfully.
  @return EFI_UNSUPPORTED       Memory profile unsupported,
                                or memory profile for the image is not required.
  @return EFI_NOT_FOUND         The image is not found.

**/
EFI_STATUS
UnregisterSmramProfileImage (
  IN EFI_SMM_DRIVER_ENTRY   *DriverEntry,
  IN BOOLEAN                UnregisterToDxe
  );

/**
  Update SMRAM profile information.

  @param CallerAddress  Address of caller who call Allocate or Free.
  @param Action         This Allocate or Free action.
  @param MemoryType     Memory type.
                        EfiMaxMemoryType means the MemoryType is unknown.
  @param Size           Buffer size.
  @param Buffer         Buffer address.
  @param ActionString   String for memory profile action.
                        Only needed for user defined allocate action.

  @return EFI_SUCCESS           Memory profile is updated.
  @return EFI_UNSUPPORTED       Memory profile is unsupported,
                                or memory profile for the image is not required,
                                or memory profile for the memory type is not required.
  @return EFI_ACCESS_DENIED     It is during memory profile data getting.
  @return EFI_ABORTED           Memory profile recording is not enabled.
  @return EFI_OUT_OF_RESOURCES  No enough resource to update memory profile for allocate action.
  @return EFI_NOT_FOUND         No matched allocate info found for free action.

**/
EFI_STATUS
EFIAPI
SmmCoreUpdateProfile (
  IN PHYSICAL_ADDRESS       CallerAddress,
  IN MEMORY_PROFILE_ACTION  Action,
  IN EFI_MEMORY_TYPE        MemoryType, // Valid for AllocatePages/AllocatePool
  IN UINTN                  Size,       // Valid for AllocatePages/FreePages/AllocatePool
  IN VOID                   *Buffer,
  IN CHAR8                  *ActionString OPTIONAL
  );

/**
  Register SMRAM profile handler.

**/
VOID
RegisterSmramProfileHandler (
  VOID
  );

/**
  SMRAM profile ready to lock callback function.

**/
VOID
SmramProfileReadyToLock (
  VOID
  );

/**
  Initialize MemoryAttributes support.
**/
VOID
EFIAPI
SmmCoreInitializeMemoryAttributesTable (
  VOID
  );

/**
  This function returns a copy of the current memory map. The map is an array of
  memory descriptors, each of which describes a contiguous block of memory.

  @param[in, out]  MemoryMapSize          A pointer to the size, in bytes, of the
                                          MemoryMap buffer. On input, this is the size of
                                          the buffer allocated by the caller.  On output,
                                          it is the size of the buffer returned by the
                                          firmware  if the buffer was large enough, or the
                                          size of the buffer needed  to contain the map if
                                          the buffer was too small.
  @param[in, out]  MemoryMap              A pointer to the buffer in which firmware places
                                          the current memory map.
  @param[out]      MapKey                 A pointer to the location in which firmware
                                          returns the key for the current memory map.
  @param[out]      DescriptorSize         A pointer to the location in which firmware
                                          returns the size, in bytes, of an individual
                                          EFI_MEMORY_DESCRIPTOR.
  @param[out]      DescriptorVersion      A pointer to the location in which firmware
                                          returns the version number associated with the
                                          EFI_MEMORY_DESCRIPTOR.

  @retval EFI_SUCCESS            The memory map was returned in the MemoryMap
                                 buffer.
  @retval EFI_BUFFER_TOO_SMALL   The MemoryMap buffer was too small. The current
                                 buffer size needed to hold the memory map is
                                 returned in MemoryMapSize.
  @retval EFI_INVALID_PARAMETER  One of the parameters has an invalid value.

**/
EFI_STATUS
EFIAPI
SmmCoreGetMemoryMap (
  IN OUT UINTN                  *MemoryMapSize,
  IN OUT EFI_MEMORY_DESCRIPTOR  *MemoryMap,
  OUT UINTN                     *MapKey,
  OUT UINTN                     *DescriptorSize,
  OUT UINT32                    *DescriptorVersion
  );

/**
  Initialize SmiHandler profile feature.
**/
VOID
SmmCoreInitializeSmiHandlerProfile (
  VOID
  );

/**
  This function is called by SmmChildDispatcher module to report
  a new SMI handler is registered, to SmmCore.

  @param This            The protocol instance
  @param HandlerGuid     The GUID to identify the type of the handler.
                         For the SmmChildDispatch protocol, the HandlerGuid
                         must be the GUID of SmmChildDispatch protocol.
  @param Handler         The SMI handler.
  @param CallerAddress   The address of the module who registers the SMI handler.
  @param Context         The context of the SMI handler.
                         For the SmmChildDispatch protocol, the Context
                         must match the one defined for SmmChildDispatch protocol.
  @param ContextSize     The size of the context in bytes.
                         For the SmmChildDispatch protocol, the Context
                         must match the one defined for SmmChildDispatch protocol.

  @retval EFI_SUCCESS           The information is recorded.
  @retval EFI_OUT_OF_RESOURCES  There is no enough resource to record the information.
**/
EFI_STATUS
EFIAPI
SmiHandlerProfileRegisterHandler (
  IN SMI_HANDLER_PROFILE_PROTOCOL   *This,
  IN EFI_GUID                       *HandlerGuid,
  IN EFI_SMM_HANDLER_ENTRY_POINT2   Handler,
  IN PHYSICAL_ADDRESS               CallerAddress,
  IN VOID                           *Context, OPTIONAL
  IN UINTN                          ContextSize OPTIONAL
  );

/**
  This function is called by SmmChildDispatcher module to report
  an existing SMI handler is unregistered, to SmmCore.

  @param This            The protocol instance
  @param HandlerGuid     The GUID to identify the type of the handler.
                         For the SmmChildDispatch protocol, the HandlerGuid
                         must be the GUID of SmmChildDispatch protocol.
  @param Handler         The SMI handler.
  @param Context         The context of the SMI handler.
                         If it is NOT NULL, it will be used to check what is registered.
  @param ContextSize     The size of the context in bytes.
                         If Context is NOT NULL, it will be used to check what is registered.

  @retval EFI_SUCCESS           The original record is removed.
  @retval EFI_NOT_FOUND         There is no record for the HandlerGuid and handler.
**/
EFI_STATUS
EFIAPI
SmiHandlerProfileUnregisterHandler (
  IN SMI_HANDLER_PROFILE_PROTOCOL   *This,
  IN EFI_GUID                       *HandlerGuid,
  IN EFI_SMM_HANDLER_ENTRY_POINT2   Handler,
  IN VOID                           *Context, OPTIONAL
  IN UINTN                          ContextSize OPTIONAL
  );

extern UINTN                    mFullSmramRangeCount;
extern EFI_SMRAM_DESCRIPTOR     *mFullSmramRanges;

extern EFI_SMM_DRIVER_ENTRY       *mSmmCoreDriverEntry;

extern EFI_LOADED_IMAGE_PROTOCOL  *mSmmCoreLoadedImage;

//
// Page management
//

typedef struct {
  LIST_ENTRY  Link;
  UINTN       NumberOfPages;
} FREE_PAGE_LIST;

extern LIST_ENTRY  mSmmMemoryMap;

//
// Pool management
//

//
// MIN_POOL_SHIFT must not be less than 5
//
#define MIN_POOL_SHIFT  6
#define MIN_POOL_SIZE   (1 << MIN_POOL_SHIFT)

//
// MAX_POOL_SHIFT must not be less than EFI_PAGE_SHIFT - 1
//
#define MAX_POOL_SHIFT  (EFI_PAGE_SHIFT - 1)
#define MAX_POOL_SIZE   (1 << MAX_POOL_SHIFT)

//
// MAX_POOL_INDEX are calculated by maximum and minimum pool sizes
//
#define MAX_POOL_INDEX  (MAX_POOL_SHIFT - MIN_POOL_SHIFT + 1)

#define POOL_HEAD_SIGNATURE   SIGNATURE_32('s','p','h','d')

typedef struct {
  UINT32            Signature;
  BOOLEAN           Available;
  EFI_MEMORY_TYPE   Type;
  UINTN             Size;
} POOL_HEADER;

#define POOL_TAIL_SIGNATURE   SIGNATURE_32('s','p','t','l')

typedef struct {
  UINT32            Signature;
  UINT32            Reserved;
  UINTN             Size;
} POOL_TAIL;

#define POOL_OVERHEAD (sizeof(POOL_HEADER) + sizeof(POOL_TAIL))

#define HEAD_TO_TAIL(a)   \
  ((POOL_TAIL *) (((CHAR8 *) (a)) + (a)->Size - sizeof(POOL_TAIL)));

typedef struct {
  POOL_HEADER  Header;
  LIST_ENTRY   Link;
} FREE_POOL_HEADER;

typedef enum {
  SmmPoolTypeCode,
  SmmPoolTypeData,
  SmmPoolTypeMax,
} SMM_POOL_TYPE;

extern LIST_ENTRY  mSmmPoolLists[SmmPoolTypeMax][MAX_POOL_INDEX];

/**
  Internal Function. Allocate n pages from given free page node.

  @param  Pages                  The free page node.
  @param  NumberOfPages          Number of pages to be allocated.
  @param  MaxAddress             Request to allocate memory below this address.

  @return Memory address of allocated pages.

**/
UINTN
InternalAllocPagesOnOneNode (
  IN OUT FREE_PAGE_LIST   *Pages,
  IN     UINTN            NumberOfPages,
  IN     UINTN            MaxAddress
  );

/**
  Update SMM memory map entry.

  @param[in]  Type                   The type of allocation to perform.
  @param[in]  Memory                 The base of memory address.
  @param[in]  NumberOfPages          The number of pages to allocate.
  @param[in]  AddRegion              If this memory is new added region.
**/
VOID
ConvertSmmMemoryMapEntry (
  IN EFI_MEMORY_TYPE       Type,
  IN EFI_PHYSICAL_ADDRESS  Memory,
  IN UINTN                 NumberOfPages,
  IN BOOLEAN               AddRegion
  );

/**
  Internal function.  Moves any memory descriptors that are on the
  temporary descriptor stack to heap.

**/
VOID
CoreFreeMemoryMapStack (
  VOID
  );

/**
  Frees previous allocated pages.

  @param[in]  Memory                 Base address of memory being freed.
  @param[in]  NumberOfPages          The number of pages to free.
  @param[in]  AddRegion              If this memory is new added region.

  @retval EFI_NOT_FOUND          Could not find the entry that covers the range.
  @retval EFI_INVALID_PARAMETER  Address not aligned, Address is zero or NumberOfPages is zero.
  @return EFI_SUCCESS            Pages successfully freed.

**/
EFI_STATUS
SmmInternalFreePagesEx (
  IN EFI_PHYSICAL_ADDRESS  Memory,
  IN UINTN                 NumberOfPages,
  IN BOOLEAN               AddRegion
  );

/**
  Hook function used to set all Guard pages after entering SMM mode.
**/
VOID
SmmEntryPointMemoryManagementHook (
  VOID
  );

#endif
