/** @file MockUefiRuntimeServicesTableLib.c
  Mock Unit Test UEFI Runtime Services Table Library Implementation for
  Unit Testing

  This library provides comprehensive mocking of UEFI Runtime Services
  functions using CMocka framework. Each function can return data based
  on mock() implementation.

  Copyright (c) 2026, American Megatrends International LLC. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <Uefi.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/MockUefiRuntimeServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/AuthVariableLib.h>

#define EARLIEST_YEAR  1900
#define MAXIMUM_YEAR   9999

STATIC CONST UINT8  mDayOfMonth[] = { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

#define EFI_VARIABLE_ATTRIBUTES_MASK  (EFI_VARIABLE_NON_VOLATILE |\
                                      EFI_VARIABLE_BOOTSERVICE_ACCESS | \
                                      EFI_VARIABLE_RUNTIME_ACCESS | \
                                      EFI_VARIABLE_HARDWARE_ERROR_RECORD | \
                                      EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS | \
                                      EFI_VARIABLE_APPEND_WRITE)

//
// Helper Functions
//

/**
  Check if a year is a leap year.

  @param[in]  Time  Pointer to EFI_TIME structure.

  @retval TRUE   Year is a leap year.
  @retval FALSE  Year is not a leap year.
**/
STATIC
BOOLEAN
IsLeapYear (
  IN EFI_TIME  *Time
  )
{
  if (Time->Year % 4 == 0) {
    if (Time->Year % 100 == 0) {
      if (Time->Year % 400 == 0) {
        return TRUE;
      }

      return FALSE;
    }

    return TRUE;
  }

  return FALSE;
}

/**
  Validate time and date ranges per UEFI specification.

  @param[in]  Time  Pointer to EFI_TIME structure to validate.

  @retval TRUE   Time is valid.
  @retval FALSE  Time contains invalid values.
**/
STATIC
BOOLEAN
ValidateTimeFields (
  IN EFI_TIME  *Time
  )
{
  // Check year, month, day
  if ((Time->Year < EARLIEST_YEAR) || (Time->Year > MAXIMUM_YEAR)) {
    return FALSE;
  }

  if ((Time->Month < 1) || (Time->Month > 12)) {
    return FALSE;
  }

  // Check day of month
  if ((Time->Day < 1) ||
      (Time->Day > mDayOfMonth[Time->Month - 1]) ||
      ((Time->Month == 2) && (!IsLeapYear (Time)) && (Time->Day > 28))
      )
  {
    return FALSE;
  }

  // Check hour, minute, second, nanosecond
  if ((Time->Hour > 23) ||
      (Time->Minute > 59) ||
      (Time->Second > 59) ||
      (Time->Nanosecond > 999999999))
  {
    return FALSE;
  }

  // Check timezone: must be -1440 to 1440 or EFI_UNSPECIFIED_TIMEZONE
  if (!((Time->TimeZone == EFI_UNSPECIFIED_TIMEZONE) || ((Time->TimeZone >= -1440) && (Time->TimeZone <= 1440)))) {
    return FALSE;
  }

  // Check daylight
  if ((Time->Daylight & (~(EFI_TIME_ADJUST_DAYLIGHT | EFI_TIME_IN_DAYLIGHT))) != 0) {
    return FALSE;
  }

  return TRUE;
}

//
// Mock Runtime Services function implementations
//

/**
  Returns the current time and date information, and the time-keeping capabilities
  of the hardware platform.

  @param[out]  Time         A pointer to storage to receive a snapshot of the current time.
  @param[out]  Capabilities An optional pointer to a buffer to receive the real time clock
                            device's capabilities.

  @retval EFI_SUCCESS           The operation completed successfully.
  @retval EFI_INVALID_PARAMETER Time is NULL.
  @retval EFI_DEVICE_ERROR      The time could not be retrieved due to hardware error.
**/
EFI_STATUS
EFIAPI
MockGetTime (
  OUT EFI_TIME               *Time,
  OUT EFI_TIME_CAPABILITIES  *Capabilities OPTIONAL
  )
{
  EFI_STATUS  Status;

  // Time is required parameter
  if (Time == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = (EFI_STATUS)mock ();

  if (!EFI_ERROR (Status)) {
    // Provide mock time data
    Time->Year       = (UINT16)mock ();
    Time->Month      = (UINT8)mock ();
    Time->Day        = (UINT8)mock ();
    Time->Hour       = (UINT8)mock ();
    Time->Minute     = (UINT8)mock ();
    Time->Second     = (UINT8)mock ();
    Time->Nanosecond = (UINT32)mock ();
    Time->TimeZone   = (INT16)mock ();
    Time->Daylight   = (UINT8)mock ();
  }

  if (!EFI_ERROR (Status) && (Capabilities != NULL)) {
    Capabilities->Resolution = (UINT32)mock ();
    Capabilities->Accuracy   = (UINT32)mock ();
    Capabilities->SetsToZero = (BOOLEAN)mock ();
  }

  return Status;
}

/**
  Sets the current local time and date information.

  @param[in]  Time  A pointer to the current time.

  @retval EFI_SUCCESS           The operation completed successfully.
  @retval EFI_INVALID_PARAMETER A time field is out of range.
  @retval EFI_DEVICE_ERROR      The time could not be set due to hardware error.
**/
EFI_STATUS
EFIAPI
MockSetTime (
  IN EFI_TIME  *Time
  )
{
  if (Time == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  // Validate time parameter
  if (!ValidateTimeFields (Time)) {
    return EFI_INVALID_PARAMETER;
  }

  return (EFI_STATUS)mock ();
}

/**
  Returns the current wakeup alarm clock setting.

  @param[out]  Enabled  Indicates if the alarm is currently enabled or disabled.
  @param[out]  Pending  Indicates if the alarm signal is pending and requires acknowledgement.
  @param[out]  Time     If the alarm is enabled, returns the current alarm setting.

  @retval EFI_SUCCESS           The alarm settings were returned.
  @retval EFI_INVALID_PARAMETER Enabled is NULL.
  @retval EFI_INVALID_PARAMETER Pending is NULL.
  @retval EFI_INVALID_PARAMETER Time is NULL.
  @retval EFI_DEVICE_ERROR      The wakeup time could not be retrieved due to a hardware error.
  @retval EFI_UNSUPPORTED       A wakeup timer is not supported on this platform.
**/
EFI_STATUS
EFIAPI
MockGetWakeupTime (
  OUT BOOLEAN   *Enabled,
  OUT BOOLEAN   *Pending,
  OUT EFI_TIME  *Time
  )
{
  EFI_STATUS  Status;

  // All output parameters are required
  if ((Enabled == NULL) || (Pending == NULL) || (Time == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = (EFI_STATUS)mock ();

  if (!EFI_ERROR (Status)) {
    *Enabled         = (BOOLEAN)mock ();
    *Pending         = (BOOLEAN)mock ();
    Time->Year       = (UINT16)mock ();
    Time->Month      = (UINT8)mock ();
    Time->Day        = (UINT8)mock ();
    Time->Hour       = (UINT8)mock ();
    Time->Minute     = (UINT8)mock ();
    Time->Second     = (UINT8)mock ();
    Time->Nanosecond = (UINT32)mock ();
    Time->TimeZone   = (INT16)mock ();
    Time->Daylight   = (UINT8)mock ();
  }

  return Status;
}

/**
  Sets the system wakeup alarm clock time.

  @param[in]  Enable  Enable or disable the wakeup alarm.
  @param[in]  Time    If Enable is TRUE, the time to set the wakeup alarm for.
                      If Enable is FALSE, then this parameter is optional, and may be NULL.

  @retval EFI_SUCCESS           If Enable is TRUE, then the wakeup alarm was enabled. If
                                Enable is FALSE, then the wakeup alarm was disabled.
  @retval EFI_INVALID_PARAMETER A time field is out of range.
  @retval EFI_DEVICE_ERROR      The wakeup time could not be set due to a hardware error.
  @retval EFI_UNSUPPORTED       A wakeup timer is not supported on this platform.
**/
EFI_STATUS
EFIAPI
MockSetWakeupTime (
  IN BOOLEAN   Enable,
  IN EFI_TIME  *Time OPTIONAL
  )
{
  // If Enable is TRUE, Time must be valid
  if (Enable) {
    if (Time == NULL) {
      return EFI_INVALID_PARAMETER;
    }

    if (!ValidateTimeFields (Time)) {
      return EFI_INVALID_PARAMETER;
    }
  }

  return (EFI_STATUS)mock ();
}

/**
  Changes the runtime addressing mode of EFI firmware from physical to virtual.

  @param[in]  MemoryMapSize      The size in bytes of VirtualMap.
  @param[in]  DescriptorSize     The size in bytes of an entry in the VirtualMap.
  @param[in]  DescriptorVersion  The version of the structure entries in VirtualMap.
  @param[in]  VirtualMap         An array of memory descriptors which contain new virtual
                                 address mapping information for all runtime ranges.

  @retval EFI_SUCCESS            The virtual address map has been applied.
  @retval EFI_UNSUPPORTED        EFI firmware is not at runtime, or the EFI firmware is already in
                                 virtual address mapped mode.
  @retval EFI_INVALID_PARAMETER  DescriptorSize or DescriptorVersion is invalid.
  @retval EFI_NO_MAPPING         A virtual address was not supplied for a range in the memory
                                 map that requires a mapping.
  @retval EFI_NOT_FOUND          A virtual address was supplied for an address that is not found
                                 in the memory map.
**/
EFI_STATUS
EFIAPI
MockSetVirtualAddressMap (
  IN UINTN                  MemoryMapSize,
  IN UINTN                  DescriptorSize,
  IN UINT32                 DescriptorVersion,
  IN EFI_MEMORY_DESCRIPTOR  *VirtualMap
  )
{
  if (VirtualMap == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if ((DescriptorVersion != EFI_MEMORY_DESCRIPTOR_VERSION) || (DescriptorSize < sizeof (EFI_MEMORY_DESCRIPTOR))) {
    return EFI_INVALID_PARAMETER;
  }

  if ((MemoryMapSize % DescriptorSize) != 0) {
    return EFI_INVALID_PARAMETER;
  }

  return (EFI_STATUS)mock ();
}

/**
  Determines the new virtual address that is to be used on subsequent memory accesses.

  @param[in]      DebugDisposition  Supplies type information for the pointer being converted.
  @param[in,out]  Address           A pointer to a pointer that is to be fixed to be the value needed
                                    for the new virtual address mappings being applied.

  @retval EFI_SUCCESS           The pointer pointed to by Address was modified.
  @retval EFI_INVALID_PARAMETER Address is NULL.
  @retval EFI_INVALID_PARAMETER *Address is NULL and DebugDisposition does not have the
                                EFI_OPTIONAL_PTR bit set.
  @retval EFI_NOT_FOUND         The pointer pointed to by Address was not found to be part
                                of the current memory map. This is normally fatal.
**/
EFI_STATUS
EFIAPI
MockConvertPointer (
  IN UINTN     DebugDisposition,
  IN OUT VOID  **Address
  )
{
  EFI_STATUS  Status;

  // Address is required parameter
  if (Address == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // If this is a null pointer, return if it's allowed
  //
  if (*Address == 0) {
    if ((DebugDisposition & EFI_OPTIONAL_PTR) != 0) {
      return EFI_SUCCESS;
    }

    return EFI_INVALID_PARAMETER;
  }

  Status = (EFI_STATUS)mock ();

  if (!EFI_ERROR (Status)) {
    // Simulate pointer conversion by providing mock converted address
    *Address = mock_ptr_type (VOID *);
  }

  return Status;
}

/**
  Returns the value of a variable.

  @param[in]       VariableName  A Null-terminated string that is the name of the vendor's variable.
  @param[in]       VendorGuid    A unique identifier for the vendor.
  @param[out]      Attributes    If not NULL, a pointer to the memory location to return the
                                 attributes bitmask for the variable.
  @param[in,out]   DataSize      On input, the size in bytes of the return Data buffer.
                                 On output the size of data returned in Data.
  @param[out]      Data          The buffer to return the contents of the variable. May be NULL
                                 with a zero DataSize in order to determine the size buffer needed.

  @retval EFI_SUCCESS            The function completed successfully.
  @retval EFI_NOT_FOUND          The variable was not found.
  @retval EFI_BUFFER_TOO_SMALL   The DataSize is too small for the result.
  @retval EFI_INVALID_PARAMETER  VariableName is NULL.
  @retval EFI_INVALID_PARAMETER  VendorGuid is NULL.
  @retval EFI_INVALID_PARAMETER  DataSize is NULL.
  @retval EFI_INVALID_PARAMETER  The DataSize is not too small and Data is NULL.
  @retval EFI_DEVICE_ERROR       The variable could not be retrieved due to a hardware error.
  @retval EFI_SECURITY_VIOLATION The variable could not be retrieved due to an authentication failure.
**/
EFI_STATUS
EFIAPI
MockGetVariable (
  IN CHAR16     *VariableName,
  IN EFI_GUID   *VendorGuid,
  OUT UINT32    *Attributes OPTIONAL,
  IN OUT UINTN  *DataSize,
  OUT VOID      *Data OPTIONAL
  )
{
  EFI_STATUS  Status;

  // VariableName, VendorGuid, and DataSize are required parameters
  if ((VariableName == NULL) || (VendorGuid == NULL) || (DataSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (VariableName[0] == 0) {
    return EFI_NOT_FOUND;
  }

  Status = (EFI_STATUS)mock ();

  if (EFI_ERROR (Status) && (Status != EFI_BUFFER_TOO_SMALL)) {
    return Status;
  }

  if (Status == EFI_BUFFER_TOO_SMALL) {
    // Get required size
    *DataSize = (UINTN)mock ();

    // Handle buffer too small case

    return Status;
  }

  // Since its a success case, and DataSize meets the
  // requirement, Data should be non-NULL
  // As per UEFI spec: "The DataSize is not too small and
  // Data is NULL" returns EFI_INVALID_PARAMETER
  //
  if (Data == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  check_expected_ptr (VariableName);
  check_expected_ptr (VendorGuid);
  check_expected (*DataSize);
  // Copy data - Data is guaranteed non-NULL at this point
  CopyMem (Data, mock_ptr_type (VOID *), *DataSize);

  if (Attributes != NULL) {
    *Attributes = (UINT32)mock ();
  }

  return Status;
}

/**
  Enumerates the current variable names.

  @param[in,out]  VariableNameSize  The size of the VariableName buffer. The size must be large
                                    enough to fit input string supplied in VariableName buffer.
  @param[in,out]  VariableName      On input, supplies the last VariableName that was returned
                                    by GetNextVariableName(). On output, returns the Nullterminated
                                    string of the current variable.
  @param[in,out]  VendorGuid        On input, supplies the last VendorGuid that was returned by
                                    GetNextVariableName(). On output, returns the
                                    VendorGuid of the current variable.

  @retval EFI_SUCCESS            The function completed successfully.
  @retval EFI_NOT_FOUND          The next variable was not found.
  @retval EFI_BUFFER_TOO_SMALL   The VariableNameSize is too small for the result.
                                 VariableNameSize has been updated with the size needed to complete the request.
  @retval EFI_INVALID_PARAMETER  VariableNameSize is NULL.
  @retval EFI_INVALID_PARAMETER  VariableName is NULL.
  @retval EFI_INVALID_PARAMETER  VendorGuid is NULL.
  @retval EFI_INVALID_PARAMETER  The input values of VariableName and VendorGuid are not a name and
                                 GUID of an existing variable.
  @retval EFI_INVALID_PARAMETER  Null-terminator is not found in the first VariableNameSize bytes of
                                 the input VariableName buffer.
  @retval EFI_DEVICE_ERROR       The variable could not be retrieved due to a hardware error.
**/
EFI_STATUS
EFIAPI
MockGetNextVariableName (
  IN OUT UINTN     *VariableNameSize,
  IN OUT CHAR16    *VariableName,
  IN OUT EFI_GUID  *VendorGuid
  )
{
  EFI_STATUS  Status;
  UINTN       Index;
  BOOLEAN     Found = FALSE;

  // All parameters are required
  if ((VariableNameSize == NULL) || (VariableName == NULL) || (VendorGuid == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  // VariableNameSize must be large enough to fit input string
  if ((*VariableNameSize > 0) && (VariableName[0] != 0)) {
    // Verify null-terminator exists within the buffer
    for (Index = 0; Index < (*VariableNameSize / sizeof (CHAR16)); Index++) {
      if (VariableName[Index] == 0) {
        Found = TRUE;
        break;
      }
    }

    if (!Found) {
      return EFI_INVALID_PARAMETER;
    }
  }

  Status = (EFI_STATUS)mock ();

  // Exit early for errors (except BUFFER_TOO_SMALL) without
  // checking expected values
  if (EFI_ERROR (Status) && (Status != EFI_BUFFER_TOO_SMALL)) {
    return Status;
  }

  if (Status == EFI_BUFFER_TOO_SMALL) {
    *VariableNameSize = (UINTN)mock ();
    return Status;
  }

  // Only check expected values when successful
  check_expected (*VariableNameSize);
  check_expected_ptr (VariableName);
  check_expected_ptr (VendorGuid);

  // Copy mock variable name and GUID for successful case
  StrCpyS (VariableName, *VariableNameSize / sizeof (CHAR16), (CHAR16 *)mock_ptr_type (VOID *));
  CopyMem (VendorGuid, mock_ptr_type (VOID *), sizeof (EFI_GUID));

  return Status;
}

/**
  Sets the value of a variable.

  @param[in]  VariableName  A Null-terminated string that is the name of the vendor's variable.
                            Each VariableName is unique for each VendorGuid. VariableName must
                            contain 1 or more characters. If VariableName is an empty string,
                            then EFI_INVALID_PARAMETER is returned.
  @param[in]  VendorGuid    A unique identifier for the vendor.
  @param[in]  Attributes    Attributes bitmask to set for the variable.
  @param[in]  DataSize      The size in bytes of the Data buffer. Unless the EFI_VARIABLE_APPEND_WRITE or
                            EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS attribute is set, a size of zero
                            causes the variable to be deleted. When the EFI_VARIABLE_APPEND_WRITE attribute is
                            set, then a SetVariable() call with a DataSize of zero will not cause any change to
                            the variable value (the timestamp associated with the variable may be updated however
                            even if no new data value is provided,see the description of the
                            EFI_VARIABLE_AUTHENTICATION_2 descriptor below. In this case the DataSize will not
                            be zero since the EFI_VARIABLE_AUTHENTICATION_2 descriptor will be populated).
  @param[in]  Data          The contents for the variable.

  @retval EFI_SUCCESS            The firmware has successfully stored the variable and its data as
                                 defined by the Attributes.
  @retval EFI_INVALID_PARAMETER  An invalid combination of attribute bits, name, and GUID was supplied, or the
                                 DataSize exceeds the maximum allowed.
  @retval EFI_INVALID_PARAMETER  VariableName is an empty string.
  @retval EFI_OUT_OF_RESOURCES   Not enough storage is available to hold the variable and its data.
  @retval EFI_DEVICE_ERROR       The variable could not be retrieved due to a hardware error.
  @retval EFI_WRITE_PROTECTED    The variable in question is read-only.
  @retval EFI_WRITE_PROTECTED    The variable in question cannot be deleted.
  @retval EFI_SECURITY_VIOLATION The variable could not be written due to EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACESS being set,
                                 but the AuthInfo does NOT pass the validation check carried out by the firmware.
  @retval EFI_NOT_FOUND          The variable trying to be updated or deleted was not found.
**/
EFI_STATUS
EFIAPI
MockSetVariable (
  IN CHAR16    *VariableName,
  IN EFI_GUID  *VendorGuid,
  IN UINT32    Attributes,
  IN UINTN     DataSize,
  IN VOID      *Data
  )
{
  EFI_STATUS  Status;
  UINTN       PayloadSize;

  //
  // Check input parameters.
  //
  if ((VariableName == NULL) || (VariableName[0] == 0) || (VendorGuid == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((DataSize != 0) && (Data == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check for reserverd bit in variable attribute.
  // EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS is deprecated but we still allow
  // the delete operation of common authenticated variable at user physical presence.
  //
  if ((Attributes & (~(EFI_VARIABLE_ATTRIBUTES_MASK | EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS))) != 0) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Check if the combination of attribute bits is valid.
  //
  if ((Attributes & (EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS)) == EFI_VARIABLE_RUNTIME_ACCESS) {
    //
    // Make sure if runtime bit is set, boot service bit is set also.
    //
    if ((Attributes & EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS) != 0) {
      return EFI_UNSUPPORTED;
    } else {
      return EFI_INVALID_PARAMETER;
    }
  } else if ((Attributes & EFI_VARIABLE_ATTRIBUTES_MASK) == EFI_VARIABLE_NON_VOLATILE) {
    //
    // Only EFI_VARIABLE_NON_VOLATILE attribute is invalid
    //
    if ((Attributes & EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS) != 0) {
      return EFI_UNSUPPORTED;
    } else {
      return EFI_INVALID_PARAMETER;
    }
  }

  //
  // EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS and EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS attribute
  // cannot be set both.
  //
  if (  ((Attributes & EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS) == EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS)
     && ((Attributes & EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS) ==
         EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS))
  {
    return EFI_UNSUPPORTED;
  }

  if ((Attributes & EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS) == EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS) {
    //
    //  If DataSize == AUTHINFO_SIZE and then PayloadSize is 0.
    //  Maybe it's the delete operation of common authenticated variable at user physical presence.
    //
    if (DataSize != AUTHINFO_SIZE) {
      return EFI_UNSUPPORTED;
    }

    PayloadSize = DataSize - AUTHINFO_SIZE;
  } else if ((Attributes & EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS) ==
             EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS)
  {
    //
    // Sanity check for EFI_VARIABLE_AUTHENTICATION_2 descriptor.
    //
    if ((DataSize < OFFSET_OF_AUTHINFO2_CERT_DATA) ||
        (((EFI_VARIABLE_AUTHENTICATION_2 *)Data)->AuthInfo.Hdr.dwLength >
         DataSize - (OFFSET_OF (EFI_VARIABLE_AUTHENTICATION_2, AuthInfo))) ||
        (((EFI_VARIABLE_AUTHENTICATION_2 *)Data)->AuthInfo.Hdr.dwLength <
         OFFSET_OF (WIN_CERTIFICATE_UEFI_GUID, CertData)))
    {
      return EFI_SECURITY_VIOLATION;
    }

    PayloadSize = DataSize - AUTHINFO2_SIZE (Data);
  } else {
    PayloadSize = DataSize;
  }

  if ((UINTN)(~0) - PayloadSize < StrSize (VariableName)) {
    //
    // Prevent whole variable size overflow
    //
    return EFI_INVALID_PARAMETER;
  }

  Status = (EFI_STATUS)mock ();

  if (!EFI_ERROR (Status)) {
    check_expected_ptr (VariableName);
    check_expected_ptr (VendorGuid);
    check_expected (Attributes);
    check_expected (DataSize);
    // Only check Data pointer if DataSize is non-zero (Data can be NULL for variable deletion)
    if (DataSize != 0) {
      check_expected_ptr (Data);
    }
  }

  return Status;
}

/**
  Returns the next high 32 bits of the platform's monotonic counter.

  @param[out]  HighCount  Pointer to returned value.

  @retval EFI_SUCCESS           The next high monotonic count was returned.
  @retval EFI_INVALID_PARAMETER HighCount is NULL.
  @retval EFI_DEVICE_ERROR      The device is not functioning properly.
**/
EFI_STATUS
EFIAPI
MockGetNextHighMonotonicCount (
  OUT UINT32  *HighCount
  )
{
  EFI_STATUS  Status;

  // HighCount is required parameter
  if (HighCount == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = (EFI_STATUS)mock ();

  if (!EFI_ERROR (Status)) {
    *HighCount = (UINT32)mock ();
  }

  return Status;
}

/**
  Resets the entire platform.

  @param[in]  ResetType    The type of reset to perform.
  @param[in]  ResetStatus  The status code for the reset.
  @param[in]  DataSize     The size, in bytes, of ResetData.
  @param[in]  ResetData    For a ResetType of EfiResetCold, EfiResetWarm, or EfiResetShutdown
                           the data buffer starts with a Null-terminated string, optionally
                           followed by additional binary data. The string is a description
                           that the caller may use to further indicate the reason for the
                           system reset.
**/
VOID
EFIAPI
MockResetSystem (
  IN EFI_RESET_TYPE  ResetType,
  IN EFI_STATUS      ResetStatus,
  IN UINTN           DataSize,
  IN VOID            *ResetData OPTIONAL
  )
{
  // Mock implementation - normally this function doesn't return
}

/**
  Passes capsules to the firmware with both virtual and physical mapping. Depending on the intended
  consumption, the firmware may process the capsule immediately. If the payload should persist
  across a system reset, the reset value returned from EFI_QueryCapsuleCapabilities must
  be passed into ResetSystem() and will cause the capsule to be processed by the firmware as
  part of the reset process.

  @param[in]  CapsuleHeaderArray  Virtual pointer to an array of virtual pointers to the capsules
                                  being passed into update capsule.
  @param[in]  CapsuleCount        Number of pointers to EFI_CAPSULE_HEADER in
                                  CapsuleHeaderArray.
  @param[in]  ScatterGatherList   Physical pointer to a set of
                                  EFI_CAPSULE_BLOCK_DESCRIPTOR that describes the
                                  location in physical memory of a set of capsules.

  @retval EFI_SUCCESS            Valid capsule was passed. If
                                 CAPSULE_FLAGS_PERSIT_ACROSS_RESET is not set, the
                                 capsule has been successfully processed by the firmware.
  @retval EFI_INVALID_PARAMETER  CapsuleSize is NULL, or an incompatible set of flags were
                                 set in the capsule header.
  @retval EFI_INVALID_PARAMETER  CapsuleCount is 0.
  @retval EFI_DEVICE_ERROR       The capsule update was started, but failed due to a device error.
  @retval EFI_UNSUPPORTED        The capsule type is not supported on this platform.
  @retval EFI_OUT_OF_RESOURCES   When ExitBootServices() has been previously called this error indicates the capsule
                                 is compatible with this platform but is not capable of being submitted or processed
                                 in runtime. The caller may resubmit the capsule prior to ExitBootServices().
  @retval EFI_OUT_OF_RESOURCES   When ExitBootServices() has not been previously called then this error indicates
                                 the capsule is compatible with this platform but there are insufficient resources to process.
**/
EFI_STATUS
EFIAPI
MockUpdateCapsule (
  IN EFI_CAPSULE_HEADER    **CapsuleHeaderArray,
  IN UINTN                 CapsuleCount,
  IN EFI_PHYSICAL_ADDRESS  ScatterGatherList OPTIONAL
  )
{
  // CapsuleHeaderArray is required, CapsuleCount must be > 0
  if (CapsuleHeaderArray == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (CapsuleCount == 0) {
    return EFI_INVALID_PARAMETER;
  }

  return (EFI_STATUS)mock ();
}

/**
  Returns if the capsule can be supported via UpdateCapsule().

  @param[in]   CapsuleHeaderArray  Virtual pointer to an array of virtual pointers to the capsules
                                   being passed into update capsule.
  @param[in]   CapsuleCount        Number of pointers to EFI_CAPSULE_HEADER in
                                   CapsuleHeaderArray.
  @param[out]  MaximumCapsuleSize  On output the maximum size that UpdateCapsule() can
                                   support as an argument to UpdateCapsule() via
                                   CapsuleHeaderArray and ScatterGatherList.
  @param[out]  ResetType           Returns the type of reset required for the capsule update.

  @retval EFI_SUCCESS            Valid answer returned.
  @retval EFI_UNSUPPORTED        The capsule image is not supported on this platform, and
                                 MaximumCapsuleSize and ResetType are undefined.
  @retval EFI_INVALID_PARAMETER  MaximumCapsuleSize is NULL.
  @retval EFI_OUT_OF_RESOURCES   When ExitBootServices() has been previously called this error indicates the capsule
                                 is compatible with this platform but is not capable of being submitted or processed
                                 in runtime. The caller may resubmit the capsule prior to ExitBootServices().
  @retval EFI_OUT_OF_RESOURCES   When ExitBootServices() has not been previously called then this error indicates
                                 the capsule is compatible with this platform but there are insufficient resources to process.
**/
EFI_STATUS
EFIAPI
MockQueryCapsuleCapabilities (
  IN EFI_CAPSULE_HEADER  **CapsuleHeaderArray,
  IN UINTN               CapsuleCount,
  OUT UINT64             *MaximumCapsuleSize,
  OUT EFI_RESET_TYPE     *ResetType
  )
{
  EFI_STATUS  Status;

  // All parameters except ScatterGatherList are required
  if ((CapsuleHeaderArray == NULL) || (MaximumCapsuleSize == NULL) || (ResetType == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (CapsuleCount == 0) {
    return EFI_INVALID_PARAMETER;
  }

  Status = (EFI_STATUS)mock ();

  if (!EFI_ERROR (Status)) {
    *MaximumCapsuleSize = (UINT64)mock ();
    *ResetType          = (EFI_RESET_TYPE)mock ();
  }

  return Status;
}

/**
  Returns information about the EFI variables.

  @param[in]   Attributes                       Attributes bitmask to specify the type of variables on
                                                which to return information.
  @param[out]  MaximumVariableStorageSize       On output the maximum size of the storage space
                                                available for the EFI variables associated with the
                                                attributes specified.
  @param[out]  RemainingVariableStorageSize     Returns the remaining size of the storage space
                                                available for the EFI variables associated with the
                                                attributes specified.
  @param[out]  MaximumVariableSize              Returns the maximum size of the individual EFI
                                                variables associated with the attributes specified.

  @retval EFI_SUCCESS            Valid answer returned.
  @retval EFI_INVALID_PARAMETER  An invalid combination of attribute bits was supplied.
  @retval EFI_UNSUPPORTED        The attribute is not supported on this platform, and the
                                 MaximumVariableStorageSize,
                                 RemainingVariableStorageSize, MaximumVariableSize
                                 are undefined.
**/
EFI_STATUS
EFIAPI
MockQueryVariableInfo (
  IN UINT32   Attributes,
  OUT UINT64  *MaximumVariableStorageSize,
  OUT UINT64  *RemainingVariableStorageSize,
  OUT UINT64  *MaximumVariableSize
  )
{
  EFI_STATUS  Status;

  // All output parameters are required
  if ((MaximumVariableStorageSize == NULL) ||
      (RemainingVariableStorageSize == NULL) ||
      (MaximumVariableSize == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  if ((Attributes & EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS) != 0) {
    //
    //  Deprecated attribute, make this check as highest priority.
    //
    return EFI_UNSUPPORTED;
  }

  if ((Attributes & EFI_VARIABLE_ATTRIBUTES_MASK) == 0) {
    //
    // Make sure the Attributes combination is supported by the platform.
    //
    return EFI_UNSUPPORTED;
  } else if ((Attributes & EFI_VARIABLE_ATTRIBUTES_MASK) == EFI_VARIABLE_NON_VOLATILE) {
    //
    // Only EFI_VARIABLE_NON_VOLATILE attribute is invalid
    //
    return EFI_INVALID_PARAMETER;
  } else if ((Attributes & (EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_BOOTSERVICE_ACCESS)) ==
             EFI_VARIABLE_RUNTIME_ACCESS)
  {
    //
    // Make sure if runtime bit is set, boot service bit is set also.
    //
    return EFI_INVALID_PARAMETER;
  } else if ((Attributes & (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_HARDWARE_ERROR_RECORD)) ==
             EFI_VARIABLE_HARDWARE_ERROR_RECORD)
  {
    //
    // Make sure Hw Attribute is set with NV.
    //
    return EFI_INVALID_PARAMETER;
  }

  Status = (EFI_STATUS)mock ();

  if (!EFI_ERROR (Status)) {
    *MaximumVariableStorageSize   = (UINT64)mock ();
    *RemainingVariableStorageSize = (UINT64)mock ();
    *MaximumVariableSize          = (UINT64)mock ();
  }

  return Status;
}

//
// Global Mock Runtime Services Table for host application - initialized with mock functions
//
EFI_RUNTIME_SERVICES  gMockRuntime = {
  .Hdr                       = {
    .Signature  = EFI_RUNTIME_SERVICES_SIGNATURE,
    .Revision   = EFI_RUNTIME_SERVICES_REVISION,
    .HeaderSize = sizeof (EFI_RUNTIME_SERVICES),
    .CRC32      = 0, // Not used in unit tests
    .Reserved   = 0
  },
  .GetTime                   = MockGetTime,
  .SetTime                   = MockSetTime,
  .GetWakeupTime             = MockGetWakeupTime,
  .SetWakeupTime             = MockSetWakeupTime,
  .SetVirtualAddressMap      = MockSetVirtualAddressMap,
  .ConvertPointer            = MockConvertPointer,
  .GetVariable               = MockGetVariable,
  .GetNextVariableName       = MockGetNextVariableName,
  .SetVariable               = MockSetVariable,
  .GetNextHighMonotonicCount = MockGetNextHighMonotonicCount,
  .ResetSystem               = MockResetSystem,
  .UpdateCapsule             = MockUpdateCapsule,
  .QueryCapsuleCapabilities  = MockQueryCapsuleCapabilities,
  .QueryVariableInfo         = MockQueryVariableInfo
};

EFI_RUNTIME_SERVICES  *gRT = &gMockRuntime;

/**
  Initializes the Mock Runtime Services Table by replacing the global gRT pointer.
  @return VOID
**/
VOID
EFIAPI
InitMockRuntimeServicesTablePointer (
  VOID
  )
{
  // Replace gRT with pre-initialized mock Runtime Services table
  gRT = &gMockRuntime;
  return;
}
