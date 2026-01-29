/** @file
  Serialize Variables Library implementation

  Copyright (c) 2004 - 2011, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SerializeVariablesLib.h"

/**
  Serialization format:

  The SerializeVariablesLib interface does not specify a format
  for the serialization of the variable data.  This library uses
  a packed array of a non-uniformly sized data structure elements.

  Each variable is stored (packed) as:
    UINT32   VendorNameSize;  // Name size in bytes
    CHAR16   VendorName[?];   // The variable unicode name including the
                              // null terminating character.
    EFI_GUID VendorGuid;      // The variable GUID
    UINT32   DataSize;        // The size of variable data in bytes
    UINT8    Data[?];         // The variable data

**/

/**
  Unpacks the next variable from the buffer

  @param[in]  Buffer - Buffer pointing to the next variable instance
                On subsequent calls, the pointer should be incremented
                by the returned SizeUsed value.
  @param[in]  MaxSize - Max allowable size for the variable data
                On subsequent calls, this should be decremented
                by the returned SizeUsed value.
  @param[out] Name - Variable name string (address in Buffer)
  @param[out] NameSize - Size of Name in bytes
  @param[out] Guid - GUID of variable (address in Buffer)
  @param[out] Attributes - Attributes of variable
  @param[out] Data - Buffer containing Data for variable (address in Buffer)
  @param[out] DataSize - Size of Data in bytes
  @param[out] SizeUsed - Total size used for this variable instance in Buffer

  @return     EFI_STATUS based on the success or failure of the operation

**/
STATIC
EFI_STATUS
UnpackVariableFromBuffer (
  IN  VOID      *Buffer,
  IN  UINTN     MaxSize,
  OUT CHAR16    **Name,
  OUT UINT32    *NameSize,
  OUT EFI_GUID  **Guid,
  OUT UINT32    *Attributes,
  OUT UINT32    *DataSize,
  OUT VOID      **Data,
  OUT UINTN     *SizeUsed
  )
{
  UINT8  *BytePtr;
  UINTN  Offset;

  BytePtr = (UINT8 *)Buffer;
  Offset  = 0;

  *NameSize = *(UINT32 *)(BytePtr + Offset);
  Offset    = Offset + sizeof (UINT32);

  if (Offset > MaxSize) {
    return EFI_INVALID_PARAMETER;
  }

  *Name  = (CHAR16 *)(BytePtr + Offset);
  Offset = Offset + *(UINT32 *)BytePtr;
  if (Offset > MaxSize) {
    return EFI_INVALID_PARAMETER;
  }

  *Guid  = (EFI_GUID *)(BytePtr + Offset);
  Offset = Offset + sizeof (EFI_GUID);
  if (Offset > MaxSize) {
    return EFI_INVALID_PARAMETER;
  }

  *Attributes = *(UINT32 *)(BytePtr + Offset);
  Offset      = Offset + sizeof (UINT32);
  if (Offset > MaxSize) {
    return EFI_INVALID_PARAMETER;
  }

  *DataSize = *(UINT32 *)(BytePtr + Offset);
  Offset    = Offset + sizeof (UINT32);
  if (Offset > MaxSize) {
    return EFI_INVALID_PARAMETER;
  }

  *Data  = (VOID *)(BytePtr + Offset);
  Offset = Offset + *DataSize;
  if (Offset > MaxSize) {
    return EFI_INVALID_PARAMETER;
  }

  *SizeUsed = Offset;

  return EFI_SUCCESS;
}

/**
  Iterates through the variables in the buffer, and calls a callback
  function for each variable found.

  @param[in]  CallbackFunction - Function called for each variable instance
  @param[in]  Context - Passed to each call of CallbackFunction
  @param[in]  Buffer - Buffer containing serialized variables
  @param[in]  MaxSize - Size of Buffer in bytes

  @return     EFI_STATUS based on the success or failure of the operation

**/
STATIC
EFI_STATUS
IterateVariablesInBuffer (
  IN VARIABLE_SERIALIZATION_ITERATION_CALLBACK  CallbackFunction,
  IN VOID                                       *CallbackContext,
  IN VOID                                       *Buffer,
  IN UINTN                                      MaxSize
  )
{
  RETURN_STATUS  Status;
  UINTN          TotalSizeUsed;
  UINTN          SizeUsed;

  CHAR16    *Name;
  UINT32    NameSize;
  CHAR16    *AlignedName;
  UINT32    AlignedNameMaxSize;
  EFI_GUID  *Guid;
  UINT32    Attributes;
  UINT32    DataSize;
  VOID      *Data;

  SizeUsed           = 0;
  AlignedName        = NULL;
  AlignedNameMaxSize = 0;
  Name               = NULL;
  Guid               = NULL;
  Attributes         = 0;
  DataSize           = 0;
  Data               = NULL;

  for (
       Status = EFI_SUCCESS, TotalSizeUsed = 0;
       !EFI_ERROR (Status) && (TotalSizeUsed < MaxSize);
       )
  {
    Status = UnpackVariableFromBuffer (
               (VOID *)((UINT8 *)Buffer + TotalSizeUsed),
               (MaxSize - TotalSizeUsed),
               &Name,
               &NameSize,
               &Guid,
               &Attributes,
               &DataSize,
               &Data,
               &SizeUsed
               );
    if (EFI_ERROR (Status)) {
      return Status;
    }

    //
    // We copy the name to a separately allocated buffer,
    // to be sure it is 16-bit aligned.
    //
    if (NameSize > AlignedNameMaxSize) {
      if (AlignedName != NULL) {
        FreePool (AlignedName);
      }

      AlignedName = AllocatePool (NameSize);
    }

    if (AlignedName == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    CopyMem (AlignedName, Name, NameSize);

    TotalSizeUsed = TotalSizeUsed + SizeUsed;

    //
    // Run the callback function
    //
    Status = (*CallbackFunction)(
  CallbackContext,
  AlignedName,
  Guid,
  Attributes,
  DataSize,
  Data
  );
  }

  if (AlignedName != NULL) {
    FreePool (AlignedName);
  }

  //
  // Make sure the entire buffer was used, or else return an error
  //
  if (TotalSizeUsed != MaxSize) {
    DEBUG ((
      DEBUG_ERROR,
      "Deserialize variables error: TotalSizeUsed(%Lu) != MaxSize(%Lu)\n",
      (UINT64)TotalSizeUsed,
      (UINT64)MaxSize
      ));
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

STATIC
RETURN_STATUS
EFIAPI
IterateVariablesCallbackNop (
  IN  VOID      *Context,
  IN  CHAR16    *VariableName,
  IN  EFI_GUID  *VendorGuid,
  IN  UINT32    Attributes,
  IN  UINTN     DataSize,
  IN  VOID      *Data
  )
{
  return RETURN_SUCCESS;
}

STATIC
RETURN_STATUS
EFIAPI
IterateVariablesCallbackSetInInstance (
  IN  VOID      *Context,
  IN  CHAR16    *VariableName,
  IN  EFI_GUID  *VendorGuid,
  IN  UINT32    Attributes,
  IN  UINTN     DataSize,
  IN  VOID      *Data
  )
{
  EFI_HANDLE  Instance;

  Instance = (EFI_HANDLE)Context;

  return SerializeVariablesAddVariable (
           Instance,
           VariableName,
           VendorGuid,
           Attributes,
           DataSize,
           Data
           );
}

STATIC
RETURN_STATUS
EFIAPI
IterateVariablesCallbackSetSystemVariable (
  IN  VOID      *Context,
  IN  CHAR16    *VariableName,
  IN  EFI_GUID  *VendorGuid,
  IN  UINT32    Attributes,
  IN  UINTN     DataSize,
  IN  VOID      *Data
  )
{
  EFI_STATUS           Status;
  STATIC CONST UINT32  AuthMask =
    EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS |
    EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS;

  Status = gRT->SetVariable (
                  VariableName,
                  VendorGuid,
                  Attributes,
                  DataSize,
                  Data
                  );

  if ((Status == EFI_SECURITY_VIOLATION) && ((Attributes & AuthMask) != 0)) {
    DEBUG ((
      DEBUG_WARN,
      "%a: setting authenticated variable \"%s\" "
      "failed with EFI_SECURITY_VIOLATION, ignoring\n",
      __func__,
      VariableName
      ));
    Status = EFI_SUCCESS;
  } else if (Status == EFI_WRITE_PROTECTED) {
    DEBUG ((
      DEBUG_WARN,
      "%a: setting ReadOnly variable \"%s\" "
      "failed with EFI_WRITE_PROTECTED, ignoring\n",
      __func__,
      VariableName
      ));
    Status = EFI_SUCCESS;
  }

  return Status;
}

STATIC
RETURN_STATUS
EnsureExtraBufferSpace (
  IN  SV_INSTANCE  *Instance,
  IN  UINTN        Size
  )
{
  VOID   *NewBuffer;
  UINTN  NewSize;

  NewSize = Instance->DataSize + Size;
  if (NewSize <= Instance->BufferSize) {
    return RETURN_SUCCESS;
  }

  //
  // Double the required size to lessen the need to re-allocate in the future
  //
  NewSize = 2 * NewSize;

  NewBuffer = AllocatePool (NewSize);
  if (NewBuffer == NULL) {
    return RETURN_OUT_OF_RESOURCES;
  }

  if (Instance->BufferPtr != NULL) {
    CopyMem (NewBuffer, Instance->BufferPtr, Instance->DataSize);
    FreePool (Instance->BufferPtr);
  }

  Instance->BufferPtr  = NewBuffer;
  Instance->BufferSize = NewSize;

  return RETURN_SUCCESS;
}

STATIC
VOID
AppendToBuffer (
  IN  SV_INSTANCE  *Instance,
  IN  VOID         *Data,
  IN  UINTN        Size
  )
{
  UINTN  NewSize;

  ASSERT (Instance != NULL);
  ASSERT (Data != NULL);

  NewSize = Instance->DataSize + Size;
  ASSERT ((Instance->DataSize + Size) <= Instance->BufferSize);

  CopyMem (
    (VOID *)(((UINT8 *)(Instance->BufferPtr)) + Instance->DataSize),
    Data,
    Size
    );

  Instance->DataSize = NewSize;
}

/**
  Creates a new variable serialization instance

  @param[out]  Handle - Handle for a variable serialization instance

  @retval      RETURN_SUCCESS - The variable serialization instance was
                 successfully created.
  @retval      RETURN_OUT_OF_RESOURCES - There we not enough resources to
                 create the variable serialization instance.

**/
RETURN_STATUS
EFIAPI
SerializeVariablesNewInstance (
  OUT EFI_HANDLE  *Handle
  )
{
  SV_INSTANCE  *New;

  New = AllocateZeroPool (sizeof (*New));
  if (New == NULL) {
    return RETURN_OUT_OF_RESOURCES;
  }

  New->Signature = SV_SIGNATURE;

  *Handle = (EFI_HANDLE)New;
  return RETURN_SUCCESS;
}

/**
  Free memory associated with a variable serialization instance

  @param[in]  Handle - Handle for a variable serialization instance

  @retval      RETURN_SUCCESS - The variable serialization instance was
                 successfully freed.
  @retval      RETURN_INVALID_PARAMETER - Handle was not a valid
                 variable serialization instance.

**/
RETURN_STATUS
EFIAPI
SerializeVariablesFreeInstance (
  IN EFI_HANDLE  Handle
  )
{
  SV_INSTANCE  *Instance;

  Instance = SV_FROM_HANDLE (Handle);

  if (Instance->Signature != SV_SIGNATURE) {
    return RETURN_INVALID_PARAMETER;
  }

  Instance->Signature = 0;

  if (Instance->BufferPtr != NULL) {
    FreePool (Instance->BufferPtr);
  }

  FreePool (Instance);

  return RETURN_SUCCESS;
}

/**
  Creates a new variable serialization instance using the given
  binary representation of the variables to fill the new instance

  @param[out] Handle - Handle for a variable serialization instance
  @param[in]  Buffer - A buffer with the serialized representation
                of the variables.  Must be the same format as produced
                by SerializeVariablesToBuffer.
  @param[in]  Size - This is the size of the binary representation
                of the variables.

  @retval      RETURN_SUCCESS - The binary representation was successfully
                 imported into a new variable serialization instance
  @retval      RETURN_OUT_OF_RESOURCES - There we not enough resources to
                 create the new variable serialization instance

**/
RETURN_STATUS
EFIAPI
SerializeVariablesNewInstanceFromBuffer (
  OUT EFI_HANDLE  *Handle,
  IN  VOID        *Buffer,
  IN  UINTN       Size
  )
{
  RETURN_STATUS  Status;

  Status = SerializeVariablesNewInstance (Handle);
  if (RETURN_ERROR (Status)) {
    return Status;
  }

  Status = IterateVariablesInBuffer (
             IterateVariablesCallbackNop,
             NULL,
             Buffer,
             Size
             );
  if (RETURN_ERROR (Status)) {
    SerializeVariablesFreeInstance (*Handle);
    return Status;
  }

  Status = IterateVariablesInBuffer (
             IterateVariablesCallbackSetInInstance,
             (VOID *)*Handle,
             Buffer,
             Size
             );
  if (RETURN_ERROR (Status)) {
    SerializeVariablesFreeInstance (*Handle);
    return Status;
  }

  return Status;
}

/**
  Iterates all variables found with RuntimeServices GetNextVariableName

  @param[in]   CallbackFunction - Function called for each variable instance
  @param[in]   Context - Passed to each call of CallbackFunction

  @retval      RETURN_SUCCESS - All variables were iterated without the
                 CallbackFunction returning an error
  @retval      RETURN_OUT_OF_RESOURCES - There we not enough resources to
                 iterate through the variables
  @return      Any of RETURN_ERROR indicates an error reading the variable
                 or an error was returned from CallbackFunction

**/
RETURN_STATUS
EFIAPI
SerializeVariablesIterateSystemVariables (
  IN VARIABLE_SERIALIZATION_ITERATION_CALLBACK  CallbackFunction,
  IN VOID                                       *Context
  )
{
  RETURN_STATUS  Status;
  UINTN          VariableNameBufferSize;
  UINTN          VariableNameSize;
  CHAR16         *VariableName;
  EFI_GUID       VendorGuid;
  UINTN          VariableDataBufferSize;
  UINTN          VariableDataSize;
  VOID           *VariableData;
  UINT32         VariableAttributes;
  VOID           *NewBuffer;

  //
  // Initialize the variable name and data buffer variables.
  //
  VariableNameBufferSize = sizeof (CHAR16);
  VariableName           = AllocateZeroPool (VariableNameBufferSize);

  VariableDataBufferSize = 0;
  VariableData           = NULL;

  for ( ; ;) {
    //
    // Get the next variable name and guid
    //
    VariableNameSize = VariableNameBufferSize;
    Status           = gRT->GetNextVariableName (
                              &VariableNameSize,
                              VariableName,
                              &VendorGuid
                              );
    if (Status == EFI_BUFFER_TOO_SMALL) {
      //
      // The currently allocated VariableName buffer is too small,
      // so we allocate a larger buffer, and copy the old buffer
      // to it.
      //
      NewBuffer = AllocatePool (VariableNameSize);
      if (NewBuffer == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        break;
      }

      CopyMem (NewBuffer, VariableName, VariableNameBufferSize);
      if (VariableName != NULL) {
        FreePool (VariableName);
      }

      VariableName           = NewBuffer;
      VariableNameBufferSize = VariableNameSize;

      //
      // Try to get the next variable name again with the larger buffer.
      //
      Status = gRT->GetNextVariableName (
                      &VariableNameSize,
                      VariableName,
                      &VendorGuid
                      );
    }

    if (EFI_ERROR (Status)) {
      if (Status == EFI_NOT_FOUND) {
        Status = EFI_SUCCESS;
      }

      break;
    }

    //
    // Get the variable data and attributes
    //
    VariableDataSize = VariableDataBufferSize;
    Status           = gRT->GetVariable (
                              VariableName,
                              &VendorGuid,
                              &VariableAttributes,
                              &VariableDataSize,
                              VariableData
                              );
    if (Status == EFI_BUFFER_TOO_SMALL) {
      //
      // The currently allocated VariableData buffer is too small,
      // so we allocate a larger buffer.
      //
      if (VariableDataBufferSize != 0) {
        FreePool (VariableData);
        VariableData           = NULL;
        VariableDataBufferSize = 0;
      }

      VariableData = AllocatePool (VariableDataSize);
      if (VariableData == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        break;
      }

      VariableDataBufferSize = VariableDataSize;

      //
      // Try to read the variable again with the larger buffer.
      //
      Status = gRT->GetVariable (
                      VariableName,
                      &VendorGuid,
                      &VariableAttributes,
                      &VariableDataSize,
                      VariableData
                      );
    }

    if (EFI_ERROR (Status)) {
      break;
    }

    //
    // Run the callback function
    //
    Status = (*CallbackFunction)(
  Context,
  VariableName,
  &VendorGuid,
  VariableAttributes,
  VariableDataSize,
  VariableData
  );
    if (EFI_ERROR (Status)) {
      break;
    }
  }

  if (VariableName != NULL) {
    FreePool (VariableName);
  }

  if (VariableData != NULL) {
    FreePool (VariableData);
  }

  return Status;
}

/**
  Iterates all variables found in the variable serialization instance

  @param[in]   Handle - Handle for a variable serialization instance
  @param[in]   CallbackFunction - Function called for each variable instance
  @param[in]   Context - Passed to each call of CallbackFunction

  @retval      RETURN_SUCCESS - All variables were iterated without the
                 CallbackFunction returning an error
  @retval      RETURN_OUT_OF_RESOURCES - There we not enough resources to
                 iterate through the variables
  @return      Any of RETURN_ERROR indicates an error reading the variable
                 or an error was returned from CallbackFunction

**/
RETURN_STATUS
EFIAPI
SerializeVariablesIterateInstanceVariables (
  IN EFI_HANDLE                                 Handle,
  IN VARIABLE_SERIALIZATION_ITERATION_CALLBACK  CallbackFunction,
  IN VOID                                       *Context
  )
{
  SV_INSTANCE  *Instance;

  Instance = SV_FROM_HANDLE (Handle);

  if ((Instance->BufferPtr != NULL) && (Instance->DataSize != 0)) {
    return IterateVariablesInBuffer (
             CallbackFunction,
             Context,
             Instance->BufferPtr,
             Instance->DataSize
             );
  } else {
    return RETURN_SUCCESS;
  }
}

/**
  Sets all variables found in the variable serialization instance

  @param[in]   Handle - Handle for a variable serialization instance

  @retval      RETURN_SUCCESS - All variables were set successfully
  @retval      RETURN_OUT_OF_RESOURCES - There we not enough resources to
                 set all the variables
  @return      Any of RETURN_ERROR indicates an error reading the variables
                 or in attempting to set a variable

**/
RETURN_STATUS
EFIAPI
SerializeVariablesSetSerializedVariables (
  IN EFI_HANDLE  Handle
  )
{
  return SerializeVariablesIterateInstanceVariables (
           Handle,
           IterateVariablesCallbackSetSystemVariable,
           NULL
           );
}

/**
  Adds a variable to the variable serialization instance

  @param[in] Handle - Handle for a variable serialization instance
  @param[in] VariableName - Refer to RuntimeServices GetVariable
  @param[in] VendorGuid - Refer to RuntimeServices GetVariable
  @param[in] Attributes - Refer to RuntimeServices GetVariable
  @param[in] DataSize - Refer to RuntimeServices GetVariable
  @param[in] Data - Refer to RuntimeServices GetVariable

  @retval      RETURN_SUCCESS - All variables were set successfully
  @retval      RETURN_OUT_OF_RESOURCES - There we not enough resources to
                 add the variable
  @retval      RETURN_INVALID_PARAMETER - Handle was not a valid
                 variable serialization instance or
                 VariableName, VariableGuid or Data are NULL.

**/
RETURN_STATUS
EFIAPI
SerializeVariablesAddVariable (
  IN EFI_HANDLE  Handle,
  IN CHAR16      *VariableName,
  IN EFI_GUID    *VendorGuid,
  IN UINT32      Attributes,
  IN UINTN       DataSize,
  IN VOID        *Data
  )
{
  RETURN_STATUS  Status;
  SV_INSTANCE    *Instance;
  UINT32         SerializedNameSize;
  UINT32         SerializedDataSize;
  UINTN          SerializedSize;

  Instance = SV_FROM_HANDLE (Handle);

  if ((Instance->Signature != SV_SIGNATURE) ||
      (VariableName == NULL) || (VendorGuid == NULL) || (Data == NULL))
  {
  }

  SerializedNameSize = (UINT32)StrSize (VariableName);

  SerializedSize =
    sizeof (SerializedNameSize) +
    SerializedNameSize +
    sizeof (*VendorGuid) +
    sizeof (Attributes) +
    sizeof (SerializedDataSize) +
    DataSize;

  Status = EnsureExtraBufferSpace (
             Instance,
             SerializedSize
             );
  if (RETURN_ERROR (Status)) {
    return Status;
  }

  //
  // Add name size (UINT32)
  //
  AppendToBuffer (Instance, (VOID *)&SerializedNameSize, sizeof (SerializedNameSize));

  //
  // Add variable unicode name string
  //
  AppendToBuffer (Instance, (VOID *)VariableName, SerializedNameSize);

  //
  // Add variable GUID
  //
  AppendToBuffer (Instance, (VOID *)VendorGuid, sizeof (*VendorGuid));

  //
  // Add variable attributes
  //
  AppendToBuffer (Instance, (VOID *)&Attributes, sizeof (Attributes));

  //
  // Add variable data size (UINT32)
  //
  SerializedDataSize = (UINT32)DataSize;
  AppendToBuffer (Instance, (VOID *)&SerializedDataSize, sizeof (SerializedDataSize));

  //
  // Add variable data
  //
  AppendToBuffer (Instance, Data, DataSize);

  return RETURN_SUCCESS;
}

/**
  Serializes the variables known to this instance into the
  provided buffer.

  @param[in]     Handle - Handle for a variable serialization instance
  @param[out]    Buffer - A buffer to store the binary representation
                   of the variables.
  @param[in,out] Size - On input this is the size of the buffer.
                   On output this is the size of the binary representation
                   of the variables.

  @retval      RETURN_SUCCESS - The binary representation was successfully
                 completed and returned in the buffer.
  @retval      RETURN_OUT_OF_RESOURCES - There we not enough resources to
                 save the variables to the buffer.
  @retval      RETURN_INVALID_PARAMETER - Handle was not a valid
                 variable serialization instance or
                 Size or Buffer were NULL.
  @retval      RETURN_BUFFER_TOO_SMALL - The Buffer size as indicated by
                 the Size parameter was too small for the serialized
                 variable data.  Size is returned with the required size.

**/
RETURN_STATUS
EFIAPI
SerializeVariablesToBuffer (
  IN     EFI_HANDLE  Handle,
  OUT    VOID        *Buffer,
  IN OUT UINTN       *Size
  )
{
  SV_INSTANCE  *Instance;

  Instance = SV_FROM_HANDLE (Handle);

  if (Size == NULL) {
    return RETURN_INVALID_PARAMETER;
  }

  if (*Size < Instance->DataSize) {
    *Size = Instance->DataSize;
    return RETURN_BUFFER_TOO_SMALL;
  }

  if (Buffer == NULL) {
    return RETURN_INVALID_PARAMETER;
  }

  *Size = Instance->DataSize;
  CopyMem (Buffer, Instance->BufferPtr, Instance->DataSize);

  return RETURN_SUCCESS;
}
