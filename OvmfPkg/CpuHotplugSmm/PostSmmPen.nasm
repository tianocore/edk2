;------------------------------------------------------------------------------
; @file
; Pen any hot-added CPU in a 16-bit, real mode HLT loop, after it leaves SMM by
; executing the RSM instruction.
;
; Copyright (c) 2020, Red Hat, Inc.
;
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
; The routine implemented here is stored into normal RAM, under 1MB, at the
; beginning of a page that is allocated as EfiReservedMemoryType. On any
; hot-added CPU, it is executed after *at least* the first RSM (i.e., after
; SMBASE relocation).
;
; The first execution of this code occurs as follows:
;
; - The hot-added CPU is in RESET state.
;
; - The ACPI CPU hotplug event handler triggers a broadcast SMI, from the OS.
;
; - Existent CPUs (BSP and APs) enter SMM.
;
; - The hot-added CPU remains in RESET state, but an SMI is pending for it now.
;   (See "SYSTEM MANAGEMENT INTERRUPT (SMI)" in the Intel SDM.)
;
; - In SMM, pre-existent CPUs that are not elected SMM Monarch, keep themselves
;   busy with their wait loops.
;
; - From the root MMI handler, the SMM Monarch:
;
;   - places this routine in the reserved page,
;
;   - clears the "about to leave SMM" byte in SMRAM,
;
;   - clears the last byte of the reserved page,
;
;   - sends an INIT-SIPI-SIPI sequence to the hot-added CPU,
;
;   - un-gates the default SMI handler by APIC ID.
;
; - The startup vector in the SIPI that is sent by the SMM Monarch points to
;   this code; i.e., to the reserved page. (Example: 0x9_F000.)
;
; - The SMM Monarch starts polling the "about to leave SMM" byte in SMRAM.
;
; - The hot-added CPU boots, and immediately enters SMM due to the pending SMI.
;   It starts executing the default SMI handler.
;
; - Importantly, the SMRAM Save State Map captures the following information,
;   when the hot-added CPU enters SMM:
;
;   - CS selector: assumes the 16 most significant bits of the 20-bit (i.e.,
;     below 1MB) startup vector from the SIPI. (Example: 0x9F00.)
;
;   - CS attributes: Accessed, Readable, User (S=1), CodeSegment (bit#11),
;     Present.
;
;   - CS limit: 0xFFFF.
;
;   - CS base: the CS selector value shifted left by 4 bits. That is, the CS
;     base equals the SIPI startup vector. (Example: 0x9_F000.)
;
;   - IP: the least significant 4 bits from the SIPI startup vector. Because
;     the routine is page-aligned, these bits are zero (hence IP is zero).
;
;   - ES, SS, DS, FS, GS selectors: 0.
;
;   - ES, SS, DS, FS, GS attributes: same as the CS attributes, minus
;     CodeSegment (bit#11).
;
;   - ES, SS, DS, FS, GS limits: 0xFFFF.
;
;   - ES, SS, DS, FS, GS bases: 0.
;
; - The hot-added CPU sets its new SMBASE value in the SMRAM Save State Map.
;
; - The hot-added CPU sets the "about to leave SMM" byte in SMRAM, then
;   executes the RSM instruction immediately after, leaving SMM.
;
; - The SMM Monarch notices that the "about to leave SMM" byte in SMRAM has
;   been set, and starts polling the last byte in the reserved page.
;
; - The hot-added CPU jumps ("returns") to the code below (in the reserved
;   page), according to the register state listed in the SMRAM Save State Map.
;
; - The hot-added CPU sets the last byte of the reserved page, then halts
;   itself.
;
; - The SMM Monarch notices that the hot-added CPU is done with SMBASE
;   relocation.
;
; Note that, if the OS is malicious and sends INIT-SIPI-SIPI to the hot-added
; CPU before allowing the ACPI CPU hotplug event handler to trigger a broadcast
; SMI, then said broadcast SMI will yank the hot-added CPU directly into SMM,
; without becoming pending for it (as the hot-added CPU is no longer in RESET
; state). This is OK, because:
;
; - The default SMI handler copes with this, as it is gated by APIC ID. The
;   hot-added CPU won't start the actual SMBASE relocation until the SMM
;   Monarch lets it.
;
; - The INIT-SIPI-SIPI sequence that the SMM Monarch sends to the hot-added CPU
;   will be ignored in this sate (it won't even be latched). See "SMI HANDLER
;   EXECUTION ENVIRONMENT" in the Intel SDM: "INIT operations are inhibited
;   when the processor enters SMM".
;
; - When the hot-added CPU (e.g., CPU#1) executes the RSM (having relocated
;   SMBASE), it returns to the OS. The OS can use CPU#1 to attack the last byte
;   of the reserved page, while another CPU (e.g., CPU#2) is relocating SMBASE,
;   in order to trick the SMM Monarch (e.g., CPU#0) to open the APIC ID gate
;   for yet another CPU (e.g., CPU#3). However, the SMM Monarch won't look at
;   the last byte of the reserved page, until CPU#2 sets the "about to leave
;   SMM" byte in SMRAM. This leaves a very small window (just one instruction's
;   worth before the RSM) for CPU#3 to "catch up" with CPU#2, and overwrite
;   CPU#2's SMBASE with its own.
;
; In other words, we do not / need not prevent a malicious OS from booting the
; hot-added CPU early; instead we provide benign OSes with a pen for hot-added
; CPUs.
;------------------------------------------------------------------------------

SECTION .data
BITS 16

GLOBAL ASM_PFX (mPostSmmPen)     ; UINT8[]
GLOBAL ASM_PFX (mPostSmmPenSize) ; UINT16

ASM_PFX (mPostSmmPen):
  ;
  ; Point DS at the same reserved page.
  ;
  mov ax, cs
  mov ds, ax

  ;
  ; Inform the SMM Monarch that we're done with SMBASE relocation, by setting
  ; the last byte in the reserved page.
  ;
  mov byte [ds : word 0xFFF], 1

  ;
  ; Halt now, until we get woken by another SMI, or (more likely) the OS
  ; reboots us with another INIT-SIPI-SIPI.
  ;
HltLoop:
  cli
  hlt
  jmp HltLoop

ASM_PFX (mPostSmmPenSize):
  dw $ - ASM_PFX (mPostSmmPen)
