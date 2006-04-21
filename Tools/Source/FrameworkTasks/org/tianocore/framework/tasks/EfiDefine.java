/** @file
 EfiDefine class.

 EfiDefine class records the UEFI return status value.
 
 Copyright (c) 2006, Intel Corporation
 All rights reserved. This program and the accompanying materials
 are licensed and made available under the terms and conditions of the BSD License
 which accompanies this distribution.  The full text of the license may be found at
 http://opensource.org/licenses/bsd-license.php
 
 THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
 WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

 **/

package org.tianocore.framework.tasks;

/**
  EfiDefine class.

  EfiDefine class records the UEFI return status value.
**/
public interface EfiDefine {
    //
    // EFI define Interface for define constant related to UEFI.
    //  
    static final int EFI_SUCCESS                = 0;
    static final int EFI_LOAD_ERROR             = 0x80000001;
    static final int EFI_INVALID_PARAMETER      = 0x80000002;
    static final int EFI_UNSUPPORTED            = 0x80000003;
    static final int EFI_BAD_BUFFER_SIZE        = 0x80000004;
    static final int EFI_BUFFER_TOO_SMALL       = 0x80000005;
    static final int EFI_NOT_READY              = 0x80000006;
    static final int EFI_DEVICE_ERROR           = 0x80000007;
    static final int EFI_WRITE_PROTECTED        = 0x80000008;
    static final int EFI_OUT_OF_RESOURCES       = 0x80000009;
    static final int EFI_VOLUME_CORRUPTED       = 0x8000000a;
    static final int EFI_VOLUME_FULL            = 0x8000000b;
    static final int EFI_NO_MEDIA               = 0x8000000c;
    static final int EFI_MEDIA_CHANGED          = 0x8000000d;
    static final int EFI_NOT_FOUND              = 0x8000000e;
    static final int EFI_ACCESS_DENIED          = 0x8000000f;
    static final int EFI_NO_RESPONSE            = 0x80000010;
    static final int EFI_NO_MAPPING             = 0x80000011;
    static final int EFI_TIMEOUT                = 0x80000012;
    static final int EFI_NOT_STARTED            = 0x80000013;
    static final int EFI_ALREADY_STARTED        = 0x80000014;
    static final int EFI_ABORTED                = 0x80000015;
    static final int EFI_ICMP_ERROR             = 0x80000016;
    static final int EFI_TFTP_ERROR             = 0x80000017;
    static final int EFI_PROTOCOL_ERROR         = 0x80000018;
    static final int EFI_INCOMPATIBLE_VERSION   = 0x80000019;
    static final int EFI_SECURITY_VIOLATION     = 0x80000020;
    static final int EFI_CRC_ERROR              = 0x80000021;
}
