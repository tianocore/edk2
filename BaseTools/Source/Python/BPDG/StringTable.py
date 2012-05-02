## @file
# This file is used to define strings used in the BPDG tool
#
# Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
##


#string table starts here...

#strings are classified as following types
#    MSG_...: it is a message string
#    ERR_...: it is a error string
#    WRN_...: it is a warning string
#    LBL_...: it is a UI label (window title, control label, etc.)
#    MNU_...: it is a menu item label
#    HLP_...: it is a help string
#    CFG_...: it is a config string used in module. Do not need to translate it.
#    XRC_...: it is a user visible string from xrc file

MAP_FILE_COMMENT_TEMPLATE = \
"""
## @file
#
#  THIS IS AUTO-GENERATED FILE BY BPDG TOOLS AND PLEASE DO NOT MAKE MODIFICATION.
#
#  This file lists all VPD informations for a platform fixed/adjusted by BPDG tool.
# 
# Copyright (c) 2010, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
"""



LBL_BPDG_LONG_UNI           = (u"Intel(r) Binary Product Data Generation Tool (Intel(r) BPDG)")
LBL_BPDG_VERSION            = (u"0.1")
LBL_BPDG_USAGE              = \
(
"""
BPDG options -o Filename.bin -m Filename.map Filename.txt
Intel(r) Binary Product Data Generation Tool (Intel(r) BPDG)
Copyright (c) 2010 Intel Corporation All Rights Reserved.

Required Flags:
  -o BIN_FILENAME, --vpd-filename=BIN_FILENAME
            Specify the file name for the VPD binary file
  -m FILENAME, --map-filename=FILENAME
            Generate file name for consumption during the build that contains 
            the mapping of Pcd name, offset, datum size and value derived 
            from the input file and any automatic calculations.
""" 
)

MSG_OPTION_HELP             = ("Show this help message and exit.")
MSG_OPTION_DEBUG_LEVEL      = ("Print DEBUG statements, where DEBUG_LEVEL is 0-9.")
MSG_OPTION_VERBOSE          = ("Print informational statements.")
MSG_OPTION_QUIET            = ("Returns the exit code and will display only error messages.")
MSG_OPTION_VPD_FILENAME     = ("Specify the file name for the VPD binary file.")
MSG_OPTION_MAP_FILENAME     = ("Generate file name for consumption during the build that contains the mapping of Pcd name, offset, datum size and value derived from the input file and any automatic calculations.")
MSG_OPTION_FORCE            = ("Will force overwriting existing output files rather than returning an error message.")

ERR_INVALID_DEBUG_LEVEL     = ("Invalid level for debug message. Only "
                                "'DEBUG', 'INFO', 'WARNING', 'ERROR', "
                                "'CRITICAL' are supported for debugging "
                                "messages.")
