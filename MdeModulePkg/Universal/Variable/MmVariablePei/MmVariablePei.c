/** @file -- MmVariablePei.c
  Provides interface for reading Secure System Variables during PEI.

  Copyright (c) 2006 - 2019, Intel Corporation. All rights reserved.<BR>
  Copyright (c) Microsoft Corporation.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "MmVariablePei.h"

#define MM_VARIABLE_COMM_BUFFER_OFFSET  (SMM_COMMUNICATE_HEADER_SIZE + SMM_VARIABLE_COMMUNICATE_HEADER_SIZE)

//
// Module globals
//
EFI_PEI_READ_ONLY_VARIABLE2_PPI  mPeiSecureVariableRead = {
  PeiMmGetVariable,
  PeiMmGetNextVariableName
};

EFI_PEI_PPI_DESCRIPTOR  mPeiMmVariablePpi = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiPeiReadOnlyVariable2PpiGuid,
  &mPeiSecureVariableRead
};

/**
  Entry point of PEI Secure Variable read driver

  @param  FileHandle   Handle of the file being invoked.
                       Type EFI_PEI_FILE_HANDLE is defined in FfsFindNextFile().
  @param  PeiServices  General purpose services available to every PEIM.

  @retval EFI_SUCCESS  If the interface could be successfully installed
  @retval Others       Returned from PeiServicesInstallPpi()
**/
EFI_STATUS
EFIAPI
PeiMmVariableInitialize (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  return PeiServicesInstallPpi (&mPeiMmVariablePpi);
}

/**
  Helper function to populate MM communicate header and variable communicate header
  and then communicate to PEI.

  @param[in, out] CommunicateBuffer       Size of the variable name.
  @param[in]      CommunicateBufferSize   The entire buffer size to be sent to MM.
  @param[in]      Function                The MM variable function value.

  @retval EFI_INVALID_PARAMETER      Invalid parameter.
  @retval EFI_SUCCESS                Find the specified variable.
  @retval Others                     Errors returned by MM communicate or variable service.

**/
EFI_STATUS
PopulateHeaderAndCommunicate (
  IN OUT  UINT8  *CommunicateBuffer,
  IN UINTN       CommunicateBufferSize,
  IN UINTN       Function
  )
{
  EFI_STATUS                       Status;
  EFI_PEI_MM_COMMUNICATION_PPI     *MmCommunicationPpi;
  EFI_MM_COMMUNICATE_HEADER        *MmCommunicateHeader;
  SMM_VARIABLE_COMMUNICATE_HEADER  *MmVarCommsHeader;

  // Minimal sanity check
  if ((CommunicateBuffer == NULL) ||
      (CommunicateBufferSize < MM_VARIABLE_COMM_BUFFER_OFFSET))
  {
    Status = EFI_INVALID_PARAMETER;
    DEBUG ((DEBUG_ERROR, "%a: Invalid incoming parameters: %p and 0x%x\n", __func__, CommunicateBuffer, CommunicateBufferSize));
    goto Exit;
  }

  if ((Function != SMM_VARIABLE_FUNCTION_GET_NEXT_VARIABLE_NAME) &&
      (Function != SMM_VARIABLE_FUNCTION_GET_VARIABLE))
  {
    Status = EFI_INVALID_PARAMETER;
    DEBUG ((DEBUG_ERROR, "%a: Invalid function value: 0x%x\n", __func__, Function));
    goto Exit;
  }

  Status = PeiServicesLocatePpi (&gEfiPeiMmCommunicationPpiGuid, 0, NULL, (VOID **)&MmCommunicationPpi);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to locate PEI MM Communication PPI: %r\n", __func__, Status));
    goto Exit;
  }

  // Zero the entire Communication Buffer Header
  MmCommunicateHeader = (EFI_MM_COMMUNICATE_HEADER *)CommunicateBuffer;

  ZeroMem (MmCommunicateHeader, SMM_COMMUNICATE_HEADER_SIZE);

  // Use gEfiSmmVariableProtocolGuid to request the MM variable service in Standalone MM
  CopyMem ((VOID *)&MmCommunicateHeader->HeaderGuid, &gEfiSmmVariableProtocolGuid, sizeof (GUID));

  // Program the MM header size
  MmCommunicateHeader->MessageLength = CommunicateBufferSize - SMM_COMMUNICATE_HEADER_SIZE;

  MmVarCommsHeader = (SMM_VARIABLE_COMMUNICATE_HEADER *)(CommunicateBuffer + SMM_COMMUNICATE_HEADER_SIZE);

  // We are only supporting GetVariable and GetNextVariableName
  MmVarCommsHeader->Function = Function;

  // Send the MM request using MmCommunicationPei
  Status = MmCommunicationPpi->Communicate (MmCommunicationPpi, CommunicateBuffer, &CommunicateBufferSize);
  if (EFI_ERROR (Status)) {
    // Received an error from MM interface.
    DEBUG ((DEBUG_ERROR, "%a - MM Interface Error: %r\n", __func__, Status));
    goto Exit;
  }

  // MM request was successfully handled by the framework.
  // Set status to the Variable Service Status Code
  Status = MmVarCommsHeader->ReturnStatus;
  if (EFI_ERROR (Status)) {
    // We received an error from Variable Service.
    // We cant do anymore so return Status
    if (Status != EFI_BUFFER_TOO_SMALL) {
      DEBUG ((DEBUG_ERROR, "%a - Variable Service Error: %r\n", __func__, Status));
    }

    goto Exit;
  }

Exit:
  return Status;
}

/**
  This service retrieves a variable's value using its name and GUID.

  This function is using the Secure Variable Store. If the Data
  buffer is too small to hold the contents of the variable, the error
  EFI_BUFFER_TOO_SMALL is returned and DataSize is set to the required buffer
  size to obtain the data.

  @param  This                  A pointer to this instance of the EFI_PEI_READ_ONLY_VARIABLE2_PPI.
  @param  VariableName          A pointer to a null-terminated string that is the variable's name.
  @param  VariableGuid          A pointer to an EFI_GUID that is the variable's GUID. The combination of
                                VariableGuid and VariableName must be unique.
  @param  Attributes            If non-NULL, on return, points to the variable's attributes.
  @param  DataSize              On entry, points to the size in bytes of the Data buffer.
                                On return, points to the size of the data returned in Data.
  @param  Data                  Points to the buffer which will hold the returned variable value.
                                May be NULL with a zero DataSize in order to determine the size of the buffer needed.

  @retval EFI_SUCCESS           The variable was read successfully.
  @retval EFI_NOT_FOUND         The variable was not found.
  @retval EFI_BUFFER_TOO_SMALL  The DataSize is too small for the resulting data.
                                DataSize is updated with the size required for
                                the specified variable.
  @retval EFI_INVALID_PARAMETER VariableName, VariableGuid, DataSize or Data is NULL.
  @retval EFI_DEVICE_ERROR      The variable could not be retrieved because of a device error.

**/
EFI_STATUS
EFIAPI
PeiMmGetVariable (
  IN CONST  EFI_PEI_READ_ONLY_VARIABLE2_PPI *This,
  IN CONST  CHAR16 *VariableName,
  IN CONST  EFI_GUID *VariableGuid,
  OUT       UINT32 *Attributes, OPTIONAL
  IN OUT    UINTN                            *DataSize,
  OUT       VOID                             *Data OPTIONAL
  )
{
  EFI_STATUS                                Status;
  UINTN                                     MessageSize;
  SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE  *MmVarAccessHeader;
  UINT8                                     *MmCommunicateBuffer;
  UINTN                                     RequiredPages;

  // Check input parameters
  if ((VariableName == NULL) || (VariableGuid == NULL) || (DataSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (VariableName[0] == 0) {
    return EFI_NOT_FOUND;
  }

  if ((*DataSize > 0) && (Data == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  // Allocate required pages to send MM request
  MessageSize = MM_VARIABLE_COMM_BUFFER_OFFSET +
                OFFSET_OF (SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE, Name) +
                StrSize (VariableName) + *DataSize;

  RequiredPages       = EFI_SIZE_TO_PAGES (MessageSize);
  MmCommunicateBuffer = (UINT8 *)AllocatePages (RequiredPages);

  if (MmCommunicateBuffer == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    DEBUG ((DEBUG_ERROR, "%a: Failed to allocate memory: %r\n", __func__, Status));
    return Status;
  }

  // Zero the entire Communication Buffer
  ZeroMem (MmCommunicateBuffer, (RequiredPages * EFI_PAGE_SIZE));

  //
  // Program all payload structure contents
  //
  MmVarAccessHeader = (SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE *)(MmCommunicateBuffer + MM_VARIABLE_COMM_BUFFER_OFFSET);

  // Variable GUID
  CopyMem ((VOID *)&MmVarAccessHeader->Guid, VariableGuid, sizeof (GUID));

  // Program the max amount of data we accept.
  MmVarAccessHeader->DataSize = *DataSize;

  // Get size of the variable name
  MmVarAccessHeader->NameSize = StrSize (VariableName);

  // Populate incoming variable name
  CopyMem ((VOID *)&MmVarAccessHeader->Name, VariableName, MmVarAccessHeader->NameSize);

  Status = PopulateHeaderAndCommunicate (MmCommunicateBuffer, MessageSize, SMM_VARIABLE_FUNCTION_GET_VARIABLE);
  if (EFI_ERROR (Status)) {
    // We received an error from either communicate or Variable Service.
    if (Status != EFI_BUFFER_TOO_SMALL) {
      DEBUG ((DEBUG_ERROR, "%a - Communite to MM for variable service errored: %r\n", __func__, Status));
    }

    goto Exit;
  }

  Status = EFI_SUCCESS;

  // User provided buffer is too small
  if (*DataSize < MmVarAccessHeader->DataSize) {
    Status = EFI_BUFFER_TOO_SMALL;
  }

Exit:
  // Check if we need to set Attributes
  if (Attributes != NULL) {
    *Attributes = MmVarAccessHeader->Attributes;
  }

  *DataSize = MmVarAccessHeader->DataSize;

  if (Status == EFI_SUCCESS) {
    CopyMem ((VOID *)Data, (UINT8 *)MmVarAccessHeader->Name + MmVarAccessHeader->NameSize, *DataSize);
  }

  // Free the Communication Buffer
  if (MmCommunicateBuffer != NULL) {
    FreePages (MmCommunicateBuffer, RequiredPages);
  }

  return Status;
}

/**
  Return the next variable name and GUID.

  This function is called multiple times to retrieve the VariableName
  and VariableGuid of all variables currently available in the system.
  On each call, the previous results are passed into the interface,
  and, on return, the interface returns the data for the next
  interface. When the entire variable list has been returned,
  EFI_NOT_FOUND is returned.

  @param  This              A pointer to this instance of the EFI_PEI_READ_ONLY_VARIABLE2_PPI.

  @param  VariableNameSize  On entry, points to the size of the buffer pointed to by VariableName.
                            On return, the size of the variable name buffer.
  @param  VariableName      On entry, a pointer to a null-terminated string that is the variable's name.
                            On return, points to the next variable's null-terminated name string.

  @param  VariableGuid      On entry, a pointer to an EFI_GUID that is the variable's GUID.
                            On return, a pointer to the next variable's GUID.

  @retval EFI_SUCCESS           The variable was read successfully.
  @retval EFI_NOT_FOUND         The variable could not be found.
  @retval EFI_BUFFER_TOO_SMALL  The VariableNameSize is too small for the resulting
                                data. VariableNameSize is updated with the size
                                required for the specified variable.
  @retval EFI_INVALID_PARAMETER VariableName, VariableGuid or
                                VariableNameSize is NULL.
  @retval EFI_DEVICE_ERROR      The variable could not be retrieved because of a device error.
**/
EFI_STATUS
EFIAPI
PeiMmGetNextVariableName (
  IN CONST  EFI_PEI_READ_ONLY_VARIABLE2_PPI  *This,
  IN OUT UINTN                               *VariableNameSize,
  IN OUT CHAR16                              *VariableName,
  IN OUT EFI_GUID                            *VariableGuid
  )
{
  EFI_STATUS                                       Status;
  UINTN                                            MessageSize;
  UINT8                                            *MmCommunicateBuffer;
  SMM_VARIABLE_COMMUNICATE_GET_NEXT_VARIABLE_NAME  *MmVarGetNextVarHeader;
  UINTN                                            RequiredPages;

  // Check input parameters
  if ((VariableName == NULL) ||
      (VariableGuid == NULL) ||
      (VariableNameSize == NULL) ||
      (*VariableNameSize == 0))
  {
    return EFI_INVALID_PARAMETER;
  }

  // Allocate required pages to send MM request
  MessageSize = MM_VARIABLE_COMM_BUFFER_OFFSET +
                OFFSET_OF (SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE, Name) +
                StrSize (VariableName) + *VariableNameSize;

  RequiredPages       = EFI_SIZE_TO_PAGES (MessageSize);
  MmCommunicateBuffer = (UINT8 *)AllocatePages (RequiredPages);

  if (MmCommunicateBuffer == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    DEBUG ((DEBUG_ERROR, "%a: Failed to allocate memory: %r\n", __func__, Status));
    return Status;
  }

  // Zero the entire Communication Buffer
  ZeroMem (MmCommunicateBuffer, (RequiredPages * EFI_PAGE_SIZE));

  //
  // Program all payload structure contents
  //
  MmVarGetNextVarHeader = (SMM_VARIABLE_COMMUNICATE_GET_NEXT_VARIABLE_NAME *)(MmCommunicateBuffer + MM_VARIABLE_COMM_BUFFER_OFFSET);

  // Variable GUID
  CopyMem ((VOID *)&MmVarGetNextVarHeader->Guid, VariableGuid, sizeof (GUID));

  // Program the maximal length of name we can accept.
  MmVarGetNextVarHeader->NameSize = *VariableNameSize;

  // Populate incoming variable name
  CopyMem ((VOID *)&MmVarGetNextVarHeader->Name, VariableName, MmVarGetNextVarHeader->NameSize);

  // Send the MM request using MmCommunicationPei
  Status = PopulateHeaderAndCommunicate (MmCommunicateBuffer, MessageSize, SMM_VARIABLE_FUNCTION_GET_NEXT_VARIABLE_NAME);
  if (EFI_ERROR (Status)) {
    // We received an error from either communicate or Variable Service.
    if (Status != EFI_BUFFER_TOO_SMALL) {
      DEBUG ((DEBUG_ERROR, "%a - Communite to MM for variable service errored: %r\n", __func__, Status));
    }

    goto Exit;
  }

  Status = EFI_SUCCESS;

  // User provided buffer is too small
  if (*VariableNameSize < MmVarGetNextVarHeader->NameSize) {
    Status = EFI_BUFFER_TOO_SMALL;
  }

Exit:
  // Update the name size to be returned
  *VariableNameSize = MmVarGetNextVarHeader->NameSize;

  if (Status == EFI_SUCCESS) {
    CopyMem ((VOID *)VariableName, (UINT8 *)MmVarGetNextVarHeader->Name, *VariableNameSize);
    CopyMem ((VOID *)VariableGuid, (UINT8 *)&(MmVarGetNextVarHeader->Guid), sizeof (EFI_GUID));
  }

  // Free the Communication Buffer
  if (MmCommunicateBuffer != NULL) {
    FreePages (MmCommunicateBuffer, RequiredPages);
  }

  return Status;
}
