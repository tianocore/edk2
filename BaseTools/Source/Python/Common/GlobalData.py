## @file
# This file is used to define common static strings used by INF/DEC/DSC files
#
# Copyright (c) 2007 - 2016, Intel Corporation. All rights reserved.<BR>
# This program and the accompanying materials
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
gAllFiles = None
gCommand = None
gSKUID_CMD = None

gGlobalDefines = {}
gPlatformDefines = {}
# PCD name and value pair for fixed at build and feature flag
gPlatformPcds = {}
# PCDs with type that are not fixed at build and feature flag
gPlatformOtherPcds = {}
gActivePlatform = None
gCommandLineDefines = {}
gEdkGlobal = {}
gOverrideDir = {}
gCommandMaxLength = 4096
# for debug trace purpose when problem occurs
gProcessingFile = ''
gBuildingModule = ''
gSkuids = []
gDefaultStores = []

## Regular expression for matching macro used in DSC/DEC/INF file inclusion
gMacroRefPattern = re.compile("\$\(([A-Z][_A-Z0-9]*)\)", re.UNICODE)
gMacroDefPattern = re.compile("^(DEFINE|EDK_GLOBAL)[ \t]+")
gMacroNamePattern = re.compile("^[A-Z][A-Z0-9_]*$")
# C-style wide string pattern
gWideStringPattern = re.compile('(\W|\A)L"')
#
# A global variable for whether current build in AutoGen phase or not.
#
gAutoGenPhase = False

#
# The Conf dir outside the workspace dir
#
gConfDirectory = ''

gBuildDirectory = ''
#
# The relative default database file path
#
gDatabasePath = ".cache/build.db"

#
# Build flag for binary build
#
gIgnoreSource = False

#
# FDF parser
#
gFdfParser = None

#
# If a module is built more than once with different PCDs or library classes
# a temporary INF file with same content is created, the temporary file is removed
# when build exits.
#
gTempInfs = []

BuildOptionPcd = []

#
# Mixed PCD name dict
#
MixedPcd = {}

# Structure Pcd dict
gStructurePcd = {}

# Pcd name for the Pcd which used in the Conditional directives
gConditionalPcds = []

gUseHashCache = None
gBinCacheDest = None
gBinCacheSource = None
gPlatformHash = None
gPackageHash = {}
gModuleHash = {}
gEnableGenfdsMultiThread = False
