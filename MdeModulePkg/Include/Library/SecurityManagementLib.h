/** @file
  This library class defines a set of interfaces to abstract the policy of 
  security measurement by managing the different security measurement services.
  The library instances can be implemented according to the different security policy.

Copyright (c) 2009 - 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under 
the terms and conditions of the BSD License that accompanies this distribution.  
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.                                            

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __SECURITY_MANAGEMENT_LIB_H__
#define __SECURITY_MANAGEMENT_LIB_H__

//
// Authentication Operation defintions for User Identity (UID), Measured and Secure boot.
//
#define EFI_AUTH_OPERATION_NONE                0x00
#define EFI_AUTH_OPERATION_VERIFY_IMAGE        0x01
#define EFI_AUTH_OPERATION_DEFER_IMAGE_LOAD    0x02
#define EFI_AUTH_OPERATION_MEASURE_IMAGE       0x04
#define EFI_AUTH_OPERATION_CONNECT_POLICY      0x08
//
// Authentication State Operation will check the authentication status of a file.
//
#define EFI_AUTH_OPERATION_AUTHENTICATION_STATE  0x10

///
/// Image buffer is required by the security handler.
///
#define EFI_AUTH_OPERATION_IMAGE_REQUIRED      0x80000000

/**
  The security handler is used to abstract platform-specific policy 
  from the DXE core response to an attempt to use a file that returns a 
  given status for the authentication check from the section extraction protocol.  

  The possible responses in a given SAP implementation may include locking 
  flash upon failure to authenticate, attestation logging for all signed drivers, 
  and other exception operations.  The File parameter allows for possible logging 
  within the SAP of the driver.
  
  If File is NULL, then EFI_INVALID_PARAMETER is returned.

  If the file specified by File with an authentication status specified by 
  AuthenticationStatus is safe for the DXE Core to use, then EFI_SUCCESS is returned.

  If the file specified by File with an authentication status specified by 
  AuthenticationStatus is not safe for the DXE Core to use under any circumstances, 
  then EFI_ACCESS_DENIED is returned.

  If the file specified by File with an authentication status specified by 
  AuthenticationStatus is not safe for the DXE Core to use at the time, but it 
  might be possible to use it at a future time, then EFI_SECURITY_VIOLATION is 
  returned.

  FileBuffer will be NULL and FileSize will be 0 if the handler being called 
  did not set EFI_AUTH_OPERATION_IMAGE_REQUIRED when it was registered.

  @param[in]    AuthenticationStatus 
                           The authentication status returned from the security
                           measurement services for the input file.
  @param[in]    File       The pointer to the device path of the file that is
                           being dispatched. This will optionally be used for logging.
  @param[in]    FileBuffer The file buffer matches the input file device path.
  @param[in]    FileSize   The size of File buffer matches the input file device path.

  @retval EFI_SUCCESS            The file specified by File did authenticate, and the
                                 platform policy dictates that the DXE Core may use File.
  @retval EFI_INVALID_PARAMETER  The file is NULL.
  @retval EFI_SECURITY_VIOLATION The file specified by File did not authenticate, and
                                 the platform policy dictates that File should be placed
                                 in the untrusted state. A file may be promoted from
                                 the untrusted to the trusted state at a future time
                                 with a call to the Trust() DXE Service.
  @retval EFI_ACCESS_DENIED      The file specified by File did not authenticate, and
                                 the platform policy dictates that File should not be
                                 used for any purpose.

**/
typedef 
EFI_STATUS
(EFIAPI *SECURITY_FILE_AUTHENTICATION_STATE_HANDLER)(
  IN  OUT   UINT32                     AuthenticationStatus,
  IN  CONST EFI_DEVICE_PATH_PROTOCOL   *File,
  IN  VOID                             *FileBuffer,
  IN  UINTN                            FileSize
  );

/**
  Register security measurement handler with its operation type. Different
  handlers with the same operation can all be registered.

  If SecurityHandler is NULL, then ASSERT().
  If no enough resources available to register new handler, then ASSERT().
  If AuthenticationOperation is not recongnized, then ASSERT().
  If the previous register handler can't be executed before the later register handler, then ASSERT().

  @param[in]  SecurityHandler           The security measurement service handler to be registered.
  @param[in]  AuthenticationOperation   Theoperation type is specified for the registered handler.

  @retval EFI_SUCCESS              The handlers were registered successfully.
**/
EFI_STATUS
EFIAPI
RegisterSecurityHandler (
  IN  SECURITY_FILE_AUTHENTICATION_STATE_HANDLER  SecurityHandler,
  IN  UINT32                                      AuthenticationOperation
  );

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

  @retval EFI_SUCCESS            The file specified by File authenticated when more
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
  );

/**
  The security handler is used to abstracts security-specific functions from the DXE 
  Foundation of UEFI Image Verification, Trusted Computing Group (TCG) measured boot, 
  User Identity policy for image loading and consoles, and for purposes of 
  handling GUIDed section encapsulations. 
  
  @param[in]    AuthenticationStatus 
                           The authentication status for the input file. 
  @param[in]    File       The pointer to the device path of the file that is
                           being dispatched. This will optionally be used for logging.
  @param[in]    FileBuffer A pointer to the buffer with the UEFI file image
  @param[in]    FileSize   The size of File buffer.
  @param[in]    BootPolicy A boot policy that was used to call LoadImage() UEFI service.

  @retval EFI_SUCCESS             The file specified by DevicePath and non-NULL
                                  FileBuffer did authenticate, and the platform policy dictates
                                  that the DXE Foundation may use the file.
  @retval EFI_SUCCESS             The device path specified by NULL device path DevicePath
                                  and non-NULL FileBuffer did authenticate, and the platform
                                  policy dictates that the DXE Foundation may execute the image in
                                  FileBuffer.
  @retval EFI_SUCCESS             FileBuffer is NULL and current user has permission to start
                                  UEFI device drivers on the device path specified by DevicePath.
  @retval EFI_SECURITY_VIOLATION  The file specified by DevicePath and FileBuffer did not
                                  authenticate, and the platform policy dictates that the file should be
                                  placed in the untrusted state. The image has been added to the file
                                  execution table.
  @retval EFI_ACCESS_DENIED       The file specified by File and FileBuffer did not
                                  authenticate, and the platform policy dictates that the DXE
                                  Foundation may not use File.
  @retval EFI_SECURITY_VIOLATION  FileBuffer is NULL and the user has no
                                  permission to start UEFI device drivers on the device path specified
                                  by DevicePath.
  @retval EFI_SECURITY_VIOLATION  FileBuffer is not NULL and the user has no permission to load
                                  drivers from the device path specified by DevicePath. The
                                  image has been added into the list of the deferred images.
**/
typedef 
EFI_STATUS
(EFIAPI *SECURITY2_FILE_AUTHENTICATION_HANDLER) (
  IN  UINT32                           AuthenticationStatus,
  IN  CONST EFI_DEVICE_PATH_PROTOCOL   *File,
  IN  VOID                             *FileBuffer,
  IN  UINTN                            FileSize,
  IN  BOOLEAN                          BootPolicy
  );

/**
  Register security measurement handler with its operation type. Different
  handlers with the same operation can all be registered.

  If SecurityHandler is NULL, then ASSERT().
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
  IN  SECURITY2_FILE_AUTHENTICATION_HANDLER       Security2Handler,
  IN  UINT32                                      AuthenticationOperation
  );

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
  @retval EFI_SECURITY_VIOLATION  The file specified by DevicePath and FileBuffer did not
                                  authenticate, and the platform policy dictates that the file should be
                                  placed in the untrusted state. The image has been added to the file
                                  execution table.
  @retval EFI_ACCESS_DENIED       The file specified by File and FileBuffer did not
                                  authenticate, and the platform policy dictates that the DXE
                                  Foundation may not use File.
  @retval EFI_SECURITY_VIOLATION  FileBuffer is NULL and the user has no
                                  permission to start UEFI device drivers on the device path specified
                                  by DevicePath.
  @retval EFI_SECURITY_VIOLATION  FileBuffer is not NULL and the user has no permission to load
                                  drivers from the device path specified by DevicePath. The
                                  image has been added into the list of the deferred images.
  @retval EFI_INVALID_PARAMETER   File and FileBuffer are both NULL. 
**/
EFI_STATUS
EFIAPI
ExecuteSecurity2Handlers (
  IN  UINT32                           AuthenticationOperation,
  IN  UINT32                           AuthenticationStatus,
  IN  CONST EFI_DEVICE_PATH_PROTOCOL   *File,
  IN  VOID                             *FileBuffer,
  IN  UINTN                            FileSize,
  IN  BOOLEAN                          BootPolicy
  );

#endif
