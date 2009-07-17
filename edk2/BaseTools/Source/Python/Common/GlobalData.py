## @file
# This file is used to define common static strings used by INF/DEC/DSC files
#
# Copyright (c) 2007, Intel Corporation
# All rights reserved. This program and the accompanying materials
# are licensed and made available under the terms and conditions of the BSD License
# which accompanies this distribution.  The full text of the license may be found at
# http://opensource.org/licenses/bsd-license.php
#
# THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
# WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

import re

gIsWindows = None

gEdkCompatibilityPkg = "EdkCompatibilityPkg"
gWorkspace = "."
gEdkSource = "EdkCompatibilityPkg"
gEfiSource = "."
gEcpSource = "EdkCompatibilityPkg"

gOptions = None
gCaseInsensitive = False
gGlobalDefines = {}
gAllFiles = None

gEdkGlobal = {}
gOverrideDir = {}

# for debug trace purpose when problem occurs
gProcessingFile = ''
gBuildingModule = ''

## Regular expression for matching macro used in DSC/DEC/INF file inclusion
gMacroPattern = re.compile("\$\(([_A-Z][_A-Z0-9]*)\)", re.UNICODE)

