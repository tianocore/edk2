/** @file

  Headers for IPMI Blob Transfer driver

  Copyright (c) 2022-2024, NVIDIA CORPORATION & AFFILIATES. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/IpmiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>

#ifndef INTERNAL_IPMI_BLOB_TRANSFER_H_
#define INTERNAL_IPMI_BLOB_TRANSFER_H_

#define PROTOCOL_RESPONSE_OVERHEAD  (4 * sizeof (UINT8))       // 1 byte completion code + 3 bytes OEN

// Subcommands for this protocol
typedef enum {
  IpmiBlobTransferSubcommandGetCount = 0,
  IpmiBlobTransferSubcommandEnumerate,
  IpmiBlobTransferSubcommandOpen,
  IpmiBlobTransferSubcommandRead,
  IpmiBlobTransferSubcommandWrite,
  IpmiBlobTransferSubcommandCommit,
  IpmiBlobTransferSubcommandClose,
  IpmiBlobTransferSubcommandDelete,
  IpmiBlobTransferSubcommandStat,
  IpmiBlobTransferSubcommandSessionStat,
  IpmiBlobTransferSubcommandWriteMeta,
} IPMI_BLOB_TRANSFER_SUBCOMMANDS;

  #pragma pack(1)

typedef struct {
  UINT8    OEN[3];
  UINT8    SubCommand;
} IPMI_BLOB_TRANSFER_HEADER;

//
// Command 0 - BmcBlobGetCount
// The BmcBlobGetCount command expects to receive an empty body.
// The BMC will return the number of enumerable blobs
//
typedef struct {
  UINT32    BlobCount;
} IPMI_BLOB_TRANSFER_GET_COUNT_RESPONSE;

//
// Command 1 - BmcBlobEnumerate
// The BmcBlobEnumerate command expects to receive a body of:
//
typedef struct {
  UINT32    BlobIndex; // 0-based index of blob to receive
} IPMI_BLOB_TRANSFER_BLOB_ENUMERATE_SEND_DATA;

typedef struct {
  CHAR8    BlobId[BLOB_MAX_DATA_PER_PACKET];
} IPMI_BLOB_TRANSFER_BLOB_ENUMERATE_RESPONSE;

//
// Command 2 - BmcBlobOpen
// The BmcBlobOpen command expects to receive a body of:
//
typedef struct {
  UINT16    Flags;
  CHAR8     BlobId[BLOB_MAX_DATA_PER_PACKET];
} IPMI_BLOB_TRANSFER_BLOB_OPEN_SEND_DATA;

#define BLOB_OPEN_FLAG_READ   0
#define BLOB_OPEN_FLAG_WRITE  1
// Bits 2-7 are reserved
// Bits 8-15 are blob-specific definitions

typedef struct {
  UINT16    SessionId;
} IPMI_BLOB_TRANSFER_BLOB_OPEN_RESPONSE;

//
// Command 3 - BmcBlobRead
// The BmcBlobRead command expects to receive a body of:
//
typedef struct {
  UINT16    SessionId; // Returned from BlobOpen
  UINT32    Offset;
  UINT32    RequestedSize;
} IPMI_BLOB_TRANSFER_BLOB_READ_SEND_DATA;

typedef struct {
  UINT8    Data[BLOB_MAX_DATA_PER_PACKET];
} IPMI_BLOB_TRANSFER_BLOB_READ_RESPONSE;

//
// Command 4 - BmcBlobWrite
// The BmcBlobWrite command expects to receive a body of:
//
typedef struct {
  UINT16    SessionId; // Returned from BlobOpen
  UINT32    Offset;
  UINT8     Data[BLOB_MAX_DATA_PER_PACKET];
} IPMI_BLOB_TRANSFER_BLOB_WRITE_SEND_DATA;

//
// Command 5 - BmcBlobCommit
// The BmcBlobCommit command expects to receive a body of:
//
typedef struct {
  UINT16    SessionId; // Returned from BlobOpen
  UINT8     CommitDataLength;
  UINT8     CommitData[BLOB_MAX_DATA_PER_PACKET];
} IPMI_BLOB_TRANSFER_BLOB_COMMIT_SEND_DATA;

//
// Command 6 - BmcBlobClose
// The BmcBlobClose command expects to receive a body of:
//
typedef struct {
  UINT16    SessionId; // Returned from BlobOpen
} IPMI_BLOB_TRANSFER_BLOB_CLOSE_SEND_DATA;

//
// Command 7 - BmcBlobDelete
// NOTE: This command will fail if there are open sessions for this blob
// The BmcBlobDelete command expects to receive a body of:
//
typedef struct {
  CHAR8    BlobId[BLOB_MAX_DATA_PER_PACKET];
} IPMI_BLOB_TRANSFER_BLOB_DELETE_SEND_DATA;

//
// Command 8 - BmcBlobStat
// This command returns statistics about a blob.
// This command expects to receive a body of:
//
typedef struct {
  CHAR8    BlobId[BLOB_MAX_DATA_PER_PACKET];
} IPMI_BLOB_TRANSFER_BLOB_STAT_SEND_DATA;

typedef struct {
  UINT16    BlobState;
  UINT32    Size; // Size in bytes of the blob
  UINT8     MetaDataLen;
  UINT8     MetaData[BLOB_MAX_DATA_PER_PACKET];
} IPMI_BLOB_TRANSFER_BLOB_STAT_RESPONSE;

//
// Command 9 - BmcBlobSessionStat
// Returns same data as BmcBlobState expect for a session, not a blob
// This command expects to receive a body of:
//
typedef struct {
  UINT16    SessionId;
} IPMI_BLOB_TRANSFER_BLOB_SESSION_STAT_SEND_DATA;

typedef struct {
  UINT16    BlobState;
  UINT32    Size; // Size in bytes of the blob
  UINT8     MetaDataLen;
  UINT8     MetaData[BLOB_MAX_DATA_PER_PACKET];
} IPMI_BLOB_TRANSFER_BLOB_SESSION_STAT_RESPONSE;

//
// Command 10 - BmcBlobWriteMeta
// The BmcBlobWriteMeta command expects to receive a body of:
//
typedef struct {
  UINT16    SessionId;
  UINT32    Offset;
  UINT8     Data[BLOB_MAX_DATA_PER_PACKET];
} IPMI_BLOB_TRANSFER_BLOB_WRITE_META_SEND_DATA;

#define IPMI_BLOB_TRANSFER_BLOB_WRITE_META_RESPONSE  NULL

  #pragma pack()

/**
  Calculate CRC-16-CCITT with poly of 0x1021

  @param[in]  Data              The target data.
  @param[in]  DataSize          The target data size.

  @return UINT16     The CRC16 value.

**/
UINT16
CalculateCrc16Ccitt (
  IN UINT8  *Data,
  IN UINTN  DataSize
  );

/**
  This function does blob transfer over IPMI command.

  @param[in]  SubCommand        The specific sub-command to be executed as part of
                                the blob transfer operation.
  @param[in]  SendData          A pointer to the data buffer that contains the data to be sent.
                                When SendDataSize is zero, SendData is not used.
  @param[in]  SendDataSize      The size of the data to be sent, in bytes. This is optional.
  @param[out] ResponseData      A pointer to the buffer where the response data will be stored.
  @param[out] ResponseDataSize  A pointer to a variable that will hold the size of the response
                                data received.

  @retval EFI_SUCCESS            Successfully sends blob data.
  @retval EFI_OUT_OF_RESOURCES   Memory allocation fails.
  @retval EFI_PROTOCOL_ERROR     Communication errors.
  @retval EFI_CRC_ERROR          Data integrity checks fail.
  @retval Other                  An error occurred

**/
EFI_STATUS
IpmiBlobTransferSendIpmi (
  IN  UINT8   SubCommand,
  IN  UINT8   *SendData OPTIONAL,
  IN  UINT32  SendDataSize OPTIONAL,
  OUT UINT8   *ResponseData,
  OUT UINT32  *ResponseDataSize
  );

/**
  This function retrieves the count of blob transfers available through the IPMI.

  @param[out]        Count       The number of active blobs

  @retval EFI_SUCCESS            Successfully retrieved the number of active blobs.
  @retval Other                  An error occurred
**/
EFI_STATUS
IpmiBlobTransferGetCount (
  OUT UINT32  *Count
  );

/**
  This function enumerates blob transfers available through the IPMI.

  @param[in]         BlobIndex       The 0-based Index of the blob to enumerate
  @param[out]        BlobId          The ID of the blob

  @retval EFI_SUCCESS                Successfully enumerated the blob.
  @retval Other                      An error occurred
**/
EFI_STATUS
IpmiBlobTransferEnumerate (
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
EFI_STATUS
IpmiBlobTransferOpen (
  IN  CHAR8   *BlobId,
  IN  UINT16  Flags,
  OUT UINT16  *SessionId
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
EFI_STATUS
IpmiBlobTransferRead (
  IN  UINT16  SessionId,
  IN  UINT32  Offset,
  IN  UINT32  RequestedSize,
  OUT UINT8   *Data
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
EFI_STATUS
IpmiBlobTransferWrite (
  IN  UINT16  SessionId,
  IN  UINT32  Offset,
  IN  UINT8   *Data,
  IN  UINT32  WriteLength
  );

/**
  This function commits data to a blob over the IPMI.

  @param[in]         SessionId        The session ID returned from a call to BlobOpen
  @param[in]         CommitDataLength The length of data to commit to the blob
  @param[in]         CommitData       A pointer to the data to commit. This is optional.

  @retval EFI_SUCCESS                Successful commit to the blob.
  @retval Other                      An error occurred
**/
EFI_STATUS
IpmiBlobTransferCommit (
  IN  UINT16  SessionId,
  IN  UINT8   CommitDataLength,
  IN  UINT8   *CommitData OPTIONAL
  );

/**
  This function close a session associated with a blob transfer over the IPMI.

  @param[in]         SessionId       The session ID returned from a call to BlobOpen

  @retval EFI_SUCCESS                The blob was closed.
  @retval Other                      An error occurred
**/
EFI_STATUS
IpmiBlobTransferClose (
  IN  UINT16  SessionId
  );

/**
  This function deletes a specific blob identified by its ID over the IPMI.

  @param[in]         BlobId          The BlobId to be deleted

  @retval EFI_SUCCESS                The blob was deleted.
  @retval Other                      An error occurred
**/
EFI_STATUS
IpmiBlobTransferDelete (
  IN  CHAR8  *BlobId
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
EFI_STATUS
IpmiBlobTransferStat (
  IN  CHAR8   *BlobId,
  OUT UINT16  *BlobState,
  OUT UINT32  *Size OPTIONAL,
  OUT UINT8   *MetadataLength OPTIONAL,
  OUT UINT8   *Metadata OPTIONAL
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
EFI_STATUS
IpmiBlobTransferSessionStat (
  IN  UINT16  SessionId,
  OUT UINT16  *BlobState,
  OUT UINT32  *Size OPTIONAL,
  OUT UINT8   *MetadataLength OPTIONAL,
  OUT UINT8   *Metadata OPTIONAL
  );

/**
  This function writes metadata to a blob associated with a session in an IPMI.

  @param[in]         SessionId       The ID of the session to write metadata for
  @param[in]         Offset          The offset of the metadata to write to
  @param[in]         Data            The data to write to the metadata
  @param[in]         WriteLength     The length to write

  @retval EFI_SUCCESS                The blob metadata was successfully written.
  @retval Other                      An error occurred
**/
EFI_STATUS
IpmiBlobTransferWriteMeta (
  IN  UINT16  SessionId,
  IN  UINT32  Offset,
  IN  UINT8   *Data,
  IN  UINT32  WriteLength
  );

#endif
