/** @file
  Library to abstract runtime services

  Copyright (c) 2006, Intel Corporation
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
  Check to see if the execute context is in Runtime phase or not.

  @param  None.

  @retval  TRUE  The driver is in SMM.
  @retval  FALSE The driver is not in SMM.

**/
BOOLEAN
EFIAPI
EfiAtRuntime (
  VOID
  );

/**
  Check to see if the SetVirtualAddressMsp() is invoked or not.

  @retval  TRUE  SetVirtualAddressMsp() has been called.
  @retval  FALSE SetVirtualAddressMsp() has not been called.

**/
BOOLEAN
EFIAPI
EfiGoneVirtual (
  VOID
  );

/**
  Return current time and date information, and time-keeping
  capabilities of hardware platform.

  @param  Time         A pointer to storage to receive a snapshot of the current time.
  @param  Capabilities An optional pointer to a buffer to receive the real time clock device's
                       capabilities.

  @retval  EFI_SUCCESS  Success to execute the function.
  @retval  !EFI_SUCCESS Failed to e3xecute the function.

**/
EFI_STATUS
EFIAPI
EfiGetTime (
  OUT EFI_TIME                    *Time,
  OUT EFI_TIME_CAPABILITIES       *Capabilities
  );

/**
  Set current time and date information.

  @param  Time         A pointer to cache of time setting.

  @retval  EFI_SUCCESS  Success to execute the function.
  @retval  !EFI_SUCCESS Failed to execute the function.

**/
EFI_STATUS
EFIAPI
EfiSetTime (
  IN EFI_TIME                   *Time
  );

/**
  Return current wakeup alarm clock setting.

  @param  Enabled Indicate if the alarm clock is enabled or disabled.
  @param  Pending Indicate if the alarm signal is pending and requires acknowledgement.
  @param  Time    Current alarm clock setting.

  @retval  EFI_SUCCESS  Success to execute the function.
  @retval  !EFI_SUCCESS Failed to e3xecute the function.

**/
EFI_STATUS
EFIAPI
EfiGetWakeupTime (
  OUT BOOLEAN                     *Enabled,
  OUT BOOLEAN                     *Pending,
  OUT EFI_TIME                    *Time
  );

/**
  Set current wakeup alarm clock.

  @param  Enable Enable or disable current alarm clock..
  @param  Time   Point to alarm clock setting.

  @retval  EFI_SUCCESS  Success to execute the function.
  @retval  !EFI_SUCCESS Failed to e3xecute the function.

**/
EFI_STATUS
EFIAPI
EfiSetWakeupTime (
  IN BOOLEAN                      Enable,
  IN EFI_TIME                     *Time
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

  @retval  EFI_SUCCESS  Success to execute the function.
  @retval  !EFI_SUCCESS Failed to e3xecute the function.

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

  @retval  EFI_SUCCESS  Success to execute the function.
  @retval  !EFI_SUCCESS Failed to e3xecute the function.

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

  @retval  EFI_SUCCESS  Success to execute the function.
  @retval  !EFI_SUCCESS Failed to e3xecute the function.

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

  @retval  EFI_SUCCESS  Success to execute the function.
  @retval  !EFI_SUCCESS Failed to e3xecute the function.

**/
EFI_STATUS
EFIAPI
EfiGetNextHighMonotonicCount (
  OUT UINT32                      *HighCount
  );

/**
  Resets the entire platform.

  @param  ResetType   The type of reset to perform.
  @param  ResetStatus The status code for reset.
  @param  DataSize    The size in bytes of reset data.
  @param  ResetData   Pointer to data buffer that includes
                      Null-Terminated Unicode string.

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
  Determines the new virtual address that is to be used on subsequent memory accesses.

  @param  DebugDisposition   Supplies type information for the pointer being converted.
  @param  Address            The pointer to a pointer that is to be fixed to be the
                             value needed for the new virtual address mapping being
                             applied.

  @retval  EFI_SUCCESS  Success to execute the function.
  @retval  !EFI_SUCCESS Failed to e3xecute the function.

**/
EFI_STATUS
EFIAPI
EfiConvertPointer (
  IN UINTN                  DebugDisposition,
  IN OUT VOID               **Address
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

  @retval EFI_SUCCESS           Valid capsule was passed. I Valid capsule was passed. If
                                CAPSULE_FLAGS_PERSIT_ACROSS_RESET is not set, the
                                capsule has been successfully processed by the firmware.
  @retval EFI_INVALID_PARAMETER CapsuleSize is NULL or ResetTye is NULL.
  @retval EFI_DEVICE_ERROR      The capsule update was started, but failed due to a device error.

**/
EFI_STATUS
EFIAPI
EfiUpdateCapsule (
  IN EFI_CAPSULE_HEADER       **CapsuleHeaderArray,
  IN UINTN                    CapsuleCount,
  IN EFI_PHYSICAL_ADDRESS     ScatterGatherList
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

  @retval EFI_SUCCESS           Valid answer returned..
  @retval EFI_INVALID_PARAMETER MaximumCapsuleSize is NULL.
  @retval EFI_UNSUPPORTED       The capsule type is not supported on this platform, and
                                MaximumCapsuleSize and ResetType are undefined.

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

