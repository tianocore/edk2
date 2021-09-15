;-----------------------------------------------------------------------------
; @file
; OVMF metadata for the confidential computing guests (TDX and SEV-SNP)
;
; Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
; Copyright (c) 2021, AMD Inc. All rights reserved.<BR>
;
; SPDX-License-Identifier: BSD-2-Clause-Patent
;-----------------------------------------------------------------------------

BITS  64

%define OVMF_METADATA_VERSION     1

%define OVMF_SECTION_TYPE_UNDEFINED   0

;The section contains the code
%define OVMF_SECTION_TYPE_CODE        0x100

; The section contains the varaibles
%define OVMF_SECTION_TYPE_VARS        0x101

; The section must be accepted or validated by the VMM before the boot
%define OVMF_SECTION_TYPE_SEC_MEM     0x102

ALIGN 16

TIMES (15 - ((OvmfGuidedStructureEnd - OvmfGuidedStructureStart + 15) % 16)) DB 0

OvmfGuidedStructureStart:
;
; Ovmf metadata descriptor
;
OvmfMetadataGuid:
  DB  0xf3, 0xf9, 0xea, 0xe9, 0x8e, 0x16, 0xd5, 0x44
  DB  0xa8, 0xeb, 0x7f, 0x4d, 0x87, 0x38, 0xf6, 0xae

_Descriptor:
  DB 'O','V','M','F'                                  ; Signature
  DD OvmfGuidedStructureEnd - _Descriptor             ; Length
  DD OVMF_METADATA_VERSION                            ; Version
  DD (OvmfGuidedStructureEnd - _Descriptor - 16) / 12 ; Number of sections

OvmfGuidedStructureEnd:
  ALIGN   16
