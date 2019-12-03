/** @file
  Serialize & Deserialize UEFI Variables

  Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __SERIALIZE_VARIABLES_LIB__
#define __SERIALIZE_VARIABLES_LIB__


/**
  Callback function for each variable

  @param[in] Context - Context as sent to the iteration function
  @param[in] VariableName - Refer to RuntimeServices GetNextVariableName
  @param[in] VendorGuid - Refer to RuntimeServices GetNextVariableName
  @param[in] Attributes - Refer to RuntimeServices GetVariable
  @param[in] DataSize - Refer to RuntimeServices GetVariable
  @param[in] Data - Refer to RuntimeServices GetVariable

  @retval RETURN_SUCCESS         Continue iterating through the variables
  @return Any RETURN_ERROR       Stop iterating through the variables

**/
typedef
RETURN_STATUS
(EFIAPI *VARIABLE_SERIALIZATION_ITERATION_CALLBACK)(
  IN  VOID                         *Context,
  IN  CHAR16                       *VariableName,
  IN  EFI_GUID                     *VendorGuid,
  IN  UINT32                       Attributes,
  IN  UINTN                        DataSize,
  IN  VOID                         *Data
  );


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
  OUT EFI_HANDLE                      *Handle
  );


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
  IN EFI_HANDLE Handle
  );


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
  OUT EFI_HANDLE                          *Handle,
  IN  VOID                                *Buffer,
  IN  UINTN                               Size
  );


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
  IN VARIABLE_SERIALIZATION_ITERATION_CALLBACK CallbackFunction,
  IN VOID                                      *Context
  );


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
  IN EFI_HANDLE                                Handle,
  IN VARIABLE_SERIALIZATION_ITERATION_CALLBACK CallbackFunction,
  IN VOID                                      *Context
  );


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
  IN EFI_HANDLE                       Handle
  );


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

**/
RETURN_STATUS
EFIAPI
SerializeVariablesAddVariable (
  IN EFI_HANDLE                   Handle,
  IN CHAR16                       *VariableName,
  IN EFI_GUID                     *VendorGuid,
  IN UINT32                       Attributes,
  IN UINTN                        DataSize,
  IN VOID                         *Data
  );


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

**/
RETURN_STATUS
EFIAPI
SerializeVariablesToBuffer (
  IN     EFI_HANDLE                       Handle,
  OUT    VOID                             *Buffer,
  IN OUT UINTN                            *Size
  );


#endif

