/** @file
  Memory management protocol definitions as specified in the FF-A Memory Management
  v1.3 APL1 specification.

  Copyright 2021 The Hafnium Authors.
  Copyright (c), Microsoft Corporation.

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Glossary:
    - FF-A  - Firmware Framework for Arm A-profile

  @par Reference(s):
    - FF-A Version 1.3 ALP1 [https://developer.arm.com/documentation/den0077/l]
    - FF-A Memory Management Protocol 1.3 ALP1 [https://developer.arm.com/documentation/den0140/f]

**/

#ifndef ARM_FFA_MEM_MGMT_H_
#define ARM_FFA_MEM_MGMT_H_

/**
  Data access attributes in a memory access permission descriptor. This corresponds
  to table 1.15 of the FF-A v1.3 APL1, "Memory access permissions descriptor",
  bits[1:0].
**/
#define FFA_DATA_ACCESS_NOT_SPECIFIED  0
#define FFA_DATA_ACCESS_RO             1
#define FFA_DATA_ACCESS_RW             2
#define FFA_DATA_ACCESS_RESERVED       3

/**
  Instruction access attributes in a memory access permission descriptor.

  This corresponds to table 1.15 of the FF-A Memory Management v1.3 APL1, "Memory
  access permissions descriptor", bits[3:2].
**/
#define FFA_INSTRUCTION_ACCESS_NOT_SPECIFIED  0
#define FFA_INSTRUCTION_ACCESS_NX             1
#define FFA_INSTRUCTION_ACCESS_X              2
#define FFA_INSTRUCTION_ACCESS_RESERVED       3

/**
  Memory access permission struct definition of memory access permissions descriptor.

  This corresponds to table 1.15 of the FF-A Memory Management v1.3 APL1, "Memory
  region attributes descriptor".
**/
typedef struct {
  UINT8    DataAccess        : 2;
  UINT8    InstructionAccess : 2;
  UINT8    Reservd           : 4;
} FFA_MEMORY_ACCESS_PERMISSIONS;

/**
  Memory shareability attributes in a memory region attributes descriptor.

  This corresponds to table 1.18 of the FF-A Memory Management v1.3 APL1, "Memory
  region attributes descriptor", bits[1:0].
**/
#define FFA_MEMORY_SHARE_NON_SHAREABLE  0
#define FFA_MEMORY_SHARE_RESERVED       1
#define FFA_MEMORY_OUTER_SHAREABLE      2
#define FFA_MEMORY_INNER_SHAREABLE      3

/**
  Memory cacheability or device memory attributes in a memory region attributes
  descriptor.

  This corresponds to table 1.18 of the FF-A Memory Management v1.3 APL1, "Memory
  region attributes descriptor", bits[3:2].
**/
#define FFA_MEMORY_CACHE_RESERVED       0x0
#define FFA_MEMORY_CACHE_NON_CACHEABLE  0x1
#define FFA_MEMORY_CACHE_RESERVED_1     0x2
#define FFA_MEMORY_CACHE_WRITE_BACK     0x3
#define FFA_MEMORY_DEV_NGNRNE           0x0
#define FFA_MEMORY_DEV_NGNRE            0x1
#define FFA_MEMORY_DEV_NGRE             0x2
#define FFA_MEMORY_DEV_GRE              0x3

/**
  Memory type in a memory region attributes descriptor.

  This corresponds to table 1.18 of the FF-A Memory Management v1.3 APL1, "Memory
  region attributes descriptor", bits[5:4].
**/
#define FFA_MEMORY_NOT_SPECIFIED_MEM  0
#define FFA_MEMORY_DEVICE_MEM         1
#define FFA_MEMORY_NORMAL_MEM         2

/**
  FF-A Memory Management v1.3 APL1 Table 1.18 "Memory region attributes descriptor"
  bits[6]. Per section 1.10.4.1, NS bit is reserved for FFA_MEM_DONATE/LEND/SHARE
  and FFA_MEM_RETRIEVE_REQUEST.
**/
#define FFA_MEMORY_SECURITY_SECURE      0
#define FFA_MEMORY_SECURITY_NON_SECURE  1

/**
  Reserved bits (SBZ) of a memory region attributes descriptor.

  This corresponds to table 1.18 of the FF-A Memory Management v1.3 APL1, "Memory
  region attributes descriptor", bits[15:7].
**/
#define FFA_MEMORY_ATTRIBUTES_MBZ_MASK  0xFF80U

/**
  Memory region attributes struct definition of memory region attribute descriptor.

  This corresponds to table 1.18 of the FF-A Memory Management v1.3 APL1, "Memory
  region attributes descriptor".
**/
typedef struct {
  UINT16    Shareability : 2;
  UINT16    Cacheability : 2;
  UINT16    Type         : 2;
  UINT16    Security     : 1;
  UINT16    Reserved     : 9;
} FFA_MEMORY_ATTRIBUTES;

/**
  A set of contiguous pages which is part of a memory region.

  This corresponds to table 1.14 of the the FF-A Memory Management v1.3 APL1, "Constituent
  memory region descriptor".
**/
typedef struct {
  /**
    The base IPA of the constituent memory region, aligned to 4 kiB page
    size granularity.
  **/
  UINT64    Address;
  /** The number of 4 kiB pages in the constituent memory region. **/
  UINT32    PageCount;
  /** Reserved field, SBZ. **/
  UINT32    Reserved;
} FFA_MEMORY_REGION_CONSTITUENT;

/**
  A set of pages comprising a memory region.

  This corresponds to table 1.13 of the FF-A Memory Management v1.3 APL1, "Composite
  memory region descriptor".
**/
typedef struct {
  /**
   The total number of 4 kiB pages included in this memory region. This
   must be equal to the sum of page counts specified in each
   `FFA_MEMORY_REGION_CONSTITUENT`.
  **/
  UINT32                           TotalPageCount;

  /**
   The number of constituents (`FFA_MEMORY_REGION_CONSTITUENT`)
   included in this memory region range.
  **/
  UINT32                           ConstituentCount;
  /** Reserved field, SBZ. */
  UINT64                           Reserved;
  /** An array of `ConstituentCount` memory region constituents. */
  FFA_MEMORY_REGION_CONSTITUENT    Constituents[];
} FFA_COMPOSITE_MEMORY_REGION;

/**
  Flags to indicate properties of memory management ABI being invoked.

  This corresponds to table 1.17 of the FF-A Memory Management v1.3 APL1, "Flags
  Flags usage in FFA_MEM_RETRIEVE_REQ and FFA_MEM_RETRIEVE_RESP ABIs", Bit[0].
**/
#define FFA_MEMORY_ACCESS_PERMISSION_FLAG_MASK           ((0x1U) << 0)
#define FFA_MEMORY_ACCESS_PERMISSION_FLAG_UNSPECIFIED    ((0x0U) << 0)
#define FFA_MEMORY_ACCESS_PERMISSION_FLAG_RETRIEVAL      ((0x0U) << 0)
#define FFA_MEMORY_ACCESS_PERMISSION_FLAG_NON_RETRIEVAL  ((0x1U) << 0)

/**
  Memory region attributes descriptor.

  This corresponds to table 1.15 of the FF-A Memory Management v1.3 APL1, "Memory
  access permissions descriptor".
**/
typedef struct {
  /** The ID of the VM to which the memory is being given or shared. **/
  UINT16                           ReceiverId;

  /**
   The permissions with which the memory region should be mapped in the
   receiver's page table.
  **/
  FFA_MEMORY_ACCESS_PERMISSIONS    Permissions;

  /**
   Flags used during FFA_MEM_RETRIEVE_REQ and FFA_MEM_RETRIEVE_RESP
   for memory regions with multiple borrowers.
  **/
  UINT8                            Flags;
} FFA_MEMORY_ACCESS_PERMISSIONS_DESCRIPTOR;

/**
  Endpoint memory access descriptor.

  This corresponds to table 1.16 of the FF-A Memory Management v1.3 APL1, "Endpoint
  memory access descriptor".
**/
typedef struct {
  FFA_MEMORY_ACCESS_PERMISSIONS_DESCRIPTOR    ReceiverPermissions;

  /**
    Offset in bytes from the start of the outer `FFA_MEMORY_TRANSACTION_DESCRIPTOR` to
    an `FFA_COMPOSITE_MEMORY_REGION` struct.
  **/
  UINT32                                      CompositeMemoryRegionOffset;
  UINT8                                       ImplementationDefined[16];
  UINT64                                      Reserved;
} FFA_ENDPOINT_MEMORY_ACCESS_DESCRIPTOR;

/**
  Clear memory region contents after unmapping it from the sender and before
  mapping it for any receiver.

  This corresponds to table 1.22 of the FF-A Memory Management v1.3 APL1, "Flags
  usage in FFA_MEM_RETRIEVE_REQ ABI", bits[0].
**/
#define FFA_MEMORY_REGION_FLAG_CLEAR  0x1

/**
  Whether the hypervisor may time slice the memory sharing or retrieval
  operation.

  This corresponds to table 1.22 of the FF-A Memory Management v1.3 APL1, "Flags
  usage in FFA_MEM_RETRIEVE_REQ ABI", bits[1].
**/
#define FFA_MEMORY_REGION_FLAG_TIME_SLICE  0x2

/**
  Whether the hypervisor should clear the memory region after the receiver
  relinquishes it or is aborted.

  This corresponds to table 1.22 of the FF-A Memory Management v1.3 APL1, "Flags
  usage in FFA_MEM_RETRIEVE_REQ ABI", bits[2].
**/
#define FFA_MEMORY_REGION_FLAG_CLEAR_RELINQUISH  0x4

/**
  Transaction type for memory region retrieval.

  This corresponds to table 1.22 of the FF-A Memory Management v1.3 APL1, "Flags
  usage in FFA_MEM_RETRIEVE_REQ ABI", bits[3].
**/
#define FFA_MEMORY_REGION_TRANSACTION_TYPE_MASK         ((0x3U) << 3)
#define FFA_MEMORY_REGION_TRANSACTION_TYPE_UNSPECIFIED  ((0x0U) << 3)
#define FFA_MEMORY_REGION_TRANSACTION_TYPE_SHARE        ((0x1U) << 3)
#define FFA_MEMORY_REGION_TRANSACTION_TYPE_LEND         ((0x2U) << 3)
#define FFA_MEMORY_REGION_TRANSACTION_TYPE_DONATE       ((0x3U) << 3)

/**
  Address range hint for memory region retrieval.

  This corresponds to table 1.22 of the FF-A Memory Management v1.3 APL1, "Flags
  usage in FFA_MEM_RETRIEVE_REQ ABI", bits[9:5].
**/
#define FFA_MEMORY_REGION_ADDRESS_RANGE_HINT_VALID  ((0x1U) << 9)
#define FFA_MEMORY_REGION_ADDRESS_RANGE_HINT_MASK   ((0xFU) << 5)

/**
  On retrieve request, bypass the multi-borrower check.

  This corresponds to table 1.22 of the FF-A Memory Management v1.3 APL1, "Flags
  usage in FFA_MEM_RETRIEVE_REQ ABI", bits[10].
**/
#define FFA_MEMORY_REGION_FLAG_BYPASS_BORROWERS_CHECK  (0x1U << 10)

/**
  Information about a set of pages which are being shared.

  This corresponds to table 1.20 of the FF-A Memory Management v1.3 APL1, "Memory
  transaction descriptor". Note that it is also used for retrieve requests and responses.
**/
typedef struct {
  /**
    The ID of the VM which originally sent the memory region, i.e. the
    owner.
  **/
  UINT16                   SenderId;
  FFA_MEMORY_ATTRIBUTES    Attributes;
  /** Flags to control behaviour of the transaction. **/
  UINT32                   Flags;
  UINT64                   Handle;

  /**
    An implementation defined value associated with the receiver and the
    memory region.
  **/
  UINT64                   Tag;
  /** Size of the memory access descriptor. **/
  UINT32                   MemoryAccessDescSize;

  /**
    The number of `FFA_ENDPOINT_MEMORY_ACCESS_DESCRIPTOR` entries included in this
    transaction.
  **/
  UINT32                   ReceiverCount;

  /**
    Offset to the 'FFA_ENDPOINT_MEMORY_ACCESS_DESCRIPTOR' field, which relates to
    the memory access descriptors.
  **/
  UINT32                   ReceiversOffset;
  /** Reserved field (12 bytes) SBZ. */
  UINT8                    Reserved[12];
} FFA_MEMORY_TRANSACTION_DESCRIPTOR;

/**
  Descriptor used for FFA_MEM_RELINQUISH requests. This corresponds to table
  2.25 of the FF-A Memory Management v1.3 APL1, "Descriptor to relinquish a memory
  region".
**/
typedef struct {
  UINT64    Handle;
  UINT32    Flags;
  UINT32    EndpointCount;
  UINT16    Endpoints[];
} FFA_MEM_RELINQUISH_DESCRIPTOR;

#endif
