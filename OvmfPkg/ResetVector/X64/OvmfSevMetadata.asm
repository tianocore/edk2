;-----------------------------------------------------------------------------
; @file
; OVMF metadata for the AMD SEV confidential computing guests
;
; Copyright (c) 2021 - 2024, AMD Inc. All rights reserved.<BR>
;
; SPDX-License-Identifier: BSD-2-Clause-Patent
;-----------------------------------------------------------------------------

BITS  64

%define OVMF_SEV_METADATA_VERSION     1

; The section must be accepted or validated by the VMM before the boot
%define OVMF_SECTION_TYPE_SNP_SEC_MEM     0x1

; AMD SEV-SNP specific sections
%define OVMF_SECTION_TYPE_SNP_SECRETS     0x2

;
; The section contains the hypervisor pre-populated CPUID values.
; In the case of SEV-SNP, the CPUID values are filtered and measured by
; the SEV-SNP firmware.
; The CPUID format is documented in SEV-SNP firmware spec 0.9 section 7.1
; (CPUID function structure).
;
%define OVMF_SECTION_TYPE_CPUID           0x3

; The SVSM Calling Area Address (CAA)
%define OVMF_SECTION_TYPE_SVSM_CAA        0x4

; Kernel hashes section for measured direct boot
%define OVMF_SECTION_TYPE_KERNEL_HASHES   0x10

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

; Region need to be pre-validated by the hypervisor
PreValidate1:
  DD  SNP_SEC_MEM_BASE_DESC_1
  DD  SNP_SEC_MEM_SIZE_DESC_1
  DD  OVMF_SECTION_TYPE_SNP_SEC_MEM
PreValidate2:
  DD  SNP_SEC_MEM_BASE_DESC_2
  DD  SNP_SEC_MEM_SIZE_DESC_2
  DD  OVMF_SECTION_TYPE_SNP_SEC_MEM

; SEV-SNP Secrets page
SevSnpSecrets:
  DD  SEV_SNP_SECRETS_BASE
  DD  SEV_SNP_SECRETS_SIZE
  DD  OVMF_SECTION_TYPE_SNP_SECRETS

; CPUID values
CpuidSec:
  DD  CPUID_BASE
  DD  CPUID_SIZE
  DD  OVMF_SECTION_TYPE_CPUID

; SVSM CAA page
SvsmCaa:
  DD  SVSM_CAA_BASE
  DD  SVSM_CAA_SIZE
  DD  OVMF_SECTION_TYPE_SVSM_CAA

; Region need to be pre-validated by the hypervisor
PreValidate3:
  DD  SNP_SEC_MEM_BASE_DESC_3
  DD  SNP_SEC_MEM_SIZE_DESC_3
  DD  OVMF_SECTION_TYPE_SNP_SEC_MEM

%if (SEV_SNP_KERNEL_HASHES_BASE > 0)
; Kernel hashes for measured direct boot, or zero page if
; there are no kernel hashes / SEV secrets
SevSnpKernelHashes:
  DD  SEV_SNP_KERNEL_HASHES_BASE
  DD  SEV_SNP_KERNEL_HASHES_SIZE
  DD  OVMF_SECTION_TYPE_KERNEL_HASHES
%endif

OvmfSevGuidedStructureEnd:
  ALIGN   16
