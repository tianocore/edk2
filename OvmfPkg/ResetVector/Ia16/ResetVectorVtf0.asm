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

;
; SEV-SNP boot support
;
; sevSnpBlock:
;   For the initial boot of SEV-SNP guest, a CPUID and Secrets page must
;   be reserved by the BIOS at a RAM area defined by SNP_CPUID_BASE and
;   SNP_SECRETS_BASE. A hypervisor will locate this information using the
;   SEV-SNP boot block GUID and provide the GPA to the PSP to populate
;   the memory area with the required information..
;
; GUID (SEV-SNP boot block): bd39c0c2-2f8e-4243-83e8-1b74cebcb7d9
;
sevSnpBootBlockStart:
    DD      SNP_SECRETS_BASE
    DD      SNP_SECRETS_SIZE
    DD      SNP_CPUID_BASE
    DD      SNP_CPUID_SIZE
    DW      sevSnpBootBlockEnd - sevSnpBootBlockStart
    DB      0xC2, 0xC0, 0x39, 0xBD, 0x8e, 0x2F, 0x43, 0x42
    DB      0x83, 0xE8, 0x1B, 0x74, 0xCE, 0xBC, 0xB7, 0xD9
sevSnpBootBlockEnd:

;
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

applicationProcessorEntryPoint:
;
; Application Processors entry point
;
; GenFv generates code aligned on a 4k boundary which will jump to this
; location.  (0xffffffe0)  This allows the Local APIC Startup IPI to be
; used to wake up the application processors.
;
    jmp     EarlyApInitReal16

ALIGN   8

    DD      0

;
; The VTF signature
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
    nop
    nop
    jmp     EarlyBspInitReal16

ALIGN   16

fourGigabytes:

