/** @file
  Firmware Update GUID header file.

  Copyright (c) 2024, Arm Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Reference(s):
  - Platform Security Firmware Update for the A-profile specification, 1.0
    (https://developer.arm.com/documentation/den0118/latest)

  @par Glossary:
    - FW    - Firmware
    - FWU   - Firmware Update
    - FWS   - Firmware Storage
    - PSA   - Platform Security update for the A-profile specification
    - IMG   - Image
    - MM    - Management Mode
    - PROP  - Property

**/

#ifndef PSA_MM_FW_UPDATE_H_
#define PSA_MM_FW_UPDATE_H_

/**  Macros defining the firmware store update ABI fuction Id.
     Cf Platform Security Firmware Update for the A-profile specification,
     1.0, 3.4 "Firmware Store Update ABI"
 */
#define PSA_MM_FWU_COMMAND_DISCOVER         0
#define PSA_MM_FWU_COMMAND_BEGIN_STAGING    16
#define PSA_MM_FWU_COMMAND_END_STAGING      17
#define PSA_MM_FWU_COMMAND_CANCEL_STAGING   18
#define PSA_MM_FWU_COMMAND_OPEN             19
#define PSA_MM_FWU_COMMAND_WRITE_STREAM     20
#define PSA_MM_FWU_COMMAND_READ_STREAM      21
#define PSA_MM_FWU_COMMAND_COMMIT           22
#define PSA_MM_FWU_COMMAND_ACCEPT_IMAGE     23
#define PSA_MM_FWU_COMMAND_SELECT_PREVIOUS  24
#define PSA_MM_FWU_COMMAND_MAX_ID           25

/** Firmware Update Protocol error codes.
    Cf Platform Security Firmware Update for the A-profile specification,
    1.0, 3.4.2.11 "Return Status"
 */
#define PSA_MM_FWU_SUCCESS        0
#define PSA_MM_FWU_UNKNOWN        (-1)
#define PSA_MM_FWU_BUSY           (-2)
#define PSA_MM_FWU_OUT_OF_BOUNDS  (-3)
#define PSA_MM_FWU_AUTH_FAIL      (-4)
#define PSA_MM_FWU_NO_PERMISSION  (-5)
#define PSA_MM_FWU_DENIED         (-6)
#define PSA_MM_FWU_RESUME         (-7)
#define PSA_MM_FWU_NOT_AVAILABLE  (-8)

/**  Macros defining the firmware store update Agent Service Status.
     Cf Platform Security Firmware Update for the A-profile specification,
     1.0, 3.4.2.1 "fwu_discover"
 */
#define SERVICE_STATUS_OPERATIVE  0
#define SERVICE_STATUS_ERR_INIT   (-1)

/**  Macros defining flags' returns value of fwu_discover.
      Cf Platform Security Firmware Update for the A-profile specification,
      1.0, 3.4.2.1 "fwu_discover"
 */
#define FWU_DISCOVER_FLAGS_PARTIAL_UPDATE_SUPPORT  BIT0

/**  Macros defining IMG_INFO_ENTRY's ClientPermissions's bit values.
     Cf Platform Security Firmware Update for the A-profile specification,
     1.0, 3.3.1 "Image directory"
 */
#define FWU_ACCEPT_AFTER_ACTIVATION  BIT2
#define FWU_READ_PERMISSION          BIT1
#define FWU_WRITE_PERMISSION         BIT0

#define FWU_INVALID_HANDLE  0xffffffffU

/**  Guid to open Firmware Image Directory via fwu_open
      Cf Platform Security Firmware Update for the A-profile specification,
      1.0 Table A3.5 or A3.8 Image properties in given FW bank version 1 & 2.
 */
#define FWU_IMAGE_UNACCEPTED  0
#define FWU_IMAGE_ACCEPTED    BIT0

/**  Firmware bank state for Metadata version 2
      Cf Platform Security Firmware Update for the A-profile specification,
      Table A3.2: Metadata version 2
 */
#define FWU_BANK_STATE_INVALID   0xFF
#define FWU_BANK_STATE_VALID     0xFE
#define FWU_BANK_STATE_ACCEPTED  0xFC

/**  Maximum number of banks for Metadata version 2
      Cf Platform Security Firmware Update for the A-profile specification,
      Table A3.2: Metadata version 2
 */
#define FWU_METADATA_V2_MAX_NUM_BANKS  4

/**  Enum value defining op_type used in fwu_open.
     Cf Platform Security Firmware Update for the A-profile specification,
     1.0, 3.4.2.5 "fwu_open"
 */
typedef enum {
  FwuOpStreamRead,
  FwuOpStreamWrite,
  FwuOpStreamMax
} FWU_OP_TYPE;

/** Below data structure is used for MM communication between PsaFwuLib and
    Firmware Update Service Agent.
 */

#pragma pack (1)

/** Below data structure is used for MM communication between PsaFwuLib and
    Firmware Update Service Agent.
 */

/** Firmware update protocol parameter header for request/return.
 */
typedef union {
  /// Firmware update protocol function id.
  UINT32    Command;

  /// Firmware update protocol function's return value.
  INT32     ResponseStatus;
} PSA_MM_FWU_PARAMETER_HEADER;

/** Firmware update protocol communication data structure.
 */
typedef struct {
  /// Parameter Header.
  PSA_MM_FWU_PARAMETER_HEADER    Header;

  /// Request or Return Data.
  UINT8                          Buffer[];
} PSA_MM_FWU_CMD_DATA;

/** Return data structure of fwu_discover operation.
    Cf Platform Security Firmware Update for the A-profile specification,
    1.0, 3.4.2.1 "fwu_discover"
 */
typedef struct {
  /// Statue of Service provider.
  UINT16    ServiceStatus;

  /// The ABI major version.
  UINT8     VersionMajor;

  /// The ABI minor version.
  UINT8     VersionMinor;

  /// The Offset (in bytes) of the function presence array.
  UINT16    OffFunctionPresence;

  /// The number of entries in the function presence array.
  UINT16    NumFunc;

  /// The maximum number of bytes that payload can contain.
  UINT64    MaxPayloadSize;

  /// The update capabilities. See FWU_DISCOVER_FLAGS_*
  UINT32    Flags;

  /// The Vendor specific capabilities.
  UINT32    VendorSpecificFlags;

  /// Array of bytes indicating functions that are implemented
  UINT8     FunctionPresence[];
} PSA_MM_FWU_DISCOVER_RESP;

/** Request data structure of fwu_begin_staging operation.
    Cf Platform Security Firmware Update for the A-profile specification,
    1.0, 3.4.2.2 "fwu_begin_staging"
 */
typedef struct {
  /// Reserved. Must be zero.
  UINT32      Reserved;

  /// Vendor specific staging flags.
  UINT32      VendorFlags;

  /// The number of elements in the update_guid array.
  UINT32      PartialUpdateCount;

  /// An array of image type GUIDs will be updated.
  EFI_GUID    UpdateGuid[];
} PSA_MM_FWU_BEGIN_STAGING_REQ;

/** Request data structure of fwu_open operation.
    Cf Platform Security Firmware Update for the A-profile specification,
    1.0, 3.4.2.5 "fwu_open"
 */
typedef struct {
  /// Guid of the image to be opened.
  EFI_GUID    ImageTypeGuid;

  /// The operation type.
  UINT8       OperationType;
} PSA_MM_FWU_OPEN_REQ;

/** Return data structure of fwu_open operation.
    Cf Platform Security Firmware Update for the A-profile specification,
    1.0, 3.4.2.5 "fwu_open"
 */
typedef struct {
  /// Image Handle.
  UINT32    Handle;
} PSA_MM_FWU_OPEN_RESP;

/** Request data structure of fwu_read_stream operation.
    Cf Platform Security Firmware Update for the A-profile specification,
    1.0, 3.4.2.7 "fwu_read_stream"
 */
typedef struct {
  /// Open Image Handle.
  UINT32    Handle;
} PSA_MM_FWU_READ_STREAM_REQ;

/** Return data structure of fwu_read_stream operation.
    Cf Platform Security Firmware Update for the A-profile specification,
    1.0, 3.4.2.7 "fwu_read_stream"
 */
typedef struct {
  /// Read bytes
  UINT32    ReadBytes;

  /// Total Image Size.
  UINT32    TotalBytes;

  /// Read Data sized with ReadBytes.
  UINT8     Payload[];
} PSA_MM_FWU_READ_STREAM_RESP;

/** Request data structure of fwu_write_stream operation.
    Cf Platform Security Firmware Update for the A-profile specification,
    1.0, 3.4.2.6 "fwu_write_stream"
 */
typedef struct {
  /// Open Image Handle.
  UINT32    Handle;

  /// Write Data Size.
  UINT32    DataLen;

  /// Write Data.
  UINT8     Payload[];
} PSA_MM_FWU_WRITE_STREAM_REQ;

/** Request data structure of fwu_commit operation.
    Cf Platform Security Firmware Update for the A-profile specification,
    1.0, 3.4.2.8 "fwu_commit"
 */
typedef struct {
  /// Open Image Handle.
  UINT32    Handle;

  /// If positive, Client requests the image to be marked as unaccepted.
  UINT32    AcceptanceReq;

  /** Hint for maximum time (in ns) Update Agent can execute continuously without
      yielding back to the Client.
  */
  UINT32    MaxAtomicLen;
} PSA_MM_FWU_COMMIT_REQ;

/** Return data structure of fwu_commit operation.
    Cf Platform Security Firmware Update for the A-profile specification,
    1.0, 3.4.2.8 "fwu_commit"
 */
typedef struct {
  /// Unit of work already completed by the Update Agent.
  UINT32    Progress;

  /// Units of work the Update Agent must perform until fwu_commit returns success.
  UINT32    TotalWork;
} PSA_MM_FWU_COMMIT_RESP;

/** Return data structure of fwu_accept_image operation.
    Cf Platform Security Firmware Update for the A-profile specification,
    1.0, 3.4.2.9 "fwu_accept_image"
 */
typedef struct {
  /// Reserved, must be zero.
  UINT32      Reserved;

  /// Image type guid to be accepted.
  EFI_GUID    ImageTypeGuid;
} PSA_MM_FWU_ACCEPT_IMAGE_REQ;

/** Image information entry describing Image.
    Cf Platform Security Firmware Update for the A-profile specification,
    1.0, 3.3 "Firmware Store management"
 */
typedef struct {
  /// Guid identifying the image type
  EFI_GUID    ImgTypeGuid;

  /// Access permission for image.
  UINT32      ClientPermissions;

  /// Maximum image size.
  UINT32      ImgMaxSize;

  /// Lowest version of the image that can execute on platform.
  UINT32      LowestAcceptedVersion;

  /// Image version in boot index.
  UINT32      ImgVersion;

  /// Accept status.
  UINT32      Accepted;

  /// Reserved, Must be zero.
  UINT32      Reserved;
} PSA_MM_FWU_IMG_INFO_ENTRY;

/** Image Directory describing detail on the firmware image.
    Cf Platform Security Firmware Update for the A-profile specification,
    1.0, 3.3 "Firmware Store management"
 */
typedef struct {
  /// The version of field in the ImageInfoEntry.
  UINT32                       DirectoryVersion;

  /// The Offset of the ImageInfoEntry.
  UINT32                       ImgInfoOffset;

  /// The number of entries in the ImageInfoEntry.
  UINT32                       NumImages;

  /// Stating if the platform booted with the active bank.
  UINT32                       CorrectBoot;

  /// The size in bytes of an entry in the ImgInfoEntry.
  UINT32                       ImgInfoSize;

  /// Reserved, Must be zero.
  UINT32                       Reserved;

  /// Image info entry array.
  PSA_MM_FWU_IMG_INFO_ENTRY    ImgInfoEntry[];
} PSA_MM_FWU_IMAGE_DIRECTORY;

/** FWU Metadata common header.
 */
typedef struct {
  /// Metadata CRC value.
  UINT32    Crc32;

  /// Metadata version.
  UINT32    Version;

  /// Bank index with which device boots.
  UINT32    ActiveIndex;

  /// Previous bank index with which device booted successfully.
  UINT32    PreviousActiveIndex;
} PSA_MM_FWU_METADATA_COMMON_HEADER;

/** FWU image entry common header.
 */
typedef struct {
  /// GUID identifying the image type.
  EFI_GUID    ImageTypeGuid;

  /// GUID of the storage volume where the image is located.
  EFI_GUID    LocationGuid;
} PSA_MM_FWU_IMAGE_ENTRY_COMMON_HEADER;

/** Image properties in a given FW bank version 1 (image bank info).
    Cf Platform Security Firmware Update for the A-profile specification,
    Table A3.5 & Table A3.8: Image Properties
 */
typedef struct {
  /// UUID of the image in this bank
  EFI_GUID    ImageGuid;

  /* [0]: bit describing the image acceptance status
   *      1 means the image is accepted
   * [31:1]: MBZ
   */
  UINT32      Accepted;

  /// reserved (MBZ)
  UINT32      Reserved;
} PSA_MM_FWU_IMAGE_PROPERTIES;

/** Metadata image entry version 2.
    Cf Platform Security Firmware Update for the A-profile specification,
    1.0, A3.4 "Metadata image entry version 2"
 */
typedef struct {
  /// Image entry common header
  PSA_MM_FWU_IMAGE_ENTRY_COMMON_HEADER    Header;

  /// Properties of images with img_type_uuid in the different FW banks.
  PSA_MM_FWU_IMAGE_PROPERTIES             ImageBankInfo[];
} PSA_MM_FWU_IMAGE_ENTRY_V2;

typedef struct {
  /// Number of banks.
  UINT8                        NumBanks;

  /// Reserved (MBZ).
  UINT8                        Reserved;

  /// Number of images per banks.
  UINT16                       NumImages;

  /// Image Entry Size
  UINT16                       ImageEntrySize;

  /// Bank Info Entry Size;
  UINT16                       BankInfoEntrySize;

  /// Array of image entries;
  PSA_MM_FWU_IMAGE_ENTRY_V2    ImageEntry[];
} PSA_MM_FWU_FW_STORE_DESC_V2;

/** FWU metadata filled by the updater and consumed by TF-A for
    various purposes as below:
      1. Get active FW bank.
      2. Rollback to previous working FW bank.
      3. Get properties of all images present in all banks.

   Cf Platform Security Firmware Update for the A-profile specification,
   1.0, A.3.2 "Metadata version 2"
 */
typedef struct {
  /// Metadata Common Header.
  PSA_MM_FWU_METADATA_COMMON_HEADER    Header;

  /// Metadata size
  UINT32                               MetadataSize;

  /// The offset, from the start of this data structure, to FwStoreDesc;
  UINT16                               DescriptorOffset;

  /// reserved (MBZ).
  UINT16                               Reserved_0;

  /// Bank State.
  UINT8                                BankState[FWU_METADATA_V2_MAX_NUM_BANKS];

  /// reserved (MBZ).
  UINT32                               Reserved_1;

  /// Image entry information
  PSA_MM_FWU_FW_STORE_DESC_V2          FwStoreDesc[];
} PSA_MM_FWU_METADATA_V2;

#pragma pack ()

/** Helper macro for getting flexible data.
 */
#define GET_FWU_DATA_BUFFER(Data) \
  ((VOID *)((UINT8 *) Data + sizeof (*Data)))

/** Helper macro for getting Firmware Storage Descriptor v2 from Metadata v2.
 */
#define GET_FWU_FW_STORE_DESC_V2(Metadata)                                    \
  ((PSA_MM_FWU_FW_STORE_DESC_V2 *)                                            \
   ((VOID *) Metadata + ((PSA_MM_FWU_METADATA_V2 *) Metadata)->DescriptorOffset)) \


/** Helper macro for getting Image Entry v2 from Firmware Storage Descriptor v2.
 */
#define GET_FWU_IMAGE_ENTRY_V2(FwFwsStoreDesc, Idx)                                 \
  ((PSA_MM_FWU_IMAGE_ENTRY_V2 *)                                                    \
   ((VOID *) FwFwsStoreDesc + OFFSET_OF (PSA_MM_FWU_FW_STORE_DESC_V2, ImageEntry) + \
    ((Idx) * (OFFSET_OF (PSA_MM_FWU_IMAGE_ENTRY_V2, ImageBankInfo) +                \
      (sizeof (PSA_MM_FWU_IMAGE_PROPERTIES) * (FwFwsStoreDesc->NumBanks))))))

/** Helper macro for getting Image Bank Infofrom Image entry v2
 */
#define GET_FWU_IMAGE_BANK_INFO_V2(ImageEntry, Idx)                               \
    ((PSA_MM_FWU_IMAGE_PROPERTIES *)                                              \
    ((VOID *) ImageEntry + OFFSET_OF (PSA_MM_FWU_IMAGE_ENTRY_V2, ImageBankInfo) + \
     ((Idx) * (sizeof (PSA_MM_FWU_IMAGE_PROPERTIES)))))

/** Helper macro for getting Image Info Entry from Image Directory.
 */
#define GET_FWU_IMG_INFO_ENTRY(ImageDirectory, Idx)                           \
    ((PSA_MM_FWU_IMG_INFO_ENTRY *)                                            \
    ((UINT8 *) ImageDirectory + sizeof (PSA_MM_FWU_IMAGE_DIRECTORY) +         \
     ((Idx) * (sizeof (PSA_MM_FWU_IMG_INFO_ENTRY)))))

extern EFI_GUID  gEfiMmFwuCommunicationGuid;

#endif // PSA_MM_FW_UPDATE_H_
