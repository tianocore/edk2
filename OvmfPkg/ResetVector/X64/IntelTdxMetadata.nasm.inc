;------------------------------------------------------------------------------
; @file
; Tdx Virtual Firmware metadata
;
; When host VMM creates a new guest TD, some initial set of TD-private pages
; are added using the TDH.MEM.PAGE.ADD function. These pages typically contain
; Virtual BIOS code and data along with some clear pages for stacks and heap.
; In the meanwhile, some configuration data need be measured by host VMM.
; Tdx Metadata is designed for this purpose to indicate host VMM how to do the
; above tasks.
;
; Tdx Metadata consists of a DESCRIPTOR as the header followed by several
; SECTIONs. Host VMM sets up the memory for TDVF according to these sections.
;
; _Bfv is the example (Bfv refers to the Virtual BIOS code).
; - By DataOffset/RawDataSize host VMM knows about the position of the code
;   in the binary image.
; - MemoryAddress/MemoryDataSize indicates the guest physical address/size of
;   the Bfv to be loaded.
; - Type field means this section is of BFV. This field is designed for the
;   purpose that in some case host VMM may do some additional processing based
;   upon the section type. TdHob section is an example. Host VMM pass the
;   physical memory information to the guest firmware by writing the data in
;   the memory region designated by TdHob section.
; - By design code part of the binary image (Bfv) should be measured by host
;   VMM. This is indicated by the Attributes field.
;
; So put all these information together, when a new guest is being created,
; the initial TD-private pages for BFV is added by TDH.MEM.PAGE.ADD function,
; and Bfv is loaded at the guest physical address indicated by MemoryAddress.
; Since the Attributes is TDX_METADATA_ATTRIBUTES_EXTENDMR, Bfv is measured by
; host VMM.
;
; Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
;------------------------------------------------------------------------------

BITS    64

%define TDX_METADATA_SECTION_TYPE_BFV       0
%define TDX_METADATA_SECTION_TYPE_CFV       1
%define TDX_METADATA_SECTION_TYPE_TD_HOB    2
%define TDX_METADATA_SECTION_TYPE_TEMP_MEM  3
%define TDX_METADATA_VERSION                1
%define TDX_METADATA_ATTRIBUTES_EXTENDMR    0x00000001

ALIGN   16
TIMES (15 - ((TdxGuidedStructureEnd - TdxGuidedStructureStart + 15) % 16)) DB 0

TdxGuidedStructureStart:

;
; TDVF meta data
;
TdxMetadataGuid:
  DB  0xf3, 0xf9, 0xea, 0xe9, 0x8e, 0x16, 0xd5, 0x44
  DB  0xa8, 0xeb, 0x7f, 0x4d, 0x87, 0x38, 0xf6, 0xae

_Descriptor:
  DB 'T','D','V','F'                                  ; Signature
  DD TdxGuidedStructureEnd - _Descriptor              ; Length
  DD TDX_METADATA_VERSION                             ; Version
  DD (TdxGuidedStructureEnd - _Descriptor - 16)/32    ; Number of sections

_Bfv:
  DD TDX_BFV_RAW_DATA_OFFSET
  DD TDX_BFV_RAW_DATA_SIZE
  DQ TDX_BFV_MEMORY_BASE
  DQ TDX_BFV_MEMORY_SIZE
  DD TDX_METADATA_SECTION_TYPE_BFV
  DD TDX_METADATA_ATTRIBUTES_EXTENDMR

_Cfv:
  DD TDX_CFV_RAW_DATA_OFFSET
  DD TDX_CFV_RAW_DATA_SIZE
  DQ TDX_CFV_MEMORY_BASE
  DQ TDX_CFV_MEMORY_SIZE
  DD TDX_METADATA_SECTION_TYPE_CFV
  DD 0

_TdxHeapStack:
  DD 0
  DD 0
  DQ TDX_HEAP_STACK_BASE
  DQ TDX_HEAP_STACK_SIZE
  DD TDX_METADATA_SECTION_TYPE_TEMP_MEM
  DD 0

_TdxInitMem:
  DD 0
  DD 0
  DQ TDX_INIT_MEMORY_BASE
  DQ TDX_INIT_MEMORY_SIZE
  DD TDX_METADATA_SECTION_TYPE_TEMP_MEM
  DD 0

_TdHob:
  DD 0
  DD 0
  DQ TDX_HOB_MEMORY_BASE
  DQ TDX_HOB_MEMORY_SIZE
  DD TDX_METADATA_SECTION_TYPE_TD_HOB
  DD 0

_OvmfPageTable:
  DD 0
  DD 0
  DQ OVMF_PAGE_TABLE_BASE
  DQ OVMF_PAGE_TABLE_SIZE
  DD TDX_METADATA_SECTION_TYPE_TEMP_MEM
  DD 0

TdxGuidedStructureEnd:
ALIGN   16
