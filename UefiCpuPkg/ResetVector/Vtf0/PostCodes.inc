;------------------------------------------------------------------------------
; @file
; Definitions of POST CODES for the reset vector module
;
; Copyright (c) 2009, Intel Corporation. All rights reserved.<BR>
; This program and the accompanying materials
; are licensed and made available under the terms and conditions of the BSD License
; which accompanies this distribution.  The full text of the license may be found at
; http://opensource.org/licenses/bsd-license.php
;
; THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
; WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
;
;------------------------------------------------------------------------------

%define POSTCODE_16BIT_MODE     0x16
%define POSTCODE_32BIT_MODE     0x32
%define POSTCODE_64BIT_MODE     0x64

%define POSTCODE_BFV_NOT_FOUND  0xb0
%define POSTCODE_BFV_FOUND      0xb1

%define POSTCODE_SEC_NOT_FOUND  0xf0
%define POSTCODE_SEC_FOUND      0xf1

