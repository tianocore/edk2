/** @file
  Provides library functions for common UEFI operations. Only available to DXE
  and UEFI module types.

  The UEFI Library provides functions and macros that simplify the development of 
  UEFI Drivers and UEFI Applications.  These functions and macros help manage EFI 
  events, build simple locks utilizing EFI Task Priority Levels (TPLs), install 
  EFI Driver Model related protocols, manage Unicode string tables for UEFI Drivers, 
  and print messages on the console output and standard error devices.

Copyright (c) 2006 - 2008, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __UEFI_LIB_H__
#define __UEFI_LIB_H__

#include <Protocol/DriverBinding.h>
#include <Protocol/DriverConfiguration.h>
#include <Protocol/ComponentName.h>
#include <Protocol/ComponentName2.h>
#include <Protocol/DriverDiagnostics.h>
#include <Protocol/DriverDiagnostics2.h>

#include <Library/BaseLib.h>

///
/// Unicode String Table
///
typedef struct {
  CHAR8   *Language;
  CHAR16  *UnicodeString;
} EFI_UNICODE_STRING_TABLE;

///
/// EFI Lock Status
///
typedef enum {
  EfiLockUninitialized = 0,
  EfiLockReleased      = 1,
  EfiLockAcquired      = 2
} EFI_LOCK_STATE;

///
/// EFI Lock 
///
typedef struct {
  EFI_TPL         Tpl;
  EFI_TPL         OwnerTpl;
  EFI_LOCK_STATE  Lock;
} EFI_LOCK;


/**
  Macro that returns the number of 100 ns units for a specified number of microseconds.
  Useful for managing EFI timer events.

  @param  Microseconds           Number of microseonds.

  @return The number of 100 ns units equivalent to the number of microseconds specified
          by Microseconds.

**/
#define EFI_TIMER_PERIOD_MICROSECONDS(Microseconds) MultU64x32((UINT64)(Microseconds), 10)


/**
  Macro that returns the number of 100 ns units for a specified number of milliseoconds.
  Useful for managing EFI timer events.

  @param  Milliseconds           Number of milliseconds.

  @return The number of 100 ns units equivalent to the number of milliseconds specified
          by Milliseconds.

**/
#define EFI_TIMER_PERIOD_MILLISECONDS(Milliseconds) MultU64x32((UINT64)(Milliseconds), 10000)


/**
  Macro that returns the number of 100 ns units for a specified number of seoconds.
  Useful for managing EFI timer events.

  @param  Seconds                Number of seconds.

  @return The number of 100 ns units equivalent to the number of seconds specified
          by Seconds.

**/
#define EFI_TIMER_PERIOD_SECONDS(Seconds)           MultU64x32((UINT64)(Seconds), 10000000)


/**
  This function searches the list of configuration tables stored in the EFI System 
  Table for a table with a GUID that matches TableGuid.  If a match is found, 
  then a pointer to the configuration table is returned in Table, and EFI_SUCCESS 
  is returned.  If a matching GUID is not found, then EFI_NOT_FOUND is returned.

  @param  TableGuid       Pointer to table's GUID type..
  @param  Table           Pointer to the table associated with TableGuid in the EFI System Table.

  @retval EFI_SUCCESS     A configuration table matching TableGuid was found.
  @retval EFI_NOT_FOUND   A configuration table matching TableGuid could not be found.

**/
EFI_STATUS
EFIAPI
EfiGetSystemConfigurationTable (  
  IN  EFI_GUID  *TableGuid,
  OUT VOID      **Table
  );

/**
  This function causes the notification function to be executed for every protocol 
  of type ProtocolGuid instance that exists in the system when this function is 
  invoked.  In addition, every time a protocol of type ProtocolGuid instance is 
  installed or reinstalled, the notification function is also executed.

  @param  ProtocolGuid    Supplies GUID of the protocol upon whose installation the event is fired.
  @param  NotifyTpl       Supplies the task priority level of the event notifications.
  @param  NotifyFunction  Supplies the function to notify when the event is signaled.
  @param  NotifyContext   The context parameter to pass to NotifyFunction.
  @param  Registration    A pointer to a memory location to receive the registration value.

  @return The notification event that was created. 

**/
EFI_EVENT
EFIAPI
EfiCreateProtocolNotifyEvent(
  IN  EFI_GUID          *ProtocolGuid,
  IN  EFI_TPL           NotifyTpl,
  IN  EFI_EVENT_NOTIFY  NotifyFunction,
  IN  VOID              *NotifyContext,  OPTIONAL
  OUT VOID              **Registration
  );

/**
  This function creates an event using NotifyTpl, NoifyFunction, and NotifyContext.
  This event is signaled with EfiNamedEventSignal().  This provide the ability for 
  one or more listeners on the same event named by the GUID specified by Name.

  @param  Name                  Supplies GUID name of the event.
  @param  NotifyTpl             Supplies the task priority level of the event notifications.
  @param  NotifyFunction        Supplies the function to notify when the event is signaled.
  @param  NotifyContext         The context parameter to pass to NotifyFunction. 
  @param  Registration          A pointer to a memory location to receive the registration value.

  @retval EFI_SUCCESS           A named event was created.
  @retval EFI_OUT_OF_RESOURCES  There are not enough resource to create the named event.

**/
EFI_STATUS
EFIAPI
EfiNamedEventListen (
  IN CONST EFI_GUID    *Name,
  IN EFI_TPL           NotifyTpl,
  IN EFI_EVENT_NOTIFY  NotifyFunction,
  IN CONST VOID        *NotifyContext,  OPTIONAL
  OUT VOID             *Registration OPTIONAL
  );

/**
  This function signals the named event specified by Name.  The named event must 
  have been created with EfiNamedEventListen().

  @param  Name                  Supplies GUID name of the event.

  @retval EFI_SUCCESS           A named event was signaled.
  @retval EFI_OUT_OF_RESOURCES  There are not enough resource to signal the named event.

**/
EFI_STATUS
EFIAPI
EfiNamedEventSignal (
  IN CONST EFI_GUID  *Name
  );

/** 
  Returns the current TPL.

  This function returns the current TPL.  There is no EFI service to directly 
  retrieve the current TPL. Instead, the RaiseTPL() function is used to raise 
  the TPL to TPL_HIGH_LEVEL.  This will return the current TPL.  The TPL level 
  can then immediately be restored back to the current TPL level with a call 
  to RestoreTPL().

  @return The current TPL.

**/
EFI_TPL
EFIAPI
EfiGetCurrentTpl (
  VOID
  );

/**
  This function initializes a basic mutual exclusion lock to the released state 
  and returns the lock.  Each lock provides mutual exclusion access at its task 
  priority level.  Since there is no preemption or multiprocessor support in EFI,
  acquiring the lock only consists of raising to the locks TPL.

  @param  Lock       A pointer to the lock data structure to initialize.
  @param  Priority   EFI TPL associated with the lock.

  @return The lock.

**/
EFI_LOCK *
EFIAPI
EfiInitializeLock (
  IN OUT EFI_LOCK  *Lock,
  IN EFI_TPL        Priority
  );

/**
  This macro initializes the contents of a basic mutual exclusion lock to the 
  released state.  Each lock provides mutual exclusion access at its task 
  priority level.  Since there is no preemption or multiprocessor support in EFI,
  acquiring the lock only consists of raising to the locks TPL.

  @param  Priority  The task priority level of the lock.

  @return The lock.

**/
#define EFI_INITIALIZE_LOCK_VARIABLE(Priority) \
  {Priority, TPL_APPLICATION, EfiLockReleased }


/**
  
  Macro that calls DebugAssert() if an EFI_LOCK structure is not in the locked state.

  If the DEBUG_PROPERTY_DEBUG_ASSERT_ENABLED bit of PcdDebugProperyMask is set, 
  then this macro evaluates the EFI_LOCK structure specified by Lock.  If Lock 
  is not in the locked state, then DebugAssert() is called passing in the source 
  filename, source line number, and Lock.

  If Lock is NULL, then ASSERT().

  @param  LockParameter  A pointer to the lock to acquire.

**/
#define ASSERT_LOCKED(LockParameter)                  \
  do {                                                \
    if (DebugAssertEnabled ()) {                      \
      ASSERT (LockParameter != NULL);                 \
      if ((LockParameter)->Lock != EfiLockAcquired) { \
        _ASSERT (LockParameter not locked);           \
      }                                               \
    }                                                 \
  } while (FALSE)


/**
  This function raises the system's current task priority level to the task 
  priority level of the mutual exclusion lock.  Then, it places the lock in the 
  acquired state.

  @param  Lock              A pointer to the lock to acquire.

**/
VOID
EFIAPI
EfiAcquireLock (
  IN EFI_LOCK  *Lock
  );

/**
  This function raises the system's current task priority level to the task 
  priority level of the mutual exclusion lock.  Then, it attempts to place the 
  lock in the acquired state.

  @param  Lock              A pointer to the lock to acquire.

  @retval EFI_SUCCESS       The lock was acquired.
  @retval EFI_ACCESS_DENIED The lock could not be acquired because it is already owned.

**/
EFI_STATUS
EFIAPI
EfiAcquireLockOrFail (
  IN EFI_LOCK  *Lock
  );

/**
  This function transitions a mutual exclusion lock from the acquired state to 
  the released state, and restores the system's task priority level to its 
  previous level.

  @param  Lock  A pointer to the lock to release.

**/
VOID
EFIAPI
EfiReleaseLock (
  IN EFI_LOCK  *Lock
  );

/**
  Tests whether a controller handle is being managed by a specific driver.

  This function tests whether the driver specified by DriverBindingHandle is
  currently managing the controller specified by ControllerHandle.  This test
  is performed by evaluating if the the protocol specified by ProtocolGuid is
  present on ControllerHandle and is was opened by DriverBindingHandle with an
  attribute of EFI_OPEN_PROTOCOL_BY_DRIVER. 
  If ProtocolGuid is NULL, then ASSERT().

  @param  ControllerHandle     A handle for a controller to test.
  @param  DriverBindingHandle  Specifies the driver binding handle for the
                               driver.
  @param  ProtocolGuid         Specifies the protocol that the driver specified
                               by DriverBindingHandle opens in its Start()
                               function.

  @retval EFI_SUCCESS          ControllerHandle is managed by the driver
                               specifed by DriverBindingHandle.
  @retval EFI_UNSUPPORTED      ControllerHandle is not managed by the driver
                               specifed by DriverBindingHandle.

**/
EFI_STATUS
EFIAPI
EfiTestManagedDevice (
  IN CONST EFI_HANDLE       ControllerHandle,
  IN CONST EFI_HANDLE       DriverBindingHandle,
  IN CONST EFI_GUID         *ProtocolGuid
  );

/**
  Tests whether a child handle is a child device of the controller.

  This function tests whether ChildHandle is one of the children of
  ControllerHandle.  This test is performed by checking to see if the protocol
  specified by ProtocolGuid is present on ControllerHandle and opened by
  ChildHandle with an attribute of EFI_OPEN_PROTOCOL_BY_CHILD_CONTROLLER.
  If ProtocolGuid is NULL, then ASSERT().

  @param  ControllerHandle     A handle for a (parent) controller to test. 
  @param  ChildHandle          A child handle to test.
  @param  ProtocolGuid         Supplies the protocol that the child controller
                               opens on its parent controller. 

  @retval EFI_SUCCESS          ChildHandle is a child of the ControllerHandle.
  @retval EFI_UNSUPPORTED      ChildHandle is not a child of the
                               ControllerHandle.

**/
EFI_STATUS
EFIAPI
EfiTestChildHandle (
  IN CONST EFI_HANDLE       ControllerHandle,
  IN CONST EFI_HANDLE       ChildHandle,
  IN CONST EFI_GUID         *ProtocolGuid
  );

/**
  This function looks up a Unicode string in UnicodeStringTable.  If Language is 
  a member of SupportedLanguages and a Unicode string is found in UnicodeStringTable
  that matches the language code specified by Language, then it is returned in 
  UnicodeString.

  @param  Language                A pointer to the ISO 639-2 language code for the 
                                  Unicode string to look up and return.
  @param  SupportedLanguages      A pointer to the set of ISO 639-2 language codes 
                                  that the Unicode string table supports.  Language 
                                  must be a member of this set.
  @param  UnicodeStringTable      A pointer to the table of Unicode strings.
  @param  UnicodeString           A pointer to the Unicode string from UnicodeStringTable
                                  that matches the language specified by Language.

  @retval EFI_SUCCESS             The Unicode string that matches the language 
                                  specified by Language was found
                                  in the table of Unicoide strings UnicodeStringTable, 
                                  and it was returned in UnicodeString.
  @retval EFI_INVALID_PARAMETER   Language is NULL.
  @retval EFI_INVALID_PARAMETER   UnicodeString is NULL.
  @retval EFI_UNSUPPORTED         SupportedLanguages is NULL.
  @retval EFI_UNSUPPORTED         UnicodeStringTable is NULL.
  @retval EFI_UNSUPPORTED         The language specified by Language is not a 
                                  member of SupportedLanguages.
  @retval EFI_UNSUPPORTED         The language specified by Language is not 
                                  supported by UnicodeStringTable.

**/
EFI_STATUS
EFIAPI
LookupUnicodeString (
  IN CONST CHAR8                     *Language,
  IN CONST CHAR8                     *SupportedLanguages,
  IN CONST EFI_UNICODE_STRING_TABLE  *UnicodeStringTable,
  OUT CHAR16                         **UnicodeString
  );

/**
  This function looks up a Unicode string in UnicodeStringTable.
  If Language is a member of SupportedLanguages and a Unicode
  string is found in UnicodeStringTable that matches the
  language code specified by Language, then it is returned in
  UnicodeString.

  @param  Language                A pointer to the ISO 639-2 or
                                  RFC 3066 language code for the
                                  Unicode string to look up and
                                  return.
  
  @param  SupportedLanguages      A pointer to the set of ISO
                                  639-2 or RFC 3066 language
                                  codes that the Unicode string
                                  table supports. Language must
                                  be a member of this set.
  
  @param  UnicodeStringTable      A pointer to the table of
                                  Unicode strings.
  
  @param  UnicodeString           A pointer to the Unicode
                                  string from UnicodeStringTable
                                  that matches the language
                                  specified by Language.

  @param  Iso639Language          Specify the language code
                                  format supported. If true,
                                  then the format follow ISO
                                  639-2. If false, then it
                                  follows RFC3066.

  @retval  EFI_SUCCESS            The Unicode string that
                                  matches the language specified
                                  by Language was found in the
                                  table of Unicoide strings
                                  UnicodeStringTable, and it was
                                  returned in UnicodeString.
  
  @retval  EFI_INVALID_PARAMETER  Language is NULL.
  
  @retval  EFI_INVALID_PARAMETER  UnicodeString is NULL.
  
  @retval  EFI_UNSUPPORTED        SupportedLanguages is NULL.
  
  @retval  EFI_UNSUPPORTED        UnicodeStringTable is NULL.
  
  @retval  EFI_UNSUPPORTED        The language specified by
                                  Language is not a member
                                  ofSupportedLanguages.
  
  @retval EFI_UNSUPPORTED         The language specified by
                                  Language is not supported by
                                  UnicodeStringTable.

**/
EFI_STATUS
EFIAPI
LookupUnicodeString2 (
  IN CONST CHAR8                     *Language,
  IN CONST CHAR8                     *SupportedLanguages,
  IN CONST EFI_UNICODE_STRING_TABLE  *UnicodeStringTable,
  OUT CHAR16                         **UnicodeString,
  IN BOOLEAN                         Iso639Language
  );

/**
  This function adds a Unicode string to UnicodeStringTable.
  If Language is a member of SupportedLanguages then UnicodeString is added to 
  UnicodeStringTable.  New buffers are allocated for both Language and 
  UnicodeString.  The contents of Language and UnicodeString are copied into 
  these new buffers.  These buffers are automatically freed when 
  FreeUnicodeStringTable() is called.

  @param  Language                A pointer to the ISO 639-2 language code for the Unicode 
                                  string to add.
  @param  SupportedLanguages      A pointer to the set of ISO 639-2 language codes
                                  that the Unicode string table supports.
                                  Language must be a member of this set.
  @param  UnicodeStringTable      A pointer to the table of Unicode strings.
  @param  UnicodeString           A pointer to the Unicode string to add.

  @retval EFI_SUCCESS             The Unicode string that matches the language 
                                  specified by Language was found in the table of 
                                  Unicode strings UnicodeStringTable, and it was 
                                  returned in UnicodeString.
  @retval EFI_INVALID_PARAMETER   Language is NULL.
  @retval EFI_INVALID_PARAMETER   UnicodeString is NULL.
  @retval EFI_INVALID_PARAMETER   UnicodeString is an empty string.
  @retval EFI_UNSUPPORTED         SupportedLanguages is NULL.
  @retval EFI_ALREADY_STARTED     A Unicode string with language Language is 
                                  already present in UnicodeStringTable.
  @retval EFI_OUT_OF_RESOURCES    There is not enough memory to add another 
                                  Unicode string to UnicodeStringTable.
  @retval EFI_UNSUPPORTED         The language specified by Language is not a 
                                  member of SupportedLanguages.

**/
EFI_STATUS
EFIAPI
AddUnicodeString (
  IN CONST CHAR8               *Language,
  IN CONST CHAR8               *SupportedLanguages,
  IN EFI_UNICODE_STRING_TABLE  **UnicodeStringTable,
  IN CONST CHAR16              *UnicodeString
  );

/**
  
  This function adds a Unicode string to UnicodeStringTable.
  If Language is a member of SupportedLanguages then
  UnicodeString is added to UnicodeStringTable.  New buffers are
  allocated for both Language and UnicodeString.  The contents
  of Language and UnicodeString are copied into these new
  buffers.  These buffers are automatically freed when
  FreeUnicodeStringTable() is called.

  @param  Language                A pointer to the ISO 639-2 or
                                  RFC 3066 language code for the
                                  Unicode string to add.
  
  @param  SupportedLanguages      A pointer to the set of ISO
                                  639-2 or RFC 3066 language
                                  codes that the Unicode string
                                  table supports. Language must
                                  be a member of this set.
  
  @param  UnicodeStringTable      A pointer to the table of
                                  Unicode strings.
  
  @param  UnicodeString           A pointer to the Unicode
                                  string to add.
  
  @param  Iso639Language          Specify the language code
                                  format supported. If true,
                                  then the format follow ISO
                                  639-2. If false, then it
                                  follows RFC3066.

  @retval EFI_SUCCESS             The Unicode string that
                                  matches the language specified
                                  by Language was found in the
                                  table of Unicode strings
                                  UnicodeStringTable, and it was
                                  returned in UnicodeString.
  
  @retval EFI_INVALID_PARAMETER   Language is NULL.
  
  @retval EFI_INVALID_PARAMETER   UnicodeString is NULL.
  
  @retval EFI_INVALID_PARAMETER   UnicodeString is an empty string.
  
  @retval EFI_UNSUPPORTED         SupportedLanguages is NULL.
  
  @retval EFI_ALREADY_STARTED     A Unicode string with language
                                  Language is already present in
                                  UnicodeStringTable.
  
  @retval EFI_OUT_OF_RESOURCES    There is not enough memory to
                                  add another Unicode string to
                                  UnicodeStringTable.
  
  @retval EFI_UNSUPPORTED         The language specified by
                                  Language is not a member of
                                  SupportedLanguages.

**/
EFI_STATUS
EFIAPI
AddUnicodeString2 (
  IN CONST CHAR8               *Language,
  IN CONST CHAR8               *SupportedLanguages,
  IN EFI_UNICODE_STRING_TABLE  **UnicodeStringTable,
  IN CONST CHAR16              *UnicodeString,
  IN BOOLEAN                   Iso639Language
  );

/**
  This function frees the table of Unicode strings in UnicodeStringTable.
  If UnicodeStringTable is NULL, then EFI_SUCCESS is returned.
  Otherwise, each language code, and each Unicode string in the Unicode string 
  table are freed, and EFI_SUCCESS is returned.

  @param  UnicodeStringTable  A pointer to the table of Unicode strings.

  @retval EFI_SUCCESS         The Unicode string table was freed.

**/
EFI_STATUS
EFIAPI
FreeUnicodeStringTable (
  IN EFI_UNICODE_STRING_TABLE  *UnicodeStringTable
  );

/**
  This function computes and returns the width of the Unicode character 
  specified by UnicodeChar.

  @param  UnicodeChar   A Unicode character.

  @retval 0             The width if UnicodeChar could not be determined.
  @retval 1             UnicodeChar is a narrow glyph.
  @retval 2             UnicodeChar is a wide glyph.

**/
UINTN
EFIAPI
GetGlyphWidth (
  IN CHAR16  UnicodeChar
  );

/**
  This function computes and returns the display length of
  the Null-terminated Unicode string specified by String.
  If String is NULL, then 0 is returned.
  If any of the widths of the Unicode characters in String
  can not be determined, then 0 is returned.

  @param  String      A pointer to a Null-terminated Unicode string.

  @return The display length of the Null-terminated Unicode string specified by String.
  
**/
UINTN
EFIAPI
UnicodeStringDisplayLength (
  IN CONST CHAR16  *String
  );

//
// Functions that abstract early Framework contamination of UEFI.
//
/**
  Signal a Ready to Boot Event.  
  
  Create a Ready to Boot Event. Signal it and close it. This causes other 
  events of the same event group to be signaled in other modules. 

**/
VOID
EFIAPI
EfiSignalEventReadyToBoot (
  VOID
  );

/**
  Signal a Legacy Boot Event.  
  
  Create a legacy Boot Event. Signal it and close it. This causes other 
  events of the same event group to be signaled in other modules. 

**/
VOID
EFIAPI
EfiSignalEventLegacyBoot (
  VOID
  );

/**
  Creates an EFI event in the Legacy Boot Event Group.  Prior to UEFI 2.0 this 
  was done via a non blessed UEFI extensions and this library abstracts the 
  implementation mechanism of this event from the caller.
  
  This function abstracts the creation of the Legacy Boot Event.  The Framework 
  moved from a proprietary to UEFI 2.0 based mechanism.  This library abstracts 
  the caller from how this event is created to prevent to code form having to 
  change with the version of the specification supported.
  If LegacyBootEvent is NULL, then ASSERT().

  @param  LegacyBootEvent   Returns the EFI event returned from gBS->CreateEvent(Ex).

  @retval EFI_SUCCESS       Event was created.
  @retval Other             Event was not created.

**/
EFI_STATUS
EFIAPI
EfiCreateEventLegacyBoot (
  OUT EFI_EVENT  *LegacyBootEvent
  );

/**
  Create an EFI event in the Legacy Boot Event Group and allows
  the caller to specify a notification function.  
  
  This function abstracts the creation of the Legacy Boot Event.
  The Framework moved from a proprietary to UEFI 2.0 based mechanism.
  This library abstracts the caller from how this event is created to prevent
  to code form having to change with the version of the specification supported.
  If LegacyBootEvent is NULL, then ASSERT().

  @param  NotifyTpl         The task priority level of the event.
  @param  NotifyFunction    The notification function to call when the event is signaled.
  @param  NotifyContext     The content to pass to NotifyFunction when the event is signaled.
  @param  LegacyBootEvent   Returns the EFI event returned from gBS->CreateEvent(Ex).

  @retval EFI_SUCCESS       Event was created.
  @retval Other             Event was not created.

**/
EFI_STATUS
EFIAPI
EfiCreateEventLegacyBootEx (
  IN  EFI_TPL           NotifyTpl,
  IN  EFI_EVENT_NOTIFY  NotifyFunction,  OPTIONAL
  IN  VOID              *NotifyContext,  OPTIONAL
  OUT EFI_EVENT         *LegacyBootEvent
  );

/**
  Create an EFI event in the Ready To Boot Event Group.  Prior to UEFI 2.0 this 
  was done via a non-standard UEFI extension, and this library abstracts the 
  implementation mechanism of this event from the caller. 
  
  This function abstracts the creation of the Ready to Boot Event.  The Framework 
  moved from a proprietary to UEFI 2.0-based mechanism.  This library abstracts 
  the caller from how this event is created to prevent the code form having to 
  change with the version of the specification supported.
  If ReadyToBootEvent is NULL, then ASSERT().

  @param  ReadyToBootEvent   Returns the EFI event returned from gBS->CreateEvent(Ex).

  @retval EFI_SUCCESS       Event was created.
  @retval Other             Event was not created.

**/
EFI_STATUS
EFIAPI
EfiCreateEventReadyToBoot (
  OUT EFI_EVENT  *ReadyToBootEvent
  );

/**
  Create an EFI event in the Ready To Boot Event Group and allows
  the caller to specify a notification function.  
  
  This function abstracts the creation of the Ready to Boot Event.
  The Framework moved from a proprietary to UEFI 2.0 based mechanism.
  This library abstracts the caller from how this event is created to prevent
  to code form having to change with the version of the specification supported.
  If ReadyToBootEvent is NULL, then ASSERT().

  @param  NotifyTpl         The task priority level of the event.
  @param  NotifyFunction    The notification function to call when the event is signaled.
  @param  NotifyContext     The content to pass to NotifyFunction when the event is signaled.
  @param  ReadyToBootEvent  Returns the EFI event returned from gBS->CreateEvent(Ex).

  @retval EFI_SUCCESS       Event was created.
  @retval Other             Event was not created.

**/
EFI_STATUS
EFIAPI
EfiCreateEventReadyToBootEx (
  IN  EFI_TPL           NotifyTpl,
  IN  EFI_EVENT_NOTIFY  NotifyFunction,  OPTIONAL
  IN  VOID              *NotifyContext,  OPTIONAL
  OUT EFI_EVENT         *ReadyToBootEvent
  );

/**
  Initialize a Firmware Volume (FV) Media Device Path node.
  
  The Framework FwVol Device Path changed to conform to the UEFI 2.0 specification.  
  This library function abstracts initializing a device path node.
  
  Initialize the MEDIA_FW_VOL_FILEPATH_DEVICE_PATH data structure.  This device 
  path changed in the DXE CIS version 0.92 in a non back ward compatible way to 
  not conflict with the UEFI 2.0 specification.  This function abstracts the 
  differences from the caller.
  
  If FvDevicePathNode is NULL, then ASSERT().
  If NameGuid is NULL, then ASSERT().
  
  @param  FvDevicePathNode  Pointer to a FV device path node to initialize
  @param  NameGuid          FV file name to use in FvDevicePathNode

**/
VOID
EFIAPI
EfiInitializeFwVolDevicepathNode (
  IN OUT MEDIA_FW_VOL_FILEPATH_DEVICE_PATH  *FvDevicePathNode,
  IN CONST EFI_GUID                         *NameGuid
  );

/**
  Check to see if the Firmware Volume (FV) Media Device Path is valid 
  
  The Framework FwVol Device Path changed to conform to the UEFI 2.0 specification.  
  This library function abstracts validating a device path node.
  
  Check the MEDIA_FW_VOL_FILEPATH_DEVICE_PATH data structure to see if it's valid.  
  If it is valid, then return the GUID file name from the device path node.  Otherwise, 
  return NULL.  This device path changed in the DXE CIS version 0.92 in a non back ward 
  compatible way to not conflict with the UEFI 2.0 specification.  This function abstracts 
  the differences from the caller.
  If FvDevicePathNode is NULL, then ASSERT().

  @param  FvDevicePathNode  Pointer to FV device path to check.

  @retval NULL              FvDevicePathNode is not valid.
  @retval Other             FvDevicePathNode is valid and pointer to NameGuid was returned.

**/
EFI_GUID *
EFIAPI
EfiGetNameGuidFromFwVolDevicePathNode (
  IN CONST MEDIA_FW_VOL_FILEPATH_DEVICE_PATH  *FvDevicePathNode
  );

/** 
  Prints a formatted Unicode string to the console output device specified by 
  ConOut defined in the EFI_SYSTEM_TABLE.

  This function prints a formatted Unicode string to the console output device 
  specified by ConOut in EFI_SYSTEM_TABLE and returns the number of Unicode 
  characters that printed to ConOut.  If the length of the formatted Unicode 
  string is greater than PcdUefiLibMaxPrintBufferSize, then only the first 
  PcdUefiLibMaxPrintBufferSize characters are sent to ConOut.

  @param Format   Null-terminated Unicode format string.
  @param ...      VARARG list consumed to process Format.
  If Format is NULL, then ASSERT().
  If Format is not aligned on a 16-bit boundary, then ASSERT().
  
  @return Number of Unicode characters printed to ConOut.

**/
UINTN
EFIAPI
Print (
  IN CONST CHAR16  *Format,
  ...
  );

/** 
  Prints a formatted Unicode string to the console output device specified by 
  StdErr defined in the EFI_SYSTEM_TABLE.

  This function prints a formatted Unicode string to the console output device 
  specified by StdErr in EFI_SYSTEM_TABLE and returns the number of Unicode 
  characters that printed to StdErr.  If the length of the formatted Unicode 
  string is greater than PcdUefiLibMaxPrintBufferSize, then only the first 
  PcdUefiLibMaxPrintBufferSize characters are sent to StdErr.

  @param Format   Null-terminated Unicode format string.
  @param ...      VARARG list consumed to process Format.
  If Format is NULL, then ASSERT().
  If Format is not aligned on a 16-bit boundary, then ASSERT().
  
  @return Number of Unicode characters printed to StdErr.

**/
UINTN
EFIAPI
ErrorPrint (
  IN CONST CHAR16  *Format,
  ...
  );

/** 
  Prints a formatted ASCII string to the console output device specified by 
  ConOut defined in the EFI_SYSTEM_TABLE.

  This function prints a formatted ASCII string to the console output device 
  specified by ConOut in EFI_SYSTEM_TABLE and returns the number of ASCII 
  characters that printed to ConOut.  If the length of the formatted ASCII 
  string is greater than PcdUefiLibMaxPrintBufferSize, then only the first 
  PcdUefiLibMaxPrintBufferSize characters are sent to ConOut.

  @param Format   Null-terminated ASCII format string.
  @param ...      VARARG list consumed to process Format.
  If Format is NULL, then ASSERT().
  If Format is not aligned on a 16-bit boundary, then ASSERT().
  
  @return Number of ASCII characters printed to ConOut.

**/
UINTN
EFIAPI
AsciiPrint (
  IN CONST CHAR8  *Format,
  ...
  );

/** 
  Prints a formatted ASCII string to the console output device specified by 
  StdErr defined in the EFI_SYSTEM_TABLE.

  This function prints a formatted ASCII string to the console output device 
  specified by StdErr in EFI_SYSTEM_TABLE and returns the number of ASCII 
  characters that printed to StdErr.  If the length of the formatted ASCII 
  string is greater than PcdUefiLibMaxPrintBufferSize, then only the first 
  PcdUefiLibMaxPrintBufferSize characters are sent to StdErr.

  @param Format   Null-terminated ASCII format string.
  @param ...      VARARG list consumed to process Format.
  If Format is NULL, then ASSERT().
  If Format is not aligned on a 16-bit boundary, then ASSERT().
  
  @return Number of ASCII characters printed to ConErr.

**/
UINTN
EFIAPI
AsciiErrorPrint (
  IN CONST CHAR8  *Format,
  ...
  );

/**
  Initializes a driver by installing the Driver Binding Protocol onto the driver's
  DriverBindingHandle.  This is typically the same as the driver's ImageHandle, but
  it can be different if the driver produces multiple DriverBinding Protocols. 
  If the Driver Binding Protocol interface is NULL, then ASSERT (). 
  If the installation fails, then ASSERT ().

  @param  ImageHandle                 The image handle of the driver.
  @param  SystemTable                 The EFI System Table that was passed to the driver's entry point.
  @param  DriverBinding               A Driver Binding Protocol instance that this driver is producing.
  @param  DriverBindingHandle         The handle that DriverBinding is to be installed onto.  If this
                                      parameter is NULL, then a new handle is created.

  @retval EFI_SUCCESS                 The protocol installation is completed successfully.
  @retval Others                      Status from gBS->InstallMultipleProtocolInterfaces().

**/
EFI_STATUS
EFIAPI
EfiLibInstallDriverBinding (
  IN CONST EFI_HANDLE             ImageHandle,
  IN CONST EFI_SYSTEM_TABLE       *SystemTable,
  IN EFI_DRIVER_BINDING_PROTOCOL  *DriverBinding,
  IN EFI_HANDLE                   DriverBindingHandle
  );


/**
  Initializes a driver by installing the Driver Binding Protocol together with the optional Component Name,
  Driver Configure and Driver Diagnostic Protocols onto the driver's DriverBindingHandle.  This is
  typically the same as the driver's ImageHandle, but it can be different if the driver produces multiple
  DriverBinding Protocols. 
  If the Driver Binding Protocol interface is NULL, then ASSERT (). 
  If the installation fails, then ASSERT ().

  @param  ImageHandle                 The image handle of the driver.
  @param  SystemTable                 The EFI System Table that was passed to the driver's entry point.
  @param  DriverBinding               A Driver Binding Protocol instance that this driver is producing.
  @param  DriverBindingHandle         The handle that DriverBinding is to be installed onto.  If this
                                      parameter is NULL, then a new handle is created.
  @param  ComponentName               A Component Name Protocol instance that this driver is producing.
  @param  DriverConfiguration         A Driver Configuration Protocol instance that this driver is producing.
  @param  DriverDiagnostics           A Driver Diagnostics Protocol instance that this driver is producing.

  @retval EFI_SUCCESS                 The protocol installation is completed successfully.
  @retval Others                      Status from gBS->InstallMultipleProtocolInterfaces().

**/
EFI_STATUS
EFIAPI
EfiLibInstallAllDriverProtocols (
  IN CONST EFI_HANDLE                         ImageHandle,
  IN CONST EFI_SYSTEM_TABLE                   *SystemTable,
  IN EFI_DRIVER_BINDING_PROTOCOL              *DriverBinding,
  IN EFI_HANDLE                               DriverBindingHandle,
  IN CONST EFI_COMPONENT_NAME_PROTOCOL        *ComponentName,       OPTIONAL
  IN CONST EFI_DRIVER_CONFIGURATION_PROTOCOL  *DriverConfiguration, OPTIONAL
  IN CONST EFI_DRIVER_DIAGNOSTICS_PROTOCOL    *DriverDiagnostics    OPTIONAL
  );



/**
  Initializes a driver by installing the Driver Binding Protocol together with the optional Component Name,
  Component Name 2 onto the driver's DriverBindingHandle.  This is typically the same as the driver's
  ImageHandle, but it can be different if the driver produces multiple DriverBinding Protocols. 
  If the Driver Binding Protocol interface is NULL, then ASSERT (). 
  If the installation fails, then ASSERT ().

  @param  ImageHandle                 The image handle of the driver.
  @param  SystemTable                 The EFI System Table that was passed to the driver's entry point.
  @param  DriverBinding               A Driver Binding Protocol instance that this driver is producing.
  @param  DriverBindingHandle         The handle that DriverBinding is to be installed onto.  If this
                                      parameter is NULL, then a new handle is created.
  @param  ComponentName               A Component Name Protocol instance that this driver is producing.
  @param  ComponentName2              A Component Name 2 Protocol instance that this driver is producing.

  @retval EFI_SUCCESS                 The protocol installation is completed successfully.
  @retval Others                      Status from gBS->InstallMultipleProtocolInterfaces().

**/
EFI_STATUS
EFIAPI
EfiLibInstallDriverBindingComponentName2 (
  IN CONST EFI_HANDLE                         ImageHandle,
  IN CONST EFI_SYSTEM_TABLE                   *SystemTable,
  IN EFI_DRIVER_BINDING_PROTOCOL              *DriverBinding,
  IN EFI_HANDLE                               DriverBindingHandle,
  IN CONST EFI_COMPONENT_NAME_PROTOCOL        *ComponentName,       OPTIONAL
  IN CONST EFI_COMPONENT_NAME2_PROTOCOL       *ComponentName2       OPTIONAL
  );


/**
  Intialize a driver by installing the Driver Binding Protocol together with the optional Component Name,
  Component Name 2, Driver Configure, Driver Diagnostic and Driver Diagnostic 2 Protocols onto the driver's
  DriverBindingHandle.  This is typically the same as the driver's ImageHandle, but it can be different if
  the driver produces multiple DriverBinding Protocols. 
  If the Drvier Binding Protocol interface is NULL, then ASSERT (). 
  If the installation fails, then ASSERT ().

  @param  ImageHandle                 The image handle of the driver.
  @param  SystemTable                 The EFI System Table that was passed to the driver's entry point.
  @param  DriverBinding               A Driver Binding Protocol instance that this driver is producing.
  @param  DriverBindingHandle         The handle that DriverBinding is to be installe onto.  If this
                                      parameter is NULL, then a new handle is created.
  @param  ComponentName               A Component Name Protocol instance that this driver is producing.
  @param  ComponentName2              A Component Name 2 Protocol instance that this driver is producing.
  @param  DriverConfiguration         A Driver Configuration Protocol instance that this driver is producing.
  @param  DriverConfiguration2        A Driver Configuration Protocol 2 instance that this driver is producing.
  @param  DriverDiagnostics           A Driver Diagnostics Protocol instance that this driver is producing.
  @param  DriverDiagnostics2          A Driver Diagnostics Protocol 2 instance that this driver is producing.

  @retval EFI_SUCCESS                 The protocol installation is completed successfully.
  @retval Others                      Status from gBS->InstallMultipleProtocolInterfaces().

**/
EFI_STATUS
EFIAPI
EfiLibInstallAllDriverProtocols2 (
  IN CONST EFI_HANDLE                         ImageHandle,
  IN CONST EFI_SYSTEM_TABLE                   *SystemTable,
  IN EFI_DRIVER_BINDING_PROTOCOL              *DriverBinding,
  IN EFI_HANDLE                               DriverBindingHandle,
  IN CONST EFI_COMPONENT_NAME_PROTOCOL        *ComponentName,        OPTIONAL
  IN CONST EFI_COMPONENT_NAME2_PROTOCOL       *ComponentName2,       OPTIONAL
  IN CONST EFI_DRIVER_CONFIGURATION_PROTOCOL  *DriverConfiguration,  OPTIONAL
  IN CONST EFI_DRIVER_CONFIGURATION_PROTOCOL  *DriverConfiguration2, OPTIONAL
  IN CONST EFI_DRIVER_DIAGNOSTICS_PROTOCOL    *DriverDiagnostics,    OPTIONAL
  IN CONST EFI_DRIVER_DIAGNOSTICS2_PROTOCOL   *DriverDiagnostics2    OPTIONAL
  );

/**
  Determine what is the current language setting. The space reserved for Lang
  must be at least RFC_3066_ENTRY_SIZE bytes;

  If Lang is NULL, then ASSERT.

  @param  Lang                   Pointer of system language. Lang will always be filled with 
                                         a valid RFC 3066 language string. If "PlatformLang" is not
                                         set in the system, the default language specifed by PcdUefiVariableDefaultPlatformLang
                                         is returned.

  @return  EFI_SUCCESS     If the EFI Variable with "PlatformLang" is set and return in Lang.
  @return  EFI_NOT_FOUND If the EFI Variable with "PlatformLang" is not set, but a valid default language is return in Lang.

**/
EFI_STATUS
EFIAPI
GetCurrentLanguage (
  OUT     CHAR8               *Lang
  );


#endif
