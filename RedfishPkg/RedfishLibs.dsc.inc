## @file
# Redfish DSC include file for [LibraryClasses*] section of all Architectures.
#
# This file can be included to the [LibraryClasses*] section(s) of a platform DSC file
# by using "!include RedfishPkg/RedfishLibs.dsc.inc" to specify the library instances
# of EDKII network library classes.
#
# (C) Copyright 2021 Hewlett Packard Enterprise Development LP<BR>
# Copyright (c) 2023-2024, NVIDIA CORPORATION & AFFILIATES. All rights reserved.
#
#    SPDX-License-Identifier: BSD-2-Clause-Patent
#
##
!if $(REDFISH_ENABLE) == TRUE
  RestExLib|RedfishPkg/Library/DxeRestExLib/DxeRestExLib.inf
  Ucs2Utf8Lib|RedfishPkg/Library/BaseUcs2Utf8Lib/BaseUcs2Utf8Lib.inf
  RedfishCrtLib|RedfishPkg/PrivateLibrary/RedfishCrtLib/RedfishCrtLib.inf
  JsonLib|RedfishPkg/Library/JsonLib/JsonLib.inf
  RedfishLib|RedfishPkg/PrivateLibrary/RedfishLib/RedfishLib.inf
  RedfishDebugLib|RedfishPkg/Library/RedfishDebugLib/RedfishDebugLib.inf
  RedfishPlatformCredentialLib|RedfishPkg/Library/RedfishPlatformCredentialIpmiLib/RedfishPlatformCredentialIpmiLib.inf
  HiiUtilityLib|RedfishPkg/Library/HiiUtilityLib/HiiUtilityLib.inf
  RedfishPlatformConfigLib|RedfishPkg/Library/RedfishPlatformConfigLib/RedfishPlatformConfigLib.inf
  RedfishHttpLib|RedfishPkg/Library/RedfishHttpLib/RedfishHttpLib.inf
!endif

