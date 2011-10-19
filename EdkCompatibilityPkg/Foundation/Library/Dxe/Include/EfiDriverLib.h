/*++

Copyright (c) 2004 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  EfiDriverLib.h

Abstract:

  Light weight lib to support EFI drivers.

--*/

#ifndef _EFI_DRIVER_LIB_H_
#define _EFI_DRIVER_LIB_H_

#include "EfiStatusCode.h"
#include "EfiCommonLib.h"
#include "EfiPerf.h"
#include "LinkedList.h"
#include "GetImage.h"
#include "EfiImageFormat.h"
#include "EfiCompNameSupport.h"

#include EFI_GUID_DEFINITION (DxeServices)
#include EFI_GUID_DEFINITION (EventGroup)
#include EFI_GUID_DEFINITION (EventLegacyBios)
#include EFI_GUID_DEFINITION (FrameworkDevicePath)
#include EFI_PROTOCOL_DEFINITION (FirmwareVolume)
#include EFI_PROTOCOL_DEFINITION (FirmwareVolume2)
#include EFI_PROTOCOL_DEFINITION (DataHub)
#include EFI_PROTOCOL_DEFINITION (DriverBinding)
#include EFI_PROTOCOL_DEFINITION (ComponentName)
#include EFI_PROTOCOL_DEFINITION (ComponentName2)
#include EFI_PROTOCOL_DEFINITION (DriverConfiguration)
#include EFI_PROTOCOL_DEFINITION (DriverConfiguration2)
#include EFI_PROTOCOL_DEFINITION (DriverDiagnostics)
#include EFI_PROTOCOL_DEFINITION (DriverDiagnostics2)

#include EFI_PROTOCOL_DEFINITION (DebugMask)

#if defined(__GNUC__) && defined(ECP_CPU_IPF)

VOID
EFIAPI
EcpEfiBreakPoint (
  VOID
  )
/*++

Routine Description:

  Generates a breakpoint on the CPU.

  Generates a breakpoint on the CPU. The breakpoint must be implemented such
  that code can resume normal execution after the breakpoint.

Arguments:

  VOID

Returns: 

  VOID

--*/
;

VOID
EFIAPI
EcpMemoryFence (
  VOID
  )
/*++

Routine Description:

  Used to serialize load and store operations.

  All loads and stores that proceed calls to this function are guaranteed to be
  globally visible when this function returns.

Arguments:

  VOID

Returns: 

  VOID

--*/
;

#endif

typedef struct {
  CHAR8   *Language;
  CHAR16  *UnicodeString;
} EFI_UNICODE_STRING_TABLE;
#if (EFI_SPECIFICATION_VERSION >= 0x00020000)
#define LANGUAGE_CODE_ENGLISH    "en-US"
#else
#define LANGUAGE_CODE_ENGLISH    "eng"
#endif

//
// Macros for EFI Driver Library Functions that are really EFI Boot Services
//
#define EfiCopyMem(_Destination, _Source, _Length)  gBS->CopyMem ((_Destination), (_Source), (_Length))
#define EfiSetMem(_Destination, _Length, _Value)    gBS->SetMem ((_Destination), (_Length), (_Value))
#define EfiZeroMem(_Destination, _Length)           gBS->SetMem ((_Destination), (_Length), 0)

//
// Driver Lib Globals.
//
extern EFI_BOOT_SERVICES        *gBS;
extern EFI_DXE_SERVICES         *gDS;
extern EFI_RUNTIME_SERVICES     *gRT;
extern EFI_SYSTEM_TABLE         *gST;
extern UINTN                    gErrorLevel;
extern EFI_GUID                 gEfiCallerIdGuid;
extern EFI_DEBUG_MASK_PROTOCOL  *gDebugMaskInterface;

EFI_STATUS
EfiInitializeDriverLib (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
/*++

Routine Description:

  Intialize Driver Lib if it has not yet been initialized. 

Arguments:

  ImageHandle     - The firmware allocated handle for the EFI image.
  
  SystemTable     - A pointer to the EFI System Table.


Returns: 

  EFI_STATUS always returns EFI_SUCCESS

--*/
;

EFI_STATUS
DxeInitializeDriverLib (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
/*++

Routine Description:

  Intialize Driver Lib if it has not yet been initialized. 

Arguments:

  ImageHandle     - The firmware allocated handle for the EFI image.
  
  SystemTable     - A pointer to the EFI System Table.

Returns: 

  EFI_STATUS always returns EFI_SUCCESS

--*/
;

EFI_STATUS
EfiLibInstallDriverBinding (
  IN EFI_HANDLE                   ImageHandle,
  IN EFI_SYSTEM_TABLE             *SystemTable,
  IN EFI_DRIVER_BINDING_PROTOCOL  *DriverBinding,
  IN EFI_HANDLE                   DriverBindingHandle
  )
/*++

Routine Description:

  Intialize a driver by installing the Driver Binding Protocol onto the 
  driver's DriverBindingHandle.  This is typically the same as the driver's
  ImageHandle, but it can be different if the driver produces multiple
  DriverBinding Protocols.  This function also initializes the EFI Driver
  Library that initializes the global variables gST, gBS, gRT.

Arguments:

  ImageHandle         - The image handle of the driver

  SystemTable         - The EFI System Table that was passed to the driver's entry point

  DriverBinding       - A Driver Binding Protocol instance that this driver is producing

  DriverBindingHandle - The handle that DriverBinding is to be installe onto.  If this
                        parameter is NULL, then a new handle is created.

Returns: 

  EFI_SUCCESS is DriverBinding is installed onto DriverBindingHandle

  Otherwise, then return status from gBS->InstallProtocolInterface()

--*/
;

EFI_STATUS
EfiLibInstallAllDriverProtocols (
  IN EFI_HANDLE                         ImageHandle,
  IN EFI_SYSTEM_TABLE                   *SystemTable,
  IN EFI_DRIVER_BINDING_PROTOCOL        *DriverBinding,
  IN EFI_HANDLE                         DriverBindingHandle,
  IN EFI_COMPONENT_NAME_PROTOCOL        *ComponentName, OPTIONAL
  IN EFI_DRIVER_CONFIGURATION_PROTOCOL  *DriverConfiguration, OPTIONAL
  IN EFI_DRIVER_DIAGNOSTICS_PROTOCOL    *DriverDiagnostics OPTIONAL
  )
/*++

Routine Description:

  Intialize a driver by installing the Driver Binding Protocol onto the 
  driver's DriverBindingHandle.  This is typically the same as the driver's
  ImageHandle, but it can be different if the driver produces multiple
  DriverBinding Protocols.  This function also initializes the EFI Driver
  Library that initializes the global variables gST, gBS, gRT.

Arguments:

  ImageHandle         - The image handle of the driver

  SystemTable         - The EFI System Table that was passed to the driver's entry point

  DriverBinding       - A Driver Binding Protocol instance that this driver is producing

  DriverBindingHandle - The handle that DriverBinding is to be installe onto.  If this
                        parameter is NULL, then a new handle is created.

  ComponentName       - A Component Name Protocol instance that this driver is producing

  DriverConfiguration - A Driver Configuration Protocol instance that this driver is producing
  
  DriverDiagnostics   - A Driver Diagnostics Protocol instance that this driver is producing

Returns: 

  EFI_SUCCESS if all the protocols were installed onto DriverBindingHandle

  Otherwise, then return status from gBS->InstallProtocolInterface()

--*/
;

EFI_STATUS
EfiLibInstallAllDriverProtocols2 (
  IN EFI_HANDLE                         ImageHandle,
  IN EFI_SYSTEM_TABLE                   *SystemTable,
  IN EFI_DRIVER_BINDING_PROTOCOL        *DriverBinding,
  IN EFI_HANDLE                         DriverBindingHandle,
  IN EFI_COMPONENT_NAME2_PROTOCOL       *ComponentName2, OPTIONAL
  IN EFI_DRIVER_CONFIGURATION2_PROTOCOL *DriverConfiguration2, OPTIONAL
  IN EFI_DRIVER_DIAGNOSTICS2_PROTOCOL   *DriverDiagnostics2 OPTIONAL
  )
/*++

Routine Description:

  Intialize a driver by installing the Driver Binding Protocol onto the 
  driver's DriverBindingHandle.  This is typically the same as the driver's
  ImageHandle, but it can be different if the driver produces multiple
  DriverBinding Protocols.  This function also initializes the EFI Driver
  Library that initializes the global variables gST, gBS, gRT.

Arguments:

  ImageHandle         - The image handle of the driver

  SystemTable         - The EFI System Table that was passed to the driver's entry point

  DriverBinding       - A Driver Binding Protocol instance that this driver is producing

  DriverBindingHandle - The handle that DriverBinding is to be installe onto.  If this
                        parameter is NULL, then a new handle is created.

  ComponentName2      - A Component Name2 Protocol instance that this driver is producing

  DriverConfiguration2- A Driver Configuration2 Protocol instance that this driver is producing
  
  DriverDiagnostics2  - A Driver Diagnostics2 Protocol instance that this driver is producing

Returns: 

  EFI_SUCCESS if all the protocols were installed onto DriverBindingHandle

  Otherwise, then return status from gBS->InstallProtocolInterface()

--*/
;

EFI_STATUS
EfiLibGetSystemConfigurationTable (
  IN EFI_GUID *TableGuid,
  OUT VOID **Table
  )
/*++

Routine Description:
  
  Return the EFI 1.0 System Tabl entry with TableGuid

Arguments:

  TableGuid - Name of entry to return in the system table
  Table     - Pointer in EFI system table associated with TableGuid

Returns: 

  EFI_SUCCESS - Table returned;
  EFI_NOT_FOUND - TableGuid not in EFI system table

--*/
;

BOOLEAN
EfiLibCompareLanguage (
  CHAR8  *Language1,
  CHAR8  *Language2
  )
/*++

Routine Description:

  Compare two languages to say whether they are identical.

Arguments:

  Language1 - first language
  Language2 - second language

Returns:

  TRUE      - identical
  FALSE     - not identical

--*/
;

//
// DevicePath.c
//
BOOLEAN
EfiIsDevicePathMultiInstance (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevicePath
  )
/*++

Routine Description:
  Return TRUE is this is a multi instance device path.

Arguments:
  DevicePath  - A pointer to a device path data structure.


Returns:
  TRUE - If DevicePath is multi instance. 
  FALSE - If DevicePath is not multi instance.

--*/
;

EFI_DEVICE_PATH_PROTOCOL        *
EfiDevicePathInstance (
  IN OUT EFI_DEVICE_PATH_PROTOCOL   **DevicePath,
  OUT UINTN                         *Size
  )
/*++

Routine Description:
  Function retrieves the next device path instance from a device path data structure.

Arguments:
  DevicePath           - A pointer to a device path data structure.

  Size                 - A pointer to the size of a device path instance in bytes.

Returns:

  This function returns a pointer to the current device path instance.
  In addition, it returns the size in bytes of the current device path instance in Size,
  and a pointer to the next device path instance in DevicePath.
  If there are no more device path instances in DevicePath, then DevicePath will be set to NULL.

--*/
;

UINTN
EfiDevicePathSize (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevPath
  )
/*++

Routine Description:

  Calculate the size of a whole device path.    
    
Arguments:

  DevPath - The pointer to the device path data.
    
Returns:

  Size of device path data structure..

--*/
;

EFI_DEVICE_PATH_PROTOCOL        *
EfiAppendDevicePath (
  IN EFI_DEVICE_PATH_PROTOCOL  *Src1,
  IN EFI_DEVICE_PATH_PROTOCOL  *Src2
  )
/*++

Routine Description:
  Function is used to append a Src1 and Src2 together.

Arguments:
  Src1  - A pointer to a device path data structure.

  Src2  - A pointer to a device path data structure.

Returns:

  A pointer to the new device path is returned.
  NULL is returned if space for the new device path could not be allocated from pool.
  It is up to the caller to free the memory used by Src1 and Src2 if they are no longer needed.

--*/
;

EFI_DEVICE_PATH_PROTOCOL        *
EfiDevicePathFromHandle (
  IN EFI_HANDLE       Handle
  )
/*++

Routine Description:

  Locate device path protocol interface on a device handle.

Arguments:

  Handle  - The device handle

Returns:

  Device path protocol interface located.

--*/
;

EFI_DEVICE_PATH_PROTOCOL        *
EfiDuplicateDevicePath (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevPath
  )
/*++

Routine Description:
  Duplicate a new device path data structure from the old one.

Arguments:
  DevPath  - A pointer to a device path data structure.

Returns:
  A pointer to the new allocated device path data.
  Caller must free the memory used by DevicePath if it is no longer needed.

--*/
;

EFI_DEVICE_PATH_PROTOCOL        *
EfiAppendDevicePathNode (
  IN EFI_DEVICE_PATH_PROTOCOL  *Src1,
  IN EFI_DEVICE_PATH_PROTOCOL  *Src2
  )
/*++

Routine Description:
  Function is used to append a device path node to the end of another device path.

Arguments:
  Src1  - A pointer to a device path data structure.

  Src2 - A pointer to a device path data structure.

Returns:
  This function returns a pointer to the new device path.
  If there is not enough temporary pool memory available to complete this function,
  then NULL is returned.


--*/
;

EFI_DEVICE_PATH_PROTOCOL        *
EfiFileDevicePath (
  IN EFI_HANDLE               Device  OPTIONAL,
  IN CHAR16                   *FileName
  )
/*++

Routine Description:
  Create a device path that appends a MEDIA_DEVICE_PATH with
  FileNameGuid to the device path of DeviceHandle.

Arguments:
  Device   - Optional Device Handle to use as Root of the Device Path

  FileName - FileName

Returns:
  EFI_DEVICE_PATH_PROTOCOL that was allocated from dynamic memory
  or NULL pointer.

--*/
;

EFI_DEVICE_PATH_PROTOCOL        *
EfiAppendDevicePathInstance (
  IN EFI_DEVICE_PATH_PROTOCOL  *Src,
  IN EFI_DEVICE_PATH_PROTOCOL  *Instance
  )
/*++

Routine Description:

  Append a device path instance to another.

Arguments:

  Src       - The device path instance to be appended with.
  Instance  - The device path instance appending the other.

Returns:

  The contaction of these two.

--*/
;

//
// Lock.c
//
typedef struct {
  EFI_TPL Tpl;
  EFI_TPL OwnerTpl;
  UINTN   Lock;
} EFI_LOCK;

VOID
EfiInitializeLock (
  IN OUT EFI_LOCK *Lock,
  IN EFI_TPL      Priority
  )
/*++

Routine Description:

  Initialize a basic mutual exclusion lock.   Each lock
  provides mutual exclusion access at it's task priority
  level.  Since there is no-premption (at any TPL) or
  multiprocessor support, acquiring the lock only consists
  of raising to the locks TPL.

  Note on a check build ASSERT()s are used to ensure proper
  lock usage.
    
Arguments:

  Lock        - The EFI_LOCK structure to initialize

  Priority    - The task priority level of the lock

    
Returns:

  An initialized Efi Lock structure.

--*/
;

//
// Macro to initialize the state of a lock when a lock variable is declared
//
#define EFI_INITIALIZE_LOCK_VARIABLE(Tpl) {Tpl,0,0}

VOID
EfiAcquireLock (
  IN EFI_LOCK *Lock
  )
/*++

Routine Description:

  Raising to the task priority level of the mutual exclusion
  lock, and then acquires ownership of the lock.
    
Arguments:

  Lock - The lock to acquire
    
Returns:

  None

--*/
;

EFI_STATUS
EfiAcquireLockOrFail (
  IN EFI_LOCK  *Lock
  )
/*++

Routine Description:

  Initialize a basic mutual exclusion lock.   Each lock
  provides mutual exclusion access at it's task priority
  level.  Since there is no-premption (at any TPL) or
  multiprocessor support, acquiring the lock only consists
  of raising to the locks TPL.
    
Arguments:

  Lock        - The EFI_LOCK structure to initialize
   
Returns:

  EFI_SUCCESS       - Lock Owned.
  EFI_ACCESS_DENIED - Reentrant Lock Acquisition, Lock not Owned.

--*/
;

VOID
EfiReleaseLock (
  IN EFI_LOCK *Lock
  )
/*++

Routine Description:

    Releases ownership of the mutual exclusion lock, and
    restores the previous task priority level.
    
Arguments:

    Lock - The lock to release
    
Returns:

    None

--*/
;

VOID *
EfiLibAllocatePool (
  IN  UINTN   AllocationSize
  )
/*++

Routine Description:

  Allocate EfiBootServicesData pool of size AllocationSize

Arguments:

  AllocationSize  - Pool size

Returns:

  Pointer to the pool allocated

--*/
;

VOID *
EfiLibAllocateRuntimePool (
  IN  UINTN   AllocationSize
  )
/*++

Routine Description:

  Allocate EfiRuntimeServicesData pool of size AllocationSize

Arguments:

  AllocationSize  - Pool size

Returns:

  Pointer to the pool allocated

--*/
;

VOID *
EfiLibAllocateZeroPool (
  IN  UINTN   AllocationSize
  )
/*++

Routine Description:

  Allocate EfiBootServicesData pool of size AllocationSize and set memory to zero.

Arguments:

  AllocationSize  - Pool size

Returns:

  Pointer to the pool allocated

--*/
;

VOID *
EfiLibAllocateRuntimeZeroPool (
  IN  UINTN   AllocationSize
  )
/*++

Routine Description:

  Allocate EfiRuntimeServicesData pool of size AllocationSize and set memory to zero.

Arguments:

  AllocationSize  - Pool size

Returns:

  Pointer to the pool allocated

--*/
;

VOID *
EfiLibAllocateCopyPool (
  IN  UINTN   AllocationSize,
  IN  VOID    *Buffer
  )
/*++

Routine Description:

  Allocate BootServicesData pool and use a buffer provided by 
  caller to fill it.

Arguments:

  AllocationSize  - The size to allocate
  
  Buffer          - Buffer that will be filled into the buffer allocated

Returns:

  Pointer of the buffer allocated.

--*/
;

VOID *
EfiLibAllocateRuntimeCopyPool (
  IN  UINTN            AllocationSize,
  IN  VOID             *Buffer
  )
/*++

Routine Description:

  Allocate RuntimeServicesData pool and use a buffer provided by 
  caller to fill it.

Arguments:

  AllocationSize  - The size to allocate
  
  Buffer          - Buffer that will be filled into the buffer allocated

Returns:

  Pointer of the buffer allocated.

--*/
;

//
// Event.c
//
EFI_EVENT
EfiLibCreateProtocolNotifyEvent (
  IN EFI_GUID             *ProtocolGuid,
  IN EFI_TPL              NotifyTpl,
  IN EFI_EVENT_NOTIFY     NotifyFunction,
  IN VOID                 *NotifyContext,
  OUT VOID                **Registration
  )
/*++

Routine Description:

  Create a protocol notification event and return it.

Arguments:

  ProtocolGuid    - Protocol to register notification event on.

  NotifyTpl       - Maximum TPL to single the NotifyFunction.

  NotifyFunction  - EFI notification routine.

  NotifyContext   - Context passed into Event when it is created.

  Registration    - Registration key returned from RegisterProtocolNotify().

Returns:

  The EFI_EVENT that has been registered to be signaled when a ProtocolGuid
  is added to the system.

--*/
;

EFI_STATUS
EfiLibNamedEventSignal (
  IN EFI_GUID            *Name
  )
/*++

Routine Description:
  Signals a named event. All registered listeners will run.
  The listeners should register using EfiLibNamedEventListen() function.

  NOTE: For now, the named listening/signalling is implemented
  on a protocol interface being installed and uninstalled.
  In the future, this maybe implemented based on a dedicated mechanism.

Arguments:
  Name - Name to perform the signaling on. The name is a GUID.

Returns:
  EFI_SUCCESS if successfull.

--*/
;

EFI_STATUS
EfiLibNamedEventListen (
  IN EFI_GUID             * Name,
  IN EFI_TPL              NotifyTpl,
  IN EFI_EVENT_NOTIFY     NotifyFunction,
  IN VOID                 *NotifyContext
  )
/*++

Routine Description:
  Listenes to signals on the name.
  EfiLibNamedEventSignal() signals the event.

  NOTE: For now, the named listening/signalling is implemented
  on a protocol interface being installed and uninstalled.
  In the future, this maybe implemented based on a dedicated mechanism.

Arguments:
  Name            - Name to register the listener on.
  NotifyTpl       - Maximum TPL to singnal the NotifyFunction.
  NotifyFunction  - The listener routine.
  NotifyContext   - Context passed into the listener routine.

Returns:
  EFI_SUCCESS if successful.

--*/
;

//
// Handle.c
//
EFI_STATUS
EfiLibLocateHandleProtocolByProtocols (
  IN OUT EFI_HANDLE        * Handle, OPTIONAL
  OUT    VOID              **Interface, OPTIONAL
  ...
  )
/*++
Routine Description:

  Function locates Protocol and/or Handle on which all Protocols specified
  as a variable list are installed.
  It supports continued search. The caller must assure that no handles are added
  or removed while performing continued search, by e.g., rising the TPL and not
  calling any handle routines. Otherwise the behavior is undefined.

Arguments:

  Handle        - The address of handle to receive the handle on which protocols
                  indicated by the variable list are installed.
                  If points to NULL, all handles are searched. If pointing to a
                  handle returned from previous call, searches starting from next handle.
                  If NULL, the parameter is ignored.

  Interface     - The address of a pointer to a protocol interface that will receive
                  the interface indicated by first variable argument.
                  If NULL, the parameter is ignored.

  ...           - A variable argument list containing protocol GUIDs. Must end with NULL.

Returns:

  EFI_SUCCESS  - All the protocols where found on same handle.
  EFI_NOT_FOUND - A Handle with all the protocols installed was not found.
  Other values as may be returned from LocateHandleBuffer() or HandleProtocol().

--*/
;

//
// Debug.c init
//
EFI_STATUS
EfiDebugAssertInit (
  VOID
  )
/*++

Routine Description:
  
  Locate Debug Assert Protocol and set as mDebugAssert

Arguments:

  None

Returns:

  Status code

--*/
;

//
// Unicode String Support
//
EFI_STATUS
EfiLibLookupUnicodeString (
  CHAR8                     *Language,
  CHAR8                     *SupportedLanguages,
  EFI_UNICODE_STRING_TABLE  *UnicodeStringTable,
  CHAR16                    **UnicodeString
  )
/*++

Routine Description:

  Translate a unicode string to a specified language if supported.
  
Arguments:

  Language              - The name of language to translate to
  SupportedLanguages    - Supported languages set
  UnicodeStringTable    - Pointer of one item in translation dictionary
  UnicodeString         - The translated string

Returns: 

  EFI_INVALID_PARAMETER - Invalid parameter
  EFI_UNSUPPORTED       - System not supported this language or this string translation
  EFI_SUCCESS           - String successfully translated

--*/
;

EFI_STATUS
EfiLibAddUnicodeString (
  CHAR8                     *Language,
  CHAR8                     *SupportedLanguages,
  EFI_UNICODE_STRING_TABLE  **UnicodeStringTable,
  CHAR16                    *UnicodeString
  )
/*++

Routine Description:

  Add an translation to the dictionary if this language if supported.
  
Arguments:

  Language              - The name of language to translate to
  SupportedLanguages    - Supported languages set
  UnicodeStringTable    - Translation dictionary
  UnicodeString         - The corresponding string for the language to be translated to

Returns: 

  EFI_INVALID_PARAMETER - Invalid parameter
  EFI_UNSUPPORTED       - System not supported this language
  EFI_ALREADY_STARTED   - Already has a translation item of this language
  EFI_OUT_OF_RESOURCES  - No enough buffer to be allocated
  EFI_SUCCESS           - String successfully translated

--*/
;

EFI_STATUS
EfiLibFreeUnicodeStringTable (
  EFI_UNICODE_STRING_TABLE  *UnicodeStringTable
  )
/*++

Routine Description:

  Free a string table.

Arguments:

  UnicodeStringTable      - The string table to be freed.

Returns: 

  EFI_SUCCESS       - The table successfully freed.

--*/
;

EFI_STATUS
EfiLibReportStatusCode (
  IN EFI_STATUS_CODE_TYPE     Type,
  IN EFI_STATUS_CODE_VALUE    Value,
  IN UINT32                   Instance,
  IN EFI_GUID                 *CallerId OPTIONAL,
  IN EFI_STATUS_CODE_DATA     *Data     OPTIONAL  
  )
/*++

Routine Description:

  Report status code.

Arguments:

  Type        - Code type
  Value       - Code value
  Instance    - Instance number
  CallerId    - Caller name
  DevicePath  - Device path that to be reported

Returns:

  Status code.

  EFI_OUT_OF_RESOURCES - No enough buffer could be allocated

--*/
;

EFI_STATUS
ReportStatusCodeWithDevicePath (
  IN EFI_STATUS_CODE_TYPE     Type,
  IN EFI_STATUS_CODE_VALUE    Value,
  IN UINT32                   Instance,
  IN EFI_GUID                 * CallerId OPTIONAL,
  IN EFI_DEVICE_PATH_PROTOCOL * DevicePath
  )
/*++

Routine Description:

  Report device path through status code.

Arguments:

  Type        - Code type
  Value       - Code value
  Instance    - Instance number
  CallerId    - Caller name
  DevicePath  - Device path that to be reported

Returns:

  Status code.

  EFI_OUT_OF_RESOURCES - No enough buffer could be allocated

--*/
;

EFI_STATUS
EFIAPI
EfiCreateEventLegacyBoot (
  IN EFI_TPL                      NotifyTpl,
  IN EFI_EVENT_NOTIFY             NotifyFunction,
  IN VOID                         *NotifyContext,
  OUT EFI_EVENT                   *LegacyBootEvent
  )
/*++

Routine Description:
  Create a Legacy Boot Event.  
  Tiano extended the CreateEvent Type enum to add a legacy boot event type. 
  This was bad as Tiano did not own the enum. In UEFI 2.0 CreateEventEx was
  added and now it's possible to not voilate the UEFI specification by 
  declaring a GUID for the legacy boot event class. This library supports
  the EFI 1.10 form and UEFI 2.0 form and allows common code to work both ways.

Arguments:
  LegacyBootEvent  Returns the EFI event returned from gBS->CreateEvent(Ex)

Returns:
  EFI_SUCCESS   Event was created.
  Other         Event was not created.

--*/
;

EFI_STATUS
EFIAPI
EfiCreateEventReadyToBoot (
  IN EFI_TPL                      NotifyTpl,
  IN EFI_EVENT_NOTIFY             NotifyFunction,
  IN VOID                         *NotifyContext,
  OUT EFI_EVENT                   *ReadyToBootEvent
  )
/*++

Routine Description:
  Create a Read to Boot Event.  
  
  Tiano extended the CreateEvent Type enum to add a ready to boot event type. 
  This was bad as Tiano did not own the enum. In UEFI 2.0 CreateEventEx was
  added and now it's possible to not voilate the UEFI specification and use 
  the ready to boot event class defined in UEFI 2.0. This library supports
  the EFI 1.10 form and UEFI 2.0 form and allows common code to work both ways.

Arguments:
  @param LegacyBootEvent  Returns the EFI event returned from gBS->CreateEvent(Ex)

Return:
  EFI_SUCCESS   - Event was created.
  Other         - Event was not created.

--*/
;

VOID
EFIAPI
EfiInitializeFwVolDevicepathNode (
  IN  MEDIA_FW_VOL_FILEPATH_DEVICE_PATH     *FvDevicePathNode,
  IN EFI_GUID                               *NameGuid
  )
/*++
Routine Description:
  Initialize a Firmware Volume (FV) Media Device Path node.
  
Arguments:
  FvDevicePathNode   - Pointer to a FV device path node to initialize
  NameGuid           - FV file name to use in FvDevicePathNode

--*/
;

EFI_GUID *
EFIAPI
EfiGetNameGuidFromFwVolDevicePathNode (
  IN  MEDIA_FW_VOL_FILEPATH_DEVICE_PATH   *FvDevicePathNode
  )
/*++
Routine Description:
  Check to see if the Firmware Volume (FV) Media Device Path is valid.
  
Arguments:
  FvDevicePathNode   - Pointer to FV device path to check

Return:
  NULL    - FvDevicePathNode is not valid.
  Other   - FvDevicePathNode is valid and pointer to NameGuid was returned.

--*/
;

VOID
EfiLibSafeFreePool (
  IN  VOID             *Buffer
  )
/*++

Routine Description:

  Free pool safely.

Arguments:
  
  Buffer          - The allocated pool entry to free

Returns:

  Pointer of the buffer allocated.

--*/
;

EFI_STATUS
EfiLibTestManagedDevice (
  IN EFI_HANDLE       ControllerHandle,
  IN EFI_HANDLE       DriverBindingHandle,
  IN EFI_GUID         *ManagedProtocolGuid
  )
/*++

Routine Description:

  Test to see if the controller is managed by a specific driver.

Arguments:

  ControllerHandle          - Handle for controller to test

  DriverBindingHandle       - Driver binding handle for controller

  ManagedProtocolGuid       - The protocol guid the driver opens on controller

Returns: 

  EFI_SUCCESS     - The controller is managed by the driver

  EFI_UNSUPPORTED - The controller is not managed by the driver

--*/
;

EFI_STATUS
EfiLibTestChildHandle (
  IN EFI_HANDLE       ControllerHandle,
  IN EFI_HANDLE       ChildHandle,
  IN EFI_GUID         *ConsumedGuid
  )
/*++

Routine Description:

  Test to see if the child handle is the child of the controller

Arguments:

  ControllerHandle          - Handle for controller (parent)

  ChildHandle               - Child handle to test

  ConsumsedGuid             - Protocol guid consumed by child from controller

Returns: 

  EFI_SUCCESS     - The child handle is the child of the controller

  EFI_UNSUPPORTED - The child handle is not the child of the controller

--*/
;
#endif
