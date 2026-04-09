## @file
# RedfishPkg DSC include file for [Defines] section of all Architectures.
#
# This file can be included to the [Defines] section of a platform DSC file by
# using "!include RedfishPkg/RedfishDefines.dsc.inc" to set value of
# flags.
#
# These flags can be defined before the !include line, or changed on the command
# line to enable or disable related feature support.
#   -D FLAG=VALUE
#
# The default value of these flags are:
#   DEFINE REDFISH_ENABLE = TRUE
#
# (C) Copyright 2020 Hewlett Packard Enterprise Development LP<BR>
#
#    SPDX-License-Identifier: BSD-2-Clause-Patent
#
##

!ifndef REDFISH_ENABLE
  #
  # This flag is to enable or disable the whole Redfish support.
  #
  DEFINE REDFISH_ENABLE = TRUE
!endif

