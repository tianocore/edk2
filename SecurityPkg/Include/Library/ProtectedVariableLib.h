/** @file
  Defines interfaces of protected variable services for non-volatile variable
  storage.

Copyright (c) 2020 - 2022, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _PROTECTED_VARIABLE_LIB_H_
#define _PROTECTED_VARIABLE_LIB_H_

#include <PiPei.h>
#include <PiDxe.h>

#include <Guid/VariableFormat.h>

#include <Protocol/VarCheck.h>
#include <Protocol/FirmwareVolumeBlock.h>

#include <Library/UefiLib.h>
#include <Library/EncryptionVariableLib.h>

#pragma pack(1)

typedef struct _VARIABLE_DIGEST {
  ///
  /// Pointer to digest of next variable in a pre-defined rule of order for
  /// integration verification. In other words, the final HMAC of all
  /// protected variables is calculated by concatenating digest of each
  /// variable in the order of this singly linked list.
  ///
  EFI_PHYSICAL_ADDRESS    Prev;
  EFI_PHYSICAL_ADDRESS    Next;
  ///
  /// Index to variable in physical store, used to locate the variable directly
  /// inside the store (Implementation dependent).
  ///
  EFI_PHYSICAL_ADDRESS    StoreIndex;
  ///
  /// Index to variable in memory cache, used to locate the variable directly
  /// inside the cache (Implementation dependent).
  ///
  EFI_PHYSICAL_ADDRESS    CacheIndex;

  ///
  /// Pointer to Cache offset within Global Structure
  ///
  UINT32                  CacheOffset;

  ///
  /// Frequently accessed information relating to the variable.
  ///
  UINT16                  DigestSize;     /// Size of digest value
  UINT16                  NameSize;       /// Size of variable name
  UINT32                  DataSize;       /// Size of variable data
  UINT32                  PlainDataSize;  /// Size of plain data of current variable (if encrypted)
  UINT32                  State;          /// State of current variable
  UINT32                  Attributes;     /// Attributes of current variable

  EFI_GUID                VendorGuid;
  struct {
    BOOLEAN                 Auth;            /// Authenticated variable format
    BOOLEAN                 Valid;           /// Valid variable data in current variable
    BOOLEAN                 Protected;       /// Protected variable (used in calculating HMAC)
    BOOLEAN                 Encrypted;       /// Encrypted variable
    BOOLEAN                 Freeable;        /// Memory reserved for current node can be freed
    BOOLEAN                 CacheIndexAhead; /// Indicates if CacheIndex is Ahead relative to Global structure
    BOOLEAN                 Reserved[2];
  }                       Flags;
  //
  // Data with variable length are put at the end of this structure.
  //
  //CHAR16                VariableName[NameSize/2];
  //UINT8                 DigestValue[DigestSize];
} VARIABLE_DIGEST;

#pragma pack()

#define VAR_DIG_NAMEOFF(VarDig)     (sizeof (VARIABLE_DIGEST))
#define VAR_DIG_DIGOFF(VarDig)      (VAR_DIG_NAMEOFF (VarDig) + (VarDig)->NameSize)

#define VAR_DIG_END(VarDig)         (VAR_DIG_DIGOFF (VarDig) + (VarDig)->DigestSize)

#define VAR_DIG_VALUE(VarDig)       (VOID *)((UINTN)(VarDig) + VAR_DIG_DIGOFF (VarDig))
#define VAR_DIG_NAME(VarDig)        (CHAR16 *)((UINTN)(VarDig) + VAR_DIG_NAMEOFF (VarDig))
#define VAR_DIG_GUID(VarDig)        &(VAR_DIG_PTR (VarDig)->VendorGuid)

#define VAR_DIG_PTR(Addr)           ((VARIABLE_DIGEST *)(UINTN)(Addr))
#define VAR_DIG_ADR(Ptr)            ((EFI_PHYSICAL_ADDRESS)(UINTN)(Ptr))
#define VAR_DIG_NEXT(VarDig)        (VAR_DIG_PTR ((VarDig)->Next))
#define VAR_DIG_PREV(VarDig)        (VAR_DIG_PTR ((VarDig)->Prev))

#define VAR_INDEX_INVALID           ((UINT64)(-1LL))

#define VAR_HDR_PTR(Addr)           ((VARIABLE_HEADER *)(UINTN)(Addr))

typedef VARIABLE_ENCRYPTION_INFO    PROTECTED_VARIABLE_INFO;

/**

  This function writes data to the NV variable storage at given position.

  Note: Per current variable service architecture, only SMM is allowed to
        (directly) change NV variable storage.

  @param VariableInfo             Pointer to structure holding details of a variable.
  @param Offset                   Offset to the given variable to write from.
  @param Size                     Size of data to be written.
  @param Buffer                   Pointer to the buffer from which data is written.

  @retval EFI_INVALID_PARAMETER  Invalid parameters passed in.
  @retval EFI_UNSUPPORTED        Updating NV variable storage is not supported.
  @retval EFI_OUT_OF_RESOURCES   Not enough resource to complete the operation.
  @retval EFI_SUCCESS            Variable store successfully updated.

**/
typedef
EFI_STATUS
(EFIAPI *PROTECTED_VAR_LIB_UPDATE_VARIABLE_STORE) (
  IN  PROTECTED_VARIABLE_INFO     *VariableInfo,
  IN  UINTN                       Offset,
  IN  UINT32                      Size,
  IN  UINT8                       *Buffer
  );

/**
  Update the variable region with Variable information.

  @param[in] AuthVariableInfo       Pointer AUTH_VARIABLE_INFO structure for
                                    input of the variable.

  @retval EFI_SUCCESS               The update operation is success.
  @retval EFI_INVALID_PARAMETER     Invalid parameter.
  @retval EFI_WRITE_PROTECTED       Variable is write-protected.
  @retval EFI_OUT_OF_RESOURCES      There is not enough resource.

**/
typedef
EFI_STATUS
(EFIAPI *PROTECTED_VAR_LIB_UPDATE_VARIABLE) (
  IN AUTH_VARIABLE_INFO     *AuthVariableInfo
  );

/**

  Retrieve details about a variable and return them in VariableInfo->Header.

  If VariableInfo->Address is given, this function will calculate its offset
  relative to given variable storage via VariableStore; Otherwise, it will try
  other internal variable storages or cached copies. It's assumed that, for all
  copies of NV variable storage, all variables are stored in the same relative
  position. If VariableInfo->Address is found in the range of any storage copies,
  its offset relative to that storage should be the same in other copies.

  If VariableInfo->Offset is given (non-zero) but not VariableInfo->Address,
  this function will return the variable memory address inside VariableStore,
  if given, via VariableInfo->Address; Otherwise, the address of other storage
  copies will be returned, if any.

  For a new variable whose offset has not been determined, a value of -1 as
  VariableInfo->Offset should be passed to skip the offset calculation.

  @param VariableInfo             Pointer to variable information.

  @retval EFI_INVALID_PARAMETER  VariableInfo is NULL or both VariableInfo->Address
                                 and VariableInfo->Offset are NULL (0).
  @retval EFI_NOT_FOUND          If given Address or Offset is out of range of
                                 any given or internal storage copies.
  @retval EFI_SUCCESS            Variable details are retrieved successfully.

**/
typedef
EFI_STATUS
(EFIAPI *PROTECTED_VAR_LIB_GET_VAR_INFO) (
  IN  OUT PROTECTED_VARIABLE_INFO   *VariableInfo
  );

/**

  Retrieve details of the variable next to given variable within VariableStore.

  If VarInfo->Address is NULL, the first one in VariableStore is returned.

  VariableStart and/or VariableEnd can be given optionally for the situation
  in which the valid storage space is smaller than the VariableStore->Size.
  This usually happens when PEI variable services make a compact variable
  cache to save memory, which cannot make use VariableStore->Size to determine
  the correct variable storage range.

  @param VariableStore            Pointer to a variable storage. It's optional.
  @param VariableStart            Start point of valid range in VariableStore.
  @param VariableEnd              End point of valid range in VariableStore.
  @param VariableInfo             Pointer to variable information.

  @retval EFI_INVALID_PARAMETER  VariableInfo or VariableStore is NULL.
  @retval EFI_NOT_FOUND          If the end of VariableStore is reached.
  @retval EFI_SUCCESS            The next variable is retrieved successfully.

**/
typedef
EFI_STATUS
(EFIAPI *PROTECTED_VAR_LIB_GET_NEXT_VAR_INFO) (
  IN  OUT PROTECTED_VARIABLE_INFO   *VariableInfo
  );


typedef
EFI_STATUS
(EFIAPI *DIGEST_METHOD_CALLBACK) (
  IN      VARIABLE_HEADER           *Variable,
  IN  OUT VARIABLE_DIGEST        *Digest
  );

/**

  Initialize a memory copy of NV variable storage.

  To save memory consumption (especially in PEI phase), it's allowed to cache
  only valid variables. In such case, an index table recording offset of each
  valid variables could be employed. The index table makes sure the cached copy
  to be synchronized with the original copy in NV variable storage. To avoid
  TOCTOU issue, once the variables are cached in memory and verified, NV
  variable storage should not be used to read variable information. The cached
  copy should be used instead.

  If StoreCacheBase is not given, this function should return the required
  cache size and valid variable number, if VariableNumber is not NULL. Then
  the caller can prepare correct cache buffer and index table buffer before
  next calling.

  @param[in]      CacheBase       Base address of NV variable storage cache.
  @param[in]      CacheSize       Size of CacheBuffer.
  @param[out]     CacheSize       Size of required cache buffer.
  @param[out]     DigestBuffer    Base address of digest of each variable.
  @param[in]      DigestSize      Digest size of one variable if DigestBuffer is NULL.
                                  Size of DigestBuffer if DigestBuffer is NOT NULL.
  @param[out]     DigestSize      Required size of DigestBuffer if DigestBuffer is NULL.
  @param[out]     DigestMethod    Method used to generate digest for each variable.
  @param[out]     VariableNumber  Number of valid variables.
  @param[out]     AuthFlag        Auth-variable indicator.

  @retval EFI_INVALID_PARAMETER   CacheSize is NULL; Or,
                                  StoreCacheBase is 0 but *CacheSize it not.
  @retval EFI_VOLUME_CORRUPTED    If original NV variable storage is corrupted.
  @retval EFI_BUFFER_TOO_SMALL    If *CacheSize is smaller than required memory.
  @retval EFI_SUCCESS             The cached variable storage is initialized.

**/
typedef
EFI_STATUS
(EFIAPI *PROTECTED_VAR_LIB_INIT_VAR_STORE) (
     OUT  VOID                      *CacheBase OPTIONAL,
  IN OUT  UINT32                    *CacheSize OPTIONAL,
     OUT  VOID                      *DigBuffer OPTIONAL,
  IN OUT  UINT32                    *DigBufferSize OPTIONAL,
  IN      UINT32                    DigSize OPTIONAL,
  IN      DIGEST_METHOD_CALLBACK    DigMethod OPTIONAL,
     OUT  UINT32                    *VarNumber OPTIONAL,
     OUT  BOOLEAN                   *AuthFlag OPTIONAL
  );

/**

  Initiate a variable retrieval in SMM environment from non-SMM environment.

  This is usually required in BS/RT environment when local cached copy is in
  encrypted form. Variable decryption can only be done in SMM environment.

  @param[in]      VariableName       Name of Variable to be found.
  @param[in]      VendorGuid         Variable vendor GUID.
  @param[out]     Attributes         Attribute value of the variable found.
  @param[in, out] DataSize           Size of Data found. If size is less than the
                                     data, this value contains the required size.
  @param[out]     Data               Data pointer.

  @retval EFI_SUCCESS                Found the specified variable.
  @retval EFI_INVALID_PARAMETER      Invalid parameter.
  @retval EFI_NOT_FOUND              The specified variable could not be found.

**/
typedef
EFI_STATUS
(EFIAPI *PROTECTED_VAR_LIB_FIND_VAR_SMM) (
  IN      CHAR16                     *VariableName,
  IN      EFI_GUID                   *VendorGuid,
      OUT UINT32                     *Attributes OPTIONAL,
  IN  OUT UINTN                      *DataSize,
      OUT VOID                       *Data OPTIONAL
  );

/**
  Check if a variable is user variable or not.

  @param[in] Variable   Pointer to variable header.

  @retval TRUE          User variable.
  @retval FALSE         System variable.

**/
typedef
BOOLEAN
(EFIAPI *PROTECTED_VAR_LIB_IS_USER_VAR) (
  IN VARIABLE_HEADER    *Variable
  );

/**
  Check if a HOB variable store is available or not.

  @retval EFI_NOT_READY  HOB variable store info not available.
  @retval EFI_NOT_FOUND  HOB variable store is NOT available.
  @retval EFI_SUCCESS    HOB variable store is available.
**/
typedef
EFI_STATUS
(EFIAPI *PROTECTED_VAR_LIB_HOB_STORE_AVAILABLE) (
  VOID
  );

typedef enum {
  FromPeiModule,
  FromBootServiceModule,
  FromRuntimeModule,
  FromSmmModule
} VARIABLE_SERVICE_USER;

#pragma pack(1)

#define PROTECTED_VARIABLE_CONTEXT_IN_STRUCT_VERSION  0x02

typedef struct _PROTECTED_VARIABLE_CONTEXT_IN {
  UINT32                                      StructVersion;
  UINT32                                      StructSize;
  UINT32                                      MaxVariableSize;

  VARIABLE_SERVICE_USER                       VariableServiceUser;

  PROTECTED_VAR_LIB_FIND_VAR_SMM              FindVariableSmm;
  PROTECTED_VAR_LIB_GET_VAR_INFO              GetVariableInfo;
  PROTECTED_VAR_LIB_GET_NEXT_VAR_INFO         GetNextVariableInfo;
  PROTECTED_VAR_LIB_UPDATE_VARIABLE_STORE     UpdateVariableStore;
  PROTECTED_VAR_LIB_UPDATE_VARIABLE           UpdateVariable;
  PROTECTED_VAR_LIB_HOB_STORE_AVAILABLE       IsHobVariableStoreAvailable;
} PROTECTED_VARIABLE_CONTEXT_IN;

#pragma pack()

/**

  Initialization for protected variable services.

  If this initialization failed upon any error, the whole variable services
  should not be used.  A system reset might be needed to re-construct NV
  variable storage to be the default state.

  @param[in]  ContextIn   Pointer to variable service context needed by
                          protected variable.

  @retval EFI_SUCCESS               Protected variable services are ready.
  @retval EFI_INVALID_PARAMETER     If ContextIn == NULL or something missing or
                                    mismatching in the content in ContextIn.
  @retval EFI_COMPROMISED_DATA      If failed to check integrity of protected variables.
  @retval EFI_OUT_OF_RESOURCES      Fail to allocate enough resource.
  @retval EFI_UNSUPPORTED           Unsupported to process protected variable.

**/
EFI_STATUS
EFIAPI
ProtectedVariableLibInitialize (
  IN  PROTECTED_VARIABLE_CONTEXT_IN   *ContextIn
  );

/**

  An alternative version of ProtectedVariableLibGetData to get plain data, if
  encrypted, from given variable, for different use cases.

  @param[in,out]      VarInfo     Pointer to structure containing variable information.

  @retval EFI_SUCCESS               Found the specified variable.
  @retval EFI_INVALID_PARAMETER     VarInfo is NULL or both VarInfo->Address and
                                    VarInfo->Offset are invalid.
  @retval EFI_NOT_FOUND             The specified variable could not be found.

**/
EFI_STATUS
EFIAPI
ProtectedVariableLibGetByInfo (
  IN  OUT PROTECTED_VARIABLE_INFO       *VarInfo
  );

/**
  This service retrieves a variable's value using its name and GUID.

  Read the specified variable from the UEFI variable store. If the Data
  buffer is too small to hold the contents of the variable, the error
  EFI_BUFFER_TOO_SMALL is returned and DataSize is set to the required buffer
  size to obtain the data.

  @param  VariableName          A pointer to a null-terminated string that is the variable's name.
  @param  VariableGuid          A pointer to an EFI_GUID that is the variable's GUID. The combination of
                                VariableGuid and VariableName must be unique.
  @param  Attributes            If non-NULL, on return, points to the variable's attributes.
  @param  DataSize              On entry, points to the size in bytes of the Data buffer.
                                On return, points to the size of the data returned in Data.
  @param  Data                  Points to the buffer which will hold the returned variable value.
                                May be NULL with a zero DataSize in order to determine the size of the buffer needed.

  @retval EFI_SUCCESS           The variable was read successfully.
  @retval EFI_NOT_FOUND         The variable was be found.
  @retval EFI_BUFFER_TOO_SMALL  The DataSize is too small for the resulting data.
                                DataSize is updated with the size required for
                                the specified variable.
  @retval EFI_INVALID_PARAMETER VariableName, VariableGuid, DataSize or Data is NULL.
  @retval EFI_DEVICE_ERROR      The variable could not be retrieved because of a device error.

**/
EFI_STATUS
EFIAPI
ProtectedVariableLibGetByName (
  IN      CONST  CHAR16                   *VariableName,
  IN      CONST  EFI_GUID                 *VariableGuid,
      OUT UINT32                          *Attributes,
  IN  OUT UINTN                           *DataSize,
      OUT VOID                            *Data OPTIONAL
  );

/**

  Retrieve plain data, if encrypted, of given variable.

  If variable encryption is employed, this function will initiate a SMM request
  to get the plain data. Due to security consideration, the decryption can only
  be done in SMM environment.

  @param[in]      Variable           Pointer to header of a Variable.
  @param[out]     Data               Pointer to plain data of the given variable.
  @param[in, out] DataSize           Size of data returned or data buffer needed.
  @param[in]      AuthFlag           Auth-variable indicator.

  @retval EFI_SUCCESS                Found the specified variable.
  @retval EFI_INVALID_PARAMETER      Invalid parameter.
  @retval EFI_NOT_FOUND              The specified variable could not be found.
  @retval EFI_BUFFER_TOO_SMALL       If *DataSize is smaller than needed.

**/
EFI_STATUS
EFIAPI
ProtectedVariableLibGetByBuffer (
  IN      VARIABLE_HEADER                   *Variable,
  IN  OUT VOID                              *Data,
  IN  OUT UINT32                            *DataSize,
  IN      BOOLEAN                           AuthFlag
  );

/**

  Prepare for variable update.

  This is needed only once during current boot to mitigate replay attack. Its
  major job is to advance RPMC (Replay Protected Monotonic Counter).

  @retval EFI_SUCCESS             Variable is ready to update hereafter.
  @retval EFI_UNSUPPORTED         Updating variable is not supported.
  @retval EFI_DEVICE_ERROR        Error in advancing RPMC.

**/
EFI_STATUS
EFIAPI
ProtectedVariableLibWriteInit (
  VOID
  );

/**

  Update a variable with protection provided by this library.

  If variable encryption is employed, the new variable data will be encrypted
  before being written to NV variable storage.

  A special variable, called "MetaDataHmacVar", will always be updated along
  with variable being updated to reflect the changes (HMAC value) of all
  protected valid variables. The only exceptions, currently, are variable
  "MetaDataHmacVar" itself and variable "VarErrorLog".

  The buffer passed by NewVariable must be double of maximum variable size,
  which allows to pass the "MetaDataHmacVar" back to caller along with encrypted
  new variable data, if any. This can make sure the new variable data and
  "MetaDataHmacVar" can be written at almost the same time to reduce the chance
  of compromising the integrity.

  If *NewVariableSize is zero, it means to delete variable passed by CurrVariable
  and/or CurrVariableInDel. "MetaDataHmacVar" will be updated as well in such
  case because of less variables in storage. NewVariable should be always passed
  in to convey new "MetaDataHmacVar" back.

  @param[in,out]  CurrVariable        Variable to be updated. It's NULL if
                                      adding a new variable.
  @param[in,out]  CurrVariableInDel   In-delete-transition copy of updating variable.
  @param[in]      NewVariable         Buffer of new variable data.
  @param[out]     NewVariable         Buffer of "MetaDataHmacVar" and new
                                      variable (encrypted).
  @param[in]      NewVariableSize     Size of NewVariable.
  @param[out]     NewVariableSize     Size of (encrypted) NewVariable and
                                      "MetaDataHmacVar".

  @retval EFI_SUCCESS             The variable is updated with protection successfully.
  @retval EFI_INVALID_PARAMETER   NewVariable is NULL.
  @retval EFI_NOT_FOUND           Information missing to finish the operation.
  @retval EFI_ABORTED             Failed to encrypt variable or calculate HMAC.
  @retval EFI_NOT_READY           The RPMC device is not yet initialized.
  @retval EFI_DEVICE_ERROR        The RPMC device has error in updating.
  @retval EFI_ACCESS_DENIED       The given variable is not allowed to update.
                                  Currently this only happens on updating
                                  "MetaDataHmacVar" from code outside of this
                                  library.

**/
EFI_STATUS
EFIAPI
ProtectedVariableLibUpdate (
  IN  OUT VARIABLE_HEADER             *CurrVariable OPTIONAL,
  IN  OUT VARIABLE_HEADER             *CurrVariableInDel OPTIONAL,
  IN  OUT VARIABLE_HEADER             *NewVariable,
  IN  OUT UINTN                       *NewVariableSize
  );

/**

  Finalize a variable updating after it's written to NV variable storage
  successfully.

  This usually includes works like increasing RPMC, synchronizing local cache,
  updating new position of "MetaDataHmacVar", deleting old copy of "MetaDataHmacVar"
  completely, etc.

  @param[in]      NewVariable       Buffer of new variables and MetaDataHmacVar.
  @param[in]      VariableSize      Size of buffer pointed by NewVariable.
  @param[in]      Offset            Offset to NV variable storage from where the new
                                    variable and MetaDataHmacVar have been written.

  @retval EFI_SUCCESS         No problem in winding up the variable write operation.
  @retval Others              Failed to updating state of old copy of updated
                              variable, or failed to increase RPMC, etc.

**/
EFI_STATUS
EFIAPI
ProtectedVariableLibWriteFinal (
  IN  VARIABLE_HEADER         *NewVariable,
  IN  UINTN                   VariableSize,
  IN  UINT64                  StoreIndex
  );

/**
  Return the request variable name and GUID as per StoreIndex.

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
ProtectedVariableLibFind (
  IN  OUT PROTECTED_VARIABLE_INFO       *VarInfo
  );

/**
  Return the next variable name and GUID.

  This function is called multiple times to retrieve the VariableName
  and VariableGuid of all variables currently available in the system.
  On each call, the previous results are passed into the interface,
  and, on return, the interface returns the data for the next
  interface. When the entire variable list has been returned,
  EFI_NOT_FOUND is returned.

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
ProtectedVariableLibFindNext (
  IN OUT UINTN                              *VariableNameSize,
  IN OUT CHAR16                             *VariableName,
  IN OUT EFI_GUID                           *VariableGuid
  );

/**
  Find variable via information in data structure PROTECTED_VARIABLE_INFO.

   If VarInfo->StoreIndex is given and valid, always used it to search variable
   in store. Otherwise, search the variable via variable name and guid pointed
   by VarInfo->Header.VariableName and VarInfo->Header.VendorGuid.

  @param VarInfo    Pointer to data containing variable information.

  @return EFI_SUCCESS           Found the variable.
  @return EFI_INVALID_PARAMETER No valid variable information is given.
  @return EFI_NOT_FOUND         The given variable was not found or no more
                                variables available.
**/
EFI_STATUS
EFIAPI
ProtectedVariableLibFindNextEx (
  IN  OUT PROTECTED_VARIABLE_INFO       *VarInfo
  );

/**
  Refresh variable information changed by variable service.

  @param Variable         Pointer to buffer of the updated variable.
  @param VariableSize     Size of variable pointed by Variable.
  @param StoreIndex       New index of the variable in store.
  @param RefreshData      Flag to indicate if the variable has been updated.

  @return EFI_SUCCESS     No error occurred in updating.
  @return EFI_NOT_FOUND   The given variable was not found in
                          ProtectedVariableLib.
**/
EFI_STATUS
EFIAPI
ProtectedVariableLibRefresh (
  IN  VARIABLE_HEADER         *Variable,
  IN  UINTN                   VariableSize,
  IN  UINT64                  StoreIndex,
  IN  BOOLEAN                 RefreshData
  );

/**
  Refresh variable information changed by variable service.

  @param Buffer           Pointer to a pointer of buffer.
  @param NumElements      Pointer to number of elements in list.


  @return EFI_SUCCESS     Successfully retrieved sorted list.
  @return others          Unsuccessful.

**/
EFI_STATUS
EFIAPI
ProtectedVariableLibGetSortedList (
  IN  OUT  EFI_PHYSICAL_ADDRESS     **Buffer,
  IN  OUT  UINTN                    *NumElements
  );

/**

  Determine if the variable is the HMAC variable

  @param VariableName   Pointer to variable name.

  @return TRUE      Variable is HMAC variable
  @return FALSE     Variable is not HMAC variable

**/
BOOLEAN
ProtectedVariableLibIsHmac (
  IN CHAR16             *VariableName
  );
#endif
