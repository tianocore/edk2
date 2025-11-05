/** @file  SmmStore.h

  Copyright (c) 2022, 9elements GmbH<BR>
  Copyright (c) 2025, 3mdeb Sp. z o.o.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef COREBOOT_SMMSTORE_H_
#define COREBOOT_SMMSTORE_H_

#define SMMSTORE_RET_SUCCESS      0
#define SMMSTORE_RET_FAILURE      1
#define SMMSTORE_RET_UNSUPPORTED  2

/* Version 2 only */
#define SMMSTORE_CMD_INIT       4
#define SMMSTORE_CMD_RAW_READ   5
#define SMMSTORE_CMD_RAW_WRITE  6
#define SMMSTORE_CMD_RAW_CLEAR  7

/* Used by capsule updates as a standalone command or modifier to v2 commands */
#define SMMSTORE_CMD_USE_FULL_FLASH  0x80

/*
 * This allows the payload to store raw data in the flash regions.
 * This can be used by a FaultTolerantWrite implementation, that uses at least
 * two regions in an A/B update scheme.
 */

#pragma pack(1)

/*
 * Reads a chunk of raw data with size BufSize from the block specified by
 * block_id starting at BufOffset.
 * The read data is placed in buf.
 *
 * block_id must be less than num_blocks
 * BufOffset + BufSize must be less than block_size
 */
typedef struct {
  UINT32    BufSize;
  UINT32    BufOffset;
  UINT32    BlockId;
} SMM_STORE_PARAMS_WRITE;

/*
 * Writes a chunk of raw data with size BufSize to the block specified by
 * block_id starting at BufOffset.
 *
 * block_id must be less than num_blocks
 * BufOffset + BufSize must be less than block_size
 */
typedef struct {
  UINT32    BufSize;
  UINT32    BufOffset;
  UINT32    BlockId;
} SMM_STORE_PARAMS_READ;

/*
 * Erases the specified block.
 *
 * block_id must be less than num_blocks
 */
typedef struct {
  UINT32    BlockId;
} SMM_STORE_PARAMS_CLEAR;

typedef union {
  SMM_STORE_PARAMS_WRITE    Write;
  SMM_STORE_PARAMS_READ     Read;
  SMM_STORE_PARAMS_CLEAR    Clear;
} SMM_STORE_COM_BUF;

#pragma pack(0)

UINTN
EFIAPI
TriggerSmi (
  IN UINTN  Cmd,
  IN UINTN  Arg,
  IN UINTN  Retry
  );

#endif // COREBOOT_SMMSTORE_H_
