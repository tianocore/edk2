/** @file
  Provides interface to advanced shell functionality for parsing both handle and protocol database.

  Copyright (c) 2010 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __HANDLE_PARSING_LIB__
#define __HANDLE_PARSING_LIB__

#include <Uefi.h>

/**
  Function to add a new GUID/Name mapping.

  This cannot overwrite an existing mapping.

  @param[in] Guid       The Guid
  @param[in] TheName    The Guid's name
  @param[in] Lang       RFC4646 language code list or NULL

  @retval EFI_SUCCESS           The operation was sucessful
  @retval EFI_ACCESS_DENIED     There was a duplicate
  @retval EFI_OUT_OF_RESOURCES  A memory allocation failed
**/
EFI_STATUS
EFIAPI
AddNewGuidNameMapping (
  IN CONST EFI_GUID  *Guid,
  IN CONST CHAR16    *TheName,
  IN CONST CHAR8     *Lang OPTIONAL
  );

/**
  Function to get the name of a protocol or struct from it's GUID.

  If Guid is NULL, then ASSERT.

  @param[in] Guid               The GUID to look for the name of.
  @param[in] Lang               The language to use.

  @return                       The pointer to a string of the name.  The caller
                                is responsible to free this memory.
**/
CHAR16 *
EFIAPI
GetStringNameFromGuid (
  IN CONST EFI_GUID  *Guid,
  IN CONST CHAR8     *Lang OPTIONAL
  );

/**
  Function to get the Guid for a protocol or struct based on it's string name.

  Do not free or modify the returned GUID.

  @param[in] Name           The pointer to the string name.
  @param[in] Lang           The pointer to the language code (string).
  @param[out] Guid          The pointer to the pointer to the Guid.

  @retval EFI_SUCCESS       The operation was successful.
**/
EFI_STATUS
EFIAPI
GetGuidFromStringName (
  IN CONST CHAR16  *Name,
  IN CONST CHAR8   *Lang OPTIONAL,
  OUT EFI_GUID     **Guid
  );

/**
  Function to dump protocol information from a handle.

  This function will return a allocated string buffer containing the
  information.  The caller is responsible for freeing the memory.

  If Guid is NULL, ASSERT().
  If TheHandle is NULL, ASSERT().

  @param[in] TheHandle      The handle to dump information from.
  @param[in] Guid           The GUID of the protocol to dump.
  @param[in] Verbose        TRUE for extra info.  FALSE otherwise.

  @return                   The pointer to string.
  @retval NULL              An error was encountered.
**/
CHAR16 *
EFIAPI
GetProtocolInformationDump (
  IN CONST EFI_HANDLE  TheHandle,
  IN CONST EFI_GUID    *Guid,
  IN CONST BOOLEAN     Verbose
  );

/**
  Function to retrieve the driver name (if possible) from the ComponentName or
  ComponentName2 protocol.

  The string returned must be callee freed.

  @param[in] TheHandle      The driver handle to get the name of.
  @param[in] Language       The language to use.

  @retval NULL              The name could not be found.
  @return                   A pointer to the string name.  Do not de-allocate the memory.
**/
CONST CHAR16 *
EFIAPI
GetStringNameFromHandle (
  IN CONST EFI_HANDLE  TheHandle,
  IN CONST CHAR8       *Language
  );

/**
  Get best support language for this driver.

  First base on the user input language  to search, second base on the current
  platform used language to search, third get the first language from the
  support language list. The caller need to free the buffer of the best language.

  @param[in] SupportedLanguages      The support languages for this driver.
  @param[in] InputLanguage           The user input language.
  @param[in] Iso639Language          Whether get language for ISO639.

  @return                            The best support language for this driver.
**/
CHAR8 *
EFIAPI
GetBestLanguageForDriver (
  IN CONST CHAR8  *SupportedLanguages,
  IN CONST CHAR8  *InputLanguage,
  IN BOOLEAN      Iso639Language
  );

#define HR_UNKNOWN                      0
#define HR_IMAGE_HANDLE                 BIT1
#define HR_DRIVER_BINDING_HANDLE        BIT2 // has driver binding
#define HR_DEVICE_DRIVER                BIT3 // device driver (hybrid?)
#define HR_BUS_DRIVER                   BIT4 // a bus driver  (hybrid?)
#define HR_DRIVER_CONFIGURATION_HANDLE  BIT5
#define HR_DRIVER_DIAGNOSTICS_HANDLE    BIT6
#define HR_COMPONENT_NAME_HANDLE        BIT7
#define HR_DEVICE_HANDLE                BIT8
#define HR_PARENT_HANDLE                BIT9
#define HR_CONTROLLER_HANDLE            BIT10
#define HR_CHILD_HANDLE                 BIT11
#define HR_VALID_MASK                   (BIT1|BIT2|BIT3|BIT4|BIT5|BIT6|BIT7|BIT8|BIT9|BIT10|BIT11)

/**
  Gets all the related EFI_HANDLEs based on the mask supplied.

  This function will scan all EFI_HANDLES in the UEFI environment's handle database
  and return all the ones with the specified relationship (Mask) to the specified
  controller handle.

  If both DriverBindingHandle and ControllerHandle are NULL, then ASSERT.
  If MatchingHandleCount is NULL, then ASSERT.

  If MatchingHandleBuffer is not NULL upon a successful return, the memory must be
  caller freed.

  @param[in] DriverBindingHandle    The handle with Driver Binding protocol on it.
  @param[in] ControllerHandle       The handle with Device Path protocol on it.
  @param[in] Mask                   The mask of what relationship(s) is desired.
  @param[in] MatchingHandleCount    The pointer to UINTN specifying number of HANDLES in
                                    MatchingHandleBuffer.
  @param[out] MatchingHandleBuffer  On a successful return, a buffer of MatchingHandleCount
                                    EFI_HANDLEs with a terminating NULL EFI_HANDLE.

  @retval EFI_SUCCESS               The operation was successful, and any related handles
                                    are in MatchingHandleBuffer.
  @retval EFI_NOT_FOUND             No matching handles were found.
  @retval EFI_INVALID_PARAMETER     A parameter was invalid or out of range.
  @sa ParseHandleDatabaseByRelationshipWithType
**/
EFI_STATUS
EFIAPI
ParseHandleDatabaseByRelationship (
  IN CONST EFI_HANDLE  DriverBindingHandle OPTIONAL,
  IN CONST EFI_HANDLE  ControllerHandle OPTIONAL,
  IN CONST UINTN       Mask,
  IN UINTN             *MatchingHandleCount,
  OUT EFI_HANDLE       **MatchingHandleBuffer OPTIONAL
  );

/**
  Gets all the related EFI_HANDLEs based on the mask supplied.

  This function scans all EFI_HANDLES in the UEFI environment's handle database
  and returns the ones with the specified relationship (Mask) to the specified
  controller handle.

  If both DriverBindingHandle and ControllerHandle are NULL, then ASSERT.
  If MatchingHandleCount is NULL, then ASSERT.

  If MatchingHandleBuffer is not NULL upon a successful return the memory must be
  caller freed.

  @param[in] DriverBindingHandle    The handle with Driver Binding protocol on it.
  @param[in] ControllerHandle       The handle with Device Path protocol on it.
  @param[in] MatchingHandleCount    The pointer to UINTN that specifies the number of HANDLES in
                                    MatchingHandleBuffer.
  @param[out] MatchingHandleBuffer  On a successful return, a buffer of MatchingHandleCount
                                    EFI_HANDLEs with a terminating NULL EFI_HANDLE.
  @param[out] HandleType            An array of type information.

  @retval EFI_SUCCESS               The operation was successful, and any related handles
                                    are in MatchingHandleBuffer.
  @retval EFI_NOT_FOUND             No matching handles were found.
  @retval EFI_INVALID_PARAMETER     A parameter was invalid or out of range.
**/
EFI_STATUS
EFIAPI
ParseHandleDatabaseByRelationshipWithType (
  IN CONST EFI_HANDLE  DriverBindingHandle OPTIONAL,
  IN CONST EFI_HANDLE  ControllerHandle OPTIONAL,
  IN UINTN             *HandleCount,
  OUT EFI_HANDLE       **HandleBuffer,
  OUT UINTN            **HandleType
  );

/**
  Gets handles for any parents of the passed in controller.

  @param[in] ControllerHandle       The handle of the controller.
  @param[in] Count                  The pointer to the number of handles in
                                    MatchingHandleBuffer on return.
  @param[out] Buffer                The buffer containing handles on a successful
                                    return.
  @retval EFI_SUCCESS               The operation was successful.
  @sa ParseHandleDatabaseByRelationship
**/
#define PARSE_HANDLE_DATABASE_PARENTS(ControllerHandle, Count, Buffer) \
  ParseHandleDatabaseByRelationship(NULL, ControllerHandle, HR_PARENT_HANDLE, Count, Buffer)

/**
  Gets handles for any UEFI drivers of the passed in controller.

  @param[in] ControllerHandle       The handle of the controller.
  @param[in] Count                  The pointer to the number of handles in
                                    MatchingHandleBuffer on return.
  @param[out] Buffer                The buffer containing handles on a successful
                                    return.
  @retval EFI_SUCCESS               The operation was successful.
  @sa ParseHandleDatabaseByRelationship
**/
#define PARSE_HANDLE_DATABASE_UEFI_DRIVERS(ControllerHandle, Count, Buffer) \
  ParseHandleDatabaseByRelationship(NULL, ControllerHandle, HR_DRIVER_BINDING_HANDLE|HR_DEVICE_DRIVER, Count, Buffer)

/**
  Gets handles for any children of the passed in controller by the passed in driver handle.

  @param[in] DriverHandle           The handle of the driver.
  @param[in] ControllerHandle       The handle of the controller.
  @param[in] Count                  The pointer to the number of handles in
                                    MatchingHandleBuffer on return.
  @param[out] Buffer                The buffer containing handles on a successful
                                    return.
  @retval EFI_SUCCESS               The operation was successful.
  @sa ParseHandleDatabaseByRelationship
**/
#define PARSE_HANDLE_DATABASE_MANAGED_CHILDREN(DriverHandle, ControllerHandle, Count, Buffer) \
  ParseHandleDatabaseByRelationship(DriverHandle, ControllerHandle, HR_CHILD_HANDLE|HR_DEVICE_HANDLE, Count, Buffer)

/**
  Gets handles for any devices managed by the passed in driver.

  @param[in] DriverHandle           The handle of the driver.
  @param[in] Count                  The pointer to the number of handles in
                                    MatchingHandleBuffer on return.
  @param[out] Buffer                The buffer containing handles on a successful
                                    return.
  @retval EFI_SUCCESS               The operation was successful.
  @sa ParseHandleDatabaseByRelationship
**/
#define PARSE_HANDLE_DATABASE_DEVICES(DriverHandle, Count, Buffer) \
  ParseHandleDatabaseByRelationship(DriverHandle, NULL, HR_CONTROLLER_HANDLE|HR_DEVICE_HANDLE, Count, Buffer)

/**
  Gets handles for any child devices produced by the passed in driver.

  @param[in] DriverHandle           The handle of the driver.
  @param[in] MatchingHandleCount    The pointer to the number of handles in
                                    MatchingHandleBuffer on return.
  @param[out] MatchingHandleBuffer  The buffer containing handles on a successful
                                    return.
  @retval EFI_SUCCESS               The operation was successful.
  @sa ParseHandleDatabaseByRelationship
**/
EFI_STATUS
EFIAPI
ParseHandleDatabaseForChildDevices (
  IN CONST EFI_HANDLE  DriverHandle,
  IN UINTN             *MatchingHandleCount,
  OUT EFI_HANDLE       **MatchingHandleBuffer OPTIONAL
  );

/**
  Gets handles for any child controllers of the passed in controller.

  @param[in] ControllerHandle       The handle of the "parent controller".
  @param[out] MatchingHandleCount   The pointer to the number of handles in
                                    MatchingHandleBuffer on return.
  @param[out] MatchingHandleBuffer  The buffer containing handles on a successful
                                    return.
  @retval EFI_SUCCESS               The operation was successful.
  @sa ParseHandleDatabaseByRelationship
**/
EFI_STATUS
EFIAPI
ParseHandleDatabaseForChildControllers (
  IN CONST EFI_HANDLE  ControllerHandle,
  OUT UINTN            *MatchingHandleCount,
  OUT EFI_HANDLE       **MatchingHandleBuffer OPTIONAL
  );

/**
  Function to retrieve the human-friendly index of a given handle.  If the handle
  does not have a index one will be automatically assigned.  The index value is valid
  until the termination of the shell application.

  @param[in] TheHandle    The handle to retrieve an index for.

  @retval 0               A memory allocation failed.
  @return                 The index of the handle.

**/
UINTN
EFIAPI
ConvertHandleToHandleIndex (
  IN CONST EFI_HANDLE  TheHandle
  );

/**
  Function to retrieve the EFI_HANDLE from the human-friendly index.

  @param[in] TheIndex     The index to retrieve the EFI_HANDLE for.

  @retval NULL            The index was invalid.
  @return                 The EFI_HANDLE that index represents.

**/
EFI_HANDLE
EFIAPI
ConvertHandleIndexToHandle (
  IN CONST UINTN  TheIndex
  );

/**
  Function to get all handles that support a given protocol or all handles.

  The caller is responsible to free this memory.

  @param[in] ProtocolGuid The guid of the protocol to get handles for.  If NULL
                          then the function will return all handles.

  @retval NULL            A memory allocation failed.
  @return                 A NULL terminated list of handles.
**/
EFI_HANDLE *
EFIAPI
GetHandleListByProtocol (
  IN CONST EFI_GUID  *ProtocolGuid OPTIONAL
  );

/**
  Function to get all handles that support some protocols.

  The caller is responsible to free this memory.

  @param[in] ProtocolGuids  A NULL terminated list of protocol GUIDs.

  @retval NULL              A memory allocation failed.
  @retval NULL              ProtocolGuids was NULL.
  @return                   A NULL terminated list of EFI_HANDLEs.
**/
EFI_HANDLE *
EFIAPI
GetHandleListByProtocolList (
  IN CONST EFI_GUID  **ProtocolGuids
  );

/**
  Return all supported GUIDs.

  @param[out]      Guids  The buffer to return all supported GUIDs.
  @param[in, out]  Count  On input, the count of GUIDs the buffer can hold,
                         On output, the count of GUIDs to return.

  @retval EFI_INVALID_PARAMETER Count is NULL.
  @retval EFI_BUFFER_TOO_SMALL  Buffer is not enough to hold all GUIDs.
  @retval EFI_SUCCESS           GUIDs are returned successfully.
**/
EFI_STATUS
EFIAPI
GetAllMappingGuids (
  OUT EFI_GUID  *Guids,
  IN OUT UINTN  *Count
  );

#endif // __HANDLE_PARSING_LIB__
