/** @file
  This library is only intended to be used by DXE modules that need save
  confidential information to LockBox and get it by PEI modules in S3 phase.

Copyright (c) 2010 - 2019, Intel Corporation. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _LOCK_BOX_LIB_H_
#define _LOCK_BOX_LIB_H_

/**
  This function will save confidential information to lockbox.

  @param Guid       the guid to identify the confidential information
  @param Buffer     the address of the confidential information
  @param Length     the length of the confidential information

  @retval RETURN_SUCCESS            the information is saved successfully.
  @retval RETURN_INVALID_PARAMETER  the Guid is NULL, or Buffer is NULL, or Length is 0
  @retval RETURN_ALREADY_STARTED    the requested GUID already exist.
  @retval RETURN_OUT_OF_RESOURCES   no enough resource to save the information.
  @retval RETURN_ACCESS_DENIED      it is too late to invoke this interface
  @retval RETURN_NOT_STARTED        it is too early to invoke this interface
  @retval RETURN_UNSUPPORTED        the service is not supported by implementaion.
**/
RETURN_STATUS
EFIAPI
SaveLockBox (
  IN  GUID   *Guid,
  IN  VOID   *Buffer,
  IN  UINTN  Length
  );

/**
  This function will set lockbox attributes.

  @param Guid       the guid to identify the confidential information
  @param Attributes the attributes of the lockbox

  @retval RETURN_SUCCESS            the information is saved successfully.
  @retval RETURN_INVALID_PARAMETER  attributes is invalid.
  @retval RETURN_NOT_FOUND          the requested GUID not found.
  @retval RETURN_ACCESS_DENIED      it is too late to invoke this interface
  @retval RETURN_NOT_STARTED        it is too early to invoke this interface
  @retval RETURN_UNSUPPORTED        the service is not supported by implementaion.
**/
RETURN_STATUS
EFIAPI
SetLockBoxAttributes (
  IN  GUID    *Guid,
  IN  UINT64  Attributes
  );

//
// With this flag, this LockBox can be restored to this Buffer
// with RestoreAllLockBoxInPlace()
//
#define LOCK_BOX_ATTRIBUTE_RESTORE_IN_PLACE  BIT0
//
// With this flag, this LockBox can be restored in S3 resume only.
// This LockBox can not be restored after SmmReadyToLock in normal boot
// and after EndOfS3Resume in S3 resume.
// It can not be set together with LOCK_BOX_ATTRIBUTE_RESTORE_IN_PLACE.
//
#define LOCK_BOX_ATTRIBUTE_RESTORE_IN_S3_ONLY  BIT1

/**
  This function will update confidential information to lockbox.

  @param Guid   the guid to identify the original confidential information
  @param Offset the offset of the original confidential information
  @param Buffer the address of the updated confidential information
  @param Length the length of the updated confidential information

  @retval RETURN_SUCCESS            the information is saved successfully.
  @retval RETURN_INVALID_PARAMETER  the Guid is NULL, or Buffer is NULL, or Length is 0.
  @retval RETURN_NOT_FOUND          the requested GUID not found.
  @retval RETURN_BUFFER_TOO_SMALL   for lockbox without attribute LOCK_BOX_ATTRIBUTE_RESTORE_IN_S3_ONLY,
                                    the original buffer to too small to hold new information.
  @retval RETURN_OUT_OF_RESOURCES   for lockbox with attribute LOCK_BOX_ATTRIBUTE_RESTORE_IN_S3_ONLY,
                                    no enough resource to save the information.
  @retval RETURN_ACCESS_DENIED      it is too late to invoke this interface
  @retval RETURN_NOT_STARTED        it is too early to invoke this interface
  @retval RETURN_UNSUPPORTED        the service is not supported by implementaion.
**/
RETURN_STATUS
EFIAPI
UpdateLockBox (
  IN  GUID   *Guid,
  IN  UINTN  Offset,
  IN  VOID   *Buffer,
  IN  UINTN  Length
  );

/**
  This function will restore confidential information from lockbox.

  @param Guid   the guid to identify the confidential information
  @param Buffer the address of the restored confidential information
                NULL means restored to original address, Length MUST be NULL at same time.
  @param Length the length of the restored confidential information

  @retval RETURN_SUCCESS            the information is restored successfully.
  @retval RETURN_INVALID_PARAMETER  the Guid is NULL, or one of Buffer and Length is NULL.
  @retval RETURN_WRITE_PROTECTED    Buffer and Length are NULL, but the LockBox has no
                                    LOCK_BOX_ATTRIBUTE_RESTORE_IN_PLACE attribute.
  @retval RETURN_BUFFER_TOO_SMALL   the Length is too small to hold the confidential information.
  @retval RETURN_NOT_FOUND          the requested GUID not found.
  @retval RETURN_NOT_STARTED        it is too early to invoke this interface
  @retval RETURN_ACCESS_DENIED      not allow to restore to the address
  @retval RETURN_UNSUPPORTED        the service is not supported by implementaion.
**/
RETURN_STATUS
EFIAPI
RestoreLockBox (
  IN  GUID       *Guid,
  IN  VOID       *Buffer  OPTIONAL,
  IN  OUT UINTN  *Length  OPTIONAL
  );

/**
  This function will restore confidential information from all lockbox which have RestoreInPlace attribute.

  @retval RETURN_SUCCESS            the information is restored successfully.
  @retval RETURN_NOT_STARTED        it is too early to invoke this interface
  @retval RETURN_UNSUPPORTED        the service is not supported by implementaion.
**/
RETURN_STATUS
EFIAPI
RestoreAllLockBoxInPlace (
  VOID
  );

#endif
