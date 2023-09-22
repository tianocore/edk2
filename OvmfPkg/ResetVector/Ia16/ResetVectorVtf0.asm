;------------------------------------------------------------------------------
; @file
; First code executed by processor after resetting.
; Derived from UefiCpuPkg/ResetVector/Vtf0/Ia16/ResetVectorVtf0.asm
;
; Copyright (c) 2008 - 2014, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
;------------------------------------------------------------------------------

BITS    16

ALIGN   16

;
; Pad the image size to 4k when page tables are in VTF0
;
; If the VTF0 image has page tables built in, then we need to make
; sure the end of VTF0 is 4k above where the page tables end.
;
; This is required so the page tables will be 4k aligned when VTF0 is
; located just below 0x100000000 (4GB) in the firmware device.
;
%ifdef ALIGN_TOP_TO_4K_FOR_PAGING
    TIMES (0x1000 - ($ - EndOfPageTables) - 0x20) DB 0
%endif

;
; Padding to ensure first guid starts at 0xffffffd0
;
TIMES (15 - ((guidedStructureEnd - guidedStructureStart + 15) % 16)) DB 0

; GUIDed structure.  To traverse this you should first verify the
; presence of the table footer guid
; (96b582de-1fb2-45f7-baea-a366c55a082d) at 0xffffffd0.  If that
; is found, the two bytes at 0xffffffce are the entire table length.
;
; The table is composed of structures with the form:
;
; Data (arbitrary bytes identified by guid)
; length from start of data to end of guid (2 bytes)
; guid (16 bytes)
;
; so work back from the footer using the length to traverse until you
; either find the guid you're looking for or run off the beginning of
; the table.
;
guidedStructureStart:

%ifdef ARCH_X64
;
; TDX Metadata offset block
;
; TdxMetadata.asm is included in ARCH_X64 because Inte TDX is only
; available in ARCH_X64. Below block describes the offset of
; TdxMetadata block in Ovmf image
;
; GUID : e47a6535-984a-4798-865e-4685a7bf8ec2
;
tdxMetadataOffsetStart:
    DD      fourGigabytes - TdxMetadataGuid - 16
    DW      tdxMetadataOffsetEnd - tdxMetadataOffsetStart
    DB      0x35, 0x65, 0x7a, 0xe4, 0x4a, 0x98, 0x98, 0x47
    DB      0x86, 0x5e, 0x46, 0x85, 0xa7, 0xbf, 0x8e, 0xc2
tdxMetadataOffsetEnd:

;
; SEV metadata descriptor
;
; Provide the start offset of the metadata blob within the OVMF binary.

; GUID : dc886566-984a-4798-A75e-5585a7bf67cc
;
OvmfSevMetadataOffsetStart:
  DD      (fourGigabytes - OvmfSevMetadataGuid)
  DW      OvmfSevMetadataOffsetEnd - OvmfSevMetadataOffsetStart
  DB      0x66, 0x65, 0x88, 0xdc, 0x4a, 0x98, 0x98, 0x47
  DB      0xA7, 0x5e, 0x55, 0x85, 0xa7, 0xbf, 0x67, 0xcc
OvmfSevMetadataOffsetEnd:

%endif

; SEV Hash Table Block
;
; This describes the guest ram area where the hypervisor should
; install a table describing the hashes of certain firmware configuration
; device files that would otherwise be passed in unchecked.  The current
; use is for the kernel, initrd and command line values, but others may be
; added.  The data format is:
;
; base physical address (32 bit word)
; table length (32 bit word)
;
; GUID (SEV FW config hash block): 7255371f-3a3b-4b04-927b-1da6efa8d454
;
sevFwHashBlockStart:
    DD      SEV_FW_HASH_BLOCK_BASE
    DD      SEV_FW_HASH_BLOCK_SIZE
    DW      sevFwHashBlockEnd - sevFwHashBlockStart
    DB      0x1f, 0x37, 0x55, 0x72, 0x3b, 0x3a, 0x04, 0x4b
    DB      0x92, 0x7b, 0x1d, 0xa6, 0xef, 0xa8, 0xd4, 0x54
sevFwHashBlockEnd:

; SEV Secret block
;
; This describes the guest ram area where the hypervisor should
; inject the secret.  The data format is:
;
; base physical address (32 bit word)
; table length (32 bit word)
;
; GUID (SEV secret block): 4c2eb361-7d9b-4cc3-8081-127c90d3d294
;
sevSecretBlockStart:
    DD      SEV_LAUNCH_SECRET_BASE
    DD      SEV_LAUNCH_SECRET_SIZE
    DW      sevSecretBlockEnd - sevSecretBlockStart
    DB      0x61, 0xB3, 0x2E, 0x4C, 0x9B, 0x7D, 0xC3, 0x4C
    DB      0x80, 0x81, 0x12, 0x7C, 0x90, 0xD3, 0xD2, 0x94
sevSecretBlockEnd:

;
; SEV-ES Processor Reset support
;
; sevEsResetBlock:
;   For the initial boot of an AP under SEV-ES, the "reset" RIP must be
;   programmed to the RAM area defined by SEV_ES_AP_RESET_IP. The data
;   format is:
;
;   IP value [0:15]
;   CS segment base [31:16]
;
;   GUID (SEV-ES reset block): 00f771de-1a7e-4fcb-890e-68c77e2fb44e
;
;   A hypervisor reads the CS segement base and IP value. The CS segment base
;   value represents the high order 16-bits of the CS segment base, so the
;   hypervisor must left shift the value of the CS segement base by 16 bits to
;   form the full CS segment base for the CS segment register. It would then
;   program the EIP register with the IP value as read.
;

sevEsResetBlockStart:
    DD      SEV_ES_AP_RESET_IP
    DW      sevEsResetBlockEnd - sevEsResetBlockStart
    DB      0xDE, 0x71, 0xF7, 0x00, 0x7E, 0x1A, 0xCB, 0x4F
    DB      0x89, 0x0E, 0x68, 0xC7, 0x7E, 0x2F, 0xB4, 0x4E
sevEsResetBlockEnd:

;
; Table footer:
;
; length of whole table (16 bit word)
; GUID (table footer): 96b582de-1fb2-45f7-baea-a366c55a082d
;
    DW      guidedStructureEnd - guidedStructureStart
    DB      0xDE, 0x82, 0xB5, 0x96, 0xB2, 0x1F, 0xF7, 0x45
    DB      0xBA, 0xEA, 0xA3, 0x66, 0xC5, 0x5A, 0x08, 0x2D

guidedStructureEnd:

ALIGN   16

;
; 0xffffffe0
;
    DD      0, 0, 0

;
; The VTF signature (0xffffffec)
;
; VTF-0 means that the VTF (Volume Top File) code does not require
; any fixups.
;
vtfSignature:
    DB      'V', 'T', 'F', 0

ALIGN   16

resetVector:
;
; Reset Vector
;
; This is where the processor will begin execution
;
; In IA32 we follow the standard reset vector flow. While in X64, Td guest
; may be supported. Td guest requires the startup mode to be 32-bit
; protected mode but the legacy VM startup mode is 16-bit real mode.
; To make NASM generate such shared entry code that behaves correctly in
; both 16-bit and 32-bit mode, more BITS directives are added.
;
%ifdef ARCH_IA32
    nop
    nop
    jmp     EarlyBspInitReal16

%else

    mov     eax, cr0
    test    al, 1
    jz      .Real
BITS 32
    jmp     Main32
BITS 16
.Real:
    jmp     EarlyBspInitReal16

%endif

ALIGN   16

fourGigabytes:

