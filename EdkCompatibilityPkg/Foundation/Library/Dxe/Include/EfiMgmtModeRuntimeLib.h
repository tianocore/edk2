/*++

Copyright (c) 2004, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:
  
    EfiMgmtModeRuntimeLib.h

Abstract:

  Light weight lib to support EFI drivers.

--*/

#ifndef _EFI_RT_SUPPORT_LIB_H_
#define _EFI_RT_SUPPORT_LIB_H_

#ifndef EFI_LOAD_IMAGE_SMM
#define EFI_LOAD_DRIVER_SMM FALSE
#else
#define EFI_LOAD_DRIVER_SMM TRUE
#endif

#ifndef EFI_NO_LOAD_IMAGE_RT
#define EFI_NO_LOAD_DRIVER_RT FALSE
#else
#define EFI_NO_LOAD_DRIVER_RT TRUE
#endif

#include "EfiCommonLib.h"
#include "LinkedList.h"
#include "ProcDep.h"

#include EFI_PROTOCOL_DEFINITION (CpuIo)
#include EFI_PROTOCOL_DEFINITION (FirmwareVolumeBlock)

//
// Driver Lib Globals.
//
extern EFI_BOOT_SERVICES        *gBS;
extern EFI_SYSTEM_TABLE         *gST;
extern UINTN                    gRtErrorLevel;
extern BOOLEAN                  mEfiLoadDriverSmm;
extern BOOLEAN                  mEfiNoLoadDriverRt;
extern EFI_DEVICE_PATH_PROTOCOL *mFilePath;

//
// Runtime Memory Allocation/De-Allocation tools (Should be used in Boot Phase only)
//
EFI_STATUS
EfiAllocateRuntimeMemoryPool (
  IN UINTN                          Size,
  OUT VOID                          **Buffer
  )
/*++

Routine Description:

  Allocate EfiRuntimeServicesData pool of specified size.

Arguments:

  Size      - Pool size
  Buffer    - Memory pointer for output

Returns:

  Status code

--*/
;

EFI_STATUS
EfiFreeRuntimeMemoryPool (
  IN VOID                          *Buffer
  )
/*++

Routine Description:

  Free allocated pool

Arguments:

  Buffer  - Pool to be freed

Returns:

  Status code

--*/
;

EFI_STATUS
EfiLocateProtocolHandleBuffers (
  IN EFI_GUID                       *Protocol,
  IN OUT UINTN                      *NumberHandles,
  OUT EFI_HANDLE                    **Buffer
  )
/*++

Routine Description:

  Returns an array of handles that support the requested protocol in a buffer allocated from pool.

Arguments:

  Protocol      - Provides the protocol to search by.
  NumberHandles - The number of handles returned in Buffer.
  Buffer        - A pointer to the buffer to return the requested array of handles that
                  support Protocol.

Returns:

  Status code

--*/
;

EFI_STATUS
EfiHandleProtocol (
  IN EFI_HANDLE                     Handle,
  IN EFI_GUID                       *Protocol,
  OUT VOID                          **Interface
  )
/*++

Routine Description:

  Queries a handle to determine if it supports a specified protocol.

Arguments:

  Handle    - The handle being queried.
  Protocol  - The published unique identifier of the protocol.
  Interface - Supplies the address where a pointer to the corresponding Protocol
              Interface is returned. NULL will be returned in *Interface if a
              structure is not associated with Protocol.

Returns:

  Status code

--*/
;

EFI_STATUS
EfiInstallProtocolInterface (
  IN OUT EFI_HANDLE                 *Handle,
  IN EFI_GUID                       *Protocol,
  IN EFI_INTERFACE_TYPE             InterfaceType,
  IN VOID                           *Interface
  )
/*++

Routine Description:

  Installs a protocol interface on a device handle. If the handle does not exist, it is created and added
to the list of handles in the system.

Arguments:

  Handle        - A pointer to the EFI_HANDLE on which the interface is to be installed.
  Protocol      - The numeric ID of the protocol interface.
  InterfaceType - Indicates whether Interface is supplied in native form.
  Interface     - A pointer to the protocol interface.

Returns:

  Status code

--*/
;

EFI_STATUS
EfiReinstallProtocolInterface (
  IN EFI_HANDLE                     SmmProtocolHandle,
  IN EFI_GUID                       *Protocol,
  IN VOID                           *OldInterface,
  IN VOID                           *NewInterface
  )
/*++

Routine Description:

  Reinstalls a protocol interface on a device handle.

Arguments:

  SmmProtocolHandle - Handle on which the interface is to be reinstalled.
  Protocol          - The numeric ID of the interface.
  OldInterface      - A pointer to the old interface.
  NewInterface      - A pointer to the new interface.

Returns:

  Status code

--*/
;

EFI_STATUS
EfiLocateProtocolInterface (
  EFI_GUID  *Protocol,
  VOID      *Registration, OPTIONAL
  VOID      **Interface
  )
/*++

Routine Description:

  Returns the first protocol instance that matches the given protocol.

Arguments:

  Protocol      - Provides the protocol to search for.
  Registration  - Optional registration key returned from
                  RegisterProtocolNotify(). If Registration is NULL, then
                  it is ignored.
  Interface     - On return, a pointer to the first interface that matches Protocol and
                  Registration.

Returns:

  Status code

--*/
;

EFI_STATUS
UninstallProtocolInterface (
  IN EFI_HANDLE                     SmmProtocolHandle,
  IN EFI_GUID                       *Protocol,
  IN VOID                           *Interface
  )
/*++

Routine Description:

  Removes a protocol interface from a device handle.

Arguments:

  SmmProtocolHandle - The handle on which the interface was installed.
  Protocol          - The numeric ID of the interface.
  Interface         - A pointer to the interface.

Returns:

  Status code

--*/
;

EFI_STATUS
EfiRegisterProtocolCallback (
  IN  EFI_EVENT_NOTIFY              CallbackFunction,
  IN  VOID                          *Context,
  IN  EFI_GUID                      *ProtocolGuid,
  IN  EFI_TPL                       NotifyTpl,
  OUT VOID                          **Registeration,
  OUT EFI_EVENT                     *Event
  )
/*++

Routine Description:

  Register a callback function to be signaled whenever an interface is installed for 
  a specified protocol.

Arguments:

  CallbackFunction  - Call back function
  Context           - Context of call back function
  ProtocolGuid      - The numeric ID of the protocol for which the callback function
                      is to be registered.
  NotifyTpl         - Notify tpl of callback function
  Registeration     - A pointer to a memory location to receive the registration value.
  Event             - Event that is to be signaled whenever a protocol interface is registered
                      for Protocol.

Returns:

  Status code

--*/
;

EFI_STATUS
EfiSignalProtocolEvent (
  EFI_EVENT                         Event
  )
/*++

Routine Description:

  Signals an event.

Arguments:

  Event - The event to signal.

Returns:

  Status code

--*/
;

EFI_STATUS
EfiInstallVendorConfigurationTable (
  IN EFI_GUID                       *Guid,
  IN VOID                           *Table
  )
/*++

Routine Description:

  Adds, updates, or removes a configuration table entry from the EFI System Table.

Arguments:

  Guid  - A pointer to the GUID for the entry to add, update, or remove.
  Table - A pointer to the configuration table for the entry to add, update, or
          remove. May be NULL.

Returns:

  Status code

--*/
;

EFI_STATUS
EfiGetVendorConfigurationTable (
  IN EFI_GUID                       *Guid,
  OUT VOID                          **Table
  )
/*++

Routine Description:
  
  Return the EFI 1.0 System Tabl entry with TableGuid

Arguments:

  Guid      - Name of entry to return in the system table
  Table     - Pointer in EFI system table associated with TableGuid

Returns: 

  EFI_SUCCESS - Table returned;
  EFI_NOT_FOUND - TableGuid not in EFI system table

--*/
;

EFI_STATUS
EfiInitializeUtilsRuntimeDriverLib (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable,
  IN EFI_EVENT_NOTIFY     GoVirtualChildEvent
  )
/*++

Routine Description:

  Intialize runtime Driver Lib if it has not yet been initialized. 

Arguments:

  ImageHandle     - The firmware allocated handle for the EFI image.
  
  SystemTable     - A pointer to the EFI System Table.

  GoVirtualChildEvent - Caller can register a virtual notification event.

Returns: 

  EFI_STATUS always returns EFI_SUCCESS

--*/
;

BOOLEAN
EfiInManagementInterrupt (
  VOID
  )
/*++

Routine Description:

  Indicate whether the caller is already in SMM or not.

Arguments:

  None

Returns:

  TRUE      - In SMM
  FALSE     - Not in SMM

--*/
;

//
// This MACRO initializes the RUNTIME invironment and optionally loads Image to SMM or Non-SMM space
// based upon the presence of build flags EFI_LOAD_DRIVER_SMM and EFI_NO_LOAD_DRIVER_RT.
//
#define EFI_INITIALIZE_RUNTIME_DRIVER_LIB(ImageHandle, SystemTable, GoVirtualChildEvent, FilePath) \
  mEfiLoadDriverSmm = EFI_LOAD_DRIVER_SMM; \
  mEfiNoLoadDriverRt = EFI_NO_LOAD_DRIVER_RT; \
  mFilePath = (EFI_DEVICE_PATH_PROTOCOL*) FilePath; \
  EfiInitializeUtilsRuntimeDriverLib ((EFI_HANDLE) ImageHandle, (EFI_SYSTEM_TABLE*) SystemTable, (EFI_EVENT_NOTIFY) GoVirtualChildEvent); \
  if (!EfiInManagementInterrupt()) { \
    if (mEfiNoLoadDriverRt) { \
      return EFI_SUCCESS; \
    } \
  }  

#endif
