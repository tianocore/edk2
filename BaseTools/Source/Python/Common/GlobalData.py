## @file
# This file is used to define common static strings used by INF/DEC/DSC files
#
# Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
# SPDX-License-Identifier: BSD-2-Clause-Patent

import re

gIsWindows = None
gWorkspace = "."
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
gCommandMaxLength = 4096
# for debug trace purpose when problem occurs
gProcessingFile = ''
gBuildingModule = ''
gSkuids = []
gDefaultStores = []

# definition for a MACRO name.  used to create regular expressions below.
_MacroNamePattern = "[A-Z][A-Z0-9_]*"

## Regular expression for matching macro used in DSC/DEC/INF file inclusion
gMacroRefPattern = re.compile("\$\(({})\)".format(_MacroNamePattern), re.UNICODE)
gMacroDefPattern = re.compile("^(DEFINE|EDK_GLOBAL)[ \t]+")
gMacroNamePattern = re.compile("^{}$".format(_MacroNamePattern))

# definition for a GUID.  used to create regular expressions below.
_HexChar = r"[0-9a-fA-F]"
_GuidPattern = r"{Hex}{{8}}-{Hex}{{4}}-{Hex}{{4}}-{Hex}{{4}}-{Hex}{{12}}".format(Hex=_HexChar)

## Regular expressions for GUID matching
gGuidPattern = re.compile(r'{}'.format(_GuidPattern))
gGuidPatternEnd = re.compile(r'{}$'.format(_GuidPattern))

## Regular expressions for HEX matching
g4HexChar = re.compile(r'{}{{4}}'.format(_HexChar))
gHexPattern = re.compile(r'0[xX]{}+'.format(_HexChar))
gHexPatternAll = re.compile(r'0[xX]{}+$'.format(_HexChar))

## Regular expressions for string identifier checking
gIdentifierPattern = re.compile('^[a-zA-Z][a-zA-Z0-9_]*$', re.UNICODE)
## Regular expression for GUID c structure format
_GuidCFormatPattern = r"{{\s*0[xX]{Hex}{{1,8}}\s*,\s*0[xX]{Hex}{{1,4}}\s*,\s*0[xX]{Hex}{{1,4}}" \
                      r"\s*,\s*{{\s*0[xX]{Hex}{{1,2}}\s*,\s*0[xX]{Hex}{{1,2}}" \
                      r"\s*,\s*0[xX]{Hex}{{1,2}}\s*,\s*0[xX]{Hex}{{1,2}}" \
                      r"\s*,\s*0[xX]{Hex}{{1,2}}\s*,\s*0[xX]{Hex}{{1,2}}" \
                      r"\s*,\s*0[xX]{Hex}{{1,2}}\s*,\s*0[xX]{Hex}{{1,2}}\s*}}\s*}}".format(Hex=_HexChar)
gGuidCFormatPattern = re.compile(r"{}".format(_GuidCFormatPattern))

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

BuildOptionPcd = []

#
# Mixed PCD name dict
#
MixedPcd = {}

# Structure Pcd dict
gStructurePcd = {}
gPcdSkuOverrides={}
# Pcd name for the Pcd which used in the Conditional directives
gConditionalPcds = []

gUseHashCache = None
gBinCacheDest = None
gBinCacheSource = None
gPlatformHash = None
gPackageHash = {}
gModuleHash = {}
gEnableGenfdsMultiThread = False
gSikpAutoGenCache = set()

# Dictionary for tracking Module build status as success or failure
# False -> Fail : True -> Success
gModuleBuildTracking = dict()

# Dictionary of booleans that dictate whether a module or
# library can be skiped
# Top Dict:     Key: Arch Type              Value: Dictionary
# Second Dict:  Key: Module\Library Name    Value: True\False
gBuildHashSkipTracking = dict()
