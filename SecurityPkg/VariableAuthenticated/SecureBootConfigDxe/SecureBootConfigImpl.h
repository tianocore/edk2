/** @file
  The header file of HII Config Access protocol implementation of SecureBoot
  configuration module.

Copyright (c) 2011 - 2017, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef __SECUREBOOT_CONFIG_IMPL_H__
#define __SECUREBOOT_CONFIG_IMPL_H__

#include <Uefi.h>

#include <Protocol/HiiConfigAccess.h>
#include <Protocol/HiiConfigRouting.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/BlockIo.h>
#include <Protocol/DevicePath.h>
#include <Protocol/DebugPort.h>
#include <Protocol/LoadFile.h>

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiHiiServicesLib.h>
#include <Library/UefiLib.h>
#include <Library/HiiLib.h>
#include <Library/DevicePathLib.h>
#include <Library/PrintLib.h>
#include <Library/PlatformSecureLib.h>
#include <Library/BaseCryptLib.h>
#include <Library/FileExplorerLib.h>
#include <Library/PeCoffLib.h>

#include <Guid/MdeModuleHii.h>
#include <Guid/AuthenticatedVariableFormat.h>
#include <Guid/FileSystemVolumeLabelInfo.h>
#include <Guid/ImageAuthentication.h>
#include <Guid/FileInfo.h>
#include <Guid/WinCertificate.h>

#include "SecureBootConfigNvData.h"

//
// Tool generated IFR binary data and String package data
//
extern  UINT8  SecureBootConfigBin[];
extern  UINT8  SecureBootConfigDxeStrings[];

//
// Shared IFR form update data
//
extern  VOID                *mStartOpCodeHandle;
extern  VOID                *mEndOpCodeHandle;
extern  EFI_IFR_GUID_LABEL  *mStartLabel;
extern  EFI_IFR_GUID_LABEL  *mEndLabel;

#define MAX_CHAR         480
#define TWO_BYTE_ENCODE  0x82
#define BUFFER_MAX_SIZE  100

//
// SHA-256 digest size in bytes
//
#define SHA256_DIGEST_SIZE  32
//
// SHA-384 digest size in bytes
//
#define SHA384_DIGEST_SIZE  48
//
// SHA-512 digest size in bytes
//
#define SHA512_DIGEST_SIZE  64

//
// Set max digest size as SHA512 Output (64 bytes) by far
//
#define MAX_DIGEST_SIZE  SHA512_DIGEST_SIZE

#define WIN_CERT_UEFI_RSA2048_SIZE  256
#define WIN_CERT_UEFI_RSA3072_SIZE  384
#define WIN_CERT_UEFI_RSA4096_SIZE  512

//
// Support hash types
//
#define HASHALG_SHA224  0x00000000
#define HASHALG_SHA256  0x00000001
#define HASHALG_SHA384  0x00000002
#define HASHALG_SHA512  0x00000003
#define HASHALG_RAW     0x00000004
#define HASHALG_MAX     0x00000004

//
// Certificate public key minimum size (bytes)
//
#define CER_PUBKEY_MIN_SIZE  256

//
// Define KeyType for public key storing file
//
#define KEY_TYPE_RSASSA  0

//
// Types of errors may occur during certificate enrollment.
//
typedef enum {
  None_Error = 0,
  //
  // Unsupported_type indicates the certificate type is not supported.
  //
  Unsupported_Type,
  //
  // Unqualified_key indicates the key strength of certificate is not
  // strong enough.
  //
  Unqualified_Key,
  Enroll_Error_Max
} ENROLL_KEY_ERROR;

typedef struct {
  UINTN         Signature;
  LIST_ENTRY    Head;
  UINTN         MenuNumber;
} SECUREBOOT_MENU_OPTION;

typedef struct {
  EFI_FILE_HANDLE    FHandle;
  UINT16             *FileName;
  UINT8              FileType;
} SECUREBOOT_FILE_CONTEXT;

#define SECUREBOOT_FREE_NON_NULL(Pointer)   \
  do {                                      \
    if ((Pointer) != NULL) {                \
      FreePool((Pointer));                  \
      (Pointer) = NULL;                     \
    }                                       \
  } while (FALSE)

#define SECUREBOOT_FREE_NON_OPCODE(Handle)  \
  do{                                       \
    if ((Handle) != NULL) {                 \
      HiiFreeOpCodeHandle((Handle));        \
    }                                       \
  } while (FALSE)

#define SIGNATURE_DATA_COUNTS(List)         \
  (((List)->SignatureListSize - sizeof(EFI_SIGNATURE_LIST) - (List)->SignatureHeaderSize) / (List)->SignatureSize)

//
// We define another format of 5th directory entry: security directory
//
typedef struct {
  UINT32    Offset;                 // Offset of certificate
  UINT32    SizeOfCert;             // size of certificate appended
} EFI_IMAGE_SECURITY_DATA_DIRECTORY;

typedef enum {
  ImageType_IA32,
  ImageType_X64
} IMAGE_TYPE;

///
/// HII specific Vendor Device Path definition.
///
typedef struct {
  VENDOR_DEVICE_PATH          VendorDevicePath;
  EFI_DEVICE_PATH_PROTOCOL    End;
} HII_VENDOR_DEVICE_PATH;

typedef enum {
  Variable_DB,
  Variable_DBX,
  Variable_DBT,
  Variable_MAX
} CURRENT_VARIABLE_NAME;

typedef enum {
  Delete_Signature_List_All,
  Delete_Signature_List_One,
  Delete_Signature_Data
} SIGNATURE_DELETE_TYPE;

typedef struct {
  UINTN                             Signature;

  EFI_HII_CONFIG_ACCESS_PROTOCOL    ConfigAccess;
  EFI_HII_HANDLE                    HiiHandle;
  EFI_HANDLE                        DriverHandle;

  SECUREBOOT_FILE_CONTEXT           *FileContext;

  EFI_GUID                          *SignatureGUID;

  CURRENT_VARIABLE_NAME             VariableName;     // The variable name we are processing.
  UINT32                            ListCount;        // Record current variable has how many signature list.
  UINTN                             ListIndex;        // Record which signature list is processing.
  BOOLEAN                           *CheckArray;      // Record which signature data checked.
} SECUREBOOT_CONFIG_PRIVATE_DATA;

extern SECUREBOOT_CONFIG_PRIVATE_DATA  mSecureBootConfigPrivateDateTemplate;
extern SECUREBOOT_CONFIG_PRIVATE_DATA  *gSecureBootPrivateData;

#define SECUREBOOT_CONFIG_PRIVATE_DATA_SIGNATURE  SIGNATURE_32 ('S', 'E', 'C', 'B')
#define SECUREBOOT_CONFIG_PRIVATE_FROM_THIS(a)  CR (a, SECUREBOOT_CONFIG_PRIVATE_DATA, ConfigAccess, SECUREBOOT_CONFIG_PRIVATE_DATA_SIGNATURE)

//
// Cryptographic Key Information
//
#pragma pack(1)
typedef struct _CPL_KEY_INFO {
  UINT32    KeyLengthInBits;        // Key Length In Bits
  UINT32    BlockSize;              // Operation Block Size in Bytes
  UINT32    CipherBlockSize;        // Output Cipher Block Size in Bytes
  UINT32    KeyType;                // Key Type
  UINT32    CipherMode;             // Cipher Mode for Symmetric Algorithm
  UINT32    Flags;                  // Additional Key Property Flags
} CPL_KEY_INFO;
#pragma pack()

/**
  Retrieves the size, in bytes, of the context buffer required for hash operations.

  @return  The size, in bytes, of the context buffer required for hash operations.

**/
typedef
EFI_STATUS
(EFIAPI *HASH_GET_CONTEXT_SIZE)(
  VOID
  );

/**
  Initializes user-supplied memory pointed by HashContext as hash context for
  subsequent use.

  If HashContext is NULL, then ASSERT().

  @param[in, out]  HashContext  Pointer to  Context being initialized.

  @retval TRUE   HASH context initialization succeeded.
  @retval FALSE  HASH context initialization failed.

**/
typedef
BOOLEAN
(EFIAPI *HASH_INIT)(
  IN OUT  VOID  *HashContext
  );

/**
  Performs digest on a data buffer of the specified length. This function can
  be called multiple times to compute the digest of long or discontinuous data streams.

  If HashContext is NULL, then ASSERT().

  @param[in, out]  HashContext  Pointer to the MD5 context.
  @param[in]       Data        Pointer to the buffer containing the data to be hashed.
  @param[in]       DataLength  Length of Data buffer in bytes.

  @retval TRUE   HASH data digest succeeded.
  @retval FALSE  Invalid HASH context. After HashFinal function has been called, the
                 HASH context cannot be reused.

**/
typedef
BOOLEAN
(EFIAPI *HASH_UPDATE)(
  IN OUT  VOID        *HashContext,
  IN      CONST VOID  *Data,
  IN      UINTN       DataLength
  );

/**
  Completes hash computation and retrieves the digest value into the specified
  memory. After this function has been called, the context cannot be used again.

  If HashContext is NULL, then ASSERT().
  If HashValue is NULL, then ASSERT().

  @param[in, out]  HashContext  Pointer to the MD5 context
  @param[out]      HashValue   Pointer to a buffer that receives the HASH digest
                               value (16 bytes).

  @retval TRUE   HASH digest computation succeeded.
  @retval FALSE  HASH digest computation failed.

**/
typedef
BOOLEAN
(EFIAPI *HASH_FINAL)(
  IN OUT  VOID   *HashContext,
  OUT     UINT8  *HashValue
  );

//
// Hash Algorithm Table
//
typedef struct {
  CHAR16                   *Name;           ///< Name for Hash Algorithm
  UINTN                    DigestLength;    ///< Digest Length
  UINT8                    *OidValue;       ///< Hash Algorithm OID ASN.1 Value
  UINTN                    OidLength;       ///< Length of Hash OID Value
  HASH_GET_CONTEXT_SIZE    GetContextSize;  ///< Pointer to Hash GetContentSize function
  HASH_INIT                HashInit;        ///< Pointer to Hash Init function
  HASH_UPDATE              HashUpdate;      ///< Pointer to Hash Update function
  HASH_FINAL               HashFinal;       ///< Pointer to Hash Final function
} HASH_TABLE;

typedef struct {
  WIN_CERTIFICATE    Hdr;
  UINT8              CertData[1];
} WIN_CERTIFICATE_EFI_PKCS;

/**
  This function publish the SecureBoot configuration Form.

  @param[in, out]  PrivateData   Points to SecureBoot configuration private data.

  @retval EFI_SUCCESS            HII Form is installed successfully.
  @retval EFI_OUT_OF_RESOURCES   Not enough resource for HII Form installation.
  @retval Others                 Other errors as indicated.

**/
EFI_STATUS
InstallSecureBootConfigForm (
  IN OUT SECUREBOOT_CONFIG_PRIVATE_DATA  *PrivateData
  );

/**
  This function removes SecureBoot configuration Form.

  @param[in, out]  PrivateData   Points to SecureBoot configuration private data.

**/
VOID
UninstallSecureBootConfigForm (
  IN OUT SECUREBOOT_CONFIG_PRIVATE_DATA  *PrivateData
  );

/**
  This function allows a caller to extract the current configuration for one
  or more named elements from the target driver.

  @param[in]   This              Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param[in]   Request           A null-terminated Unicode string in
                                 <ConfigRequest> format.
  @param[out]  Progress          On return, points to a character in the Request
                                 string. Points to the string's null terminator if
                                 request was successful. Points to the most recent
                                 '&' before the first failing name/value pair (or
                                 the beginning of the string if the failure is in
                                 the first name/value pair) if the request was not
                                 successful.
  @param[out]  Results           A null-terminated Unicode string in
                                 <ConfigAltResp> format which has all values filled
                                 in for the names in the Request string. String to
                                 be allocated by the called function.

  @retval EFI_SUCCESS            The Results is filled with the requested values.
  @retval EFI_OUT_OF_RESOURCES   Not enough memory to store the results.
  @retval EFI_INVALID_PARAMETER  Request is illegal syntax, or unknown name.
  @retval EFI_NOT_FOUND          Routing data doesn't match any storage in this
                                 driver.

**/
EFI_STATUS
EFIAPI
SecureBootExtractConfig (
  IN CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN CONST EFI_STRING                      Request,
  OUT EFI_STRING                           *Progress,
  OUT EFI_STRING                           *Results
  );

/**
  This function processes the results of changes in configuration.

  @param[in]  This               Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param[in]  Configuration      A null-terminated Unicode string in <ConfigResp>
                                 format.
  @param[out] Progress           A pointer to a string filled in with the offset of
                                 the most recent '&' before the first failing
                                 name/value pair (or the beginning of the string if
                                 the failure is in the first name/value pair) or
                                 the terminating NULL if all was successful.

  @retval EFI_SUCCESS            The Results is processed successfully.
  @retval EFI_INVALID_PARAMETER  Configuration is NULL.
  @retval EFI_NOT_FOUND          Routing data doesn't match any storage in this
                                 driver.

**/
EFI_STATUS
EFIAPI
SecureBootRouteConfig (
  IN CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN CONST EFI_STRING                      Configuration,
  OUT EFI_STRING                           *Progress
  );

/**
  This function processes the results of changes in configuration.

  @param[in]  This               Points to the EFI_HII_CONFIG_ACCESS_PROTOCOL.
  @param[in]  Action             Specifies the type of action taken by the browser.
  @param[in]  QuestionId         A unique value which is sent to the original
                                 exporting driver so that it can identify the type
                                 of data to expect.
  @param[in]  Type               The type of value for the question.
  @param[in]  Value              A pointer to the data being sent to the original
                                 exporting driver.
  @param[out] ActionRequest      On return, points to the action requested by the
                                 callback function.

  @retval EFI_SUCCESS            The callback successfully handled the action.
  @retval EFI_OUT_OF_RESOURCES   Not enough storage is available to hold the
                                 variable and its data.
  @retval EFI_DEVICE_ERROR       The variable could not be saved.
  @retval EFI_UNSUPPORTED        The specified Action is not supported by the
                                 callback.

**/
EFI_STATUS
EFIAPI
SecureBootCallback (
  IN CONST EFI_HII_CONFIG_ACCESS_PROTOCOL  *This,
  IN     EFI_BROWSER_ACTION                Action,
  IN     EFI_QUESTION_ID                   QuestionId,
  IN     UINT8                             Type,
  IN     EFI_IFR_TYPE_VALUE                *Value,
  OUT EFI_BROWSER_ACTION_REQUEST           *ActionRequest
  );

/**
  This function converts an input device structure to a Unicode string.

  @param[in] DevPath                  A pointer to the device path structure.

  @return A new allocated Unicode string that represents the device path.

**/
CHAR16 *
EFIAPI
DevicePathToStr (
  IN EFI_DEVICE_PATH_PROTOCOL  *DevPath
  );

/**
  Clean up the dynamic opcode at label and form specified by both LabelId.

  @param[in] LabelId         It is both the Form ID and Label ID for opcode deletion.
  @param[in] PrivateData     Module private data.

**/
VOID
CleanUpPage (
  IN UINT16                          LabelId,
  IN SECUREBOOT_CONFIG_PRIVATE_DATA  *PrivateData
  );

/**
  Read file content into BufferPtr, the size of the allocate buffer
  is *FileSize plus AdditionAllocateSize.

  @param[in]       FileHandle            The file to be read.
  @param[in, out]  BufferPtr             Pointers to the pointer of allocated buffer.
  @param[out]      FileSize              Size of input file
  @param[in]       AdditionAllocateSize   Addition size the buffer need to be allocated.
                                         In case the buffer need to contain others besides the file content.

  @retval   EFI_SUCCESS                  The file was read into the buffer.
  @retval   EFI_INVALID_PARAMETER        A parameter was invalid.
  @retval   EFI_OUT_OF_RESOURCES         A memory allocation failed.
  @retval   others                       Unexpected error.

**/
EFI_STATUS
ReadFileContent (
  IN      EFI_FILE_HANDLE  FileHandle,
  IN OUT  VOID             **BufferPtr,
  OUT  UINTN               *FileSize,
  IN      UINTN            AdditionAllocateSize
  );

/**
  Close an open file handle.

  @param[in] FileHandle           The file handle to close.

**/
VOID
CloseFile (
  IN EFI_FILE_HANDLE  FileHandle
  );

/**
  Converts a nonnegative integer to an octet string of a specified length.

  @param[in]   Integer          Pointer to the nonnegative integer to be converted
  @param[in]   IntSizeInWords   Length of integer buffer in words
  @param[out]  OctetString      Converted octet string of the specified length
  @param[in]   OSSizeInBytes    Intended length of resulting octet string in bytes

Returns:

  @retval   EFI_SUCCESS            Data conversion successfully
  @retval   EFI_BUFFER_TOOL_SMALL  Buffer is too small for output string

**/
EFI_STATUS
EFIAPI
Int2OctStr (
  IN     CONST UINTN  *Integer,
  IN     UINTN        IntSizeInWords,
  OUT UINT8           *OctetString,
  IN     UINTN        OSSizeInBytes
  );

/**
  Worker function that prints an EFI_GUID into specified Buffer.

  @param[in]     Guid          Pointer to GUID to print.
  @param[in]     Buffer        Buffer to print Guid into.
  @param[in]     BufferSize    Size of Buffer.

  @retval    Number of characters printed.

**/
UINTN
GuidToString (
  IN  EFI_GUID  *Guid,
  IN  CHAR16    *Buffer,
  IN  UINTN     BufferSize
  );

/**
  Update the PK form base on the input file path info.

  @param FilePath    Point to the file path.

  @retval TRUE   Exit caller function.
  @retval FALSE  Not exit caller function.
**/
BOOLEAN
EFIAPI
UpdatePKFromFile (
  IN EFI_DEVICE_PATH_PROTOCOL  *FilePath
  );

/**
  Update the KEK form base on the input file path info.

  @param FilePath    Point to the file path.

  @retval TRUE   Exit caller function.
  @retval FALSE  Not exit caller function.
**/
BOOLEAN
EFIAPI
UpdateKEKFromFile (
  IN EFI_DEVICE_PATH_PROTOCOL  *FilePath
  );

/**
  Update the DB form base on the input file path info.

  @param FilePath    Point to the file path.

  @retval TRUE   Exit caller function.
  @retval FALSE  Not exit caller function.
**/
BOOLEAN
EFIAPI
UpdateDBFromFile (
  IN EFI_DEVICE_PATH_PROTOCOL  *FilePath
  );

/**
  Update the DBX form base on the input file path info.

  @param FilePath    Point to the file path.

  @retval TRUE   Exit caller function.
  @retval FALSE  Not exit caller function.
**/
BOOLEAN
EFIAPI
UpdateDBXFromFile (
  IN EFI_DEVICE_PATH_PROTOCOL  *FilePath
  );

/**
  Update the DBT form base on the input file path info.

  @param FilePath    Point to the file path.

  @retval TRUE   Exit caller function.
  @retval FALSE  Not exit caller function.
**/
BOOLEAN
EFIAPI
UpdateDBTFromFile (
  IN EFI_DEVICE_PATH_PROTOCOL  *FilePath
  );

#endif
