/** @file
  The header file of HII Config Access protocol implementation of SecureBoot
  configuration module.

Copyright (c) 2011 - 2014, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

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
#include <Guid/MdeModuleHii.h>
#include <Guid/AuthenticatedVariableFormat.h>
#include <Guid/FileSystemVolumeLabelInfo.h>
#include <Guid/ImageAuthentication.h>
#include <Guid/FileInfo.h>

#include "SecureBootConfigNvData.h"

//
// Tool generated IFR binary data and String package data
//
extern  UINT8                      SecureBootConfigBin[];
extern  UINT8                      SecureBootConfigDxeStrings[];

//
// Shared IFR form update data
//
extern  VOID                       *mStartOpCodeHandle;
extern  VOID                       *mEndOpCodeHandle;
extern  EFI_IFR_GUID_LABEL         *mStartLabel;
extern  EFI_IFR_GUID_LABEL         *mEndLabel;

#define MAX_CHAR              480
#define TWO_BYTE_ENCODE       0x82

//
// SHA-1 digest size in bytes.
//
#define SHA1_DIGEST_SIZE    20
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
#define MAX_DIGEST_SIZE    SHA512_DIGEST_SIZE

#define WIN_CERT_UEFI_RSA2048_SIZE               256

//
// Support hash types
//
#define HASHALG_SHA1                           0x00000000
#define HASHALG_SHA224                         0x00000001
#define HASHALG_SHA256                         0x00000002
#define HASHALG_SHA384                         0x00000003
#define HASHALG_SHA512                         0x00000004
#define HASHALG_RAW                            0x00000005
#define HASHALG_MAX                            0x00000005


#define SECUREBOOT_MENU_OPTION_SIGNATURE   SIGNATURE_32 ('S', 'b', 'M', 'u')
#define SECUREBOOT_MENU_ENTRY_SIGNATURE    SIGNATURE_32 ('S', 'b', 'M', 'r')

typedef struct {
  EFI_DEVICE_PATH_PROTOCOL  Header;
  EFI_GUID                  Guid;
  UINT8                     VendorDefinedData[1];
} VENDOR_DEVICE_PATH_WITH_DATA;

typedef struct {
  EFI_DEVICE_PATH_PROTOCOL  Header;
  UINT16                    NetworkProtocol;
  UINT16                    LoginOption;
  UINT64                    Lun;
  UINT16                    TargetPortalGroupTag;
  CHAR16                    TargetName[1];
} ISCSI_DEVICE_PATH_WITH_NAME;

typedef enum _FILE_EXPLORER_DISPLAY_CONTEXT {
  FileExplorerDisplayFileSystem,
  FileExplorerDisplayDirectory,
  FileExplorerDisplayUnknown
} FILE_EXPLORER_DISPLAY_CONTEXT;

typedef enum _FILE_EXPLORER_STATE {
  FileExplorerStateInActive                      = 0,
  FileExplorerStateEnrollPkFile,
  FileExplorerStateEnrollKekFile,
  FileExplorerStateEnrollSignatureFileToDb,
  FileExplorerStateEnrollSignatureFileToDbx,
  FileExplorerStateEnrollSignatureFileToDbt,
  FileExplorerStateUnknown
} FILE_EXPLORER_STATE;

typedef struct {
  CHAR16  *Str;
  UINTN   Len;
  UINTN   Maxlen;
} POOL_PRINT;

typedef
VOID
(*DEV_PATH_FUNCTION) (
  IN OUT POOL_PRINT       *Str,
  IN VOID                 *DevPath
  );

typedef struct {
  UINT8             Type;
  UINT8             SubType;
  DEV_PATH_FUNCTION Function;
} DEVICE_PATH_STRING_TABLE;

typedef struct {
  UINTN             Signature;
  LIST_ENTRY        Head;
  UINTN             MenuNumber;
} SECUREBOOT_MENU_OPTION;

extern  SECUREBOOT_MENU_OPTION     FsOptionMenu;
extern  SECUREBOOT_MENU_OPTION     DirectoryMenu;

typedef struct {
  UINTN             Signature;
  LIST_ENTRY        Link;
  UINTN             OptionNumber;
  UINT16            *DisplayString;
  UINT16            *HelpString;
  EFI_STRING_ID     DisplayStringToken;
  EFI_STRING_ID     HelpStringToken;
  VOID              *FileContext;
} SECUREBOOT_MENU_ENTRY;

typedef struct {
  EFI_HANDLE                        Handle;
  EFI_DEVICE_PATH_PROTOCOL          *DevicePath;
  EFI_FILE_HANDLE                   FHandle;
  UINT16                            *FileName;
  EFI_FILE_SYSTEM_VOLUME_LABEL      *Info;

  BOOLEAN                           IsRoot;
  BOOLEAN                           IsDir;
  BOOLEAN                           IsRemovableMedia;
  BOOLEAN                           IsLoadFile;
  BOOLEAN                           IsBootLegacy;
} SECUREBOOT_FILE_CONTEXT;


//
// We define another format of 5th directory entry: security directory
//
typedef struct {
  UINT32               Offset;      // Offset of certificate
  UINT32               SizeOfCert;  // size of certificate appended
} EFI_IMAGE_SECURITY_DATA_DIRECTORY;

typedef enum{
  ImageType_IA32,
  ImageType_X64
} IMAGE_TYPE;

///
/// HII specific Vendor Device Path definition.
///
typedef struct {
  VENDOR_DEVICE_PATH                VendorDevicePath;
  EFI_DEVICE_PATH_PROTOCOL          End;
} HII_VENDOR_DEVICE_PATH;

typedef struct {
  UINTN                             Signature;

  EFI_HII_CONFIG_ACCESS_PROTOCOL    ConfigAccess;
  EFI_HII_HANDLE                    HiiHandle;
  EFI_HANDLE                        DriverHandle;

  FILE_EXPLORER_STATE               FeCurrentState;
  FILE_EXPLORER_DISPLAY_CONTEXT     FeDisplayContext;

  SECUREBOOT_MENU_ENTRY             *MenuEntry;
  SECUREBOOT_FILE_CONTEXT           *FileContext;

  EFI_GUID                          *SignatureGUID;
} SECUREBOOT_CONFIG_PRIVATE_DATA;

extern SECUREBOOT_CONFIG_PRIVATE_DATA      mSecureBootConfigPrivateDateTemplate;

#define SECUREBOOT_CONFIG_PRIVATE_DATA_SIGNATURE     SIGNATURE_32 ('S', 'E', 'C', 'B')
#define SECUREBOOT_CONFIG_PRIVATE_FROM_THIS(a)  CR (a, SECUREBOOT_CONFIG_PRIVATE_DATA, ConfigAccess, SECUREBOOT_CONFIG_PRIVATE_DATA_SIGNATURE)

//
// Cryptograhpic Key Information
//
#pragma pack(1)
typedef struct _CPL_KEY_INFO {
  UINT32        KeyLengthInBits;    // Key Length In Bits
  UINT32        BlockSize;          // Operation Block Size in Bytes
  UINT32        CipherBlockSize;    // Output Cipher Block Size in Bytes
  UINT32        KeyType;            // Key Type
  UINT32        CipherMode;         // Cipher Mode for Symmetric Algorithm
  UINT32        Flags;              // Additional Key Property Flags
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
  WIN_CERTIFICATE Hdr;
  UINT8           CertData[1];
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
  IN OUT SECUREBOOT_CONFIG_PRIVATE_DATA    *PrivateData
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
  IN CONST EFI_HII_CONFIG_ACCESS_PROTOCOL        *This,
  IN CONST EFI_STRING                            Request,
       OUT EFI_STRING                            *Progress,
       OUT EFI_STRING                            *Results
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
  IN CONST EFI_HII_CONFIG_ACCESS_PROTOCOL      *This,
  IN CONST EFI_STRING                          Configuration,
       OUT EFI_STRING                          *Progress
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
  IN CONST EFI_HII_CONFIG_ACCESS_PROTOCOL      *This,
  IN     EFI_BROWSER_ACTION                    Action,
  IN     EFI_QUESTION_ID                       QuestionId,
  IN     UINT8                                 Type,
  IN     EFI_IFR_TYPE_VALUE                    *Value,
     OUT EFI_BROWSER_ACTION_REQUEST            *ActionRequest
  );


/**
  This function converts an input device structure to a Unicode string.

  @param[in] DevPath                  A pointer to the device path structure.

  @return A new allocated Unicode string that represents the device path.

**/
CHAR16 *
EFIAPI
DevicePathToStr (
  IN EFI_DEVICE_PATH_PROTOCOL     *DevPath
  );


/**
  Clean up the dynamic opcode at label and form specified by both LabelId.

  @param[in] LabelId         It is both the Form ID and Label ID for opcode deletion.
  @param[in] PrivateData     Module private data.

**/
VOID
CleanUpPage (
  IN UINT16                           LabelId,
  IN SECUREBOOT_CONFIG_PRIVATE_DATA   *PrivateData
  );


/**
  Update the file explorer page with the refreshed file system.

  @param[in] PrivateData     Module private data.
  @param[in] KeyValue        Key value to identify the type of data to expect.

  @retval  TRUE           Inform the caller to create a callback packet to exit file explorer.
  @retval  FALSE          Indicate that there is no need to exit file explorer.

**/
BOOLEAN
UpdateFileExplorer (
  IN SECUREBOOT_CONFIG_PRIVATE_DATA   *PrivateData,
  IN UINT16                           KeyValue
  );


/**
  Free resources allocated in Allocate Rountine.

  @param[in, out]  MenuOption        Menu to be freed

**/
VOID
FreeMenu (
  IN OUT SECUREBOOT_MENU_OPTION        *MenuOption
  );


/**
  Read file content into BufferPtr, the size of the allocate buffer
  is *FileSize plus AddtionAllocateSize.

  @param[in]       FileHandle            The file to be read.
  @param[in, out]  BufferPtr             Pointers to the pointer of allocated buffer.
  @param[out]      FileSize              Size of input file
  @param[in]       AddtionAllocateSize   Addtion size the buffer need to be allocated.
                                         In case the buffer need to contain others besides the file content.

  @retval   EFI_SUCCESS                  The file was read into the buffer.
  @retval   EFI_INVALID_PARAMETER        A parameter was invalid.
  @retval   EFI_OUT_OF_RESOURCES         A memory allocation failed.
  @retval   others                       Unexpected error.

**/
EFI_STATUS
ReadFileContent (
  IN      EFI_FILE_HANDLE           FileHandle,
  IN OUT  VOID                      **BufferPtr,
     OUT  UINTN                     *FileSize,
  IN      UINTN                     AddtionAllocateSize
  );


/**
  Close an open file handle.

  @param[in] FileHandle           The file handle to close.

**/
VOID
CloseFile (
  IN EFI_FILE_HANDLE   FileHandle
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
  IN     CONST UINTN       *Integer,
  IN     UINTN             IntSizeInWords,
     OUT UINT8             *OctetString,
  IN     UINTN             OSSizeInBytes
  );


/**
  Convert a String to Guid Value.

  @param[in]   Str        Specifies the String to be converted.
  @param[in]   StrLen     Number of Unicode Characters of String (exclusive \0)
  @param[out]  Guid       Return the result Guid value.

  @retval    EFI_SUCCESS           The operation is finished successfully.
  @retval    EFI_NOT_FOUND         Invalid string.

**/
EFI_STATUS
StringToGuid (
  IN   CHAR16           *Str,
  IN   UINTN            StrLen,
  OUT  EFI_GUID         *Guid
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

#endif