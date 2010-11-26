/*++

Copyright (c) 2004 - 2010, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.           


Module Name:

  RuntimeService.c

Abstract:

  Runtime Lib

--*/

#include "../RuntimeLibInternal.h"

VOID
EFIAPI
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
{
  EFI_GUID Guid;

  *((UINT64 *) &Guid) = EFI_EXTENDED_SAL_RESET_SERVICES_PROTOCOL_GUID_LO;
  *(((UINT64 *)&Guid) + 1) = EFI_EXTENDED_SAL_RESET_SERVICES_PROTOCOL_GUID_HI;

  EfiCallEsalService (
    &Guid,
    ResetSystem,
    (UINT64) ResetType,
    (UINT64) ResetStatus,
    (UINT64) DataSize,
    (UINT64) ResetData,
    0,
    0,
    0
    );
}


//
// The following functions hide the mRTEdkDxeRuntimeDriverLib local global from the call to
// runtime service in the EFI system table.
//
EFI_STATUS
EFIAPI
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
{
  SAL_RETURN_REGS ReturnReg;
  EFI_GUID        Guid;

  *((UINT64 *) &Guid) = EFI_EXTENDED_SAL_RTC_SERVICES_PROTOCOL_GUID_LO;
  *(((UINT64 *)&Guid) + 1) = EFI_EXTENDED_SAL_RTC_SERVICES_PROTOCOL_GUID_HI;

  ReturnReg = EfiCallEsalService (&Guid, GetTime, (UINT64) Time, (UINT64) Capabilities, 0, 0, 0, 0, 0);
  return ReturnReg.Status;
}

EFI_STATUS
EFIAPI
EfiSetTime (
  IN EFI_TIME                   *Time
  )
/*++

Routine Description:

  Sets the current local time and date information.

Arguments:

  Time  - A pointer to the current time.

Returns:

  Status code

--*/
{
  SAL_RETURN_REGS ReturnReg;
  EFI_GUID        Guid;

  *((UINT64 *) &Guid) = EFI_EXTENDED_SAL_RTC_SERVICES_PROTOCOL_GUID_LO;
  *(((UINT64 *)&Guid) + 1) = EFI_EXTENDED_SAL_RTC_SERVICES_PROTOCOL_GUID_HI;

  ReturnReg = EfiCallEsalService (&Guid, SetTime, (UINT64) Time, 0, 0, 0, 0, 0, 0);
  return ReturnReg.Status;
}

EFI_STATUS
EFIAPI
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
{
  SAL_RETURN_REGS ReturnReg;
  EFI_GUID        Guid;

  *((UINT64 *) &Guid) = EFI_EXTENDED_SAL_RTC_SERVICES_PROTOCOL_GUID_LO;
  *(((UINT64 *)&Guid) + 1) = EFI_EXTENDED_SAL_RTC_SERVICES_PROTOCOL_GUID_HI;

  ReturnReg = EfiCallEsalService (&Guid, GetWakeupTime, (UINT64) Enabled, (UINT64) Pending, (UINT64) Time, 0, 0, 0, 0);
  return ReturnReg.Status;
}

EFI_STATUS
EFIAPI
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
{
  SAL_RETURN_REGS ReturnReg;
  EFI_GUID        Guid;

  *((UINT64 *) &Guid) = EFI_EXTENDED_SAL_RTC_SERVICES_PROTOCOL_GUID_LO;
  *(((UINT64 *)&Guid) + 1) = EFI_EXTENDED_SAL_RTC_SERVICES_PROTOCOL_GUID_HI;

  ReturnReg = EfiCallEsalService (&Guid, SetWakeupTime, (UINT64) Enable, (UINT64) Time, 0, 0, 0, 0, 0);
  return ReturnReg.Status;
}

EFI_STATUS
EFIAPI
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
{
  SAL_RETURN_REGS ReturnReg;
  EFI_GUID        Guid;

  *((UINT64 *) &Guid) = EFI_EXTENDED_SAL_VARIABLE_SERVICES_PROTOCOL_GUID_LO;
  *(((UINT64 *)&Guid) + 1) = EFI_EXTENDED_SAL_VARIABLE_SERVICES_PROTOCOL_GUID_HI;

  ReturnReg = EfiCallEsalService (
                &Guid,
                EsalGetVariable,
                (UINT64) VariableName,
                (UINT64) VendorGuid,
                (UINT64) Attributes,
                (UINT64) DataSize,
                (UINT64) Data,
                0,
                0
                );
  return (EFI_STATUS) ReturnReg.Status;
}

EFI_STATUS
EFIAPI
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
{
  SAL_RETURN_REGS ReturnReg;
  EFI_GUID        Guid;

  *((UINT64 *) &Guid) = EFI_EXTENDED_SAL_VARIABLE_SERVICES_PROTOCOL_GUID_LO;
  *(((UINT64 *)&Guid) + 1) = EFI_EXTENDED_SAL_VARIABLE_SERVICES_PROTOCOL_GUID_HI;

  ReturnReg = EfiCallEsalService (
                &Guid,
                EsalGetNextVariableName,
                (UINT64) VariableNameSize,
                (UINT64) VariableName,
                (UINT64) VendorGuid,
                0,
                0,
                0,
                0
                );
  return (EFI_STATUS) ReturnReg.Status;
}

EFI_STATUS
EFIAPI
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
{
  SAL_RETURN_REGS ReturnReg;
  EFI_GUID        Guid;

  *((UINT64 *) &Guid) = EFI_EXTENDED_SAL_VARIABLE_SERVICES_PROTOCOL_GUID_LO;
  *(((UINT64 *)&Guid) + 1) = EFI_EXTENDED_SAL_VARIABLE_SERVICES_PROTOCOL_GUID_HI;

  ReturnReg = EfiCallEsalService (
                &Guid,
                EsalSetVariable,
                (UINT64) VariableName,
                (UINT64) VendorGuid,
                (UINT64) Attributes,
                (UINT64) DataSize,
                (UINT64) Data,
                0,
                0
                );
  return (EFI_STATUS) ReturnReg.Status;
}

EFI_STATUS
EFIAPI
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
{
  SAL_RETURN_REGS ReturnReg;
  EFI_GUID        Guid;

  *((UINT64 *) &Guid) = EFI_EXTENDED_SAL_MTC_SERVICES_PROTOCOL_GUID_LO;
  *(((UINT64 *)&Guid) + 1) = EFI_EXTENDED_SAL_MTC_SERVICES_PROTOCOL_GUID_HI;

  ReturnReg = EfiCallEsalService (&Guid, GetNextHighMonotonicCount, (UINT64) HighCount, 0, 0, 0, 0, 0, 0);
  return (EFI_STATUS) ReturnReg.Status;
}

EFI_STATUS
EFIAPI
EfiConvertPointer (
  IN UINTN                  DebugDisposition,
  IN OUT VOID               **Address
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
{
  return mRTEdkDxeRuntimeDriverLib->ConvertPointer (DebugDisposition, Address);
}

EFI_STATUS
EFIAPI
EfiConvertList (
  IN UINTN                DebugDisposition,
  IN OUT LIST_ENTRY       *ListHead
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
{
  LIST_ENTRY  *Link;
  LIST_ENTRY  *NextLink;

  //
  // Convert all the ForwardLink & BackLink pointers in the list
  //
  Link = ListHead;
  do {
    NextLink = Link->ForwardLink;

    EfiConvertPointer (
      Link->ForwardLink == ListHead ? DebugDisposition : 0,
      (VOID **) &Link->ForwardLink
      );

    EfiConvertPointer (
      Link->BackLink == ListHead ? DebugDisposition : 0,
      (VOID **) &Link->BackLink
      );

    Link = NextLink;
  } while (Link != ListHead);
  return EFI_SUCCESS;
}


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
  )
{
  SAL_RETURN_REGS ReturnReg;
  EFI_GUID        Guid;

  *((UINT64 *) &Guid) = EFI_EXTENDED_SAL_VIRTUAL_SERVICES_PROTOCOL_GUID_LO;
  *(((UINT64 *)&Guid) + 1) = EFI_EXTENDED_SAL_VIRTUAL_SERVICES_PROTOCOL_GUID_HI;

  ReturnReg = EfiCallEsalService (
                &Guid,
                SetVirtualAddress,
                (UINT64) MemoryMapSize,
                (UINT64) DescriptorSize,
                (UINT64) DescriptorVersion,
                (UINT64) VirtualMap,
                0,
                0,
                0
                );

  return ReturnReg.Status;
}


EFI_STATUS
EFIAPI
EfiUpdateCapsule (
  IN UEFI_CAPSULE_HEADER  **CapsuleHeaderArray,
  IN UINTN                CapsuleCount,
  IN EFI_PHYSICAL_ADDRESS ScatterGatherList OPTIONAL
  )
{
  return EFI_UNSUPPORTED;
}

EFI_STATUS
EFIAPI
EfiQueryCapsuleCapabilities (
  IN UEFI_CAPSULE_HEADER  **CapsuleHeaderArray,
  IN UINTN                CapsuleCount,
  OUT UINT64              *MaximumCapsuleSize,
  OUT EFI_RESET_TYPE    *ResetType
  )
{
  return EFI_UNSUPPORTED;
}


EFI_STATUS
EFIAPI
EfiQueryVariableInfo (
  IN UINT32       Attributes,
  OUT UINT64      *MaximumVariableStorageSize,
  OUT  UINT64     *RemainingVariableStorageSize,
  OUT UINT64      *MaximumVariableSize
  )
{
  return EFI_UNSUPPORTED;
}
