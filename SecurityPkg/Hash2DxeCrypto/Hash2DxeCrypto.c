/** @file
  This module implements Hash2 Protocol.

(C) Copyright 2015 Hewlett-Packard Development Company, L.P.<BR>
Copyright (c) 2015 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Protocol/Hash2.h>
#include <Library/BaseLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseCryptLib.h>

#include "Driver.h"

/**
  Retrieves the size, in bytes, of the context buffer required for hash operations.

  If this interface is not supported, then return zero.

  @return  The size, in bytes, of the context buffer required for hash operations.
  @retval  0   This interface is not supported.

**/
typedef
UINTN
(EFIAPI *EFI_HASH_GET_CONTEXT_SIZE)(
  VOID
  );

/**
  Initializes user-supplied memory pointed by Sha1Context as hash context for
  subsequent use.

  If HashContext is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[out]  HashContext  Pointer to Hashcontext being initialized.

  @retval TRUE   Hash context initialization succeeded.
  @retval FALSE  Hash context initialization failed.
  @retval FALSE  This interface is not supported.

**/
typedef
BOOLEAN
(EFIAPI *EFI_HASH_INIT)(
  OUT  VOID  *HashContext
  );

/**
  Digests the input data and updates Hash context.

  This function performs Hash digest on a data buffer of the specified size.
  It can be called multiple times to compute the digest of long or discontinuous data streams.
  Hash context should be already correctly initialized by HashInit(), and should not be finalized
  by HashFinal(). Behavior with invalid context is undefined.

  If HashContext is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in, out]  HashContext  Pointer to the Hash context.
  @param[in]       Data         Pointer to the buffer containing the data to be hashed.
  @param[in]       DataSize     Size of Data buffer in bytes.

  @retval TRUE   SHA-1 data digest succeeded.
  @retval FALSE  SHA-1 data digest failed.
  @retval FALSE  This interface is not supported.

**/
typedef
BOOLEAN
(EFIAPI *EFI_HASH_UPDATE)(
  IN OUT  VOID        *HashContext,
  IN      CONST VOID  *Data,
  IN      UINTN       DataSize
  );

/**
  Completes computation of the Hash digest value.

  This function completes hash computation and retrieves the digest value into
  the specified memory. After this function has been called, the Hash context cannot
  be used again.
  Hash context should be already correctly initialized by HashInit(), and should not be
  finalized by HashFinal(). Behavior with invalid Hash context is undefined.

  If HashContext is NULL, then return FALSE.
  If HashValue is NULL, then return FALSE.
  If this interface is not supported, then return FALSE.

  @param[in, out]  HashContext  Pointer to the Hash context.
  @param[out]      HashValue    Pointer to a buffer that receives the Hash digest
                                value.

  @retval TRUE   Hash digest computation succeeded.
  @retval FALSE  Hash digest computation failed.
  @retval FALSE  This interface is not supported.

**/
typedef
BOOLEAN
(EFIAPI *EFI_HASH_FINAL)(
  IN OUT  VOID   *HashContext,
  OUT     UINT8  *HashValue
  );

typedef struct {
  EFI_GUID                     *Guid;
  UINT32                       HashSize;
  EFI_HASH_GET_CONTEXT_SIZE    GetContextSize;
  EFI_HASH_INIT                Init;
  EFI_HASH_UPDATE              Update;
  EFI_HASH_FINAL               Final;
} EFI_HASH_INFO;

EFI_HASH_INFO  mHashInfo[] = {
  { &gEfiHashAlgorithmSha256Guid, sizeof (EFI_SHA256_HASH2), Sha256GetContextSize, Sha256Init, Sha256Update, Sha256Final },
  { &gEfiHashAlgorithmSha384Guid, sizeof (EFI_SHA384_HASH2), Sha384GetContextSize, Sha384Init, Sha384Update, Sha384Final },
  { &gEfiHashAlgorithmSha512Guid, sizeof (EFI_SHA512_HASH2), Sha512GetContextSize, Sha512Init, Sha512Update, Sha512Final },
};

/**
  Returns the size of the hash which results from a specific algorithm.

  @param[in]  This                  Points to this instance of EFI_HASH2_PROTOCOL.
  @param[in]  HashAlgorithm         Points to the EFI_GUID which identifies the algorithm to use.
  @param[out] HashSize              Holds the returned size of the algorithm's hash.

  @retval EFI_SUCCESS           Hash size returned successfully.
  @retval EFI_INVALID_PARAMETER This or HashSize is NULL.
  @retval EFI_UNSUPPORTED       The algorithm specified by HashAlgorithm is not supported by this driver
                                or HashAlgorithm is null.

**/
EFI_STATUS
EFIAPI
BaseCrypto2GetHashSize (
  IN  CONST EFI_HASH2_PROTOCOL  *This,
  IN  CONST EFI_GUID            *HashAlgorithm,
  OUT UINTN                     *HashSize
  );

/**
  Creates a hash for the specified message text. The hash is not extendable.
  The output is final with any algorithm-required padding added by the function.

  @param[in]  This          Points to this instance of EFI_HASH2_PROTOCOL.
  @param[in]  HashAlgorithm Points to the EFI_GUID which identifies the algorithm to use.
  @param[in]  Message       Points to the start of the message.
  @param[in]  MessageSize   The size of Message, in bytes.
  @param[in,out]  Hash      On input, points to a caller-allocated buffer of the size
                              returned by GetHashSize() for the specified HashAlgorithm.
                            On output, the buffer holds the resulting hash computed from the message.

  @retval EFI_SUCCESS           Hash returned successfully.
  @retval EFI_INVALID_PARAMETER This or Hash is NULL.
  @retval EFI_UNSUPPORTED       The algorithm specified by HashAlgorithm is not supported by this driver
                                or HashAlgorithm is Null.
  @retval EFI_OUT_OF_RESOURCES  Some resource required by the function is not available
                                or MessageSize is greater than platform maximum.

**/
EFI_STATUS
EFIAPI
BaseCrypto2Hash (
  IN CONST EFI_HASH2_PROTOCOL  *This,
  IN CONST EFI_GUID            *HashAlgorithm,
  IN CONST UINT8               *Message,
  IN UINTN                     MessageSize,
  IN OUT EFI_HASH2_OUTPUT      *Hash
  );

/**
  This function must be called to initialize a digest calculation to be subsequently performed using the
  EFI_HASH2_PROTOCOL functions HashUpdate() and HashFinal().

  @param[in]  This          Points to this instance of EFI_HASH2_PROTOCOL.
  @param[in]  HashAlgorithm Points to the EFI_GUID which identifies the algorithm to use.

  @retval EFI_SUCCESS           Initialized successfully.
  @retval EFI_INVALID_PARAMETER This is NULL.
  @retval EFI_UNSUPPORTED       The algorithm specified by HashAlgorithm is not supported by this driver
                                or HashAlgorithm is Null.
  @retval EFI_OUT_OF_RESOURCES  Process failed due to lack of required resource.
  @retval EFI_ALREADY_STARTED   This function is called when the operation in progress is still in processing Hash(),
                                or HashInit() is already called before and not terminated by HashFinal() yet on the same instance.

**/
EFI_STATUS
EFIAPI
BaseCrypto2HashInit (
  IN CONST EFI_HASH2_PROTOCOL  *This,
  IN CONST EFI_GUID            *HashAlgorithm
  );

/**
  Updates the hash of a computation in progress by adding a message text.

  @param[in]  This          Points to this instance of EFI_HASH2_PROTOCOL.
  @param[in]  Message       Points to the start of the message.
  @param[in]  MessageSize   The size of Message, in bytes.

  @retval EFI_SUCCESS           Digest in progress updated successfully.
  @retval EFI_INVALID_PARAMETER This or Hash is NULL.
  @retval EFI_OUT_OF_RESOURCES  Some resource required by the function is not available
                                or MessageSize is greater than platform maximum.
  @retval EFI_NOT_READY         This call was not preceded by a valid call to HashInit(),
                                or the operation in progress was terminated by a call to Hash() or HashFinal() on the same instance.

**/
EFI_STATUS
EFIAPI
BaseCrypto2HashUpdate (
  IN CONST EFI_HASH2_PROTOCOL  *This,
  IN CONST UINT8               *Message,
  IN UINTN                     MessageSize
  );

/**
  Finalizes a hash operation in progress and returns calculation result.
  The output is final with any necessary padding added by the function.
  The hash may not be further updated or extended after HashFinal().

  @param[in]  This          Points to this instance of EFI_HASH2_PROTOCOL.
  @param[in,out]  Hash      On input, points to a caller-allocated buffer of the size
                              returned by GetHashSize() for the specified HashAlgorithm specified in preceding HashInit().
                            On output, the buffer holds the resulting hash computed from the message.

  @retval EFI_SUCCESS           Hash returned successfully.
  @retval EFI_INVALID_PARAMETER This or Hash is NULL.
  @retval EFI_NOT_READY         This call was not preceded by a valid call to HashInit() and at least one call to HashUpdate(),
                                or the operation in progress was canceled by a call to Hash() on the same instance.

**/
EFI_STATUS
EFIAPI
BaseCrypto2HashFinal (
  IN CONST EFI_HASH2_PROTOCOL  *This,
  IN OUT EFI_HASH2_OUTPUT      *Hash
  );

EFI_HASH2_PROTOCOL  mHash2Protocol = {
  BaseCrypto2GetHashSize,
  BaseCrypto2Hash,
  BaseCrypto2HashInit,
  BaseCrypto2HashUpdate,
  BaseCrypto2HashFinal,
};

/**
  Returns hash information.

  @param[in]  HashAlgorithm         Points to the EFI_GUID which identifies the algorithm to use.

  @return Hash information.
**/
EFI_HASH_INFO *
GetHashInfo (
  IN CONST EFI_GUID  *HashAlgorithm
  )
{
  UINTN  Index;

  for (Index = 0; Index < sizeof (mHashInfo)/sizeof (mHashInfo[0]); Index++) {
    if (CompareGuid (HashAlgorithm, mHashInfo[Index].Guid)) {
      return &mHashInfo[Index];
    }
  }

  return NULL;
}

/**
  Returns the size of the hash which results from a specific algorithm.

  @param[in]  This                  Points to this instance of EFI_HASH2_PROTOCOL.
  @param[in]  HashAlgorithm         Points to the EFI_GUID which identifies the algorithm to use.
  @param[out] HashSize              Holds the returned size of the algorithm's hash.

  @retval EFI_SUCCESS           Hash size returned successfully.
  @retval EFI_INVALID_PARAMETER This or HashSize is NULL.
  @retval EFI_UNSUPPORTED       The algorithm specified by HashAlgorithm is not supported by this driver
                                or HashAlgorithm is null.

**/
EFI_STATUS
EFIAPI
BaseCrypto2GetHashSize (
  IN  CONST EFI_HASH2_PROTOCOL  *This,
  IN  CONST EFI_GUID            *HashAlgorithm,
  OUT UINTN                     *HashSize
  )
{
  EFI_HASH_INFO  *HashInfo;

  if ((This == NULL) || (HashSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (HashAlgorithm == NULL) {
    return EFI_UNSUPPORTED;
  }

  HashInfo = GetHashInfo (HashAlgorithm);
  if (HashInfo == NULL) {
    return EFI_UNSUPPORTED;
  }

  *HashSize = HashInfo->HashSize;
  return EFI_SUCCESS;
}

/**
  Creates a hash for the specified message text. The hash is not extendable.
  The output is final with any algorithm-required padding added by the function.

  @param[in]  This          Points to this instance of EFI_HASH2_PROTOCOL.
  @param[in]  HashAlgorithm Points to the EFI_GUID which identifies the algorithm to use.
  @param[in]  Message       Points to the start of the message.
  @param[in]  MessageSize   The size of Message, in bytes.
  @param[in,out]  Hash      On input, points to a caller-allocated buffer of the size
                              returned by GetHashSize() for the specified HashAlgorithm.
                            On output, the buffer holds the resulting hash computed from the message.

  @retval EFI_SUCCESS           Hash returned successfully.
  @retval EFI_INVALID_PARAMETER This or Hash is NULL.
  @retval EFI_UNSUPPORTED       The algorithm specified by HashAlgorithm is not supported by this driver
                                or HashAlgorithm is Null.
  @retval EFI_OUT_OF_RESOURCES  Some resource required by the function is not available
                                or MessageSize is greater than platform maximum.

**/
EFI_STATUS
EFIAPI
BaseCrypto2Hash (
  IN CONST EFI_HASH2_PROTOCOL  *This,
  IN CONST EFI_GUID            *HashAlgorithm,
  IN CONST UINT8               *Message,
  IN UINTN                     MessageSize,
  IN OUT EFI_HASH2_OUTPUT      *Hash
  )
{
  EFI_HASH_INFO        *HashInfo;
  VOID                 *HashCtx;
  UINTN                CtxSize;
  BOOLEAN              Ret;
  EFI_STATUS           Status;
  HASH2_INSTANCE_DATA  *Instance;

  Status = EFI_SUCCESS;

  if ((This == NULL) || (Hash == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if (HashAlgorithm == NULL) {
    return EFI_UNSUPPORTED;
  }

  HashInfo = GetHashInfo (HashAlgorithm);
  if (HashInfo == NULL) {
    return EFI_UNSUPPORTED;
  }

  Instance = HASH2_INSTANCE_DATA_FROM_THIS (This);
  if (Instance->HashContext != NULL) {
    FreePool (Instance->HashContext);
  }

  Instance->HashInfoContext = NULL;
  Instance->HashContext     = NULL;

  //
  // Start hash sequence
  //
  CtxSize = HashInfo->GetContextSize ();
  if (CtxSize == 0) {
    return EFI_UNSUPPORTED;
  }

  HashCtx = AllocatePool (CtxSize);
  if (HashCtx == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Ret = HashInfo->Init (HashCtx);
  if (!Ret) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  //
  // Setup the context
  //
  Instance->HashContext     = HashCtx;
  Instance->HashInfoContext = HashInfo;

  Ret = HashInfo->Update (HashCtx, Message, MessageSize);
  if (!Ret) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  Ret = HashInfo->Final (HashCtx, (UINT8 *)Hash->Sha1Hash);
  if (!Ret) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

Done:
  //
  // Cleanup the context
  //
  FreePool (HashCtx);
  Instance->HashInfoContext = NULL;
  Instance->HashContext     = NULL;
  return Status;
}

/**
  This function must be called to initialize a digest calculation to be subsequently performed using the
  EFI_HASH2_PROTOCOL functions HashUpdate() and HashFinal().

  @param[in]  This          Points to this instance of EFI_HASH2_PROTOCOL.
  @param[in]  HashAlgorithm Points to the EFI_GUID which identifies the algorithm to use.

  @retval EFI_SUCCESS           Initialized successfully.
  @retval EFI_INVALID_PARAMETER This is NULL.
  @retval EFI_UNSUPPORTED       The algorithm specified by HashAlgorithm is not supported by this driver
                                or HashAlgorithm is Null.
  @retval EFI_OUT_OF_RESOURCES  Process failed due to lack of required resource.
  @retval EFI_ALREADY_STARTED   This function is called when the operation in progress is still in processing Hash(),
                                or HashInit() is already called before and not terminated by HashFinal() yet on the same instance.

**/
EFI_STATUS
EFIAPI
BaseCrypto2HashInit (
  IN CONST EFI_HASH2_PROTOCOL  *This,
  IN CONST EFI_GUID            *HashAlgorithm
  )
{
  EFI_HASH_INFO        *HashInfo;
  VOID                 *HashCtx;
  UINTN                CtxSize;
  BOOLEAN              Ret;
  HASH2_INSTANCE_DATA  *Instance;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (HashAlgorithm == NULL) {
    return EFI_UNSUPPORTED;
  }

  HashInfo = GetHashInfo (HashAlgorithm);
  if (HashInfo == NULL) {
    return EFI_UNSUPPORTED;
  }

  //
  // Consistency Check
  //
  Instance = HASH2_INSTANCE_DATA_FROM_THIS (This);
  if ((Instance->HashContext != NULL) || (Instance->HashInfoContext != NULL)) {
    return EFI_ALREADY_STARTED;
  }

  //
  // Start hash sequence
  //
  CtxSize = HashInfo->GetContextSize ();
  if (CtxSize == 0) {
    return EFI_UNSUPPORTED;
  }

  HashCtx = AllocatePool (CtxSize);
  if (HashCtx == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Ret = HashInfo->Init (HashCtx);
  if (!Ret) {
    FreePool (HashCtx);
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Setup the context
  //
  Instance->HashContext     = HashCtx;
  Instance->HashInfoContext = HashInfo;
  Instance->Updated         = FALSE;

  return EFI_SUCCESS;
}

/**
  Updates the hash of a computation in progress by adding a message text.

  @param[in]  This          Points to this instance of EFI_HASH2_PROTOCOL.
  @param[in]  Message       Points to the start of the message.
  @param[in]  MessageSize   The size of Message, in bytes.

  @retval EFI_SUCCESS           Digest in progress updated successfully.
  @retval EFI_INVALID_PARAMETER This or Hash is NULL.
  @retval EFI_OUT_OF_RESOURCES  Some resource required by the function is not available
                                or MessageSize is greater than platform maximum.
  @retval EFI_NOT_READY         This call was not preceded by a valid call to HashInit(),
                                or the operation in progress was terminated by a call to Hash() or HashFinal() on the same instance.

**/
EFI_STATUS
EFIAPI
BaseCrypto2HashUpdate (
  IN CONST EFI_HASH2_PROTOCOL  *This,
  IN CONST UINT8               *Message,
  IN UINTN                     MessageSize
  )
{
  EFI_HASH_INFO        *HashInfo;
  VOID                 *HashCtx;
  BOOLEAN              Ret;
  HASH2_INSTANCE_DATA  *Instance;

  if (This == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Consistency Check
  //
  Instance = HASH2_INSTANCE_DATA_FROM_THIS (This);
  if ((Instance->HashContext == NULL) || (Instance->HashInfoContext == NULL)) {
    return EFI_NOT_READY;
  }

  HashInfo = Instance->HashInfoContext;
  HashCtx  = Instance->HashContext;

  Ret = HashInfo->Update (HashCtx, Message, MessageSize);
  if (!Ret) {
    return EFI_OUT_OF_RESOURCES;
  }

  Instance->Updated = TRUE;

  return EFI_SUCCESS;
}

/**
  Finalizes a hash operation in progress and returns calculation result.
  The output is final with any necessary padding added by the function.
  The hash may not be further updated or extended after HashFinal().

  @param[in]  This          Points to this instance of EFI_HASH2_PROTOCOL.
  @param[in,out]  Hash      On input, points to a caller-allocated buffer of the size
                              returned by GetHashSize() for the specified HashAlgorithm specified in preceding HashInit().
                            On output, the buffer holds the resulting hash computed from the message.

  @retval EFI_SUCCESS           Hash returned successfully.
  @retval EFI_INVALID_PARAMETER This or Hash is NULL.
  @retval EFI_NOT_READY         This call was not preceded by a valid call to HashInit() and at least one call to HashUpdate(),
                                or the operation in progress was canceled by a call to Hash() on the same instance.

**/
EFI_STATUS
EFIAPI
BaseCrypto2HashFinal (
  IN CONST EFI_HASH2_PROTOCOL  *This,
  IN OUT EFI_HASH2_OUTPUT      *Hash
  )
{
  EFI_HASH_INFO        *HashInfo;
  VOID                 *HashCtx;
  BOOLEAN              Ret;
  HASH2_INSTANCE_DATA  *Instance;

  if ((This == NULL) || (Hash == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Consistency Check
  //
  Instance = HASH2_INSTANCE_DATA_FROM_THIS (This);
  if ((Instance->HashContext == NULL) || (Instance->HashInfoContext == NULL) ||
      (!Instance->Updated))
  {
    return EFI_NOT_READY;
  }

  HashInfo = Instance->HashInfoContext;
  HashCtx  = Instance->HashContext;

  Ret = HashInfo->Final (HashCtx, (UINT8 *)Hash->Sha1Hash);

  //
  // Cleanup the context
  //
  FreePool (HashCtx);
  Instance->HashInfoContext = NULL;
  Instance->HashContext     = NULL;
  Instance->Updated         = FALSE;

  if (!Ret) {
    return EFI_OUT_OF_RESOURCES;
  }

  return EFI_SUCCESS;
}
