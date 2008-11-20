/** @file
  Provides library functions for each of the UEFI Runtime Services.
  Only available to DXE and UEFI module types.

Copyright (c) 2006 - 2008, Intel Corporation<BR>
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#ifndef __UEFI_RUNTIME_LIB__
#define __UEFI_RUNTIME_LIB__


extern const EFI_EVENT_NOTIFY _gDriverExitBootServicesEvent[];

extern const EFI_EVENT_NOTIFY _gDriverSetVirtualAddressMapEvent[];

/**
  This function allows the caller to determine if UEFI ExitBootServices() has been called.

  This function returns TRUE after all the EVT_SIGNAL_EXIT_BOOT_SERVICES functions have
  executed as a result of the OS calling ExitBootServices().  Prior to this time FALSE
  is returned. This function is used by runtime code to decide it is legal to access
  services that go away after ExitBootServices().

  @retval  TRUE  The system has finished executing the EVT_SIGNAL_EXIT_BOOT_SERVICES event.
  @retval  FALSE The system has not finished executing the EVT_SIGNAL_EXIT_BOOT_SERVICES event.

**/
BOOLEAN
EFIAPI
EfiAtRuntime (
  VOID
  );

/**
  This function allows the caller to determine if UEFI SetVirtualAddressMap() has been called. 

  This function returns TRUE after all the EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE functions have
  executed as a result of the OS calling SetVirtualAddressMap(). Prior to this time FALSE
  is returned. This function is used by runtime code to decide it is legal to access services
  that go away after SetVirtualAddressMap().

  @retval  TRUE  The system has finished executing the EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE event.
  @retval  FALSE The system has not finished executing the EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE event.

**/
BOOLEAN
EFIAPI
EfiGoneVirtual (
  VOID
  );

/**
  This service is a wrapper for the UEFI Runtime Service GetTime().

  The GetTime() function returns a time that was valid sometime during the call to the function.
  While the returned EFI_TIME structure contains TimeZone and Daylight savings time information,
  the actual clock does not maintain these values. The current time zone and daylight saving time
  information returned by GetTime() are the values that were last set via SetTime().
  The GetTime() function should take approximately the same amount of time to read the time each
  time it is called. All reported device capabilities are to be rounded up.
  During runtime, if a PC-AT CMOS device is present in the platform the caller must synchronize
  access to the device before calling GetTime().

  @param  Time         A pointer to storage to receive a snapshot of the current time.
  @param  Capabilities An optional pointer to a buffer to receive the real time clock device's
                       capabilities.

  @retval  EFI_SUCCESS            The operation completed successfully.
  @retval  EFI_INVALID_PARAMETER  Time is NULL.
  @retval  EFI_DEVICE_ERROR       The time could not be retrieved due to a hardware error.

**/
EFI_STATUS
EFIAPI
EfiGetTime (
  OUT EFI_TIME                    *Time,
  OUT EFI_TIME_CAPABILITIES       *Capabilities  OPTIONAL
  );

/**
  This service is a wrapper for the UEFI Runtime Service SetTime().

  The SetTime() function sets the real time clock device to the supplied time, and records the
  current time zone and daylight savings time information. The SetTime() function is not allowed
  to loop based on the current time. For example, if the device does not support a hardware reset
  for the sub-resolution time, the code is not to implement the feature by waiting for the time to
  wrap.
  During runtime, if a PC-AT CMOS device is present in the platform the caller must synchronize
  access to the device before calling SetTime().

  @param  Time  A pointer to the current time. Type EFI_TIME is defined in the GetTime()
                function description. Full error checking is performed on the different
                fields of the EFI_TIME structure (refer to the EFI_TIME definition in the
                GetTime() function description for full details), and EFI_INVALID_PARAMETER
                is returned if any field is out of range.

  @retval  EFI_SUCCESS            The operation completed successfully.
  @retval  EFI_INVALID_PARAMETER  A time field is out of range.
  @retval  EFI_DEVICE_ERROR       The time could not be set due to a hardware error.

**/
EFI_STATUS
EFIAPI
EfiSetTime (
  IN EFI_TIME                   *Time
  );

/**
  Returns the current wakeup alarm clock setting.

  The alarm clock time may be rounded from the set alarm clock time to be within the resolution
  of the alarm clock device. The resolution of the alarm clock device is defined to be one second.
  During runtime, if a PC-AT CMOS device is present in the platform the caller must synchronize
  access to the device before calling GetWakeupTime().

  @param  Enabled  Indicates if the alarm is currently enabled or disabled.
  @param  Pending  Indicates if the alarm signal is pending and requires acknowledgement.
  @param  Time     The current alarm setting. Type EFI_TIME is defined in the GetTime()
                   function description.

  @retval  EFI_SUCCESS           The alarm settings were returned.
  @retval  EFI_INVALID_PARAMETER  Enabled is NULL.
  @retval  EFI_INVALID_PARAMETER  Pending is NULL.
  @retval  EFI_INVALID_PARAMETER  Time is NULL.
  @retval  EFI_DEVICE_ERROR       The wakeup time could not be retrieved due to a hardware error.
  @retval  EFI_UNSUPPORTED        A wakeup timer is not supported on this platform.

**/
EFI_STATUS
EFIAPI
EfiGetWakeupTime (
  OUT BOOLEAN                     *Enabled,
  OUT BOOLEAN                     *Pending,
  OUT EFI_TIME                    *Time
  );

/**
  Sets the system wakeup alarm clock time.

  @param  Enable  Enable or disable the wakeup alarm.
  @param  Time    If Enable is TRUE, the time to set the wakeup alarm for. Type EFI_TIME
                  is defined in the GetTime() function description. If Enable is FALSE,
                  then this parameter is optional, and may be NULL.

  @retval  EFI_SUCCESS            If Enable is TRUE, then the wakeup alarm was enabled.
                                  If Enable is FALSE, then the wakeup alarm was disabled.
  @retval  EFI_INVALID_PARAMETER  A time field is out of range.
  @retval  EFI_DEVICE_ERROR       The wakeup time could not be set due to a hardware error.
  @retval  EFI_UNSUPPORTED        A wakeup timer is not supported on this platform.

**/
EFI_STATUS
EFIAPI
EfiSetWakeupTime (
  IN BOOLEAN                      Enable,
  IN EFI_TIME                     *Time   OPTIONAL
  );

/**
  Return value of variable.

  @param  VariableName the name of the vendor's variable, it's a
                       Null-Terminated Unicode String
  @param  VendorGuid   Unify identifier for vendor.
  @param  Attributes   Point to memory location to return the attributes of variable. If the point
                       is NULL, the parameter would be ignored.
  @param  DataSize     As input, point to the maxinum size of return Data-Buffer.
                       As output, point to the actual size of the returned Data-Buffer.
  @param  Data         Point to return Data-Buffer.

  @retval  EFI_SUCCESS            The function completed successfully.
  @retval  EFI_NOT_FOUND          The variable was not found.
  @retval  EFI_BUFFER_TOO_SMALL   The DataSize is too small for the result. DataSize has
                                  been updated with the size needed to complete the request.
  @retval  EFI_INVALID_PARAMETER  VariableName is NULL.
  @retval  EFI_INVALID_PARAMETER  VendorGuid is NULL.
  @retval  EFI_INVALID_PARAMETER  DataSize is NULL.
  @retval  EFI_INVALID_PARAMETER  The DataSize is not too small and Data is NULL.
  @retval  EFI_DEVICE_ERROR       The variable could not be retrieved due to a hardware error.
  @retval  EFI_SECURITY_VIOLATION The variable could not be retrieved due to an authentication failure.
**/
EFI_STATUS
EFIAPI
EfiGetVariable (
  IN      CHAR16                   *VariableName,
  IN      EFI_GUID                 *VendorGuid,
  OUT     UINT32                   *Attributes OPTIONAL,
  IN OUT  UINTN                    *DataSize,
  OUT     VOID                     *Data
  );

/**
  Enumerates variable's name.

  @param  VariableNameSize As input, point to maxinum size of variable name.
                           As output, point to actual size of varaible name.
  @param  VariableName     As input, supplies the last VariableName that was returned by
                           GetNextVariableName().
                           As output, returns the name of variable. The name
                           string is Null-Terminated Unicode string.
  @param  VendorGuid       As input, supplies the last VendorGuid that was returned by
                           GetNextVriableName().
                           As output, returns the VendorGuid of the current variable.

  @retval  EFI_SUCCESS           The function completed successfully.
  @retval  EFI_NOT_FOUND         The next variable was not found.
  @retval  EFI_BUFFER_TOO_SMALL  The VariableNameSize is too small for the result.
                                 VariableNameSize has been updated with the size needed
                                 to complete the request.
  @retval  EFI_INVALID_PARAMETER VariableNameSize is NULL.
  @retval  EFI_INVALID_PARAMETER VariableName is NULL.
  @retval  EFI_INVALID_PARAMETER VendorGuid is NULL.
  @retval  EFI_DEVICE_ERROR      The variable name could not be retrieved due to a hardware error.

**/
EFI_STATUS
EFIAPI
EfiGetNextVariableName (
  IN OUT UINTN                    *VariableNameSize,
  IN OUT CHAR16                   *VariableName,
  IN OUT EFI_GUID                 *VendorGuid
  );

/**
  Sets value of variable.

  @param  VariableName the name of the vendor's variable, it's a
                       Null-Terminated Unicode String
  @param  VendorGuid   Unify identifier for vendor.
  @param  Attributes   Point to memory location to return the attributes of variable. If the point
                       is NULL, the parameter would be ignored.
  @param  DataSize     The size in bytes of Data-Buffer.
  @param  Data         Point to the content of the variable.

  @retval  EFI_SUCCESS            The firmware has successfully stored the variable and its data as
                                  defined by the Attributes.
  @retval  EFI_INVALID_PARAMETER  An invalid combination of attribute bits was supplied, or the
                                  DataSize exceeds the maximum allowed.
  @retval  EFI_INVALID_PARAMETER  VariableName is an empty Unicode string.
  @retval  EFI_OUT_OF_RESOURCES   Not enough storage is available to hold the variable and its data.
  @retval  EFI_DEVICE_ERROR       The variable could not be saved due to a hardware failure.
  @retval  EFI_WRITE_PROTECTED    The variable in question is read-only.
  @retval  EFI_WRITE_PROTECTED    The variable in question cannot be deleted.
  @retval  EFI_SECURITY_VIOLATION The variable could not be written due to EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS
                                  set but the AuthInfo does NOT pass the validation check carried
                                  out by the firmware.
  @retval  EFI_NOT_FOUND          The variable trying to be updated or deleted was not found.

**/
EFI_STATUS
EFIAPI
EfiSetVariable (
  IN CHAR16                       *VariableName,
  IN EFI_GUID                     *VendorGuid,
  IN UINT32                       Attributes,
  IN UINTN                        DataSize,
  IN VOID                         *Data
  );

/**
  Returns the next high 32 bits of platform's monotonic counter.

  @param  HighCount Pointer to returned value.

  @retval  EFI_SUCCESS           The next high monotonic count was returned.
  @retval  EFI_DEVICE_ERROR      The device is not functioning properly.
  @retval  EFI_INVALID_PARAMETER HighCount is NULL.

**/
EFI_STATUS
EFIAPI
EfiGetNextHighMonotonicCount (
  OUT UINT32                      *HighCount
  );

/**
  Resets the entire platform.

  @param  ResetType   The type of reset to perform.
  @param  ResetStatus The status code for the reset. If the system reset is part of a
                      normal operation, the status code would be EFI_SUCCESS. If the system
                      reset is due to some type of failure the most appropriate EFI Status
                      code would be used.
  @param  DataSizeThe size, in bytes, of ResetData.
  @param  ResetData   For a ResetType of EfiResetCold, EfiResetWarm, or EfiResetShutdown
                      the data buffer starts with a Null-terminated Unicode string, optionally
                      followed by additional binary data. The string is a description that the
                      caller may use to further indicate the reason for the system reset. ResetData
                      is only valid if ResetStatus is something other then EFI_SUCCESS. This
                      pointer must be a physical address. For a ResetType of EfiRestUpdate the
                      data buffer also starts with a Null-terminated string that is followed by
                      a physical VOID * to an EFI_CAPSULE_HEADER.

**/
VOID
EFIAPI
EfiResetSystem (
  IN EFI_RESET_TYPE               ResetType,
  IN EFI_STATUS                   ResetStatus,
  IN UINTN                        DataSize,
  IN CHAR16                       *ResetData
  );

/**
  This service converts a function pointer from physical to virtual addressing. 

  @param  DebugDisposition   Supplies type information for the pointer being converted.
  @param  Address            The pointer to a pointer that is to be fixed to be the
                             value needed for the new virtual address mapping being
                             applied.

  @retval  EFI_SUCCESS            The pointer pointed to by Address was modified.
  @retval  EFI_NOT_FOUND          The pointer pointed to by Address was not found to be part of
                                  the current memory map. This is normally fatal.
  @retval  EFI_INVALID_PARAMETER  Address is NULL.
  @retval  EFI_INVALID_PARAMETER  *Address is NULL and DebugDispositio

**/
EFI_STATUS
EFIAPI
EfiConvertPointer (
  IN UINTN                  DebugDisposition,
  IN OUT VOID               **Address
  );

/**
  Determines the new virtual address that is to be used on subsequent memory accesses.

  For IA32, X64, and EBC, this service is a wrapper for the UEFI Runtime Service
  ConvertPointer().  See the UEFI Specification for details. 
  For IPF, this function interprets Address as a pointer to an EFI_PLABEL structure
  and both the EntryPoint and GP fields of an EFI_PLABEL are converted from physical
  to virtiual addressing.  Since IPF allows the GP to point to an address outside
  a PE/COFF image, the physical to virtual offset for the EntryPoint field is used
  to adjust the GP field.  The UEFI Runtime Service ConvertPointer() is used to convert
  EntryPoint and the status code for this conversion is always returned.   If the convertion
  of EntryPoint fails, then neither EntryPoint nor GP are modified.  See the UEFI
  Specification for details on the UEFI Runtime Service ConvertPointer().

  @param  DebugDisposition   Supplies type information for the pointer being converted.
  @param  Address            The pointer to a pointer that is to be fixed to be the
                             value needed for the new virtual address mapping being
                             applied.

  @return  EFI_STATUS value from EfiConvertPointer().

**/
EFI_STATUS
EFIAPI
EfiConvertFunctionPointer (
  IN UINTN                DebugDisposition,
  IN OUT VOID             **Address
  );

/**
  Change the runtime addressing mode of EFI firmware from physical to virtual.

  @param  MemoryMapSize         The size in bytes of VirtualMap.
  @param  DescriptorSize        The size in bytes of an entry in the VirtualMap.
  @param  DescriptorVersion     The version of the structure entries in VirtualMap.
  @param  VirtualMap            An array of memory descriptors which contain new virtual
                                address mapping information for all runtime ranges. Type
                                EFI_MEMORY_DESCRIPTOR is defined in the
                                GetMemoryMap() function description.

  @retval EFI_SUCCESS           The virtual address map has been applied.
  @retval EFI_UNSUPPORTED       EFI firmware is not at runtime, or the EFI firmware is already in
                                virtual address mapped mode.
  @retval EFI_INVALID_PARAMETER DescriptorSize or DescriptorVersion is
                                invalid.
  @retval EFI_NO_MAPPING        A virtual address was not supplied for a range in the memory
                                map that requires a mapping.
  @retval EFI_NOT_FOUND         A virtual address was supplied for an address that is not found
                                in the memory map.
**/
EFI_STATUS
EFIAPI
EfiSetVirtualAddressMap (
  IN UINTN                          MemoryMapSize,
  IN UINTN                          DescriptorSize,
  IN UINT32                         DescriptorVersion,
  IN CONST EFI_MEMORY_DESCRIPTOR    *VirtualMap
  );


/**
  Convert the standard Lib double linked list to a virtual mapping.

  This service uses EfiConvertPointer() to walk a double linked list and convert all the link
  pointers to their virtual mappings. This function is only guaranteed to work during the
  EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE event and calling it at other times has undefined results.

  @param  DebugDisposition   Supplies type information for the pointer being converted.
  @param  ListHead           Head of linked list to convert.

  @retval  EFI_SUCCESS  Success to execute the function.
  @retval  !EFI_SUCCESS Failed to e3xecute the function.

**/
EFI_STATUS
EFIAPI
EfiConvertList (
  IN UINTN                DebugDisposition,
  IN OUT LIST_ENTRY       *ListHead
  );

/**
  Passes capsules to the firmware with both virtual and physical mapping.
  Depending on the intended consumption, the firmware may
  process the capsule immediately. If the payload should persist across a
  system reset, the reset value returned from EFI_QueryCapsuleCapabilities must
  be passed into ResetSystem() and will cause the capsule to be processed by
  the firmware as part of the reset process.

  @param  CapsuleHeaderArray    Virtual pointer to an array of virtual pointers to the capsules
                                being passed into update capsule. Each capsules is assumed to
                                stored in contiguous virtual memory. The capsules in the
                                CapsuleHeaderArray must be the same capsules as the
                                ScatterGatherList. The CapsuleHeaderArray must
                                have the capsules in the same order as the ScatterGatherList.
  @param  CapsuleCount          Number of pointers to EFI_CAPSULE_HEADER in
                                CaspuleHeaderArray.
  @param  ScatterGatherList     Physical pointer to a set of
                                EFI_CAPSULE_BLOCK_DESCRIPTOR that describes the
                                location in physical memory of a set of capsules. See Related
                                Definitions for an explanation of how more than one capsule is
                                passed via this interface. The capsules in the
                                ScatterGatherList must be in the same order as the
                                CapsuleHeaderArray. This parameter is only referenced if
                                the capsules are defined to persist across system reset.

  @retval EFI_SUCCESS           Valid capsule was passed. If CAPSULE_FLAGS_PERSIT_ACROSS_RESET is not set,
                                the capsule has been successfully processed by the firmware.
  @retval EFI_INVALID_PARAMETER CapsuleSize or HeaderSize is NULL.
  @retval EFI_INVALID_PARAMETER CapsuleCount is 0
  @retval EFI_DEVICE_ERROR      The capsule update was started, but failed due to a device error.
  @retval EFI_UNSUPPORTED       The capsule type is not supported on this platform.
  @retval EFI_OUT_OF_RESOURCES  There were insufficient resources to process the capsule.

**/
EFI_STATUS
EFIAPI
EfiUpdateCapsule (
  IN EFI_CAPSULE_HEADER       **CapsuleHeaderArray,
  IN UINTN                    CapsuleCount,
  IN EFI_PHYSICAL_ADDRESS     ScatterGatherList OPTIONAL
  );


/**
  The QueryCapsuleCapabilities() function allows a caller to test to see if a capsule or
  capsules can be updated via UpdateCapsule(). The Flags values in the capsule header and
  size of the entire capsule is checked.
  If the caller needs to query for generic capsule capability a fake EFI_CAPSULE_HEADER can be
  constructed where CapsuleImageSize is equal to HeaderSize that is equal to sizeof
  (EFI_CAPSULE_HEADER). To determine reset requirements,
  CAPSULE_FLAGS_PERSIST_ACROSS_RESET should be set in the Flags field of the
  EFI_CAPSULE_HEADER.
  The firmware must support any capsule that has the
  CAPSULE_FLAGS_PERSIST_ACROSS_RESET flag set in EFI_CAPSULE_HEADER. The
  firmware sets the policy for what capsules are supported that do not have the
  CAPSULE_FLAGS_PERSIST_ACROSS_RESET flag set.

  @param  CapsuleHeaderArray    Virtual pointer to an array of virtual pointers to the capsules
                                being passed into update capsule. The capsules are assumed to
                                stored in contiguous virtual memory.
  @param  CapsuleCount          Number of pointers to EFI_CAPSULE_HEADER in
                                CaspuleHeaderArray.
  @param  MaximumCapsuleSize     On output the maximum size that UpdateCapsule() can
                                support as an argument to UpdateCapsule() via
                                CapsuleHeaderArray and ScatterGatherList.
                                Undefined on input.
  @param  ResetType             Returns the type of reset required for the capsule update.

  @retval EFI_SUCCESS           Valid answer returned.
  @retval EFI_INVALID_PARAMETER MaximumCapsuleSize is NULL.
  @retval EFI_UNSUPPORTED       The capsule type is not supported on this platform, and
                                MaximumCapsuleSize and ResetType are undefined.
  @retval EFI_OUT_OF_RESOURCES  There were insufficient resources to process the query request.

**/
EFI_STATUS
EFIAPI
EfiQueryCapsuleCapabilities (
  IN  EFI_CAPSULE_HEADER       **CapsuleHeaderArray,
  IN  UINTN                    CapsuleCount,
  OUT UINT64                   *MaximumCapsuleSize,
  OUT EFI_RESET_TYPE           *ResetType
  );


/**
  Returns information about the EFI variables.

  The QueryVariableInfo() function allows a caller to obtain the information about the
  maximum size of the storage space available for the EFI variables, the remaining size of the storage
  space available for the EFI variables and the maximum size of each individual EFI variable,
  associated with the attributes specified.
  The returned MaximumVariableStorageSize, RemainingVariableStorageSize,
  MaximumVariableSize information may change immediately after the call based on other
  runtime activities including asynchronous error events. Also, these values associated with different
  attributes are not additive in nature.

  @param  Attributes            Attributes bitmask to specify the type of variables on
                                which to return information. Refer to the
                                GetVariable() function description.
  @param  MaximumVariableStorageSize
                                On output the maximum size of the storage space
                                available for the EFI variables associated with the
                                attributes specified.
  @param  RemainingVariableStorageSize
                                Returns the remaining size of the storage space
                                available for the EFI variables associated with the
                                attributes specified..
  @param  MaximumVariableSize   Returns the maximum size of the individual EFI
                                variables associated with the attributes specified.

  @retval EFI_SUCCESS           Valid answer returned.
  @retval EFI_INVALID_PARAMETER An invalid combination of attribute bits was supplied.
  @retval EFI_UNSUPPORTED       EFI_UNSUPPORTED The attribute is not supported on this platform, and the
                                MaximumVariableStorageSize,
                                RemainingVariableStorageSize, MaximumVariableSize
                                are undefined.
**/
EFI_STATUS
EFIAPI
EfiQueryVariableInfo (
  IN UINT32   Attributes,
  OUT UINT64  *MaximumVariableStorageSize,
  OUT UINT64  *RemainingVariableStorageSize,
  OUT UINT64  *MaximumVariableSize
  );

#endif

