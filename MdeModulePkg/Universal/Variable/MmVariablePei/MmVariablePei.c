/** @file -- MmVariablePei.c
  Provides interface for reading Secure System Variables during PEI.

  Copyright (c) 2006 - 2019, Intel Corporation. All rights reserved.<BR>
  Copyright (c) Microsoft Corporation.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "MmVariablePei.h"

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
  Initialize the communicate buffer using DataSize and Function.

  This function will locate the PEI MM Communication PPI v3 or v1, and populate
  the corresponding header data and return the located PPI stub.

  The communicate size is: SMM_COMMUNICATE_HEADER_SIZE(_V3) + SMM_VARIABLE_COMMUNICATE_HEADER_SIZE +
  DataSize.

  Caution: This function may receive untrusted input.
  The data size external input, so this function will validate it carefully to avoid buffer overflow.

  @param[out]      DataPtr                  Points to the data in the communicate buffer.
  @param[in]       DataSize                 The data size to send to MM.
  @param[in]       Function                 The function number to initialize the communicate header.
  @param[out]      DataBuffer               The allocated communicate buffer to be used for communication.
  @param[out]      BufferPageSize           The size of the allocated communicate buffer in pages.
  @param[out]      MmCommunicationPpiStub   The located PEI MM Communication PPI stub.
  @param[out]      MmCommunicationPpiV3Stub The located PEI MM Communication3 PPI stub.

  @retval EFI_INVALID_PARAMETER     The data size is too big.
  @retval EFI_SUCCESS               Find the specified variable.

**/
EFI_STATUS
InitCommunicateBuffer (
  OUT     VOID                       **DataPtr OPTIONAL,
  IN      UINTN                      DataSize,
  IN      UINTN                      Function,
  OUT     VOID                       **DataBuffer,
  OUT     UINTN                      *BufferPageSize,
  OUT EFI_PEI_MM_COMMUNICATION_PPI   **MmCommunicationPpiStub OPTIONAL,
  OUT EFI_PEI_MM_COMMUNICATION3_PPI  **MmCommunicationPpiV3Stub OPTIONAL
  )
{
  EFI_STATUS                       Status;
  EFI_PEI_MM_COMMUNICATION_PPI     *MmCommunicationPpi;
  EFI_PEI_MM_COMMUNICATION3_PPI    *MmCommunicationPpiV3;
  EFI_MM_COMMUNICATE_HEADER        *MmCommunicateHeader;
  EFI_MM_COMMUNICATE_HEADER_V3     *MmCommunicateHeaderV3;
  SMM_VARIABLE_COMMUNICATE_HEADER  *MmVariableFunctionHeader;
  UINT8                            *MmCommunicateBuffer;
  UINTN                            RequiredPages;

  MmCommunicationPpiV3 = NULL;
  MmCommunicationPpi   = NULL;

  // Should not both stubs be NULL
  if ((MmCommunicationPpiStub == NULL) && (MmCommunicationPpiV3Stub == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (DataBuffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (MmCommunicationPpiStub != NULL) {
    *MmCommunicationPpiStub = NULL;
  }

  if (MmCommunicationPpiV3Stub != NULL) {
    *MmCommunicationPpiV3Stub = NULL;
  }

  Status = PeiServicesLocatePpi (&gEfiPeiMmCommunication3PpiGuid, 0, NULL, (VOID **)&MmCommunicationPpiV3);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "%a: Unable to locate PEI MM Communication3 PPI: %r\n", __func__, Status));
    // Try to locate the older version of the PPI
    Status = PeiServicesLocatePpi (&gEfiPeiMmCommunicationPpiGuid, 0, NULL, (VOID **)&MmCommunicationPpi);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: Failed to locate PEI MM Communication PPI: %r\n", __func__, Status));
      return Status;
    }
  }

  if (MmCommunicationPpiV3 != NULL) {
    RequiredPages       = EFI_SIZE_TO_PAGES (DataSize + SMM_COMMUNICATE_HEADER_SIZE_V3 + SMM_VARIABLE_COMMUNICATE_HEADER_SIZE);
    MmCommunicateBuffer = (UINT8 *)AllocatePages (RequiredPages);
    if (MmCommunicateBuffer == NULL) {
      DEBUG ((DEBUG_ERROR, "%a: Failed to allocate memory: %r\n", __func__, Status));
      return EFI_OUT_OF_RESOURCES;
    }

    ZeroMem (MmCommunicateBuffer, RequiredPages * EFI_PAGE_SIZE);
    MmCommunicateHeaderV3 = (EFI_MM_COMMUNICATE_HEADER_V3 *)MmCommunicateBuffer;
    CopyGuid (&MmCommunicateHeaderV3->HeaderGuid, &gEfiMmCommunicateHeaderV3Guid);
    MmCommunicateHeaderV3->BufferSize = RequiredPages * EFI_PAGE_SIZE;
    CopyGuid (&MmCommunicateHeaderV3->MessageGuid, &gEfiSmmVariableProtocolGuid);
    MmCommunicateHeaderV3->MessageSize = DataSize + SMM_VARIABLE_COMMUNICATE_HEADER_SIZE;
    MmVariableFunctionHeader           = (SMM_VARIABLE_COMMUNICATE_HEADER *)MmCommunicateHeaderV3->MessageData;
    if (MmCommunicationPpiV3Stub != NULL) {
      *MmCommunicationPpiV3Stub = MmCommunicationPpiV3;
    } else {
      DEBUG ((DEBUG_ERROR, "%a: System supports MM Communication v3 PPI, but caller does not want it...\n", __func__));
      ASSERT (FALSE);
      FreePages (MmCommunicateBuffer, RequiredPages);
      return EFI_UNSUPPORTED;
    }
  } else {
    // Use v1 communication header, if v3 protocol is not available.
    RequiredPages       = EFI_SIZE_TO_PAGES (DataSize + SMM_COMMUNICATE_HEADER_SIZE + SMM_VARIABLE_COMMUNICATE_HEADER_SIZE);
    MmCommunicateBuffer = (UINT8 *)AllocatePages (RequiredPages);
    if (MmCommunicateBuffer == NULL) {
      DEBUG ((DEBUG_ERROR, "%a: Failed to allocate memory: %r\n", __func__, Status));
      return EFI_OUT_OF_RESOURCES;
    }

    ZeroMem (MmCommunicateBuffer, RequiredPages * EFI_PAGE_SIZE);
    MmCommunicateHeader = (EFI_MM_COMMUNICATE_HEADER *)MmCommunicateBuffer;
    CopyGuid (&MmCommunicateHeader->HeaderGuid, &gEfiSmmVariableProtocolGuid);
    MmCommunicateHeader->MessageLength = DataSize + SMM_VARIABLE_COMMUNICATE_HEADER_SIZE;
    MmVariableFunctionHeader           = (SMM_VARIABLE_COMMUNICATE_HEADER *)MmCommunicateHeader->Data;
    if (MmCommunicationPpiStub != NULL) {
      *MmCommunicationPpiStub = MmCommunicationPpi;
    } else {
      DEBUG ((DEBUG_ERROR, "%a: System supports only MM Communication v1 PPI, but caller does not want it...\n", __func__));
      ASSERT (FALSE);
      FreePages (MmCommunicateBuffer, RequiredPages);
      return EFI_UNSUPPORTED;
    }
  }

  *DataBuffer     = MmCommunicateBuffer;
  *BufferPageSize = RequiredPages;

  MmVariableFunctionHeader->Function = Function;
  if (DataPtr != NULL) {
    *DataPtr = MmVariableFunctionHeader->Data;
  }

  return EFI_SUCCESS;
}

/**
  This function will use the provided PEI MM Communication PPI v3 or v1 to send
  the communicate buffer to MM.

  @param[in]   MmCommunicationPpi     The PEI MM Communication PPI stub.
  @param[in]   MmCommunicationPpiV3   The PEI MM Communication3 PPI stub.
  @param[in]   DataBuffer             The communicate buffer to send to MM.
  @param[in]   DataSize               This size of the function header and the data.

  @retval      EFI_SUCCESS            Success is returned from the function in MM.
  @retval      Others                 Failure is returned from the function in MM.

**/
EFI_STATUS
SendCommunicateBuffer (
  IN EFI_PEI_MM_COMMUNICATION_PPI   *MmCommunicationPpi,
  IN EFI_PEI_MM_COMMUNICATION3_PPI  *MmCommunicationPpiV3,
  IN VOID                           *DataBuffer,
  IN      UINTN                     DataSize
  )
{
  EFI_STATUS                       Status;
  UINTN                            CommSize;
  EFI_MM_COMMUNICATE_HEADER        *MmCommunicateHeader;
  EFI_MM_COMMUNICATE_HEADER_V3     *MmCommunicateHeaderV3;
  SMM_VARIABLE_COMMUNICATE_HEADER  *MmVariableFunctionHeader;

  if ((MmCommunicationPpi == NULL) && (MmCommunicationPpiV3 == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (MmCommunicationPpiV3 != NULL) {
    Status = MmCommunicationPpiV3->Communicate (
                                     MmCommunicationPpiV3,
                                     DataBuffer
                                     );
    ASSERT_EFI_ERROR (Status);

    MmCommunicateHeaderV3    = (EFI_MM_COMMUNICATE_HEADER_V3 *)DataBuffer;
    MmVariableFunctionHeader = (SMM_VARIABLE_COMMUNICATE_HEADER *)MmCommunicateHeaderV3->MessageData;

    Status = MmVariableFunctionHeader->ReturnStatus;
  } else {
    CommSize = DataSize + SMM_COMMUNICATE_HEADER_SIZE + SMM_VARIABLE_COMMUNICATE_HEADER_SIZE;
    Status   = MmCommunicationPpi->Communicate (
                                     MmCommunicationPpi,
                                     DataBuffer,
                                     &CommSize
                                     );
    ASSERT_EFI_ERROR (Status);

    MmCommunicateHeader      = (EFI_MM_COMMUNICATE_HEADER *)DataBuffer;
    MmVariableFunctionHeader = (SMM_VARIABLE_COMMUNICATE_HEADER *)MmCommunicateHeader->Data;

    Status = MmVariableFunctionHeader->ReturnStatus;
  }

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
  UINTN                                     RequiredPages;
  SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE  *MmVarAccessHeader;
  UINT8                                     *MmCommunicateBuffer;
  EFI_PEI_MM_COMMUNICATION_PPI              *MmCommunicationPpi;
  EFI_PEI_MM_COMMUNICATION3_PPI             *MmCommunicationPpiV3;

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
  MessageSize = OFFSET_OF (SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE, Name) +
                StrSize (VariableName) + *DataSize;

  Status = InitCommunicateBuffer (
             (VOID **)&MmVarAccessHeader,
             MessageSize,
             SMM_VARIABLE_FUNCTION_GET_VARIABLE,
             (VOID **)&MmCommunicateBuffer,
             &RequiredPages,
             &MmCommunicationPpi,
             &MmCommunicationPpiV3
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: InitCommunicateBuffer failed: %r\n", __func__, Status));
    goto Exit;
  }

  // Variable GUID
  CopyMem ((VOID *)&MmVarAccessHeader->Guid, VariableGuid, sizeof (GUID));

  // Program the max amount of data we accept.
  MmVarAccessHeader->DataSize = *DataSize;

  // Get size of the variable name
  MmVarAccessHeader->NameSize = StrSize (VariableName);

  // Populate incoming variable name
  CopyMem ((VOID *)&MmVarAccessHeader->Name, VariableName, MmVarAccessHeader->NameSize);

  Status = SendCommunicateBuffer (MmCommunicationPpi, MmCommunicationPpiV3, MmCommunicateBuffer, MessageSize);
  if (EFI_ERROR (Status)) {
    // We received an error from either communicate or Variable Service.
    if (Status != EFI_BUFFER_TOO_SMALL) {
      DEBUG ((DEBUG_ERROR, "%a - Communicate to MM for getting variable errored: %r\n", __func__, Status));
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
  UINTN                                            RequiredPages;
  UINT8                                            *MmCommunicateBuffer;
  SMM_VARIABLE_COMMUNICATE_GET_NEXT_VARIABLE_NAME  *MmVarGetNextVarHeader;
  EFI_PEI_MM_COMMUNICATION_PPI                     *MmCommunicationPpi;
  EFI_PEI_MM_COMMUNICATION3_PPI                    *MmCommunicationPpiV3;

  // Check input parameters
  if ((VariableName == NULL) ||
      (VariableGuid == NULL) ||
      (VariableNameSize == NULL) ||
      (*VariableNameSize == 0))
  {
    return EFI_INVALID_PARAMETER;
  }

  // Allocate required pages to send MM request
  MessageSize = OFFSET_OF (SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE, Name) +
                StrSize (VariableName) + *VariableNameSize;

  Status = InitCommunicateBuffer (
             (VOID **)&MmVarGetNextVarHeader,
             MessageSize,
             SMM_VARIABLE_FUNCTION_GET_NEXT_VARIABLE_NAME,
             (VOID **)&MmCommunicateBuffer,
             &RequiredPages,
             &MmCommunicationPpi,
             &MmCommunicationPpiV3
             );
  if (EFI_ERROR (Status)) {
    goto Exit;
  }

  ASSERT (MmVarGetNextVarHeader != NULL);

  // Variable GUID
  CopyMem ((VOID *)&MmVarGetNextVarHeader->Guid, VariableGuid, sizeof (GUID));

  // Program the maximal length of name we can accept.
  MmVarGetNextVarHeader->NameSize = *VariableNameSize;

  // Populate incoming variable name
  CopyMem ((VOID *)&MmVarGetNextVarHeader->Name, VariableName, MmVarGetNextVarHeader->NameSize);

  // Send the MM request using MmCommunicationPei
  Status = SendCommunicateBuffer (MmCommunicationPpi, MmCommunicationPpiV3, MmCommunicateBuffer, MessageSize);
  if (EFI_ERROR (Status)) {
    // We received an error from either communicate or Variable Service.
    if (Status != EFI_BUFFER_TOO_SMALL) {
      DEBUG ((DEBUG_ERROR, "%a - Communicate to MM for next variable name errored: %r\n", __func__, Status));
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
