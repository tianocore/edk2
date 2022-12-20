;------------------------------------------------------------------------------
; @file
; TDX Common defitions used by the APs in mailbox
;
; Copyright (c) 2021, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
;------------------------------------------------------------------------------

CommandOffset                             equ       00h
ApicidOffset                              equ       04h
WakeupVectorOffset                        equ       08h
OSArgsOffset                              equ       10h
FirmwareArgsOffset                        equ       800h
WakeupArgsRelocatedMailBox                equ       800h
AcceptPageArgsPhysicalStart               equ       800h
AcceptPageArgsPhysicalEnd                 equ       808h
AcceptPageArgsTopStackAddress             equ       810h
AcceptPageArgsApStackSize                 equ       818h
CpuArrivalOffset                          equ       900h
CpusExitingOffset                         equ       0a00h
TalliesOffset                             equ       0a08h
ErrorsOffset                              equ       0e08h

SIZE_4KB                                  equ       1000h
SIZE_2MB                                  equ       200000h
SIZE_1GB                                  equ       40000000h

PAGE_ACCEPT_LEVEL_4K                      equ       0
PAGE_ACCEPT_LEVEL_2M                      equ       1
PAGE_ACCEPT_LEVEL_1G                      equ       2

TDX_PAGE_ALREADY_ACCEPTED                 equ       0x00000b0a
TDX_PAGE_SIZE_MISMATCH                    equ       0xc0000b0b

; Errors of APs in Mailbox
ERROR_NON                                 equ       0
ERROR_INVALID_ACCEPT_PAGE_SIZE            equ       1
ERROR_ACCEPT_PAGE_ERROR                   equ       2
ERROR_INVALID_FALLBACK_PAGE_LEVEL         equ       3

MpProtectedModeWakeupCommandNoop          equ       0
MpProtectedModeWakeupCommandWakeup        equ       1
MpProtectedModeWakeupCommandSleep         equ       2
MpProtectedModeWakeupCommandAcceptPages   equ       3

MailboxApicIdInvalid                      equ       0xffffffff
MailboxApicidBroadcast                    equ       0xfffffffe

%define TDCALL_TDINFO                          0x1
%define TDCALL_TDACCEPTPAGE                    0x6
