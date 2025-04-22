/** @file
  Header file for FF-A ABI's that will be used for
  communication between S-EL0 and the Secure Partition
  Manager(SPM)

  Copyright (c) 2020-2024, Arm Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - FF-A - Firmware Framework for Arm A-profile

  @par Reference(s):
    - FF-A Version 1.2 [https://developer.arm.com/documentation/den0077/latest/]
    - FF-A Memory Management Protocol Version 1.2 [https://developer.arm.com/documentation/den0140/latest]

**/

#ifndef ARM_FFA_SVC_H_
#define ARM_FFA_SVC_H_

#define ARM_FID_FFA_ERROR            0x84000060
#define ARM_FID_FFA_SUCCESS_AARCH32  0x84000061
#define ARM_FID_FFA_SUCCESS_AARCH64  0xC4000061
#define ARM_FID_FFA_INTERRUPT        0x84000062

#define ARM_FID_FFA_VERSION                  0x84000063
#define ARM_FID_FFA_FEATURES                 0x84000064
#define ARM_FID_FFA_RX_ACQUIRE               0x84000084
#define ARM_FID_FFA_RX_RELEASE               0x84000065
#define ARM_FID_FFA_RXTX_MAP_AARCH32         0x84000066
#define ARM_FID_FFA_RXTX_MAP_AARCH64         0xC4000066
#define ARM_FID_FFA_RXTX_UNMAP               0x84000067
#define ARM_FID_FFA_PARTITION_INFO_GET       0x84000068
#define ARM_FID_FFA_PARTITION_INFO_GET_REGS  0xC400008B
#define ARM_FID_FFA_ID_GET                   0x84000069
#define ARM_FID_FFA_SPM_ID_GET               0x84000085
#define ARM_FID_FFA_CONSOLE_LOG_AARCH32      0x8400008A
#define ARM_FID_FFA_CONSOLE_LOG_AARCH64      0xC400008A

#define ARM_FID_FFA_WAIT                 0x8400006B
#define ARM_FID_FFA_YIELD                0x8400006C
#define ARM_FID_FFA_RUN                  0x8400006D
#define ARM_FID_FFA_NORMAL_WORLD_RESUME  0x8400007C

#define ARM_FID_FFA_MSG_SEND2                     0x84000086
#define ARM_FID_FFA_MSG_SEND_DIRECT_REQ_AARCH32   0x8400006F
#define ARM_FID_FFA_MSG_SEND_DIRECT_REQ_AARCH64   0xC400006F
#define ARM_FID_FFA_MSG_SEND_DIRECT_RESP_AARCH32  0x84000070
#define ARM_FID_FFA_MSG_SEND_DIRECT_RESP_AARCH64  0xC4000070
#define ARM_FID_FFA_MSG_SEND_DIRECT_REQ2          0xC400008D
#define ARM_FID_FFA_MSG_SEND_DIRECT_RESP2         0xC400008E

#define ARM_FID_FFA_NOTIFICATION_BITMAP_CREATE     0x8400007D
#define ARM_FID_FFA_NOTIFICATION_BITMAP_DESTROY    0x8400007E
#define ARM_FID_FFA_NOTIFICATION_BIND              0x8400007F
#define ARM_FID_FFA_NOTIFICATION_UNBIND            0x84000080
#define ARM_FID_FFA_NOTIFICATION_SET               0x84000081
#define ARM_FID_FFA_NOTIFICATION_GET               0x84000082
#define ARM_FID_FFA_NOTIFICATION_INFO_GET_AARCH32  0x84000083
#define ARM_FID_FFA_NOTIFICATION_INFO_GET_AARCH64  0xC4000083

#define ARM_FID_FFA_EL3_INTR_HANDLE  0x8400008C

#define ARM_FID_FFA_MEM_DONATE_AARCH32        0x84000071
#define ARM_FID_FFA_MEM_DONATE_AARCH64        0xC4000071
#define ARM_FID_FFA_MEM_LEND_AARCH32          0x84000072
#define ARM_FID_FFA_MEM_LEND_AARCH64          0xC4000072
#define ARM_FID_FFA_MEM_SHARE_AARCH32         0x84000073
#define ARM_FID_FFA_MEM_SHARE_AARCH64         0xC4000073
#define ARM_FID_FFA_MEM_RETRIEVE_REQ_AARCH32  0x84000074
#define ARM_FID_FFA_MEM_RETRIEVE_REQ_AARCH64  0xC4000074
#define ARM_FID_FFA_MEM_RETRIEVE_RESP         0x84000075
#define ARM_FID_FFA_MEM_RETRIEVE_RELINQUISH   0x84000076
#define ARM_FID_FFA_MEM_RETRIEVE_RECLAIM      0x84000077
#define ARM_FID_FFA_MEM_PERM_GET_AARCH32      0x84000088
#define ARM_FID_FFA_MEM_PERM_GET_AARCH64      0xC4000088
#define ARM_FID_FFA_MEM_PERM_SET_AARCH32      0x84000089
#define ARM_FID_FFA_MEM_PERM_SET_AARCH64      0xC4000089

/* Generic IDs when using AArch32 or AArch64 execution state */
#if defined (MDE_CPU_AARCH64)

#define ARM_FID_FFA_RXTX_MAP     ARM_FID_FFA_RXTX_MAP_AARCH64
#define ARM_FID_FFA_CONSOLE_LOG  ARM_FID_FFA_CONSOLE_LOG_AARCH64

#define ARM_FID_FFA_MSG_SEND_DIRECT_REQ   ARM_FID_FFA_MSG_SEND_DIRECT_REQ_AARCH64
#define ARM_FID_FFA_MSG_SEND_DIRECT_RESP  ARM_FID_FFA_MSG_SEND_DIRECT_RESP_AARCH64

#define ARM_FID_FFA_NOTIFICATION_INFO_GET  ARM_FID_FFA_NOTIFICATION_INFO_GET_AARCH64

#define ARM_FID_FFA_MEM_DONATE        ARM_FID_FFA_MEM_DONATE_AARCH64
#define ARM_FID_FFA_MEM_LEND          ARM_FID_FFA_MEM_LEND_AARCH64
#define ARM_FID_FFA_MEM_SHARE         ARM_FID_FFA_MEM_SHARE_AARCH64
#define ARM_FID_FFA_MEM_RETRIEVE_REQ  ARM_FID_FFA_MEM_RETRIEVE_REQ_AARCH64
#define ARM_FID_FFA_MEM_PERM_GET      ARM_FID_FFA_MEM_PERM_GET_AARCH64
#define ARM_FID_FFA_MEM_PERM_SET      ARM_FID_FFA_MEM_PERM_SET_AARCH64

#elif defined (MDE_CPU_ARM)

#define ARM_FID_FFA_RXTX_MAP     ARM_FID_FFA_RXTX_MAP_AARCH32
#define ARM_FID_FFA_CONSOLE_LOG  ARM_FID_FFA_CONSOLE_LOG_AARCH32

#define ARM_FID_FFA_MSG_SEND_DIRECT_REQ   ARM_FID_FFA_MSG_SEND_DIRECT_REQ_AARCH32
#define ARM_FID_FFA_MSG_SEND_DIRECT_RESP  ARM_FID_FFA_MSG_SEND_DIRECT_RESP_AARCH32

#define ARM_FID_FFA_NOTIFICATION_INFO_GET  ARM_FID_FFA_NOTIFICATION_INFO_GET_AARCH32

#define ARM_FID_FFA_MEM_DONATE        ARM_FID_FFA_MEM_DONATE_AARCH32
#define ARM_FID_FFA_MEM_LEND          ARM_FID_FFA_MEM_LEND_AARCH32
#define ARM_FID_FFA_MEM_SHARE         ARM_FID_FFA_MEM_SHARE_AARCH32
#define ARM_FID_FFA_MEM_RETRIEVE_REQ  ARM_FID_FFA_MEM_RETRIEVE_REQ_AARCH32
#define ARM_FID_FFA_MEM_PERM_GET      ARM_FID_FFA_MEM_PERM_GET_AARCH32
#define ARM_FID_FFA_MEM_PERM_SET      ARM_FID_FFA_MEM_PERM_SET_AARCH32

#else
  #error "Invalid architecture."
#endif

#define ARM_FFA_MAJOR_VERSION        1
#define ARM_FFA_MINOR_VERSION        2
#define ARM_FFA_MAJOR_VERSION_MASK   0x7FFF
#define ARM_FFA_MINOR_VERSION_MASK   0xFFFF
#define ARM_FFA_MAJOR_VERSION_SHIFT  16
#define ARM_FFA_MINOR_VERSION_SHIFT  0

#define ARM_FFA_MAJOR_VERSION_GET(version) \
  (((version) >> ARM_FFA_MAJOR_VERSION_SHIFT) & ARM_FFA_MAJOR_VERSION_MASK)

#define ARM_FFA_MINOR_VERSION_GET(version) \
  (((version) >> ARM_FFA_MINOR_VERSION_SHIFT) & ARM_FFA_MINOR_VERSION_MASK)

#define ARM_FFA_CREATE_VERSION(major, minor)  \
  (((major) << ARM_FFA_MAJOR_VERSION_SHIFT) | \
   ((minor) << ARM_FFA_MINOR_VERSION_SHIFT))

#define ARM_FFA_FEATURES_ID_TYPE_SHIFT     31
#define ARM_FFA_FEATURES_ID_TYPE_MASK      1
#define ARM_FFA_FEATURES_ID_TYPE_FEATURE   0
#define ARM_FFA_FEATURES_ID_TYPE_FUNCTION  1

/*
 * macro used in FFA_FEATURE ABI.
 * See FF-A spec Chapther 13.3 FFA_FEATURE
 */
#define ARM_FFA_FEATURE_ID_SHIFT                           0
#define ARM_FFA_FEATURE_ID_MASK                            0xff
#define ARM_FFA_FEATURE_ID_NOTIFICATION_PENDING_INTERRUPT  0x01
#define ARM_FFA_FEATURE_ID_SCHEDULE_RECEIVER_INTERRUPT     0x02
#define ARM_FFA_FEATURE_ID_MANAGED_EXIT_INTERRUPT          0x03

#define ARM_FFA_BUFFER_MINSIZE_AND_ALIGN_MASK   0x03
#define ARM_FFA_BUFFER_MINSIZE_AND_ALIGN_SHIFT  0
#define ARM_FFA_BUFFER_MINSIZE_AND_ALIGN_4K     0x00
#define ARM_FFA_BUFFER_MINSIZE_AND_ALIGN_16K    0x02
#define ARM_FFA_BUFFER_MINSIZE_AND_ALIGN_64K    0x01

#define ARM_FFA_BUFFER_MAXSIZE_PAGE_COUNT_MASK   0xffff
#define ARM_FFA_BUFFER_MAXSIZE_PAGE_COUNT_SHIFT  16

/*
 * return value of FFA ABI.
 */
#define ARM_FFA_RET_SUCCESS             0
#define ARM_FFA_RET_NOT_SUPPORTED       -1
#define ARM_FFA_RET_INVALID_PARAMETERS  -2
#define ARM_FFA_RET_NO_MEMORY           -3
#define ARM_FFA_RET_BUSY                -4
#define ARM_FFA_RET_INTERRUPTED         -5
#define ARM_FFA_RET_DENIED              -6
#define ARM_FFA_RET_RETRY               -7
#define ARM_FFA_RET_ABORTED             -8
#define ARM_FFA_RET_NODATA              -9
#define ARM_FFA_RET_NOT_READY           -10

// For now, the destination id to be used in the FF-A calls
// is being hard-coded. Subsequently, support will be added
// to get the endpoint id's dynamically
// This is the endpoint id used by the optee os's implementation
// of the spmc.
// https://github.com/OP-TEE/optee_os/blob/master/core/arch/arm/kernel/stmm_sp.c#L66
#define ARM_FFA_DESTINATION_ENDPOINT_ID  3

/*
 * memory permission value used in ARM_FID_FFA_MEM_PERM_SET.
 * See FF-A Memory Management Protocol Spec Chapter 2.9 FFA_MEM_PERM_SET
 */
#define ARM_FFA_SET_MEM_ATTR_DATA_PERM_MASK       0x03
#define ARM_FFA_SET_MEM_ATTR_DATA_PERM_SHIFT      0
#define ARM_FFA_SET_MEM_ATTR_DATA_PERM_NO_ACCESS  0
#define ARM_FFA_SET_MEM_ATTR_DATA_PERM_RW         0x01U
#define ARM_FFA_SET_MEM_ATTR_DATA_PERM_RO         0x03U

#define ARM_FFA_SET_MEM_ATTR_CODE_PERM_MASK   0x1
#define ARM_FFA_SET_MEM_ATTR_CODE_PERM_SHIFT  2
#define ARM_FFA_SET_MEM_ATTR_CODE_PERM_X      0
#define ARM_FFA_SET_MEM_ATTR_CODE_PERM_XN     1

#define ARM_FFA_MEM_PERM_RESERVED_MASK  0xFFFFFFF8

#define ARM_FFA_SET_MEM_ATTR_MAKE_PERM_REQUEST(dataperm, codeperm)  \
    ((((codeperm) & ARM_FFA_SET_MEM_ATTR_CODE_PERM_MASK) <<         \
      ARM_FFA_SET_MEM_ATTR_CODE_PERM_SHIFT) |                       \
    (( (dataperm) & ARM_FFA_SET_MEM_ATTR_DATA_PERM_MASK) <<         \
     ARM_FFA_SET_MEM_ATTR_DATA_PERM_SHIFT))

/*
 * macro used in FFA_MSG_DIRECT_REQ/REQ2
 * See FF-A spec Chapther 15.2 FFA_MSG_SEND_DIRECT_REQ and
 * 15.4 FFA_MSG_SEND_DIRECT_REQ2
 */
#define ARM_FFA_SOURCE_EP_SHIFT    16
#define ARM_FFA_DEST_EP_SHIFT      0
#define ARM_FFA_PARTITION_ID_MASK  0xffff

#define GET_SOURCE_PARTITION_ID(PackedId) \
  ((PackedId >> ARM_FFA_SOURCE_EP_SHIFT) & ARM_FFA_PARTITION_ID_MASK)

#define GET_DEST_PARTITION_ID(PackedId) \
  ((PackedId >> ARM_FFA_DEST_EP_SHIFT) & ARM_FFA_PARTITION_ID_MASK)

#define PACK_PARTITION_ID_INFO(SourceId, DestId)                          \
  (((SourceId & ARM_FFA_PARTITION_ID_MASK) << ARM_FFA_SOURCE_EP_SHIFT) |  \
   ((DestId & ARM_FFA_PARTITION_ID_MASK) << ARM_FFA_DEST_EP_SHIFT))

#define IS_FID_FFA_ERROR(fid) \
  (fid == ARM_FID_FFA_ERROR)

/*
 * macro used in FFA_NOTIFICATION_GET/SET
 * See FF-A spec sections for FFA_NOTIFICATION_GET and
 * FFA_NOTIFICATION_SET
 */
#define ARM_FFA_NOTIFICATION_FLAG_PER_VCPU  (0x1 << 0)

/** Flag to delay Schedule Receiver Interrupt. */
#define ARM_FFA_NOTIFICATION_FLAG_DELAY_SRI  (0x1 << 1)

#define ARM_FFA_NOTIFICATION_FLAGS_VCPU_ID(id)  ((id & 0xFFFF) << 16)

#define ARM_FFA_NOTIFICATION_FLAG_BITMAP_SP   (0x1 << 0)
#define ARM_FFA_NOTIFICATION_FLAG_BITMAP_VM   (0x1 << 1)
#define ARM_FFA_NOTIFICATION_FLAG_BITMAP_SPM  (0x1 << 2)
#define ARM_FFA_NOTIFICATION_FLAG_BITMAP_HYP  (0x1 << 3)

/*
 * macro used in FFA_FEATURES function.
 * See FF-A spec sections for FFA_FEATURES, Table of
 * Feature IDs and properties table.
 */

/** Query interrupt ID of Notification Pending Interrupt. */
#define ARM_FFA_FEATURE_NPI  0x1U

/** Query interrupt ID of Schedule Receiver Interrupt. */
#define ARM_FFA_FEATURE_SRI  0x2U

/** Query interrupt ID of the Managed Exit Interrupt. */
#define ARM_FFA_FEATURE_MEI  0x3U

/** Query notifications features. */
#define ARM_FFA_FEATURE_NOTIFICATIONS  0x4U

#endif // ARM_FFA_SVC_H_
