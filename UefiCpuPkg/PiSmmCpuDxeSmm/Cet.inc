;------------------------------------------------------------------------------
;
; Copyright (c) 2023, Intel Corporation. All rights reserved.<BR>
; SPDX-License-Identifier: BSD-2-Clause-Patent
;
; Abstract:
;
;   This file provides macro definitions for CET feature for NASM files.
;
;------------------------------------------------------------------------------

%define MSR_IA32_U_CET                     0x6A0
%define MSR_IA32_S_CET                     0x6A2
%define MSR_IA32_CET_SH_STK_EN             (1<<0)
%define MSR_IA32_CET_WR_SHSTK_EN           (1<<1)
%define MSR_IA32_CET_ENDBR_EN              (1<<2)
%define MSR_IA32_CET_LEG_IW_EN             (1<<3)
%define MSR_IA32_CET_NO_TRACK_EN           (1<<4)
%define MSR_IA32_CET_SUPPRESS_DIS          (1<<5)
%define MSR_IA32_CET_SUPPRESS              (1<<10)
%define MSR_IA32_CET_TRACKER               (1<<11)
%define MSR_IA32_PL0_SSP                   0x6A4
%define MSR_IA32_INTERRUPT_SSP_TABLE_ADDR  0x6A8

%define CR4_CET_BIT                        23
%define CR4_CET                            (1<<CR4_CET_BIT)
