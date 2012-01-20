## @file
# This file is used to define common static strings and global data used by UPT
#
# Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>
#
# This program and the accompanying materials are licensed and made available 
# under the terms and conditions of the BSD License which accompanies this 
# distribution. The full text of the license may be found at 
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

'''
GlobalData
'''

#
# The workspace directory 
#
gWORKSPACE = '.'

#
# INF module directory
#
gINF_MODULE_DIR = "."
gINF_MODULE_NAME = ''

#
# the directory to holds upt related files
#
gUPT_DIR = r"Conf/upt/"

#
# Log file for invalid meta-data files during force removing
#
gINVALID_MODULE_FILE = gUPT_DIR + r"Invalid_Modules.log"

#
# File name for content zip file in the distribution
#
gCONTENT_FILE = "dist.content"

#
# File name for XML file in the distibution
#
gDESC_FILE = 'dist.pkg'

#
# Case Insensitive flag
#
gCASE_INSENSITIVE = ''

#
# All Files dictionary
#
gALL_FILES = {}

#
# Database instance
#
gDB = None

#
# list for files that are found in module level but not in INF files,
# items are (File, ModulePath), all these should be relative to $(WORKSPACE)
#
gMISS_FILE_IN_MODLIST = []

#
# Global Current Line
#
gINF_CURRENT_LINE = None

#
# Global pkg list
#
gWSPKG_LIST = []

#
# Flag used to take WARN as ERROR.
# By default, only ERROR message will break the tools execution.
#
gWARNING_AS_ERROR = False

#
# Used to specify the temp directory to hold the unpacked distribution files
#
gUNPACK_DIR = None

#
# Flag used to mark whether the INF file is Binary INF or not.
#
gIS_BINARY_INF = False
