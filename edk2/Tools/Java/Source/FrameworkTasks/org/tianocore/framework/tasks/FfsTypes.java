/** @file
 FfsTypes class.

 FfsType class record the costant value of Ffs File attribute, type, and 
 architecture.
 
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
  FfsType
  
  FfsType class record the costant value of Ffs File attribute, type, and 
  architecture.
  
**/
public interface FfsTypes {
    //
    // Ffs file attributes
    //
    static final int FFS_ATTRIB_TAIL_PRESENT             = 0x01;

    static final int FFS_ATTRIB_RECOVERY                 = 0x02;

    static final int FFS_ATTRIB_HEADER_EXTENSION         = 0x04;

    static final int FFS_ATTRIB_DATA_ALIGNMENT           = 0x38;

    static final int FFS_ATTRIB_CHECKSUM                 = 0x40;

    //
    // Ffs states difinitions
    //
    static final int EFI_FILE_HEADER_CONSTRUCTION        = 0x01;

    static final int EFI_FILE_HEADER_VALID               = 0x02;

    static final int EFI_FILE_DATA_VALID                 = 0x04;

    static final int EFI_FILE_MARKED_FOR_UPDATE          = 0x08;

    static final int EFI_FILE_DELETED                    = 0x10;

    static final int EFI_FILE_HEADER_INVALID             = 0x20;

    //
    // FFS_FIXED_CHECKSUM is the default checksum value used when the
    // FFS_ATTRIB_CHECKSUM attribute bit is clear note this is NOT an
    // architecturally defined value, but is in this file for implementation
    // convenience
    //
    static final int FFS_FIXED_CHECKSUM = 0x5a;

    //
    // Architectural file types
    //
    static final int EFI_FV_FILETYPE_ALL                   = 0x00;

    static final int EFI_FV_FILETYPE_RAW                   = 0x01;

    static final int EFI_FV_FILETYPE_FREEFORM              = 0x02;

    static final int EFI_FV_FILETYPE_SECURITY_CORE         = 0x03;

    static final int EFI_FV_FILETYPE_PEI_CORE              = 0x04;

    static final int EFI_FV_FILETYPE_DXE_CORE              = 0x05;

    static final int EFI_FV_FILETYPE_PEIM                  = 0x06;

    static final int EFI_FV_FILETYPE_DRIVER                = 0x07;

    static final int EFI_FV_FILETYPE_COMBINED_PEIM_DRIVER  = 0x08;

    static final int EFI_FV_FILETYPE_APPLICATION           = 0x09;

    static final int EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE = 0x0B;

    static final int EFI_FV_FILETYPE_FFS_PAD               = 0xF0;

    //
    // Ffs file type
    //
    static final String EFI_FV_FFS_FILETYPE_STR            = ".FFS";
    static final String EFI_FV_DXE_FILETYPE_STR            = ".DXE";
    static final String EFI_FV_PEI_FILETYPE_STR            = ".PEI";
    static final String EFI_FV_APP_FILETYPE_STR            = ".APP";
    static final String EFI_FV_FVI_FILETYPE_STR            = ".FVI";
    static final String EFI_FV_SEC_FILETYPE_STR            = ".SEC";
    
    //
    //  Section Type copy from EfiImageFormat.h
    //
    static final int  EFI_SECTION_COMPRESSION              = 0x01;
    static final int  EFI_SECTION_GUID_DEFINED             = 0x02;
    
    //
    //  CompressionType values, we currently don't support 
    //  "EFI_CUSTOMIZED_COMPRESSION".
    //
    static final int  EFI_NOT_COMPRESSED                   = 0x00;
    static final int  EFI_STANDARD_COMPRESSION             = 0x01;
    static final int  EFI_CUSTOMIZED_COMPRESSION           = 0x02;
    

}