/** @file
  ToolDefinitions Class.

  ToolDefinitions class incldes the common Tool definitions.

Copyright (c) 2006, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

package org.tianocore.common.definitions;

import java.io.File;

/**
  This class includes the common Tool definitions.
 **/
public class ToolDefinitions {
    ///
    /// Line separator (carriage return-line feed, CRLF)
    ///
    public final static String LINE_SEPARATOR = "\r\n";

    ///
    /// Framework Database (FrameworkDatabase.db) file path
    ///
    public final static String FRAMEWORK_DATABASE_FILE_PATH =
        "Tools" + File.separatorChar + "Conf" + File.separatorChar + "FrameworkDatabase.db";

    ///
    /// Target (target.txt) file path
    ///
    public final static String TARGET_FILE_PATH =
        "Tools" + File.separatorChar + "Conf" + File.separatorChar + "target.txt";

    ///
    /// Default Tools Definition (tools_def.txt) file path
    ///
    public final static String DEFAULT_TOOLS_DEF_FILE_PATH =
        "Tools" + File.separatorChar + "Conf" + File.separatorChar + "tools_def.txt";

    ///
    /// Extension names for SPD, FPD, and MSA
    ///
    public final static String SPD_EXTENSION    = ".spd";
    public final static String FPD_EXTENSION    = ".fpd";
    public final static String MSA_EXTENSION    = ".msa";

    ///
    /// Tool Chain Elements in the Tools Definition file
    ///
    public final static String TOOLS_DEF_ELEMENT_TARGET       = "TARGET";
    public final static String TOOLS_DEF_ELEMENT_TOOLCHAIN    = "TOOLCHAIN";
    public final static String TOOLS_DEF_ELEMENT_ARCH         = "ARCH";
    public final static String TOOLS_DEF_ELEMENT_TOOLCODE     = "TOOLCODE";
    public final static String TOOLS_DEF_ELEMENT_ATTRIBUTE    = "ATTRIBUTE";

    ///
    /// Index of Tool Chain elements in the Tools Definition file
    ///
    public final static int TOOLS_DEF_ELEMENT_INDEX_TARGET      = 0;
    public final static int TOOLS_DEF_ELEMENT_INDEX_TOOLCHAIN   = 1;
    public final static int TOOLS_DEF_ELEMENT_INDEX_ARCH        = 2;
    public final static int TOOLS_DEF_ELEMENT_INDEX_TOOLCODE    = 3;
    public final static int TOOLS_DEF_ELEMENT_INDEX_ATTRIBUTE   = 4;
    public final static int TOOLS_DEF_ELEMENT_INDEX_MAXIMUM     = 5;

    ///
    /// Tool Chain Attributes in the Tools Definition file
    ///
    public final static String TOOLS_DEF_ATTRIBUTE_NAME       = "NAME";
    public final static String TOOLS_DEF_ATTRIBUTE_PATH       = "PATH";
    public final static String TOOLS_DEF_ATTRIBUTE_DPATH      = "DPATH";
    public final static String TOOLS_DEF_ATTRIBUTE_SPATH      = "SPATH";
    public final static String TOOLS_DEF_ATTRIBUTE_EXT        = "EXT";
    public final static String TOOLS_DEF_ATTRIBUTE_FAMILY     = "FAMILY";
    public final static String TOOLS_DEF_ATTRIBUTE_FLAGS      = "FLAGS";
    public final static String TOOLS_DEF_ATTRIBUTE_LIBPATH    = "LIBPATH";
    public final static String TOOLS_DEF_ATTRIBUTE_INCLUDEPATH= "INCLUDEPATH";

    ///
    /// Tool Chain Families in the Tools Definition file
    ///
    public final static String TOOLS_DEF_FAMILY_MSFT          = "MSFT";
    public final static String TOOLS_DEF_FAMILY_INTEL         = "INTEL";
    public final static String TOOLS_DEF_FAMILY_GCC           = "GCC";

    ///
    /// Key name in the Target file
    ///
    public final static String TARGET_KEY_ACTIVE_PLATFORM     = "ACTIVE_PLATFORM";
    public final static String TARGET_KEY_TARGET              = "TARGET";
    public final static String TARGET_KEY_TOOLCHAIN           = "TOOL_CHAIN_TAG";
    public final static String TARGET_KEY_ARCH                = "TARGET_ARCH";
    public final static String TARGET_KEY_TOOLS_DEF           = "TOOL_CHAIN_CONF";
    public final static String TARGET_KEY_MULTIPLE_THREAD     = "MULTIPLE_THREAD";
    public final static String TARGET_KEY_MAX_CONCURRENT_THREAD_NUMBER 
        = "MAX_CONCURRENT_THREAD_NUMBER";
}
