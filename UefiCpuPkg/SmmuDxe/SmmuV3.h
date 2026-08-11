/** @file SmmuV3.h

    This file is the SmmuV3 header file for SMMU driver compliant with the Smmu spec:

    <https://developer.arm.com/documentation/ihi0070/latest/>

    Copyright (c) Microsoft Corporation.
    SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#pragma once

#include <Register/SmmuV3Registers.h>
#include <Uefi/UefiBaseType.h>
#include <IndustryStandard/IoRemappingTable.h>
#include <Protocol/IoMmu.h>
#include <Protocol/HardwareInterrupt2.h>
#include <Guid/SmmuConfig.h>
#include "IoMmu.h"

// Number of levels in the page table
#define PAGE_TABLE_DEPTH  4
// Maximum address width covered by a single-page L1 root.
#define PAGE_TABLE_L1_ADDRESS_WIDTH_MAX  39
// Calculate the page-table entry index for a single-page table at the given level.
#define PAGE_TABLE_INDEX(VirtualAddress, Level) \
        (((VirtualAddress) >> (12 + (9 * ((PAGE_TABLE_DEPTH - 1) - (Level))))) & 0x1FF)

#define PAGE_TABLE_OUTPUT_ADDRESS_WIDTH_MAX  48
#define PAGE_TABLE_OUTPUT_ADDRESS_WIDTH_MIN  32
#define PAGE_TABLE_ROOT_PAGES                1

// Cacheability and Shareability attributes
#define ARM64_RGNCACHEATTR_NONCACHEABLE               0
#define ARM64_RGNCACHEATTR_WRITEBACK_WRITEALLOCATE    1
#define ARM64_RGNCACHEATTR_WRITETHROUGH               2
#define ARM64_RGNCACHEATTR_WRITEBACK_NOWRITEALLOCATE  3

#define ARM64_SHATTR_NON_SHAREABLE    0
#define ARM64_SHATTR_OUTER_SHAREABLE  2
#define ARM64_SHATTR_INNER_SHAREABLE  3

#define SMMUV3_PAGE_1_OFFSET  0x10000

// log2 size of the command queue
#define SMMUV3_COMMAND_QUEUE_LOG2ENTRIES  (8)

//
// Define the size of each entry in the command queue.
//
#define SMMUV3_COMMAND_QUEUE_ENTRY_SIZE  (sizeof(SMMUV3_CMD_GENERIC))

//
// Macros to compute command queue size given its Log2 size.
//
#define SMMUV3_COMMAND_QUEUE_SIZE_FROM_LOG2(QueueLog2Size) \
    ((UINT32)(1UL << (QueueLog2Size)) * \
        (UINT16)(SMMUV3_COMMAND_QUEUE_ENTRY_SIZE))

// log2 size of the event queue
#define SMMUV3_EVENT_QUEUE_LOG2ENTRIES  (7)

//
// Define the size of each entry in the event queue.
//
#define SMMUV3_EVENT_QUEUE_ENTRY_SIZE  (sizeof(SMMUV3_FAULT_RECORD))

//
// Macros to compute event queue size given its Log2 size.
//
#define SMMUV3_EVENT_QUEUE_SIZE_FROM_LOG2(QueueLog2Size) \
    ((UINT32)(1UL << (QueueLog2Size)) * (UINT16)(SMMUV3_EVENT_QUEUE_ENTRY_SIZE))

#define SMMUV3_COUNT_FROM_LOG2(Log2Size)  (1UL << (Log2Size))

//
// Macro to determine if a queue is empty. It is empty if the producer and
// consumer indices are equal and their wrap bits are also equal.
//
#define SMMUV3_IS_QUEUE_EMPTY(ProducerIndex, \
                              ProducerWrap, \
                              ConsumerIndex, \
                              ConsumerWrap) \
                                            \
    (((ProducerIndex) == (ConsumerIndex)) && ((ProducerWrap) == (ConsumerWrap)))

//
// Macro to determine if a queue is full. It is full if the producer and
// consumer indices are equal and their wrap bits are different.
//
#define SMMUV3_IS_QUEUE_FULL(ProducerIndex, \
                             ProducerWrap, \
                             ConsumerIndex, \
                             ConsumerWrap) \
                                           \
    (((ProducerIndex) == (ConsumerIndex)) && ((ProducerWrap) != (ConsumerWrap)))

//
// SMMUV3 Stream Table Entry bit definitions
//
#define SMMUV3_STREAM_TABLE_ENTRY_CONFIG_STAGE_1_TRANSLATE_STAGE_2_BYPASS  0x5   // Stage 1 Translate, Stage 2 Bypass
#define SMMUV3_STREAM_TABLE_ENTRY_CONFIG_STAGE_2_BYPASS_STAGE_1_BYPASS     0x4   // Stage 2 Bypass, Stage 1 Bypass
#define SMMUV3_STREAM_TABLE_ENTRY_EATS_NOT_SUPPORTED                       0     // ATS not supported
#define SMMUV3_STREAM_TABLE_ENTRY_OUTPUT_ADDRESS_MAX                       48    // 48 bit output address width max
#define SMMUV3_STREAM_TABLE_ENTRY_SHCFG_INCOMING_SHAREABILITY              1     // Incoming shareability attribute
#define SMMUV3_STREAM_TABLE_ENTRY_VALID                                    1     // Entry is valid

//
// SMMUV3 Stage 1 Stream Table Entry bit definitions (used when
// Config = STAGE_1_TRANSLATE_STAGE_2_BYPASS). The S2VMID field is
// ignored when only Stage 1 is enabled and TLB entries
// are tagged with VMID = 0, so STE.S2Vmid is left 0 and every Stage 1
// TLB invalidation targets VMID = 0.
//
#define SMMUV3_STREAM_TABLE_ENTRY_S1CONTEXTPTR_OFFSET  6                         // S1ContextPtr address is stored shifted right by 6
#define SMMUV3_STREAM_TABLE_ENTRY_S1FMT_LINEAR         0                         // Linear single-CD format
#define SMMUV3_STREAM_TABLE_ENTRY_S1CDMAX_SINGLE_CD    0                         // 2^0 = 1 CD, no SubStreamID support
#define SMMUV3_STREAM_TABLE_ENTRY_S1DSS_ABORT          0x2                       // Default SubStreamID handling: abort untagged DMA
#define SMMUV3_STREAM_TABLE_ENTRY_S1STALLD_TERMINATE   1                         // Force Stage 1 stalling faults to terminate
#define SMMUV3_STREAM_TABLE_ENTRY_S1_ONLY_VMID         0                         // Per §5.2: S2Vmid ignored, TLB tagged VMID 0

//
// SMMUV3 Context Descriptor field values used by the Stage 1 CD.
//
#define SMMUV3_CD_TG0_4KB      0                            // 4KB granule
#define SMMUV3_CD_AA64         1                            // AArch64 translation regime
#define SMMUV3_CD_TTB0_OFFSET  4                            // Ttb0 is stored shifted right by 4
#define SMMUV3_CD_EPD1         1                            // Disable TTBR1 (TTBR0-only walk)

//
// CD.Ars packed { A(bit2), R(bit1), S(bit0) }: 0x6 = abort + record fault
// event (visible in event queue).
//
#define SMMUV3_CD_ARS_ABORT_RECORD          0x6
#define SMMUV3_CD_MAIR_ATTR0_NORMAL_WBWA    0xFFULL         // MAIR[0] = Normal Inner+Outer WBWA
#define SMMUV3_CD_MAIR_ATTR1_DEVICE_NGNRNE  0x00ULL         // MAIR[1] = Device-nGnRnE
#define SMMUV3_CD_MAIR0_NORMAL_WBWA         (SMMUV3_CD_MAIR_ATTR0_NORMAL_WBWA | (SMMUV3_CD_MAIR_ATTR1_DEVICE_NGNRNE << 8))

// ASID 0 is reserved to mark allocator exhaustion after the maximum ASID.
// Applies to both 8-bit and 16-bit ASID widths.
#define SMMU_ASID_RESERVED  0

//
// SMMUV3 Configuration bit definitions
//
#define SMMUV3_STR_TAB_BASE_CFG_FMT_LINEAR  0                // Linear Stream Table format
#define SMMUV3_STR_TAB_BASE_CFG_FMT_2LEVEL  1                // 2-Level Stream Table format
#define SMMUV3_STR_TAB_BASE_CFG_SPLIT       6                // Split bit for 2-Level Stream Table
#define SMMUV3_STR_TAB_BASE_L2_PTR_OFFSET   6                // Offset of L2 pointer in the L1 stream table entry
#define SMMUV3_STR_TAB_BASE_ADDR_OFFSET     6                // Stream Table base address offset
#define SMMUV3_STR_TAB_BASE_CMDQ_OFFSET     5                // Command queue base address offset
#define SMMUV3_STR_TAB_BASE_EVENTQ_OFFSET   5                // Event queue base address offset
#define SMMUV3_CR2_E2H                      0                // E2H bit 0
#define SMMUV3_CR2_REC_INV_SID              1                // Record C_BAD_STREAMID for invalid input streams
#define SMMUV3_CR2_PTM                      1                // PTM bit
#define SMMUV3_CR0_EVENTQ_EN                1                // Event queue enable
#define SMMUV3_CR0_CMDQ_EN                  1                // Command queue enable
#define SMMUV3_CR0_SMMU_EN                  1                // SMMU enable
#define SMMUV3_CR0_EVENTQ_EN                1                // Event queue enable
#define SMMUV3_CR0_CMDQ_EN                  1                // Command queue enable
#define SMMUV3_CR0_PRIQ_EN_DISABLED         0                // Disable PRI queue
#define SMMUV3_CR0_VMW_DISABLED             0                // Disable VMID wildcard matching
#define SMMUV3_CR0_ATS_CHK_DISABLE          1                // Disable bypass for ATS translated traffic

typedef enum _SMMU_ADDRESS_SIZE_TYPE {
  SmmuAddressSize32Bit = 0,
  SmmuAddressSize36Bit = 1,
  SmmuAddressSize40Bit = 2,
  SmmuAddressSize42Bit = 3,
  SmmuAddressSize44Bit = 4,
  SmmuAddressSize48Bit = 5,
  SmmuAddressSize52Bit = 6,
} SMMU_ADDRESS_SIZE_TYPE;

typedef struct _RMR_NODE_INFO {
  EFI_ACPI_6_0_IO_REMAPPING_RMR_NODE    *RmrNode; // Pointer to the RMR node
  LIST_ENTRY                            Link;     // Link to the RMR node in the list
} RMR_NODE_INFO;

// Single node in a StreamID list returned by DeviceHandleToStreamId.
// Allocated from pool; freed by SmmuStreamIdListFree.
typedef struct {
  LIST_ENTRY    Link;
  UINT32        StreamId;
} SMMU_STREAM_ID_ENTRY;

// General SMMU Information for a SMMU instance
typedef struct _SMMU_INFO {
  VOID          *SharedAbortL2; // 2-level only: shared L2 page of all-ABORT STEs. L1 entries point here until split-on-write.
  VOID          *StreamTable;
  VOID          *CommandQueue;
  VOID          *EventQueue;
  LIST_ENTRY    RmrNodeList;
  UINT64        SmmuBase;
  UINT64        CachedProducer;
  UINT64        CachedConsumer;
  UINT32        StreamTableSize;
  UINT32        StreamTableEntryMax;
  UINT32        Flags;
  UINT32        CommandQueueSize;
  UINT32        EventQueueSize;
  UINT32        StreamTableLog2Size;
  UINT32        CommandQueueLog2Size;
  UINT32        EventQueueLog2Size;
  UINT32        OutputAddressWidth;
  UINT8         TranslationStartingLevel;
  BOOLEAN       RangeInvalidationSupported;
  BOOLEAN       EBSBehaviorAbort;
  BOOLEAN       Enabled;
  BOOLEAN       TwoLevelStreamTableSupported; // Whether the SMMU supports 2-level stream tables, which allows more entries than can fit in a single page.
  BOOLEAN       Asid16Supported;              // IDR0.ASID16. FALSE = 8-bit ASIDs.
  UINT16        NextAsid;                     // Next per-stream ASID to hand out. Starts at 1.
  UINTN         EvtqIrqNum;
  UINTN         GerrIrqNum;
} SMMU_INFO;

// IoMmu configuration structure
typedef struct _IOMMU_CONFIG {
  UINT32                  SmmuCount;
  SMMU_INFO               *SmmuInfo;
  // Platform-provided UniqueId -> Named Component ObjectName mapping for NonDiscoverable devices.
  SMMU_NC_DEVICE_ENTRY    *NcDeviceList;
  UINT32                  NcDeviceCount;
} IOMMU_CONFIG;

// IOMMU/SMMU instance
extern IOMMU_CONFIG                      *mIoMmu;
extern EFI_HARDWARE_INTERRUPT2_PROTOCOL  *mGicInterrupt;
extern VOID                              *mIortData;     // Global IORT data pointer

/**
  Decode the address width from the given address size type.

  @param [in]  AddressSizeType  The address size type.

  @return The decoded address width. 0 if the address size type is invalid.
**/
UINT32
SmmuV3DecodeAddressWidth (
  IN UINT32  AddressSizeType
  );

/**
  Encode the address width to the corresponding address size type.

  @param [in]  AddressWidth  The address width.

  @return The encoded address size type. 0 if the address width is invalid.
**/
UINT8
SmmuV3EncodeAddressWidth (
  IN UINT32  AddressWidth
  );

/**
  Set the translation starting level for SMMUv3 page tables.
  Only 3 and 4 level paging are supported.

  A single-page L1 root covers up to PAGE_TABLE_L1_ADDRESS_WIDTH_MAX bits.
  Wider inputs use an L0 root; the CD starting level is inferred from T0Sz.

  @param [in]  SmmuInfo            Pointer to the SMMU_INFO structure.
  @param [in]  OutputAddressWidth  The output address width.

  @retval EFI_SUCCESS              Success.
  @retval EFI_INVALID_PARAMETER    Invalid parameter.
**/
EFI_STATUS
SmmuV3SetTranslationStartingLevel (
  IN SMMU_INFO  *SmmuInfo,
  IN UINT32     OutputAddressWidth
  );

/**
  Read a 32-bit value from the specified SMMU register.

  @param [in]  SmmuBase   The base address of the SMMU.
  @param [in]  Register   The offset of the register.

  @return The 32-bit value read from the register. 0 if the SMMU base address is invalid.
**/
UINT32
SmmuV3ReadRegister32 (
  IN UINT64  SmmuBase,
  IN UINT64  Register
  );

/**
  Read a 64-bit value from the specified SMMU register.

  @param [in]  SmmuBase   The base address of the SMMU.
  @param [in]  Register   The offset of the register.

  @return The 64-bit value read from the register. 0 if the SMMU base address is invalid.
**/
UINT64
SmmuV3ReadRegister64 (
  IN UINT64  SmmuBase,
  IN UINT64  Register
  );

/**
  Write a 32-bit value to the specified SMMU register.

  @param [in]  SmmuBase   The base address of the SMMU.
  @param [in]  Register   The offset of the register.
  @param [in]  Value      The 32-bit value to write.

  @return The 32-bit value written to the register, or 0 if the SMMU base address is invalid.
**/
UINT32
SmmuV3WriteRegister32 (
  IN UINT64  SmmuBase,
  IN UINT64  Register,
  IN UINT32  Value
  );

/**
  Write a 64-bit value to the specified SMMU register.

  @param [in]  SmmuBase   The base address of the SMMU.
  @param [in]  Register   The offset of the register.
  @param [in]  Value      The 64-bit value to write.

  @return The 64-bit value written to the register, or 0 if the SMMU base address is invalid.
**/
UINT64
SmmuV3WriteRegister64 (
  IN UINT64  SmmuBase,
  IN UINT64  Register,
  IN UINT64  Value
  );

/**
  Disable interrupts for the SMMUv3.

  @param [in]  SmmuBase          The base address of the SMMU.
  @param [in]  ClearStaleErrors  Whether to clear stale errors.

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  Invalid Parameters.
  @retval EFI_TIMEOUT            Timeout.
**/
EFI_STATUS
SmmuV3DisableInterrupts (
  IN UINT64   SmmuBase,
  IN BOOLEAN  ClearStaleErrors
  );

/**
  Enable interrupts for the SMMUv3.

  @param [in]  SmmuBase  The base address of the SMMU.

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  Invalid Parameters.
  @retval EFI_TIMEOUT            Timeout.
**/
EFI_STATUS
SmmuV3EnableInterrupts (
  IN UINT64  SmmuBase
  );

/**
  Disable translation for the SMMUv3.

  @param [in]  SmmuBase  The base address of the SMMU.

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  Invalid Parameters.
  @retval EFI_TIMEOUT            Timeout.
**/
EFI_STATUS
SmmuV3DisableTranslation (
  IN UINT64  SmmuBase
  );

/**
  Set the Smmu in ABORT mode and stop DMA.

  @param [in]  SmmuReg    Base address of the SMMUv3.

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  Invalid Parameters.
  @retval EFI_TIMEOUT            Timeout.
**/
EFI_STATUS
SmmuV3GlobalAbort (
  IN  UINT64  SmmuBase
  );

/**
  Set all streams to bypass the SMMU.

  @param [in]  SmmuReg    Base address of the SMMUv3.

  @retval EFI_SUCCESS            Success.
  @retval EFI_TIMEOUT            Timeout.
  @retval EFI_INVALID_PARAMETER  Invalid Parameters.
**/
EFI_STATUS
SmmuV3SetGlobalBypass (
  IN UINT64  SmmuBase
  );

/**
  Poll the SMMU register and test the value based on the mask.

  @param [in]  SmmuBase   Base address of the SMMU.
  @param [in]  SmmuReg    The SMMU register to poll.
  @param [in]  Mask       Mask of register bits to monitor.
  @param [in]  Value      Expected value.

  @retval EFI_SUCCESS            Success.
  @retval EFI_TIMEOUT            Timeout.
  @retval EFI_INVALID_PARAMETER  Invalid Parameters.
**/
EFI_STATUS
SmmuV3Poll (
  IN UINT64  SmmuBase,
  IN UINT64  SmmuReg,
  IN UINT32  Mask,
  IN UINT32  Value
  );

/**
  Consume the event queue for errors and retrieve the fault record.
  Clears the outputted FaultRecord if the queue is empty.

  @param [in]  SmmuInfo     Pointer to the SMMU_INFO structure.
  @param [out] FaultRecord  Pointer to the fault record structure.
  @param [out] IsEmpty      Flag to indicate if the queue is empty.

  @retval EFI_SUCCESS            Success.
  @retval EFI_TIMEOUT            Timeout.
  @retval EFI_INVALID_PARAMETER  Invalid Parameters.
**/
EFI_STATUS
SmmuV3ConsumeEventQueueForErrors (
  IN SMMU_INFO             *SmmuInfo,
  OUT SMMUV3_FAULT_RECORD  *FaultRecord,
  OUT BOOLEAN              *IsEmpty
  );

/**
  Dump the page table entries for a given virtual address.
  Dumps PTE's for all levels regardless of the starting level chosen for translation.

  @param [in]  SmmuInfo        Pointer to the SMMU_INFO structure.
  @param [in]  VirtualAddress  The virtual address to dump.
  @param [in]  Root            Pointer to the root page table.
**/
VOID
SmmuV3DumpPageTableEntries (
  IN SMMU_INFO   *SmmuInfo,
  IN UINT64      VirtualAddress,
  IN PAGE_TABLE  *Root
  );

/**
  Check if an entire address range has a valid identity mapping in the
  Stage 1 translation table.

  Walks Root using SmmuInfo's translation parameters for every 4 KB page in
  [Address, Address + Pages * EFI_PAGE_SIZE). Address is rounded down to the
  enclosing page. Since this driver identity-maps DMA a page counts as mapped only when:
    - every intermediate level has a non-zero descriptor,
    - the leaf entry has PAGE_TABLE_ENTRY_VALID_BIT set, and
    - the leaf entry's encoded physical address matches the page address.

  Returns TRUE only if every page in the range satisfies the above; FALSE on
  the first page that fails (short-circuit), bad parameters, or Pages == 0.

  @param [in]  SmmuInfo  Pointer to the SMMU_INFO structure whose translation
                         parameters drive the walk.
  @param [in]  Root      Pointer to the root page table.
  @param [in]  Address   Start address of the range (same value used as both
                         VA and PA under identity mapping). Aligned down to
                         the enclosing 4 KB page.
  @param [in]  Pages     Number of 4 KB pages to check, starting at Address.

  @retval TRUE   Every page in the range has a valid identity-mapped leaf.
  @retval FALSE  At least one page is not mapped, encodes a different PA, or
                 a required parameter is invalid.
**/
BOOLEAN
SmmuV3IsAddressRangeMapped (
  IN SMMU_INFO   *SmmuInfo,
  IN PAGE_TABLE  *Root,
  IN UINT64      Address,
  IN UINTN       Pages
  );

/**
  Log the errors if found from the SMMUv3. Prints Event Queue entries and GError register.
  Does nothing if no errors found.

  @param [in]  SmmuInfo  Pointer to the SMMU_INFO structure.

  @retval EFI_SUCCESS            No SMMU errors found.
  @retval EFI_INVALID_PARAMETER  Invalid Parameters.
  @retval EFI_DEVICE_ERROR       SMMU error found.
**/
EFI_STATUS
SmmuV3LogErrors (
  IN SMMU_INFO  *SmmuInfo
  );

/**
  Register GIC interrupt sources for SmmuV3 EVTQ and GERR interrupts.

  @param[in] GicInterrupt  Pointer to the GIC interrupt protocol.
  @param[in] SmmuInfo      Pointer to the SMMU_INFO structure.

  @retval EFI_SUCCESS           The interrupt sources were registered successfully.
  @retval EFI_INVALID_PARAMETER The GicInterrupt or SmmuInfo is NULL.
**/
EFI_STATUS
SmmuV3RegisterGicIsr (
  IN EFI_HARDWARE_INTERRUPT2_PROTOCOL  *GicInterrupt,
  IN SMMU_INFO                         *SmmuInfo
  );

/**
  Send a SMMUV3_CMD_GENERIC command to the SMMUv3.

  @param [in]  SmmuInfo  Pointer to the SMMU_INFO structure.
  @param [in]  Command   Pointer to the command to send.

  @retval EFI_SUCCESS            Success.
  @retval EFI_TIMEOUT            Timeout.
  @retval EFI_INVALID_PARAMETER  Invalid Parameters.
**/
EFI_STATUS
SmmuV3SendCommand (
  IN SMMU_INFO           *SmmuInfo,
  IN SMMUV3_CMD_GENERIC  *Command
  );

/**
  Invalidate all Stage 1 TLB entries owned by the given ASID on this SMMU.
  Uses CMD_TLBI_NH_ASID with VMID = 0 (Stage 1 only mode tags TLBs with VMID = 0).

  @param [in]  SmmuInfo  Pointer to the SMMU_INFO structure.
  @param [in]  Asid      ASID to invalidate.

  @retval EFI_SUCCESS            Success.
  @retval EFI_TIMEOUT            Timeout.
  @retval EFI_INVALID_PARAMETER  Invalid Parameters.
**/
EFI_STATUS
SmmuV3TLBInvalidateAllStage1 (
  IN SMMU_INFO  *SmmuInfo,
  IN UINT16     Asid
  );

/**
 * Add RMR mappings for each SMMU node in the SmmuInfo structure.
 * This function iterates through the RMR nodes and updates the page table
 * for each memory range described in the RMR node.
 *
 * @param [in] SmmuInfo  Pointer to the SMMU_INFO structure.
 *
 * @retval EFI_SUCCESS            Success.
 * @retval EFI_INVALID_PARAMETER  Invalid Parameters.
 * @retval Other                  RMR mapping update failure.
 */
EFI_STATUS
SmmuV3AddRMRMapping (
  IN SMMU_INFO  *SmmuInfo
  );

/**
 * Parse IORT table and extract SMMU information
 *
 * @param[in]  IortTable    Pointer to the IORT table
 * @param[out] SmmuInfo     Pointer to store the array of SMMU_INFO structures
 * @param[out] SmmuCount    Pointer to store the number of SMMU nodes found
 *
 * @return EFI_SUCCESS on success
 * @return EFI_INVALID_PARAMETER if any parameter is NULL
 * @return EFI_OUT_OF_RESOURCES if memory allocation fails
 * @return EFI_NOT_FOUND if no SMMU nodes are found
 * @return EFI_UNSUPPORTED if the IORT table is not supported
 */
EFI_STATUS
SmmuV3ParseIort (
  IN  VOID       *IortTable,
  OUT SMMU_INFO  **SmmuInfo,
  OUT UINT32     *SmmuCount
  );

/**
  Allocate a single-page per-stream page-table root suitable for use as
  CD.TTB0 for Stage 1 translation.

  @param [in]  SmmuInfo  SMMU instance the root is allocated for. Must not be NULL.

  @retval Pointer to the zeroed root, or NULL on failure.
**/
PAGE_TABLE *
SmmuV3AllocatePageTableRoot (
  IN SMMU_INFO  *SmmuInfo
  );

/**
  Recursively free a per-stream page-table tree previously returned by
  SmmuV3AllocatePageTableRoot(). Each table, including the root, occupies
  a single page.

  @param [in]  SmmuInfo   SMMU instance the tree was allocated for.
  @param [in]  Level      Current level (caller must pass 0 for the root).
  @param [in]  PageTable  The page-table tree to free. May be NULL.
**/
VOID
SmmuV3FreePageTableTree (
  IN SMMU_INFO   *SmmuInfo,
  IN UINT8       Level,
  IN PAGE_TABLE  *PageTable
  );

/**
  Ensure a Stage 1 page-table root exists for the given StreamID. If this is
  the first call for the StreamID on this SMMU, allocate a fresh root, CD,
  and ASID and promote the corresponding STE from ABORT to Stage 1
  translation using a break-before-make sequence (CFGI_STE + CMD_SYNC
  twice). Subsequent calls return the root and ASID from the live STE's CD.

  @param [in]   SmmuInfo  Pointer to the SMMU_INFO structure.
  @param [in]   StreamId  The StreamID.
  @param [out]  OutRoot   Receives the Stage 1 page-table root.
  @param [out]  OutTagId  Receives the ASID tag installed in the CD.

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  Invalid parameters.
  @retval EFI_OUT_OF_RESOURCES   Allocation failed / ASID space exhausted.
  @retval Other                  STE promotion failure.
**/
EFI_STATUS
SmmuV3StreamGetOrCreate (
  IN  SMMU_INFO   *SmmuInfo,
  IN  UINT32      StreamId,
  OUT PAGE_TABLE  **OutRoot,
  OUT UINT16      *OutTagId
  );

/**
  Free every SMMU_STREAM_ID_ENTRY hanging off the given list head and
  re-initialize the head as empty. Safe to call on an already-empty list.

  @param[in,out] StreamIdList  List head previously populated by
                               DeviceHandleToStreamId.
**/
VOID
SmmuStreamIdListFree (
  IN OUT LIST_ENTRY  *StreamIdList
  );

/**
  Resolve a DeviceHandle to its IORT-derived StreamID(s).

  For real PCIe devices (Segment != 0xFF):
    PciIo->GetLocation() -> RID -> IORT RC node ID mapping -> single StreamID.
    The matched mapping's OutputReference identifies the owning SMMUv3 node,
    whose base address is returned in *SmmuBase.

  For NonDiscoverable devices (Segment == 0xFF):
    UniqueId -> platform NC table entry -> IORT Named Component node ->
    full StreamID list (every mapping expanded, including ranges).

  @param[in]      IortTable     Pointer to the IORT ACPI table.
  @param[in]      DeviceHandle  The device handle to resolve.
  @param[in,out]  StreamIdList  Caller-supplied, initialized-empty list head.
                                On success contains one SMMU_STREAM_ID_ENTRY
                                per resolved StreamID, in IORT order (first
                                entry is the primary; rest are aliases that
                                share the primary's Stage 1 page table, CD,
                                and ASID). Caller must release via
                                SmmuStreamIdListFree.
  @param[out]     SmmuBase      Optional. If non-NULL, receives the base
                                address of the SMMUv3 node that owns these
                                StreamIDs (0 if unknown).

  @retval EFI_SUCCESS           StreamIDs resolved.
  @retval EFI_INVALID_PARAMETER One or more required parameters are NULL.
  @retval EFI_UNSUPPORTED       DeviceHandle has no PciIo protocol.
  @retval EFI_NOT_FOUND         No IORT mapping found.
  @retval EFI_OUT_OF_RESOURCES  Allocation failure while building the list.
**/
EFI_STATUS
DeviceHandleToStreamId (
  IN     VOID        *IortTable,
  IN     EFI_HANDLE  DeviceHandle,
  IN OUT LIST_ENTRY  *StreamIdList,
  OUT    UINT64      *SmmuBase
  );

/**
  Build an invalid Stage 1 stream-table entry with Cd = NULL and Valid = 0.
  Used at init to populate every STE slot before any device has been mapped.

  @param [in]   SmmuInfo     SMMU instance (needed for IDR-derived fields).
  @param [out]  StreamEntry  STE buffer to populate.

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  Invalid parameters.
  @retval Other                  Failure from the underlying translate STE builder.
**/
EFI_STATUS
SmmuV3BuildInvalidStreamTableEntry (
  IN  SMMU_INFO                  *SmmuInfo,
  OUT SMMUV3_STREAM_TABLE_ENTRY  *StreamEntry
  );

/**
  Locate the STE slot in the SMMU's stream table for a given StreamID.
  Supports both linear and 2-level stream tables.

  @param [in]  SmmuInfo  Pointer to the SMMU_INFO structure.
  @param [in]  StreamId  The StreamID.

  @retval Pointer to the STE slot, or NULL if out-of-range.
**/
SMMUV3_STREAM_TABLE_ENTRY *
SmmuV3GetSteSlot (
  IN SMMU_INFO  *SmmuInfo,
  IN UINT32     StreamId
  );

/**
  Allocate a Context Descriptor (CD) suitable for use as a Stage 1 STE's
  S1ContextPtr. The CD is 64-byte aligned as required by the SMMUv3 spec
  and zero-initialized (V=0). The caller populates Ttb0 / Asid / TCR
  fields via SmmuV3BuildStage1ContextDescriptor before publishing it via
  SmmuV3PromoteSteToStage1Translate.

  @retval Pointer to the zeroed CD, or NULL on failure.
**/
SMMUV3_CONTEXT_DESCRIPTOR *
SmmuV3AllocateContextDescriptor (
  VOID
  );

/**
  Free a Context Descriptor previously returned by
  SmmuV3AllocateContextDescriptor.

  @param [in]  Cd  Context Descriptor to free. May be NULL.
**/
VOID
SmmuV3FreeContextDescriptor (
  IN SMMUV3_CONTEXT_DESCRIPTOR  *Cd
  );

/**
  Populate a Context Descriptor for Stage 1 identity-mapped translation.
  Sets Ttb0 to the given page-table root, ASID to the supplied per-stream
  tag, T0Sz / TG0 / IPS / IR0 / OR0 / SH0 based on the SMMU's output
  address width and the platform's coherency configuration, MAIR to
  attribute index 0 = Normal WB Inner+Outer, disables TTBR1, and sets
  AArch64 + Valid = 1.

  @param [in]   SmmuInfo       SMMU instance (needed for IDR-derived fields).
  @param [in]   PageTableRoot  Stage 1 page-table root to install in Ttb0.
                               NULL builds an invalid (V = 0) CD suitable
                               for the init-time template.
  @param [in]   Asid           ASID tag installed in CD.Asid.
  @param [out]  Cd             CD buffer to populate.

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  Invalid parameters.
  @retval Other                  Failure from starting-level computation.
**/
EFI_STATUS
SmmuV3BuildStage1ContextDescriptor (
  IN  SMMU_INFO                  *SmmuInfo,
  IN  PAGE_TABLE                 *PageTableRoot,
  IN  UINT16                     Asid,
  OUT SMMUV3_CONTEXT_DESCRIPTOR  *Cd
  );

/**
  Build a STAGE_1_TRANSLATE / STAGE_2_BYPASS stream-table entry that
  points at the supplied Context Descriptor. Passing Cd = NULL builds an
  invalid (Valid = 0) STE suitable for the init-time template.

  @param [in]   SmmuInfo     SMMU instance.
  @param [in]   Cd           CD the STE's S1ContextPtr should point at, or
                             NULL to build an invalid STE.
  @param [out]  StreamEntry  STE buffer to populate.

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  Invalid parameters.
**/
EFI_STATUS
SmmuV3BuildStage1TranslateStreamTableEntry (
  IN  SMMU_INFO                  *SmmuInfo,
  IN  SMMUV3_CONTEXT_DESCRIPTOR  *Cd,
  OUT SMMUV3_STREAM_TABLE_ENTRY  *StreamEntry
  );

/**
  Promote the STE for StreamId from ABORT to STAGE_1_TRANSLATE /
  STAGE_2_BYPASS with the supplied CD, using the SMMU break-before-make
  sequence required for STE Config changes.

  @param [in]  SmmuInfo        Pointer to the SMMU_INFO structure.
  @param [in]  StreamId        The StreamID whose STE is being promoted.
  @param [in]  Cd              Context Descriptor to install in the STE.
  @param [in]  NewL2           Caller-provided L2 page used by
                               SmmuV3SplitL1IfShared() when the covering L1
                               descriptor still points at the shared-ABORT
                               L2. Because the caller owns Cd and NewL2,
                               this function does not allocate or free them.
  @param [out] NewL2Consumed   Set to TRUE if NewL2 was installed into an L1
                               descriptor by the split step (the caller must
                               NOT free it), FALSE otherwise (the caller
                               should free NewL2).

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  Invalid parameters.
  @retval Other                  Command-queue / sync failure.
**/
EFI_STATUS
SmmuV3PromoteSteToStage1Translate (
  IN  SMMU_INFO                  *SmmuInfo,
  IN  UINT32                     StreamId,
  IN  SMMUV3_CONTEXT_DESCRIPTOR  *Cd,
  IN  SMMUV3_STREAM_TABLE_ENTRY  *NewL2,
  OUT BOOLEAN                    *NewL2Consumed
  );

/**
  Update the page table mapping with the given physical address and attributes.

  @param [in]  SmmuInfo                   SMMU instance.
  @param [in]  Root                       Pointer to the root page table.
  @param [in]  TagId                      Per-stream ASID whose TLB entries
                                          should be invalidated on unmap.
  @param [in]  PhysicalAddress            Physical address to map.
  @param [in]  Bytes                      Number of bytes to map.
  @param [in]  Attributes                 IOMMU access flags encoded by PAGE_TABLE_READ_WRITE_FROM_IOMMU_ACCESS (12 bits or less).
  @param [in]  Valid                      Boolean to indicate if the entry is valid.

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  Invalid parameter.
  @retval EFI_OUT_OF_RESOURCES   Out of resources.
**/
EFI_STATUS
UpdatePageTable (
  IN SMMU_INFO   *SmmuInfo,
  IN PAGE_TABLE  *Root,
  IN UINT16      TagId,
  IN UINT64      PhysicalAddress,
  IN UINT64      Bytes,
  IN UINT16      Attributes,
  IN BOOLEAN     Valid
  );
