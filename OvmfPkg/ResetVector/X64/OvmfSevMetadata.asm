;-----------------------------------------------------------------------------
; @file
; OVMF metadata for the AMD SEV confidential computing guests
;
; Copyright (c) 2021, AMD Inc. All rights reserved.<BR>
;
; SPDX-License-Identifier: BSD-2-Clause-Patent
;-----------------------------------------------------------------------------

BITS  64

%define OVMF_SEV_METADATA_VERSION     1

; The section must be accepted or validated by the VMM before the boot
%define OVMF_SECTION_TYPE_SNP_SEC_MEM     0x1

ALIGN 16

TIMES (15 - ((OvmfSevGuidedStructureEnd - OvmfSevGuidedStructureStart + 15) % 16)) DB 0

OvmfSevGuidedStructureStart:
;
; OvmfSev metadata descriptor
;
OvmfSevMetadataGuid:

_DescriptorSev:
  DB 'A','S','E','V'                                        ; Signature
  DD OvmfSevGuidedStructureEnd - _DescriptorSev             ; Length
  DD OVMF_SEV_METADATA_VERSION                              ; Version
  DD (OvmfSevGuidedStructureEnd - _DescriptorSev - 16) / 12 ; Number of sections

OvmfSevGuidedStructureEnd:
  ALIGN   16
