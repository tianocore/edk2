/** @file SmmuV3.h

    This file is the SmmuV3 header file for SMMU driver compliant with the Smmu spec:

    <https://developer.arm.com/documentation/ihi0070/latest/>

    Copyright (c) Microsoft Corporation.
    SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef SMMUV3_H_
#define SMMUV3_H_

#include <Register/SmmuV3Registers.h>
#include <Uefi/UefiBaseType.h>
#include <IndustryStandard/IoRemappingTable.h>
#include <Protocol/IoMmu.h>
#include <Protocol/HardwareInterrupt2.h>
#include <Guid/SmmuConfig.h>
#include "IoMmu.h"

// Number of levels in the page table
#define PAGE_TABLE_DEPTH  4
// If the starting level is 1, and the address width exceeds 39 bits, then the page table is concatenated
#define PAGE_TABLE_CONCATENATED_PAGES_BITS_CUTOFF  39
// Page Table Index macro to calculate the index of the page table entry based on the level and address width, supports concatenated page tables
#define PAGE_TABLE_INDEX(VirtualAddress, Level, OutputAddressWidth, TranslationStartingLevel, PageTableRootConcatenated) \
    (PageTableRootConcatenated && (Level == 1) && (TranslationStartingLevel == Level)) ? \
        (((VirtualAddress) >> (12 + (9 * ((PAGE_TABLE_DEPTH - 1) - (Level))))) & \
         ((1 << (9 + ((OutputAddressWidth) - PAGE_TABLE_CONCATENATED_PAGES_BITS_CUTOFF))) - 1)) : \
        (((VirtualAddress) >> (12 + (9 * ((PAGE_TABLE_DEPTH - 1) - (Level))))) & 0x1FF)

#define PAGE_TABLE_4_LEVEL_OUTPUT_ADDRESS_WIDTH_MIN  44
#define PAGE_TABLE_OUTPUT_ADDRESS_WIDTH_MAX          48
#define PAGE_TABLE_OUTPUT_ADDRESS_WIDTH_MIN          32
#define PAGE_TABLE_ROOT_CONCATENATED_PAGES_MAX       16

// Macro to align values down. Alignment is required to be power of 2.
#define ALIGN_DOWN_BY(length, alignment) \
    ((UINT64)(length) & ~((UINT64)(alignment) - 1))

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
#define SMMUV3_STREAM_TABLE_ENTRY_CCA                                      1     // Cache Coherent Attribute
#define SMMUV3_STREAM_TABLE_ENTRY_CPM                                      1     // Coherent Path to Memory
#define SMMUV3_STREAM_TABLE_ENTRY_DACS                                     1     // Device attributes are Cacheable and Inner-Shareable
#define SMMUV3_STREAM_TABLE_ENTRY_CONFIG_STAGE_2_TRANSLATE_STAGE_1_BYPASS  0x6   // Stage 2 Translate, Stage 1 Bypass
#define SMMUV3_STREAM_TABLE_ENTRY_CONFIG_STAGE_2_BYPASS_STAGE_1_BYPASS     0x4   // Stage 2 Bypass, Stage 1 Bypass
#define SMMUV3_STREAM_TABLE_ENTRY_EATS_NOT_SUPPORTED                       0     // ATS not supported
#define SMMUV3_STREAM_TABLE_ENTRY_S2VMID                                   1     // Pick non-zero value
#define SMMUV3_STREAM_TABLE_ENTRY_S2TG_4KB                                 0     // Granule size 4KB
#define SMMUV3_STREAM_TABLE_ENTRY_S2AA64                                   1     // AA64
#define SMMUV3_STREAM_TABLE_ENTRY_S2TTB_OFFSET                             4     // S2TTB offset >> 4
#define SMMUV3_STREAM_TABLE_ENTRY_S2PTW                                    1     // S2PTW bit
#define SMMUV3_STREAM_TABLE_ENTRY_S2SL0                                    2     // Encoded level of page table walk, 2 -> 4-level page table starting from 0
#define SMMUV3_STREAM_TABLE_ENTRY_OUTPUT_ADDRESS_MAX                       48    // 48 bit output address width max
#define SMMUV3_STREAM_TABLE_ENTRY_S2RS_RECORD_FAULTS                       2     // Record faults
#define SMMUV3_STREAM_TABLE_ENTRY_SHCFG_INCOMING_SHAREABILITY              1     // Incoming shareability attribute
#define SMMUV3_STREAM_TABLE_ENTRY_SHCFG_INNER_SHAREABLE                    3     // Inner shareable
#define SMMUV3_STREAM_TABLE_ENTRY_MTCFG                                    1     // MTCFG bit
#define SMMUV3_STREAM_TABLE_ENTRY_MEMATTR_INNER_OUTTER_WRITEBACK_CACHED    0xF   // Inner+Outer write-back cached
#define SMMUV3_STREAM_TABLE_ENTRY_VALID                                    1     // Entry is valid

//
// VMID 0 is not a valid VMID so whenever we wrap past the max VMID, we set it to 0 to mark exhaustion.
// (Applies to both 8/16-bit VMID width).
//
#define SMMU_VMID_RESERVED  0

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
  VOID          *SharedAbortL2;          // 2-level only: shared L2 page of all-ABORT STEs. L1 entries point here until split-on-write.
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
  BOOLEAN       PageTableRootConcatenated;
  BOOLEAN       RangeInvalidationSupported;
  BOOLEAN       EBSBehaviorAbort;
  BOOLEAN       Enabled;
  BOOLEAN       TwoLevelStreamTableSupported; // Whether the SMMU supports 2-level stream tables, which allows more entries than can fit in a single page.
  BOOLEAN       Vmid16Supported;              // IDR0.VMID16. FALSE = 8-bit VMIDs only.
  UINT16        NextVmid;                     // Next per-stream VMID to hand out. Starts at 1.
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

  @param [in]  SmmuInfo           Pointer to the SMMU_INFO structure.
  @param [in]  OutputAddressWidth  The output address width.
  @param [out] S2Sl0              The starting level for stage 2 translation.

  @retval EFI_SUCCESS              Success.
  @retval EFI_INVALID_PARAMETER    Invalid parameter.
**/
EFI_STATUS
SmmuV3SetTranslationStartingLevel (
  IN SMMU_INFO  *SmmuInfo,
  IN UINT32     OutputAddressWidth,
  OUT UINT64    *S2Sl0
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

  @retval None.
**/
VOID
SmmuV3DumpPageTableEntries (
  IN SMMU_INFO   *SmmuInfo,
  IN UINT64      VirtualAddress,
  IN PAGE_TABLE  *Root
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
  Invalidate all TLB entries in the SMMUv3.
  Uses CMD_TLBI_S12_VMALL to invalidate all Stage 2 TLB entries for the specified VMID.

  @param [in]  SmmuInfo  Pointer to the SMMU_INFO structure.
  @param [in]  Vmid      The VMID to invalidate.

  @retval EFI_SUCCESS            Success.
  @retval EFI_TIMEOUT            Timeout.
  @retval EFI_INVALID_PARAMETER  Invalid Parameters.
**/
EFI_STATUS
SmmuV3TLBInvalidateAll (
  IN SMMU_INFO  *SmmuInfo,
  IN UINT16     Vmid
  );

/**
  Invalidate TLB entries for specified InputAddress for Stage 2 of SmmuV3.

  @param [in]  SmmuInfo      Pointer to the SMMU_INFO structure.
  @param [in]  Vmid          The VMID to invalidate.
  @param [in]  InputAddress  The input address to invalidate.

  @retval EFI_SUCCESS            Success.
  @retval EFI_TIMEOUT            Timeout.
  @retval EFI_INVALID_PARAMETER  Invalid Parameters.
**/
EFI_STATUS
SmmuV3TLBInvalidateAddress (
  IN SMMU_INFO  *SmmuInfo,
  IN UINT16     Vmid,
  IN UINT64     InputAddress
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
  Allocate a stage-2 page table root suitable for use as an STE's S2TTB.
  Allocates the maximum concatenated-root size so it can be used regardless
  of the SMMU's configured starting level / OAS.

  @retval Pointer to the zeroed root, or NULL on failure.
**/
PAGE_TABLE *
SmmuV3AllocatePageTableRoot (
  VOID
  );

/**
  Recursively free a stage-2 page table tree previously returned by
  SmmuV3AllocatePageTableRoot().

  @param [in]  Level      Current level (caller must pass 0 for the root).
  @param [in]  PageTable  The page-table tree to free. May be NULL.
**/
VOID
SmmuV3FreePageTableTree (
  IN UINT8       Level,
  IN PAGE_TABLE  *PageTable
  );

/**
  Ensure a stage-2 page-table root exists for the given StreamID. If this is
  the first call for the StreamID on this SMMU, allocate a fresh root + VMID
  and promote the corresponding STE from ABORT to STAGE_2_TRANSLATE using a
  break-before-make sequence (CFGI_STE + CMD_SYNC twice). Subsequent calls
  return the (Root, Vmid) already encoded in the live STE.

  @param [in]   SmmuInfo  Pointer to the SMMU_INFO structure.
  @param [in]   StreamId  The StreamID.
  @param [out]  OutRoot   Receives the stage-2 page-table root.
  @param [out]  OutVmid   Receives the VMID tag installed in the STE.

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  Invalid parameters.
  @retval EFI_OUT_OF_RESOURCES   Allocation failed / VMID space exhausted.
  @retval Other                  STE promotion failure.
**/
EFI_STATUS
SmmuV3StreamGetOrCreate (
  IN  SMMU_INFO   *SmmuInfo,
  IN  UINT32      StreamId,
  OUT PAGE_TABLE  **OutRoot,
  OUT UINT16      *OutVmid
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
                                share the primary's stage-2 page table and
                                VMID). Caller must release via
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
  Build a default "abort-equivalent" stream-table entry. Equivalent to a
  STAGE_2_TRANSLATE STE with S2Ttb = 0 — stage 2 is enabled but no
  translations are installed, so DMA misses fault and are recorded. Used at
  init to populate every STE slot before any device has been mapped.

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
  Build a STAGE_2_TRANSLATE stream-table entry using the given page-table root.

  @param [in]   SmmuInfo        Pointer to the SMMU_INFO structure.
  @param [in]   PageTableRoot   Page-table root the STE should point at.
  @param [in]   Vmid            VMID tag to install in the STE's S2VMID field.
  @param [out]  StreamEntry     STE buffer to populate.

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  Invalid parameters.
**/
EFI_STATUS
SmmuV3BuildTranslateStreamTableEntry (
  IN  SMMU_INFO                  *SmmuInfo,
  IN  PAGE_TABLE                 *PageTableRoot,
  IN  UINT16                     Vmid,
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
  Promote the STE for StreamId from ABORT to STAGE_2_TRANSLATE with the given
  page-table root, using the SMMU break-before-make sequence.

  @param [in]  SmmuInfo        Pointer to the SMMU_INFO structure.
  @param [in]  StreamId        The StreamID whose STE is being promoted.
  @param [in]  Vmid            VMID tag to install in the STE's S2VMID field.
  @param [in]  PageTableRoot   Page-table root to install in the STE.

  @retval EFI_SUCCESS            Success.
  @retval EFI_INVALID_PARAMETER  Invalid parameters.
  @retval Other                  Command-queue / sync failure.
**/
EFI_STATUS
SmmuV3PromoteSteToTranslate (
  IN SMMU_INFO   *SmmuInfo,
  IN UINT32      StreamId,
  IN UINT16      Vmid,
  IN PAGE_TABLE  *PageTableRoot
  );

#endif
