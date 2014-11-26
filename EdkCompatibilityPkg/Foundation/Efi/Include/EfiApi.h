/*++

Copyright (c) 2004 - 2007, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  EfiApi.h

Abstract:

  EFI intrinsic definitions. This includes all EFI 1.0 boot and runtime
  services APIs.

  Drivers and applications are passed in a pointer to the EFI system table.
  The EFI system table contains pointers to the boot and runtime services
  tables.

--*/

#ifndef _EFI_API_H_
#define _EFI_API_H_

#include EFI_PROTOCOL_DEFINITION (DevicePath)
#include EFI_PROTOCOL_DEFINITION (SimpleTextIn)
#include EFI_PROTOCOL_DEFINITION (SimpleTextOut)

#if (EFI_SPECIFICATION_VERSION >= 0x00020000)
#include "EfiCapsule.h"
#else
#include "EfiStatusCode.h"
#endif

//
// Declare forward referenced data structures
//
EFI_FORWARD_DECLARATION (EFI_SYSTEM_TABLE);

//
// EFI Memory
//
typedef
EFI_BOOTSERVICE
EFI_STATUS
(EFIAPI *EFI_ALLOCATE_PAGES) (
  IN EFI_ALLOCATE_TYPE            Type,
  IN EFI_MEMORY_TYPE              MemoryType,
  IN UINTN                        NoPages,
  OUT EFI_PHYSICAL_ADDRESS        * Memory
  );

typedef
EFI_BOOTSERVICE
EFI_STATUS
(EFIAPI *EFI_FREE_PAGES) (
  IN EFI_PHYSICAL_ADDRESS         Memory,
  IN UINTN                        NoPages
  );

typedef
EFI_BOOTSERVICE
EFI_STATUS
(EFIAPI *EFI_GET_MEMORY_MAP) (
  IN OUT UINTN                    *MemoryMapSize,
  IN OUT EFI_MEMORY_DESCRIPTOR    * MemoryMap,
  OUT UINTN                       *MapKey,
  OUT UINTN                       *DescriptorSize,
  OUT UINT32                      *DescriptorVersion
  );

#define NextMemoryDescriptor(_Ptr, _Size)   ((EFI_MEMORY_DESCRIPTOR *) (((UINT8 *) (_Ptr)) + (_Size)))
#define NEXT_MEMORY_DESCRIPTOR(_Ptr, _Size) NextMemoryDescriptor (_Ptr, _Size)

typedef
EFI_BOOTSERVICE
EFI_STATUS
(EFIAPI *EFI_ALLOCATE_POOL) (
  IN EFI_MEMORY_TYPE              PoolType,
  IN UINTN                        Size,
  OUT VOID                        **Buffer
  );

typedef
EFI_BOOTSERVICE
EFI_STATUS
(EFIAPI *EFI_FREE_POOL) (
  IN VOID                         *Buffer
  );

typedef
EFI_RUNTIMESERVICE
EFI_STATUS
(EFIAPI *EFI_SET_VIRTUAL_ADDRESS_MAP) (
  IN UINTN                        MemoryMapSize,
  IN UINTN                        DescriptorSize,
  IN UINT32                       DescriptorVersion,
  IN EFI_MEMORY_DESCRIPTOR        * VirtualMap
  );

typedef
EFI_BOOTSERVICE11
EFI_STATUS
(EFIAPI *EFI_CONNECT_CONTROLLER) (
  IN  EFI_HANDLE                    ControllerHandle,
  IN  EFI_HANDLE                    * DriverImageHandle OPTIONAL,
  IN  EFI_DEVICE_PATH_PROTOCOL      * RemainingDevicePath OPTIONAL,
  IN  BOOLEAN                       Recursive
  );

typedef
EFI_BOOTSERVICE11
EFI_STATUS
(EFIAPI *EFI_DISCONNECT_CONTROLLER) (
  IN EFI_HANDLE                              ControllerHandle,
  IN EFI_HANDLE                              DriverImageHandle, OPTIONAL
  IN EFI_HANDLE                              ChildHandle        OPTIONAL
  );

//
// ConvertPointer DebugDisposition type.
//
#define EFI_OPTIONAL_POINTER  0x00000001

typedef
EFI_RUNTIMESERVICE
EFI_STATUS
(EFIAPI *EFI_CONVERT_POINTER) (
  IN UINTN                        DebugDisposition,
  IN OUT VOID                     **Address
  );

//
// EFI Event Types
//
#define EFI_EVENT_TIMER                         0x80000000
#define EFI_EVENT_RUNTIME                       0x40000000
#define EFI_EVENT_RUNTIME_CONTEXT               0x20000000

#define EFI_EVENT_NOTIFY_WAIT                   0x00000100
#define EFI_EVENT_NOTIFY_SIGNAL                 0x00000200

#define EFI_EVENT_SIGNAL_EXIT_BOOT_SERVICES     0x00000201
#define EFI_EVENT_SIGNAL_VIRTUAL_ADDRESS_CHANGE 0x60000202

#define EFI_EVENT_EFI_SIGNAL_MASK               0x000000FF
#define EFI_EVENT_EFI_SIGNAL_MAX                4

typedef
VOID
(EFIAPI *EFI_EVENT_NOTIFY) (
  IN EFI_EVENT                Event,
  IN VOID                     *Context
  );

typedef
EFI_BOOTSERVICE
EFI_STATUS
(EFIAPI *EFI_CREATE_EVENT) (
  IN UINT32                       Type,
  IN EFI_TPL                      NotifyTpl,
  IN EFI_EVENT_NOTIFY             NotifyFunction,
  IN VOID                         *NotifyContext,
  OUT EFI_EVENT                   * Event
  );

typedef
EFI_BOOTSERVICE
EFI_STATUS
(EFIAPI *EFI_CREATE_EVENT_EX) (
  IN UINT32                 Type,
  IN EFI_TPL                NotifyTpl      OPTIONAL,
  IN EFI_EVENT_NOTIFY       NotifyFunction OPTIONAL,
  IN CONST VOID             *NotifyContext OPTIONAL,
  IN CONST EFI_GUID         *EventGroup    OPTIONAL,
  OUT EFI_EVENT             *Event
  );

typedef enum {
  TimerCancel,
  TimerPeriodic,
  TimerRelative,
  TimerTypeMax
} EFI_TIMER_DELAY;

typedef
EFI_BOOTSERVICE
EFI_STATUS
(EFIAPI *EFI_SET_TIMER) (
  IN EFI_EVENT                Event,
  IN EFI_TIMER_DELAY          Type,
  IN UINT64                   TriggerTime
  );

typedef
EFI_BOOTSERVICE
EFI_STATUS
(EFIAPI *EFI_SIGNAL_EVENT) (
  IN EFI_EVENT                Event
  );

typedef
EFI_BOOTSERVICE
EFI_STATUS
(EFIAPI *EFI_WAIT_FOR_EVENT) (
  IN UINTN                    NumberOfEvents,
  IN EFI_EVENT                * Event,
  OUT UINTN                   *Index
  );

typedef
EFI_BOOTSERVICE
EFI_STATUS
(EFIAPI *EFI_CLOSE_EVENT) (
  IN EFI_EVENT                Event
  );

typedef
EFI_BOOTSERVICE
EFI_STATUS
(EFIAPI *EFI_CHECK_EVENT) (
  IN EFI_EVENT                Event
  );

//
// Task priority level
//
#define EFI_TPL_APPLICATION 4
#define EFI_TPL_CALLBACK    8
#define EFI_TPL_NOTIFY      16
#define EFI_TPL_HIGH_LEVEL  31

typedef
EFI_BOOTSERVICE
EFI_TPL
(EFIAPI *EFI_RAISE_TPL) (
  IN EFI_TPL      NewTpl
  );

typedef
EFI_BOOTSERVICE
VOID
(EFIAPI *EFI_RESTORE_TPL) (
  IN EFI_TPL      OldTpl
  );

//
// Variable attributes
//
#define EFI_VARIABLE_NON_VOLATILE       0x00000001
#define EFI_VARIABLE_BOOTSERVICE_ACCESS 0x00000002
#define EFI_VARIABLE_RUNTIME_ACCESS     0x00000004

#if (EFI_SPECIFICATION_VERSION >= 0x0002000A)
#define EFI_VARIABLE_HARDWARE_ERROR_RECORD 0x00000008
#endif

typedef
EFI_RUNTIMESERVICE
EFI_STATUS
(EFIAPI *EFI_GET_VARIABLE) (
  IN CHAR16                       *VariableName,
  IN EFI_GUID                     * VendorGuid,
  OUT UINT32                      *Attributes OPTIONAL,
  IN OUT UINTN                    *DataSize,
  OUT VOID                        *Data
  );

typedef
EFI_RUNTIMESERVICE
EFI_STATUS
(EFIAPI *EFI_GET_NEXT_VARIABLE_NAME) (
  IN OUT UINTN                    *VariableNameSize,
  IN OUT CHAR16                   *VariableName,
  IN OUT EFI_GUID                 * VendorGuid
  );

typedef
EFI_RUNTIMESERVICE
EFI_STATUS
(EFIAPI *EFI_SET_VARIABLE) (
  IN CHAR16                       *VariableName,
  IN EFI_GUID                     * VendorGuid,
  IN UINT32                       Attributes,
  IN UINTN                        DataSize,
  IN VOID                         *Data
  );

//
// EFI Time
//
typedef struct {
  UINT32  Resolution;
  UINT32  Accuracy;
  BOOLEAN SetsToZero;
} EFI_TIME_CAPABILITIES;

typedef
EFI_RUNTIMESERVICE
EFI_STATUS
(EFIAPI *EFI_GET_TIME) (
  OUT EFI_TIME                    * Time,
  OUT EFI_TIME_CAPABILITIES       * Capabilities OPTIONAL
  );

typedef
EFI_RUNTIMESERVICE
EFI_STATUS
(EFIAPI *EFI_SET_TIME) (
  IN EFI_TIME                     * Time
  );

typedef
EFI_RUNTIMESERVICE
EFI_STATUS
(EFIAPI *EFI_GET_WAKEUP_TIME) (
  OUT BOOLEAN                     *Enabled,
  OUT BOOLEAN                     *Pending,
  OUT EFI_TIME                    * Time
  );

typedef
EFI_RUNTIMESERVICE
EFI_STATUS
(EFIAPI *EFI_SET_WAKEUP_TIME) (
  IN BOOLEAN                      Enable,
  IN EFI_TIME                     * Time OPTIONAL
  );

//
// Image Entry prototype
//
typedef
EFI_STATUS
(EFIAPI *EFI_IMAGE_ENTRY_POINT) (
  IN EFI_HANDLE                   ImageHandle,
  IN EFI_SYSTEM_TABLE             * SystemTable
  );

//
// Image functions
//
typedef
EFI_BOOTSERVICE
EFI_STATUS
(EFIAPI *EFI_IMAGE_LOAD) (
  IN BOOLEAN                      BootPolicy,
  IN EFI_HANDLE                   ParentImageHandle,
  IN EFI_DEVICE_PATH_PROTOCOL     * FilePath,
  IN VOID                         *SourceBuffer OPTIONAL,
  IN UINTN                        SourceSize,
  OUT EFI_HANDLE                  * ImageHandle
  );

typedef
EFI_BOOTSERVICE
EFI_STATUS
(EFIAPI *EFI_IMAGE_START) (
  IN EFI_HANDLE                   ImageHandle,
  OUT UINTN                       *ExitDataSize,
  OUT CHAR16                      **ExitData OPTIONAL
  );

typedef
EFI_BOOTSERVICE
EFI_STATUS
(EFIAPI *EFI_EXIT) (
  IN EFI_HANDLE                   ImageHandle,
  IN EFI_STATUS                   ExitStatus,
  IN UINTN                        ExitDataSize,
  IN CHAR16                       *ExitData OPTIONAL
  );

typedef
EFI_BOOTSERVICE
EFI_STATUS
(EFIAPI *EFI_IMAGE_UNLOAD) (
  IN EFI_HANDLE                   ImageHandle
  );

typedef
EFI_BOOTSERVICE
EFI_STATUS
(EFIAPI *EFI_EXIT_BOOT_SERVICES) (
  IN EFI_HANDLE                   ImageHandle,
  IN UINTN                        MapKey
  );

//
// Misc
//
typedef
EFI_BOOTSERVICE
EFI_STATUS
(EFIAPI *EFI_STALL) (
  IN UINTN                    Microseconds
  );

typedef
EFI_BOOTSERVICE
EFI_STATUS
(EFIAPI *EFI_SET_WATCHDOG_TIMER) (
  IN UINTN                    Timeout,
  IN UINT64                   WatchdogCode,
  IN UINTN                    DataSize,
  IN CHAR16                   *WatchdogData OPTIONAL
  );

typedef enum {
  EfiResetCold,
  EfiResetWarm,
  EfiResetShutdown

#if ((TIANO_RELEASE_VERSION != 0) && (EFI_SPECIFICATION_VERSION < 0x00020000))
  , EfiResetUpdate
#endif

} EFI_RESET_TYPE;

typedef
EFI_RUNTIMESERVICE
VOID
(EFIAPI *EFI_RESET_SYSTEM) (
  IN EFI_RESET_TYPE           ResetType,
  IN EFI_STATUS               ResetStatus,
  IN UINTN                    DataSize,
  IN CHAR16                   *ResetData OPTIONAL
  );

typedef
EFI_BOOTSERVICE
EFI_STATUS
(EFIAPI *EFI_GET_NEXT_MONOTONIC_COUNT) (
  OUT UINT64                  *Count
  );

typedef
EFI_RUNTIMESERVICE
EFI_STATUS
(EFIAPI *EFI_GET_NEXT_HIGH_MONO_COUNT) (
  OUT UINT32                  *HighCount
  );

typedef
EFI_BOOTSERVICE
EFI_STATUS
(EFIAPI *EFI_CALCULATE_CRC32) (
  IN  VOID                              *Data,
  IN  UINTN                             DataSize,
  OUT UINT32                            *Crc32
  );

typedef
EFI_BOOTSERVICE
VOID
(EFIAPI *EFI_COPY_MEM) (
  IN VOID     *Destination,
  IN VOID     *Source,
  IN UINTN    Length
  );

typedef
EFI_BOOTSERVICE
VOID
(EFIAPI *EFI_SET_MEM) (
  IN VOID     *Buffer,
  IN UINTN    Size,
  IN UINT8    Value
  );

//
// Protocol handler functions
//
typedef enum {
  EFI_NATIVE_INTERFACE
} EFI_INTERFACE_TYPE;

typedef
EFI_BOOTSERVICE
EFI_STATUS
(EFIAPI *EFI_INSTALL_PROTOCOL_INTERFACE) (
  IN OUT EFI_HANDLE           * Handle,
  IN EFI_GUID                 * Protocol,
  IN EFI_INTERFACE_TYPE       InterfaceType,
  IN VOID                     *Interface
  );

typedef
EFI_BOOTSERVICE11
EFI_STATUS
(EFIAPI *EFI_INSTALL_MULTIPLE_PROTOCOL_INTERFACES) (
  IN OUT EFI_HANDLE           * Handle,
  ...
  );

typedef
EFI_BOOTSERVICE
EFI_STATUS
(EFIAPI *EFI_REINSTALL_PROTOCOL_INTERFACE) (
  IN EFI_HANDLE               Handle,
  IN EFI_GUID                 * Protocol,
  IN VOID                     *OldInterface,
  IN VOID                     *NewInterface
  );

typedef
EFI_BOOTSERVICE
EFI_STATUS
(EFIAPI *EFI_UNINSTALL_PROTOCOL_INTERFACE) (
  IN EFI_HANDLE               Handle,
  IN EFI_GUID                 * Protocol,
  IN VOID                     *Interface
  );

typedef
EFI_BOOTSERVICE11
EFI_STATUS
(EFIAPI *EFI_UNINSTALL_MULTIPLE_PROTOCOL_INTERFACES) (
  IN EFI_HANDLE           Handle,
  ...
  );

typedef
EFI_BOOTSERVICE
EFI_STATUS
(EFIAPI *EFI_HANDLE_PROTOCOL) (
  IN EFI_HANDLE               Handle,
  IN EFI_GUID                 * Protocol,
  OUT VOID                    **Interface
  );

#define EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL  0x00000001
#define EFI_OPEN_PROTOCOL_GET_PROTOCOL        0x00000002
#define EFI_OPEN_PROTOCOL_TEST_PROTOCOL       0x00000004
#define EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER 0x00000008
#define EFI_OPEN_PROTOCOL_BY_DRIVER           0x00000010
#define EFI_OPEN_PROTOCOL_EXCLUSIVE           0x00000020

//
// ///////////////////////////////////////////////////////////////////////////////////////////////
// OpenProtocol() Attribute Values
/////////////////////////////////////////////////////////////////////////////////////////////////
// BY_HANDLE_PROTOCOL    - Used by EFI 1.0 Drivers and Applications
//                         May not actually add an elemnt to the open list in a production build
//
// GET_PROTOCOL          - Used by EFI 1.1 Drivers to get a protocol interface
//                         May not actually add an elemnt to the open list in a production build
//
// TEST_PROTOCOL         - Used by EFI 1.1 Drivers to test for the existence of a protocol interface
//                         The interface is not returned, and it is an optional parameter tham may be NULL.
//                         May not actually add an elemnt to the open list in a production build
//
// BY_DRIVER             - Used by EFI 1.1 Drivers that are able to share a protocol with other
//                         agents other than its children.  A driver is always able to share
//                         a protocol with its children, since the driver is in control of the
//                         parent controller's and the child controller's use of the protocol.
//
// BY_DRIVER | EXCLUSIVE - Used by EFI 1.1 Drivers that will not share a protocol with any other
//                         agents except its children.  A driver is always able to share
//                         a protocol with its children, since the driver is in control of the
//                         parent controller's and the child controller's use of the protocol.
//                         This attribute will force all other drivers to disconnect from the protocol
//                         before this driver attaches.  When this driver closes the handle, the other
//                         drivers will reconnect to the protocol.
//
//
// BY_CHILD_CONTROLLER   - Used by EFI 1.1 Driver to show that a protocol is consumed by a child
//                         of the driver.  This is information used by DisconnectController() to
//                         determine the list of children that a protocol has.  It has
//                         no affect on the OpenProtocol()/ClosePrototocol() behavior.
//
// EXCLUSIVE             - Used by EFI 1.1 Applications to gain exclusive access to a protocol.
//                         All drivers are disconnected from the handle while the application has
//                         the handle open.  These drivers are reconnected when the application
//                         closes the handle.
//
/////////////////////////////////////////////////////////////////////////////////////////////////
// OpenProtocol() behavior based on Attribute values
/////////////////////////////////////////////////////////////////////////////////////////////////
//
// OpenProtocol (Handle, Protocol, Interface, ImageHandle, DeviceHandle, Attributes)
// * EFI_UNSUPPORTED        if Protocol does not exist on Handle
// * EFI_INVALID_PARAMETER  if Handle is not a valid handle.
// * EFI_INVALID_PARAMETER  if Protocol is NULL or not a valid GUID
// * EFI_INVALID_PARAMETER  if Interface is NULL
// * EFI_INVALID_PARAMETER  if Attributes is not one of the following values:
//                            BY_HANDLE_PROTOCOL
//                            GET_PROTOCOL
//                            TEST_PROTOCOL
//                            BY_CHILD_CONTROLLER
//                            BY_DRIVER
//                            BY_DRIVER | EXCLUSIVE
//                            EXCLUSIVE
// * EFI_INVALID_PARAMETER  if Attributes BY_CHILD_CONTROLLER and ImageHandle is not a valid handle
// * EFI_INVALID_PARAMETER  if Attributes BY_CHILD_CONTROLLER and DeviceHandle is not a valid handle
// * EFI_INVALID_PARAMETER  if Attributes BY_CHILD_CONTROLLER and Handle == DeviceHandle
// * EFI_INVALID_PARAMETER  if Attributes BY_DRIVER and ImageHandle is not a valid handle
// * EFI_INVALID_PARAMETER  if Attributes BY_DRIVER and DeviceHandle is not a valid handle
// * EFI_INVALID_PARAMETER  if Attributes BY_DRIVER | EXCLUSIVE and ImageHandle is not a valid handle
// * EFI_INVALID_PARAMETER  if Attributes BY_DRIVER | EXCLUSIVE and DeviceHandle is not a valid handle
// * EFI_INVALID_PARAMETER  if Attributes EXCLUSIVE and ImageHandle is not a valid handle
//
// OpenProtocol() Attributes = BY_HANDLE_PROTOCOL, GET_PROTOCOL, TEST_PROTOCOL, BY_CHILD_CONTROLLER
// * EFI_SUCCESS      if Protocol exists on the Handle
//
// OpenProtocol() Attributes = BY_DRIVER
// * EFI_SUCCESS        if there are no items in the Open List for (Handle, Protocol)
// * EFI_SUCCESS        if there are only items in the Open List for (Handle, Protocol)
//                      that have the one of the following Attributes
//                        BY_HANDLE_PROTOCOL
//                        GET_PROTOCOL
//                        TEST_PROTOCOL
//                        BY_CHILD_CONTROLLER
// * EFI_ACCESS_DENIED  if there are any items in the Open List for (Handle, Protocol)
//                      that have the one of the following Attributes
//                        BY_DRIVER
//                      AND ImageHandle != OpenListItem.IH
// * EFI_ALREADY_STARTED if there are any items in the Open List for (Handle, Protocol)
//                       that have the one of the following Attributes
//                         BY_DRIVER
//                       AND ImageHandle == OpenListItem.IH
// * EFI_ACCESS_DENIED  if there are any items in the Open List for (Handle, Protocol)
//                      that have the one of the following Attributes
//                        BY_DRIVER | EXCLUSIVE
//                        EXCLUSIVE
//
// OpenProtocol() Attributes = BY_DRIVER | EXCLUSIVE
// * EFI_SUCCESS        if there are no items in the Open List for (Handle, Protocol)
// * EFI_SUCCESS        if there are only items in the Open List for (Handle, Protocol)
//                      that have the one of the following Attributes
//                        BY_HANDLE_PROTOCOL
//                        GET_PROTOCOL
//                        TEST_PROTOCOL
//                        BY_CHILD_CONTROLLER
// * EFI_SUCCESS        if there are any items in the Open List for (Handle, Protocol)
//                      that have the one of the following Attributes
//                        BY_DRIVER
//                      AND the driver is removed by DisconnectController(IH,DH)
// * EFI_ALREADY_STARTED if there are any items in the Open List for (Handle, Protocol)
//                       that have the one of the following Attributes
//                         BY_DRIVER | EXCLUSIVE
//                       AND ImageHandle == OpenListItem.IH
// * EFI_ACCESS_DENIED  if there are any items in the Open List for (Handle, Protocol)
//                      that have the one of the following Attributes
//                        BY_DRIVER
//                      AND the driver can not be removed by DisconnectController(IH,DH)
// * EFI_ACCESS_DENIED  if there are any items in the Open List for (Handle, Protocol)
//                      that have the one of the following Attributes
//                        BY_DRIVER | EXCLUSIVE
//                        EXCLUSIVE
//
// OpenProtocol() Attributes = EXCLUSIVE
// * EFI_SUCCESS        if there are no items in the Open List for (Handle, Protocol)
// * EFI_SUCCESS        if there are only items in the Open List for (Handle, Protocol)
//                      that have the one of the following Attributes
//                        BY_HANDLE_PROTOCOL
//                        GET_PROTOCOL
//                        TEST_PROTOCOL
//                        BY_CHILD_CONTROLLER
// * EFI_SUCCESS        if there are any items in the Open List for (Handle, Protocol)
//                      that have the one of the following Attributes
//                        BY_DRIVER
//                      AND the driver is removed by DisconnectController(IH,DH)
// * EFI_ACCESS_DENIED  if there are any items in the Open List for (Handle, Protocol)
//                      that have the one of the following Attributes
//                        BY_DRIVER
//                      AND the driver can not be removed by DisconnectController(IH,DH)
// * EFI_ACCESS_DENIED  if there are any items in the Open List for (Handle, Protocol)
//                      that have the one of the following Attributes
//                        BY_DRIVER | EXCLUSIVE
//                        EXCLUSIVE
//
/////////////////////////////////////////////////////////////////////////////////////////////////
// CloseProtocol() Behavior based on the Attributes of the item being closed and the items
//                 remaining on the Open List
/////////////////////////////////////////////////////////////////////////////////////////////////
// CloseProtocol(Handle, Protocol, ImageHandle, DeviceHandle)
// CloseProtocol() Attributes of item = BY_HANDLE_PROTOCOL,
//                                      GET_PROTOCOL
//                                      TEST_PROTOCOL
//                                      BY_CHILD_CONTROLLER,
//                                      BY_DRIVER
//                                      BY_DRIVER | EXCLUSIVE
//                                      EXCLUSIVE
//   EFI_NOT_FOUND          if Protocol does not exist on Handle
//   EFI_INVALID_PARAMETER  if Handle is not a valid handle.
//   EFI_INVALID_PARAMETER  if Protocol is NULL or not a valid GUID
//   EFI_INVALID_PARAMETER  if ImageHandle is not a valid handle
//   EFI_INVALID_PARAMETER  if DeviceHandle is not a valid handle
//   EFI_NOT_FOUND      if (ImageHandle, DeviceHandle) is not present in the Open List
//                      for (Handle, Protocol)
//   EFI_ACCESS_DENIED  if (ImageHandle, DeviceHandle) is present in the Open List
//                      for (Handle, Protocol), but the item can not be removed.
//   EFI_SUCCESS        if (ImageHandle, DeviceHandle) is present in the Open List
//                      for (Handle, Protocol), and the item can be removed.
//
/////////////////////////////////////////////////////////////////////////////////////////////////
// UninstallProtocolInterface() behavior
/////////////////////////////////////////////////////////////////////////////////////////////////
//
// UninstallProtocolInterface (Handle, Protocol, Interface)
//
//   EFI_INVALID_PARAMETER if Handle is not a valid handle.
//   EFI_INVALID_PARAMETER if Protocol is not a vlaid GUID
//   EFI_NOT_FOUND         if Handle doe not support Protocol
//   EFI_NOT_FOUND         if the interface for (Handle, Protocol) does not match Interface
//   EFI_ACCESS_DENIED     if the list of Open Items for (Handle, Protocol) can not be removed
//   EFI_SUCCESS           if the list of Open Items is empty, and Protocol is removed from Handle
//
// Algorithm to remove Open Item List:
//
// Loop through all Open Item List entries
//   if (OpenItem.Attributes & BY_DRIVER) then
//     DisconnectController (OpenItem.IH, OpenItem.DH)
//   end if
// end loop
// Loop through all Open Item List entries
//   if (OpenItem.Attributes & BY_HANDLE_PROTOCOL or GET_PROTOCOL or TEST_PROTOCOL) then
//     CloseProtocol (Handle, Protocol, OpenItem.IH, OpenItem.DH)
//   end if
// end loop
// if Open Item List is empty then remove Protocol from Handle and return EFI_SUCCESS
// if Open Item List is not empty then return EFI_ACCESS_DENIED
//
/////////////////////////////////////////////////////////////////////////////////////////////////
typedef
EFI_BOOTSERVICE11
EFI_STATUS
(EFIAPI *EFI_OPEN_PROTOCOL) (
  IN EFI_HANDLE                 Handle,
  IN EFI_GUID                   * Protocol,
  OUT VOID                      **Interface,
  IN  EFI_HANDLE                ImageHandle,
  IN  EFI_HANDLE                ControllerHandle, OPTIONAL
  IN  UINT32                    Attributes
  );

typedef
EFI_BOOTSERVICE11
EFI_STATUS
(EFIAPI *EFI_CLOSE_PROTOCOL) (
  IN EFI_HANDLE               Handle,
  IN EFI_GUID                 * Protocol,
  IN EFI_HANDLE               ImageHandle,
  IN EFI_HANDLE               DeviceHandle
  );

typedef struct {
  EFI_HANDLE  AgentHandle;
  EFI_HANDLE  ControllerHandle;
  UINT32      Attributes;
  UINT32      OpenCount;
} EFI_OPEN_PROTOCOL_INFORMATION_ENTRY;

typedef
EFI_BOOTSERVICE11
EFI_STATUS
(EFIAPI *EFI_OPEN_PROTOCOL_INFORMATION) (
  IN  EFI_HANDLE                          UserHandle,
  IN  EFI_GUID                            * Protocol,
  IN  EFI_OPEN_PROTOCOL_INFORMATION_ENTRY **EntryBuffer,
  OUT UINTN                               *EntryCount
  );

typedef
EFI_BOOTSERVICE11
EFI_STATUS
(EFIAPI *EFI_PROTOCOLS_PER_HANDLE) (
  IN EFI_HANDLE       UserHandle,
  OUT EFI_GUID        ***ProtocolBuffer,
  OUT UINTN           *ProtocolBufferCount
  );

typedef
EFI_BOOTSERVICE
EFI_STATUS
(EFIAPI *EFI_REGISTER_PROTOCOL_NOTIFY) (
  IN EFI_GUID                 * Protocol,
  IN EFI_EVENT                Event,
  OUT VOID                    **Registration
  );

typedef enum {
  AllHandles,
  ByRegisterNotify,
  ByProtocol
} EFI_LOCATE_SEARCH_TYPE;

typedef
EFI_BOOTSERVICE
EFI_STATUS
(EFIAPI *EFI_LOCATE_HANDLE) (
  IN EFI_LOCATE_SEARCH_TYPE   SearchType,
  IN EFI_GUID                 * Protocol OPTIONAL,
  IN VOID                     *SearchKey OPTIONAL,
  IN OUT UINTN                *BufferSize,
  OUT EFI_HANDLE              * Buffer
  );

typedef
EFI_BOOTSERVICE
EFI_STATUS
(EFIAPI *EFI_LOCATE_DEVICE_PATH) (
  IN EFI_GUID                         * Protocol,
  IN OUT EFI_DEVICE_PATH_PROTOCOL     **DevicePath,
  OUT EFI_HANDLE                      * Device
  );

typedef
EFI_BOOTSERVICE
EFI_STATUS
(EFIAPI *EFI_INSTALL_CONFIGURATION_TABLE) (
  IN EFI_GUID                 * Guid,
  IN VOID                     *Table
  );

typedef
EFI_BOOTSERVICE
EFI_STATUS
(EFIAPI *EFI_RESERVED_SERVICE) (
  VOID
  );

typedef
EFI_BOOTSERVICE11
EFI_STATUS
(EFIAPI *EFI_LOCATE_HANDLE_BUFFER) (
  IN EFI_LOCATE_SEARCH_TYPE       SearchType,
  IN EFI_GUID                     * Protocol OPTIONAL,
  IN VOID                         *SearchKey OPTIONAL,
  IN OUT UINTN                    *NumberHandles,
  OUT EFI_HANDLE                  **Buffer
  );

typedef
EFI_BOOTSERVICE11
EFI_STATUS
(EFIAPI *EFI_LOCATE_PROTOCOL) (
  EFI_GUID  * Protocol,
  VOID      *Registration, OPTIONAL
  VOID      **Interface
  );

//
// Definition of Status Code extended data header
//
//  HeaderSize    The size of the architecture. This is specified to enable
//                the future expansion
//
//  Size          The size of the data in bytes. This does not include the size
//                of the header structure.
//
//  Type          A GUID defining the type of the data
//
//
#if ((TIANO_RELEASE_VERSION != 0) && (EFI_SPECIFICATION_VERSION < 0x00020000))

typedef
EFI_RUNTIMESERVICE
EFI_STATUS
(EFIAPI *EFI_REPORT_STATUS_CODE) (
  IN EFI_STATUS_CODE_TYPE     Type,
  IN EFI_STATUS_CODE_VALUE    Value,
  IN UINT32                   Instance,
  IN EFI_GUID                 * CallerId OPTIONAL,
  IN EFI_STATUS_CODE_DATA     * Data OPTIONAL
  );

#endif

#if (EFI_SPECIFICATION_VERSION >= 0x00020000)

typedef
EFI_RUNTIMESERVICE
EFI_STATUS
(EFIAPI *EFI_UPDATE_CAPSULE) (
  IN EFI_CAPSULE_HEADER     **CapsuleHeaderArray,
  IN UINTN                  CapsuleCount,
  IN EFI_PHYSICAL_ADDRESS   ScatterGatherList OPTIONAL
 );


typedef
EFI_RUNTIMESERVICE
EFI_STATUS
(EFIAPI *EFI_QUERY_CAPSULE_CAPABILITIES) (
  IN  EFI_CAPSULE_HEADER   **CapsuleHeaderArray,
  IN  UINTN                CapsuleCount,
  OUT UINT64               *MaximumCapsuleSize,
  OUT EFI_RESET_TYPE       *ResetType
);

typedef
EFI_RUNTIMESERVICE
EFI_STATUS
(EFIAPI *EFI_QUERY_VARIABLE_INFO) (
  IN UINT32           Attributes,
  OUT UINT64          *MaximumVariableStorageSize,
  OUT UINT64          *RemainingVariableStorageSize,
  OUT UINT64          *MaximumVariableSize
  );

#endif

//
// EFI Runtime Services Table
//
#define EFI_RUNTIME_SERVICES_SIGNATURE  0x56524553544e5552ULL
#define EFI_RUNTIME_SERVICES_REVISION   EFI_SPECIFICATION_VERSION

typedef struct {
  EFI_TABLE_HEADER              Hdr;

  //
  // Time services
  //
  EFI_GET_TIME                  GetTime;
  EFI_SET_TIME                  SetTime;
  EFI_GET_WAKEUP_TIME           GetWakeupTime;
  EFI_SET_WAKEUP_TIME           SetWakeupTime;

  //
  // Virtual memory services
  //
  EFI_SET_VIRTUAL_ADDRESS_MAP   SetVirtualAddressMap;
  EFI_CONVERT_POINTER           ConvertPointer;

  //
  // Variable services
  //
  EFI_GET_VARIABLE              GetVariable;
  EFI_GET_NEXT_VARIABLE_NAME    GetNextVariableName;
  EFI_SET_VARIABLE              SetVariable;

  //
  // Misc
  //
  EFI_GET_NEXT_HIGH_MONO_COUNT  GetNextHighMonotonicCount;
  EFI_RESET_SYSTEM              ResetSystem;

#if (EFI_SPECIFICATION_VERSION >= 0x00020000)
  //
  // New Boot Service added by UEFI 2.0
  //
  EFI_UPDATE_CAPSULE             UpdateCapsule;
  EFI_QUERY_CAPSULE_CAPABILITIES QueryCapsuleCapabilities;
  EFI_QUERY_VARIABLE_INFO        QueryVariableInfo;
#elif (TIANO_RELEASE_VERSION != 0)
  //
  // Tiano extension to EFI 1.10 runtime table
  // It was moved to a protocol to not conflict with UEFI 2.0
  // If Tiano is disabled, this item is not enabled for EFI1.10
  //
  EFI_REPORT_STATUS_CODE        ReportStatusCode;
#endif

} EFI_RUNTIME_SERVICES;

//
// EFI Boot Services Table
//
#define EFI_BOOT_SERVICES_SIGNATURE 0x56524553544f4f42ULL
#define EFI_BOOT_SERVICES_REVISION  EFI_SPECIFICATION_VERSION

typedef struct {
  EFI_TABLE_HEADER                            Hdr;

  //
  // Task priority functions
  //
  EFI_RAISE_TPL                               RaiseTPL;
  EFI_RESTORE_TPL                             RestoreTPL;

  //
  // Memory functions
  //
  EFI_ALLOCATE_PAGES                          AllocatePages;
  EFI_FREE_PAGES                              FreePages;
  EFI_GET_MEMORY_MAP                          GetMemoryMap;
  EFI_ALLOCATE_POOL                           AllocatePool;
  EFI_FREE_POOL                               FreePool;

  //
  // Event & timer functions
  //
  EFI_CREATE_EVENT                            CreateEvent;
  EFI_SET_TIMER                               SetTimer;
  EFI_WAIT_FOR_EVENT                          WaitForEvent;
  EFI_SIGNAL_EVENT                            SignalEvent;
  EFI_CLOSE_EVENT                             CloseEvent;
  EFI_CHECK_EVENT                             CheckEvent;

  //
  // Protocol handler functions
  //
  EFI_INSTALL_PROTOCOL_INTERFACE              InstallProtocolInterface;
  EFI_REINSTALL_PROTOCOL_INTERFACE            ReinstallProtocolInterface;
  EFI_UNINSTALL_PROTOCOL_INTERFACE            UninstallProtocolInterface;
  EFI_HANDLE_PROTOCOL                         HandleProtocol;
  VOID                                        *Reserved;
  EFI_REGISTER_PROTOCOL_NOTIFY                RegisterProtocolNotify;
  EFI_LOCATE_HANDLE                           LocateHandle;
  EFI_LOCATE_DEVICE_PATH                      LocateDevicePath;
  EFI_INSTALL_CONFIGURATION_TABLE             InstallConfigurationTable;

  //
  // Image functions
  //
  EFI_IMAGE_LOAD                              LoadImage;
  EFI_IMAGE_START                             StartImage;
  EFI_EXIT                                    Exit;
  EFI_IMAGE_UNLOAD                            UnloadImage;
  EFI_EXIT_BOOT_SERVICES                      ExitBootServices;

  //
  // Misc functions
  //
  EFI_GET_NEXT_MONOTONIC_COUNT                GetNextMonotonicCount;
  EFI_STALL                                   Stall;
  EFI_SET_WATCHDOG_TIMER                      SetWatchdogTimer;

  //
  // ////////////////////////////////////////////////////
  // EFI 1.1 Services
    //////////////////////////////////////////////////////
  //
  // DriverSupport Services
  //
  EFI_CONNECT_CONTROLLER                      ConnectController;
  EFI_DISCONNECT_CONTROLLER                   DisconnectController;

  //
  // Added Open and Close protocol for the new driver model
  //
  EFI_OPEN_PROTOCOL                           OpenProtocol;
  EFI_CLOSE_PROTOCOL                          CloseProtocol;
  EFI_OPEN_PROTOCOL_INFORMATION               OpenProtocolInformation;

  //
  // Added new services to EFI 1.1 as Lib to reduce code size.
  //
  EFI_PROTOCOLS_PER_HANDLE                    ProtocolsPerHandle;
  EFI_LOCATE_HANDLE_BUFFER                    LocateHandleBuffer;
  EFI_LOCATE_PROTOCOL                         LocateProtocol;

  EFI_INSTALL_MULTIPLE_PROTOCOL_INTERFACES    InstallMultipleProtocolInterfaces;
  EFI_UNINSTALL_MULTIPLE_PROTOCOL_INTERFACES  UninstallMultipleProtocolInterfaces;

  //
  // CRC32 services
  //
  EFI_CALCULATE_CRC32                         CalculateCrc32;

  //
  // Memory Utility Services
  //
  EFI_COPY_MEM                                CopyMem;
  EFI_SET_MEM                                 SetMem;
#if (EFI_SPECIFICATION_VERSION >= 0x00020000)
  //
  // UEFI 2.0 Extension to the table
  //
  EFI_CREATE_EVENT_EX                         CreateEventEx;
#endif

} EFI_BOOT_SERVICES;

//
// EFI Configuration Table
//
typedef struct {
  EFI_GUID  VendorGuid;
  VOID      *VendorTable;
} EFI_CONFIGURATION_TABLE;

//
// EFI System Table
//
#define EFI_SYSTEM_TABLE_SIGNATURE      0x5453595320494249ULL
#define EFI_SYSTEM_TABLE_REVISION       EFI_SPECIFICATION_VERSION
#define EFI_1_02_SYSTEM_TABLE_REVISION  ((1 << 16) | 02)
#define EFI_1_10_SYSTEM_TABLE_REVISION  ((1 << 16) | 10)
#define EFI_2_00_SYSTEM_TABLE_REVISION  ((2 << 16) | 00)
#define EFI_2_10_SYSTEM_TABLE_REVISION  ((2 << 16) | 10)

struct _EFI_SYSTEM_TABLE {
  EFI_TABLE_HEADER              Hdr;

  CHAR16                        *FirmwareVendor;
  UINT32                        FirmwareRevision;

  EFI_HANDLE                    ConsoleInHandle;
  EFI_SIMPLE_TEXT_IN_PROTOCOL   *ConIn;

  EFI_HANDLE                    ConsoleOutHandle;
  EFI_SIMPLE_TEXT_OUT_PROTOCOL  *ConOut;

  EFI_HANDLE                    StandardErrorHandle;
  EFI_SIMPLE_TEXT_OUT_PROTOCOL  *StdErr;

  EFI_RUNTIME_SERVICES          *RuntimeServices;
  EFI_BOOT_SERVICES             *BootServices;

  UINTN                         NumberOfTableEntries;
  EFI_CONFIGURATION_TABLE       *ConfigurationTable;

};

#endif
