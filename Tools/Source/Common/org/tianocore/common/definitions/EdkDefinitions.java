/** @file
  EdkDefinitions Class.

  EdkDefinitions class incldes the common EDK definitions which are used
  by the Tools.

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

package org.tianocore.common.definitions;

/**
  This class includes the common EDK definitions.
 **/
public class EdkDefinitions {
    ///
    /// MODULE_TYPE definitions
    ///
    public final static String MODULE_TYPE_BASE                 = "BASE";
    public final static String MODULE_TYPE_SEC                  = "SEC";
    public final static String MODULE_TYPE_PEI_CORE             = "PEI_CORE";
    public final static String MODULE_TYPE_PEIM                 = "PEIM";
    public final static String MODULE_TYPE_DXE_CORE             = "DXE_CORE";
    public final static String MODULE_TYPE_DXE_DRIVER           = "DXE_DRIVER";
    public final static String MODULE_TYPE_DXE_RUNTIME_DRIVER   = "DXE_RUNTIME_DRIVER";
    public final static String MODULE_TYPE_DXE_SMM_DRIVER       = "DXE_SMM_DRIVER";
    public final static String MODULE_TYPE_DXE_SAL_DRIVER       = "DXE_SAL_DRIVER";
    public final static String MODULE_TYPE_UEFI_DRIVER          = "UEFI_DRIVER";
    public final static String MODULE_TYPE_UEFI_APPLICATION     = "UEFI_APPLICATION";
    public final static String MODULE_TYPE_USER_DEFINED         = "USER_DEFINED";
    public final static String MODULE_TYPE_TOOL                 = "TOOL";

    ///
    /// Extension definitions for each of module types
    ///
    public final static String ModuleTypeExtensions[][] = {
        { MODULE_TYPE_BASE,                 ".FFS" },
        { MODULE_TYPE_SEC,                  ".SEC" },
        { MODULE_TYPE_PEI_CORE,             ".PEI" },
        { MODULE_TYPE_PEIM,                 ".PEI" },
        { MODULE_TYPE_DXE_CORE,             ".DXE" },
        { MODULE_TYPE_DXE_DRIVER,           ".DXE" },
        { MODULE_TYPE_DXE_RUNTIME_DRIVER,   ".DXE" },
        { MODULE_TYPE_DXE_SMM_DRIVER,       ".DXE" },
        { MODULE_TYPE_DXE_SAL_DRIVER,       ".DXE" },
        { MODULE_TYPE_UEFI_DRIVER,          ".DXE" },
        { MODULE_TYPE_UEFI_APPLICATION,     ".APP" },
        { MODULE_TYPE_USER_DEFINED,         ".FFS" },
        { MODULE_TYPE_TOOL,                 ".FFS" }
    };

    ///
    /// FFS_TYPE definitions
    ///
    public final static int EFI_FV_FILETYPE_ALL                     = 0x00;
    public final static int EFI_FV_FILETYPE_RAW                     = 0x01;
    public final static int EFI_FV_FILETYPE_FREEFORM                = 0x02;
    public final static int EFI_FV_FILETYPE_SECURITY_CORE           = 0x03;
    public final static int EFI_FV_FILETYPE_PEI_CORE                = 0x04;
    public final static int EFI_FV_FILETYPE_DXE_CORE                = 0x05;
    public final static int EFI_FV_FILETYPE_PEIM                    = 0x06;
    public final static int EFI_FV_FILETYPE_DRIVER                  = 0x07;
    public final static int EFI_FV_FILETYPE_COMBINED_PEIM_DRIVER    = 0x08;
    public final static int EFI_FV_FILETYPE_APPLICATION             = 0x09;
    public final static int EFI_FV_FILETYPE_FIRMWARE_VOLUME_IMAGE   = 0x0B;
    public final static int EFI_FV_FILETYPE_FFS_PAD                 = 0xF0;

    ///
    /// SECTION_TYPE definitions
    ///
    public final static String EFI_SECTION_COMPRESSION              = "EFI_SECTION_COMPRESSION";
    public final static String EFI_SECTION_GUID_DEFINED             = "EFI_SECTION_GUID_DEFINED";
    public final static String EFI_SECTION_PE32                     = "EFI_SECTION_PE32";
    public final static String EFI_SECTION_PIC                      = "EFI_SECTION_PIC";
    public final static String EFI_SECTION_TE                       = "EFI_SECTION_TE";
    public final static String EFI_SECTION_DXE_DEPEX                = "EFI_SECTION_DXE_DEPEX";
    public final static String EFI_SECTION_VERSION                  = "EFI_SECTION_VERSION";
    public final static String EFI_SECTION_USER_INTERFACE           = "EFI_SECTION_USER_INTERFACE";
    public final static String EFI_SECTION_COMPATIBILITY16          = "EFI_SECTION_COMPATIBILITY16";
    public final static String EFI_SECTION_FIRMWARE_VOLUME_IMAGE    = "EFI_SECTION_FIRMWARE_VOLUME_IMAGE";
    public final static String EFI_SECTION_FREEFORM_SUBTYPE_GUID    = "EFI_SECTION_FREEFORM_SUBTYPE_GUID";
    public final static String EFI_SECTION_RAW                      = "EFI_SECTION_RAW";
    public final static String EFI_SECTION_PEI_DEPEX                = "EFI_SECTION_PEI_DEPEX";

    ///
    /// Extension definitions for each of section types
    ///
    public final static String SectionTypeExtensions[][] = {
        { EFI_SECTION_COMPRESSION,              ".sec"  },
        { EFI_SECTION_GUID_DEFINED,             ".sec"  },
        { EFI_SECTION_PE32,                     ".pe32" },
        { EFI_SECTION_PIC,                      ".pic"  },
        { EFI_SECTION_TE,                       ".tes"  },
        { EFI_SECTION_DXE_DEPEX,                ".dpx"  },
        { EFI_SECTION_VERSION,                  ".ver"  },
        { EFI_SECTION_USER_INTERFACE,           ".ui"   },
        { EFI_SECTION_COMPATIBILITY16,          ".sec"  },
        { EFI_SECTION_FIRMWARE_VOLUME_IMAGE,    ".sec"  },
        { EFI_SECTION_FREEFORM_SUBTYPE_GUID,    ".sec"  },
        { EFI_SECTION_RAW,                      ".sec"  },
        { EFI_SECTION_PEI_DEPEX,                ".dpx"  }
    };
}
