/** @file
  Partition information description as specfied in the FF-A v1.2 specification.

  Copyright (c) 2024, Arm Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - FF-A  - Firmware Framework for Arm A-profile
    - ST    - Start idx and Tag
    - IM    - Information Metadata
    - PART  - Partition
    - PROP  - Property
    - PE    - Processing Element
    - INDEP - Independent
    - AUX   - Auxiliary


  @par Reference(s):
    - FF-A Version 1.2 [https://developer.arm.com/documentation/den0077/latest]

**/

#ifndef ARM_FFA_PART_INFO_H_
#define ARM_FFA_PART_INFO_H_

/** Partition info flags used in FFA_PARTITION_INFO_GET
 *  See FF-A spec chapter 13.8 FFA_PARTITION_INFO_GET
 */
#define FFA_PART_INFO_FLAG_TYPE_SHIFT  0
#define FFA_PART_INFO_FLAG_TYPE_MASK   1
#define FFA_PART_INFO_FLAG_TYPE_COUNT  1
#define FFA_PART_INFO_FLAG_TYPE_DESC   0

#define FFA_PART_INFO_IDX_MASK        0xffff
#define FFA_PART_INFO_TAG_MASK        0xffff
#define FFA_PART_INFO_DESC_SIZE_MASK  0xffff

/** macro used in FFA_PARTITION_INFO_GET_REGS request
 *  See FF-A spec chapter 13.9 FFA_PARTITION_INFO_GET_REGS
 */
#define FFA_PART_INFO_START_IDX_SHIFT  0
#define FFA_PART_INFO_START_TAG_SHIFT  16

/** macro used in FFA_PARTITION_INFO_GET_REGS response
 *  See FF-A spec chapter 13.9 FFA_PARTITION_INFO_GET_REGS
 */
#define FFA_PART_INFO_METADATA_LAST_IDX_SHIFT     0
#define FFA_PART_INFO_METADATA_CURRENT_IDX_SHIFT  16
#define FFA_PART_INFO_METADATA_TAG_SHIFT          32
#define FFA_PART_INFO_METADATA_DESC_SIZE_SHIFT    48

/** Partition properties values in EFI_FFA_PART_INFO_DESC->PartitionProps
 *  See FF-A spec chapter 6.2 partition discovery.
 */
#define FFA_PART_PROP_RECV_DIRECT_REQ    (1 << 0)
#define FFA_PART_PROP_SEND_DIRECT_REQ    (1 << 1)
#define FFA_PART_PROP_SEND_INDIRECT_REQ  (1 << 2)
#define FFA_PART_PROP_RECV_INDIRECT_REQ  (1 << 3)

#define FFA_PART_PROP_ID_TYPE_SHIFT             4
#define FFA_PART_PROP_ID_TYPE_MASK              0x3
#define FFA_PART_PROP_ID_PE_EP                  0x00
#define FFA_PART_PROP_ID_STREAM_EP_INDEPENDENT  0x01
#define FFA_PART_PROP_ID_STREAM_EP_DEPENDENT    0x02
#define FFA_PART_PROP_ID_AUX                    0x03

#define FFA_PART_PROP_VM_STATUS_CREATED    (1 << 6)
#define FFA_PART_PROP_VM_STATUS_DESTROYED  (1 << 7)

#define FFA_PART_PROP_EXECUTE_STATE  (1 << 8)

#define FFA_PART_PROP_RECV_DIRECT_REQ2  (1 << 9)
#define FFA_PART_PROP_SEND_DIRECT_REQ2  (1 << 10)

typedef union {
  /// Number of Execution context
  UINT16    ExecContextCount;

  ///  ID of proxy endpoint for a dependent peripheral device
  UINT16    ProxyPartitionId;
} EXEC_CONTEXT;

/** Partition information Descriptor in the FF-A v1.2 spec.
 *  See FF-A spec chapter 6.2 partition discovery.
 */
typedef struct {
  /// Partition id
  UINT16          PartitionId;

  /// Execution context count or Proxy partition id
  EXEC_CONTEXT    ExecContextCountOrProxyPartitionId;

  /// Flags to determine partition properties
  UINT32          PartitionProps;

  /// UUID of partition
  UINT32          PartitionUuid[4];
} EFI_FFA_PART_INFO_DESC;

#endif
