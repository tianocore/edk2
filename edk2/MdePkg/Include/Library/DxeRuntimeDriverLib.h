/** @file
	Library to abstract runtime services

	Copyright (c) 2006, Intel Corporation                                                         
	All rights reserved. This program and the accompanying materials                          
	are licensed and made available under the terms and conditions of the BSD License         
	which accompanies this distribution.  The full text of the license may be found at        
	http://opensource.org/licenses/bsd-license.php                                            

	THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
	WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

	Module Name:	DxeRuntimeDriverLib.h

**/

#ifndef __DXE_RUNTIME_DRIVER_LIB__
#define __DXE_RUNTIME_DRIVER_LIB__


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
  @param  Capabilities An optional pointer to a buffer to receive the real time clock device¡¯s
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
  OUT     UINT32                   *Attributes,
  IN OUT  UINTN                    *DataSize,
  OUT     VOID                     *Data
  )
;

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

  @retval  EFI_SUCCESS  Success to execute the function.
  @retval  !EFI_SUCCESS Failed to e3xecute the function.

**/
VOID
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
  IN OUT VOID               *Address
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
  Conver the standard Lib double linked list to a virtual mapping.

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


EFI_STATUS
EFIAPI
EfiUpdateCapsule (
  IN UEFI_CAPSULE_HEADER      **CapsuleHeaderArray,
  IN UINTN                    CapsuleCount,
  IN EFI_PHYSICAL_ADDRESS     ScatterGatherList
  );


EFI_STATUS
EFIAPI
EfiQueryCapsuleCapabilities (
  IN UEFI_CAPSULE_HEADER       **CapsuleHeaderArray,
  IN UINTN                     CapsuleCount,
  OUT UINT64                   *MaximumCapsuleSize,
  OUT EFI_RESET_TYPE           *ResetType
  );

EFI_STATUS
EFIAPI
EfiQueryVariableInfo (
  IN UINT32   Attrubutes,
  OUT UINT64  *MaximumVariableStorageSize,
  OUT UINT64  *RemainingVariableStorageSize,
  OUT UINT64  *MaximumVariableSize
  );

#endif
