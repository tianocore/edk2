/** @file

  Generic type and macro definitions corresponding to the virtio-0.9.5
  specification.

  Copyright (C) 2012, Red Hat, Inc.
  Portion of Copyright (C) 2013, ARM Ltd.

  This program and the accompanying materials are licensed and made available
  under the terms and conditions of the BSD License which accompanies this
  distribution. The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS, WITHOUT
  WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _VIRTIO_H_
#define _VIRTIO_H_

#include <Base.h>

//
// VirtIo Subsystem Device IDs
//
#define VIRTIO_SUBSYSTEM_NETWORK_CARD       1
#define VIRTIO_SUBSYSTEM_BLOCK_DEVICE       2
#define VIRTIO_SUBSYSTEM_CONSOLE            3
#define VIRTIO_SUBSYSTEM_ENTROPY_SOURCE     4
#define VIRTIO_SUBSYSTEM_MEMORY_BALLOONING  5
#define VIRTIO_SUBSYSTEM_IO_MEMORY          6
#define VIRTIO_SUBSYSTEM_RPMSG              7
#define VIRTIO_SUBSYSTEM_SCSI_HOST          8
#define VIRTIO_SUBSYSTEM_9P_TRANSPORT       9
#define VIRTIO_SUBSYSTEM_MAC80211_WLAN      10

//
// Virtio IDs
//
#define VIRTIO_VENDOR_ID                    0x1AF4
#define VIRTIO_MMIO_MAGIC                   0x74726976 // "virt"


//
// VirtIo Device Specific Configuration Offsets
//
#define VIRTIO_DEVICE_SPECIFIC_CONFIGURATION_OFFSET_PCI             20
#define VIRTIO_DEVICE_SPECIFIC_CONFIGURATION_OFFSET_PCI_WITH_MSI_X  24
#define VIRTIO_DEVICE_SPECIFIC_CONFIGURATION_OFFSET_MMIO            0x100

//
// PCI VirtIo Header Offsets
//
#define VIRTIO_PCI_OFFSET_DEVICE_FEATURES           0x00
#define VIRTIO_PCI_OFFSET_GUEST_FEATURES            0x04
#define VIRTIO_PCI_OFFSET_QUEUE_ADDRESS             0x08
#define VIRTIO_PCI_OFFSET_QUEUE_SIZE                0x0C
#define VIRTIO_PCI_OFFSET_QUEUE_SELECT              0x0E
#define VIRTIO_PCI_OFFSET_QUEUE_NOTIFY              0x10
#define VIRTIO_PCI_OFFSET_QUEUE_DEVICE_STATUS       0x12
#define VIRTIO_PCI_OFFSET_QUEUE_DEVICE_ISR          0x13

//
// MMIO VirtIo Header Offsets
//
#define VIRTIO_MMIO_OFFSET_MAGIC                    0x00
#define VIRTIO_MMIO_OFFSET_VERSION                  0x04
#define VIRTIO_MMIO_OFFSET_DEVICE_ID                0x08
#define VIRTIO_MMIO_OFFSET_VENDOR_ID                0x0C
#define VIRTIO_MMIO_OFFSET_HOST_FEATURES            0x10
#define VIRTIO_MMIO_OFFSET_HOST_FEATURES_SEL        0x14
#define VIRTIO_MMIO_OFFSET_GUEST_FEATURES           0x20
#define VIRTIO_MMIO_OFFSET_GUEST_FEATURES_SEL       0x24
#define VIRTIO_MMIO_OFFSET_GUEST_PAGE_SIZE          0x28
#define VIRTIO_MMIO_OFFSET_QUEUE_SEL                0x30
#define VIRTIO_MMIO_OFFSET_QUEUE_NUM_MAX            0x34
#define VIRTIO_MMIO_OFFSET_QUEUE_NUM                0x38
#define VIRTIO_MMIO_OFFSET_QUEUE_ALIGN              0x3C
#define VIRTIO_MMIO_OFFSET_QUEUE_PFN                0x40
#define VIRTIO_MMIO_OFFSET_QUEUE_NOTIFY             0x50
#define VIRTIO_MMIO_OFFSET_INTERRUPT_STATUS         0x60
#define VIRTIO_MMIO_OFFSET_INTERRUPT_ACK            0x64
#define VIRTIO_MMIO_OFFSET_STATUS                   0x70

//
// Data in the communication area is defined as packed and accessed as
// volatile.
//
// Some structures contain arrays with dynamically determined size. In such
// cases the array and its sibling fields are replaced with pointers.
//
// All indices (variables and fields named *Idx) are free-running and wrap
// around after 0xFFFF. The queue size reported by the host is always an
// integral power of 2, not greater than 32768. Actual array indices are
// consistently calculated by taking the remainder of a given Idx object modulo
// QueueSize. Since 0x10000 is an integral multiple of the QueueSize, UINT16
// wraparound is a correct wraparound modulo QueueSize too (it doesn't offset
// the remainder class).
//
// virtio-0.9.5, 2.3.4 Available Ring
//
#define VRING_AVAIL_F_NO_INTERRUPT BIT0

typedef struct {
  volatile UINT16 *Flags;
  volatile UINT16 *Idx;

  volatile UINT16 *Ring;      // QueueSize elements
  volatile UINT16 *UsedEvent; // unused as per negotiation
} VRING_AVAIL;


//
// virtio-0.9.5, 2.3.5 Used Ring
//
#define VRING_USED_F_NO_NOTIFY BIT0

#pragma pack(1)
typedef struct {
  UINT32 Id;
  UINT32 Len;
} VRING_USED_ELEM;
#pragma pack()

typedef struct {
  volatile UINT16          *Flags;
  volatile UINT16          *Idx;
  volatile VRING_USED_ELEM *UsedElem;   // QueueSize elements
  volatile UINT16          *AvailEvent; // unused as per negotiation
} VRING_USED;


//
// virtio-0.9.5, 2.3.2 Descriptor Table
//
#define VRING_DESC_F_NEXT     BIT0 // more descriptors in this request
#define VRING_DESC_F_WRITE    BIT1 // buffer to be written *by the host*
#define VRING_DESC_F_INDIRECT BIT2 // unused

#pragma pack(1)
typedef struct {
  UINT64 Addr;
  UINT32 Len;
  UINT16 Flags;
  UINT16 Next;
} VRING_DESC;
#pragma pack()

typedef struct {
  UINTN               NumPages;
  VOID                *Base;     // deallocate only this field
  volatile VRING_DESC *Desc;     // QueueSize elements
  VRING_AVAIL         Avail;
  VRING_USED          Used;
  UINT16              QueueSize;
} VRING;

//
// virtio-0.9.5, 2.2.2.1 Device Status
//
#define VSTAT_ACK       BIT0
#define VSTAT_DRIVER    BIT1
#define VSTAT_DRIVER_OK BIT2
#define VSTAT_FAILED    BIT7

//
// virtio-0.9.5, Appendix B: Reserved (Device-Independent) Feature Bits
//
#define VIRTIO_F_NOTIFY_ON_EMPTY    BIT24
#define VIRTIO_F_RING_INDIRECT_DESC BIT28
#define VIRTIO_F_RING_EVENT_IDX     BIT29


#endif // _VIRTIO_H_
