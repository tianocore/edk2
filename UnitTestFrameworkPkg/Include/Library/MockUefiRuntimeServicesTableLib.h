/** @file MockUefiRuntimeServicesTableLib.h
  Mock Unit Test UEFI Runtime Services Table Library Implementation for Unit Testing

  This library provides comprehensive mocking of UEFI Runtime Services
  functions using CMocka framework. Each function can return data based
  on mock() implementation.

  Copyright (c) 2026, American Megatrends International LLC. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#ifndef __MOCK_UEFI_RUNTIME_SERVICES_TABLE_LIB_H__
#define __MOCK_UEFI_RUNTIME_SERVICES_TABLE_LIB_H__

#include <Uefi.h>
#include <Library/BaseLib.h>

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

/**
  Initializes the Mock Runtime Services Table by replacing the global gRT pointer.
  @return VOID
**/
VOID
EFIAPI
InitMockRuntimeServicesTablePointer (
  VOID
  );

//
// Helper Macros for setting up mock return values and expected parameters
//

/**
  Sets up a successful MockGetTime mock expectation.
  Queues EFI_SUCCESS as the return status and enqueues each field of
  the supplied EFI_TIME structure as successive mock return values.

  @param[in]  Time  Pointer to an EFI_TIME structure whose fields are
                    queued as the values that MockGetTime will output.
**/
#define MOCK_GET_TIME_RETURN_SUCCESS(Time) \
  will_return (MockGetTime, EFI_SUCCESS); \
  will_return (MockGetTime, (Time)->Year); \
  will_return (MockGetTime, (Time)->Month); \
  will_return (MockGetTime, (Time)->Day); \
  will_return (MockGetTime, (Time)->Hour); \
  will_return (MockGetTime, (Time)->Minute); \
  will_return (MockGetTime, (Time)->Second); \
  will_return (MockGetTime, (Time)->Nanosecond); \
  will_return (MockGetTime, (Time)->TimeZone); \
  will_return (MockGetTime, (Time)->Daylight);

/**
  Sets up a successful MockGetTime mock expectation that also returns
  hardware capability information.
  Queues EFI_SUCCESS as the return status, enqueues each field of the
  supplied EFI_TIME structure, and then enqueues each field of the
  supplied EFI_TIME_CAPABILITIES structure as successive mock return values.

  @param[in]  Time          Pointer to an EFI_TIME structure whose fields are
                            queued as the time values that MockGetTime will output.
  @param[in]  Capabilities  Pointer to an EFI_TIME_CAPABILITIES structure whose
                            fields are queued as the capability values that
                            MockGetTime will output.
**/
#define MOCK_GET_TIME_WITH_CAPABILITIES_RETURN_SUCCESS(Time, Capabilities) \
  will_return (MockGetTime, EFI_SUCCESS); \
  will_return (MockGetTime, (Time)->Year); \
  will_return (MockGetTime, (Time)->Month); \
  will_return (MockGetTime, (Time)->Day); \
  will_return (MockGetTime, (Time)->Hour); \
  will_return (MockGetTime, (Time)->Minute); \
  will_return (MockGetTime, (Time)->Second); \
  will_return (MockGetTime, (Time)->Nanosecond); \
  will_return (MockGetTime, (Time)->TimeZone); \
  will_return (MockGetTime, (Time)->Daylight); \
  will_return (MockGetTime, (Capabilities)->Resolution); \
  will_return (MockGetTime, (Capabilities)->Accuracy); \
  will_return (MockGetTime, (Capabilities)->SetsToZero);

/**
  Sets up a failing MockGetTime mock expectation.
  Queues the given error status as the sole mock return value so that
  MockGetTime reports a failure without populating any output fields.

  @param[in]  Status  The EFI_STATUS error code to queue as the return value.
**/
#define MOCK_GET_TIME_RETURN_ERROR(Status) \
  will_return (MockGetTime, Status);

/**
  Sets up a MockSetTime mock expectation with the given return status.
  Queues Status as the mock return value so that MockSetTime returns
  the specified code when called by the code under test.

  @param[in]  Status  The EFI_STATUS code to queue as the return value
                      (e.g. EFI_SUCCESS, EFI_INVALID_PARAMETER).
**/
#define MOCK_SET_TIME_RETURN_STATUS(Status) \
  will_return (MockSetTime, Status);

/**
  Sets up a successful MockGetWakeupTime mock expectation.
  Queues EFI_SUCCESS as the return status, then enqueues the Enabled and
  Pending boolean values, and finally enqueues each field of the supplied
  EFI_TIME structure as successive mock return values.

  @param[in]  Enabled  Boolean value indicating whether the wakeup alarm is
                       enabled; queued as a mock return value for MockGetWakeupTime.
  @param[in]  Pending  Boolean value indicating whether an alarm signal is pending;
                       queued as a mock return value for MockGetWakeupTime.
  @param[in]  Time     Pointer to an EFI_TIME structure whose fields are queued
                       as the alarm time values that MockGetWakeupTime will output.
**/
#define MOCK_GET_WAKEUP_TIME_RETURN_SUCCESS(Enabled, Pending, Time) \
  will_return (MockGetWakeupTime, EFI_SUCCESS); \
  will_return (MockGetWakeupTime, Enabled); \
  will_return (MockGetWakeupTime, Pending); \
  will_return (MockGetWakeupTime, (Time)->Year); \
  will_return (MockGetWakeupTime, (Time)->Month); \
  will_return (MockGetWakeupTime, (Time)->Day); \
  will_return (MockGetWakeupTime, (Time)->Hour); \
  will_return (MockGetWakeupTime, (Time)->Minute); \
  will_return (MockGetWakeupTime, (Time)->Second); \
  will_return (MockGetWakeupTime, (Time)->Nanosecond); \
  will_return (MockGetWakeupTime, (Time)->TimeZone); \
  will_return (MockGetWakeupTime, (Time)->Daylight);

/**
  Sets up a failing MockGetWakeupTime mock expectation.
  Queues the given error status as the sole mock return value so that
  MockGetWakeupTime reports a failure without populating any output fields.

  @param[in]  Status  The EFI_STATUS error code to queue as the return value.
**/
#define MOCK_GET_WAKEUP_TIME_RETURN_ERROR(Status) \
  will_return (MockGetWakeupTime, Status);

/**
  Sets up a MockSetWakeupTime mock expectation with the given return status.
  Queues Status as the mock return value so that MockSetWakeupTime returns
  the specified code when called by the code under test.

  @param[in]  Status  The EFI_STATUS code to queue as the return value
                      (e.g. EFI_SUCCESS, EFI_INVALID_PARAMETER).
**/
#define MOCK_SET_WAKEUP_TIME_RETURN_STATUS(Status) \
  will_return (MockSetWakeupTime, Status);

/**
  Sets up a MockSetVirtualAddressMap mock expectation with the given return status.
  Queues Status as the mock return value so that MockSetVirtualAddressMap
  returns the specified code when called by the code under test.

  @param[in]  Status  The EFI_STATUS code to queue as the return value
                      (e.g. EFI_SUCCESS, EFI_UNSUPPORTED).
**/
#define MOCK_SET_VIRTUAL_ADDRESS_MAP_RETURN_STATUS(Status) \
  will_return (MockSetVirtualAddressMap, Status);

/**
  Sets up a successful MockConvertPointer mock expectation.
  Queues EFI_SUCCESS as the return status and then enqueues
  ConvertedAddress as the converted virtual address that MockConvertPointer
  will write back to the caller's pointer.

  @param[in]  ConvertedAddress  The virtual address value to queue as the
                                converted pointer output of MockConvertPointer.
**/
#define MOCK_CONVERT_POINTER_RETURN_SUCCESS(ConvertedAddress) \
  will_return (MockConvertPointer, EFI_SUCCESS); \
  will_return (MockConvertPointer, ConvertedAddress);

/**
  Sets up a failing MockConvertPointer mock expectation.
  Queues the given error status as the sole mock return value so that
  MockConvertPointer reports a failure without modifying the caller's pointer.

  @param[in]  Status  The EFI_STATUS error code to queue as the return value.
**/
#define MOCK_CONVERT_POINTER_RETURN_ERROR(Status) \
  will_return (MockConvertPointer, Status);

/**
  Sets up a successful MockGetVariable mock expectation that validates
  exact input parameters and returns data when Attributes is NULL.
  Queues EFI_SUCCESS as the return status, registers exact-match
  expectations for VariableName, VendorGuid, and DataSize, and enqueues
  the data buffer pointer as the mock output.

  @param[in]  VarName   Pointer to the Null-terminated Unicode variable name
                        that the code under test is expected to pass.
  @param[in]  VenGuid   Pointer to the EFI_GUID that the code under test
                        is expected to pass as VendorGuid.
  @param[in]  Size      The exact DataSize value the code under test is
                        expected to pass on input.
  @param[in]  Data      Pointer to the data buffer to queue as the variable
                        contents returned by MockGetVariable.
**/
#define MOCK_GET_VARIABLE_RETURN_SUCCESS(VarName, VenGuid, Size, Data) \
  will_return (MockGetVariable, EFI_SUCCESS); \
  expect_memory (MockGetVariable, VariableName, VarName, StrSize(VarName)); \
  expect_memory (MockGetVariable, VendorGuid, VenGuid, sizeof (EFI_GUID)); \
  expect_value (MockGetVariable, *DataSize, Size); \
  will_return (MockGetVariable, Data);

/**
  Sets up a successful MockGetVariable mock expectation that validates
  exact input parameters, returns data, and also returns an Attributes value.
  Queues EFI_SUCCESS as the return status, registers exact-match
  expectations for VariableName, VendorGuid, and DataSize, and enqueues
  both the data buffer pointer and the attributes value as mock outputs.
  Use this macro when the code under test passes a non-NULL Attributes pointer.

  @param[in]  VarName   Pointer to the Null-terminated Unicode variable name
                        that the code under test is expected to pass.
  @param[in]  VenGuid   Pointer to the EFI_GUID that the code under test
                        is expected to pass as VendorGuid.
  @param[in]  Size      The exact DataSize value the code under test is
                        expected to pass on input.
  @param[in]  Data      Pointer to the data buffer to queue as the variable
                        contents returned by MockGetVariable.
  @param[in]  Attr      The UINT32 attributes bitmask to queue as the value
                        written to the caller's Attributes output parameter.
**/
#define MOCK_GET_VARIABLE_RETURN_SUCCESS_WITH_ATTR(VarName, VenGuid, Size, Data, Attr) \
  will_return (MockGetVariable, EFI_SUCCESS); \
  expect_memory (MockGetVariable, VariableName, VarName, StrSize(VarName)); \
  expect_memory (MockGetVariable, VendorGuid, VenGuid, sizeof (EFI_GUID)); \
  expect_value (MockGetVariable, *DataSize, Size); \
  will_return (MockGetVariable, Data); \
  will_return (MockGetVariable, Attr);

/**
  Sets up a successful MockGetVariable mock expectation without validating
  the VariableName, VendorGuid, or DataSize inputs.
  Queues EFI_SUCCESS as the return status, accepts any value for VariableName,
  VendorGuid, and DataSize, and enqueues the data buffer pointer as the mock
  output. Use this macro when the code under test passes Attributes = NULL
  and exact input validation is not required.

  @param[in]  Data  Pointer to the data buffer to queue as the variable
                    contents returned by MockGetVariable.
**/
#define MOCK_GET_VARIABLE_RETURN_SUCCESS_ANY(Data) \
  will_return (MockGetVariable, EFI_SUCCESS); \
  expect_any (MockGetVariable, VariableName); \
  expect_any (MockGetVariable, VendorGuid); \
  expect_any (MockGetVariable, *DataSize); \
  will_return (MockGetVariable, Data);

/**
  Sets up a successful MockGetVariable mock expectation without validating
  the VariableName, VendorGuid, or DataSize inputs, and also returns an
  Attributes value.
  Queues EFI_SUCCESS as the return status, accepts any value for VariableName,
  VendorGuid, and DataSize, and enqueues both the data buffer pointer and the
  attributes value as mock outputs. Use this macro when the code under test
  passes a non-NULL Attributes pointer and exact input validation is not required.

  @param[in]  Data  Pointer to the data buffer to queue as the variable
                    contents returned by MockGetVariable.
  @param[in]  Attr  The UINT32 attributes bitmask to queue as the value
                    written to the caller's Attributes output parameter.
**/
#define MOCK_GET_VARIABLE_RETURN_SUCCESS_ANY_WITH_ATTR(Data, Attr) \
  will_return (MockGetVariable, EFI_SUCCESS); \
  expect_any (MockGetVariable, VariableName); \
  expect_any (MockGetVariable, VendorGuid); \
  expect_any (MockGetVariable, *DataSize); \
  will_return (MockGetVariable, Data); \
  will_return (MockGetVariable, Attr);

/**
  Sets up a MockGetVariable mock expectation that returns EFI_BUFFER_TOO_SMALL.
  Queues EFI_BUFFER_TOO_SMALL as the return status and enqueues TargetSize
  as the required DataSize value that MockGetVariable will report back
  to the caller.

  @param[in]  TargetSize  The UINTN buffer size value to queue as the required
                          DataSize output when the caller's buffer is too small.
**/
#define MOCK_GET_VARIABLE_RETURN_BUFFER_TOO_SMALL(TargetSize) \
  will_return (MockGetVariable, EFI_BUFFER_TOO_SMALL); \
  will_return (MockGetVariable, TargetSize);

/**
  Sets up a failing MockGetVariable mock expectation.
  Queues the given error status as the sole mock return value so that
  MockGetVariable reports a failure without populating any output fields.

  @param[in]  Status  The EFI_STATUS error code to queue as the return value.
**/
#define MOCK_GET_VARIABLE_RETURN_ERROR(Status) \
  will_return (MockGetVariable, Status);

/**
  Sets up a successful MockGetNextVariableName mock expectation that validates
  exact input parameters and returns the next variable name and GUID.
  Registers exact-match expectations for VariableNameSize, VariableName, and
  VendorGuid, queues EFI_SUCCESS as the return status, and enqueues the
  returned variable name and vendor GUID pointers as mock output values.

  @param[in]  VarNameSize   The exact value of *VariableNameSize the code under
                            test is expected to pass on input.
  @param[in]  VarName       Pointer to the Null-terminated Unicode variable name
                            that the code under test is expected to pass on input.
  @param[in]  VenGuid       Pointer to the EFI_GUID that the code under test is
                            expected to pass as VendorGuid on input.
  @param[in]  RetVarName    Pointer to the Null-terminated Unicode string to queue
                            as the next variable name output of MockGetNextVariableName.
  @param[in]  RetVenGuid    Pointer to the EFI_GUID to queue as the next vendor GUID
                            output of MockGetNextVariableName.
**/
#define MOCK_GET_NEXT_VARIABLE_NAME_RETURN_SUCCESS(VarNameSize, VarName, VenGuid, RetVarName, RetVenGuid) \
  expect_value (MockGetNextVariableName, *VariableNameSize, VarNameSize); \
  expect_memory (MockGetNextVariableName, VariableName, VarName, VarNameSize); \
  expect_memory (MockGetNextVariableName, VendorGuid, VenGuid, sizeof (EFI_GUID)); \
  will_return (MockGetNextVariableName, EFI_SUCCESS); \
  will_return (MockGetNextVariableName, RetVarName); \
  will_return (MockGetNextVariableName, RetVenGuid);

/**
  Sets up a MockGetNextVariableName mock expectation that returns
  EFI_BUFFER_TOO_SMALL.
  Queues EFI_BUFFER_TOO_SMALL as the return status and enqueues TargetSize
  as the required VariableNameSize value that MockGetNextVariableName will
  report back to the caller.

  @param[in]  TargetSize  The UINTN buffer size value to queue as the required
                          VariableNameSize output when the caller's buffer is
                          too small.
**/
#define MOCK_GET_NEXT_VARIABLE_NAME_RETURN_BUFFER_TOO_SMALL(TargetSize) \
  will_return (MockGetNextVariableName, EFI_BUFFER_TOO_SMALL); \
  will_return (MockGetNextVariableName, TargetSize);

/**
  Sets up a failing MockGetNextVariableName mock expectation.
  Queues the given error status as the sole mock return value so that
  MockGetNextVariableName reports a failure without populating any output fields.

  @param[in]  Status  The EFI_STATUS error code to queue as the return value.
**/
#define MOCK_GET_NEXT_VARIABLE_NAME_RETURN_ERROR(Status) \
  will_return (MockGetNextVariableName, Status);

/**
  Sets up a MockSetVariable mock expectation that deletes a variable.
  Queues EFI_SUCCESS as the return status and registers exact-match expectations
  for VariableName, VendorGuid, and Attributes, with DataSize expected to be
  zero. Use this macro when the code under test calls SetVariable() to delete
  an existing variable (DataSize = 0, Data = NULL).

  @param[in]  VarName  Pointer to the Null-terminated Unicode variable name
                       that the code under test is expected to pass.
  @param[in]  VenGuid  Pointer to the EFI_GUID that the code under test is
                       expected to pass as VendorGuid.
  @param[in]  Attr     The UINT32 attributes bitmask that the code under test
                       is expected to pass as Attributes.
**/
#define MOCK_SET_VARIABLE_DELETE(VarName, VenGuid, Attr) \
  will_return (MockSetVariable, EFI_SUCCESS); \
  expect_memory (MockSetVariable, VariableName, VarName, StrSize(VarName)); \
  expect_memory (MockSetVariable, VendorGuid, VenGuid, sizeof (EFI_GUID)); \
  expect_value (MockSetVariable, Attributes, Attr); \
  expect_value (MockSetVariable, DataSize, 0);

/**
  Sets up a successful MockSetVariable mock expectation that validates all
  input parameters when writing variable data.
  Queues EFI_SUCCESS as the return status and registers exact-match
  expectations for VariableName, VendorGuid, Attributes, DataSize, and the
  contents of the Data buffer. Use this macro when the code under test calls
  SetVariable() to store a non-empty variable (DataSize > 0, Data != NULL).

  @param[in]  VarName     Pointer to the Null-terminated Unicode variable name
                          that the code under test is expected to pass.
  @param[in]  VenGuid     Pointer to the EFI_GUID that the code under test is
                          expected to pass as VendorGuid.
  @param[in]  Attr        The UINT32 attributes bitmask that the code under test
                          is expected to pass as Attributes.
  @param[in]  Size        The exact UINTN DataSize value the code under test is
                          expected to pass.
  @param[in]  DataBuffer  Pointer to the data buffer whose contents the code
                          under test is expected to pass in the Data parameter.
**/
#define MOCK_SET_VARIABLE_RETURN_SUCCESS(VarName, VenGuid, Attr, Size, DataBuffer) \
  will_return (MockSetVariable, EFI_SUCCESS); \
  expect_memory (MockSetVariable, VariableName, VarName, StrSize(VarName)); \
  expect_memory (MockSetVariable, VendorGuid, VenGuid, sizeof (EFI_GUID)); \
  expect_value (MockSetVariable, Attributes, Attr); \
  expect_value (MockSetVariable, DataSize, Size); \
  expect_memory (MockSetVariable, Data, DataBuffer, Size);

/**
  Sets up a successful MockSetVariable mock expectation without validating
  any input parameters.
  Queues EFI_SUCCESS as the return status and accepts any caller-supplied
  values for VariableName, VendorGuid, Attributes, DataSize, and Data.
  Use this macro when parameter validation is not required by the test.
**/
#define MOCK_SET_VARIABLE_RETURN_SUCCESS_ANY() \
  will_return (MockSetVariable, EFI_SUCCESS); \
  expect_any  (MockSetVariable, VariableName); \
  expect_any  (MockSetVariable, VendorGuid); \
  expect_any  (MockSetVariable, Attributes); \
  expect_any  (MockSetVariable, DataSize); \
  expect_any  (MockSetVariable, Data)

/**
  Sets up a failing MockSetVariable mock expectation.
  Queues the given error status as the sole mock return value so that
  MockSetVariable reports a failure without storing any data.

  @param[in]  Status  The EFI_STATUS error code to queue as the return value.
**/
#define MOCK_SET_VARIABLE_RETURN_ERROR(Status) \
  will_return (MockSetVariable, Status);

/**
  Sets up a successful MockGetNextHighMonotonicCount mock expectation.
  Queues EFI_SUCCESS as the return status and enqueues HighCount as the
  high 32-bit counter value that MockGetNextHighMonotonicCount will output.

  @param[in]  HighCount  The UINT32 high monotonic count value to queue as
                         the output of MockGetNextHighMonotonicCount.
**/
#define MOCK_GET_NEXT_HIGH_MONOTONIC_COUNT_RETURN_SUCCESS(HighCount) \
  will_return (MockGetNextHighMonotonicCount, EFI_SUCCESS); \
  will_return (MockGetNextHighMonotonicCount, HighCount);

/**
  Sets up a failing MockGetNextHighMonotonicCount mock expectation.
  Queues the given error status as the sole mock return value so that
  MockGetNextHighMonotonicCount reports a failure without populating
  the HighCount output.

  @param[in]  Status  The EFI_STATUS error code to queue as the return value.
**/
#define MOCK_GET_NEXT_HIGH_MONOTONIC_COUNT_RETURN_ERROR(Status) \
  will_return (MockGetNextHighMonotonicCount, Status);

/**
  Sets up a MockUpdateCapsule mock expectation with the given return status.
  Queues Status as the mock return value so that MockUpdateCapsule returns
  the specified code when called by the code under test.

  @param[in]  Status  The EFI_STATUS code to queue as the return value
                      (e.g. EFI_SUCCESS, EFI_UNSUPPORTED).
**/
#define MOCK_UPDATE_CAPSULE_RETURN_STATUS(Status) \
  will_return (MockUpdateCapsule, Status);

/**
  Sets up a successful MockQueryCapsuleCapabilities mock expectation.
  Queues EFI_SUCCESS as the return status, then enqueues MaximumCapsuleSize
  and ResetType as the output values that MockQueryCapsuleCapabilities will
  return to the caller.

  @param[in]  MaximumCapsuleSize  The UINT64 maximum capsule size value to
                                  queue as the output of
                                  MockQueryCapsuleCapabilities.
  @param[in]  ResetType           The EFI_RESET_TYPE value to queue as the
                                  required reset type output of
                                  MockQueryCapsuleCapabilities.
**/
#define MOCK_QUERY_CAPSULE_CAPABILITIES_RETURN_SUCCESS(MaximumCapsuleSize, ResetType) \
  will_return (MockQueryCapsuleCapabilities, EFI_SUCCESS); \
  will_return (MockQueryCapsuleCapabilities, MaximumCapsuleSize); \
  will_return (MockQueryCapsuleCapabilities, ResetType);

/**
  Sets up a failing MockQueryCapsuleCapabilities mock expectation.
  Queues the given error status as the sole mock return value so that
  MockQueryCapsuleCapabilities reports a failure without populating
  any output fields.

  @param[in]  Status  The EFI_STATUS error code to queue as the return value.
**/
#define MOCK_QUERY_CAPSULE_CAPABILITIES_RETURN_ERROR(Status) \
  will_return (MockQueryCapsuleCapabilities, Status);

/**
  Sets up a successful MockQueryVariableInfo mock expectation.
  Queues EFI_SUCCESS as the return status, then enqueues
  MaximumVariableStorageSize, RemainingVariableStorageSize, and
  MaximumVariableSize as the successive output values that
  MockQueryVariableInfo will return to the caller.

  @param[in]  MaximumVariableStorageSize    The UINT64 maximum variable storage
                                            size value to queue as output.
  @param[in]  RemainingVariableStorageSize  The UINT64 remaining variable storage
                                            size value to queue as output.
  @param[in]  MaximumVariableSize           The UINT64 maximum individual variable
                                            size value to queue as output.
**/
#define MOCK_QUERY_VARIABLE_INFO_RETURN_SUCCESS(MaximumVariableStorageSize, RemainingVariableStorageSize, MaximumVariableSize) \
  will_return (MockQueryVariableInfo, EFI_SUCCESS); \
  will_return (MockQueryVariableInfo, MaximumVariableStorageSize); \
  will_return (MockQueryVariableInfo, RemainingVariableStorageSize); \
  will_return (MockQueryVariableInfo, MaximumVariableSize);

/**
  Sets up a failing MockQueryVariableInfo mock expectation.
  Queues the given error status as the sole mock return value so that
  MockQueryVariableInfo reports a failure without populating any output fields.

  @param[in]  Status  The EFI_STATUS error code to queue as the return value.
**/
#define MOCK_QUERY_VARIABLE_INFO_RETURN_ERROR(Status) \
  will_return (MockQueryVariableInfo, Status);

#endif // __MOCK_UEFI_RUNTIME_SERVICES_TABLE_LIB_H__
