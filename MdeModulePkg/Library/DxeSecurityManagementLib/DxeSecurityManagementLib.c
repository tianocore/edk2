/** @file
  Provides generic security measurement functions for DXE module.

Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Protocol/LoadFile.h>
#include <Library/DebugLib.h>
#include <Library/DxeServicesLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/SecurityManagementLib.h>
#include <Library/DevicePathLib.h>
#include <Library/UefiBootServicesTableLib.h>

#define SECURITY_HANDLER_TABLE_SIZE  0x10

//
// Secruity Operation on Image and none Image.
//
#define EFI_AUTH_IMAGE_OPERATION_MASK       (EFI_AUTH_OPERATION_VERIFY_IMAGE \
                                              | EFI_AUTH_OPERATION_DEFER_IMAGE_LOAD \
                                              | EFI_AUTH_OPERATION_MEASURE_IMAGE)
#define EFI_AUTH_NONE_IMAGE_OPERATION_MASK  (EFI_AUTH_OPERATION_CONNECT_POLICY \
                                              | EFI_AUTH_OPERATION_AUTHENTICATION_STATE)

typedef struct {
  UINT32                                        SecurityOperation;
  SECURITY_FILE_AUTHENTICATION_STATE_HANDLER    SecurityHandler;
} SECURITY_INFO;

typedef struct {
  UINT32                                   Security2Operation;
  SECURITY2_FILE_AUTHENTICATION_HANDLER    Security2Handler;
} SECURITY2_INFO;

UINT32         mCurrentAuthOperation       = 0;
UINT32         mNumberOfSecurityHandler    = 0;
UINT32         mMaxNumberOfSecurityHandler = 0;
SECURITY_INFO  *mSecurityTable             = NULL;

UINT32          mCurrentAuthOperation2       = 0;
UINT32          mNumberOfSecurity2Handler    = 0;
UINT32          mMaxNumberOfSecurity2Handler = 0;
SECURITY2_INFO  *mSecurity2Table             = NULL;

/**
  Reallocates more global memory to store the registered Handler list.

  @retval  RETURN_SUCCESS            Reallocate memory successfully.
  @retval  RETURN_OUT_OF_RESOURCES   No enough memory to allocated.
**/
RETURN_STATUS
EFIAPI
ReallocateSecurityHandlerTable (
  VOID
  )
{
  //
  // Reallocate memory for security info structure.
  //
  mSecurityTable = ReallocatePool (
                     mMaxNumberOfSecurityHandler * sizeof (SECURITY_INFO),
                     (mMaxNumberOfSecurityHandler + SECURITY_HANDLER_TABLE_SIZE) * sizeof (SECURITY_INFO),
                     mSecurityTable
                     );

  //
  // No enough resource is allocated.
  //
  if (mSecurityTable == NULL) {
    return RETURN_OUT_OF_RESOURCES;
  }

  //
  // Increase max handler number
  //
  mMaxNumberOfSecurityHandler = mMaxNumberOfSecurityHandler + SECURITY_HANDLER_TABLE_SIZE;
  return RETURN_SUCCESS;
}

/**
 Check whether an operation is valid according to the requirement of current operation,
 which must make sure that the measure image operation is the last one.

 @param CurrentAuthOperation  Current operation.
 @param CheckAuthOperation    Operation to be checked.

 @retval  TRUE   Operation is valid for current operation.
 @retval  FALSE  Operation is invalid for current operation.
**/
BOOLEAN
CheckAuthenticationOperation (
  IN  UINT32  CurrentAuthOperation,
  IN  UINT32  CheckAuthOperation
  )
{
  //
  // Make sure new auth operation can be recognized.
  //
  ASSERT ((CheckAuthOperation & ~(EFI_AUTH_IMAGE_OPERATION_MASK | EFI_AUTH_OPERATION_AUTHENTICATION_STATE | EFI_AUTH_OPERATION_IMAGE_REQUIRED)) == 0);

  //
  // When current operation includes measure image operation,
  // only another measure image operation or none operation will be allowed.
  //
  if ((CurrentAuthOperation & EFI_AUTH_OPERATION_MEASURE_IMAGE) == EFI_AUTH_OPERATION_MEASURE_IMAGE) {
    if (((CheckAuthOperation & EFI_AUTH_OPERATION_MEASURE_IMAGE) == EFI_AUTH_OPERATION_MEASURE_IMAGE) ||
        ((CheckAuthOperation & EFI_AUTH_IMAGE_OPERATION_MASK) == EFI_AUTH_OPERATION_NONE))
    {
      return TRUE;
    } else {
      return FALSE;
    }
  }

  //
  // When current operation doesn't include measure image operation,
  // any new operation will be allowed.
  //
  return TRUE;
}

/**
  Register security measurement handler with its operation type. The different
  handler with the same operation can all be registered.

  If SecurityHandler is NULL, then ASSERT().
  If no enough resources available to register new handler, then ASSERT().
  If AuthenticationOperation is not recongnized, then ASSERT().
  If the previous register handler can't be executed before the later register handler, then ASSERT().

  @param[in]  SecurityHandler           Security measurement service handler to be registered.
  @param[in]  AuthenticationOperation   Operation type is specified for the registered handler.

  @retval EFI_SUCCESS              The handlers were registered successfully.
**/
EFI_STATUS
EFIAPI
RegisterSecurityHandler (
  IN  SECURITY_FILE_AUTHENTICATION_STATE_HANDLER  SecurityHandler,
  IN  UINT32                                      AuthenticationOperation
  )
{
  EFI_STATUS  Status;

  ASSERT (SecurityHandler != NULL);

  //
  // Make sure AuthenticationOperation is valid in the register order.
  //
  ASSERT (CheckAuthenticationOperation (mCurrentAuthOperation, AuthenticationOperation));
  mCurrentAuthOperation = mCurrentAuthOperation | AuthenticationOperation;

  //
  // Check whether the handler lists is enough to store new handler.
  //
  if (mNumberOfSecurityHandler == mMaxNumberOfSecurityHandler) {
    //
    // Allocate more resources for new handler.
    //
    Status = ReallocateSecurityHandlerTable ();
    ASSERT_EFI_ERROR (Status);
  }

  //
  // Register new handler into the handler list.
  //
  mSecurityTable[mNumberOfSecurityHandler].SecurityOperation = AuthenticationOperation;
  mSecurityTable[mNumberOfSecurityHandler].SecurityHandler   = SecurityHandler;
  mNumberOfSecurityHandler++;

  return EFI_SUCCESS;
}

/**
  Execute registered handlers until one returns an error and that error is returned.
  If none of the handlers return an error, then EFI_SUCCESS is returned.

  Before exectue handler, get the image buffer by file device path if a handler
  requires the image file. And return the image buffer to each handler when exectue handler.

  The handlers are executed in same order to their registered order.

  @param[in]  AuthenticationStatus
                           This is the authentication type returned from the Section
                           Extraction protocol. See the Section Extraction Protocol
                           Specification for details on this type.
  @param[in]  FilePath     This is a pointer to the device path of the file that is
                           being dispatched. This will optionally be used for logging.

  @retval EFI_SUCCESS            The file specified by File did authenticate when more
                                 than one security handler services were registered,
                                 or the file did not authenticate when no security
                                 handler service was registered. And the platform policy
                                 dictates that the DXE Core may use File.
  @retval EFI_INVALID_PARAMETER  File is NULL.
  @retval EFI_SECURITY_VIOLATION The file specified by File did not authenticate, and
                                 the platform policy dictates that File should be placed
                                 in the untrusted state. A file may be promoted from
                                 the untrusted to the trusted state at a future time
                                 with a call to the Trust() DXE Service.
  @retval EFI_ACCESS_DENIED      The file specified by File did not authenticate, and
                                 the platform policy dictates that File should not be
                                 used for any purpose.
**/
EFI_STATUS
EFIAPI
ExecuteSecurityHandlers (
  IN  UINT32                          AuthenticationStatus,
  IN  CONST EFI_DEVICE_PATH_PROTOCOL  *FilePath
  )
{
  UINT32                    Index;
  EFI_STATUS                Status;
  UINT32                    HandlerAuthenticationStatus;
  VOID                      *FileBuffer;
  UINTN                     FileSize;
  EFI_HANDLE                Handle;
  EFI_DEVICE_PATH_PROTOCOL  *Node;
  EFI_DEVICE_PATH_PROTOCOL  *FilePathToVerify;

  if (FilePath == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Directly return successfully when no handler is registered.
  //
  if (mNumberOfSecurityHandler == 0) {
    return EFI_SUCCESS;
  }

  Status                      = EFI_SUCCESS;
  FileBuffer                  = NULL;
  FileSize                    = 0;
  HandlerAuthenticationStatus = AuthenticationStatus;
  FilePathToVerify            = (EFI_DEVICE_PATH_PROTOCOL *)FilePath;
  //
  // Run security handler in same order to their registered list
  //
  for (Index = 0; Index < mNumberOfSecurityHandler; Index++) {
    if ((mSecurityTable[Index].SecurityOperation & EFI_AUTH_OPERATION_IMAGE_REQUIRED) == EFI_AUTH_OPERATION_IMAGE_REQUIRED) {
      //
      // Try get file buffer when the handler requires image buffer.
      //
      if (FileBuffer == NULL) {
        Node   = FilePathToVerify;
        Status = gBS->LocateDevicePath (&gEfiLoadFileProtocolGuid, &Node, &Handle);
        //
        // Try to get image by FALSE boot policy for the exact boot file path.
        //
        FileBuffer = GetFileBufferByFilePath (FALSE, FilePath, &FileSize, &AuthenticationStatus);
        if (FileBuffer == NULL) {
          //
          // Try to get image by TRUE boot policy for the inexact boot file path.
          //
          FileBuffer = GetFileBufferByFilePath (TRUE, FilePath, &FileSize, &AuthenticationStatus);
        }

        if ((FileBuffer != NULL) && (!EFI_ERROR (Status))) {
          //
          // LoadFile () may cause the device path of the Handle be updated.
          //
          FilePathToVerify = AppendDevicePath (DevicePathFromHandle (Handle), Node);
        }
      }
    }

    if (FilePathToVerify == NULL) {
      ASSERT (FilePathToVerify != NULL);
      continue;
    }

    Status = mSecurityTable[Index].SecurityHandler (
                                     HandlerAuthenticationStatus,
                                     FilePathToVerify,
                                     FileBuffer,
                                     FileSize
                                     );
    if (EFI_ERROR (Status)) {
      break;
    }
  }

  if (FileBuffer != NULL) {
    FreePool (FileBuffer);
  }

  if (FilePathToVerify != FilePath) {
    FreePool (FilePathToVerify);
  }

  return Status;
}

/**
  Reallocates more global memory to store the registered Securit2Handler list.

  @retval  RETURN_SUCCESS            Reallocate memory successfully.
  @retval  RETURN_OUT_OF_RESOURCES   No enough memory to allocated.
**/
RETURN_STATUS
EFIAPI
ReallocateSecurity2HandlerTable (
  VOID
  )
{
  //
  // Reallocate memory for security info structure.
  //
  mSecurity2Table = ReallocatePool (
                      mMaxNumberOfSecurity2Handler * sizeof (SECURITY2_INFO),
                      (mMaxNumberOfSecurity2Handler + SECURITY_HANDLER_TABLE_SIZE) * sizeof (SECURITY2_INFO),
                      mSecurity2Table
                      );

  //
  // No enough resource is allocated.
  //
  if (mSecurity2Table == NULL) {
    return RETURN_OUT_OF_RESOURCES;
  }

  //
  // Increase max handler number
  //
  mMaxNumberOfSecurity2Handler = mMaxNumberOfSecurity2Handler + SECURITY_HANDLER_TABLE_SIZE;
  return RETURN_SUCCESS;
}

/**
  Check whether an operation is valid according to the requirement of current operation,
  which must make sure that the measure image operation is the last one.

  If AuthenticationOperation is not recongnized, return FALSE.
  If AuthenticationOperation is EFI_AUTH_OPERATION_NONE, return FALSE.
  If AuthenticationOperation includes security operation and authentication operation, return FALSE.
  If the previous register handler can't be executed before the later register handler, return FALSE.

  @param CurrentAuthOperation  Current operation.
  @param CheckAuthOperation    Operation to be checked.

  @retval  TRUE   Operation is valid for current operation.
  @retval  FALSE  Operation is invalid for current operation.
**/
BOOLEAN
CheckAuthentication2Operation (
  IN  UINT32  CurrentAuthOperation,
  IN  UINT32  CheckAuthOperation
  )
{
  //
  // Make sure new auth operation can be recognized.
  //
  if (CheckAuthOperation == EFI_AUTH_OPERATION_NONE) {
    return FALSE;
  }

  if ((CheckAuthOperation & ~(EFI_AUTH_IMAGE_OPERATION_MASK |
                              EFI_AUTH_NONE_IMAGE_OPERATION_MASK |
                              EFI_AUTH_OPERATION_IMAGE_REQUIRED)) != 0)
  {
    return FALSE;
  }

  //
  // When current operation includes measure image operation,
  // only another measure image or none image operation will be allowed.
  //
  if ((CurrentAuthOperation & EFI_AUTH_OPERATION_MEASURE_IMAGE) == EFI_AUTH_OPERATION_MEASURE_IMAGE) {
    if (((CheckAuthOperation & EFI_AUTH_OPERATION_MEASURE_IMAGE) == EFI_AUTH_OPERATION_MEASURE_IMAGE) ||
        ((CheckAuthOperation & EFI_AUTH_IMAGE_OPERATION_MASK) == 0))
    {
      return TRUE;
    } else {
      return FALSE;
    }
  }

  //
  // Any other operation will be allowed.
  //
  return TRUE;
}

/**
  Register security measurement handler with its operation type. Different
  handlers with the same operation can all be registered.

  If Security2Handler is NULL, then ASSERT().
  If no enough resources available to register new handler, then ASSERT().
  If AuthenticationOperation is not recongnized, then ASSERT().
  If AuthenticationOperation is EFI_AUTH_OPERATION_NONE, then ASSERT().
  If the previous register handler can't be executed before the later register handler, then ASSERT().

  @param[in]  Security2Handler          The security measurement service handler to be registered.
  @param[in]  AuthenticationOperation   The operation type is specified for the registered handler.

  @retval EFI_SUCCESS              The handlers were registered successfully.
**/
EFI_STATUS
EFIAPI
RegisterSecurity2Handler (
  IN  SECURITY2_FILE_AUTHENTICATION_HANDLER  Security2Handler,
  IN  UINT32                                 AuthenticationOperation
  )
{
  EFI_STATUS  Status;

  ASSERT (Security2Handler != NULL);

  //
  // Make sure AuthenticationOperation is valid in the register order.
  //
  ASSERT (CheckAuthentication2Operation (mCurrentAuthOperation2, AuthenticationOperation));
  mCurrentAuthOperation2 = mCurrentAuthOperation2 | AuthenticationOperation;

  //
  // Check whether the handler lists is enough to store new handler.
  //
  if (mNumberOfSecurity2Handler == mMaxNumberOfSecurity2Handler) {
    //
    // Allocate more resources for new handler.
    //
    Status = ReallocateSecurity2HandlerTable ();
    ASSERT_EFI_ERROR (Status);
  }

  //
  // Register new handler into the handler list.
  //
  mSecurity2Table[mNumberOfSecurity2Handler].Security2Operation = AuthenticationOperation;
  mSecurity2Table[mNumberOfSecurity2Handler].Security2Handler   = Security2Handler;
  mNumberOfSecurity2Handler++;

  return EFI_SUCCESS;
}

/**
  Execute registered handlers based on input AuthenticationOperation until
  one returns an error and that error is returned.

  If none of the handlers return an error, then EFI_SUCCESS is returned.
  The handlers those satisfy AuthenticationOperation will only be executed.
  The handlers are executed in same order to their registered order.

  @param[in]  AuthenticationOperation
                           The operation type specifies which handlers will be executed.
  @param[in]  AuthenticationStatus
                           The authentication status for the input file.
  @param[in]  File         This is a pointer to the device path of the file that is
                           being dispatched. This will optionally be used for logging.
  @param[in]  FileBuffer   A pointer to the buffer with the UEFI file image
  @param[in]  FileSize     The size of File buffer.
  @param[in]  BootPolicy   A boot policy that was used to call LoadImage() UEFI service.

  @retval EFI_SUCCESS             The file specified by DevicePath and non-NULL
                                  FileBuffer did authenticate, and the platform policy dictates
                                  that the DXE Foundation may use the file.
  @retval EFI_SUCCESS             The device path specified by NULL device path DevicePath
                                  and non-NULL FileBuffer did authenticate, and the platform
                                  policy dictates that the DXE Foundation may execute the image in
                                  FileBuffer.
  @retval EFI_SUCCESS             FileBuffer is NULL and current user has permission to start
                                  UEFI device drivers on the device path specified by DevicePath.
  @retval EFI_SECURITY_VIOLATION  The file specified by File or FileBuffer did not
                                  authenticate, and the platform policy dictates that
                                  the file should be placed in the untrusted state.
  @retval EFI_SECURITY_VIOLATION  FileBuffer FileBuffer is NULL and the user has no
                                  permission to start UEFI device drivers on the device path specified
                                  by DevicePath.
  @retval EFI_SECURITY_VIOLATION  FileBuffer is not NULL and the user has no permission to load
                                  drivers from the device path specified by DevicePath. The
                                  image has been added into the list of the deferred images.
  @retval EFI_ACCESS_DENIED       The file specified by File did not authenticate, and
                                  the platform policy dictates that the DXE
                                  Foundation may not use File.
  @retval EFI_INVALID_PARAMETER   File and FileBuffer are both NULL.
**/
EFI_STATUS
EFIAPI
ExecuteSecurity2Handlers (
  IN  UINT32                          AuthenticationOperation,
  IN  UINT32                          AuthenticationStatus,
  IN  CONST EFI_DEVICE_PATH_PROTOCOL  *File  OPTIONAL,
  IN  VOID                            *FileBuffer,
  IN  UINTN                           FileSize,
  IN  BOOLEAN                         BootPolicy
  )
{
  UINT32      Index;
  EFI_STATUS  Status;

  //
  // Invalid case if File and FileBuffer are both NULL.
  //
  if ((File == NULL) && (FileBuffer == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Directly return successfully when no handler is registered.
  //
  if (mNumberOfSecurity2Handler == 0) {
    return EFI_SUCCESS;
  }

  //
  // Run security handler in same order to their registered list
  //
  for (Index = 0; Index < mNumberOfSecurity2Handler; Index++) {
    //
    // If FileBuffer is not NULL, the input is Image, which will be handled by EFI_AUTH_IMAGE_OPERATION_MASK operation.
    // If FileBuffer is NULL, the input is not Image, which will be handled by EFI_AUTH_NONE_IMAGE_OPERATION_MASK operation.
    // Other cases are ignored.
    //
    if (((FileBuffer != NULL) && ((mSecurity2Table[Index].Security2Operation & EFI_AUTH_IMAGE_OPERATION_MASK) != 0)) ||
        ((FileBuffer == NULL) && ((mSecurity2Table[Index].Security2Operation & EFI_AUTH_NONE_IMAGE_OPERATION_MASK) != 0)))
    {
      //
      // Execute registered handlers based on input AuthenticationOperation
      //
      if ((mSecurity2Table[Index].Security2Operation & AuthenticationOperation) != 0) {
        Status = mSecurity2Table[Index].Security2Handler (
                                          AuthenticationStatus,
                                          File,
                                          FileBuffer,
                                          FileSize,
                                          BootPolicy
                                          );
        if (EFI_ERROR (Status)) {
          return Status;
        }
      }
    }
  }

  return EFI_SUCCESS;
}
