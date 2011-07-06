#------------------------------------------------------------------------------
#
# Copyright (c) 2006 - 2009, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
# Module Name:
#
#   IvtAsm.S
#
# Abstract:
#
#   Interrupt Vector Table
#
#------------------------------------------------------------------------------

#
# Interrupt Vector Table
#


ASM_GLOBAL ASM_PFX(AsmIdtVector00)
.p2align 3
ASM_PFX(AsmIdtVector00):
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x00
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x01
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x02
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x03
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x04
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x05
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x06
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x07
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x08
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x09
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x0a
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x0b
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x0c
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x0d
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x0e
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x0f
    nop
    
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x10
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x11
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x12
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x13
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x14
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x15
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x16
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x17
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x18
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x19
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x1a
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x1b
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x1c
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x1d
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x1e
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x1f
    nop
    
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x00
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x21
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x22
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x23
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x24
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x25
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x26
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x27
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x28
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x29
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x2a
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x2b
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x2c
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x2d
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x2e
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x2f
    nop
    
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x30
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x31
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x32
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x33
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x34
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x35
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x36
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x37
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x38
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x39
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x3a
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x3b
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x3c
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x3d
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x3e
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x3f
    nop
    
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x40
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x41
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x42
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x43
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x44
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x45
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x46
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x47
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x48
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x49
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x4a
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x4b
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x4c
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x4d
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x4e
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x4f
    nop
    
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x50
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x51
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x52
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x53
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x54
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x55
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x56
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x57
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x58
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x59
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x5a
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x5b
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x5c
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x5d
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x5e
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x5f
    nop
    
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x60
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x61
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x62
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x63
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x64
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x65
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x66
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x67
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x68
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x69
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x6a
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x6b
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x6c
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x6d
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x6e
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x6f
    nop
    
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x70
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x71
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x72
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x73
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x74
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x75
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x76
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x77
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x78
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x79
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x7a
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x7b
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x7c
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x7d
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x7e
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x7f
    nop
    
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x80
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x81
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x82
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x83
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x84
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x85
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x86
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x87
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x88
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x89
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x8a
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x8b
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x8c
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x8d
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x8e
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x8f
    nop
    
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x90
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x91
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x92
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x93
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x94
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x95
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x96
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x97
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x98
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x99
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x9a
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x9b
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x9c
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x9d
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x9e
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0x9f
    nop
    
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xa0
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xa1
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xa2
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xa3
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xa4
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xa5
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xa6
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xa7
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xa8
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xa9
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xaa
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xab
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xac
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xad
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xae
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xaf
    nop
    
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xb0
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xb1
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xb2
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xb3
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xb4
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xb5
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xb6
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xb7
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xb8
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xb9
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xba
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xbb
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xbc
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xbd
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xbe
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xbf
    nop
    
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xc0
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xc1
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xc2
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xc3
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xc4
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xc5
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xc6
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xc7
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xc8
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xc9
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xca
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xcb
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xcc
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xcd
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xce
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xcf
    nop
    
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xd0
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xd1
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xd2
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xd3
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xd4
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xd5
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xd6
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xd7
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xd8
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xd9
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xda
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xdb
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xdc
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xdd
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xde
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xdf
    nop
    
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xe0
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xe1
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xe2
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xe3
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xe4
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xe5
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xe6
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xe7
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xe8
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xe9
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xea
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xeb
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xec
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xed
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xee
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xef
    nop
    
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xf0
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xf1
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xf2
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xf3
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xf4
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xf5
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xf6
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xf7
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xf8
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xf9
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xfa
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xfb
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xfc
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xfd
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xfe
    nop
    call    ASM_PFX(CommonInterruptEntry)
    .short  0xff
    nop
    
ASM_GLOBAL ASM_PFX(AsmCommonIdtEnd)
ASM_PFX(AsmCommonIdtEnd):
    .byte 0

    
