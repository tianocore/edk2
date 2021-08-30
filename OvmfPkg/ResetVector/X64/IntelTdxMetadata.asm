;------------------------------------------------------------------------------
; @file
; Tdx Virtual Firmware metadata
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

_Stack:
  DD 0
  DD 0
  DQ TDX_STACK_MEMORY_BASE
  DQ TDX_STACK_MEMORY_SIZE
  DD TDX_METADATA_SECTION_TYPE_TEMP_MEM
  DD 0

_Heap:
  DD 0
  DD 0
  DQ TDX_HEAP_MEMORY_BASE
  DQ TDX_HEAP_MEMORY_SIZE
  DD TDX_METADATA_SECTION_TYPE_TEMP_MEM
  DD 0

_MailBox:
  DD 0
  DD 0
  DQ TDX_MAILBOX_MEMORY_BASE
  DQ TDX_MAILBOX_MEMORY_SIZE
  DD TDX_METADATA_SECTION_TYPE_TEMP_MEM
  DD 0

_OvmfWorkarea:
  DD 0
  DD 0
  DQ OVMF_WORK_AREA_BASE
  DQ OVMF_WORK_AREA_SIZE
  DD TDX_METADATA_SECTION_TYPE_TEMP_MEM
  DD 0

_TdHob:
  DD 0
  DD 0
  DQ TDX_HOB_MEMORY_BASE
  DQ TDX_HOB_MEMORY_SIZE
  DD TDX_METADATA_SECTION_TYPE_TD_HOB
  DD 0

_TdxPageTable:
  DD 0
  DD 0
  DQ TDX_EXTRA_PAGE_TABLE_BASE
  DQ TDX_EXTRA_PAGE_TABLE_SIZE
  DD TDX_METADATA_SECTION_TYPE_TEMP_MEM
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
