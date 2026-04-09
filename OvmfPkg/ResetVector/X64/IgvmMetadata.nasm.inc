;-----------------------------------------------------------------------------
; @file
; OVMF metadata for IGVM parameters
;
; Copyright (c) 2021 - 2024, AMD Inc. All rights reserved.<BR>
;
; SPDX-License-Identifier: BSD-2-Clause-Patent
;-----------------------------------------------------------------------------

BITS  64
ALIGN 16

%define IGVM_ID_PARAM_AREA                               0x100
%define IGVM_ID_PARAM_MEMORY_MAP                         0x101
%define IGVM_ID_PARAM_VP_COUNT                           0x102

%define IGVM_ID_HOB_AREA                                 0x200

; keep in sync with OvmfPkg/Library/PlatformInitLib/Igvm.c
%define MEMORY_MAP_OFFSET                                    0
%define MEMORY_MAP_ENTRIES                                   8
%define MEMORY_MAP_SIZE              (MEMORY_MAP_ENTRIES * 24)

%define VP_COUNT_OFFSET  (MEMORY_MAP_OFFSET + MEMORY_MAP_SIZE)
%define VP_COUNT_SIZE                                       16

IgvmParamStart:
_IgvmDescriptor:
  DB 'I','G','V','M'                            ; Signature
  DD IgvmParamEnd - IgvmParamStart              ; Length
  DD 1                                          ; Version
  DD (IgvmParamEnd - IgvmParamStart - 16) / 12  ; Number of sections

%if (IGVM_PARAM_SIZE > 0)

_IgvmParamArea:
  DD  IGVM_PARAM_START
  DD  IGVM_PARAM_SIZE
  DD  IGVM_ID_PARAM_AREA

_IgvmMemoryMap:
  DD  MEMORY_MAP_OFFSET
  DD  MEMORY_MAP_SIZE
  DD  IGVM_ID_PARAM_MEMORY_MAP

_IgvmVpCount:
  DD  VP_COUNT_OFFSET
  DD  VP_COUNT_SIZE
  DD  IGVM_ID_PARAM_VP_COUNT

%endif

%if (IGVM_HOB_SIZE > 0)

_IgvmHobArea:
  DD  IGVM_HOB_START
  DD  IGVM_HOB_SIZE
  DD  IGVM_ID_HOB_AREA

%endif

IgvmParamEnd:
