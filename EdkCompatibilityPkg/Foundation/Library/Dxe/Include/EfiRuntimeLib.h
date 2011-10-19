/*++

Copyright (c) 2004 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

    EfiRuntimeLib.h

Abstract:

  Light weight lib to support EFI drivers.

--*/

#ifndef _EFI_RUNTIME_LIB_H_
#define _EFI_RUNTIME_LIB_H_
#define MAX_FVB_COUNT 16
#include "EfiStatusCode.h"
#include "EfiCommonLib.h"

#include "LinkedList.h"
#include "GetImage.h"
#include "RtDevicePath.h"

#include EFI_GUID_DEFINITION (DxeServices)
#include EFI_GUID_DEFINITION (EventGroup)
#include EFI_GUID_DEFINITION (EventLegacyBios)
#include EFI_PROTOCOL_DEFINITION (CpuIo)
#include EFI_PROTOCOL_DEFINITION (FirmwareVolume)
#include EFI_PROTOCOL_DEFINITION (FirmwareVolume2)
#include EFI_PROTOCOL_DEFINITION (FirmwareVolumeBlock)
#include EFI_PROTOCOL_DEFINITION (FvbExtension)
#include "ProcDep.h"

typedef struct {
  EFI_HANDLE                          Handle;
  EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL  *Fvb;
  EFI_FVB_EXTENSION_PROTOCOL          *FvbExtension;
} FVB_ENTRY;

//
// Driver Lib Globals.
//
extern EFI_BOOT_SERVICES  *gBS;
extern EFI_SYSTEM_TABLE   *gST;
extern EFI_DXE_SERVICES   *gDS;
extern UINTN              gRtErrorLevel;
extern FVB_ENTRY          *mFvbEntry;

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

VOID
EFIAPI
EfiRuntimeLibFvbVirtualNotifyEvent (
  IN EFI_EVENT        Event,
  IN VOID             *Context
  )
/*++

Routine Description:

  Notify function to convert pointers to Fvb functions after ExitBootServices

Arguments:

  Event   - Event whose notification function is being invoked.
  Context - Pointer to the notification function's context, which is
            implementation-dependent.

Returns:

  None

--*/
;

EFI_STATUS
EfiInitializeRuntimeDriverLib (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable,
  IN EFI_EVENT_NOTIFY     RuntimeNotifyEventHandler
  )
/*++

Routine Description:

  Intialize Runtime Driver Lib if it has not yet been initialized. 

Arguments:

  ImageHandle     - The firmware allocated handle for the EFI image.
  
  SystemTable     - A pointer to the EFI System Table.
  
  RuntimeNotifyEventHandler     - Virtual address change notification event

Returns: 

  EFI_STATUS always returns EFI_SUCCESS

--*/
;

EFI_STATUS
EfiShutdownRuntimeDriverLib (
  VOID
  )
/*++

Routine Description:

  This routine will free some resources which have been allocated in
  EfiInitializeRuntimeDriverLib(). If a runtime driver exits with an error, 
  it must call this routine to free the allocated resource before the exiting.

Arguments:

  None

Returns: 

  EFI_SUCCESS     - Shotdown the Runtime Driver Lib successfully
  EFI_UNSUPPORTED - Runtime Driver lib was not initialized at all

--*/
;

EFI_STATUS
EfiInitializeSmmDriverLib (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
/*++

Routine Description:

  Intialize Smm Driver Lib if it has not yet been initialized. 

Arguments:

  ImageHandle     - The firmware allocated handle for the EFI image.
  
  SystemTable     - A pointer to the EFI System Table.

Returns: 

  EFI_STATUS always returns EFI_SUCCESS

--*/
;

EFI_STATUS
EfiLibGetSystemConfigurationTable (
  IN EFI_GUID *TableGuid,
  IN OUT VOID **Table
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
EfiAtRuntime (
  VOID
  )
/*++

Routine Description:

  Am I at runtime?

Arguments:

  None

Returns:

  TRUE      - At runtime
  FALSE     - Not at runtime

--*/
;

BOOLEAN
EfiGoneVirtual (
  VOID
  )
/*++

Routine Description:
  Return TRUE if SetVirtualAddressMap () has been called

Arguments:
  NONE

Returns: 
  TRUE - If SetVirtualAddressMap () has been called
  FALSE - If SetVirtualAddressMap () has not been called

--*/
;

EFI_STATUS
EfiLibGetSystemConfigurationTable (
  IN EFI_GUID *TableGuid,
  IN OUT VOID **Table
  )
/*++

 

Routine Description:

 

  Get table from configuration table by name

 

Arguments:

 

  TableGuid       - Table name to search

  

  Table           - Pointer to the table caller wants

 

Returns: 

 

  EFI_NOT_FOUND   - Not found the table

  

  EFI_SUCCESS     - Found the table

 

--*/

;

EFI_EVENT
RtEfiLibCreateProtocolNotifyEvent (
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

  Lock owned

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

#define EfiCopyMem  EfiCommonLibCopyMem
#define EfiSetMem   EfiCommonLibSetMem
#define EfiZeroMem  EfiCommonLibZeroMem

INTN
EfiCompareMem (
  IN VOID     *MemOne,
  IN VOID     *MemTwo,
  IN UINTN    Len
  )
/*++

Routine Description:

  Compares two memory buffers of a given length.

Arguments:

  MemOne - First memory buffer

  MemTwo - Second memory buffer

  Len    - Length of Mem1 and Mem2 memory regions to compare

Returns:

  = 0     if MemOne == MemTwo
  
  > 0     if MemOne > MemTwo
  
  < 0     if MemOne < MemTwo

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
// Wrapper for EFI runtime functions
//
VOID
EfiResetSystem (
  IN EFI_RESET_TYPE               ResetType,
  IN EFI_STATUS                   ResetStatus,
  IN UINTN                        DataSize,
  IN CHAR16                       *ResetData
  )
/*++

Routine Description:

  Resets the entire platform.

Arguments:

  ResetType   - The type of reset to perform.
  ResetStatus - The status code for the reset.
  DataSize    - The size, in bytes, of ResetData.
  ResetData   - A data buffer that includes a Null-terminated Unicode string, optionally
                followed by additional binary data.

Returns:

  None

--*/
;

EFI_STATUS
EfiGetNextHighMonotonicCount (
  OUT UINT32                      *HighCount
  )
/*++

Routine Description:

  Returns the next high 32 bits of the platform's monotonic counter.

Arguments:

  HighCount - Pointer to returned value.

Returns:

  Status code

--*/
;

EFI_STATUS
EfiGetTime (
  OUT EFI_TIME                    *Time,
  OUT EFI_TIME_CAPABILITIES       *Capabilities
  )
/*++

Routine Description:

  Returns the current time and date information, and the time-keeping 
  capabilities of the hardware platform.

Arguments:

  Time          - A pointer to storage to receive a snapshot of the current time.
  Capabilities  - An optional pointer to a buffer to receive the real time clock device's
                  capabilities.

Returns:

  Status code

--*/
;

EFI_STATUS
EfiSetTime (
  OUT EFI_TIME                    *Time
  )
/*++

Routine Description:

  Sets the current local time and date information.

Arguments:

  Time  - A pointer to the current time.

Returns:

  Status code

--*/
;

EFI_STATUS
EfiGetWakeupTime (
  OUT BOOLEAN                     *Enabled,
  OUT BOOLEAN                     *Pending,
  OUT EFI_TIME                    *Time
  )
/*++

Routine Description:

  Returns the current wakeup alarm clock setting.

Arguments:

  Enabled - Indicates if the alarm is currently enabled or disabled.
  Pending - Indicates if the alarm signal is pending and requires acknowledgement.
  Time    - The current alarm setting.

Returns:

  Status code

--*/
;

EFI_STATUS
EfiSetWakeupTime (
  IN BOOLEAN                      Enable,
  IN EFI_TIME                     *Time
  )
/*++

Routine Description:

  Sets the system wakeup alarm clock time.

Arguments:

  Enable  - Enable or disable the wakeup alarm.
  Time    - If Enable is TRUE, the time to set the wakeup alarm for.
            If Enable is FALSE, then this parameter is optional, and may be NULL.

Returns:

  Status code

--*/
;

EFI_STATUS
EfiGetVariable (
  IN CHAR16                       *VariableName,
  IN EFI_GUID                     * VendorGuid,
  OUT UINT32                      *Attributes OPTIONAL,
  IN OUT UINTN                    *DataSize,
  OUT VOID                        *Data
  )
/*++

Routine Description:

  Returns the value of a variable.

Arguments:

  VariableName  - A Null-terminated Unicode string that is the name of the
                  vendor's variable.
  VendorGuid    - A unique identifier for the vendor.
  Attributes    - If not NULL, a pointer to the memory location to return the
                  attributes bitmask for the variable.
  DataSize      - On input, the size in bytes of the return Data buffer.
                  On output the size of data returned in Data.
  Data          - The buffer to return the contents of the variable.

Returns:

  Status code

--*/
;

EFI_STATUS
EfiGetNextVariableName (
  IN OUT UINTN                    *VariableNameSize,
  IN OUT CHAR16                   *VariableName,
  IN OUT EFI_GUID                 *VendorGuid
  )
/*++

Routine Description:

  Enumerates the current variable names.

Arguments:

  VariableNameSize  - The size of the VariableName buffer.
  VariableName      - On input, supplies the last VariableName that was returned
                      by GetNextVariableName(). 
                      On output, returns the Nullterminated Unicode string of the
                      current variable.
  VendorGuid        - On input, supplies the last VendorGuid that was returned by
                      GetNextVariableName(). 
                      On output, returns the VendorGuid of the current variable.

Returns:

  Status code

--*/
;

EFI_STATUS
EfiSetVariable (
  IN CHAR16                       *VariableName,
  IN EFI_GUID                     *VendorGuid,
  IN UINT32                       Attributes,
  IN UINTN                        DataSize,
  IN VOID                         *Data
  )
/*++

Routine Description:

  Sets the value of a variable.

Arguments:

  VariableName  - A Null-terminated Unicode string that is the name of the
                  vendor's variable.
  VendorGuid    - A unique identifier for the vendor.
  Attributes    - Attributes bitmask to set for the variable.
  DataSize      - The size in bytes of the Data buffer.
  Data          - The contents for the variable.

Returns:

  Status code

--*/
;

#if (EFI_SPECIFICATION_VERSION >= 0x00020000)

EFI_STATUS
EfiQueryVariableInfo (
  IN UINT32           Attributes,
  OUT UINT64          *MaximumVariableStorageSize,
  OUT UINT64          *RemainingVariableStorageSize,
  OUT UINT64          *MaximumVariableSize
  )
/*++

Routine Description:

  This code returns information about the EFI variables.

Arguments:

  Attributes                      Attributes bitmask to specify the type of variables 
                                  on which to return information.
  MaximumVariableStorageSize      Pointer to the maximum size of the storage space available
                                  for the EFI variables associated with the attributes specified.
  RemainingVariableStorageSize    Pointer to the remaining size of the storage space available 
                                  for the EFI variables associated with the attributes specified.
  MaximumVariableSize             Pointer to the maximum size of the individual EFI variables
                                  associated with the attributes specified.

Returns:

  Status code

--*/
;
#endif


EFI_STATUS
EfiReportStatusCode (
  IN EFI_STATUS_CODE_TYPE     CodeType,
  IN EFI_STATUS_CODE_VALUE    Value,
  IN UINT32                   Instance,
  IN EFI_GUID                 * CallerId,
  IN EFI_STATUS_CODE_DATA     * Data OPTIONAL
  )
/*++

Routine Description:

  Status Code reporter

Arguments:
  
  CodeType    - Type of Status Code.
  
  Value       - Value to output for Status Code.
  
  Instance    - Instance Number of this status code.
  
  CallerId    - ID of the caller of this status code.
  
  Data        - Optional data associated with this status code.

Returns:

  Status code
  
--*/
;

EFI_STATUS
EfiConvertPointer (
  IN UINTN                     DebugDisposition,
  IN OUT VOID                  *Address
  )
/*++

Routine Description:

  Determines the new virtual address that is to be used on subsequent memory accesses.

Arguments:

  DebugDisposition  - Supplies type information for the pointer being converted.
  Address           - A pointer to a pointer that is to be fixed to be the value needed
                      for the new virtual address mappings being applied.

Returns:

  Status code

--*/
;

EFI_STATUS
EfiConvertList (
  IN UINTN                DebugDisposition,
  IN OUT EFI_LIST_ENTRY   *ListHead
  )
/*++

Routine Description:

  Conver the standard Lib double linked list to a virtual mapping.

Arguments:

  DebugDisposition - Argument to EfiConvertPointer (EFI 1.0 API)

  ListHead         - Head of linked list to convert

Returns: 

  EFI_SUCCESS

--*/
;

//
//  Base IO Class Functions
//
EFI_STATUS
EfiIoRead (
  IN     EFI_CPU_IO_PROTOCOL_WIDTH  Width,
  IN     UINT64                     Address,
  IN     UINTN                      Count,
  IN OUT VOID                       *Buffer
  )
/*++

Routine Description:
  Perform an IO read into Buffer.

Arguments:
  Width   - Width of read transaction, and repeat operation to use
  Address - IO address to read
  Count   - Number of times to read the IO address.
  Buffer  - Buffer to read data into. size is Width * Count

Returns: 
  Status code

--*/
;

UINT8
IoRead8 (
  IN  UINT64    Address
  )
/*++

Routine Description:
  Do a one byte IO read

Arguments:
  Address - IO address to read

Returns: 
  Data read

--*/
;

UINT16
IoRead16 (
  IN  UINT64    Address
  )
/*++

Routine Description:
  Do a two byte IO read

Arguments:
  Address - IO address to read

Returns: 
  Data read

--*/
;

UINT32
IoRead32 (
  IN  UINT64    Address
  )
/*++

Routine Description:
  Do a four byte IO read

Arguments:
  Address - IO address to read

Returns: 
  Data read

--*/
;

EFI_STATUS
EfiIoWrite (
  IN     EFI_CPU_IO_PROTOCOL_WIDTH  Width,
  IN     UINT64                     Address,
  IN     UINTN                      Count,
  IN OUT VOID                       *Buffer
  )
/*++

Routine Description:
  Perform an IO write into Buffer.

Arguments:
  Width   - Width of write transaction, and repeat operation to use
  Address - IO address to write
  Count   - Number of times to write the IO address.
  Buffer  - Buffer to write data from. size is Width * Count

Returns: 
  Status code

--*/
;

VOID
IoWrite8 (
  IN  UINT64    Address,
  IN  UINT8     Data
  )
/*++

Routine Description:
  Do a one byte IO write

Arguments:
  Address - IO address to write
  Data    - Data to write to Address

Returns: 
  NONE

--*/
;

VOID
IoWrite16 (
  IN  UINT64    Address,
  IN  UINT16    Data
  )
/*++

Routine Description:
  Do a two byte IO write

Arguments:
  Address - IO address to write
  Data    - Data to write to Address

Returns: 
  NONE

--*/
;

VOID
IoWrite32 (
  IN  UINT64    Address,
  IN  UINT32    Data
  )
/*++

Routine Description:
  Do a four byte IO write

Arguments:
  Address - IO address to write
  Data    - Data to write to Address

Returns: 
  NONE

--*/
;

EFI_STATUS
EfiMemRead (
  IN     EFI_CPU_IO_PROTOCOL_WIDTH  Width,
  IN     UINT64                     Address,
  IN     UINTN                      Count,
  IN  OUT VOID                      *Buffer
  )
/*++

Routine Description:
  Perform a Memory mapped IO read into Buffer.

Arguments:
  Width   - Width of each read transaction.
  Address - Memory mapped IO address to read
  Count   - Number of Width quanta to read
  Buffer  - Buffer to read data into. size is Width * Count

Returns: 
  Status code

--*/
;

UINT8
MemRead8 (
  IN  UINT64    Address
  )
/*++

Routine Description:
  Do a one byte Memory mapped IO read

Arguments:
  Address - Memory mapped IO address to read

Returns: 
  Data read

--*/
;

UINT16
MemRead16 (
  IN  UINT64    Address
  )
/*++

Routine Description:
  Do a two byte Memory mapped IO read

Arguments:
  Address - Memory mapped IO address to read

Returns: 
  Data read

--*/
;

UINT32
MemRead32 (
  IN  UINT64    Address
  )
/*++

Routine Description:
  Do a four byte Memory mapped IO read

Arguments:
  Address - Memory mapped IO address to read

Returns: 
  Data read

--*/
;

UINT64
MemRead64 (
  IN  UINT64    Address
  )
/*++

Routine Description:
  Do a eight byte Memory mapped IO read

Arguments:
  Address - Memory mapped IO address to read

Returns: 
  Data read

--*/
;

EFI_STATUS
EfiMemWrite (
  IN     EFI_CPU_IO_PROTOCOL_WIDTH  Width,
  IN     UINT64                     Address,
  IN     UINTN                      Count,
  IN OUT VOID                       *Buffer
  )
/*++

Routine Description:
  Perform a memory mapped IO write into Buffer.

Arguments:
  Width   - Width of write transaction, and repeat operation to use
  Address - IO address to write
  Count   - Number of times to write the IO address.
  Buffer  - Buffer to write data from. size is Width * Count

Returns: 
  Status code

--*/
;

VOID
MemWrite8 (
  IN  UINT64    Address,
  IN  UINT8     Data
  )
/*++

Routine Description:
  Do a one byte Memory mapped IO write

Arguments:
  Address - Memory mapped IO address to write
  Data    - Data to write to Address

Returns: 
  NONE

--*/
;

VOID
MemWrite16 (
  IN  UINT64    Address,
  IN  UINT16    Data
  )
/*++

Routine Description:
  Do a two byte Memory mapped IO write

Arguments:
  Address - Memory mapped IO address to write
  Data    - Data to write to Address

Returns: 
  NONE

--*/
;

VOID
MemWrite32 (
  IN  UINT64    Address,
  IN  UINT32    Data
  )
/*++

Routine Description:
  Do a four byte Memory mapped IO write

Arguments:
  Address - Memory mapped IO address to write
  Data    - Data to write to Address

Returns: 
  NONE

--*/
;

VOID
MemWrite64 (
  IN  UINT64    Address,
  IN  UINT64    Data
  )
/*++

Routine Description:
  Do a eight byte Memory mapped IO write

Arguments:
  Address - Memory mapped IO address to write
  Data    - Data to write to Address

Returns: 
  NONE

--*/
;

//
//  Platform specific functions
//
UINT8
PciRead8 (
  UINT8   Segment,
  UINT8   Bus,
  UINT8   DevFunc,
  UINT8   Register
  )
/*++

Routine Description:
  Perform an one byte PCI config cycle read
    
Arguments:
  Segment   - PCI Segment ACPI _SEG
  Bus       - PCI Bus
  DevFunc   - PCI Device(7:3) and Func(2:0)
  Register  - PCI config space register

Returns:
  Data read from PCI config space

--*/
;

UINT16
PciRead16 (
  UINT8   Segment,
  UINT8   Bus,
  UINT8   DevFunc,
  UINT8   Register
  )
/*++

Routine Description:
  Perform an two byte PCI config cycle read
    
Arguments:
  Segment   - PCI Segment ACPI _SEG
  Bus       - PCI Bus
  DevFunc   - PCI Device(7:3) and Func(2:0)
  Register  - PCI config space register

Returns:
  Data read from PCI config space

--*/
;

UINT32
PciRead32 (
  UINT8   Segment,
  UINT8   Bus,
  UINT8   DevFunc,
  UINT8   Register
  )
/*++

Routine Description:
  Perform an four byte PCI config cycle read
    
Arguments:
  Segment   - PCI Segment ACPI _SEG
  Bus       - PCI Bus
  DevFunc   - PCI Device(7:3) and Func(2:0)
  Register  - PCI config space register

Returns:
  Data read from PCI config space

--*/
;

VOID
PciWrite8 (
  UINT8   Segment,
  UINT8   Bus,
  UINT8   DevFunc,
  UINT8   Register,
  UINT8   Data
  )
/*++

Routine Description:
  Perform an one byte PCI config cycle write
    
Arguments:
  Segment   - PCI Segment ACPI _SEG
  Bus       - PCI Bus
  DevFunc   - PCI Device(7:3) and Func(2:0)
  Register  - PCI config space register
  Data      - Data to write

Returns:
  NONE

--*/
;

VOID
PciWrite16 (
  UINT8   Segment,
  UINT8   Bus,
  UINT8   DevFunc,
  UINT8   Register,
  UINT16  Data
  )
/*++

Routine Description:
  Perform an two byte PCI config cycle write
    
Arguments:
  Segment   - PCI Segment ACPI _SEG
  Bus       - PCI Bus
  DevFunc   - PCI Device(7:3) and Func(2:0)
  Register  - PCI config space register
  Data      - Data to write

Returns:
  NONE

--*/
;

VOID
PciWrite32 (
  UINT8   Segment,
  UINT8   Bus,
  UINT8   DevFunc,
  UINT8   Register,
  UINT32  Data
  )
/*++

Routine Description:
  Perform an four byte PCI config cycle write

Arguments:
  Segment   - PCI Segment ACPI _SEG
  Bus       - PCI Bus
  DevFunc   - PCI Device(7:3) and Func(2:0)
  Register  - PCI config space register
  Data      - Data to write

Returns:
  NONE

--*/
;

VOID
EfiStall (
  IN  UINTN   Microseconds
  )
/*++

Routine Description:
 Delay for at least the request number of microseconds
    
Arguments:
  Microseconds - Number of microseconds to delay.

Returns:
  NONE

--*/
;

//
//  FVB Services.
//
EFI_STATUS
EfiFvbInitialize (
  VOID
  )
/*++

Routine Description:
  Initialize globals and register Fvb Protocol notification function.

Arguments:
  None 

Returns: 
  EFI_SUCCESS

--*/
;

EFI_STATUS
EfiFvbShutdown (
  VOID
  )
/*++

Routine Description:
  Release resources allocated in EfiFvbInitialize.

Arguments:
  None 

Returns: 
  EFI_SUCCESS

--*/
;

EFI_STATUS
EfiFvbReadBlock (
  IN UINTN                                        Instance,
  IN EFI_LBA                                      Lba,
  IN UINTN                                        Offset,
  IN OUT UINTN                                    *NumBytes,
  IN UINT8                                        *Buffer
  )
/*++

Routine Description:
  Reads specified number of bytes into a buffer from the specified block

Arguments:
  Instance              - The FV instance to be read from
  Lba                   - The logical block address to be read from
  Offset                - Offset into the block at which to begin reading
  NumBytes              - Pointer that on input contains the total size of
                          the buffer. On output, it contains the total number
                          of bytes read
  Buffer                - Pointer to a caller allocated buffer that will be
                          used to hold the data read

Returns: 
  Status code

--*/
;

EFI_STATUS
EfiFvbWriteBlock (
  IN UINTN                                        Instance,
  IN EFI_LBA                                      Lba,
  IN UINTN                                        Offset,
  IN OUT UINTN                                    *NumBytes,
  IN UINT8                                        *Buffer
  )
/*++

Routine Description:
  Writes specified number of bytes from the input buffer to the block

Arguments:
  Instance              - The FV instance to be written to
  Lba                   - The starting logical block index to write to
  Offset                - Offset into the block at which to begin writing
  NumBytes              - Pointer that on input contains the total size of
                          the buffer. On output, it contains the total number
                          of bytes actually written
  Buffer                - Pointer to a caller allocated buffer that contains
                          the source for the write

Returns: 
  Status code

--*/
;

EFI_STATUS
EfiFvbEraseBlock (
  IN UINTN                                Instance,
  IN EFI_LBA                              Lba
  )
/*++

Routine Description:
  Erases and initializes a firmware volume block

Arguments:
  Instance              - The FV instance to be erased
  Lba                   - The logical block index to be erased
  
Returns: 
  Status code

--*/
;

EFI_STATUS
EfiFvbGetVolumeAttributes (
  IN UINTN                                Instance,
  OUT EFI_FVB_ATTRIBUTES                  *Attributes
  )
/*++

Routine Description:
  Retrieves attributes, insures positive polarity of attribute bits, returns
  resulting attributes in output parameter

Arguments:
  Instance              - The FV instance whose attributes is going to be 
                          returned
  Attributes            - Output buffer which contains attributes

Returns: 
  Status code

--*/
;

EFI_STATUS
EfiFvbSetVolumeAttributes (
  IN UINTN                                Instance,
  IN EFI_FVB_ATTRIBUTES                   Attributes
  )
/*++

Routine Description:
  Modifies the current settings of the firmware volume according to the 
  input parameter.

Arguments:
  Instance              - The FV instance whose attributes is going to be 
                          modified
  Attributes            - It is a pointer to EFI_FVB_ATTRIBUTES 
                          containing the desired firmware volume settings.

Returns: 
  Status code

--*/
;

EFI_STATUS
EfiFvbGetPhysicalAddress (
  IN UINTN                                Instance,
  OUT EFI_PHYSICAL_ADDRESS                *Address
  )
/*++

Routine Description:
  Retrieves the physical address of a memory mapped FV

Arguments:
  Instance              - The FV instance whose base address is going to be
                          returned
  Address               - Pointer to a caller allocated EFI_PHYSICAL_ADDRESS 
                          that on successful return, contains the base address
                          of the firmware volume. 

Returns: 
  Status code

--*/
;

EFI_STATUS
EfiFvbGetBlockSize (
  IN UINTN                                Instance,
  IN EFI_LBA                              Lba,
  OUT UINTN                               *BlockSize,
  OUT UINTN                               *NumOfBlocks
  )
/*++

Routine Description:
  Retrieve the size of a logical block

Arguments:
  Instance              - The FV instance whose block size is going to be
                          returned
  Lba                   - Indicates which block to return the size for.
  BlockSize             - A pointer to a caller allocated UINTN in which
                          the size of the block is returned
  NumOfBlocks           - a pointer to a caller allocated UINTN in which the
                          number of consecutive blocks starting with Lba is
                          returned. All blocks in this range have a size of
                          BlockSize

Returns: 
  EFI_SUCCESS           - The firmware volume was read successfully and 
                          contents are in Buffer

--*/
;
EFI_STATUS
EfiFvbEraseCustomBlockRange (
  IN UINTN                                Instance,
  IN EFI_LBA                              StartLba,
  IN UINTN                                OffsetStartLba,
  IN EFI_LBA                              LastLba,
  IN UINTN                                OffsetLastLba
  )
/*++

Routine Description:
  Erases and initializes a specified range of a firmware volume

Arguments:
  Instance              - The FV instance to be erased
  StartLba              - The starting logical block index to be erased
  OffsetStartLba        - Offset into the starting block at which to 
                          begin erasing
  LastLba               - The last logical block index to be erased
  OffsetLastLba         - Offset into the last block at which to end erasing

Returns: 
  Status code

--*/
;

EFI_STATUS
EfiCpuFlushCache (
  IN EFI_PHYSICAL_ADDRESS          Start,
  IN UINT64                        Length
  )
/*++

Routine Description:

  Flush cache with specified range.

Arguments:

  Start   - Start address
  Length  - Length in bytes

Returns:

  Status code

--*/
;

EFI_STATUS
EFIAPI
RtEfiCreateEventLegacyBoot (
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
RtEfiCreateEventReadyToBoot (
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
  ReadyToBootEvent  Returns the EFI event returned from gBS->CreateEvent(Ex)

Return:
  EFI_SUCCESS   - Event was created.
  Other         - Event was not created.

--*/
;

#endif
