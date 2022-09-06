/** @file
  EDKII PEI Variable PPI provides an implementation of variables
  intended for use as a means to store data in the PEI environment.

  Copyright (c) 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef EDKII_PEI_VARIABLE_PPI_H_
#define EDKII_PEI_VARIABLE_PPI_H_

#define EDKII_PEI_VARIABLE_PPI_GUID \
  { \
    0xe7b2cd04, 0x4b14, 0x44c2, { 0xb7, 0x48, 0xce, 0xaf, 0x2b, 0x66, 0x4a, 0xb0 } \
  }

typedef struct _EDKII_PEI_VARIABLE_PPI EDKII_PEI_VARIABLE_PPI;

/**
  This service retrieves a variable's value using its name and GUID.

  Read the specified variable from the UEFI variable store. If the Data
  buffer is too small to hold the contents of the variable,
  the error EFI_BUFFER_TOO_SMALL is returned and DataSize is set to the
  required buffer size to obtain the data.

  @param[in]        This              A pointer to this instance of the EDKII_PEI_VARIABLE_PPI.
  @param[in]        VariableName      A pointer to a null-terminated string that is the variable's name.
  @param[in]        VariableGuid      A pointer to an EFI_GUID that is the variable's GUID. The combination of
                                      VariableGuid and VariableName must be unique.
  @param[out]       Attributes        If non-NULL, on return, points to the variable's attributes.
  @param[in, out]   DataSize          On entry, points to the size in bytes of the Data buffer.
                                      On return, points to the size of the data returned in Data.
  @param[out]       Data              Points to the buffer which will hold the returned variable value.
                                      May be NULL with a zero DataSize in order to determine the size of the
                                      buffer needed.

  @retval EFI_SUCCESS                 The variable was read successfully.
  @retval EFI_NOT_FOUND               The variable was not found.
  @retval EFI_BUFFER_TOO_SMALL        The DataSize is too small for the resulting data.
                                      DataSize is updated with the size required for
                                      the specified variable.
  @retval EFI_INVALID_PARAMETER       VariableName, VariableGuid, DataSize or Data is NULL.
  @retval EFI_DEVICE_ERROR            The variable could not be retrieved because of a device error.

**/
typedef
EFI_STATUS
(EFIAPI *EDKII_PEI_GET_VARIABLE)(
  IN CONST  EDKII_PEI_VARIABLE_PPI    *This,
  IN CONST  CHAR16                    *VariableName,
  IN CONST  EFI_GUID                  *VariableGuid,
  OUT       UINT32                    *Attributes   OPTIONAL,
  IN OUT    UINTN                     *DataSize,
  OUT       VOID                      *Data         OPTIONAL
  );

/**
  Return the next variable name and GUID.

  This function is called multiple times to retrieve the VariableName
  and VariableGuid of all variables currently available in the system.
  On each call, the previous results are passed into the interface,
  and, on return, the interface returns the data for the next
  variable. To get started, VariableName should initially contain L"\0"
  and VariableNameSize should be sizeof(CHAR16). When the entire
  variable list has been returned, EFI_NOT_FOUND is returned.

  @param[in]        This              A pointer to this instance of the EDKII_PEI_VARIABLE_PPI.
  @param[in, out]   VariableNameSize  On entry, points to the size of the buffer pointed to by VariableName.
                                      On return, the size of the variable name buffer.
  @param[in, out]   VariableName      On entry, a pointer to a null-terminated string that is the variable's name.
                                      On return, points to the next variable's null-terminated name string.
  @param[in, out]   VariableGuid      On entry, a pointer to an EFI_GUID that is the variable's GUID.
                                      On return, a pointer to the next variable's GUID.

  @retval EFI_SUCCESS                 The next variable name was read successfully.
  @retval EFI_NOT_FOUND               All variables have been enumerated.
  @retval EFI_BUFFER_TOO_SMALL        The VariableNameSize is too small for the resulting
                                      data. VariableNameSize is updated with the size
                                      required for the specified variable.
  @retval EFI_INVALID_PARAMETER       VariableName, VariableGuid or
                                      VariableNameSize is NULL.
  @retval EFI_DEVICE_ERROR            The variable could not be retrieved because of a device error.

**/
typedef
EFI_STATUS
(EFIAPI *EDKII_PEI_GET_NEXT_VARIABLE_NAME)(
  IN CONST  EDKII_PEI_VARIABLE_PPI    *This,
  IN OUT    UINTN                     *VariableNameSize,
  IN OUT    CHAR16                    *VariableName,
  IN OUT    EFI_GUID                  *VariableGuid
  );

/**
  Sets the value of a variable.

  @param[in]        This              A pointer to this instance of the EDKII_PEI_VARIABLE_PPI.
  @param[in]        VariableName      A Null-terminated string that is the name of the vendor's variable.
                                      Each VariableName is unique for each VendorGuid. VariableName must
                                      contain 1 or more characters. If VariableName is an empty string,
                                      then EFI_INVALID_PARAMETER is returned.
  @param[in]        VendorGuid        A unique identifier for the vendor.
  @param[in]        Attributes        Attributes bitmask to set for the variable.
  @param[in]        DataSize          The size in bytes of the Data buffer. Unless the EFI_VARIABLE_APPEND_WRITE
                                      attribute is set, a size of zero causes the variable to be deleted. When the
                                      EFI_VARIABLE_APPEND_WRITE attribute is set, then a SetVariable() call with a
                                      DataSize of zero will not cause any change to the variable value.
  @param[in]        Data              The contents for the variable.

  @retval EFI_SUCCESS                 The firmware has successfully stored the variable and its data as
                                      defined by the Attributes.
  @retval EFI_INVALID_PARAMETER       An invalid combination of attribute bits, name, and GUID was supplied, or the
                                      DataSize exceeds the maximum allowed.
  @retval EFI_INVALID_PARAMETER       VariableName is an empty string.
  @retval EFI_OUT_OF_RESOURCES        Not enough storage is available to hold the variable and its data.
  @retval EFI_DEVICE_ERROR            The variable could not be stored due to a hardware error.
  @retval EFI_WRITE_PROTECTED         The variable in question is read-only.
  @retval EFI_WRITE_PROTECTED         The variable in question cannot be deleted.
  @retval EFI_SECURITY_VIOLATION      The variable could not be written due to EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS,
                                      or EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS, or
                                      EFI_VARIABLE_ENHANCED_AUTHENTICATED_ACCESS being set. Writing to authenticated
                                      variables is not supported in the PEI environment. Updates to authenticated
                                      variables can be requested during PEI via the EFI_AUTHENTICATED_VARIABLE_HOB, but
                                      these updates won't be written to non-volatile storage until later in DXE.
                                      The EFI_AUTHENTICATED_VARIABLE_HOB is a HOB with the GUID
                                      gEfiAuthenticatedVariableGuid. This HOB contains a VARIABLE_STORE_HEADER followed
                                      by one or more UEFI variables, which are stored as DWORD aligned tuples of
                                      (VARIABLE_HEADER + CHAR16 VariableName + VariableData).
                                      See MdeModulePkg/Include/Guid/VariableFormat.h for these data structure
                                      definitions and MdeModulePkg/Universal/Variable/RuntimeDxe/VariableParsing.c for
                                      an example of how to parse these data structures.
  @retval EFI_NOT_FOUND               The variable trying to be updated or deleted was not found.

**/
typedef
EFI_STATUS
(EFIAPI *EDKII_PEI_SET_VARIABLE)(
  IN CONST  EDKII_PEI_VARIABLE_PPI    *This,
  IN        CHAR16                    *VariableName,
  IN        EFI_GUID                  *VendorGuid,
  IN        UINT32                    Attributes,
  IN        UINTN                     DataSize,
  IN        VOID                      *Data
  );

/**
  Returns information about the UEFI variables.

  @param[in]        This                          A pointer to this instance of the EDKII_PEI_VARIABLE_PPI.
  @param[in]        Attributes                    Attributes bitmask to specify the type of variables on
                                                  which to return information.
  @param[out]       MaximumVariableStorageSize    On output the maximum size of the storage space
                                                  available for the EFI variables associated with the
                                                  attributes specified.
  @param[out]       RemainingVariableStorageSize  Returns the remaining size of the storage space
                                                  available for the EFI variables associated with the
                                                  attributes specified.
  @param[out]       MaximumVariableSize           Returns the maximum size of the individual EFI
                                                  variables associated with the attributes specified.

  @retval EFI_SUCCESS                             Valid answer returned.
  @retval EFI_INVALID_PARAMETER                   An invalid combination of attribute bits was supplied
  @retval EFI_UNSUPPORTED                         The attribute is not supported on this platform, and the
                                                  MaximumVariableStorageSize,
                                                  RemainingVariableStorageSize, MaximumVariableSize
                                                  are undefined.

**/
typedef
EFI_STATUS
(EFIAPI *EDKII_PEI_QUERY_VARIABLE_INFO)(
  IN CONST  EDKII_PEI_VARIABLE_PPI    *This,
  IN        UINT32                    Attributes,
  OUT       UINT64                    *MaximumVariableStorageSize,
  OUT       UINT64                    *RemainingVariableStorageSize,
  OUT       UINT64                    *MaximumVariableSize
  );

///
/// PEI Variable PPI is intended for use as a means
/// to store data in the PEI environment.
///
struct _EDKII_PEI_VARIABLE_PPI {
  EDKII_PEI_GET_VARIABLE              GetVariable;
  EDKII_PEI_GET_NEXT_VARIABLE_NAME    GetNextVariableName;
  EDKII_PEI_SET_VARIABLE              SetVariable;
  EDKII_PEI_QUERY_VARIABLE_INFO       QueryVariableInfo;
};

extern EFI_GUID  gEdkiiPeiVariablePpiGuid;

#endif
