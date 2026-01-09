/** @file

  IPMI Blob Transfer driver

  Copyright (c) 2022-2024, NVIDIA CORPORATION & AFFILIATES. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @Par: https://github.com/openbmc/phosphor-ipmi-blobs/blob/master/README.md
**/

#ifndef EDKII_IPMI_BLOB_TRANSFER_H_
#define EDKII_IPMI_BLOB_TRANSFER_H_

#include <Library/IpmiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <IndustryStandard/Ipmi.h>
#include <IndustryStandard/IpmiNetFnOem.h>

#define IPMI_OEM_BLOB_TRANSFER_CMD  0x80
#define BLOB_MAX_DATA_PER_PACKET    64

#define BLOB_TRANSFER_STAT_OPEN_R        BIT0
#define BLOB_TRANSFER_STAT_OPEN_W        BIT1
#define BLOB_TRANSFER_STAT_COMMITING     BIT2
#define BLOB_TRANSFER_STAT_COMMITTED     BIT3
#define BLOB_TRANSFER_STAT_COMMIT_ERROR  BIT4
// Bits 5-7 are reserved
// Bits 8-15 are blob-specific definitions

//
// OpenBMC OEN code in little endian format
//
CONST UINT8  OpenBmcOen[] = { 0xCF, 0xC2, 0x00 };

//
//  Blob Transfer Function Prototypes
//

/**
  This function retrieves the count of blob transfers available through the IPMI.

  @param[out]        Count       The number of active blobs

  @retval EFI_SUCCESS            Successfully retrieved the number of active blobs.
  @retval Other                  An error occurred
**/
typedef
EFI_STATUS
(EFIAPI *EDKII_IPMI_BLOB_TRANSFER_PROTOCOL_GET_COUNT)(
  OUT UINT32 *Count
  );

/**
  This function enumerates blob transfers available through the IPMI.

  @param[in]         BlobIndex       The 0-based Index of the blob to enumerate
  @param[out]        BlobId          The ID of the blob

  @retval EFI_SUCCESS                Successfully enumerated the blob.
  @retval Other                      An error occurred
**/
typedef
EFI_STATUS
(EFIAPI *EDKII_IPMI_BLOB_TRANSFER_PROTOCOL_ENUMERATE)(
  IN  UINT32  BlobIndex,
  OUT CHAR8   *BlobId
  );

/**
  This function is designed to open a session for a specific blob
  identified by its ID, using the IPMI.

  @param[in]         BlobId          The ID of the blob to open
  @param[in]         Flags           Flags to control how the blob is opened
                                     Available flags are:
                                       BLOB_TRANSFER_STAT_OPEN_R
                                       BLOB_TRANSFER_STAT_OPEN_W
                                       BLOB_TRANSFER_STAT_COMMITING
                                       BLOB_TRANSFER_STAT_COMMITTED
                                       BLOB_TRANSFER_STAT_COMMIT_ERROR
  @param[out]        SessionId       A unique session identifier

  @retval EFI_SUCCESS                Successfully opened the blob.
  @retval Other                      An error occurred
**/
typedef
EFI_STATUS
(EFIAPI *EDKII_IPMI_BLOB_TRANSFER_PROTOCOL_OPEN)(
  IN  CHAR8  *BlobId,
  IN  UINT16 Flags,
  OUT UINT16 *SessionId
  );

/**
  This function reads data from a blob over the IPMI.

  @param[in]         SessionId       The session ID returned from a call to BlobOpen
  @param[in]         Offset          The offset of the blob from which to start reading
  @param[in]         RequestedSize   The length of data to read
  @param[out]        Data            Data read from the blob

  @retval EFI_SUCCESS                Successfully read from the blob.
  @retval EFI_BAD_BUFFER_SIZE        RequestedSize is bigger than BLOB_MAX_DATA_PER_PACKET
  @retval Other                      An error occurred
**/
typedef
EFI_STATUS
(EFIAPI *EDKII_IPMI_BLOB_TRANSFER_PROTOCOL_READ)(
  IN  UINT16      SessionId,
  IN  UINT32      Offset,
  IN  UINT32      RequestedSize,
  OUT UINT8       *Data
  );

/**
  This function writes data to a blob over the IPMI.

  @param[in]         SessionId       The session ID returned from a call to BlobOpen
  @param[in]         Offset          The offset of the blob from which to start writing
  @param[in]         Data            A pointer to the data to write
  @param[in]         WriteLength     The length to write

  @retval EFI_SUCCESS                Successfully wrote to the blob.
  @retval EFI_BAD_BUFFER_SIZE        WriteLength is bigger than BLOB_MAX_DATA_PER_PACKET
  @retval Other                      An error occurred
**/
typedef
EFI_STATUS
(EFIAPI *EDKII_IPMI_BLOB_TRANSFER_PROTOCOL_WRITE)(
  IN  UINT16      SessionId,
  IN  UINT32      Offset,
  IN  UINT8       *Data,
  IN  UINT32      WriteLength
  );

/**
  This function commits data to a blob over the IPMI.

  @param[in]         SessionId        The session ID returned from a call to BlobOpen
  @param[in]         CommitDataLength The length of data to commit to the blob
  @param[in]         CommitData       A pointer to the data to commit. This is optional.

  @retval EFI_SUCCESS                Successful commit to the blob.
  @retval Other                      An error occurred
**/
typedef
EFI_STATUS
(EFIAPI *EDKII_IPMI_BLOB_TRANSFER_PROTOCOL_COMMIT)(
  IN  UINT16      SessionId,
  IN  UINT8       CommitDataLength,
  IN  UINT8       *CommitData OPTIONAL
  );

/**
  This function close a session associated with a blob transfer over the IPMI.

  @param[in]         SessionId       The session ID returned from a call to BlobOpen

  @retval EFI_SUCCESS                The blob was closed.
  @retval Other                      An error occurred
**/
typedef
EFI_STATUS
(EFIAPI *EDKII_IPMI_BLOB_TRANSFER_PROTOCOL_CLOSE)(
  IN  UINT16      SessionId
  );

/**
  This function deletes a specific blob identified by its ID over the IPMI.

  @param[in]         BlobId          The BlobId to be deleted

  @retval EFI_SUCCESS                The blob was deleted.
  @retval Other                      An error occurred
**/
typedef
EFI_STATUS
(EFIAPI *EDKII_IPMI_BLOB_TRANSFER_PROTOCOL_DELETE)(
  IN  CHAR8 *BlobId
  );

/**
  This function retrieve the status of a specific blob identified by BlobId from an IPMI.

  @param[in]         BlobId          The Blob ID to gather statistics for
  @param[out]        BlobState       The current state of the blob
  @param[out]        Size            Size in bytes of the blob. This is optional.
  @param[out]        MetadataLength  Length of the optional metadata
  @param[out]        Metadata        Optional blob-specific metadata

  @retval EFI_SUCCESS                The blob statistics were successfully gathered.
  @retval Other                      An error occurred
**/
typedef
EFI_STATUS
(EFIAPI *EDKII_IPMI_BLOB_TRANSFER_PROTOCOL_STAT)(
  IN  CHAR8  *BlobId,
  OUT UINT16 *BlobState,
  OUT UINT32 *Size OPTIONAL,
  OUT UINT8  *MetadataLength OPTIONAL,
  OUT UINT8  *Metadata OPTIONAL
  );

/**
  This function query the status of a blob transfer session in an IPMI.

  @param[in]         SessionId       The ID of the session to gather statistics for
  @param[out]        BlobState       The current state of the blob
  @param[out]        Size            Size in bytes of the blob. This is optional.
  @param[out]        MetadataLength  Length of the optional metadata
  @param[out]        Metadata        Optional blob-specific metadata

  @retval EFI_SUCCESS                The blob statistics were successfully gathered.
  @retval Other                      An error occurred
**/
typedef
EFI_STATUS
(EFIAPI *EDKII_IPMI_BLOB_TRANSFER_PROTOCOL_SESSION_STAT)(
  IN  UINT16 SessionId,
  OUT UINT16 *BlobState,
  OUT UINT32 *Size OPTIONAL,
  OUT UINT8  *MetadataLength OPTIONAL,
  OUT UINT8  *Metadata OPTIONAL
  );

/**
  This function writes metadata to a blob associated with a session in an IPMI.

  @param[in]         SessionId       The ID of the session to write metadata for
  @param[in]         Offset          The offset of the metadata to write to
  @param[in]         Data            The data to write to the metadata
  @param[in]         WriteLength     The length to write

  @retval EFI_SUCCESS                The blob metadata was successfully written.
  @retval EFI_BAD_BUFFER_SIZE        WriteLength is bigger than BLOB_MAX_DATA_PER_PACKET
  @retval Other                      An error occurred
**/
typedef
EFI_STATUS
(EFIAPI *EDKII_IPMI_BLOB_TRANSFER_PROTOCOL_WRITE_META)(
  IN  UINT16      SessionId,
  IN  UINT32      Offset,
  IN  UINT8       *Data,
  IN  UINT32      WriteLength
  );

//
// Structure of EDKII_IPMI_BLOB_TRANSFER_PROTOCOL
//
struct _EDKII_IPMI_BLOB_TRANSFER_PROTOCOL {
  EDKII_IPMI_BLOB_TRANSFER_PROTOCOL_GET_COUNT       BlobGetCount;
  EDKII_IPMI_BLOB_TRANSFER_PROTOCOL_ENUMERATE       BlobEnumerate;
  EDKII_IPMI_BLOB_TRANSFER_PROTOCOL_OPEN            BlobOpen;
  EDKII_IPMI_BLOB_TRANSFER_PROTOCOL_READ            BlobRead;
  EDKII_IPMI_BLOB_TRANSFER_PROTOCOL_WRITE           BlobWrite;
  EDKII_IPMI_BLOB_TRANSFER_PROTOCOL_COMMIT          BlobCommit;
  EDKII_IPMI_BLOB_TRANSFER_PROTOCOL_CLOSE           BlobClose;
  EDKII_IPMI_BLOB_TRANSFER_PROTOCOL_DELETE          BlobDelete;
  EDKII_IPMI_BLOB_TRANSFER_PROTOCOL_STAT            BlobStat;
  EDKII_IPMI_BLOB_TRANSFER_PROTOCOL_SESSION_STAT    BlobSessionStat;
  EDKII_IPMI_BLOB_TRANSFER_PROTOCOL_WRITE_META      BlobWriteMeta;
};

typedef struct _EDKII_IPMI_BLOB_TRANSFER_PROTOCOL EDKII_IPMI_BLOB_TRANSFER_PROTOCOL;

extern EFI_GUID  gEdkiiIpmiBlobTransferProtocolGuid;

#endif
