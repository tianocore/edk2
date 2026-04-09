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
  Helper function to check if the PEI MM Communication PPI v3 is available,
  and initialize the v3 communicate buffer accordingly.

  @param   DataSize                 The data size to send to MM.
  @param   DataBuffer               The allocated communicate buffer to be used for communication.
  @param   BufferPageSize           The size of the allocated communicate buffer in pages.
  @param   MmCommunicationPpiV3Stub The located PEI MM Communication3 PPI stub.
  @param   MmVariableFunctionHeader The pointer to the variable function header in the communicate buffer.

  @retval EFI_INVALID_PARAMETER     The data size is too big. Or input pointers are NULL.
  @retval EFI_OUT_OF_RESOURCES      Failed to allocate memory.
  @retval EFI_NOT_FOUND             The PEI MM Communication3 PPI is not available.
  @retval EFI_SUCCESS               The communicate buffer is initialized successfully.

**/
static
EFI_STATUS
InternalInitCommBufferV3 (
  IN      UINTN                        DataSize,
  OUT     VOID                         **DataBuffer,
  OUT     UINTN                        *BufferPageSize,
  OUT EFI_PEI_MM_COMMUNICATION3_PPI    **MmCommunicationPpiV3Stub,
  OUT SMM_VARIABLE_COMMUNICATE_HEADER  **MmVariableFunctionHeader
  )
{
  EFI_STATUS                    Status;
  EFI_MM_COMMUNICATE_HEADER_V3  *MmCommunicateHeaderV3;
  UINT8                         *MmCommunicateBuffer;
  UINTN                         RequiredPages;

  // Should not be NULL
  if ((MmCommunicationPpiV3Stub == NULL) ||
      (DataBuffer == NULL) ||
      (BufferPageSize == NULL) ||
      (MmVariableFunctionHeader == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  *DataBuffer = NULL;

  Status = PeiServicesLocatePpi (&gEfiPeiMmCommunication3PpiGuid, 0, NULL, (VOID **)MmCommunicationPpiV3Stub);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "%a: Unable to locate PEI MM Communication3 PPI: %r\n", __func__, Status));
    return EFI_NOT_FOUND;
  }

  RequiredPages       = EFI_SIZE_TO_PAGES (DataSize + SMM_COMMUNICATE_HEADER_SIZE_V3 + SMM_VARIABLE_COMMUNICATE_HEADER_SIZE);
  MmCommunicateBuffer = (UINT8 *)AllocatePages (RequiredPages);
  if (MmCommunicateBuffer == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to allocate memory\n", __func__));
    return EFI_OUT_OF_RESOURCES;
  }

  ZeroMem (MmCommunicateBuffer, RequiredPages * EFI_PAGE_SIZE);
  MmCommunicateHeaderV3 = (EFI_MM_COMMUNICATE_HEADER_V3 *)MmCommunicateBuffer;
  CopyGuid (&MmCommunicateHeaderV3->HeaderGuid, &gEfiMmCommunicateHeaderV3Guid);
  MmCommunicateHeaderV3->BufferSize = RequiredPages * EFI_PAGE_SIZE;
  CopyGuid (&MmCommunicateHeaderV3->MessageGuid, &gEfiSmmVariableProtocolGuid);
  MmCommunicateHeaderV3->MessageSize = DataSize + SMM_VARIABLE_COMMUNICATE_HEADER_SIZE;
  *MmVariableFunctionHeader          = (SMM_VARIABLE_COMMUNICATE_HEADER *)MmCommunicateHeaderV3->MessageData;

  *DataBuffer     = MmCommunicateBuffer;
  *BufferPageSize = RequiredPages;

  return EFI_SUCCESS;
}

/**
  Helper function to check if the PEI MM Communication PPI v1 is available,
  and initialize the v1 communicate buffer accordingly.

  @param   DataSize                 The data size to send to MM.
  @param   DataBuffer               The allocated communicate buffer to be used for communication.
  @param   BufferPageSize           The size of the allocated communicate buffer in pages.
  @param   MmCommunicationPpiV1Stub The located PEI MM Communication1 PPI stub.
  @param   MmVariableFunctionHeader The pointer to the variable function header in the communicate buffer.

  @retval EFI_INVALID_PARAMETER     The data size is too big. Or input pointers are NULL.
  @retval EFI_OUT_OF_RESOURCES      Failed to allocate memory.
  @retval EFI_UNSUPPORTED           The PEI MM Communication1 PPI is not available.
  @retval EFI_SUCCESS               The communicate buffer is initialized successfully.

**/
static
EFI_STATUS
InternalInitCommBufferV1 (
  IN      UINTN                        DataSize,
  OUT     VOID                         **DataBuffer,
  OUT     UINTN                        *BufferPageSize,
  OUT EFI_PEI_MM_COMMUNICATION_PPI     **MmCommunicationPpiV1Stub,
  OUT SMM_VARIABLE_COMMUNICATE_HEADER  **MmVariableFunctionHeader
  )
{
  EFI_STATUS                 Status;
  EFI_MM_COMMUNICATE_HEADER  *MmCommunicateHeader;
  UINT8                      *MmCommunicateBuffer;
  UINTN                      RequiredPages;

  // Should not be NULL
  if ((MmCommunicationPpiV1Stub == NULL) ||
      (DataBuffer == NULL) ||
      (BufferPageSize == NULL) ||
      (MmVariableFunctionHeader == NULL))
  {
    return EFI_INVALID_PARAMETER;
  }

  *DataBuffer = NULL;

  Status = PeiServicesLocatePpi (&gEfiPeiMmCommunicationPpiGuid, 0, NULL, (VOID **)MmCommunicationPpiV1Stub);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_WARN, "%a: Unable to locate PEI MM Communication PPI: %r\n", __func__, Status));
    return EFI_UNSUPPORTED;
  }

  // Use v1 communication header, if v3 protocol is not available.
  RequiredPages       = EFI_SIZE_TO_PAGES (DataSize + SMM_COMMUNICATE_HEADER_SIZE + SMM_VARIABLE_COMMUNICATE_HEADER_SIZE);
  MmCommunicateBuffer = (UINT8 *)AllocatePages (RequiredPages);
  if (MmCommunicateBuffer == NULL) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to allocate memory\n", __func__));
    return EFI_OUT_OF_RESOURCES;
  }

  ZeroMem (MmCommunicateBuffer, RequiredPages * EFI_PAGE_SIZE);
  MmCommunicateHeader = (EFI_MM_COMMUNICATE_HEADER *)MmCommunicateBuffer;
  CopyGuid (&MmCommunicateHeader->HeaderGuid, &gEfiSmmVariableProtocolGuid);
  MmCommunicateHeader->MessageLength = DataSize + SMM_VARIABLE_COMMUNICATE_HEADER_SIZE;
  *MmVariableFunctionHeader          = (SMM_VARIABLE_COMMUNICATE_HEADER *)MmCommunicateHeader->Data;

  *DataBuffer     = MmCommunicateBuffer;
  *BufferPageSize = RequiredPages;

  return EFI_SUCCESS;
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
static
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
  SMM_VARIABLE_COMMUNICATE_HEADER  *MmVariableFunctionHeader;

  // Neither stub shall be NULL
  if ((MmCommunicationPpiStub == NULL) || (MmCommunicationPpiV3Stub == NULL)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: MmCommunicationPpiStub (0x%p) or MmCommunicationPpiV3Stub (0x%p) are NULL\n",
      __func__,
      MmCommunicationPpiStub,
      MmCommunicationPpiV3Stub
      ));
    return EFI_INVALID_PARAMETER;
  }

  if ((DataBuffer == NULL) || (BufferPageSize == NULL)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: DataBuffer (0x%p) or BufferPageSize (0x%p) are NULL\n",
      __func__,
      DataBuffer,
      BufferPageSize
      ));
    return EFI_INVALID_PARAMETER;
  }

  *DataBuffer               = NULL;
  *MmCommunicationPpiStub   = NULL;
  *MmCommunicationPpiV3Stub = NULL;

  // Attempt to locate PEI MM Communication3 PPI first
  Status = InternalInitCommBufferV3 (
             DataSize,
             DataBuffer,
             BufferPageSize,
             MmCommunicationPpiV3Stub,
             &MmVariableFunctionHeader
             );
  if (Status == EFI_NOT_FOUND) {
    // Try to locate PEI MM Communication1 PPI
    Status = InternalInitCommBufferV1 (
               DataSize,
               DataBuffer,
               BufferPageSize,
               MmCommunicationPpiStub,
               &MmVariableFunctionHeader
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "%a: InternalInitCommBufferV1 failed: %r\n", __func__, Status));
      return Status;
    }
  } else if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: InternalInitCommBufferV3 failed: %r\n", __func__, Status));
    return Status;
  }

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
  UINTN                                     IncomingDataSize;
  SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE  *MmVarAccessHeader;
  UINT8                                     *MmCommunicateBuffer;
  EFI_PEI_MM_COMMUNICATION_PPI              *MmCommunicationPpi;
  EFI_PEI_MM_COMMUNICATION3_PPI             *MmCommunicationPpiV3;

  MmCommunicateBuffer  = NULL;
  MmCommunicationPpi   = NULL;
  MmCommunicationPpiV3 = NULL;
  MmVarAccessHeader    = NULL;

  // Check input parameters
  if ((VariableName == NULL) || (VariableGuid == NULL) || (DataSize == NULL)) {
    DEBUG ((DEBUG_ERROR, "%a: Invalid VariableName (0x%p), VariableGuid (0x%p), or DataSize (0x%p)\n", __func__, VariableName, VariableGuid, DataSize));
    Status = EFI_INVALID_PARAMETER;
    goto Exit;
  }

  if (VariableName[0] == 0) {
    DEBUG ((DEBUG_ERROR, "%a: VariableName is empty string\n", __func__));
    Status = EFI_NOT_FOUND;
    goto Exit;
  }

  IncomingDataSize = *DataSize;

  if ((IncomingDataSize > 0) && (Data == NULL)) {
    DEBUG ((DEBUG_ERROR, "%a: DataSize is non-zero (%u) but Data is NULL\n", __func__, (UINT32)IncomingDataSize));
    Status = EFI_INVALID_PARAMETER;
    goto Exit;
  }

  // Allocate required pages to send MM request
  Status = SafeUintnAdd (
             OFFSET_OF (SMM_VARIABLE_COMMUNICATE_ACCESS_VARIABLE, Name),
             StrSize (VariableName),
             &MessageSize
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Size overflow calculating MessageSize part 1: %r\n", __func__, Status));
    Status = EFI_INVALID_PARAMETER;
    goto Exit;
  }

  Status = SafeUintnAdd (
             MessageSize,
             IncomingDataSize,
             &MessageSize
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Size overflow calculating MessageSize part 2: %r\n", __func__, Status));
    Status = EFI_INVALID_PARAMETER;
    goto Exit;
  }

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
  MmVarAccessHeader->DataSize = IncomingDataSize;

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
  if (IncomingDataSize < MmVarAccessHeader->DataSize) {
    Status = EFI_BUFFER_TOO_SMALL;
  }

Exit:
  if ((Status == EFI_SUCCESS) || (Status == EFI_BUFFER_TOO_SMALL)) {
    *DataSize = MmVarAccessHeader->DataSize;
  }

  if (Status == EFI_SUCCESS) {
    // Check if we need to set Attributes
    if (Attributes != NULL) {
      *Attributes = MmVarAccessHeader->Attributes;
    }

    CopyMem ((VOID *)Data, (UINT8 *)MmVarAccessHeader->Name + MmVarAccessHeader->NameSize, MmVarAccessHeader->DataSize);
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

  MmCommunicateBuffer   = NULL;
  MmCommunicationPpi    = NULL;
  MmCommunicationPpiV3  = NULL;
  MmVarGetNextVarHeader = NULL;

  // Check input parameters
  if ((VariableName == NULL) ||
      (VariableGuid == NULL) ||
      (VariableNameSize == NULL) ||
      (*VariableNameSize == 0))
  {
    DEBUG ((DEBUG_ERROR, "%a: Invalid VariableName (0x%p), VariableGuid (0x%p), or VariableNameSize (0x%p)\n", __func__, VariableName, VariableGuid, VariableNameSize));
    Status = EFI_INVALID_PARAMETER;
    goto Exit;
  }

  // Allocate required pages to send MM request
  Status = SafeUintnAdd (
             OFFSET_OF (SMM_VARIABLE_COMMUNICATE_GET_NEXT_VARIABLE_NAME, Name),
             StrSize (VariableName),
             &MessageSize
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Size overflow calculating MessageSize part 1: %r\n", __func__, Status));
    Status = EFI_INVALID_PARAMETER;
    goto Exit;
  }

  Status = SafeUintnAdd (
             MessageSize,
             *VariableNameSize,
             &MessageSize
             );
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Size overflow calculating MessageSize part 2: %r\n", __func__, Status));
    Status = EFI_INVALID_PARAMETER;
    goto Exit;
  }

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
    DEBUG ((DEBUG_ERROR, "%a: InitCommunicateBuffer failed: %r\n", __func__, Status));
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
  if ((Status == EFI_BUFFER_TOO_SMALL) || (Status == EFI_SUCCESS)) {
    // Update the name size to be returned
    *VariableNameSize = MmVarGetNextVarHeader->NameSize;
  }

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
