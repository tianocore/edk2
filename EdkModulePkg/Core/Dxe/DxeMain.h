/*++

Copyright (c) 2006 - 2007, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  DxeMain.h

Abstract:

Revision History

--*/

#ifndef _DXE_MAIN_H_
#define _DXE_MAIN_H_


#include "DebugImageInfo.h"
#include "Library.h"
#include "FwVolBlock.h"
#include "FwVolDriver.h"
#include "gcd.h"
#include "imem.h"
#include "Image.h"
#include "Exec.h"
#include "hand.h"

///
/// EFI_DEP_REPLACE_TRUE - Used to dynamically patch the dependecy expression
///                        to save time.  A EFI_DEP_PUSH is evauated one an
///                        replaced with EFI_DEP_REPLACE_TRUE
///
#define EFI_DEP_REPLACE_TRUE  0xff

///
/// Define the initial size of the dependency expression evaluation stack
///
#define DEPEX_STACK_SIZE_INCREMENT  0x1000

typedef struct {
  EFI_GUID                    *ProtocolGuid;
  VOID                        **Protocol;
  EFI_EVENT                   Event;
  VOID                        *Registration;
  BOOLEAN                     Present;
} ARCHITECTURAL_PROTOCOL_ENTRY;


//
// DXE Dispatcher Data structures
//

#define KNOWN_HANDLE_SIGNATURE  EFI_SIGNATURE_32('k','n','o','w')
typedef struct {
  UINTN           Signature;
  LIST_ENTRY      Link;         // mFvHandleList           
  EFI_HANDLE      Handle;
} KNOWN_HANDLE;


#define EFI_CORE_DRIVER_ENTRY_SIGNATURE EFI_SIGNATURE_32('d','r','v','r')
typedef struct {
  UINTN                           Signature;
  LIST_ENTRY                      Link;             // mDriverList

  LIST_ENTRY                      ScheduledLink;    // mScheduledQueue

  EFI_HANDLE                      FvHandle;
  EFI_GUID                        FileName;
  EFI_DEVICE_PATH_PROTOCOL        *FvFileDevicePath;
  EFI_FIRMWARE_VOLUME_PROTOCOL    *Fv;

  VOID                            *Depex;
  UINTN                           DepexSize;

  BOOLEAN                         Before;
  BOOLEAN                         After;
  EFI_GUID                        BeforeAfterGuid;

  BOOLEAN                         Dependent;
  BOOLEAN                         Unrequested;
  BOOLEAN                         Scheduled;
  BOOLEAN                         Untrusted;
  BOOLEAN                         Initialized;
  BOOLEAN                         DepexProtocolError;

  EFI_HANDLE                      ImageHandle;

} EFI_CORE_DRIVER_ENTRY;

//
//The data structure of GCD memory map entry
//
#define EFI_GCD_MAP_SIGNATURE  EFI_SIGNATURE_32('g','c','d','m')
typedef struct {
  UINTN                 Signature;
  LIST_ENTRY            Link;
  EFI_PHYSICAL_ADDRESS  BaseAddress;
  UINT64                EndAddress;
  UINT64                Capabilities;
  UINT64                Attributes;
  EFI_GCD_MEMORY_TYPE   GcdMemoryType;
  EFI_GCD_IO_TYPE       GcdIoType;
  EFI_HANDLE            ImageHandle;
  EFI_HANDLE            DeviceHandle;
} EFI_GCD_MAP_ENTRY;

//
// DXE Core Global Variables
//
extern EFI_SYSTEM_TABLE                         *gDxeCoreST;
extern EFI_BOOT_SERVICES                        *gDxeCoreBS;
extern EFI_RUNTIME_SERVICES                     *gDxeCoreRT;
extern EFI_DXE_SERVICES                         *gDxeCoreDS;
extern EFI_HANDLE                               gDxeCoreImageHandle;

extern EFI_DECOMPRESS_PROTOCOL                  gEfiDecompress;
extern EFI_PEI_PE_COFF_LOADER_PROTOCOL          *gEfiPeiPeCoffLoader;

extern EFI_RUNTIME_ARCH_PROTOCOL                *gRuntime;
extern EFI_CPU_ARCH_PROTOCOL                    *gCpu;
extern EFI_WATCHDOG_TIMER_ARCH_PROTOCOL         *gWatchdogTimer;
extern EFI_METRONOME_ARCH_PROTOCOL              *gMetronome;
extern EFI_TIMER_ARCH_PROTOCOL                  *gTimer;
extern EFI_SECURITY_ARCH_PROTOCOL               *gSecurity;
extern EFI_BDS_ARCH_PROTOCOL                    *gBds;
extern EFI_STATUS_CODE_PROTOCOL                 *gStatusCode;

extern EFI_TPL                                  gEfiCurrentTpl;

extern EFI_GUID                                 *gDxeCoreFileName;
extern EFI_LOADED_IMAGE_PROTOCOL                *gDxeCoreLoadedImage;

extern EFI_MEMORY_TYPE_INFORMATION              gMemoryTypeInformation[EfiMaxMemoryType + 1];

extern BOOLEAN                                  gDispatcherRunning;
extern EFI_RUNTIME_ARCH_PROTOCOL                gRuntimeTemplate;

//
// Service Initialization Functions
//


VOID
CoreInitializePool (
  VOID
  )
/*++

Routine Description:

  Called to initialize the pool.

Arguments:

  None

Returns:

  None

--*/
;

VOID
CoreAddMemoryDescriptor (
  IN EFI_MEMORY_TYPE       Type,
  IN EFI_PHYSICAL_ADDRESS  Start,
  IN UINT64                NumberOfPages,
  IN UINT64                Attribute
  )
/*++

Routine Description:

  Called to initialize the memory map and add descriptors to
  the current descriptor list.

       The first descriptor that is added must be general usable
  memory as the addition allocates heap.

Arguments:

  Type          - The type of memory to add

  Start         - The starting address in the memory range
                  Must be page aligned

  NumberOfPages - The number of pages in the range

  Attribute     - Attributes of the memory to add

Returns:

  None.  The range is added to the memory map

--*/
;

VOID
CoreReleaseGcdMemoryLock (
  VOID
  )
/*++

Routine Description:
    Release memory lock on mGcdMemorySpaceLock

Arguments:
    None

Returns:
    None

--*/
;

VOID
CoreAcquireGcdMemoryLock (
  VOID
  )
/*++

Routine Description:
    Acquire memory lock on mGcdMemorySpaceLock

Arguments:
    None

Returns:
    None

--*/
;

EFI_STATUS
CoreInitializeMemoryServices (
  IN VOID                  **HobStart,
  IN EFI_PHYSICAL_ADDRESS  *MemoryBaseAddress,
  IN UINT64                *MemoryLength
  )
/*++

Routine Description:

  External function. Initializes the GCD and memory services based on the memory 
  descriptor HOBs.  This function is responsible for priming the GCD map and the
  memory map, so memory allocations and resource allocations can be made.  The first
  part of this function can not depend on any memory services until at least one
  memory descriptor is provided to the memory services.  Then the memory services
  can be used to intialize the GCD map.

Arguments:

  HobStart - The start address of the HOB.
  
  MemoryBaseAddress   - Start address of memory region found to init DXE core.
  
  MemoryLength        - Length of memory region found to init DXE core.

Returns:

  EFI_SUCCESS         - Memory services successfully initialized.

--*/
;


EFI_STATUS
CoreInitializeGcdServices (
  IN VOID                  **HobStart,
  IN EFI_PHYSICAL_ADDRESS  MemoryBaseAddress,
  IN UINT64                MemoryLength
  )
/*++

Routine Description:

  External function. Initializes the GCD and memory services based on the memory 
  descriptor HOBs.  This function is responsible for priming the GCD map and the
  memory map, so memory allocations and resource allocations can be made.  The first
  part of this function can not depend on any memory services until at least one
  memory descriptor is provided to the memory services.  Then the memory services
  can be used to intialize the GCD map.

Arguments:

  HobStart - The start address of the HOB
  
  MemoryBaseAddress   - Start address of memory region found to init DXE core.
  
  MemoryLength        - Length of memory region found to init DXE core.


Returns:

  EFI_SUCCESS         - GCD services successfully initialized.

--*/
;

EFI_STATUS
CoreInitializeEventServices (
  VOID
  )
/*++

Routine Description:

  Initializes "event" support and populates parts of the System and Runtime Table.

Arguments:

  None
    
Returns:

  EFI_SUCCESS - Always return success

--*/
;

EFI_STATUS
CoreInitializeImageServices (
  IN  VOID *HobStart
  )
/*++

Routine Description:

  Add the Image Services to EFI Boot Services Table and install the protocol
  interfaces for this image.

Arguments:

  HobStart        - The HOB to initialize

Returns:

  Status code.

--*/
;

VOID
CoreNotifyOnArchProtocolInstallation (
  VOID
  )
/*++

Routine Description:
  Creates an event that is fired everytime a Protocol of a specific type is installed

Arguments:
  NONE

Returns:
  NONE

--*/
;

EFI_STATUS
CoreAllEfiServicesAvailable (
  VOID
  )
/*++

Routine Description:
  Return TRUE if all AP services are availible.

Arguments:
  NONE

Returns:
  EFI_SUCCESS   - All AP services are available
  EFI_NOT_FOUND - At least one AP service is not available 

--*/
;

VOID
CalculateEfiHdrCrc (
  IN  OUT EFI_TABLE_HEADER    *Hdr
  )
/*++

Routine Description:

  Calcualte the 32-bit CRC in a EFI table using the service provided by the
  gRuntime service.

Arguments:

  Hdr  - Pointer to an EFI standard header

Returns:

  None

--*/
;

VOID
EFIAPI
CoreTimerTick (
  IN UINT64     Duration
  )
/*++

Routine Description:

  Called by the platform code to process a tick.

Arguments:

  Duration    - The number of 100ns elasped since the last call to TimerTick
    
Returns:

  None

--*/
;

VOID
CoreInitializeDispatcher (
  VOID
  )
/*++

Routine Description:

  Initialize the dispatcher. Initialize the notification function that runs when
  a FV protocol is added to the system.

Arguments:

  NONE

Returns:

  NONE 

--*/
;

BOOLEAN
CoreIsSchedulable (
  IN  EFI_CORE_DRIVER_ENTRY   *DriverEntry  
  )
/*++

Routine Description:

  This is the POSTFIX version of the dependency evaluator.  This code does 
  not need to handle Before or After, as it is not valid to call this 
  routine in this case. The SOR is just ignored and is a nop in the grammer.

  POSTFIX means all the math is done on top of the stack.

Arguments:

  DriverEntry - DriverEntry element to update
  
Returns:

  TRUE - If driver is ready to run.

  FALSE - If driver is not ready to run or some fatal error was found.

--*/
;

EFI_STATUS
CorePreProcessDepex (
  IN  EFI_CORE_DRIVER_ENTRY   *DriverEntry  
  )
/*++

Routine Description:

  Preprocess dependency expression and update DriverEntry to reflect the
  state of  Before, After, and SOR dependencies. If DriverEntry->Before
  or DriverEntry->After is set it will never be cleared. If SOR is set
  it will be cleared by CoreSchedule(), and then the driver can be 
  dispatched.

Arguments:

  DriverEntry - DriverEntry element to update

Returns:

  EFI_SUCCESS - It always works.

--*/
;


EFI_STATUS
EFIAPI
CoreExitBootServices (
  IN EFI_HANDLE   ImageHandle,
  IN UINTN        MapKey
  )
/*++

Routine Description:

  EFI 1.0 API to terminate Boot Services

Arguments:

  ImageHandle - Handle that represents the identity of the calling image

  MapKey      -Key to the latest memory map.

Returns:

  EFI_SUCCESS - Boot Services terminated
  EFI_INVALID_PARAMETER - MapKey is incorrect.

--*/
;

EFI_STATUS
CoreTerminateMemoryMap (
  IN UINTN        MapKey
  )
/*++

Routine Description:

  Make sure the memory map is following all the construction rules, 
  it is the last time to check memory map error before exit boot services.

Arguments:

  MapKey        - Memory map key

Returns:

  EFI_INVALID_PARAMETER       - Memory map not consistent with construction rules.
  
  EFI_SUCCESS                 - Valid memory map.

--*/
;

VOID
CoreNotifySignalList (
  IN EFI_GUID     *EventGroup
  )
/*++

Routine Description:

  Signals all events on the requested list

Arguments:

  SignalType      - The list to signal
    
Returns:

  None

--*/
;


EFI_STATUS
EFIAPI
CoreInstallConfigurationTable (
  IN EFI_GUID         *Guid,
  IN VOID             *Table
  )
/*++

Routine Description:

  Boot Service called to add, modify, or remove a system configuration table from 
  the EFI System Table.

Arguments:

  Guid:   Pointer to the GUID for the entry to add, update, or remove
  Table:  Pointer to the configuration table for the entry to add, update, or
          remove, may be NULL.

Returns:
  
  EFI_SUCCESS               Guid, Table pair added, updated, or removed.
  EFI_INVALID_PARAMETER     Input GUID not valid.
  EFI_NOT_FOUND             Attempted to delete non-existant entry
  EFI_OUT_OF_RESOURCES      Not enough memory available

--*/
;


EFI_TPL
EFIAPI
CoreRaiseTpl (
  IN EFI_TPL  NewTpl
  )
/*++

Routine Description:

  Raise the task priority level to the new level.
  High level is implemented by disabling processor interrupts.

Arguments:

  NewTpl  - New task priority level
    
Returns:

  The previous task priority level

--*/
;


VOID
EFIAPI
CoreRestoreTpl (
  IN EFI_TPL  NewTpl
  )
/*++

Routine Description:

  Lowers the task priority to the previous value.   If the new 
  priority unmasks events at a higher priority, they are dispatched.

Arguments:

  NewTpl  - New, lower, task priority
    
Returns:

  None

--*/
;


EFI_STATUS
EFIAPI
CoreStall (
  IN UINTN            Microseconds
  )
/*++

Routine Description:

  Introduces a fine-grained stall.

Arguments:

  Microseconds      The number of microseconds to stall execution

Returns:

  EFI_SUCCESS            - Execution was stalled for at least the requested amount
                           of microseconds.

  EFI_NOT_AVAILABLE_YET  - gMetronome is not available yet

--*/
;


EFI_STATUS
EFIAPI
CoreSetWatchdogTimer (
  IN UINTN            Timeout,
  IN UINT64           WatchdogCode,
  IN UINTN            DataSize,
  IN CHAR16           *WatchdogData   OPTIONAL
  )
/*++

Routine Description:

  Sets the system's watchdog timer.

Arguments:

  Timeout         The number of seconds.  Zero disables the timer.

  ///////following  three parameters are left for platform specific using  
  
  WatchdogCode    The numberic code to log.  0x0 to 0xffff are firmware
  DataSize        Size of the optional data
  WatchdogData    Optional Null terminated unicode string followed by binary 
                  data.

Returns:

  EFI_SUCCESS               Timeout has been set
  EFI_NOT_AVAILABLE_YET     WatchdogTimer is not available yet 
  EFI_UNSUPPORTED           System does not have a timer (currently not used)
  EFI_DEVICE_ERROR          Could not complete due to hardware error

--*/
;


EFI_STATUS
EFIAPI
CoreInstallProtocolInterface (
  IN OUT EFI_HANDLE     *UserHandle,
  IN EFI_GUID           *Protocol,
  IN EFI_INTERFACE_TYPE InterfaceType,
  IN VOID               *Interface
  )
/*++

Routine Description:

  Wrapper function to CoreInstallProtocolInterfaceNotify.  This is the public API which
  Calls the private one which contains a BOOLEAN parameter for notifications

Arguments:

  UserHandle     - The handle to install the protocol handler on,
                    or NULL if a new handle is to be allocated

  Protocol       - The protocol to add to the handle

  InterfaceType  - Indicates whether Interface is supplied in native form.

  Interface      - The interface for the protocol being added

Returns:

  Status code    

--*/
;

EFI_STATUS
CoreInstallProtocolInterfaceNotify (
  IN OUT EFI_HANDLE     *UserHandle,
  IN EFI_GUID           *Protocol,
  IN EFI_INTERFACE_TYPE InterfaceType,
  IN VOID               *Interface,
  IN BOOLEAN            Notify
  )
/*++

Routine Description:

  Installs a protocol interface into the boot services environment.

Arguments:

  UserHandle     - The handle to install the protocol handler on,
                   or NULL if a new handle is to be allocated

  Protocol       - The protocol to add to the handle

  InterfaceType  - Indicates whether Interface is supplied in native form.

  Interface      - The interface for the protocol being added
  
  Notify         - Whether to notify the notification list for this protocol 

Returns:

  EFI_INVALID_PARAMETER     - Invalid parameter
  
  EFI_OUT_OF_RESOURCES       - No enough buffer to allocate
  
  EFI_SUCCESS               - Protocol interface successfully installed

--*/
;


EFI_STATUS
EFIAPI
CoreInstallMultipleProtocolInterfaces (
  IN OUT EFI_HANDLE           *Handle,
  ...
  )
/*++

Routine Description:

  Installs a list of protocol interface into the boot services environment.
  This function calls InstallProtocolInterface() in a loop. If any error
  occures all the protocols added by this function are removed. This is 
  basically a lib function to save space.

Arguments:

  Handle      - The handle to install the protocol handlers on,
                or NULL if a new handle is to be allocated
  ...         - EFI_GUID followed by protocol instance. A NULL terminates the 
                list. The pairs are the arguments to InstallProtocolInterface().
                All the protocols are added to Handle.

Returns:

  EFI_INVALID_PARAMETER       - Handle is NULL.
  
  EFI_SUCCESS                 - Protocol interfaces successfully installed.

--*/
;


EFI_STATUS
EFIAPI
CoreUninstallMultipleProtocolInterfaces (
  IN EFI_HANDLE           Handle,
  ...
  )
/*++

Routine Description:

  Uninstalls a list of protocol interface in the boot services environment. 
  This function calls UnisatllProtocolInterface() in a loop. This is 
  basically a lib function to save space.

Arguments:

  Handle      - The handle to uninstall the protocol

  ...         - EFI_GUID followed by protocol instance. A NULL terminates the 
                list. The pairs are the arguments to UninstallProtocolInterface().
                All the protocols are added to Handle.

Returns:

  Status code    

--*/
;


EFI_STATUS
EFIAPI
CoreReinstallProtocolInterface (
  IN EFI_HANDLE     UserHandle,
  IN EFI_GUID       *Protocol,
  IN VOID           *OldInterface,
  IN VOID           *NewInterface
  )
/*++

Routine Description:

  Reinstall a protocol interface on a device handle.  The OldInterface for Protocol is replaced by the NewInterface.

Arguments:

  UserHandle    - Handle on which the interface is to be reinstalled
  Protocol      - The numeric ID of the interface
  OldInterface  - A pointer to the old interface
  NewInterface  - A pointer to the new interface 


Returns:

  Status code.

  On EFI_SUCCESS            The protocol interface was installed
  On EFI_NOT_FOUND          The OldInterface on the handle was not found
  On EFI_INVALID_PARAMETER  One of the parameters has an invalid value
  
--*/
;


EFI_STATUS
EFIAPI
CoreUninstallProtocolInterface (
  IN EFI_HANDLE       UserHandle,
  IN EFI_GUID         *Protocol,
  IN VOID             *Interface
  )
/*++

Routine Description:

  Uninstalls all instances of a protocol:interfacer from a handle. 
  If the last protocol interface is remove from the handle, the 
  handle is freed.

Arguments:

  UserHandle      - The handle to remove the protocol handler from

  Protocol        - The protocol, of protocol:interface, to remove

  Interface       - The interface, of protocol:interface, to remove

Returns:

  EFI_INVALID_PARAMETER       - Protocol is NULL.
  
  EFI_SUCCESS                 - Protocol interface successfully uninstalled.

--*/
;


EFI_STATUS
EFIAPI
CoreHandleProtocol (
  IN  EFI_HANDLE       UserHandle,
  IN  EFI_GUID         *Protocol,
  OUT VOID             **Interface
  )
/*++

Routine Description:

  Queries a handle to determine if it supports a specified protocol.

Arguments:

  UserHandle  - The handle being queried.

  Protocol    - The published unique identifier of the protocol.

  Interface   - Supplies the address where a pointer to the corresponding Protocol
               Interface is returned.

Returns:

  The requested protocol interface for the handle
  
--*/  
;


EFI_STATUS
EFIAPI
CoreOpenProtocol (
  IN  EFI_HANDLE                UserHandle,
  IN  EFI_GUID                  *Protocol,
  OUT VOID                      **Interface OPTIONAL,
  IN  EFI_HANDLE                ImageHandle,
  IN  EFI_HANDLE                ControllerHandle,
  IN  UINT32                    Attributes
  )
/*++

Routine Description:

  Locates the installed protocol handler for the handle, and
  invokes it to obtain the protocol interface. Usage information
  is registered in the protocol data base.

Arguments:

  UserHandle        - The handle to obtain the protocol interface on

  Protocol          - The ID of the protocol 

  Interface         - The location to return the protocol interface

  ImageHandle       - The handle of the Image that is opening the protocol interface
                    specified by Protocol and Interface.
  
  ControllerHandle  - The controller handle that is requiring this interface.

  Attributes     - The open mode of the protocol interface specified by Handle
                    and Protocol.

Returns:

  EFI_INVALID_PARAMETER       - Protocol is NULL.
  
  EFI_SUCCESS                 - Get the protocol interface.
  
--*/
;


EFI_STATUS
EFIAPI
CoreOpenProtocolInformation (
  IN  EFI_HANDLE                          UserHandle,
  IN  EFI_GUID                            *Protocol,
  OUT EFI_OPEN_PROTOCOL_INFORMATION_ENTRY **EntryBuffer,
  OUT UINTN                               *EntryCount
  )
/*++

Routine Description:

  Return information about Opened protocols in the system

Arguments:

  UserHandle  - The handle to close the protocol interface on

  Protocol    - The ID of the protocol 

  EntryBuffer - A pointer to a buffer of open protocol information in the form of
                EFI_OPEN_PROTOCOL_INFORMATION_ENTRY structures.

  EntryCount  - Number of EntryBuffer entries

Returns:

  
--*/
;


EFI_STATUS
EFIAPI
CoreCloseProtocol (
  IN  EFI_HANDLE                UserHandle,
  IN  EFI_GUID                  *Protocol,
  IN  EFI_HANDLE                ImageHandle,
  IN  EFI_HANDLE                ControllerHandle
  )
/*++

Routine Description:

  Close Protocol

Arguments:

  UserHandle       - The handle to close the protocol interface on

  Protocol         - The ID of the protocol 

  ImageHandle      - The user of the protocol to close

  ControllerHandle - The user of the protocol to close

Returns:

  EFI_INVALID_PARAMETER     - Protocol is NULL.
    
--*/
;


EFI_STATUS
EFIAPI
CoreProtocolsPerHandle (
  IN  EFI_HANDLE       UserHandle,
  OUT EFI_GUID         ***ProtocolBuffer,
  OUT UINTN            *ProtocolBufferCount
  )
/*++

Routine Description:

  Retrieves the list of protocol interface GUIDs that are installed on a handle in a buffer allocated
 from pool.

Arguments:

  UserHandle           - The handle from which to retrieve the list of protocol interface
                          GUIDs.

  ProtocolBuffer       - A pointer to the list of protocol interface GUID pointers that are
                          installed on Handle.

  ProtocolBufferCount  - A pointer to the number of GUID pointers present in
                          ProtocolBuffer.

Returns:
  EFI_SUCCESS   -  The list of protocol interface GUIDs installed on Handle was returned in
                   ProtocolBuffer. The number of protocol interface GUIDs was
                   returned in ProtocolBufferCount.
  EFI_INVALID_PARAMETER   -  Handle is NULL.
  EFI_INVALID_PARAMETER   -  Handle is not a valid EFI_HANDLE.
  EFI_INVALID_PARAMETER   -  ProtocolBuffer is NULL.
  EFI_INVALID_PARAMETER   -  ProtocolBufferCount is NULL.
  EFI_OUT_OF_RESOURCES    -  There is not enough pool memory to store the results.
  
--*/
;


EFI_STATUS
EFIAPI
CoreRegisterProtocolNotify (
  IN  EFI_GUID       *Protocol,
  IN  EFI_EVENT      Event,
  OUT VOID           **Registration
  )
/*++

Routine Description:

  Add a new protocol notification record for the request protocol.

Arguments:

  Protocol      - The requested protocol to add the notify registration

  Event         - The event to signal 

  Registration  - Returns the registration record


Returns:

  EFI_INVALID_PARAMETER       - Invalid parameter

  EFI_SUCCESS                 - Successfully returned the registration record that has been added
  
--*/
;
  

EFI_STATUS
EFIAPI
CoreLocateHandle (
  IN     EFI_LOCATE_SEARCH_TYPE         SearchType,
  IN     EFI_GUID                       *Protocol OPTIONAL,
  IN     VOID                           *SearchKey OPTIONAL,
  IN OUT UINTN                          *BufferSize,
  OUT    EFI_HANDLE                     *Buffer
  )
/*++

Routine Description:

  Locates the requested handle(s) and returns them in Buffer.

Arguments:

  SearchType  - The type of search to perform to locate the handles

  Protocol    - The protocol to search for
  
  SearchKey   - Dependant on SearchType

  BufferSize  - On input the size of Buffer.  On output the 
                size of data returned.  

  Buffer      - The buffer to return the results in


Returns:

  EFI_BUFFER_TOO_SMALL      - Buffer too small, required buffer size is returned in BufferSize.

  EFI_INVALID_PARAMETER     - Invalid parameter
  
  EFI_SUCCESS               - Successfully found the requested handle(s) and returns them in Buffer.
  
--*/
;
  

EFI_STATUS
EFIAPI
CoreLocateDevicePath (
  IN     EFI_GUID                       *Protocol,
  IN OUT EFI_DEVICE_PATH_PROTOCOL       **FilePath,
  OUT    EFI_HANDLE                     *Device
  )
/*++

Routine Description:

  Locates the handle to a device on the device path that supports the specified protocol.

Arguments:

  Protocol    - The protocol to search for.
  FilePath    - On input, a pointer to a pointer to the device path. On output, the device
                  path pointer is modified to point to the remaining part of the devicepath.
  Device      - A pointer to the returned device handle.              

Returns:

  EFI_SUCCESS           - The resulting handle was returned.
  EFI_NOT_FOUND         - No handles matched the search.
  EFI_INVALID_PARAMETER - One of the parameters has an invalid value.

--*/
;

 
EFI_STATUS
EFIAPI
CoreLocateHandleBuffer (
  IN     EFI_LOCATE_SEARCH_TYPE         SearchType,
  IN     EFI_GUID                       *Protocol OPTIONAL,
  IN     VOID                           *SearchKey OPTIONAL,
  IN OUT UINTN                          *NumberHandles,
  OUT    EFI_HANDLE                     **Buffer
  )
/*++

Routine Description:

  Function returns an array of handles that support the requested protocol 
  in a buffer allocated from pool. This is a version of CoreLocateHandle()
  that allocates a buffer for the caller.

Arguments:

  SearchType           - Specifies which handle(s) are to be returned.
  Protocol             - Provides the protocol to search by.   
                         This parameter is only valid for SearchType ByProtocol.
  SearchKey            - Supplies the search key depending on the SearchType.
  NumberHandles      - The number of handles returned in Buffer.
  Buffer               - A pointer to the buffer to return the requested array of 
                         handles that support Protocol.

Returns:
  
  EFI_SUCCESS             - The result array of handles was returned.
  EFI_NOT_FOUND           - No handles match the search. 
  EFI_OUT_OF_RESOURCES    - There is not enough pool memory to store the matching results.
  EFI_INVALID_PARAMETER   - Invalid parameter

--*/
;

 
EFI_STATUS
EFIAPI
CoreLocateProtocol (
  IN    EFI_GUID  *Protocol,
  IN    VOID      *Registration OPTIONAL,
  OUT   VOID      **Interface
  )
/*++

Routine Description:

  Return the first Protocol Interface that matches the Protocol GUID. If
  Registration is pasased in return a Protocol Instance that was just add
  to the system. If Retistration is NULL return the first Protocol Interface
  you find.

Arguments:

  Protocol     - The protocol to search for
  
  Registration - Optional Registration Key returned from RegisterProtocolNotify() 

  Interface    - Return the Protocol interface (instance).

Returns:

  EFI_SUCCESS                 - If a valid Interface is returned
  
  EFI_INVALID_PARAMETER       - Invalid parameter
  
  EFI_NOT_FOUND               - Protocol interface not found

--*/
;

UINT64
CoreGetHandleDatabaseKey (
  VOID
  )
/*++

Routine Description:

  return handle database key.

Arguments:

  None
  
Returns:
  
  Handle database key.
  
--*/
;

VOID
CoreConnectHandlesByKey (
  UINT64  Key
  )
/*++

Routine Description:

  Go connect any handles that were created or modified while a image executed.

Arguments:

  Key  -  The Key to show that the handle has been created/modified

Returns:
  
  None
--*/
;


EFI_STATUS 
EFIAPI
CoreConnectController (
  IN  EFI_HANDLE                ControllerHandle,
  IN  EFI_HANDLE                *DriverImageHandle    OPTIONAL,
  IN  EFI_DEVICE_PATH_PROTOCOL  *RemainingDevicePath  OPTIONAL,
  IN  BOOLEAN                   Recursive
  )
/*++

Routine Description:

  Connects one or more drivers to a controller.

Arguments:

  ControllerHandle            - Handle of the controller to be connected.

  DriverImageHandle           - DriverImageHandle A pointer to an ordered list of driver image handles.

  RemainingDevicePath         - RemainingDevicePath A pointer to the device path that specifies a child of the
                                controller specified by ControllerHandle.
    
  Recursive -                 - Whether the function would be called recursively or not.

Returns:

  Status code.

--*/
;


EFI_STATUS 
EFIAPI
CoreDisconnectController (
  IN EFI_HANDLE  ControllerHandle,
  IN EFI_HANDLE  DriverImageHandle  OPTIONAL,
  IN EFI_HANDLE  ChildHandle        OPTIONAL
  )
/*++

Routine Description:

  Disonnects a controller from a driver

Arguments:

  ControllerHandle  - ControllerHandle The handle of the controller from which driver(s) 
                        are to be disconnected.
  DriverImageHandle - DriverImageHandle The driver to disconnect from ControllerHandle.
  ChildHandle       - ChildHandle The handle of the child to destroy.

Returns:

  EFI_SUCCESS           -  One or more drivers were disconnected from the controller.
  EFI_SUCCESS           -  On entry, no drivers are managing ControllerHandle.
  EFI_SUCCESS           -  DriverImageHandle is not NULL, and on entry DriverImageHandle is not managing ControllerHandle.
  EFI_INVALID_PARAMETER -  ControllerHandle is not a valid EFI_HANDLE.
  EFI_INVALID_PARAMETER -  DriverImageHandle is not NULL, and it is not a valid EFI_HANDLE.
  EFI_INVALID_PARAMETER -  ChildHandle is not NULL, and it is not a valid EFI_HANDLE.
  EFI_OUT_OF_RESOURCES  -  There are not enough resources available to disconnect any drivers from ControllerHandle.
  EFI_DEVICE_ERROR      -  The controller could not be disconnected because of a device error.

--*/
;


EFI_STATUS
EFIAPI
CoreAllocatePages (
  IN      EFI_ALLOCATE_TYPE       Type,
  IN      EFI_MEMORY_TYPE         MemoryType,
  IN      UINTN                   NumberOfPages,
  IN OUT  EFI_PHYSICAL_ADDRESS    *Memory
  )
/*++

Routine Description:

  Allocates pages from the memory map.

Arguments:

  Type          - The type of allocation to perform

  MemoryType    - The type of memory to turn the allocated pages into

  NumberOfPages - The number of pages to allocate

  Memory        - A pointer to receive the base allocated memory address

Returns:

  Status. On success, Memory is filled in with the base address allocated
  
  EFI_INVALID_PARAMETER     - Parameters violate checking rules defined in spec.
  
  EFI_NOT_FOUND             - Could not allocate pages match the requirement.
  
  EFI_OUT_OF_RESOURCES      - No enough pages to allocate.
  
  EFI_SUCCESS               - Pages successfully allocated.

--*/
;


EFI_STATUS 
EFIAPI
CoreFreePages (
  IN EFI_PHYSICAL_ADDRESS   Memory,
  IN UINTN                  NumberOfPages
  )
/*++

Routine Description:

  Frees previous allocated pages.

Arguments:

  Memory        - Base address of memory being freed

  NumberOfPages - The number of pages to free

Returns:

  EFI_NOT_FOUND       - Could not find the entry that covers the range
  
  EFI_INVALID_PARAMETER   - Address not aligned
  
  EFI_SUCCESS         -Pages successfully freed.

--*/
;


EFI_STATUS
EFIAPI
CoreGetMemoryMap (
  IN OUT UINTN                       *MemoryMapSize,
  IN OUT EFI_MEMORY_DESCRIPTOR       *Desc,
  OUT    UINTN                       *MapKey,
  OUT    UINTN                       *DescriptorSize,
  OUT    UINT32                      *DescriptorVersion
  )
/*++

Routine Description:

  Returns the current memory map.

Arguments:

  MemoryMapSize     - On input the buffer size of MemoryMap allocated by caller
                      On output the required buffer size to contain the memory map 
                      
  Desc              - The buffer to return the current memory map

  MapKey            - The address to return the current map key

  DescriptorSize    - The size in bytes for an individual EFI_MEMORY_DESCRIPTOR

  DescriptorVersion - The version number associated with the EFI_MEMORY_DESCRIPTOR

Returns:

  EFI_SUCCESS           The current memory map was returned successfully

  EFI_BUFFER_TOO_SMALL  The MemoryMap buffer was too small

  EFI_INVALID_PARAMETER One of the parameters has an invalid value

--*/
;


EFI_STATUS
EFIAPI
CoreAllocatePool (
  IN   EFI_MEMORY_TYPE  PoolType,
  IN   UINTN            Size,
  OUT  VOID             **Buffer
  )
/*++

Routine Description:

  Allocate pool of a particular type.

Arguments:

  PoolType    - Type of pool to allocate

  Size        - The amount of pool to allocate

  Buffer      - The address to return a pointer to the allocated pool

Returns:

  EFI_INVALID_PARAMETER     - PoolType not valid
  
  EFI_OUT_OF_RESOURCES      - Size exceeds max pool size or allocation failed.  
  
  EFI_SUCCESS               - Pool successfully allocated.

--*/
;


EFI_STATUS
EFIAPI
CoreFreePool (
  IN VOID      *Buffer
  )
/*++

Routine Description:

  Frees pool.

Arguments:

  Buffer      - The allocated pool entry to free

Returns:

  EFI_INVALID_PARAMETER   - Buffer is not a valid value.
  
  EFI_SUCCESS             - Pool successfully freed.

--*/
;


EFI_STATUS
EFIAPI
CoreLoadImage (
  IN  BOOLEAN                    BootPolicy,
  IN  EFI_HANDLE                 ParentImageHandle,
  IN  EFI_DEVICE_PATH_PROTOCOL   *FilePath,
  IN  VOID                       *SourceBuffer   OPTIONAL,
  IN  UINTN                      SourceSize,
  OUT EFI_HANDLE                 *ImageHandle
  )
/*++

Routine Description:

  Loads an EFI image into memory and returns a handle to the image.

Arguments:

  BootPolicy          - If TRUE, indicates that the request originates from the boot manager,
                        and that the boot manager is attempting to load FilePath as a boot selection.
  ParentImageHandle   - The caller's image handle.
  FilePath            - The specific file path from which the image is loaded.
  SourceBuffer        - If not NULL, a pointer to the memory location containing a copy of 
                        the image to be loaded.
  SourceSize          - The size in bytes of SourceBuffer.
  ImageHandle         - Pointer to the returned image handle that is created when the image 
                        is successfully loaded.

Returns:

  EFI_SUCCESS            - The image was loaded into memory.
  EFI_NOT_FOUND          - The FilePath was not found.
  EFI_INVALID_PARAMETER  - One of the parameters has an invalid value.
  EFI_UNSUPPORTED        - The image type is not supported, or the device path cannot be 
                           parsed to locate the proper protocol for loading the file.
  EFI_OUT_OF_RESOURCES   - Image was not loaded due to insufficient resources.
--*/
;


EFI_STATUS
EFIAPI
CoreUnloadImage (
  IN EFI_HANDLE  ImageHandle
  )
/*++

Routine Description:

  Unload the specified image.

Arguments:

  ImageHandle       - The specified image handle.

Returns:

  EFI_INVALID_PARAMETER       - Image handle is NULL.
  
  EFI_UNSUPPORTED             - Attempt to unload an unsupported image.
  
  EFI_SUCCESS                 - Image successfully unloaded.

--*/
;


EFI_STATUS
EFIAPI
CoreStartImage (
  IN  EFI_HANDLE  ImageHandle,
  OUT UINTN       *ExitDataSize,
  OUT CHAR16      **ExitData  OPTIONAL
  )
/*++

Routine Description:

  Transfer control to a loaded image's entry point.

Arguments:

  ImageHandle     - Handle of image to be started.
  
  ExitDataSize    - Pointer of the size to ExitData
  
  ExitData        - Pointer to a pointer to a data buffer that includes a Null-terminated
                    Unicode string, optionally followed by additional binary data. The string
                    is a description that the caller may use to further indicate the reason for
                    the image's exit.

Returns:

  EFI_INVALID_PARAMETER     - Invalid parameter
  
  EFI_OUT_OF_RESOURCES       - No enough buffer to allocate
  
  EFI_SUCCESS               - Successfully transfer control to the image's entry point.

--*/
;


EFI_STATUS
EFIAPI
CoreExit (
  IN EFI_HANDLE  ImageHandle,
  IN EFI_STATUS  Status,
  IN UINTN       ExitDataSize,
  IN CHAR16      *ExitData  OPTIONAL
  )
/*++

Routine Description:

  Terminates the currently loaded EFI image and returns control to boot services.

Arguments:

  ImageHandle       - Handle that identifies the image. This parameter is passed to the image 
                      on entry.
  Status            - The image's exit code.
  ExitDataSize      - The size, in bytes, of ExitData. Ignored if ExitStatus is
                      EFI_SUCCESS.
  ExitData          - Pointer to a data buffer that includes a Null-terminated Unicode string,
                      optionally followed by additional binary data. The string is a 
                      description that the caller may use to further indicate the reason for
                      the image's exit.

Returns:

  EFI_INVALID_PARAMETER     - Image handle is NULL or it is not current image.
  
  EFI_SUCCESS               - Successfully terminates the currently loaded EFI image.
  
  EFI_ACCESS_DENIED         - Should never reach there.

--*/
;


EFI_STATUS
EFIAPI
CoreCreateEvent (
  IN  UINT32               Type,
  IN  EFI_TPL              NotifyTpl,
  IN  EFI_EVENT_NOTIFY     NotifyFunction,
  IN  VOID                 *NotifyContext,
  OUT EFI_EVENT            *pEvent
  )
/*++

Routine Description:

  Creates a general-purpose event structure

Arguments:

  Type                - The type of event to create and its mode and attributes
  NotifyTpl           - The task priority level of event notifications
  NotifyFunction      - Pointer to the event's notification function
  NotifyContext       - Pointer to the notification function's context; corresponds to
                        parameter "Context" in the notification function
  pEvent              - Pointer to the newly created event if the call succeeds; undefined otherwise

Returns:

  EFI_SUCCESS           - The event structure was created
  EFI_INVALID_PARAMETER - One of the parameters has an invalid value
  EFI_OUT_OF_RESOURCES  - The event could not be allocated

--*/
;


EFI_STATUS
EFIAPI
CoreCreateEventEx (
  IN UINT32                   Type,
  IN EFI_TPL                  NotifyTpl,
  IN EFI_EVENT_NOTIFY         NotifyFunction, OPTIONAL
  IN CONST VOID               *NotifyContext, OPTIONAL
  IN CONST EFI_GUID           *EventGroup,    OPTIONAL
  OUT EFI_EVENT               *Event
  )
/*++

Routine Description:
  Creates a general-purpose event structure

Arguments:
  Type                - The type of event to create and its mode and attributes
  NotifyTpl           - The task priority level of event notifications
  NotifyFunction      - Pointer to the events notification function
  NotifyContext       - Pointer to the notification functions context; corresponds to
                        parameter "Context" in the notification function
  EventGrout          - GUID for EventGroup if NULL act the same as gBS->CreateEvent().
  Event               - Pointer to the newly created event if the call succeeds; undefined otherwise

Returns:
  EFI_SUCCESS           - The event structure was created
  EFI_INVALID_PARAMETER - One of the parameters has an invalid value
  EFI_OUT_OF_RESOURCES  - The event could not be allocated

--*/
;


EFI_STATUS
EFIAPI
CoreSetTimer (
  IN EFI_EVENT            Event,
  IN EFI_TIMER_DELAY      Type,
  IN UINT64               TriggerTime
  )
/*++

Routine Description:

  Sets the type of timer and the trigger time for a timer event.

Arguments:

  UserEvent   - The timer event that is to be signaled at the specified time
  Type        - The type of time that is specified in TriggerTime
  TriggerTime - The number of 100ns units until the timer expires
  
Returns:

  EFI_SUCCESS           - The event has been set to be signaled at the requested time
  EFI_INVALID_PARAMETER - Event or Type is not valid

--*/
;


EFI_STATUS
EFIAPI
CoreSignalEvent (
  IN EFI_EVENT            Event
  )
/*++

Routine Description:

  Signals the event.  Queues the event to be notified if needed
    
Arguments:

  Event - The event to signal
    
Returns:

  EFI_INVALID_PARAMETER - Parameters are not valid.
  
  EFI_SUCCESS - The event was signaled.

--*/
;


EFI_STATUS
EFIAPI
CoreWaitForEvent (
  IN  UINTN        NumberOfEvents,
  IN  EFI_EVENT    *UserEvents,
  OUT UINTN        *UserIndex
  )
/*++

Routine Description:

  Stops execution until an event is signaled.
    
Arguments:

  NumberOfEvents  - The number of events in the UserEvents array
  UserEvents      - An array of EFI_EVENT
  UserIndex       - Pointer to the index of the event which satisfied the wait condition
    
Returns:

  EFI_SUCCESS           - The event indicated by Index was signaled.
  EFI_INVALID_PARAMETER - The event indicated by Index has a notification function or 
                          Event was not a valid type
  EFI_UNSUPPORTED       - The current TPL is not TPL_APPLICATION

--*/
;


EFI_STATUS
EFIAPI
CoreCloseEvent (
  IN EFI_EVENT            Event
  )
/*++

Routine Description:

  Closes an event and frees the event structure.
    
Arguments:

  UserEvent - Event to close
    
Returns:

  EFI_INVALID_PARAMETER - Parameters are not valid.
  
  EFI_SUCCESS - The event has been closed

--*/
;


EFI_STATUS
EFIAPI
CoreCheckEvent (
  IN EFI_EVENT            Event
  )
/*++

Routine Description:

  Check the status of an event
    
Arguments:

  UserEvent - The event to check
    
Returns:

  EFI_SUCCESS           - The event is in the signaled state
  EFI_NOT_READY         - The event is not in the signaled state
  EFI_INVALID_PARAMETER - Event is of type EVT_NOTIFY_SIGNAL

--*/
;

EFI_STATUS
CoreAddMemorySpace (
  IN EFI_GCD_MEMORY_TYPE   GcdMemoryType,
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINT64                Length,
  IN UINT64                Capabilities
  )
/*++

Routine Description:

  Add a segment of memory space to GCD map and add all available pages in this segment 
  as memory descriptors.

Arguments:
    
  GcdMemoryType     - Memory type of the segment.
  
  BaseAddress       - Base address of the segment.
  
  Length            - Length of the segment.
  
  Capabilities      - alterable attributes of the segment.

Returns:

  EFI_SUCCESS       - Merged this segment into GCD map.

--*/
;

EFI_STATUS
CoreAllocateMemorySpace (
  IN     EFI_GCD_ALLOCATE_TYPE  GcdAllocateType,
  IN     EFI_GCD_MEMORY_TYPE    GcdMemoryType,
  IN     UINTN                  Alignment,
  IN     UINT64                 Length,
  IN OUT EFI_PHYSICAL_ADDRESS   *BaseAddress,
  IN     EFI_HANDLE             ImageHandle,
  IN     EFI_HANDLE             DeviceHandle OPTIONAL
  )
/*++

Routine Description:

  Allocate memory space on GCD map.

Arguments:
  
  GcdAllocateType   - The type of allocate operation
  
  GcdMemoryType     - The desired memory type
  
  Alignment         - Align with 2^Alignment
  
  Length            - Length to allocate
  
  BaseAddress       - Base address to allocate
  
  ImageHandle       - The image handle consume the allocated space.
  
  DeviceHandle      - The device handle consume the allocated space.

Returns:

  EFI_INVALID_PARAMETER       - Invalid parameter.
  
  EFI_NOT_FOUND               - No descriptor contains the desired space.
  
  EFI_SUCCESS                 - Memory space successfully allocated.

--*/
;

EFI_STATUS
CoreFreeMemorySpace (
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINT64                Length
  )
/*++

Routine Description:Routine Description:

  Free a segment of memory space in GCD map.

Arguments:
    
  BaseAddress       - Base address of the segment.
  
  Length            - Length of the segment.
  
Returns:

  EFI_SUCCESS       - Space successfully freed.

--*/
;

EFI_STATUS
CoreRemoveMemorySpace (
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINT64                Length
  )
/*++

Routine Description:Routine Description:

  Remove a segment of memory space in GCD map.

Arguments:
    
  BaseAddress       - Base address of the segment.
  
  Length            - Length of the segment.
  
Returns:

  EFI_SUCCESS       - Successfully a segment of memory space.

--*/
;

EFI_STATUS
CoreGetMemorySpaceDescriptor (
  IN  EFI_PHYSICAL_ADDRESS             BaseAddress,
  OUT EFI_GCD_MEMORY_SPACE_DESCRIPTOR  *Descriptor
  )
/*++

Routine Description:

  Search all entries in GCD map which contains specified segment and build it to a descriptor.

Arguments:

  BaseAddress       - Specified start address
  
  Descriptor        - Specified length

Returns:

  EFI_INVALID_PARAMETER       - Invalid parameter
  
  EFI_SUCCESS                 - Successfully get memory space descriptor.

--*/
;

EFI_STATUS
CoreSetMemorySpaceAttributes (
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINT64                Length,
  IN UINT64                Attributes
  )
/*++

Routine Description:

  Set memory space with specified attributes.

Arguments:

  BaseAddress       - Specified start address
  
  Length            - Specified length
  
  Attributes        - Specified attributes

Returns:

  EFI_SUCCESS       - Successfully set attribute of a segment of memory space.

--*/
;

EFI_STATUS
CoreGetMemorySpaceMap (
  OUT UINTN                            *NumberOfDescriptors,
  OUT EFI_GCD_MEMORY_SPACE_DESCRIPTOR  **MemorySpaceMap
  )
/*++

Routine Description:

  Transer all entries of GCD memory map into memory descriptors and pass to caller.

Arguments:

  NumberOfDescriptors       - Number of descriptors.
  
  MemorySpaceMap            - Descriptor array

Returns:

  EFI_INVALID_PARAMETER     - Invalid parameter
  
  EFI_OUT_OF_RESOURCES      - No enough buffer to allocate
  
  EFI_SUCCESS               - Successfully get memory space map.

--*/
;

EFI_STATUS
CoreAddIoSpace (
  IN EFI_GCD_IO_TYPE       GcdIoType,
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINT64                Length
  )
/*++

Routine Description:

  Add a segment of IO space to GCD map.

Arguments:
    
  GcdIoType         - IO type of the segment.
  
  BaseAddress       - Base address of the segment.
  
  Length            - Length of the segment.

Returns:

  EFI_SUCCESS       - Merged this segment into GCD map.

--*/
;

EFI_STATUS
CoreAllocateIoSpace (
  IN     EFI_GCD_ALLOCATE_TYPE  GcdAllocateType,
  IN     EFI_GCD_IO_TYPE        GcdIoType,
  IN     UINTN                  Alignment,
  IN     UINT64                 Length,
  IN OUT EFI_PHYSICAL_ADDRESS   *BaseAddress,
  IN     EFI_HANDLE             ImageHandle,
  IN     EFI_HANDLE             DeviceHandle OPTIONAL
  )
/*++

Routine Description:

  Allocate IO space on GCD map.

Arguments:
  
  GcdAllocateType   - The type of allocate operation
  
  GcdIoType         - The desired IO type
  
  Alignment         - Align with 2^Alignment
  
  Length            - Length to allocate
  
  BaseAddress       - Base address to allocate
  
  ImageHandle       - The image handle consume the allocated space.
  
  DeviceHandle      - The device handle consume the allocated space.

Returns:

  EFI_INVALID_PARAMETER       - Invalid parameter.
  
  EFI_NOT_FOUND               - No descriptor contains the desired space.
  
  EFI_SUCCESS                 - IO space successfully allocated.

--*/
;

EFI_STATUS
CoreFreeIoSpace (
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINT64                Length
  )
/*++

Routine Description:Routine Description:

  Free a segment of IO space in GCD map.

Arguments:
    
  BaseAddress       - Base address of the segment.
  
  Length            - Length of the segment.
  
Returns:

  EFI_SUCCESS       - Space successfully freed.

--*/
;

EFI_STATUS
CoreRemoveIoSpace (
  IN EFI_PHYSICAL_ADDRESS  BaseAddress,
  IN UINT64                Length
  )
/*++

Routine Description:Routine Description:

  Remove a segment of IO space in GCD map.

Arguments:
    
  BaseAddress       - Base address of the segment.
  
  Length            - Length of the segment.
  
Returns:

  EFI_SUCCESS       - Successfully removed a segment of IO space.

--*/
;

EFI_STATUS
CoreGetIoSpaceDescriptor (
  IN  EFI_PHYSICAL_ADDRESS         BaseAddress,
  OUT EFI_GCD_IO_SPACE_DESCRIPTOR  *Descriptor
  )
/*++

Routine Description:

  Search all entries in GCD map which contains specified segment and build it to a descriptor.

Arguments:

  BaseAddress       - Specified start address
  
  Descriptor        - Specified length

Returns:

  EFI_INVALID_PARAMETER       - Descriptor is NULL.
  
  EFI_SUCCESS                 - Successfully get the IO space descriptor.

--*/
;

EFI_STATUS
CoreGetIoSpaceMap (
  OUT UINTN                        *NumberOfDescriptors,
  OUT EFI_GCD_IO_SPACE_DESCRIPTOR  **IoSpaceMap
  )
/*++

Routine Description:

  Transer all entries of GCD IO map into IO descriptors and pass to caller.

Arguments:

  NumberOfDescriptors       - Number of descriptors.
  
  IoSpaceMap                - Descriptor array

Returns:

  EFI_INVALID_PARAMETER     - Invalid parameter
  
  EFI_OUT_OF_RESOURCES      - No enough buffer to allocate
  
  EFI_SUCCESS               - Successfully get IO space map.

--*/
;

EFI_DXESERVICE
EFI_STATUS
EFIAPI
CoreDispatcher (
  VOID
  )
/*++

Routine Description:

  This is the main Dispatcher for DXE and it exits when there are no more 
  drivers to run. Drain the mScheduledQueue and load and start a PE
  image for each driver. Search the mDiscoveredList to see if any driver can 
  be placed on the mScheduledQueue. If no drivers are placed on the
  mScheduledQueue exit the function. On exit it is assumed the Bds()
  will be called, and when the Bds() exits the Dispatcher will be called 
  again.

Arguments:

  NONE

Returns:

  EFI_ALREADY_STARTED - The DXE Dispatcher is already running

  EFI_NOT_FOUND       - No DXE Drivers were dispatched

  EFI_SUCCESS         - One or more DXE Drivers were dispatched

--*/
;
EFI_DXESERVICE
EFI_STATUS
EFIAPI
CoreSchedule (
  IN  EFI_HANDLE  FirmwareVolumeHandle,
  IN  EFI_GUID    *DriverName
  )
/*++

Routine Description:

  Check every driver and locate a matching one. If the driver is found, the Unrequested
  state flag is cleared.

Arguments:

  FirmwareVolumeHandle - The handle of the Firmware Volume that contains the firmware 
                         file specified by DriverName.

  DriverName           - The Driver name to put in the Dependent state.

Returns:

  EFI_SUCCESS   - The DriverName was found and it's SOR bit was cleared

  EFI_NOT_FOUND - The DriverName does not exist or it's SOR bit was not set.

--*/
;

EFI_DXESERVICE
EFI_STATUS
EFIAPI
CoreTrust (
  IN  EFI_HANDLE  FirmwareVolumeHandle,
  IN  EFI_GUID    *DriverName
  )
/*++

Routine Description:

  Convert a driver from the Untrused back to the Scheduled state

Arguments:

  FirmwareVolumeHandle - The handle of the Firmware Volume that contains the firmware 
                         file specified by DriverName.

  DriverName           - The Driver name to put in the Scheduled state

Returns:

  EFI_SUCCESS   - The file was found in the untrusted state, and it was promoted 
                  to the trusted state.

  EFI_NOT_FOUND - The file was not found in the untrusted state.

--*/
;

BOOLEAN
CoreGrowBuffer (
  IN OUT EFI_STATUS       *Status,
  IN OUT VOID             **Buffer,
  IN     UINTN            BufferSize
  )
/*++

Routine Description:

    Helper function called as part of the code needed
    to allocate the proper sized buffer for various 
    EFI interfaces.

Arguments:

    Status      - Current status

    Buffer      - Current allocated buffer, or NULL

    BufferSize  - Current buffer size needed
    
Returns:
    
    TRUE - if the buffer was reallocated and the caller 
    should try the API again.

    FALSE - buffer could not be allocated and the caller
    should not try the API again.

--*/
;

EFI_STATUS
EFIAPI
FwVolDriverInit (
  IN EFI_HANDLE                   ImageHandle,
  IN EFI_SYSTEM_TABLE             *SystemTable
  )
/*++

Routine Description:
    This routine is the driver initialization entry point.  It initializes the
    libraries, and registers two notification functions.  These notification
    functions are responsible for building the FV stack dynamically.
    
Arguments:
    ImageHandle   - The image handle.
    SystemTable   - The system table.
    
Returns:
    EFI_SUCCESS   - Function successfully returned.

--*/
;

EFI_STATUS
EFIAPI
InitializeSectionExtraction (
  IN EFI_HANDLE                   ImageHandle,
  IN EFI_SYSTEM_TABLE             *SystemTable
  )
/*++

Routine Description: 
  Entry point of the section extraction code. Initializes an instance of the 
  section extraction interface and installs it on a new handle.

Arguments:  
  ImageHandle   EFI_HANDLE: A handle for the image that is initializing this driver
  SystemTable   EFI_SYSTEM_TABLE: A pointer to the EFI system table        

Returns:  
  EFI_SUCCESS:  Driver initialized successfully
  EFI_OUT_OF_RESOURCES:   Could not allocate needed resources

--*/
;

EFI_STATUS
CoreProcessFirmwareVolume (
  IN  VOID                         *FvHeader,
  IN  UINTN                        Size, 
  OUT EFI_HANDLE                   *FVProtocolHandle
  )
/*++

Routine Description:
    This DXE service routine is used to process a firmware volume. In
    particular, it can be called by BDS to process a single firmware
    volume found in a capsule. 

Arguments:
    FvHeader              - pointer to a firmware volume header
    Size                  - the size of the buffer pointed to by FvHeader
    FVProtocolHandle      - the handle on which a firmware volume protocol
                            was produced for the firmware volume passed in.

Returns:
    EFI_OUT_OF_RESOURCES  - if an FVB could not be produced due to lack of 
                            system resources
    EFI_VOLUME_CORRUPTED  - if the volume was corrupted
    EFI_SUCCESS           - a firmware volume protocol was produced for the
                            firmware volume

--*/
;

//
//Functions used during debug buils
//
VOID
CoreDisplayMissingArchProtocols (
  VOID
  )
/*++

  Routine Description:
  Displays Architectural protocols that were not loaded and are required for DXE core to function
  Only used in Debug Builds

  Arguments:
    NONE

  Returns:
    NONE

--*/;
  
VOID
CoreDisplayDiscoveredNotDispatched (
  VOID
  )
/*++

  Routine Description:

    Traverse the discovered list for any drivers that were discovered but not loaded 
    because the dependency experessions evaluated to false

  Arguments:

    NONE

  Returns:

    NONE 

--*/;

EFI_STATUS
EFIAPI
CoreEfiNotAvailableYetArg0 (
  VOID
  )
/*++

Routine Description:

  Place holder function until all the Boot Services and Runtime Services are available

Arguments:

  None

Returns:

  EFI_NOT_AVAILABLE_YET

--*/
;

EFI_STATUS
EFIAPI
CoreEfiNotAvailableYetArg1 (
  UINTN Arg1
  )
/*++

Routine Description:

  Place holder function until all the Boot Services and Runtime Services are available

Arguments:

  Arg1        - Undefined

Returns:

  EFI_NOT_AVAILABLE_YET

--*/
;

EFI_STATUS
EFIAPI
CoreEfiNotAvailableYetArg2 (
  UINTN Arg1,
  UINTN Arg2
  )
/*++

Routine Description:

  Place holder function until all the Boot Services and Runtime Services are available

Arguments:

  Arg1        - Undefined
  
  Arg2        - Undefined

Returns:

  EFI_NOT_AVAILABLE_YET

--*/
;

EFI_STATUS
EFIAPI
CoreEfiNotAvailableYetArg3 (
  UINTN Arg1,
  UINTN Arg2,
  UINTN Arg3
  )
/*++

Routine Description:

  Place holder function until all the Boot Services and Runtime Services are available

Arguments:

  Arg1        - Undefined
  
  Arg2        - Undefined
  
  Arg3        - Undefined

Returns:

  EFI_NOT_AVAILABLE_YET

--*/
;

EFI_STATUS
EFIAPI
CoreEfiNotAvailableYetArg4 (
  UINTN Arg1,
  UINTN Arg2,
  UINTN Arg3,
  UINTN Arg4
  )
/*++

Routine Description:

  Place holder function until all the Boot Services and Runtime Services are available

Arguments:

  Arg1        - Undefined
  
  Arg2        - Undefined
  
  Arg3        - Undefined
  
  Arg4        - Undefined

Returns:

  EFI_NOT_AVAILABLE_YET

--*/
;

EFI_STATUS
EFIAPI
CoreEfiNotAvailableYetArg5 (
  UINTN Arg1,
  UINTN Arg2,
  UINTN Arg3,
  UINTN Arg4,
  UINTN Arg5
  )
/*++

Routine Description:

  Place holder function until all the Boot Services and Runtime Services are available

Arguments:

  Arg1        - Undefined
  
  Arg2        - Undefined
  
  Arg3        - Undefined
  
  Arg4        - Undefined
  
  Arg5        - Undefined

Returns:

  EFI_NOT_AVAILABLE_YET

--*/
;

EFI_STATUS
CoreGetPeiProtocol (
  IN EFI_GUID  *ProtocolGuid,
  IN VOID      **Interface
  )
/*++

Routine Description:

  Searches for a Protocol Interface passed from PEI through a HOB

Arguments:

  ProtocolGuid - The Protocol GUID to search for in the HOB List

  Interface    - A pointer to the interface for the Protocol GUID

Returns:

  EFI_SUCCESS   - The Protocol GUID was found and its interface is returned in Interface

  EFI_NOT_FOUND - The Protocol GUID was not found in the HOB List

--*/
;
  
EFI_STATUS
DxeMainUefiDecompressGetInfo (
  IN EFI_DECOMPRESS_PROTOCOL            *This,
  IN   VOID                             *Source,
  IN   UINT32                           SourceSize,
  OUT  UINT32                           *DestinationSize,
  OUT  UINT32                           *ScratchSize
  );

EFI_STATUS
EFIAPI
DxeMainUefiDecompress (
  IN EFI_DECOMPRESS_PROTOCOL              *This,
  IN     VOID                             *Source,
  IN     UINT32                           SourceSize,
  IN OUT VOID                             *Destination,
  IN     UINT32                           DestinationSize,
  IN OUT VOID                             *Scratch,
  IN     UINT32                           ScratchSize
  );

EFI_STATUS
DxeMainTianoDecompressGetInfo (
  IN EFI_TIANO_DECOMPRESS_PROTOCOL      *This,
  IN   VOID                             *Source,
  IN   UINT32                           SourceSize,
  OUT  UINT32                           *DestinationSize,
  OUT  UINT32                           *ScratchSize
  );

EFI_STATUS
EFIAPI
DxeMainTianoDecompress (
  IN EFI_TIANO_DECOMPRESS_PROTOCOL        *This,
  IN     VOID                             *Source,
  IN     UINT32                           SourceSize,
  IN OUT VOID                             *Destination,
  IN     UINT32                           DestinationSize,
  IN OUT VOID                             *Scratch,
  IN     UINT32                           ScratchSize
  );

EFI_STATUS
DxeMainCustomDecompressGetInfo (
  IN EFI_CUSTOMIZED_DECOMPRESS_PROTOCOL  *This,
  IN   VOID                              *Source,
  IN   UINT32                            SourceSize,
  OUT  UINT32                            *DestinationSize,
  OUT  UINT32                            *ScratchSize
  );

EFI_STATUS
EFIAPI
DxeMainCustomDecompress (
  IN EFI_CUSTOMIZED_DECOMPRESS_PROTOCOL  *This,
  IN     VOID                            *Source,
  IN     UINT32                          SourceSize,
  IN OUT VOID                            *Destination,
  IN     UINT32                          DestinationSize,
  IN OUT VOID                            *Scratch,
  IN     UINT32                          ScratchSize
  );

#endif
