/** @file
  Provides generic security measurement functions for DXE module.

Copyright (c) 2009 Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiDxe.h>
#include <Library/DebugLib.h>
#include <Library/DxeServicesLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/SecurityManagementLib.h>

#define SECURITY_HANDLER_TABLE_SIZE   0x10

#define EFI_AUTH_OPERATION_MASK       (EFI_AUTH_OPERATION_VERIFY_IMAGE \
                                      | EFI_AUTH_OPERATION_DEFER_IMAGE_LOAD \
                                      | EFI_AUTH_OPERATION_MEASURE_IMAGE)

typedef struct {
  UINT32  SecurityOperation;
  SECURITY_FILE_AUTHENTICATION_STATE_HANDLER  SecurityHandler;
} SECURITY_INFO;

UINT32  mCurrentAuthOperation       = 0;
UINT32  mNumberOfSecurityHandler    = 0;
UINT32  mMaxNumberOfSecurityHandler = 0;
SECURITY_INFO  *mSecurityTable      = NULL;

/**
  Reallocates more global memory to store the registered Handler list.

  @retval  RETURN_SUCCESS            Reallocate memory successfully.
  @retval  RETURN_OUT_OF_RESOURCES   No enough memory to allocated.
**/
RETURN_STATUS
EFIAPI
ReallocateSecurityHandlerTable (
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
  IN  UINT32    CurrentAuthOperation,
  IN  UINT32    CheckAuthOperation
  )
{ 
  //
  // Make sure new auth operation can be recognized.
  //
  ASSERT ((CheckAuthOperation & ~(EFI_AUTH_OPERATION_MASK | EFI_AUTH_OPERATION_IMAGE_REQUIRED)) == 0);
  
  //
  // When current operation includes measure image operation, 
  // only another measure image operation or none operation will be allowed.
  //
  if ((CurrentAuthOperation & EFI_AUTH_OPERATION_MEASURE_IMAGE) == EFI_AUTH_OPERATION_MEASURE_IMAGE) {
    if (((CheckAuthOperation & EFI_AUTH_OPERATION_MEASURE_IMAGE) == EFI_AUTH_OPERATION_MEASURE_IMAGE) ||
        ((CheckAuthOperation & EFI_AUTH_OPERATION_MASK) == EFI_AUTH_OPERATION_NONE)) {
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
    Status = ReallocateSecurityHandlerTable();
    ASSERT_EFI_ERROR (Status);
  }

  //
  // Register new handler into the handler list.
  //
  mSecurityTable[mNumberOfSecurityHandler].SecurityOperation = AuthenticationOperation;
  mSecurityTable[mNumberOfSecurityHandler].SecurityHandler   = SecurityHandler;
  mNumberOfSecurityHandler ++;

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
  IN  UINT32                            AuthenticationStatus,
  IN  CONST EFI_DEVICE_PATH_PROTOCOL    *FilePath
  )
{
  UINT32        Index;
  EFI_STATUS    Status;
  UINT32        HandlerAuthenticationStatus;
  VOID          *FileBuffer;
  UINTN         FileSize;
  
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
  //
  // Run security handler in same order to their registered list
  //
  for (Index = 0; Index < mNumberOfSecurityHandler; Index ++) {
    if ((mSecurityTable[Index].SecurityOperation & EFI_AUTH_OPERATION_IMAGE_REQUIRED) == EFI_AUTH_OPERATION_IMAGE_REQUIRED) {
      //
      // Try get file buffer when the handler requires image buffer.
      //
      if (FileBuffer == NULL) {
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
      }
    }
    Status = mSecurityTable[Index].SecurityHandler (
               HandlerAuthenticationStatus,
               FilePath,
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

  return Status;
}
